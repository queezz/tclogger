#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include "Display.h"
#include "TimeSync.h"
#include <Wire.h>
#include "rtc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

extern SemaphoreHandle_t I2C_MTX;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setupDisplay()
{
  // Acquire I2C before initializing display
  if (I2C_MTX) xSemaphoreTake(I2C_MTX, pdMS_TO_TICKS(200));
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println("SSD1306 init failed");
    while (true)
      ;
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.println("Initializing...");
  display.display();
  if (I2C_MTX) xSemaphoreGive(I2C_MTX);
}

void showMessage(const String &message)
{
  display.println(message);
}

void showStatusDisplay(double temp, const String &status, const String &sdstatus)
{
  if (I2C_MTX == nullptr) {
    display.clearDisplay();
  } else {
    if (xSemaphoreTake(I2C_MTX, pdMS_TO_TICKS(100)) != pdTRUE) return;
    display.clearDisplay();
  }

  display.setTextSize(3); // Big temperature
  display.setCursor(0, 0);
  if (isnan(temp))
  {
    display.setTextSize(1);
    display.println("Temp error!");
  }
  else
  {
    display.print(temp, 1);
    display.println(" C");
  }

  display.setTextSize(1);
  display.setCursor(0, 25);
  display.println(sdstatus);

  char timestamp[20];
  getTimestamp(timestamp, sizeof(timestamp));
  display.setCursor(0, 40);
  display.println(timestamp + 11);

  display.setCursor(0, 55);
  display.println(status);

  display.display();
  if (I2C_MTX) xSemaphoreGive(I2C_MTX);
}
