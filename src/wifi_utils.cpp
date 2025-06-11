
#include "board_pinout.h"
#include "wifi_utils.h"
#include "display.h"
#include "utils.h"

#include <HTTPClient.h>

bool isInternetAvailable() {
    HTTPClient http;
    http.begin("http://clients3.google.com/generate_204");
    int httpCode = http.GET();
    http.end();
    return (httpCode == 204);
}

uint32_t lastInternetCheck = 0;
const uint32_t internetCheckInterval = 5 * 60 * 1000; // 5 minuta

void WIFI_Utils::checkWiFi() {
    if (!Config.digi.ecoMode) {
        // Provjera interneta iako je WiFi povezan
        if (!backUpDigiMode && (WiFi.status() == WL_CONNECTED) && ((millis() - lastInternetCheck) >= internetCheckInterval)) {
            lastInternetCheck = millis();
            if (!isInternetAvailable()) {
                Serial.println("*** Internet NOT available despite WiFi connected! Activating Backup Digi Mode ***");
                backUpDigiMode = true;
                lastBackupDigiTime = millis();
            } else {
                Serial.println("*** Internet available ***");
            }
        }
    }

    if (WiFi.status() != WL_CONNECTED && millis() - lastWiFiCheck >= 30000) {
        lastWiFiCheck = millis();
        if (wifiTryCount < 2) {
            Serial.println("Reconnecting WiFi...");
            connectToWiFi();
            wifiTryCount++;
        } else {
            Serial.println("No WiFi - Activating Backup Digi Mode");
            backUpDigiMode = true;
            lastBackupDigiTime = millis();
        }
    }

    if (WiFi.status() == WL_CONNECTED && backUpDigiMode) {
        Serial.println("WiFi reconnected - Deactivating Backup Digi Mode");
        backUpDigiMode = false;
    }
}
