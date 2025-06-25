#include <Arduino.h>
#include "Thermocouple.h"
#include "Display.h"
#include "Network.h"

void setup() {
  Serial.begin(115200);
  setupDisplay();
  setupThermocouple();
  setupNetwork();
}

void loop() {
  double c = readTemperature();
  Serial.print("Temp (raw): ");
  Serial.println(c);

  String status;

  if (!isNetworkUp()) {
    status = "LAN error / no IP";
  } else {
    IPAddress ip = getLocalIP();
    status = String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
  }

  // status = String("place holder");
  showStatusDisplay(c, status);
  delay(500);
}

