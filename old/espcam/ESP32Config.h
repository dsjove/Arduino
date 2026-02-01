#pragma once

#include <Preferences.h>
#include "FS.h"
#include <map>

class ESP32Config {
public:
    ESP32Config(const char* ns = NULL, fs::FS* fs = NULL);

    ~ESP32Config();

    bool begin();

    const char* namespaceName() const { return _namespace; }

    String getString(const char* key, const String& defaultValue = "");

    int getInt(const char* key, int defaultValue = 0);

    inline bool getBool(const char* key, bool defaultValue = false)
        {return getInt(key, defaultValue) != 0;}

private:
    Preferences preferences;
    std::map<String, String> configCache;
    const char* _namespace;
    fs::FS* filesystem;

    bool loadCache();
    void clearCache();
};

class ESP32ConfigWrite {
public:
    ESP32ConfigWrite(const char* ns = NULL);

    ~ESP32ConfigWrite();

    void setString(const char* key, const String& value);

    void setInt(const char* key, int value);
    
    inline void setBool(const char* key, int value)
        {setInt(key, value ? 1 : 0);}

private:
    Preferences preferences;
    const char* _namespace;
};

