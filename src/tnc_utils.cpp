#include <WiFi.h>
#include "kiss_utils.h"
#include "kiss_protocol.h"
#include "lora_utils.h"

#define MAX_CLIENTS 4
#define INPUT_TNC_BUFFER_SIZE (2 + MAX_CLIENTS)

#define TNC_PORT 8001

WiFiClient* clients[MAX_CLIENTS];

WiFiServer tncServer(TNC_PORT);

String inputBuffer[INPUT_TNC_BUFFER_SIZE];

namespace TNC_Utils {
    void setup() {
        tncServer.stop();
        tncServer.begin();
    }

    void checkNewClients() {
        WiFiClient new_client = tncServer.available();

        if (new_client.connected()) {
            for (int i = 0; i < MAX_CLIENTS; i++) {
                WiFiClient* client = clients[i];

                if (client == nullptr) {
                    clients[i] = new WiFiClient(new_client);

                    Serial.println("New TNC client connected");

                    break;
                }
            }
        }
    }

    void handleInputData(char character, int bufferIndex) {
        String* inTNCData = &inputBuffer[bufferIndex];

        if (inTNCData->length() == 0 && character != (char)FEND) {
            return;
        }

        inTNCData->concat(character);

        if (character == (char)FEND && inTNCData->length() > 3) {
            bool isDataFrame = false;
            const String& frame = decodeKISS(*inTNCData, isDataFrame);

            if (isDataFrame) {
                Serial.print("---> Got from TNC      : ");
                Serial.println(frame);

                LoRa_Utils::sendNewPacket("APRS", frame);
            }

            inTNCData->clear();
        }

        if (inTNCData->length() > 255) {
            inTNCData->clear();
        }
    }

    void readFromClients() {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            auto client = clients[i];
            if (client != nullptr) {
                if (client->connected()) {
                    while (client->available() > 0) {
                        char character = client->read();
                        handleInputData(character, 2 + i);
                    }
                } else {
                    delete client;
                    clients[i] = nullptr;
                }
            }
        }
    }

    void sendToClients(String packet) {
        packet = packet.substring(3);

        const String kissEncoded = encodeKISS(packet);

        for (int i = 0; i < MAX_CLIENTS; i++) {
            auto client = clients[i];
            if (client != nullptr) {
                if (client->connected()) {
                    client->print(kissEncoded);
                    client->flush();
                } else {
                    delete client;
                    clients[i] = nullptr;
                }
            }
        }

        Serial.print("---> Sent to TNC       : ");
        Serial.println(packet);
    }

    void loop() {
        checkNewClients();

        readFromClients();
    }
}