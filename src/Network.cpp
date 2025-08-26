#include "Network.h"
#include <WiFi.h>
#include "Secrets.h"

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

// helper to start the soft AP (kept in anonymous namespace)
void startAP_impl() {
  WiFi.mode(WIFI_AP);
  // Ensure a known AP IP (default 192.168.4.1) and netmask
  WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
  bool ok = WiFi.softAP(AP_SSID, (strlen(AP_PASS) >= 8) ? AP_PASS : nullptr);
  if (!ok) WiFi.softAP(AP_SSID);
}
} // namespace

namespace Network {

static Mode g_mode = Mode::None;

void startAccessPoint()
{
  // If already in AP mode, nothing to do
  if (g_mode == Mode::AP) return;

  Serial.println("[NET] Switching to AP mode...");
  // Ensure we disconnect from STA and don't persist credentials/mode
  if (WiFi.getMode() != WIFI_OFF) WiFi.disconnect(true, true);
  WiFi.persistent(false);
  startAP_impl();
  g_mode = Mode::AP;
  Serial.printf("[NET] AP %s started: %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
}

void begin(uint32_t connect_timeout_ms) {
  if (WiFi.getMode() != WIFI_OFF) WiFi.disconnect(true, true);
  // Avoid writing WiFi credentials/mode to NVS during mode switches
  WiFi.persistent(false);
  g_mode = Mode::None;

  Serial.println("[NET] Connecting STA...");
  if (trySTA(connect_timeout_ms)) {
    g_mode = Mode::STA;
    Serial.printf("[NET] STA OK: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("[NET] STA failed, starting AP...");
    startAP_impl();
    g_mode = Mode::AP;
    Serial.printf("[NET] AP %s started: %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
  }
}

bool isConnected() {
  if (g_mode == Mode::STA) return WiFi.status() == WL_CONNECTED;
  if (g_mode == Mode::AP)  return WiFi.softAPgetStationNum() > 0;
  return false;
}

Mode mode() { return g_mode; }

IPAddress ip() {
  if (g_mode == Mode::STA) return WiFi.localIP();
  if (g_mode == Mode::AP)  return WiFi.softAPIP();
  return INADDR_NONE;
}

String modeName() {
  switch (g_mode) {
    case Mode::STA: return "STA";
    case Mode::AP:  return "AP";
    default:        return "None";
  }
}

} // namespace Network
