#include <WiFi.h>
#include <WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <HardwareSerial.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Замените на свои учетные данные Wi-Fi
const char* ssid = "test";
const char* password = "test";

WebServer server(80);

// Настройка NTP клиента для московского времени
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 10800, 60000);

// Настройка Serial для термопринтера
HardwareSerial ThermalPrinter(2);

// Пины для термопринтера
#define PRINTER_TX_PIN 17
#define PRINTER_RX_PIN 16

// Координаты Москвы для Open-Meteo
const float MOSCOW_LAT = 55.7558;
const float MOSCOW_LON = 37.6173;

// Структура для хранения сообщения и времени
struct Message {
  String text;
  String timestamp;
};

// Массив для хранения истории (максимум 10 сообщений)
Message messageHistory[10];
int historyCount = 0;

// Переменные для автоматической печати
unsigned long lastPrintCheck = 0;
bool todayPrinted = false;
String lastPrintDate = "";

// Семафор для защиты общих ресурсов
SemaphoreHandle_t xSemaphore;

// Переменные для управления WiFi
unsigned long lastWifiCheck = 0;
const unsigned long WIFI_CHECK_INTERVAL = 30000;
bool wifiConnected = false;
int wifiReconnectAttempts = 0;
const int MAX_RECONNECT_ATTEMPTS = 5;

// Переменные для кэширования
String cachedHomePage = "";
unsigned long lastCacheUpdate = 0;
const unsigned long CACHE_UPDATE_INTERVAL = 30000;

// HTML страница с формой и историей (упрощенная версия)
const char* htmlPage = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Printer</title>
  <style>
    body { 
      font-family: Arial, sans-serif; 
      text-align: center; 
      margin: 10px;
      background: #f0f0f0;
    }
    .container {
      background: white;
      padding: 15px;
      border-radius: 8px;
      box-shadow: 0 2px 4px rgba(0,0,0,0.1);
      max-width: 500px;
      margin: 0 auto;
    }
    .form-section {
      margin-bottom: 20px;
      padding: 15px;
      border-bottom: 1px solid #ddd;
    }
    .history-section {
      text-align: left;
    }
    input[type="text"] {
      width: 65%;
      padding: 10px;
      margin: 5px 0;
      border: 1px solid #ccc;
      border-radius: 4px;
      font-size: 14px;
    }
    input[type="submit"] {
      background: #4CAF50;
      color: white;
      padding: 10px 20px;
      border: none;
      border-radius: 4px;
      cursor: pointer;
      font-size: 14px;
    }
    .print-btn {
      background: #2196F3;
      color: white;
      padding: 5px 10px;
      border: none;
      border-radius: 3px;
      cursor: pointer;
      font-size: 11px;
      margin-left: 8px;
    }
    .weather-btn {
      background: #FF9800;
      color: white;
      padding: 10px 20px;
      border: none;
      border-radius: 4px;
      cursor: pointer;
      font-size: 14px;
      margin: 8px 5px;
    }
    .history-item {
      background: #f8f9fa;
      margin: 8px 0;
      padding: 10px;
      border-radius: 4px;
      border-left: 3px solid #4CAF50;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    .message-content {
      flex-grow: 1;
    }
    .history-header {
      color: #333;
      margin-bottom: 15px;
      font-size: 18px;
    }
    .empty-history {
      color: #666;
      font-style: italic;
      padding: 15px;
    }
    .timestamp {
      color: #888;
      font-size: 11px;
      margin-top: 3px;
    }
    .message-text {
      color: #333;
      font-size: 14px;
      word-break: break-word;
    }
    .current-time {
      color: #666;
      font-size: 14px;
      margin-bottom: 15px;
    }
    .info-note {
      color: #666;
      font-size: 11px;
      margin-top: 8px;
    }
    .input-group {
      display: flex;
      justify-content: center;
      align-items: center;
      gap: 8px;
      flex-wrap: wrap;
    }
    .weather-section {
      margin: 15px 0;
      padding: 15px;
      background: #e3f2fd;
      border-radius: 6px;
    }
    .auto-print-info {
      background: #e8f5e8;
      padding: 8px;
      border-radius: 4px;
      margin: 8px 0;
      font-size: 12px;
    }
    .weather-display {
      font-size: 18px;
      margin: 10px 0;
      padding: 10px;
      background: rgba(255, 255, 255, 0.8);
      border-radius: 6px;
    }
    .weather-icon {
      font-size: 32px;
      margin: 5px 0;
    }
    .temperature {
      font-size: 24px;
      font-weight: bold;
      color: #2196F3;
    }
    .forecast-item {
      display: inline-block;
      margin: 0 10px;
      padding: 8px;
      background: rgba(255, 255, 255, 0.9);
      border-radius: 4px;
    }
    .wifi-status {
      margin: 8px 0;
      padding: 6px;
      border-radius: 4px;
      font-size: 12px;
      font-weight: bold;
    }
    .wifi-connected {
      background: #e8f5e8;
      color: #2e7d32;
    }
    .wifi-disconnected {
      background: #ffebee;
      color: #c62828;
    }
    h1 {
      color: #333;
      margin-bottom: 8px;
      font-size: 1.8em;
    }
    h3 {
      color: #555;
      margin-bottom: 10px;
      font-size: 16px;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>🌤️ ESP32 Принтер</h1>
    <div class="current-time" id="currentTime"></div>
    
    <div class="wifi-status" id="wifiStatus">
      <span id="wifiIcon">📶</span>
      <span id="wifiText">Проверка...</span>
    </div>
    
    <div class="auto-print-info">
      <strong>Автопечать:</strong> погода печатается каждый день в 10:00
    </div>
    
    <div class="weather-section">
      <h3>Погода в Москве</h3>
      <div class="weather-display">
        <div class="weather-icon" id="weatherIcon">⏳</div>
        <div class="temperature" id="weatherTemp">Загрузка...</div>
      </div>
      <div class="forecast">
        <div class="forecast-item">
          <div>15:00</div>
          <div id="temp15">--°C</div>
        </div>
        <div class="forecast-item">
          <div>19:00</div>
          <div id="temp19">--°C</div>
        </div>
      </div>
      <button class="weather-btn" onclick="printWeather()">🖨️ Печать погоды</button>
    </div>
    
    <div class="form-section">
      <h3>Отправить сообщение</h3>
      <form action="/submit" method="POST" id="messageForm">
        <div class="input-group">
          <input type="text" name="inputValue" id="inputValue" placeholder="Введите сообщение..." required>
          <input type="submit" value="📤 Отправить">
        </div>
      </form>
      <div class="info-note">Кириллица преобразуется в латиницу на принтере</div>
    </div>

    <div class="history-section">
      <h3 class="history-header">История сообщений</h3>
      <div id="historyList">
        )rawliteral";

