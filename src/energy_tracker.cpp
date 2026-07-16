#include "energy_tracker.h"

// 1. นิยาม Array สำหรับเก็บประวัติ
float hourlyEnergy[24] = {0};
float dailyEnergy[30] = {0};

static float lastHourEnergySnapshot = 0;
static float lastDayEnergySnapshot = 0;
static int secondCounter = 0;
static int hourCounter = 0;
static bool isFirstRun = true;

// พาธสำหรับเก็บไฟล์ JSON ใน LittleFS
const char *jsonFilePath = "/energy_history.json";

// ฟังก์ชันสำหรับเลื่อนข้อมูลใน Array (FIFO)
static void updateHistoryArray(float *array, int size, float newValue)
{
    for (int i = size - 1; i > 0; i--)
    {
        array[i] = array[i - 1];
    }
    array[0] = newValue;
}

// 💾 ฟังก์ชัน: ส่งค่าเป็น JSON ไปบันทึกใน LittleFS
bool saveEnergyToJson()
{
    // จองพื้นที่สำหรับจัดรูปแบบ JSON (คำนวณจากขนาด Array 24 + 30 ช่อง)
    JsonDocument doc;

    // สร้าง Array ของประวัติชั่วโมงลง JSON
    JsonArray hourlyJsonArray = doc["hourly"].to<JsonArray>();
    for (int i = 0; i < 24; i++)
    {
        hourlyJsonArray.add(hourlyEnergy[i]);
    }

    // สร้าง Array ของประวัติวันลง JSON
    JsonArray dailyJsonArray = doc["daily"].to<JsonArray>();
    for (int i = 0; i < 30; i++)
    {
        dailyJsonArray.add(dailyEnergy[i]);
    }

    // บันทึกค่า Snapshot และสถานะตัวจับเวลาไปด้วย เพื่อไม่ให้เวลาเคลื่อนตอนดับ
    doc["last_hour_snapshot"] = lastHourEnergySnapshot;
    doc["last_day_snapshot"] = lastDayEnergySnapshot;
    doc["second_counter"] = secondCounter;
    doc["hour_counter"] = hourCounter;

    // เปิดไฟล์ในระบบ LittleFS เพื่อเขียนทับ (Write Mode)
    File file = LittleFS.open(jsonFilePath, FILE_WRITE);
    if (!file)
    {
        Serial.println("❌ [FS] Failed to open file for writing");
        return false;
    }

    // แปลงออบเจกต์เป็นข้อความ JSON ลงไฟล์
    if (serializeJson(doc, file) == 0)
    {
        Serial.println("❌ [FS] Failed to write JSON to file");
        file.close();
        return false;
    }

    file.close();
    Serial.println("💾 [FS] Energy history saved successfully to JSON!");
    return true;
}

// 📖 ฟังก์ชัน: ดึงค่าจากไฟล์ JSON มาใช้งาน
bool loadEnergyFromJson()
{
    // ตรวจสอบว่ามีไฟล์อยู่จริงไหม
    if (!LittleFS.exists(jsonFilePath))
    {
        Serial.println("ℹ️ [FS] No history file found. Starting with empty arrays.");
        return false;
    }

    File file = LittleFS.open(jsonFilePath, FILE_READ);
    if (!file)
    {
        Serial.println("❌ [FS] Failed to open file for reading");
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error)
    {
        Serial.print("❌ [FS] JSON Deserialization failed: ");
        Serial.println(error.c_str());
        return false;
    }

    // ถอดค่ากลังลง Array ชั่วโมง
    JsonArray hourlyJsonArray = doc["hourly"];
    for (int i = 0; i < 24 && i < hourlyJsonArray.size(); i++)
    {
        hourlyEnergy[i] = hourlyJsonArray[i];
    }

    // ถอดค่ากลับลง Array วัน
    JsonArray dailyJsonArray = doc["daily"];
    for (int i = 0; i < 30 && i < dailyJsonArray.size(); i++)
    {
        dailyEnergy[i] = dailyJsonArray[i];
    }

    // คืนค่าสถานะตัวจับเวลากลับมาทำต่อ
    lastHourEnergySnapshot = doc["last_hour_snapshot"] | 0.0f;
    lastDayEnergySnapshot = doc["last_day_snapshot"] | 0.0f;
    secondCounter = doc["second_counter"] | 0;
    hourCounter = doc["hour_counter"] | 0;
    isFirstRun = false; // ข้ามการยึดค่าใหม่ตอนเปิดเครื่อง เพราะดึงค่าเดิมกลับมาแล้ว

    Serial.println("📖 [FS] Energy history successfully restored from JSON!");
    return true;
}

