#include "syslog_utils.h"
#include "gps_utils.h"

namespace SYSLOG_Utils {

void processPacket(String packet, int rssi, float snr, int freqError) {
    String syslogPacket;
    syslogPacket = packet.substring(3,packet.indexOf(">")) + " / TIME / ";
    syslogPacket += packet.substring(packet.indexOf(">")+1,packet.indexOf(",")) + " / ";
   if (packet.indexOf("WIDE1-1") > 10) {
        syslogPacket += "WIDE1-1 / ";
    } else {
        syslogPacket += " _ / ";
    }
    syslogPacket += String(rssi) + "dBm / " + String(snr) + "dB / " + String(freqError) + "Hz / ";
    // Callsign / Time / Destination / Path / RSSI / SNR / FreqError /gpsLat / gpsLon / Distance
    Serial.println(syslogPacket + GPS_Utils::getDistance(packet));
}

}