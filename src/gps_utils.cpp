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
        const String& GPSPacket         = packet.substring(packet.indexOf(":!")+3);
        const String& encodedLatitude   = GPSPacket.substring(0,4);
        const String& encodedLongtitude = GPSPacket.substring(4,8);
        const String& comment           = GPSPacket.substring(12);

        int Y1 = int(encodedLatitude[0]);
        int Y2 = int(encodedLatitude[1]);
        int Y3 = int(encodedLatitude[2]);
        int Y4 = int(encodedLatitude[3]);
        float decodedLatitude = 90.0 - ((((Y1-33) * pow(91,3)) + ((Y2-33) * pow(91,2)) + ((Y3-33) * 91) + Y4-33) / 380926.0);
        
        int X1 = int(encodedLongtitude[0]);
        int X2 = int(encodedLongtitude[1]);
        int X3 = int(encodedLongtitude[2]);
        int X4 = int(encodedLongtitude[3]);
        float decodedLongitude = -180.0 + ((((X1-33) * pow(91,3)) + ((X2-33) * pow(91,2)) + ((X3-33) * 91) + X4-33) / 190463.0);

        distance = String(calculateDistanceTo(decodedLatitude, decodedLongitude),1);

        String decodedGPS = String(decodedLatitude,5);
        decodedGPS += "N / ";
        decodedGPS += String(decodedLongitude,5);
        decodedGPS += "E / ";
        decodedGPS += distance;
        decodedGPS += "km";

        if (comment != "") {
            decodedGPS += " / ";
            decodedGPS += comment;
        }
        return decodedGPS;
    }

    String getReceivedGPS(const String& packet) {
        String infoGPS;
        if (packet.indexOf(":!") > 10) {
            infoGPS = packet.substring(packet.indexOf(":!") + 2);
        } else if (packet.indexOf(":=") > 10) {
            infoGPS = packet.substring(packet.indexOf(":=") + 2);
        }
        const String& Latitude      = infoGPS.substring(0,8);
        const String& Longitude     = infoGPS.substring(9,18);
        const String& comment       = infoGPS.substring(19);  
        
        float convertedLatitude, convertedLongitude;
        const String& firstLatPart   = Latitude.substring(0,2);
        const String& secondLatPart  = Latitude.substring(2,4);
        const String& thirdLatPart   = Latitude.substring(Latitude.indexOf(".") + 1, Latitude.indexOf(".") + 3);
        convertedLatitude     = firstLatPart.toFloat() + (secondLatPart.toFloat()/60) + (thirdLatPart.toFloat()/(60*100));

        const String& firstLngPart   = Longitude.substring(0,3);
        const String& secondLngPart  = Longitude.substring(3,5);
        const String& thirdLngPart   = Longitude.substring(Longitude.indexOf(".") + 1, Longitude.indexOf(".") + 3);        
        convertedLongitude    = firstLngPart.toFloat() + (secondLngPart.toFloat()/60) + (thirdLngPart.toFloat()/(60*100));
        
        if (String(Latitude[7]) == "S") {
            convertedLatitude = -convertedLatitude;
        } 
        if (String(Longitude[8]) == "W") {
            convertedLongitude = -convertedLongitude;
        }
        
        distance = String(calculateDistanceTo(convertedLatitude, convertedLongitude),1);

        String decodedGPS = String(convertedLatitude,5);
        decodedGPS += "N / ";
        decodedGPS += String(convertedLongitude,5);
        decodedGPS += "E / ";
        decodedGPS += distance;
        decodedGPS += "km";

        if (comment != "") {
            decodedGPS += " / ";
            decodedGPS += comment;
        }
        return decodedGPS;
    }

    String getDistanceAndComment(const String& packet) {
        uint8_t encodedBytePosition = 0;
        if (packet.indexOf(":!") > 10) {
            encodedBytePosition = packet.indexOf(":!") + 14;
        }
        if (packet.indexOf(":=") > 10) {
            encodedBytePosition = packet.indexOf(":=") + 14;
        }
        if (encodedBytePosition != 0) {
            char currentChar = packet[encodedBytePosition];
            if (currentChar == 'G' || currentChar == 'Q' || currentChar == '[' || currentChar == 'H' || currentChar == 'X') {
                return decodeEncodedGPS(packet);
            } else {
                return getReceivedGPS(packet);
            }
        } else {
            return " _ / _ / _ ";
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