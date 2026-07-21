#include "inv_control.h"

//////////////////////////////////////////////////////////////////////////////////
int gridCutOff;
int gridStart;
float energy_kWh = 0.0;
float gridPower = 0.0;
const float GRID_ON_THRESHOLD = 2.0;
const float GRID_OFF_THRESHOLD = 1.0;
//////////////////////////////////////////////////////////////////////////////////
unsigned long lastQvalue = 0;
unsigned long lastQrate = 0;

unsigned long lastFile = 0;
unsigned long lastGridOpr = 0;
unsigned long lastGridCheck = 0;
unsigned long lastEnergy = 0;
/////////////////////////////////////////////////////////////////////////////////
const unsigned long qvalInterval = 5000;
const unsigned long qrateInterval = 19000;

const unsigned long fileInterval = 15 * 60 * 1000; //record to file every 15 minutes
const unsigned long gridOprInterval = 1000;
const unsigned long gridCheckInterval = 1 * 60 * 1000; //(1 * 60 * 1000 ms)

/////////////////////////////////////////////////////////////////////////////////
bool toggle;
bool gridState = false;
/////////////////////////////////////////////////////////////////////////////////

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
            wsJsonInverter(String(inv.invData.length()));       
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
            wsJsonInverter(String(inv.invData.length()));       
        }
    }
}

void gridOperation()
{
    unsigned long currentMillis = millis();
    float dt = (currentMillis - lastEnergy) / 1000.0;
    lastEnergy = currentMillis;

    static bool clearedToday = false;
    if (rtc.hour == 18 && rtc.minute == 0 && !clearedToday)
    {
        Serial.println("🕕 18:00 detected — initiating daily reset...");
        bool success = clearEnergyFile();

        if (!success)
        {
            Serial.println("⚠️ Warning: daily reset failed, retrying...");
            vTaskDelay(pdMS_TO_TICKS(1000)); 
            success = clearEnergyFile();
        }

        if (success)
            Serial.println("✅ Daily energy reset complete");
        else
            Serial.println("❌ Daily reset failed after retry");

        clearedToday = true;
    }

    if (rtc.hour == 0 && rtc.minute == 0)
    {
        clearedToday = false;
    }

    energy_kWh += ((inv.data.gridPower * dt) / 3600000.0);

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
            inv.valueToinv("GridTieOperation", 0);
            Serial.println("🔴 Grid OFF (within cut-off period)");
            wsJsonControll("Grid OFF (within cut-off period)");
            Serial.println(" >>>>> ENTER DATE BLOCK <<<<<");
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
                inv.valueToinv("GridTieOperation", 0);
                gridState = false;
                Serial.println("🔴 Grid OFF (energy < 1.0 kWh)");
                wsJsonControll("Grid OFF (energy < 1.0 kWh)");
            }
            else if (energy_kWh > GRID_ON_THRESHOLD)
            {
                inv.valueToinv("GridTieOperation", 1);
                gridState = true;
                Serial.println("🟢 Grid ON (energy > 2.0 kWh)");
                wsJsonControll("Grid ON (energy > 2.0 kWh)");
            }
            else
            {
                inv.valueToinv("GridTieOperation", gridState ? 1 : 0);
                Serial.printf("⚙️ Confirming Grid %s (energy = %.3f)\n",
                              gridState ? "ON" : "OFF", energy_kWh);
                wsJsonControll(String("⚙️ Confirming Grid ") + (gridState ? "ON" : "OFF") +
                             String(" (energy = ") + String(energy_kWh, 3) + String(" kWh)"));
            }
        }
    }
}
