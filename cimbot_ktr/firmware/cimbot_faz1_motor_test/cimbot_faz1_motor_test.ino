/*
 * ÇİMBOT - Faz 1: Motor Test (WiFi yok)
 * ESP32 + L298N + 2 DC motor (sol/sağ)
 * !!! TEKERLEKLERİ HAVADA TUT - ilk testi masada, tekerlekler boşta yap !!!
 *
 * Not: Bu kod ESP32 Arduino Core 3.x içindir (ledcAttach/ledcWrite).
 *      Eski 2.x core kullanıyorsan PWM satırlarını değiştirmen gerekir
 *      (ledcSetup + ledcAttachPin). Kurulumda genelde 3.x gelir.
 */

// --- Sol motor ---
#define ENA 14   // Sol hız (PWM)
#define IN1 27
#define IN2 26
// --- Sağ motor ---
#define IN3 25
#define IN4 33
#define ENB 32   // Sağ hız (PWM)

const int HIZ = 180;   // 0-255 arası. Test için orta hız. Az dönerse artır.

// --- Yön fonksiyonları ---
void solIleri() { digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);  }
void solGeri()  { digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); }
void sagIleri() { digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);  }
void sagGeri()  { digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH); }

void hiz(int s) { ledcWrite(ENA, s); ledcWrite(ENB, s); }

void dur() {
  ledcWrite(ENA, 0); ledcWrite(ENB, 0);
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  ledcAttach(ENA, 1000, 8);  // pin, 1 kHz, 8-bit çözünürlük (0-255)
  ledcAttach(ENB, 1000, 8);
  dur();
  Serial.println(">>> Motor testi basliyor - TEKERLEKLER HAVADA OLSUN! <<<");
  delay(2000);
}

void loop() {
  Serial.println("ILERI");    solIleri(); sagIleri(); hiz(HIZ); delay(2000);
  Serial.println("DUR");      dur();                            delay(1000);
  Serial.println("GERI");     solGeri();  sagGeri();  hiz(HIZ); delay(2000);
  Serial.println("DUR");      dur();                            delay(1000);
  Serial.println("SAGA DON"); solIleri(); sagGeri();  hiz(HIZ); delay(1500);
  Serial.println("DUR");      dur();                            delay(1000);
  Serial.println("SOLA DON"); solGeri();  sagIleri(); hiz(HIZ); delay(1500);
  Serial.println("DUR");      dur();                            delay(2000);
}
