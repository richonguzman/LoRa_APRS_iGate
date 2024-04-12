#ifndef GPS_UTILS_H_
#define GPS_UTILS_H_

#include <Arduino.h>


namespace GPS_Utils {

    char *ax25_base91enc(char *s, uint8_t n, uint32_t v);
    String encodeGPS(float latitude, float longitude, String overlay, String symbol);
    String generateBeacon();
    String generateiGateLoRaBeacon();
    double calculateDistanceCourse(double latitude, double longitude);
    String decodeEncodedGPS(String packet);
    String getReceivedGPS(String packet);
    String getDistance(String packet);

}

#endif