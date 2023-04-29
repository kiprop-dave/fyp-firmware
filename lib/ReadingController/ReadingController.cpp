#include "ReadingController.h"
#include <Arduino.h>
#include <ArduinoJson.h>

ApplicationReading::ApplicationReading() {
  char ideal[] = "ideal";
  char empty[] = "";
  this->status.decision = ideal;
  this->status.enclosure[0] = empty;
  this->status.enclosure[1] = empty;
  this->avian.temperature = 0.0;
  this->avian.humidity = 0.0;
  this->reptilian.temperature = 0.0;
  this->reptilian.humidity = 0.0;
}

/*
 *This method is used to set the status of the reading.
 *The parameter is a pointer to the readingStatus struct.
 *The method copies the data from the struct into the private variable of the class.
 */
void ApplicationReading::set_status(readingStatus *status) {
  Serial.print("Setting status,decision: ");
  Serial.println(status->decision);
  memcpy(&this->status, status, sizeof(readingStatus));
  Serial.print("Status.decision set to: ");
  Serial.println(this->status.decision);
}

/*
 *This method is used to set the reading of the avian and reptilian enclosures.
 *The two parameters are pointers to the reading structs of the avian and reptilian enclosures.
 *The method copies the data from the structs into the private variables of the class.
 */
void ApplicationReading::set_reading(reading *avian, reading *reptilian) {
  memcpy(&this->avian, avian, sizeof(reading));
  memcpy(&this->reptilian, reptilian, sizeof(reading));
}

/*
 *This method is used to convert the data in the class into a JSON string.
 *The method uses the ArduinoJson library to convert the data into a JSON string.
 *The method returns the JSON string.
 */
String ApplicationReading::stringify_reading() {
  StaticJsonDocument<512> doc;

  // Create a nested "status" object and add "decision" and "enclosure" fields
  JsonObject status = doc.createNestedObject("status");
  // Use strncpy to copy the character array to a char array and ensure null termination
  char decision[10];
  strncpy(decision, this->status.decision, sizeof(decision) - 1);
  decision[sizeof(decision) - 1] = '\0';
  status["decision"] = decision;
  JsonArray enclosure = status.createNestedArray("enclosure");
  char enclosure0[10];
  strncpy(enclosure0, this->status.enclosure[0], sizeof(enclosure0) - 1);
  enclosure0[sizeof(enclosure0) - 1] = '\0';
  enclosure.add(enclosure0);
  char enclosure1[10];
  strncpy(enclosure1, this->status.enclosure[1], sizeof(enclosure1) - 1);
  enclosure1[sizeof(enclosure1) - 1] = '\0';
  enclosure.add(enclosure1);

  // Create nested "avian" object and add "temperature" and "humidity" fields
  JsonObject avian = doc.createNestedObject("avian");
  avian["temperature"] = this->avian.temperature;
  avian["humidity"] = this->avian.humidity;

  // Create nested "reptilian" object and add "temperature" and "humidity" fields
  JsonObject reptilian = doc.createNestedObject("reptilian");
  reptilian["temperature"] = this->reptilian.temperature;
  reptilian["humidity"] = this->reptilian.humidity;

  // Serialize the JSON document to a String
  String output;
  serializeJson(doc, output);

  // Return the JSON string
  return output;
}

/*
 *This method is used to reset the data in the class.
 *The method sets the status to "ideal" and the readings to 0.0.
 */
void ApplicationReading::reset() {
  char ideal[] = "ideal";
  char empty[] = "";
  this->status.decision = ideal;
  this->status.enclosure[0] = empty;
  this->status.enclosure[1] = empty;
  this->avian.temperature = 0.0;
  this->avian.humidity = 0.0;
  this->reptilian.temperature = 0.0;
  this->reptilian.humidity = 0.0;
}