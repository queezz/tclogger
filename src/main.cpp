#include <Arduino.h>
#include "Thermocouple.h"
#include "Display.h"
#include "Network.h"
#include "SdCard.h"
#include "SpiDevices.h"
#include "TimeSync.h"
#include "WebServer.h"
#include "Sampling.h"

unsigned long lastSample = 0;
String currentLogFile = ""; // for displaying

unsigned long lastFlush = 0;
const unsigned long flushInterval = 10000; // 10s

// MARK: Setup
void setup()
{
  Serial.begin(115200);
  syncTimeFromRTC();

  setupSPIChipSelects();
  setupDisplay();
  setupNetwork();
  setupThermocouple();
  setupSDCard();
  setupWebServer();
}

// MARK: Loop
void loop()
{
  unsigned long now = millis();
  double temperature = readTemperature();

  if (now - lastSample >= getSamplingInterval())
  {
    logTemperature(temperature);
    lastSample = now;
  }

  if (millis() - lastFlush >= flushInterval)
  {
    flushLogBufferToSD();
    lastFlush = now;
  }

  String sdstatus;

  if (!isSDReady())
  {
    sdstatus = "SD not ready";
  }
  else
  {
    sdstatus = "SD OK";
  }

  String status;

  if (!isNetworkUp())
  {
    status = "LAN error / no IP";
  }
  else
  {
    IPAddress ip = getLocalIP();
    status = String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
  }

  showStatusDisplay(temperature, status, sdstatus);
  handleClient();
  delay(std::min(500ul, getSamplingInterval()));
}
