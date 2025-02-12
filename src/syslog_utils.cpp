#include <WiFiUdp.h>
#include <WiFi.h>
#include "configuration.h"
#include "syslog_utils.h"
#include "gps_utils.h"

extern Configuration    Config;

WiFiUDP udpClient;


namespace SYSLOG_Utils {

    void log(const uint8_t type, const String& packet, const int rssi, const float snr, const int freqError) {
        if (Config.syslog.active && WiFi.status() == WL_CONNECTED) {
            String syslogPacket = "<165>1 - ";
            syslogPacket.concat(Config.callsign);
            syslogPacket.concat(" CA2RXU_LoRa_iGate_1.3 - - - "); //RFC5424 The Syslog Protocol
            
            char signalData[35];
            snprintf(signalData, sizeof(signalData), " / %ddBm / %.2fdB / %dHz", rssi, snr, freqError);

            int colonIndex  = packet.indexOf(":");
            char nextChar   = packet[colonIndex + 1];
            String sender   = packet.substring(3, packet.indexOf(">"));

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
                    } else if (nextChar == '!' || nextChar == '=') {
                        syslogPacket.concat("GPS / ");
                        syslogPacket.concat(sender);
                        syslogPacket.concat(" / ");
                        if (packet.indexOf("WIDE1-1") > 10) {
                            syslogPacket.concat(packet.substring(packet.indexOf(">") + 1, packet.indexOf(",")));
                            syslogPacket.concat(" / WIDE1-1");
                        } else {
                            syslogPacket.concat(packet.substring(packet.indexOf(">") + 1, colonIndex));
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
                    } else if (packet.indexOf(":T#") >= 10 && packet.indexOf(":=/") == -1) {
                        syslogPacket.concat("TELEMETRY / ");
                        syslogPacket.concat(sender);
                        syslogPacket.concat(" ---> ");
                        syslogPacket.concat(packet.substring(packet.indexOf(":T#") + 3));
                    } else {
                        syslogPacket.concat(packet);
                    }
                    syslogPacket.concat(signalData);
                    if (nextChar == '!' || nextChar == '=') {
                        syslogPacket.concat(" / ");
                        syslogPacket.concat(GPS_Utils::getDistanceAndComment(packet));
                    }
                    break;
                case 2:     // APRSIS TX
                    syslogPacket.concat("APRSIS TX / ");
                    if (nextChar == '>') {
                        syslogPacket.concat("StartUp_Status / ");
                        syslogPacket.concat(packet.substring(colonIndex + 2));
                    } else {
                        syslogPacket.concat("QUERY / ");
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
        if (Config.syslog.active && WiFi.status() == WL_CONNECTED) {
            udpClient.begin(WiFi.localIP(), 0);
            Serial.println("init : Syslog Server  ...     done!    (at " + Config.syslog.server + ")");
        }
    }

}