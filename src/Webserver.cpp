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

  // --- Serve /explore (new Explore Data page) ---
  if (req.indexOf("GET /explore ") == 0 || req.indexOf("GET /explore?") == 0 || req.indexOf("GET /explore.html") >= 0)
  {
    // Try to serve precompressed asset from LittleFS if present
    String path = "/explore.html";
    bool useGz = false;
    String filePath = path;
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
        String ct = "text/html";
        size_t len = f.size();

        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: " + ct);
        if (useGz) client.println("Content-Encoding: gzip");
        client.println("Content-Length: " + String(len));
        client.println("Cache-Control: no-cache, must-revalidate, max-age=0");
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

    // Fallback: if file missing, redirect to main page
    serveMainPage(client);
    delay(1);
    client.stop();
    return;
  }

  // --- Preview (redirect to new Explore page) ---
  if (req.indexOf("GET /preview") >= 0)
  {
    client.println("HTTP/1.1 302 Found");
    client.println("Location: /explore");
    client.println("Connection: close");
    client.println();
    client.stop();
    return;
  }

  // --- API: /files.json ---
  if (req.indexOf("GET /files.json") >= 0)
  {
    // Return a JSON array of CSV log files (name=basename, size, mtime)
    deselectAllSPI();
    digitalWrite(CS_SD, LOW);

    auto appendDirCsv = [&](const char *dirPath, String &outJson, std::vector<String> &seen){
      File dir = SD.open(dirPath);
      if (!dir || !dir.isDirectory()) { if (dir) dir.close(); return; }
      File entry = dir.openNextFile();
      while (entry)
      {
        if (!entry.isDirectory())
        {
          String full = entry.name();
          if (full.endsWith(".csv"))
          {
            // basename
            String base = full;
            int slash = base.lastIndexOf('/');
            if (slash >= 0 && slash < (int)base.length()-1) base = base.substring(slash+1);
            bool dup = false;
            for (const auto &s : seen) { if (s == base) { dup = true; break; } }
            if (!dup)
            {
              seen.push_back(base);
              size_t sz = entry.size();
              outJson += "{\"name\":\"" + base + "\",\"size\":" + String(sz) + ",\"mtime\":0},";
            }
          }
        }
        entry.close();
        entry = dir.openNextFile();
      }
      dir.close();
    };

    String j = "[";
    std::vector<String> seen;
    appendDirCsv("/logs", j, seen);
    appendDirCsv("/", j, seen);

    digitalWrite(CS_SD, HIGH);

    if (j.endsWith(",")) j.remove(j.length()-1);
    j += "]";

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    client.println(j);
    client.stop();
    return;
  }

  // --- API: /logs/<filename> ---
  if (req.indexOf("GET /logs/") >= 0)
  {
    // Extract filename (path between /logs/ and space)
    int idx = req.indexOf("GET /logs/") + 10; // length of "GET /logs/"
    int sp = req.indexOf(' ', idx);
    if (sp < 0) sp = req.length();
    String filename = req.substring(idx, sp);
    filename.trim();

    // Security checks
    if (filename.length() == 0 || filename.indexOf("/") >= 0 || !filename.endsWith(".csv"))
    {
      client.println("HTTP/1.1 400 Bad Request");
      client.println("Connection: close");
      client.println();
      client.println("Invalid filename.");
      client.stop();
      return;
    }

    String path = "/logs/" + filename;
    deselectAllSPI();
    digitalWrite(CS_SD, LOW);
    File f = SD.open(path);
    if (!f)
    {
      // Fallback to SD root for legacy files
      path = "/" + filename;
      f = SD.open(path);
    }
    if (!f || f.isDirectory())
    {
      client.println("HTTP/1.1 404 Not Found");
      client.println("Connection: close");
      client.println();
      client.println("File not found.");
      if (f) f.close();
      digitalWrite(CS_SD, HIGH);
      client.stop();
      return;
    }

    size_t len = f.size();
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/csv");
    client.println("Content-Length: " + String(len));
    client.println("Connection: close");
    client.println();

    const size_t bufSize = 256;
    uint8_t buffer[bufSize];
    while (f.available())
    {
      size_t r = f.read(buffer, bufSize);
      if (r > 0) client.write(buffer, r);
    }
    f.close();
    digitalWrite(CS_SD, HIGH);
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
    // strip query string if present (e.g., /plotter.html?file=...)
    int q = path.indexOf('?');
    if (q >= 0) {
      path = path.substring(0, q);
    }
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
  // Serve LittleFS /index.html (prefer precompressed .gz) if present
  String path = "/index.html";
  String filePath = path;
  bool useGz = false;
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
      String ct = "text/html";
      size_t len = f.size();

      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: " + ct);
      if (useGz) client.println("Content-Encoding: gzip");
      client.println("Content-Length: " + String(len));
      client.println("Cache-Control: no-cache, must-revalidate, max-age=0");
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
      return;
    }
  }

  // Fallback: minimal text response if index.html missing
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();
  client.println("ESP32 Temp Logger\n\nPlease upload 'index.html' to LittleFS.");
}

// MARK: Preview
void servePreview(WiFiClient &client)
{
  // Redirect preview requests to the Explore page (served from LittleFS)
  client.println("HTTP/1.1 302 Found");
  client.println("Location: /explore");
  client.println("Connection: close");
  client.println();
}

// MARK: list Logs
void listRecentLogs(WiFiClient &client)
{
  // NOTE: listing of logs is now handled client-side via /files.json and the Explore page.
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
