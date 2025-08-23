#include <Arduino.h>
#include <WiFi.h>
#include <SD.h>
#include <vector>
#include <algorithm>
#include <FS.h>
#include <LittleFS.h>
#include "Webserver.h"
#include "Network.h"
#include "Thermocouple.h"
#include "SdCard.h"
#include "SpiDevices.h"
#include "Sampling.h"

WiFiServer server(80);

void setupWebServer()
{
  // Initialize LittleFS (for serving data/index.html)
  if (!LittleFS.begin(true))
  {
    Serial.println("LittleFS mount failed");
  }

  server.begin();
}
// MARK: Client
void handleClient()
{
  deselectAllSPI();
  WiFiClient client = server.available();
  if (!client)
    return;

  while (!client.available())
    delay(1); // wait for data

  client.setNoDelay(true);

  String req = client.readStringUntil('\r');
  client.read(); // consume \n
  // --- API: /api/status ---
  if (req.indexOf("GET /api/status") >= 0)
  {
    float temp = readTemperature();
    String j = "{";
    j += "\"temp\":" + String(temp, 2) + ",";
    j += "\"log\":\"" + String(getLogFilename()) + "\",";
    j += "\"interval_ms\":" + String(getSamplingInterval());
    j += "}";
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    client.println(j);
    client.stop();
    return;
  }

  // --- API: /api/temp ---
  if (req.indexOf("GET /api/temp") >= 0)
  {
    double tavg = getCurrentAverage();
    float temp = isnan(tavg) ? readTemperature() : (float)tavg;
    unsigned long ts = millis();
    String j = "{";
    j += "\"t\":" + String(temp, 2) + ",";
    j += "\"ts\":" + String(ts);
    j += "}";
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    client.println(j);
    client.stop();
    return;
  }

  // --- API: /api/health ---
  if (req.indexOf("GET /api/health") >= 0)
  {
    long rssi = WiFi.RSSI();
    IPAddress ip = Network::ip();
    String j = "{";
    j += "\"rssi\":" + String(rssi) + ",";
    j += "\"ip\":\"" + String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]) + "\",";
    j += "\"uptime_ms\":" + String(millis());
    j += "}";
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    client.println(j);
    client.stop();
    return;
  }

  // --- API: /api/set_interval?ms=NNN  (POST expected, but support GET-like query too) ---
  if (req.indexOf("/api/set_interval") >= 0)
  {
    // Look for ms= in the request line (handles query param)
    int idx = req.indexOf("ms=");
    if (idx >= 0)
    {
      idx += 3;
      String val = req.substring(idx);
      // strip trailing HTTP (space)
      int sp = val.indexOf(' ');
      if (sp >= 0) val = val.substring(0, sp);
      val.trim();
      unsigned long ms = val.toInt();
      if (ms > 0)
      {
        // convert ms to seconds for existing setter
        setSamplingInterval((float)ms / 1000.0f);
      }
    }
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println("OK");
    client.stop();
    return;
  }

  // --- API: /api/start_log ---
  if (req.indexOf("/api/start_log") >= 0)
  {
    // check for ?new=1 in the request line
    bool wantNew = false;
    int idxNew = req.indexOf("new=");
    if (idxNew >= 0)
    {
      String v = req.substring(idxNew + 4);
      int sp = v.indexOf(' ');
      if (sp >= 0) v = v.substring(0, sp);
      v.trim();
      if (v == "1" || v == "true") wantNew = true;
    }

    // If wantNew, rotate unconditionally
    if (wantNew)
    {
      startNewLogFile();
      String j = "{\"ok\":true,\"running\":true,\"log\":\"" + String(getLogFilename()) + "\"}";
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      client.println(j);
      client.stop();
      return;
    }

    // Not requesting new file: if already logging, return current path; otherwise start new
    if (isLogging())
    {
      String j = "{\"ok\":true,\"running\":true,\"log\":\"" + String(getLogFilename()) + "\"}";
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      client.println(j);
      client.stop();
      return;
    }

    // not logging: start new
    startNewLogFile();
    {
      String j = "{\"ok\":true,\"running\":true,\"log\":\"" + String(getLogFilename()) + "\"}";
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      client.println(j);
      client.stop();
      return;
    }
  }

  // --- Preview ---
  if (req.indexOf("GET /preview") >= 0)
  {
    servePreview(client);
    delay(1);
    client.stop();
    return;
  }

  // --- Download old CSV files: /download?file=NAME.csv ---
  if (req.indexOf("GET /download?file=") >= 0)
  {
    int idx = req.indexOf("file=") + 5;
    String filename = req.substring(idx);
    filename.trim();
    int endIdx = filename.indexOf('.');
    if (endIdx != -1)
    {
      // truncate anything after the name (removes HTTP tail)
      filename = filename.substring(0, endIdx) + filename.substring(endIdx, endIdx + 4);
    }
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

  // --- Serve static files from LittleFS (fall back to main page) ---
  // GET / or GET /index.html
  if (req.indexOf("GET /") >= 0)
  {
    // quick handle favicon requests to avoid LittleFS create attempts
    if (req.indexOf("GET /favicon.ico") >= 0 || req.indexOf("GET /favicon.png") >= 0)
    {
      client.println("HTTP/1.1 204 No Content");
      client.println("Connection: close");
      client.println();
      client.stop();
      return;
    }
    // extract path (very small parser)
    int s = req.indexOf(' ');
    int e = req.indexOf(' ', s + 1);
    String path = "/";
    if (s >= 0 && e > s)
    {
      path = req.substring(s + 1, e);
    }
    if (path == "/") path = "/index.html";
    // serve from LittleFS if exists (support precompressed .gz and Cache-Control)
    String filePath = path;
    bool useGz = false;
    // prefer precompressed asset if present
    if (LittleFS.exists(path + ".gz"))
    {
      filePath = path + ".gz";
      useGz = true;
    }
    if (LittleFS.exists(filePath))
    {
      File f = LittleFS.open(filePath, "r");
      if (f)
      {
        // determine content-type based on original path (without .gz)
        String orig = path;
        String ct = "text/plain";
        if (orig.endsWith(".html")) ct = "text/html";
        else if (orig.endsWith(".js")) ct = "application/javascript";
        else if (orig.endsWith(".css")) ct = "text/css";
        else if (orig.endsWith(".svg")) ct = "image/svg+xml";
        else if (orig.endsWith(".png")) ct = "image/png";
        else if (orig.endsWith(".jpg") || orig.endsWith(".jpeg")) ct = "image/jpeg";

        size_t len = f.size();

        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: " + ct);
        if (useGz) client.println("Content-Encoding: gzip");
        client.println("Content-Length: " + String(len));

        // Cache policy: short for HTML, long for static assets
        if (orig.endsWith(".html") || orig == "/index.html")
          client.println("Cache-Control: no-cache, must-revalidate, max-age=0");
        else
          client.println("Cache-Control: public, max-age=31536000, immutable");

        client.println("Connection: close");
        client.println();

        const size_t bufSize = 512;
        uint8_t buffer[bufSize];
        while (f.available())
        {
          size_t r = f.read(buffer, bufSize);
          if (r > 0) client.write(buffer, r);
        }
        f.close();
        client.stop();
        return;
      }
    }
    // otherwise fall back to old main page
    serveMainPage(client);
    delay(1);
    client.stop();
    return;
  }

  // Default: main page
  serveMainPage(client);
  delay(1);
  client.stop();
}

