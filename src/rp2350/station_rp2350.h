#pragma once
#include <Arduino.h>

// Lean port of STATION_Utils (src/station_utils.cpp) for the RP2350 iGate.
// Provides the shared packet-policy services: blacklist/managers, last-heard
// tracking, a 25 s duplicate filter, and an anti-collision RF output buffer.
//
// CONCURRENCY: every function here is meant to be called from loraTask only (the
// owner of the radio). All state is therefore single-threaded — no locks. The
// one exception is activeCount(), which reads a volatile snapshot so netTask can
// print it in the heartbeat. Dropped from the original: display hooks, EcoMode /
// low-voltage sleep, and config-write-on-TX (not in the RP2350 feature set).
namespace Station {
    void   setup();                                   // load blacklist/managers from Config (call at boot)

    bool   isBlacklisted(const String& callsign);
    bool   isManager(const String& callsign);

    void   updateLastHeard(const String& station);    // record/refresh a heard station
    bool   wasHeard(const String& station);           // heard within rememberStationTime?
    size_t activeCount();                             // # stations currently tracked (volatile snapshot)
    String heardListString();                        // space-separated heard callsigns (for ?APRSL)

    // 25 s duplicate filter on (station, payload). True if the same packet was
    // seen within the window (caller should neither gate nor digipeat it).
    bool   isDuplicate(const String& station, const String& payload);

    // RF output buffer (anti-collision TX scheduling). Enqueue digipeats/beacons
    // from loraTask; processTxBuffer() sends at most one packet per call once the
    // channel has been idle (no RX/TX) for SECS_TO_WAIT seconds.
    void   enqueueTx(const String& packet, bool isBeacon = false);
    void   noteRx();                                   // call on every RX (feeds the TX backoff)
    void   processTxBuffer();                          // call each loraTask iteration
    size_t txPending();
}
