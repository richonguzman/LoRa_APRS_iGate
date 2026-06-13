#pragma once
#include <Arduino.h>

// MQTT bridge for the RP2350 iGate (PubSubClient over EthernetClient; replaces
// the ESP32 mqtt_utils' WiFiClient). When Config.mqtt.active:
//   - publishes each received RF packet to "<topic>/<sender>"
//   - subscribes to "<topic>/<callsign>/#"; payloads are transmitted over RF
//   - optionally publishes our own beacon to "<topic>/<callsign>" (beaconOverMqtt)
// netTask only (W5500 owner); RF downlink goes through enqueueRfFrame().
namespace Mqtt {
    void setup();                          // configure broker + callback if active
    void loop();                           // (re)connect + pubSub.loop(); call each net loop
    void publishRx(const String &frame);   // a received RF frame -> MQTT
    void publishBeacon(const String &line); // own beacon -> MQTT (if beaconOverMqtt)
}
