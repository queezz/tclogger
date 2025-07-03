#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <time.h> 
#include "Pins.h"
#include "SpiDevices.h"
#include "TimeSync.h"

static const int sdCS = CS_SD;
static bool sdReady = false;
static char logFilename[32] = "/log.txt";

void setupSDCard() {
  Serial.print("Initializing SD card... ");
  if (!SD.begin(sdCS)) {
    Serial.println("card failed or not present.");
    sdReady = false;
    return;
  }
  Serial.println("initialized successfully.");
  sdReady = true;

  time_t now = time(nullptr);
  struct tm* t = localtime(&now);

  if (now < 1609459200) {  // Jan 1, 2021 — arbitrarily "sane" threshold
    // Time not set — fallback name
    strncpy(logFilename, "/log_fallback.csv", sizeof(logFilename));
  } else {
    strftime(logFilename, sizeof(logFilename), "/log_%Y%m%d_%H%M.csv", t);
  }

  File f = SD.open(logFilename, FILE_WRITE);
  if (f) {
    f.println("Time,Temperature(C)");
    f.close();
  }
}

const char* getLogFilename() {
  return logFilename;
}


bool isSDReady() {
  return sdReady;
}

struct LogEntry {
  char timestamp[32];
  double temperature;
};

const size_t LOG_BUFFER_SIZE = 32;
LogEntry logBuffer[LOG_BUFFER_SIZE];
size_t logIndex = 0;

void logTemperature(double temperature) {
  if (logIndex < LOG_BUFFER_SIZE) {
    getTimestamp(logBuffer[logIndex].timestamp, sizeof(logBuffer[logIndex].timestamp));
    logBuffer[logIndex].temperature = temperature;
    logIndex++;
  }
}


void flushLogBufferToSD() {
  if (!isSDReady() || logIndex == 0) return;

  deselectAllSPI();
  digitalWrite(CS_SD, LOW);

  File file = SD.open(logFilename, FILE_WRITE);
  if (file) {
    file.seek(file.size());

    for (size_t i = 0; i < logIndex; ++i) {
      file.print(logBuffer[i].timestamp);
      file.print(", ");
      file.println(logBuffer[i].temperature, 2);
    }

    file.close();
  }

  digitalWrite(CS_SD, HIGH);

  logIndex = 0;  // Clear buffer
}
