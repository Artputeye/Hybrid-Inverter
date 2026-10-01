// storage_manager.cpp
#include "storage_manager.h"
const char *targetDirectory = "/";
String energyFile = "/energy.json";
unsigned long lastSaveTime = 0;
unsigned long lastClearEnergy = 0;
const unsigned long saveInterval = 1UL * 60UL * 1000UL; // 15 นาที

// แสดงรายการไฟล์ทั้งหมด (รองรับ Sub-folder)
void listAllFiles(const char *dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\n", dirname);

    File root = LittleFS.open(dirname);
    if (!root || !root.isDirectory())
    {
        Serial.println(F("❌ Failed to open directory or not a directory"));
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.printf("  DIR : %s\n", file.name());
            if (levels > 0)
            {
                listAllFiles(file.path(), levels - 1);
            }
        }
        else
        {
            Serial.printf("  FILE: %-20s  SIZE: %u bytes\n", file.name(), file.size());
        }
        file = root.openNextFile();
    }
}

void formatFS()
{
    Serial.println(F("⚠️ Formatting LittleFS..."));
    if (LittleFS.format())
    {
        Serial.println(F("✅ Format successful"));
    }
    else
    {
        Serial.println(F("❌ Format failed"));
    }
}

// ฟังก์ชันกลางสำหรับโหลด JSON
bool loadJsonFile(const char *filename, JsonDocument &doc)
{
    if (!LittleFS.exists(filename))
    {
        Serial.printf("⚠️ File not found: %s\n", filename);
        return false;
    }

    File file = LittleFS.open(filename, "r");
    if (!file)
        return false;

    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error)
    {
        Serial.printf("❌ JSON Parse Error (%s): %s\n", filename, error.c_str());
        return false;
    }
    return true;
}

// ฟังก์ชันกลางสำหรับบันทึก JSON
bool saveJsonFile(const char *filename, const JsonDocument &doc)
{
    File file = LittleFS.open(filename, "w"); // "w" จะทับไฟล์เดิมโดยอัตโนมัติ
    if (!file)
    {
        Serial.printf("❌ Failed to open %s for writing\n", filename);
        return false;
    }

    if (serializeJson(doc, file) == 0)
    {
        Serial.printf("❌ Failed to write to %s\n", filename);
        file.close();
        return false;
    }

    file.close();
    Serial.printf("✅ Saved: %s\n", filename);
    return true;
}

// โหลดการตั้งค่าทั้งหมดของระบบ
bool loadAllSettings()
{
    JsonDocument doc;
    // Load General Settings
    doc.clear(); // ล้างข้อมูลเดิมก่อนโหลดไฟล์ใหม่
    if (loadJsonFile("/setting.json", doc))
    {
        inv.gridOpr = atoi(doc["Grid Tie Auto"] | "0");
        gridCutOff = atoi(doc["gridCutOff"] | "");
        gridStart = atoi(doc["gridStart"] | "");
        Serial.println(F("📂 General settings loaded"));
    }
    else
    {
        Serial.println("❌ Failed to load setting.json");
        inv.gridOpr = 0; // default
    }

    Serial.printf("🔧 Grid Settings Loaded:\n");
    Serial.printf("   - Wifi Mode         : %d\n", isWifiApMode);
    Serial.printf("   - Grid Tie Auto     : %d\n", inv.gridOpr);
    Serial.printf("   - Grid Cut-Off Date : %d\n", gridCutOff);
    Serial.printf("   - Grid Start Date   : %d\n", gridStart);

    return true;
}

