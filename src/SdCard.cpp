#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <time.h>
#include "Pins.h"
#include "SpiDevices.h"
#include "TimeSync.h"
#include "SdCard.h"

static const int sdCS = CS_SD;
static bool sdReady = false;
static char logFilename[32] = "/log.txt";

unsigned long startMillis = 0;
char startTimeString[32] = {0}; // e.g., "2025-07-04 17:26:39"

// MARK: setup
void setupSDCard()
{
  Serial.print("Initializing SD card... ");
  if (!SD.begin(sdCS))
  {
    Serial.println("card failed or not present.");
    sdReady = false;
    return;
  }
  Serial.println("initialized successfully.");
  sdReady = true;
  startNewLogFile();
}

const char *getLogFilename()
{
  return logFilename;
}

bool isSDReady()
{
  return sdReady;
}

// MARK: Log
struct LogEntry
{
  char timestamp[32];
  double temperature;
};

const size_t LOG_BUFFER_SIZE = 32;
LogEntry logBuffer[LOG_BUFFER_SIZE];
size_t logIndex = 0;

void logTemperature(double temperature)
{
  if (logIndex < LOG_BUFFER_SIZE)
  {
    getTimestamp(logBuffer[logIndex].timestamp, sizeof(logBuffer[logIndex].timestamp));
    logBuffer[logIndex].temperature = temperature;
    logIndex++;
  }
}
// MARK: new file
void startNewLogFile()
{
  time_t now = time(nullptr);
  struct tm *t = localtime(&now);
  strftime(startTimeString, sizeof(startTimeString), "%Y-%m-%d %H:%M:%S", t);
  startMillis = millis();

  char baseName[32];
  if (now < 1609459200)
  {
    strncpy(baseName, "/log_manual", sizeof(baseName));
  }
  else
  {
    strftime(baseName, sizeof(baseName), "/log_%Y%m%d_%H%M", t);
  }

  int counter = 0;
  while (true)
  {
    if (counter == 0)
    {
      snprintf(logFilename, sizeof(logFilename), "%s.csv", baseName);
    }
    else
    {
      snprintf(logFilename, sizeof(logFilename), "%s_%d.csv", baseName, counter);
    }

    if (!SD.exists(logFilename))
      break;
    counter++;
  }

  File f = SD.open(logFilename, FILE_WRITE);
  if (f)
  {
    f.print("# Start time: ");
    f.println(startTimeString);
    f.println("# Format: ms_since_start, temperature (C)");
    f.println("ms,temp");
    f.close();

    Serial.println("New file: " + String(logFilename));
  }

  logIndex = 0; // clear buffer
}

bool isLogging()
{
  // Consider logging active if filename is not the default placeholder
  return strlen(logFilename) > 1 && strcmp(logFilename, "/log.txt") != 0;
}

// MARK: flush
void flushLogBufferToSD()
{
  if (!isSDReady() || logIndex == 0)
    return;

  deselectAllSPI();

  File file = SD.open(logFilename, FILE_WRITE);
  if (file)
  {
    file.seek(file.size());

    for (size_t i = 0; i < logIndex; ++i)
    {
      file.print(logBuffer[i].timestamp);
      file.print(", ");
      file.println(logBuffer[i].temperature, 2);
    }

    file.close();
  }

  digitalWrite(CS_SD, HIGH);

  logIndex = 0; // Clear buffer
}
