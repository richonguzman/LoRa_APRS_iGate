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

#include <WiFiUdp.h>
#include <WiFi.h>
#include "configuration.h"
#include "syslog_utils.h"
#include "gps_utils.h"


extern Configuration    Config;
extern String           versionDate;
extern String           versionNumber;

WiFiUDP udpClient;


namespace SYSLOG_Utils {

    void log(const uint8_t type, const String& packet, const int rssi, const float snr, const int freqError) {
        if (Config.syslog.active && WiFi.status() == WL_CONNECTED) {
            String syslogPacket = "<165>1 - ";
            syslogPacket.concat(Config.callsign);
            syslogPacket.concat(" CA2RXU_LoRa_iGate_");
            syslogPacket.concat(versionNumber);
            syslogPacket.concat(" - - - "); //RFC5424 The Syslog Protocol

            char signalData[35];
            snprintf(signalData, sizeof(signalData), " / %ddBm / %.2fdB / %dHz", rssi, snr, freqError);

            int colonIndex              = packet.indexOf(":");
            char nextChar               = packet[colonIndex + 1];
            int greaterThanIndex        = packet.indexOf(">");
            int telemetryPacketIndex    = packet.indexOf(":T#");
            String sender               = packet.substring(3, greaterThanIndex);

            switch (type) {
                case 0:     // CRC
                    syslogPacket.concat("CRC / CRC-ERROR / ");
                    syslogPacket.concat(packet);
                    syslogPacket.concat(signalData);
                    break;
                case 1:     // RX
                    syslogPacket.concat("RX / ");
                    if (nextChar == ':') {
                        syslogPacket.concat("MESSAGE / ");
                        syslogPacket.concat(sender);
                        syslogPacket.concat(" ---> ");
                        syslogPacket.concat(packet.substring(colonIndex + 2));
                    } else if (nextChar == '!' || nextChar == '=' || nextChar == '@') {
                        syslogPacket.concat("GPS / ");
                        syslogPacket.concat(sender);
                        syslogPacket.concat(" / ");
                        if (packet.indexOf("WIDE1-1") > 10) {
                            syslogPacket.concat(packet.substring(greaterThanIndex + 1, packet.indexOf(",")));
                            syslogPacket.concat(" / WIDE1-1");
                        } else {
                            syslogPacket.concat(packet.substring(greaterThanIndex + 1, colonIndex));
                            syslogPacket.concat(" / -");
                        }
                    } else if (nextChar == '>') {
                        syslogPacket.concat("STATUS / ");
                        syslogPacket.concat(sender);
                        syslogPacket.concat(" ---> ");
                        syslogPacket.concat(packet.substring(colonIndex + 2));
                    } else if (nextChar == '`') {
                        syslogPacket.concat("MIC-E / ");
                        syslogPacket.concat(sender);
                        syslogPacket.concat(" ---> ");
                        syslogPacket.concat(packet.substring(colonIndex + 2));
                    } else if (nextChar == ';') {
                        syslogPacket.concat("OBJECT / ");
                        syslogPacket.concat(sender);
                        syslogPacket.concat(" ---> ");
                        syslogPacket.concat(packet.substring(colonIndex + 2));
                    } else if (telemetryPacketIndex >= 10 && packet.indexOf(":=/") == -1) {
                        syslogPacket.concat("TELEMETRY / ");
                        syslogPacket.concat(sender);
                        syslogPacket.concat(" ---> ");
                        syslogPacket.concat(packet.substring(telemetryPacketIndex + 3));
                    } else {
                        syslogPacket.concat(packet);
                    }
                    syslogPacket.concat(signalData);
                    if (nextChar == '!' || nextChar == '=' || nextChar == '@') {
                        syslogPacket.concat(" / ");
                        syslogPacket.concat(GPS_Utils::getDistanceAndComment(packet));
                    }
                    break;
                case 2:     // APRSIS TX
                    syslogPacket.concat("APRSIS TX / ");
                    if (nextChar == '>') {
                        syslogPacket.concat("StartUp_Status / ");
                        syslogPacket.concat(packet.substring(colonIndex + 2));
                    } else if (nextChar == ':') {
                        syslogPacket.concat("QUERY / ");
                        syslogPacket.concat(packet);
                    } else {
                        syslogPacket.concat("BEACON / ");
                        syslogPacket.concat(packet);
                    }
                    break;
                case 3:     // TX
                    syslogPacket.concat("TX / ");
                    if (packet.indexOf("RFONLY") > 10) {
                        syslogPacket.concat("RFONLY / ");
                        syslogPacket.concat(packet);
                    } else if (nextChar == ':') {
                        syslogPacket.concat("MESSAGE / ");
                        syslogPacket.concat(sender);
                        syslogPacket.concat(" ---> ");
                        syslogPacket.concat(packet.substring(colonIndex + 2));
                    } else {
                        syslogPacket.concat(packet);
                    }
                    break;
                default:
                    syslogPacket = "<165>1 - ERROR LoRa - - - ERROR / Error in Syslog Packet"; //RFC5424 The Syslog Protocol
                    break;
            }
            udpClient.beginPacket(Config.syslog.server.c_str(), Config.syslog.port);
            udpClient.write((const uint8_t*)syslogPacket.c_str(), syslogPacket.length());
            udpClient.endPacket();
        }
    }

    void setup() {
        if (WiFi.status() == WL_CONNECTED) {
            udpClient.begin(0);
            if (Config.syslog.active) Serial.println("init : Syslog Server  ...     done!    (at " + Config.syslog.server + ")");
        }
    }

}