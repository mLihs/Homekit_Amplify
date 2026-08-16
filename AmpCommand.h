/*
 * AmpCommand.h – Multi-Brand RS232 (Rotel / NAD / Denon / Marantz)
 *
 * Rotel Gen1/Gen2: unveraenderte Dialekt-Logik (pick, Byte-Count, AUTO-Detect).
 * NAD v2: Main.key=val + CR, Prefix-Strip, CR-Preamble (Docs im Ordner NAD).
 * Denon / Marantz modern: positionale PW/MV/MU/SI-Frames (TO VERIFY).
 * Marantz classic: @CMD:val (TO VERIFY).
 *
 * RX/TX nur mit festen char-Puffern – keine Heap-Allokation im Hotpath.
 */

#ifndef AMP_COMMAND_H
#define AMP_COMMAND_H

#include "Arduino.h"
#include <HardwareSerial.h>
#include "AmpModels.h"

constexpr uint8_t RS232_RX_PIN = 4;
constexpr uint8_t RS232_TX_PIN = 10;

// Parser-Limits: NAD "ToneDefeat" (10) nach Strip; Denon-Zeilen bis ~16
constexpr uint8_t CMD_BUF_LEN = 24;
constexpr uint8_t VAL_BUF_LEN = 32;
constexpr uint8_t LINE_BUF_LEN = 48;   // Denon/Marantz Zeilenpuffer

constexpr uint8_t  TX_QUEUE_LEN        = 16;
constexpr uint8_t  TX_CMD_MAX          = 28;   // "Main.ToneDefeat=Off" + Reserve
constexpr uint32_t MIN_CMD_INTERVAL_MS = 100;
constexpr uint32_t RESPONSE_TIMEOUT_MS = 300;

constexpr uint32_t DETECT_TIMEOUT_MS = 1000;
constexpr uint32_t DETECT_BACKOFF_MS = 15000;

constexpr int TONE_MAX = 10;

#define SENDEND        '!'
#define RECEIVEEND     '$'
#define RECEIVEDEVIDER '='

#define ROTEL_POWER_ON     "power_on!"
#define ROTEL_POWER_OFF    "power_off!"
#define ROTEL_POWER_TOGGLE "power_toggle!"
#define ROTEL_MUTE_ON      "mute_on!"
#define ROTEL_MUTE_OFF     "mute_off!"
#define ROTEL_MUTE_TOGGLE  "mute!"
#define ROTEL_BASS_UP      "bass_up!"
#define ROTEL_BASS_DOWN    "bass_down!"
#define ROTEL_BASS_ZERO    "bass_000!"
#define ROTEL_TREBLE_UP    "treble_up!"
#define ROTEL_TREBLE_DOWN  "treble_down!"
#define ROTEL_TREBLE_ZERO  "treble_000!"
#define ROTEL_BALANCE_ZERO "balance_000!"
#define ROTEL_DIMMER_TOGGLE "dimmer!"

#define ROTEL_POWER_RECEIVE   "power"
#define ROTEL_VOLUME_RECEIVE  "volume"
#define ROTEL_MUTE_RECEIVE    "mute"
#define ROTEL_SOURCE_RECEIVE  "source"
#define ROTEL_BALANCE_RECEIVE "balance"
#define ROTEL_BYPASS_RECEIVE  "bypass"
#define ROTEL_TONE_RECEIVE    "tone"
#define ROTEL_BASS_RECEIVE    "bass"
#define ROTEL_TREBLE_RECEIVE  "treble"
#define ROTEL_FIRMWARE_SYSTEM_RECEIVE "version"
#define ROTEL_FIRMWARE_GEN1_RECEIVE   "product_version"
#define ROTEL_FIRMWARE_USB_RECEIVE    "pc_version"

// NAD-Keys nach "Main."-Strip (Gross/Kleinschreibung wie Spec)
#define NAD_POWER_KEY   "Power"
#define NAD_VOLUME_KEY  "Volume"
#define NAD_MUTE_KEY    "Mute"
#define NAD_SOURCE_KEY  "Source"
#define NAD_BASS_KEY    "Bass"
#define NAD_TREBLE_KEY  "Treble"
#define NAD_TONE_KEY    "ToneDefeat"
#define NAD_VERSION_KEY "Version"
#define NAD_MODEL_KEY   "Model"

struct RotelCmdPair {
  const char *gen2;
  const char *gen1;
};

