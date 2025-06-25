#include "Network.h"
#include <SPI.h>
#include <Ethernet.h>

// MAC and static IP
static byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
static IPAddress staticIP(192, 168, 1, 177);

void setupNetwork() {
  Ethernet.init(4);  // CS pin
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Ethernet DHCP failed, using static IP.");
    Ethernet.begin(mac, staticIP);
  }
  delay(1000);
  Serial.print("IP Address: ");
  Serial.println(Ethernet.localIP());
}

IPAddress getLocalIP() {
  return Ethernet.localIP();
}

bool isNetworkUp() {
  return Ethernet.linkStatus() == LinkON && Ethernet.localIP()[0] != 0;
}
