#include "ha_integration.h"

const char *device_id = "esp32_hybrid_inverter"; // Unique ID for the device
const char *discovery_prefix = "homeassistant";
const char *availability_topic = "homeassistant/sensor/hybrid_inverter/availability";
const char *state_topic = "homeassistant/sensor/hybrid_inverter/state";

unsigned long lastMsg = 0;

void iotHAsetup()
{
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setBufferSize(4096);
  client.setKeepAlive(60);
  client.setSocketTimeout(3);
}

void iotHAloop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  // อัปเดตข้อมูลส่งไป HA ทุกๆ 3 วินาที
  unsigned long now = millis();
  if (now - lastMsg > 3000)
  {
    lastMsg = now;
    publish_all_states();
  }
}

// ฟังก์ชันศูนย์กลางในการทำ MQTT Discovery ของเซนเซอร์แต่ละตัว
void send_sensor_config(const char *state_key, const char *name, const char *unit, const char *device_class, const char *icon, const char *category)
{
  // =========================================================================
  // MQTT Discovery: Topic และ Entity ID
  // state_key คือ key ที่ใช้ใน state JSON และต้องคงเดิม
  // ส่วน object_id ของ Home Assistant จะผูกกับ DEVICE_NAME
  // เช่น DEVICE_NAME = "Hybrid Inverter"
  // -> sensor.hybrid_inverter_output_current
  // =========================================================================
  String config_topic = String(discovery_prefix) + "/sensor/" + device_id + "/" + state_key + "/config";

  String entity_object_id = String(DEVICE_NAME) + "_" + state_key;
  entity_object_id.toLowerCase();

  // ทำชื่อให้เหมาะกับ Home Assistant: เว้นวรรค/อักขระพิเศษ -> _
  for (size_t i = 0; i < entity_object_id.length(); ++i)
  {
    char c = entity_object_id[i];
    if (!isalnum(static_cast<unsigned char>(c)) && c != '_')
      entity_object_id.setCharAt(i, '_');
  }

  JsonDocument doc;

  // =========================================================================
  // Sensor Identity / State
  // =========================================================================
  doc["name"] = name;
  doc["object_id"] = entity_object_id.c_str();
  doc["state_topic"] = state_topic;

  String value_template = String("{{ value_json.") + state_key + " }}";
  doc["value_template"] = value_template.c_str();

  String unique_id = String(device_id) + "_" + entity_object_id;
  doc["unique_id"] = unique_id.c_str();

  // =========================================================================
  // Sensor Metadata
  // =========================================================================
  if (unit && strlen(unit) > 0)
    doc["unit_of_measurement"] = unit;
  if (device_class && strlen(device_class) > 0)
    doc["device_class"] = device_class;
  if (icon && strlen(icon) > 0)
    doc["icon"] = icon;
  if (category && strlen(category) > 0)
    doc["entity_category"] = category;

  // =========================================================================
  // Availability
  // =========================================================================
  doc["availability_topic"] = availability_topic;
  doc["payload_available"] = "online";
  doc["payload_not_available"] = "offline";

  // =========================================================================
  // Home Assistant Device
  // =========================================================================
  JsonObject dev = doc["device"].to<JsonObject>();
  JsonArray ids = dev["identifiers"].to<JsonArray>();
  ids.add(device_id);
  dev["name"] = DEVICE_NAME;
  dev["sw_version"] = D_SoftwareVersion;
  dev["manufacturer"] = D_Mfac;
  dev["model"] = D_Model;

  // =========================================================================
  // Publish Discovery Configuration
  // =========================================================================
  char buffer[1024];
  serializeJson(doc, buffer, sizeof(buffer));

  Serial.print("Publishing discovery: ");
  Serial.println(config_topic);
  client.publish(config_topic.c_str(), buffer, true);
  client.loop();
}

