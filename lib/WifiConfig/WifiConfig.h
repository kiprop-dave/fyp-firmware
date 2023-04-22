#ifndef WifiConfig_h
#define WifiConfig_h
#include <Arduino.h>
#include <SPIFFS.h>
#include <WiFi.h>

// Function declarations
void init_spiffs();
String read_file(fs::FS &fs, const char *path);
void write_file(fs::FS &fs, const char *path, const char *message);
bool wifi_connected(String ssid, String password);
void wifi_config();

#endif