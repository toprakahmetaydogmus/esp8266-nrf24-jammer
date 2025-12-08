#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SPI.h>
#include <RF24.h>
#include <EEPROM.h>
#include <Ticker.h>
#include <ArduinoJson.h>

// Pin tanımlamaları
#define RF24_CE_PIN D4
#define RF24_CSN_PIN D8
#define BUTTON_PIN D3     // TEK BUTON (D3 pinine bağlı)
#define LED_PIN D0        // Durum LED'i

// RF24 nesnesi
RF24 radio(RF24_CE_PIN, RF24_CSN_PIN);

// Web sunucusu
ESP8266WebServer server(80);

// Ticker'lar
Ticker attackTicker;
Ticker ledPatternTicker;
Ticker wifiScanTicker;

// Wi-Fi kanalları (2.4GHz)
const uint8_t wifiChannels[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
const uint8_t bleChannels[] = {37, 38, 39, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36};

// LED Pattern yapısı
struct LedPattern {
  uint8_t blinkCount;      // Kaç kez yanıp sönecek
  uint16_t onTime;         // Yanık kalma süresi (ms)
  uint16_t offTime;        // Sönük kalma süresi (ms)
  uint16_t repeatDelay;    // Tekrarlar arası bekleme (ms)
};

// Modlara göre LED pattern'leri
const LedPattern patternIdle = {1, 200, 800, 0};       // 1 kez, 200ms yanık, 800ms sönük
const LedPattern patternWiFi = {2, 200, 200, 400};     // 2 kez, 200ms yanık/200ms sönük, 400ms bekle
const LedPattern patternBLE = {3, 150, 150, 400};      // 3 kez, 150ms yanık/150ms sönük, 400ms bekle
const LedPattern patternCombined = {4, 100, 100, 800}; // 4 kez, 100ms yanık/100ms sönük, 800ms bekle
const LedPattern patternAttack = {0, 50, 50, 0};       // Sürekli hızlı yanıp sönme

// Global değişkenler - DÜZELTİLDİ!
volatile uint8_t attackMode = 3;          // 0: Idle, 1: Wi-Fi only, 2: BLE only, 3: Combined
volatile bool attackActive = false;        // Attack durumu
volatile bool buttonPressed = false;       // Buton interrupt flag
volatile bool buttonLongPress = false;     // Buton uzun basma flag
bool settingsChanged = false;              // Ayarlar değişti flag'i
bool ledBusy = false;                      // LED pattern çalışıyor mu?

// LED Pattern değişkenleri - DÜZELTİLDİ!
LedPattern currentPattern = patternCombined;  // VOLATILE DEĞİL!
volatile uint8_t patternStep = 0;
volatile uint8_t blinkCounter = 0;
volatile bool ledState = false;
volatile unsigned long patternLastTime = 0;

unsigned long lastDebounceTime = 0;
unsigned long buttonPressTime = 0;
const unsigned long debounceDelay = 50;
const unsigned long longPressTime = 1000;  // 1 saniye uzun basma
const unsigned long doubleClickTime = 300; // 300ms çift tıklama süresi

// Buton için çift tıklama değişkenleri
unsigned long lastClickTime = 0;
uint8_t clickCount = 0;

// EEPROM'dan okunacak/saklanacak ayarlar
struct Settings {
  uint8_t defaultMode = 3;        // Varsayılan attack modu (Combined)
  uint8_t wifiChannelHop = 1;     // Wi-Fi kanal atlama hızı (ms)
  uint8_t bleChannelHop = 1;      // BLE kanal atlama hızı (ms)
  uint16_t attackDuration = 0;    // Attack süresi (0 = süresiz)
  bool ledEnabled = true;         // LED aktif mi?
  bool autoStart = true;          // Başlangıçta otomatik başlasın mı?
  char apSSID[32] = "ESP_Jammer"; // AP SSID
  char apPassword[32] = "12345678"; // AP şifresi
};

Settings settings;

// Attack istatistikleri
struct AttackStats {
  uint32_t totalAttackTime = 0;
  uint32_t wifiChannelChanges = 0;
  uint32_t bleChannelChanges = 0;
  uint32_t totalAttacks = 0;
  uint32_t lastAttackDuration = 0;
  uint8_t modeChanges = 0;
  uint8_t attackToggleCount = 0;
};

AttackStats stats;

// Wi-Fi ağları için struct
struct WiFiNetwork {
  String ssid;
  int32_t rssi;
  uint8_t channel;
};

WiFiNetwork detectedNetworks[20];
uint8_t networkCount = 0;

// Prototipler
void ICACHE_RAM_ATTR handleButtonInterrupt();
void checkButtonPress();
void loadSettings();
void saveSettings();
void updateLedPattern();
void showModeLedPattern(uint8_t mode);
void scanWiFiNetworks();
void performAttack();
String generateHTML();
String generateStatsJSON();
String generateNetworksJSON();
void handleRoot();
void handleStats();
void handleNetworks();
void handleStartAttack();
void handleStopAttack();
void handleSetMode();
void handleScanNetworks();
void handleUpdateSettings();
void handleResetStats();
void handleFactoryReset();

// HTML sayfa şablonu (kısa versiyon)
String htmlPage() {
  return R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP8266 Jammer Controller</title>
    <style>
        body { font-family: Arial; background: #f0f0f0; padding: 20px; }
        .container { max-width: 800px; margin: auto; background: white; padding: 20px; border-radius: 10px; }
        .header { background: #333; color: white; padding: 15px; text-align: center; border-radius: 10px 10px 0 0; }
        .status-bar { background: #eee; padding: 10px; margin-bottom: 20px; }
        .btn { padding: 10px; margin: 5px; border: none; border-radius: 5px; cursor: pointer; }
        .btn-success { background: green; color: white; }
        .btn-danger { background: red; color: white; }
        .btn-primary { background: blue; color: white; }
        .btn-warning { background: orange; color: white; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>📡 RF Jammer Controller</h1>
            <p>ESP8266 + nRF24L01 | Educational Purposes Only</p>
        </div>
        
        <div class="status-bar">
            <div>Status: <span id="statusText">Initializing...</span></div>
            <div>Mode: <span id="currentMode">Combined</span></div>
        </div>
        
        <button class="btn btn-success" onclick="startAttack()">▶️ Start Attack</button>
        <button class="btn btn-danger" onclick="stopAttack()">⏹️ Stop Attack</button>
        <button class="btn btn-primary" onclick="setMode(1)">📶 Wi-Fi Only</button>
        <button class="btn btn-primary" onclick="setMode(2)">📱 BLE Only</button>
        <button class="btn btn-warning" onclick="setMode(3)">⚡ Combined</button>
        <button class="btn" onclick="setMode(0)">💤 Idle</button>
        
        <div id="statsDisplay" style="margin-top: 20px;"></div>
    </div>
    
    <script>
        async function updateStatus() {
            try {
                const response = await fetch('/stats');
                const data = await response.json();
                
                document.getElementById('statusText').textContent = 
                    data.attackActive ? 'Attack Active' : 'Attack Inactive';
                
                const modes = ['Idle', 'Wi-Fi Only', 'BLE Only', 'Combined'];
                document.getElementById('currentMode').textContent = modes[data.attackMode];
                
                document.getElementById('statsDisplay').innerHTML = `
                    <div>Total Attack Time: ${Math.floor(data.totalAttackTime / 1000)}s</div>
                    <div>Wi-Fi Channel Changes: ${data.wifiChannelChanges}</div>
                    <div>BLE Channel Changes: ${data.bleChannelChanges}</div>
                `;
            } catch (error) {
                console.error('Error:', error);
            }
        }
        
        async function startAttack() {
            await fetch('/start');
            updateStatus();
        }
        
        async function stopAttack() {
            await fetch('/stop');
            updateStatus();
        }
        
        async function setMode(mode) {
            await fetch('/set?mode=' + mode);
            updateStatus();
        }
        
        setInterval(updateStatus, 2000);
        updateStatus();
    </script>
</body>
</html>
)=====";
}

// Buton Interrupt
void ICACHE_RAM_ATTR handleButtonInterrupt() {
  unsigned long currentTime = millis();
  if ((currentTime - lastDebounceTime) > debounceDelay) {
    buttonPressed = true;
    buttonPressTime = currentTime;
    lastDebounceTime = currentTime;
  }
}

// Buton kontrol fonksiyonu - DÜZELTİLDİ!
void checkButtonPress() {
  static bool buttonActive = false;
  static unsigned long buttonStartTime = 0;
  
  if (buttonPressed) {
    buttonPressed = false;
    
    if (millis() - lastClickTime < doubleClickTime) {
      clickCount++;
    } else {
      clickCount = 1;
    }
    lastClickTime = millis();
    
    // Çift tıklama
    if (clickCount == 2) {
      clickCount = 0;
      Serial.println("Double click - Showing current mode pattern");
      showModeLedPattern(attackMode);
      return;
    }
    
    buttonActive = true;
    buttonStartTime = millis();
  }
  
  if (buttonActive) {
    unsigned long pressDuration = millis() - buttonStartTime;
    
    // Uzun basma kontrolü
    if (digitalRead(BUTTON_PIN) == LOW) {
      if (pressDuration >= longPressTime && !buttonLongPress) {
        buttonLongPress = true;
        
        // Mod değiştir
        uint8_t newMode = (attackMode + 1) % 4;
        attackMode = newMode;
        stats.modeChanges++;
        
        Serial.print("Long press - Mode changed to: ");
        Serial.println(attackMode);
        
        showModeLedPattern(attackMode);
        
        if (!attackActive && attackMode != 0) {
          attackActive = true;
          Serial.println("Attack auto-started");
        }
      }
    }
    
    // Buton bırakıldı
    if (digitalRead(BUTTON_PIN) == HIGH) {
      if (pressDuration < longPressTime && !buttonLongPress) {
        if (clickCount != 2) {
          attackActive = !attackActive;
          stats.attackToggleCount++;
          
          if (attackActive) {
            Serial.println("Short press - Attack started");
            if (attackMode == 0) {
              attackMode = 3;
              stats.modeChanges++;
              showModeLedPattern(attackMode);
            }
          } else {
            Serial.println("Short press - Attack stopped");
          }
        }
      }
      buttonActive = false;
      buttonLongPress = false;
    }
  }
}

void loadSettings() {
  EEPROM.begin(sizeof(Settings) + sizeof(AttackStats));
  uint32_t magic = EEPROM.read(0) | (EEPROM.read(1) << 8) | 
                   (EEPROM.read(2) << 16) | (EEPROM.read(3) << 24);
  
  if (magic == 0x4A414D4D) {
    EEPROM.get(4, settings);
    EEPROM.get(4 + sizeof(Settings), stats);
  }
  EEPROM.end();
}

void saveSettings() {
  EEPROM.begin(sizeof(Settings) + sizeof(AttackStats));
  uint32_t magic = 0x4A414D4D;
  EEPROM.put(0, magic);
  EEPROM.put(4, settings);
  EEPROM.put(4 + sizeof(Settings), stats);
  EEPROM.commit();
  EEPROM.end();
  settingsChanged = true;
}

void updateLedPattern() {
  if (ledBusy || !settings.ledEnabled) return;
  
  unsigned long currentTime = millis();
  
  switch (patternStep) {
    case 0:
      if (currentTime - patternLastTime >= currentPattern.repeatDelay) {
        patternStep = 1;
        blinkCounter = 0;
        patternLastTime = currentTime;
      }
      break;
      
    case 1:
      if (!ledState) {
        digitalWrite(LED_PIN, LOW);
        ledState = true;
        patternLastTime = currentTime;
      } else if (currentTime - patternLastTime >= currentPattern.onTime) {
        patternStep = 2;
        patternLastTime = currentTime;
      }
      break;
      
    case 2:
      if (ledState) {
        digitalWrite(LED_PIN, HIGH);
        ledState = false;
        patternLastTime = currentTime;
        blinkCounter++;
      } else if (currentTime - patternLastTime >= currentPattern.offTime) {
        if (blinkCounter < currentPattern.blinkCount || currentPattern.blinkCount == 0) {
          patternStep = 1;
        } else {
          patternStep = 0;
          if (currentPattern.blinkCount == 0) {
            patternStep = 1;
            blinkCounter = 0;
          }
        }
      }
      break;
  }
}

void showModeLedPattern(uint8_t mode) {
  if (!settings.ledEnabled) return;
  
  ledBusy = true;
  patternStep = 0;
  digitalWrite(LED_PIN, HIGH);
  
  LedPattern pattern;
  switch (mode) {
    case 0: pattern = patternIdle; break;
    case 1: pattern = patternWiFi; break;
    case 2: pattern = patternBLE; break;
    case 3: pattern = patternCombined; break;
    default: pattern = patternCombined; break;
  }
  
  for (int i = 0; i < pattern.blinkCount; i++) {
    digitalWrite(LED_PIN, LOW);
    delay(pattern.onTime);
    digitalWrite(LED_PIN, HIGH);
    if (i < pattern.blinkCount - 1) delay(pattern.offTime);
  }
  
  delay(pattern.repeatDelay);
  
  if (attackActive) {
    currentPattern = patternAttack;
  } else {
    switch (attackMode) {
      case 0: currentPattern = patternIdle; break;
      case 1: currentPattern = patternWiFi; break;
      case 2: currentPattern = patternBLE; break;
      case 3: currentPattern = patternCombined; break;
    }
  }
  
  patternStep = 0;
  blinkCounter = 0;
  ledState = false;
  patternLastTime = millis();
  ledBusy = false;
}

void scanWiFiNetworks() {
  networkCount = 0;
  int numNetworks = WiFi.scanNetworks(false, true);
  
  for (int i = 0; i < min(numNetworks, 20); i++) {
    detectedNetworks[networkCount].ssid = WiFi.SSID(i);
    detectedNetworks[networkCount].rssi = WiFi.RSSI(i);
    detectedNetworks[networkCount].channel = WiFi.channel(i);
    networkCount++;
  }
  WiFi.scanDelete();
}

void performAttack() {
  static uint32_t wifiChannelIndex = 0;
  static uint32_t bleChannelIndex = 0;
  static uint32_t lastWifiHop = 0;
  static uint32_t lastBleHop = 0;
  static bool combinedState = false;
  
  uint32_t currentTime = millis();
  
  if (!attackActive) return;
  
  if (currentPattern.blinkCount != 0) {
    currentPattern = patternAttack;
    patternStep = 0;
  }
  
  switch (attackMode) {
    case 1: // Wi-Fi only
      if (currentTime - lastWifiHop >= settings.wifiChannelHop) {
        radio.setChannel(wifiChannels[wifiChannelIndex]);
        wifiChannelIndex = (wifiChannelIndex + 1) % (sizeof(wifiChannels) / sizeof(wifiChannels[0]));
        stats.wifiChannelChanges++;
        lastWifiHop = currentTime;
      }
      break;
      
    case 2: // BLE only
      if (currentTime - lastBleHop >= settings.bleChannelHop) {
        radio.setChannel(bleChannels[bleChannelIndex]);
        bleChannelIndex = (bleChannelIndex + 1) % (sizeof(bleChannels) / sizeof(bleChannels[0]));
        stats.bleChannelChanges++;
        lastBleHop = currentTime;
      }
      break;
      
    case 3: // Combined
      if (combinedState) {
        if (currentTime - lastWifiHop >= settings.wifiChannelHop) {
          radio.setChannel(wifiChannels[wifiChannelIndex]);
          wifiChannelIndex = (wifiChannelIndex + 1) % (sizeof(wifiChannels) / sizeof(wifiChannels[0]));
          stats.wifiChannelChanges++;
          lastWifiHop = currentTime;
        }
      } else {
        if (currentTime - lastBleHop >= settings.bleChannelHop) {
          radio.setChannel(bleChannels[bleChannelIndex]);
          bleChannelIndex = (bleChannelIndex + 1) % (sizeof(bleChannels) / sizeof(bleChannels[0]));
          stats.bleChannelChanges++;
          lastBleHop = currentTime;
        }
      }
      
      if (currentTime % 10 < 5) {
        combinedState = true;
      } else {
        combinedState = false;
      }
      break;
  }
  
  static uint32_t attackStartTime = 0;
  if (attackStartTime == 0) {
    attackStartTime = currentTime;
    stats.totalAttacks++;
    Serial.println("Attack started!");
  }
  
  stats.lastAttackDuration = currentTime - attackStartTime;
  
  if (settings.attackDuration > 0 && stats.lastAttackDuration >= settings.attackDuration) {
    attackActive = false;
    stats.totalAttackTime += stats.lastAttackDuration;
    attackStartTime = 0;
    Serial.println("Attack duration limit reached");
  }
}

String generateStatsJSON() {
  DynamicJsonDocument doc(1024);
  doc["attackMode"] = attackMode;
  doc["attackActive"] = attackActive;
  doc["totalAttackTime"] = stats.totalAttackTime;
  doc["wifiChannelChanges"] = stats.wifiChannelChanges;
  doc["bleChannelChanges"] = stats.bleChannelChanges;
  doc["totalAttacks"] = stats.totalAttacks;
  doc["lastAttackDuration"] = stats.lastAttackDuration;
  
  JsonObject settingsObj = doc.createNestedObject("settings");
  settingsObj["defaultMode"] = settings.defaultMode;
  settingsObj["wifiChannelHop"] = settings.wifiChannelHop;
  settingsObj["bleChannelHop"] = settings.bleChannelHop;
  
  String output;
  serializeJson(doc, output);
  return output;
}

String generateNetworksJSON() {
  DynamicJsonDocument doc(4096);
  JsonArray networks = doc.createNestedArray("networks");
  
  for (int i = 0; i < networkCount; i++) {
    JsonObject network = networks.createNestedObject();
    network["ssid"] = detectedNetworks[i].ssid;
    network["rssi"] = detectedNetworks[i].rssi;
    network["channel"] = detectedNetworks[i].channel;
  }
  
  String output;
  serializeJson(doc, output);
  return output;
}

void handleRoot() { server.send(200, "text/html", htmlPage()); }
void handleStats() { server.send(200, "application/json", generateStatsJSON()); }
void handleNetworks() { server.send(200, "application/json", generateNetworksJSON()); }
void handleStartAttack() { attackActive = true; server.send(200, "application/json", "{\"status\":\"ok\"}"); }
void handleStopAttack() { attackActive = false; server.send(200, "application/json", "{\"status\":\"ok\"}"); }

void handleSetMode() {
  if (server.hasArg("mode")) {
    attackMode = server.arg("mode").toInt();
    showModeLedPattern(attackMode);
  }
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleScanNetworks() {
  scanWiFiNetworks();
  server.send(200, "application/json", generateNetworksJSON());
}

void handleUpdateSettings() {
  String body = server.arg("plain");
  DynamicJsonDocument doc(512);
  deserializeJson(doc, body);
  
  if (doc.containsKey("defaultMode")) settings.defaultMode = doc["defaultMode"];
  if (doc.containsKey("wifiChannelHop")) settings.wifiChannelHop = doc["wifiChannelHop"];
  if (doc.containsKey("bleChannelHop")) settings.bleChannelHop = doc["bleChannelHop"];
  if (doc.containsKey("attackDuration")) settings.attackDuration = doc["attackDuration"];
  if (doc.containsKey("ledEnabled")) settings.ledEnabled = doc["ledEnabled"];
  if (doc.containsKey("autoStart")) settings.autoStart = doc["autoStart"];
  if (doc.containsKey("apSSID")) strlcpy(settings.apSSID, doc["apSSID"], sizeof(settings.apSSID));
  if (doc.containsKey("apPassword")) strlcpy(settings.apPassword, doc["apPassword"], sizeof(settings.apPassword));
  
  saveSettings();
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleResetStats() {
  stats.totalAttackTime = 0;
  stats.wifiChannelChanges = 0;
  stats.bleChannelChanges = 0;
  stats.totalAttacks = 0;
  stats.lastAttackDuration = 0;
  stats.modeChanges = 0;
  stats.attackToggleCount = 0;
  saveSettings();
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleFactoryReset() {
  settings.defaultMode = 3;
  settings.wifiChannelHop = 1;
  settings.bleChannelHop = 1;
  settings.attackDuration = 0;
  settings.ledEnabled = true;
  settings.autoStart = true;
  strlcpy(settings.apSSID, "ESP_Jammer", sizeof(settings.apSSID));
  strlcpy(settings.apPassword, "12345678", sizeof(settings.apPassword));
  
  stats.totalAttackTime = 0;
  stats.wifiChannelChanges = 0;
  stats.bleChannelChanges = 0;
  stats.totalAttacks = 0;
  stats.lastAttackDuration = 0;
  stats.modeChanges = 0;
  stats.attackToggleCount = 0;
  
  saveSettings();
  server.send(200, "application/json", "{\"status\":\"ok\"}");
  delay(1000);
  ESP.restart();
}

void setup() {
  Serial.begin(9600);
  Serial.println("\n\n=== RF Jammer Controller ===\n");
  Serial.println("Educational Purposes Only!\n");
  
  // Pin modları - DÜZELTİLDİ!
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonInterrupt, FALLING);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  
  loadSettings();
  
  // RF24 başlat
  Serial.println("Initializing RF24 module...");
  if (!radio.begin()) {
    Serial.println("RF24 initialization failed!");
    while(1);
  }
  
  radio.setAutoAck(false);
  radio.stopListening();
  radio.setPayloadSize(5);
  radio.setAddressWidth(3);
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_2MBPS);
  radio.setCRCLength(RF24_CRC_DISABLED);
  radio.startConstCarrier(RF24_PA_MAX, 45);
  Serial.println("RF24 initialized!");
  
  attackMode = settings.defaultMode;
  
  // LED pattern ayarla - DÜZELTİLDİ!
  switch (attackMode) {
    case 0: currentPattern = patternIdle; break;
    case 1: currentPattern = patternWiFi; break;
    case 2: currentPattern = patternBLE; break;
    case 3: currentPattern = patternCombined; break;
  }
  
  WiFi.softAP(settings.apSSID, settings.apPassword);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  
  server.on("/", handleRoot);
  server.on("/stats", handleStats);
  server.on("/networks", handleNetworks);
  server.on("/start", handleStartAttack);
  server.on("/stop", handleStopAttack);
  server.on("/set", handleSetMode);
  server.on("/scan", handleScanNetworks);
  server.on("/updateSettings", HTTP_POST, handleUpdateSettings);
  server.on("/resetStats", handleResetStats);
  server.on("/factoryReset", handleFactoryReset);
  
  server.begin();
  Serial.println("HTTP server started!");
  
  attackTicker.attach_ms(1, performAttack);
  ledPatternTicker.attach_ms(10, updateLedPattern);
  wifiScanTicker.attach(10, scanWiFiNetworks);
  
  if (settings.autoStart && attackMode != 0) {
    attackActive = true;
    Serial.println("Auto-start: Attack activated!");
  }
  
  showModeLedPattern(attackMode);
  
  Serial.println("\n=== System Ready ===\n");
  Serial.println("Single Button Control (D3):");
  Serial.println("  • Short press: Toggle attack on/off");
  Serial.println("  • Long press (1s): Change attack mode");
  Serial.println("  • Double click: Show current mode pattern");
  Serial.println("\nWeb Interface: http://" + WiFi.softAPIP().toString());
}

void loop() {
  server.handleClient();
  checkButtonPress();
  
  static bool lastAttackState = false;
  if (attackActive != lastAttackState) {
    if (attackActive) {
      radio.startConstCarrier(RF24_PA_MAX, 45);
      Serial.println("RF24 carrier started");
    } else {
      radio.stopConstCarrier();
      Serial.println("RF24 carrier stopped");
      
      switch (attackMode) {
        case 0: currentPattern = patternIdle; break;
        case 1: currentPattern = patternWiFi; break;
        case 2: currentPattern = patternBLE; break;
        case 3: currentPattern = patternCombined; break;
      }
      patternStep = 0;
    }
    lastAttackState = attackActive;
  }
  
  delay(10);
}
