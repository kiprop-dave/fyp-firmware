#ifndef ReadingController_h
#define ReadingController_h
#include <Arduino.h>

struct readingStatus {
  char *decision;
  char *enclosure[2]; // enclosure[0] = "avian",enclosure[1] = "temperature"
};

struct reading {
  float temperature;
  float humidity;
};

class ApplicationReading {
  private:
  readingStatus status;
  reading avian;
  reading reptilian;

  public:
  ApplicationReading();
  void set_status(readingStatus *status);
  void set_reading(reading *avian, reading *reptilian);
  String stringify_reading();
  void reset();
};

#endif