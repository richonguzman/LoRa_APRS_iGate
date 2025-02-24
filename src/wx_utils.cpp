#include <TinyGPS++.h>
#include "configuration.h"
#include "board_pinout.h"
#include "wx_utils.h"
#include "display.h"


#define SEALEVELPRESSURE_HPA    (1013.25)
#define CORRECTION_FACTOR       (8.2296)      // for meters

extern Configuration            Config;
extern String                   fifthLine;
#ifdef HAS_GPS
extern TinyGPSPlus              gps;
#endif

int         wxModuleType        = 0;
uint8_t     wxModuleAddress     = 0x00;

float newHum, newTemp, newPress, newGas;


Adafruit_BME280     bme280;
#if defined(HELTEC_V3) || defined(HELTEC_V3_2)
Adafruit_BMP280     bmp280(&Wire1);
Adafruit_Si7021     sensor = Adafruit_Si7021();
#else
Adafruit_BMP280     bmp280;
Adafruit_BME680     bme680;
Adafruit_Si7021     sensor = Adafruit_Si7021();
#endif



namespace WX_Utils {

    void getWxModuleAddres() {
        uint8_t err, addr;
        for(addr = 1; addr < 0x7F; addr++) {
            #if defined(HELTEC_V3) || defined(HELTEC_V3_2) || defined(HELTEC_WSL_V3) || defined(HELTEC_WSL_V3_DISPLAY)
                Wire1.beginTransmission(addr);
                err = Wire1.endTransmission();
            #else
                Wire.beginTransmission(addr);
                err = Wire.endTransmission();
            #endif
            if (err == 0) {
                //Serial.println(addr); this shows any connected board to I2C
                if (addr == 0x76 || addr == 0x77) { // BME/BMP
                    wxModuleAddress = addr;
                    return;
                } else if (addr == 0x40) {          // Si7011
                    wxModuleAddress = addr;
                    return;
                }
            }
        }
    }

    void setup() {
        if (Config.wxsensor.active) {
            getWxModuleAddres();
            if (wxModuleAddress != 0x00) {
                bool wxModuleFound = false;
                if (wxModuleAddress == 0x76 || wxModuleAddress == 0x77) {
                    #if defined(HELTEC_V3) || defined(HELTEC_V3_2) || defined(HELTEC_WSL_V3) || defined(HELTEC_WSL_V3_DISPLAY)
                        if (bme280.begin(wxModuleAddress, &Wire1)) {
                            Serial.println("BME280 sensor found");
                            wxModuleType = 1;
                            wxModuleFound = true;
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
                } else if (wxModuleAddress == 0x40) {
                    if(sensor.begin()) {
                        Serial.println("Si7021 sensor found");
                        wxModuleType = 4;
                        wxModuleFound = true;
                    }
                }                
                if (!wxModuleFound) {
                    displayShow("ERROR", "", "BME/BMP/Si7021 sensor active", "but no sensor found...", 2000);
                    Serial.println("BME/BMP/Si7021 sensor Active in config but not found! Check Wiring");
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
                            #if !defined(HELTEC_V3) && !defined(HELTEC_V3_2)
                                bme680.setTemperatureOversampling(BME680_OS_1X);
                                bme680.setHumidityOversampling(BME680_OS_1X);
                                bme680.setPressureOversampling(BME680_OS_1X);
                                bme680.setIIRFilterSize(BME680_FILTER_SIZE_0);
                                Serial.println("BMP680 Module init done!");
                            #endif
                            break;
                    }
                }
            }
        }
    }

    String generateTempString(const float sensorTemp) {
        String strTemp = String((int)sensorTemp);
        switch (strTemp.length()) {
            case 1:
                return "00" + strTemp;
            case 2:
                return "0" + strTemp;
            case 3:
                return strTemp;
            default:
                return "-999";
        }
    }

    String generateHumString(const float sensorHum) {
        String strHum = String((int)sensorHum);
        switch (strHum.length()) {
            case 1:
                return "0" + strHum;
            case 2:
                return strHum;
            case 3:
                if ((int)sensorHum == 100) {
                    return "00";
                } else {
                    return "-99";
                }
            default:
                return "-99";
        }
    }

    String generatePresString(const float sensorPres) {
        String strPress = String((int)sensorPres);
        String decPress = String(int((sensorPres - int(sensorPres)) * 10));
        switch (strPress.length()) {
            case 1:
                return "000" + strPress + decPress;
            case 2:
                return "00" + strPress + decPress;
            case 3:
                return "0" + strPress + decPress;
            case 4:
                return strPress + decPress;
            case 5:
                return strPress;
            default:
                return "-99999";
        }
    }

    String readDataSensor() {
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
                #if !defined(HELTEC_V3) && !defined(HELTEC_V3_2)
                    bme680.performReading();
                    delay(50);
                    if (bme680.endReading()) {
                        newTemp     = bme680.temperature;
                        newPress    = (bme680.pressure / 100.0F);
                        newHum      = bme680.humidity;
                        newGas      = bme680.gas_resistance / 1000.0; // in Kilo ohms
                    }
                #endif
                break;
            case 4: // Si7021
                newTemp     = sensor.readTemperature();
                newPress    = 0;
                newHum      = sensor.readHumidity();
                break;
        }    

        if (isnan(newTemp) || isnan(newHum) || isnan(newPress)) {
            Serial.println("BME/BMP/Si7021 Module data failed");
            fifthLine = "";
            return ".../...g...t...";
        } else {
            String tempStr = generateTempString(((newTemp + Config.wxsensor.temperatureCorrection) * 1.8) + 32);
            
            String humStr;
            if (wxModuleType == 1 || wxModuleType == 3 || wxModuleType == 4) {
                humStr  = generateHumString(newHum);
            } else if (wxModuleType == 2) {
                humStr  = "..";
            }
            
            String presStr = (wxModuleAddress == 4) 
                ? "....." 
            #ifdef HAS_GPS
                : generatePresString(newPress + (gps.altitude.meters() / CORRECTION_FACTOR));
            #else
                : generatePresString(newPress + (Config.wxsensor.heightCorrection / CORRECTION_FACTOR));
            #endif
                       
            fifthLine = "BME-> ";
            fifthLine += String(int(newTemp + Config.wxsensor.temperatureCorrection));
            fifthLine += "C ";
            fifthLine += humStr;
            fifthLine += "% ";
            fifthLine += presStr.substring(0,4);
            fifthLine += "hPa";

            String wxPayload = ".../...g...t";
            wxPayload += tempStr;
            wxPayload += "h";
            wxPayload += humStr;
            wxPayload += "b";
            wxPayload += presStr;

            if (wxModuleType == 3) {
                wxPayload += "Gas: ";
                wxPayload += String(newGas);
                wxPayload += "Kohms";
            }
            return wxPayload;
        }
    }

}