#ifndef SDCARD_H
#define SDCARD_H

void setupSDCard();
bool isSDReady();
void flushLogBufferToSD();
void readLogFile();
void logTemperature(double temperature);

#endif
