#ifndef INV_CONTROL_H
#define INV_CONTROL_H
#include "config.h"

extern int gridCutOff;
extern int gridStart;
extern float energy_kWh;
extern float gridPower ;

void gridRun();
void gridOperation();

#endif