static const RotelCmdPair CMDP_POWER_QUERY   = { "power?",           "get_current_power!"   };
static const RotelCmdPair CMDP_VOLUME_UP     = { "vol_up!",          "volume_up!"           };
static const RotelCmdPair CMDP_VOLUME_DOWN   = { "vol_dwn!",         "volume_down!"         };
static const RotelCmdPair CMDP_VOLUME_QUERY  = { "volume?",          "get_volume!"          };
static const RotelCmdPair CMDP_MUTE_QUERY    = { "mute?",            "get_mute_status!"     };
static const RotelCmdPair CMDP_SOURCE_QUERY  = { "source?",          "get_current_source!"  };
static const RotelCmdPair CMDP_TONE_ENABLE   = { "bypass_off!",      "tone_on!"  };
static const RotelCmdPair CMDP_TONE_DISABLE  = { "bypass_on!",       "tone_off!" };
static const RotelCmdPair CMDP_TONE_QUERY    = { "bypass?",          "get_tone!"            };
static const RotelCmdPair CMDP_BASS_QUERY    = { "bass?",            "get_bass!"            };
static const RotelCmdPair CMDP_TREBLE_QUERY  = { "treble?",          "get_treble!"          };
static const RotelCmdPair CMDP_BALANCE_L     = { "balance_l!",       "balance_left!"        };
static const RotelCmdPair CMDP_BALANCE_R     = { "balance_r!",       "balance_right!"       };
static const RotelCmdPair CMDP_BALANCE_QUERY = { "balance?",         "get_balance!"         };
static const RotelCmdPair CMDP_UPDATE_AUTO   = { "rs232_update_on!", "display_update_auto!" };
static const RotelCmdPair CMDP_VERSION_QUERY = { "version?",         "get_product_version!" };

#define ROTEL_FIRMWARE_USB_STATE "pc_version?"

enum AmpUpdateEvent {
  HOMEKIT_UPDATE_NONE = 0,
  HOMEKIT_UPDATE_ALL,
  HOMEKIT_UPDATE_VOLUME,
  HOMEKIT_UPDATE_MUTE,
  HOMEKIT_UPDATE_POWER,
  HOMEKIT_UPDATE_SOURCE,
  HOMEKIT_UPDATE_DIMMER,
  HOMEKIT_UPDATE_BYPASS,
  HOMEKIT_UPDATE_BALANCE,
  HOMEKIT_UPDATE_BASS,
  HOMEKIT_UPDATE_TREBLE,
  HOMEKIT_UPDATE_FIRMWARE_USB,
  HOMEKIT_UPDATE_FIRMWARE_SYSTEM,
};

typedef void (*callbackUpdateFunction)(AmpUpdateEvent);


class ampCallback {
public:

    bool _currentPower = false;
    int _currentVolume = 0;
    int _currentBalance = 0;
    int _currentBass = 0;
    int _currentTreble = 0;
    bool _currentMute = false;
    int  _currentSource = 0;
    bool _currentBypass = false;
    char _currentFirmwareSystem[VAL_BUF_LEN] = "n.a.";
    char _currentFirmwareUSB[VAL_BUF_LEN] = "n.a.";

    void ampCallbackUpdate(callbackUpdateFunction newFunction) {
      _updateCallback = newFunction;
    }

    void setModel(const AmpModelDef *m){
      _model = m;
      _cmds = ampCommandSet(m->protocol);

      if (!ampIsRotel(m->protocol)) {
        _gen1Active = false;
        _detectState = DETECT_OFF;
        return;
      }

      if (m->generation == ROTEL_GEN1) {
        _gen1Active = true;
        _detectState = DETECT_OFF;
      } else if (m->generation == ROTEL_GEN2) {
        _gen1Active = false;
        _detectState = DETECT_OFF;
      } else {
        const uint8_t cached = rotelLoadGenCache();
        if (cached != ROTEL_GEN_UNKNOWN) {
          _gen1Active = (cached == ROTEL_GEN1);
          _detectState = DETECT_DONE;
          LOG1("ROTEL: Generation aus NVS-Cache: Gen %d\n", _gen1Active ? 1 : 2);
        } else {
          _gen1Active = false;
          startProbe(DETECT_PROBE_G2);
        }
      }
    }

    const AmpModelDef* model() const { return _model; }
    bool isGen1() const { return _gen1Active; }
    bool isPowerAmp() const { return _model->deviceType == AMP_POWER_AMP; }
    bool isRotel() const { return ampIsRotel(_model->protocol); }
    // Schrittweite Bass/Treble (Rotel 1, NAD 2) – auch fuer das Web-UI
    uint8_t toneStep() const {
      const uint8_t s = proto()->toneStep;
      return s ? s : 1;
    }

    const char* generationLabel() const {
      if (!isRotel()) {
        switch (_model->protocol) {
          case PROTO_NAD_V2:          return _model->verified ? "NAD v2" : "NAD v2 (to verify)";
          case PROTO_DENON:           return "Denon (to verify)";
          case PROTO_MARANTZ_CLASSIC: return "Marantz classic (to verify)";
          default:                    return "unknown";
        }
      }
      if (_detectState == DETECT_PROBE_G2 || _detectState == DETECT_PROBE_G1 ||
          _detectState == DETECT_BACKOFF) {
        return "detecting...";
      }
      return _gen1Active ? "Gen 1 (legacy)" : "Gen 2";
    }

    void ampInitSerial(){
      _serial.setRxBufferSize(1024);
      const uint32_t baud = proto()->baud;
      _serial.begin(baud, SERIAL_8N1, RS232_RX_PIN, RS232_TX_PIN);
      LOG1("RS232 UART1 gestartet (RX=4, TX=10, baud=%lu)\n", (unsigned long)baud);
    }

    void ampLoop(){
      ampReceive();
      processDetect();
      processTx();
    }

