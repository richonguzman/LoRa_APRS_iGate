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
extern WiFiClient       espClient;
extern HardwareSerial   gpsSerial;
extern TinyGPSPlus      gps;
String                  distance, iGateBeaconPacket, iGateLoRaBeaconPacket;


namespace GPS_Utils {

    String getiGateLoRaBeaconPacket() {
        return iGateLoRaBeaconPacket;
    }

    char *ax25_base91enc(char *s, uint8_t n, uint32_t v) {
        for(s += n, *s = '\0'; n; n--) {
            *(--s) = v % 91 + 33;
            v /= 91;
        }
        return(s);
    }

    float roundToTwoDecimals(float degrees) {
        return round(degrees * 100) / 100;
    }

    String encodeGPS(float latitude, float longitude, const String& overlay, const String& symbol) {
        String encodedData = overlay;
        uint32_t aprs_lat, aprs_lon;

        float processedLatitude     = latitude;
        float processedLongitude    = longitude;
        if (Config.beacon.gpsActive && Config.beacon.gpsAmbiguity) {
            processedLatitude       = roundToTwoDecimals(latitude);
            processedLongitude      = roundToTwoDecimals(longitude);
        }

        aprs_lat = 900000000 - processedLatitude * 10000000;
        aprs_lat = aprs_lat / 26 - aprs_lat / 2710 + aprs_lat / 15384615;
        aprs_lon = 900000000 + processedLongitude * 10000000 / 2;
        aprs_lon = aprs_lon / 26 - aprs_lon / 2710 + aprs_lon / 15384615;

        String Ns, Ew, helper;
        if(processedLatitude < 0) { Ns = "S"; } else { Ns = "N"; }
        if(processedLatitude < 0) { processedLatitude = -processedLatitude; }

        if(processedLongitude < 0) { Ew = "W"; } else { Ew = "E"; }
        if(processedLongitude < 0) { processedLongitude = -processedLongitude; }

        char helper_base91[] = {"0000\0"};
        int i;
        ax25_base91enc(helper_base91, 4, aprs_lat);
        for (i = 0; i < 4; i++) {
            encodedData += helper_base91[i];
        }
        ax25_base91enc(helper_base91, 4, aprs_lon);
        for (i = 0; i < 4; i++) {
            encodedData += helper_base91[i];
        }
        encodedData += symbol;
        encodedData += "  ";
        encodedData += "\x47";
        return encodedData;
    }

    void generateBeaconFirstPart() {
        String beaconPacket = Config.callsign;
        beaconPacket += ">APLRG1";
        if (Config.beacon.path.indexOf("WIDE") == 0) {
            beaconPacket += ",";
            beaconPacket += Config.beacon.path;
        }
        iGateBeaconPacket       = beaconPacket;
        iGateBeaconPacket       += ",qAC:!";
        iGateLoRaBeaconPacket   = beaconPacket;
        iGateLoRaBeaconPacket   += ":!";
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
        generateBeaconFirstPart();
        String encodedGPS       = encodeGPS(Config.beacon.latitude, Config.beacon.longitude, Config.beacon.overlay, Config.beacon.symbol);
        iGateBeaconPacket       += encodedGPS;        
        iGateLoRaBeaconPacket   += encodedGPS;
    }

    double calculateDistanceTo(double latitude, double longitude) {
        return TinyGPSPlus::distanceBetween(Config.beacon.latitude,Config.beacon.longitude, latitude, longitude) / 1000.0;
    }

    String decodeEncodedGPS(const String& packet) {
        int indexOfExclamation  = packet.indexOf(":!");
        int indexOfEqual        = packet.indexOf(":=");

        const uint8_t OFFSET = 3;     // Offset for encoded data in the packet
        String GPSPacket;
        if (indexOfExclamation > 10) {
            GPSPacket = packet.substring(indexOfExclamation + OFFSET);
        } else if (indexOfEqual > 10) {
            GPSPacket = packet.substring(indexOfEqual + OFFSET);
        }

        String encodedLatitude = GPSPacket.substring(0,4);
        int Y1 = encodedLatitude[0] - 33;
        int Y2 = encodedLatitude[1] - 33;
        int Y3 = encodedLatitude[2] - 33;
        int Y4 = encodedLatitude[3] - 33;
        float decodedLatitude = 90.0 - (((Y1 * pow(91,3)) + (Y2 * pow(91,2)) + (Y3 * 91) + Y4) / 380926.0);

        String encodedLongitude = GPSPacket.substring(4,8);
        int X1 = encodedLongitude[0] - 33;
        int X2 = encodedLongitude[1] - 33;
        int X3 = encodedLongitude[2] - 33;
        int X4 = encodedLongitude[3] - 33;
        float decodedLongitude = -180.0 + (((X1 * pow(91,3)) + (X2 * pow(91,2)) + (X3 * 91) + X4) / 190463.0);

        distance = String(calculateDistanceTo(decodedLatitude, decodedLongitude),1);

        String decodedGPS = String(decodedLatitude,5);
        decodedGPS += "N / ";
        decodedGPS += String(decodedLongitude,5);
        decodedGPS += "E / ";
        decodedGPS += distance;
        decodedGPS += "km";

        String comment = GPSPacket.substring(12);
        if (comment != "") {
            decodedGPS += " / ";
            decodedGPS += comment;
        }
        return decodedGPS;
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
        
        distance = String(calculateDistanceTo(convertedLatitude, convertedLongitude),1);

        String decodedGPS = String(convertedLatitude,5);
        decodedGPS += "N / ";
        decodedGPS += String(convertedLongitude,5);
        decodedGPS += "E / ";
        decodedGPS += distance;
        decodedGPS += "km";

        String comment = infoGPS.substring(19);  
        if (comment != "") {
            decodedGPS += " / ";
            decodedGPS += comment;
        }
        return decodedGPS;
    }

    String getDistanceAndComment(const String& packet) {
        int indexOfAt = packet.indexOf(":@");
        if (indexOfAt > 10) {
            return getReceivedGPS(packet);
        } else {
            const uint8_t ENCODED_BYTE_OFFSET = 14;     // Offset for encoded data in the packet
            int indexOfExclamation  = packet.indexOf(":!");
            int indexOfEqual        = packet.indexOf(":=");
            uint8_t encodedBytePosition = 0;
            if (indexOfExclamation > 10) {              // Determine the position where encoded data starts
                encodedBytePosition = indexOfExclamation + ENCODED_BYTE_OFFSET;
            } else if (indexOfEqual > 10) {
                encodedBytePosition = indexOfEqual + ENCODED_BYTE_OFFSET;
            }

            if (encodedBytePosition != 0) {
                char currentChar = packet[encodedBytePosition];
                if (currentChar == 'G' || currentChar == 'Q' || currentChar == '[' || currentChar == 'H' || currentChar == 'X') {
                    return decodeEncodedGPS(packet);    // If valid encoded data position is found, decode it
                } else {
                    return getReceivedGPS(packet);
                }
            } else {
                return " _ / _ / _ ";
            }
        }
    }

    void setup() {
        #ifdef HAS_GPS
            if (Config.beacon.gpsActive) {
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