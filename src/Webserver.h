#pragma once
#include <WiFi.h>

void setupWebServer();
void handleClient();

void serveMainPage(WiFiClient &client);
void servePreview(WiFiClient &client);
void listRecentLogs(WiFiClient &client);
void serveFileDownload(WiFiClient &client, const String &filename);