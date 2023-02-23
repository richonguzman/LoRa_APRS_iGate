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
  String aprsisData = "";
  String packet = "";
  String mensaje1 = "";
  String mensaje2 = "";
  String emisario = "";
  String mensajeRespuesta;


  while (espClient.connected()) {
    while (espClient.available()) {
      uint32_t lastTx = millis() - lastTxTime;
      if (lastTx >= BeaconInterval) {
        beacon_update = true;    
      }

      if (beacon_update) { 
        Serial.println("enviando Beacon Estacion/iGate");
        espClient.write(WeatherReportBeaconPacket.c_str()); 
        lastTxTime = millis();
        beacon_update = false;
      }




      aprsisData = espClient.readStringUntil('\n');       //char c = espClient.read();      //if (c == '\n') {
      Serial.println(aprsisData);      //Serial.println(aprsisData.indexOf("CD2RXU-9"));
      packet.concat(aprsisData);
      if (packet.indexOf("CD2RXU-10") > 0){
        if (packet.indexOf("::")>0) {
          mensaje1 = packet.substring(packet.indexOf("::")+2);
          mensaje2 = mensaje1.substring(mensaje1.indexOf(":")+1);
          emisario = packet.substring(0,packet.indexOf(">"));
          Serial.print("--> es un mensaje para CD2RXU-10 = ");
          Serial.println(mensaje2);
          Serial.print("--> enviado por : ");
          Serial.println(emisario);
          for(int i = emisario.length(); i < 9; i++) {
            emisario += ' ';
          }
          mensajeRespuesta = "CD2RXU-10>APLG01,TCPIP*,qAC,CHILE::" + emisario + ":" + "hola para ti tambien5" + "\n";  
          Serial.print(mensajeRespuesta);
          espClient.write(mensajeRespuesta.c_str());
          
        }
      }
      aprsisData = "";
      packet = "";
      mensaje1 = "";
      mensaje2 = "";
      emisario = "";      //}      //aprsisData += c;      
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