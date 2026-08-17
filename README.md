# Homekit Amplify (ESP32-C3)

Control your hi-fi amplifier natively from Apple Home – no bridge, no cloud.

This firmware turns an ESP32-C3 RS232 adapter into a genuine HomeKit accessory
for RS232-controlled integrated amplifiers and AV receivers. The amplifier
appears in the Home app as a TV accessory with power, input selection and
volume control, including full support for the iOS Remote widget in Control
Center. A built-in web dashboard handles Wi-Fi onboarding, amplifier model
selection, firmware updates and HomeKit pairing – no recompiling required for
end users. A mobile-friendly **web remote** is included as well, so the
amplifier can be controlled from any browser even without HomeKit.

**Rotel**, **NAD**, **Denon** and **Marantz** protocols are supported; the
brand and model are picked at runtime from the dashboard.

![Platform](https://img.shields.io/badge/platform-ESP32--C3-blue)
![Framework](https://img.shields.io/badge/framework-Arduino%20%2B%20HomeSpan-green)
![License](https://img.shields.io/badge/license-MIT-lightgrey)

---

## Features

- **Native HomeKit accessory** (via [HomeSpan](https://github.com/HomeSpan/HomeSpan)) –
  pairs directly with the Home app, works with Siri, automations and scenes
- **Multi-brand RS232 core**: one driver, six protocol dialects – Rotel Gen 1,
  Rotel Gen 2, NAD v2, Denon, modern Marantz and classic Marantz. Baud rate,
  terminators, frame layout and command pacing come from a protocol table in
  flash, so adding a brand does not touch the HomeKit or web layer
- **TV accessory with input sources**: power on/off, source selection,
  sources can be renamed and hidden per-user in the Home app (persisted in NVS)
- **iOS Remote widget support**: volume, mute, and a modal scheme to adjust
  bass, treble and balance from the Control Center remote
- **Web remote** at `/remote`: power, source wheel, volume, mute, bypass,
  treble, bass and balance from any browser – no HomeKit pairing required
- **Automatic protocol detection**: Rotel models that changed their RS232
  dialect across firmware revisions (RA-1572/RA-1592) are probed at boot and
  the detected generation is cached in NVS
- **Web dashboard** (port 80): status, model selection, HomeKit setup code,
  device address, OTA firmware update, Wi-Fi settings and factory reset
- **Configurable device address**: default mDNS hostname is `amplify.local`
  (base name `Amplify`); change it (e.g. `martin-remote.local`) and the
  setup Wi-Fi name from the dashboard – useful when running more than one
  adapter in the same network
- **Wi-Fi onboarding** with a captive portal (WiFiManagerLite) – no hardcoded
  credentials
- **OTA updates** over HTTP with a one-click installer (GitFirmwareUpdate)
- **Built for 24/7 operation**: no heap allocation in the hot path, fixed
  buffers everywhere, response-paced TX queue, millis()-rollover safe
- **Host test suite**: parser, TX framing and queue logic are covered by 555
  assertions that run on the development machine without hardware

## Supported amplifiers

Models marked *to verify* are implemented from public protocol documentation
but have not yet been confirmed against real hardware. They are labelled as
such in the dashboard dropdown.

### Rotel

| Model | Protocol | Device type |
|---|---|---|
| A11 (+MKII) | Gen 2 | Integrated amp |
| A12 / A14 (+MKII) *(default)* | Gen 2 | Integrated amp |
| RA-11 / RA-12 | Gen 1 | Integrated amp |
| RA-1570 | Gen 1 | Integrated amp |
| RA-1572 (+MKII) | auto-detected | Integrated amp |
| RA-1592 (+MKII) | auto-detected | Integrated amp |
| RA-6000 | Gen 2 | Integrated amp |
| Michi X3 / X5 | Gen 2 | Integrated amp |
| Michi M8 / S5 | Gen 2 | Power amp (power switch only) |

*Gen 1* devices answer with `key=value!` frames and use `get_*!` queries,
*Gen 2* devices (A12/A14 era and newer) answer with `key=value$` and use `*?`
queries. Both dialects sit behind a single command table, including quirks
like the inverted tone-control logic (`tone_on!` vs. `bypass_off!`) and the
Gen 1 byte-count response format (`product_version={len},{text}`).

### NAD

| Model | Protocol | Volume range |
|---|---|---|
| T 755, T 765, T 775, T 785, T 175, T 777, T 787, T 187, M15 HD | NAD v2 | −99 … +19 dB |
| M 33, C 399, C 389, C 388, C 379, C 368, C 328 | NAD v2 | −80 … 0 dB |

NAD speaks `Main.Volume=-30` style frames terminated by CR/LF, with a leading
CR sent before every command to flush line noise. Bass and treble only accept
even values, which the firmware enforces (both on the wire and in the web UI).
NAD has no balance control, so the Remote widget's tone cycle skips it
automatically.

> **C 328:** its RS232 port is a 3.5 mm jack, not a DB9 – a jack-to-DB9 cable
> is required. The **C 338** has no RS232 port at all and is not supported.

### Denon / Marantz *(to verify)*

| Entry | Protocol | Notes |
|---|---|---|
| Denon AVR | Denon | Positional frames (`MV45`, `PWON`), 0 … 98 |
| Marantz NR/SR modern | Denon | Same grammar as Denon |
| Marantz SR classic | Marantz classic | `@CMD:` prefixed frames, 0 … 99 |

Denon requires a 1 s pause after power-on before it accepts further commands;
the TX queue handles that delay on its own.

Other models speaking one of the documented dialects will most likely work
too – pick the family with the closest input list and hide unused inputs in
the Home app.

## Hardware

The firmware targets the **LEOTRO ESP32-C3 RS232 adapter** (Rev 1.1):
an ESP32-C3-MINI-1 module driving an SP3232 level shifter behind a male
DB9 connector, powered over USB-C.

```
ESP32-C3 UART (3.3 V TTL) → SP3232 (±RS232 level shifter) → ESD protection → DB9
```

| Signal | GPIO |
|---|---|
| UART TX (→ RS232) | GPIO10 |
| UART RX (← RS232) | GPIO4 |
| BOOT button (HomeSpan control button) | GPIO9 |
| Flashing / logs | USB-C (USB-Serial-JTAG) |

- Serial parameters are **per protocol**, applied when the model is selected:
  115200 baud for Rotel, 115200 for NAD, 9600 for Denon/Marantz – always 8N1
  without flow control
- The board is wired as **DTE** – connect it to the amplifier with a
  **straight-through** RS232 cable
- Any ESP32-C3 board with an RS232 transceiver on the pins above will work

## How it works

### Architecture

```
┌────────────────────────────────────────────────────────────┐
│ Homekit_Amplify.ino                                        │
│   HomeKit services (Television, InputSource, TVSpeaker)    │
│   Event-driven sync: amplifier state → HomeKit chars       │
├──────────────────────┬─────────────────────────────────────┤
│ AmpCommand.h         │ WebPortal.h                         │
│   RS232 driver:      │   WiFiManagerLite (captive portal)  │
│   TX queue, parser,  │   AsyncWebServer (port 80):         │
│   dialect dispatch,  │   dashboard, web remote, REST API   │
│   auto-detection     │   GitFirmwareUpdate (OTA)           │
├──────────────────────┼─────────────────────────────────────┤
│ AmpModels.h          │ AmpProtocols.h                      │
│   Model table in     │   Protocol profiles (baud, framing, │
│   flash: brand,      │   timing) and generic command sets  │
│   inputs, limits +   │   per dialect                       │
│   NVS helpers        │                                     │
└──────────────────────┴─────────────────────────────────────┘
```

- **HomeSpan** owns HomeKit (HAP server on port **8080**, mDNS hostname
  `amplify.local` by default, base name `Amplify` – renamable from the
  dashboard).
- **WiFiManagerLite** owns the Wi-Fi connection and the captive portal;
  HomeSpan picks up connectivity passively through network events.
- The dashboard and the WML portal share one **AsyncWebServer** on port 80.
  Handlers never block: they only set flags, all real work (OTA download,
  SRP code calculation, NVS writes, reboots) runs in the main loop.

### RS232 link layer

RS232 amplifiers have **no flow control** and Rotel's documentation explicitly
warns that command bursts can crash the device CPU. The driver therefore uses:

- a **TX ring buffer** (16 entries, statically allocated) – commands are sent
  strictly sequentially,
- **response-aware pacing** – the next command is only sent after the reply
  frame to the previous one arrived (or after the protocol's timeout), with a
  protocol-specific minimum spacing,
- an **RX state machine with fixed buffers** – no `String`, no heap in the
  receive path. Frames are parsed in one of three styles (`key=value`,
  Rotel's Gen 1 byte-count form, or Denon's positional form) and dispatched
  into an event callback that updates the HomeKit characteristics.

On Rotel a subscribe command (`rs232_update_on!` on Gen 2,
`display_update_auto!` on Gen 1) puts the amplifier in push mode. NAD, Denon
and Marantz push state changes unsolicited and need no subscription. Either
way, front-panel and IR changes are reflected in the Home app within a second.

### Protocol generation auto-detection (Rotel only)

For RA-1572/RA-1592 the RS232 dialect depends on the installed amplifier
firmware. At boot the driver runs a non-blocking state machine:

1. Send the Gen 2 power query (`power?`) and wait up to 1 s for a `$` frame.
2. On timeout, send a lone `!` (flushes the partially parsed Gen 2 probe from
   the amplifier's command buffer), then the Gen 1 query
   (`get_current_power!`) and wait for a `!` frame.
3. If both stay silent (amplifier unplugged or powered off), retry every 15 s.

The detected generation is cached in NVS, so detection runs only once per
installation. Changing the model in the dashboard clears the cache.

### HomeKit mapping

| Home app / Remote widget | Amplifier |
|---|---|
| Power tile / power button | power on / off |
| Input picker | source commands (`cd!`, `Main.Source=3`, `SICD`, …) |
| Volume rocker (side buttons) | volume up / down |
| Mute button | mute on / off |
| **SELECT** | tone control off → enable it; otherwise cycle bass → treble → balance |
| **LEFT / RIGHT** | decrease / increase the selected parameter |
| **BACK** | leave tone mode (re-enable bypass) |
| UP / DOWN | volume up / down |

The tone cycle adapts to the model: devices without balance (all NAD models)
alternate between bass and treble only. On Rotel the currently selected tone
parameter is flashed on the amplifier display using a ±1 "blink trick" that
never changes the net value.

Renames, visibility checkboxes and the display order of the input sources are
stored in NVS and survive reboots. For power amps (Michi M8/S5) the accessory
is reduced to a power switch – no inputs, no volume.

### Web dashboard

Reachable at `http://amplify.local` (or the device IP) once Wi-Fi is set up:

- **Status**: firmware version, selected brand and model, detected protocol,
  amplifier state, HomeKit setup code, free heap
- **Open remote**: link to the built-in web remote (see below)
- **Amplifier model**: dropdown grouped by brand, with *to verify* entries
  marked; applying a change stores the selection in NVS, clears the stored
  characteristic values (the input list changes) and reboots
- **HomeKit code**: set a custom 8-digit pairing code (trivial codes are
  rejected, matching HomeSpan's own rules)
- **Device address**: rename the mDNS hostname (default base name `Amplify`
  → `amplify.local`; e.g. `martin-remote.local`); the same name is used for
  the setup access point. Stored in NVS, applied after an automatic restart
- **Firmware update**: check + one-click install (HTTP,
  `http://studioymr.com/amplify/firmware/latest.json`)
- **Wi-Fi settings** and **factory reset** (clears Wi-Fi, pairing and all
  preferences)

### Web remote

`http://amplify.local/remote` is a self-contained, mobile-first remote control
that works without any HomeKit pairing – useful for Android users or guests:

- **Power button**, **source wheel** (native CSS scroll-snap), **volume** with
  mute toggle, and **treble / bass / balance** sliders with a bypass toggle
- The volume scale and the bass/treble step size follow the selected
  protocol, so a NAD shows −80 … 0 dB in steps of 2 where a Rotel shows 0 … 96
- Controls that don't apply are dimmed (device in standby, mute active,
  bypass active) or hidden entirely (tone card on models without tone
  control, balance on models without balance, everything except power on
  power amps)
- The UI updates optimistically on tap and polls the device state every
  1.5 s to stay in sync with the front panel and IR remote
- **Click coalescing**: rapid +/− taps are debounced client-side and sent as
  a single absolute command (`vol_42!`, `bass_+04!`) – command bursts are the
  documented way to reset an amplifier CPU
- HTTP handlers run in the `async_tcp` task and only push actions into a
  small lock-free ring; the main loop is the sole producer of the RS232 TX
  queue

## Getting started

### Requirements

- Arduino IDE 2.x (or `arduino-cli`) with the **esp32** core (tested with 3.3.x)
- Libraries:
  - [HomeSpan](https://github.com/HomeSpan/HomeSpan) (tested with 2.1.8)
  - ESP Async WebServer + Async TCP
  - WiFiManagerLite
  - GitFirmwareUpdate (+ ArduinoJson)

### Build settings

| Option | Value |
|---|---|
| Board | ESP32C3 Dev Module |
| Partition scheme | **Minimal SPIFFS (1.9 MB APP with OTA)** |
| USB CDC on boot | Enabled |

The sketch ships a `build_opt.h` that disables the MAC suffix in the
setup-AP name (`-DWML_AP_APPEND_MAC=0`).

```bash
arduino-cli compile --fqbn esp32:esp32:esp32c3:PartitionScheme=min_spiffs,CDCOnBoot=cdc .
```

### First-time setup

1. Flash the firmware and power the board from USB-C.
2. Join the Wi-Fi access point **`Amplify`** and follow the captive portal to
   enter your Wi-Fi credentials.
3. Open `http://amplify.local` and select your amplifier model
   (default: Rotel A12/A14).
4. Connect the DB9 port to the amplifier with a straight-through cable.
5. In the Home app: *Add Accessory* → *More options* → select
   **Homekit Amplify** and pair with the default code **466-37-726**
   (changeable in the dashboard).
6. Optional: skip HomeKit entirely and just use the web remote at
   `http://amplify.local/remote`.

### Notes

- HomeKit's HAP server runs on port 8080; port 80 belongs to the dashboard
  and the captive portal. This is transparent to the Home app (mDNS).
- Running two adapters in one network? Rename one via the dashboard's
  *Device address* card so the `.local` hostnames stay unambiguous
  (the setup access point follows the same name).
- The BOOT button (GPIO9) acts as HomeSpan's control button
  (e.g. long-press actions for unpairing); serial CLI is available over USB.

## Tests

The RS232 layer is testable without hardware. `test/stub/` provides minimal
replacements for `Arduino.h`, `HardwareSerial.h` and `Preferences.h`, so
`AmpCommand.h` compiles and runs on the host:

```bash
./test/run.sh
```

The suite covers frame parsing and TX framing for every dialect, the model
table's invariants (volume ranges, source counts, command length limits),
tone-step snapping, queue overflow, recovery from junk frames and
millis() rollover.

The stubs deliberately live in `test/`, not in the sketch root – the Arduino
build only compiles the sketch directory and `src/`, so they can never shadow
the real Arduino headers.

## Project layout

| File | Purpose |
|---|---|
| `Homekit_Amplify.ino` | HomeKit services, remote-key logic, event sync |
| `AmpCommand.h` | RS232 driver: TX queue, parser, dialect dispatch, auto-detection |
| `AmpModels.h` | Model table (brand, protocol, inputs, limits) + NVS helpers |
| `AmpProtocols.h` | Protocol profiles and generic command sets per dialect |
| `WebPortal.h` | Wi-Fi onboarding, dashboard, web remote, REST API, OTA, factory reset |
| `firmware/latest.json` | OTA manifest (`version` + `.bin` URL) for the update server |
| `build_opt.h` | Compile-time flags for bundled libraries |
| `amp-rs232-protocols.md` | Cross-brand protocol overview and implementation status |
| `Rotel/rs232-serial.md` | Rotel RS232 reference (Gen 1 + Gen 2) |
| `NAD/` | NAD protocol specs and the verified-model addendum |
| `test/` | Host test suite and Arduino stubs |

## Stability by design

The device is meant to run unattended around the clock:

- No `String`/heap allocation in recurring code paths – fixed `char` buffers
  with `snprintf`/`strlcpy` throughout
- All tables (`const`) live in flash; NVS is only touched at boot and on
  explicit user actions
- Non-blocking everywhere: no `delay()` in the loop, all waits are
  millis()-based and rollover-safe
- Oversized UART RX buffer (1 KB) bridges the blocking phases of HomeKit's
  pairing cryptography
- Automatic re-sync after Wi-Fi reconnects and after the amplifier wakes from
  standby (many models don't answer queries while in standby)

## Credits

- [HomeSpan](https://github.com/HomeSpan/HomeSpan) by Gregg E. Berman – the
  HomeKit implementation this project is built on (MIT license)
- Rotel and NAD for publicly documenting their RS232 protocols

## License

MIT – see the license header in the sketch.
