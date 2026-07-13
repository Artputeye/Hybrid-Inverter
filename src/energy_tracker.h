#ifndef ENERGY_TRACKER_H
#define ENERGY_TRACKER_H

#include "config.h"

extern float hourlyEnergy[24];
extern float dailyEnergy[30];

void initEnergyTracker();
bool saveEnergyToJson();
bool loadEnergyFromJson();

// ฟังก์ชันสำหรับฝั่งเว็บดึงข้อมูลไปใช้
String getHourlyHistoryJson();
String getDailyHistoryJson();
String getAllEnergyHistoryJson(); // รวมทั้งชั่วโมงและวันในไฟล์เดียว (เผื่อใช้)

#endif // ENERGY_TRACKER_H