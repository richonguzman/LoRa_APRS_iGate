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

#include <WiFi.h>
#include "ESPmDNS.h"
#include "configuration.h"
#include "station_utils.h"
#include "kiss_protocol.h"
#include "aprs_is_utils.h"
#include "kiss_utils.h"
#include "tnc_utils.h"
#include "utils.h"


extern Configuration    Config;
extern WiFiClient       aprsIsClient;
extern bool             passcodeValid;

#define MAX_CLIENTS 4
#define INPUT_BUFFER_SIZE (2 + MAX_CLIENTS)

#define TNC_PORT 8001

WiFiClient* clients[MAX_CLIENTS];

WiFiServer tncServer(TNC_PORT);

String inputServerBuffer[INPUT_BUFFER_SIZE];
String inputSerialBuffer = "";


namespace TNC_Utils {

    void setup() {
        if (Config.tnc.enableServer && Config.digi.ecoMode == 0) {
            tncServer.stop();
            tncServer.begin();
            String host = "igate-" + Config.callsign;
            if (!MDNS.begin(host.c_str())) {
                Serial.println("Error Starting mDNS");
                tncServer.stop();
                return;
            }
            if (!MDNS.addService("tnc", "tcp", TNC_PORT)) {
                Serial.println("Error: Could not add mDNS service");
            }
            Serial.println("TNC server started successfully");
            Serial.println("mDNS Host: " + host + ".local");
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
                    if (Config.loramodule.txActive) STATION_Utils::addToOutputPacketBuffer(frame);
                    if (Config.tnc.aprsBridgeActive && Config.aprs_is.active && passcodeValid && aprsIsClient.connected()) APRS_IS_Utils::upload(frame);
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

    void sendToClients(const String& packet, bool stripBytes) {
        String cleanPacket = stripBytes ? packet.substring(3): packet;

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

    void sendToSerial(const String& packet, bool stripBytes) {
        String cleanPacket = stripBytes ? packet.substring(3): packet;
        Serial.print(encodeKISS(cleanPacket));
        Serial.flush();
    }

    void loop() {
        if (Config.digi.ecoMode == 0) {
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