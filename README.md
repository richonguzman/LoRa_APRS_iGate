# Richonguzman / CA2RXU LoRa APRS iGate/Digirepeater

___________________________________________________
NO OTA FOR NOW!!!!


BAD NEWS : AsyncTCP library has a bug and gets this new and old firmware as NOT COMPILING STATUE, SO PLEASE WAIT, as soon as I can I will have it running again!
___________________________________________________



This firmware is for using ESP32 based boards with LoRa Modules and GPS to live in the APRS world.

![Screenshot](https://github.com/richonguzman/LoRa_APRS_iGate/blob/main/images/OledScreen.jpeg)

__(NOTE: This iGate Firmware was develop to work with all LoRa APRS Trackers and specially with this firmware <a href="https://github.com/richonguzman/LoRa_APRS_Tracker" target="_blank">LoRa APRS Tracker</a>)__

___________________________________________________

## You can support this project to continue to grow:

[<img src="https://github.com/richonguzman/LoRa_APRS_Tracker/blob/main/images/github-sponsors.png">](https://github.com/sponsors/richonguzman)     [<img src="https://github.com/richonguzman/LoRa_APRS_Tracker/blob/main/images/paypalme.png">](http://paypal.me/richonguzman)

____________________________________________________

This next generation LoRa iGate can work as:
- pure RX-iGate, 
- Rx+Tx-iGate and distribute messages and weather forecasts to heard trackers, and 
- Digipeater in simplex or split-frequency environment.

In all configurations the display shows the current stationMode, heard packets and events the iGate is currently performing.

But under the hood is much more:

- Sending events to remote syslog server.
- OTA update capability (for Firmware and Filesystem).
- RX first, TX will only be done if there is no traffic on the frequency.
- automatic update of the Lora symbol at APRS-IS, black "L" for pure RX, red "L" for TX capability, green star "L" for digipeater and blue round "L" for WX iGate.
- support for multiple WLAN with corresponding coordinates.
- support for BME280 sensors, sending to WX data to APRS-IS.

and more will come:
- Web-UI
- ...

____________________________________________________

# WIKI

### 1. Installation Guide --> <a href="https://github.com/richonguzman/LoRa_APRS_iGate/wiki/1.-Installation-Guide" target="_blank">here</a>.

### 2. iGate Configuration and Explanation for each setting --> <a href="https://github.com/richonguzman/LoRa_APRS_iGate/wiki/2.-iGate-Configuration" target="_blank">here</a>.

### 3. Supported Boards and Environment Selection --> <a href="https://github.com/richonguzman/LoRa_APRS_iGate/wiki/3.-Supported-Boards-and-Environment-Selection" target="_blank">here</a>.

### 4. Upload Firmware and Filesystem --> <a href="https://github.com/richonguzman/LoRa_APRS_iGate/wiki/4.-Upload-Firmware-and-Filesystem" target="_blank">here</a>.

### 5. Adding BME280 Module --> <a href="https://github.com/richonguzman/LoRa_APRS_iGate/wiki/5.-Adding-BME280-Module" target="_blank">here</a>.

____________________________________________________
## Timeline (Versions):

- 2023.12.27 HELTEC V3 board support added.
- 2023.12.26 Added BME680 module to BME/BMP280 modules supported.
- 2023.12.20 Updated to ElegantOTA v.3 (AsyncElegantOTA was deprecated).
- 2023.12.07 MIC-E process and syslog added.
- 2023.12.06 HELTEC V2 board support added.
- 2023.11.26 Small correction to enable Syslog in stationMode5.
- 2023.10.09 Added "WIDE1-1" to Tx packets from iGate to be *repeated* by Digirepeaters.
- 2023.10.09 Added Support also for BMP280 module.
- 2023.10.08 Added Serial Comunication with PinPoint APRS app (https://www.pinpointaprs.com)
- 2023.08.20 Added External Voltage Measurement (Max 15V!)
- 2023.08.05 Ground Height Correction for Pressure readings added.
- 2023.07.31 StationMode5 added: iGate when WiFi and APRS available, DigiRepeater when not.
- 2023.07.16 Small OTA, BME module update.
- 2023.07.05 Adding monitor info of Battery connected.
- 2023.06.18 Info on Oled Screen mayor update, added RSSI and Distance to Listened Station.
- 2023.06.17 Support for BME280 Module (Temperature, Humidity, Pressure) added.
- 2023.06.12 Syslog added.
- 2023.06.10 OTA update support for Firmware and Filesystem.
- 2023.06.08 Adding Digirepeater Functions.
- 2023.06.06 Full repack of Code and adding _enableTx_ only for Ham Ops.
- 2023.05.23 Processing Query's from RF/LoRa or APRS-IS (Send "Help" Message to test).
- 2023.05.19 Saving Last-Heard Stations for validating Tx Responses.
- 2023.05.12 Tx Packet from APRS-IS to LoRa-RF Correction.
- 2023.03.01 Tx Packet from APRS-IS to nearby LoRa Stations.
- 2023.02.17 Receiving Feed from APRS-IS.
- 2023.02.10 First Beta (receiving LoRa Beacon/Packets and uploading to APRS-IS).

____________________________________________________


Instructions (add your information into the '/data/igate_conf.json'):

a) Change _callsign_ from "NOCALL-10" to your CALLSIGN + SSID.

b) Choose _stationMode_:

    1 = RX iGate, black "L" as symbol

    2 = Rx + TX iGate, red "L" as symbol, HAM only. RX will be sent to APRS-IS, Messages will be sent via Lora. Same frequency for RX and TX. By using this feature you have comply with the regulations of your country.

    3 = Digipeater simplex, green "L" as symbol, HAM only. Received packets containing WIDEx-x in path will be digipeated on the same frequency. By using this feature you have comply with the regulations of your country.

    4 = Digipeater split frequency, green "L" as symbol, HAM only. Received packets will be digipeated on a different frequency. Frequency separation must be 125kHz or more. By using this feature you have comply with the regulations of your country.

    IgateComment and DigirepeaterComment will be sent to APRS-IS or via RF, depending on your stationmode

c) WiFi section: 

    adjust SSID and Password to you WiFi, add the GPS to "Latitude" and "Longitude" (info from GoogleMaps) of your new LoRa iGate. (If stationMode 3 or 4 selected, add also GPS info to Digirepeater Section).

d) APRS_IS section: 

    change "passcode" from "VWXYZ" to yours (remember that is 5 digits integer) and choose a server close to your location (see https://www.aprs2.net/)

e) LORA section:

    adjust TX frequency and RX frequency matching your stationmode and country. Remember,

        at stationmode 1, 2, and 3, RX and TX frequency shall be set to 433775000 (443.775MHz, deviations possible, depending on your country) 

        at stationmode 4, RX frequency shall be set to 433775000, TX frequency shall be set to 433900000 (deviations possible, depending on your country). There must be a frequency separation of 125kHz or more. 
    
    adjust power to your need, valid values are from 1 to 20

f) Syslog section:
    
    adjust server and port to a suitable value if needed.

g) BME section:

    adjust to "active" if BME280 sensor connected through I2C pins


__________________________________________

Special Thanks to the help in testing and developing to Manfred (DC2MH) , for showing me the "way of good coding" to Tihomir (CA3TSK) and much more Ham Licence Ops all over the world.

# Hope You Enjoy this, 73 !!  CA2RXU , Valparaiso, Chile