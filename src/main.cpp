#include <Arduino.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <PubSubClient.h>
#include <WiFi.h>

// Define two pins for DHT sensor
#define DHTPIN1 4
#define DHTPIN2 5
#define DHTTYPE DHT11

// Initialize DHT sensor.
DHT dht1(DHTPIN1, DHTTYPE);
DHT dht2(DHTPIN2, DHTTYPE);

// WiFi
const char *ssid = "<wifi name>";
const char *password = "<wifi password>";

// MQTT
const char *mqtt_server = "<server address>";
const int mqtt_port = 1883;               // Port number for MQTT server
const char *mqtt_user = "<username>";     // specific username for MQTT server
const char *mqtt_password = "<password>"; // specific password for MQTT server

/*
 * GPIO pins used for LED1,LED2 and button
 * LED connected to sirenPin simulates a siren
 * Button is used to turn off the siren
 * LED connected to avianIdealPin, avianWarning and avianCritical indicate the status of the avian enclosure
 * LED connected to reptileIdealPin, reptileWarning and reptileCritical indicate the status of the reptile enclosure
 */
const uint8_t sirenPin = 2;
const uint8_t offButtonPin = 23;
const uint8_t avianIdealPin = 18;
const uint8_t avianWarning = 19;
const uint8_t avianCritical = 21;
const uint8_t reptileIdealPin = 13;
const uint8_t reptileWarning = 12;
const uint8_t reptileCritical = 14;

/*
 * A function to initialize all the pins
 */
void initPins() {
  pinMode(sirenPin, OUTPUT);
  pinMode(offButtonPin, INPUT_PULLDOWN);
  pinMode(avianIdealPin, OUTPUT);
  pinMode(avianWarning, OUTPUT);
  pinMode(avianCritical, OUTPUT);
  pinMode(reptileIdealPin, OUTPUT);
  pinMode(reptileWarning, OUTPUT);
  pinMode(reptileCritical, OUTPUT);
}

// Timers for MQTT and temperature readings
unsigned long lastCheck = 0;
unsigned long lastRead = 0;

/*
 * A struct to hold the readings from the DHT sensors
 */
struct Reading {
  float avianTemp;
  float avianHumidity;
  float reptileTemp;
  float reptileHumidity;
};

/*
 * A function to read the temperature and humidity from the DHT sensor and return a Reading struct
 */
Reading getReadings() {
  Reading reading;
  while (true) {
    float h1 = dht1.readHumidity();
    float t1 = dht1.readTemperature();
    float h2 = dht2.readHumidity();
    float t2 = dht2.readTemperature();
    if (!isnan(h1) && !isnan(t1) && !isnan(h2) && !isnan(t2)) {
      reading.avianTemp = t1;
      reading.avianHumidity = h1;
      reading.reptileTemp = t2;
      reading.reptileHumidity = h2;
      break;
    }
  }
  return reading;
}

/*
 * A function to serialize a Reading struct to JSON and return the JSON string
 * It takes a pointer to a Reading struct as an argument
 */
String serializeReading(Reading *reading) {
  StaticJsonDocument<200> doc;
  JsonObject sensorOne = doc.createNestedObject("sensorOne");
  JsonObject sensorTwo = doc.createNestedObject("sensorTwo");
  sensorOne["temperature"] = reading->avianTemp;
  sensorOne["humidity"] = reading->avianHumidity;
  sensorTwo["temperature"] = reading->reptileTemp;
  sensorTwo["humidity"] = reading->reptileHumidity;
  String readings;
  serializeJson(doc, readings);
  return readings;
}

/*
 *Initialize the PubSubClient class by passing in the WiFiClient object
 */
WiFiClient espClient;
PubSubClient client(espClient);

void callback(char *topic, byte *payload, unsigned int length) {
  // Check if message says "siren on" and turn on led

  Serial.print("Message arrived on topic: ");
  Serial.println(topic);

  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  if (message == "siren on") {
    Serial.println("Siren on");
    digitalWrite(sirenPin, HIGH);
  }
}

/*
 *A function to check a reading and return if it's "ideal","warning" or "critical"
 *It takes a float as an argument and a string to indicate the enclosure which can be "avian" or "reptile"
 *It also takes a readin type that can be "temperature" or "humidity"
 */
