#pragma once
#include <Arduino.h>

// Lean APRS digipeater for the RP2350 port. Faithful port of the WIDEn-N path
// rewriting from src/digi_utils.cpp (DIGI_Utils), trimmed to the single-radio /
// same-frequency case (no cross-freq, no third-party, no message queries — those
// belong to the iGate/TNC paths). Read-only on Config; keeps its own small dedup
// ring buffer to break repeater loops.
//
// Modes (Config.digi.mode, matching the web UI since V4.0.0):
//   0  OFF
//   1  WIDE1 (fill-in) digi  — consume WIDE1-1 only
//   2  WIDE2 (+WIDE1) digi   — consume WIDE1-1 and WIDE2-n
//   3  own-callsign digi     — repeat only when our callsign is next in the path
// While the backup digi runs (Config.digi.backupDigiMode ticked AND APRS-IS down,
// see g_backupDigi in main.cpp) only WIDE1-1 is repeated, whatever the mode.
namespace Digi {
    // True when digipeating is enabled (so the caller can skip the work entirely).
    bool enabled();

    // Given the RAW received packet (WITH the 3-byte LoRa-APRS header), returns
    // the headerless packet to re-transmit over RF ("SENDER>PATH:payload"), or ""
    // if this packet should not be digipeated (off, self, dup, no WIDE to consume,
    // invalid callsign, NOGATE in the path).
    String process(const String& rawPacket);
}
