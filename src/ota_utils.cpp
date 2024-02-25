#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <AsyncTCP.h>
#include "configuration.h"
#include "ota_utils.h"
#include "display.h"

extern Configuration        Config;
extern uint32_t             lastScreenOn;

unsigned long ota_progress_millis = 0;


namespace OTA_Utils {
    
    void setup(AsyncWebServer *server) {
        if (Config.ota.username != ""  && Config.ota.password != "") {
            ElegantOTA.begin(server, Config.ota.username.c_str(), Config.ota.password.c_str());
        } else {
            ElegantOTA.begin(server);
        }
        
        ElegantOTA.setAutoReboot(true);
        ElegantOTA.onStart(onOTAStart);
        ElegantOTA.onProgress(onOTAProgress);
        ElegantOTA.onEnd(onOTAEnd);
    }

    void onOTAStart() {
        Serial.println("OTA update started!");
        display_toggle(true);
        lastScreenOn = millis();
        show_display("", "", "", " OTA update started!", "", "", "", 1000);
    }

    void onOTAProgress(size_t current, size_t final) {
        if (millis() - ota_progress_millis > 1000) {
            display_toggle(true);
            lastScreenOn = millis();
            ota_progress_millis = millis();
            Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
            show_display("", "", "  OTA Progress : " + String((current*100)/final) + "%", "", "", "", "", 100);
        }
    }

    void onOTAEnd(bool success) {
        display_toggle(true);
        lastScreenOn = millis();
        if (success) {
            Serial.println("OTA update finished successfully!");
            show_display("", "", " OTA update success!", "", "    Rebooting ...", "", "", 4000);
        } else {
            Serial.println("There was an error during OTA update!");
            show_display("", "", " OTA update fail!", "", "", "", "", 4000);
        }
    }
    
}