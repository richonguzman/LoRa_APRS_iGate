/* Copyright (C) 2026 Ricardo Guzman - CA2RXU
 *
 * This file is part of LoRa APRS iGate.
 *
 * LoRa APRS iGate is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LoRa APRS iGate is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LoRa APRS iGate. If not, see <https://www.gnu.org/licenses/>.
 */

#include <atomic>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include "configuration.h"
#include "station_utils.h"
#include "mqtt_utils.h"


extern          Configuration   Config;
extern          WiFiClient      mqttClient;

PubSubClient    pubSub;


namespace MQTT_Utils {

    static const uint32_t   RETRY_FIRST_MS  = 15000;        // after a failed connect: wait this long, then twice as long each time, up to RETRY_MAX_MS
    static const uint32_t   RETRY_MAX_MS    = 300000;
    static uint32_t         nextTry         = 0;            // earliest time of the next connect attempt
    static uint32_t         retryDelayMs    = RETRY_FIRST_MS;
    static std::atomic<bool> connecting(false);             // the connect task is running: the main loop must not touch the MQTT client

    // The client may be used only while no connect is in progress (the connect task owns it then), and only when it is connected.
    static bool usable() {
        return !connecting.load() && pubSub.connected();
    }

    bool isConnected() {
        return usable();
    }

    void sendToMqtt(const String& packet) {
        if (!usable()) {
            Serial.println("Can not send to MQTT because it is not connected");
            return;
        }
        const String cleanPacket    = packet.substring(3);
        const String sender         = cleanPacket.substring(0, cleanPacket.indexOf(">"));
        const String topic          = String(Config.mqtt.topic + "/" + sender);

        const bool result           = pubSub.publish(topic.c_str(), cleanPacket.c_str());
        if (result) {
            Serial.print("Packet sent to MQTT topic "); Serial.println(topic);
        } else {
            Serial.println("Packet not sent to MQTT (check connection)");
        }
    }

    void receivedFromMqtt(char* topic, byte* payload, unsigned int length) {
        Serial.print("Received from MQTT topic "); Serial.print(topic); Serial.print(": ");
        for (int i = 0; i < length; i++) {
            Serial.print((char)payload[i]);
        }
        Serial.println();
        STATION_Utils::addToOutputPacketBuffer(String(payload, length));
    }

    // Connecting is slow when something is wrong: a DNS lookup, a TCP connect that times out, and up to 15 s waiting for a broker that accepts the
    // connection but never answers. Done in the main loop, that stalls the radio for as long as it takes, and the loop retried on every pass.
    // It now runs in its own task; while it runs the main loop leaves the MQTT client alone (usable() is false). After a failure the next
    // attempt waits 15 s, then 30, 60 ... up to 5 minutes.
    static void connectTask(void*) {
        const String server = Config.mqtt.server;
        const int    port   = Config.mqtt.port;
        const String call   = Config.callsign;
        const String user   = Config.mqtt.username;
        const String pass   = Config.mqtt.password;
        const String topic  = Config.mqtt.topic;

        pubSub.setServer(server.c_str(), port);
        bool connected = false;
        if (!user.isEmpty()) {
            connected = pubSub.connect(call.c_str(), user.c_str(), pass.c_str());
        } else {
            connected = pubSub.connect(call.c_str());
        }

        if (connected) {
            mqttClient.setTimeout(2);                       // seconds: a write on a dead socket must not hold the main loop for long
            retryDelayMs = RETRY_FIRST_MS;
            Serial.println("Trying to connect with MQTT Server: " + server + " MqttServerPort: " + String(port) + " -> Connected !");
            const String subscribedTopic = topic + "/" + call + "/#";
            if (!pubSub.subscribe(subscribedTopic.c_str())) {
                Serial.println("Subscribed to MQTT Failed");
            }
            Serial.print("Subscribed to MQTT topic : ");
            Serial.println(subscribedTopic);
        } else {
            nextTry = millis() + retryDelayMs;
            Serial.println("Trying to connect with MQTT Server: " + server + " MqttServerPort: " + String(port) + " -> Not Connected (retry in " + String(retryDelayMs / 1000) + " s)");
            retryDelayMs = retryDelayMs * 2 > RETRY_MAX_MS ? RETRY_MAX_MS : retryDelayMs * 2;
        }
        connecting.store(false);
        vTaskDelete(nullptr);
    }

    void connect() {
        if (connecting.load() || pubSub.connected()) return;
        if (Config.mqtt.server.isEmpty() || Config.mqtt.port <= 0) return;      // nothing to connect to
        if ((int32_t)(millis() - nextTry) < 0) return;                          // still waiting after a failure
        connecting.store(true);
        if (xTaskCreatePinnedToCore(connectTask, "mqttconn", 8192, nullptr, 1, nullptr, 0) != pdPASS) {
            connecting.store(false);
            nextTry = millis() + retryDelayMs;                                  // no memory for the task right now: try again later
            Serial.println("MQTT: could not start the connect task, will retry");
        }
    }

    void loop() {
        if (!Config.mqtt.active) return;
        if (!usable()) return;
        pubSub.loop();
    }

    void setup() {
        if (!Config.mqtt.active) return;
        pubSub.setClient(mqttClient);
        pubSub.setCallback(receivedFromMqtt);
    }

}