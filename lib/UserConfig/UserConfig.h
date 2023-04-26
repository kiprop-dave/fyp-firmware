#ifndef UserConfig_h
#define UserConfig_h
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <WiFi.h>

// Function declarations
void init_spiffs();
String read_file(fs::FS &fs, const char *path);
void write_file(fs::FS &fs, const char *path, const char *message);
bool wifi_connected(String ssid, String password);
void wifi_config();

#endif