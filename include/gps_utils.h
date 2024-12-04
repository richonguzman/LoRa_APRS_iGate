#ifndef GPS_UTILS_H_
#define GPS_UTILS_H_

#include <Arduino.h>


namespace GPS_Utils {
    
    String  getiGateLoRaBeaconPacket();
    char    *ax25_base91enc(char *s, uint8_t n, uint32_t v);
    String  encodeGPS(float latitude, float longitude, const String& overlay, const String& symbol);
    void    generateBeaconFirstPart();
    void    generateBeacons();
    String  decodeEncodedGPS(const String& packet);
    String  getReceivedGPS(const String& packet);
    String  getDistanceAndComment(const String& packet);

    void    setup();
    void    getData();

}

#endif