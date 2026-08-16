/*
 * AmpModels.h – Multi-Brand-Modelltabelle (Rotel / NAD / Denon / Marantz)
 *
 * Rotel-Eintraege behalten feste Indizes 0..N-1 (NVS-Kompatibilitaet).
 * Neue Marken werden angehaengt.
 *
 * NAD-AVRs: verifiziert gegen die PDFs im Ordner NAD (Spec 2.03 + Command-Docs).
 * Denon/Marantz + NAD-C-Serie ohne PDF hier: verified=false (TO VERIFY).
 *
 * Alle Tabellen const im Flash – keine Laufzeit-Allokation.
 */

#ifndef AMP_MODELS_H
#define AMP_MODELS_H

#include "Arduino.h"
#include <Preferences.h>
#include "AmpProtocols.h"

// Protokollgeneration (nur Rotel; Werte = NVS-Cache-Kodierung, 0 = unbekannt)
enum RotelGeneration : uint8_t {
  ROTEL_GEN_UNKNOWN = 0,
  ROTEL_GEN1        = 1,
  ROTEL_GEN2        = 2,
  ROTEL_GEN_AUTO    = 3
};

enum AmpDeviceType : uint8_t {
  AMP_FULL_AMP  = 0,
  AMP_POWER_AMP = 1
};

// HomeKit-Source-ID (Index+1) <-> Geraete-Token
struct AmpSourceDef {
  const char *replyToken;   // Wert in der Antwort (nach Prefix-Strip)
  const char *command;      // kompletter Set-Befehl (Rotel inkl. '!', sonst ohne Term)
  const char *name;         // Anzeigename Home-App / Webremote
};

struct AmpModelDef {
  const char *brand;              // "Rotel" / "NAD" / …
  const char *name;               // Anzeigename Dashboard
  AmpProtocolId protocol;
  uint8_t     generation;         // nur Rotel: GEN1/GEN2/AUTO, sonst ROTEL_GEN_UNKNOWN
  uint8_t     deviceType;         // FULL_AMP / POWER_AMP
  const AmpSourceDef *sources;
  uint8_t     sourceCount;
  int8_t      balanceMax;         // 0 = kein Balance
  bool        hasTone;            // Bass/Treble/(Bypass|ToneDefeat)
  int16_t     volumeMin;          // Rotel 0, NAD -99, Denon 0
  int16_t     volumeMax;          // Rotel 96, NAD 19, Denon 98
  bool        verified;           // false = TO VERIFY am Geraet
};

// ---------- Quellenlisten Rotel ----------

static const AmpSourceDef ROTEL_SRC_A14[] = {
  { "cd",        "cd!",        "CD"        },
  { "coax1",     "coax1!",     "Coax 1"    },
  { "coax2",     "coax2!",     "Coax 2"    },
  { "opt1",      "opt1!",      "Optical 1" },
  { "opt2",      "opt2!",      "Optical 2" },
  { "aux1",      "aux1!",      "AUX 1"     },
  { "aux2",      "aux2!",      "AUX 2"     },
  { "tuner",     "tuner!",     "Tuner"     },
  { "phono",     "phono!",     "Phono"     },
  { "usb",       "usb!",       "USB"       },
  { "bluetooth", "bluetooth!", "Bluetooth" },
  { "pc_usb",    "pcusb!",     "PC USB"    },
};

static const AmpSourceDef ROTEL_SRC_A11[] = {
  { "cd",        "cd!",        "CD"        },
  { "coax1",     "coax1!",     "Coax 1"    },
  { "coax2",     "coax2!",     "Coax 2"    },
  { "opt1",      "opt1!",      "Optical 1" },
  { "opt2",      "opt2!",      "Optical 2" },
  { "aux1",      "aux1!",      "AUX 1"     },
  { "aux2",      "aux2!",      "AUX 2"     },
  { "tuner",     "tuner!",     "Tuner"     },
  { "phono",     "phono!",     "Phono"     },
  { "bluetooth", "bluetooth!", "Bluetooth" },
};

