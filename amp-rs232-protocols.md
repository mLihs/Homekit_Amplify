# Multi-Brand Amplifier RS232 Control — AI Agent Knowledge Base

**Scope:** Serial (RS232) control of Rotel, NAD, Denon, and Marantz amplifiers/receivers.
**Target project:** Rotel-HomeKit (ESP32-C3 + HomeSpan) — extension to a multi-brand bridge.
**Audience:** AI coding agents. All statements are implementation-relevant. Items marked `VERIFY` must be checked against the model-specific manufacturer document before shipping.

---

## 1. Protocol Family Overview

| Property | Rotel Gen2 | Rotel Gen1 | NAD | Denon / Marantz (modern) | Marantz (classic) |
|---|---|---|---|---|---|
| Baud rate | 115200 | 115200 | 115200 | 9600 | 9600 |
| Data format | 8N1 | 8N1 | 8N1 | 8N1 | 8N1 |
| Flow control | none | none | none | none | none |
| TX frame | `cmd!` | `cmd!` | `<CR>Section.Var=Val<CR>` | `CMDparam<CR>` | `@CMD:val<CR>` |
| TX terminator | `!` | `!` | `\r` (0x0D) | `\r` (0x0D) | `\r` (0x0D) |
| RX frame | `key=value$` | `key=value!` or byte-count | `Section.Var=Val<CR>` | `CMDparam<CR>` | `@CMD:val<CR>` |
| RX terminator | `$` | `!` / none (byte-count) | `\r` | `\r` | `\r` |
| Key/value divider | `=` | `=` | `=` (strip `Main.` prefix) | positional (first 2 chars = key) | `:` (strip `@` prefix) |
| Query syntax | `volume?` | `get_volume!` | `Main.Volume?` | `MV?` | `@VOL:?` |
| Unsolicited events | yes, after `rs232_update_on!` | yes, after `display_update_auto!` | yes (echo on change) | yes (always on) | yes |
| Special timing | ≥50–100 ms between commands | same | none documented | **wait 1 s after `PWON`** | ACK-based, `VERIFY` |
| DB9 wiring from DTE adapter | straight | straight | straight (`VERIFY` per model) | straight (device is DCE) | straight (`VERIFY`) |

**Universal rule:** send commands sequentially. Send next command only after the response arrived OR after a per-protocol timeout. Never burst.

---

## 2. Protocol Details

### 2.1 Rotel (see `rotel-rs232-serial.md` for the full catalog)
- Gen2: commands end `!`, responses end `$`, e.g. TX `power_on!` → RX `power=on$`.
- Gen1: responses end `!` or use byte-count format `key={len},{text}` with **no terminator**.
- Generation auto-detect: send `power?`; a `$`-terminated reply ⇒ Gen2.
- Tone logic inverted between generations: Gen1 `tone_on!` ≡ Gen2 `bypass_off!`.

### 2.2 NAD (protocol spec v2.x, `nad_ethernet_rs232_spec_2.03.pdf`)
- All communication at 115200 8N1, no flow control.
- Command grammar: `<section>.<variable><operator><value>` where operator ∈ `=`, `+`, `-`, `?`.
  - `Main.Power=On`, `Main.Power?`, `Main.Volume+`, `Main.Volume=-40` (dB values).
- Recommended: prepend `\r` (and/or `\n`) before each message to flush line noise on the receiving side.
- Responses echo the resulting state: `Main.Power=On<CR>`. Changes made on the device front panel are also pushed in this format.
- Volume is in **dB** on most models (float steps possible). Some models expose percentage display mode; keep dB.
- Sections: `Main.` for the integrated amp; tuner-equipped models add `Tuner.`; zone-capable models add `Zone2.` etc.
- Typical variables: `Power`, `Volume`, `Mute`, `Source`, `Model`, `Version`, `SpeakerA`, `SpeakerB`, `Bass`, `Treble` (`VERIFY` per model).
- Source values are model-specific integers or names (`Main.Source=1` vs named); `VERIFY` against the per-model NAD doc (nadelectronics.com → software → protocol docs per model).

### 2.3 Denon (AVR control protocol, versions 4.x–8.x)
- 9600 8N1. Connector on device: DB-9 female, **DCE**, slave straight connection ⇒ use a straight cable from the ESP32 DTE adapter.
- Command = 2-letter command code + parameter + `<CR>`. No spaces, no divider.
  - Power: `PWON`, `PWSTANDBY`, query `PW?`
  - Master volume: `MVUP`, `MVDOWN`, `MV45` (=45), `MV455` (=45.5, three ASCII chars), query `MV?`
  - Mute: `MUON`, `MUOFF`, query `MU?`
  - Source: `SICD`, `SITUNER`, `SIDVD`, `SIBD`, `SITV`, `SISAT/CBL`, `SIMPLAY`, `SIGAME`, `SIAUX1`, `SIBT`, `SIPHONO`, `SINET`, `SIUSB` (set varies per model, `VERIFY`), query `SI?`
  - Zone2: `Z2ON`, `Z2OFF`, `Z2UP`, `Z2DOWN`, `Z2<vol>`
