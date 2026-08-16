# Rotel RS232 Serial Control – Knowledge Base für KI-Agenten

> **Zweck dieses Dokuments:** Diese Datei ist eine konsolidierte, maschinenlesbare Referenz für einen KI-Agenten, der Rotel-Verstärker (Integrated Amplifiers, Preamps, Endstufen, Distribution Amps, Michi) per RS232 (seriell) steuert bzw. Fragen dazu beantwortet.
> **Quelle:** Offizielle Rotel-Protokolldokumente (rotel.com/en-gb/manuals-resources/rs232-protocols), ausgewertet Stand 2026. Modellabdeckung siehe Abschnitt 3.
> **Wichtig für den Agenten:** Bei Unsicherheit zum exakten Befehl eines Modells immer das modellspezifische PDF prüfen – die URLs sind in Abschnitt 3 gelistet.

---

## 1. Funktionsweise der seriellen Kommunikation (gilt für ALLE Rotel-Geräte)

### 1.1 Physische Verbindungsparameter

| Parameter | Wert |
|---|---|
| Baudrate | **115200** |
| Datenbits | 8 |
| Parität | keine (N) |
| Stoppbits | 1 |
| Flusskontrolle / Handshaking | **keine** |
| Datentyp | ASCII-String |

Kurzform: **115200 8N1, kein Flow Control.**

⚠️ Da es keine Flusskontrolle gibt, muss die Steuerungssoftware selbst Paketverlust vermeiden: Befehle sequenziell senden, auf Antwort warten bzw. kurze Pausen (~50–100 ms) zwischen Befehlen einhalten.

### 1.2 Befehlsformat (Senderichtung → Gerät)

- Befehle sind reine ASCII-Strings in Kleinschreibung.
- **Jeder Befehl endet mit `!` als Terminator.** Beispiel: `power_on!`
- **Keine Leerzeichen, kein Carriage Return (`\r`), kein Line Feed (`\n`)** – nur das `!`.
- Statusabfragen enden je nach Protokollgeneration auf `!` (Gen 1: `get_current_power!`) oder `?` (Gen 2: `power?`).

### 1.3 Antwortformat (Gerät → Steuerung)

Format immer `schlüssel=wert` + Terminator. **Der Terminator unterscheidet die beiden Protokollgenerationen:**

| Generation | Antwort-Terminator | Beispiel |
|---|---|---|
| **Gen 1 (Legacy)** | `!` | `power=on!` |
| **Gen 2 (aktuell)** | `$` | `power=on$` |

Sonderfall Gen 1 – Texte variabler Länge (Display, Metadaten, Produktname): kein Terminator, stattdessen **Byte-Count-Präfix**:
```
display1=20,Sample Text Here 20c
product_type=05,RA-12
```
Der Zähler umfasst nur den Text (nicht Länge und Komma). Der Parser muss beide Fälle behandeln: Terminator-basiert UND längenbasiert. Ein `!` kann innerhalb solcher Textdaten vorkommen – niemals blind auf `!` splitten.

### 1.4 Push- vs. Poll-Modus (Feedback)

Alle Geräte kennen zwei Feedback-Modi:

| Modus | Gen-1-Befehl | Gen-2-Befehl | Verhalten |
|---|---|---|---|
| Auto (Push) | `display_update_auto!` | `rs232_update_on!` | Gerät sendet Statusänderungen unaufgefordert (Volume, Power, Source …) |
| Manuell (Poll) | `display_update_manual!` | `rs232_update_off!` | Gerät antwortet nur auf explizite Abfragen |

Für Agenten-/Automationszwecke ist **Auto-Modus** empfohlen, kombiniert mit einem ereignisgesteuerten Parser.

### 1.5 Sonderzeichen im Display-Feedback (Gen 1)

