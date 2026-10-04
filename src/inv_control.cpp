#include "inv_control.h"

//////////////////////////////////////////////////////////////////////////////////
int gridCutOff;
int gridStart;
float energy_kWh = 0.0;
float energy_m_kWh = 0.0;
float solar_kWh = 0.0;
float solar_m_kWh = 0.0;
float gridPower = 0.0;
float expenseUnitCost1 = 200.0;
float expensePriceCost1 = 3.0000;
float expenseUnitCost2 = 400.0;
float expensePriceCost2 = 4.1584;
float expensePriceCost3 = 4.3583;
float expenseUnitSolar = 450.0;
float expenseFt = 0.3972;
float expenseServiceFee = 38.22;
float expenseVatRate = 7.0;
float gridCostMonthly = 0.0;
float solarSavingsMonthly = 0.0;
float co2ReductionMonthlyKg = 0.0;
float selfSufficiencyPct = 0.0;
const float GRID_ON_THRESHOLD = 2.0;
const float GRID_OFF_THRESHOLD = 1.0;
//////////////////////////////////////////////////////////////////////////////////
unsigned long lastQvalue = 0;
unsigned long lastQrate = 0;
unsigned long lastQfault = 0;

unsigned long lastFile = 0;
unsigned long lastGridOpr = 0;
unsigned long lastGridCheck = 0;
unsigned long lastEnergy = 0;
/////////////////////////////////////////////////////////////////////////////////
const unsigned long qvalInterval = 3000;
const unsigned long qrateInterval = 19000;
const unsigned long qfaultInterval = 10000;

const unsigned long fileInterval = 15 * 60 * 1000; // record to file every 15 minutes
const unsigned long gridOprInterval = 1000;
const unsigned long gridCheckInterval = 1 * 60 * 1000; //(1 * 60 * 1000 ms)

/////////////////////////////////////////////////////////////////////////////////
bool toggle;
bool gridState = false;
/////////////////////////////////////////////////////////////////////////////////

// ------------------------------------------------------------------------------
// Expense calculation helpers (progressive utility tariff and solar savings)
// ------------------------------------------------------------------------------
static const float GRID_CO2_FACTOR_KG_PER_KWH = 0.50f; // Approximate grid emissions factor; update for local data.

static float calculateProgressiveGridCost(float units)
{
    if (units <= 0.0f) return 0.0f;

    float tier1Limit = expenseUnitCost1;
    float tier2Limit = expenseUnitCost2 > tier1Limit ? expenseUnitCost2 : tier1Limit;
    float tier1Units = units < tier1Limit ? units : tier1Limit;
    float cost = tier1Units * expensePriceCost1;

    if (units > tier1Limit)
    {
        float tier2Units = units - tier1Limit;
        if (tier2Units > tier2Limit - tier1Limit) tier2Units = tier2Limit - tier1Limit;
        cost += tier2Units * expensePriceCost2;
    }
    if (units > tier2Limit)
    {
        cost += (units - tier2Limit) * expensePriceCost3;
    }
    return cost;
}

static float calculateEstimatedBill(float units)
{
    float subtotal = calculateProgressiveGridCost(units) + (units * expenseFt) + expenseServiceFee;
    if (subtotal < 0.0f) subtotal = 0.0f;
    return subtotal * (1.0f + expenseVatRate / 100.0f);
}

void updateExpenseTotals()
{
    float gridUnits = energy_m_kWh > 0.0f ? energy_m_kWh : 0.0f;
    float solarUnits = solar_m_kWh > 0.0f ? solar_m_kWh : 0.0f;
    gridCostMonthly = calculateEstimatedBill(gridUnits);

    // Estimate self-consumed solar as the difference between bills before and after generation.
    float billWithSolarOffset = calculateEstimatedBill(gridUnits + solarUnits);
    solarSavingsMonthly = billWithSolarOffset - gridCostMonthly;
    if (solarSavingsMonthly < 0.0f) solarSavingsMonthly = 0.0f;

    // Estimate avoided grid emissions from monthly solar generation.
    co2ReductionMonthlyKg = solarUnits * GRID_CO2_FACTOR_KG_PER_KWH;
}

// ------------------------------------------------------------------------------
// Energy benefits: estimate the monthly share of energy supplied by solar.
// ------------------------------------------------------------------------------
void updateSelfSufficiency()
{
    float gridUnits = energy_m_kWh > 0.0f ? energy_m_kWh : 0.0f;
    float solarUnits = solar_m_kWh > 0.0f ? solar_m_kWh : 0.0f;
    float totalUnits = gridUnits + solarUnits;

    // This estimate assumes solar generation is consumed on site; exports are not measured.
    selfSufficiencyPct = totalUnits > 0.0f ? (solarUnits / totalUnits) * 100.0f : 0.0f;
}

