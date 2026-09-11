#pragma once
// Compatibility shim for W5100S builds (the rp2350_igate_w5100s env).
//
// The RAK13800_W5100S driver ships its API in <RAK13800_W5100S.h> and provides
// no <Ethernet.h>, whereas the W5500 (arduino-libraries/Ethernet) build does.
// This directory is added to the include path ONLY by the W5100S env, so the
// iGate sources' `#include <Ethernet.h>` resolves here and pulls the W5100S
// driver — which exposes the same Ethernet/EthernetClient/EthernetServer/
// EthernetUDP API, so no other source changes are needed.
#include <RAK13800_W5100S.h>
