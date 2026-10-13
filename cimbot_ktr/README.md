# 🤖🌱 ÇİMBOT — Mobil Kontrollü ve Otonom Çim Biçme Robotu

ESP32 tabanlı, telefondan **joystick** ile sürülebilen veya **otonom** modda çalışabilen, engel algılayan ve güvenlik sensörlerine sahip kompakt çim biçme robotu.

> Kontrol arayüzü ESP32'nin yayınladığı WiFi ağı üzerinden, uygulama kurmadan tarayıcıdan çalışır (WebSocket ile düşük gecikme).

---

## ✨ Özellikler

- 📱 **Mobil joystick kontrolü** — her yöne oranlı hız, geri + dönüş desteği (differential mixing)
- 🤝 **Otonom mod (AUTO)** — engel görünce sağa kaç → geç → sola dön → düz devam (swerve)
- 🔪 **Kontrollü kesici motor** — RS775, MOSFET ile PWM hız ve yumuşak başlatma
- 📡 **Engel algılama** — TF-Luna ToF sensörü + servo ile tarama
- 🛡️ **Güvenlik** — MPU6050 ile devrilme algılanınca motor + bıçak otomatik durur, acil stop
- 🔋 **Kablosuz güç** — Bosch 18V Li-ion akü + buck çeviriciler
- 🖥️ **Web arayüzü** — canlı mesafe görseli, hız/bıçak kontrolü, mod seçimi

---

## 🧩 Donanım

| Bileşen | Rol |
|---|---|
| **ESP32** | Beyin: kontrol, WiFi, sensör yönetimi |
| **L298N + 2× 37mm redüktörlü DC motor** | Sürüş (diferansiyel / skid-steer) |
| **RS775 + IRLZ44N MOSFET + flyback diyot** | Çim kesme |
| **TF-Luna (ToF) + SG90 servo** | Engel algılama / tarama |
| **MPU6050** | Devrilme / eğim güvenliği |
| **Bosch GBA 18V 5.0Ah + GAL 18V-40** | Güç + şarj |
| **Mini560 (18→12V), buck (→5V)** | Gerilim dönüşümü |
| **30A ANL sigorta** | Kısa devre koruması |
| **3D baskı şase / tekerlek / caster** | Mekanik (~3.4 kg) |

Detaylı malzeme listesi: [`docs/malzeme_listesi/`](docs/malzeme_listesi/)

---

## 🔌 Bağlantı / Pin Haritası (ESP32)

| İşlev | Pin |
|---|---|
| Sürüş — ENA / IN1 / IN2 / IN3 / IN4 / ENB | 14 / 27 / 26 / 25 / 33 / 32 |
| Kesici motor (MOSFET gate, 100Ω üzerinden) | GPIO13 |
| TF-Luna (UART, Serial2) | RX 16 / TX 17 |
| MPU6050 (I2C) | SDA 21 / SCL 22 |
| Servo (TF-Luna tarama) | GPIO4 |

**Güç zinciri:**
```
Akü(+) 18V → 30A Sigorta → +18V hattı ─┬→ RS775 (+) / MOSFET
                                       ├→ Mini560 (18→12V) → L298N → sürüş motorları
                                       └→ Buck (→5V) → ESP32 (USB)
Akü(−) → ORTAK GND (ESP32, L298N, MOSFET Source, bucklar, MPU6050 hepsi ortak)
```

---

## 💾 Firmware (geliştirme fazları)

Kodlar [`firmware/`](firmware/) altında. Her klasör bir Arduino sketch'idir.

| Sketch | Açıklama |
|---|---|
| `cimbot_faz1_motor_test` | Faz 1 — sürüş motorları temel test (WiFi yok) |
| `cimbot_faz2_wifi_kontrol` | Faz 2 — WiFi + WebSocket telefon kontrolü |
| `cimbot_faz3_tfluna` | Faz 3 — TF-Luna engel algılama + görselleştirme |
| `cimbot_faz6_kesici_test` | Faz 6 — kesici motor (RS775) misinasız test |
| `cimbot_tam_kontrol` | İlk tam entegre sürüm (sürüş + kesici + TF-Luna) |
| **`cimbot_joystick_v5`** | **GÜNCEL/FINAL** — joystick + AUTO + ayarlanabilir dönüş |

### Gerekli kütüphane
Arduino IDE → Library Manager → **"WebSockets" by Markus Sattler**

### Yükleme
1. Arduino IDE'de **ESP32 board desteği** kurulu olsun (Boards Manager → `esp32`).
2. Kart: **ESP32 Dev Module**, doğru COM portu.
3. İlgili `.ino`'yu aç, **Upload**.

### Kullanım
1. Telefonda WiFi ağlarından **`CIMBOT`** ağına bağlan — şifre: `12345678`
2. Tarayıcıda **`http://192.168.4.1`** adresini aç.
3. Joystick ile sür / **BIÇAK** aç-kapa / **AUTO** modu / **ACİL STOP**.

### Ayarlanabilir değişkenler (`cimbot_joystick_v5`)
- `DONUS_GUCU` (0–1) — dönüş sertliği (düşür = geniş viraj, iç teker durmaz)
- `STEER_SIGN` (1 / -1) — dönüş yönü ters gelirse çevir
- `ESIK` — engel mesafesi eşiği (cm)

---

## ⚠️ Güvenlik

- **Kesici motoru her zaman önce MİSİNASIZ test edin**, etrafı boşaltın, koruyucu gözlük takın.
- **ACİL STOP** elinizin altında olsun; bağlantı koparsa araç otomatik durur (ölü adam).
- Devrilme/kaldırma durumunda MPU6050 motorları ve bıçağı keser.
- Li-ion akü ve yüksek akım hatlarında kısa devreye dikkat; sigortayı atlamayın.

---

## 🚀 Gelecek Versiyonlar

- 🔌 Otomatik şarj istasyonuna dönüş
- 🌀 Çim toplama / vakumlama
- 📶 GPRS/GSM + GPS ile uzaktan izleme/kontrol
- 🗺️ GPS-RTK ile hassas otonom navigasyon ve haritalama
- 🧠 LiDAR veya derinlik kamerası (Jetson/Raspberry Pi) ile gelişmiş algılama

Detaylı yol haritası ve LiDAR vs derinlik kamerası karşılaştırması: [`docs/sunumlar/4_Cimbot_Gelecek_Versiyonlar.pdf`](docs/sunumlar/)

---

## 📁 Depo Yapısı

```
cimbot_ktr/
├── README.md
├── firmware/              # ESP32 Arduino kodları (fazlara göre)
└── docs/
    ├── sunumlar/          # 4 sunum (pptx + pdf): donanım, reklam, proje, gelecek
    └── malzeme_listesi/   # BOM (Excel)
```

## 📄 Dokümanlar
- **1 — Kullanılan Donanımlar**
- **2 — Reklam / Neden ÇİMBOT**
- **3 — Genel Proje ve Fazlar**
- **4 — Gelecek Versiyonlar (LiDAR vs Derinlik Kamerası dahil)**

---

## 👤 Yazar
**Halil ALPAK** — [github.com/HalilALPAK](https://github.com/HalilALPAK)

> ÇİMBOT — fikirden çalışan prototipe, modüler ve geliştirilebilir bir otonom robot projesi. 🌱
