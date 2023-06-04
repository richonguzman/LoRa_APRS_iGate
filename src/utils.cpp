#include "utils.h"

namespace utils {

char *ax25_base91enc(char *s, uint8_t n, uint32_t v) {
  /* Creates a Base-91 representation of the value in v in the string */
  /* pointed to by s, n-characters long. String length should be n+1. */
  for(s += n, *s = '\0'; n; n--) {
    *(--s) = v % 91 + 33;
    v /= 91;
  }
  return(s);
}

static String padding(unsigned int number, unsigned int width) {
    String result;
    String num(number);
    if (num.length() > width) {
        width = num.length();
    }
    for (unsigned int i = 0; i < width - num.length(); i++) {
        result.concat('0');
    }
    result.concat(num);
    return result;
}

String createDateString(time_t t) {
    return String(padding(year(t), 4) + "-" + padding(month(t), 2) + "-" + padding(day(t), 2));
}

String createTimeString(time_t t) {
    return String(padding(hour(t), 2) + ":" + padding(minute(t), 2) + ":" + padding(second(t), 2));
}

}