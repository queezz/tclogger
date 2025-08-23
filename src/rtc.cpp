#include "rtc.h"
#include <Wire.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

extern SemaphoreHandle_t I2C_MTX;

static RTC_DS3231 rtc;

static String twoDigits(int v)
{
  if (v < 10)
    return String("0") + String(v);
  return String(v);
}

void rtc_clear_osf()
{
  Wire.beginTransmission(0x68);
  Wire.write(0x0F);
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 1);
  uint8_t s = 0;
  if (Wire.available())
  {
    s = Wire.read();
  }
  s &= ~(1 << 7);
  Wire.beginTransmission(0x68);
  Wire.write(0x0F);
  Wire.write(s);
  Wire.endTransmission();
}

uint8_t rtc_status()
{
  Wire.beginTransmission(0x68);
  Wire.write(0x0F);
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 1);
  if (Wire.available())
  {
    return Wire.read();
  }
  return 0xFF; // error indicator
}

bool rtc_time_valid()
{
  return time(nullptr) > 1700000000; // ~2023-11-14
}

DateTime rtc_now()
{
  return rtc.now();
}

float rtc_temp()
{
  return rtc.getTemperature();
}

String rtc_timestamp(const DateTime &dt)
{
  return String(dt.year()) + "-" + twoDigits(dt.month()) + "-" + twoDigits(dt.day()) +
         " " + twoDigits(dt.hour()) + ":" + twoDigits(dt.minute()) + ":" + twoDigits(dt.second());
}

String rtc_datestamp(const DateTime &dt)
{
  return String(dt.year()) + twoDigits(dt.month()) + twoDigits(dt.day());
}

void rtc_set_system_from_rtc()
{
  DateTime dt = rtc.now();
  struct tm tmv;
  tmv.tm_year = dt.year() - 1900;
  tmv.tm_mon = dt.month() - 1;
  tmv.tm_mday = dt.day();
  tmv.tm_hour = dt.hour();
  tmv.tm_min = dt.minute();
  tmv.tm_sec = dt.second();
  time_t t = mktime(&tmv);
  struct timeval tv;
  tv.tv_sec = t;
  tv.tv_usec = 0;
  settimeofday(&tv, nullptr);
}

void rtc_update_from_system()
{
  time_t t = time(nullptr);
  struct tm *tmv = localtime(&t);
  if (tmv)
  {
    rtc.adjust(DateTime(1900 + tmv->tm_year, 1 + tmv->tm_mon, tmv->tm_mday, tmv->tm_hour, tmv->tm_min, tmv->tm_sec));
    rtc_clear_osf();
  }
}

void rtc_setup()
{
  if (!rtc.begin())
  {
    Serial.println("RTC not found!");
    return;
  }

  if (rtc.lostPower())
  {
    Serial.println("RTC lost power, setting from compile time");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    rtc_clear_osf();
  }

  // Always initialize ESP32 system clock from RTC at boot
  rtc_set_system_from_rtc();
}


