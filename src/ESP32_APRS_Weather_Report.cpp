#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include "config.h"


WiFiClient espClient;
uint32_t lastTxTime                 = 0;
static bool beacon_update           = true;
unsigned long previousWiFiMillis    = 0;

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

void APRS_IS_connect(){
  int count = 0;
  String aprsauth;
  Serial.println("Connecting to APRS-IS ...");
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

String GetTime() {
  struct tm timeinfo;
  String currentTime, year, month, day, hour, minute, seconds;  
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return "no time info... restarting";
    ESP.restart();
  }
  year = (String)(timeinfo.tm_year + 1900);
  month = (String)(timeinfo.tm_mon+1);
  day = (String)(timeinfo.tm_mday);
  hour = (String)(timeinfo.tm_hour);
  minute = (String)(timeinfo.tm_min);
  seconds = (String)(timeinfo.tm_sec);
  currentTime = year + "/" + month + "/" + day + " " + hour + ":" + minute + ":" + seconds;
  //Serial.println(currentTime);
  return currentTime;
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  btStop();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  GetTime();
  Serial.println("Starting Weather Report APRS\n");
}

void loop() {
  unsigned long currentWiFiMillis   = millis();

  if ((WiFi.status() != WL_CONNECTED) && (currentWiFiMillis - previousWiFiMillis >= WifiCheckInterval)) {   // if WiFi is down, try reconnecting
    Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    previousWiFiMillis = currentWiFiMillis;
  }

  if (!espClient.connected()) {
    APRS_IS_connect();
  }
  
  while (espClient.connected()) {
    while (espClient.available()) {
      uint32_t lastTx = millis() - lastTxTime;
      
      if (lastTx >= BeaconInterval) {
        beacon_update = true;    
      }
      if (beacon_update) { 
        Serial.println("---- Sending WeatherReport Beacon ----");
        espClient.write(WeatherReportBeaconPacket.c_str()); 
        lastTxTime = millis();
        beacon_update = false;
      }
      
      String aprsisData, packet, subpacket1, subpacket2, questioner, answerMessage, ackNumber, ackMessage, currentDate;

      aprsisData = espClient.readStringUntil('\n');
      packet.concat(aprsisData);
      if (!packet.startsWith("#")){
        Serial.println(packet);
        if (packet.indexOf("WRCLP") > 0){
          if (packet.indexOf("::")>0) {
            subpacket1 = packet.substring(packet.indexOf("::")+2);
            subpacket2 = subpacket1.substring(subpacket1.indexOf(":")+1);
            questioner = packet.substring(0,packet.indexOf(">"));
            for(int i = questioner.length(); i < 9; i++) {
              questioner += ' ';
            }
            if (subpacket2.indexOf("{")>0) {                                  // if questioner solicitates ack 
              ackNumber = subpacket2.substring(subpacket2.indexOf("{")+1);
              ackMessage = "WRCLP>APLG01,TCPIP*,qAC,CHILE::" + questioner + ":ack" + ackNumber + "\n";
              Serial.print("---> " + ackMessage);
              espClient.write(ackMessage.c_str());
              delay(500);
            }
            currentDate = GetTime();
            answerMessage = "WRCLP>APLG01,TCPIP*,qAC,CHILE::" + questioner + ":" + "hola, " + questioner + " " + currentDate + "\n";  
            Serial.print("-------> " + answerMessage);
            espClient.write(answerMessage.c_str());          
          }
        }
      }
    }
  }
}