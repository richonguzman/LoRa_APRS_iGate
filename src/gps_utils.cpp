#include <TinyGPS++.h>
#include <WiFi.h>
#include "configuration.h"
#include "gps_utils.h"

extern Configuration  Config;
extern WiFi_AP        *currentWiFi;
extern WiFiClient     espClient;
extern int            stationMode;
String                distance;

namespace GPS_Utils {

String double2string(double n, int ndec) {
   String r = "";
    if (n>-1 && n<0) {
      r = "-";
    }   
    int v = n;
    r += v;
    r += '.';
    for (int i=0;i<ndec;i++) {
        n -= v;
        n = 10 * abs(n);
        v = n;
        r += v;
    }
    return r;
}

String processLatitudeAPRS(double lat) {
  String degrees = double2string(lat,6);
  String north_south, latitude, convDeg3;
  float convDeg, convDeg2;

  if (abs(degrees.toFloat()) < 10) {
    latitude += "0";
  }
  Serial.println(latitude);
  if (degrees.indexOf("-") == 0) {
    north_south = "S";
    latitude += degrees.substring(1,degrees.indexOf("."));
  } else {
    north_south = "N";
    latitude += degrees.substring(0,degrees.indexOf("."));
  }
  convDeg = abs(degrees.toFloat()) - abs(int(degrees.toFloat()));
  convDeg2 = (convDeg * 60)/100;
  convDeg3 = String(convDeg2,6);
  latitude += convDeg3.substring(convDeg3.indexOf(".")+1,convDeg3.indexOf(".")+3) + "." + convDeg3.substring(convDeg3.indexOf(".")+3,convDeg3.indexOf(".")+5);
  latitude += north_south;
  return latitude;
}

String processLongitudeAPRS(double lon) {
  String degrees = double2string(lon,6);
  String east_west, longitude, convDeg3;
  float convDeg, convDeg2;
  
  if (abs(degrees.toFloat()) < 100) {
    longitude += "0";
  }
  if (abs(degrees.toFloat()) < 10) {
    longitude += "0";
  }
  if (degrees.indexOf("-") == 0) {
    east_west = "W";
    longitude += degrees.substring(1,degrees.indexOf("."));
  } else {
    east_west = "E";
    longitude += degrees.substring(0,degrees.indexOf("."));
  }
  convDeg = abs(degrees.toFloat()) - abs(int(degrees.toFloat()));
  convDeg2 = (convDeg * 60)/100;
  convDeg3 = String(convDeg2,6);
  longitude += convDeg3.substring(convDeg3.indexOf(".")+1,convDeg3.indexOf(".")+3) + "." + convDeg3.substring(convDeg3.indexOf(".")+3,convDeg3.indexOf(".")+5);
  longitude += east_west;
  return longitude;
}

String generateBeacon() {
  String stationLatitude, stationLongitude, beaconPacket;
  if (stationMode==1 || stationMode==2 || (stationMode==5 && WiFi.status() == WL_CONNECTED && espClient.connected()) || stationMode==6) {
    stationLatitude = processLatitudeAPRS(currentWiFi->latitude);
    stationLongitude = processLongitudeAPRS(currentWiFi->longitude);
    beaconPacket = Config.callsign + ">APLRG1,WIDE1-1";
    if (stationMode!=6) {
      beaconPacket += ",qAC";
    }
    beaconPacket += ":=" + stationLatitude + "L" + stationLongitude;
    if (stationMode == 1) {
      beaconPacket += "&";
    } else {
      beaconPacket += "a";
    }
    beaconPacket += Config.iGateComment;
  } else { //stationMode 3, 4 and 5
    if (stationMode==5) {
      stationLatitude = processLatitudeAPRS(currentWiFi->latitude);
      stationLongitude = processLongitudeAPRS(currentWiFi->longitude);
    } else {
      stationLatitude = processLatitudeAPRS(Config.digi.latitude);
      stationLongitude = processLongitudeAPRS(Config.digi.longitude);
    }
    beaconPacket = Config.callsign + ">APLRG1,WIDE1-1:=" + stationLatitude + "L" + stationLongitude + "#" + Config.digi.comment;
  }
  return beaconPacket;
}

double calculateDistanceTo(double latitude, double longitude) {
  return TinyGPSPlus::distanceBetween(currentWiFi->latitude,currentWiFi->longitude, latitude, longitude) / 1000.0;
}

String decodeEncodedGPS(String packet) {
  String GPSPacket = packet.substring(packet.indexOf(":!")+3);
  String encodedLatitude    = GPSPacket.substring(0,4);
  String encodedLongtitude  = GPSPacket.substring(4,8);

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
  return String(decodedLatitude,5) + "N / " + String(decodedLongitude,5) + "E / " + distance + "km";
}

String getReceivedGPS(String packet) {
  String infoGPS;
  if (packet.indexOf(":!") > 10) {
    infoGPS = packet.substring(packet.indexOf(":!")+2);
  } else if (packet.indexOf(":=") > 10) {
    infoGPS = packet.substring(packet.indexOf(":=")+2);
  }
  String Latitude       = infoGPS.substring(0,8);
  String Longitude      = infoGPS.substring(9,18);

  float convertedLatitude, convertedLongitude;
  String firstLatPart   = Latitude.substring(0,2);
  String secondLatPart  = Latitude.substring(2,4);
  String thirdLatPart   = Latitude.substring(Latitude.indexOf(".")+1,Latitude.indexOf(".")+3);
  String firstLngPart   = Longitude.substring(0,3);
  String secondLngPart  = Longitude.substring(3,5);
  String thirdLngPart   = Longitude.substring(Longitude.indexOf(".")+1,Longitude.indexOf(".")+3);
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
  return String(convertedLatitude,5) + "N / " + String(convertedLongitude,5) + "E / " + distance + "km";
}

String getDistance(String packet) {
  int encodedBytePosition = 0;
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