    void requestInitialState(){
      if (_detectState == DETECT_PROBE_G2 || _detectState == DETECT_PROBE_G1 ||
          _detectState == DETECT_BACKOFF) {
        _initPending = true;
        return;
      }
      if (isRotel()) {
        queueCommand(pick(CMDP_UPDATE_AUTO));
        queueCommand(pick(CMDP_POWER_QUERY));
      } else if (_cmds) {
        if (_cmds->subscribe) queueCommand(_cmds->subscribe);
        if (_cmds->powerQuery) queueCommand(_cmds->powerQuery);
      }
      requestStateQueries();
    }

    void requestStateQueries(){
      if (isPowerAmp()) return;
      if (isRotel()) {
        queueCommand(pick(CMDP_VOLUME_QUERY));
        queueCommand(pick(CMDP_SOURCE_QUERY));
        queueCommand(pick(CMDP_MUTE_QUERY));
        if (_model->hasTone) {
          queueCommand(pick(CMDP_TONE_QUERY));
          queueCommand(pick(CMDP_BASS_QUERY));
          queueCommand(pick(CMDP_TREBLE_QUERY));
        }
        queueCommand(pick(CMDP_BALANCE_QUERY));
        return;
      }
      if (!_cmds) return;
      if (_cmds->volQuery)    queueCommand(_cmds->volQuery);
      if (_cmds->sourceQuery) queueCommand(_cmds->sourceQuery);
      if (_cmds->muteQuery)   queueCommand(_cmds->muteQuery);
      if (_model->hasTone) {
        if (_cmds->toneQuery)   queueCommand(_cmds->toneQuery);
        if (_cmds->bassQuery)   queueCommand(_cmds->bassQuery);
        if (_cmds->trebleQuery) queueCommand(_cmds->trebleQuery);
      }
      if (_model->balanceMax > 0 && _cmds->balanceQuery)
        queueCommand(_cmds->balanceQuery);
    }

    // ---- RX ----
    void ampReceive(){
      while (_serial.available() > 0) {
        const char c = (char)_serial.read();
        const ProtocolDef *p = proto();

        // Denon/Marantz modern: zeilenorientiert bis CR/LF
        if (p->style == FS_POSITIONAL) {
          if (c == p->rxTerm || (p->rxTermAlt && c == p->rxTermAlt)) {
            finishPositionalLine();
          } else if (c >= ' ') {
            if (_lineLen < LINE_BUF_LEN - 1) _lineBuf[_lineLen++] = c;
            else _discardFrame = true;
          }
          continue;
        }

        // Gen-1-Byte-Count (nur Rotel Gen1)
        if (_byteCountRemaining > 0) {
          if (_valLen < VAL_BUF_LEN - 1) _valBuf[_valLen++] = c;
          _byteCountRemaining--;
          if (_byteCountRemaining == 0) finishFrame();
          continue;
        }

        const char term = frameTerminator();
        const char termAlt = p->rxTermAlt;
        if (c == term || (termAlt && c == termAlt)) {
          finishFrame();
        } else if (c == p->divider && p->divider != 0) {
          _readingValue = true;
          _valDigitsOnly = true;
        } else if (c >= ' ') {
          if (!_readingValue) {
            if (c == ' ') continue;
            if (_cmdLen < CMD_BUF_LEN - 1) {
              _cmdBuf[_cmdLen++] = c;
            } else {
              _discardFrame = true;
            }
          } else if (_gen1Active && isRotel() && c == ',' && _valDigitsOnly &&
                     _valLen > 0 && _valLen <= 3) {
            _valBuf[_valLen] = '\0';
            _byteCountRemaining = (int16_t)atoi(_valBuf);
            _valLen = 0;
            if (_byteCountRemaining <= 0) finishFrame();
          } else {
            if (c < '0' || c > '9') _valDigitsOnly = false;
            if (_valLen < VAL_BUF_LEN - 1) {
              _valBuf[_valLen++] = c;
            } else {
              _discardFrame = true;
            }
          }
        }
      }
    }


    void setPower(bool pwr){
      if (isRotel()) {
        queueCommand(pwr ? ROTEL_POWER_ON : ROTEL_POWER_OFF);
      } else if (_cmds) {
        queueCommand(pwr ? _cmds->powerOn : _cmds->powerOff);
      }
    }

    void getPower(){
      if (isRotel()) queueCommand(pick(CMDP_POWER_QUERY));
      else if (_cmds && _cmds->powerQuery) queueCommand(_cmds->powerQuery);
    }

    void updatePower(bool pwr){
      const bool wasOn = _currentPower;
      _currentPower = pwr;
      notifyEvent(HOMEKIT_UPDATE_POWER);
      if (pwr && !wasOn) requestStateQueries();
    }

    void PowerToggle(){
      if (isRotel()) queueCommand(ROTEL_POWER_TOGGLE);
      else if (_cmds && _cmds->powerQuery) {
        // kein generisches Toggle: Zustand invertieren
        setPower(!_currentPower);
      }
    }


