#include <Arduino.h>
static unsigned long sampleInterval = 500;

void setSamplingInterval(float seconds)
{
  sampleInterval = (unsigned long)(seconds * 1000);
  Serial.println("Sampling interval updated: " + String(sampleInterval / 1000.) + " s");
}

unsigned long getSamplingInterval()
{
  return sampleInterval;
}
