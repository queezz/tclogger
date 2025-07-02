#include <Arduino.h>
// #include <EthernetENC.h> 
#include <Ethernet.h>
#include "WebServer.h"
#include "Thermocouple.h"

// === Workaround: patch missing begin(uint16_t) ===
class FixedEthernetServer : public EthernetServer {
public:
  using EthernetServer::EthernetServer;  // inherit constructors
  using EthernetServer::begin;           // expose both begin() and begin(uint16_t)

  void begin(uint16_t port) override {
    EthernetServer::begin();  // call the no-argument version
  }
};

FixedEthernetServer server(80);  // <- use the fixed version

void setupWebServer() {
  server.begin();
}

void handleClient() {
  EthernetClient client = server.available();
  if (client) {
    while (client.connected()) {
      if (client.available()) {
        String req = client.readStringUntil('\r');
        client.read(); // Skip '\n'

        // Simple HTML response
        float temp = readTemperature();
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println("Connection: close");
        client.println();
        client.println("<!DOCTYPE html><html><head><title>ESP32 Temp</title></head><body>");
        client.print("<h1>Current Temperature: ");
        client.print(temp);
        client.println(" &deg;C</h1>");
        client.println("</body></html>");
        break;
      }
    }
    delay(1);
    client.stop();
  }
}
