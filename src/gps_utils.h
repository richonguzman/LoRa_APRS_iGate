#ifndef GPS_UTILS_H_
#define GPS_UTILS_H_

#include <Arduino.h>

namespace GPS_Utils {

    String double2string(double n, int ndec);
    String processLatitudeAPRS();
    String processLongitudeAPRS();
    String generateBeacon();
    String generateiGateLoRaBeacon();
    double calculateDistanceCourse(double latitude, double longitude);
    String decodeEncodedGPS(String packet);
    String getReceivedGPS(String packet);
    String getDistance(String packet);

}

#endif