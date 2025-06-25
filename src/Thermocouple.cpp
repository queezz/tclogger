#include "Thermocouple.h"

// Pins
static const int thermoCLK = 18;
static const int thermoCS  = 5;
static const int thermoDO  = 19;

Adafruit_MAX31855 thermocouple(thermoCLK, thermoCS, thermoDO);

void setupThermocouple() {
  if (isnan(thermocouple.readCelsius())) {
    Serial.println("MAX31855 not found");
  }
}

double readTemperature() {
  return thermocouple.readCelsius();
}
