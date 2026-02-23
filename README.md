# Smart Water Bottle — IoT Temperature Controller

An ESP32-powered, dual-mode temperature-controlled portable bottle simulation. The ESP32 runs a closed-loop on/off controller with deadband in simulation mode (and can read a DS18B20 sensor on real hardware). The device publishes telemetry to Firebase Realtime Database and reads a remote setpoint from Firebase; a modern web dashboard allows live monitoring and remote control via a slider.

---

## Table of Contents

- [Project Overview](#project-overview)
- [Features](#features)
- [Design Decisions](#design-decisions)
- [Repo Structure](#repo-structure)
- [Firebase Data Model](#firebase-data-model)
- [Getting Started](#getting-started)
  - [Prerequisites](#prerequisites)
  - [Clone the Repo](#clone-the-repo)
  - [Firmware: PlatformIO + Wokwi](#firmware-platformio--wokwi)
  - [Firebase Setup](#firebase-setup)
  - [Web Dashboard](#web-dashboard)
- [Hardware Wiring](#hardware-wiring)
- [Firmware Logic](#firmware-logic)
- [Web Dashboard Logic](#web-dashboard-logic)
- [Testing & Acceptance Criteria](#testing--acceptance-criteria)

---

## Project Overview

Smart Bottle is a bi-directional IoT demo built on the ESP32. It maintains a user-set liquid temperature (hot or cold) using a software thermal model in simulation mode. On real hardware it can read a physical DS18B20 temperature probe. The system supports both local control (OLED display + 3 buttons) and remote control (Firebase Realtime Database + web dashboard).

---

## Features

- **Dual-mode firmware** — `SIM` mode uses a software thermal model for deterministic demos; `REAL` mode reads a DS18B20 sensor. Toggle via button or Firebase.
- **Closed-loop control** — On/off controller with deadband. Designed to be swapped later for PID or adaptive control.
- **Local UI** — SSD1306 OLED display and three tactile buttons (Up / Down / Mode) show current temperature and setpoint.
- **Bi-directional IoT** — Device → Cloud (telemetry via HTTP PUT) and Cloud → Device (setpoint via HTTP GET), both over Firebase Realtime Database.
- **Web dashboard** — Static HTML/CSS/JS page with a live-updating slider for remote setpoint control.
- **Safety & realism** — Simulation enforces safe temperature ranges; firmware is prepared for hardware safety limits (hard max/min, current sense).

---

## Design Decisions

| Choice                                     | Rationale                                                                                         |
| ------------------------------------------ | ------------------------------------------------------------------------------------------------- |
| **ESP32**                                  | Built-in Wi-Fi + BLE, ample PWM and ADC pins, widely supported by PlatformIO and Wokwi            |
| **Dual-mode firmware (SIM / REAL)**        | Allows deterministic demos while staying ready for real hardware deployment                       |
| **DS18B20 sensor**                         | Simple waterproof OneWire probe; PT100 is an option if higher accuracy is needed                  |
| **SSD1306 OLED + 3 buttons**               | Minimal, clear local UI for demo and testing                                                      |
| **Wokwi + PlatformIO**                     | Wokwi for visual simulation; PlatformIO for robust builds inside VS Code                          |
| **Firebase Realtime Database (HTTP REST)** | Lightweight HTTP PUT/GET integration minimises firmware footprint — no heavy SDK libraries needed |
| **Static web dashboard**                   | No backend required; free hosting via Firebase Hosting or GitHub Pages                            |

---

## Repo Structure

```
Embedded_Project/                       ← repo root
├── README.md
├── LICENSE
├── .gitignore
├── smart-water-bottle/                 ← PlatformIO firmware & Wokwi simulation
│   ├── platformio.ini
│   ├── diagram.json
│   ├── wokwi.toml
│   ├── include/
│   │   └── README
│   ├── lib/
│   │   └── README
│   └── src/
│       └── main.cpp                    ← firmware entry point
├── smart-bottle-dashboard/             ← static web app (HTML/CSS/JS)
│   ├── index.html
│   ├── css/
│   │   └── style.css
│   ├── js/
│   │   └── app.js
│   └── assets/
├── docs/
│   ├── diagrams/
│   ├── BOM.md
│   └── test-plan.md
└── demo/
    ├── demo_script.md
    └── demo_notes.md
```

---

## Firebase Data Model

The project uses a small, flat schema designed for easy maintenance:

```
bottle/
  telemetry/
    temperature   ← number  (current measured or simulated temperature)
    setpoint      ← number  (active setpoint on device)
    heater        ← 0 | 1
    cooler        ← 0 | 1
    mode          ← "SIM" | "REAL"
    ts            ← (optional) unix seconds
  control/
    setpoint      ← number  (written by web dashboard or buttons)
    mode          ← "SIM" | "REAL"  (optional)
```

**Rules:**

- The device writes to `bottle/telemetry` (HTTP PUT).
- The device reads `bottle/control/setpoint` periodically (throttled, every 5 s).
- The web dashboard writes to `bottle/control/setpoint`.
- Button presses also write to `bottle/control/setpoint` so the cloud and all other clients remain consistent.

---

## Getting Started

### Prerequisites

- [VS Code](https://code.visualstudio.com/) with the [PlatformIO extension](https://platformio.org/)
- [Wokwi VS Code extension](https://marketplace.visualstudio.com/items?itemName=wokwi.wokwi-vscode)
- Git
- Python 3 (optional — for running a local web server)
- Node.js & npm (optional — only if hosting with Firebase CLI)
- A Firebase account and project

---

### Clone the Repo

```bash
git clone https://github.com/Nagasai-Datta/iot-smart-water-bottle.git Embedded_Project
cd Embedded_Project
```

---

### Firmware: PlatformIO + Wokwi

1. Open the `Embedded_Project/smart-water-bottle/` folder in VS Code.
2. Build the firmware via **PlatformIO → Build** (or run `pio run` in the terminal).
3. To start the Wokwi simulation, open the Wokwi extension panel. If you have just rebuilt, stop any running simulation first, rebuild, then start it again so Wokwi picks up the latest binary.
4. Open the **Serial Monitor** at **115200 baud** to observe debug output:
   - Wi-Fi connection logs
   - Telemetry PUT responses (expect HTTP 200)
   - Button-press messages
   - Firebase setpoint reads

---

### Firebase Setup

1. Create a new Firebase project in the [Firebase Console](https://console.firebase.google.com/) and choose your region.
2. Enable **Realtime Database** and create the following initial path with a numeric value (e.g., `50`):
   ```
   bottle/control/setpoint
   ```
3. For a quick demo you can set open security rules. For any production use, restrict access with proper Firebase Authentication rules.
4. Note your **Database URL** — it will look like:
   ```
   https://<your-project-id>-default-rtdb.firebaseio.com/
   ```
   or (for some regions):
   ```
   https://<your-project-id>.<region>.firebasedatabase.app/
   ```
5. Add this URL to the relevant constant in `src/main.cpp` and in `js/app.js`.

---

### Web Dashboard

To test locally, run a simple HTTP server from the dashboard folder:

```bash
cd smart-bottle-dashboard
python3 -m http.server 8000
```

Then open [http://localhost:8000](http://localhost:8000) in your browser.

Move the slider and verify that `bottle/control/setpoint` updates in the Firebase Console in real time.

The dashboard is already live and publicly accessible at:

**[https://nagasai-datta.github.io/smart-bottle-dashboard/](https://nagasai-datta.github.io/smart-bottle-dashboard/)**

To redeploy updates, push changes to the `smart-bottle-dashboard` repo and GitHub Pages will rebuild automatically.

---

## Hardware Wiring

> This section applies when moving from Wokwi simulation to a physical build.

| Signal                         | ESP32 GPIO                     |
| ------------------------------ | ------------------------------ |
| SSD1306 SDA                    | GPIO 21                        |
| SSD1306 SCL                    | GPIO 22                        |
| DS18B20 DQ (OneWire)           | GPIO 4 (4.7 kΩ pull-up to 3V3) |
| Button UP                      | GPIO 18 (INPUT_PULLUP, to GND) |
| Button DOWN                    | GPIO 19 (INPUT_PULLUP, to GND) |
| Button OK / Mode               | GPIO 23 (INPUT_PULLUP, to GND) |
| Heater indicator / MOSFET gate | GPIO 26 (via resistor to GND)  |
| Cooler indicator / MOSFET gate | GPIO 27 (via resistor to GND)  |

**Power:** SSD1306 VCC and DS18B20 VCC → 3V3. All GND pins → common GND.

> **Physical Peltier build note:** If building real heating/cooling hardware, add proper power electronics — MOSFET H-bridges or low-Rds MOSFETs, current limiting, heat sinks, fan, battery pack, charger, and battery fuel gauge. Include safety provisions: thermal cutoff, PTC fuses, and battery protection circuitry.

---

## Firmware Logic

```
Boot
 └─ Init: WiFi · I2C OLED · OneWire (DS18B20) · HTTP client

Main loop (~0.8 – 1 s)
 ├─ Read buttons (debounced)
 │    └─ On setpoint change → write bottle/control/setpoint to Firebase
 ├─ Every 5 s → GET bottle/control/setpoint from Firebase and apply if changed
 ├─ Select temperature source
 │    ├─ SIM mode  → use simTemperature (software thermal model)
 │    └─ REAL mode → read DS18B20
 ├─ Controller: error = setpoint − measuredTemp
 │    ├─ error >  0.5 → heater ON
 │    ├─ error < −0.5 → cooler ON
 │    └─ SIM mode only → update simTemperature (heater/cooler effect + ambient leakage)
 ├─ Update OLED and Serial output
 └─ HTTP PUT telemetry → bottle/telemetry

OK button (long press / dedicated) → toggle SIM / REAL mode
```

> DS18B20 failure fallback: if the sensor read fails in REAL mode, the device falls back to SIM mode and optionally sets a `sensor_error` flag in telemetry.

---

## Web Dashboard Logic

- Uses the **Firebase Realtime Database JS SDK** with `on('value', ...)` to listen to `bottle/telemetry` and update all UI fields live.
- The slider's `onchange` handler writes to `bottle/control/setpoint` immediately.
- Optional mode buttons can write to `bottle/control/mode` to toggle the device between SIM and REAL remotely.

---

## Testing & Acceptance Criteria

| ID  | Test                 | Pass Condition                                                                                                                                 |
| --- | -------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------- |
| T1  | **Telemetry**        | `bottle/telemetry` updates every ~1 s; all HTTP responses are 200                                                                              |
| T2  | **Remote control**   | Changing `bottle/control/setpoint` in Firebase or the dashboard is reflected on the OLED within 5 s                                            |
| T3  | **Local buttons**    | UP / DOWN updates the OLED setpoint immediately and writes the new value to `bottle/control/setpoint` in Firebase                              |
| T4  | **Sync convergence** | If cloud and button both change setpoint in quick succession, the most recent action wins and both cloud and device converge to the same value |
| T5  | **Dashboard UI**     | Dashboard displays temperature, setpoint, mode, and heater/cooler states correctly and updates live                                            |
| T6  | **Sensor fallback**  | If the DS18B20 is disconnected in REAL mode, the device falls back to SIM mode and optionally reports the error in telemetry                   |

---

_Project developed using PlatformIO, Wokwi, Firebase Realtime Database, and vanilla HTML/CSS/JS._
