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

#include "configuration.h"
#include "board_pinout.h"
#include "sleep_utils.h"
#include "digi_utils.h"
#include "lora_utils.h"


extern  Configuration   Config;
extern  bool            shouldSleepStop;
extern  uint32_t        lastBeaconTx;

bool    wakeUpFlag      = false;


namespace SLEEP_Utils {

    void wakeUpLoRaPacketReceived() {
        wakeUpFlag = true;
    }

    void checkWakeUpFlag() {
        if (wakeUpFlag) {
            String packet = LoRa_Utils::receivePacketFromSleep();
            if (packet != "") {
                DIGI_Utils::processLoRaPacket(packet);
            }
            wakeUpFlag = false;
        }
    }

    void setup() {
        #ifndef HAS_A7670
        if (Config.digi.ecoMode == 1) {
            pinMode(RADIO_WAKEUP_PIN, INPUT);
            attachInterrupt(digitalPinToInterrupt(RADIO_WAKEUP_PIN), wakeUpLoRaPacketReceived, RISING);
            #if defined(TTGO_LORA32_V2_1) || defined(TTGO_LORA32_V2_1_915) || defined(TTGO_LORA32_T3S3_V1_2) || defined(TTGO_T_BEAM_V1_0) || defined(TTGO_T_BEAM_V1_0_915) || defined(TTGO_T_BEAM_V1_0_SX1268) || defined(TTGO_T_BEAM_V1_2) || defined(TTGO_T_BEAM_V1_2_915) || defined(TTGO_T_BEAM_V1_2_SX1262) || defined(TTGO_T_DECK_PLUS) || defined(TTGO_T_DECK_GPS) || defined(TTGO_T_Beam_S3_SUPREME_V3) || defined(HELTEC_V3) || defined(HELTEC_V3_2) || defined(HELTEC_WP) || defined(HELTEC_WS) || defined(HELTEC_WSL_V3) || defined(HELTEC_WSL_V3_DISPLAY) || defined(HELTEC_WIRELESS_TRACKER) || defined(HELTEC_V2) || defined(XIAO_ESP32S3_LORA) || defined(LIGHTGATEWAY_1_0) || defined(LIGHTGATEWAY_PLUS_1_0) || defined(TROY_LoRa_APRS) || defined(OE5HWN_MeshCom) || defined(ESP32_DIY_LoRa) || defined(ESP32_DIY_LoRa_915) || defined(ESP32_DIY_1W_LoRa) || defined(ESP32_DIY_1W_LoRa_915) || defined(ESP32_DIY_1W_LoRa_LLCC68) || defined(ESP32_DIY_1W_LoRa_Mesh_V1_2) || defined(WEMOS_S2_MINI_DIY_LoRa) || defined(WEMOS_D1_R32_RA02) || defined(WEMOS_LOLIN32_OLED_DIY_LoRa)
                esp_sleep_enable_ext1_wakeup(GPIO_WAKEUP_PIN, ESP_EXT1_WAKEUP_ANY_HIGH);
            #endif
            #if defined(HELTEC_HTCT62) || defined(ESP32C3_DIY_1W_LoRa) || defined(ESP32C3_DIY_1W_LoRa_915) || defined(ESP32_C3_OctopusLab_LoRa)
                esp_deep_sleep_enable_gpio_wakeup(1ULL << GPIO_WAKEUP_PIN, ESP_GPIO_WAKEUP_GPIO_HIGH);
            #endif
        }
        #endif
    }

    uint32_t getSecondsToSleep() {
        uint32_t elapsedTime    = (millis() - lastBeaconTx) / 1000; // in secs
        uint32_t intervalTime   = Config.beacon.interval * 60;      // in secs
        return (elapsedTime < intervalTime) ? (intervalTime - elapsedTime) : 0;
    }

    void startSleeping() {
        if (!shouldSleepStop) {
            uint32_t timeToSleep = getSecondsToSleep();
            esp_sleep_enable_timer_wakeup(timeToSleep * 1000000);   // 1 min = 60sec
            Serial.print("(Sleeping : "); Serial.print(timeToSleep); Serial.println("seconds)");
            delay(100);
            LoRa_Utils::wakeRadio();
            esp_light_sleep_start();
        }
    }

    void checkSerial() {
        if (Config.digi.ecoMode == 1) Serial.end();
    }

}