void gridRun()
{
    // 1. ถาม QPIGS (ทุก 5 วินาที)
    if ((millis() - lastQvalue) > qvalInterval)
    {
        lastQvalue = millis();
        if (inv.RunMode)
        {
            inv.sendCommand("QPIGS");
            wsJsonSerial("Sent to inv: QPIGS");
            inv.Response(); // เรียก Response ทันทีหลังจาก sendCommand
            wsJsonInverter(inv.invData);
        }
    }

    // 2. ถาม QPIRI (ทุก 19 วินาที)
    if ((millis() - lastQrate) > qrateInterval)
    {
        lastQrate = millis();
        if (inv.RunMode)
        {
            vTaskDelay(pdMS_TO_TICKS(200)); // 🔴 เว้นช่วง 200ms เผื่อรอบ QPIGS เพิ่งทำงานไป
            inv.sendCommand("QPIRI");
            wsJsonSerial("Sent to inv: QPIRI");
            inv.Response();
            wsJsonInverter(inv.invData);
        }
    }

    // Keep the shared WebSocket fault field fresh for every connected page.
    if ((millis() - lastQfault) > qfaultInterval)
    {
        lastQfault = millis();
        if (inv.RunMode)
        {
            inv.sendCommand("QPIWS");
            inv.Response();
            wsJsonInverter(inv.invData);
        }
    }
}

void gridOperation()
{
    unsigned long currentMillis = millis();
    float dt = (currentMillis - lastEnergy) / 1000.0;
    lastEnergy = currentMillis;

    if (inv.energyreset)
    {
        Serial.println("Energy Reset Command Received");
        updateEnergyHistory(true);
        bool success = clearEnergyFile();
        energy_kWh = 0.0;
        energy_m_kWh = 0.0;
        solar_kWh = 0.0;
        solar_m_kWh = 0.0;
        if (!success)
        {
            Serial.println("⚠️ Warning: energy reset failed, retrying...");
            vTaskDelay(pdMS_TO_TICKS(1000));
            success = clearEnergyFile();
        }

        if (success)
            Serial.println("✅ Energy reset complete");
        else
            Serial.println("❌ Energy reset failed after retry");

        inv.energyreset = false; // Reset the flag after processing
    }

    // --------------------------------------------------------------------------
    // 🔹 Daily Reset (เคลียร์เฉพาะค่ารายวัน โดยคงค่ารายเดือนไว้)
    // --------------------------------------------------------------------------
    static bool clearedToday = false;
    if (rtc.hour == 18 && rtc.minute == 0 && !clearedToday)
    {
        Serial.println("🕕 18:00 detected — initiating daily reset...");
        updateEnergyHistory(true);
        bool success = clearDailyEnergyCounters();

        if (!success)
        {
            Serial.println("⚠️ Warning: daily reset failed, retrying...");
            vTaskDelay(pdMS_TO_TICKS(1000));
            success = clearDailyEnergyCounters();
        }

        if (success)
            Serial.println("✅ Daily energy reset complete");
        else
            Serial.println("❌ Daily reset failed after retry");

        clearedToday = true;
    }

    if (rtc.hour != 18 || rtc.minute != 0)
    {
        clearedToday = false;
    }

    // --------------------------------------------------------------------------
    // 🔹 Monthly Reset (คิดจากค่ากลางระหว่าง gridCutOff และ gridStart)
    // --------------------------------------------------------------------------
    int monthlyResetDay = (gridCutOff + gridStart) / 2;
    if (monthlyResetDay <= 0) monthlyResetDay = 1; // Default fallback เป็นวันที่ 1

    static bool clearedMonthly = false;
    if (rtc.day == monthlyResetDay && rtc.hour == 0 && rtc.minute == 0 && !clearedMonthly)
    {
        Serial.printf("🗓️ Mid-point Day %d 00:00 detected — resetting monthly energy counters...\n", monthlyResetDay);
        updateEnergyHistory(true);
        energy_m_kWh = 0.0;
        solar_m_kWh = 0.0;
        clearedMonthly = true;
        saveEnergyToFile();
    }
    if (rtc.day != monthlyResetDay || rtc.hour != 0 || rtc.minute != 0)
    {
        clearedMonthly = false;
    }

    // --------------------------------------------------------------------------
    // 🔹 คำนวณสะสมพลังงานไฟฟ้า (Energy Integration)
    // --------------------------------------------------------------------------
    float gridEnergyDelta = (inv.data.gridPower * dt) / 3600000.0;
    float pvEnergyDelta   = (inv.data.pvPower   * dt) / 3600000.0;

    if (gridEnergyDelta > 0)
    {
        energy_kWh += gridEnergyDelta;
        energy_m_kWh += gridEnergyDelta;
    }

    if (pvEnergyDelta > 0)
    {
        solar_kWh += pvEnergyDelta;
        solar_m_kWh += pvEnergyDelta;
    }

    addEnergyHistory(gridEnergyDelta, pvEnergyDelta);

    updateExpenseTotals();
    updateSelfSufficiency();

    if (millis() - lastGridOpr > gridOprInterval) // debug grid operation
    {
        lastGridOpr = millis();
        timeUpdade();
        if (inv.test)
        {
            Serial.println("Grid Operate : " + String(inv.gridOpr));
        }
    }

    if (millis() - lastFile > fileInterval)
    {
        lastFile = millis();
        saveEnergyToFile();
    }

    if (millis() - lastGridCheck > gridCheckInterval)
    {
        lastGridCheck = millis();
        Serial.println("--- Grid Operation Check ---");
        Serial.println("Current Date: " + String(rtc.day));

        ///////////////////////////////////////////////
        Serial.printf("tm_mday=%d, gridCutOff=%d, gridStart=%d\n",
                      rtc.day, gridCutOff, gridStart);

        wsJsonControll(String("tm_mday=") + String(rtc.day) +
                       String(" gridCutOff=") + String(gridCutOff) +
                       String(" gridStart=") + String(gridStart) +
                       String(" tm_hour=") + String(rtc.hour) + String(" tm_min=") + String(rtc.minute));

        /////////////////////////////////////////////////

        if (rtc.day >= gridCutOff && rtc.day <= gridStart)
        {
            // Only write to the inverter when the state actually changes.
            // Re-sending this Modbus command every minute is unnecessary and
            // can collide with the periodic QPIGS/QPIRI/QPIWS traffic.
            if (gridState)
            {
                inv.valueToinv("GridTieOperation", 0);
                gridState = false;
                Serial.println("🔴 Grid OFF (within cut-off period)");
                wsJsonControll("Grid OFF (within cut-off period)");
            }
            return;
        }
        else
        {
            Serial.println("NOT in date range");
        }

        if (inv.gridOpr)
        {
            if (energy_kWh < GRID_OFF_THRESHOLD)
            {
                if (gridState)
                {
                    inv.valueToinv("GridTieOperation", 0);
                    gridState = false;
                    Serial.println("🔴 Grid OFF (energy < 1.0 kWh)");
                    wsJsonControll("Grid OFF (energy < 1.0 kWh)");
                }
            }
            else if (energy_kWh > GRID_ON_THRESHOLD)
            {
                if (!gridState)
                {
                    inv.valueToinv("GridTieOperation", 1);
                    gridState = true;
                    Serial.println("🟢 Grid ON (energy > 2.0 kWh)");
                    wsJsonControll("Grid ON (energy > 2.0 kWh)");
                }
            }
        }
    }
}