// Вторая часть HTML (упрощенная)
String getHtmlPageEnd() {
  return R"rawliteral(
      </div>
    </div>
  </div>
  
  <script>
    function updateCurrentTime() {
      const now = new Date();
      const timeString = now.toLocaleString('ru-RU', {
        timeZone: 'Europe/Moscow',
        year: 'numeric',
        month: '2-digit',
        day: '2-digit',
        hour: '2-digit',
        minute: '2-digit'
      });
      document.getElementById('currentTime').textContent = 'Время: ' + timeString;
    }
    
    function updateWifiStatus() {
      fetch('/wifi-status')
        .then(response => response.json())
        .then(data => {
          const wifiStatus = document.getElementById('wifiStatus');
          const wifiIcon = document.getElementById('wifiIcon');
          const wifiText = document.getElementById('wifiText');
          
          if (data.connected) {
            wifiStatus.className = 'wifi-status wifi-connected';
            wifiIcon.textContent = '📶';
            wifiText.textContent = 'WiFi: ' + data.ip;
          } else {
            wifiStatus.className = 'wifi-status wifi-disconnected';
            wifiIcon.textContent = '❌';
            wifiText.textContent = 'Нет WiFi';
          }
        })
        .catch(error => {
          console.error('WiFi status error:', error);
        });
    }
    
    function printMessage(index) {
      fetch('/print?index=' + index)
        .then(response => response.text())
        .then(result => {
          alert(result);
        });
    }
    
    function printWeather() {
      fetch('/print-weather')
        .then(response => response.text())
        .then(result => {
          alert(result);
        });
    }
    
    function updateWeather() {
      fetch('/weather-data')
        .then(response => response.json())
        .then(data => {
          document.getElementById('weatherIcon').textContent = data.icon;
          document.getElementById('weatherTemp').textContent = data.temperature;
          document.getElementById('temp15').textContent = data.temp15 + '°C';
          document.getElementById('temp19').textContent = data.temp19 + '°C';
        })
        .catch(error => {
          console.error('Weather error:', error);
        });
    }
    
    // Обновляем время каждые 30 секунд
    updateCurrentTime();
    setInterval(updateCurrentTime, 30000);
    
    // Обновляем статус WiFi каждые 10 секунд
    updateWifiStatus();
    setInterval(updateWifiStatus, 10000);
    
    // Обновляем погоду каждые 10 минут
    updateWeather();
    setInterval(updateWeather, 600000);
    
    // Обновляем историю каждые 5 секунд
    setInterval(function() {
      fetch('/history')
        .then(response => response.text())
        .then(html => {
          document.getElementById('historyList').innerHTML = html;
        });
    }, 5000);
  </script>
</body>
</html>
)rawliteral";
}

// Функция для получения кэшированной главной страницы
String getCachedHomePage() {
  if (cachedHomePage == "" || millis() - lastCacheUpdate > CACHE_UPDATE_INTERVAL) {
    // Обновляем кэш
    cachedHomePage = String(htmlPage);
    cachedHomePage += getHistoryHTML();
    cachedHomePage += getHtmlPageEnd();
    lastCacheUpdate = millis();
    Serial.println("🔄 Кэш главной страницы обновлен");
  }
  return cachedHomePage;
}