    void setVolumeNN(int vol){
      if (vol < _model->volumeMin) vol = _model->volumeMin;
      if (vol > _model->volumeMax) vol = _model->volumeMax;
      char cmd[TX_CMD_MAX];
      if (isRotel()) {
        if (vol < 0) vol = 0;
        if (vol > 96) vol = 96;
        if (vol == 0) strlcpy(cmd, _gen1Active ? "volume_min!" : "vol_min!", sizeof(cmd));
        else          snprintf(cmd, sizeof(cmd), _gen1Active ? "volume_%02d!" : "vol_%02d!", vol);
        queueCommand(cmd);
        return;
      }
      if (_model->protocol == PROTO_NAD_V2) {
        snprintf(cmd, sizeof(cmd), "Main.Volume=%d", vol);
        queueCommand(cmd);
      } else if (_model->protocol == PROTO_DENON) {
        // TO VERIFY: MV45 / MV455 (halbe Schritte)
        snprintf(cmd, sizeof(cmd), "MV%02d", vol);
        queueCommand(cmd);
      }
      // Marantz classic: kein sicheres Absolutformat -> nur Steps
    }

    void setVolumeStep(bool up){
      if (isRotel()) {
        queueCommand(pick(up ? CMDP_VOLUME_UP : CMDP_VOLUME_DOWN));
        return;
      }
      if (_cmds) {
        const char *c = up ? _cmds->volUp : _cmds->volDown;
        if (c) queueCommand(c);
      }
    }

    void getVolume(){
      if (isRotel()) queueCommand(pick(CMDP_VOLUME_QUERY));
      else if (_cmds && _cmds->volQuery) queueCommand(_cmds->volQuery);
    }

    void updateVolume(int vol){
      _currentVolume = vol;
      notifyEvent(HOMEKIT_UPDATE_VOLUME);
    }


    void toggleBypass() {
      if (!_model->hasTone) return;
      if (isRotel()) {
        queueCommand(pick(_currentBypass ? CMDP_TONE_ENABLE : CMDP_TONE_DISABLE));
        return;
      }
      if (_cmds) {
        const char *c = _currentBypass ? _cmds->toneEnable : _cmds->toneDisable;
        if (c) queueCommand(c);
      }
    }

    void updateBypass(bool bp){
      _currentBypass = bp;
      notifyEvent(HOMEKIT_UPDATE_BYPASS);
    }

    void getBypass(){
      if (isRotel()) queueCommand(pick(CMDP_TONE_QUERY));
      else if (_cmds && _cmds->toneQuery) queueCommand(_cmds->toneQuery);
    }


    void setMute(bool mute){
      if (isRotel()) {
        queueCommand(mute ? ROTEL_MUTE_ON : ROTEL_MUTE_OFF);
      } else if (_cmds) {
        queueCommand(mute ? _cmds->muteOn : _cmds->muteOff);
      }
    }

    void toggleMute(){
      if (isRotel()) queueCommand(ROTEL_MUTE_TOGGLE);
      else setMute(!_currentMute);
    }

    void getMute(){
      if (isRotel()) queueCommand(pick(CMDP_MUTE_QUERY));
      else if (_cmds && _cmds->muteQuery) queueCommand(_cmds->muteQuery);
    }

    void updateMute(bool mute){
      _currentMute = mute;
      notifyEvent(HOMEKIT_UPDATE_MUTE);
    }


    void setBalance(bool right){
      if (_model->balanceMax <= 0) return;
      if (isRotel()) queueCommand(pick(right ? CMDP_BALANCE_R : CMDP_BALANCE_L));
      else if (_cmds) {
        const char *c = right ? _cmds->balanceR : _cmds->balanceL;
        if (c) queueCommand(c);
      }
    }

    void zeroBalance(){
      if (_model->balanceMax <= 0) return;
      if (isRotel()) queueCommand(ROTEL_BALANCE_ZERO);
    }

    void getBalance(){
      if (_model->balanceMax <= 0) return;
      if (isRotel()) queueCommand(pick(CMDP_BALANCE_QUERY));
      else if (_cmds && _cmds->balanceQuery) queueCommand(_cmds->balanceQuery);
    }

    void setBalanceValue(int balance){
      if (_model->balanceMax <= 0) return;
      if (!isRotel()) return;
      const int maxBal = _model->balanceMax;
      if (balance >  maxBal) balance =  maxBal;
      if (balance < -maxBal) balance = -maxBal;
      _currentBalance = balance;
      char cmd[TX_CMD_MAX];
      if (balance > 0) {
        snprintf(cmd, sizeof(cmd), _gen1Active ? "balance_R%02d!" : "balance_r%02d!", balance);
      } else if (balance < 0) {
        snprintf(cmd, sizeof(cmd), _gen1Active ? "balance_L%02d!" : "balance_l%02d!", -balance);
      } else {
        strlcpy(cmd, ROTEL_BALANCE_ZERO, sizeof(cmd));
      }
      queueCommand(cmd);
    }

    void updateBalance(int balance){
      _currentBalance = balance;
      notifyEvent(HOMEKIT_UPDATE_BALANCE);
    }


    void setBassStep(bool up){
      if (!_model->hasTone) return;
      if (isRotel()) queueCommand(up ? ROTEL_BASS_UP : ROTEL_BASS_DOWN);
      else if (_cmds) {
        const char *c = up ? _cmds->bassUp : _cmds->bassDown;
        if (c) queueCommand(c);
      }
    }

