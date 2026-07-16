// ha_integration.cpp
#include "ha_integration.h"

// --- Diagnostic Entities Pointer ---
HASensor *ipAddrSensor = nullptr;
HASensor *macAddrSensor = nullptr;
HASensor *uptimeSensor = nullptr;
HASensor *rssiSensor = nullptr;

// --- Switch Object Pointer ---
HASwitch *grid = nullptr;

// --- Sensor Object Pointer ---
HASensorNumber *LoadPercent = nullptr;
HASensorNumber *EnergyDaily = nullptr;
HASensorNumber *GridPower = nullptr;
HASensorNumber *ActivePower = nullptr;
HASensorNumber *ApparentPower = nullptr;
HASensorNumber *OutputVolt = nullptr;
HASensorNumber *OutputCurrent = nullptr;
HASensorNumber *OutputFrequency = nullptr;
HASensorNumber *PowerFactor = nullptr;
HASensorNumber *pvPower = nullptr;
HASensorNumber *pvCurrent = nullptr;
HASensorNumber *pvVoltage = nullptr;
HASensorNumber *BusVoltage = nullptr;
HASensorNumber *BattVoltage = nullptr;
HASensorNumber *Temp = nullptr;

void iotHAsetup()
{
    Serial.println(F("[HA] Initializing Device Details..."));

    // 1. ตั้งค่าข้อมูลพื้นฐานของอุปกรณ์
    device.setName(DEVICE_NAME);
    device.setSoftwareVersion(D_SoftwareVersion);
    device.setManufacturer(D_Mfac);
    device.setModel(D_Model);

    // 2. สร้างและตั้งค่า Diagnostic Entities
    ipAddrSensor = new HASensor("ip_address");
    ipAddrSensor->setName("IP Address");
    ipAddrSensor->setIcon("mdi:ip-network");

    macAddrSensor = new HASensor("mac_address");
    macAddrSensor->setName("MAC Address");
    macAddrSensor->setIcon("mdi:lan-connect");

    uptimeSensor = new HASensor("uptime");
    uptimeSensor->setName("Uptime");
    uptimeSensor->setIcon("mdi:clock-start");

    rssiSensor = new HASensor("rssi");
    rssiSensor->setName("WiFi Signal");
    rssiSensor->setUnitOfMeasurement("dBm");
    rssiSensor->setDeviceClass("signal_strength");

    // 3. สร้างและตั้งค่า Entities (สวิตช์/เซนเซอร์)
    grid = new HASwitch("grid");
    grid->onCommand(GridTie);
    grid->setName("Grid Tie");
    grid->setIcon("mdi:transmission-tower-export");
    // grid->getCurrentState(); // หากต้องการใช้ให้เปลี่ยนเป็น ->

    ////////////////////////////////////////////////////////////////////////////////
    // สร้างวัตถุเซนเซอร์พร้อมกำหนดความละเอียด (Precision) ตามของเดิม
    LoadPercent = new HASensorNumber("LoadPercent");
    LoadPercent->setName("Load Percent");
    LoadPercent->setIcon("mdi:ticket-percent");
    LoadPercent->setUnitOfMeasurement("%");

    EnergyDaily = new HASensorNumber("EnergyDaily", HASensorNumber::PrecisionP3);
    EnergyDaily->setName("Energy Daily");
    EnergyDaily->setIcon("mdi:meter-electric");
    EnergyDaily->setUnitOfMeasurement("kWh");

    GridPower = new HASensorNumber("GridPower");
    GridPower->setName("Grid Power");
    GridPower->setIcon("mdi:transmission-tower");
    GridPower->setUnitOfMeasurement("W");

    ActivePower = new HASensorNumber("ActivePower");
    ActivePower->setName("Active Power");
    ActivePower->setIcon("mdi:transmission-tower");
    ActivePower->setUnitOfMeasurement("W");

    ApparentPower = new HASensorNumber("ApparentPower");
    ApparentPower->setName("Apparent Power");
    ApparentPower->setIcon("mdi:transmission-tower");
    ApparentPower->setUnitOfMeasurement("VA");

    OutputVolt = new HASensorNumber("OutputVolt", HASensorNumber::PrecisionP1);
    OutputVolt->setName("Output Voltage");
    OutputVolt->setIcon("mdi:flash-triangle");
    OutputVolt->setUnitOfMeasurement("V");

    OutputCurrent = new HASensorNumber("OutputCurrent", HASensorNumber::PrecisionP1);
    OutputCurrent->setName("Output Current");
    OutputCurrent->setIcon("mdi:current-ac");
    OutputCurrent->setUnitOfMeasurement("A");

    OutputFrequency = new HASensorNumber("OutputFrequency", HASensorNumber::PrecisionP1);
    OutputFrequency->setName("Output Frequency");
    OutputFrequency->setIcon("mdi:sine-wave");
    OutputFrequency->setUnitOfMeasurement("Hz");

    PowerFactor = new HASensorNumber("PowerFactor", HASensorNumber::PrecisionP2);
    PowerFactor->setName("Power Factor");
    PowerFactor->setIcon("mdi:angle-acute");

    pvPower = new HASensorNumber("pvPower", HASensorNumber::PrecisionP1);
    pvPower->setName("PV Power");
    pvPower->setIcon("mdi:transmission-tower-import");
    pvPower->setUnitOfMeasurement("W");

    pvCurrent = new HASensorNumber("pvCurrent", HASensorNumber::PrecisionP1);
    pvCurrent->setName("pvCurrent");
    pvCurrent->setIcon("mdi:current-dc");
    pvCurrent->setUnitOfMeasurement("A");

    pvVoltage = new HASensorNumber("pvVoltage", HASensorNumber::PrecisionP1);
    pvVoltage->setName("PV Voltage");
    pvVoltage->setIcon("mdi:flash-triangle");
    pvVoltage->setUnitOfMeasurement("V");

    BusVoltage = new HASensorNumber("BusVoltage");
    BusVoltage->setName("Bus Voltage");
    BusVoltage->setIcon("mdi:flash-triangle");
    BusVoltage->setUnitOfMeasurement("V");

    BattVoltage = new HASensorNumber("BattVoltage", HASensorNumber::PrecisionP1);
    BattVoltage->setName("Battery Voltage");
    BattVoltage->setIcon("mdi:flash-triangle");
    BattVoltage->setUnitOfMeasurement("V");

    Temp = new HASensorNumber("Temp");
    Temp->setName("Temperature");
    Temp->setIcon("mdi:thermometer");
    Temp->setUnitOfMeasurement("°C");

    // 4. เตรียมการเชื่อมต่อ MQTT
    uint16_t port = (uint16_t)atoi(MQTT_PORT);
    IPAddress mqttIP;

    if (mqttIP.fromString(MQTT_ADDR))
    {
        Serial.printf("[HA] Connecting to MQTT: %s:%d\n", MQTT_ADDR, port);
        mqtt.begin(mqttIP, port, MQTT_USER, MQTT_PASS);
    }
    else
    {
        Serial.println(F("❌ [HA] Invalid MQTT IP Address"));
    }
}

