#pragma once

#include <ArduinoBLE.h>
#include <string>
#include <array>
#include <algorithm>

using BLEUUID = std::array<char, 37>;

inline BLEUUID makeUuidWithService(
    const std::string& serviceName,
    const std::string& overrideId)
{
  BLEUUID result;
  if (!overrideId.empty())
  {
    std::copy_n(overrideId.begin(), 37, result.begin());
  }
  else
  {
    result.fill('0');
    result[8] = '-';
    result[36] = 0;
    int pos = 9;
    for (size_t i = 0; i < 12 && i < serviceName.length(); i++) {
      sprintf(result.data() + pos, "%02X", serviceName[i]);
      pos+=2;
      if (i == 1 || i == 3 || i == 5)
      {
         result[pos] = '-';
         pos+=1;
      }
    }
    if (serviceName.length() < 12) {
	  result[pos] = '0';
    }
  }
  return result;
}
