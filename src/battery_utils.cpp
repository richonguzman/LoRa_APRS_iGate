#include <Arduino.h>
#include "battery_utils.h"
#include "configuration.h"
#include "boards_pinout.h"
#include "power_utils.h"
#include "utils.h"
#include "driver/adc.h"
#include <esp_adc_cal.h>

#define EXTCHAN ADC1_CHANNEL_6
#define INTCHAN ADC1_CHANNEL_7
#ifdef TTGO_T_LORA32_V2_1
    #define INTBATTDIVIDER 2 // Internal Battery Divider ratio
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

extern  Configuration               Config;
extern  uint32_t                    lastBatteryCheck;

bool  shouldSleepLowVoltage       = false;
float adcReadingTransformation    = (3.3/4095);
float voltageDividerCorrection    = 0.288;
float readingCorrection = 0.125;
float multiplyCorrection = 0.035;
float extbattdivider = 4.7037; // Calculated for R1 = 100k, R2=27k (With those resistors max voltage input is 15 Volts !!!) Actuall number is calculated on every startup based on R! and R2 values set up in Web GUI.

bool cali_enable = false;
esp_adc_cal_characteristics_t adc_chars;

namespace BATTERY_Utils {

    float mapVoltage(float voltage, float in_min, float in_max, float out_min, float out_max) {
        return (voltage - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }

    float checkInternalVoltage() { 
        float voltage = 0.0;
        #if defined(HAS_AXP192) || defined(HAS_AXP2101)
            if(POWER_Utils::isBatteryConnected()) {
                return POWER_Utils::getBatteryVoltage();
            } else {
                return 0.0;
            }
        #else
            int sample;
            int sampleSum = 0;
            #ifdef ADC_CTRL
                #if defined(HELTEC_WIRELESS_TRACKER)
                    digitalWrite(ADC_CTRL, HIGH);
                #endif
                #if defined(HELTEC_V3) || defined(HELTEC_V2) || defined(HELTEC_WSL_V3)
                    digitalWrite(ADC_CTRL, LOW);
                #endif
            #endif

            for (int i = 0; i < 100; i++) {
                #ifdef TTGO_T_LORA32_V2_1
                    if(cali_enable){
                        sample = adc1_get_raw(INTCHAN);
                    }
                    else{
                        sample = analogRead(BATTERY_PIN);
                    }
                #else
                    #ifndef TTGO_T_LORA32_V2_1
                        #ifdef BATTERY_PIN
                            sample = analogRead(BATTERY_PIN);
                        #endif
                        #if defined(ESP32_DIY_LoRa) || defined(ESP32_DIY_1W_LoRa)
                            sample = 0;
                        #endif
                    #endif
                #endif
                sampleSum += sample;
                delayMicroseconds(50); 
            }

            #ifdef ADC_CTRL
                #if defined(HELTEC_WIRELESS_TRACKER)
                    digitalWrite(ADC_CTRL, LOW);
                #endif
                #if defined(HELTEC_V3) || defined(HELTEC_V2) || defined(HELTEC_WSL_V3)
                    digitalWrite(ADC_CTRL, HIGH);
                #endif
                double inputDivider = (1.0 / (390.0 + 100.0)) * 100.0;  // The voltage divider is a 390k + 100k resistor in series, 100k on the low side.
                voltage = (((sampleSum/100) * adcReadingTransformation) / inputDivider) + 0.285; // Yes, this offset is excessive, but the ADC on the ESP32s3 is quite inaccurate and noisy. Adjust to own measurements.
            #else
                #ifdef TTGO_T_LORA32_V2_1
                    if (cali_enable){
                        voltage = esp_adc_cal_raw_to_voltage(sampleSum / 100, &adc_chars) * INTBATTDIVIDER; // in mV
                        voltage /=1000;
                    }
                    else{
                        voltage = (2 * (sampleSum/100) * adcReadingTransformation) + voltageDividerCorrection;
                    }
                #else
                    voltage = (2 * (sampleSum/100) * adcReadingTransformation) + voltageDividerCorrection; // raw voltage without mapping
                #endif
            #endif

            return voltage;

            // return mapVoltage(voltage, 3.34, 4.71, 3.0, 4.2); // mapped voltage
        #endif
    }

    float checkExternalVoltage() {
        int sample;
        int sampleSum = 0;
        float voltage = 0.0;
        for (int i = 0; i < 100; i++) {
            #ifdef TTGO_T_LORA32_V2_1
                if(cali_enable){    
                    sample = adc1_get_raw(EXTCHAN);
                }
                else{
                    sample = analogRead(Config.battery.externalVoltagePin);
                }
            #else
            sample = analogRead(Config.battery.externalVoltagePin);
            #endif
            
            sampleSum += sample;
            delayMicroseconds(50); 
        }
        #ifdef TTGO_T_LORA32_V2_1
            if (cali_enable){
            voltage = esp_adc_cal_raw_to_voltage(sampleSum / 100, &adc_chars) * extbattdivider; // in mV
            voltage /= 1000;
            }
        else{
            voltage = ((((sampleSum/100)* adcReadingTransformation) + readingCorrection) * extbattdivider) - multiplyCorrection;
        }
        #else
        voltage = ((((sampleSum/100)* adcReadingTransformation) + readingCorrection) * extbattdivider) - multiplyCorrection;
        #endif
        
        return voltage; // raw voltage without mapping

        // return mapVoltage(voltage, 5.05, 6.32, 4.5, 5.5); // mapped voltage
    }

    bool adc_calibration_init()
    {
        esp_err_t ret;

        ret = esp_adc_cal_check_efuse(ADC_EXAMPLE_CALI_SCHEME);
        if (ret == ESP_ERR_NOT_SUPPORTED)
        {
            Serial.println("Calibration scheme not supported, skip software calibration");
        }
        else if (ret == ESP_ERR_INVALID_VERSION)
        {
            Serial.println("eFuse not burnt, skip software calibration");
        }
        else if (ret == ESP_OK)
        {
            cali_enable = true;
            esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 1100, &adc_chars);
            Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
        }
        else
        {
            Serial.println("Invalid arg");
        }
        return cali_enable;
    }

