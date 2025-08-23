#pragma once
void setSamplingInterval(float seconds);
unsigned long getSamplingInterval();

// Rolling averaging helpers
void accumulateRolling(double temperature);
double getCurrentAverage();
double finalizeIntervalAverage();
double getLastAveragedTemp();