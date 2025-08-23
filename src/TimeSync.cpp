#include <time.h>
#include <sys/time.h>
#include "rtc.h"

void syncTimeFromRTC()
{
  rtc_set_system_from_rtc();
  time_t now = time(nullptr);
  struct tm *t = localtime(&now);
  if (t)
  {
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", t);
    Serial.println(buf);
    Serial.println("System time synced from RTC.");
  }
}

void getTimestamp(char *out, size_t len)
{
  time_t now = time(nullptr);
  struct tm *t = localtime(&now);
  strftime(out, len, "%Y-%m-%d %H:%M:%S", t);
}


