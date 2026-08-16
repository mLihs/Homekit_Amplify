# AGENTS.md – LEOTRO ESP32-C3 RS232 Adapter (Rev 1.1)

> **Zweck:** Diese Datei beschreibt das Hardwareboard für KI-Agenten (Claude Code, Copilot etc.), die Firmware (ESPHome, Arduino, ESP-IDF) für dieses Board schreiben, ändern oder debuggen. Alle Pin- und Schaltungsangaben stammen aus dem Schaltplan `Schematic_ESP32-C3_RS232_Adapter_V1_1.pdf` (LEOTRO, Rev 1.1, 2025-09-15).

---

## 1. Was dieses Board ist

Ein USB-C-versorgter **ESP32-C3 → RS232-Adapter**: Ein ESP32-C3-MINI-1-N4-Modul spricht über einen SP3232-Pegelwandler mit echten ±RS232-Pegeln auf einem male D-Sub-9-Stecker. Primärer Einsatzzweck: serielle Steuerung von Geräten mit RS232-Port (z. B. Rotel-Verstärker, Labornetzteile, Messgeräte) als WLAN-Bridge, typischerweise unter ESPHome mit Home-Assistant-Anbindung.

**Signalkette:** `ESP32-C3 UART (3,3 V TTL) → SP3232 (Pegelwandler) → ESD-Schutz (PESD15VL2BT) → D-Sub-9`

## 2. Pinbelegung ESP32-C3 (verbindlich)

| Funktion | GPIO | Modul-Pin | Richtung | Hinweis |
|---|---|---|---|---|
| **UART TX** (→ RS232) | **GPIO10** | 16 | Ausgang | geht auf SP3232 T1IN |
| **UART RX** (← RS232) | **GPIO4** | 18 | Eingang | kommt von SP3232 R1OUT |
| UART RTS | GPIO0 | 12 | Ausgang | ⚠️ Strapping-Pin; nur nutzen, wenn das Zielgerät Hardware-Handshake braucht |
| UART CTS | GPIO1 | 13 | Eingang | ⚠️ ADC-Pin; i. d. R. unbenutzt lassen |
| BOOT-Taster | GPIO9 | 23 | Eingang | Strapping (Download-Modus); nach Boot als Taster nutzbar |
| EN-Taster | EN | 8 | – | Reset, kein GPIO |
| USB D− | GPIO18 | 26 | – | **reserviert** für USB-Serial-JTAG (Type-C) |
| USB D+ | GPIO19 | 27 | – | **reserviert** für USB-Serial-JTAG (Type-C) |
| GPIO2, 3, 5, 6, 7, 8 | – | – | – | im Schaltplan **nicht beschaltet** (NC) – nicht verwenden |

**Regeln für Agenten:**
- UART immer auf **TX=GPIO10, RX=GPIO4** konfigurieren. Es gibt keine alternative Verdrahtung; RXD0/TXD0 (GPIO20/21) sind auf diesem Board unbeschaltet.
- Logging/Flashen läuft ausschließlich über **USB-Serial-JTAG** (GPIO18/19, Type-C). `logger:`-Ausgaben nie auf UART0/GPIO-Pins legen, sonst kollidieren sie mit nichts – aber sie kommen auch nirgends an.
- Die einzige LED (LED2) ist eine reine **Power-LED an 3V3** – nicht per GPIO steuerbar. Für Status-Feedback stehen keine Onboard-LEDs zur Verfügung; Status über Home Assistant / Logger abbilden.
- GPIO0/GPIO1 sind zwar als RTS/CTS bis zum D-Sub geführt, praktisch aber fast nie nötig (Rotel & Co. nutzen kein Handshaking). Unkonfiguriert lassen.

## 3. D-Sub-9-Belegung (Geräteseite)

Das Board ist als **DTE** verdrahtet (verhält sich wie ein PC-COM-Port):

| D-Sub-Pin | Signal | Richtung (aus Board-Sicht) |
|---|---|---|
| 2 | RX | Eingang |
| 3 | TX | Ausgang |
| 5 | GND | – |
| 7 | RTS | Ausgang |
| 8 | CTS | Eingang |
| 1, 4, 6, 9 | – | nicht beschaltet |

- Verbindung zu Geräten, die einen PC erwarten (Rotel, Tenma, Korad …): **1:1-Kabel (straight-through)**.
- Verbindung zu einem anderen DTE (PC, zweiter Adapter): **Nullmodemkabel** nötig.

## 4. Stromversorgung

- Eingang: USB-C (5 V), Verpolschutz-/Rückstromdiode 1N5819WS, LDO ME6211 → 3,3 V.
- CC1/CC2 mit 5,1 kΩ nach GND: Board meldet sich als Stromsenke – funktioniert auch an USB-C-Netzteilen ohne A-auf-C-Kabel-Tricks.
- Keine Batterie, kein Deep-Sleep-Zwang; das Board darf dauerhaft pollen.

## 5. Firmware-Konventionen (ESPHome-Standard)

Referenz-Basis für neue Configs (Board-Definition und UART-Pins sind fix):

```yaml
esp32:
  board: esp32-c3-devkitm-1
  framework:
    type: arduino

uart:
  id: uart_bus
  tx_pin: GPIO10
  rx_pin: GPIO04
  baud_rate: 115200   # geräteabhängig! Rotel: 115200 / Tenma, Korad, MPM-1010B: 9600
  data_bits: 8
  parity: NONE
  stop_bits: 1
```

- Baudrate, Terminator/Delimiter und Befehlsformat sind **eigenschaften des angeschlossenen Geräts**, nicht des Boards. Vor Firmware-Änderungen immer die zugehörige Protokolldatei des Zielgeräts konsultieren (für Rotel: `Rotel/rs232-serial.md`).
- Frame-Parsing per `uart: debug: after: delimiter:` ist das etablierte Muster auf diesem Board (siehe vorhandene Configs für Tenma 72-2645, Korad KEL103, Matrix MPM-1010B).
- Beispiel Rotel Gen 2: Befehle mit `!` terminieren, **kein CR/LF anhängen**, Antworten auf `$` splitten.
- Arduino-Framework: UART über `HardwareSerial(1)` mit explizitem Pin-Mapping: `serial.begin(115200, SERIAL_8N1, 4, 10);`

## 6. Bekannte Einschränkungen

1. **Kein Flow Control in Software erwarten:** Weder SP3232 noch typische Zielgeräte handhaben RTS/CTS automatisch – Timing (Delays zwischen Befehlen, sequenzielles Senden) muss die Firmware selbst sicherstellen.
2. **GPIO0 (RTS) ist Strapping-Pin:** Wenn ein Zielgerät RTS beim Boot auf Low zieht, kann das den Boot-Modus stören. Bei Boot-Problemen mit angeschlossenem Kabel zuerst RTS/CTS-Nutzung prüfen.
3. **4 MB Flash (N4-Modul):** Für ESPHome ausreichend; bei großen Arduino-Projekten Partitionsschema beachten.
4. **Ein UART für die Anwendung:** Der zweite UART-Kanal des SP3232 (T2/R2) ist im Schaltplan nur für RTS/CTS beschaltet, nicht als zweite Datenleitung nutzbar.
5. **NAD C 328:** RS232 am Gerät ist eine **3,5-mm-Klinke**, kein DB9. Verbindung zum LEOTRO-Adapter braucht ein Klinken-auf-DB9-Kabel; Pinout (Tip/Ring/Sleeve → TX/RX/GND) vor Verdrahtung am Modell-Protokoll dokumentieren (`NAD/nad-models-verified-addendum.md`). **C 338** hat kein RS232 – nicht unterstützen.
