#include <Arduino.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <PubSubClient.h>
#include <ReadingController.h> // This is used to create and stringify the readings
#include <UserConfig.h>        // This is used to configure the wifi credentials and the limits for the readings
#include <WiFi.h>

/*
 *Initialize the reading class
 *This class is used to create and stringify the readings
 */
ApplicationReading application_reading;
/*
 *Initialize the limits that will be used to check the readings and determine the status of the enclosures
 */
ApplicationLimits application_limits;

/*
 * A function to check if the limits file exists
 *if the limits file exists, the limits are set and the server is ended
 *if the limits file does not exist, the get_limits() function is called
 *The get_limits() function starts the server and waits for the user to set the limits
 */
void setLimits() {
  bool file_exists = limits_file_exists();
  Serial.println("Checking if limits file exists");
  if (file_exists) {
    Limits limits = read_limits_from_file();
    application_limits.set_limits(&limits);
    if (server_running) {
      server.end();
      server_running = false;
    }
  } else {
    if (!server_running) {
      get_limits();
    }
    delay(3500);
  }
}

/*
 * DHT sensor pins and type
 * The first pin is for the avian enclosure
 * The second pin is for the reptile enclosure
 * The type is the type of DHT sensor
 */
#define DHTPIN1 4
#define DHTPIN2 13
#define DHTTYPE DHT11

/*
 * Create two DHT objects
 * The first object is for the avian enclosure
 * The second object is for the reptile enclosure
 */
DHT dhtAvian(DHTPIN1, DHTTYPE);
DHT dhtReptilian(DHTPIN2, DHTTYPE);

/*
 * MQTT server details
 * The server address can be an IP address or a domain name
 * The port is the port that the MQTT server is listening on
 * The username and password are the credentials for the MQTT server
 */
const char *mqtt_server = "192.168.100.68";
const char *mqtt_user = "<username>";
const char *mqtt_password = "<password>";
const uint16_t mqtt_port = 1883;

/*
 * GPIO pins used for LED1,LED2 and button
 * LED connected to sirenPin simulates a siren
 * The reset_wifi_pin is used to reset the WiFi credentials
 * The setLimitsPin is used to delete the limits and set new limits
 * LED connected to avianIdealPin, avianWarning and avianCritical indicate the status of the avian enclosure
 * LED connected to reptileIdealPin, reptileWarning and reptileCritical indicate the status of the reptile enclosure
 */
const uint8_t reset_wifi_pin = 15;
const uint8_t setLimitsPin = 23;
const uint8_t sirenPin = 2;
const uint8_t avianIdealPin = 21;
const uint8_t avianWarning = 19;
const uint8_t avianCritical = 18;
const uint8_t reptileIdealPin = 27;
const uint8_t reptileWarning = 26;
const uint8_t reptileCritical = 25;
const uint8_t stop_siren = 22;

/*
 * A function to initialize all the pins
 */
void initPins() {
  pinMode(sirenPin, OUTPUT);
  pinMode(avianIdealPin, OUTPUT);
  pinMode(avianWarning, OUTPUT);
  pinMode(avianCritical, OUTPUT);
  pinMode(reptileIdealPin, OUTPUT);
  pinMode(reptileWarning, OUTPUT);
  pinMode(reptileCritical, OUTPUT);
  pinMode(setLimitsPin, INPUT_PULLUP);
  pinMode(reset_wifi_pin, INPUT_PULLUP);
  pinMode(stop_siren, INPUT_PULLUP);
}

/*
 * A function to turn off all the LEDs
 */
void turn_off_leds() {
  digitalWrite(avianIdealPin, LOW);
  digitalWrite(avianWarning, LOW);
  digitalWrite(avianCritical, LOW);
  digitalWrite(reptileIdealPin, LOW);
  digitalWrite(reptileWarning, LOW);
  digitalWrite(reptileCritical, LOW);
  digitalWrite(sirenPin, LOW);
}

/*
 *A boolean to indicate if server state has been reset at startup
 */
bool reset = false;

