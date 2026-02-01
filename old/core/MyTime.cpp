#include "MyTime.h"
#include <time.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"

namespace MyTime {

  bool configureTime(TimeZone timezone, bool dlst, struct tm* now) {
    configTime(timezone, dlst ? 3600 : 0, "pool.ntp.org");
    struct tm local;
    return getLocalTime(now ? now : &local);
  }

//// timeval from camera does not have usecs and localtime does not use config time!
//bool timestamp(char buffer[27], struct timeval tv) {
//  Serial.printf("(%d, %d)", tv.tv_sec, tv.tv_usec);
//  struct tm *timeinfo = localtime(&tv.tv_sec);
//  snprintf(buffer, 27, "%04d-%02d-%02d_%02d-%02d-%02d-%06ld",
//      timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
//      timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, tv.tv_usec);
//  return true;
//}

  bool timestamp(char buffer[27], struct tm* time) {
    struct tm* formatting;
    struct tm timeinfo;
    if (time) {
      formatting = time;
    }
    else if (getLocalTime(&timeinfo)) {
      formatting = &timeinfo;
    }
    else {
      return false;
    }
    unsigned long microseconds = micros() % 1000000;
    snprintf(buffer, 27, "%04d-%02d-%02d_%02d-%02d-%02d-%06ld",
        formatting->tm_year + 1900, formatting->tm_mon + 1, formatting->tm_mday,
        formatting->tm_hour, formatting->tm_min, formatting->tm_sec, microseconds);
    return true;
  }
}

#pragma GCC diagnostic pop
