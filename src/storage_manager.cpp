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
// 🔹 ฟังก์ชันโหลดค่า energy_kWh จาก LittleFS
// ======================================================
bool loadEnergyFromFile()
{
    if (!loadAllSettings())
    {
        Serial.println("⚠️ Using default gridOpr = 0 (manual mode)");
    }

    Serial.println("📂 Loading energy data...");

    if (!LittleFS.exists(energyFile))
    {
        Serial.println("⚠️ No previous energy.json found, setting energy_kWh = 0.0");
        energy_kWh = 0.0;
        return false;
    }

    File file = LittleFS.open(energyFile, "r");
    if (!file)
    {
        Serial.println("❌ Failed to open energy.json for reading");
        energy_kWh = 0.0;
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error)
    {
        Serial.printf("❌ Failed to parse energy.json: %s\n", error.c_str());
        energy_kWh = 0.0;
        return false;
    }

    energy_kWh = doc["energy_kWh"] | 0.0;

    if (isnan(energy_kWh))
    {
        Serial.println("⚠️ Warning: Loaded value is NaN, resetting to 0.0");
        energy_kWh = 0.0;
        return false;
    }

    Serial.printf("✅ Loaded energy_kWh = %.4f kWh from %s\n", energy_kWh, energyFile.c_str());
    return true;
}

// ======================================================
// 🔹 ฟังก์ชันบันทึก energy_kWh ลงใน LittleFS
// ======================================================
bool saveEnergyToFile()
{
    Serial.printf("💾 Saving energy_kWh = %.4f to file...\n", energy_kWh);

    File file = LittleFS.open(energyFile, "w");
    if (!file)
    {
        Serial.println("❌ Failed to open energy.json for writing");
        return false;
    }

    JsonDocument doc;
    doc["energy_kWh"] = energy_kWh;

    size_t written = serializeJson(doc, file);
    file.flush();
    file.close();
    delay(50); // รอ Flash เขียนจริง

    if (written == 0)
    {
        Serial.println("❌ Failed to serialize JSON to file");
        return false;
    }

    // ตรวจสอบจากไฟล์จริงหลังบันทึก
    float verifyValue = 0.0;
    File verifyFile = LittleFS.open(energyFile, "r");
    if (verifyFile)
    {
        JsonDocument verifyDoc;
        if (deserializeJson(verifyDoc, verifyFile) == DeserializationError::Ok)
        {
            verifyValue = verifyDoc["energy_kWh"] | -1.0;
        }
        verifyFile.close();
    }

    if (fabs(verifyValue - energy_kWh) > 0.0001)
    {
        Serial.printf("⚠️ Warning: Mismatch detected (expected %.4f, got %.4f)\n", energy_kWh, verifyValue);
        return false;
    }

    Serial.printf("✅ File write verified: %.4f kWh saved successfully\n", energy_kWh);
    return true;
}

// ======================================================
// 🔹 ฟังก์ชันเคลียร์ค่า energy_kWh และรีเซ็ตไฟล์
// ======================================================
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
        delay(50);
    }

    energy_kWh = 0.0;

    if (!saveEnergyToFile())
    {
        Serial.println("⚠️ Warning: Failed to create new cleared file, retrying...");
        delay(200);
        if (!saveEnergyToFile())
        {
            Serial.println("❌ Retry failed: energy_kWh reset failed");
            return false;
        }
    }

    if (!loadEnergyFromFile())
    {
        Serial.println("⚠️ Warning: Reload after clear failed");
        return false;
    }
    if (energy_kWh == 0.0)
    {
        Serial.println("✅ energy_kWh cleared successfully (0.0 kWh)");
        return true;
    }
    else
    {
        Serial.printf("⚠️ Warning: energy_kWh reset failed (%.4f != 0.0)\n", energy_kWh);
        return false;
    }
}
