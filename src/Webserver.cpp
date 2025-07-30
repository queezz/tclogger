#include <WiFi.h>
#include <WebServer.h>
#include "WebserverMy.h"
#include <SD.h>
#include <vector>
#include <algorithm>
#include <HTTP_Method.h>

#include "Thermocouple.h"
#include "SdCard.h"
#include "Sampling.h"

WebServer server(80);

// MARK: Setup
void setupWebServer()
{
  server.on("/", HTTP_GET, serveMainPage);
  server.on("/preview", HTTP_GET, servePreview);
  server.on("/download", HTTP_GET, serveDownload);
  server.on("/sampling", HTTP_GET, handleSamplingInterval);
  server.on("/newfile", HTTP_POST, handleNewLogFile);

  server.begin();
  Serial.println("Web server started.");
}

void handleClient()
{
  server.handleClient();
}

// MARK: Main Page
void serveMainPage()
{
  float temp = readTemperature();
  String html = "<!DOCTYPE html><html><head><title>ESP32 Temp</title></head><body>";
  html += "<h1>" + String(temp) + " &deg;C</h1>";
  html += "<p>Current Log File: <b>" + String(getLogFilename()) + "</b></p>";

  html += "<form action=\"/sampling\" method=\"get\">";
  html += "<label for=\"interval\">Sampling Interval:</label>";
  html += "<select name=\"interval\">";
  html += "<option value=\"0.1\">0.1 s</option><option value=\"0.5\">0.5 s</option>";
  html += "<option value=\"1\">1 s</option><option value=\"10\">10 s</option><option value=\"30\">30 s</option>";
  html += "</select><input type=\"submit\" value=\"Set\"></form>";

  html += "<form action=\"/newfile\" method=\"post\">";
  html += "<button type=\"submit\">Start New Log File</button></form>";

  html += "<p><a href=\"/preview\">View Log Preview</a></p>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

// MARK: Preview
void servePreview()
{
  String html = "<!DOCTYPE html><html><head><title>Log Preview</title></head><body>";
  html += "<p><a href=\"/\">Main Page</a></p>";
  html += "<h2>Recent Log Files</h2><ul>";

  std::vector<String> filenames;

  File root = SD.open("/");
  if (root && root.isDirectory())
  {
    File entry = root.openNextFile();
    while (entry)
    {
      if (!entry.isDirectory())
      {
        String name = entry.name();
        if (name.endsWith(".csv"))
          filenames.push_back(name);
      }
      entry.close();
      entry = root.openNextFile();
    }
  }

  std::sort(filenames.begin(), filenames.end(), std::greater<String>());
  size_t count = std::min(filenames.size(), size_t(10));

  for (size_t i = 0; i < count; ++i)
  {
    html += "<li><a href=\"/download?file=" + filenames[i] + "\">" + filenames[i] + "</a></li>";
  }

  html += "</ul></body></html>";
  server.send(200, "text/html", html);
}

// MARK: Sampling Interval
void handleSamplingInterval()
{
  if (server.hasArg("interval"))
  {
    float seconds = server.arg("interval").toFloat();
    if (seconds >= 0.1 && seconds <= 30.0)
      setSamplingInterval(seconds);
  }
  serveMainPage(); // Return to main page
}

// MARK: New File
void handleNewLogFile()
{
  startNewLogFile();
  serveMainPage(); // Return to main page
}

// MARK: File Download
void serveDownload()
{
  if (!server.hasArg("file"))
  {
    server.send(400, "text/plain", "Missing filename");
    return;
  }

  String filename = server.arg("file");
  filename.trim();

  // Clean filename
  if (filename.indexOf('/') >= 0 || !filename.endsWith(".csv"))
  {
    server.send(400, "text/plain", "Invalid filename");
    return;
  }

  String path = "/" + filename;
  File file = SD.open(path);
  if (!file || file.isDirectory())
  {
    server.send(404, "text/plain", "File not found");
    return;
  }

  server.sendHeader("Content-Type", "text/csv");
  server.sendHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
  server.sendHeader("Connection", "close");
  server.streamFile(file, "text/csv");
  file.close();
}