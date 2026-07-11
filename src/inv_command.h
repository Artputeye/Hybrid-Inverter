#ifndef INV_COMMAND_H
#define INV_COMMAND_H
#include <Arduino.h>
#include <string.h>
#include <map>

class inv_command
{
public:
  /***************************************** public variable********************************/
  String invData;
  String serialData;
  String faultList;
  unsigned int len;
  unsigned long lastResponseTime = 0;

  // parameter setting
  bool debug1 = false; // debug1
  bool test = false;   // test sent data to home assistant //simulate.cpp, iotHA.cpp
  bool print = false;  // serail print data //
  bool para = false;   // parameter reste
  bool format = false; // fomat littleFS //parameterData.cpp
  bool dir = false;    // dir littleFS //parameterData.cpp
  bool RunMode = true; // auto mode //main.cpp, wifiConfig.cpp
  bool wifi_config = false;
  bool ip_config = false;
  bool energy = false;
  bool gridOpr = true;

  std::map<String, uint16_t> InvAddress = {
      {"Buzzer", 0x138A},
      {"PowerSavingMode", 0x138B},
      {"LCDBlacklight", 0x138C},
      {"OverloadAutoRestart", 0x138D},
      {"OverTempAutoRestart", 0x138E},
      {"BeepsWhilePrimarySuorceInterrupt", 0x138F},
      {"ReturnToTheMainLCDPage", 0x1390},
      {"TransferToBypassOverload", 0x1391},
      {"RecordFaultCode", 0x1392},
      {"BatteryEqualization", 0x1393},
      {"ActivatedImmediately", 0x1394},
      {"RestoreDefaults", 0x1398},
      {"ChargerSourcePriority", 0x1399},
      {"OutputSourcePriority", 0x139A},
      {"ACInputRange", 0x139B},
      {"BatteryType", 0x139C},
      {"MaximumChargingCurrent", 0x139E},
      {"OutputVoltage", 0x139F},
      {"MaximumUtilityChargingCurrent", 0x13A0},
      {"VoltagePointBacktoUtility", 0x13A1},
      {"VoltagePointBacktoBattery", 0x13A2},
      {"BulkChargingVoltage", 0x13A3},
      {"FloatingChargingVoltage", 0x13A4},
      {"LowBatteryCutoffVoltage", 0x13A5},
      {"BatteryEqualizationVoltage", 0x13A6},
      {"BatteryEqualizationTime", 0x13A7},
      {"BatteryEqualizationTimeout", 0x13A8},
      {"BatteryEqualizationInterval", 0x13A9},
      {"GridTieOperation", 0x13AA},
      {"GridTieCurrent", 0x13AB},
      {"LedPatternLight", 0x13AC},
      {"DualOutput", 0x13AD},
      {"DualVoltagePoint", 0x13AE}};

  // Structure to store the data for QPIGS
  struct QPIGSVals_t
  {
    float gridVoltage;       // 0
    float gridFrequency;     // 1
    float outputVoltage;     // 2
    float outputFrequency;   // 3
    uint32_t ApparentPower;  // 4
    uint32_t ActivePower;    // 5
    uint32_t loadPercent;    // 6
    uint32_t busVoltage;     // 7
    float batteryVoltage;    // 8
    uint32_t unknow9;        // 9
    uint32_t unknow10;       // 10
    uint32_t temp;           // 11
    float pvCurrent;         // 12
    float pvVoltage;         // 13
    float unknow14;          // 14
    uint32_t unknow15;       // 15
    uint32_t InverterStatus; // 16
    uint32_t unknow17;       // 17
    uint32_t unknow18;       // 18
    uint32_t unknow19;       // 19
    uint32_t unknow20;       // 20
    float outputCurrent;       // 21
    float powerFactor;        // 22
    uint32_t pvPower;         // 23
  } data;