static const AmpSourceDef ROTEL_SRC_RA157X[] = {
  { "cd",        "cd!",        "CD"        },
  { "coax1",     "coax1!",     "Coax 1"    },
  { "coax2",     "coax2!",     "Coax 2"    },
  { "coax3",     "coax3!",     "Coax 3"    },
  { "opt1",      "opt1!",      "Optical 1" },
  { "opt2",      "opt2!",      "Optical 2" },
  { "opt3",      "opt3!",      "Optical 3" },
  { "aux1",      "aux1!",      "AUX 1"     },
  { "aux2",      "aux2!",      "AUX 2"     },
  { "tuner",     "tuner!",     "Tuner"     },
  { "phono",     "phono!",     "Phono"     },
  { "usb",       "usb!",       "USB"       },
  { "bluetooth", "bluetooth!", "Bluetooth" },
  { "pc_usb",    "pcusb!",     "PC USB"    },
  { "bal_xlr",   "bal_xlr!",   "XLR"       },
};

static const AmpSourceDef ROTEL_SRC_MICHI_X[] = {
  { "coax1",     "coax1!",     "Coax 1"    },
  { "coax2",     "coax2!",     "Coax 2"    },
  { "opt1",      "opt1!",      "Optical 1" },
  { "opt2",      "opt2!",      "Optical 2" },
  { "aux1",      "aux1!",      "AUX 1"     },
  { "aux2",      "aux2!",      "AUX 2"     },
  { "phono",     "phono!",     "Phono"     },
  { "usb",       "usb!",       "USB"       },
  { "bluetooth", "bluetooth!", "Bluetooth" },
  { "pc_usb",    "pcusb!",     "PC USB"    },
  { "bal_xlr",   "bal_xlr!",   "XLR"       },
};

static const AmpSourceDef ROTEL_SRC_RA12[] = {
  { "cd",        "cd!",        "CD"        },
  { "coax1",     "coax1!",     "Coax 1"    },
  { "coax2",     "coax2!",     "Coax 2"    },
  { "opt1",      "opt1!",      "Optical 1" },
  { "opt2",      "opt2!",      "Optical 2" },
  { "aux1",      "aux1!",      "AUX 1"     },
  { "aux2",      "aux2!",      "AUX 2"     },
  { "tuner",     "tuner!",     "Tuner"     },
  { "phono",     "phono!",     "Phono"     },
  { "usb",       "usb!",       "USB"       },
};

static const AmpSourceDef ROTEL_SRC_RA1570[] = {
  { "cd",        "cd!",        "CD"        },
  { "coax1",     "coax1!",     "Coax 1"    },
  { "coax2",     "coax2!",     "Coax 2"    },
  { "opt1",      "opt1!",      "Optical 1" },
  { "opt2",      "opt2!",      "Optical 2" },
  { "aux1",      "aux1!",      "AUX 1"     },
  { "aux2",      "aux2!",      "AUX 2"     },
  { "tuner",     "tuner!",     "Tuner"     },
  { "phono",     "phono!",     "Phono"     },
  { "usb",       "usb!",       "USB"       },
  { "bal_xlr",   "bal_xlr!",   "XLR"       },
  { "pc_usb",    "pc_usb!",    "PC USB"    },
};

// NAD AVR: Quellen sind nummeriert 1..10 (Namen geraete-/OSD-seitig konfigurierbar)
static const AmpSourceDef NAD_SRC_1_10[] = {
  { "1",  "Main.Source=1",  "Source 1"  },
  { "2",  "Main.Source=2",  "Source 2"  },
  { "3",  "Main.Source=3",  "Source 3"  },
  { "4",  "Main.Source=4",  "Source 4"  },
  { "5",  "Main.Source=5",  "Source 5"  },
  { "6",  "Main.Source=6",  "Source 6"  },
  { "7",  "Main.Source=7",  "Source 7"  },
  { "8",  "Main.Source=8",  "Source 8"  },
  { "9",  "Main.Source=9",  "Source 9"  },
  { "10", "Main.Source=10", "Source 10" },
};

