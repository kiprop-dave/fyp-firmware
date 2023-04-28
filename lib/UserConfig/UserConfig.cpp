#include "UserConfig.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

// Set web server port number to 80
AsyncWebServer server(80);

void init_spiffs() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  Serial.println("SPIFFS mounted successfully");
}

String read_file(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return String();
  }

  String fileContent;
  while (file.available()) {
    fileContent = file.readStringUntil('\n');
    break;
  }
  file.close();
  return fileContent;
}

void write_file(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

void delete_file(fs::FS &fs, const char *path) {
  Serial.printf("Deleting file: %s\r\n", path);
  if (fs.remove(path)) {
    Serial.println("- file deleted");
  } else {
    Serial.println("- delete failed");
  }
}

void delete_file_abstraction(const char *path) {
  delete_file(SPIFFS, path);
}

bool wifi_connected(String ssid, String password) {
  if (ssid == "" || password == "") {
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.println("Connecting to WiFi network: " + ssid);
  unsigned long start = millis();
  unsigned long elapsed = 0;

  while (WiFi.status() != WL_CONNECTED) {
    elapsed = millis() - start;
    if (elapsed > 5000) {
      Serial.println("Connection timed out");
      return false;
    }
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  return true;
}

void wifi_config() {
  // Initialize SPIFFS
  init_spiffs();

  // Read SSID and password from file
  String ssid = read_file(SPIFFS, "/ssid.txt");
  String password = read_file(SPIFFS, "/password.txt");
  Serial.println("SSID: " + ssid);
  Serial.println("Password: " + password);

  bool connected = wifi_connected(ssid, password);
  if (connected) {
    Serial.println("WiFi connected successfully");
    server.end();
  } else {
    Serial.println("WiFi connection failed");
    Serial.println("Starting access point");

    // Set WiFi to AP mode
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32-Access-Point", "password");
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    // Start web server
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/index.html", "text/html");
    });

    // Serve static files
    server.serveStatic("/", SPIFFS, "/");

    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      String ssid = request->arg("ssid");
      String password = request->arg("pass");
      Serial.println("SSID: " + ssid);
      Serial.println("Password: " + password);

      // Write SSID and password to file
      write_file(SPIFFS, "/ssid.txt", ssid.c_str());
      write_file(SPIFFS, "/password.txt", password.c_str());

      request->send(200, "text/plain", "Credentials saved. Restarting ESP32... ");
      delay(1000);
      // Restart ESP32
      ESP.restart();
    });
    server.begin();
  }
}

// A function to read a json file and return a char pointer
char *read_file_json(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return NULL;
  }

  size_t fileSize = file.size();
  char *buffer = (char *)malloc(fileSize + 1); // allocate memory for file contents
  if (!buffer) {
    Serial.println("- failed to allocate memory");
    return NULL;
  }

  size_t bytesRead = file.readBytes(buffer, fileSize); // read file contents into buffer
  buffer[bytesRead] = '\0';                            // terminate buffer with null character
  file.close();
  return buffer;
}

// A function to save the limits to a json file
void save_limits_config(Limits *limits) {
  // Initialize SPIFFS
  init_spiffs();

  // Create a json object
  StaticJsonDocument<384> doc;

  JsonArray avian_temp = doc.createNestedArray("avian_temp");
  avian_temp.add(limits->avian_temp_limits[0]);
  avian_temp.add(limits->avian_temp_limits[1]);
  avian_temp.add(limits->avian_temp_limits[2]);
  avian_temp.add(limits->avian_temp_limits[3]);

  JsonArray avian_humid = doc.createNestedArray("avian_humid");
  avian_humid.add(limits->avian_humid_limits[0]);
  avian_humid.add(limits->avian_humid_limits[1]);
  avian_humid.add(limits->avian_humid_limits[2]);
  avian_humid.add(limits->avian_humid_limits[3]);

  JsonArray rept_temp = doc.createNestedArray("rept_temp");
  rept_temp.add(limits->reptile_temp_limits[0]);
  rept_temp.add(limits->reptile_temp_limits[1]);
  rept_temp.add(limits->reptile_temp_limits[2]);
  rept_temp.add(limits->reptile_temp_limits[3]);

  JsonArray rept_humid = doc.createNestedArray("rept_humid");
  rept_humid.add(limits->reptile_humid_limits[0]);
  rept_humid.add(limits->reptile_humid_limits[1]);
  rept_humid.add(limits->reptile_humid_limits[2]);
  rept_humid.add(limits->reptile_humid_limits[3]);

  String json;
  serializeJson(doc, json);

  // Write the json to a file
  write_file(SPIFFS, "/limits.json", json.c_str());
}

