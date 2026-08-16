/*********************************************************************************
 *  HOMESPAN

 *  MIT License
 *  
 *  Copyright (c) 2021-2022 Gregg E. Berman
 *  
 *  https://github.com/HomeSpan/HomeSpan
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *  
 ********************************************************************************/



#include "HomeSpan.h"
#include "AmpCommand.h"


ampCallback _amp;

// Explizite Prototypen: Arduinos Auto-Generierung scheitert an Funktionen
// mit eigenem Parametertyp (AmpUpdateEvent) und zerschiesst dann die Klassen
void WifiDone(int count);
void HomeKitUpdate(AmpUpdateEvent event);


// Pairing Code: Standard 466-37-726, aenderbar ueber das Web-Dashboard (Port 80)

const char * wymrfirmware = "1.3.0";

// Aktives Verstärkermodell; wird in setup() aus dem NVS geladen (Auswahl
// über das Web-Dashboard, Default A12/A14). Quellen/Namen kommen aus der
// Modelltabelle in AmpModels.h – Index+1 = HomeKit-Identifier.
const AmpModelDef *ampModel = &AMP_MODELS[AMP_DEFAULT_MODEL];

// WLAN-Portal, Dashboard und GitHub-OTA (braucht _amp, wymrfirmware, ampModel)
#include "WebPortal.h"

struct InfoService : Service::AccessoryInformation {

  InfoService() : Service::AccessoryInformation(){
    new Characteristic::Identify();
    // Ohne Name-Characteristic schlaegt die Home-App beim Hinzufuegen den
    // generischen Kategorienamen "TV" vor; Markenname aus der Modelltabelle
    new Characteristic::Name(ampModel->brand);
    new Characteristic::Manufacturer(ampModel->brand);
    new Characteristic::Model(ampModel->name);
    new Characteristic::FirmwareRevision(wymrfirmware);
  }
};

struct Speaker : Service::TelevisionSpeaker {
  SpanCharacteristic *volume;
  SpanCharacteristic *mute;

  // Hinweis: Mute/Active stehen nicht in HomeSpans REQ/OPT-Liste und erzeugen
  // beim Boot je eine Log-Warnung – funktionieren aber (siehe HomeSpan #869).
  // Seit iOS 18 bedient der Mute-Button des Remote-Widgets diese Characteristic.
  Speaker() : Service::TelevisionSpeaker(){
    new Characteristic::Active(1);
    new Characteristic::VolumeControlType(3);
    volume = new Characteristic::VolumeSelector();
    mute   = new Characteristic::Mute(false);
  }

  boolean update() override {
    if (volume->updated()) {
      // HAP VolumeSelector: 0 = lauter, 1 = leiser
      LOG1("Volume-Step: %s\n", volume->getNewVal() ? "down" : "up");
      _amp.setVolumeStep(volume->getNewVal() ? 0 : 1);
    }
    if (mute->updated()) {
      LOG1("Mute: %d\n", mute->getNewVal());
      _amp.setMute(mute->getNewVal());
    }
    return (true);
  }
};

// Zeiger auf die Speaker-Instanz für den Mute-Rück-Sync in HomeKitUpdate()
Speaker *ampSpeaker = nullptr;


// Eingangsquelle mit persistentem Namen und ein-/ausblendbarer Sichtbarkeit.
// nvsStore=true legt die Werte im NVS ab (Schlüssel = feste aid/iid, daher
// Reihenfolge der Service-Erzeugung in setup() nicht ändern!). Einmalige
// Allokation beim Boot, kein Heap-Verkehr zur Laufzeit.
struct AmpInput : Service::InputSource {

  SpanCharacteristic *targetVis;
  SpanCharacteristic *currentVis;

  AmpInput(uint8_t id, const char *name) : Service::InputSource() {
    new Characteristic::ConfiguredName(name, true);       // Umbenennung in der Home-App überlebt Reboot
    new Characteristic::Identifier(id);
    new Characteristic::IsConfigured(1);
    targetVis  = new Characteristic::TargetVisibilityState(0, true);   // Checkbox im TV-Einstellungsmenü, NVS-persistent
    currentVis = new Characteristic::CurrentVisibilityState(targetVis->getVal());  // Startwert = gespeicherte Checkbox
  }

  boolean update() override {
    if (targetVis->updated()) {
      currentVis->setVal(targetVis->getNewVal());   // Checkbox -> Sichtbarkeit in der Quellenliste
    }
    return true;
  }
};


struct HomeSpanTV : Service::Television {

  SpanCharacteristic *active = new Characteristic::Active();                  // Verstärker An/Aus (Start: Aus)
  SpanCharacteristic *activeID = new Characteristic::ActiveIdentifier(1);     // Startwert 1 ("CD"), bis der echte Wert vom Verstaerker kommt
  SpanCharacteristic *remoteKey = new Characteristic::RemoteKey();            // Tasten des Remote-Widgets

