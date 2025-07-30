#include "Network.h"
#include "secrets.h"

void setupNetwork()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 30)
  {
    delay(500);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nWiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("\nFailed to connect to WiFi");
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
