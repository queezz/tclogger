#include <Arduino.h>
#include <EthernetUdp.h>
#include <SPI.h>
#include <Ethernet.h>
#include "TimeSync.h"

void sendNTPPacket(IPAddress& address);

EthernetUDP udp;
const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];

IPAddress ntpServer(129, 6, 15, 28); // time.nist.gov
unsigned int localPort = 8888;

void setupTime() {
  udp.begin(localPort);
  Serial.print("Sending NTP request... ");
  sendNTPPacket(ntpServer);

  int attempts = 0;
  while (!udp.parsePacket() && attempts < 20) {
    delay(500);
    attempts++;
    Serial.print(".");
  }

  if (udp.parsePacket()) {
    udp.read(packetBuffer, NTP_PACKET_SIZE);

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = (highWord << 16) | lowWord;

    const unsigned long seventyYears = 2208988800UL;
    time_t epoch = secsSince1900 - seventyYears;

    struct timeval now = { .tv_sec = epoch };
    settimeofday(&now, nullptr);

    Serial.println("\nNTP time set.");
  } else {
    Serial.println("\nNTP request failed.");
  }
}

void sendNTPPacket(IPAddress& address) {
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  udp.beginPacket(address, 123);
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

String getTimestamp() {
  time_t now = time(nullptr);
  struct tm* t = localtime(&now);
  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", t);
  return String(buf);
}
