#include <TinyGPS++.h>
#include <WiFi.h>
#include "configuration.h"
#include "gps_utils.h"

extern Configuration  Config;
extern WiFiClient     espClient;
String                distance, iGateBeaconPacket, iGateLoRaBeaconPacket;


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

    String encodeGPS(float latitude, float longitude, const String& overlay, const String& symbol) {
        String encodedData = overlay;
        uint32_t aprs_lat, aprs_lon;
        aprs_lat = 900000000 - latitude * 10000000;
        aprs_lat = aprs_lat / 26 - aprs_lat / 2710 + aprs_lat / 15384615;
        aprs_lon = 900000000 + longitude * 10000000 / 2;
        aprs_lon = aprs_lon / 26 - aprs_lon / 2710 + aprs_lon / 15384615;

        String Ns, Ew, helper;
        if(latitude < 0) { Ns = "S"; } else { Ns = "N"; }
        if(latitude < 0) { latitude= -latitude; }

        if(longitude < 0) { Ew = "W"; } else { Ew = "E"; }
        if(longitude < 0) { longitude= -longitude; }

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
        encodedData += symbol + " x" + "\x47";
        return encodedData;
    }

    void generateBeacons() {
        String beaconPacket = Config.callsign + ">APLRG1";
        if (Config.beacon.path != "") {
            beaconPacket += ",";
            beaconPacket += Config.beacon.path;
        }
        String encodedGPS = encodeGPS(Config.beacon.latitude, Config.beacon.longitude, Config.beacon.overlay, Config.beacon.symbol);
        
        iGateBeaconPacket = beaconPacket;
        iGateBeaconPacket += ",qAC:!";
        iGateBeaconPacket += encodedGPS;

        iGateLoRaBeaconPacket = beaconPacket;
        iGateLoRaBeaconPacket += ":!";
        iGateLoRaBeaconPacket += encodedGPS;
    }

    double calculateDistanceTo(double latitude, double longitude) {
        return TinyGPSPlus::distanceBetween(Config.beacon.latitude,Config.beacon.longitude, latitude, longitude) / 1000.0;
    }

    String decodeEncodedGPS(const String& packet) {
        String GPSPacket            = packet.substring(packet.indexOf(":!")+3);
        String encodedLatitude      = GPSPacket.substring(0,4);
        String encodedLongtitude    = GPSPacket.substring(4,8);
        String comment              = GPSPacket.substring(13);

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
        if (comment != "") {
            return String(decodedLatitude,5) + "N / " + String(decodedLongitude,5) + "E / " + distance + "km / " + comment;
        } else {
            return String(decodedLatitude,5) + "N / " + String(decodedLongitude,5) + "E / " + distance + "km";
        }
    }

    String getReceivedGPS(const String& packet) {
        String infoGPS;
        if (packet.indexOf(":!") > 10) {
            infoGPS = packet.substring(packet.indexOf(":!") + 2);
        } else if (packet.indexOf(":=") > 10) {
            infoGPS = packet.substring(packet.indexOf(":=") + 2);
        }
        String Latitude       = infoGPS.substring(0,8);
        String Longitude      = infoGPS.substring(9,18);
        String comment        = infoGPS.substring(19);  

        float convertedLatitude, convertedLongitude;
        String firstLatPart   = Latitude.substring(0,2);
        String secondLatPart  = Latitude.substring(2,4);
        String thirdLatPart   = Latitude.substring(Latitude.indexOf(".") + 1, Latitude.indexOf(".") + 3);
        String firstLngPart   = Longitude.substring(0,3);
        String secondLngPart  = Longitude.substring(3,5);
        String thirdLngPart   = Longitude.substring(Longitude.indexOf(".") + 1, Longitude.indexOf(".") + 3);
        
        convertedLatitude     = firstLatPart.toFloat() + (secondLatPart.toFloat()/60) + (thirdLatPart.toFloat()/(60*100));
        convertedLongitude    = firstLngPart.toFloat() + (secondLngPart.toFloat()/60) + (thirdLngPart.toFloat()/(60*100));
        
        String LatSign = String(Latitude[7]);
        String LngSign = String(Longitude[8]);
        if (LatSign == "S") {
            convertedLatitude = -convertedLatitude;
        } 
        if (LngSign == "W") {
            convertedLongitude = -convertedLongitude;
        }
        distance = String(calculateDistanceTo(convertedLatitude, convertedLongitude),1);
        if (comment != "") {
            return String(convertedLatitude,5) + "N / " + String(convertedLongitude,5) + "E / " + distance + "km / " + comment;
        } else {
            return String(convertedLatitude,5) + "N / " + String(convertedLongitude,5) + "E / " + distance + "km";
        }
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
            if (String(packet[encodedBytePosition]) == "G" || String(packet[encodedBytePosition]) == "Q" || String(packet[encodedBytePosition]) == "[" || String(packet[encodedBytePosition]) == "H") {
                return decodeEncodedGPS(packet);
            } else {
                return getReceivedGPS(packet);
            }
        } else {
            return " _ / _ / _ ";
        }
    }

}