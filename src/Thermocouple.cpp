#include <Adafruit_MAX31855.h>
#include "Thermocouple.h"
#include <SPI.h>

static const int thermoCS = 5;  // must be different from Ethernet CS (4)

// Use hardware SPI
Adafruit_MAX31855 thermocouple(thermoCS);

void setupThermocouple() {
  pinMode(thermoCS, OUTPUT);
  digitalWrite(thermoCS, HIGH); // deselect
  delay(10);

  double temp = thermocouple.readCelsius(); // dummy read
  if (isnan(temp)) {
    Serial.println("MAX31855 not found");
  }
}

double readTemperature() {
  return thermocouple.readCelsius();
}
