#include "UserConfig.h"
#include <Arduino.h>
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
    if (elapsed > 2000) {
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