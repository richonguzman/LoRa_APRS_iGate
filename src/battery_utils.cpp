/* Copyright (C) 2025 Ricardo Guzman - CA2RXU
 * 
 * This file is part of LoRa APRS iGate.
 * 
 * LoRa APRS iGate is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or 
 * (at your option) any later version.
 * 
 * LoRa APRS iGate is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with LoRa APRS iGate. If not, see <https://www.gnu.org/licenses/>.
 */

#include <Adafruit_INA219.h>
#include "battery_utils.h"
#include "configuration.h"
#include "board_pinout.h"
#include "power_utils.h"
#include "utils.h"


extern  Configuration                   Config;
extern  uint32_t                        lastBatteryCheck;

bool    shouldSleepLowVoltage           = false;

float   adcReadingTransformation        = (3.3/4095);
int     adcReadings                     = 20;
float   voltageDividerCorrection        = 0.288;
float   readingCorrection               = 0.125;
float   multiplyCorrection              = 0.035;

float   voltageDividerTransformation    = 0.0;

//bool    externalI2CSensorFound          = false;
uint8_t externalI2CSensorAddress        = 0x00;
int     externalI2CSensorType           = 0; // 0 = None | 1 = INA219 | 2 = INA226 | 3 = INA3221

Adafruit_INA219     ina219;


#ifdef HAS_ADC_CALIBRATION
    #include <esp_adc_cal.h>

    #if defined(TTGO_LORA32_V2_1) || defined(TTGO_LORA32_V2_1_915)
        #define InternalBattery_ADC_Channel ADC1_CHANNEL_7  // t_lora32 pin35
        #define ExternalVoltage_ADC_Channel ADC1_CHANNEL_6  // t_lora32 pin34
    #endif

    #if CONFIG_IDF_TARGET_ESP32
    #define ADC_EXAMPLE_CALI_SCHEME ESP_ADC_CAL_VAL_EFUSE_VREF
    #elif CONFIG_IDF_TARGET_ESP32S2
    #define ADC_EXAMPLE_CALI_SCHEME ESP_ADC_CAL_VAL_EFUSE_TP
    #elif CONFIG_IDF_TARGET_ESP32C3
    #define ADC_EXAMPLE_CALI_SCHEME ESP_ADC_CAL_VAL_EFUSE_TP
    #elif CONFIG_IDF_TARGET_ESP32S3
    #define ADC_EXAMPLE_CALI_SCHEME ESP_ADC_CAL_VAL_EFUSE_TP_FIT
    #endif

    esp_adc_cal_characteristics_t adc_chars;
#endif

bool calibrationEnable = false;


namespace BATTERY_Utils {

    float mapVoltage(float voltage, float in_min, float in_max, float out_min, float out_max) {
        return (voltage - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }

    void adcCalibration() {
        #ifdef HAS_ADC_CALIBRATION
            if (calibrationEnable) {
                adc1_config_width(ADC_WIDTH_BIT_12);
                adc1_config_channel_atten(InternalBattery_ADC_Channel, ADC_ATTEN_DB_12);
                adc1_config_channel_atten(ExternalVoltage_ADC_Channel, ADC_ATTEN_DB_12);
            }
        #endif
    }

    void adcCalibrationCheck() {
        #ifdef HAS_ADC_CALIBRATION
            esp_err_t ret;
            ret = esp_adc_cal_check_efuse(ADC_EXAMPLE_CALI_SCHEME);
            /*if (ret == ESP_ERR_NOT_SUPPORTED) {
                Serial.println("Calibration scheme not supported, skip software calibration");
            } else if (ret == ESP_ERR_INVALID_VERSION) {
                Serial.println("eFuse not burnt, skip software calibration");
            } else */
            if (ret == ESP_OK) {
                esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 1100, &adc_chars);
                //Serial.printf("eFuse Vref:%u mV\n", adc_chars.vref);
                calibrationEnable = true;
            } /*else {
                Serial.println("Invalid Calibration Arg");
            }*/
        #endif
    }

    void getI2CVoltageSensorAddress() {
        uint8_t err, addr;
        for(addr = 1; addr < 0x7F; addr++) {
            #if defined(HELTEC_V3) || defined(HELTEC_V3_2) || defined(HELTEC_WSL_V3) || defined(HELTEC_WSL_V3_DISPLAY)
                Wire1.beginTransmission(addr);
                err = Wire1.endTransmission();
            #else
                Wire.beginTransmission(addr);
                err = Wire.endTransmission();
            #endif
            delay(5);
            if (err == 0) {
                if (addr == 0x40) { // INA219
                    externalI2CSensorAddress = addr;
                }
            }
        }
    }

    bool detectINA219(uint8_t addr) {
        ina219 = Adafruit_INA219(addr);
        return ina219.begin();
    }

    void setup() {
        if ((Config.battery.sendExternalVoltage || Config.battery.monitorExternalVoltage) && Config.battery.voltageDividerR2 != 0) voltageDividerTransformation = (Config.battery.voltageDividerR1 + Config.battery.voltageDividerR2) / Config.battery.voltageDividerR2;

        #if defined(HAS_ADC_CALIBRATION)
            if (Config.battery.sendInternalVoltage || Config.battery.monitorInternalVoltage || Config.battery.sendExternalVoltage || Config.battery.monitorExternalVoltage) {
                adcCalibrationCheck();
                adcCalibration();
            }
        #endif

        getI2CVoltageSensorAddress();
        if (externalI2CSensorAddress != 0x00) {
            if (detectINA219(externalI2CSensorAddress)) {
                Serial.println("INA219 sensor found");
                externalI2CSensorType = 1;  // INA219
            }
        }
    }

