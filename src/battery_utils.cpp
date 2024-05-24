#include "battery_utils.h"
#include "configuration.h"
#include "boards_pinout.h"

extern Configuration    Config;
extern uint32_t         lastBatteryCheck;

float adcReadingTransformation = (3.3/4095);
float voltageDividerCorrection = 0.288;

// for External Voltage Measurment (MAX = 15Volts !!!)
float R1 = 100.000; //in Kilo-Ohms
float R2 = 27.000; //in Kilo-Ohms
float readingCorrection = 0.125;
float multiplyCorrection = 0.035;


namespace BATTERY_Utils {

    float mapVoltage(float voltage, float in_min, float in_max, float out_min, float out_max) {
        return (voltage - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }

    float checkBattery() { 
        int sample;
        int sampleSum = 0;
        #ifdef ADC_CTRL
            #if defined(HELTEC_WSL_V3) || defined(HELTEC_WIRELESS_TRACKER)
                digitalWrite(ADC_CTRL, HIGH);
            #endif
            #if defined(HELTEC_V3) || defined(HELTEC_V2)
                digitalWrite(ADC_CTRL, LOW);
            #endif
        #endif

        for (int i = 0; i < 100; i++) {
            #ifdef BATTERY_PIN
                sample = analogRead(BATTERY_PIN);
            #endif
            #if defined(ESP32_DIY_LoRa) || defined(ESP32_DIY_1W_LoRa)
                sample = 0;
            #endif
            sampleSum += sample;
            delayMicroseconds(50); 
        }

        #ifdef ADC_CTRL
            #if defined(HELTEC_WSL_V3) || defined(HELTEC_WIRELESS_TRACKER)
                digitalWrite(ADC_CTRL, LOW);
            #endif
            #if defined(HELTEC_V3) || defined(HELTEC_V2)
                digitalWrite(ADC_CTRL, HIGH);
            #endif
            double inputDivider = (1.0 / (390.0 + 100.0)) * 100.0;  // The voltage divider is a 390k + 100k resistor in series, 100k on the low side.
            return (((sampleSum/100) * adcReadingTransformation) / inputDivider) + 0.285; // Yes, this offset is excessive, but the ADC on the ESP32s3 is quite inaccurate and noisy. Adjust to own measurements.
        #else
            return (2 * (sampleSum/100) * adcReadingTransformation) + voltageDividerCorrection; // raw voltage without mapping
        #endif

        // return mapVoltage(voltage, 3.34, 4.71, 3.0, 4.2); // mapped voltage
    }

    float checkExternalVoltage() {
        int sample;
        int sampleSum = 0;
        for (int i = 0; i < 100; i++) {
            sample = analogRead(Config.battery.externalVoltagePin);
            sampleSum += sample;
            delayMicroseconds(50); 
        }

        float voltage = ((((sampleSum/100)* adcReadingTransformation) + readingCorrection) * ((R1+R2)/R2)) - multiplyCorrection;

        return voltage; // raw voltage without mapping

        // return mapVoltage(voltage, 5.05, 6.32, 4.5, 5.5); // mapped voltage
    }

    void checkIfShouldSleep() {
        if (lastBatteryCheck == 0 || millis() - lastBatteryCheck >= 15 * 60 * 1000) {
            lastBatteryCheck = millis();

            float voltage = checkBattery();
            
            if (voltage < Config.lowVoltageCutOff) {
                ESP.deepSleep(1800000000); // 30 min sleep (60s = 60e6)
            }
        }
    }

}