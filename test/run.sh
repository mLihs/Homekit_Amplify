#!/usr/bin/env bash
# Host-Tests fuer Parser/TX-Logik (AmpCommand.h) – laeuft ohne Hardware.
# Die Stubs in test/stub/ ersetzen Arduino.h, HardwareSerial.h und Preferences.h;
# sie liegen bewusst NICHT im Sketch-Ordner, damit der Arduino-Build sie ignoriert.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(dirname "$here")"
out="$here/build"
mkdir -p "$out"

g++ -std=c++17 -Wall -Wextra -O1 \
    -I "$here/stub" -I "$root" \
    "$here/test_amp.cpp" -o "$out/test_amp"

"$out/test_amp"
