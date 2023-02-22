#include <Arduino.h>
#include <WiFi.h>
#include "config.h"

WiFiClient espClient;
uint32_t lastTxTime = 0;
static bool beacon_update = true;

void setup_wifi() {
  int status = WL_IDLE_STATUS;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("\nConnecting to '"); Serial.print(WIFI_SSID); Serial.println("' WiFi ...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.print("Connected as ");
  Serial.println(WiFi.localIP());
}

void APRS_connect(){
  int count = 0;
  String aprsauth;
  Serial.println("Conectando a APRS-IS");
  while (!espClient.connect(AprsServer.c_str(), AprsServerPort) && count < 20) {
    Serial.println("Didn't connect with server...");
    delay(1000);
    espClient.stop();
    espClient.flush();
    Serial.println("Run client.stop");
    Serial.println("Trying to connect with Server: " + String(AprsServer) + " AprsServerPort: " + String(AprsServerPort));
    count++;
    Serial.println("Try: " + String(count));
  }
  if (count == 20) {
    Serial.println("Tried: " + String(count) + " FAILED!");
  } else {
    Serial.println("Connected with Server: " + String(AprsServer) + " Port: " + String(AprsServerPort));
    aprsauth = "user " + WeatherReportCallsign + " pass " + WeatherReportPasscode + " vers " + AprsSoftwareName + " " + AprsSoftwareVersion + " filter " + AprsFilter + "\n\r"; 
    espClient.write(aprsauth.c_str());  
    delay(200);
  }
}

void APRS_IS_READ(){
  String aprsisData;
  while (espClient.connected()) {
    while (espClient.available() > 0) {
      char c = espClient.read();
      if (c == '\n') {
        Serial.print(aprsisData);
        aprsisData = "";
      }
      aprsisData += c;
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  btStop();
  Serial.println("Starting Weather Report APRS\n");
  APRS_connect();
}

void loop() {
  APRS_IS_READ();



  /*uint32_t lastTx = millis() - lastTxTime;
  if (lastTx >= BeaconInterval) {
    beacon_update = true;    
  }

  if (beacon_update) { 
    Serial.println("enviando Beacon Estacion/iGate");
    espClient.write(WeatherReportBeaconPacket.c_str()); 
    lastTxTime = millis();
    beacon_update = false;
  }*/
}