// A function to read the limits from a json file
Limits read_limits_from_file() {
  // Initialize SPIFFS
  init_spiffs();

  char *json = read_file_json(SPIFFS, "/limits.json");
  if (json == NULL) {
    Serial.println("Failed to read limits.json");
    free(json);
    return Limits();
  }
  StaticJsonDocument<384> doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    Serial.println("Failed to deserialize limits.json");
    Serial.println(error.c_str());
    free(json);
    return Limits();
  }

  Limits limits;
  JsonArray avian_temp = doc["avian_temp"];
  limits.avian_temp_limits[0] = static_cast<uint16_t>(avian_temp[0]);
  limits.avian_temp_limits[1] = static_cast<uint16_t>(avian_temp[1]);
  limits.avian_temp_limits[2] = static_cast<uint16_t>(avian_temp[2]);
  limits.avian_temp_limits[3] = static_cast<uint16_t>(avian_temp[3]);

  JsonArray avian_humid = doc["avian_humid"];
  limits.avian_humid_limits[0] = static_cast<uint16_t>(avian_humid[0]);
  limits.avian_humid_limits[1] = static_cast<uint16_t>(avian_humid[1]);
  limits.avian_humid_limits[2] = static_cast<uint16_t>(avian_humid[2]);
  limits.avian_humid_limits[3] = static_cast<uint16_t>(avian_humid[3]);

  JsonArray rept_temp = doc["rept_temp"];
  limits.reptile_temp_limits[0] = static_cast<uint16_t>(rept_temp[0]);
  limits.reptile_temp_limits[1] = static_cast<uint16_t>(rept_temp[1]);
  limits.reptile_temp_limits[2] = static_cast<uint16_t>(rept_temp[2]);
  limits.reptile_temp_limits[3] = static_cast<uint16_t>(rept_temp[3]);

  JsonArray rept_humid = doc["rept_humid"];
  limits.reptile_humid_limits[0] = static_cast<uint16_t>(rept_humid[0]);
  limits.reptile_humid_limits[1] = static_cast<uint16_t>(rept_humid[1]);
  limits.reptile_humid_limits[2] = static_cast<uint16_t>(rept_humid[2]);
  limits.reptile_humid_limits[3] = static_cast<uint16_t>(rept_humid[3]);

  free(json);
  return limits;
}

// A function to check if the limits.json file exists
bool limits_file_exists() {
  // Initialize SPIFFS
  init_spiffs();

  bool exists = SPIFFS.exists("/limits.json");
  if (!exists) {
    Serial.println("limits.json does not exist");
    return false;
  }
  return true;
}

// listen for requests to set the limits
void get_limits() {
  server.on("/config/limits", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/params.html", "text/html");
  });

  server.serveStatic("/", SPIFFS, "/");

  server.on("/config/index-20d33217.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index-20d33217.js", "text/javascript");
  });

  server.on("/config/index-b1ad9639.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index-b1ad9639.css", "text/css");
  });

  server.on(
      "/config/limits",
      HTTP_POST,
      [](AsyncWebServerRequest *request) {},
      NULL,
      parse_body);

  server.begin();
}

// Parse the request body
void parse_body(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  StaticJsonDocument<384> doc;
  DeserializationError error = deserializeJson(doc, data, len);
  if (error) {
    Serial.println("Failed to deserialize request body");
    Serial.println(error.c_str());
    request->send(400);
    return;
  }

  // Get the limits from the request body
  Limits limits;
  JsonArray avian_temp = doc["avian_temp"];
  limits.avian_temp_limits[0] = static_cast<uint16_t>(avian_temp[0]);
  limits.avian_temp_limits[1] = static_cast<uint16_t>(avian_temp[1]);
  limits.avian_temp_limits[2] = static_cast<uint16_t>(avian_temp[2]);
  limits.avian_temp_limits[3] = static_cast<uint16_t>(avian_temp[3]);

  JsonArray avian_humid = doc["avian_humid"];
  limits.avian_humid_limits[0] = static_cast<uint16_t>(avian_humid[0]);
  limits.avian_humid_limits[1] = static_cast<uint16_t>(avian_humid[1]);
  limits.avian_humid_limits[2] = static_cast<uint16_t>(avian_humid[2]);
  limits.avian_humid_limits[3] = static_cast<uint16_t>(avian_humid[3]);

  JsonArray rept_temp = doc["rept_temp"];
  limits.reptile_temp_limits[0] = static_cast<uint16_t>(rept_temp[0]);
  limits.reptile_temp_limits[1] = static_cast<uint16_t>(rept_temp[1]);
  limits.reptile_temp_limits[2] = static_cast<uint16_t>(rept_temp[2]);
  limits.reptile_temp_limits[3] = static_cast<uint16_t>(rept_temp[3]);

  JsonArray rept_humid = doc["rept_humid"];
  limits.reptile_humid_limits[0] = static_cast<uint16_t>(rept_humid[0]);
  limits.reptile_humid_limits[1] = static_cast<uint16_t>(rept_humid[1]);
  limits.reptile_humid_limits[2] = static_cast<uint16_t>(rept_humid[2]);
  limits.reptile_humid_limits[3] = static_cast<uint16_t>(rept_humid[3]);

  // Save the limits to the limits.json file
  save_limits_config(&limits);

  // Send a response
  request->send(200, "text/plain", "Limits saved successfully");
}

ApplicationLimits::ApplicationLimits() {}
void ApplicationLimits::set_limits(Limits *limits) {
  this->limits = *limits;
  this->limits_set = true;
}
Limits ApplicationLimits::get_limits() {
  return this->limits;
}

bool ApplicationLimits::limits_are_set() {
  return this->limits_set;
}

void ApplicationLimits::reset_limits() {
  this->limits_set = false;
}