- **Timing rule: after `PWON`, wait 1 second before the next command.**
- Responses/events use the same grammar (`PWON<CR>`, `MV45<CR>`, `MVMAX 85<CR>`). Events are pushed on any state change without subscription.
- Parser: match longest known 2–3 letter prefix (`PW`, `MV`, `MU`, `SI`, `Z2`, `MS`, `CV`), remainder is the value. Note `MVMAX` — filter before treating as volume.

### 2.4 Marantz — modern NR/SR AV receivers (≈2011+)
- Identical grammar to Denon (`PWON`, `MV`, `MU`, `SI...`) — shared D+M platform. Official doc: "Marantz 2014 NR Series / SR Series RS232 IP Protocol".
- Devices may additionally emit classic-format lines (e.g. reply to `PWON` can include `@PWR:2`). Parser must **ignore unknown `@`-prefixed lines** gracefully when driving in Denon mode.

### 2.5 Marantz — classic protocol (older SR/DV, e.g. SR4200–SR8500)
- 9600 8N1. Frame: `@CMD:value<CR>`.
  - Power: `@PWR:2` = on, `@PWR:1` = standby, `@PWR:0` = toggle, query `@PWR:?`
  - Volume: `@VOL:0+` / `@VOL:0-` (step), `@VOL:0±xx` absolute, query `@VOL:?` (`VERIFY` exact syntax per model doc)
  - Mute: `@AMT:1`/`@AMT:2`, Source: `@SRC:xx`
- Device must not be in "economy standby" mode or it will not accept serial power-on.
- Only implement if a user actually owns such a device — lowest priority.

---

## 3. Supported Models (JSON)