// Функция инициализации термопринтера
void initThermalPrinter() {
  ThermalPrinter.begin(9600, SERIAL_8N1, PRINTER_RX_PIN, PRINTER_TX_PIN);
  delay(2000);
  Serial.println("Thermal printer UART initialized at 9600 baud");
  ThermalPrinter.println("Printer initialized");
  delay(500);
  Serial.println("Thermal printer ready");
}

// Функция для подключения к WiFi с повторными попытками
bool connectToWiFi() {
  Serial.println("Подключение к WiFi...");
  Serial.print("SSID: ");
  Serial.println(ssid);
  
  WiFi.disconnect(true);
  delay(1000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(1000);
    Serial.print(".");
    attempts++;
    if (attempts % 10 == 0) {
      Serial.println();
    }
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Успешное подключение к WiFi!");
    Serial.print("📡 IP адрес: ");
    Serial.println(WiFi.localIP());
    wifiConnected = true;
    wifiReconnectAttempts = 0;
    
    // Печатаем сообщение о подключении
    // String wifiMessage = "Uspeshnoe podklyuchenie k WiFi! IP: " + WiFi.localIP().toString();
    // printToThermalPrinter(wifiMessage);
    
    return true;
  } else {
    Serial.println("\n❌ Не удалось подключиться к WiFi");
    wifiConnected = false;
    return false;
  }
}

// Функция проверки и восстановления WiFi соединения
void checkWiFiConnection() {
  if (millis() - lastWifiCheck > WIFI_CHECK_INTERVAL) {
    lastWifiCheck = millis();
    
    if (WiFi.status() != WL_CONNECTED) {
      wifiConnected = false;
      Serial.println("❌ Потеряно соединение с WiFi");
      
      if (wifiReconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
        wifiReconnectAttempts++;
        Serial.print("🔄 Попытка переподключения #");
        Serial.println(wifiReconnectAttempts);
        
        if (connectToWiFi()) {
          Serial.println("✅ WiFi соединение восстановлено");
        } else {
          Serial.println("❌ Не удалось восстановить WiFi соединение");
        }
      } else {
        Serial.println("⚠️ Достигнуто максимальное количество попыток переподключения");
        Serial.println("🔄 Перезагрузка ESP32 через 10 секунд...");
        delay(10000);
        ESP.restart();
      }
    } else {
      if (!wifiConnected) {
        wifiConnected = true;
        Serial.println("✅ WiFi соединение активно");
      }
    }
  }
}

// Функция транслитерации кириллицы в латиницу
String transliterate(String text) {
  String result = "";
  
  for (int i = 0; i < text.length(); i++) {
    char c = text[i];
    
    if ((c & 0xE0) == 0xC0 && i + 1 < text.length()) {
      unsigned char c1 = text[i];
      unsigned char c2 = text[i + 1];
      unsigned int unicode = ((c1 & 0x1F) << 6) | (c2 & 0x3F);
      
      switch (unicode) {
        // Строчные русские буквы
        case 0x430: result += "a"; i++; break;
        case 0x431: result += "b"; i++; break;
        case 0x432: result += "v"; i++; break;
        case 0x433: result += "g"; i++; break;
        case 0x434: result += "d"; i++; break;
        case 0x435: result += "e"; i++; break;
        case 0x451: result += "e"; i++; break;
        case 0x436: result += "zh"; i++; break;
        case 0x437: result += "z"; i++; break;
        case 0x438: result += "i"; i++; break;
        case 0x439: result += "y"; i++; break;
        case 0x43A: result += "k"; i++; break;
        case 0x43B: result += "l"; i++; break;
        case 0x43C: result += "m"; i++; break;
        case 0x43D: result += "n"; i++; break;
        case 0x43E: result += "o"; i++; break;
        case 0x43F: result += "p"; i++; break;
        case 0x440: result += "r"; i++; break;
        case 0x441: result += "s"; i++; break;
        case 0x442: result += "t"; i++; break;
        case 0x443: result += "u"; i++; break;
        case 0x444: result += "f"; i++; break;
        case 0x445: result += "kh"; i++; break;
        case 0x446: result += "ts"; i++; break;
        case 0x447: result += "ch"; i++; break;
        case 0x448: result += "sh"; i++; break;
        case 0x449: result += "shch"; i++; break;
        case 0x44A: result += ""; i++; break;
        case 0x44B: result += "y"; i++; break;
        case 0x44C: result += ""; i++; break;
        case 0x44D: result += "e"; i++; break;
        case 0x44E: result += "yu"; i++; break;
        case 0x44F: result += "ya"; i++; break;
        
        // Заглавные русские буквы
        case 0x410: result += "A"; i++; break;
        case 0x411: result += "B"; i++; break;
        case 0x412: result += "V"; i++; break;
        case 0x413: result += "G"; i++; break;
        case 0x414: result += "D"; i++; break;
        case 0x415: result += "E"; i++; break;
        case 0x401: result += "E"; i++; break;
        case 0x416: result += "Zh"; i++; break;
        case 0x417: result += "Z"; i++; break;
        case 0x418: result += "I"; i++; break;
        case 0x419: result += "Y"; i++; break;
        case 0x41A: result += "K"; i++; break;
        case 0x41B: result += "L"; i++; break;
        case 0x41C: result += "M"; i++; break;
        case 0x41D: result += "N"; i++; break;
        case 0x41E: result += "O"; i++; break;
        case 0x41F: result += "P"; i++; break;
        case 0x420: result += "R"; i++; break;
        case 0x421: result += "S"; i++; break;
        case 0x422: result += "T"; i++; break;
        case 0x423: result += "U"; i++; break;
        case 0x424: result += "F"; i++; break;
        case 0x425: result += "Kh"; i++; break;
        case 0x426: result += "Ts"; i++; break;
        case 0x427: result += "Ch"; i++; break;
        case 0x428: result += "Sh"; i++; break;
        case 0x429: result += "Shch"; i++; break;
        case 0x42A: result += ""; i++; break;
        case 0x42B: result += "Y"; i++; break;
        case 0x42C: result += ""; i++; break;
        case 0x42D: result += "E"; i++; break;
        case 0x42E: result += "Yu"; i++; break;
        case 0x42F: result += "Ya"; i++; break;
        
        default: 
          result += "?"; 
          i++;
          break;
      }
    } else {
      result += c;
    }
  }
  
  return result;
}

