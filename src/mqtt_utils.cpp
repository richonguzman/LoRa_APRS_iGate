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
#include "serial_ports.h"
#include "station_utils.h"
#include "mqtt_utils.h"


extern          Configuration   Config;
extern          WiFiClient      mqttClient;

PubSubClient    pubSub;


namespace MQTT_Utils {

    void sendToMqtt(const String& packet) {
        if (!pubSub.connected()) {
            DEBUG_PRINTLN("Can not send to MQTT because it is not connected");
            return;
        }
        const String cleanPacket    = packet.substring(3);
        const String sender         = cleanPacket.substring(0, cleanPacket.indexOf(">"));
        const String topic          = String(Config.mqtt.topic + "/" + sender);

        const bool result           = pubSub.publish(topic.c_str(), cleanPacket.c_str());
        if (result) {
            DEBUG_PRINT("Packet sent to MQTT topic "); DEBUG_PRINTLN(topic);
        } else {
            DEBUG_PRINTLN("Packet not sent to MQTT (check connection)");
        }
    }

    void receivedFromMqtt(char* topic, byte* payload, unsigned int length) {
        DEBUG_PRINT("Received from MQTT topic "); DEBUG_PRINT(topic); DEBUG_PRINT(": ");
        for (int i = 0; i < length; i++) {
            DEBUG_PRINT((char)payload[i]);
        }
        DEBUG_PRINTLN();
        STATION_Utils::addToOutputPacketBuffer(String(payload, length));
    }

    void connect() {
        if (pubSub.connected()) return;
        if (Config.mqtt.server.isEmpty() || Config.mqtt.port <= 0) {
            DEBUG_PRINTLN("Connect to MQTT server KO because no host or port given");
            return;
        }
        pubSub.setServer(Config.mqtt.server.c_str(), Config.mqtt.port);
        DEBUG_PRINT("Trying to connect with MQTT Server: " + String(Config.mqtt.server) + " MqttServerPort: " + String(Config.mqtt.port));

        bool connected = false;
        if (!Config.mqtt.username.isEmpty()) {
            connected = pubSub.connect(Config.callsign.c_str(), Config.mqtt.username.c_str(), Config.mqtt.password.c_str());
        } else {
            connected = pubSub.connect(Config.callsign.c_str());
        }

        if (connected) {
            DEBUG_PRINTLN(" -> Connected !");
            const String subscribedTopic = Config.mqtt.topic + "/" + Config.callsign + "/#";
            if (!pubSub.subscribe(subscribedTopic.c_str())) {
                DEBUG_PRINTLN("Subscribed to MQTT Failed");
            }
            DEBUG_PRINT("Subscribed to MQTT topic : ");
            DEBUG_PRINTLN(subscribedTopic);
        } else {
            DEBUG_PRINTLN(" -> Not Connected (Retry in a few secs)");
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