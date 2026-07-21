// ha_integration.h
#ifndef HA_INTEGRATION_H
#define HA_INTEGRATION_H
#include "config.h"

void iotHAsetup();
void iotHAloop();
void send_ha_discovery();
void reconnect();
void publish_all_states();
void send_sensor_config(const char* object_id, const char* name, const char* unit, const char* device_class, const char* icon, const char* category);

#endif