  // Auswahl für die Pfeiltasten L/R; folgt dem Bypass-Zustand (siehe HomeKitUpdate):
  // Bypass aktiv (Klangregelung aus) -> nur Balance; Bypass aus -> Start mit Bass
  // (kein "enum : uint8_t" – die Basistyp-Syntax verwirrt Arduinos Präprozessor)
  enum ToneSelect { TONE_BASS = 0, TONE_TREBLE, TONE_BALANCE };
  uint8_t toneSelect = TONE_BALANCE;

  // Zeigt den selektierten Parameter im Verstärker-Display an (Blink-Trick)
  void showToneSelection(){
    switch (toneSelect) {
      case TONE_BASS:   _amp.blinkBass();    break;
      case TONE_TREBLE: _amp.blinkTreble();  break;
      default:          _amp.blinkBalance(); break;
    }
  }

  HomeSpanTV(const char *name) : Service::Television() {
    new Characteristic::ConfiguredName(name, true);   // TV-Name persistent (NVS)

    // DisplayOrder: feste Reihenfolge der Quellen in der Home-App
    // (TLV8: Tag 1 = Identifier, Tag 0 = Trenner). Das TLV-Objekt ist lokal,
    // HomeSpan kopiert den Wert einmalig – keine Laufzeit-Allokation.
    // Endstufen haben keine Quellen -> Characteristic ganz weglassen.
    if (ampModel->sourceCount > 0) {
      TLV8 orderTLV;
      for (uint8_t i = 0; i < ampModel->sourceCount; i++) {
        if (i > 0) orderTLV.add(0);
        orderTLV.add(1, i + 1);
      }
      new Characteristic::DisplayOrder(orderTLV);
    }
  }

  boolean update() override {



    if (active->updated()) {
      LOG1("TV Power: %d\n", active->getNewVal());
      _amp.setPower(active->getNewVal());
    }

    if (activeID->updated()) {
      LOG1("Input Source: %d\n", activeID->getNewVal());
      _amp.setSource(activeID->getNewVal());
    }

    if (remoteKey->updated()) {
      LOG1("Remote-Taste: %d (tone=%d)\n", remoteKey->getNewVal(), toneSelect);
      // Endstufen (M8/S5) haben weder Volume noch Klangregelung -> Tasten ignorieren
      const bool hasVolume = !_amp.isPowerAmp();
      const bool hasTone   = ampModel->hasTone;
      switch (remoteKey->getNewVal()) {
        case 4:   // UP: lauter
          if (hasVolume) _amp.setVolumeStep(1);
          break;
        case 5:   // DOWN: leiser
          if (hasVolume) _amp.setVolumeStep(0);
          break;
        case 6:   // LEFT: selektierten Klangparameter verringern
          if (!hasTone) break;
          switch (toneSelect) {
            case TONE_BASS:   _amp.setBassStep(false);   break;
            case TONE_TREBLE: _amp.setTrebleStep(false); break;
            default:          _amp.setBalance(0);        break;
          }
          break;
        case 7:   // RIGHT: selektierten Klangparameter erhöhen
          if (!hasTone) break;
          switch (toneSelect) {
            case TONE_BASS:   _amp.setBassStep(true);   break;
            case TONE_TREBLE: _amp.setTrebleStep(true); break;
            default:          _amp.setBalance(1);       break;
          }
          break;
        case 8:   // SELECT: Bypass an -> Klangmodus aktivieren (bypass_off);
                  //         Bypass aus -> Bass/Treble/Balance durchschalten
          if (!hasTone) break;
          if (_amp._currentBypass) {
            _amp.toggleBypass();   // Klangregelung aktivieren; Auswahl springt per Event auf Bass
          } else {
            // Modelle ohne Balance (z. B. NAD) nur zwischen Bass/Treble wechseln,
            // sonst landet die Auswahl auf einem toten Parameter
            const uint8_t steps = (ampModel->balanceMax > 0) ? 3 : 2;
            toneSelect = (uint8_t)((toneSelect + 1) % steps);
            showToneSelection();
          }
          break;
        case 9:   // BACK: Klangmodus verlassen -> Bypass wieder aktivieren
          if (!hasTone) break;
          if (!_amp._currentBypass) {
            _amp.toggleBypass();   // Klangregelung deaktivieren; Auswahl springt per Event auf Balance
          }
          break;
        case 11:  // PLAY/PAUSE: frei
        case 15:  // INFO: frei
        default:
          break;
      }
    }

    return (true);
  }

  // Kein loop() mehr: Zustandssync passiert eventgetrieben in HomeKitUpdate()
};

// Zeiger auf die TV-Instanz, damit HomeKitUpdate() setVal() aufrufen kann
HomeSpanTV *ampTV = nullptr;

///////////////////////////////

