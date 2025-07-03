#include <Arduino.h>
#include "Thermocouple.h"
#include "Display.h"
#include "Network.h"
#include "SdCard.h"
#include "SpiDevices.h"
#include "TimeSync.h"
#include "WebServer.h"


unsigned long lastSample = 0;
const unsigned long sampleInterval = 500;  // 0.5s

unsigned long lastFlush = 0;
const unsigned long flushInterval = 5000;  // 5s


void setup() {
  Serial.begin(115200);
  syncTimeFromRTC();

  setupSPIChipSelects();
  setupDisplay();
  showMessage(String("Setting LAN"));
  setupNetwork();
  showMessage(String("Setting TC"));
  setupThermocouple();
  // setupTime();
  showMessage(String("Setting SD card"));
  setupSDCard();
  setupWebServer();
}

void loop() {
  unsigned long now = millis();
  double temperature = readTemperature();
  logTemperature(temperature);
  if (millis() - lastFlush >= flushInterval) {
    flushLogBufferToSD();
    lastFlush = millis();
  }

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
  handleClient();
  delay(500);
}

