// Host-Test des Multi-Brand-Parsers/TX-Pfads (AmpCommand.h)
#define private public
#define protected public
#include "AmpCommand.h"
#undef private
#undef protected

#include <iostream>
#include <iomanip>

static int g_fail = 0, g_pass = 0;

static void check(bool ok, const std::string &what, const std::string &detail = "") {
  if (ok) { g_pass++; }
  else { g_fail++; std::cout << "  FAIL: " << what << (detail.empty() ? "" : "  [" + detail + "]") << "\n"; }
}

// Modell per Name aus der Tabelle holen
static const AmpModelDef* modelByName(const char *name) {
  for (uint8_t i = 0; i < AMP_MODEL_COUNT; i++)
    if (strcmp(AMP_MODELS[i].name, name) == 0) return &AMP_MODELS[i];
  return nullptr;
}

// Queue leerlaufen lassen (Zeit vorspulen, Antworten sind optional)
static void drain(ampCallback &amp, unsigned steps = 60) {
  for (unsigned i = 0; i < steps; i++) {
    g_millis += 100;
    amp.ampLoop();
  }
}

static std::string joinTx(ampCallback &amp) {
  std::string s;
  for (auto &f : amp._serial.txFrames) { s += f; s += "|"; }
  return s;
}

static std::string escape(const std::string &s) {
  std::string o;
  for (char c : s) {
    if (c == '\r') o += "<CR>";
    else if (c == '\n') o += "<LF>";
    else o += c;
  }
  return o;
}

// ---------------------------------------------------------------- Rotel Gen2
static void testRotelGen2() {
  std::cout << "== Rotel Gen2 (A12/A14) ==\n";
  ampCallback amp;
  const AmpModelDef *m = modelByName("A12 / A14 (+MKII)");
  check(m != nullptr, "Modell A12/A14 gefunden");
  amp.setModel(m);
  amp.ampInitSerial();
  check(amp._serial.baud == 115200, "Baud 115200", std::to_string(amp._serial.baud));

  amp._serial.feed("power=on$volume=42$source=coax2$mute=off$bypass=on$bass=+05$treble=-03$balance=R07$");
  amp.ampLoop();

  check(amp._currentPower == true, "power=on");
  check(amp._currentVolume == 42, "volume=42", std::to_string(amp._currentVolume));
  check(amp._currentSource == 3, "source=coax2 -> Index 3", std::to_string(amp._currentSource));
  check(amp._currentMute == false, "mute=off");
  check(amp._currentBypass == true, "bypass=on");
  check(amp._currentBass == 5, "bass=+05", std::to_string(amp._currentBass));
  check(amp._currentTreble == -3, "treble=-03", std::to_string(amp._currentTreble));
  check(amp._currentBalance == 7, "balance=R07", std::to_string(amp._currentBalance));

  // TX: kein CR, '!' steckt im Befehl
  drain(amp, 40);
  amp._serial.txFrames.clear(); amp._serial.txRaw.clear();
  amp.setPower(true);
  drain(amp, 5);
  check(amp._serial.txRaw == "power_on!", "TX power_on! ohne Terminator-Zusatz", escape(amp._serial.txRaw));

  drain(amp, 40);
  amp._serial.txRaw.clear();
  amp.setVolumeNN(55);
  drain(amp, 5);
  check(amp._serial.txRaw == "vol_55!", "TX vol_55!", escape(amp._serial.txRaw));
}

// ---------------------------------------------------------------- Rotel Gen1
static void testRotelGen1() {
  std::cout << "== Rotel Gen1 (RA-11/RA-12) ==\n";
  ampCallback amp;
  amp.setModel(modelByName("RA-11 / RA-12"));
  amp.ampInitSerial();

  amp._serial.feed("power=on!volume=33!");
  amp.ampLoop();
  check(amp._currentPower == true, "Gen1 power=on!");
  check(amp._currentVolume == 33, "Gen1 volume=33!", std::to_string(amp._currentVolume));

  // Byte-Count-Format: product_version={len},{text}  (kein Terminator)
  amp._serial.feed("product_version=5,V1.23");
  amp.ampLoop();
  check(std::string(amp._currentFirmwareSystem) == "V1.23", "Gen1 Byte-Count product_version",
        amp._currentFirmwareSystem);

  drain(amp, 40);
  amp._serial.txRaw.clear();
  amp.setVolumeStep(true);
  drain(amp, 5);
  check(amp._serial.txRaw == "volume_up!", "Gen1 TX volume_up!", escape(amp._serial.txRaw));
}

