#include "wx_rp2350.h"
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include "configuration.h"

extern Configuration Config;

#ifndef PIN_WX_SDA
#define PIN_WX_SDA 4          // I2C0 default SDA on arduino-pico (GP4)
#endif
#ifndef PIN_WX_SCL
#define PIN_WX_SCL 5          // I2C0 default SCL on arduino-pico (GP5)
#endif

#define WX_CORRECTION_FACTOR 8.2296f   // metres -> hPa (matches wx_utils.cpp)

namespace Wx {

static Adafruit_BMP280 bmp280(&Wire);
static bool sensorPresent = false;

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

void setup() {
    Wire.setSDA(PIN_WX_SDA);
    Wire.setSCL(PIN_WX_SCL);
    Wire.begin();
    // BMP280 ships at 0x76 or 0x77 depending on the SDO strap.
    if (bmp280.begin(0x76) || bmp280.begin(0x77)) {
        bmp280.setSampling(Adafruit_BMP280::MODE_FORCED,
                           Adafruit_BMP280::SAMPLING_X2,    // temperature
                           Adafruit_BMP280::SAMPLING_X16,   // pressure
                           Adafruit_BMP280::FILTER_X16,
                           Adafruit_BMP280::STANDBY_MS_500);
        sensorPresent = true;
        Serial.println("[wx] BMP280 found");
    } else {
        Serial.println("[wx] no BMP280 on I2C (temp/pressure disabled)");
    }
}

bool present() { return sensorPresent; }

String readAprs() {
    if (!sensorPresent) return "";
    bmp280.takeForcedMeasurement();
    float t = bmp280.readTemperature();              // degC
    float p = bmp280.readPressure() / 100.0f;        // hPa
    if (isnan(t) || isnan(p)) return ".../...g...t...";

    String tempStr = tempString((t + Config.wxsensor.temperatureCorrection) * 1.8f + 32.0f);
    String presStr = presString(p + Config.wxsensor.heightCorrection / WX_CORRECTION_FACTOR);

    String wx = ".../...g...t";
    wx += tempStr;
    wx += "h..";          // BMP280 has no humidity
    wx += "b";
    wx += presStr;
    return wx;
}

}  // namespace Wx
