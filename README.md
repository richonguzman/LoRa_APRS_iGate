# Richonguzman / CA2RXU LoRa APRS iGate/Digirepeater

This firmware is for using ESP32 based boards with LoRa Modules and GPS to live in the APRS world.

![Screenshot](https://github.com/richonguzman/LoRa_APRS_iGate/blob/main/images/iGateOledScreen.jpeg)

__(This iGate Firmware works with all LoRa Tracker Firmwares (specially this <a href="https://github.com/richonguzman/LoRa_APRS_Tracker" target="_blank">LoRa APRS Tracker Firmware</a>))__

___________________________________________________

## You can support this project to continue to grow:

[<img src="https://github.com/richonguzman/LoRa_APRS_Tracker/blob/main/images/github-sponsors.png">](https://github.com/sponsors/richonguzman)     [<img src="https://github.com/richonguzman/LoRa_APRS_Tracker/blob/main/images/paypalme.png">](http://paypal.me/richonguzman)

____________________________________________________

# WEB FLASHER/INSTALLER is <a href="https://richonguzman.github.io/lora-igate-web-flasher/installer.html" target="_blank">here</a>

____________________________________________________

# WIKI

### FAQ, BME280, TNC and more --> <a href="https://github.com/richonguzman/LoRa_APRS_iGate/wiki/00.-FAQ-(frequently-asked-questions)" target="_blank">here</a>.

### Installation Guide --> <a href="https://github.com/richonguzman/LoRa_APRS_iGate/wiki/01.-Installation-Guide" target="_blank">here</a>.


____________________________________________________

# SUPPORTED BOARDS

### Buying links --> <a href="https://github.com/richonguzman/LoRa_APRS_iGate/wiki/108.-Supported-Boards-and-Buying-Links" target="_blank">here</a>.

(NOTE: all boards with 433-868-915 MHz versions)

- TTGO Lilygo LoRa32 v2.1 / v1.6 (they work the same).

- TTGO T-Beam v1.0 , v1.1, v1.2 (also variations with SX1262 and SX1268 LoRa Modules).

- HELTEC V2, V3 , Wireless Stick, Wireless Stick Lite, HT-CT62, Wireless Tracker.

- ESP32 Wroom + SX1278 LoRa Module or Ebyte 400M30S/900M30S 1W LoRa Module for a DIY Versions.

- ESP32C3 + Ebyte 400M30S 1W LoRa Module for another DIY version.

- ESP32 + 4G/LTE A7670 Modem + SX1278 DIY Version.

- Wemos Lolin32 Oled + SX1278 DIY Version.

____________________________________________________
## Timeline (Versions):

- 2024.06.21 Tx packets comming from APRS-IS are (now) formated into 3rd Party (as they should have been since the beginning).
- 2024.06.18 All boards with 433 / 868 / 915 MHz versions.
- 2024.06.10 ESP32C3 + 1W LoRa Module (E22 400M30S) support added.
- 2024.06.09 Si7021 module added (with autodetected I2C Address)
- 2024.06.08 Callsign Validation for all Station that iGate/Digi hears.
- 2024.05.27 Battery Monitor for internal and External Voltages (to make board sleep and avoid low discharge of batterys) T-Beam boards now with Battery readings as well.
- 2024.05.23 Forced Reboot Mode added.
- 2024.05.22 Experimental backup-Digirepeater-Mode when "only" iGate mode loses WiFi connection added.
- 2024.05.20 WebConfig update to control whether Messages and Objects should be Tx to RF.
- 2024.05.17 HELTEC Wireless Stick Lite v3 support added.
- 2024.05.14 BME modules will be autodetected (I2C Address and if it is BME280/BMP280/BME680).
- 2024.05.13 PacketBuffer for Rx (25 Seg).
- 2024.05.11 HELTEC Wireless Tracker support added.
- 2024.04.23 T-LoRa32 v1.6/v2.1 with 915MHz support added.
- 2024.04.23 ESP32 + 4G/LTE MODEM A7670SA + LoRa (SX1278) support added.
- 2024.04.22 Wemos Lolin32 OLED DIY LoRa support added .
- 2024.04.21 WEB INSTALLER (thanks Damian SQ2CPA).
- 2024.04.20 New Output Buffer process: no more packets lost.
- 2024.04.13 Received Packets added on WebUI.
- 2024.04.09 iGate/Digirepeater own GPS beacon is encoded (Base91) now.
- 2024.03.18 OE5HWN MeshCom board support added.
- 2024.02.25 New Web Configuration UI with WiFi AP (thanks Damian SQ2CPA).
- 2023.01.28 Updated to ElegantOTA v.3 (AsyncElegantOTA was deprecated).
- 2024.01.19 TextSerialOutputForApp added to get text from Serial-Output over USB into PC for PinPoint App (https://www.pinpointaprs.com) and APRSIS32 App (http://aprsisce.wikidot.com)
- 2024.01.12 Added iGate Mode to also repeat packets (like a iGate+DigiRepeater) in stationMode 2 and 5.
- 2024.01.11 Added iGate Mode to enable APRS-IS and LoRa beacon report at the same time.
- 2024.01.05 Lilygo TTGO T-Beam V1, V1.2, V1 + SX1268, V1.2 + SX1262 support added.
- 2024.01.02 EByte 400M30S 1 Watt LoRa module for DIY ESP32 iGate support added.
- 2023.12.27 HELTEC V3 board support added. Thanks Luc ON2ON.
- 2023.12.26 BME680 module support added.
- 2023.12.07 MIC-E process and Syslog added.
- 2023.12.06 HELTEC V2 board support added.
- 2023.11.26 Small correction to enable Syslog in stationMode5.
- 2023.10.09 Added "WIDE1-1" to Tx packets from iGate to be *repeated* by Digirepeaters.
- 2023.10.09 BMP280 module support added.
- 2023.08.20 Added External Voltage Measurement (Max 15V!)
- 2023.08.05 Ground Height Correction for Pressure readings added.
- 2023.07.31 StationMode5 added: iGate when WiFi and APRS available, DigiRepeater when not.
- 2023.07.16 Small OTA, BME module update.
- 2023.07.05 Adding monitor info of Battery connected.
- 2023.06.18 Info on Oled Screen mayor update, added RSSI and Distance to Listened Station.
- 2023.06.17 BME280 Module (Temperature, Humidity, Pressure) support added.
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