// ------------------------------------------------------------------------------
// Expense Settings: load progressive tariff rates and refresh monthly estimates.
// ------------------------------------------------------------------------------
bool loadExpenseSettings()
{
    // Load progressive tariff and additional expense settings.
    JsonDocument expenseDoc;
    expenseUnitCost1 = 200.0f;
    expensePriceCost1 = 3.0000f;
    expenseUnitCost2 = 400.0f;
    expensePriceCost2 = 4.1584f;
    expensePriceCost3 = 4.3583f;
    expenseUnitSolar = 450.0f;
    expenseFt = 0.3972f;
    expenseServiceFee = 38.22f;
    expenseVatRate = 7.0f;

    bool expenseLoaded = loadJsonFile("/expense.json", expenseDoc);
    if (expenseLoaded)
    {
        expenseUnitCost1 = expenseDoc["UnitCost1"] | 200.0f;
        expensePriceCost1 = expenseDoc["PriceCost1"] | 3.0000f;
        expenseUnitCost2 = expenseDoc["UnitCost2"] | 400.0f;
        expensePriceCost2 = expenseDoc["PriceCost2"] | 4.1584f;
        expensePriceCost3 = expenseDoc["PriceCost3"] | 4.3583f;
        expenseUnitSolar = expenseDoc["UnitSolar"] | 450.0f;
        expenseFt = expenseDoc["ft"] | 0.3972f;
        expenseServiceFee = expenseDoc["ServiceFee"] | 38.22f;
        expenseVatRate = expenseDoc["VatRate"] | 7.0f;
    }

    if (!isfinite(expenseUnitCost1) || expenseUnitCost1 < 0.0f) expenseUnitCost1 = 200.0f;
    if (!isfinite(expensePriceCost1) || expensePriceCost1 < 0.0f) expensePriceCost1 = 3.0000f;
    if (!isfinite(expenseUnitCost2) || expenseUnitCost2 < expenseUnitCost1) expenseUnitCost2 = 400.0f;
    if (!isfinite(expensePriceCost2) || expensePriceCost2 < 0.0f) expensePriceCost2 = 4.1584f;
    if (!isfinite(expensePriceCost3) || expensePriceCost3 < 0.0f) expensePriceCost3 = 4.3583f;
    if (!isfinite(expenseUnitSolar) || expenseUnitSolar < 0.0f) expenseUnitSolar = 450.0f;
    if (!isfinite(expenseFt)) expenseFt = 0.3972f;
    if (!isfinite(expenseServiceFee) || expenseServiceFee < 0.0f) expenseServiceFee = 38.22f;
    if (!isfinite(expenseVatRate) || expenseVatRate < 0.0f || expenseVatRate > 100.0f) expenseVatRate = 7.0f;

    updateExpenseTotals();
    Serial.println(expenseLoaded ? F("📂 Expense settings loaded") : F("⚠️ Using default expense settings"));

    return expenseLoaded;
}