  struct QPIRIvals_t // Device Rating Information inquiry
  {
    uint32_t GridRatingVoltage;         // 1  Grid rating voltage               xx.x V * 10
    uint32_t GridRatingCurrent;         // 2  Grid rating current               xx.x A * 10
    uint32_t OutputRatingVoltage;       // 3  AC output rating voltage          xxx.x V * 10
    uint32_t OutputRatingFrequency;     // 4  AC output rating frequency        xx.x Hz * 10
    uint32_t OutputRatingCurrent;       // 5  AC output rating current          xx.x A * 10
    uint32_t OutputRatingApparentPower; // 6  AC output rating apparent power   xxxx VA
    uint32_t OutputRatingActivePower;   // 7  AC output rating active power     xxxx W
    uint32_t BatteryRatingVoltage;      // 8  Battery rating voltage            xx.x V * 10
    uint32_t BatteryReChargeVoltage;    // 9 Battery re-charge voltage         xx.x V * 10
    uint32_t BatteryUnderVoltage;       // 10 Battery under voltage             xx.x V * 10
    uint32_t BatteryBulkVoltage;        // 11 Battery bulk voltage              xx.x V * 10
    uint32_t BatteryFloatVoltage;       // 12 Battery float voltage             xx.x V * 10
    uint8_t BatteryType;                // 13 Battery type 0-9                  0: AGM 1: Flooded 2: User 3: Pylon 5: Weco 6: Soltaro 8: Lib 9: Lic (5,6,8,9 protocol 2 )
    uint8_t MaxAC_ChargingCurrent;      // 14 Max AC charging current           xxx A
    uint8_t MaxChargingCurrent;         // 15 Max charging current              xxx A
    uint8_t InputVoltageRange;          // 16 input voltage range 0-1           0: Appliance 1: UPS
    uint8_t OutputSourcePriority;       // 17 output source priority 0-2        0: UtilitySolarBat 1: SolarUtilityBat 2: SolarBatUtility
    uint8_t ChargerSourcePriority;      // 18 charger source priority 0-3       0: Utility first 1: Solar first 2: Solar + Utility 3: Only solar charging permitted ( protocol 2 1-3 )
    uint8_t ParallelMaxNum;             // 19 parallel max num 0-9
    uint8_t MachineType;                // 20 Machine type                      00: Grid tie; 01: Off Grid; 10: Hybrid.
    uint8_t Topology;                   // 21 Topology                          0: transformerless 1: transformer
    uint8_t OutputMode;                 // 22 Output mode 0-7
    uint32_t BatteryReDischargeVoltage; // 23 Battery re-discharge voltage xx.x V *10
    uint8_t PV_Parallel;                // 24 PV OK condition for parallel
    uint8_t PV_Balance;                 // 25 PV power balance
    uint32_t MaxChargingTime;           // 26 Max. charging time at C.V stage (only 48V model)     xxx minute  ( Protocol 2 only )
    uint8_t OperationLogic;             // 27 Operation Logic (only 48V model) 0-2                             ( Protocol 2 only )
    uint8_t MaxDischargingCurrent;      // 28 Max discharging current (only 48V model)     xxx A
  } rated;

  const char *faultNames[32] = {
      "reserved0", "Inverter Fault", "Bus Over", "Bus Under",
      "Bus Soft Fail", "Line Fail", "OPV Short", "Over Temperature",
      "Fan Locked", "Battery Voltage High", "Battery Low Alarm",
      "reserved13", "Battery Under Shutdown", "reserved15", "Overload",
      "EEPROM Fault", "Inverter Over Current", "Inverter Soft Fail",
      "Self Test Fail", "OP DC Voltage Over", "Battery Open",
      "Current Sensor Fail", "Battery Short", "Power Limit",
      "PV Voltage High", "MPPT Overload Fault", "MPPT Overload Warning",
      "Battery Too Low To Charge", "reserved30", "reserved31"};

  /**************************************** public function ********************************/
  void serialSent();
  void Response();
  uint16_t modbusCRC(const uint8_t *buf, uint16_t len);
  size_t buildModbusWrite(uint8_t slaveID, uint16_t regAddr, uint16_t value, uint8_t *frame);
  void valueToinv(String Name, uint16_t val);
  void cmd_inv(String data);

private:
  /***************************************** private function *******************************/
  void parseQPIGS(String response);
  void parseQPIRI(String response);
  void parseQPIWS(const String& resp);
  void sentinv(String data);
  void help();
};

#endif