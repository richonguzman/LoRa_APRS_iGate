/* Copyright (C) 2026 Ricardo Guzman - CA2RXU
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
extern bool             stationCallsignIsValid;
String                  distance, iGateBeaconPacket, iGateLoRaBeaconPacket;


namespace GPS_Utils {

    String getiGateLoRaBeaconPacket() {
        return iGateLoRaBeaconPacket;
    }

    void generateBeacons() {
        String beaconPacket = APRSPacketLib::generateBasePacket(Config.callsign, "APLRG1", Config.beacon.path);
        String encodedGPS   = APRSPacketLib::encodeGPSIntoBase91(Config.beacon.latitude, Config.beacon.longitude, 0, 0, Config.beacon.symbol, false, 0, true, Config.beacon.ambiguityLevel);

        if (Config.callsign.indexOf("NOCALL-10") != 0) {
            if (!stationCallsignIsValid) {
                displayShow("***** ERROR ******", "CALLSIGN = NOT VALID!", "", "Only Rx Mode Active", 3000);
                Config.loramodule.txActive  = false;
                Config.aprs_is.messagesToRF = false;
                Config.aprs_is.objectsToRF  = false;
                Config.beacon.sendViaRF     = false;
                Config.digi.mode            = 0;
                Config.digi.backupDigiMode  = false;
            } else if (stationCallsignIsValid && Config.tacticalCallsign != "") {
                beaconPacket = APRSPacketLib::generateBasePacket(Config.tacticalCallsign, "APLRG1", Config.beacon.path);
                Config.aprs_is.active       = false;
                Config.beacon.sendViaAPRSIS = false;
                Config.digi.backupDigiMode  = false;
            }
        } else {
            Config.beacon.sendViaAPRSIS = false;
            Config.beacon.sendViaRF     = false;
        }

        iGateBeaconPacket       = beaconPacket;
        iGateBeaconPacket       += ",qAC:=";
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