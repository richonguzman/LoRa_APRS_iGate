#include "battery_utils.h"
#include "pins_config.h"

float adcReadingTransformation = (3.3/4095);
float voltageDividerCorrection = 0.288;

namespace BATTERY_Utils {

float checkVoltages() { 
    float sample;
    int sampleSum = 0;
    for (int i=0; i<100; i++) {
        sample = analogRead(batteryPin);
        sampleSum += sample;
        delayMicroseconds(50); 
    }
    return (2 * (sampleSum/100) * adcReadingTransformation) + voltageDividerCorrection;
}

}