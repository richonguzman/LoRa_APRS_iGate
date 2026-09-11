#pragma once
#include <Arduino.h>

// Lean APRS digipeater for the RP2350 port. Faithful port of the WIDEn-N path
// rewriting from src/digi_utils.cpp (DIGI_Utils), trimmed to the single-radio /
// same-frequency case (no cross-freq, no third-party, no message queries — those
// belong to the iGate/TNC paths). Read-only on Config; keeps its own small dedup
// ring buffer to break repeater loops.
//
// Modes (Config.digi.mode, matching the web UI):
//   0  OFF
//   2  WIDE1 (fill-in) digi  — consume WIDE1-1 only
//   3  WIDE2 (+WIDE1) digi   — consume WIDE1-1 and WIDE2-n
// Config.digi.backupDigiMode forces fill-in (mode-2) behaviour even when mode==0.
namespace Digi {
    // True when digipeating is enabled (so the caller can skip the work entirely).
    bool enabled();

    // Given the RAW received packet (WITH the 3-byte LoRa-APRS header), returns
    // the headerless packet to re-transmit over RF ("SENDER>PATH:payload"), or ""
    // if this packet should not be digipeated (off, self, dup, no WIDE to consume,
    // invalid callsign, NOGATE/RFONLY excluded by caller via the iGate path).
    String process(const String& rawPacket);
}
