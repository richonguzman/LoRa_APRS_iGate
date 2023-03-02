#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "time.h"
#include "config.h"

#define VERSION "V.0.0.9" // 2023.03.02

WiFiClient espClient;
HTTPClient http;
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

String getWeatherForecast(String infoJson){
  StaticJsonDocument<700> doc;
  DeserializationError error = deserializeJson(doc, infoJson);
  if (error) {        // Test if parsing succeeds.
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return "No Location info for Callsign";
  }
  String latitude = doc["entries"][0]["lat"];
  String longitude = doc["entries"][0]["lng"];
  latitude = latitude.substring(0,latitude.indexOf(".")+3);
  longitude = longitude.substring(0,longitude.indexOf(".")+3);

  //openweather api key
  String OpenWeatherApiKey;
  OpenWeatherApiKey = "ac33704cb73d416103ede5e5d86aabd5";
  //OpenWeatherApiKey = "d732610ae742d5d2d057c10825e3e7eb";
  String request_info;
  request_info = "https://api.openweathermap.org/data/2.5/weather?lat=" + (String)latitude + "&lon=" + (String)longitude + "&appid=" + OpenWeatherApiKey + "&units=metric";

    String payload;
  int httpResponseCode;
  
  http.begin(request_info.c_str());
  httpResponseCode =  http.GET();
  if (httpResponseCode > 0) {                   //    Serial.print("HTTP Response code: ");      //Serial.println(httpResponseCode);
    payload = http.getString();                 //    Serial.println(payload);
  } else {                                      //    Serial.print("Error code: ");             //Serial.println(httpResponseCode);
    payload = "0";
  }
  http.end();

  if (payload != "0") {
    StaticJsonDocument<1000> doc2;
    DeserializationError error2 = deserializeJson(doc2, payload);
    if (error2) {        // Test if parsing succeeds.
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error2.f_str());
      return "No WeatherForcast Available";
    }

    String resumen      = doc2["weather"][0]["description"];
    String temperatura  = doc2["main"]["temp"];
    String presion      = doc2["main"]["pressure"];
    String humedad      = doc2["main"]["humidity"];
    String viento       = doc2["wind"]["speed"];
    String lugar        = doc2["name"];
    String respuesta    = lugar + ", " + resumen + ",T:" + temperatura.substring(0,temperatura.indexOf(".")) + ",P:" + presion + ",H:" + humedad + ",W:" + viento.substring(0,viento.indexOf("."));
    return respuesta;
  }
}

String GetWeatherForecast(String callsign) {
  String request_info, payload, weatherForecast;
  int httpResponseCode;
  callsign.trim();
  request_info = "https://api.aprs.fi/api/get?name=" + callsign + "&what=loc&apikey=" + APRS_API_KEY + "&format=json";
  http.begin(request_info.c_str());
  httpResponseCode =  http.GET();
  if (httpResponseCode > 0) {           //Serial.print("HTTP Response code: ");    //Serial.println(httpResponseCode);
    payload = http.getString();         //Serial.println(payload);
  } else {
    //Serial.print("Error code: ");     //Serial.println(httpResponseCode);
    payload = "0";
  }
  http.end();
  if (payload != "0") {
    weatherForecast = getWeatherForecast(payload);
    return weatherForecast;
  } else {
    return "No WeatherForcast Available";
  }
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
        Serial.println("\n---- Sending WeatherReport Beacon ----\n");
        espClient.write(WeatherReportBeaconPacket.c_str()); 
        lastTxTime = millis();
        beacon_update = false;
      }
      
      String aprsisData, packet, subpacket1, receivedMessage, questioner, answerMessage, ackNumber, ackMessage, currentDate, weatherForecast;

      aprsisData = espClient.readStringUntil('\n');
      packet.concat(aprsisData);
      if (!packet.startsWith("#")){
        Serial.println(packet);
        if (packet.indexOf("WRCLP") > 0){
          if (packet.indexOf("::")>0) {
            subpacket1 = packet.substring(packet.indexOf("::")+2);
            receivedMessage = subpacket1.substring(subpacket1.indexOf(":")+1);
            questioner = packet.substring(0,packet.indexOf(">"));
            //Serial.println(receivedMessage);

            if (receivedMessage.indexOf("{")>0) {                                  // if questioner solicitates ack 
              ackNumber = receivedMessage.substring(receivedMessage.indexOf("{")+1);
              for(int i = questioner.length(); i < 9; i++) {
                questioner += ' ';
              }
              ackMessage = "WRCLP>APRS,TCPIP*,qAC,CHILE::" + questioner + ":ack" + ackNumber + "\n";
              //Serial.print("---> " + ackMessage);
              espClient.write(ackMessage.c_str());
              delay(500);
              receivedMessage = receivedMessage.substring(0,receivedMessage.indexOf("{"));
              //Serial.println(receivedMessage);
            }

            for(int i = questioner.length(); i < 9; i++) {
                questioner += ' ';
            }
            receivedMessage.trim();
            if (receivedMessage == "hora") {
              currentDate = getDateTime();
              answerMessage = "WRCLP>APRS,TCPIP*,qAC,CHILE::" + questioner + ":" + currentDate + "\n";
            } else if (receivedMessage == "clima") {
              weatherForecast = GetWeatherForecast(questioner);
              answerMessage = "WRCLP>APRS,TCPIP*,qAC,CHILE::" + questioner + ":" + weatherForecast + "\n";  
            } else {
              answerMessage = "WRCLP>APRS,TCPIP*,qAC,CHILE::" + questioner + ":" + "hola " + questioner + "\n";  
            }

            Serial.print("\n-------> " + answerMessage + "\n");
            espClient.write(answerMessage.c_str());          
          }
        }
      }
    }
  }
}