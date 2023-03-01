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

String getDateTime() {
  struct tm timeinfo;
  String currentTime, year, month, day, hour, minute, seconds;  
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return "no time info... restarting";
    ESP.restart();
  }

  year = (String)(timeinfo.tm_year + 1900);

  month = (String)(timeinfo.tm_mon+1);
  if (month.length() == 1) {
    month = "0" + month;
  }

  day = (String)(timeinfo.tm_mday);
  if (day.length() == 1) {
    day = "0" + day;
  }

  hour = (String)(timeinfo.tm_hour);
  if (hour == 0) {
    hour = "00";
  } else if (hour.length() == 1) {
    hour = "0" + hour;
  }

  minute = (String)(timeinfo.tm_min);
  if (minute == 0) {
    minute = "00";
  } else if (minute.length() == 1) {
    minute = "0" + minute;
  }

  seconds = (String)(timeinfo.tm_sec);
  if (seconds == 0) {
    seconds = "00";
  } else if (seconds.length() == 1) {
    seconds = "0" + seconds;
  }

  currentTime = year + "/" + month + "/" + day + " " + hour + ":" + minute + ":" + seconds;
  //Serial.println(currentTime);
  return currentTime;
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  btStop();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  getDateTime();
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
              ackMessage = "WRCLP>APRS,TCPIP*,qAC,CHILE::" + questioner + ":ack" + ackNumber + "\n";
              //Serial.print("---> " + ackMessage);
              espClient.write(ackMessage.c_str());
              delay(500);
            }
            currentDate = getDateTime();
            answerMessage = "WRCLP>APRS,TCPIP*,qAC,CHILE::" + questioner + ":" + "hola, " + questioner + " " + currentDate + "\n";  
            Serial.print("\n-------> " + answerMessage + "\n");
            espClient.write(answerMessage.c_str());          
          }
        }
      }
    }
  }
}