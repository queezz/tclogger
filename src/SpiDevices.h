#ifndef SPIDEVICES_H
#define SPIDEVICES_H

#include <Arduino.h>
#include "Pins.h"

inline void deselectAllSPI() {
  digitalWrite(CS_SD, HIGH);
  digitalWrite(CS_MAX31855, HIGH);
  digitalWrite(CS_W5500, HIGH);
}

inline void setupSPIChipSelects() {
  pinMode(CS_SD, OUTPUT);
  pinMode(CS_MAX31855, OUTPUT);
  pinMode(CS_W5500, OUTPUT);
  deselectAllSPI();  // Ensure all are HIGH by default
}

#endif
