/* Copyright (C) 2025 Ricardo Guzman - CA2RXU
 * 
 * This file is part of LoRa APRS iGate.
 * 
 * LoRa APRS iGate is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or 
 * (at your option) any later version.
 * 
 * LoRa APRS iGate is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with LoRa APRS iGate. If not, see <https://www.gnu.org/licenses/>.
 */

#include <APRSPacketLib.h>
#include <TinyGPS++.h>
#include <WiFi.h>
#include "configuration.h"
#include "board_pinout.h"
#include "gps_utils.h"
#include "display.h"
#include "utils.h"

#ifdef GPS_BAUDRATE
    #define GPS_BAUD    GPS_BAUDRATE
#else
    #define GPS_BAUD    9600
#endif

extern Configuration    Config;
extern HardwareSerial   gpsSerial;
extern TinyGPSPlus      gps;
String                  distance, iGateBeaconPacket, iGateLoRaBeaconPacket;


namespace GPS_Utils {

    String getiGateLoRaBeaconPacket() {
        return iGateLoRaBeaconPacket;
    }

    void generateBeacons() {
        if (Config.callsign.indexOf("NOCALL-10") != 0 && !Utils::checkValidCallsign(Config.callsign)) {
            displayShow("***** ERROR ******", "CALLSIGN = NOT VALID!", "", "Only Rx Mode Active", 3000);
            Config.loramodule.txActive  = false;
            Config.aprs_is.messagesToRF = false;
            Config.aprs_is.objectsToRF  = false;
            Config.beacon.sendViaRF     = false;
            Config.digi.mode            = 0;
            Config.backupDigiMode       = false;
        }
        String beaconPacket     = APRSPacketLib::generateBasePacket(Config.callsign, "APLRG1", Config.beacon.path);
        String encodedGPS       = APRSPacketLib::encodeGPSIntoBase91(Config.beacon.latitude, Config.beacon.longitude, 0, 0, Config.beacon.symbol, false, 0, true, Config.beacon.ambiguityLevel);
        
        iGateBeaconPacket       = beaconPacket;
        iGateBeaconPacket       += ",qAC:!";
        iGateBeaconPacket       += Config.beacon.overlay;
        iGateBeaconPacket       += encodedGPS;

        iGateLoRaBeaconPacket   = beaconPacket;
        iGateLoRaBeaconPacket   += ":=";
        iGateLoRaBeaconPacket   += Config.beacon.overlay;
        iGateLoRaBeaconPacket   += encodedGPS;
    }

    double calculateDistanceTo(double latitude, double longitude) {
        return TinyGPSPlus::distanceBetween(Config.beacon.latitude,Config.beacon.longitude, latitude, longitude) / 1000.0;
    }

    String buildDistanceAndComment(float latitude, float longitude, const String& comment) {
        distance = String(calculateDistanceTo(latitude, longitude),1);

        String distanceAndComment = String(latitude,5);
        distanceAndComment += "N / ";
        distanceAndComment += String(longitude,5);
        distanceAndComment += "E / ";
        distanceAndComment += distance;
        distanceAndComment += "km";

        if (comment != "") {
            distanceAndComment += " / ";
            distanceAndComment += comment;
        }
        return distanceAndComment;

    }

    String decodeEncodedGPS(const String& packet) {
        int indexOfExclamation  = packet.indexOf(":!");
        int indexOfEqual        = packet.indexOf(":=");

        const uint8_t OFFSET = 3;     // Offset for encoded data in the packet
        String infoGPS;
        if (indexOfExclamation > 10) {
            infoGPS = packet.substring(indexOfExclamation + OFFSET);
        } else if (indexOfEqual > 10) {
            infoGPS = packet.substring(indexOfEqual + OFFSET);
        }

        float decodedLatitude   = APRSPacketLib::decodeBase91EncodedLatitude(infoGPS.substring(0,4));
        float decodedLongitude  = APRSPacketLib::decodeBase91EncodedLongitude(infoGPS.substring(4,8));

        return buildDistanceAndComment(decodedLatitude, decodedLongitude, infoGPS.substring(12));
    }

