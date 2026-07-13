// serial_handler.cpp
#include "serial_handler.h"

void processSerialCommand(String input)
{
    input.trim();
    lastSerialMsg = input;
    String upperInput = input;
    upperInput.toUpperCase();

    if (upperInput.startsWith("SET:"))
    {
        String payload = upperInput.substring(4); // ตัด "SET:" ออก เหลือ "RELAY1:ON"
        int firstColon = payload.indexOf(':');

        if (firstColon != -1)
        {
            String key = payload.substring(0, firstColon);  // "RELAY1"
            String val = payload.substring(firstColon + 1); // "ON"

            // serialRelay(key, val);

            // Debug ดูค่าที่แยกได้
            Serial.print("Command Key: ");
            Serial.println(key);
            Serial.print("Command Val: ");
            Serial.println(val);
        }
    }
    else if (input.length() > 0)
    {
        inv.executeCommand(input);
        Serial.println(">>> [INV] Command format to inverter.");
    }
}