    void zeroBass(){
      if (!_model->hasTone) return;
      if (isRotel()) queueCommand(ROTEL_BASS_ZERO);
      else if (_model->protocol == PROTO_NAD_V2) queueCommand("Main.Bass=0");
    }

    void setBassValue(int bass){
      if (!_model->hasTone) return;
      bass = snapToneValue(bass);
      char cmd[TX_CMD_MAX];
      if (isRotel()) {
        if (bass == 0) strlcpy(cmd, ROTEL_BASS_ZERO, sizeof(cmd));
        else           snprintf(cmd, sizeof(cmd), "bass_%+03d!", bass);
        queueCommand(cmd);
      } else if (_model->protocol == PROTO_NAD_V2) {
        snprintf(cmd, sizeof(cmd), "Main.Bass=%d", bass);
        queueCommand(cmd);
      }
    }

    void getBass(){
      if (!_model->hasTone) return;
      if (isRotel()) queueCommand(pick(CMDP_BASS_QUERY));
      else if (_cmds && _cmds->bassQuery) queueCommand(_cmds->bassQuery);
    }

    void updateBass(int bass){
      _currentBass = bass;
      notifyEvent(HOMEKIT_UPDATE_BASS);
    }

    void setTrebleStep(bool up){
      if (!_model->hasTone) return;
      if (isRotel()) queueCommand(up ? ROTEL_TREBLE_UP : ROTEL_TREBLE_DOWN);
      else if (_cmds) {
        const char *c = up ? _cmds->trebleUp : _cmds->trebleDown;
        if (c) queueCommand(c);
      }
    }

    void zeroTreble(){
      if (!_model->hasTone) return;
      if (isRotel()) queueCommand(ROTEL_TREBLE_ZERO);
      else if (_model->protocol == PROTO_NAD_V2) queueCommand("Main.Treble=0");
    }

    void setTrebleValue(int treble){
      if (!_model->hasTone) return;
      treble = snapToneValue(treble);
      char cmd[TX_CMD_MAX];
      if (isRotel()) {
        if (treble == 0) strlcpy(cmd, ROTEL_TREBLE_ZERO, sizeof(cmd));
        else             snprintf(cmd, sizeof(cmd), "treble_%+03d!", treble);
        queueCommand(cmd);
      } else if (_model->protocol == PROTO_NAD_V2) {
        snprintf(cmd, sizeof(cmd), "Main.Treble=%d", treble);
        queueCommand(cmd);
      }
    }

    void getTreble(){
      if (!_model->hasTone) return;
      if (isRotel()) queueCommand(pick(CMDP_TREBLE_QUERY));
      else if (_cmds && _cmds->trebleQuery) queueCommand(_cmds->trebleQuery);
    }

    void updateTreble(int treble){
      _currentTreble = treble;
      notifyEvent(HOMEKIT_UPDATE_TREBLE);
    }


    void blinkBalance(){
      if (_model->balanceMax <= 0 || _txCount > 0) return;
      if (!isRotel()) return;
      if (_currentBalance >= _model->balanceMax) {
        queueCommand(pick(CMDP_BALANCE_L));
        queueCommand(pick(CMDP_BALANCE_R));
      } else {
        queueCommand(pick(CMDP_BALANCE_R));
        queueCommand(pick(CMDP_BALANCE_L));
      }
    }

    void blinkBass(){
      // Blink nur Rotel: zwei Gegenbefehle ohne Netto-Aenderung
      if (!isRotel() || !_model->hasTone || _txCount > 0) return;
      if (_currentBass >= TONE_MAX) {
        queueCommand(ROTEL_BASS_DOWN);
        queueCommand(ROTEL_BASS_UP);
      } else {
        queueCommand(ROTEL_BASS_UP);
        queueCommand(ROTEL_BASS_DOWN);
      }
    }

    void blinkTreble(){
      if (!isRotel() || !_model->hasTone || _txCount > 0) return;
      if (_currentTreble >= TONE_MAX) {
        queueCommand(ROTEL_TREBLE_DOWN);
        queueCommand(ROTEL_TREBLE_UP);
      } else {
        queueCommand(ROTEL_TREBLE_UP);
        queueCommand(ROTEL_TREBLE_DOWN);
      }
    }


    void setSource(int src){
      if (src < 1 || src > _model->sourceCount) return;
      queueCommand(_model->sources[src - 1].command);
    }

    void getSource(){
      if (isRotel()) queueCommand(pick(CMDP_SOURCE_QUERY));
      else if (_cmds && _cmds->sourceQuery) queueCommand(_cmds->sourceQuery);
    }

    void updateSource(int src){
      _currentSource = src;
      notifyEvent(HOMEKIT_UPDATE_SOURCE);
    }


    void getFirmwareSystem(){
      if (isRotel()) queueCommand(pick(CMDP_VERSION_QUERY));
      else if (_cmds && _cmds->versionQuery) queueCommand(_cmds->versionQuery);
    }

