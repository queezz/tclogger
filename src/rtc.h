#pragma once

#include <Arduino.h>
#include <RTClib.h>

void rtc_setup();
void rtc_update_from_system();
void rtc_set_system_from_rtc();
void rtc_clear_osf();
uint8_t rtc_status();
bool rtc_time_valid();
DateTime rtc_now();
String rtc_timestamp(const DateTime &dt);
String rtc_datestamp(const DateTime &dt);
float rtc_temp();

// I2C mutex provided by main
extern SemaphoreHandle_t I2C_MTX; // defined in main.cpp


