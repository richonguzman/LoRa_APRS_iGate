#include "battery_utils.h"
#include "configuration.h"
#include "pins_config.h"

extern Configuration    Config;

float adcReadingTransformation = (3.3/4095);
float voltageDividerCorrection = 0.288;

// for External Voltage Measurment (MAX = 15Volts !!!)
float R1 = 100.000; //in Kilo-Ohms
float R2 = 27.000; //in Kilo-Ohms
float readingCorrection = 0.125;
float multiplyCorrection = 0.035;

namespace BATTERY_Utils {

    float checkBattery() { 
        int sample;
        int sampleSum = 0;
        for (int i=0; i<100; i++) {
            sample = analogRead(batteryPin);
            sampleSum += sample;
            delayMicroseconds(50); 
        }
        return (2 * (sampleSum/100) * adcReadingTransformation) + voltageDividerCorrection;
    }

    float checkExternalVoltage() {
        int sample;
        int sampleSum = 0;
        for (int i=0; i<100; i++) {
            sample = analogRead(Config.externalVoltagePin);
            sampleSum += sample;
            delayMicroseconds(50); 
        }
        return ((((sampleSum/100)* adcReadingTransformation) + readingCorrection) * ((R1+R2)/R2)) - multiplyCorrection;
    }

}