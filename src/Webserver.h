#pragma once
#include <Ethernet.h> 

void setupWebServer();
void handleClient();

void serveMainPage(EthernetClient& client);
void servePreview(EthernetClient& client);
void listRecentLogs(EthernetClient& client);