```json
{
  "protocols": {
    "rotel_gen2": { "baud": 115200, "txTerm": "!", "rxTerm": "$", "style": "keyvalue", "divider": "=" },
    "rotel_gen1": { "baud": 115200, "txTerm": "!", "rxTerm": "!", "style": "keyvalue_bytecount", "divider": "=" },
    "nad_v2":     { "baud": 115200, "txTerm": "\r", "rxTerm": "\r", "style": "keyvalue", "divider": "=", "stripPrefix": "Main.", "txPreamble": "\r" },
    "denon":      { "baud": 9600,  "txTerm": "\r", "rxTerm": "\r", "style": "positional", "prefixLen": 2, "postPowerOnDelayMs": 1000 },
    "marantz_classic": { "baud": 9600, "txTerm": "\r", "rxTerm": "\r", "style": "keyvalue", "divider": ":", "stripPrefix": "@" }
  },
  "brands": [
    {
      "brand": "Rotel",
      "models": [
        { "id": "a14",     "name": "A14 / A14 MKII",   "protocol": "rotel_gen2", "type": "integrated", "verified": true },
        { "id": "a12",     "name": "A12 / A12 MKII",   "protocol": "rotel_gen2", "type": "integrated", "verified": true },
        { "id": "a11",     "name": "A11 / A11 MKII",   "protocol": "rotel_gen2", "type": "integrated", "verified": true },
        { "id": "ra6000",  "name": "RA-6000",          "protocol": "rotel_gen2", "type": "integrated", "verified": true },
        { "id": "ra1572",  "name": "RA-1572 (FW<2.65)","protocol": "rotel_gen1", "type": "integrated", "verified": true, "note": "gen switches with firmware 2.65 -> auto-detect" },
        { "id": "ra1592",  "name": "RA-1592 (FW<2.65)","protocol": "rotel_gen1", "type": "integrated", "verified": true, "note": "gen switches with firmware 2.65 -> auto-detect" },
        { "id": "michi_x3","name": "Michi X3/X5",      "protocol": "rotel_gen2", "type": "integrated", "verified": true },
        { "id": "michi_m8","name": "Michi M8/S5",      "protocol": "rotel_gen2", "type": "poweramp",   "verified": true, "note": "power/dimmer only" }
      ]
    },
    {
      "brand": "NAD",
      "models": [
        { "id": "m33",  "name": "M 33",  "protocol": "nad_v2", "type": "integrated", "verified": true, "note": "RS232 confirmed; BluOS -> VERIFY command subset" },
        { "id": "c399", "name": "C 399", "protocol": "nad_v2", "type": "integrated", "verified": true },
        { "id": "c389", "name": "C 389", "protocol": "nad_v2", "type": "integrated", "verified": true, "note": "VERIFY per-model protocol doc (BluOS-D optional)" },
        { "id": "c388", "name": "C 388", "protocol": "nad_v2", "type": "integrated", "verified": true, "note": "VERIFY source numbering vs C 368" },
        { "id": "c379", "name": "C 379", "protocol": "nad_v2", "type": "integrated", "verified": true },
        { "id": "c368", "name": "C 368", "protocol": "nad_v2", "type": "integrated", "verified": true },
        { "id": "c328", "name": "C 328", "protocol": "nad_v2", "type": "integrated", "verified": true, "note": "RS232 as 3.5mm jack, not DB9; adapter + pinout VERIFY" },
        { "id": "t755", "name": "T 755", "protocol": "nad_v2", "type": "avr", "verified": true },
        { "id": "t765", "name": "T 765", "protocol": "nad_v2", "type": "avr", "verified": true },
        { "id": "t775", "name": "T 775", "protocol": "nad_v2", "type": "avr", "verified": true },
        { "id": "t785", "name": "T 785", "protocol": "nad_v2", "type": "avr", "verified": true },
        { "id": "t175", "name": "T 175", "protocol": "nad_v2", "type": "avr", "verified": true },
        { "id": "t777", "name": "T 777", "protocol": "nad_v2", "type": "avr", "verified": true },
        { "id": "t787", "name": "T 787", "protocol": "nad_v2", "type": "avr", "verified": true },
        { "id": "t187", "name": "T 187", "protocol": "nad_v2", "type": "avr", "verified": true },
        { "id": "m15hd","name": "M15 HD","protocol": "nad_v2", "type": "avr", "verified": true }
      ],
      "excluded": [
        { "id": "c338", "reason": "no RS232/IR/trigger on rear panel — do not implement" }
      ],
      "modelDocs": "per-model protocol docs at nadelectronics.com -> support/software; see also NAD/ and nad-models-verified-addendum.md"
    },
    {
      "brand": "Denon",
      "models": [
        { "id": "avr_legacy", "name": "AVR-2106 .. AVR-4802 era", "protocol": "denon", "type": "avr", "verified": true, "note": "protocol v4.x docs exist per model" },
        { "id": "avr_ci",     "name": "AVR-2310CI/2312CI/3310CI/3311CI", "protocol": "denon", "type": "avr", "verified": true },
        { "id": "avr_x_2016", "name": "AVR-X1x00H .. X4x00H (with RS232)", "protocol": "denon", "type": "avr", "verified": true, "note": "lower X models lack RS232 port -> check rear panel" },
        { "id": "avr_x_2021", "name": "AVR-X3700H/X3800H/X4800H", "protocol": "denon", "type": "avr", "verified": true }
      ],
      "note": "Denon PMA stereo line generally has NO RS232 port"
    },
    {
      "brand": "Marantz",
      "models": [
        { "id": "nr_sr_modern", "name": "NR1504+ / SR5006+ (2011+)", "protocol": "denon", "type": "avr", "verified": true, "note": "Denon grammar; may also emit @-lines" },
        { "id": "sr_classic",   "name": "SR4200..SR8500, DV players", "protocol": "marantz_classic", "type": "avr", "verified": true, "note": "disable economy standby for serial power-on" },
        { "id": "model_series", "name": "MODEL 30/40n", "protocol": "denon", "type": "integrated", "verified": false, "note": "VERIFY protocol doc; PM8006 and most PM have NO RS232 (RC-5 bus only)" }
      ]
    }
  ]
}
```

---

## 4. Implementation Instructions (refactor plan for Rotel-HomeKit)

### 4.1 Goals
- Keep the HomeKit layer (`HomeSpanTV`, InputSources, TelevisionSpeaker) **unchanged** in behavior.
- Generalize `RotelCommand.h` / `RotelModels.h` into a protocol-agnostic core.
- No heap allocation in the RX/TX hot path (retain fixed char buffers).
- Model + protocol selection persisted in NVS (existing namespace `rotelcfg` → migrate to `ampcfg`, keep read-fallback).

### 4.2 Architecture changes (ordered)

**Step 1 — Introduce `ProtocolDef`.**
```cpp
enum FrameStyle : uint8_t { FS_KEYVALUE, FS_KEYVALUE_BYTECOUNT, FS_POSITIONAL };
struct ProtocolDef {
  uint32_t baud;
  char     txTerm;        // '!' or '\r'
  char     rxTerm;        // '$', '!', '\r'
  FrameStyle style;
  char     divider;       // '=' or ':' (keyvalue styles)
  const char *stripPrefix; // "Main." / "@" / nullptr
  const char *txPreamble;  // "\r" for NAD, nullptr otherwise
  uint8_t  prefixLen;      // 2 for denon positional matching
  uint32_t minCmdIntervalMs;
  uint32_t responseTimeoutMs;
  uint32_t postPowerOnDelayMs; // 1000 for denon/marantz-modern, else 0
};
```
Baud rate moves from `constexpr BAUD_RATE` into `ProtocolDef`; `Serial.begin` happens after model load from NVS.

