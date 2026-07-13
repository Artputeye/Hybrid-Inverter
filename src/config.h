//config.h
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <ArduinoHA.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <Update.h>
#include "base64.h"
#include <esp_wifi.h>
#include <esp_task_wdt.h>
#include <time.h>
#include <esp_now.h>
#include "inv_command.h"
//#include "driver/rmt.h"
#include "rom/rtc.h" 

// --- System Definitions ---
#define WDT_TIMEOUT 120
#define STATUS_LED 2 //Pin 2 for ESP32 Devkit/Pin 23 for ESP32 Relay 4CH
#define AP_PIN 0

#define FILESYSTEM LittleFS

// --- Inverter command Objects (Extern) ---
extern inv_command inv;

// --- Global Objects (Extern) ---
extern AsyncWebServer server;
extern HADevice device;
extern HAMqtt mqtt;
extern File fsUploadFile;

// --- Device Info ---
extern char D_SoftwareVersion[15];
extern char D_Mfac[15];
extern char D_Model[15];

// --- Network Settings ---
extern char DEVICE_NAME[28];
extern char DEVICE_PASS[28];
extern char WIFI_SSID[30];
extern char WIFI_PASS[25];
extern char HOSTNAME[30];

// --- Static IP Settings ---
extern char DIVICE_IP[16];
extern char IP_ADDR[16];
extern char SUBNET_MASK[16];
extern char GATEWAY[16];

// --- MQTT Settings ---
extern char MQTT_ADDR[16];
extern char MQTT_USER[28];
extern char MQTT_PASS[28];
extern char MQTT_PORT[6];

// --- MAC : ADDRESS ---
extern char MAC_RECEIVE[18];
extern uint8_t Mac[6];
extern uint8_t MAC_RX[6];
extern String MacAddr; // เพิ่มตัวแปรนี้ไว้เก็บ Mac ในรูปแบบ String จะได้ใช้สะดวก

// --- Auth & Status ---
extern char AUTH_USER[10];
extern char AUTH_PASS[10];

extern bool isWifiApMode;
extern bool isIpConfigStatic;
extern String serialData;
extern String lastSerialMsg;



// --- Project Sub-Programs (Include หลังการประกาศ extern เสมอ) ---
#include "app_main.h"
#include "ha_integration.h"
#include "http_server.h"
#include "inv_control.h"
#include "logger.h"
#include "network_manager.h"
#include "ota_update.h"
#include "serial_handler.h"
#include "simulate.h"
#include "storage_manager.h"
#include "time_sync.h"
#include "ui_indicator.h"
#include "websocket_handler.h"