void Webserver_begin()
{
  setupWebServer();
  Serial.printf("[WEB] %s server on http://%s/\n", Network::modeName().c_str(), Network::ip().toString().c_str());
}

// MARK: Main Page
void serveMainPage(WiFiClient &client)
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
void servePreview(WiFiClient &client)
{
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>Download CSV</title>");
  client.println("<style>:root{--bg:#0f1720;--card:#0b1220;--text:#e6eef6;--muted:#9fb0c8;--accent:#4fd1c5} @media(prefers-color-scheme:light){:root{--bg:#f6f9fc;--card:#ffffff;--text:#06202b;--muted:#456275;--accent:#0ea5a4}} html,body{height:100%;margin:0;font-family:system-ui,-apple-system,Segoe UI,Roboto,'Helvetica Neue',Arial;background:var(--bg);color:var(--text)} .app{max-width:900px;margin:0 auto;padding:18px;box-sizing:border-box} header{display:flex;align-items:center;justify-content:space-between;margin-bottom:12px} h1{font-size:1rem;margin:0;color:var(--accent)} .card{background:var(--card);border-radius:12px;padding:14px;box-shadow:0 6px 18px rgba(2,6,23,0.6)} a{color:var(--accent);text-decoration:none} ul{margin:0;padding-left:18px} li{margin:8px 0} .row{display:flex;gap:8px;align-items:center;flex-wrap:wrap} .btn{display:inline-block;background:linear-gradient(90deg,var(--accent),#7af0e6);color:#022029;border:0;padding:10px 12px;border-radius:8px;font-size:.95rem}</style></head><body>");
  client.println("<div class=\"app\">");
  client.println("<header><h1>ESP32 Temp Logger</h1><a class=\"btn\" href=\"/\">Back</a></header>");
  client.println("<section class=\"card\">");
  client.print("<div class=\"row\"><div>Current log: <b>");
  client.print(getLogFilename());
  client.println("</b></div></div>");
  client.println("<h3 style=\"margin:12px 0 8px\">Download CSV</h3>");
  client.println("<ul>");
  listRecentLogs(client);
  client.println("</ul>");
  client.println("<p class=\"row\" style=\"margin-top:12px\"><span class=\"muted\">Tap a file to download.</span></p>");
  client.println("</section>");
  client.println("</div></body></html>");
}

// MARK: list Logs
void listRecentLogs(WiFiClient &client)
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

void serveFileDownload(WiFiClient &client, const String &filename)
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
