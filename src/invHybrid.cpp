#include "invHybrid.h"
#include "websocket_handler.h"
// ************************  invHybrid class  ************************
// public:

void invHybrid::begin()
{
  Serial2.begin(2400, SERIAL_8N1, RX_pin, TX_pin);
  Serial.println("Control Inverter Setup Completed");
  delay(500);
}

void invHybrid::executeCommand(String input)
{
  input.trim();
  if (input.length() == 0)
    return;
  Serial.print("Executing Command : ");
  Serial.println(input);
  sendCommand(input);
  len = input.length();
  Serial.println("len1: " + String(len));
}

void invHybrid::Response()
{
  // Non-blocking response reader. readStringUntil() uses Stream timeout and
  // can stall the main inverter task for up to 1 second on a lost response.
  const unsigned long timeout = 1000;
  const unsigned long startTime = millis();
  char responseBuffer[256];
  size_t responseLen = 0;

  while (millis() - startTime < timeout)
  {
    while (Serial2.available() > 0)
    {
      const int value = Serial2.read();
      if (value < 0)
        break;

      const char ch = static_cast<char>(value);
      if (ch == '\r')
      {
        responseBuffer[responseLen] = '\0';
        invData = String(responseBuffer);
        invData.trim();

        const int lastParen = invData.lastIndexOf('(');
        if (lastParen > 0)
          invData = invData.substring(lastParen);

        len = invData.length();

        if (invData.startsWith("(") && len > 3)
        {
          if (print)
          {
            Serial.printf("Inverter respond (%s): %s\n",
                          lastSentCommand.c_str(), invData.c_str());
            Serial.printf("len: %u\n", static_cast<unsigned>(len));
          }

          if (lastSentCommand == "QPIGS")
            parseQPIGS(invData);
          else if (lastSentCommand == "QPIRI")
            parseQPIRI(invData);
          else if (lastSentCommand == "QPIWS")
            parseQPIWS(invData);

          lastResponseTime = millis();
          return;
        }

        responseLen = 0;
        continue;
      }

      if (responseLen < sizeof(responseBuffer) - 1)
        responseBuffer[responseLen++] = ch;
      else
        responseLen = 0;
    }

    vTaskDelay(pdMS_TO_TICKS(5));
  }

  // Discard incomplete/stale bytes without blocking the task.
  while (Serial2.available() > 0)
    Serial2.read();

  invData = "";
  len = 0;
}

uint16_t invHybrid::modbusCRC(const uint8_t *buf, uint16_t len)
{
  uint16_t crc = 0xFFFF;
  for (uint16_t pos = 0; pos < len; pos++)
  {
    crc ^= (uint16_t)buf[pos];
    for (uint8_t i = 0; i < 8; i++)
    {
      if (crc & 0x0001)
      {
        crc >>= 1;
        crc ^= 0xA001;
      }
      else
      {
        crc >>= 1;
      }
    }
  }
  return crc;
}

size_t invHybrid::buildModbusWrite(uint8_t slaveID, uint16_t regAddr, uint16_t value, uint8_t *frame)
{
  frame[0] = slaveID;        // Slave ID
  frame[1] = 0x06;           // Function code (Write Single Register)
  frame[2] = regAddr >> 8;   // Register High
  frame[3] = regAddr & 0xFF; // Register Low
  frame[4] = value >> 8;     // Value High
  frame[5] = value & 0xFF;   // Value Low

  uint16_t crc = modbusCRC(frame, 6);
  frame[6] = crc & 0xFF; // CRC Low
  frame[7] = crc >> 8;   // CRC High

  return 8; // frame length
}