**Step 2 — Generalize command tables.**
Replace `RotelCmdPair {gen2, gen1}` + `pick()` with a per-protocol `CommandSet`:
```cpp
struct CommandSet {
  const char *powerOn, *powerOff, *powerQuery;
  const char *volUp, *volDown, *volQuery;
  const char *muteOn, *muteOff, *muteQuery;
  const char *subscribe;        // rs232_update_on! / display_update_auto! / nullptr
  // tone/balance members nullable — nullptr = feature absent
};
```
Rotel Gen1/Gen2 become two `CommandSet` instances; `pick()` collapses into pointer selection at init/auto-detect. Denon volume set uses a formatter callback (`fmtVolume(char* out, int val)`) because `MV45` embeds the value without divider.

**Step 3 — Split the RX parser into tokenizer + semantic layer.**
- Tokenizer per `FrameStyle`:
  - `FS_KEYVALUE`: split at `divider`, strip `stripPrefix`, terminator = `rxTerm`.
  - `FS_KEYVALUE_BYTECOUNT`: existing Rotel Gen1 logic (keep as-is).
  - `FS_POSITIONAL`: match longest known prefix from a per-protocol key list (`MVMAX` before `MV`!), remainder = value.
- Semantic layer (`setStatus`) keeps mapping normalized keys (`power`, `volume`, `mute`, `source`, ...) to state + `RotelUpdateEvent` — rename to `AmpUpdateEvent`, logic unchanged.
- Unknown lines: log at debug level, never block the queue. Marantz-modern emits `@...` lines in Denon mode — must be silently skipped.

**Step 4 — TX queue adjustments.**
- Keep sequential send + response/timeout gate.
- Add `postPowerOnDelayMs` gate: after sending the protocol's `powerOn`, hold the queue for that duration (Denon requirement).
- Prepend `txPreamble` bytes when configured (NAD noise-flush CR).

**Step 5 — Model table.**
`RotelModels.h` → `AmpModels.h`: add `brand`, `protocolId`, per-model source list mapping HomeKit `Identifier` → `{setCommand, replyToken, displayName}`.
- Rotel: `{"cd!", "cd", "CD"}`
- NAD: `{"Main.Source=1", "1", "CD"}` (`VERIFY` numbering per model doc)
- Denon: `{"SICD", "CD", "CD"}` — replyToken compares against the value part after positional split.

**Step 6 — Volume semantics.**
HomeKit `VolumeSelector` is relative (up/down) — maps 1:1 to every protocol (`vol_up!`, `Main.Volume+`, `MVUP`). Keep the existing pattern: send step, then query (`volQuery`) where the protocol does not echo automatically. Denon echoes `MV<val>` on its own ⇒ skip the query for `FS_POSITIONAL`.

**Step 7 — Web portal & NVS.**
- Dashboard model dropdown: group by brand.
- NVS keys: store `brandId` + `modelId` + cached generation (Rotel only). Migration: if legacy key exists, map to Rotel brand.

**Step 8 — Naming.**
- `Characteristic::Manufacturer` from brand table instead of hardcoded.
- Keep accessory name user-configurable; remember the Siri lesson: default names must be speech-friendly ("Amplifier", not "Rotel").

### 4.3 Test procedure per new protocol
1. Bench test with USB-RS232 dongle + terminal before flashing (send `PW?` / `Main.Power?` manually, confirm response format and terminator).
2. Flash with debug logging of raw RX bytes (hex) — confirm terminator assumptions.
3. Verify: power on/off round-trip, unsolicited event on front-panel volume change, source switch + reply-token match, queue behavior after power-on (Denon 1 s gate).
4. Regression: full Rotel A14 pass (power, volume, mute, source, tone, balance, visibility checkboxes, DisplayOrder).

### 4.4 Priority order
1. **NAD** — implemented (AVRs aus `NAD/*.pdf`; C-Serie als to verify).
2. **Denon** — implemented als generischer Eintrag (to verify); deckt modern Marantz mit ab.
3. **Marantz classic** — implemented als generischer Eintrag (to verify).

**Status Firmware 1.3.0:** Multi-Protokoll-Kern (`AmpProtocols.h`, erweiterte `RotelModels.h` / `RotelCommand.h`) aktiv. Rotel-Indizes 0–8 unveraendert (NVS-kompatibel).

### 4.5 Hard rules for the agent
- Never mix protocol dialects on one connection; protocol is fixed per selected model (except Rotel AUTO detection, which stays).
- Every new model entry requires the manufacturer protocol doc as source; entries without doc get `"verified": false` and a `note`.
- Do not remove the Rotel byte-count parser — Gen1 devices depend on it.
- All new tables `const`/flash-resident; no `String` in the hot path.
