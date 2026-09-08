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

#include <WiFi.h>
#include "ESPmDNS.h"
#include "configuration.h"
#include "station_utils.h"
#include "aprs_is_utils.h"
#include "tnc_utils.h"
#include "utils.h"
#include "lora_utils.h"

extern Configuration    Config;
extern WiFiClient       aprsIsClient;
extern bool             passcodeValid;
extern int              rssi;
extern float            snr;
extern int              freqOffset;

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
            Serial.println("TNC server started successfully (TNC2 text mode)");
            Serial.println("mDNS Host: " + host + ".local");
        }
    }

    void checkNewClients() {
        WiFiClient new_client = tncServer.accept();
        if (new_client.connected()) {
            bool placed = false;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                WiFiClient* client = clients[i];
                if (client == nullptr) {
                    clients[i] = new WiFiClient(new_client);
                    Utils::println("New TNC2 client connected");
                    placed = true;
                    break;
                }
            }
            if (!placed) {
                // All MAX_CLIENTS slots full -- reject explicitly rather
                // than accepting at the TCP level and then silently
                // abandoning the connection with no close and no log line.
                Utils::println("TNC2 client rejected: max clients (" + String(MAX_CLIENTS) + ") already connected");
                new_client.stop();
            }
        }
    }

    void handleInputData(char character, int bufferIndex) {
        String* data = (bufferIndex == -1) ? &inputSerialBuffer : &inputServerBuffer[bufferIndex];
        
        if (character == '\r') return;

        if (character == '\n') {
            if (data->length() > 3) {
                String frame = *data;
                frame.trim();
                
                if (frame.length() > 0) {
                    if (bufferIndex != -1) {
                        Utils::print("<--- Got from TNC2     : ");
                        Utils::println(frame);
                    }

                    int gtIdx = frame.indexOf('>');
                    if (gtIdx != -1) {
                        String sender = frame.substring(0, gtIdx);

                        if (Config.tnc.acceptOwn || sender != Config.callsign) {
                            if (Config.loramodule.txActive) STATION_Utils::addToOutputPacketBuffer(frame);
                            if (Config.tnc.aprsBridgeActive && Config.aprs_is.active && passcodeValid && aprsIsClient.connected()) {
                                APRS_IS_Utils::upload(frame);
                            }
                        } else {
                            Utils::println("Ignored own frame from TNC2 line");
                        }
                    }
                }
            }
            data->clear();
            return;
        }

        data->concat(character);
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

    // Formats a value with an explicit "+" for zero/positive readings, so
    // + and - values print with the same visual width. Only used for
    // fields that can genuinely go either way (SNR, FO) -- RSSI and TTH
    // are structurally one-signed under the current encoding and are left
    // as plain numbers.
    String signedInt(int value) {
        return (value >= 0 ? "+" : "") + String(value);
    }

    String signedFloat(float value, int decimals) {
        return (value >= 0 ? "+" : "") + String(value, decimals);
    }

    void sendToClients(const String& packet, bool levelInfo, const std::vector<LoRa_Utils::RxtHopMetric>& hopMetrics) {
        if (packet.length() == 0) return;

        // packet now arrives with any RXT trailer still attached (needed
        // upstream so a second RXT digi can concatenate onto it when
        // forwarding). Strip it here for the client-facing line -- TNC
        // clients should only ever see the clean APRS packet.
        String lineToSend = LoRa_Utils::stripRxtTrailer(packet) + "\r\n"; // Line 1: Clean APRS packet

        for (int i = 0; i < MAX_CLIENTS; i++) {
            auto client = clients[i];
            if (client != nullptr) {
                if (client->connected()) {
                    // Send Line 1
                    client->print(lineToSend);
                    
                    // Send Line 2: Local receiver metrics
                    if (levelInfo) {
                        client->print("RSSI:" + String(rssi) + " SNR:" + signedFloat(snr, 2) + " FO:" + signedInt(freqOffset) + "\r\n");
                    }

                    // Send Line 3+: full hop chain -- real data or NA per hop.
                    // Printed receiver-first (toNode<--fromNode) so the
                    // measuring node is always the leading callsign, matching
                    // the LOCAL line's implicit "receiver = me" convention.
                    for (const auto& hop : hopMetrics) {
                        if (hop.hasData) {
                            client->print(hop.toNode + "<--" + hop.fromNode +
                                          " RSSI:" + String(hop.rssi) +
                                          " SNR:" + signedFloat(hop.snr, 2) +
                                          " FO:" + signedInt(hop.fo) +
                                          " TTH:" + String(hop.tth) + "\r\n");
                        } else {
                            client->print(hop.toNode + "<--" + hop.fromNode + " NA\r\n");
                        }
                    }
                    client->flush();
                } else {
                    delete client;
                    clients[i] = nullptr;
                }
            }
        }
        Utils::print("---> Sent to TNC2     : ");
        Utils::println(packet);
    }

    void sendToSerial(const String& packet, bool levelInfo, const std::vector<LoRa_Utils::RxtHopMetric>& hopMetrics) {
        if (packet.length() == 0) return;

        // packet now arrives with any RXT trailer still attached (needed
        // upstream so a second RXT digi can concatenate onto it when
        // forwarding). Strip it here for the client-facing line -- serial
        // clients should only ever see the clean APRS packet.
        Serial.print("\r\n");
        Serial.print(LoRa_Utils::stripRxtTrailer(packet) + "\r\n");
        Serial.flush();
        
        // Line 2: Local receiver metrics
        if (levelInfo) {
            Serial.println("LOCAL -- RSSI:" + String(rssi) + " SNR:" + signedFloat(snr, 2) + " FO:" + signedInt(freqOffset));
        }

        // Line 3+: full hop chain -- real data or NA per hop. Printed
        // receiver-first (toNode<--fromNode) so the measuring node is
        // always the leading callsign, matching the LOCAL line's implicit
        // "receiver = me" convention. Already in most-recent-hop-first
        // display order; no further parsing here.
        for (const auto& hop : hopMetrics) {
            if (hop.hasData) {
                Serial.println(hop.toNode + "<--" + hop.fromNode +
                               " RSSI:" + String(hop.rssi) +
                               " SNR:" + signedFloat(hop.snr, 2) +
                               " FO:" + signedInt(hop.fo) +
                               " TTH:" + String(hop.tth));
            } else {
                Serial.println(hop.toNode + "<--" + hop.fromNode + " NA");
            }
        }
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
