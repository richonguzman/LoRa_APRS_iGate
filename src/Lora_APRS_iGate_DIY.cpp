#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include "iGate_config.h"
#include "pins_config.h"

WiFiClient espClient;
int status = WL_IDLE_STATUS;
uint32_t lastTxTime = 0;

#define txInterval 15 * 60 * 1000
const String LAT = "3302.03S";      // por corregir               // write your latitude
const String LON = "07134.42W";     //por corregir   
const String IGATE = "CD2RXU-10";
const String Mensaje_iGate = "DIY ESP32 - LoRa APRS iGATE";


void setup_lora() {
  Serial.println("Set LoRa pins!");
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ); 

  long freq = 433775000;
  Serial.print("frequency: ");
  Serial.println(String(freq));
  if (!LoRa.begin(freq)) {
    Serial.println("Starting LoRa failed!");
    while (true) {
    }
  }
  LoRa.setSpreadingFactor(12);
  LoRa.setSignalBandwidth(125000);
  LoRa.setCodingRate4(5);
  LoRa.enableCrc();
  LoRa.setTxPower(20);
  Serial.println("LoRa init done!\n");
}

void setup_wifi() {
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
      delay(1000);
      aprsauth = "user " + callsign_igate + " pass " + passcode_igate + "\n"; //info igate
      espClient.write(aprsauth.c_str());         //Serial.println("Run client.connected()");
      delay(500);
      espClient.write(packet.c_str());           //Serial.println("Send client.write=" + aprsauth);
      delay(500);                                //Serial.println("Send espClient.write = " + packet_para_APRS_IS);
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
  packet_para_APRS_IS = callsign_y_path_tracker + ",qAO," + callsign_igate + payload_tracker + "\n";
  //Serial.print("Mensaje APRS_IS    : "); Serial.println(packet_para_APRS_IS);
  //packet = "CD2RXU-9>APLT00,qAO,CD2RXU-10:=" + LAT + "/" + LON + ">" +  "\n"; // ejemplo!!!
  connect_and_upload_to_APRS_IS(packet_para_APRS_IS);
}

void valida_y_procesa_packet(String mensaje) {
  String packetStart = "";
  Serial.print("MENSAJE RECIBIDO!!!   ");
  Serial.print("(Validando inicio ---> ");
  packetStart = mensaje.substring(0, 3);
  if (packetStart == "\x3c\xff\x01") {
    Serial.println("Packet Valido)");
    procesa_y_sube_APRS_IS(mensaje);
  } else {
    Serial.println("Packet NO Valido)");
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  btStop();
  setup_lora();
  Serial.println("Starting iGate\n");
}

void loop() {  
  String mensaje_recibido = "";
  String mensaje_beacon_estacion = IGATE + ">APLG01,TCPIP*,qAC,T2BRAZIL:=" + LAT + "L" + LON + "&" +  Mensaje_iGate+ "\n"; //
  uint32_t lastTx = millis() - lastTxTime;
  bool valida_inicio;
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    while (LoRa.available()) {
      int inChar = LoRa.read();
      mensaje_recibido += (char)inChar;
    }
    valida_y_procesa_packet(mensaje_recibido);          //Serial.println("Mensaje Recibido   : " + String(mensaje_recibido));
  }
  if (lastTx >= txInterval) {
    Serial.println("enviando Beacon Estacion/iGate");
    connect_and_upload_to_APRS_IS(mensaje_beacon_estacion);
    lastTxTime = millis();
		}
}