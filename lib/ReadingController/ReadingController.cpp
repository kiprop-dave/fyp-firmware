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
  StaticJsonDocument<256> doc;
  JsonObject status = doc.createNestedObject("status");
  status["decision"] = this->status.decision;
  JsonArray enclosure = status.createNestedArray("enclosure");
  enclosure.add(this->status.enclosure[0]);
  enclosure.add(this->status.enclosure[1]);
  JsonObject avian = doc.createNestedObject("avian");
  avian["temperature"] = this->avian.temperature;
  avian["humidity"] = this->avian.humidity;
  JsonObject reptilian = doc.createNestedObject("reptilian");
  reptilian["temperature"] = this->reptilian.temperature;
  reptilian["humidity"] = this->reptilian.humidity;
  String output;
  serializeJson(doc, output);
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