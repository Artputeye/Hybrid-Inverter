#ifndef ENERGY_TRACKER_H
#define ENERGY_TRACKER_H

#include "config.h"

extern float hourlyEnergy[24];
extern float dailyEnergy[30];

void initEnergyTracker();
bool saveEnergyToJson();
bool loadEnergyFromJson();

#endif // ENERGY_TRACKER_H