    float checkInternalVoltage() { 
        #if defined(HAS_AXP192) || defined(HAS_AXP2101)
            if(POWER_Utils::isBatteryConnected()) {
                return POWER_Utils::getBatteryVoltage();
            } else {
                return 0.0;
            }
        #else
            
            #ifdef ADC_CTRL
                POWER_Utils::adc_ctrl_ON();
            #endif

            int sampleSum = 0;
            for (int i = 0; i < adcReadings; i++) {
                #if defined(ESP32_DIY_LoRa) || defined(ESP32_DIY_LoRa_915) || defined(ESP32_DIY_1W_LoRa) || defined(ESP32_DIY_1W_LoRa_915)
                    sampleSum = 0;
                #else
                    #ifdef HAS_ADC_CALIBRATION
                        if (calibrationEnable){
                            sampleSum += adc1_get_raw(InternalBattery_ADC_Channel);
                        } else {
                            sampleSum += analogRead(BATTERY_PIN);
                        }
                    #else
                        #ifdef BATTERY_PIN
                            sampleSum += analogRead(BATTERY_PIN);
                        #else
                            sampleSum += 0;
                        #endif
                    #endif
                #endif
                delay(3); 
            }

            #ifdef ADC_CTRL
                POWER_Utils::adc_ctrl_OFF();

                #ifdef HELTEC_WP_V1
                double inputDivider = (1.0 / (10.0 + 10.0)) * 10.0;  // The voltage divider is a 10k + 10k resistor in series
                #else
                double inputDivider = (1.0 / (390.0 + 100.0)) * 100.0;  // The voltage divider is a 390k + 100k resistor in series, 100k on the low side.
                #endif
                return (((sampleSum/adcReadings) * adcReadingTransformation) / inputDivider) + 0.285; // Yes, this offset is excessive, but the ADC on the ESP32s3 is quite inaccurate and noisy. Adjust to own measurements.
            #else
                #ifdef HAS_ADC_CALIBRATION
                    if (calibrationEnable){
                        float voltage = esp_adc_cal_raw_to_voltage(sampleSum / adcReadings, &adc_chars);
                        voltage *= 2;       // for 100K/100K voltage divider
                        voltage /= 1000;
                        return voltage;
                    } else {
                        return (2 * (sampleSum/adcReadings) * adcReadingTransformation) + voltageDividerCorrection;  // raw voltage without mapping
                    }
                #else
                    #ifdef LIGHTGATEWAY_PLUS_1_0
                        double inputDivider = (1.0 / (560.0 + 100.0)) * 100.0;  // The voltage divider is a 560k + 100k resistor in series, 100k on the low side.
                        return (((sampleSum/adcReadings) * adcReadingTransformation) / inputDivider) + 0.41;
                    #else
                        return (2 * (sampleSum/adcReadings) * adcReadingTransformation) + voltageDividerCorrection;  // raw voltage without mapping
                    #endif
                #endif
            #endif
            // return mapVoltage(voltage, 3.34, 4.71, 3.0, 4.2); // mapped voltage
        #endif
    }

    float checkExternalVoltage() {
        if (externalI2CSensorType == 0) {
            int sample;
            int sampleSum = 0;
            for (int i = 0; i < 100; i++) {
                #ifdef HAS_ADC_CALIBRATION
                    if (calibrationEnable){
                        sample = adc1_get_raw(ExternalVoltage_ADC_Channel);
                    } else {
                        sample = analogRead(Config.battery.externalVoltagePin);
                    }
                #else
                    sample = analogRead(Config.battery.externalVoltagePin);
                #endif
                sampleSum += sample;
                delayMicroseconds(50);
            }

            float extVoltage;
            #ifdef HAS_ADC_CALIBRATION
                if (calibrationEnable) {
                    extVoltage = esp_adc_cal_raw_to_voltage(sampleSum / 100.0, &adc_chars) * voltageDividerTransformation; // in mV
                    extVoltage /= 1000.0;
                } else {
                    extVoltage = ((((sampleSum/100.0)* adcReadingTransformation) + readingCorrection) * voltageDividerTransformation) - multiplyCorrection;
                }
            #else
                extVoltage = ((((sampleSum/100.0)* adcReadingTransformation) + readingCorrection) * voltageDividerTransformation) - multiplyCorrection;
            #endif
            
            return extVoltage; // raw voltage without mapping

            // return mapVoltage(voltage, 5.05, 6.32, 4.5, 5.5); // mapped voltage
        } else if (externalI2CSensorType == 1) { // INA219
            int sampleSum = 0;
            for (int i = 0; i < 100; i++) {
                sampleSum += ina219.getBusVoltage_V() * 1000.0;
                delayMicroseconds(50);
            }
            float extVoltage = sampleSum/100.0;
            return extVoltage/1000.0;
        } else {
            return 0.0;
        }
    }

    void startupBatteryHealth() {
        #ifdef BATTERY_PIN
            if (Config.battery.monitorInternalVoltage && checkInternalVoltage() < Config.battery.internalSleepVoltage + 0.1) {
                shouldSleepLowVoltage = true;
            }
        #endif
        #ifndef HELTEC_WP_V1
            if (Config.battery.monitorExternalVoltage && checkExternalVoltage() < Config.battery.externalSleepVoltage + 0.1) {
                shouldSleepLowVoltage = true;
            }
        #endif
        if (shouldSleepLowVoltage) {
            Utils::checkSleepByLowBatteryVoltage(0);
        }
    }

}