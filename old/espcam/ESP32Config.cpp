#include "ESP32Config.h"

ESP32Config::ESP32Config(const char* ns, fs::FS* fs)
  : _namespace(ns)
  , filesystem(fs) {
}

ESP32ConfigWrite::ESP32ConfigWrite(const char* ns)
  : _namespace(ns) {
    if (_namespace) {
        preferences.begin(_namespace, false);
    }
 }

bool ESP32Config::begin() {
    if (_namespace) {
        preferences.begin(_namespace, false);
        return loadCache();
    }
    return true;
}

ESP32Config::~ESP32Config() {
    if (_namespace) {
        preferences.end();
    }
}

ESP32ConfigWrite::~ESP32ConfigWrite() {
    if (_namespace) {
        preferences.end();
    }
}

String ESP32Config::getString(const char* key, const String& defaultValue) {
    // Allow changes to file overwrite prefs
    if (configCache.find(key) != configCache.end()) {
        if (_namespace) {
            // Have prefs reflect changes from SD card
            preferences.putString(key, configCache[key]);
        }
        return configCache[key];
    }
    // If not in file, then get from prefs
    if (_namespace && preferences.isKey(key)) {
        return preferences.getString(key, "");
    }
    // Write the paramterized default to prefs
    if (_namespace) {
        preferences.putString(key, defaultValue);
    }

    return defaultValue;
}

int ESP32Config::getInt(const char* key, int defaultValue) {
    // Allow changes to file overwrite prefs
    if (configCache.find(key) != configCache.end()) {
        int intValue = configCache[key].toInt();
        if (_namespace) {
            // Have prefs reflect changes from SD card
            preferences.putInt(key, intValue);
        }
        return intValue;
    }
    // If not in file, then get from prefs
    if (_namespace && preferences.isKey(key)) {
        return preferences.getInt(key, 0);
    }
    // Write the paramterized default to prefs
    if (_namespace) {
        preferences.putInt(key, defaultValue);
    }

    return defaultValue;
}

void ESP32ConfigWrite::setString(const char* key, const String& value) {
    if (!_namespace) {
        return;
    }
    preferences.putString(key, value);
}

void ESP32ConfigWrite::setInt(const char* key, int value) {
    if (!_namespace) {
        return;
    }
    preferences.putInt(key, value);
}

void ESP32Config::clearCache() {
    configCache.clear();
}

bool ESP32Config::loadCache() {
    if (!filesystem || !_namespace) {
        return true;
    }

    String defaultFilePath = "/" + String(_namespace) + "_config.txt";

    File file = filesystem->open(defaultFilePath, "r");
    if (!file) {
        return false;
    }

    String line;
    while (file.available()) {
        line = file.readStringUntil('\n');
        if (line.isEmpty()) continue;
        if (line[0] == '#') continue;
        int delimiterIndex = line.indexOf('=');
        if (delimiterIndex == -1) continue;
        String name = line.substring(0, delimiterIndex);
        String value = line.substring(delimiterIndex + 1);
        configCache[name] = value;
    }
    file.close();
    return true;
}
