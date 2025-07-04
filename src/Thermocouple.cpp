#include <Adafruit_MAX31855.h>
#include <SPI.h>
#include "Thermocouple.h"
#include "Pins.h"
#include "SpiDevices.h"

// Define software SPI pins (use unused GPIOs)
constexpr int MAX_SOFT_SCK = 14;
constexpr int MAX_SOFT_MISO = 12;
constexpr int MAX_CS = CS_MAX31855;

Adafruit_MAX31855 thermocouple(MAX_SOFT_SCK, MAX_CS, MAX_SOFT_MISO); // SCK, CS, MISO

// Use hardware SPI
// Adafruit_MAX31855 thermocouple(CS_MAX31855);

// void setupThermocouple() {
//   deselectAllSPI();
//   pinMode(CS_MAX31855, OUTPUT);
//   digitalWrite(CS_MAX31855, HIGH); // deselect
//   delay(10);

//   double temp = thermocouple.readCelsius(); // dummy read
//   if (isnan(temp)) {
//     Serial.println("MAX31855 not found");
//   }
// }

void setupThermocouple()
{
  pinMode(MAX_CS, OUTPUT);
  digitalWrite(MAX_CS, HIGH);
  delay(10);

  double temp = thermocouple.readCelsius();
  if (isnan(temp))
  {
    Serial.println("MAX31855 not found");
  }
}

double readTemperature()
{
  deselectAllSPI();
  digitalWrite(CS_MAX31855, LOW); // Select MAX31855
  double t = thermocouple.readCelsius();
  digitalWrite(CS_MAX31855, HIGH); // Deselect after read
  return t;
}
