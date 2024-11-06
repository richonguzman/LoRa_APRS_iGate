#ifndef NTP_UTILS_H_
#define NTP_UTILS_H_

#include <Arduino.h>


namespace NTP_Utils {

    void    setup();
    void    update();
    String  getFormatedTime();

}

#endif