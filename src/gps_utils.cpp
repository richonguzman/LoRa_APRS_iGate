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
#include <SPIFFS.h>
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

#define GPS_BAUD_FILE           "/gps_baud.dat"
#define GPS_BAUD_PROBE_TIME_MS  1500
const uint32_t gpsBaudRates[] = {9600, 115200, 38400, 4800, 57600};     // most common first

extern Configuration    Config;
extern HardwareSerial   gpsSerial;
extern TinyGPSPlus      gps;
extern bool             stationCallsignIsValid;
String                  distance, iGateAPRSISBeaconPacket, iGateLoRaBeaconPacket;


namespace GPS_Utils {

#ifdef HAS_GPS
    bool isKnownBaudRate(uint32_t baud) {
        if (baud == GPS_BAUD) return true;
        for (uint32_t knownBaud : gpsBaudRates) {
            if (knownBaud == baud) return true;
        }
        return false;
    }

    uint32_t readSavedBaudRate() {
        File file = SPIFFS.open(GPS_BAUD_FILE, "r");
        if (!file) return 0;
        uint32_t baud = file.readString().toInt();
        file.close();
        return isKnownBaudRate(baud) ? baud : 0;
    }

    void saveBaudRate(uint32_t baud) {
        File file = SPIFFS.open(GPS_BAUD_FILE, "w");
        if (!file) {
            Utils::println("GPS: could not save baud rate");
            return;
        }
        file.print(baud);
        file.close();
    }

    bool probeBaudRate(uint32_t baud) {
        gpsSerial.end();
        delay(50);
        gpsSerial.begin(baud, SERIAL_8N1, GPS_TX, GPS_RX);
        uint32_t validSentences = gps.passedChecksum();
        uint32_t probeStart     = millis();
        while (millis() - probeStart < GPS_BAUD_PROBE_TIME_MS) {
            while (gpsSerial.available() > 0) gps.encode(gpsSerial.read());
            if (gps.passedChecksum() > validSentences) return true;   // NMEA with valid checksum
            delay(10);
        }
        return false;
    }

    uint32_t detectBaudRate() {
        uint32_t savedBaud = readSavedBaudRate();

        uint32_t candidates[2 + sizeof(gpsBaudRates) / sizeof(gpsBaudRates[0])];
        size_t   count = 0;
        auto addCandidate = [&](uint32_t baud) {
            if (baud == 0) return;
            for (size_t i = 0; i < count; i++) {
                if (candidates[i] == baud) return;
            }
            candidates[count++] = baud;
        };
        addCandidate(savedBaud);                                // saved first, then board default, then common ones
        addCandidate(GPS_BAUD);
        for (uint32_t baud : gpsBaudRates) addCandidate(baud);

        for (size_t i = 0; i < count; i++) {
            if (i == 1) displayShow("GPS", "Searching", "baud rate...", "", 0);
            bool found = probeBaudRate(candidates[i]);
            if (!found && i == 0) found = probeBaudRate(candidates[i]);    // retry first one: GPS may be slow to start
            if (found) {
                if (candidates[i] != savedBaud) saveBaudRate(candidates[i]);
                return candidates[i];
            }
        }
        return 0;
    }
#endif

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

        iGateAPRSISBeaconPacket = beaconPacket;
        iGateAPRSISBeaconPacket += ",qAC:=";
        iGateAPRSISBeaconPacket += Config.beacon.overlay;
        iGateAPRSISBeaconPacket += encodedGPS;

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
                uint32_t detectedBaud = detectBaudRate();
                if (detectedBaud != 0) {
                    Utils::println("GPS: baud rate detected: " + String(detectedBaud));
                } else {
                    Utils::println("GPS: no NMEA data found, using default baud rate: " + String(GPS_BAUD));
                    gpsSerial.end();
                    delay(50);
                    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_TX, GPS_RX);
                }
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