    void configADC() {
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten(INTCHAN, ADC_ATTEN_DB_12);
        adc1_config_channel_atten(EXTCHAN, ADC_ATTEN_DB_12);

        //calculate ext batt divider just once at startup
        if(!Config.battery.externalR1 && !Config.battery.externalR2){ //In case there are no divider values set in the WEB GUI (For example after update) use R1 = 100k, R2 = 27k.
            Config.battery.externalR1 = 100;
            Config.battery.externalR2 = 27;
        }

        extbattdivider = (Config.battery.externalR1 + Config.battery.externalR2)/Config.battery.externalR2;       
    }

    void checkIfShouldSleep() {
        if (lastBatteryCheck == 0 || millis() - lastBatteryCheck >= 15 * 60 * 1000) {
            lastBatteryCheck = millis();            
            if (checkInternalVoltage() < Config.lowVoltageCutOff) {
                ESP.deepSleep(1800000000); // 30 min sleep (60s = 60e6)
            }
        }
    }

    void startupBatteryHealth() {
        #ifdef BATTERY_PIN
            if (Config.battery.monitorInternalVoltage && checkInternalVoltage() < Config.battery.internalSleepVoltage + 0.1) {
                shouldSleepLowVoltage = true;
            }
        #endif
        if (Config.battery.monitorExternalVoltage && checkExternalVoltage() < Config.battery.externalSleepVoltage + 0.1) {
            shouldSleepLowVoltage = true;
        }
        if (shouldSleepLowVoltage) {
            Utils::checkSleepByLowBatteryVoltage(0);
        }
    }

}