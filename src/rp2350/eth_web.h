#pragma once
// Minimal HTTP config server for the RP2350 iGate port, over the W5500
// (arduino-libraries/Ethernet). Adapted from the Meshtastic fork's mesh/eth
// transport (request parsing / CORS / response helpers), without the PhoneAPI
// / protobuf / OSThread coupling. Plain EthernetClient, served from a FreeRTOS
// task. Routes: GET /status, GET /configuration.json (more added incrementally).

void ethWebSetup();   // bind EthernetServer on :80 + mount LittleFS
void ethWebPoll();    // accept + handle one pending client (call from the net task loop)
