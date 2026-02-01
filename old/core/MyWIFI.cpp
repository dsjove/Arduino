#include "MyWifi.h"
#include <WiFi.h>

MyWifi::MyWifi() {
}

bool MyWifi::begin(const char* ssid, const char* password, int timeoutSecs) {
  WiFi.begin(ssid, password);
  //WiFi.setSleep(false);
  Serial.printf("WiFi '%s':", ssid);
  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i < timeoutSecs) {
      Serial.print(".");
      delay(1000);
      i++;
  }
  if (WiFi.status() == WL_CONNECTED) {
	Serial.println(WiFi.localIP());
    return true;
  }
  Serial.println("failed");
  return false;
}
