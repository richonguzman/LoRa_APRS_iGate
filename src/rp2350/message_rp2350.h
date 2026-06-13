#pragma once
#include <Arduino.h>

// Originate APRS text messages from the iGate. A small feature (no UI yet — first
// driven over the web /action API for testing the query responder, later wirable
// to the SPA). Builds a standard APRS message packet:
//   CALL>APLRG1[,path]::ADDRESSEE :text        (addressee padded to 9 chars)
// The text is sent verbatim; append a "{id" suffix if you want the recipient to
// ack it.
namespace Message {
    // Pure builders (read-only on Config) — safe to call from any task. Return ""
    // on empty to/text.
    //   buildRF    : headerless packet for Station::enqueueTx (RF; LoRa_Utils
    //                prepends the 3-byte LoRa-APRS header at TX).
    //   buildAprsis: line for AprsIs::send (APRS-IS over TCP, TCPIP* q-construct).
    String buildRF(const String& to, const String& text);
    String buildAprsis(const String& to, const String& text);
}
