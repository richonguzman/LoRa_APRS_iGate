#include <Arduino.h>
#include <WiFi.h>
#include "config.h"

WiFiClient espClient;
uint32_t lastTxTime = 0;

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

void connect_and_upload_to_APRS_IS(String packet) {
  int count = 0;
  String aprsauth;
  Serial.println("Conectando a APRS-IS");
  while (!espClient.connect(SERVER.c_str(), APRSPORT) && count < 20) {
    Serial.println("Didn't connect with server: " + String(SERVER) + " APRSPORT: " + String(APRSPORT));
    delay(1000);
    espClient.stop();
    espClient.flush();
    Serial.println("Run client.stop");
    Serial.println("Trying to connect with server: " + String(SERVER) + " APRSPORT: " + String(APRSPORT));
    count++;
    Serial.println("Try: " + String(count));
  }
  if (count == 20) {
    Serial.println("Tried: " + String(count) + " don't send the packet!");
  } else {
    Serial.println("Connected with server: " + String(SERVER) + " APRSPORT: " + String(APRSPORT));
    while (espClient.connected()) {
      delay(200);
      aprsauth = "user " + iGate_Callsign + " pass " + passcode_igate + "\n"; //info igate
      espClient.write(aprsauth.c_str());         //Serial.println("Run client.connected()");
      delay(200);
      espClient.write(packet.c_str());           //Serial.println("Send client.write=" + aprsauth);
      delay(200);                                //Serial.println("Send espClient.write = " + packet_para_APRS_IS);
      Serial.println("Packet uploaded =)\n");
      espClient.stop();
      espClient.flush();                         //Serial.println("(Telnet client disconnect)\n");
    }
  }
}

void procesa_y_sube_APRS_IS(String mensaje) {
  String packet_para_APRS_IS = "";
  String callsign_y_path_tracker = "";
  String payload_tracker;

  int posicion_dos_puntos = mensaje.indexOf(':');
  callsign_y_path_tracker = mensaje.substring(3, posicion_dos_puntos);
  payload_tracker = mensaje.substring(posicion_dos_puntos);
  packet_para_APRS_IS = callsign_y_path_tracker + ",qAO," + iGate_Callsign + payload_tracker + "\n";
  //Serial.print("Mensaje APRS_IS    : "); Serial.println(packet_para_APRS_IS);
  //packet = "CD2RXU-9>APLT00,qAO,CD2RXU-10:=" + LAT + "/" + LON + ">" +  "\n"; // ejemplo!!!
  connect_and_upload_to_APRS_IS(packet_para_APRS_IS);
}

void APRS_connect(){
  int count = 0;
  String aprsauth;
  Serial.println("Conectando a APRS-IS");
  while (!espClient.connect(SERVER.c_str(), APRSPORT) && count < 20) {
    Serial.println("Didn't connect with server: " + String(SERVER) + " APRSPORT: " + String(APRSPORT));
    delay(1000);
    espClient.stop();
    espClient.flush();
    Serial.println("Run client.stop");
    Serial.println("Trying to connect with server: " + String(SERVER) + " APRSPORT: " + String(APRSPORT));
    count++;
    Serial.println("Try: " + String(count));
  }
  if (count == 20) {
    Serial.println("Tried: " + String(count) + " don't send the packet!");
  } else {
    Serial.println("Connected with server: " + String(SERVER) + " APRSPORT: " + String(APRSPORT));
    
    //aprsauth = "user " + iGate_Callsign + " pass " + passcode_igate + " vers " + "ESP32_TEST" + " " + "0.2" + " filter " + "r/-33.034/-70.573/50 t/ms" + "\n\r"; //info igate
    aprsauth = "user " + iGate_Callsign + " pass " + passcode_igate + " vers " + "ESP32_TEST" + " " + "0.2" + " filter " + "t/poms/CD2RXU-10/50" + "\n\r"; //info igate
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
  Serial.println("Starting iGate\n");
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
    connect_and_upload_to_APRS_IS(iGateBeaconPacket);
    lastTxTime = millis();
    beacon_update = false;
  }*/
}