Displayinhalte können Symbole als 2–3-Byte-Hex-Sequenzen mit Präfix `EE 82 xx` enthalten (z. B. `EE 82 82` = ▶, `EE 82 83` = ■). Der Parser sollte Bytes ≥ 0x80 tolerieren und nicht als UTF-8 erzwingen.

---

## 2. Die zwei Protokollgenerationen im Überblick

| Merkmal | **Gen 1 (Legacy)** | **Gen 2 (aktuell)** |
|---|---|---|
| Befehls-Terminator | `!` | `!` (unverändert) |
| Antwort-Terminator | `!` oder Byte-Count | `$` |
| Statusabfrage | `get_current_power!` | `power?` |
| Volume rel. | `volume_up!` / `volume_down!` | `vol_up!` / `vol_dwn!` |
| Volume absolut | `volume_45!` | `vol_45!` |
| Klangregelung an/aus | `tone_on!` / `tone_off!` | `bypass_off!` / `bypass_on!` ⚠️ **invertierte Logik!** |
| Modellabfrage | `get_product_type!` | `model?` |
| Typische Modelle | RA-11/12, RA-1570, RC-1570, RA-1572 (< FW 2.65), RA-1592 (frühe FW), RKB-Serie | A11/A12/A14 (+MKII), RA-1572 ≥ FW 2.65 + MKII, RA-1592MKII, RA-6000, Michi X3/X5/P5, M8/S5 |

⚠️ **Kritische Falle:** Bei RA-1572 (und analog RA-1592) hängt das Protokoll von der **Firmware-Version** ab (Umstellung bei V2.65). Ein Agent sollte zur Laufzeit erkennen, welche Generation antwortet: `power?` senden → Antwort endet auf `$` → Gen 2; keine/fehlerhafte Antwort → Gen 1 mit `get_current_power!` probieren.

⚠️ **Semantik-Falle Klangregelung:** Gen 1 `tone_on!` (Klangregelung EIN) entspricht Gen 2 `bypass_off!` (Bypass AUS = Klangregelung EIN).

### Sonderprotokoll: RKB-Distributionsverstärker (RKB-850/8100/D850/D8100, C8/C8+ ähnlich)

Gen-1-Basis plus **Kanalpräfixe** `0A:` – `0D:` für kanalspezifische Befehle:
```
0C:get_channel_status!   →  0C:ch_power=on!
get_channel_status!      →  0A:ch_power=on!\r0B:ch_power=on!\r0C:...\r0D:...
```
- Ohne Präfix wirkt der Befehl auf alle Kanäle; Mehrkanal-Antworten sind durch Carriage Return getrennt.
- RS232-Volume funktioniert nur, wenn alle Frontpanel-Trims auf Minimum stehen.
- Entweder `power_on/off!` ODER `channel_on/off!` verwenden, nicht mischen.
- Im Signal-Sense-Modus sind Power-Befehle deaktiviert (`power=SignalSenseMode!`).
- Zusätzliche Telemetrie: `get_temperature!`, `get_fan_status!`, `get_amp_status!` (normal/protection).

### Sonderfall: Reine Endstufen (Michi M8/S5)

Minimaler Befehlssatz (Gen 2): nur Power, Dimmer, `rs232_update_on/off!`, `power?`, `dimmer?`, `version?`, `model?`. Keine Volume-/Source-Befehle.

---

## 3. Modell-zu-Protokoll-Zuordnung (Verstärker)

