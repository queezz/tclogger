#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>

void setupNetwork();
IPAddress getLocalIP();
bool isNetworkUp();

#endif
