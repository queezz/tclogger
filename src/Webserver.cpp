#include <Arduino.h>
#include <Ethernet.h>
#include <SD.h>
#include <vector>
#include <algorithm>
#include "WebServer.h"
#include "Thermocouple.h"
#include "SdCard.h"
#include "SpiDevices.h"
#include "Sampling.h"

// === Workaround: patch missing begin(uint16_t) ===
class FixedEthernetServer : public EthernetServer
{
public:
  using EthernetServer::begin;          // expose both begin() and begin(uint16_t)
  using EthernetServer::EthernetServer; // inherit constructors

  void begin(uint16_t port) override
  {
    EthernetServer::begin(); // call the no-argument version
  }
};

FixedEthernetServer server(80); // <- use the fixed version

void setupWebServer()
{
  server.begin();
}
// MARK: Client
void handleClient()
{
  deselectAllSPI();
  EthernetClient client = server.available();
  if (!client)
    return;

  while (!client.available())
    delay(1); // wait for data

  String req = client.readStringUntil('\r');
  client.read(); // consume \n

  if (req.indexOf("GET /preview") >= 0)
  {
    servePreview(client);
    delay(1);
    client.stop();
    digitalWrite(CS_W5500, HIGH);
    return;
  }

  if (req.indexOf("GET /download?file=") >= 0)
  {
    int idx = req.indexOf("file=") + 5;
    String filename = req.substring(idx);
    filename.trim();

    // Truncate at ".csv" to remove trailing HTTP
    int endIdx = filename.indexOf(".csv");
    if (endIdx != -1)
    {
      filename = filename.substring(0, endIdx + 4);
    }

    // Only allow safe filenames
    if (filename.indexOf('/') >= 0 || !filename.endsWith(".csv"))
    {
      client.println("HTTP/1.1 400 Bad Request\r\n\r\nInvalid filename.");
    }
    else
    {
      serveFileDownload(client, filename);
    }
    delay(1);
    client.stop();
    return;
  }

  if (req.indexOf("GET /sampling?interval=") >= 0)
  {
    int idx = req.indexOf("interval=") + 9;
    String val = req.substring(idx);
    val.trim();
    float seconds = val.toFloat();
    if (seconds >= 0.1 && seconds <= 30.0)
    {
      setSamplingInterval(seconds);
    }
    serveMainPage(client);
    client.stop();
    digitalWrite(CS_W5500, HIGH);
    return;
  }

  if (req.startsWith("POST /newfile"))
  {
    startNewLogFile();
    serveMainPage(client);
    client.stop();
    digitalWrite(CS_W5500, HIGH);
    return;
  }

  // Default: main page
  serveMainPage(client);
  delay(1);
  client.stop();
  digitalWrite(CS_W5500, HIGH);
}

// MARK: Main Page
void serveMainPage(EthernetClient &client)
{
  float temp = readTemperature();
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();

  client.println("<!DOCTYPE html><html><head><title>ESP32 Temp</title></head><body>");
  client.print("<h1>");
  client.print(temp);
  client.println(" &deg;C</h1>");
  client.print("<p>Current Log File: <b>");
  client.print(getLogFilename());
  client.println("</b></p>");

  client.println("<form action=\"/sampling\" method=\"get\">");
  client.println("<label for=\"interval\">Sampling Interval:</label>");
  client.println("<select name=\"interval\">");
  client.println("<option value=\"0.1\">0.1 s</option><option value=\"0.5\">0.5 s</option><option value=\"1\">1 s</option><option value=\"10\">10 s</option><option value=\"30\">30 s</option>");
  client.println("</select>");
  client.println("<input type=\"submit\" value=\"Set\">");
  client.println("</form>");

  client.println("<form action=\"/newfile\" method=\"post\">");
  client.println("<button type=\"submit\">Start New Log File</button>");
  client.println("</form>");

  client.println("<p><a href=\"/preview\">View Log Preview</a></p>");
  client.println("</body></html>");
}

// MARK: Preview
void servePreview(EthernetClient &client)
{
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE html><html><head><title>Log Preview</title></head><body>");
  client.println("<p><a href=\"/\">Main Page</a></p>");
  client.println("<h2>Recent Log Files</h2>");
  client.println("<ul>");
  listRecentLogs(client);
  client.println("</ul>");
  client.println("</body></html>");
}

// MARK: list Logs
void listRecentLogs(EthernetClient &client)
{
  deselectAllSPI();
  digitalWrite(CS_SD, LOW);

  File root = SD.open("/");
  if (!root || !root.isDirectory())
  {
    client.println("<li>Failed to open SD root.</li>");
    return;
  }

  std::vector<String> filenames;
  File entry = root.openNextFile();
  while (entry)
  {
    if (!entry.isDirectory())
    {
      String name = entry.name();
      if (name.endsWith(".csv"))
      {
        filenames.push_back(name);
      }
    }
    entry.close();
    entry = root.openNextFile();
  }
  digitalWrite(CS_SD, HIGH);

  std::sort(filenames.begin(), filenames.end(), std::greater<String>());

  // size_t count = filenames.size();
  size_t count = std::min(filenames.size(), size_t(10));
  for (size_t i = 0; i < count; ++i)
  {
    client.print("<li><a href=\"/download?file=");
    client.print(filenames[i]);
    client.print("\">");
    client.print(filenames[i]);
    client.println("</a></li>");
  }
}

// MARK: Download

void serveFileDownload(EthernetClient &client, const String &filename)
{
  deselectAllSPI();         // Ensure SD is only selected
  digitalWrite(CS_SD, LOW); // Select SD

  File file = SD.open("/" + filename);
  if (!file || file.isDirectory())
  {
    client.println("HTTP/1.1 404 Not Found\r\n\r\nFile not found.");
    digitalWrite(CS_SD, HIGH);
    return;
  }

  // === Only after successful file open: Send HTTP headers ===
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/csv");
  client.print("Content-Disposition: attachment; filename=\"");
  client.print(filename);
  client.println("\"");
  client.println("Connection: close");
  client.println(); // blank line after headers!

  // === Stream file content ===
  const size_t bufSize = 64;
  uint8_t buffer[bufSize];
  while (file.available())
  {
    size_t len = file.read(buffer, bufSize);
    client.write(buffer, len);
  }
  file.close();

  digitalWrite(CS_SD, HIGH);
}