    String getReceivedGPS(const String& packet) {
        int indexOfExclamation  = packet.indexOf(":!");
        int indexOfEqual        = packet.indexOf(":=");
        int indexOfAt           = packet.indexOf(":@");

        String infoGPS;
        if (indexOfExclamation > 10) {
            infoGPS = packet.substring(indexOfExclamation + 2);
        } else if (indexOfEqual > 10) {
            infoGPS = packet.substring(indexOfEqual + 2);
        } else if (indexOfAt > 10) {
            infoGPS = packet.substring(indexOfAt + 9);  // 9 = 2+7 (when 7 is timestamp characters)
        }

        String Latitude             = infoGPS.substring(0,8);                   // First 8 characters are Latitude
        float convertedLatitude     = Latitude.substring(0,2).toFloat();        // First 2 digits (Degrees)
        convertedLatitude += Latitude.substring(2,4).toFloat() / 60;            // Next 2 digits (Minutes)
        convertedLatitude += Latitude.substring(Latitude.indexOf(".") + 1, Latitude.indexOf(".") + 3).toFloat() / (60*100);
        if (Latitude.endsWith("S")) convertedLatitude = -convertedLatitude;     // Handle Southern Hemisphere

        String Longitude            = infoGPS.substring(9,18);                  // Next 9 characters are Longitude
        float convertedLongitude    = Longitude.substring(0,3).toFloat();       // First 3 digits (Degrees)
        convertedLongitude += Longitude.substring(3,5).toFloat() / 60;          // Next 2 digits (Minutes)
        convertedLongitude += Longitude.substring(Longitude.indexOf(".") + 1, Longitude.indexOf(".") + 3).toFloat() / (60*100);
        if (Longitude.endsWith("W")) convertedLongitude = -convertedLongitude;  // Handle Western Hemisphere
        
        return buildDistanceAndComment(convertedLatitude, convertedLongitude, infoGPS.substring(19));
    }

    String getDistanceAndComment(const String& packet) {
        int indexOfAt = packet.indexOf(":@");
        if (indexOfAt > 10) return getReceivedGPS(packet);

        const uint8_t nonEncondedLatitudeOffset     = 9;    // "N" / "S"
        const uint8_t nonEncondedLongitudeOffset    = 19;   // "E" / "W"
        const uint8_t encondedByteOffset            = 14;
        
        int indexOfExclamation  = packet.indexOf(":!");
        int indexOfEqual        = packet.indexOf(":=");
        int baseIndex           = - 1;
        if (indexOfExclamation > 10) {
            baseIndex = indexOfExclamation;
        } else if (indexOfEqual > 10) {
            baseIndex = indexOfEqual;
        }
        if (baseIndex == -1) return " _ / _ / _ ";

        int latitudeIndex       = baseIndex + nonEncondedLatitudeOffset;
        int longitudeIndex      = baseIndex + nonEncondedLongitudeOffset;
        int encondedByteIndex   = baseIndex + encondedByteOffset;
        int packetLength        = packet.length();

        if (latitudeIndex >= packetLength || longitudeIndex >= packetLength || encondedByteIndex >= packetLength) return " _ / _ / _ ";

        char latChar = packet[latitudeIndex];
        char lngChar = packet[longitudeIndex];
        if ((latChar == 'N' || latChar == 'S') && (lngChar == 'E' || lngChar == 'W')) return getReceivedGPS(packet);
        
        char byteChar = packet[encondedByteIndex];
        if (byteChar == 'G' || byteChar == 'Q' || byteChar == '[' || byteChar == 'H' || byteChar == 'X' || byteChar == '3') return decodeEncodedGPS(packet);

        return " _ / _ / _ ";
    }

    void setup() {
        #ifdef HAS_GPS
            if (Config.beacon.gpsActive && Config.digi.ecoMode != 1) {
                gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_TX, GPS_RX);
            }
        #endif
        generateBeacons();
    }

    void getData() {
        while (gpsSerial.available() > 0) {
            gps.encode(gpsSerial.read());
        }
    }

}