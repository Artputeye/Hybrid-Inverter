#include "websocket_handler.h"

AsyncWebSocket ws("/ws");
unsigned long lastTimeMonitor = 0;
unsigned long lastPingTime = 0;
unsigned long MonitorDelay = 3000;
const unsigned long pingInterval = 25000; // 25 วินาที
String wsSerial;
String wsInverter;
String wsControll;

String wsAllDataBase64()
{
    JsonDocument doc;

    ///////////////////////Serial Sent////////////////////////////
    doc["Serial"] = wsSerial;
    doc["Inverter"] = wsInverter;
    doc["controll"] = wsControll;

    ///////////////////////DIVICE_IP////////////////////////////
    doc["DIVICE_IP"] = DIVICE_IP;

    ///////////////////////Monotor////////////////////////////////
    doc["Load Percent"] = inv.data.loadPercent;
    doc["Energy Daily"] = String(energy_kWh, 3);
    doc["Grid Power"] = gridPower;
    doc["Output Apparent Power"] = inv.data.ApparentPower;
    doc["Output Active Power"] = inv.data.ActivePower;
    doc["Output Voltage"] = inv.data.outputVoltage;
    doc["Output Current"] = String(inv.data.outputCurrent, 2);
    doc["Output Frequency"] = inv.data.outputFrequency;
    doc["Power Factor"] = String(inv.data.powerFactor, 2);
    doc["PV Power"] = inv.data.pvPower;
    doc["PV Voltage"] = inv.data.pvVoltage;
    doc["PV Current"] = inv.data.pvCurrent;
    doc["Grid Voltage"] = inv.data.gridVoltage;
    doc["Grid Frequency"] = inv.data.gridFrequency;
    doc["Bus Voltage"] = inv.data.busVoltage;
    doc["Battery Voltage"] = inv.data.batteryVoltage;
    doc["Temperature"] = inv.data.temp;
    doc["Inverter Status"] = inv.data.InverterStatus;

    //////////////////////////Status//////////////////////////////
    doc["Grid Rating Voltage"] = inv.rated.GridRatingVoltage;
    doc["Grid Rating Current"] = inv.rated.GridRatingCurrent;
    doc["Output Rating Voltage"] = inv.rated.OutputRatingVoltage;
    doc["Output Rating Frequency"] = inv.rated.OutputRatingFrequency;
    doc["Output Rating Current"] = inv.rated.OutputRatingCurrent;
    doc["Output Rating Apparent Power"] = inv.rated.OutputRatingApparentPower;
    doc["Output Rating Active Power"] = inv.rated.OutputRatingActivePower;
    doc["Battery Rating Voltage"] = inv.rated.BatteryRatingVoltage;
    doc["Battery Re-charge Voltage"] = inv.rated.BatteryReChargeVoltage;
    doc["Battery Under Voltage"] = inv.rated.BatteryUnderVoltage;
    doc["Battery Bulk Voltage"] = inv.rated.BatteryBulkVoltage;
    doc["Battery Float Voltage"] = inv.rated.BatteryFloatVoltage;
    doc["Battery Type"] = inv.rated.BatteryType;
    doc["Max AC Charging Current"] = inv.rated.MaxAC_ChargingCurrent;
    doc["Max Charging Current"] = inv.rated.MaxChargingCurrent;
    doc["Input Voltage Range"] = inv.rated.InputVoltageRange;
    doc["Output Source Priority"] = inv.rated.OutputSourcePriority;
    doc["Charger Source Priority"] = inv.rated.ChargerSourcePriority;
    doc["Parallel Max Num"] = inv.rated.ParallelMaxNum;
    doc["Machine Type"] = inv.rated.MachineType;
    doc["Topology"] = inv.rated.Topology;
    doc["Output Mode"] = inv.rated.OutputMode;
    doc["Battery Re-discharge Voltage"] = inv.rated.BatteryReDischargeVoltage;
    doc["PV Parallel"] = inv.rated.PV_Parallel;
    doc["PV Balance"] = inv.rated.PV_Balance;
    doc["Max Charging Time"] = inv.rated.MaxChargingTime;
    doc["Operation Logic"] = inv.rated.OperationLogic;
    doc["Max Discharging Current"] = inv.rated.MaxDischargingCurrent;
    /////////////////////////////////Alarm////////////////////////////////////
    doc["Inverter Faults"] = inv.faultList;

    /////////////////////////////////////////////////////////////////////////
    String jsonOut; // serialize JSON
    serializeJson(doc, jsonOut);
    return base64::encode(jsonOut); // encode Base64
}

