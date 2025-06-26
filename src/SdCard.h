#ifndef SDCARD_H
#define SDCARD_H

void setupSDCard();
bool isSDReady();
void logTemperature(double temperature);
void readLogFile();

#endif
