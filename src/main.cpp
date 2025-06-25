#include <Arduino.h>
#include "Thermocouple.h"
#include "Display.h"

void setup() {
  Serial.begin(115200);
  setupDisplay();
  setupThermocouple();
}

void loop() {
  double c = readTemperature();
  if (isnan(c)) {
    showError();
  } else {
    showTemperature(c);
  }
  delay(500);
}