void wsClear()
{
    JsonDocument doc;
    ///////////////////////Monitor////////////////////////////////
    doc["Load Percent"] = "";
    doc["Energy Daily"] = "";
    doc["Apparent Power"] = "";
    doc["Active Power"] = "";
    doc["Output Voltage"] = "";
    doc["Output Current"] = "";
    doc["Output Frequency"] = "";
    doc["Power Factor"] = "";
    doc["PV Power"] = "";
    doc["PV Voltage"] = "";
    doc["PV Current"] = "";
    doc["Grid Voltage"] = "";
    doc["Grid Frequency"] = "";
    doc["Bus Voltage"] = "";
    doc["Battery Voltage"] = "";
    doc["Temperature"] = "";
    doc["Inverter Status"] = "";

    //////////////////////////Status//////////////////////////////
    doc["Grid Rating Voltage"] = "";
    doc["Grid Current"] = "";
    doc["AC Output Rating Voltage"] = "";
    doc["AC Output Rating Frequency"] = "";
    doc["AC Output Rating Current"] = "";
    doc["AC Output Rating Apparent Power"] = "";
    doc["AC Output Rating Active Power"] = "";
    doc["Battery Rating Voltage"] = "";
    doc["Battery Re-charge Voltage"] = "";
    doc["Battery Under Voltage"] = "";
    doc["Battery Bulk Voltage"] = "";
    doc["Battery Float Voltage"] = "";
    doc["Battery Type"] = "";
    doc["Max AC Charging Current"] = "";
    doc["Max Charging Current"] = "";
    doc["Input Voltage Range"] = "";
    doc["Output Source Priority"] = "";
    doc["Charger Source Priority"] = "";
    doc["Parallel Max Num"] = "";
    doc["Machine Type"] = "";
    doc["Topology"] = "";
    doc["Output Mode"] = "";
    doc["Battery Re-discharge Voltage"] = "";
    doc["PV OK Condition for Parallel"] = "";
    doc["PV OK Condition for PV Power Balance"] = "";
    doc["Max Charging Time"] = "";
    doc["Operation Logic"] = "";
    doc["Max Discharging Current"] = "";

    String jsonOut; // serialize JSON
    serializeJson(doc, jsonOut);
}

void notifyClients(const String &base64Msg)
{
    ws.textAll(base64Msg); // ส่ง Text Frame
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len &&
        (info->opcode == WS_TEXT || info->opcode == WS_BINARY))
    {
        notifyClients(wsAllDataBase64());
    }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len)
{
    switch (type)
    {
    case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n",
                      client->id(), client->remoteIP().toString().c_str());
        break;
    case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
    case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
    }
}

void ws_init()
{
    ws.onEvent(onEvent);
    server.addHandler(&ws);
}

void wsJsonSerial(const String &msg)
{
    wsSerial = msg;
    if (!wsSerial.isEmpty())
    {
        notifyClients(wsAllDataBase64());
        wsSerial = "";
        inv.serialData = "";
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}

void wsJsonInverter(const String &msg)
{
    wsInverter = msg;
    if (!wsInverter.isEmpty())
    {
        notifyClients(wsAllDataBase64());
        wsInverter = "";
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}

void wsJsonControll(const String &msg)
{
    wsControll = msg;
    if (!wsControll.isEmpty())
    {
        notifyClients(wsAllDataBase64());
        wsControll = "";
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}


/////////////////////////////////////////////////////////////////////////////////////
void ws_process()
{
    if (millis() - lastTimeMonitor > MonitorDelay)
    {
        lastTimeMonitor = millis();
        notifyClients(wsAllDataBase64());
        // wsClear();
    }

    if (millis() - lastPingTime > pingInterval)
    {
        lastPingTime = millis();
        ws.pingAll();
    }
    ws.cleanupClients();
}
