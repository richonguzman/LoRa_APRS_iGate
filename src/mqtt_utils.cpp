/* Copyright (C) 2025 Ricardo Guzman - CA2RXU
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

#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "configuration.h"
#include "station_utils.h"
#include "mqtt_utils.h"


extern          Configuration   Config;
extern          WiFiClient      mqttClient;

PubSubClient    pubSub;


namespace MQTT_Utils {

    void sendToMqtt(const String& packet) {
        if (!pubSub.connected()) {
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

    void connect() {
        if (pubSub.connected()) return;
        if (Config.mqtt.server.isEmpty() || Config.mqtt.port <= 0) {
            Serial.println("Connect to MQTT server KO because no host or port given");
            return;
        }
        pubSub.setServer(Config.mqtt.server.c_str(), Config.mqtt.port);
        Serial.print("Trying to connect with MQTT Server: " + String(Config.mqtt.server) + " MqttServerPort: " + String(Config.mqtt.port));

        bool connected = false;
        if (!Config.mqtt.username.isEmpty()) {
            connected = pubSub.connect(Config.callsign.c_str(), Config.mqtt.username.c_str(), Config.mqtt.password.c_str());
        } else {
            connected = pubSub.connect(Config.callsign.c_str());
        }

        if (connected) {
            Serial.println(" -> Connected !");
            const String subscribedTopic = Config.mqtt.topic + "/" + Config.callsign + "/#";
            if (!pubSub.subscribe(subscribedTopic.c_str())) {
                Serial.println("Subscribed to MQTT Failed");
            }
            Serial.print("Subscribed to MQTT topic : ");
            Serial.println(subscribedTopic);
        } else {
            Serial.println(" -> Not Connected (Retry in a few secs)");
        }
    }

    void loop() {
        if (!Config.mqtt.active) return;
        if (!pubSub.connected()) return;
        pubSub.loop();
    }

    void setup() {
        if (!Config.mqtt.active) return;
        pubSub.setClient(mqttClient);
        pubSub.setCallback(receivedFromMqtt);
    }

}