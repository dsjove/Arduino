#pragma once

#include <WiFi.h>
#include <WiFiManager.h>
#define _TASK_LTS_POINTER
#include <TaskScheduler.h>

#include "TheTime.h"

class TheWifi {
public:
  inline TheWifi(
    Scheduler& sched,
    const std::string& name,
    int connectTimeoutSeconds = 15,
    int portalTimeoutSeconds  = 180,
    bool getTime = true)
  : _name(name)
  , _connectTimeoutSeconds(connectTimeoutSeconds)
  , _portalTimeoutSeconds(portalTimeoutSeconds)
  , _attempted(false)
  , _ok(false)
  , _getTime(getTime)
  // one time task for auto connect
  {
  }

  inline void begin() {
	// auto connect task enable
  }

private:
  const std::string _name;
  const int _connectTimeoutSeconds;
  const int _portalTimeoutSeconds;
  bool _attempted;
  bool _ok;
  bool _getTime;
  Task _autoConnect;

  WiFiManager manager;

  inline void _autoConnectCb() {
    _attempted = true;

    WiFi.mode(WIFI_STA);

    manager.setConnectTimeout(_connectTimeoutSeconds);
    manager.setConfigPortalTimeout(_portalTimeoutSeconds);

    _ok = manager.autoConnect((_name + "-Setup").c_str(), nullptr);

    if (_ok) {
      Serial.println("WiFi connected");
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());

      // Initialize SNTP time on successful WiFi connection (if not already done)
      if (_getTime)
      {
        TheTime::reinitTime();
	  }

    } else {
      Serial.println("WiFi not connected (portal timed out or connect failed)");
    }

    //autoConnectTask.disable();
  }
};