    void updateFirmwareSystem(const char* firmware){
      strlcpy(_currentFirmwareSystem, firmware, sizeof(_currentFirmwareSystem));
      notifyEvent(HOMEKIT_UPDATE_FIRMWARE_SYSTEM);
    }

    void getFirmwareUSB(){
      if (!isRotel() || _gen1Active) return;
      queueCommand(ROTEL_FIRMWARE_USB_STATE);
    }

    void updateFirmwareUSB(const char* firmware){
      strlcpy(_currentFirmwareUSB, firmware, sizeof(_currentFirmwareUSB));
      notifyEvent(HOMEKIT_UPDATE_FIRMWARE_USB);
    }


private:
    HardwareSerial _serial{1};
    callbackUpdateFunction _updateCallback = nullptr;

    const AmpModelDef *_model = &AMP_MODELS[AMP_DEFAULT_MODEL];
    const CommandSet *_cmds = nullptr;
    bool _gen1Active = false;

    enum DetectState : uint8_t {
      DETECT_OFF = 0,
      DETECT_PROBE_G2,
      DETECT_PROBE_G1,
      DETECT_BACKOFF,
      DETECT_DONE
    };
    DetectState _detectState = DETECT_OFF;
    uint32_t _detectStartMs = 0;
    bool _frameSeen = false;
    bool _initPending = false;

    char _cmdBuf[CMD_BUF_LEN];
    char _valBuf[VAL_BUF_LEN];
    char _lineBuf[LINE_BUF_LEN];
    uint8_t _cmdLen = 0;
    uint8_t _valLen = 0;
    uint8_t _lineLen = 0;
    bool _readingValue = false;
    bool _discardFrame = false;
    bool _valDigitsOnly = false;
    int16_t _byteCountRemaining = 0;

    char _txQueue[TX_QUEUE_LEN][TX_CMD_MAX];
    uint8_t _txHead = 0;
    uint8_t _txTail = 0;
    uint8_t _txCount = 0;
    uint32_t _lastTxMs = 0;
    bool _awaitingResponse = false;
    bool _postPowerHold = false;   // Denon: Pause nach gesendetem PWON

    const ProtocolDef* proto() const {
      // Rotel AUTO/Gen: effektives Profil nach _gen1Active
      if (isRotel()) {
        return _gen1Active ? &PROTODEF_ROTEL_GEN1 : &PROTODEF_ROTEL_GEN2;
      }
      return ampProtocolDef(_model->protocol);
    }

    const char* pick(const RotelCmdPair &p) const {
      return _gen1Active ? p.gen1 : p.gen2;
    }

    // Bass/Treble auf Bereich UND erlaubtes Raster bringen. NAD verwirft
    // ungerade Werte kommentarlos -> UI und Geraet wuerden auseinanderlaufen.
    int snapToneValue(int v) const {
      if (v >  TONE_MAX) v =  TONE_MAX;
      if (v < -TONE_MAX) v = -TONE_MAX;
      const int step = toneStep();
      if (step > 1) {
        const int sign = (v < 0) ? -1 : 1;
        const int mag  = ((v * sign) + step / 2) / step * step;
        v = sign * (mag > TONE_MAX ? TONE_MAX : mag);
      }
      return v;
    }

    char frameTerminator() const {
      return proto()->rxTerm;
    }

    void finishFrame(){
      _awaitingResponse = false;
      _frameSeen = true;
      if (!_discardFrame && (_cmdLen > 0 || _valLen > 0)) {
        _cmdBuf[_cmdLen] = '\0';
        _valBuf[_valLen] = '\0';
        // Prefix strip (NAD "Main.", Marantz "@")
        const char *key = _cmdBuf;
        const char *pfx = proto()->stripPrefix;
        if (pfx && pfx[0]) {
          const size_t n = strlen(pfx);
          if (strncmp(_cmdBuf, pfx, n) == 0) key = _cmdBuf + n;
        }
        setStatus(key, _valBuf);
      }
      resetParser();
    }

    // Denon: längstes bekanntes Praefix matchen (MVMAX vor MV)
    void finishPositionalLine(){
      _awaitingResponse = false;
      _frameSeen = true;
      if (!_discardFrame && _lineLen > 0) {
        _lineBuf[_lineLen] = '\0';
        // Marantz-modern kann @-Zeilen mitschicken -> ignorieren
        if (_lineBuf[0] == '@') {
          resetParser();
          return;
        }
        // Reihenfolge: laengere Prefixe zuerst
        static const char *const PREFIXES[] = {
          "MVMAX", "PW", "MV", "MU", "SI", "Z2", "MS", "CV"
        };
        const char *key = nullptr;
        const char *val = _lineBuf;
        for (uint8_t i = 0; i < sizeof(PREFIXES) / sizeof(PREFIXES[0]); i++) {
          const size_t n = strlen(PREFIXES[i]);
          if (strncmp(_lineBuf, PREFIXES[i], n) == 0) {
            key = PREFIXES[i];
            val = _lineBuf + n;
            // fuehrendes Leerzeichen in MVMAX " 85" ueberspringen
            while (*val == ' ') val++;
            break;
          }
        }
        if (key) setStatus(key, val);
      }
      resetParser();
    }

