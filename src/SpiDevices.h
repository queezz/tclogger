#ifndef SPIDEVICES_H
#define SPIDEVICES_H

#include <Arduino.h>
#include "Pins.h"

inline void deselectAllSPI()
{
  digitalWrite(CS_SD, HIGH);
  // digitalWrite(CS_MAX31855, HIGH); // On another SPI
}

inline void setupSPIChipSelects()
{
  pinMode(CS_SD, OUTPUT);
  pinMode(CS_MAX31855, OUTPUT);
  deselectAllSPI(); // Ensure all are HIGH by default
}

// Wrapper for safe SPI access
template <typename Func>
void withSPIChip(int csPin, Func func)
{
  deselectAllSPI();
  digitalWrite(csPin, LOW);  // Select the target
  func();                    // Run the action
  digitalWrite(csPin, HIGH); // Deselect afterward
}

#endif
