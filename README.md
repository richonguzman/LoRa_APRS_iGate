# Richonguzman / CD2RXU LoRa APRS iGate/Digirepeater
# Firmware for Tx and Rx !!!

LoRa APRS iGATE/Digirepeater for:
- LILYGO ESP32 LoRa32 v2-1-1.6
- ESP32 Wroom Dev +  SX1278 LoRa Module for a DIY Version

__________________________________________

Achievements:
- Rx LoRa packets (up to 100kms away) and upload to APRS-IS servers.
- Tx LoRa packets from APRS-IS feed.
- Digirepeater Modes.
__________________________________________

Instrucctions (add your information into the '/data/igate_conf.json'):

1.- Change "callsign" from "NOCALL-10" to your CALLSIGN + SSID.

2.- Change "wifi">"AP">"SSID" from "WiFi_AP_1"  to your own WIFI-SSID-NAME.

3.- Change "wifi">"AP">"Password" from "password_WiFi_2" to your own WIFI-SSID-PASSWORD.

4.- Add GPS to "wifi">"AP">"Latitude" and "Longitude"  (info from GoogleMaps) of new LoRa iGate.

5.- Change "aprs_is">"passcode" from "XYZVW" to yours (remember that is 5 digits integer).

6.- Change "stationMode" value to other than 1 ONLY(!) if you are an valid Ham Operator.

__________________________________________

Digirepeater Modes:
1.- iGate (only Rx).

2.- iGate (Tx and Rx) HAM LICENSE REQUIRED!

3.- Digirepeater (Rx Freq = Tx Freq) HAM LICENSE REQUIRED!

4.- Digirepeater (Rx Freq != Tx Freq) HAM LICENSE REQUIRED! 

5.- iGate changes to Digirepeater when it looses APRS-IS+WiFi connection (on development).

__________________________________________
Versions:
- 2023.02.10 First Beta (receiving LoRa Beacon/Packets and uploading to APRS-IS).
- 2023.02.17 Receiving Feed from APRS-IS.
- 2023.03.01 Tx Packet from APRS-IS to nearby LoRa Stations.
- 2023.05.12 Tx Packet from APRS-IS to LoRa-RF Correction.
- 2023.05.19 Saving Last-Heard Stations for validating Tx Responses
- 2023.05.23 Processing Query's from RF/LoRa or APRS-IS (Send "Help" Message to test)
- 2023.06.06 Full repack of Code and adding _enableTx_ only for Ham Ops
- 2023.06.08 Adding Digirepeater Funtions
__________________________________________


Enjoy!!!

# Hope You Enjoy this, 73 !!  CD2RXU , Valparaiso, Chile