// Denon / Marantz modern – generische SI-Liste (TO VERIFY pro Modell)
static const AmpSourceDef DENON_SRC_GENERIC[] = {
  { "CD",      "SICD",      "CD"         },
  { "TUNER",   "SITUNER",   "Tuner"      },
  { "DVD",     "SIDVD",     "DVD"        },
  { "BD",      "SIBD",      "Blu-ray"    },
  { "TV",      "SITV",      "TV"         },
  { "SAT/CBL", "SISAT/CBL", "Sat/Cable"  },
  { "MPLAY",   "SIMPLAY",   "Media Player"},
  { "GAME",    "SIGAME",    "Game"       },
  { "AUX1",    "SIAUX1",    "AUX 1"      },
  { "BT",      "SIBT",      "Bluetooth"  },
  { "PHONO",   "SIPHONO",   "Phono"      },
  { "NET",     "SINET",     "Network"    },
  { "USB",     "SIUSB",     "USB"        },
};

// Marantz classic – Platzhalter (TO VERIFY)
static const AmpSourceDef MARANTZ_CLASSIC_SRC[] = {
  { "00", "@SRC:00", "Source 0" },
  { "01", "@SRC:01", "Source 1" },
  { "02", "@SRC:02", "Source 2" },
  { "03", "@SRC:03", "Source 3" },
  { "04", "@SRC:04", "Source 4" },
  { "05", "@SRC:05", "Source 5" },
};

// ---------- Modelltabelle ----------

#define AMP_SRC_ENTRY(list) list, (uint8_t)(sizeof(list) / sizeof(list[0]))

