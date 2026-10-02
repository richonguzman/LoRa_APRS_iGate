/*
 * RP2350 MQTT bridge — lean port of mqtt_utils (PubSubClient), WiFiClient ->
 * EthernetClient. Publishes RX packets, subscribes for an RF downlink. netTask
 * only (the W5500 owner; PubSubClient and the EthernetClient live here).
 */
#include "mqtt_rp2350.h"
#include <Ethernet.h>
#include <PubSubClient.h>
#include "configuration.h"

extern Configuration Config;
extern void enqueueRfFrame(const String &frame);   // main.cpp: -> txMsgQueue -> loraTask

static EthernetClient mqttNet;
static PubSubClient   pubSub(mqttNet);
static bool           started = false;
static uint32_t       lastTry = 0;

// True if `sender` is this iGate (callsign or tactical). Used to break feedback
// loops: our subscription "<topic>/<callsign>/#" also matches "<topic>/<callsign>"
// (MQTT '#' includes the parent), so our own publishes echo back to us.
static bool isOurs(const String &sender) {
    return sender == Config.callsign ||
           (Config.tacticalCallsign.length() && sender == Config.tacticalCallsign);
}

// Inbound MQTT message on the subscribed topic -> transmit over RF.
static void onMessage(char *topic, byte *payload, unsigned int len) {
    String msg;
    msg.reserve(len);
    for (unsigned int i = 0; i < len; i++) msg += (char)payload[i];
    int gt = msg.indexOf('>');
    String sender = (gt > 0) ? msg.substring(0, gt) : "";
    if (isOurs(sender)) {           // our own packet echoed back by the broker — don't re-TX
        Serial.println("[mqtt] ignored own frame echoed back");
        return;
    }
    Serial.println("[mqtt] rx " + String(topic) + ": " + msg);
    enqueueRfFrame(msg);
}

namespace Mqtt {

void setup() {
    if (!Config.mqtt.active) return;
    pubSub.setServer(Config.mqtt.server.c_str(), Config.mqtt.port);
    pubSub.setCallback(onMessage);
    started = true;
    Serial.printf("[mqtt] enabled, broker %s:%d\n", Config.mqtt.server.c_str(), Config.mqtt.port);
}

static void connect() {
    if (Config.mqtt.server.isEmpty() || Config.mqtt.port <= 0) return;
    Serial.println("[mqtt] connecting...");
    bool ok = Config.mqtt.username.length()
                  ? pubSub.connect(Config.callsign.c_str(), Config.mqtt.username.c_str(), Config.mqtt.password.c_str())
                  : pubSub.connect(Config.callsign.c_str());
    if (ok) {
        String sub = Config.mqtt.topic + "/" + Config.callsign + "/#";
        pubSub.subscribe(sub.c_str());
        Serial.println("[mqtt] connected, subscribed " + sub);
    } else {
        Serial.println("[mqtt] connect failed rc=" + String(pubSub.state()));
    }
}

void loop() {
    if (!started) return;
    if (!pubSub.connected()) {
        if (millis() - lastTry > 10000) { lastTry = millis(); connect(); }
        return;
    }
    pubSub.loop();
}

void publishRx(const String &frame) {
    if (!started || !pubSub.connected()) return;
    int gt = frame.indexOf('>');
    String sender = (gt > 0) ? frame.substring(0, gt) : "unknown";
    if (isOurs(sender)) return;        // don't publish our own packets (they'd echo back -> RF loop)
    String topic = Config.mqtt.topic + "/" + sender;
    pubSub.publish(topic.c_str(), frame.c_str());
}

void publishBeacon(const String &line) {
    if (!started || !pubSub.connected() || !Config.mqtt.beaconOverMqtt) return;
    String topic = Config.mqtt.topic + "/" + Config.callsign;
    pubSub.publish(topic.c_str(), line.c_str());
}

}  // namespace Mqtt
