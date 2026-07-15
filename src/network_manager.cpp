// network_manager.cpp
#include "network_manager.h"

const unsigned long HOLD_MS = 5000;

// State variables
static unsigned long pressStart = 0;
static bool pressed = false;
static unsigned long lastChangeTime = 0;

void APmode_Check()
{
    bool isPressed = (digitalRead(AP_PIN) == LOW);

    if (isPressed && !pressed)
    {
        pressed = true;
        pressStart = millis();
    }
    else if (!isPressed && pressed)
    {
        pressed = false;
    }

    if (pressed && (millis() - pressStart >= HOLD_MS))
    {
        Serial.println(F("⚠️ Long press detected! Switching to AP Mode..."));
        isWifiApMode = 0;      // Set to AP
        saveWifiModeSetting(); // สมมติว่ามี function นี้ใน globals
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP.restart();
    }
}

void wifi_Setup()
{
    WiFi.onEvent(WiFiEvent);
    readNetworkConfig();

    if (isWifiApMode == 1)
    { // STA Mode
        setupIPConfig();
    }

    setupWiFiMode();
    lastChangeTime = millis();
}

void mac_config()
{
    uint8_t mac[6];
    WiFi.macAddress(mac); // ดึงค่า MAC Address มาเก็บในอาเรย์ mac

    // --- ส่วนที่เพิ่มสำหรับ Serial Print ---
    Serial.print("Device MAC Address: ");
    for (int i = 0; i < 6; i++)
    {
        Serial.printf("%02X", mac[i]);
        if (i < 5)
            Serial.print(":");
    }
    Serial.println();
    // ------------------------------------

    // กำหนด Unique ID ให้ HADevice
    device.setUniqueId(mac, sizeof(mac));

    // สร้าง String สำหรับเก็บ MAC Address ในรูปแบบข้อความ
    char tempMac[18];
    snprintf(tempMac, sizeof(tempMac), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    MacAddr = String(tempMac); // เก็บลงตัวแปร Global String
}

bool parseMacAddress(const char *macStr, uint8_t mac[6])
{
    if (!macStr)
        return false;

    int values[6];

    if (sscanf(macStr, "%x:%x:%x:%x:%x:%x",
               &values[0], &values[1], &values[2],
               &values[3], &values[4], &values[5]) != 6)
    {
        return false;
    }

    for (int i = 0; i < 6; i++)
        mac[i] = (uint8_t)values[i];

    return true;
}

void readNetworkConfig()
{
    Serial.println(F("\n--- [FS] Starting Network Config Load ---"));

    if (!LittleFS.begin(false))
    {
        Serial.println(F("❌ FS Error: LittleFS not mounted. Check setup()!"));
        return;
    }

    if (!LittleFS.exists("/networkconfig.json"))
    {
        Serial.println(F("❌ FS Error: /networkconfig.json NOT FOUND on Flash"));

        File root = LittleFS.open("/");
        File file = root.openNextFile();
        Serial.println(F("Files currently on FS:"));
        while (file)
        {
            Serial.printf("  - %s (%d bytes)\n", file.name(), file.size());
            file = root.openNextFile();
        }
        return;
    }

    File file = LittleFS.open("/networkconfig.json", "r");
    if (!file)
    {
        Serial.println(F("❌ FS Error: Could not open file for reading"));
        return;
    }

    Serial.printf("📂 File opened. Size: %d bytes\n", file.size());

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error)
    {
        Serial.printf("❌ JSON Error: %s\n", error.c_str());
        return;
    }

    isWifiApMode = atoi(doc["wifi_mode"] | "0");
    isIpConfigStatic = atoi(doc["ip_config"] | "0");

    strlcpy(WIFI_SSID, doc["wifi_ssid"] | "NONE", sizeof(WIFI_SSID));
    strlcpy(WIFI_PASS, doc["wifi_pass"] | "", sizeof(WIFI_PASS));
    strlcpy(MAC_RECEIVE, doc["mac_receive"] | "", sizeof(MAC_RECEIVE));

    strlcpy(IP_ADDR, doc["ip_address"] | "", sizeof(IP_ADDR));
    strlcpy(SUBNET_MASK, doc["subnet_mask"] | "", sizeof(SUBNET_MASK));
    strlcpy(GATEWAY, doc["gateway"] | "", sizeof(GATEWAY));

    strlcpy(MQTT_ADDR, doc["mqtt_server"] | "", sizeof(MQTT_ADDR));
    strlcpy(MQTT_USER, doc["mqtt_user"] | "", sizeof(MQTT_USER));
    strlcpy(MQTT_PASS, doc["mqtt_pass"] | "", sizeof(MQTT_PASS));
    strlcpy(MQTT_PORT, doc["mqtt_port"] | "1883", sizeof(MQTT_PORT));

    // ================== DEBUG AFTER MAPPING ==================
    Serial.println(F("\n✅ Data Mapped Successfully:"));
    Serial.printf(" WIFI_SSID     : %s\n", WIFI_SSID);
    Serial.printf(" WIFI_PASS     : %s\n", WIFI_PASS);
    Serial.printf(" WIFI_MODE     : %s (%d)\n", (isWifiApMode == 1 ? "STA" : "AP"), isWifiApMode);
    Serial.printf(" IP_CONFIG     : %s (%d)\n", (isIpConfigStatic == 1 ? "STATIC" : "DHCP"), isIpConfigStatic);

    Serial.printf(" MAC_RECEIVE   : %s\n", MAC_RECEIVE);

    Serial.printf(" IP_ADDRESS    : %s\n", IP_ADDR);
    Serial.printf(" SUBNET_MASK   : %s\n", SUBNET_MASK);
    Serial.printf(" GATEWAY       : %s\n", GATEWAY);

    Serial.printf(" MQTT_SERVER   : %s\n", MQTT_ADDR);
    Serial.printf(" MQTT_USER     : %s\n", MQTT_USER);
    Serial.printf(" MQTT_PASS     : %s\n", MQTT_PASS);
    Serial.printf(" MQTT_PORT     : %s\n", MQTT_PORT);

    // ================== MAC Parse ==================
    if (parseMacAddress(MAC_RECEIVE, MAC_RX))
    {
        Serial.print(F(" MAC_RX Parsed : "));
        for (int i = 0; i < 6; i++)
        {
            Serial.printf("%02X%s", MAC_RX[i], (i < 5 ? ":" : ""));
        }
        Serial.println();
    }
    else
    {
        Serial.println(F(" ⚠️ Invalid MAC format in JSON"));
    }

    Serial.println(F("---------------------------------------\n"));
}

