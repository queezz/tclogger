#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include "Display.h"
#include "TimeSync.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setupDisplay()
{
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
}

void showMessage(const String &message)
{
  display.println(message);
}

void showStatusDisplay(double temp, const String &status, const String &sdstatus)
{
  display.clearDisplay();

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
}