// ------------------------------------------------------------------ NAD v2
static void testNad() {
  std::cout << "== NAD v2 (T 787) ==\n";
  ampCallback amp;
  const AmpModelDef *m = modelByName("T 787");
  check(m != nullptr, "Modell T 787 gefunden");
  amp.setModel(m);
  amp.ampInitSerial();
  check(amp._serial.baud == 115200, "NAD Baud 115200", std::to_string(amp._serial.baud));
  check(amp.isRotel() == false, "NAD ist kein Rotel-Dialekt");

  // Antworten laut Spec 2.03: Main.<Var>=<Val><CR>
  amp._serial.feed("Main.Power=On\rMain.Volume=-40\rMain.Mute=Off\rMain.Source=3\r"
                   "Main.ToneDefeat=Off\rMain.Bass=-4\rMain.Treble=6\r");
  amp.ampLoop();
  check(amp._currentPower == true, "Main.Power=On");
  check(amp._currentVolume == -40, "Main.Volume=-40 (dB)", std::to_string(amp._currentVolume));
  check(amp._currentMute == false, "Main.Mute=Off");
  check(amp._currentSource == 3, "Main.Source=3", std::to_string(amp._currentSource));
  check(amp._currentBypass == false, "ToneDefeat=Off -> Klang EIN");
  check(amp._currentBass == -4, "Main.Bass=-4", std::to_string(amp._currentBass));
  check(amp._currentTreble == 6, "Main.Treble=6", std::to_string(amp._currentTreble));

  // CRLF-Variante darf keinen Schaden anrichten
  amp._serial.feed("Main.Power=Off\r\n");
  amp.ampLoop();
  check(amp._currentPower == false, "Main.Power=Off mit CRLF");

  // Zonen/unbekannte Keys muessen ignoriert werden
  int volBefore = amp._currentVolume;
  amp._serial.feed("Zone2.Volume=-10\rMain.Speaker.Front.Config=Small\rMain.Sleep=30\r");
  amp.ampLoop();
  check(amp._currentVolume == volBefore, "Zone2.Volume aendert Main-Volume nicht",
        std::to_string(amp._currentVolume));

  // TX: CR-Preamble + CR-Terminator
  drain(amp, 40);
  amp._serial.txRaw.clear();
  amp.setPower(true);
  drain(amp, 5);
  check(amp._serial.txRaw == "\rMain.Power=On\r", "TX <CR>Main.Power=On<CR>", escape(amp._serial.txRaw));

  drain(amp, 40);
  amp._serial.txRaw.clear();
  amp.setVolumeNN(-35);
  drain(amp, 5);
  check(amp._serial.txRaw == "\rMain.Volume=-35\r", "TX Absolut-Volume", escape(amp._serial.txRaw));

  drain(amp, 40);
  amp._serial.txRaw.clear();
  amp.setVolumeStep(true);
  drain(amp, 5);
  check(amp._serial.txRaw == "\rMain.Volume+\r", "TX Volume-Step", escape(amp._serial.txRaw));

  drain(amp, 40);
  amp._serial.txRaw.clear();
  amp.setSource(5);
  drain(amp, 5);
  check(amp._serial.txRaw == "\rMain.Source=5\r", "TX Source", escape(amp._serial.txRaw));

  // Volume-Clamping an Modellgrenzen (-99..19)
  drain(amp, 40);
  amp._serial.txRaw.clear();
  amp.setVolumeNN(999);
  drain(amp, 5);
  check(amp._serial.txRaw == "\rMain.Volume=19\r", "Volume-Clamp oben", escape(amp._serial.txRaw));
  drain(amp, 40);
  amp._serial.txRaw.clear();
  amp.setVolumeNN(-999);
  drain(amp, 5);
  check(amp._serial.txRaw == "\rMain.Volume=-99\r", "Volume-Clamp unten", escape(amp._serial.txRaw));

  // Init-Burst darf keine Rotel-Befehle enthalten
  drain(amp, 60);
  amp._serial.txFrames.clear(); amp._serial.txRaw.clear();
  amp.requestInitialState();
  drain(amp, 40);
  std::string tx = joinTx(amp);
  check(tx.find('!') == std::string::npos, "Init-Burst ohne Rotel-'!'", tx);
  check(tx.find("Main.Power?") != std::string::npos, "Init-Burst enthaelt Main.Power?", tx);
  check(tx.find("Main.Source?") != std::string::npos, "Init-Burst enthaelt Main.Source?", tx);
}

