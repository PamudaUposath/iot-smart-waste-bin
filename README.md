# 🌱 IoT-Driven Smart Waste Bin (ESP32 + MQTT + Node-RED)

This repository contains the full implementation of our **IoT Smart Waste Bin Prototype**, developed as part of the **Emerging Trends in Computer Science (2nd Year)** module at the **University of Jaffna**.

The project integrates **ESP32 firmware**, **ultrasonic sensors**, **servo automation**, **MQTT communication**, and a **Node-RED dashboard** to automate waste monitoring and improve data-driven waste collection.

---

## 🚀 Features

* Automatic lid open/close using presence detection
* Real-time fill-level monitoring using ultrasonic distance
* Auto-locking when bin usage reaches **≥ 90%**
* Remote control: **Lock/Unlock**, **Open/Close Lid**
* LED indicator system (Blue = normal, Red = locked/full)
* Real-time telemetry, logs & timestamps
* Node-RED admin dashboard with gauge, LED status & log viewer
* JSON-based MQTT communication

---

## 🛠 Tech Stack

**Embedded Hardware**

* ESP32
* HC-SR04 Ultrasonic Sensors × 2
* SG90 Servo Motor
* Red & Blue LEDs

**Firmware Development**

* Arduino IDE
* ESP32Servo
* PubSubClient
* C++

**IoT Communication**

* MQTT (HiveMQ public broker or your own)
* JSON payloads
* Pub/Sub messaging

**Dashboard & Visualization**

* Node-RED Dashboard
* UI Gauge, Switches, Buttons, Text Nodes, Custom LED Template

**Networking & Time Sync**

* Wi-Fi (STA mode)
* NTP Time Sync
* Local Time Conversion using POSIX TZ (LKT +5:30)

---

## 📂 Folder Structure

```text
iot-smart-waste-bin/
├─ firmware/
│   └─ smart_bin_esp32.ino
│
├─ node-red/
│   └─ smart_bin_flow.json
│
├─ docs/
│   └─ IoT-poster.pdf
│
├─ .gitignore
├─ LICENSE
└─ README.md
```

---

## 🔌 Hardware Setup

### Wiring Diagram (ASCII View)

```
        ┌─────────────── ESP32 ───────────────┐
        │                                      │
HC-SR04 (Person)                               HC-SR04 (Bin)
TRIG → GPIO 5                                   TRIG → GPIO 23
ECHO → GPIO 18                                  ECHO → GPIO 22

Servo Motor
Signal → GPIO 19
5V → 5V
GND → GND

LEDs
BLUE → GPIO 26  
RED  → GPIO 25  
(GND for both)
        │                                      │
        └──────────────────────────────────────┘
```

---

## 📦 Required Libraries

Install these via Arduino IDE:

* **PubSubClient** – MQTT Client
* **ESP32Servo** – Servo Control Library
* **WiFi.h** – Built-in for ESP32
* **time.h** – Built-in

Installation:
**Arduino IDE → Sketch → Include Library → Manage Libraries…**

---

## ⚙️ How to Set Up the Project

---

# 1️⃣ Install ESP32 Board Package

In Arduino IDE:
**File → Preferences → Additional Boards Manager URLs**

Add this URL:

```
https://dl.espressif.com/dl/package_esp32_index.json
```

Then go to:
**Tools → Board → Board Manager → Search "ESP32" → Install**

---

# 2️⃣ Configure Firmware

Open:

```
firmware/smart_bin_esp32.ino
```

Update your WiFi and MQTT details:

```cpp
const char* ssid        = "YOUR_WIFI_SSID";
const char* password    = "YOUR_WIFI_PASSWORD";
const char* mqtt_server = "YOUR_MQTT_BROKER_URL";  // e.g., "broker.hivemq.com"
const uint16_t mqtt_port = 1883;
```

Select your board:
**Tools → Board → ESP32 Dev Module**

Upload to ESP32.

---

# 3️⃣ Set Up Node-RED Dashboard

### Install Node-RED

(If not installed)

```bash
npm install -g node-red
node-red
```

### Install Dashboard nodes

In Node-RED:

**Menu → Manage Palette → Install → `node-red-dashboard`**

### Import the dashboard

File:

```
node-red/smart_bin_flow.json
```

In Node-RED:
**Menu → Import → Paste JSON → Import**

Configure MQTT Broker Node:

* Server: `broker.hivemq.com`
* Port: `1883`

Deploy → Open UI:

```
http://localhost:1880/ui
```

You will now see:
✔ Fill Gauge
✔ LED status
✔ Last Trigger Time
✔ Admin Lock Switch
✔ Open/Close Lid Buttons

---
## 🌐 Online Simulator (Wokwi)

You can try a virtual version of our Smart Bin using the Wokwi simulator:

🔗 **Live Simulator:**  
https://wokwi.com/projects/441368271264615425

This simulation demonstrates the ESP32 logic, ultrasonic sensing, and servo control in a virtual environment, making it easy to understand and test the core functionality without real hardware.

---
## 📡 MQTT Topic Reference

| Purpose      | Topic                     | Direction        |
| ------------ | ------------------------- | ---------------- |
| Telemetry    | `smartbin/Bin1/tele`      | ESP32 → Node-RED |
| State        | `smartbin/Bin1/state`     | ESP32 → Node-RED |
| Admin Lock   | `smartbin/Bin1/cmd/lock`  | Node-RED → ESP32 |
| Remote Open  | `smartbin/Bin1/cmd/open`  | Node-RED → ESP32 |
| Remote Close | `smartbin/Bin1/cmd/close` | Node-RED → ESP32 |

---

## 🧪 Testing the System

1. Approach the bin → **Lid auto-opens**
2. Move away → **Closes after 5s**
3. Fill the bin until distance reading indicates 90% → **Auto-locks**
4. Try pressing "Open Lid" → Should be **blocked**
5. Unlock manually → Remote commands work
6. Trigger events → Watch LED + logs + timestamps update

---

## 🎓 Academic Context

This project was developed for the
**Emerging Trends in Computer Science (2nd Year)** module
at the **University of Jaffna**.

It demonstrates core IoT concepts:

* Embedded systems
* Sensor fusion
* Pub/Sub architecture
* Real-time dashboards
* Automation & control systems

---

## 👥 Team Members

* **Vishmitha** (Team Lead)
* **Pamuda**
* **Vihanga**
* **Sudula**
* **Maleesha**

**Software:** ESP32 firmware, MQTT logic, Node-RED dashboard
**Hardware:** Wiring, electronics assembly, bin construction

---

## 📜 License

Licensed under the **MIT License**.
See the `LICENSE` file for details.

