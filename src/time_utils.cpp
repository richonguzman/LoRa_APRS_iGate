#include "time_utils.h"
#include "display.h"
#include "time.h"

extern String   firstLine;
extern String   secondLine;
extern String   thirdLine;
extern String   fourthLine;
extern String   fifthLine;
extern String   sixthLine;
extern String   seventhLine;
extern String   eigthLine;

namespace TIME_Utils {

void getDateTime() {
    struct tm timeinfo;
    String year, month, day, hour, minute, seconds;
    if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time");
        fourthLine =  "no time info... restarting";
        ESP.restart();
    }

    year = (String)(timeinfo.tm_year + 1900);

    month = (String)(timeinfo.tm_mon+1);
    if (month.length() == 1) {
        month = "0" + month;
    }

    day = (String)(timeinfo.tm_mday);
    if (day.length() == 1) {
        day = "0" + day;
    }

    hour = (String)(timeinfo.tm_hour);
    if (hour == 0) {
        hour = "00";
    } else if (hour.length() == 1) {
        hour = "0" + hour;
    }

    minute = (String)(timeinfo.tm_min);
    if (minute == 0) {
        minute = "00";
    } else if (minute.length() == 1) {
        minute = "0" + minute;
    }

    seconds = (String)(timeinfo.tm_sec);
    if (seconds == 0) {
        seconds = "00";
    } else if (seconds.length() == 1) {
        seconds = "0" + seconds;
    }

    fourthLine = year + "/" + month + "/" + day + " - " + hour + ":" + minute + ":" + seconds;
    show_display(firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, seventhLine, eigthLine, 0); 
}

void setup() {
    const char*     ntpServer                   = "pool.ntp.org";
    const long      GMT                         = 0;                    // for receiving UTC time
    const long      gmtOffset_sec               = GMT*60*60;
    const int       daylightOffset_sec          = 3600;

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    //configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    getDateTime();
}



}
