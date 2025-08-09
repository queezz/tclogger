#include <Arduino.h>
#include "Thermocouple.h"
#include "Display.h"
#include "Network.h"
#include "SdCard.h"
#include "SpiDevices.h"
#include "TimeSync.h"
struct LogEntry {
  unsigned long timestamp;
  double temperature;
};

const size_t LOG_BUFFER_SIZE = 32;
LogEntry logBuffer[LOG_BUFFER_SIZE];
size_t logIndex = 0;

unsigned long lastSample = 0;
const unsigned long sampleInterval = 500;  // 0.5s

unsigned long lastFlush = 0;
const unsigned long flushInterval = 5000;  // 5s


void setup() {
  Serial.begin(115200);
  // Set to 2025-06-26 19:00:00 JST (UTC+9)
  struct tm t;
  t.tm_year = 2025 - 1900;
  t.tm_mon  = 6 - 1;     // June
  t.tm_mday = 26;
  t.tm_hour = 19;
  t.tm_min  = 0;
  t.tm_sec  = 0;
  time_t now = mktime(&t);
  struct timeval tv = { .tv_sec = now };
  settimeofday(&tv, nullptr);

  Serial.println("Manual time set");

  setupSPIChipSelects();
  setupDisplay();
  setupNetwork();
  setupThermocouple();
  // setupTime();
  setupSDCard();
}

void loop() {
  unsigned long now = millis();
  double temperature = readTemperature();
  // Serial.println(temperature);
  logTemperature(temperature);

  String sdstatus;

  if (!isSDReady()){
    sdstatus = "SD not ready";
  } else {
    sdstatus = "SD OK";
  }

  String status;

  if (!isNetworkUp()) {
    status = "LAN error / no IP";
  } else {
    IPAddress ip = getLocalIP();
    status = String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
  }

  showStatusDisplay(temperature, status, sdstatus);
  handleNetwork();
  delay(500);
}

