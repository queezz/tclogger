#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>
#include <WiFi.h> // for ESP32

void setupNetwork();
IPAddress getLocalIP();
bool isNetworkUp();

#endif
