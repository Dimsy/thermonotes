# ESP32 Thermal Printer Web Server

![ESP32 Thermal Printer Project](https://img.shields.io/badge/ESP32-Thermal%20Printer%20Web%20Server-blue?style=for-the-badge&logo=arduino)
![WiFi Enabled](https://img.shields.io/badge/WiFi-Enabled-green?style=for-the-badge&logo=wifi)
![NTP Sync](https://img.shields.io/badge/Time-NTP%20Sync-orange?style=for-the-badge&logo=clock)
![Weather API](https://img.shields.io/badge/Weather-Open--Meteo%20API-brightgreen?style=for-the-badge&logo=cloud)

A sophisticated ESP32-based web server that controls a thermal printer, displays real-time weather information, and supports multiple simultaneous client connections with a beautiful web interface.

## 🌟 Features

### 🖨️ Printing Capabilities
- **Message Printing**: Send custom messages to thermal printer
- **Automatic Cyrillic Transliteration**: Converts Russian text to Latin characters for printing
- **Weather Reports**: Automatic daily weather printing at 10:00 AM
- **Message History**: Stores last 10 messages with timestamps

### 🌤️ Weather Integration
- **Real-time Moscow Weather**: Current temperature and conditions
- **Weather Forecast**: Temperature predictions for 15:00 and 19:00
- **Open-Meteo API**: Free weather data without API keys
- **Automatic Daily Reports**: Scheduled printing at 10:00 AM

### 🌐 Web Interface
- **Responsive Design**: Beautiful, mobile-friendly interface
- **Real-time Updates**: Auto-refreshing message history
- **Multiple Client Support**: Handles simultaneous connections
- **Moscow Time Display**: Accurate time synchronization via NTP

### ⚡ Technical Features
- **Multi-threaded Architecture**: Web server runs on separate core
- **Thread-safe Operations**: Protected resource access with semaphores
- **WiFi Management**: Stable connection handling
- **Hardware Serial**: Reliable communication with thermal printer

## 🛠️ Hardware Requirements

### Components
- **ESP32 Development Board**
- **Thermal Printer** (compatible with 9600 baud UART)
- **Jumper Wires**
- **Power Supply** (appropriate for your thermal printer)

### Pin Configuration
| Component | ESP32 Pin |
|-----------|-----------|
| Printer TX | GPIO 17 |
| Printer RX | GPIO 16 |

## 📋 Software Requirements

### Arduino Libraries
- `WiFi.h` (included with ESP32)
- `WebServer.h` (included with ESP32)
- `NTPClient.h`
- `WiFiUdp.h` (included with ESP32)
- `HardwareSerial.h` (included with Arduino)
- `HTTPClient.h` (included with ESP32)
- `ArduinoJson.h`

### Platform
- **Arduino IDE** or **PlatformIO**
- **ESP32 Board Support**

## 🔧 Installation & Setup

### 1. Hardware Setup
```cpp
// Connect thermal printer to ESP32
#define PRINTER_TX_PIN 17  // ESP32 TX → Printer RX
#define PRINTER_RX_PIN 16  // ESP32 RX → Printer TX