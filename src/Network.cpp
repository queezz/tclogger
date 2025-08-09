#include "Network.h"
#include "Thermocouple.h"

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

static Preferences prefs;
static WebServer server(80);

static String ssid;
static String pass;

static void handleRoot() {
  double temp = readTemperature();
  String page = "<html><body><h1>WiFi Config</h1>";
  page += "<form method='POST' action='/save'>";
  page += "SSID: <input name='ssid' value='" + ssid + "'><br>";
  page += "Password: <input name='pass' type='password'><br>";
  page += "<input type='submit' value='Save'></form>";
  page += "<p>Temperature: " + String(temp) + " C</p>";
  page += "</body></html>";
  server.send(200, "text/html", page);
}

static void handleSave() {
  if (server.hasArg("ssid") && server.hasArg("pass")) {
    ssid = server.arg("ssid");
    pass = server.arg("pass");
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    server.send(200, "text/html", "Saved. Rebooting...");
    delay(500);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "Missing params");
  }
}

static void startAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP32-Setup");
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
  Serial.println("Started AP for configuration");
}

static void startServer() {
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
}

void setupNetwork() {
  prefs.begin("wifi", false);
  ssid = prefs.getString("ssid", "");
  pass = prefs.getString("pass", "");

  if (ssid.length() > 0) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
      delay(500);
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("WiFi connected: ");
      Serial.println(WiFi.localIP());
      startServer();
      return;
    }
    Serial.println("WiFi connection failed");
  }

  startAP();
}

IPAddress getLocalIP() {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP();
  }
  return WiFi.softAPIP();
}

bool isNetworkUp() {
  return WiFi.status() == WL_CONNECTED;
}

void handleNetwork() {
  server.handleClient();
}