/*
 * Timers for checking MQTT messages and reading DHT sensor
 * This prevents blocking the main thread by using delay() and allows the ESP32 to handle other tasks
 */
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
  Reading measurements;
  while (true) {
    float h1 = dhtAvian.readHumidity();
    float t1 = dhtAvian.readTemperature();
    float h2 = dhtReptilian.readHumidity();
    float t2 = dhtReptilian.readTemperature();
    if (!isnan(h1) && !isnan(t1) && !isnan(h2) && !isnan(t2)) {
      measurements.avianTemp = t1;
      measurements.avianHumidity = h1;
      measurements.reptileTemp = t2;
      measurements.reptileHumidity = h2;
      break;
    }
  }

  reading avian_unit;
  reading reptilian_unit;
  avian_unit.temperature = measurements.avianTemp;
  avian_unit.humidity = measurements.avianHumidity;
  reptilian_unit.temperature = measurements.reptileTemp;
  reptilian_unit.humidity = measurements.reptileHumidity;
  application_reading.set_reading(&avian_unit, &reptilian_unit);
  return measurements;
}

/*
 *Initialize the PubSubClient class by passing in the WiFiClient object
 */
WiFiClient espClient;
PubSubClient client(espClient);

/*
 * A function to handle MQTT messages
 * It takes three arguments: a pointer to a char array representing the topic of the message,
 * a byte array representing the payload of the message,
 * and an unsigned integer representing the length of the payload.
 */
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
 * A function to set the reading property of the application_reading object
 *The status object looks like this:
 * {
 *  "decision": "ideal",
 * "enclosure": [
 *  "avian",
 * "temperature"
 * ],
 * }
 * It takes five arguments: a pointer to a char array representing the enclosure,
 * a pointer to a char array representing the measurement,
 * a boolean representing if the reading is a warning,
 * a boolean representing if the reading is critical,
 * and a pointer to an unsigned integer representing the severity of the reading.
 */
void set_reading_status(char *enclosure, char *measurement, bool warning, bool critical, uint8_t *severity) {
  readingStatus status;
  if (!warning && !critical && *severity == 0) {
    *severity = 0;
    char ideal[] = "ideal";
    Serial.println(ideal);
    char empty[] = "";
    status.decision = ideal;
    status.enclosure[0] = empty;
    status.enclosure[1] = empty;
    application_reading.set_status(&status);
  } else if (warning && !critical && *severity <= 1) {
    *severity = 1;
    char warning[] = "warning";
    Serial.println(warning);
    status.decision = warning;
    status.enclosure[0] = enclosure;
    status.enclosure[1] = measurement;
    application_reading.set_status(&status);
  } else if (critical && *severity <= 2) {
    *severity = 2;
    char critical[] = "critical";
    Serial.println(critical);
    status.decision = critical;
    status.enclosure[0] = enclosure;
    status.enclosure[1] = measurement;
    application_reading.set_status(&status);
  }
  Serial.print("decision: ");
  Serial.println(status.decision);
}

/*
 *A function to check a reading and return if it's "ideal","warning" or "critical"
 *It takes a float as an argument and a string to indicate the enclosure which can be "avian" or "reptile"
 *It also takes a reading type that can be "temperature" or "humidity"
 *It also takes a pointer to a uint8_t to indicate the severity of the reading
 *severity ensures that the status is not downgraded and only set to the highest severity
 */
