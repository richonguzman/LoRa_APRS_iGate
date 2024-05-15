#include <WiFiUdp.h>
#include <WiFi.h>
#include "configuration.h"
#include "syslog_utils.h"
#include "gps_utils.h"

extern Configuration    Config;

WiFiUDP udpClient;


namespace SYSLOG_Utils {

    void log(uint8_t type, const String& packet, int rssi, float snr, int freqError) {
        String syslogPacket = "<165>1 - " + Config.callsign + " CA2RXU_LoRa_iGate_1.3" + " - - - "; //RFC5424 The Syslog Protocol
        if (Config.syslog.active && WiFi.status() == WL_CONNECTED) {
            switch (type) {
                case 0:     // CRC
                    syslogPacket += type + " / CRC-ERROR / " + packet;
                    syslogPacket += " / " + String(rssi) + "dBm / " + String(snr) + "dB / " + String(freqError) + "Hz";
                    break;
                case 1:     // RX
                    if (packet.indexOf("::") > 10) {
                        syslogPacket += type + " / MESSAGE / " + packet.substring(3,packet.indexOf(">")) + " ---> " + packet.substring(packet.indexOf("::")+2);
                        syslogPacket += " / " + String(rssi) + "dBm / " + String(snr) + "dB / " + String(freqError) + "Hz";
                    } else if (packet.indexOf(":!") > 10 || packet.indexOf(":=") > 10) {
                        syslogPacket += type + " / GPS / " + packet.substring(3,packet.indexOf(">")) + " / ";
                        if (packet.indexOf("WIDE1-1") > 10) {
                            syslogPacket += packet.substring(packet.indexOf(">")+1,packet.indexOf(",")) + " / WIDE1-1 / ";
                        } else {
                            syslogPacket += packet.substring(packet.indexOf(">")+1,packet.indexOf(":")) + " / - / ";
                        }
                        syslogPacket += String(rssi) + "dBm / " + String(snr) + "dB / " + String(freqError) + "Hz / " +  GPS_Utils::getDistance(packet);
                    } else if (packet.indexOf(":>") > 10) {
                        syslogPacket += type + " / STATUS / " + packet.substring(3,packet.indexOf(">")) + " ---> " + packet.substring(packet.indexOf(":>")+2);
                        syslogPacket += " / " + String(rssi) + "dBm / " + String(snr) + "dB / " + String(freqError) + "Hz";
                    } else if (packet.indexOf(":`") > 10) {
                        syslogPacket += type + " / MIC-E / " + packet.substring(3,packet.indexOf(">")) + " ---> " + packet.substring(packet.indexOf(":`")+2);
                        syslogPacket += " / " + String(rssi) + "dBm / " + String(snr) + "dB / " + String(freqError) + "Hz";
                    } else if (packet.indexOf(":T#") >= 10 && packet.indexOf(":=/") == -1) {
                        syslogPacket += type + " / TELEMETRY / " + packet.substring(3,packet.indexOf(">")) + " ---> " + packet.substring(packet.indexOf(":T#")+3);
                        syslogPacket += " / " + String(rssi) + "dBm / " + String(snr) + "dB / " + String(freqError) + "Hz";
                    } else if (packet.indexOf(":;") > 10) {
                        syslogPacket += type + " / OBJECT / " + packet.substring(3,packet.indexOf(">")) + " ---> " + packet.substring(packet.indexOf(":;")+2);
                        syslogPacket += " / " + String(rssi) + "dBm / " + String(snr) + "dB / " + String(freqError) + "Hz";
                    } else {
                        syslogPacket += type + " / " + packet;
                        syslogPacket += " / " + String(rssi) + "dBm / " + String(snr) + "dB / " + String(freqError) + "Hz";
                    }
                    break;
                case 2:     // APRSIS TX
                    if (packet.indexOf(":>") > 10) {
                        syslogPacket += type + " / StartUp_Status / " + packet.substring(packet.indexOf(":>")+2);
                    } else {
                        syslogPacket += type + " / QUERY / " + packet;
                    }
                    break;
                case 3:     // TX
                    if (packet.indexOf("RFONLY") > 10) {
                        syslogPacket += type + " / RFONLY / " + packet;
                    } else if (packet.indexOf("::") > 10) {
                        syslogPacket += type + " / MESSAGE / " + packet.substring(0,packet.indexOf(">")) + " ---> " + packet.substring(packet.indexOf("::")+2);
                    } else {
                        syslogPacket += type + " / " + packet;
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