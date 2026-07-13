// app_main.cpp
#include "app_main.h"

void app_setup()
{
  loadEnergyFromFile();
  Serial.println("Load Energy From FS");
  delay(500);
  initEnergyTracker();
  delay(500);
}

void app_loop()
{
  gridRun();
  gridOperation();
}

void updateSystemStatus()
{
}