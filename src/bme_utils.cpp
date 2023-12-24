#include "bme_utils.h"
#include "configuration.h"
#include "gps_utils.h"
#include "display.h"

#define SEALEVELPRESSURE_HPA (1013.25)
#define HEIGHT_CORRECTION 0             // in meters
#define CORRECTION_FACTOR (8.2296)      // for meters

extern Configuration  Config;
extern String         fifthLine;


namespace BME_Utils {

  #ifdef BME280Sensor
  Adafruit_BME280   bme;
  #endif
  #ifdef BMP280Sensor
  Adafruit_BMP280   bme;
  #endif
  #ifdef BME680Sensor
  Adafruit_BME680 bme; 
  #endif

  void setup() {
    if (Config.bme.active) {
      bool status;
      status = bme.begin(0x76);  // Don't forget to join pins for righ direction on BME280!
      if (!status) {
        Serial.println("Could not find a valid BME280 or BMP280 sensor, check wiring!");
        show_display("ERROR", "", "BME/BMP sensor active", "but no sensor found...");
        while (1); // sacar esto para que quede pegado si no encuentra BME280
      } else {
        #ifdef BME280Sensor
        Serial.println("init : BME280 Module  ...     done!");
        #endif
        #ifdef BMP280Sensor
        Serial.println("init : BMP280 Module  ...     done!");
        #endif
        #ifdef BME680Sensor
        Serial.println("init : BME680 Module  ...     done!");
        #endif   
      }
    } else {
      Serial.println("(BME/BMP sensor not 'active' in 'igate_conf.json')");
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
    float newHum;
    
    float newTemp   = bme.readTemperature();
    #if defined(BME280Sensor) || defined(BME680Sensor)
    newHum = bme.readHumidity();
    #endif
    #ifdef BMP280Sensor
    newHum = 0;
    #endif
    float newPress  = (bme.readPressure() / 100.0F);

    #ifdef BME680Sensor
    float newGas = bme.gas_resistance / 1000.0; // in Kilo ohms
    #endif
    
    //bme.readAltitude(SEALEVELPRESSURE_HPA) // this is for approximate Altitude Calculation.
    
    if (isnan(newTemp) || isnan(newHum) || isnan(newPress)) {
      Serial.println("BME/BMP Module data failed");
      wx = ".../...g...t...r...p...P...h..b.....";
      fifthLine = "";
      return wx;
    } else {
      tempStr = generateTempString((newTemp * 1.8) + 32);
      #if defined(BME280Sensor) || defined(BME680Sensor)
      humStr  = generateHumString(newHum,type);
      #endif
      #ifdef BMP280Sensor
      humStr  = "..";
      #endif
      presStr = generatePresString(newPress + (HEIGHT_CORRECTION/CORRECTION_FACTOR));
      fifthLine = "BME-> " + String(int(newTemp))+"C " + humStr + "% " + presStr.substring(0,4) + "hPa";
      wx = ".../...g...t" + tempStr + "r...p...P...h" + humStr + "b" + presStr;
      #ifdef BME680Sensor
      wx += "Gas: " + String(newGas) + "Kohms";
      #endif
      return wx;
    }
  }

}