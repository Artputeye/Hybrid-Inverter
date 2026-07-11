//ui_indicator.cpp
#include "ui_indicator.h"

volatile LedMode ledMode = LED_DISCONNECTED; 

void ledIndicator(unsigned long onTime, unsigned long offTime)
{
    static bool ledState = false;
    static unsigned long previousMillis = 0;

    unsigned long currentMillis = millis();
    unsigned long interval = ledState ? onTime : offTime;

    if (currentMillis - previousMillis >= interval)
    {
        previousMillis = currentMillis;
        ledState = !ledState;
        digitalWrite(STATUS_LED, ledState ? HIGH : LOW);
    }
}

void ledPatternSelect()
{
    switch (ledMode)
    {

        case LED_CONNECTED:
            ledIndicator(100, 2000); // short blink every 2 sec
            break;

        case LED_DISCONNECTED:
            ledIndicator(100, 100);  // slow blink
            break;
        
        case LED_AP_MODE:
            ledIndicator(1000, 1000);  // AP blink
            break;

        case LED_FAULT:
            ledIndicator(500, 500);  // steady blink
            break;

        case LED_BUSY:
            ledIndicator(200, 200);   // medium blink
            break;
    }
}

