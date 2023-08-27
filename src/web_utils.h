#ifndef WEB_UTILS_H_
#define WEB_UTILS_H_


#include "configuration.h"
#include <Arduino.h>

extern Configuration Config;

//
const char* htmlPage1 = "<html><body><h1>Page 1</h1></body></html>";
//const char* htmlPage2 = "<html><body><h1>Page 2</h1></body></html>";
const char* htmlPage3 = "<!DOCTYPE html><html><head><title>Dynamic Content Example</title></head><body><h1>LoRa iGate Configuration Data</h1><p>Callsign: %VCALLSIGN%</p></body></html>";

//


namespace WEB_Utils {

/*void checkWiFi();
void startWiFi();
void setup();*/

}

#endif