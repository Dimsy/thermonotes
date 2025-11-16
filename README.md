ESP32 Thermal Printer Web Server
https://img.shields.io/badge/ESP32-Thermal%2520Printer%2520Web%2520Server-blue?style=for-the-badge&logo=arduino
https://img.shields.io/badge/WiFi-Enabled-green?style=for-the-badge&logo=wifi
https://img.shields.io/badge/Time-NTP%2520Sync-orange?style=for-the-badge&logo=clock
https://img.shields.io/badge/Weather-Open--Meteo%2520API-brightgreen?style=for-the-badge&logo=cloud

A sophisticated ESP32-based web server that controls a thermal printer, displays real-time weather information, and supports multiple simultaneous client connections with a beautiful web interface.

🌟 Features
🖨️ Printing Capabilities
Message Printing: Send custom messages to thermal printer

Automatic Cyrillic Transliteration: Converts Russian text to Latin characters for printing

Weather Reports: Automatic daily weather printing at 10:00 AM

Message History: Stores last 10 messages with timestamps

🌤️ Weather Integration
Real-time Moscow Weather: Current temperature and conditions

Weather Forecast: Temperature predictions for 15:00 and 19:00

Open-Meteo API: Free weather data without API keys

Automatic Daily Reports: Scheduled printing at 10:00 AM

🌐 Web Interface
Responsive Design: Beautiful, mobile-friendly interface

Real-time Updates: Auto-refreshing message history

Multiple Client Support: Handles simultaneous connections

Moscow Time Display: Accurate time synchronization via NTP

⚡ Technical Features
Multi-threaded Architecture: Web server runs on separate core

Thread-safe Operations: Protected resource access with semaphores

WiFi Management: Stable connection handling

Hardware Serial: Reliable communication with thermal printer

🛠️ Hardware Requirements
Components
ESP32 Development Board

Thermal Printer (compatible with 9600 baud UART)

Jumper Wires

Power Supply (appropriate for your thermal printer)

Pin Configuration
Component	ESP32 Pin
Printer TX	GPIO 17
Printer RX	GPIO 16
📋 Software Requirements
Arduino Libraries
WiFi.h (included with ESP32)

WebServer.h (included with ESP32)

NTPClient.h

WiFiUdp.h (included with ESP32)

HardwareSerial.h (included with Arduino)

HTTPClient.h (included with ESP32)

ArduinoJson.h

Platform
Arduino IDE or PlatformIO

ESP32 Board Support

🔧 Installation & Setup
1. Hardware Setup
cpp
// Connect thermal printer to ESP32
#define PRINTER_TX_PIN 17  // ESP32 TX → Printer RX
#define PRINTER_RX_PIN 16  // ESP32 RX → Printer TX
2. Software Configuration
Install required libraries through Arduino Library Manager

Configure WiFi credentials in the code:

cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
3. Upload Code
Select ESP32 board in Arduino IDE

Choose correct COM port

Upload the sketch

🚀 Usage
Web Interface Access
Power on the ESP32

Wait for WiFi connection (check Serial Monitor)

Note the IP address displayed in Serial Monitor

Open web browser and navigate to: http://[ESP32_IP_ADDRESS]

Web Interface Features
📝 Send Messages
Type messages in the text input field

Click "Отправить" to print and save to history

Cyrillic characters automatically transliterated

🌡️ Weather Information
Current Weather: Real-time Moscow weather conditions

Manual Print: Click "Печать погоды и даты" for immediate weather report

Automatic Print: Daily weather report at 10:00 AM

📚 Message History
View last 10 sent messages

Re-print any message from history

Real-time updates every 3 seconds

📊 API Endpoints
Endpoint	Method	Description
/	GET	Main web interface
/submit	POST	Submit new message
/history	GET	Get message history (AJAX)
/print	GET	Print specific message
/print-weather	GET	Print weather report
🔄 Automatic Features
Scheduled Printing
Weather Reports: Automatically prints at 10:00 AM daily

Date Reset: Resets print flag at 00:01 daily

Time Sync: Regular NTP time synchronization

System Monitoring
WiFi Status: Continuous connection monitoring

Time Updates: Periodic NTP synchronization

Resource Protection: Thread-safe operations

🎨 Web Interface Preview
The web interface features:

Modern CSS Design with shadows and gradients

Responsive Layout for mobile and desktop

Real-time Moscow Time display

Interactive Buttons with hover effects

Auto-refreshing message history

Weather Information section

Automatic Print status indicator

📝 Code Structure
Main Components
Web Server: Handles HTTP requests (separate thread)

Thermal Printer: Manages printing operations

NTP Client: Time synchronization

Weather API: Open-Meteo integration

Message History: Thread-safe storage

Key Functions
transliterate(): Cyrillic to Latin conversion

printToThermalPrinter(): Printer communication

getCurrentWeather(): Weather data fetching

addMessageToHistory(): Thread-safe history management

checkAutoPrint(): Scheduled printing logic

🔒 Thread Safety
The implementation uses FreeRTOS features for safe multi-client operation:

Semaphores: Protect shared resources (message history)

Dedicated Core: Web server runs on Core 0

Non-blocking: Main loop continues during web operations

🐛 Troubleshooting
Common Issues
Printer not responding

Check TX/RX connections

Verify baud rate (9600)

Ensure adequate power supply

WiFi connection failed

Verify SSID and password

Check WiFi signal strength

Restart ESP32

Time not syncing

Check internet connection

Verify NTP server accessibility

Wait for automatic retry

Serial Monitor Debugging
The ESP32 provides detailed status information:

WiFi connection progress

NTP time synchronization

Printing operations

Error messages

📈 Future Enhancements
Potential improvements:

WebSocket support for real-time updates

Printer status monitoring

Multiple language support

Advanced weather metrics

Print queue management

Mobile app integration

🤝 Contributing
Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

📄 License
This project is open source and available under the MIT License.

🙏 Acknowledgments
Open-Meteo for free weather API

ESP32 Community for excellent documentation

Arduino for the development platform

Note: Remember to update WiFi credentials and verify hardware connections before deployment. For optimal performance, ensure stable WiFi connection and adequate power supply for the thermal printer.

Happy Printing! 🖨️✨