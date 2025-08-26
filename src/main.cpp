#include <Arduino.h>
#include "Thermocouple.h"
#include "Display.h"
#include "Network.h"
#include "SdCard.h"
#include "SpiDevices.h"
#include "TimeSync.h"
#include "Webserver.h"
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
  IPAddress ip = Network::ip();
  if (Network::mode() == Network::Mode::AP)
  {
    status = String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
  }
  else if (Network::isConnected())
  {
    status = String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
  }
  else
  {
    status = "WiFi error / no IP";
  }

  // For OLED, show smoothed value if available
  double displayVal = getCurrentAverage();
  if (isnan(displayVal)) displayVal = temperature;
  showStatusDisplay(displayVal, status, sdstatus);
  handleClient();
  // If we're supposed to be in STA mode but lost connection, start AP
  if (Network::mode() == Network::Mode::STA && WiFi.status() != WL_CONNECTED)
  {
    // Give a small grace period before switching: try reconnecting briefly
    static unsigned long lastNetCheck = 0;
    unsigned long nowMs = millis();
    if (nowMs - lastNetCheck > 5000)
    {
      lastNetCheck = nowMs;
      Serial.println("[MAIN] STA lost connection — starting AP...");
      Network::startAccessPoint();
      // Reinitialize webserver so it binds to AP interface
      Webserver_begin();
    }
  }
  delay(std::min(500ul, getSamplingInterval()));

}