// Функция для получения времени в формате HH:MM (только часы и минуты)
String getShortMoscowTime() {
  timeClient.update();
  String time = timeClient.getFormattedTime();
  return time.substring(0, 5);
}

// Функция для получения текущей даты в формате YYYY-MM-DD
String getCurrentDateString() {
  timeClient.update();
  time_t rawTime = timeClient.getEpochTime();
  struct tm *timeInfo;
  timeInfo = localtime(&rawTime);
  
  char dateStr[11];
  strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", timeInfo);
  return String(dateStr);
}

// Функция для получения названия дня недели на русском
String getDayOfWeek() {
  timeClient.update();
  time_t rawTime = timeClient.getEpochTime();
  struct tm *timeInfo;
  timeInfo = localtime(&rawTime);
  
  int dayOfWeek = timeInfo->tm_wday;
  
  switch(dayOfWeek) {
    case 0: return "Воскресенье";
    case 1: return "Понедельник";
    case 2: return "Вторник";
    case 3: return "Среда";
    case 4: return "Четверг";
    case 5: return "Пятница";
    case 6: return "Суббота";
    default: return "Неизвестно";
  }
}

// Функция для получения названия месяца на русском
String getMonthName() {
  timeClient.update();
  time_t rawTime = timeClient.getEpochTime();
  struct tm *timeInfo;
  timeInfo = localtime(&rawTime);
  
  int month = timeInfo->tm_mon;
  
  switch(month) {
    case 0: return "января";
    case 1: return "февраля";
    case 2: return "марта";
    case 3: return "апреля";
    case 4: return "мая";
    case 5: return "июня";
    case 6: return "июля";
    case 7: return "августа";
    case 8: return "сентября";
    case 9: return "октября";
    case 10: return "ноября";
    case 11: return "декабря";
    default: return "неизвестно";
  }
}

// Функция для получения текущей даты
String getCurrentDate() {
  timeClient.update();
  time_t rawTime = timeClient.getEpochTime();
  struct tm *timeInfo;
  timeInfo = localtime(&rawTime);
  
  return String(timeInfo->tm_mday);
}

// Функция для получения текущей погоды из Open-Meteo
String getCurrentWeather() {
  // Проверяем подключение к WiFi перед запросом
  if (WiFi.status() != WL_CONNECTED) {
    return "❓ Нет подключения к WiFi";
  }
  
  HTTPClient http;
  
  String url = "https://api.open-meteo.com/v1/forecast?";
  url += "latitude=" + String(MOSCOW_LAT, 6);
  url += "&longitude=" + String(MOSCOW_LON, 6);
  url += "&current=temperature_2m,weather_code";
  url += "&timezone=Europe/Moscow";
  
  http.begin(url);
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    
    float currentTemp = doc["current"]["temperature_2m"];
    int weatherCode = doc["current"]["weather_code"];
    
    http.end();
    
    String weatherIcon = getWeatherIcon(weatherCode);
    return weatherIcon + " " + String(currentTemp, 1) + "°C";
  } else {
    http.end();
    return "❓ Ошибка получения погоды";
  }
}

