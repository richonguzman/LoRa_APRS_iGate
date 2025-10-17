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

#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFi.h>
#include "configuration.h"
#include "ntp_utils.h"
#include "time.h"


extern      Configuration  Config;

WiFiUDP     ntpUDP;
NTPClient*  timeClient;


namespace NTP_Utils {

    void setup() {
        if (WiFi.status() == WL_CONNECTED && Config.digi.ecoMode == 0 && Config.callsign != "NOCALL-10") {
            int gmt = Config.ntp.gmtCorrection * 3600;
            timeClient = new NTPClient(ntpUDP, Config.ntp.server.c_str(), gmt, 15 * 60 * 1000); // Update interval 15 min
            timeClient->begin();
        }
    }

    void update() {
        if (WiFi.status() == WL_CONNECTED && Config.digi.ecoMode == 0 && Config.callsign != "NOCALL-10") timeClient->update();
    }

    String getFormatedTime() {
        if (Config.digi.ecoMode == 0) return timeClient->getFormattedTime();
        return "DigiEcoMode Active";
    }

}