#pragma once
#include <Arduino.h>

// Lean port of QUERY_Utils (src/query_utils.cpp) for the RP2350 iGate. Answers
// APRS message queries directed at our callsign, received over RF. Supported
// public queries: ?APRS?/H/HELP/? (help), ?APRSV (version), ?APRSP (position),
// ?APRSL (last-heard list), ?APRSSR (signal report of the last RX). Manager-only
// remote control (?EM=/?TX=/?COMMIT) is NOT ported (EcoMode/txActive gating not
// in the RP2350 feature set).
//
// Replies and acks are enqueued into the Station RF output buffer (anti-collision).
// Call from loraTask only.
namespace Query {
    // If `body` (headerless "SENDER>PATH::ADDRESSEE :text") is an APRS message
    // addressed to our callsign: send an ack (when it carries a {msgid}) and, if
    // the text is a query, enqueue the reply over RF. Returns true when it was a
    // query for us (caller should NOT gate/digipeat it). A non-query message for
    // us is acked but returns false (let the caller gate it as usual).
    bool handleMessage(const String& body, const String& sender);
}
