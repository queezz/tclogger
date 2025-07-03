#include <Arduino.h>
// #include <EthernetENC.h> 
#include <Ethernet.h>
#include <SD.h>
#include <vector>
#include <algorithm>
#include "WebServer.h"
#include "Thermocouple.h"
#include "SdCard.h"
#include "SpiDevices.h"

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
// MARK: Client
void handleClient() {
  EthernetClient client = server.available();
  if (!client) return;

  while (client.connected()) {
    if (!client.available()) continue;

    String req = client.readStringUntil('\r');
    client.read(); // consume \n

    if (req.indexOf("GET /preview") >= 0) {
      servePreview(client);
    } else {
      serveMainPage(client);
    }

    break;
  }

  delay(1);
  client.stop();
}

// MARK: Main Page
void serveMainPage(EthernetClient& client) {
  float temp = readTemperature();
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE html><html><head><title>ESP32 Temp</title></head><body>");
  client.print("<h1>Current Temperature: ");
  client.print(temp);
  client.println(" &deg;C</h1>");
  client.println("<p><a href=\"/preview\">View Log Preview</a></p>");
  client.println("</body></html>");
}

// MARK: Preview
void servePreview(EthernetClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE html><html><head><title>Log Preview</title></head><body>");
  client.println("<h2>Recent Log Files</h2>");
  client.println("<ul>");
  listRecentLogs(client);
  client.println("</ul>");
  client.println("</body></html>");
}


// MARK: list Logs
void listRecentLogs(EthernetClient& client) {
  deselectAllSPI();             
  digitalWrite(CS_SD, LOW);     

  File root = SD.open("/");
  if (!root || !root.isDirectory()) {
    client.println("<li>Failed to open SD root.</li>");
    return;
  }

  std::vector<String> filenames;
  File entry = root.openNextFile();
  while (entry) {
    if (!entry.isDirectory()) {
      String name = entry.name();
      if (name.endsWith(".csv")) {
        filenames.push_back(name);
      }
    }
    entry.close();
    entry = root.openNextFile();
  }
  digitalWrite(CS_SD, HIGH);

  std::sort(filenames.begin(), filenames.end());

  size_t count = filenames.size();
  size_t start = count > 10 ? count - 10 : 0;

  for (size_t i = start; i < count; ++i) {
    client.print("<li>");
    client.print(filenames[i]);
    client.println("</li>");
  }
}