// FreeRTOS Task จัดการบันทึกพลังงาน
static void energyTrackerTask(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(5000);
    float previousEnergy = 0;

    while (1)
    {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        float currentTotal = energy_kWh;

        if (isFirstRun)
        {
            lastHourEnergySnapshot = currentTotal;
            lastDayEnergySnapshot = currentTotal;
            previousEnergy = currentTotal;
            isFirstRun = false;
            Serial.printf("[ENERGY] Initialized Snapshot: %.4f kWh\n", currentTotal);
            continue;
        }

        // ดักจับระบบรีเซ็ตพลังงานสะสม (ตอน 18:00 น.)
        if (currentTotal < previousEnergy)
        {
            Serial.println("\n⚠️ [ENERGY] Detected main energy_kWh reset!");
            lastHourEnergySnapshot = lastHourEnergySnapshot - previousEnergy;
            lastDayEnergySnapshot = lastDayEnergySnapshot - previousEnergy;
        }

        previousEnergy = currentTotal;
        secondCounter += 5;

        // 1. ครบ 1 ชั่วโมง
        if (secondCounter >= 3600)
        {
            float hourDelta = currentTotal - lastHourEnergySnapshot;
            if (hourDelta < 0)
                hourDelta = 0;

            updateHistoryArray(hourlyEnergy, 24, hourDelta);
            lastHourEnergySnapshot = currentTotal;
            secondCounter = 0;
            hourCounter++;

            Serial.printf(">>> [ENERGY] 1 Hour passed. Consumed: %.4f kWh\n", hourDelta);

            // 💾 เซฟประวัติลง LittleFS อัตโนมัติเมื่อข้อมูลมีการเปลี่ยนแปลง
            saveEnergyToJson();
        }

        // 2. ครบ 1 วัน (24 ชั่วโมง)
        if (hourCounter >= 24)
        {
            float dayDelta = currentTotal - lastDayEnergySnapshot;
            if (dayDelta < 0)
                dayDelta = 0;

            updateHistoryArray(dailyEnergy, 30, dayDelta);
            lastDayEnergySnapshot = currentTotal;
            hourCounter = 0;

            Serial.printf(">>>>>> [ENERGY] 1 Day passed. Consumed: %.4f kWh\n", dayDelta);

            // 💾 เซฟประวัติลง LittleFS อัตโนมัติเมื่อข้อมูลมีการเปลี่ยนแปลง
            saveEnergyToJson();
        }
    }
}

/**
 * @brief ฟังก์ชันลงทะเบียน API สำหรับจัดการไฟล์เดี่ยวตามที่กำหนด
 * @param filename ชื่อไฟล์ใน LittleFS (เช่น "/networkconfig.json")
 * @param mode โหมดการทำงาน: 
 *             "r"  = สร้างเฉพาะ API สำหรับดึงข้อมูล (GET)
 *             "w"  = สร้างเฉพาะ API สำหรับบันทึกข้อมูล (POST)
 *             "rw" = สร้างทั้งคู่ (ทั้งดึงข้อมูลและบันทึกข้อมูล)
 */
void setupFileAPI(String filename, String mode) 
{
  // จัดการรูปแบบชื่อไฟล์: ปลายทาง URL จะถอดเครื่องหมาย '/' ออก เพื่อให้เรียกง่ายขึ้น
  // เช่น จากไฟล์ "/networkconfig.json" จะได้ Endpoint URL เป็น "/networkconfig.json"
  String urlPath = filename;
  if (urlPath.startsWith("/")) {
    urlPath = urlPath.substring(1); 
  }
  urlPath = "/" + urlPath; // ตรวจสอบให้มั่นใจว่า URL เริ่มต้นด้วย /

  // ==========================================
  // ส่วนที่ 1: ลงทะเบียน API สำหรับดึงข้อมูล [GET]
  // ==========================================
  if (mode == "r" || mode == "rw") {
    server.on(urlPath.c_str(), HTTP_GET, [filename](AsyncWebServerRequest *request) 
    {
      if (!LittleFS.exists(filename)) {
        request->send(404, "application/json", "{\"error\":\"File '" + filename + "' not found\"}");
        return;
      }

      File file = LittleFS.open(filename, "r");
      if (!file) {
        request->send(500, "application/json", "{\"error\":\"Failed to open file for reading\"}");
        return;
      }

      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, file);
      file.close();

      if (error) {
        request->send(500, "application/json", "{\"error\":\"Failed to parse JSON\"}");
        return;
      }

      String jsonResponse;
      serializeJson(doc, jsonResponse);
      Serial.printf("GET %s : %s\n", filename.c_str(), jsonResponse.c_str());

      request->send(200, "application/json", jsonResponse);
    });
  }

  // ==========================================
  // ส่วนที่ 2: ลงทะเบียน API สำหรับบันทึกข้อมูล [POST]
  // ==========================================
  if (mode == "w" || mode == "rw") {
    server.on(urlPath.c_str(), HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, 
    [filename](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) 
    {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data, len);
      if (error) {
        Serial.println("JSON parse failed!");
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
      }

      // เปิดไฟล์เพื่อเขียนทับ
      File file = LittleFS.open(filename, "w");
      if (!file) {
        request->send(500, "application/json", "{\"error\":\"Failed to open file for writing\"}");
        return;
      }

      // บันทึกข้อมูลลงในไฟล์
      if (serializeJson(doc, file) == 0) {
        request->send(500, "application/json", "{\"error\":\"Failed to write JSON\"}");
        file.close();
        return;
      }
      
      file.close();
      
      Serial.printf("POST %s : Saved successfully\n", filename.c_str());
      request->send(200, "application/json", "{\"status\":\"success\"}");
    });
  }
}


// ฟังก์ชันเริ่มต้นระบบ
void initEnergyTracker()
{
    // ตรวจสอบความปลอดภัยและเปิดใช้งาน LittleFS ก่อน
    if (!LittleFS.begin(true))
    {
        Serial.println("❌ [FS] LittleFS Mount Failed. (Format applied if true)");
    }
    else
    {
        // 📖 โหลดข้อมูลประวัติเก่ากลับคืนมาก่อนเริ่ม Task ใหม่
        loadEnergyFromJson();
    }

    xTaskCreatePinnedToCore(
        energyTrackerTask,
        "EnergyTracker",
        4096,
        NULL,
        1,
        NULL,
        1);
}