| Modell(e) | Generation | Protokoll-PDF |
|---|---|---|
| RA-11, RA-12 (V02) | Gen 1 | rotel.com/sites/default/files/product/rs232/RA12%20Protocol.pdf |
| RA-1570 | Gen 1 | …/RA1570%20Protocol.pdf |
| RA-1572 (FW < 2.65) | Gen 1 | …/RA1572MKII_Protocol.pdf (Sections 3–4) |
| RA-1572 (FW ≥ 2.65), RA-1572MKII | Gen 2 | …/RA1572MKII_Protocol.pdf (Sections 1–2) |
| RA-1592 / RA-1592MKII | Gen 1→2 (analog 1572) | …/RA1592%20Protocol.pdf, …/RA1592MKII_Protocol.pdf |
| A11 / A11MKII | Gen 2 | …/A11-A11MKII%20Protocol.pdf |
| A12 / A12MKII / A14 / A14MKII | Gen 2 | …/A12-A12MKII-A14-A14MKII%20Protocol.pdf |
| RA-6000 | Gen 2 | …/RA6000%20Protocol.pdf |
| Michi X3 / X5 (+ Series 2) | Gen 2 | …/X3-X3S2-X5-X5S2%20Protocol.pdf |
| Michi P5 / P5 S2 (Preamp) | Gen 2 | …/P5-P5S2%20Protocol.pdf |
| Michi M8 / S5 (Endstufen) | Gen 2 (minimal) | …/M8-S5%20Protocol.pdf |
| RKB-850/8100/D850/D8100 | Gen 1 + Kanalpräfix | …/RKB850%20Protocol.pdf |
| C8 / C8+ (Multiroom) | eigenes Zonen-Protokoll | …/C8%26C8%2B%20Protocol.pdf |
| X430 | Gen 2 | …/X430_Protocol.pdf |

---

## 4. Standardisierter Kommandokatalog (JSON)

Abstraktionsschicht für den Agenten: Jede Funktion hat eine kanonische `action`-ID, die pro Protokollgeneration auf den konkreten ASCII-Befehl gemappt wird. Platzhalter: `{n}` = Zahlenwert (zweistellig, ggf. mit führender Null), `{sign}` = `+`/`-`.