// Функция для получения иконки погоды по коду (для веб-интерфейса)
String getWeatherIcon(int weatherCode) {
  if (weatherCode == 0) return "☀️";
  else if (weatherCode == 1) return "🌤️";
  else if (weatherCode == 2) return "⛅";
  else if (weatherCode == 3) return "☁️";
  else if (weatherCode >= 45 && weatherCode <= 48) return "🌫️";
  else if (weatherCode >= 51 && weatherCode <= 55) return "🌧️";
  else if (weatherCode >= 56 && weatherCode <= 57) return "🌧️❄️";
  else if (weatherCode >= 61 && weatherCode <= 65) return "🌧️";
  else if (weatherCode >= 66 && weatherCode <= 67) return "🌧️❄️";
  else if (weatherCode >= 71 && weatherCode <= 75) return "❄️";
  else if (weatherCode == 77) return "🌨️";
  else if (weatherCode >= 80 && weatherCode <= 82) return "⛈️";
  else if (weatherCode >= 85 && weatherCode <= 86) return "🌨️";
  else if (weatherCode >= 95 && weatherCode <= 99) return "⛈️";
  else return "❓";
}

// Функция для получения ASCII иконки погоды для принтера (широкие иконки)
String getWeatherAsciiArt(int weatherCode) {
  if (weatherCode == 0) { // Ясно
    return 
    "     \\   /     \n"
    "      .-.      \n"
    "   -- (   ) -- \n"
    "      `-'      \n"
    "     /   \\     \n";
  }
  else if (weatherCode == 1) { // Преимущественно ясно
    return 
    "     \\  /      \n"
    "   _ /\"\".-.    \n"
    "     \\_(   ).  \n"
    "     /(___(__) \n";
  }
  else if (weatherCode == 2) { // Переменная облачность
    return 
    "    .-.        \n"
    " .-(    ).     \n"
    "(___.__)__)    \n";
  }
  else if (weatherCode == 3) { // Пасмурно
    return 
    "    .--.       \n"
    " .-(    ).     \n"
    "(___.__)__)    \n";
  }
  else if (weatherCode >= 45 && weatherCode <= 48) { // Туман
    return 
    " _ - _ - _ - _ \n"
    "  _ - _ - _ -  \n"
    "_ - _ - _ - _  \n";
  }
  else if (weatherCode >= 51 && weatherCode <= 67) { // Дождь
    return 
    "    .--.       \n"
    " .-(    ).     \n"
    "(___.__)__)    \n"
    "  '  '  '  '   \n";
  }
  else if (weatherCode >= 71 && weatherCode <= 86) { // Снег
    return 
    "    .--.       \n"
    " .-(    ).     \n"
    "(___.__)__)    \n"
    "  *  *  *  *   \n";
  }
  else if (weatherCode >= 95 && weatherCode <= 99) { // Гроза
    return 
    "    .--.       \n"
    " .-(    ).     \n"
    "(___.__)__)    \n"
    "   /\\    /\\    \n";
  }
  else { // Неизвестно
    return 
    "               \n"
    "   ???????     \n"
    "  ?       ?    \n"
    "   ???????     \n";
  }
}

// Функция для получения текстового описания погоды (для принтера)
String getWeatherDescription(int weatherCode) {
  if (weatherCode == 0) return "Clear";
  else if (weatherCode == 1) return "Mainly clear";
  else if (weatherCode == 2) return "Partly cloudy";
  else if (weatherCode == 3) return "Overcast";
  else if (weatherCode >= 45 && weatherCode <= 48) return "Fog";
  else if (weatherCode >= 51 && weatherCode <= 55) return "Drizzle";
  else if (weatherCode >= 56 && weatherCode <= 57) return "Freezing drizzle";
  else if (weatherCode >= 61 && weatherCode <= 65) return "Rain";
  else if (weatherCode >= 66 && weatherCode <= 67) return "Freezing rain";
  else if (weatherCode >= 71 && weatherCode <= 75) return "Snow";
  else if (weatherCode == 77) return "Snow grains";
  else if (weatherCode >= 80 && weatherCode <= 82) return "Rain showers";
  else if (weatherCode >= 85 && weatherCode <= 86) return "Snow showers";
  else if (weatherCode >= 95 && weatherCode <= 99) return "Thunderstorm";
  else return "Unknown";
}

