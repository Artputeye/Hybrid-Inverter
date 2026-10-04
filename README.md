# ⚡ Hybrid Inverter

> ESP32 Web UI & Controller สำหรับ Hybrid Solar Inverter — Monitoring, Energy Tracking, WebSocket, MQTT, Home Assistant และ OTA

<p align="center">
  <img src="https://img.shields.io/badge/ESP32-esp32dev-blue?style=for-the-badge&logo=espressif" alt="ESP32">
  <img src="https://img.shields.io/badge/PlatformIO-6.0.1-orange?style=for-the-badge&logo=platformio" alt="PlatformIO">
  <img src="https://img.shields.io/badge/Arduino-2.0.6-00979D?style=for-the-badge&logo=arduino" alt="Arduino">
  <img src="https://img.shields.io/badge/WebSocket-%2Fws-purple?style=for-the-badge" alt="WebSocket">
</p>

---

## 📖 Overview

โปรเจกต์นี้ใช้ **ESP32** เป็น Web Controller สำหรับ **Anern Hybrid Inverter 4.2kW** เชื่อมต่อผ่าน Serial2/UART และให้บริการ Web Dashboard จาก LittleFS พร้อม telemetry แบบ Real-time ผ่าน WebSocket

## ✨ Features

| ระบบ | รายละเอียด |
|---|---|
| 🔌 Inverter | Anern Hybrid Inverter 4.2kW |
| ⚡ Communication | Serial2 / UART — 2400 8N1 |
| 🌐 Web UI | LittleFS + AsyncWebServer |
| 📡 Real-time | WebSocket `/ws` |
| 🏠 Smart Home | MQTT + Home Assistant Discovery |
| 🔋 Energy | Grid / Solar / Battery + kWh history |
| 📶 Network | Wi-Fi Station + AP configuration |
| 🕐 Time | NTP / GMT+7 |
| 🔄 OTA | Firmware + LittleFS update |

---

## 🧩 Architecture

```text
 Hybrid Inverter
       │
       │ Serial2 / UART
       ▼
  ┌─────────────┐
  │  invHybrid  │  Communication
  └──────┬──────┘
         ▼
  ┌─────────────┐
  │ inv_control │  Command / Parser
  └──────┬──────┘
         ▼
  Application State
      ┌──┴───────────┐
      ▼              ▼
 WebSocket / UI   MQTT / Home Assistant
```

---

## 🔧 Hardware

- **ESP32 Dev Module** — `esp32dev`
- **Anern Hybrid Inverter 4.2kW**
- Serial2 / UART
- Baud rate: `2400`
- Format: `8N1`

> รายละเอียด RX/TX และ hardware configuration อยู่ใน `src/invHybrid.h` และไฟล์ configuration ที่เกี่ยวข้อง

## 💻 Software Stack

- PlatformIO + Arduino Framework
- Espressif32 `6.0.1`
- Arduino-ESP32 `2.0.6`
- ArduinoJson `7.x`
- WiFiManager `2.0.17`
- PubSubClient `2.8`
- ESPAsyncWebServer / AsyncTCP
- LittleFS / ESPmDNS

---

## 📁 Project Structure

```text
Hybrid-Inverter/
├── data/                  # Web UI / LittleFS
├── Json/                  # JSON / configuration
├── partitions/            # OTA partition table
├── src/                   # ESP32 application
│   ├── main.cpp
│   ├── invHybrid.cpp/.h
│   ├── inv_control.cpp/.h
│   ├── energy_tracker.cpp/.h
│   ├── network_manager.cpp/.h
│   ├── websocket_handler.cpp/.h
│   ├── ha_integration.cpp/.h
│   ├── storage_manager.cpp/.h
│   └── logger.cpp/.h
├── platformio.ini
└── README.md
```

---

## 🔌 Inverter Commands

| Command | หน้าที่ |
|---|---|
| `QPIGS` | ค่าการทำงานปัจจุบัน |
| `QPIRI` | Configuration / Rating |
| `QPIWS` | Warning / Fault |
| `QFLAG` | Status flags |
| `QDI` | ข้อมูลเพิ่มเติม |
| `QMOD` | Operating mode |

## 🔋 Energy Tracking

เก็บประวัติพลังงานเป็น:

- 🕐 24 ชั่วโมง
- 📅 30 วัน
- 📆 12 เดือน

ข้อมูลหลักอยู่ใน `/energy_history.json`

## ⚡ Grid Operation

ใช้ hysteresis เพื่อลดการสลับ Grid บ่อยเกินไป:

```text
GRID_ON_THRESHOLD  = 2.0
GRID_OFF_THRESHOLD = 1.0
```

---

## 🌐 Web UI & WebSocket

Frontend อยู่ใน `data/` และ upload ไปยัง ESP32 LittleFS

### WebSocket

```text
/ws
```

### API ที่ใช้งาน

```text
/getsetting
/savesetting
/getbattsetting
/getnetworkconfig
```

Telemetry รองรับข้อมูล inverter, power, energy และสถานะระบบแบบ Real-time

---

## 🏠 MQTT / Home Assistant

`src/ha_integration.cpp` ดูแล MQTT และ Home Assistant Discovery โดยรองรับข้อมูล เช่น:

- Grid Power
- Active / Apparent Power
- Voltage / Current / Frequency
- PV / Solar
- Battery
- Temperature
- Energy
- Cost / Savings

---

## 📶 Network & Time

ใช้ **WiFiManager** สำหรับ Station Mode และ AP Configuration Mode

Configuration หลัก:

```text
/networkconfig.json
```

เวลาใช้ NTP `pool.ntp.org` และ timezone **GMT+7**

---

## 🔄 OTA

| Update | Endpoint |
|---|---|
| Firmware | `/otafirmware` |
| LittleFS | `/otalittlefs` |

Partition configuration: `partitions/ota.csv`

---

## 🛠️ Build & Upload

```bash
# Build
pio run

# Upload firmware
pio run --target upload

# Upload LittleFS
pio run --target uploadfs

# Serial monitor
pio device monitor -b 115200

# Clean build
pio run --target clean
pio run
```

### 💡 Development Notes

- ใช้ **PlatformIO** เป็น source of truth สำหรับ compile
- แก้ Web UI แล้วให้ `uploadfs`
- หลีกเลี่ยง blocking operation ใน communication loop/task
- รักษา timing ของ Serial2 และ inverter polling
- MQTT entity IDs มีผลต่อ Home Assistant Discovery

---

## 📊 Current Build Status

| Item | Status |
|---|---|
| Board | `esp32dev` |
| Espressif32 | `6.0.1` |
| Arduino-ESP32 | `2.0.6` |
| RAM Usage | ~15.1% |
| Flash Usage | ~77.0% |
| PlatformIO Build | ✅ SUCCESS |

---

## 📜 License

กำหนด License ของโปรเจกต์ตามความเหมาะสมก่อนเผยแพร่

<p align="center"><sub>⚡ Hybrid Inverter • ESP32 • Solar Energy Monitoring & Control</sub></p>