String analyzeReading(float reading, char *enclosure, char *readingType, uint8_t *severity) {
  bool warning = false;
  bool critical = false;
  Limits limits = application_limits.get_limits();
  if (strcmp(enclosure, "avian") == 0) {
    uint16_t lowest_temp = limits.avian_temp_limits[0];
    uint16_t ideal_lowest_temp = limits.avian_temp_limits[1];
    uint16_t ideal_highest_temp = limits.avian_temp_limits[2];
    uint16_t highest_temp = limits.avian_temp_limits[3];
    uint16_t lowest_humidity = limits.avian_humid_limits[0];
    uint16_t ideal_lowest_humidity = limits.avian_humid_limits[1];
    uint16_t ideal_highest_humidity = limits.avian_humid_limits[2];
    uint16_t highest_humidity = limits.avian_humid_limits[3];
    if (strcmp(readingType, "temperature") == 0) {
      warning = reading < ideal_lowest_temp && reading >= lowest_temp || reading > ideal_highest_temp && reading <= highest_temp;
      critical = reading < lowest_temp || reading > highest_temp;
      set_reading_status(enclosure, readingType, warning, critical, severity);
    } else {
      warning = reading < ideal_lowest_humidity && reading >= lowest_humidity || reading > ideal_highest_humidity && reading <= highest_humidity;
      critical = reading < lowest_humidity || reading > highest_humidity;
      set_reading_status(enclosure, readingType, warning, critical, severity);
    }
  } else {
    uint16_t lowest_temp = limits.reptile_temp_limits[0];
    uint16_t ideal_lowest_temp = limits.reptile_temp_limits[1];
    uint16_t ideal_highest_temp = limits.reptile_temp_limits[2];
    uint16_t highest_temp = limits.reptile_temp_limits[3];
    uint16_t lowest_humidity = limits.reptile_humid_limits[0];
    uint16_t ideal_lowest_humidity = limits.reptile_humid_limits[1];
    uint16_t ideal_highest_humidity = limits.reptile_humid_limits[2];
    uint16_t highest_humidity = limits.reptile_humid_limits[3];
    if (strcmp(readingType, "temperature") == 0) {
      warning = reading < ideal_lowest_temp && reading >= lowest_temp || reading > ideal_highest_temp && reading <= highest_temp;
      critical = reading < lowest_temp || reading > highest_temp;
      set_reading_status(enclosure, readingType, warning, critical, severity);
    } else {
      warning = reading < ideal_lowest_humidity && reading >= lowest_humidity || reading > ideal_highest_humidity && reading <= highest_humidity;
      critical = reading < lowest_humidity || reading > highest_humidity;
      set_reading_status(enclosure, readingType, warning, critical, severity);
    }
  }
  if (critical == true) {
    return "critical";
  } else if (warning == true) {
    return "warning";
  } else {
    return "ideal";
  }
}

/*
 * A function to connect to the MQTT server
 */
