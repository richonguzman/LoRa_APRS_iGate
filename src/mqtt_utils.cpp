#include "mqtt_utils.h"
#include <configuration.h>
#include <station_utils.h>

extern Configuration    Config;

namespace MQTT_Utils {

    PubSubClient pubSub;

    void setup(WiFiClient &client) {
        if (!Config.mqtt.active) {
            return;
        }

        pubSub.setClient(client);
        pubSub.setCallback(receivedFromMqtt);
    }

    bool connect() {
        if (pubSub.connected()) {
            return true;
        }

        Serial.println("Connect to MQTT server...");

        if (Config.mqtt.server.isEmpty() || Config.mqtt.port <= 0) {
            Serial.println("Connect to MQTT server KO because no host or port given");
            return false;
        }

        pubSub.setServer(Config.mqtt.server.c_str(), Config.mqtt.port);

        int count = 0;
        while (!pubSub.connect(Config.callsign.c_str(), Config.mqtt.username.c_str(), Config.mqtt.password.c_str()) && count < 20) {
            Serial.println("Didn't connect to MQTT server...");
            delay(1000);
            Serial.println("Trying to connect with MQTT Server: " + String(Config.mqtt.server) + " MqttServerPort: " + String(Config.mqtt.port));
            count++;
            Serial.println("Try: " + String(count));
        }
        if (count == 20) {
            Serial.println("Tried: " + String(count) + " FAILED!");
        }

        if (pubSub.connected()) {
            Serial.println("Connect to MQTT server OK");

            const String subscribedTopic = Config.mqtt.topic + "/" + Config.callsign + "/#";
            if (!pubSub.subscribe(subscribedTopic.c_str())) {
                Serial.println("Subscribe to MQTT topic KO");
            }
            Serial.print("Subscribed to MQTT topic "); Serial.println(subscribedTopic);

            return true;
        }

        Serial.println("Connect to MQTT server KO");
        return false;
    }

    void loop() {
        if (!Config.mqtt.active) {
            return;
        }

        connect();
        pubSub.loop();
    }

    void sendToMqtt(const String& packet) {
        if (!pubSub.connected() && !connect()) {
            Serial.println("Can not send to MQTT because it is not connected");
            return;
        }

        Serial.println("Send packet to MQTT...");

        const String cleanPacket = packet.substring(3);
        const String sender = cleanPacket.substring(0, cleanPacket.indexOf(">"));
        const String topic = String(Config.mqtt.topic + "/" + sender);

        const bool result = pubSub.publish(topic.c_str(), cleanPacket.c_str());
        if (result) {
            Serial.print("Sended packet to MQTT topic "); Serial.println(topic);
        } else {
            Serial.println("Send packet to MQTT KO");
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
}