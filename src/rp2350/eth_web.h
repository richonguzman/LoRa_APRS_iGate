#pragma once
// Minimal HTTP config server for the RP2350 iGate port, over the W5500
// (arduino-libraries/Ethernet). Adapted from the Meshtastic fork's mesh/eth
// transport (request parsing / CORS / response helpers), without the PhoneAPI
// / protobuf / OSThread coupling. Plain EthernetClient, served from a FreeRTOS
// task. Routes: GET /status, GET /configuration.json (more added incrementally).

#include <Arduino.h>

void ethWebSetup();   // bind EthernetServer on :80 + mount LittleFS
void ethWebPoll();    // accept + handle one pending client (call from the net task loop)

// Record an incoming APRS message addressed to us (served at GET /messages.json).
// `via` is "RF" or "IS". Call from the net task only (store is netTask-owned).
void ethWebAddMessage(const String &from, const String &text, const String &via);

// Record a received LoRa packet for GET /received-packets.json (net task only).
void ethWebAddPacket(const String &frame, int rssi, float snr);