```json
{
  "meta": {
    "vendor": "Rotel",
    "transport": {
      "serial": { "baud": 115200, "data_bits": 8, "parity": "none", "stop_bits": 1, "flow_control": "none" }
    },
    "command_terminator": "!",
    "response_terminator": { "gen1": "!", "gen2": "$" },
    "response_format": "key=value",
    "no_cr_lf": true,
    "generation_detection": "Send 'power?'. If response ends with '$' → gen2. If no valid response, send 'get_current_power!' → gen1."
  },
  "actions": [
    { "action": "power.on",        "gen1": "power_on!",        "gen2": "power_on!",     "response": "power=on",              "params": null },
    { "action": "power.off",       "gen1": "power_off!",       "gen2": "power_off!",    "response": "power=standby",         "params": null },
    { "action": "power.toggle",    "gen1": "power_toggle!",    "gen2": "power_toggle!", "response": "power=on|standby",      "params": null },
    { "action": "power.get",       "gen1": "get_current_power!", "gen2": "power?",      "response": "power=on|standby",      "params": null },

    { "action": "volume.up",       "gen1": "volume_up!",       "gen2": "vol_up!",       "response": "volume={n}",            "params": null },
    { "action": "volume.down",     "gen1": "volume_down!",     "gen2": "vol_dwn!",      "response": "volume={n}",            "params": null },
    { "action": "volume.set",      "gen1": "volume_{n}!",      "gen2": "vol_{n}!",      "response": "volume={n}",            "params": { "n": "01-96, zweistellig" } },
    { "action": "volume.min",      "gen1": "volume_min!",      "gen2": "vol_min!",      "response": "volume=min|00",         "params": null },
    { "action": "volume.max",      "gen1": "volume_max!",      "gen2": null,            "response": "volume=max",            "note": "In Gen 2 entfernt" },
    { "action": "volume.get",      "gen1": "get_volume!",      "gen2": "volume?",       "response": "volume={n}",            "params": null },

    { "action": "mute.toggle",     "gen1": "mute!",            "gen2": "mute!",         "response": "mute=on|off",           "params": null },
    { "action": "mute.on",         "gen1": "mute_on!",         "gen2": "mute_on!",      "response": "mute=on",               "params": null },
    { "action": "mute.off",        "gen1": "mute_off!",        "gen2": "mute_off!",     "response": "mute=off",              "params": null },
    { "action": "mute.get",        "gen1": "get_mute_status!", "gen2": "mute?",         "response": "mute=on|off",           "params": null },

    { "action": "source.select",   "gen1": "{source}!",        "gen2": "{source}!",     "response": "source={source_id}",
      "params": { "source": "cd|coax1|coax2|coax3|opt1|opt2|opt3|aux|aux1|aux2|tuner|phono|usb|bluetooth|bal_xlr|pcusb|rcd" },
      "note": "Verfügbare Quellen modellabhängig. Gen1 RA-1572: 'pc_usb!', Gen2: 'pcusb!'. Antwortwert kann abweichen (pcusb! → source=pc_usb$)." },
    { "action": "source.get",      "gen1": "get_current_source!", "gen2": "source?",    "response": "source={source_id}",    "params": null },

    { "action": "transport.play",  "gen1": "play!",  "gen2": "play!",  "response": null, "params": null },
    { "action": "transport.stop",  "gen1": "stop!",  "gen2": "stop!",  "response": null, "params": null },
    { "action": "transport.pause", "gen1": "pause!", "gen2": "pause!", "response": null, "params": null },
    { "action": "transport.next",  "gen1": "track_fwd!",  "gen2": "trkf!", "response": null, "params": null },
    { "action": "transport.prev",  "gen1": "track_back!", "gen2": "trkb!", "response": null, "params": null },

    { "action": "tone.enable",     "gen1": "tone_on!",   "gen2": "bypass_off!", "response": "tone=on | bypass=off",
      "note": "ACHTUNG: invertierte Logik zwischen Generationen" },
    { "action": "tone.disable",    "gen1": "tone_off!",  "gen2": "bypass_on!",  "response": "tone=off | bypass=on" },
    { "action": "tone.get",        "gen1": "get_tone!",  "gen2": "bypass?",     "response": "tone=on|off | bypass=on|off" },

    { "action": "bass.up",         "gen1": "bass_up!",    "gen2": "bass_up!",    "response": "bass=000|{sign}{n}" },
    { "action": "bass.down",       "gen1": "bass_down!",  "gen2": "bass_down!",  "response": "bass=000|{sign}{n}" },
    { "action": "bass.set",        "gen1": "bass_{sign}{n}! oder bass_000!", "gen2": "bass_{sign}{n}! oder bass_000!",
      "response": "bass={sign}{n}|000", "params": { "n": "01-10" },
      "note": "Ältere Docs listen nur -10/000/+10 als Direktwerte; RA-6000 dokumentiert beliebige ±nn" },
    { "action": "bass.get",        "gen1": "get_bass!",   "gen2": "bass?",       "response": "bass=+01..+10|-01..-10|000" },

    { "action": "treble.up",       "gen1": "treble_up!",   "gen2": "treble_up!",   "response": "treble=000|{sign}{n}" },
    { "action": "treble.down",     "gen1": "treble_down!", "gen2": "treble_down!", "response": "treble=000|{sign}{n}" },
    { "action": "treble.set",      "gen1": "treble_{sign}{n}!", "gen2": "treble_{sign}{n}!", "response": "treble={sign}{n}|000", "params": { "n": "01-10" } },
    { "action": "treble.get",      "gen1": "get_treble!",  "gen2": "treble?",      "response": "treble=+01..+10|-01..-10|000" },

    { "action": "balance.left",    "gen1": "balance_left!",  "gen2": "balance_l!",  "response": "balance=000|L{n}|R{n}" },
    { "action": "balance.right",   "gen1": "balance_right!", "gen2": "balance_r!",  "response": "balance=000|L{n}|R{n}" },
    { "action": "balance.set",     "gen1": "balance_L15!/balance_000!/balance_R15!", "gen2": "balance_l{n}!/balance_000!/balance_r{n}!",
      "response": "balance=l{n}|r{n}|000", "params": { "n": "01-15 (Michi X: 01-10)" } },
    { "action": "balance.get",     "gen1": "get_balance!", "gen2": "balance?",     "response": "balance=L01-15|R01-15|000" },

    { "action": "speaker.a.toggle", "gen1": "speaker_a!", "gen2": "speaker_a!",    "response": "speaker=a|a_b|off", "note": "Nur Modelle mit Speaker-A/B-Ausgängen" },
    { "action": "speaker.b.toggle", "gen1": "speaker_b!", "gen2": "speaker_b!",    "response": "speaker=b|a_b|off" },
    { "action": "speaker.a.on",     "gen1": null,         "gen2": "speaker_a_on!", "response": "speaker=a|a_b" },
    { "action": "speaker.a.off",    "gen1": null,         "gen2": "speaker_a_off!","response": "speaker=b|off" },
    { "action": "speaker.b.on",     "gen1": null,         "gen2": "speaker_b_on!", "response": "speaker=b|a_b" },
    { "action": "speaker.b.off",    "gen1": null,         "gen2": "speaker_b_off!","response": "speaker=a|off" },
    { "action": "speaker.get",      "gen1": "get_current_speaker!", "gen2": "speaker?", "response": "speaker=a|b|a_b|off" },

    { "action": "dimmer.toggle",   "gen1": "dimmer!",     "gen2": "dimmer!",       "response": "dimmer={n}" },
    { "action": "dimmer.set",      "gen1": "dimmer_{n}!", "gen2": "dimmer_{n}!",   "response": "dimmer={n}", "params": { "n": "0 (hellste) - 6 (dunkelste); Michi X/M8/S5: 0-4" } },
    { "action": "dimmer.get",      "gen1": "get_current_dimmer!", "gen2": "dimmer?", "response": "dimmer=0-6" },

    { "action": "freq.get",        "gen1": "get_current_freq!", "gen2": "freq?",
      "response": "freq=off|none|32|44.1|48|88.2|96|176.4|192|384", "note": "Nur bei aktiver Digitalquelle; 384 nur neuere Modelle" },

    { "action": "feedback.auto",   "gen1": "display_update_auto!",   "gen2": "rs232_update_on!",  "response": "display_update=auto | update_mode=auto" },
    { "action": "feedback.manual", "gen1": "display_update_manual!", "gen2": "rs232_update_off!", "response": "display_update=manual | update_mode=manual" },

    { "action": "info.model",      "gen1": "get_product_type!",    "gen2": "model?",   "response": "product_type={len},{text} | model={text}" },
    { "action": "info.version",    "gen1": "get_product_version!", "gen2": "version?", "response": "product_version={len},{text} | version={x.xx}" },

    { "action": "display.get",       "gen1": "get_display!",  "gen2": null, "response": "display={len3},{text}",  "note": "Byte-Count-Format, kein Terminator; in Gen 2 entfernt" },
    { "action": "display.get_line1", "gen1": "get_display1!", "gen2": null, "response": "display1={len2},{text}" },
    { "action": "display.get_line2", "gen1": "get_display2!", "gen2": null, "response": "display2={len2},{text}" },

    { "action": "pcusb.class.set", "gen1": "pcusb_class_1!/pcusb_class_2!", "gen2": "pcusb_class_1!/pcusb_class_2!", "response": "pcusb_class=1|2" },
    { "action": "pcusb.class.get", "gen1": "get_pcusb_class!", "gen2": "pcusb?", "response": "pcusb_class=1|2" },
    { "action": "factory_reset",   "gen1": "factory_default_on!", "gen2": "factory_default_on!", "response": null, "note": "Vorsicht: setzt alle Nutzereinstellungen zurück" }
  ],
  "rkb_extension": {
    "applies_to": ["RKB-850", "RKB-8100", "RKB-D850", "RKB-D8100"],
    "channel_prefix": { "syntax": "{0A|0B|0C|0D}:{command}", "no_prefix_means": "alle Kanäle", "multi_response_separator": "\\r" },
    "actions": [
      { "action": "channel.on",       "cmd": "channel_on!",        "response": "0A:ch_power=on" },
      { "action": "channel.off",      "cmd": "channel_off!",       "response": "0A:ch_power=off" },
      { "action": "channel.get",      "cmd": "get_channel_status!","response": "0A:ch_power=on|off|amp_on|amp_off" },
      { "action": "volume.lr.set",    "cmd": "volume_l_{n}! / volume_r_{n}!", "response": "0A:volume_l_={n} / 0A:volume_r_={n}", "params": { "n": "1-96" } },
      { "action": "input.mode",       "cmd": "input_sel_auto!/input_sel_digital!/input_sel_analog!", "response": "input_sel_mode=auto|digital|analog", "note": "Nur D-Modelle, FW >= 2.45" },
      { "action": "telemetry.temp",   "cmd": "get_temperature!",   "response": "temperature=32,32,34[,34]", "note": "AB, CD, Netzteil(e), Celsius" },
      { "action": "telemetry.fan",    "cmd": "get_fan_status!",    "response": "fan=normal|high" },
      { "action": "telemetry.amp",    "cmd": "get_amp_status!",    "response": "amp=normal|protection" },
      { "action": "telemetry.trim",   "cmd": "get_amp_trim!",      "response": "0A:amp_trim={n}|min|max" }
    ],
    "constraints": [
      "RS232-Volume nur wirksam, wenn alle 4 Frontpanel-Trims auf Minimum stehen",
      "power_on/off und channel_on/off nicht mischen",
      "Signal-Sense-Modus deaktiviert Power-/Channel-Befehle"
    ]
  }
}
```

