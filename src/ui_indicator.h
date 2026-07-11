//ui_indicator.h
#ifndef UI_INDICATOR_H
#define UI_INDICATOR_H
#include "config.h"

enum LedMode {
    LED_OFF,
    LED_ON,
    LED_CONNECTED,
    LED_DISCONNECTED,
    LED_AP_MODE,  
    LED_MQTT_FAIL,
    LED_OTA_RUNNING,
    LED_FAULT,
    LED_BUSY
};
extern volatile LedMode ledMode;

void ledIndicator(unsigned long onTime, unsigned long offTime) ;
void ledPatternSelect();

#endif