void setup() {

  Serial.begin(115200);

  homeSpan.setControlPin(9);
  // Kein deleteStoredValues() mehr: würde die NVS-gespeicherten Werte
  // (Quellen-Namen, Sichtbarkeit) bei jedem Boot löschen

  // Modellauswahl aus dem NVS (Dashboard-Karte "Verstärker"); bestimmt
  // Quellenliste, Protokollgeneration und Gerätetyp (Vollverstärker/Endstufe)
  ampModel = &AMP_MODELS[ampLoadModelIndex()];
  LOG0("Amp-Modell: %s / %s\n", ampModel->brand, ampModel->name);

  _amp.ampCallbackUpdate(HomeKitUpdate);
  _amp.setModel(ampModel);   // setzt Dialekt bzw. startet Auto-Erkennung
  _amp.ampInitSerial();


  // WLAN besitzt WiFiManagerLite (siehe WebPortal.h) – daher kein
  // enableAutoStartAP() mehr; HomeSpan erkennt die Verbindung ueber Events.
  // HAP zieht auf Port 8080 um: Port 80 gehoert dem Portal/Dashboard
  // (Captive-Portal-Erkennung funktioniert nur auf Port 80). HomeKit ist
  // der Port egal, er wird per mDNS bekanntgegeben.
  homeSpan.setPortNum(8080);

  // mDNS-Hostname statt "HomeSpan-<MAC>": Standard "Amplify" ->
  // http://amplify.local; ueber die Dashboard-Karte "Geraete-Adresse"
  // aenderbar (NVS, wpLoadHostName). Leerer Suffix = keine MAC.
  wpLoadHostName();
  homeSpan.setHostNameSuffix("");
  homeSpan.begin(Category::Television, "Homekit Amplify", wpHostName);
  
  
  homeSpan.setConnectionCallback(WifiDone);
  

  new SpanAccessory(); 
    new InfoService();

  // InputSources aus der Modelltabelle erzeugen; Identifier (Index+1) bleibt
  // automatisch deckungsgleich mit sources[] in AmpModels.h.
  // Endstufen (POWER_AMP) haben weder Quellen noch Lautsprecher-Service:
  // der TV-Service liefert dann nur den Ein/Aus-Schalter.
  SpanService *inputs[AMP_MAX_SOURCES];
  for (uint8_t i = 0; i < ampModel->sourceCount; i++) {
    inputs[i] = new AmpInput(i + 1, ampModel->sources[i].name);
  }

  if (ampModel->deviceType == AMP_FULL_AMP) {
    ampSpeaker = new Speaker();
  }

  // Neutraler HomeKit-Name (Modellnamen enthalten '/' und '(' – von der
  // Home-App nicht erlaubt); Umbenennung durch den Nutzer bleibt NVS-persistent
  ampTV = new HomeSpanTV("Rotel");   // Television Service, InputSources müssen verlinkt sein
  for (uint8_t i = 0; i < ampModel->sourceCount; i++) {
    ampTV->addLink(inputs[i]);
  }
  if (ampSpeaker) {
    ampTV->addLink(ampSpeaker);
  }

  // WLAN-Manager, Captive Portal und Dashboard starten (nach homeSpan.begin(),
  // damit HomeSpans Netzwerk-Event-Queue die Verbindungs-Events mitbekommt)
  webPortalSetup();

}


// Wird bei jeder (Re-)Verbindung aufgerufen (count = Anzahl der Verbindungen).
// Aktiviert rs232_update_on! (Push-Modus) und fragt den Grundzustand ab;
// die TX-Queue sendet antwortgesteuert (min. 100 ms Abstand). Auch nach einem
// WLAN-Ausfall sinnvoll: synchronisiert den HomeKit-Zustand neu.
void WifiDone(int count){
  _amp.requestInitialState();
}


// Eventgetriebener Zustandssync: setVal() nur bei tatsächlicher Änderung
// (vermeidet unnötige HomeKit-Notifications). Läuft im loop()-Task, daher
// unkritisch gegenüber homeSpan.poll().
void HomeKitUpdate(AmpUpdateEvent event) {

  if (ampTV == nullptr) return;   // Events vor Ende von setup() ignorieren

  switch (event) {
    case HOMEKIT_UPDATE_POWER:
      if (ampTV->active->getVal() != (int)_amp._currentPower) {
        ampTV->active->setVal(_amp._currentPower);
      }
      break;
    case HOMEKIT_UPDATE_SOURCE:
      if (ampTV->activeID->getVal() != _amp._currentSource) {
        ampTV->activeID->setVal(_amp._currentSource);
      }
      break;
    case HOMEKIT_UPDATE_MUTE:
      if (ampSpeaker && ampSpeaker->mute->getVal() != (int)_amp._currentMute) {
        ampSpeaker->mute->setVal(_amp._currentMute);
      }
      break;
    case HOMEKIT_UPDATE_BYPASS:
      // Auswahl folgt dem Gerätezustand (greift auch bei Änderung am Gerät):
      // Bypass an -> nur Balance; Bypass aus -> automatisch Bass.
      // Ohne Balance (NAD) bleibt die Auswahl auf Bass.
      ampTV->toneSelect = (_amp._currentBypass && ampModel->balanceMax > 0)
                              ? HomeSpanTV::TONE_BALANCE
                              : HomeSpanTV::TONE_BASS;
      break;
    default:
      break;
  }

}


///////////////////////////////

void loop() {
  _amp.ampLoop();   // RS232 empfangen + TX-Queue abarbeiten

  homeSpan.poll();

  webPortalLoop();      // WLAN-Manager, Portal, Update-/Reset-Auftraege
}
