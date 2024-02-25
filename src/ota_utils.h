#ifndef OTA_UTILS_H_
#define OTA_UTILS_H_

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

namespace OTA_Utils {

    void setup(AsyncWebServer *server);
    void onOTAStart();
    void onOTAProgress(size_t current, size_t final);
    void onOTAEnd(bool success);

}

#endif