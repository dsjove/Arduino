#pragma once

#include <Arduino.h>

#if defined(ARDUINO_ARCH_ESP32)
  #include <WiFi.h>
#endif

#include <WiFiManager.h>

#include "../PinIO/SBJTask.h"
#include "TheTime.h"

// Forward declare descriptor so we can friend it.
struct TheWifiAutoConnectDesc;

class TheWifi
{
public:
  explicit TheWifi(const char* name,
                   int connectTimeoutSeconds = 15,
                   int portalTimeoutSeconds  = 180,
                   bool syncTimeAfterConnect = true)
  : _name(name ? name : "SBJ")
  , _connectTimeoutSeconds(connectTimeoutSeconds)
  , _portalTimeoutSeconds(portalTimeoutSeconds)
  , _syncTimeAfterConnect(syncTimeAfterConnect)
  , _attempted(false)
  , _ok(false)
  , _manager()
  , _autoConnectTask("WiFi", this, TheWifiAutoConnectDesc{})
  {
  }

  // Kick off WiFiManager autoConnect via SBJTask (one-shot).
  // Safe to call repeatedly; only the first call does anything.
  inline void begin()
  {
    if (_attempted) return;
    _autoConnectTask.begin();
  }

  inline bool attempted() const { return _attempted; }
  inline bool ok() const { return _ok; }

  inline bool connected() const
  {
#if defined(ARDUINO_ARCH_ESP32)
    return WiFi.status() == WL_CONNECTED;
#else
    return false;
#endif
  }

private:
  friend struct TheWifiAutoConnectDesc;

  const char* _name;
  const int   _connectTimeoutSeconds;
  const int   _portalTimeoutSeconds;
  const bool  _syncTimeAfterConnect;

  bool _attempted;
  bool _ok;

  WiFiManager _manager;
  SBJTask     _autoConnectTask;

  void autoConnectCb()
  {
    _attempted = true;

#if defined(ARDUINO_ARCH_ESP32)
    WiFi.mode(WIFI_STA);
#endif

    _manager.setConnectTimeout(_connectTimeoutSeconds);
    _manager.setConfigPortalTimeout(_portalTimeoutSeconds);

    // WiFiManager expects a C string SSID for the portal AP name.
    // Keep this local buffer alive for the call.
    char apName[48];
    apName[0] = '\0';
    snprintf(apName, sizeof(apName), "%s-Setup", _name);

    _ok = _manager.autoConnect(apName);

    if (_ok)
    {
#if defined(ARDUINO_ARCH_ESP32)
      Serial.print("WiFi connected, IP=");
      Serial.println(WiFi.localIP());
#else
      Serial.println("WiFi connected");
#endif

      if (_syncTimeAfterConnect)
      {
        TheTime::reinitTime();
      }
    }
    else
    {
      Serial.println("WiFi not connected (portal timed out or connect failed)");
    }
  }

  struct TheWifiAutoConnectDesc
  {
    using Obj = TheWifi;
    static constexpr void (Obj::*Method)() = &Obj::autoConnectCb;
    static constexpr SBJTask::Schedule schedule{
      10, 1, 0,
      8192, TaskPriority::Medium, 0
    };
  };
};