// --------------------------------------------------------------- NAD C 328
static void testNadC328() {
  std::cout << "== NAD C 328 (Stereo) ==\n";
  ampCallback amp;
  const AmpModelDef *m = modelByName("C 328");
  check(m != nullptr, "Modell C 328 gefunden");
  amp.setModel(m);
  amp.ampInitSerial();
  amp._serial.feed("Main.Power=On\rMain.Volume=-25\r");
  amp.ampLoop();
  check(amp._currentPower && amp._currentVolume == -25, "C 328 Grundparsing",
        std::to_string(amp._currentVolume));
  check(m->balanceMax == 0, "C 328 ohne Balance");
}

// ------------------------------------------------------------------- Denon
static void testDenon() {
  std::cout << "== Denon / Marantz modern ==\n";
  ampCallback amp;
  const AmpModelDef *m = modelByName("AVR (Denon RS232, to verify)");
  check(m != nullptr, "Denon-Modell gefunden");
  amp.setModel(m);
  amp.ampInitSerial();
  check(amp._serial.baud == 9600, "Denon Baud 9600", std::to_string(amp._serial.baud));

  amp._serial.feed("PWON\rMV45\rMUOFF\rSICD\r");
  amp.ampLoop();
  check(amp._currentPower == true, "PWON");
  check(amp._currentVolume == 45, "MV45", std::to_string(amp._currentVolume));
  check(amp._currentMute == false, "MUOFF");
  check(amp._currentSource == 1, "SICD -> Index 1", std::to_string(amp._currentSource));

  // MVMAX darf Volume NICHT ueberschreiben
  amp._serial.feed("MVMAX 85\r");
  amp.ampLoop();
  check(amp._currentVolume == 45, "MVMAX ueberschreibt Volume nicht", std::to_string(amp._currentVolume));

  // Halbe Schritte
  amp._serial.feed("MV455\r");
  amp.ampLoop();
  check(amp._currentVolume == 45, "MV455 -> 45", std::to_string(amp._currentVolume));

  // Marantz-modern @-Zeilen muessen ignoriert werden
  amp._serial.feed("@PWR:2\r");
  amp.ampLoop();
  check(amp._currentPower == true, "@-Zeile ignoriert (kein Absturz/Fehlparsing)");

  amp._serial.feed("PWSTANDBY\r");
  amp.ampLoop();
  check(amp._currentPower == false, "PWSTANDBY");

  // TX-Format
  drain(amp, 40);
  amp._serial.txRaw.clear();
  amp.setSource(3);
  drain(amp, 5);
  check(amp._serial.txRaw == "SIDVD\r", "TX SIDVD<CR>", escape(amp._serial.txRaw));

  // Power-On-Delay: nach PWON darf 1 s lang nichts folgen
  ampCallback amp2;
  amp2.setModel(m);
  amp2.ampInitSerial();
  amp2.setPower(true);
  amp2.setMute(true);
  g_millis += 200; amp2.ampLoop();            // PWON raus
  check(amp2._serial.txRaw == "PWON\r", "PWON zuerst", escape(amp2._serial.txRaw));
  for (int i = 0; i < 8; i++) { g_millis += 100; amp2.ampLoop(); }   // +800 ms
  check(amp2._serial.txRaw == "PWON\r", "innerhalb 1 s kein Folgebefehl", escape(amp2._serial.txRaw));
  for (int i = 0; i < 6; i++) { g_millis += 100; amp2.ampLoop(); }   // >1 s
  check(amp2._serial.txRaw == "PWON\rMUON\r", "nach 1 s folgt MUON", escape(amp2._serial.txRaw));
}

