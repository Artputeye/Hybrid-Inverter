//ota_update.h
#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H
#include "config.h"

// ฟังก์ชันหลักสำหรับ Setup ทั้งหมด
void setupOTAManagement();

// Internal Handlers (แยกออกมาให้เป็นระเบียบ)
void handleFirmwareUpload();
void handleFileSystemUpload();
void handleFileList();
void handleFileDelete();

#endif