#ifndef ENERGY_TRACKER_H
#define ENERGY_TRACKER_H

#include "config.h"

void initEnergyTracker();
bool saveEnergyToJson();
bool loadEnergyFromJson();
void updateEnergyHistory(bool force = false);

#endif // ENERGY_TRACKER_H