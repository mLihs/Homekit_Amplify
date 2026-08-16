# NAD Models — RS232 Verification Addendum

**Purpose:** Supplement to `amp-rs232-protocols.md`. Result of a rear-panel/manual verification pass (2026-08). Contains only models with a confirmed RS232 port. Checked but excluded: **C 338** (no RS232/IR/trigger on rear panel, Chromecast/app control only — do not implement). Merge into the `NAD` brand block of the main knowledge base; entries here **override** any conflicting entries there.

**Verification method:** owner's manuals (rear panel sections) and manufacturer/review documentation. ## Verified model list (JSON)

```json
{
  "brand": "NAD",
  "protocol_default": "nad_v2",
  "models": [
    {
      "id": "m33",
      "name": "M 33",
      "protocol": "nad_v2",
      "type": "integrated",
      "rs232": true,
      "verified": true,
      "note": "Masters series. RS232 port confirmed on rear panel and in owner's manual (references NAD RS232 protocol docs). BluOS built-in -> VERIFY command subset in the M33-specific protocol doc before implementation."
    },
    {
      "id": "c399",
      "name": "C 399",
      "protocol": "nad_v2",
      "type": "integrated",
      "rs232": true,
      "verified": true,
      "note": "Owner's manual references NAD RS232 protocol docs. Two MDC2 slots; BluOS-D module optional -> base RS232 control independent of BluOS."
    },
    {
      "id": "c389",
      "name": "C 389",
      "protocol": "nad_v2",
      "type": "integrated",
      "rs232": true,
      "verified": true,
      "note": "RS232 + IR ports present. Rear connections identical to C 399 / C 379. BluOS-D optional via MDC2 -> VERIFY per-model protocol doc."
    },
    {
      "id": "c388",
      "name": "C 388",
      "protocol": "nad_v2",
      "type": "integrated",
      "rs232": true,
      "verified": true,
      "note": "Same platform as C 368. VERIFY source numbering/list in per-model doc."
    },
    {
      "id": "c379",
      "name": "C 379",
      "protocol": "nad_v2",
      "type": "integrated",
      "rs232": true,
      "verified": true,
      "note": "Rear connections identical to C 399 / C 389 (RS232 + IR confirmed for the series)."
    },
    {
      "id": "c368",
      "name": "C 368",
      "protocol": "nad_v2",
      "type": "integrated",
      "rs232": true,
      "verified": true,
      "note": "RS232 labeled on rear panel next to IR IN/OUT and +12V trigger. Community-tested (MQTT bridge project)."
    },
    {
      "id": "c328",
      "name": "C 328",
      "protocol": "nad_v2",
      "type": "integrated",
      "rs232": true,
      "verified": true,
      "note": "RS232 present but as 3.5mm mini-jack, NOT DB9 -> requires jack-to-DB9 adapter cable; VERIFY pinout in the C 328 protocol/firmware doc. Port doubles as firmware-upgrade port."
    }
  ]
}
```

## Agent instructions

1. **Merge rule:** replace/insert these entries by `id` in the main knowledge base. All listed models have `rs232: true`. The C 338 was checked and has no RS232 port; it is intentionally absent from this list and must not be added to the supported-model dropdown.
2. **Protocol:** all supported entries use `nad_v2` (115200 8N1, `Main.<Var><op><val>` grammar, CR terminator, `\r` preamble) as defined in the main file.
3. **VERIFY obligations before shipping a model:**
   - `m33`, `c389`, `c399`, `c3050` (BluOS): fetch the model-specific protocol PDF from nadelectronics.com → software and diff the variable set against the generic `nad_v2` table (`Main.Power`, `Main.Volume`, `Main.Mute`, `Main.Source`); BluOS-era firmware may rename or extend variables.
   - `c328`: confirm mini-jack RS232 pinout (tip/ring/sleeve → TX/RX/GND) before wiring to the SP3232 adapter.
   - `c388`: confirm `Main.Source` numbering (source list differs from C 368 in digital-input count).
4. **Hardware:** DB9 models connect straight-through to the LEOTRO ESP32-C3 adapter (DTE). The C 328 mini-jack requires a custom cable; document it in `AGENTS.md` once the pinout is verified.
