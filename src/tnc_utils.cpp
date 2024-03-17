#include <WiFi.h>
#include "kiss_utils.h"
#include "kiss_protocol.h"
#include "lora_utils.h"
#include "configuration.h"
#include "utils.h"

extern Configuration        Config;

#define MAX_CLIENTS 4
#define INPUT_BUFFER_SIZE (2 + MAX_CLIENTS)

#define TNC_PORT 8001

WiFiClient* clients[MAX_CLIENTS];

WiFiServer tncServer(TNC_PORT);

String inputServerBuffer[INPUT_BUFFER_SIZE];
String inputSerialBuffer = "";

namespace TNC_Utils {
    void setup() {
        if (Config.tnc.enableServer) {
            tncServer.stop();
            tncServer.begin();
        }
    }

    void checkNewClients() {
        WiFiClient new_client = tncServer.available();

        if (new_client.connected()) {
            for (int i = 0; i < MAX_CLIENTS; i++) {
                WiFiClient* client = clients[i];

                if (client == nullptr) {
                    clients[i] = new WiFiClient(new_client);

                    Utils::println("New TNC client connected");

                    break;
                }
            }
        }
    }

    void handleServerInputData(char character, int bufferIndex) {
        String* inTNCData = &inputServerBuffer[bufferIndex];

        if (inTNCData->length() == 0 && character != (char)FEND) {
            return;
        }

        inTNCData->concat(character);

        if (character == (char)FEND && inTNCData->length() > 3) {
            bool isDataFrame = false;
            const String& frame = decodeKISS(*inTNCData, isDataFrame);

            if (isDataFrame) {
                Utils::print("<--- Got from TNC      : ");
                Utils::println(frame);

                String sender = frame.substring(0,frame.indexOf(">"));

                if (Config.tnc.acceptOwn || sender != Config.callsign) {
                    LoRa_Utils::sendNewPacket("APRS", frame);
                } else {
                    Utils::println("Ignored own frame from TNC");
                }
            }

            inTNCData->clear();
        }

        if (inTNCData->length() > 255) {
            inTNCData->clear();
        }
    }

    void handleSerialInputData(char character) {
        if (inputSerialBuffer.length() == 0 && character != (char)FEND) {
            return;
        }

        inputSerialBuffer.concat(character);

        if (character == (char)FEND && inputSerialBuffer.length() > 3) {
            bool isDataFrame = false;
            const String& frame = decodeKISS(inputSerialBuffer, isDataFrame);

            if (isDataFrame) {
                String sender = frame.substring(0,frame.indexOf(">"));

                if (Config.tnc.acceptOwn || sender != Config.callsign) {
                    LoRa_Utils::sendNewPacket("APRS", frame);
                }
            }

            inputSerialBuffer.clear();
        }

        if (inputSerialBuffer.length() > 255) {
            inputSerialBuffer.clear();
        }
    }

    void readFromClients() {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            auto client = clients[i];
            if (client != nullptr) {
                if (client->connected()) {
                    while (client->available() > 0) {
                        char character = client->read();
                        handleServerInputData(character, 2 + i);
                    }
                } else {
                    delete client;
                    clients[i] = nullptr;
                }
            }
        }
    }

    void readFromSerial() {
        while (Serial.available() > 0) {
            char character = Serial.read();
            handleSerialInputData(character);
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

        Utils::print("---> Sent to TNC       : ");
        Utils::println(packet);
    }

    void sendToSerial(String packet) {
        packet = packet.substring(3);

        Serial.print(encodeKISS(packet));
        Serial.flush();
    }

    void loop() {
        if (Config.tnc.enableServer) {
            checkNewClients();

            readFromClients();
        }

        if (Config.tnc.enableSerial) {
            readFromSerial();
        }
    }
}