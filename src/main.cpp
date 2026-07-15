// main.cpp
#include "config.h"

// --- Global Objects Definition ---
// ประกาศที่นี่ที่เดียวเพื่อให้ Linker หาเจอได้ง่าย
WiFiClient client;
HADevice device;
HAMqtt mqtt(client, device);

// --- Function Prototypes ---
void TaskMain(void *pvParameters);
void TaskSub(void *pvParameters);
void TaskLED(void *pvParameters);
void TaskSerialReader(void *pvParameters);

void setup()
{
    // 1. Initial HW & Debug
    pinMode(STATUS_LED, OUTPUT);
    digitalWrite(STATUS_LED, HIGH);
    pinMode(AP_PIN, INPUT_PULLUP);

    Serial.begin(115200);
    // ไม่ควรใช้ while(!Serial) นานเกินไปในโปรดักชัน เพราะถ้าไม่ต่อคอมเครื่องจะค้าง
    unsigned long startSerial = millis();
    while (!Serial && millis() - startSerial < 3000)
        ;

    Serial.print("Booting");
    for (int i = 0; i < 5; i++)
    {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial2.begin(2400, SERIAL_8N1, RX_pin, TX_pin);
    Serial.println("Serial Setup Completed");
    delay(500);

    // 2. Storage Setup
    delay(500);
    if (!LittleFS.begin(true))
    {
        Serial.println(F("❌ LittleFS Mount Failed"));
    }
    else
    {
        Serial.println(F("✅ LittleFS Mounted"));
    }

    delay(500);
    displayLogs();            // 1. ดึง Log เก่าที่เคยค้างไว้ขึ้นมาโชว์ตอนเปิดเครื่อง
    checkAndLogResetReason(); // 2. เช็คว่าเปิดเครื่องรอบนี้ เพราะรอบก่อนหน้านี้ค้างจนโดน WDT สั่งรีเซ็ตไหม

    // 3. Network & Config Setup
    delay(500);
    wifi_Setup();
    delay(500);

    // 4. Services Setup for connect the internet
    if (isWifiApMode == 1 && WiFi.status() == WL_CONNECTED)
    {
        iotHAsetup();
        NTPbegin();
    }
    delay(500);

    if (MDNS.begin(HOSTNAME))
    {
        Serial.printf("[System] mDNS Started: %s.local\n", HOSTNAME);
    }
    delay(500);
    initWebRoutes();
    delay(500);
    ws_init();
    delay(500);
    setupOTAManagement();
    delay(500);
    server.begin();
    delay(500);
    app_setup();
    delay(500);

    // 5. Watchdog Configuration (ESP32-IDF Style)
    // หมายเหตุ: หากใช้ ESP32 Core 3.x ขึ้นไป อาจต้องปรับ syntax ตามที่แจ้งในรอบก่อน
    esp_task_wdt_init(WDT_TIMEOUT, true);
    esp_task_wdt_delete(NULL); // ไม่ใช้ Loop() หลักคุม WDT

    // 6. Task Creation (แบ่งโหลดความสำคัญ)
    // Core 0: งานหลัก (Processing/Operation)
    // Core 1: งานสื่อสาร (WiFi/MQTT/Web) และ UI (LED)
    xTaskCreatePinnedToCore(TaskMain, "Main", 4096, NULL, 3, NULL, 0);
    xTaskCreatePinnedToCore(TaskSub, "Sub", 8192, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(TaskLED, "LED", 2048, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(TaskSerialReader, "SerialReader", 4096, NULL, 1, NULL, 0);

    Serial.println(F("[System] Setup Complete"));
}

void loop()
{
    // ปล่อยว่างไว้ เพราะเราใช้ TaskManagement (FreeRTOS)
    vTaskDelete(NULL);
}

// ---------------------------------------------------------
// TaskMain: งานประมวลผลหลัก (Core 0)
// ---------------------------------------------------------
void TaskMain(void *pvParameters)
{
    esp_task_wdt_add(NULL); // เพิ่มตัวมันเองลงใน Watchdog

    unsigned long lastMain = 0;

    for (;;)
    {
        esp_task_wdt_reset();

        // ฟังก์ชันคำนวณหลัก
        app_loop();

        if (millis() - lastMain > 60000)
        {
            lastMain = millis();
            // ตรวจสอบ Stack เพื่อป้องกันโปรแกรมแฮงค์ (Debug)
            // Serial.printf("[Debug] Main Stack: %u\n", uxTaskGetStackHighWaterMark(NULL));
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ---------------------------------------------------------
// TaskSub: งานสื่อสาร Network (Core 1)
// ---------------------------------------------------------
void TaskSub(void *pvParameters)
{
    esp_task_wdt_add(NULL);

    unsigned long lastStatus = 0;

    for (;;)
    {
        esp_task_wdt_reset();
        iotHArun();
        ws_process();
        APmode_Check();
        keepWiFiAlive();

        // ตรวจสอบสถานะทุก 10 วินาที
        if (millis() - lastStatus > 10000)
        {
            lastStatus = millis();
            showAPClients();
        }
        vTaskDelay(pdMS_TO_TICKS(5)); // ให้เวลาความสำคัญกับ Network สูงหน่อย
    }
}

// ---------------------------------------------------------
// TaskLED: การแสดงผลไฟสถานะ (Core 1)
// ---------------------------------------------------------
void TaskLED(void *pvParameters)
{
    // งาน LED มักไม่จำเป็นต้องลงใน WDT เพราะถ้ามันค้างระบบยังทำงานต่อได้
    for (;;)
    {
        ledPatternSelect();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// ---------------------------------------------------------
// Task สำหรับดักฟัง Serial (Core 0)
// ---------------------------------------------------------
void TaskSerialReader(void *pvParameters)
{
    for (;;)
    {
        if (Serial.available() > 0)
        {
            // ลองใช้ Serial.readString() แบบไม่มี Until เพื่อดูว่ามีอะไรเข้ามาบ้าง
            String receivedData = Serial.readString();
            receivedData.trim(); // ตัด \r \n ทิ้งเอง

            Serial.print("Received: "); // Debug ดูว่ามันเห็นอะไร
            Serial.println(receivedData);

            processSerialCommand(receivedData);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
