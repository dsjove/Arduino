#include "Camera.h"
#include "CamServer.h"
#include "Flash.h"
#include "MySDCard.h"
#include "ESP32Config.h"

#include "shared/core/MyTime.cpp"
#include "shared/core/MyWifi.cpp"

MySDCard _sdCard;
Camera _camera;
MyWifi _wifi;
CamServer _server;

bool begin() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println();

  if (_sdCard.begin()) {
  }
  else {
  }

  Serial.print("Configuration: ");
  ESP32Config config("server", _sdCard);
  if (config.begin()) {
    Serial.println("success");
  }
  else {
    Serial.println("failed");
  }

  Serial.print("Camera: ");
  if (_camera.begin(config)) {
    Serial.println("success");
  }
  else {
    Serial.println("failed");
    return false;
  }

  if (_wifi.begin(
      config.getString("ssid").c_str(), 
      config.getString("password").c_str())) {
  }
  else {
    return false;
  }

  Serial.print("Time: ");
  struct tm time;
  if (MyTime::configureTime(
      (MyTime::TimeZone)config.getInt("timezone", MyTime::TimeZone::CST),
      config.getBool("dlst", true), 
      &time)) {
    char buffer[27];
    MyTime::timestamp(buffer, &time);
    Serial.println(buffer);
  }
  else {
    Serial.println("failed");
    return false;
  }

  Serial.print("HTTP Server: ");
  if (_server.begin(
      config.getString("name", "Christof").c_str(), 
      config.getString("service", "espcam").c_str())) {
    Serial.printf("'%s' %s\n", _server.service().c_str(), _server.url().c_str());
  }
  else {
    return false;
  }

  return true;
}

void setup() {
  if (begin()) {
    Flash::setIntensity(255);
    delay(250);
    Flash::setIntensity(0);
  }
  else {
    ESP.restart();
  }
}

void loop() {
  // Everything is done in another task by the web server
  delay(10000);
}
