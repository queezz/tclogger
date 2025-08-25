#pragma once
#include <WiFi.h>

void setupWebServer();
void handleClient();

// Optional helper for a one-call init+start that logs URL
void Webserver_begin();

void serveMainPage(WiFiClient &client);
void servePreview(WiFiClient &client);
void listRecentLogs(WiFiClient &client);
void serveFileDownload(WiFiClient &client, const String &filename);