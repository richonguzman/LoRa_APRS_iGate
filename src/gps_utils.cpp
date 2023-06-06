#include "gps_utils.h"
#include "configuration.h"

extern Configuration  Config;
extern WiFi_AP        *currentWiFi;
extern int            myWiFiAPIndex;
extern int            myWiFiAPSize;

namespace GPS_Utils {

String double2string(double n, int ndec) {
    String r = "";
    int v = n;
    r += v;
    r += '.';
    int i;
    for (i=0;i<ndec;i++) {
        n -= v;
        n = 10 * abs(n);
        v = n;
        r += v;
    }
    return r;
}

String processLatitudeAPRS() {
  String degrees = double2string(currentWiFi->latitude,6);
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

String processLongitudeAPRS() {
  String degrees = double2string(currentWiFi->longitude,6);
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
  String iGateLat = processLatitudeAPRS();
  String iGateLon = processLongitudeAPRS();
  String beaconPacket = Config.callsign + ">APLR10,qAC:=";
  if (Config.loramodule.enableTx) {
    beaconPacket += iGateLat + "L" + iGateLon + "a";
  } else {
    beaconPacket += iGateLat + "L" + iGateLon + "&";
  }
  beaconPacket += Config.comment;
  return beaconPacket;
}

}