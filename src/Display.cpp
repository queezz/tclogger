#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include "Display.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setupDisplay() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 init failed");
    while (true);
  }
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(SSD1306_WHITE);
}

void showStatusDisplay(double temp, const String& status) {
  display.clearDisplay();

  display.setTextSize(3);  // Big temperature
  display.setCursor(0, 0);
  if (isnan(temp)) {
    display.setTextSize(1);
    display.println("Temp error!");
  } else {
    display.print(temp, 1);
    display.println(" C");
  }

  display.setTextSize(1);  // Smaller status line
  display.setCursor(0, 50);
  display.println(status);

  display.display();
}

