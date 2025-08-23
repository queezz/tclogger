#ifndef DISPLAY_H
#define DISPLAY_H

void setupDisplay();
void showStatusDisplay(double temp, const String& status, const String& sdstatus);
void showMessage(const String& message);

// I2C mutex provided by main
extern SemaphoreHandle_t I2C_MTX;

#endif
