#include "bme_utils.h"
#include "configuration.h"
#include "gps_utils.h"
#include "display.h"

#define SEALEVELPRESSURE_HPA (1013.25)
#define CORRECTION_FACTOR (8.2296)      // for meters

extern Configuration    Config;
extern String           fifthLine;

float newHum, newTemp, newPress, newGas;

int         wxModuleType        = 0;
uint8_t     wxModuleAddress     = 0x00;

Adafruit_BME280     bme280;
Adafruit_BME680     bme680;
#ifdef HELTEC_V3_GPS
Adafruit_BMP280     bmp280(&Wire1);
#else
Adafruit_BMP280     bmp280;
#endif



namespace BME_Utils {

    void getWxModuleAddres() {
        uint8_t err, addr;
        for(addr = 1; addr < 0x7F; addr++) {
            #ifdef HELTEC_V3
            Wire1.beginTransmission(addr);
            err = Wire1.endTransmission();
            #else
            Wire.beginTransmission(addr);
            err = Wire.endTransmission();
            #endif
            if (err == 0) {
                if (addr == 0x76 || addr == 0x77) {
                    wxModuleAddress = addr;
                    return;
                }
            }
        }
    }

    void setup() {
        if (Config.bme.active) {
            getWxModuleAddres();
            if (wxModuleAddress != 0x00) {
                bool wxModuleFound = false;
                #ifdef HELTEC_V3
                    if (bme280.begin(wxModuleAddress, &Wire1)) {
                        Serial.println("BME280 sensor found");
                        wxModuleType = 1;
                        wxModuleFound = true;
                    } 
                    if (!wxModuleFound) {
                        if (bme680.begin(wxModuleAddress, &Wire1)) {
                            Serial.println("BME680 sensor found");
                            wxModuleType = 3;
                            wxModuleFound = true;
                        }
                    }
                #else
                    if (bme280.begin(wxModuleAddress)) {
                        Serial.println("BME280 sensor found");
                        wxModuleType = 1;
                        wxModuleFound = true;
                    }
                    if (!wxModuleFound) {
                        if (bme680.begin(wxModuleAddress)) {
                            Serial.println("BME680 sensor found");
                            wxModuleType = 3;
                            wxModuleFound = true;
                        }
                    }
                #endif
                if (!wxModuleFound) {
                    if (bmp280.begin(wxModuleAddress)) {
                        Serial.println("BMP280 sensor found");
                        wxModuleType = 2;
                        wxModuleFound = true;
                    }
                }
                if (!wxModuleFound) {
                    show_display("ERROR", "", "BME/BMP sensor active", "but no sensor found...", 2000);
                    Serial.println("BME/BMP sensor Active in config but not found! Check Wiring");
                } else {
                    switch (wxModuleType) {
                        case 1:
                            bme280.setSampling(Adafruit_BME280::MODE_FORCED,
                                        Adafruit_BME280::SAMPLING_X1,
                                        Adafruit_BME280::SAMPLING_X1,
                                        Adafruit_BME280::SAMPLING_X1,
                                        Adafruit_BME280::FILTER_OFF
                                        );
                            Serial.println("BME280 Module init done!");
                            break;
                        case 2:
                            bmp280.setSampling(Adafruit_BMP280::MODE_FORCED,
                                        Adafruit_BMP280::SAMPLING_X1,
                                        Adafruit_BMP280::SAMPLING_X1,
                                        Adafruit_BMP280::FILTER_OFF
                                        ); 
                            Serial.println("BMP280 Module init done!");
                            break;
                        case 3:
                            bme680.setTemperatureOversampling(BME680_OS_1X);
                            bme680.setHumidityOversampling(BME680_OS_1X);
                            bme680.setPressureOversampling(BME680_OS_1X);
                            bme680.setIIRFilterSize(BME680_FILTER_SIZE_0);
                            Serial.println("BMP680 Module init done!");
                            break;
                    }
                }
            }            
        }
    }

    String generateTempString(float bmeTemp) {
        String strTemp;
        strTemp = String((int)bmeTemp);
        switch (strTemp.length()) {
            case 1:
                return "00" + strTemp;
                break;
            case 2:
                return "0" + strTemp;
                break;
            case 3:
                return strTemp;
                break;
            default:
                return "-999";
        }
    }

    String generateHumString(float bmeHum) {
        String strHum;
        strHum = String((int)bmeHum);
        switch (strHum.length()) {
            case 1:
                return "0" + strHum;
                break;
            case 2:
                return strHum;
                break;
            case 3:
                if ((int)bmeHum == 100) {
                return "00";
                } else {
                return "-99";
                }
                break;
            default:
                return "-99";
        }
    }

    String generatePresString(float bmePress) {
        String strPress;
        strPress = String((int)bmePress);
        switch (strPress.length()) {
            case 1:
                return "000" + strPress + "0";
                break;
            case 2:
                return "00" + strPress + "0";
                break;
            case 3:
                return "0" + strPress + "0";
                break;
            case 4:
                return strPress + "0";
                break;
            case 5:
                return strPress;
                break;
            default:
                return "-99999";
        }
    }

    String readDataSensor() {
        String wx, tempStr, humStr, presStr;
        switch (wxModuleType) {
            case 1: // BME280
                bme280.takeForcedMeasurement();
                newTemp     = bme280.readTemperature();
                newPress    = (bme280.readPressure() / 100.0F);
                newHum      = bme280.readHumidity();
                break;
            case 2: // BMP280
                bmp280.takeForcedMeasurement();
                newTemp     = bmp280.readTemperature();
                newPress    = (bmp280.readPressure() / 100.0F);
                newHum      = 0;
                break;
            case 3: // BME680
                bme680.performReading();
                delay(50);
                if (bme680.endReading()) {
                    newTemp     = bme680.temperature;
                    newPress    = (bme680.pressure / 100.0F);
                    newHum      = bme680.humidity;
                    newGas      = bme680.gas_resistance / 1000.0; // in Kilo ohms
                }
                break;
        }    

        if (isnan(newTemp) || isnan(newHum) || isnan(newPress)) {
            Serial.println("BME/BMP Module data failed");
            wx = ".../...g...t...r...p...P...h..b.....";
            fifthLine = "";
            return wx;
        } else {
            tempStr = generateTempString(((newTemp + Config.bme.temperatureCorrection) * 1.8) + 32);
            if (wxModuleType == 1 || wxModuleType == 3) {
                humStr  = generateHumString(newHum);
            } else if (wxModuleType == 2) {
                humStr  = "..";
            }
            presStr = generatePresString(newPress + (Config.bme.heightCorrection/CORRECTION_FACTOR));
            fifthLine = "BME-> " + String(int(newTemp))+"C " + humStr + "% " + presStr.substring(0,4) + "hPa";
            wx = ".../...g...t" + tempStr + "r...p...P...h" + humStr + "b" + presStr;
            if (wxModuleType == 3) {
                wx += "Gas: " + String(newGas) + "Kohms";
            }
            return wx;
        }
    }

}