    void resetParser(){
      _cmdLen = 0;
      _valLen = 0;
      _lineLen = 0;
      _readingValue = false;
      _discardFrame = false;
      _valDigitsOnly = false;
      _byteCountRemaining = 0;
    }

    void startProbe(DetectState st){
      _detectState = st;
      _frameSeen = false;
      resetParser();
      _detectStartMs = millis();
      if (st == DETECT_PROBE_G1) queueCommand("!");
      queueCommand(pick(CMDP_POWER_QUERY));
      LOG1("ROTEL: Generation-Probe Gen %d\n", _gen1Active ? 1 : 2);
    }

    void processDetect(){
      if (!isRotel()) return;
      if (_detectState == DETECT_OFF || _detectState == DETECT_DONE) return;

      const uint32_t now = millis();

      if (_frameSeen) {
        _detectState = DETECT_DONE;
        const uint8_t gen = _gen1Active ? ROTEL_GEN1 : ROTEL_GEN2;
        rotelSaveGenCache(gen);
        LOG0("ROTEL: Generation erkannt: Gen %d (im NVS gespeichert)\n", _gen1Active ? 1 : 2);
        if (_initPending) {
          _initPending = false;
          requestInitialState();
        }
        return;
      }

      if (_detectState == DETECT_BACKOFF) {
        if (now - _detectStartMs >= DETECT_BACKOFF_MS) {
          _gen1Active = false;
          startProbe(DETECT_PROBE_G2);
        }
        return;
      }

      if (_txCount > 0) {
        _detectStartMs = now;
        return;
      }

      if (now - _detectStartMs >= DETECT_TIMEOUT_MS) {
        if (_detectState == DETECT_PROBE_G2) {
          _gen1Active = true;
          startProbe(DETECT_PROBE_G1);
        } else {
          _gen1Active = false;
          _detectState = DETECT_BACKOFF;
          _detectStartMs = now;
        }
      }
    }

    void notifyEvent(AmpUpdateEvent ev){
      if (_updateCallback) _updateCallback(ev);
    }

    bool queueCommand(const char* cmd){
      if (!cmd || !cmd[0]) return false;
      if (_txCount >= TX_QUEUE_LEN) {
        LOG0("AMP: TX-Queue voll, Befehl verworfen\n");
        return false;
      }
      strlcpy(_txQueue[_txHead], cmd, TX_CMD_MAX);
      _txHead = (uint8_t)((_txHead + 1) % TX_QUEUE_LEN);
      _txCount++;
      return true;
    }

    void processTx(){
      if (_txCount == 0) return;
      const uint32_t now = millis();
      const ProtocolDef *p = proto();

      if (_postPowerHold) {
        if (now - _lastTxMs < p->postPowerOnDelayMs) return;
        _postPowerHold = false;
      }

      const uint32_t minIv = p->minCmdIntervalMs ? p->minCmdIntervalMs : MIN_CMD_INTERVAL_MS;
      const uint32_t to    = p->responseTimeoutMs ? p->responseTimeoutMs : RESPONSE_TIMEOUT_MS;
      if (now - _lastTxMs < minIv) return;
      if (_awaitingResponse && (now - _lastTxMs) < to) return;

      if (p->txPreambleCr) _serial.write((uint8_t)'\r');
      _serial.print(_txQueue[_txTail]);
      if (p->txTerm) _serial.write((uint8_t)p->txTerm);

      // Denon: nach PWON 1 s warten, bevor der naechste Befehl rausgeht
      if (p->postPowerOnDelayMs > 0 && _cmds && _cmds->powerOn &&
          strcmp(_txQueue[_txTail], _cmds->powerOn) == 0) {
        _postPowerHold = true;
      }

      _txTail = (uint8_t)((_txTail + 1) % TX_QUEUE_LEN);
      _txCount--;
      _lastTxMs = now;
      _awaitingResponse = true;
    }

    static bool eqOn(const char *val) {
      return strcmp(val, "on") == 0 || strcmp(val, "On") == 0 || strcmp(val, "ON") == 0;
    }
    static bool eqOff(const char *val) {
      return strcmp(val, "off") == 0 || strcmp(val, "Off") == 0 || strcmp(val, "OFF") == 0
          || strcmp(val, "standby") == 0;
    }

    void matchSource(const char *val){
      for (uint8_t i = 0; i < _model->sourceCount; i++) {
        if (strcmp(val, _model->sources[i].replyToken) == 0) {
          updateSource(i + 1);
          return;
        }
      }
    }