// Indizes 0..8 = bestehende Rotel-NVS-Werte – Reihenfolge nicht aendern!
static constexpr AmpModelDef AMP_MODELS[] = {
  // brand, name, protocol, generation, type, sources, balMax, tone, volMin, volMax, verified
  { "Rotel", "A11 (+MKII)",              PROTO_ROTEL_GEN2, ROTEL_GEN2,     AMP_FULL_AMP,  AMP_SRC_ENTRY(ROTEL_SRC_A11),    15, true,  0, 96, true },
  { "Rotel", "A12 / A14 (+MKII)",        PROTO_ROTEL_GEN2, ROTEL_GEN2,     AMP_FULL_AMP,  AMP_SRC_ENTRY(ROTEL_SRC_A14),    15, true,  0, 96, true },
  { "Rotel", "RA-11 / RA-12",            PROTO_ROTEL_GEN1, ROTEL_GEN1,     AMP_FULL_AMP,  AMP_SRC_ENTRY(ROTEL_SRC_RA12),   15, true,  0, 96, true },
  { "Rotel", "RA-1570",                  PROTO_ROTEL_GEN1, ROTEL_GEN1,     AMP_FULL_AMP,  AMP_SRC_ENTRY(ROTEL_SRC_RA1570), 15, true,  0, 96, true },
  { "Rotel", "RA-1572 (+MKII)",          PROTO_ROTEL_AUTO, ROTEL_GEN_AUTO, AMP_FULL_AMP,  AMP_SRC_ENTRY(ROTEL_SRC_RA157X), 15, true,  0, 96, true },
  { "Rotel", "RA-1592 (+MKII)",          PROTO_ROTEL_AUTO, ROTEL_GEN_AUTO, AMP_FULL_AMP,  AMP_SRC_ENTRY(ROTEL_SRC_RA157X), 15, true,  0, 96, true },
  { "Rotel", "RA-6000",                  PROTO_ROTEL_GEN2, ROTEL_GEN2,     AMP_FULL_AMP,  AMP_SRC_ENTRY(ROTEL_SRC_RA157X), 15, true,  0, 96, true },
  { "Rotel", "Michi X3 / X5",            PROTO_ROTEL_GEN2, ROTEL_GEN2,     AMP_FULL_AMP,  AMP_SRC_ENTRY(ROTEL_SRC_MICHI_X),10, true,  0, 96, true },
  { "Rotel", "Michi M8 / S5 (Endstufe)", PROTO_ROTEL_GEN2, ROTEL_GEN2,     AMP_POWER_AMP, nullptr, 0,                         0, false, 0, 96, true },

  // NAD AVRs – Spec + Command-PDFs im Ordner NAD/ (Source 1..10)
  { "NAD", "T 755",   PROTO_NAD_V2, ROTEL_GEN_UNKNOWN, AMP_FULL_AMP, AMP_SRC_ENTRY(NAD_SRC_1_10), 0, true, -99, 19, true },
  { "NAD", "T 765",   PROTO_NAD_V2, ROTEL_GEN_UNKNOWN, AMP_FULL_AMP, AMP_SRC_ENTRY(NAD_SRC_1_10), 0, true, -99, 19, true },
  { "NAD", "T 775",   PROTO_NAD_V2, ROTEL_GEN_UNKNOWN, AMP_FULL_AMP, AMP_SRC_ENTRY(NAD_SRC_1_10), 0, true, -99, 19, true },
  { "NAD", "T 785",   PROTO_NAD_V2, ROTEL_GEN_UNKNOWN, AMP_FULL_AMP, AMP_SRC_ENTRY(NAD_SRC_1_10), 0, true, -99, 19, true },
  { "NAD", "T 175",   PROTO_NAD_V2, ROTEL_GEN_UNKNOWN, AMP_FULL_AMP, AMP_SRC_ENTRY(NAD_SRC_1_10), 0, true, -99, 19, true },
  { "NAD", "T 777",   PROTO_NAD_V2, ROTEL_GEN_UNKNOWN, AMP_FULL_AMP, AMP_SRC_ENTRY(NAD_SRC_1_10), 0, true, -99, 19, true },
  { "NAD", "T 787",   PROTO_NAD_V2, ROTEL_GEN_UNKNOWN, AMP_FULL_AMP, AMP_SRC_ENTRY(NAD_SRC_1_10), 0, true, -99, 19, true },
  { "NAD", "T 187",   PROTO_NAD_V2, ROTEL_GEN_UNKNOWN, AMP_FULL_AMP, AMP_SRC_ENTRY(NAD_SRC_1_10), 0, true, -99, 19, true },
  { "NAD", "M15 HD",  PROTO_NAD_V2, ROTEL_GEN_UNKNOWN, AMP_FULL_AMP, AMP_SRC_ENTRY(NAD_SRC_1_10), 0, true, -99, 19, true },

  // NAD Stereo / Masters – RS232 per Addendum 2026-08 bestaetigt (nad-models-verified-addendum.md).
  // Quellenliste generisch 1..10; modellspezifische Nummern noch VERIFY (BluOS/C 388).
  // C 328: RS232 als 3,5-mm-Klinke (Adapter noetig), kein C 338 (kein RS232).
  { "NAD", "M 33",   PROTO_NAD_V2, ROTEL_GEN_UNKNOWN, AMP_FULL_AMP, AMP_SRC_ENTRY(NAD_SRC_1_10), 0, true, -80, 0, true },
  { "NAD", "C 399",  PROTO_NAD_V2, ROTEL_GEN_UNKNOWN, AMP_FULL_AMP, AMP_SRC_ENTRY(NAD_SRC_1_10), 0, true, -80, 0, true },
  { "NAD", "C 389",  PROTO_NAD_V2, ROTEL_GEN_UNKNOWN, AMP_FULL_AMP, AMP_SRC_ENTRY(NAD_SRC_1_10), 0, true, -80, 0, true },
  { "NAD", "C 388",  PROTO_NAD_V2, ROTEL_GEN_UNKNOWN, AMP_FULL_AMP, AMP_SRC_ENTRY(NAD_SRC_1_10), 0, true, -80, 0, true },
  { "NAD", "C 379",  PROTO_NAD_V2, ROTEL_GEN_UNKNOWN, AMP_FULL_AMP, AMP_SRC_ENTRY(NAD_SRC_1_10), 0, true, -80, 0, true },
  { "NAD", "C 368",  PROTO_NAD_V2, ROTEL_GEN_UNKNOWN, AMP_FULL_AMP, AMP_SRC_ENTRY(NAD_SRC_1_10), 0, true, -80, 0, true },
  { "NAD", "C 328",  PROTO_NAD_V2, ROTEL_GEN_UNKNOWN, AMP_FULL_AMP, AMP_SRC_ENTRY(NAD_SRC_1_10), 0, true, -80, 0, true },

  // Denon – Protokoll laut Doku implementiert, Geraetetest offen
  { "Denon", "AVR (Denon RS232, to verify)", PROTO_DENON, ROTEL_GEN_UNKNOWN, AMP_FULL_AMP, AMP_SRC_ENTRY(DENON_SRC_GENERIC), 0, false, 0, 98, false },

  // Marantz modern = Denon-Grammatik
  { "Marantz", "NR/SR modern (to verify)", PROTO_DENON, ROTEL_GEN_UNKNOWN, AMP_FULL_AMP, AMP_SRC_ENTRY(DENON_SRC_GENERIC), 0, false, 0, 98, false },
  // Marantz classic
  { "Marantz", "SR classic @CMD (to verify)", PROTO_MARANTZ_CLASSIC, ROTEL_GEN_UNKNOWN, AMP_FULL_AMP, AMP_SRC_ENTRY(MARANTZ_CLASSIC_SRC), 0, false, 0, 99, false },
};

