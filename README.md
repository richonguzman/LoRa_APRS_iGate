# Richonguzman / CD2RXU LoRa APRS iGate/Digirepeater

This next generation LoRa iGate can work as:
- pure RX-iGate, 
- Rx+Tx-iGate and distribute messages and weather forecasts to heard trackers, and 
- Digipeater in simplex or split-frequency environment.

In all configurations the display shows the current stationMode, heard packets and events the iGate is currently performing.

But under the hood is much more:

- Sending events to remote syslog server.
- OTA update capability (for Firmware and Filesystem).
- RX first, TX will only be done if there is no traffic on the frequency.
- automatic update of the Lora symbol at APRS-IS, black "L" for pure RX, red "L" for TX capability and green "L" for digipeater.
- support for multiple WLAN with corresponding coordinates.
- Syslog capability.

and more will come:
- Web-UI
- support for BME280 sensors, sending to APRS-IS
- ...

__________________________________________

See also the software for LoRa Tracker https://github.com/richonguzman/LoRa_APRS_Tracker

__________________________________________


Instructions (add your information into the '/data/igate_conf.json'):

a) Change "callsign" from "NOCALL-10" to your CALLSIGN + SSID.

b) Choose _stationMode_:

    1 = RX iGate, black "L" as symbol

    2 = Rx + TX iGate, red "L" as symbol, HAM only. RX will be sent to APRS-IS, Messages will be sent via Lora. Same frequency for RX and TX. By using this feature you have comply with the regulations of your country.

    3 = Digipeater simplex, green "L" as symbol, HAM only. Received packets containing WIDEx-x in path will be digipeated on the same frequency. By using this feature you have comply with the regulations of your country.

    4 = Digipeater split frequency, green "L" as symbol, HAM only. Received packets will be digipeated on a different frequency. Frequency separation must be 125kHz or more. By using this feature you have comply with the regulations of your country.

    IgateComment and DigirepeaterComment will be sent to APRS-IS or via RF, depending on your stationmode

c) WiFi section: adjust SSID and Password to you WiFi, add the GPS to "Latitude" and "Longitude" (info from GoogleMaps) of your new LoRa iGate. (If stationMode 3 or 4 selected, add also GPS info to Digirepeater Section).

d) APRS_IS section, change "passcode" from "VWXYZ" to yours (remember that is 5 digits integer) and choose a server close to your location (see https://www.aprs2.net/)

e) LORA section, 

    adjust TX frequency and RX frequency matching your stationmode and country. Remember,

        at stationmode 1, 2, and 3, RX and TX frequency shall be set to 433775000 (443.775MHz, deviations possible, depending on your country) 

        at stationmode 4, RX frequency shall be set to 433775000, TX frequency shall be set to 433900000 (deviations possible, depending on your country). There must be a frequency separation of 125kHz or more. 
    
    adjust power to your need, valid values are from 1 to 20

f) Syslog section:
    
    adjust server, port to a suitable value.

__________________________________________

LoRa APRS iGATE/Digirepeater working on this boards:
- LILYGO ESP32 LoRa32 v2-1-1.6
- ESP32 Wroom Dev +  SX1278 LoRa Module for a DIY Version
- HELTEC_WIFI_LORA_32_V2 (add "#define OLED_RESET 16" on "pins_config.h")
__________________________________________
Versions:
- 2023.02.10 First Beta (receiving LoRa Beacon/Packets and uploading to APRS-IS).
- 2023.02.17 Receiving Feed from APRS-IS.
- 2023.03.01 Tx Packet from APRS-IS to nearby LoRa Stations.
- 2023.05.12 Tx Packet from APRS-IS to LoRa-RF Correction.
- 2023.05.19 Saving Last-Heard Stations for validating Tx Responses.
- 2023.05.23 Processing Query's from RF/LoRa or APRS-IS (Send "Help" Message to test).
- 2023.06.06 Full repack of Code and adding _enableTx_ only for Ham Ops.
- 2023.06.08 Adding Digirepeater Functions.
- 2023.06.10 OTA for Firmware and Filesystem.
- 2023.06.12 Syslog added.
__________________________________________

# Hope You Enjoy this, 73 !!  CD2RXU , Valparaiso, Chile