---

## 5. Agenten-Regeln (Handlungsanweisungen)

1. **Generation bestimmen, bevor gesteuert wird:** `power?` senden. Antwort auf `$` → Gen 2. Keine Antwort binnen ~500 ms → `get_current_power!` senden → Antwort auf `!` → Gen 1.
2. **Immer `!` als Befehls-Terminator, niemals `\r\n` anhängen.** Das gilt für beide Generationen.
3. **Parser-Regeln:** Auf `$` (Gen 2) bzw. `!` (Gen 1) als Frame-Ende splitten; bei Gen-1-Antworten mit `={len},`-Muster stattdessen den Byte-Count auswerten. `\r` als Zeilentrenner bei RKB-Mehrkanal-Antworten erwarten.
4. **Volume-Skala ist geräteintern 0/1–96** (nicht dB). `volume_max`-Setting des Nutzers kann die Obergrenze begrenzen (`get_volume_max!`, nur Gen 1 abfragbar).
5. **Klangregelung:** Nutzerintention „Klangregelung an" heißt Gen 1 `tone_on!`, Gen 2 `bypass_off!` – nie 1:1 übersetzen.
6. **Quellenliste ist modellabhängig.** Vor Source-Umschaltung im Zweifel `source?`-Antwortwerte bzw. das Modell-PDF konsultieren; unbekannte Quellen-Befehle werden vom Gerät ignoriert.
7. **Firmware-Sensitivität:** RA-1572/RA-1592 wechseln bei FW 2.65 die Generation; RKB-Funktionen hängen an FW 1.31/2.43/2.45.
8. **Keine Antwort ist normal** bei reinen Tastenbefehlen (play!, menu!, Zifferntasten) – nicht als Fehler werten.
9. **Nach `power_on!` kurz warten** (Gerät bootet aus Standby), bevor weitere Befehle gesendet werden.
10. **Endstufen (M8/S5) können nur Power/Dimmer** – Volume-/Source-Anfragen des Nutzers an den vorgeschalteten Preamp (P5) richten.
