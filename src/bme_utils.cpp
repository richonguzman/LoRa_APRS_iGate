#include "bme_utils.h"
#include "configuration.h"
#include "gps_utils.h"
#include "display.h"

extern Configuration Config;

namespace BME_Utils {

Adafruit_BME280   bme;

void setup() {
  if (Config.bme.active) {
    bool status;
    status = bme.begin(0x76);  // Don't forget to join pins for righ direction on BME280!
    if (!status) {
      Serial.println("Could not find a valid BME280 sensor, check wiring!");
      show_display("ERROR", "", "BME sensor active", "but no sensor found...");
      while (1); // sacar esto para que quede pegado si no encuentra BME280
    } else {
      Serial.println("init : BME280 Module  ...     done!");
    }
  } else {
    Serial.println("(BME not 'active' in 'igate_config.json')");
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
  float newTemp   = bme.readTemperature();
  float newHum    = bme.readHumidity();
  float newPress  = (bme.readPressure() / 100.0F);
  //float bat       = analogRead(battery);
  //bme.readAltitude(SEALEVELPRESSURE_HPA)
  
  if (isnan(newTemp) || isnan(newHum) || isnan(newPress)) {
    Serial.println("BME280 Module data failed");
    wx = ".../...g...t...r...p...P...h..b.....";
    return wx;
  } else {
    tempStr = generateTempString((newTemp * 1.8) + 32);
    humStr  = generateHumString(newHum);
    presStr = generatePresString(newPress);
    wx = ".../...g...t" + tempStr + "r...p...P...h" + humStr + "b" + presStr;
    return wx;
  }
}

}