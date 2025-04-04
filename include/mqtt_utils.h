#ifndef MQTT_UTILS_H_
#define MQTT_UTILS_H_

#include <Arduino.h>
#include <WiFiClient.h>
#include <PubSubClient.h>


namespace MQTT_Utils {

    void setup(WiFiClient &client);
    bool connect();
    void loop();
    void sendToMqtt(const String& packet);
    void receivedFromMqtt(char* topic, byte* payload, unsigned int length);

}

#endif