void iotHAsim()
{
    // ตรวจสอบความปลอดภัยว่า Pointer ถูกจองเนื้อที่แล้ว และไม่อยู่ในโหมดทดสอบ
    if (LoadPercent != nullptr && !inv.test)
    {
        LoadPercent->setValue(inv.data.loadPercent);
        EnergyDaily->setValue(energy_kWh);
        GridPower->setValue(gridPower);
        ActivePower->setValue(inv.data.ActivePower);
        ApparentPower->setValue(inv.data.ApparentPower);
        OutputVolt->setValue(inv.data.outputVoltage);
        OutputCurrent->setValue(inv.data.outputCurrent);
        OutputFrequency->setValue(inv.data.outputFrequency);
        PowerFactor->setValue(inv.data.powerFactor);
        pvPower->setValue(inv.data.pvPower);
        pvCurrent->setValue(inv.data.pvCurrent);
        pvVoltage->setValue(inv.data.pvVoltage);
        BusVoltage->setValue(inv.data.busVoltage);
        BattVoltage->setValue(inv.data.batteryVoltage);
        Temp->setValue(inv.data.temp);
    }
}

void HA_Diagnostic()
{
    // ตรวจสอบตัวใดตัวหนึ่งในกลุ่ม Diagnostic เพื่อป้องกัน nullptr crash
    if (ipAddrSensor == nullptr)
        return;

    // 1. IP และ MAC
    ipAddrSensor->setValue(WiFi.localIP().toString().c_str());
    macAddrSensor->setValue(MacAddr.c_str());

    // 2. Uptime (แปลงจาก millis เป็น dd:hh:mm)
    uint32_t totalSeconds = millis() / 1000;

    uint32_t days = totalSeconds / 86400;
    uint32_t hours = (totalSeconds % 86400) / 3600;
    uint32_t minutes = (totalSeconds % 3600) / 60;

    // จัดฟอร์แมตให้อยู่ในรูป dd:hh:mm (ใส่ %02u เพื่อให้มีเลข 0 นำหน้ากรณีเป็นเลขหลักเดียว)
    char uptimeStr[16];
    snprintf(uptimeStr, sizeof(uptimeStr), "%02u:%02u:%02u", days, hours, minutes);

    uptimeSensor->setValue(uptimeStr);

    // 3. RSSI
    int8_t rssiVal = WiFi.RSSI();
    rssiSensor->setValue(String(rssiVal).c_str());

    // ปรับ Serial log ให้แสดงค่ารูปแบบใหม่
    Serial.printf("[HA] Diag Update - Uptime: %s, RSSI: %d dBm\n", uptimeStr, rssiVal);
}

//////////////////////////////////////////////////////////////////////////////////////
// sent switch command to inverter
void GridTie(bool state, HASwitch *sender)
{
    sender->setState(state); // report state back to the Home Assistant
    if (state)
    {
        inv.valueToinv("GridTieOperation", 1);
    }
    if (!state)
    {
        inv.valueToinv("GridTieOperation", 0);
    }
}

void iotHArun()
{
    // ฟังก์ชันนี้ต้องถูกเรียกใน loop() หลักของโปรแกรม
    if (isWifiApMode == 1 && WiFi.status() == WL_CONNECTED)
    {
        mqtt.loop(); // Or whatever your MQTT client instance loop is called
        iotHAsim();
    }

    // อัปเดต Diagnostic ทุกๆ 30 วินาที (ตัวอย่าง)
    static unsigned long lastDiag = 0;
    if (millis() - lastDiag > 30000)
    {
        lastDiag = millis();
        HA_Diagnostic();
    }
}
