#pragma once
#include <string>

class CamServer {
public:
  CamServer();

  bool begin(const char* name, const char* service);

  const std::string& url() const { return _url; }
  const std::string& service() const { return _service; }

private:
  std::string _url;
  std::string _service;
};
