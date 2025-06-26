#include <Arduino.h>
#include "Thermocouple.h"
#include "Display.h"
#include "Network.h"
#include "SdCard.h"
#include "SpiDevices.h"

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
  setupSPIChipSelects();
  setupDisplay();
  // deselectAllSPI();
  setupSDCard();
  deselectAllSPI();
  setupThermocouple();
  deselectAllSPI();
  setupNetwork();
}

void loop() {
  unsigned long now = millis();
  double temperature = readTemperature();
  Serial.println(temperature);
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
  delay(500);
}

