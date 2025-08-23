#include "Network.h"
#include <WiFi.h>
#include "Secrets.h"

void setupNetwork()
{
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000)
  {
    delay(250);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("WiFi connected. IP Address: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("WiFi connect failed");
  }
}

IPAddress getLocalIP()
{
  return WiFi.localIP();
}

bool isNetworkUp()
{
  return WiFi.status() == WL_CONNECTED;
}
