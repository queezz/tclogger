#pragma once
#include <Arduino.h>
#include <WiFi.h>

namespace Network {
enum class Mode { None, STA, AP };

void begin(uint32_t connect_timeout_ms = 12000);
bool isConnected();
Mode mode();
IPAddress ip();
String modeName();
}
