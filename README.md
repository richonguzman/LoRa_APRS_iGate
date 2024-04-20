# Richonguzman / CA2RXU LoRa APRS iGate/Digirepeater

This firmware is for using ESP32 based boards with LoRa Modules and GPS to live in the APRS world.

![Screenshot](https://github.com/richonguzman/LoRa_APRS_iGate/blob/main/images/iGateOledScreen.jpeg)

__(NOTE: This iGate Firmware was develop to work with all LoRa APRS Trackers and specially with this firmware <a href="https://github.com/richonguzman/LoRa_APRS_Tracker" target="_blank">LoRa APRS Tracker</a>)__

___________________________________________________

## You can support this project to continue to grow:

[<img src="https://github.com/richonguzman/LoRa_APRS_Tracker/blob/main/images/github-sponsors.png">](https://github.com/sponsors/richonguzman)     [<img src="https://github.com/richonguzman/LoRa_APRS_Tracker/blob/main/images/paypalme.png">](http://paypal.me/richonguzman)

____________________________________________________

# WEB INSTALLER

As easy as it gets, the new <a href="https://sq2cpa.github.io/lora-flasher/ca2rxu.html" target="_blank">Web Installer</a> (thanks Damian SQ2CPA)

[<img src="https://github.com/richonguzman/LoRa_APRS_iGate/blob/main/images/WebFlasher.png">](https://sq2cpa.github.io/lora-flasher/ca2rxu.html)

____________________________________________________

# WIKI

### Supported Boards and buying links --> <a href="https://github.com/richonguzman/LoRa_APRS_iGate/wiki/Z.-------Supported-Boards-and-Buying-Links" target="_blank">here</a>.

### Installation Guide --> <a href="https://github.com/richonguzman/LoRa_APRS_iGate/wiki/01.-Installation-Guide" target="_blank">here</a>.

*(Wiki has all configuration explanation, supported boards list, adding BME/BMP Wx modules and more)*

____________________________________________________
## Timeline (Versions):

- 2024.04.20 New Output Buffer process: no more packets lost.
- 2024.04.13 Received Packets added on WebUI.
- 2024.04.09 iGate/Digirepeater own GPS beacon is encoded (Base91) now.
- 2024.03.18 OE5HWN MeshCom board support added.
- 2024.02.25 New Web Configuration UI with WiFi AP (thanks Damian SQ2CPA).
- 2023.01.28 Updated to ElegantOTA v.3 (AsyncElegantOTA was deprecated).
- 2024.01.19 TextSerialOutputForApp added to get text from Serial-Output over USB into PC for PinPoint App (https://www.pinpointaprs.com) and APRSIS32 App (http://aprsisce.wikidot.com)
- 2024.01.12 Added iGate Mode to also repeat packets (like a iGate+DigiRepeater) in stationMode 2 and 5.
- 2024.01.11 Added iGate Mode to enable APRS-IS and LoRa beacon report at the same time.
- 2024.01.05 Added support for Lilygo TTGO T-Beam V1, V1.2, V1 + SX1268, V1.2 + SX1262.
- 2024.01.02 Added support for EByte 400M30S 1Watt LoRa module for DIY ESP32 iGate.
- 2023.12.27 HELTEC V3 board support added. Thanks Luc ON2ON.
- 2023.12.26 Added BME680 module to BME/BMP280 modules supported.
- 2023.12.07 MIC-E process and syslog added.
- 2023.12.06 HELTEC V2 board support added.
- 2023.11.26 Small correction to enable Syslog in stationMode5.
- 2023.10.09 Added "WIDE1-1" to Tx packets from iGate to be *repeated* by Digirepeaters.
- 2023.10.09 Added Support also for BMP280 module.
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

__________________________________________

# Hope You Enjoy this, 73 !!  CA2RXU , Valparaiso, Chile