String analyzeReading(float reading, String enclosure, String readingType) {
  bool warning = false;
  bool critical = false;
  if (enclosure == "avian") {
    if (readingType == "temperature") {
      warning = reading < 23 && reading >= 21 || reading > 30 && reading <= 35;
      critical = reading < 21 || reading > 35;
    } else {
      warning = reading < 30 && reading >= 25 || reading > 55 && reading <= 60;
      critical = reading < 25 || reading > 60;
    }
  } else {
    if (readingType == "temperature") {
      warning = reading < 22 && reading >= 20 || reading > 28 && reading <= 32;
      critical = reading < 20 || reading > 35;
    } else {
      warning = reading < 30 && reading >= 25 || reading > 70 && reading <= 75;
      critical = reading < 25 || reading > 75;
    }
  }
  if (warning == true) {
    return "warning";
  } else if (critical == true) {
    return "critical";
  } else {
    return "ideal";
  }
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// connect to MQTT server
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
      // ... and resubscribe
      client.subscribe("siren");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// A function to read the temperature and humidity from the DHT sensor and serialize it to JSON and return the JSON string
String readDHT() {
  // Read temperature in Celsius and humidity
  float h1 = dht1.readHumidity();
  float t1 = dht1.readTemperature();
  float h2 = dht2.readHumidity();
  float t2 = dht2.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h1) || isnan(t1) || isnan(h2) || isnan(t2)) {
    Serial.println("Failed to read from DHT sensor!");
    return "failed";
  }

  // Create a nested JSON object
  StaticJsonDocument<200> doc;
  JsonObject sensorOne = doc.createNestedObject("sensorOne");
  JsonObject sensorTwo = doc.createNestedObject("sensorTwo");

  // Add values to the object
  sensorOne["temperature"] = t1;
  sensorOne["humidity"] = h1;
  sensorTwo["temperature"] = t2;
  sensorTwo["humidity"] = h2;

  // Serialize JSON to a string
  String readings;
  serializeJson(doc, readings);

  return readings;
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  dht1.begin();
  dht2.begin();
  pinMode(ledPin, OUTPUT);
  pinMode(offButtonPin, INPUT_PULLDOWN);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  }

  if (!client.connected()) {
    reconnect();
  }

  // Check if 1 second has passed since last check for MQTT messages
  if (millis() - lastCheck >= 1000) {
    client.loop();
    lastCheck = millis();
  }

  // Read temperature and humidity from DHT sensor every 30 seconds
  if (millis() - lastRead >= 15000) {
    Reading reading = getReadings();
    String avianTempStatus = analyzeReading(reading.avianTemp, "avian", "temperature");
    String avianHumidityStatus = analyzeReading(reading.avianHumidity, "avian", "humidity");
    String reptileTempStatus = analyzeReading(reading.reptileTemp, "reptile", "temperature");
    String reptileHumidityStatus = analyzeReading(reading.reptileHumidity, "reptile", "humidity");

    if (avianTempStatus == "warning" || avianHumidityStatus == "warning") {
      Serial.println("Avian warning");
      digitalWrite(avianIdealPin, LOW);
      digitalWrite(avianCritical, LOW);
      digitalWrite(avianWarning, HIGH);
    } else if (avianTempStatus == "critical" || avianHumidityStatus == "critical") {
      Serial.println("Avian critical");
      digitalWrite(avianIdealPin, LOW);
      digitalWrite(avianWarning, LOW);
      digitalWrite(avianCritical, HIGH);
    } else {
      Serial.println("Avian ideal");
      digitalWrite(avianWarning, LOW);
      digitalWrite(avianCritical, LOW);
      digitalWrite(avianIdealPin, HIGH);
    }

    if (reptileTempStatus == "warning" || reptileHumidityStatus == "warning") {
      Serial.println("Reptile warning");
      digitalWrite(reptileIdealPin, LOW);
      digitalWrite(reptileCritical, LOW);
      digitalWrite(reptileWarning, HIGH);
    } else if (reptileTempStatus == "critical" || reptileHumidityStatus == "critical") {
      Serial.println("Reptile critical");
      digitalWrite(reptileIdealPin, LOW);
      digitalWrite(reptileWarning, LOW);
      digitalWrite(reptileCritical, HIGH);
    } else {
      Serial.println("Reptile ideal");
      digitalWrite(reptileWarning, LOW);
      digitalWrite(reptileCritical, LOW);
      digitalWrite(reptileIdealPin, HIGH);
    }

    /*
     *Turn off the siren if the avian and reptile enclosures are ideal and the siren is on
     *This automates the process of turning off the siren and does not require the user to press the off button
     */
    if (digitalRead(sirenPin) == HIGH) {
      bool avianIdeal = digitalRead(avianIdealPin) == HIGH;
      bool reptileIdeal = digitalRead(reptileIdealPin) == HIGH;
      if (avianIdeal && reptileIdeal) {
        client.publish("/siren/off", "reset");
        Serial.println("Siren off");
        digitalWrite(sirenPin, LOW);
      }
    }

    String stringifiedReading = serializeReading(&reading);
    client.publish("readings", stringifiedReading.c_str());
    lastRead = millis();
  }

  // Turn the siren off if the off button is pressed and if avian and reptile enclosures are ideal
  if (digitalRead(offButtonPin) == LOW && digitalRead(sirenPin) == HIGH) {
    bool avianIdeal = digitalRead(avianIdealPin) == HIGH;
    bool reptileIdeal = digitalRead(reptileIdealPin) == HIGH;
    if (avianIdeal && reptileIdeal) {
      client.publish("/siren/off", "reset");
      Serial.println("Siren off");
      digitalWrite(sirenPin, LOW);
    }
  }
}