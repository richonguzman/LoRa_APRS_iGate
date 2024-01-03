#include "configuration.h"
#include "power_utils.h"
#include "pins_config.h"

#if defined(TTGO_T_Beam_V1_0) || defined(TTGO_T_Beam_V1_0_SX1268) || defined(TTGO_T_Beam_V1_2) || defined(TTGO_T_Beam_V1_2_SX1262)
#define I2C_SDA 21
#define I2C_SCL 22
#define IRQ_PIN 35
#endif

#if defined(TTGO_T_Beam_V1_0) || defined(TTGO_T_Beam_V1_0_SX1268)
XPowersAXP192 PMU;
#endif
#if defined(TTGO_T_Beam_V1_2) || defined(TTGO_T_Beam_V1_2_SX1262)
XPowersAXP2101 PMU;
#endif

extern Configuration    Config;


namespace POWER_Utils {

  bool   BatteryIsConnected = false;
  String batteryVoltage = "";
  String batteryChargeDischargeCurrent = "";

  void activateMeasurement() {
    #if defined(TTGO_T_Beam_V1_0) || defined(TTGO_T_Beam_V1_0_SX1268) || defined(TTGO_T_Beam_V1_2) || defined(TTGO_T_Beam_V1_2_SX1262)
    PMU.disableTSPinMeasure();
    PMU.enableBattDetection();
    PMU.enableVbusVoltageMeasure();
    PMU.enableBattVoltageMeasure();
    PMU.enableSystemVoltageMeasure();
    #endif
  }

  void activateLoRa() {
    #if defined(TTGO_T_Beam_V1_0) || defined(TTGO_T_Beam_V1_0_SX1268)
    PMU.setLDO2Voltage(3300);
    PMU.enableLDO2();
    #endif
    #if defined(TTGO_T_Beam_V1_2) || defined(TTGO_T_Beam_V1_2_SX1262)
    PMU.setALDO2Voltage(3300);
    PMU.enableALDO2();
    #endif
  }

  void deactivateLoRa() {
    #if defined(TTGO_T_Beam_V1_0) || defined(TTGO_T_Beam_V1_0_SX1268)
    PMU.disableLDO2();
    #endif
    #if defined(TTGO_T_Beam_V1_2) || defined(TTGO_T_Beam_V1_2_SX1262)
    PMU.disableALDO2();
    #endif
  }

  bool begin(TwoWire &port) {
    #if defined (TTGO_T_LORA_V2_1) || defined(HELTEC_V2) || defined(ESP32_DIY_LoRa) || defined(HELTEC_V3) || defined(ESP32_DIY_1W_LoRa)
    return true;
    #endif

    #if defined(TTGO_T_Beam_V1_0) || defined(TTGO_T_Beam_V1_0_SX1268)
    bool result = PMU.begin(Wire, AXP192_SLAVE_ADDRESS, I2C_SDA, I2C_SCL);
    if (result) {
      PMU.disableDC2();
      PMU.disableLDO2();
      PMU.disableLDO3();
      PMU.setDC1Voltage(3300);
      PMU.enableDC1();
      PMU.setProtectedChannel(XPOWERS_DCDC3);
      PMU.disableIRQ(XPOWERS_AXP192_ALL_IRQ);
    }
    return result;
    #endif

    #if defined(TTGO_T_Beam_V1_2) || defined(TTGO_T_Beam_V1_2_SX1262)
    bool result = PMU.begin(Wire, AXP2101_SLAVE_ADDRESS, I2C_SDA, I2C_SCL);
    if (result) {
      PMU.disableDC2();
      PMU.disableDC3();
      PMU.disableDC4();
      PMU.disableDC5();
      PMU.disableALDO1();
      PMU.disableALDO4();
      PMU.disableBLDO1();
      PMU.disableBLDO2();
      PMU.disableDLDO1();
      PMU.disableDLDO2();
      PMU.setDC1Voltage(3300);
      PMU.enableDC1();
      PMU.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
    }
    return result;
    #endif
  }

  void setup() {
    Wire.end();
    #if defined(TTGO_T_Beam_V1_0) || defined(TTGO_T_Beam_V1_0_SX1268)
    Wire.begin(SDA, SCL);
    if (begin(Wire)) {
      Serial.println("AXP192 init done!");
    } else {
      Serial.println("AXP192 init failed!");
    }
    activateLoRa();
    //activateGPS();
    activateMeasurement();
    PMU.setChargerTerminationCurr(XPOWERS_AXP192_CHG_ITERM_LESS_10_PERCENT);
    PMU.setChargeTargetVoltage(XPOWERS_AXP192_CHG_VOL_4V2);
    PMU.setChargerConstantCurr(XPOWERS_AXP192_CHG_CUR_780MA);
    PMU.setSysPowerDownVoltage(2600);
    #endif

    #if defined(TTGO_T_Beam_V1_2) || defined(TTGO_T_Beam_V1_2_SX1262)
    Wire.begin(SDA, SCL);
    if (begin(Wire)) {
      Serial.println("AXP2101 init done!");
    } else {
      Serial.println("AXP2101 init failed!");
    }
    activateLoRa();
    //activateGPS();
    activateMeasurement();
    PMU.setPrechargeCurr(XPOWERS_AXP2101_PRECHARGE_200MA);
    PMU.setChargerTerminationCurr(XPOWERS_AXP2101_CHG_ITERM_25MA);
    PMU.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);
    PMU.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_800MA);
    PMU.setSysPowerDownVoltage(2600);
    #endif
  }

  /*void lowerCpuFrequency() {
    #if defined(TTGO_T_Beam_V1_0) || defined(TTGO_T_Beam_V1_0_SX1268) || defined(TTGO_T_Beam_V1_2) || defined(ESP32_DIY_LoRa_GPS) || defined(TTGO_T_LORA_V2_1_GPS) || defined(TTGO_T_LORA_V2_1_TNC) || defined(ESP32_DIY_1W_LoRa_GPS) || defined(TTGO_T_Beam_V1_2_SX1262)
    if (setCpuFrequencyMhz(80)) {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Main", "CPU frequency set to 80MHz");
    } else {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_WARN, "Main", "CPU frequency unchanged");
    }
    #endif
  }*/

}