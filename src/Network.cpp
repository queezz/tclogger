#include "Network.h"
#include <WiFi.h>
#include "Secrets.h"

static Network::Mode g_mode = Network::Mode::None;

namespace {
bool trySTA(uint32_t timeout_ms) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t0) < timeout_ms) {
    delay(100);
  }
  return WiFi.status() == WL_CONNECTED;
}

void startAP() {
  WiFi.mode(WIFI_AP);
  // Ensure a known AP IP (default 192.168.4.1) and netmask
  WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
  bool ok = WiFi.softAP(AP_SSID, (strlen(AP_PASS) >= 8) ? AP_PASS : nullptr);
  if (!ok) WiFi.softAP(AP_SSID);
}
} // namespace

void Network::begin(uint32_t connect_timeout_ms) {
  if (WiFi.getMode() != WIFI_OFF) WiFi.disconnect(true, true);
  // Avoid writing WiFi credentials/mode to NVS during mode switches
  WiFi.persistent(false);
  g_mode = Network::Mode::None;

  Serial.println("[NET] Connecting STA...");
  if (trySTA(connect_timeout_ms)) {
    g_mode = Network::Mode::STA;
    Serial.printf("[NET] STA OK: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("[NET] STA failed, starting AP...");
    startAP();
    g_mode = Network::Mode::AP;
    Serial.printf("[NET] AP %s started: %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
  }
}

bool Network::isConnected() {
  if (g_mode == Mode::STA) return WiFi.status() == WL_CONNECTED;
  if (g_mode == Mode::AP)  return WiFi.softAPgetStationNum() > 0;
  return false;
}

Network::Mode Network::mode() { return g_mode; }

IPAddress Network::ip() {
  if (g_mode == Mode::STA) return WiFi.localIP();
  if (g_mode == Mode::AP)  return WiFi.softAPIP();
  return INADDR_NONE;
}

String Network::modeName() {
  switch (g_mode) {
    case Mode::STA: return "STA";
    case Mode::AP:  return "AP";
    default:        return "None";
  }
}