// ---------------------------------------------------------- Marantz classic
static void testMarantzClassic() {
  std::cout << "== Marantz classic ==\n";
  ampCallback amp;
  const AmpModelDef *m = modelByName("SR classic @CMD (to verify)");
  check(m != nullptr, "Marantz-classic-Modell gefunden");
  amp.setModel(m);
  amp.ampInitSerial();
  check(amp._serial.baud == 9600, "Baud 9600", std::to_string(amp._serial.baud));

  amp._serial.feed("@PWR:2\r@VOL:35\r@AMT:2\r@SRC:02\r");
  amp.ampLoop();
  check(amp._currentPower == true, "@PWR:2 -> on");
  check(amp._currentVolume == 35, "@VOL:35", std::to_string(amp._currentVolume));
  check(amp._currentMute == false, "@AMT:2 -> mute off");
  check(amp._currentSource == 3, "@SRC:02 -> Index 3", std::to_string(amp._currentSource));

  amp._serial.feed("@PWR:1\r");
  amp.ampLoop();
  check(amp._currentPower == false, "@PWR:1 -> standby");

  drain(amp, 40);
  amp._serial.txRaw.clear();
  amp.setPower(true);
  drain(amp, 5);
  check(amp._serial.txRaw == "@PWR:2\r", "TX @PWR:2<CR>", escape(amp._serial.txRaw));
}

// ------------------------------------------------ Tabellen-/Konsistenztests
static void testTable() {
  std::cout << "== Modelltabelle ==\n";
  check(AMP_MODEL_COUNT == 28, "28 Modelle", std::to_string(AMP_MODEL_COUNT));

  // Rotel-Indizes 0..8 muessen fuer NVS-Kompatibilitaet stabil bleiben
  const char *rotelOrder[9] = {
    "A11 (+MKII)", "A12 / A14 (+MKII)", "RA-11 / RA-12", "RA-1570",
    "RA-1572 (+MKII)", "RA-1592 (+MKII)", "RA-6000", "Michi X3 / X5",
    "Michi M8 / S5 (Endstufe)"
  };
  for (int i = 0; i < 9; i++)
    check(strcmp(AMP_MODELS[i].name, rotelOrder[i]) == 0,
          "NVS-Index " + std::to_string(i) + " stabil", AMP_MODELS[i].name);

  for (uint8_t i = 0; i < AMP_MODEL_COUNT; i++) {
    const AmpModelDef &m = AMP_MODELS[i];
    std::string id = std::string(m.brand) + " " + m.name;
    check(m.volumeMin < m.volumeMax, id + ": volumeMin < volumeMax");
    check(m.sourceCount <= AMP_MAX_SOURCES, id + ": sourceCount <= MAX");
    check(m.sourceCount == 0 || m.sources != nullptr, id + ": sources-Pointer gesetzt");
    // Jeder Befehl/replyToken muss in TX_CMD_MAX passen
    for (uint8_t s = 0; s < m.sourceCount; s++) {
      check(strlen(m.sources[s].command) < TX_CMD_MAX,
            id + ": Quellbefehl passt in TX_CMD_MAX", m.sources[s].command);
    }
    // Nicht-Rotel muss ein CommandSet haben
    if (!ampIsRotel(m.protocol))
      check(ampCommandSet(m.protocol) != nullptr, id + ": CommandSet vorhanden");
    // C 338 darf nicht auftauchen
    check(strstr(m.name, "338") == nullptr, "C 338 nicht in der Liste");
  }

  // Alle Befehle der CommandSets muessen in TX_CMD_MAX passen
  const CommandSet *sets[] = { &CMDSET_NAD_V2, &CMDSET_DENON, &CMDSET_MARANTZ_CLASSIC };
  const char *setNames[] = { "NAD", "Denon", "Marantz" };
  for (int i = 0; i < 3; i++) {
    const char *const *p = reinterpret_cast<const char *const *>(sets[i]);
    const size_t n = sizeof(CommandSet) / sizeof(const char *);
    for (size_t k = 0; k < n; k++)
      if (p[k]) check(strlen(p[k]) < TX_CMD_MAX,
                      std::string(setNames[i]) + ": Befehl passt in TX_CMD_MAX", p[k]);
  }
}