void setupWiFiMode()
{
    if (isWifiApMode == 0)
    { // AP MODE
        Serial.println(F("📡 Starting AP Mode..."));
        ledMode = LED_AP_MODE;
        WiFi.disconnect(true, true);
        delay(500);
        WiFi.mode(WIFI_OFF);
        delay(500);
        WiFi.mode(WIFI_AP);
        WiFi.softAP(
            DEVICE_NAME,
            "12345678",
            6,     // channel
            false, // hidden
            4      // max client
        );

        Serial.printf("AP Started = %d\n", WiFi.getMode());

        Serial.print("SSID : ");
        Serial.println(WiFi.softAPSSID());

        Serial.print("IP : ");
        Serial.println(WiFi.softAPIP());

        Serial.print("Station Num : ");
        Serial.println(WiFi.softAPgetStationNum());
    }
    else
    { // STA MODE
        Serial.printf("📡 Connecting to: %s\n", WIFI_SSID);
        ledMode = LED_DISCONNECTED;
        WiFi.mode(WIFI_STA);
        WiFi.setSleep(false);
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        mac_config();
    }
}

void setupIPConfig()
{
    if (isIpConfigStatic == 1)
    {
        Serial.println(F("🌐 Setting Static IP..."));
        IPAddress local_IP = parseIP(IP_ADDR);
        IPAddress subnet = parseIP(SUBNET_MASK);
        IPAddress gateway = parseIP(GATEWAY);

        if (!WiFi.config(local_IP, gateway, subnet))
        {
            Serial.println(F("❌ Static IP Failed"));
        }
    }
}

IPAddress parseIP(const char *ipStr)
{
    IPAddress ip;
    if (ip.fromString(ipStr))
        return ip; // ใช้ built-in method ของ Arduino จะคลีนกว่า sscanf
    return IPAddress(0, 0, 0, 0);
}

void WiFiEvent(WiFiEvent_t event)
{
    switch (event)
    {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        Serial.printf("✅ Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        snprintf(DIVICE_IP, sizeof(DIVICE_IP), "%s", WiFi.localIP().toString().c_str());
        ledMode = LED_CONNECTED;

        // NTP Sync
        configTime(7 * 3600, 0, "time.google.com", "pool.ntp.org");

        if (ntpTaskHandle == NULL)
        {
            xTaskCreatePinnedToCore(TaskNTP, "NTP", 4096, NULL, 1, &ntpTaskHandle, 1);
        }
        break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        ledMode = LED_DISCONNECTED;
        Serial.println(F("⚠️ WiFi Lost Connection"));
        WiFi.reconnect();

        break;

    default:
        break;
    }
}

void keepWiFiAlive()
{
    // ทำงานเฉพาะตอนที่ตั้งค่าเป็น STA Mode เท่านั้น
    if (isWifiApMode == 1)
    {
        static unsigned long lastReconnectCheck = 0;
        // ตรวจสอบทุกๆ 20 วินาที เพื่อไม่ให้รบกวนการทำงานอื่นมากเกินไป
        if (millis() - lastReconnectCheck > 20000)
        {
            if (WiFi.status() != WL_CONNECTED)
            {
                Serial.println(F("🔄 KeepAlive: WiFi disconnected. Reconnecting..."));
                WiFi.disconnect();
                WiFi.begin(WIFI_SSID, WIFI_PASS);
            }
            lastReconnectCheck = millis();
        }
    }
}

void showAPClients()
{
    if (isWifiApMode != 0)
        return;

    wifi_sta_list_t wifi_sta_list;
    esp_netif_sta_list_t netif_sta_list;

    if (esp_wifi_ap_get_sta_list(&wifi_sta_list) == ESP_OK)
    {
        if (esp_netif_get_sta_list(&wifi_sta_list, &netif_sta_list) == ESP_OK)
        {
            Serial.printf("👥 Clients: %d\n", netif_sta_list.num);
            for (int i = 0; i < netif_sta_list.num; i++)
            {
                esp_netif_sta_info_t station = netif_sta_list.sta[i];
                Serial.printf(" - [%d] MAC: %02X:%02X:%02X... IP: %s\n",
                              i + 1, station.mac[0], station.mac[1], station.mac[2],
                              ip4addr_ntoa((ip4_addr_t *)&station.ip.addr));
            }
        }
    }
}