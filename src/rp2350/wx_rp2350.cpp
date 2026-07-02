#include "wx_rp2350.h"
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include "configuration.h"

extern Configuration Config;

// Which I2C peripheral the weather sensor hangs off. arduino-pico: Wire = I2C0
// (GP4/GP5 by default), Wire1 = I2C1 (GP6/GP7 etc.). Select with -D WX_USE_WIRE1.
#ifdef WX_USE_WIRE1
#define WX_WIRE Wire1
#else
#define WX_WIRE Wire
#endif

#ifndef PIN_WX_SDA
#define PIN_WX_SDA 4          // I2C0 default SDA on arduino-pico (GP4)
#endif
#ifndef PIN_WX_SCL
#define PIN_WX_SCL 5          // I2C0 default SCL on arduino-pico (GP5)
#endif

#define WX_CORRECTION_FACTOR 8.2296f   // metres -> hPa (matches wx_utils.cpp)

namespace Wx {

static Adafruit_BMP280 bmp280(&WX_WIRE);
static bool bmpPresent = false;   // temperature + barometric pressure
static bool shtPresent = false;   // temperature + relative humidity (SHT40)
static uint8_t sht40Addr = 0;     // 0x44 (AD1B) or 0x45 (BD1B)

// --- APRS field formatters (byte-for-byte the ESP32 wx_utils.cpp output) ---
static String tempString(float degF) {       // temperature in Fahrenheit, 3 digits
    String s = String((int)degF);
    switch (s.length()) {
        case 1:  return "00" + s;
        case 2:  return "0" + s;
        case 3:  return s;
        default: return "-999";
    }
}

static String presString(float hPa) {         // pressure x10 (0.1 hPa), 5 digits
    String whole = String((int)hPa);
    String dec   = String(int((hPa - int(hPa)) * 10));
    switch (whole.length()) {
        case 1:  return "000" + whole + dec;
        case 2:  return "00" + whole + dec;
        case 3:  return "0" + whole + dec;
        case 4:  return whole + dec;
        case 5:  return whole;
        default: return "-99999";
    }
}

static String humString(float rh) {           // relative humidity, 2 digits (h00 == 100%)
    long h = lround(rh);
    if (h >= 100) return "00";                 // APRS encodes 100% as "00"
    if (h < 0) h = 0;
    char b[3];
    snprintf(b, sizeof(b), "%02ld", h);
    return String(b);
}

// SHT40: write 0xFD (measure, high precision), wait, read 6 bytes
// [tMSB tLSB tCRC hMSB hLSB hCRC]. Datasheet conversions. CRC not checked (lean).
static bool sht40Read(float &tC, float &rh) {
    WX_WIRE.beginTransmission(sht40Addr);
    WX_WIRE.write(0xFD);
    if (WX_WIRE.endTransmission() != 0) return false;
    delay(10);                                 // high-precision conversion ~8.2 ms
    if (WX_WIRE.requestFrom(sht40Addr, (uint8_t)6) != 6) return false;
    uint8_t d[6];
    for (int i = 0; i < 6; i++) d[i] = WX_WIRE.read();
    uint16_t rawT = ((uint16_t)d[0] << 8) | d[1];
    uint16_t rawH = ((uint16_t)d[3] << 8) | d[4];
    tC = -45.0f + 175.0f * ((float)rawT / 65535.0f);
    rh = -6.0f + 125.0f * ((float)rawH / 65535.0f);
    if (rh < 0.0f) rh = 0.0f;
    if (rh > 100.0f) rh = 100.0f;
    return true;
}

void setup() {
    WX_WIRE.setSDA(PIN_WX_SDA);
    WX_WIRE.setSCL(PIN_WX_SCL);
    WX_WIRE.begin();
    // The board carries dedicated 4.7k external I2C pull-ups, so no internal
    // pull-ups are enabled here.

    // I2C scan (diagnostic): list everything answering on PIN_WX_SDA/SCL.
    Serial.printf("[wx] I2C scan on SDA=%d SCL=%d:", PIN_WX_SDA, PIN_WX_SCL);
    int found = 0;
    for (uint8_t a = 0x08; a < 0x78; a++) {
        WX_WIRE.beginTransmission(a);
        if (WX_WIRE.endTransmission() == 0) { Serial.printf(" 0x%02X", a); found++; }
    }
    Serial.println(found ? "" : " (none)");

    // BMP280 ships at 0x76 or 0x77 depending on the SDO strap.
    if (bmp280.begin(0x76) || bmp280.begin(0x77)) {
        bmp280.setSampling(Adafruit_BMP280::MODE_FORCED,
                           Adafruit_BMP280::SAMPLING_X2,    // temperature
                           Adafruit_BMP280::SAMPLING_X16,   // pressure
                           Adafruit_BMP280::FILTER_X16,
                           Adafruit_BMP280::STANDBY_MS_500);
        bmpPresent = true;
        Serial.println("[wx] BMP280 found (temp + pressure)");
    }

    // SHT40 at 0x44 or 0x45 (temp + humidity). Detected by an ACK on the bus.
    for (uint8_t a = 0x44; a <= 0x45; a++) {
        WX_WIRE.beginTransmission(a);
        if (WX_WIRE.endTransmission() == 0) {
            sht40Addr  = a;
            shtPresent = true;
            Serial.printf("[wx] SHT40 found at 0x%02X (temp + humidity)\n", a);
            break;
        }
    }

    if (!bmpPresent && !shtPresent)
        Serial.println("[wx] no BMP280/SHT40 on I2C (weather disabled)");
}

bool present() { return bmpPresent || shtPresent; }

// APRS weather payload: ".../...g...t<TTT>[h<HH>][b<PPPPP>]". Temperature comes
// from whichever sensor is present (SHT40 preferred); humidity from the SHT40,
// pressure from the BMP280. Fields for absent sensors are omitted.
String readAprs() {
    if (!bmpPresent && !shtPresent) return "";

    float tC = NAN, rh = NAN, p = NAN;

    if (shtPresent) {
        float t, h;
        if (sht40Read(t, h)) { tC = t; rh = h; }
    }
    if (bmpPresent) {
        bmp280.takeForcedMeasurement();
        float bt = bmp280.readTemperature();          // degC
        float bp = bmp280.readPressure() / 100.0f;    // hPa
        if (!isnan(bp)) p = bp;
        if (isnan(tC) && !isnan(bt)) tC = bt;          // fall back to BMP280 temp
    }

    if (isnan(tC)) return ".../...g...t...";

    String wx = ".../...g...t";
    wx += tempString((tC + Config.wxsensor.temperatureCorrection) * 1.8f + 32.0f);
    wx += "h";
    wx += isnan(rh) ? String("..") : humString(rh);
    if (!isnan(p)) {
        wx += "b";
        wx += presString(p + Config.wxsensor.heightCorrection / WX_CORRECTION_FACTOR);
    }
    return wx;
}

}  // namespace Wx
