#include "Network.h"
#include <SPI.h>
#include <Ethernet.h>
#include "Pins.h"
#include "SpiDevices.h"

// MAC and static IP
// static byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
static byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x01 };

static IPAddress staticIP(192, 168, 1, 177);

void setupNetwork() {
  Ethernet.init(CS_W5500);  // W5500 CS pin setup
  pinMode(CS_W5500, OUTPUT);
  digitalWrite(CS_W5500, HIGH);  // deselect before init

  Serial.println("Getting IP from the router");

  if (Ethernet.begin(mac, 10000) == 0) {  // ✅ Try DHCP with 10s timeout
    Serial.println("Ethernet DHCP failed, using static IP.");
    Ethernet.begin(mac, staticIP);       // fallback to static
  }

  delay(500);
  Serial.print("IP Address: ");
  Serial.println(Ethernet.localIP());
}

IPAddress getLocalIP() {
  return Ethernet.localIP();
}

bool isNetworkUp() {
  return Ethernet.linkStatus() == LinkON && Ethernet.localIP()[0] != 0;
}
