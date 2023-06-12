#include "configuration.h"
#include "syslog_utils.h"
#include "gps_utils.h"
#include "logger.h"

extern Configuration    Config;
extern logging::Logger  logger;

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
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Main", (syslogPacket + GPS_Utils::getDistance(packet)).c_str());
}

void setup() {
    if (Config.syslog.active) {
        logger.setSyslogServer(Config.syslog.server, Config.syslog.port, "ESP32 LoRa APRS iGate");
        Serial.println("Syslog Server (" + Config.syslog.server + ") connected!\n");
    }
}

}