#include <WiFi.h>
#include "configuration.h"
#include "pins_config.h"
#include "wifi_utils.h"
#include "display.h"

extern Configuration  Config;
extern WiFi_AP        *currentWiFi;
extern int            myWiFiAPIndex;
extern int            myWiFiAPSize;
extern int            stationMode;
extern uint32_t       previousWiFiMillis;

namespace WIFI_Utils {

void checkWiFi() {
  if ((WiFi.status() != WL_CONNECTED) && ((millis() - previousWiFiMillis) >= 30*1000)) {
    Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    previousWiFiMillis = millis();
  }
}

void startWiFi() {
  int status = WL_IDLE_STATUS;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(500);
  unsigned long start = millis();
  show_display("", "", "Connecting to Wifi:", "", currentWiFi->ssid + " ...", 0);
  Serial.print("\nConnecting to WiFi '"); Serial.print(currentWiFi->ssid); Serial.println("' ...");
  WiFi.begin(currentWiFi->ssid.c_str(), currentWiFi->password.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    digitalWrite(greenLed,HIGH);
    Serial.print('.');
    delay(500);
    digitalWrite(greenLed,LOW);
    if ((millis() - start) > 15000){
      delay(1000);
      if(myWiFiAPIndex >= (myWiFiAPSize-1)) {
        myWiFiAPIndex = 0;
      } else {
        myWiFiAPIndex++;
      }
      currentWiFi = &Config.wifiAPs[myWiFiAPIndex];
      start = millis();
      Serial.print("\nConnecting to WiFi '"); Serial.print(currentWiFi->ssid); Serial.println("' ...");
      show_display("", "", "Connecting to Wifi:", "", currentWiFi->ssid + " ...", 0);
      WiFi.disconnect();
      WiFi.begin(currentWiFi->ssid.c_str(), currentWiFi->password.c_str());
    }
  }
  digitalWrite(greenLed,LOW);
  Serial.print("Connected as ");
  Serial.println(WiFi.localIP());
  show_display("", "", "     Connected!!", "" , "     loading ...", 1000);
}

void startWiFi2() {
  int wifiCounter = 0;
  int status = WL_IDLE_STATUS;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(500);
  unsigned long start = millis();
  show_display("", "", "Connecting to Wifi:", "", currentWiFi->ssid + " ...", 0);
  Serial.print("\nConnecting to WiFi '"); Serial.print(currentWiFi->ssid); Serial.println("' ...");
  WiFi.begin(currentWiFi->ssid.c_str(), currentWiFi->password.c_str());
  while (WiFi.status() != WL_CONNECTED && wifiCounter<2) {
    delay(500);
    digitalWrite(greenLed,HIGH);
    Serial.print('.');
    delay(500);
    digitalWrite(greenLed,LOW);
    if ((millis() - start) > 10000){
      delay(1000);
      if(myWiFiAPIndex >= (myWiFiAPSize-1)) {
        myWiFiAPIndex = 0;
        wifiCounter++;
      } else {
        myWiFiAPIndex++;
      }
      currentWiFi = &Config.wifiAPs[myWiFiAPIndex];
      start = millis();
      Serial.print("\nConnecting to WiFi '"); Serial.print(currentWiFi->ssid); Serial.println("' ...");
      show_display("", "", "Connecting to Wifi:", "", currentWiFi->ssid + " ...", 0);
      WiFi.disconnect();
      WiFi.begin(currentWiFi->ssid.c_str(), currentWiFi->password.c_str());
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(greenLed,LOW);
    Serial.print("Connected as ");
    Serial.println(WiFi.localIP());
    show_display("", "", "     Connected!!", "" , "     loading ...", 1000);
  } else {
    show_display("", "", " WiFi Not Connected!", "  DigiRepeater MODE" , "     loading ...", 2000);
  }
}

void setup() {
    if (stationMode == 1 || stationMode == 2) {
        if (stationMode==1) {
            Serial.println("stationMode ---> iGate (only Rx)");
        } else {
            Serial.println("stationMode ---> iGate (Rx + Tx)");
        }
        startWiFi();
        btStop();
    } else if (stationMode == 3 || stationMode == 4) {
        if (stationMode == 3) {
            Serial.println("stationMode ---> DigiRepeater (Rx freq == Tx freq)");
        } else {
            Serial.println("stationMode ---> DigiRepeater (Rx freq != Tx freq)");
        }
        WiFi.mode(WIFI_OFF);
        btStop();
    } else if (stationMode == 5) {
      Serial.println("stationMode ---> iGate when Wifi/APRS available (DigiRepeater when not)");
    } else { 
      Serial.println("stationMode ---> NOT VALID, check '/data/igate_conf.json'");
      show_display("------- ERROR -------", "stationMode Not Valid", "change it on : /data/", "igate_conf.json", 0);
      while (1);
    }
}

}