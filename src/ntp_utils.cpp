#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFi.h>
#include "configuration.h"
#include "ntp_utils.h"
#include "time.h"


extern      Configuration  Config;

WiFiUDP     ntpUDP;
NTPClient   timeClient(ntpUDP, "pool.ntp.org", 0, 15 * 60 * 1000);  // Update interval 15 min


namespace NTP_Utils {

    void setup() {
        if (WiFi.status() == WL_CONNECTED && !Config.digi.ecoMode && Config.callsign != "NOCALL-10") {
            int gmt = Config.ntp.gmtCorrection * 3600;
            timeClient.setTimeOffset(gmt);
            timeClient.begin();
        }
    }

    void update() {
        if (WiFi.status() == WL_CONNECTED && !Config.digi.ecoMode && Config.callsign != "NOCALL-10") timeClient.update();
    }

    String getFormatedTime() {
        if (!Config.digi.ecoMode) return timeClient.getFormattedTime();
        return "DigiEcoMode Active";
    }

}