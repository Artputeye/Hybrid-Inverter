
#include "logger.h"

#define LOG_FILE_PATH "/system_log.txt"
#define MAX_LOG_SIZE_BYTES 10240 // จำกัดขนาดไฟล์ที่ 10KB (เพื่อป้องกันไม่ให้ Flash เต็ม)

// ฟังก์ชันสำหรับบันทึก Log ลง LittleFS
void writeLog(const String &level, const String &message) {
    // 1. ตรวจสอบว่า LittleFS พร้อมใช้งานไหม
    if (!LittleFS.begin(true)) {
        Serial.println(F("[Logger] LittleFS not ready, can't write log."));
        return;
    }

    // 2. เช็คขนาดไฟล์ปัจจุบัน หากใหญ่เกินไปให้ลบสร้างใหม่ (ป้องกันไฟล์โตจนเต็ม)
    if (LittleFS.exists(LOG_FILE_PATH)) {
        File checkFile = LittleFS.open(LOG_FILE_PATH, "r");
        if (checkFile && checkFile.size() > MAX_LOG_SIZE_BYTES) {
            checkFile.close();
            LittleFS.remove(LOG_FILE_PATH);
            Serial.println(F("[Logger] Log file exceeded max size. Cleared."));
        } else if (checkFile) {
            checkFile.close();
        }
    }

    // 3. บันทึกข้อมูลลงไฟล์ในโหมด Append (เขียนต่อท้าย)
    File logFile = LittleFS.open(LOG_FILE_PATH, "a");
    if (!logFile) {
        Serial.println(F("[Logger] Failed to open log file for writing"));
        return;
    }

    // รูปแบบ: [TIMESTAMP/UPTIME] [LEVEL] MESSAGE
    // หากเชื่อมต่อ NTP แล้ว millis() ตรงนี้สามารถเปลี่ยนเป็นเวลาจริง (DateTime) ได้ครับ
    String logLine = "[" + String(millis() / 1000) + "s] [" + level + "] " + message + "\n";
    logFile.print(logLine);
    logFile.close();
}

// ฟังก์ชันสำหรับอ่าน Log ทั้งหมดออกมาแสดงทาง Serial
void displayLogs() {
    Serial.println(F("\n====== [ SYSTEM LOG HISTORY ] ======"));
    if (!LittleFS.exists(LOG_FILE_PATH)) {
        Serial.println(F("No log history found. System is clean."));
        Serial.println(F("====================================\n"));
        return;
    }

    File logFile = LittleFS.open(LOG_FILE_PATH, "r");
    if (!logFile) {
        Serial.println(F("❌ Failed to open log file for reading."));
        return;
    }

    while (logFile.available()) {
        Serial.write(logFile.read());
    }
    logFile.close();
    Serial.println(F("====================================\n"));
}

// ฟังก์ชันตรวจสอบและบันทึกสาเหตุการเกิด Reset ล่าสุด (รวมถึง Watchdog)
void checkAndLogResetReason() {
    RESET_REASON reason_core0 = rtc_get_reset_reason(0);
    RESET_REASON reason_core1 = rtc_get_reset_reason(1);

    // หากพบการรีสตาร์ทจาก Watchdog (ทั้งระบบหรือรายตัว CPU)
    if (reason_core0 == RTCWDT_RTC_RESET || reason_core0 == TGWDT_CPU_RESET || 
        reason_core1 == RTCWDT_RTC_RESET || reason_core1 == TGWDT_CPU_RESET) {
        
        String wdtMessage = "System crash detected! Reset by Watchdog. Core0 Reason: " + 
                            String(reason_core0) + ", Core1 Reason: " + String(reason_core1);
        
        writeLog("CRITICAL", wdtMessage);
    } 
    // หากเป็นการเปิดเครื่องใหม่ปกติ หรือกดปุ่ม Reset จะไม่บันทึกให้รกไฟล์ แต่พิมพ์บอกใน Serial พอ
    else {
        Serial.println(F("[Logger] Normal boot up sequence."));
    }
}

// ฟังก์ชันสำหรับล้างประวัติ Log ทั้งหมด (เผื่อเรียกใช้ผ่าน Web หรือ Serial)
void clearLogHistory() {
    if (LittleFS.exists(LOG_FILE_PATH)) {
        if (LittleFS.remove(LOG_FILE_PATH)) {
            Serial.println(F("✅ Clear system log successfully."));
        } else {
            Serial.println(F("❌ Failed to clear system log."));
        }
    }
}