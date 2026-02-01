#include "CamServer.h"
#include <format>
#include <ESPmDNS.h>

bool startCameraServer();

CamServer::CamServer() {
}

bool CamServer::begin(const char* name, const char* service) {
  _service = service;
  if (!MDNS.begin(name)) {
    return false;
  }
  int port = 80;
  std::string str(name);
  std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
  _url = std::format("http://{}.local:{}", str.c_str(), port);
  MDNS.addService(service, "tcp", port);
  MDNS.addService("http", "tcp", port);
  return startCameraServer();
}
