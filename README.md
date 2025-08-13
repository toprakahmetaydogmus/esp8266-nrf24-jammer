Bu proje, **ESP8266** ve **nRF24L01** modüllerini kullanarak **2.4 GHz frekans bandında test ve simülasyon** yapmanıza olanak tanır.  
Web arayüzü üzerinden çalışma modu seçilebilir:  
- **BLE & All 2.4GHz** (Bluetooth Low Energy ve tüm 2.4 GHz kanallar)  
- **Just Wi-Fi** (Sadece Wi-Fi kanalları)  
- **Idle** (Boşta, yayın yapmaz)  

⚠️ **UYARI:** Bu proje **eğitim ve laboratuvar ortamı** için tasarlanmıştır. Gerçek ortamda, izinsiz olarak RF sinyali bozmak pek çok ülkede (Türkiye dahil) suçtur. Bu kodu kullanırken bulunduğunuz ülkenin yasa ve yönetmeliklerine uymak tamamen sizin sorumluluğunuzdadır.

---

## 🔧 Donanım Gereksinimleri
- NodeMCU ESP8266 (ESP-12E/D)
- nRF24L01 modülü (tercihen PA/LNA antenli)
- Breadboard ve jumper kablolar
- Buton (mod değiştirme için)
- 3.3V regülatör (nRF24L01 için, özellikle PA/LNA modelde)

---

## 🔌 Donanım Bağlantıları

**nRF24L01 → NodeMCU ESP8266**
| nRF24L01 Pin | NodeMCU Pin | Açıklama |
|--------------|------------|----------|
| **GND**      | GND        | Toprak hattı |
| **VCC**      | 3V3        | 3.3V besleme (5V bağlama!) |
| **CE**       | D4 (GPIO2) | Kodda `RF24 radio(D4, D8);` ile tanımlı |
| **CSN**      | D8 (GPIO15)| Kodda `RF24 radio(D4, D8);` ile tanımlı |
| **SCK**      | D5 (GPIO14)| SPI saat hattı |
| **MOSI**     | D7 (GPIO13)| SPI veri hattı (master → slave) |
| **MISO**     | D6 (GPIO12)| SPI veri hattı (slave → master) |

**Buton Bağlantısı**
| Buton Ucu | NodeMCU Pin | Açıklama |
|-----------|-------------|----------|
| Uç 1      | D3 (GPIO0)  | INPUT_PULLUP ile tanımlı, basıldığında LOW algılar |
| Uç 2      | GND         | Toprak hattı |

> Antenli nRF24L01 PA/LNA modülü kullanıyorsanız harici 3.3V regülatör (AMS1117 vb.) önerilir.

---

## 🖥 Web Arayüz Özellikleri
- Tarayıcı üzerinden mod değiştirme
- Anlık durum görüntüleme
- Buton ile hızlı mod değiştirme desteği
- Basit HTML/CSS ile mobil uyumlu arayüz

---

## ⚙️ Kurulum
1. Arduino IDE üzerinden ESP8266 kart paketini yükleyin.
2. `RF24` ve `ESP8266WebServer` kütüphanelerini ekleyin.
3. Kodu ESP8266 üzerine yükleyin.
4. ESP8266 açıldığında **ESP_Jammer** isimli bir Wi-Fi ağı oluşturacaktır. Şifre: `12345678`
5. Tarayıcıdan `192.168.4.1` adresine girerek kontrol paneline erişebilirsiniz.

---

## 📜 Yasal Uyarı
Bu yazılım **yalnızca kapalı devre test ortamları ve Faraday kafesi gibi izole laboratuvar koşullarında** kullanılmalıdır.  
RF spektrumunu bozmak, haberleşmeyi engellemek veya lisanssız sinyal yaymak Türkiye dahil olmak üzere pek çok ülkede **yasalara aykırıdır**.  
Projenin kötüye kullanımından **yazar hiçbir şekilde sorumlu tutulamaz**.

---

## 🌍 English Description

**ESP8266 + nRF24L01 Web Controlled Jammer (For Educational & Research Purposes Only)**  
This project allows you to **test and simulate** the 2.4 GHz frequency band using **ESP8266** and **nRF24L01** modules.  
Three modes available:  
- **BLE & All 2.4GHz** (Bluetooth Low Energy + all 2.4 GHz channels)  
- **Just Wi-Fi** (Wi-Fi channels only)  
- **Idle** (No signal transmission)  

⚠️ **WARNING:** For lab and research purposes only. Unauthorized RF interference is illegal in most countries. The user is fully responsible for any misuse.

---

## 👨‍💻 Geliştirici / Developer
**Toprak Ahmet Aydoğmuş**  
Siber Güvenlik Uzmanı | BTK Sertifikalı  
30+ ulusal/uluslararası sertifika sahibi | Penetrasyon testi, ağ güvenliği, adli bilişim uzmanı  
🌐 [Web Sitesi](https://cybertoprak.wixsite.com/siberegitim)  
🔗 [LinkedIn](https://www.linkedin.com/in/toprak-ahmet-aydoğmuş-60462534b/)

---

## 📄 Lisans
Bu proje **MIT Lisansı** ile yayınlanmaktadır.  
Kullanıcı, projenin kullanımından doğacak tüm sorumluluğu kabul eder.