// -------------------------------------------------- Queue-/Robustheitstests
static void testRobustness() {
  std::cout << "== Robustheit ==\n";
  ampCallback amp;
  amp.setModel(modelByName("T 787"));
  amp.ampInitSerial();

  // Queue-Overflow darf nicht abstuerzen und nichts ueberschreiben
  for (int i = 0; i < 100; i++) amp.setVolumeStep(true);
  check(amp._txCount <= TX_QUEUE_LEN, "TX-Queue laeuft nicht ueber",
        std::to_string(amp._txCount));

  // Ueberlanger Muell-Frame darf den Parser nicht sprengen
  ampCallback amp2;
  amp2.setModel(modelByName("T 787"));
  amp2.ampInitSerial();
  std::string junk(500, 'X');
  junk += "\rMain.Power=On\r";
  amp2._serial.feed(junk.c_str());
  amp2.ampLoop();
  check(amp2._currentPower == true, "Parser erholt sich nach Muell-Frame");

  // millis()-Rollover: Queue muss weiterlaufen
  ampCallback amp3;
  amp3.setModel(modelByName("A12 / A14 (+MKII)"));
  amp3.ampInitSerial();
  g_millis = 0xFFFFFF00;
  amp3.setPower(true);
  for (int i = 0; i < 10; i++) { g_millis += 100; amp3.ampLoop(); }
  check(amp3._serial.txRaw == "power_on!", "TX ueber millis()-Rollover", escape(amp3._serial.txRaw));
  g_millis = 100000;

  // Endstufe: keine Quellen/Volume-Queries
  ampCallback amp4;
  amp4.setModel(modelByName("Michi M8 / S5 (Endstufe)"));
  amp4.ampInitSerial();
  amp4._serial.txFrames.clear();
  amp4.requestStateQueries();
  check(amp4._txCount == 0, "Endstufe stellt keine Queries", std::to_string(amp4._txCount));
}

// ------------------------------------------------------ Bass/Treble-Raster
static void testToneStep() {
  std::cout << "== Tone-Raster ==\n";
  ampCallback rotel;
  rotel.setModel(modelByName("A12 / A14 (+MKII)"));
  rotel.ampInitSerial();
  check(rotel.toneStep() == 1, "Rotel toneStep 1", std::to_string(rotel.toneStep()));
  drain(rotel, 40); rotel._serial.txRaw.clear();
  rotel.setBassValue(3);
  drain(rotel, 5);
  check(rotel._serial.txRaw == "bass_+03!", "Rotel erlaubt ungerade Werte", escape(rotel._serial.txRaw));

  ampCallback nad;
  nad.setModel(modelByName("T 787"));
  nad.ampInitSerial();
  check(nad.toneStep() == 2, "NAD toneStep 2", std::to_string(nad.toneStep()));

  struct { int in; const char *out; } cases[] = {
    { 3,  "\rMain.Bass=4\r"   },   // 3 -> 4 (gerundet)
    { 1,  "\rMain.Bass=2\r"   },
    { 0,  "\rMain.Bass=0\r"   },
    { -3, "\rMain.Bass=-4\r"  },
    { 99, "\rMain.Bass=10\r"  },   // Clamp
    { -99,"\rMain.Bass=-10\r" },
  };
  for (auto &c : cases) {
    drain(nad, 40); nad._serial.txRaw.clear();
    nad.setBassValue(c.in);
    drain(nad, 5);
    check(nad._serial.txRaw == c.out,
          "NAD Bass " + std::to_string(c.in) + " -> gerade", escape(nad._serial.txRaw));
    // Ergebnis muss immer im Raster liegen
    int v = atoi(nad._serial.txRaw.c_str() + strlen("\rMain.Bass="));
    check(v % 2 == 0 && v >= -10 && v <= 10, "NAD Bass-Wert gerade und im Bereich",
          std::to_string(v));
  }

  drain(nad, 40); nad._serial.txRaw.clear();
  nad.setTrebleValue(-5);
  drain(nad, 5);
  check(nad._serial.txRaw == "\rMain.Treble=-6\r", "NAD Treble -5 -> -6", escape(nad._serial.txRaw));
}

int main() {
  g_millis = 100000;
  testTable();
  testRotelGen2();
  testRotelGen1();
  testNad();
  testNadC328();
  testDenon();
  testMarantzClassic();
  testToneStep();
  testRobustness();

  std::cout << "\n=== " << g_pass << " OK, " << g_fail << " FEHLER ===\n";
  return g_fail ? 1 : 0;
}
