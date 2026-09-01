/*
 * RP2350 GPS reader (UART1 / Serial2 -> TinyGPS++). Ported from the ESP32
 * gps_utils.cpp (excluded from the RP2350 build_src_filter). The UART is opened
 * whenever the firmware is built with -D HAS_GPS so the fix can be observed on
 * the serial log for bring-up; the beacon only *uses* the GPS position when
 * Config.beacon.gpsActive is set (see beacon_rp2350.cpp encodedPosition()).
 *
 * poll() runs in netTask and updates a plain snapshot; the beacon builders read
 * that snapshot (from netTask and loraTask). Only scalar reads cross tasks, so a
 * momentarily torn value is harmless (a slightly stale beacon position).
 */
#include "gps_rp2350.h"
#include "configuration.h"

extern Configuration Config;

#ifdef HAS_GPS
#include <TinyGPS++.h>

#ifndef GPS_BAUDRATE
#define GPS_BAUDRATE 9600           // most NMEA modules (incl. the DL20U9A) default here
#endif
#ifndef PIN_GPS_TX
#define PIN_GPS_TX 8                // RP2350 UART1 TX (GP08) -> GPS RX
#endif
#ifndef PIN_GPS_RX
#define PIN_GPS_RX 9                // RP2350 UART1 RX (GP09) <- GPS TX
#endif

namespace {
    TinyGPSPlus tinyGps;
    bool        started = false;
    struct {
        bool     valid = false;
        double   lat   = 0.0;
        double   lng   = 0.0;
        double   alt   = NAN;
        uint32_t sats  = 0;
    } fix;
}
#endif  // HAS_GPS

namespace Gps {

void setup() {
#ifdef HAS_GPS
    Serial2.setTX(PIN_GPS_TX);
    Serial2.setRX(PIN_GPS_RX);
    // Enlarge the RX ring buffer: poll() only drains the UART when the net loop
    // runs, so a brief stall (web request, APRS-IS reconnect) would overflow the
    // 32-byte default and corrupt NMEA sentences. 256 B covers a full 1 Hz burst.
    Serial2.setFIFOSize(256);
    Serial2.begin(GPS_BAUDRATE);
    started = true;
    Serial.printf("[gps] UART1 @%d baud (TX=GP%d RX=GP%d), gpsActive=%d\n",
                  GPS_BAUDRATE, PIN_GPS_TX, PIN_GPS_RX, (int)Config.beacon.gpsActive);
#endif
}

void poll() {
#ifdef HAS_GPS
    if (!started) return;
    while (Serial2.available() > 0) tinyGps.encode(Serial2.read());

    if (tinyGps.location.isValid() &&
        tinyGps.location.lat() != 0.0 && tinyGps.location.lng() != 0.0) {
        fix.valid = true;
        fix.lat   = tinyGps.location.lat();
        fix.lng   = tinyGps.location.lng();
        fix.alt   = tinyGps.altitude.isValid() ? tinyGps.altitude.meters() : NAN;
        fix.sats  = tinyGps.satellites.isValid() ? tinyGps.satellites.value() : 0;
    }

#ifdef GPS_DEBUG
    // Bring-up diagnostic (opt-in, -D GPS_DEBUG): charsProcessed()==0 means no
    // NMEA is arriving (wiring or baud); otherwise shows fix progress (sats climb,
    // then a valid position). Off in release images to keep the serial log clean.
    static uint32_t lastLog = 0;
    if (millis() - lastLog > 15000) {
        lastLog = millis();
        Serial.printf("[gps] chars=%lu csum-ok=%lu csum-fail=%lu sats=%d fix=%d lat=%.5f lng=%.5f alt=%.0f\n",
                      (unsigned long)tinyGps.charsProcessed(),
                      (unsigned long)tinyGps.passedChecksum(),
                      (unsigned long)tinyGps.failedChecksum(),
                      tinyGps.satellites.isValid() ? (int)tinyGps.satellites.value() : 0,
                      (int)fix.valid, fix.lat, fix.lng, fix.alt);
    }
#endif
#endif
}

bool hasFix() {
#ifdef HAS_GPS
    return started && fix.valid;
#else
    return false;
#endif
}

double lat()        {
#ifdef HAS_GPS
    return fix.lat;
#else
    return 0.0;
#endif
}

double lng()        {
#ifdef HAS_GPS
    return fix.lng;
#else
    return 0.0;
#endif
}

double altMeters()  {
#ifdef HAS_GPS
    return fix.alt;
#else
    return NAN;
#endif
}

uint32_t satellites() {
#ifdef HAS_GPS
    return fix.sats;
#else
    return 0;
#endif
}

}  // namespace Gps