// Функция для получения температуры на определенное время
float getTemperatureForTime(String targetTime) {
  // Проверяем подключение к WiFi перед запросом
  if (WiFi.status() != WL_CONNECTED) {
    return -999;
  }
  
  HTTPClient http;
  
  String url = "https://api.open-meteo.com/v1/forecast?";
  url += "latitude=" + String(MOSCOW_LAT, 6);
  url += "&longitude=" + String(MOSCOW_LON, 6);
  url += "&hourly=temperature_2m";
  url += "&timezone=Europe/Moscow";
  url += "&forecast_days=3";
  
  http.begin(url);
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String payload = http.getString();
    DynamicJsonDocument doc(4096);
    deserializeJson(doc, payload);
    
    JsonArray timeArray = doc["hourly"]["time"];
    JsonArray tempArray = doc["hourly"]["temperature_2m"];
    
    timeClient.update();
    time_t rawTime = timeClient.getEpochTime();
    struct tm *timeInfo;
    timeInfo = localtime(&rawTime);
    
    char today[11];
    strftime(today, sizeof(today), "%Y-%m-%d", timeInfo);
    
    for (size_t i = 0; i < timeArray.size(); i++) {
      String timeStr = timeArray[i].as<String>();
      if (timeStr.indexOf(String(today) + "T" + targetTime) != -1) {
        float temp = tempArray[i].as<float>();
        http.end();
        return temp;
      }
    }
    
    http.end();
    return -999;
  } else {
    http.end();
    return -999;
  }
}

// Функция печати текста на термопринтере (только латинские символы)
void printToThermalPrinter(String text) {
  String currentTime = getShortMoscowTime();
  String transliteratedText = transliterate(text);
  String printText = currentTime + " - " + transliteratedText;
  
  Serial.print("Printing: ");
  Serial.println(printText);
  
  ThermalPrinter.println(printText);
  delay(300);
  Serial.println("Print completed");
}

// Функция для печати информации о погоде и дате с ASCII графикой (только латинские символы)
void printWeatherInfo() {
  Serial.println("Printing weather information with ASCII art...");
  
  String dayOfWeek = transliterate(getDayOfWeek());
  String currentDate = getCurrentDate();
  String monthName = transliterate(getMonthName());
  
  // Получаем данные о погоде
  float currentTemp = -999;
  int weatherCode = -1;
  
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://api.open-meteo.com/v1/forecast?";
    url += "latitude=" + String(MOSCOW_LAT, 6);
    url += "&longitude=" + String(MOSCOW_LON, 6);
    url += "&current=temperature_2m,weather_code";
    url += "&timezone=Europe/Moscow";
    
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode == 200) {
      String payload = http.getString();
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);
      
      currentTemp = doc["current"]["temperature_2m"];
      weatherCode = doc["current"]["weather_code"];
    }
    http.end();
  }
  
  // Печатаем дату
  String dateLine = dayOfWeek + ", " + currentDate + " " + monthName;
  ThermalPrinter.println("Date: " + dateLine);
  ThermalPrinter.println("");
  
  // Печатаем ASCII арт погоды
  if (weatherCode != -1) {
    String asciiArt = getWeatherAsciiArt(weatherCode);
    ThermalPrinter.println(asciiArt);
  }
  
  // Печатаем текущую температуру
  if (currentTemp != -999) {
    String tempLine = "Temperature: " + String(currentTemp, 1) + "C";
    ThermalPrinter.println(tempLine);
  } else {
    ThermalPrinter.println("Temperature: No data");
  }
  
  // Печатаем описание погоды
  if (weatherCode != -1) {
    String weatherDesc = getWeatherDescription(weatherCode);
    ThermalPrinter.println("Weather: " + weatherDesc);
  } else {
    ThermalPrinter.println("Weather: No data");
  }
   
  // Печатаем прогноз на день
  float temp15 = getTemperatureForTime("15:00");
  float temp19 = getTemperatureForTime("19:00");
  
  if (temp15 != -999) {
    ThermalPrinter.print("15:00: " + String(temp15, 1) + "C, ");
  }
  if (temp19 != -999) {
    ThermalPrinter.println("19:00: " + String(temp19, 1) + "C");
  }
  delay(500);
  ThermalPrinter.println("");
  ThermalPrinter.println("==========================");

  delay(500);
  Serial.println("Weather info with ASCII art print completed");
}

// Функция для проверки и автоматической печати в 10:00
void checkAutoPrint() {
  timeClient.update();
  String currentTime = getShortMoscowTime();
  String currentDate = getCurrentDateString();
  
  if (currentTime == "10:00") {
    if (currentDate != lastPrintDate) {
      Serial.println("Auto-printing weather info at 10:00");
      printWeatherInfo();
      todayPrinted = true;
      lastPrintDate = currentDate;
    }
  } else if (currentTime == "00:01") {
    todayPrinted = false;
  }
}

// Функция для получения московского времени в формате HH:MM:SS
String getMoscowTime() {
  timeClient.update();
  String formattedTime = timeClient.getFormattedTime();
  time_t rawTime = timeClient.getEpochTime();
  struct tm *timeInfo;
  timeInfo = localtime(&rawTime);
  
  char dateTimeStr[20];
  snprintf(dateTimeStr, sizeof(dateTimeStr), "%02d.%02d.%04d %s", 
           timeInfo->tm_mday, timeInfo->tm_mon + 1, timeInfo->tm_year + 1900, 
           formattedTime.c_str());
  
  return String(dateTimeStr);
}