    void setStatus(const char* cmd, const char* val){
      // ---- Denon positional keys ----
      if (_model->protocol == PROTO_DENON) {
        if (strcmp(cmd, "MVMAX") == 0) {
          return;  // kein Volume
        }
        if (strcmp(cmd, "PW") == 0) {
          if (strncmp(val, "ON", 2) == 0) updatePower(true);
          else if (strncmp(val, "STANDBY", 7) == 0 || val[0] == '0') updatePower(false);
          return;
        }
        if (strcmp(cmd, "MV") == 0) {
          // "45" oder "455" (45.5) -> Ganzzahl /10 bei 3 Ziffern
          const size_t n = strlen(val);
          if (n >= 3) updateVolume(atoi(val) / 10);
          else        updateVolume(atoi(val));
          return;
        }
        if (strcmp(cmd, "MU") == 0) {
          if (strncmp(val, "ON", 2) == 0) updateMute(true);
          else if (strncmp(val, "OFF", 3) == 0) updateMute(false);
          return;
        }
        if (strcmp(cmd, "SI") == 0) {
          matchSource(val);
          return;
        }
        return;
      }

      // ---- Marantz classic (@ bereits gestrippt): PWR / VOL / AMT / SRC ----
      if (_model->protocol == PROTO_MARANTZ_CLASSIC) {
        if (strcmp(cmd, "PWR") == 0) {
          if (val[0] == '2') updatePower(true);
          else if (val[0] == '1') updatePower(false);
          return;
        }
        if (strcmp(cmd, "VOL") == 0) {
          // TO VERIFY: Antwortformat
          updateVolume(atoi(val));
          return;
        }
        if (strcmp(cmd, "AMT") == 0) {
          // TO VERIFY: 1/2 Zuordnung
          if (val[0] == '1') updateMute(true);
          else if (val[0] == '2') updateMute(false);
          return;
        }
        if (strcmp(cmd, "SRC") == 0) {
          matchSource(val);
          return;
        }
        return;
      }

      // ---- NAD (Keys nach Main.-Strip, z. B. Power) ----
      if (_model->protocol == PROTO_NAD_V2) {
        if (strcmp(cmd, NAD_VOLUME_KEY) == 0) {
          updateVolume(atoi(val));
        } else if (strcmp(cmd, NAD_POWER_KEY) == 0) {
          if (eqOn(val)) updatePower(true);
          else if (eqOff(val)) updatePower(false);
        } else if (strcmp(cmd, NAD_MUTE_KEY) == 0) {
          if (eqOn(val)) updateMute(true);
          else if (eqOff(val)) updateMute(false);
        } else if (strcmp(cmd, NAD_SOURCE_KEY) == 0) {
          matchSource(val);
        } else if (strcmp(cmd, NAD_TONE_KEY) == 0) {
          // ToneDefeat=On -> Klangregelung AUS (= Bypass)
          if (eqOn(val)) updateBypass(true);
          else if (eqOff(val)) updateBypass(false);
        } else if (strcmp(cmd, NAD_BASS_KEY) == 0) {
          updateBass(atoi(val));
        } else if (strcmp(cmd, NAD_TREBLE_KEY) == 0) {
          updateTreble(atoi(val));
        } else if (strcmp(cmd, NAD_VERSION_KEY) == 0 || strcmp(cmd, NAD_MODEL_KEY) == 0) {
          updateFirmwareSystem(val);
        }
        return;
      }

      // ---- Rotel (Gen1 + Gen2) ----
      if (strcmp(cmd, ROTEL_VOLUME_RECEIVE) == 0) {
        updateVolume(atoi(val));
      } else if (strcmp(cmd, ROTEL_POWER_RECEIVE) == 0) {
        if (strcmp(val, "on") == 0)           { updatePower(true); }
        else if (strcmp(val, "standby") == 0) { updatePower(false); }
      } else if (strcmp(cmd, ROTEL_MUTE_RECEIVE) == 0) {
        if (strcmp(val, "on") == 0)       { updateMute(true); }
        else if (strcmp(val, "off") == 0) { updateMute(false); }
      } else if (strcmp(cmd, ROTEL_SOURCE_RECEIVE) == 0) {
        matchSource(val);
      } else if (strcmp(cmd, ROTEL_BYPASS_RECEIVE) == 0) {
        if (strcmp(val, "on") == 0)       { updateBypass(true); }
        else if (strcmp(val, "off") == 0) { updateBypass(false); }
      } else if (strcmp(cmd, ROTEL_TONE_RECEIVE) == 0) {
        if (strcmp(val, "on") == 0)       { updateBypass(false); }
        else if (strcmp(val, "off") == 0) { updateBypass(true); }
      } else if (strcmp(cmd, ROTEL_BALANCE_RECEIVE) == 0) {
        if (val[0] == 'R' || val[0] == 'r')      { updateBalance(atoi(val + 1)); }
        else if (val[0] == 'L' || val[0] == 'l') { updateBalance(-atoi(val + 1)); }
        else                                     { updateBalance(0); }
      } else if (strcmp(cmd, ROTEL_BASS_RECEIVE) == 0) {
        updateBass(atoi(val));
      } else if (strcmp(cmd, ROTEL_TREBLE_RECEIVE) == 0) {
        updateTreble(atoi(val));
      } else if (strcmp(cmd, ROTEL_FIRMWARE_SYSTEM_RECEIVE) == 0 ||
                 strcmp(cmd, ROTEL_FIRMWARE_GEN1_RECEIVE) == 0) {
        updateFirmwareSystem(val);
      } else if (strcmp(cmd, ROTEL_FIRMWARE_USB_RECEIVE) == 0) {
        updateFirmwareUSB(val);
      }
    }

};

#endif
