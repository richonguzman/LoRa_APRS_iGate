#ifndef QUERY_UTILS_H_
#define QUERY_UTILS_H_

#include <Arduino.h>


namespace QUERY_Utils {

    String process(const String& query, const String& station, bool queryFromAPRSIS, bool thirdParty);

}

#endif