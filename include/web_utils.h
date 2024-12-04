#ifndef WEB_UTILS_H_
#define WEB_UTILS_H_

#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Arduino.h>
#include <SPIFFS.h>
#include <WiFi.h>


namespace WEB_Utils {

    void handleNotFound(AsyncWebServerRequest *request);
    void handleStatus(AsyncWebServerRequest *request);
    void handleHome(AsyncWebServerRequest *request);
    void handleStyle(AsyncWebServerRequest *request);
    void handleScript(AsyncWebServerRequest *request);
    void handleBootstrapStyle(AsyncWebServerRequest *request);
    void handleBootstrapScript(AsyncWebServerRequest *request);

    void setup();

}

#endif