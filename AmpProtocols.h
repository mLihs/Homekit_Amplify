/*
 * AmpProtocols.h – Protokoll-Definitionen fuer Multi-Brand RS232
 *
 * Quellen:
 *   Rotel:  Rotel/rs232-serial.md
 *   NAD:    NAD/…/nad_ethernet_rs232_spec_2.03.pdf + Modell-Command-PDFs
 *   Denon / Marantz modern: amp-rs232-protocols.md (TO VERIFY am Geraet)
 *   Marantz classic:        amp-rs232-protocols.md (TO VERIFY am Geraet)
 *
 * Alles const/Flash – keine Laufzeit-Allokation.
 */

#ifndef AMP_PROTOCOLS_H
#define AMP_PROTOCOLS_H

#include "Arduino.h"

enum AmpProtocolId : uint8_t {
  PROTO_ROTEL_GEN2 = 0,
  PROTO_ROTEL_GEN1 = 1,
  PROTO_ROTEL_AUTO = 2,   // nur Rotel: Gen zur Laufzeit erkennen
  PROTO_NAD_V2     = 3,
  PROTO_DENON      = 4,   // inkl. Marantz modern (gleiche Grammatik)
  PROTO_MARANTZ_CLASSIC = 5
};

enum FrameStyle : uint8_t {
  FS_KEYVALUE = 0,           // key<divider>value + rxTerm
  FS_KEYVALUE_BYTECOUNT = 1, // Rotel Gen1 Byte-Count
  FS_POSITIONAL = 2          // Denon: Praefix + Rest (kein Divider)
};

// Festes Protokollprofil; Baud/Terminatoren/Timing modellunabhaengig
struct ProtocolDef {
  uint32_t   baud;
  char       txTerm;              // 0 = Terminator steckt schon im Befehl (Rotel '!')
  char       rxTerm;              // primaerer Empfangs-Terminator
  char       rxTermAlt;           // optional zweiter (z. B. '\n' neben '\r'), sonst 0
  FrameStyle style;
  char       divider;             // '=' / ':' / 0 bei positional
  const char *stripPrefix;        // "Main." / "@" / nullptr
  bool       txPreambleCr;        // NAD: CR vor dem Befehl (Noise-Flush)
  uint8_t    prefixLen;           // Denon: Mindest-Praefixlaenge (2)
  uint8_t    toneStep;            // erlaubte Schrittweite Bass/Treble (NAD: 2)
  uint32_t   minCmdIntervalMs;
  uint32_t   responseTimeoutMs;
  uint32_t   postPowerOnDelayMs;  // Denon: 1000 ms nach PWON
};

// Statische Befehlssaetze (ohne Terminator, ausser Rotel wo '!' eingebettet ist).
// nullptr = Feature nicht unterstuetzt / nicht senden.
struct CommandSet {
  const char *powerOn;
  const char *powerOff;
  const char *powerQuery;
  const char *volUp;
  const char *volDown;
  const char *volQuery;
  const char *muteOn;
  const char *muteOff;
  const char *muteQuery;
  const char *sourceQuery;
  const char *subscribe;     // Push-Events aktivieren; nullptr = immer an
  const char *toneEnable;    // Klangregelung EIN (Bypass aus / ToneDefeat Off)
  const char *toneDisable;
  const char *toneQuery;
  const char *bassUp;
  const char *bassDown;
  const char *bassQuery;
  const char *trebleUp;
  const char *trebleDown;
  const char *trebleQuery;
  const char *balanceL;
  const char *balanceR;
  const char *balanceQuery;
  const char *versionQuery;
};

// ---------- Protokoll-Profile ----------

