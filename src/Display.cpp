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

void showTemperature(double c) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(c, 1);
  display.println(" C");
  display.display();
}

void showError() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Thermocouple error!");
  display.display();
}
