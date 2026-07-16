// network_manager.H
#ifndef NETWORK_MANAGR_H
#define NETWORK_MANAGR_H
#include "config.h"

// Functions
void network_setup();
void apModeCheck(); // เปลี่ยนชื่อให้สื่อความหมายว่าเป็นการเช็คปุ่ม
void mac_config();
void readNetworkConfig();
void setupWiFiMode();
void setupIPConfig();
void keepWiFiAlive();
void showAPClients();
void WiFiEvent(WiFiEvent_t event);
IPAddress parseIP(const char *ipStr);

#endif