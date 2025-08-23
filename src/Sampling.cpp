#include <Arduino.h>
static unsigned long sampleInterval = 500;

// Rolling average state
static double rollingSum = 0.0;
static unsigned long rollingCount = 0;
static double lastAveraged = NAN;

void setSamplingInterval(float seconds)
{
  sampleInterval = (unsigned long)(seconds * 1000);
  Serial.println("Sampling interval updated: " + String(sampleInterval / 1000.) + " s");
}

unsigned long getSamplingInterval()
{
  return sampleInterval;
}

void accumulateRolling(double temperature)
{
  rollingSum += temperature;
  rollingCount++;
}

double getCurrentAverage()
{
  if (rollingCount == 0) return NAN;
  return rollingSum / (double)rollingCount;
}

double finalizeIntervalAverage()
{
  if (rollingCount == 0)
  {
    lastAveraged = NAN;
  }
  else
  {
    lastAveraged = rollingSum / (double)rollingCount;
  }
  rollingSum = 0.0;
  rollingCount = 0;
  return lastAveraged;
}

double getLastAveragedTemp()
{
  return lastAveraged;
}