constexpr uint8_t AMP_MODEL_COUNT   = sizeof(AMP_MODELS) / sizeof(AMP_MODELS[0]);
constexpr uint8_t AMP_DEFAULT_MODEL = 1;    // A12/A14
constexpr uint8_t AMP_MAX_SOURCES   = 16;

constexpr bool ampSourcesFit(uint8_t i = 0){
  return i >= AMP_MODEL_COUNT
      || (AMP_MODELS[i].sourceCount <= AMP_MAX_SOURCES && ampSourcesFit((uint8_t)(i + 1)));
}
static_assert(ampSourcesFit(), "Quellenliste eines Modells ueberschreitet AMP_MAX_SOURCES");

// ---------- NVS: Modellauswahl + Generation-Cache (Namespace "rotelcfg") ----------
// Namespace bleibt "rotelcfg": Umbenennen wuerde die gespeicherte
// Modellauswahl bestehender Geraete verwerfen.

inline uint8_t ampLoadModelIndex(){
  Preferences p;
  p.begin("rotelcfg", true);
  uint8_t idx = p.getUChar("model", AMP_DEFAULT_MODEL);
  p.end();
  if (idx >= AMP_MODEL_COUNT) idx = AMP_DEFAULT_MODEL;
  return idx;
}

inline void ampSaveModelIndex(uint8_t idx){
  Preferences p;
  p.begin("rotelcfg", false);
  p.putUChar("model", idx);
  p.remove("gencache");
  p.end();
}

inline uint8_t rotelLoadGenCache(){
  Preferences p;
  p.begin("rotelcfg", true);
  uint8_t g = p.getUChar("gencache", ROTEL_GEN_UNKNOWN);
  p.end();
  return (g == ROTEL_GEN1 || g == ROTEL_GEN2) ? g : (uint8_t)ROTEL_GEN_UNKNOWN;
}

inline void rotelSaveGenCache(uint8_t gen){
  Preferences p;
  p.begin("rotelcfg", false);
  p.putUChar("gencache", gen);
  p.end();
}

#endif
