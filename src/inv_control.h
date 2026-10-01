#ifndef INV_CONTROL_H
#define INV_CONTROL_H
#include "config.h"

extern int gridCutOff;
extern int gridStart;
extern float energy_kWh;   // ค่า grid energy (สะสม)
extern float energy_m_kWh; // ค่า grid energy รายเดือน
extern float solar_kWh;    // ค่า solar energy รายวัน (จาก inv.data.pvPower)
extern float solar_m_kWh;  // ค่า solar energy รายเดือน (จาก inv.data.pvPower)
extern float gridPower;
extern float expenseUnitCost1;
extern float expensePriceCost1;
extern float expenseUnitCost2;
extern float expensePriceCost2;
extern float expensePriceCost3;
extern float expenseUnitSolar;
extern float expenseFt;
extern float expenseServiceFee;
extern float expenseVatRate;
extern float gridCostMonthly;
extern float solarSavingsMonthly;
extern float co2ReductionMonthlyKg;
extern float selfSufficiencyPct;

void gridRun();
void gridOperation();
void updateExpenseTotals();
void updateSelfSufficiency();
String processInverterSetting(const JsonDocument &doc);

#endif