void send_ha_discovery()
{
  Serial.println("Sending HA Discovery Configurations...");

  // 1. กลุ่ม Diagnostic Entities (ใส่หมวดหมู่เป็น "diagnostic")
  send_sensor_config("ip_address", "IP Address", "", "", "mdi:ip-network", "diagnostic");
  send_sensor_config("mac_address", "MAC Address", "", "", "mdi:lan-connect", "diagnostic");
  send_sensor_config("uptime", "Uptime", "", "timestamp", "mdi:clock", "diagnostic");
  send_sensor_config("rssi", "WiFi Signal", "dBm", "signal_strength", "mdi:clock", "diagnostic");

  // 2. กลุ่มเซนเซอร์วัดค่าพลังงานและอื่นๆ (เซนเซอร์หลักไม่ระบุ category)
  send_sensor_config("LoadPercent", "Load Percent", "%", "", "mdi:ticket-percent", "");
  send_sensor_config("EnergyDaily", "Energy Daily", "kWh", "energy", "mdi:meter-electric", "");
  send_sensor_config("energy_kWh", "Grid Energy Daily", "kWh", "energy", "mdi:transmission-tower", "");
  send_sensor_config("energy_m_kWh", "Grid Energy Monthly", "kWh", "energy", "mdi:calendar-month", "");
  send_sensor_config("solar_kWh", "Solar Energy Daily", "kWh", "energy", "mdi:solar-power", "");
  send_sensor_config("solar_m_kWh", "Solar Energy Monthly", "kWh", "energy", "mdi:solar-panel-large", "");
  send_sensor_config("gridCostMonthly", "Monthly Grid Bill", "THB", "", "mdi:cash-minus", "");
  send_sensor_config("solarSavingsMonthly", "Monthly Solar Savings", "THB", "", "mdi:cash-plus", "");
  send_sensor_config("GridPower", "Grid Power", "W", "power", "mdi:transmission-tower", "");
  send_sensor_config("ActivePower", "Active Power", "W", "power", "mdi:transmission-tower", "");
  send_sensor_config("ApparentPower", "Apparent Power", "VA", "apparent_power", "mdi:transmission-tower", "");
  send_sensor_config("OutputVolt", "Output Voltage", "V", "voltage", "mdi:flash-triangle", "");
  send_sensor_config("OutputCurrent", "Output Current", "A", "current", "mdi:current-ac", "");
  send_sensor_config("OutputFrequency", "Output Frequency", "Hz", "frequency", "mdi:sine-wave", "");
  send_sensor_config("PowerFactor", "Power Factor", "", "power_factor", "mdi:angle-acute", "");
  send_sensor_config("pvPower", "PV Power", "W", "power", "mdi:transmission-tower-import", "");
  send_sensor_config("pvCurrent", "PV Current", "A", "current", "mdi:current-dc", "");
  send_sensor_config("pvVoltage", "PV Voltage", "V", "voltage", "mdi:flash-triangle", "");
  send_sensor_config("BusVoltage", "Bus Voltage", "V", "voltage", "mdi:flash-triangle", "");
  send_sensor_config("BattVoltage", "Battery Voltage", "V", "voltage", "mdi:flash-triangle", "");
  send_sensor_config("Temp", "Temperature", "°C", "temperature", "mdi:thermometer", "");

  Serial.println("All HA Discovery Configs sent!");
}

// ฟังก์ชันเก็บรวบรวมค่าปัจจุบันทั้งหมดแล้วส่งออกไปยัง HA
void publish_all_states()
{
  JsonDocument doc;

  // --- ดึงค่าฝั่ง Network/Diagnostics ---
  doc["ip_address"] = WiFi.localIP().toString();
  doc["mac_address"] = WiFi.macAddress();
  doc["rssi"] = WiFi.RSSI();

  // --- คำนวณ Uptime (Timestamp) ---
  time_t nowSec;
  time(&nowSec);

  // เช็คว่า NTP ซิงค์เวลาปัจจุบันได้แล้วหรือยัง (1577836800 = 1 Jan 2020)
  if (nowSec > 1577836800)
  {
    time_t bootTime = nowSec - (millis() / 1000);
    char bootTimeStr[25];
    struct tm *timeinfo = gmtime(&bootTime);
    strftime(bootTimeStr, sizeof(bootTimeStr), "%Y-%m-%dT%H:%M:%SZ", timeinfo);

    doc["uptime"] = bootTimeStr; // ตัวอย่าง: "2026-07-20T10:00:00Z"
  }

  // --- ดึงค่าฝั่ง Inverter ---
  doc["LoadPercent"] = inv.data.loadPercent;
  doc["EnergyDaily"] = energy_kWh;
  doc["energy_kWh"] = energy_kWh;
  doc["energy_m_kWh"] = energy_m_kWh;
  doc["solar_kWh"] = solar_kWh;
  doc["solar_m_kWh"] = solar_m_kWh;
  doc["gridCostMonthly"] = gridCostMonthly;
  doc["solarSavingsMonthly"] = solarSavingsMonthly;
  doc["GridPower"] = inv.data.gridPower;
  doc["ActivePower"] = inv.data.ActivePower;
  doc["ApparentPower"] = inv.data.ApparentPower;
  doc["OutputVolt"] = inv.data.outputVoltage;
  doc["OutputCurrent"] = inv.data.outputCurrent;
  doc["OutputFrequency"] = inv.data.outputFrequency;
  doc["PowerFactor"] = inv.data.powerFactor;
  doc["pvPower"] = inv.data.pvPower;
  doc["pvCurrent"] = inv.data.pvCurrent;
  doc["pvVoltage"] = inv.data.pvVoltage;
  doc["BusVoltage"] = inv.data.busVoltage;
  doc["BattVoltage"] = inv.data.batteryVoltage;
  doc["Temp"] = inv.data.temp;

  char buffer[1024];
  serializeJson(doc, buffer, sizeof(buffer));

  // ส่งข่าวก้อนข้อมูลหลัก และ ยืนยันสถานะออนไลน์
  client.publish(state_topic, buffer, true);
  client.publish(availability_topic, "online", true);
  client.loop();
  // Serial.print("Published Data: ");
  // Serial.println(buffer);
}

void reconnect()
{
  static unsigned long lastReconnectAttempt = 0;
  unsigned long now = millis();

  if (!client.connected())
  {
    if (now - lastReconnectAttempt > 5000)
    { // พยายามเชื่อมต่อทุกๆ 5 วินาที
      lastReconnectAttempt = now;
      Serial.print("Attempting MQTT connection...");

      if (client.connect(DEVICE_NAME, MQTT_USER, MQTT_PASS, availability_topic, 0, true, "offline"))
      {
        Serial.println("connected");
        send_ha_discovery();
        client.publish(availability_topic, "online", true);
        publish_all_states();
      }
      else
      {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
      }
    }
  }
}
