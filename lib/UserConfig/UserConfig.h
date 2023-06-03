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
void delete_file(fs::FS &fs, const char *path);
void delete_file_abstraction(const char *path);
bool wifi_connected(String ssid, String password);
void wifi_config();
extern AsyncWebServer server;
extern bool server_running;

struct Limits {
  uint16_t avian_temp_limits[4];
  uint16_t avian_humid_limits[4];
  uint16_t reptile_temp_limits[4];
  uint16_t reptile_humid_limits[4];
};
bool limits_file_exists();
char *read_file_json(fs::FS &fs, const char *path);
Limits read_limits_from_file();
void save_limits_config(Limits *limits);
void get_limits();
void parse_body(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
class ApplicationLimits {
  private:
  Limits limits;
  bool limits_set;

  public:
  ApplicationLimits();
  void set_limits(Limits *limits);
  Limits get_limits();
  bool limits_are_set();
  void reset_limits();
};

#endif