// บันทึกเฉพาะโหมด WiFi (เช่น เมื่อกดเปลี่ยนโหมดผ่านปุ่ม IO0)
bool saveWifiModeSetting()
{
    JsonDocument doc;

    // โหลดไฟล์เดิมมาก่อนเพื่อไม่ให้ค่าอื่นๆ หาย (ถ้ามีข้อมูลอื่นในไฟล์นั้น)
    loadJsonFile("/networkconfig.json", doc);

    doc["wifi_mode"] = isWifiApMode;

    return saveJsonFile("/networkconfig.json", doc);
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

// For Energy management

// ======================================================
// 🔹 ฟังก์ชันโหลดค่า energy จาก LittleFS
// ======================================================
bool loadEnergyFromFile()
{
    if (!loadAllSettings())
    {
        Serial.println("⚠️ Using default gridOpr = 0 (manual mode)");
    }
    if (!loadExpenseSettings())
    {
        Serial.println("⚠️ Using default expense settings");
    }

    Serial.println("📂 Loading energy data...");

    if (!LittleFS.exists(energyFile))
    {
        Serial.println("⚠️ No previous energy.json found, setting defaults to 0.0");
        energy_kWh = 0.0;
        energy_m_kWh = 0.0;
        solar_kWh = 0.0;
        solar_m_kWh = 0.0;
        return false;
    }

    File file = LittleFS.open(energyFile, "r");
    if (!file)
    {
        Serial.println("❌ Failed to open energy.json for reading");
        energy_kWh = 0.0;
        energy_m_kWh = 0.0;
        solar_kWh = 0.0;
        solar_m_kWh = 0.0;
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error)
    {
        Serial.printf("❌ Failed to parse energy.json: %s\n", error.c_str());
        energy_kWh = 0.0;
        energy_m_kWh = 0.0;
        solar_kWh = 0.0;
        solar_m_kWh = 0.0;
        return false;
    }

    energy_kWh   = doc["energy_kWh"]   | 0.0;
    energy_m_kWh = doc["energy_m_kWh"] | 0.0;
    solar_kWh    = doc["solar_kWh"]    | 0.0;
    solar_m_kWh  = doc["solar_m_kWh"]  | 0.0;

    if (isnan(energy_kWh)) energy_kWh = 0.0;
    if (isnan(energy_m_kWh)) energy_m_kWh = 0.0;
    if (isnan(solar_kWh)) solar_kWh = 0.0;
    if (isnan(solar_m_kWh)) solar_m_kWh = 0.0;

    // Refresh monthly financial and environmental benefits using restored counters.
    updateExpenseTotals();
    updateSelfSufficiency();

    Serial.printf("✅ Loaded energy: Grid(daily)=%.4f, Grid(monthly)=%.4f, Solar(daily)=%.4f, Solar(monthly)=%.4f kWh\n",
                  energy_kWh, energy_m_kWh, solar_kWh, solar_m_kWh);
    return true;
}

// ======================================================
// 🔹 ฟังก์ชันบันทึก energy ลงใน LittleFS
// ======================================================
bool saveEnergyToFile()
{
    Serial.printf("💾 Saving energy counters to file...\n");

    File file = LittleFS.open(energyFile, "w");
    if (!file)
    {
        Serial.println("❌ Failed to open energy.json for writing");
        return false;
    }

    JsonDocument doc;
    doc["energy_kWh"]   = energy_kWh;
    doc["energy_m_kWh"] = energy_m_kWh;
    doc["solar_kWh"]    = solar_kWh;
    doc["solar_m_kWh"]  = solar_m_kWh;

    size_t written = serializeJson(doc, file);
    file.flush();
    file.close();
    vTaskDelay(pdMS_TO_TICKS(50)); // รอ Flash เขียนจริง

    if (written == 0)
    {
        Serial.println("❌ Failed to serialize JSON to file");
        return false;
    }

    // ตรวจสอบจากไฟล์จริงหลังบันทึก
    float verifyGrid = 0.0;
    File verifyFile = LittleFS.open(energyFile, "r");
    if (verifyFile)
    {
        JsonDocument verifyDoc;
        if (deserializeJson(verifyDoc, verifyFile) == DeserializationError::Ok)
        {
            verifyGrid = verifyDoc["energy_kWh"] | -1.0;
        }
        verifyFile.close();
    }

    if (fabs(verifyGrid - energy_kWh) > 0.0001)
    {
        Serial.printf("⚠️ Warning: Mismatch detected (expected %.4f, got %.4f)\n", energy_kWh, verifyGrid);
        return false;
    }

    Serial.printf("✅ File write verified: Grid=%.4f, Solar=%.4f kWh saved successfully\n", energy_kWh, solar_kWh);
    return true;
}

// ======================================================
// 🔹 ฟังก์ชันเคลียร์ค่า energy และรีเซ็ตไฟล์
// ======================================================
bool clearDailyEnergyCounters()
{
    energy_kWh = 0.0;
    solar_kWh = 0.0;
    return saveEnergyToFile();
}

bool clearEnergyFile()
{
    Serial.println("🧹 Clearing energy.json file...");

    if (LittleFS.exists(energyFile))
    {
        if (!LittleFS.remove(energyFile))
        {
            Serial.println("❌ Failed to remove old energy.json");
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    energy_kWh   = 0.0;
    energy_m_kWh = 0.0;
    solar_kWh    = 0.0;
    solar_m_kWh  = 0.0;

    if (!saveEnergyToFile())
    {
        Serial.println("⚠️ Warning: Failed to create new cleared file, retrying...");
        vTaskDelay(pdMS_TO_TICKS(200)); 
        if (!saveEnergyToFile())
        {
            Serial.println("❌ Retry failed: energy reset failed");
            return false;
        }
    }

    if (!loadEnergyFromFile())
    {
        Serial.println("⚠️ Warning: Reload after clear failed");
        return false;
    }
    if (energy_kWh == 0.0 && solar_kWh == 0.0)
    {
        Serial.println("✅ All energy counters cleared successfully (0.0 kWh)");
        return true;
    }
    else
    {
        Serial.printf("⚠️ Warning: energy reset failed\n");
        return false;
    }
}
