//ota_update.CPP
#include "ota_update.h"

void setupOTAManagement() {
    handleFirmwareUpload();
    handleFileSystemUpload();
    handleFileList();
    handleFileDelete();
}

// --- 1. Firmware & Partition Update ---
void handleFirmwareUpload() {
    server.on("/otafirmware", HTTP_POST, 
        [](AsyncWebServerRequest *request) {
            bool success = !Update.hasError();
            if (success) {
                request->send(200, "text/plain", "OK");
                delay(1000);
                ESP.restart();
            } else {
                request->send(500, "text/plain", "Update Failed");
            }
        },
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            if (index == 0) {
                ledMode = LED_BUSY;
                Serial.printf("[OTA] Start: %s\n", filename.c_str());
                
                // ตรวจสอบพื้นที่ว่าง (U_FLASH สำหรับ Firmware, U_SPIFFS สำหรับ LittleFS)
                if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
                    Update.printError(Serial);
                }
            }

            if (!Update.hasError()) {
                if (Update.write(data, len) != len) {
                    Update.printError(Serial);
                }
            }

            if (final) {
                if (Update.end(true)) {
                    Serial.printf("[OTA] Success: %u bytes\n", index + len);
                } else {
                    Update.printError(Serial);
                }
            }
        }
    );
}

// --- 2. Upload File to LittleFS ---
void handleFileSystemUpload() {
    server.on("/otalittlefs", HTTP_POST, 
        [](AsyncWebServerRequest *request) { request->send(200); },
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            if (!filename.startsWith("/")) filename = "/" + filename;

            if (index == 0) {
                // ตรวจสอบและสร้าง Directory อัตโนมัติ
                String path = filename;
                int lastSlash = path.lastIndexOf('/');
                if (lastSlash > 0) {
                    String dirPath = path.substring(0, lastSlash);
                    if (!FILESYSTEM.exists(dirPath)) {
                        FILESYSTEM.mkdir(dirPath);
                    }
                }
                
                Serial.printf("[FS] Uploading: %s\n", filename.c_str());
                fsUploadFile = FILESYSTEM.open(filename, "w");
            }

            if (fsUploadFile) {
                fsUploadFile.write(data, len);
            }

            if (final) {
                if (fsUploadFile) fsUploadFile.close();
                Serial.printf("[FS] Upload Complete: %u bytes\n", index + len);
            }
        }
    );
}

// --- 3. List Files (JSON) ---
void handleFileList() {
    server.on("/list", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("dir")) {
            request->send(400, "text/plain", "Missing 'dir' param");
            return;
        }

        String path = request->getParam("dir")->value();
        File root = FILESYSTEM.open(path);
        
        String output = "[";
        if (root && root.isDirectory()) {
            File file = root.openNextFile();
            while (file) {
                if (output != "[") output += ",";
                
                output += "{\"type\":\"";
                output += (file.isDirectory() ? "dir" : "file");
                output += "\",\"name\":\"";
                output += String(file.path());
                output += "\",\"size\":";
                output += String(file.size());
                output += "}";
                
                file = root.openNextFile();
            }
        }
        output += "]";
        request->send(200, "application/json", output);
    });
}

// --- 4. Delete File ---
void handleFileDelete() {
    server.on("/delete", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("file")) {
            request->send(400, "text/plain", "Missing 'file' param");
            return;
        }

        String path = request->getParam("file")->value();
        
        // ป้องกันการลบ Root หรือไฟล์ที่ไม่มีอยู่จริง
        if (path == "/" || !FILESYSTEM.exists(path)) {
            request->send(404, "text/plain", "Invalid Path");
            return;
        }

        if (FILESYSTEM.remove(path)) {
            Serial.printf("[FS] Deleted: %s\n", path.c_str());
            request->send(200, "text/plain", "Deleted");
        } else {
            request->send(500, "text/plain", "Delete Failed");
        }
    });
}