// Функция для безопасного добавления сообщения в историю
void addMessageToHistory(String text, String timestamp) {
  if (xSemaphoreTake(xSemaphore, portMAX_DELAY)) {
    Message newMessage;
    newMessage.text = text;
    newMessage.timestamp = timestamp;
    
    if (historyCount < 10) {
      messageHistory[historyCount] = newMessage;
      historyCount++;
    } else {
      for (int i = 0; i < 9; i++) {
        messageHistory[i] = messageHistory[i + 1];
      }
      messageHistory[9] = newMessage;
    }
    xSemaphoreGive(xSemaphore);
  }
}

// Функция для безопасного получения HTML истории
String getHistoryHTML() {
  String historyHtml = "";
  
  if (xSemaphoreTake(xSemaphore, portMAX_DELAY)) {
    if (historyCount == 0) {
      historyHtml = "<div class='empty-history'>Нет сообщений</div>";
    } else {
      // Показываем только последние 5 сообщений для скорости
      int startIndex = (historyCount > 5) ? historyCount - 5 : 0;
      for (int i = historyCount - 1; i >= startIndex; i--) {
        historyHtml += "<div class='history-item'>";
        historyHtml += "<div class='message-content'>";
        
        // Обрезаем длинные сообщения
        String displayText = messageHistory[i].text;
        if (displayText.length() > 50) {
          displayText = displayText.substring(0, 47) + "...";
        }
        historyHtml += "<div class='message-text'>" + displayText + "</div>";
        
        // Упрощенная временная метка
        String shortTime = messageHistory[i].timestamp.substring(11, 16);
        historyHtml += "<div class='timestamp'>" + shortTime + "</div>";
        historyHtml += "</div>";
        historyHtml += "<button class='print-btn' onclick='printMessage(" + String(historyCount - 1 - i) + ")'>Печать</button>";
        historyHtml += "</div>";
      }
      
      if (historyCount > 5) {
        historyHtml += "<div class='info-note'>Показаны последние 5 из " + String(historyCount) + " сообщений</div>";
      }
    }
    xSemaphoreGive(xSemaphore);
  }
  
  return historyHtml;
}

// API для получения данных о погоде в JSON формате
void handleWeatherData() {
  // Проверяем подключение к WiFi перед запросом
  if (WiFi.status() != WL_CONNECTED) {
    server.send(200, "application/json", "{\"icon\":\"❌\",\"temperature\":\"Нет WiFi\",\"temp15\":\"--\",\"temp19\":\"--\"}");
    return;
  }
  
  HTTPClient http;
  
  String url = "https://api.open-meteo.com/v1/forecast?";
  url += "latitude=" + String(MOSCOW_LAT, 6);
  url += "&longitude=" + String(MOSCOW_LON, 6);
  url += "&current=temperature_2m,weather_code";
  url += "&timezone=Europe/Moscow";
  
  http.begin(url);
  http.setTimeout(5000); // Таймаут 5 секунд
  
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    
    float currentTemp = doc["current"]["temperature_2m"];
    int weatherCode = doc["current"]["weather_code"];
    
    http.end();
    
    String weatherIcon = getWeatherIcon(weatherCode);
    float temp15 = getTemperatureForTime("15:00");
    float temp19 = getTemperatureForTime("19:00");
    
    String jsonResponse = "{";
    jsonResponse += "\"icon\":\"" + weatherIcon + "\",";
    jsonResponse += "\"temperature\":\"" + String(currentTemp, 1) + "°C\",";
    jsonResponse += "\"temp15\":\"" + (temp15 != -999 ? String(temp15, 1) : "--") + "\",";
    jsonResponse += "\"temp19\":\"" + (temp19 != -999 ? String(temp19, 1) : "--") + "\"";
    jsonResponse += "}";
    
    server.send(200, "application/json", jsonResponse);
  } else {
    http.end();
    server.send(200, "application/json", "{\"icon\":\"❓\",\"temperature\":\"Ошибка\",\"temp15\":\"--\",\"temp19\":\"--\"}");
  }
}

// API для получения статуса WiFi
void handleWifiStatus() {
  String jsonResponse = "{";
  jsonResponse += "\"connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
  jsonResponse += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  jsonResponse += "\"attempts\":" + String(wifiReconnectAttempts);
  jsonResponse += "}";
  
  server.send(200, "application/json", jsonResponse);
}

// Главная страница - использует кэш
void handleRoot() {
  server.send(200, "text/html; charset=UTF-8", getCachedHomePage());
}

// API для получения только истории (для AJAX)
void handleHistory() {
  server.send(200, "text/html; charset=UTF-8", getHistoryHTML());
}

