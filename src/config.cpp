//config.cpp
#include "config.h"

// --- Global Objects ---
AsyncWebServer server(80);
File fsUploadFile;
invHybrid inv;

// --- Device Info ---
char D_SoftwareVersion[15] = "1.2.8"; //1.2.8
char D_Mfac[15] = "ARTTECH"; //ARTTECH
char D_Model[15] = "IoT Solar"; //IoT Solar

// --- Network Settings ---
char DEVICE_NAME[28] = "Hybrid Inverter"; //Hybrid Inverter
char DEVICE_PASS[28] = "12345678"; //12345678
char WIFI_SSID[30] = "";
char WIFI_PASS[25] = "";
char HOSTNAME[30] = "inverter"; //inverter

// --- Static IP Settings ---
char DIVICE_IP[16];
char IP_ADDR[16] = "0.0.0.0"; //0.0.0.0
char SUBNET_MASK[16] = "255.255.255.0"; //255.255.255.0
char GATEWAY[16] = "0.0.0.0"; //0.0.0.0

// --- MQTT Settings ---
char MQTT_SERVER[16] = "192.168.1.100"; //192.168.1.100
char MQTT_USER[28] = "inverter"; //inverter 
char MQTT_PASS[28] = "12345678"; //12345678
uint16_t MQTT_PORT = 1883; //1883

// --- MAC : ADDRESS ---
char MAC_RECEIVE[18];
uint8_t Mac[6];
uint8_t MAC_RX[6];
String MacAddr;

// --- Auth & Status ---
char AUTH_USER[10] = "admin"; //admin
char AUTH_PASS[10] = "12345678"; //12345678

bool isWifiApMode = false;
bool isIpConfigStatic = false;
String serialData = "";
String lastSerialMsg = ""; //ค่าที่เข้ามาจาก Serial Input