void invHybrid::valueToinv(String Name, uint16_t val)
{
  if (Name == "Grid Tie Auto" && val == 1)
  {
    gridOpr = true;
    Serial.println("Grid Tie Auto ON");
    return;
  }
  if (Name == "Grid Tie Auto" && val == 0)
  {
    gridOpr = false;
    Serial.println("Grid Tie Auto OFF");
    return;
  }

  uint8_t frame[8];
  auto it = InvAddress.find(Name);
  if (it == InvAddress.end())
  {
    Serial.println("Register not found: " + Name);
    return;
  }

  uint16_t regAddr = it->second;                            // ดึงค่าจริงจาก map
  size_t len = buildModbusWrite(0x05, regAddr, val, frame); // สร้าง Modbus frame
  Serial2.write(frame, len);                                // ส่งคำสั่งไปยัง Inverter ผ่าน Serial2 ตามจำนวนไบต์จริง

  Serial.print("Command HEX: "); // แสดง HEX frame ใน Serial
  for (size_t i = 0; i < len; i++)
  {
    if (frame[i] < 0x10)
      Serial.print("0");
    Serial.print(frame[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

void invHybrid::sendCommand(String data)
{
  // 🔴 1. ล้างขยะที่ค้างอยู่ใน Serial2 RX Buffer ให้หมดก่อนส่งคำสั่งใหม่ไป Inverter
  while (Serial2.available() > 0)
  {
    Serial2.read();
  }

  // 🔴 2. บันทึกคำสั่งล่าสุดเพื่อเอาไว้เช็คในฟังก์ชัน Response()
  lastSentCommand = data; 

  // erial.println("Sent in function sendCommand");

  // inquiry command to inverter
  // it will be calculated and added before send) // crc "\xB7\xA9" // CR "\x0D"
  byte QPIGS[] = {0x51, 0x50, 0x49, 0x47, 0x53, 0xB7, 0xA9, 0x0D}; // len = 110 Device general status parameters inquiry
  byte QPIRI[] = {0X51, 0X50, 0X49, 0X52, 0X49, 0XF8, 0X54, 0x0D}; // len = 112 Device Rating Information inquiry
  byte QFLAG[] = {0X51, 0X46, 0X4C, 0X41, 0X47, 0X98, 0X74, 0x0D}; // len = 18 Device flag status inquiry
  byte QPIWS[] = {0X51, 0X50, 0X49, 0X57, 0X53, 0xB4, 0XDA, 0x0D}; // len = 36 DeviceWarning Status inquiry
  byte QDI[]   = {0X51, 0X44, 0X49, 0x71, 0X1B, 0x0D};             // len = 85 The default setting value information
  byte QMOD[]  = {0X51, 0X4D, 0X4F, 0X44, 0XC1, 0x0D};             // len = 5 Device Mode inquiry
  ////////////////////////////////////////////////////////////////////////

  // Inquiry to inverter
  if (data == "QPIGS") // Device general status parameters inquiry
  {
    Serial2.write(QPIGS, sizeof(QPIGS));
    if (print) { sentinv(data); }
  }

  if (data == "QPIRI") // Device Rating Information inquiry
  {
    Serial2.write(QPIRI, sizeof(QPIRI));
    if (print) { sentinv(data); }
  }

  if (data == "QFLAG") // Device flag status inquiry
  {
    Serial2.write(QFLAG, sizeof(QFLAG));
    if (print) { sentinv(data); }
  }

  if (data == "QPIWS") // DeviceWarning Status inquiry
  {
    Serial2.write(QPIWS, sizeof(QPIWS));
    if (print) { sentinv(data); }
  }

  if (data == "QDI") // The default setting value information
  {
    Serial2.write(QDI, sizeof(QDI));
    if (print) { sentinv(data); }
  }

  if (data == "QMOD") // The default setting value information
  {
    Serial2.write(QMOD, sizeof(QMOD));
    if (print) { sentinv(data); }
  }

  // หน่วงเวลาเล็กน้อยให้ Inverter ประมวลผลก่อนอ่าน Response
  if (data == "QPIGS" || data == "QPIRI" || data == "QFLAG" || data == "QPIWS" || data == "QDI" || data == "QMOD")
  {
    vTaskDelay(pdMS_TO_TICKS(50));
  }

  ///////////////////////////////////////////////////////////////////////////////
  // ESP Reset
  if (data == "espreset")
  {
    Serial.println("ESP Reset");
    vTaskDelay(pdMS_TO_TICKS(5000));
    ESP.restart();
  }

  ///////////////////////////////////////////////////////////////////////////////
  // energy Reset
  if (data == "energy reset")
  {
    energy = true;
    Serial.println("Energy Reset");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  //////////////////////////////////////////////////////////////////////////////
  // Run mode
  if (data == "run mode")
  {
    RunMode = true;
    Serial.println("Run mode");
  }
  if (data == "stop mode")
  {
    RunMode = false;
    Serial.println("Stop mode");
  }

  //////////////////////////////////////////////////////////////////////////////
  // wifi configuration
  if (data == "wifi mode 1")
  {
    wifi_config = true;
    Serial.println("wifi mode 1");
  }
  if (data == "wifi mode 0")
  {
    wifi_config = false;
    Serial.println("wifi mode 0");
  }

  //////////////////////////////////////////////////////////////////////////////
  // ip configuration
  if (data == "ip mode 1")
  {
    ip_config = true;
    Serial.println("ip mode 1");
  }
  if (data == "ip mode 0")
  {
    ip_config = false;
    Serial.println("ip mode 0");
  }

  //////////////////////////////////////////////////////////////////////////////
  // debug mode
  if (data == "debug1 on")
  {
    debug1 = true;
    Serial.println("Debug1 mode on");
  }
  if (data == "debug1 off")
  {
    debug1 = false;
    Serial.println("Debug1 mode off ");
  }

  //////////////////////////////////////////////////////////////////////////////
  // Test mode
  if (data == "test on")
  {
    test = true;
    Serial.println("test mode on");
  }
  if (data == "test off")
  {
    test = false;
    Serial.println("test mode off");
  }

  //////////////////////////////////////////////////////////////////////////////
  // print mode
  if (data == "print on")
  {
    print = true;
    Serial.println("print mode on");
  }
  if (data == "print off")
  {
    print = false;
    Serial.println("print mode off");
  }

  ///////////////////////////////////////////////////////////////////////////////
  // reset wifi para
  if (data == "para res")
  {
    para = true;
    Serial.println("Resset setting");
  }

  ///////////////////////////////////////////////////////////////////////////////
  // read DIR SPIFFS
  if (data == "littleFS")
  {
    dir = true;
    Serial.println("read DIR SPIFFS");
  }

  ///////////////////////////////////////////////////////////////////////////////
  // Format SPIFFS
  if (data == "formatFS")
  {
    format = true;
    Serial.println("Format SPIFFS");
  }

  ///////////////////////////////////////////////////////////////////////////////
  // Energy Reset
  if(data == "energyreset")
  {
    energyreset = true;
    Serial.println("Energy Reset");
  }

  ///////////////////////////////////////////////////////////////////////////////
  // Help
  if (data == "help")
  {
    help();
  }
}

// private:
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

void invHybrid::parseQPIGS(String response)
{
  if (telemetryMutex)
    xSemaphoreTake(telemetryMutex, pdMS_TO_TICKS(50));

  if (response.startsWith("("))
  {
    response = response.substring(1, response.length() - 1); // Remove parentheses
    char dataArray[120];                                     // Fixed-size buffer
    response.toCharArray(dataArray, sizeof(dataArray));      // Convert to C-string
    char *token = strtok(dataArray, " ");                    // Split by space
    int index = 0;
    while (token != NULL)
    {
      switch (index)
      {
      case 0:  data.gridVoltage = atof(token); break;
      case 1:  data.gridFrequency = atof(token); break;
      case 2:  data.outputVoltage = atof(token); break;
      case 3:  data.outputFrequency = atof(token); break;
      case 4:  data.ApparentPower = strtoul(token, NULL, 10); break;
      case 5:  data.ActivePower = strtoul(token, NULL, 10); break;
      case 6:  data.loadPercent = strtoul(token, NULL, 10); break;
      case 7:  data.busVoltage = strtoul(token, NULL, 10); break;
      case 8:  data.batteryVoltage = atof(token); break;
      case 9:  data.batteryChargeCurrent = atof(token); break;
      case 10: data.batterySOC = atof(token); break;
      case 11: data.temp = strtoul(token, NULL, 10); break;
      case 12: data.pvCurrent = atof(token); break;
      case 13: data.pvVoltage = atof(token); break;
      case 14: data.batterySccVoltage = atof(token); break;
      case 15: data.batteryDischargeCurrent = atof(token); break;
      case 16:
        data.InverterStatus = strtoul(token, NULL, 10);
        data.batteryStatusBits = String(token);
        break;
      case 17: data.unknow17 = strtoul(token, NULL, 10); break;
      case 18: data.unknow18 = strtoul(token, NULL, 10); break;
      case 19: data.unknow19 = strtoul(token, NULL, 10); break;
      case 20: data.unknow20 = strtoul(token, NULL, 10); break;
      }
      token = strtok(NULL, " ");
      index++;
    }
  }

  // 1. คำนวณ Output Current (กระแสฝั่งโหลด)
  if (data.gridVoltage > 0) {
    data.outputCurrent = (float)data.ApparentPower / data.gridVoltage;
    data.outputCurrent = roundf(data.outputCurrent * 10.0f) / 10.0f; // ทศนิยม 1 ตำแหน่ง
  } else {
    data.outputCurrent = 0.0f;
  }

  // 2. คำนวณ Power Factor
  if (data.ApparentPower > 0) {
    data.powerFactor = (float)data.ActivePower / (float)data.ApparentPower;
    if (isnan(data.powerFactor) || data.powerFactor < 0.0f || data.powerFactor > 1.0f) {
      data.powerFactor = 0.0f;
    }
    data.powerFactor = roundf(data.powerFactor * 100.0f) / 100.0f; // ทศนิยม 2 ตำแหน่ง
  } else {
    data.powerFactor = 0.0f;
  }

  // 3. คำนวณ กำลังไฟจาก PV (Watt) -> รองรับทศนิยม
  data.pvPower = data.pvCurrent * data.pvVoltage;
  data.pvPower = roundf(data.pvPower * 10.0f) / 10.0f; // ทศนิยม 1 ตำแหน่ง

  // 4. คำนวณ กำลังไฟ Grid (Watt) -> รองรับทศนิยม และ ติดลบได้
  data.gridPower = (float)data.ActivePower - data.pvPower;
  data.gridPower = roundf(data.gridPower * 10.0f) / 10.0f; // ทศนิยม 1 ตำแหน่ง

  // 5. Battery power and direction.
  // Positive = charging, negative = discharging.
  const float chargeCurrent = max(0.0f, data.batteryChargeCurrent);
  const float dischargeCurrent = max(0.0f, data.batteryDischargeCurrent);
  data.batteryPower = data.batteryVoltage * (chargeCurrent - dischargeCurrent);

  if (fabsf(data.batteryPower) < 5.0f) data.batteryPower = 0.0f;
  data.batteryPower = roundf(data.batteryPower * 10.0f) / 10.0f;

  if (data.batteryPower > 0.0f) {
    data.batteryDirection = "charging";
  } else if (data.batteryPower < 0.0f) {
    data.batteryDirection = "discharging";
  } else {
    bool chargingFlag = false;
    if (data.batteryStatusBits.length() >= 8) {
      chargingFlag =
          data.batteryStatusBits.charAt(5) == '1' ||
          data.batteryStatusBits.charAt(6) == '1' ||
          data.batteryStatusBits.charAt(7) == '1';
    }
    data.batteryDirection = chargingFlag ? "charging" : "idle";
  }

  if (telemetryMutex)
    xSemaphoreGive(telemetryMutex);
}

void invHybrid::parseQPIRI(String response)
{
  if (response.startsWith("("))
  {
    response = response.substring(1, response.length() - 1); // Remove parentheses
    char dataArray[120];                                     // Fixed-size buffer (QPIRI max ~110 chars after trim)
    response.toCharArray(dataArray, sizeof(dataArray));      // Convert to C-string
    char *token = strtok(dataArray, " ");                    // Split by space // First token
    int index = 0;
    while (token != NULL)
    {
      switch (index)
      {
      case 0:
        rated.GridRatingVoltage = atof(token);
        break; // Convert to float
      case 1:
        rated.GridRatingCurrent = atof(token);
        break;
      case 2:
        rated.OutputRatingVoltage = atof(token);
        break;
      case 3:
        rated.OutputRatingFrequency = atof(token);
        break;
      case 4:
        rated.OutputRatingCurrent = strtoul(token, NULL, 10);
        break;
      case 5:
        rated.OutputRatingApparentPower = strtoul(token, NULL, 10);
        break;
      case 6:
        rated.OutputRatingActivePower = strtoul(token, NULL, 10);
        break;
      case 7:
        rated.BatteryRatingVoltage = strtoul(token, NULL, 10);
        break;
      case 8:
        rated.BatteryReChargeVoltage = atof(token);
        break;
      case 9:
        rated.BatteryUnderVoltage = strtoul(token, NULL, 10);
        break;
      case 10:
        rated.BatteryBulkVoltage = strtoul(token, NULL, 10);
        break;
      case 11:
        rated.BatteryFloatVoltage = strtoul(token, NULL, 10);
        break;
      case 12:
        rated.BatteryType = atof(token);
        break;
      case 13:
        rated.MaxAC_ChargingCurrent = atof(token);
        break;
      case 14:
        rated.MaxChargingCurrent = atof(token);
        break;
      case 15:
        rated.InputVoltageRange = strtoul(token, NULL, 10);
        break;
      case 16:
        rated.OutputSourcePriority = strtoul(token, NULL, 10);
        break;
      case 17:
        rated.ChargerSourcePriority = strtoul(token, NULL, 10);
        break;
      case 18:
        rated.ParallelMaxNum = strtoul(token, NULL, 10);
        break;
      case 19:
        rated.MachineType = strtoul(token, NULL, 10);
        break;
      case 20:
        rated.Topology = strtoul(token, NULL, 10);
        break;
      case 21:
        rated.OutputMode = strtoul(token, NULL, 10);
        break;
      case 22:
        rated.BatteryReDischargeVoltage = strtoul(token, NULL, 10);
        break;
      case 23:
        rated.PV_Parallel = strtoul(token, NULL, 10);
        break;
      case 24:
        rated.PV_Balance = strtoul(token, NULL, 10);
        break;
      case 25:
        rated.MaxChargingTime = strtoul(token, NULL, 10);
        break;
      case 26:
        rated.OperationLogic = strtoul(token, NULL, 10);
        break;
      case 27:
        rated.MaxDischargingCurrent = strtoul(token, NULL, 10);
        break;
      }
      token = strtok(NULL, " ");
      index++;
    }
  }
}

void invHybrid::parseQPIWS(const String &resp)
{
  if (telemetryMutex)
    xSemaphoreTake(telemetryMutex, pdMS_TO_TICKS(50));

  if (resp.length() < 34)
  {
    faultList = "Invalid QPIWS";
    Serial.println("QPIWS too short: " + resp);
    Serial.println("faultList: " + faultList);
    if (telemetryMutex)
      xSemaphoreGive(telemetryMutex);
    return;
  }
  // ดึงเฉพาะ 32 บิต (index 1 ถึง 32)
  String bits = resp.substring(1, 33);
  faultList = "";
  for (int i = 0; i < 32; i++)
  {
    if (bits[i] == '1' && strncmp(faultNames[i], "reserved", 8) != 0)
    {
      if (faultList.length())
        faultList += ", ";
      faultList += faultNames[i];
    }
  }
  if (!faultList.length())
  {
    faultList = "Normal";
  }
  // Serial print เพื่อตรวจสอบ
  Serial.println("QPIWS Response: " + resp);
  Serial.println("Parsed Fault List: " + faultList);
  if (telemetryMutex)
    xSemaphoreGive(telemetryMutex);
}

void invHybrid::sentinv(String data)
{
  Serial.println("Sent command " + data + " to Inverter");
}

void invHybrid::help()
{
  Serial.println("*********************** command ***********************");
  Serial.println("QPIGS //Device general status parameters inquiry");
  Serial.println("QPIRI //Device Rating Information inquiry");
  Serial.println("QFLAG //Device flag status inquiry");
  Serial.println("QPIWS //DeviceWarning Status inquiry");
  Serial.println("QDI //The default setting value information");
  Serial.println("QMOD //Device Mode inquiry");
  Serial.println("run mode //Sent QPIGS to inverter any 3 second");
  Serial.println("stop mode //Stop sent QPIGS to inverter any 3 second");
  Serial.println("wifi mode 1 //Access Point Mode");
  Serial.println("wifi mode 0 //Station Moe");
  Serial.println("ip mode 1 //Staic IP");
  Serial.println("ip mode 0 //DHCP");
  Serial.println("print on");
  Serial.println("print off");
  Serial.println("test on");
  Serial.println("test off");
  Serial.println("para res //reset setting");
  Serial.println("littleFS //read DIR SPIFFS");
  Serial.println("formatFS //Format SPIFFS");
}
