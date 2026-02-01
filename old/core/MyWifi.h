#pragma once

#include <time.h>
#include <array>

class MyWifi {
public:
  MyWifi();

  bool begin(const char* ssid, const char* password, int timeoutSecs = 5);
};
