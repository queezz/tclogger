#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "Pins.h"
#include "SpiDevices.h"

static const int sdCS = CS_SD;
static bool sdReady = false;

void setupSDCard() {
  Serial.print("Initializing SD card... ");
  if (!SD.begin(sdCS)) {
    Serial.println("card failed or not present.");
    sdReady = false;
  } else {
    Serial.println("initialized successfully.");
    sdReady = true;
  }
}

bool isSDReady() {
  return sdReady;
}

void logTemperature(double temperature) {
  if (!isSDReady()) return;

  deselectAllSPI();
  digitalWrite(CS_SD, LOW);  // Select SD

  File file = SD.open("/log.txt", FILE_WRITE);
  if (file) {
    file.seek(file.size());
    file.print(millis());
    file.print(", ");
    file.println(temperature, 2);
    file.close();
  }

  digitalWrite(CS_SD, HIGH);  // Deselect SD
}