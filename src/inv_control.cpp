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
unsigned long lastRespons = 0;
unsigned long lastFile = 0;
unsigned long lastGridOpr = 0;
unsigned long lastGridCheck = 0;
unsigned long lastEnergy = 0;
/////////////////////////////////////////////////////////////////////////////////
const unsigned long qvalInterval = 5000;
const unsigned long qrateInterval = 10000;
const unsigned long resInterval = 100;
const unsigned long fileInterval = 15 * 60 * 1000; //record to file every 15 minutes
const unsigned long gridOprInterval = 1000;
const unsigned long gridCheckInterval = 1 * 60 * 1000; //(1 * 60 * 1000 ms)

/////////////////////////////////////////////////////////////////////////////////
bool toggle;
bool gridState = false;
/////////////////////////////////////////////////////////////////////////////////

void gridRun()
{
    // 1. ส่วนส่งคำสั่งถามข้อมูล (Inquiry)
    if ((millis() - lastQvalue) > qvalInterval) // ทุกๆ 5 วินาที
    {
        lastQvalue = millis();
        if (inv.RunMode)
        {
            inv.cmd_inv("QPIGS");
            //Serial.println("Sent in function gridRun");
            //wsJsonControll("Sent in function gridRun");
        }
        simulateData();
    }

    // if ((millis() - lastQrate) > qrateInterval) // ทุกๆ 10 วินาที
    // {
    //     lastQrate = millis();
    //     if (inv.RunMode)
    //     {
    //         if (toggle)
    //         {
    //             inv.cmd_inv("QPIRI"); 
    //         }
    //         else
    //         {
    //             inv.cmd_inv("QPIWS"); 
    //         }
    //         toggle = !toggle;
    //     }
    // }

    // 2. ส่วนรับข้อมูล (Response Handler) ทำหน้าที่คอยตรวจเช็กทุกๆ 100ms
    if ((millis() - lastRespons) > resInterval) 
    {
        lastRespons = millis();
        
        // บันทึกค่าความยาวก่อนเรียก Response เพื่อเอาไว้เช็กว่ามีข้อมูลใหม่เข้ามาจริงไหม
        int oldLen = inv.invData.length(); 
        
        inv.Response(); // เรียกตรวจสอบข้อมูลขาเข้า
        
        // ถ้าค่า invData เปลี่ยนไป และไม่เป็นค่าว่าง แสดงว่าได้รับข้อมูลชุดใหม่เรียบร้อยแล้ว
        if (inv.invData.length() > 0 && inv.invData.length() != oldLen)
        {
            //wsJsonInverter("Respond from inv.invData: " + inv.invData);
            //wsJsonSerial(inv.serialData);
        }
    }
}

void gridOperation()
{
    unsigned long currentMillis = millis();
    float dt = (currentMillis - lastEnergy) / 1000.0;
    lastEnergy = currentMillis;

    float outputPower = inv.data.ActivePower;
    float pvPower = inv.data.pvPower;
    gridPower = outputPower - pvPower;

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

    energy_kWh += ((gridPower * dt) / 3600000.0);

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
