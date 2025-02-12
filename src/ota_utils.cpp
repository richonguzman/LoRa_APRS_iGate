#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <AsyncTCP.h>
#include "configuration.h"
#include "ota_utils.h"
#include "display.h"

extern Configuration        Config;
extern uint32_t             lastScreenOn;
extern bool                 isUpdatingOTA;

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
        displayToggle(true);
        lastScreenOn = millis();
        displayShow("", "", "", " OTA update started!", "", "", "", 1000);
        isUpdatingOTA = true;
    }

    void onOTAProgress(size_t current, size_t final) {
        if (millis() - ota_progress_millis > 1000) {
            displayToggle(true);
            lastScreenOn = millis();
            ota_progress_millis = millis();
            Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
            displayShow("", "", "  OTA Progress : " + String((current*100)/final) + "%", "", "", "", "", 100);
        }
    }

    void onOTAEnd(bool success) {
        displayToggle(true);
        lastScreenOn = millis();

        String statusMessage = success ? "OTA update success!" : "OTA update fail!";
        String rebootMessage = success ? "Rebooting ..." : "";

        Serial.println(success ? "OTA update finished successfully!" : "There was an error during OTA update!");
        displayShow("", "", statusMessage, "", rebootMessage, "", "", 4000);
        
        isUpdatingOTA = false;
    }
    
}