// Обработка печати сообщения
void handlePrint() {
  if (server.hasArg("index")) {
    int index = server.arg("index").toInt();
    int actualIndex = historyCount - 1 - index;
    
    if (actualIndex >= 0 && actualIndex < historyCount) {
      String messageToPrint;
      
      if (xSemaphoreTake(xSemaphore, portMAX_DELAY)) {
        messageToPrint = messageHistory[actualIndex].text;
        xSemaphoreGive(xSemaphore);
      }
      
      Serial.println("Printing message: " + messageToPrint);
      printToThermalPrinter(messageToPrint);
      
      server.send(200, "text/plain", "Сообщение отправлено на печать: " + messageToPrint);
    } else {
      server.send(400, "text/plain", "Ошибка: неверный индекс сообщения");
    }
  } else {
    server.send(400, "text/plain", "Ошибка: параметр index не указан");
  }
}

// Обработка печати информации о погоде
void handlePrintWeather() {
  Serial.println("Printing weather information...");
  printWeatherInfo();
  server.send(200, "text/plain", "Информация о погоде отправлена на печать");
}

// Обработка отправки формы
void handleSubmit() {
  if (server.hasArg("inputValue")) {
    String inputValue = server.arg("inputValue");
    String currentTime = getMoscowTime();
    String shortTime = getShortMoscowTime();
    
    String transliteratedText = transliterate(inputValue);
    Serial.println(shortTime + ": " + transliteratedText);
    
    printToThermalPrinter(inputValue);
    
    addMessageToHistory(inputValue, currentTime);
    
    server.sendHeader("Location", "/");
    server.send(303);
  } else {
    server.send(400, "text/plain", "Ошибка: поле inputValue не найдено");
  }
}

// Обработка несуществующих страниц
void handleNotFound() {
  String response = "<!DOCTYPE HTML><html><head><meta charset=\"UTF-8\"></head><body>";
  response += "<h2>Страница не найдена</h2>";
  response += "<a href='/'>Вернуться на главную</a>";
  response += "</body></html>";
  server.send(404, "text/html; charset=UTF-8", response);
}

// Задача для обработки веб-сервера в отдельном потоке
void webServerTask(void *parameter) {
  for(;;) {
    server.handleClient();
    delay(1);
  }
}

void setup() {
  Serial.begin(115200);
  
  // Создание семафора для защиты общих ресурсов
  xSemaphore = xSemaphoreCreateMutex();
  
  // Инициализация термопринтера
  initThermalPrinter();
  
  // Подключение к WiFi
  if (!connectToWiFi()) {
    Serial.println("❌ Не удалось подключиться к WiFi при запуске");
  }
  
  // Инициализация NTP клиента
  timeClient.begin();
  timeClient.setTimeOffset(10800);
  
  Serial.print("⏰ Получение времени от NTP сервера");
  for (int i = 0; i < 10; i++) {
    if (timeClient.update()) {
      Serial.println("\n✅ Время получено!");
      break;
    }
    Serial.print(".");
    delay(1000);
  }
  
  lastPrintDate = getCurrentDateString();
  
  // Настройка маршрутов
  server.on("/", handleRoot);
  server.on("/submit", HTTP_POST, handleSubmit);
  server.on("/history", handleHistory);
  server.on("/print", handlePrint);
  server.on("/print-weather", handlePrintWeather);
  server.on("/weather-data", handleWeatherData);
  server.on("/wifi-status", handleWifiStatus);
  server.onNotFound(handleNotFound);
  
  // Запуск сервера
  server.begin();
  Serial.println("🌐 HTTP сервер запущен");
  
  // Создание отдельного потока для веб-сервера
  xTaskCreatePinnedToCore(
    webServerTask,
    "WebServer",
    10000,
    NULL,
    1,
    NULL,
    0
  );
  
  String startupTime = getShortMoscowTime();
  String moscowTime = getMoscowTime();
  Serial.println(startupTime + ": Sistema zapushchena. Gotov k priemu soobshcheniy.");
  Serial.println(startupTime + ": Tekushchee moskovskoe vremya: " + moscowTime);
  Serial.println(startupTime + ": Avtomaticheskaya pechat pogody aktivirovana (10:00 kazhdyy den)");
  
  Serial.println("\n=== INFORMATSIYA ===");
  Serial.println("Ispolzuetsya Open-Meteo API - besplatno, bez API klyucha");
  Serial.println("Prognoz pogody dlya Moskvy");
  Serial.println("Avtomaticheskaya pechat: 10:00 ezhednevno");
  Serial.println("Podderzhka mnozhestvennykh podklyucheniy: DA (mnogopotochnost)");
  Serial.println("Shirokie ASCII-ikonki na termoprintere: DA");
  Serial.println("Avtomaticheskoe vosstanovlenie WiFi: DA");
  Serial.println("Legkaya i bystraya web-stranica: DA");
  Serial.println("===================");
}

void loop() {
  // Периодически обновляем время от NTP сервера
  timeClient.update();
  
  // Проверяем и восстанавливаем WiFi соединение
  checkWiFiConnection();
  
  // Проверяем автоматическую печать каждую минуту
  if (millis() - lastPrintCheck > 60000) {
    checkAutoPrint();
    lastPrintCheck = millis();
  }
  
  delay(100);
}