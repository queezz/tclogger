#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <time.h> 
#include "Pins.h"
#include "SpiDevices.h"
#include "TimeSync.h"

time_t now = time(nullptr);
struct tm* t = localtime(&now);


static const int sdCS = CS_SD;
static bool sdReady = false;
static char logFilename[32] = "/log.txt";

void setupSDCard() {
  Serial.print("Initializing SD card... ");
  if (!SD.begin(sdCS)) {
    Serial.println("card failed or not present.");
    sdReady = false;
  } else {
    Serial.println("initialized successfully.");
    sdReady = true;
  }

  char filename[32];
  strftime(filename, sizeof(filename), "/log_%Y%m%d_%H%M.csv", t);

  File f = SD.open(filename, FILE_WRITE);
  if (f) {
    f.println("Time,Temperature(C)");
    f.close();
}

}

bool isSDReady() {
  return sdReady;
}

void logTemperature(double temperature) {
  if (!isSDReady()) return;

  deselectAllSPI();
  digitalWrite(CS_SD, LOW);  // Select SD

  // File file = SD.open("/log.txt", FILE_WRITE);
  File file = SD.open(logFilename, FILE_WRITE);
  if (file) {
    file.seek(file.size());
    // file.print(millis());
    file.print(getTimestamp());
    file.print(", ");
    file.println(temperature, 2);
    file.close();
  }

  digitalWrite(CS_SD, HIGH);  // Deselect SD
}