// ==============================================================================
// 🔹 ฟังก์ชันประมวลผลการตั้งค่า Inverter (แยกออกมาจาก HTTP Server POST Route)
// ==============================================================================
String processInverterSetting(const JsonDocument &doc)
{
    String setting = doc["setting"].is<String>() ? doc["setting"].as<String>() : "";
    uint16_t value = doc["value"].is<uint16_t>() ? doc["value"].as<uint16_t>() : 0;
    gridCutOff     = doc["gridCutOff"].is<int>()  ? doc["gridCutOff"].as<int>()  : -1;
    gridStart      = doc["gridStart"].is<int>()   ? doc["gridStart"].as<int>()   : -1;

    // 1. ส่งค่าคำสั่งไปยัง Inverter Object
    if (setting != "")
    {
        inv.valueToinv(setting, value);
        Serial.printf("📥 Setting: %s = %d\n", setting.c_str(), value);
    }

    // 2. จัดการโหมด Grid Tie Auto (เปิดใช้งาน)
    if (setting == "Grid Tie Auto" && value == 1)
    {
        inv.gridOpr = true;
        Serial.printf("📥 Grid Tie Auto: %s = %d\n", setting.c_str(), value);
    }

    // 3. จัดการโหมด Grid Tie Auto (ปิดใช้งาน)
    if (setting == "Grid Tie Auto" && value == 0)
    {
        inv.gridOpr = false;
        Serial.printf("📥 Grid Tie Auto: %s = %d\n", setting.c_str(), value);
    }

    // 4. สร้าง JSON Response สรุปผล
    String response = "{\"status\":\"ok\"";
    if (setting != "") response += ",\"setting\":\"" + setting + "\",\"value\":" + String(value);
    if (gridCutOff != -1) response += ",\"gridCutOff\":" + String(gridCutOff);
    if (gridStart  != -1) response += ",\"gridStart\":"  + String(gridStart);
    response += "}";

    return response;
}
