// ha_integration.h
#ifndef HA_INTEGRATION_H
#define HA_INTEGRATION_H
#include "config.h"

// ตัวแปรที่อนุญาตให้ไฟล์อื่นเรียกใช้
extern HASensor *ipAddrSensor;
extern HASensor *macAddrSensor;
extern HASensor *uptimeSensor;
extern HASensor *rssiSensor;

extern HASwitch *grid;

extern HASensorNumber *LoadPercent;
extern HASensorNumber *EnergyDaily;
extern HASensorNumber *GridPower;
extern HASensorNumber *ActivePower;
extern HASensorNumber *ApparentPower;
extern HASensorNumber *OutputVolt;
extern HASensorNumber *OutputCurrent;
extern HASensorNumber *OutputFrequency;
extern HASensorNumber *PowerFactor;
extern HASensorNumber *pvPower;
extern HASensorNumber *pvCurrent;
extern HASensorNumber *pvVoltage;
extern HASensorNumber *BusVoltage;
extern HASensorNumber *BattVoltage;
extern HASensorNumber *Temp;

// ฟังก์ชันหลัก
void GridTie(bool state, HASwitch *sender);
void iotHAsetup();
void HA_Diagnostic();
void iotHArun();

#endif