static const ProtocolDef PROTODEF_ROTEL_GEN2 = {
  115200, 0, '$', 0, FS_KEYVALUE, '=', nullptr, false, 0, 1, 100, 300, 0
};
static const ProtocolDef PROTODEF_ROTEL_GEN1 = {
  115200, 0, '!', 0, FS_KEYVALUE_BYTECOUNT, '=', nullptr, false, 0, 1, 100, 300, 0
};
// AUTO startet mit Gen2-Profil; Gen1-Wechsel setzt _gen1Active
static const ProtocolDef PROTODEF_ROTEL_AUTO = {
  115200, 0, '$', 0, FS_KEYVALUE, '=', nullptr, false, 0, 1, 100, 300, 0
};
// toneStep 2: NAD akzeptiert laut Command-Docs nur gerade Bass/Treble-Werte
// ("Range -10 - 10 Step 2"); ungerade Werte werden vom Geraet still verworfen
static const ProtocolDef PROTODEF_NAD_V2 = {
  115200, '\r', '\r', '\n', FS_KEYVALUE, '=', "Main.", true, 0, 2, 50, 400, 0
};
static const ProtocolDef PROTODEF_DENON = {
  9600, '\r', '\r', '\n', FS_POSITIONAL, 0, nullptr, false, 2, 1, 100, 500, 1000
};
static const ProtocolDef PROTODEF_MARANTZ_CLASSIC = {
  9600, '\r', '\r', '\n', FS_KEYVALUE, ':', "@", false, 0, 1, 100, 500, 0
};

inline const ProtocolDef* ampProtocolDef(AmpProtocolId id) {
  switch (id) {
    case PROTO_ROTEL_GEN1:       return &PROTODEF_ROTEL_GEN1;
    case PROTO_ROTEL_AUTO:       return &PROTODEF_ROTEL_AUTO;
    case PROTO_NAD_V2:           return &PROTODEF_NAD_V2;
    case PROTO_DENON:            return &PROTODEF_DENON;
    case PROTO_MARANTZ_CLASSIC:  return &PROTODEF_MARANTZ_CLASSIC;
    case PROTO_ROTEL_GEN2:
    default:                     return &PROTODEF_ROTEL_GEN2;
  }
}

// ---------- CommandSets (Nicht-Rotel; Rotel bleibt bei RotelCmdPair/pick) ----------

static const CommandSet CMDSET_NAD_V2 = {
  "Main.Power=On", "Main.Power=Off", "Main.Power?",
  "Main.Volume+", "Main.Volume-", "Main.Volume?",
  "Main.Mute=On", "Main.Mute=Off", "Main.Mute?",
  "Main.Source?",
  nullptr,
  "Main.ToneDefeat=Off", "Main.ToneDefeat=On", "Main.ToneDefeat?",
  "Main.Bass+", "Main.Bass-", "Main.Bass?",
  "Main.Treble+", "Main.Treble-", "Main.Treble?",
  nullptr, nullptr, nullptr,
  "Main.Version?"
};

// Denon / Marantz modern – TO VERIFY am Geraet
static const CommandSet CMDSET_DENON = {
  "PWON", "PWSTANDBY", "PW?",
  "MVUP", "MVDOWN", "MV?",
  "MUON", "MUOFF", "MU?",
  "SI?",
  nullptr,
  nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr,
  nullptr
};

// Marantz classic – TO VERIFY (Syntax laut amp-rs232-protocols.md)
static const CommandSet CMDSET_MARANTZ_CLASSIC = {
  "@PWR:2", "@PWR:1", "@PWR:?",
  "@VOL:0+", "@VOL:0-", "@VOL:?",
  "@AMT:1", "@AMT:2", "@AMT:?",
  "@SRC:?",
  nullptr,
  nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr,
  nullptr
};

inline const CommandSet* ampCommandSet(AmpProtocolId id) {
  switch (id) {
    case PROTO_NAD_V2:          return &CMDSET_NAD_V2;
    case PROTO_DENON:           return &CMDSET_DENON;
    case PROTO_MARANTZ_CLASSIC: return &CMDSET_MARANTZ_CLASSIC;
    default:                    return nullptr;  // Rotel: pick()
  }
}

inline bool ampIsRotel(AmpProtocolId id) {
  return id == PROTO_ROTEL_GEN1 || id == PROTO_ROTEL_GEN2 || id == PROTO_ROTEL_AUTO;
}

#endif
