/*
 * ÇİMBOT - Faz 6: Kesici Motor (RS775) TEST
 * !!! MISINASIZ, DÜŞÜK GÜÇ, MOTOR SABİT, ETRAF BOŞ, GÖZLÜK TAK !!!
 *
 * Bağlantı:
 *   RS775(+) -> +18V (sigorta+anahtar üzerinden)
 *   RS775(-) -> IRLZ44N Drain
 *   IRLZ44N Source -> GND (akü -)
 *   IRLZ44N Gate -> 220Ω -> GPIO13   (Gate-GND arası 10kΩ pull-down)
 *   1N5822 flyback RS775 uçlarına paralel (bant/katot +18V tarafına)
 *   Ortak GND: ESP32 GND = akü - = MOSFET Source
 *
 * Bu kod: 0'dan %40'a rampalı başlatır, 3 sn döner, durur, 5 sn bekler, tekrar.
 */

#define CUT 13          // MOSFET gate pini (220Ω üzerinden)
const int MAX_PWM = 100; // 0-255. 100 ~ %40 güç. İstersen düşür (ör. 60).

void setup(){
  Serial.begin(115200);
  ledcAttach(CUT, 1000, 8);   // 1 kHz PWM, 8-bit (0-255)
  ledcWrite(CUT, 0);
  Serial.println(">>> KESICI TEST - MISINASIZ! Motoru sabitle, etrafi bosalt <<<");
  delay(3000);
}

void loop(){
  Serial.println("Rampali baslatma (yumusak)...");
  for(int d = 0; d <= MAX_PWM; d++){
    ledcWrite(CUT, d);
    delay(25);               // yavas rampa = dusuk kalkis akimi
  }
  Serial.printf("%d/255 gucte 3 sn calisiyor\n", MAX_PWM);
  delay(3000);

  Serial.println("Durduruluyor");
  ledcWrite(CUT, 0);
  delay(5000);               // 5 sn dur, sonra tekrar
}
