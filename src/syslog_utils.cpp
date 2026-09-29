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

#include <APRSPacketLib.h>
#include <WiFiUdp.h>
#include "configuration.h"
#include "network_manager.h"
#include "syslog_utils.h"
#include "gps_utils.h"


extern Configuration    Config;
extern NetworkManager   *networkManager;
extern String           versionDate;
extern String           versionNumber;

WiFiUDP udpClient;

namespace {

    String createSyslogStart() {
        String syslogStartPacket = "<165>1 - ";
        syslogStartPacket.concat(Config.callsign);
        syslogStartPacket.concat(" CA2RXU_LoRa_iGate_");
        syslogStartPacket.concat(versionNumber);
        syslogStartPacket.concat(" - - - "); //RFC5424 The Syslog Protocol
        return syslogStartPacket;
    }

    void sendSyslogPacket(const String& syslogPacket) {
        udpClient.beginPacket(Config.syslog.server.c_str(), Config.syslog.port);
        udpClient.write((const uint8_t*)syslogPacket.c_str(), syslogPacket.length());
        udpClient.endPacket();
    }

    String formatSignalData(int rssi, float snr, int freqError) {
        char signalData[35];
        snprintf(signalData, sizeof(signalData), " / %ddBm / %.2fdB / %dHz", rssi, snr, freqError);
        return String(signalData);
    }

}

namespace SYSLOG_Utils {

    void logLoRaRx(APRSPacket& aprsPacket, const String& packet, const int rssi, const float snr, const int freqError) {
        if (Config.syslog.active && networkManager->isConnected()) {
            String syslogPacket = createSyslogStart();
            syslogPacket.concat("RX / ");

            switch (aprsPacket.type) {
                case 1:     // MESSAGE
                    syslogPacket.concat("MESSAGE / ");
                    syslogPacket.concat(aprsPacket.sender);
                    syslogPacket.concat(" ---> ");
                    syslogPacket.concat(aprsPacket.addressee);
                    syslogPacket.concat(":");
                    syslogPacket.concat(aprsPacket.payload);
                    break;
                case 0:     // GPS
                    syslogPacket.concat("GPS / ");
                    syslogPacket.concat(aprsPacket.sender);
                    syslogPacket.concat(" / ");
                    if (aprsPacket.path.indexOf("WIDE1-1") != -1) {
                        syslogPacket.concat(aprsPacket.tocall);
                        syslogPacket.concat(" / WIDE1-1");
                    } else {
                        syslogPacket.concat(aprsPacket.tocall);
                        if (aprsPacket.path != "") {
                            syslogPacket.concat(",");
                            syslogPacket.concat(aprsPacket.path);
                        }
                        syslogPacket.concat(" / -");
                    }
                    break;
                case 2:     // STATUS
                    syslogPacket.concat("STATUS / ");
                    syslogPacket.concat(aprsPacket.sender);
                    syslogPacket.concat(" ---> ");
                    syslogPacket.concat(aprsPacket.payload);
                    break;
                case 4:     // MIC-E
                    syslogPacket.concat("MIC-E / ");
                    syslogPacket.concat(aprsPacket.sender);
                    syslogPacket.concat(" ---> ");
                    syslogPacket.concat(packet.indexOf(":`") != -1 ? "`" : "'");   // restore the actual indicator received
                    break;
                case 5:     // OBJECT
                    syslogPacket.concat("OBJECT / ");
                    syslogPacket.concat(aprsPacket.sender);
                    syslogPacket.concat(" ---> ");
                    syslogPacket.concat(aprsPacket.payload);
                    break;
                case 3:     // TELEMETRY
                    syslogPacket.concat("TELEMETRY / ");
                    syslogPacket.concat(aprsPacket.sender);
                    syslogPacket.concat(" ---> ");
                    syslogPacket.concat(aprsPacket.payload);
                    break;
                default:    // type == 6, unrecognized -- payload is the full, uncut packet
                    syslogPacket.concat(aprsPacket.payload);
                    break;
            }
            syslogPacket.concat(formatSignalData(rssi, snr, freqError));
            if (aprsPacket.type == 0 || aprsPacket.type == 4) {   // GPS or Mic-E -- both already carry
                syslogPacket.concat(" / ");                        // clean lat/lon + comment in aprsPacket
                syslogPacket.concat(GPS_Utils::buildDistanceAndComment(aprsPacket.latitude, aprsPacket.longitude, aprsPacket.payload));
            }
            sendSyslogPacket(syslogPacket);
        }
    }

    void logAPRSISTx(const String& packet) {
        if (Config.syslog.active && networkManager->isConnected()) {
            String syslogPacket = createSyslogStart();
            syslogPacket.concat("APRSIS TX / ");

            APRSPacket aprsPacket = APRSPacketLib::processReceivedPacket(packet, 0, 0, 0);
            if (aprsPacket.type == 2) {          // STATUS
                syslogPacket.concat("StartUp_Status / ");
                syslogPacket.concat(aprsPacket.payload);
            } else if (aprsPacket.type == 1) {   // MESSAGE
                syslogPacket.concat("QUERY / ");
                syslogPacket.concat(packet);
            } else {
                syslogPacket.concat("BEACON / ");
                syslogPacket.concat(packet);
            }
            sendSyslogPacket(syslogPacket);
        }
    }

    void logLoRaTx(const String& packet) {
        if (Config.syslog.active && networkManager->isConnected()) {
            String syslogPacket = createSyslogStart();
            syslogPacket.concat("TX / ");

            APRSPacket aprsPacket = APRSPacketLib::processReceivedPacket(packet, 0, 0, 0);
            if (aprsPacket.path.indexOf("RFONLY") != -1) {
                syslogPacket.concat("RFONLY / ");
                syslogPacket.concat(packet);
            } else if (aprsPacket.type == 1) {
                syslogPacket.concat("MESSAGE / ");
                syslogPacket.concat(aprsPacket.sender);
                syslogPacket.concat(" ---> ");
                syslogPacket.concat(aprsPacket.addressee);
                syslogPacket.concat(":");
                syslogPacket.concat(aprsPacket.payload);
            } else {
                syslogPacket.concat(packet);
            }
            sendSyslogPacket(syslogPacket);
        }
    }

    void logCRCError(const String& packet, const int rssi, const float snr, const int freqError) {
        if (Config.syslog.active && networkManager->isConnected()) {
            String syslogPacket = createSyslogStart();
            syslogPacket.concat("CRC / CRC-ERROR / ");
            syslogPacket.concat(packet);
            syslogPacket.concat(formatSignalData(rssi, snr, freqError));
            sendSyslogPacket(syslogPacket);
        }
    }

    void setup() {
        if (networkManager->isConnected()) {
            udpClient.begin(0);
            if (Config.syslog.active) Serial.println("init : Syslog Server  ...     done!    (at " + Config.syslog.server + ")");
        }
    }

}