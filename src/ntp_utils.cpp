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

#include <esp_sntp.h>
#include "configuration.h"
#include "network_manager.h"
#include "ntp_utils.h"
#include "time.h"


extern      Configuration  Config;
extern      NetworkManager *networkManager;
bool        ntpStarted  = false;
bool        ntpSynced   = false;


namespace NTP_Utils {

    bool setup() {
        if (networkManager->isConnected() && Config.digi.ecoMode == 0 && Config.callsign != "NOCALL-10") {
            long gmt = Config.ntp.gmtCorrection * 3600;
            Serial.println("[NTP] Setting up, TZ offset: " + String(gmt) + " Server: " +  Config.ntp.server);
            sntp_set_sync_interval(3 * 60 * 60 * 1000);         // Update interval 3 hours (ESP32 internal clock drift is negligible in that time)
            configTime(gmt, 0, Config.ntp.server.c_str());      // SNTP runs in background (DNS + retries) and never blocks the loop
            ntpStarted = true;
            return true;
        }
        return false;
    }

    void update() {
        if (!networkManager->isConnected() || Config.digi.ecoMode != 0 || Config.callsign == "NOCALL-10") {
            return;
        }
        if (!ntpStarted) {
            if (!setup()) {
                return;
            }
        }
        if (!ntpSynced) {
            struct tm timeinfo;
            if (getLocalTime(&timeinfo, 0)) {
                ntpSynced = true;
                Serial.println("[NTP] Time synced: " + getFormatedTime());
            }
        }
    }

    String getFormatedTime() {
        if (Config.digi.ecoMode != 0) return "DigiEcoMode Active";
        struct tm timeinfo;
        if (!ntpStarted || !getLocalTime(&timeinfo, 0)) return "";     // 0 ms: don't wait (default timeout would block 5 s)
        char formatedTime[9];
        strftime(formatedTime, sizeof(formatedTime), "%H:%M:%S", &timeinfo);
        return String(formatedTime);
    }

}
