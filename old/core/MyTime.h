#pragma once

namespace MyTime {
  enum TimeZone {
    UTC = 0,      // UTC (Coordinated Universal Time)
    EST = -18000, // Eastern Standard Time (UTC-5)
    CST = -21600, // Central Standard Time (UTC-6)
    MST = -25200, // Mountain Standard Time (UTC-7)
    PST = -28800  // Pacific Standard Time (UTC-8)
  };

  bool configureTime(TimeZone timezone, bool dlst, struct tm* now = NULL);

  bool timestamp(char buffer[27], struct tm* time = NULL);
}
