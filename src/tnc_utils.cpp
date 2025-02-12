#include <WiFi.h>
#include "kiss_utils.h"
#include "kiss_protocol.h"
#include "configuration.h"
#include "station_utils.h"
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
        if (Config.tnc.enableServer && !Config.digi.ecoMode) {
            tncServer.stop();
            tncServer.begin();
        }
    }

    void checkNewClients() {
        WiFiClient new_client = tncServer.accept();
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

    void handleInputData(char character, int bufferIndex) {
        String* data = (bufferIndex == -1) ? &inputSerialBuffer : &inputServerBuffer[bufferIndex];
        if (data->length() == 0 && character != (char)FEND) return;
        
        data->concat(character);

        if (character == (char)FEND && data->length() > 3) {
            bool isDataFrame = false;
            const String& frame = decodeKISS(*data, isDataFrame);

            if (isDataFrame) {
                if (bufferIndex != -1) {
                    Utils::print("<--- Got from TNC      : ");
                    Utils::println(frame);
                }

                String sender = frame.substring(0,frame.indexOf(">"));

                if (Config.tnc.acceptOwn || sender != Config.callsign) {
                    STATION_Utils::addToOutputPacketBuffer(frame);
                } else {
                    Utils::println("Ignored own frame from KISS");
                }
            }
            data->clear();
        }

        if (data->length() > 255) {
            data->clear();
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

    void readFromSerial() {
        while (Serial.available() > 0) {
            char character = Serial.read();
            handleInputData(character, -1);
        }
    }

    void sendToClients(const String& packet) {
        String cleanPacket = packet.substring(3);

        const String kissEncoded = encodeKISS(cleanPacket);

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
        Utils::println(cleanPacket);
    }

    void sendToSerial(const String& packet) {
        String cleanPacket = packet.substring(3);
        Serial.print(encodeKISS(cleanPacket));
        Serial.flush();
    }

    void loop() {
        if (!Config.digi.ecoMode) {
            if (Config.tnc.enableServer) {
                checkNewClients();
                readFromClients();
            }
            if (Config.tnc.enableSerial) {
                readFromSerial();
            }
        }
    }
}