#pragma once
#include <WiFi.h>
#include <WebServer.h>

void setupWebServer();
void handleClient();

void serveMainPage();
void servePreview();
void handleSamplingInterval();
void handleNewLogFile();
void serveDownload();
