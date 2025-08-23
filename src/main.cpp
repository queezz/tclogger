#include <Arduino.h>
#include "Thermocouple.h"
#include "Display.h"
#include "Network.h"
#include "SdCard.h"
#include "SpiDevices.h"
#include "TimeSync.h"
#include "WebServer.h"
#include "Sampling.h"
#include "rtc.h"

unsigned long lastSample = 0;
String currentLogFile = ""; // for displaying

unsigned long lastFlush = 0;
const unsigned long flushInterval = 10000; // 10s

// I2C mutex
SemaphoreHandle_t I2C_MTX = NULL;

// MARK: Setup
void setup()
{
  Serial.begin(115200);
  rtc_setup();

  setupSPIChipSelects();
  setupDisplay();
  Network::begin(12000);
  setupThermocouple();
  setupSDCard();
  Webserver_begin();
  // Initialize I2C mutex and Wire
  I2C_MTX = xSemaphoreCreateMutex();
  Wire.begin();
  Wire.setTimeOut(50);
}

// MARK: Loop
void loop()
{
  unsigned long now = millis();
  double temperature = readTemperature();
  // accumulate for rolling average regardless of display/log cadence
  accumulateRolling(temperature);

  if (now - lastSample >= getSamplingInterval())
  {
    // use averaged value over the sampling interval
    double avg = finalizeIntervalAverage();
    double toLog = isnan(avg) ? temperature : avg;
    logTemperature(toLog);
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

  if (!Network::isConnected())
  {
    status = "WiFi error / no IP";
  }
  else
  {
    IPAddress ip = Network::ip();
    status = String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
  }

  // For OLED, show smoothed value if available
  double displayVal = getCurrentAverage();
  if (isnan(displayVal)) displayVal = temperature;
  showStatusDisplay(displayVal, status, sdstatus);
  handleClient();
  delay(std::min(500ul, getSamplingInterval()));

}