void connectMqtt() {
  // Loop until the connection is established
  String id = "esp_client";
  id.concat(millis());
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(id.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("connected");
      // subscribe to the siren topic
      client.subscribe("siren");
      // publish a reset message at startup to reset server state
      if (reset == false) {
        client.publish("/siren/off", "reset");
        reset = true;
      }
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/*
 *Setup function that runs once at the start of the program
 */
void setup() {
  Serial.begin(115200);
  wifi_config();
  dhtAvian.begin();
  dhtReptilian.begin();
  initPins();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  delay(1000);
  connectMqtt();
  setLimits();
}

/*
 *Loop function that runs continuously
 */
void loop() {
  // Reconnect to WiFi if connection is lost
  if (WiFi.status() != WL_CONNECTED) {
    if (!server_running) {
      wifi_config();
    }
    delay(3500);
    return;
  }

  if (!client.connected()) {
    connectMqtt();
  }

  if (application_limits.limits_are_set() == false) {
    setLimits();
    return;
  }

  if (digitalRead(setLimitsPin) == LOW) {
    turn_off_leds();
    delete_file_abstraction("/limits.json");
    application_limits.reset_limits();
    setLimits();
    return;
  }

  if (digitalRead(reset_wifi_pin) == LOW) {
    turn_off_leds();
    delete_file_abstraction("/wifi.txt ");
    delete_file_abstraction("/password.txt");
    wifi_config();
    return;
  }

  // Check for MQTT messages every second
  if (millis() - lastCheck >= 1000) {
    client.loop();
    lastCheck = millis();
  }

  // Read temperature and humidity from DHT sensor every 15 seconds
  if (millis() - lastRead >= 15000) {
    Reading reading = getReadings();
    char avian_char[] = "avian";
    char reptile_char[] = "reptilian";
    char temperature_char[] = "temperature";
    char humidity_char[] = "humidity";
    uint8_t severity = 0; // 0 = ideal, 1 = warning, 2 = critical
    String avianTempStatus = analyzeReading(reading.avianTemp, avian_char, temperature_char, &severity);
    String avianHumidityStatus = analyzeReading(reading.avianHumidity, avian_char, humidity_char, &severity);
    String reptileTempStatus = analyzeReading(reading.reptileTemp, reptile_char, temperature_char, &severity);
    String reptileHumidityStatus = analyzeReading(reading.reptileHumidity, reptile_char, humidity_char, &severity);

    if (avianTempStatus == "critical" || avianHumidityStatus == "critical") {
      // Serial.println("Avian critical");
      digitalWrite(avianIdealPin, LOW);
      digitalWrite(avianWarning, LOW);
      digitalWrite(avianCritical, HIGH);
    } else if (avianTempStatus == "warning" || avianHumidityStatus == "warning") {
      // Serial.println("Avian warning");
      digitalWrite(avianIdealPin, LOW);
      digitalWrite(avianWarning, HIGH);
      digitalWrite(avianCritical, LOW);
    } else {
      // Serial.println("Avian ideal");
      digitalWrite(avianWarning, LOW);
      digitalWrite(avianCritical, LOW);
      digitalWrite(avianIdealPin, HIGH);
    }

    if (reptileTempStatus == "critical" || reptileHumidityStatus == "critical") {
      // Serial.println("Reptile critical");
      digitalWrite(reptileIdealPin, LOW);
      digitalWrite(reptileWarning, LOW);
      digitalWrite(reptileCritical, HIGH);
    } else if (reptileTempStatus == "warning" || reptileHumidityStatus == "warning") {
      // Serial.println("Reptile warning");
      digitalWrite(reptileIdealPin, LOW);
      digitalWrite(reptileWarning, HIGH);
      digitalWrite(reptileCritical, LOW);
    } else {
      // Serial.println("Reptile ideal");
      digitalWrite(reptileWarning, LOW);
      digitalWrite(reptileCritical, LOW);
      digitalWrite(reptileIdealPin, HIGH);
    }

    /*
     *Turn off the siren if the avian and reptile enclosures are ideal and the siren is on
     *This automates the process of turning off the siren and does not require the user to press the off button
     *Also turn on the siren if the avian or reptile enclosures are critical and the siren is off
     */
    bool sirenOn = digitalRead(sirenPin) == HIGH;
    if (sirenOn) {
      bool avianIdeal = digitalRead(avianIdealPin) == HIGH;
      bool reptileIdeal = digitalRead(reptileIdealPin) == HIGH;
      if (avianIdeal && reptileIdeal) {
        client.publish("/siren/off", "reset");
        Serial.println("Siren off");
        digitalWrite(sirenPin, LOW);
      }
    }
    if (!sirenOn) {
      bool avianEncCritical = digitalRead(avianCritical) == HIGH;
      bool reptileEnvCritical = digitalRead(reptileCritical) == HIGH;
      if (avianEncCritical || reptileEnvCritical) {
        Serial.println("Siren on");
        digitalWrite(sirenPin, HIGH);
      }
    }

    String stringifiedReading = application_reading.stringify_reading();
    client.publish("readings", stringifiedReading.c_str());
    application_reading.reset();
    lastRead = millis();
  }

  // Turn off the siren if the stop button is pressed and the both enclosures are not critical
  if (digitalRead(stop_siren) == LOW && digitalRead(sirenPin) == HIGH) {
    Serial.println("Siren off");
    bool avian_critical = digitalRead(avianCritical) == HIGH;
    bool reptile_critical = digitalRead(reptileCritical) == HIGH;
    if (!avian_critical && !reptile_critical) {
      client.publish("/siren/off", "reset");
      Serial.println("Siren off");
      digitalWrite(sirenPin, LOW);
    }
  }
}