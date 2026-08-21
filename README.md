# 📡 ESP8266 + nRF24L01 2.4GHz RF Signal Testing & Analysis Tool

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform: ESP8266](https://img.shields.io/badge/Platform-ESP8266-red.svg)](https://www.espressif.com)
[![Hardware: nRF24L01+](https://img.shields.io/badge/Module-nRF24L01%2B-blue.svg)](https://www.nordicsemi.com)

> ⚠️ **LEGAL DISCLAIMER & ETHICAL USE NOTICE**  
> This project is designed exclusively for **educational, academic research, RF testing, and authorized laboratory environments**. Transmitting unauthorized interference or jamming signals on public or private RF spectrum bands is illegal and strictly prohibited by telecommunications laws (such as FCC, CE, BTK). The author assumes no liability for misuse.

---

## 🎯 1. Overview
This project pairs an **ESP8266** microcontroller with an **nRF24L01+** 2.4GHz transceiver module to provide an embedded RF testbed and spectral monitoring tool controlled via a responsive Web Interface.

### Core Features:
- **Embedded Web GUI:** Host a local Wi-Fi Access Point (AP) with an interactive control dashboard.
- **Multi-Channel RF Testing:** Supports full 2.400 GHz – 2.525 GHz sweep (Channels 0–125).
- **Multiple Test Modes:** Single carrier frequency test, hopping carrier mode, and full-spectrum sweeps.
- **Power Level Management:** Dynamically configure RF output power (`MIN`, `LOW`, `HIGH`, `MAX`).
- **Real-time Status Monitoring:** Live diagnostics, active channel indicators, and safety timers.

---

## 🔌 2. Hardware Pinout & Wiring

| nRF24L01+ Pin | ESP8266 (NodeMCU / D1 Mini) | Description |
|---|---|---|
| **VCC** | 3.3V *(Requires 10µF–100µF capacitor across VCC/GND)* | Power Supply |
| **GND** | GND | Ground |
| **CSN** | D8 (GPIO15) | SPI Chip Select Not |
| **CE** | D4 (GPIO2) | Chip Enable |
| **MOSI** | D7 (GPIO13) | SPI Data In |
| **SCK** | D5 (GPIO14) | SPI Clock |
| **MISO** | D6 (GPIO12) | SPI Data Out |

---

## 🚀 3. Installation & Flashing

1. Open Arduino IDE or PlatformIO.
2. Install required libraries: `ESP8266WiFi`, `ESP8266WebServer`, and `RF24` (by TMRh20).
3. Connect your ESP8266 board via USB and select the appropriate COM port.
4. Compile and flash the sketch.
5. Connect to the Wi-Fi AP:
   - **SSID:** `RF-Test-Lab`
   - **IP Address:** `http://192.168.4.1`

---

## 📜 4. License
Licensed under the [MIT License](LICENSE).  
Developer: **Toprak Ahmet Aydoğmuş**.
