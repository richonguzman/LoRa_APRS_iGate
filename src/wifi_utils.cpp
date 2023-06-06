#include <WiFi.h>
#include "wifi_utils.h"
#include "configuration.h"
#include "display.h"

extern Configuration  Config;
extern WiFi_AP        *currentWiFi;
extern int            myWiFiAPIndex;
extern int            myWiFiAPSize;

namespace WIFI_Utils {

void setup() {
  int status = WL_IDLE_STATUS;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(500);
  unsigned long start = millis();
  show_display("", "Connecting to Wifi:", currentWiFi->ssid + " ...", 0);
  Serial.print("\nConnecting to '"); Serial.print(currentWiFi->ssid); Serial.println("' WiFi ...");
  WiFi.begin(currentWiFi->ssid.c_str(), currentWiFi->password.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
    if ((millis() - start) > 15000){
      if(myWiFiAPIndex >= (myWiFiAPSize-1)) {
        myWiFiAPIndex = 0;
      } else {
        myWiFiAPIndex++;
      }
      currentWiFi = &Config.wifiAPs[myWiFiAPIndex];
      start = millis();
      Serial.print("\nConnect to WiFi '"); Serial.print(currentWiFi->ssid); Serial.println("' ...");
      show_display("", "Connect to Wifi:", currentWiFi->ssid + " ...", 0);
      WiFi.begin(currentWiFi->ssid.c_str(), currentWiFi->password.c_str());
    }
  }
  Serial.print("Connected as ");
  Serial.println(WiFi.localIP());
}

}