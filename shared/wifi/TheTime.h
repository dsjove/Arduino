#pragma once

#include <time.h>
#include "lwip/apps/sntp.h"

class TheTime
{
public:
  inline static bool timeValid(time_t minEpoch = 1672531200)
  {
    time_t now;
    time(&now);
    return now > minEpoch;
  }

  inline static void reinitTime(const char* tz = "CST6CDT,M3.2.0,M11.1.0")
  {
    if (!_timeInit) {
      initTime(tz);
    } else {
      // If time was already configured, nudge a resync after reconnect
      resyncTime();
    }
  }

private:
  inline static const char* _tz = nullptr;
  inline static const char* _ntp1 = "pool.ntp.org";
  inline static const char* _ntp2 = "time.nist.gov";
  inline static const char* _ntp3 = "time.google.com";
  inline static bool _timeInit  = false;

  inline static void initTime( const char* tz = nullptr,
                        const char* ntp1 = nullptr,
                        const char* ntp2 = nullptr,
                        const char* ntp3 = nullptr) {
    if (tz && *tz) { _tz = tz; }
    if (ntp1 && *ntp1) { _ntp1 = ntp1; }
    if (ntp2 && *ntp2) { _ntp2 = ntp2; }
    if (ntp3 && *ntp3) { _ntp3 = ntp3; }

    if (_tz) {
      setenv("TZ", _tz, 1);
      tzset();
    }
    configTime(0, 0, _ntp1, _ntp2, _ntp3);
    _timeInit = true;
  }

  inline static void resyncTime() {
    // Safe to call after WiFi reconnects or periodically.
    sntp_stop();
    configTime(0, 0, _ntp1, _ntp2, _ntp3);
  }

  inline static bool getLocalTime(struct tm& out, uint32_t timeoutMs = 0) {
    // If timeoutMs == 0, do a single-shot read.
    // If timeoutMs > 0, wait up to timeoutMs for time to become valid.
    const uint32_t start = millis();
    while (true) {
      time_t now;
      time(&now);
      if (now > 1672531200) { // Jan 1, 2023
        localtime_r(&now, &out);
        return true;
      }
      if (timeoutMs == 0) {
        return false;
      }
      if ((millis() - start) >= timeoutMs) {
        return false;
      }
      delay(10);
    }
  }
};
