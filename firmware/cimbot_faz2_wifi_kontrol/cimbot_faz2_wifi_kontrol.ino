/*
 * ÇİMBOT - Faz 2: WiFi + WebSocket Telefon Kontrolü (düşük gecikme)
 * ESP32 + L298N + 2 DC motor
 *
 * GEREKLİ KÜTÜPHANE: Arduino IDE > Library Manager > "WebSockets" by Markus Sattler kur.
 *
 * KULLANIM:
 *  1) Kodu yükle (ESP32 Dev Module).
 *  2) Telefonda WiFi ağlarından "CIMBOT" ağına bağlan (şifre: 12345678).
 *  3) Tarayıcıda  http://192.168.4.1  aç.
 *  4) Butonları BASILI TUT -> hareket eder, bırak -> durur. Üstteki kaydırıcı hız.
 *  !!! İlk testte tekerlekler HAVADA olsun !!!
 */

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

// --- Sol motor ---
#define ENA 14
#define IN1 27
#define IN2 26
// --- Sağ motor ---
#define IN3 25
#define IN4 33
#define ENB 32

const char* ssid = "CIMBOT";
const char* password = "12345678";

WebServer server(80);
WebSocketsServer webSocket(81);

int HIZ = 180;   // 0-255

void hiz(int s){ ledcWrite(ENA, s); ledcWrite(ENB, s); }
void dur(){
  ledcWrite(ENA,0); ledcWrite(ENB,0);
  digitalWrite(IN1,LOW); digitalWrite(IN2,LOW);
  digitalWrite(IN3,LOW); digitalWrite(IN4,LOW);
}
void ileri(){ digitalWrite(IN1,HIGH);digitalWrite(IN2,LOW); digitalWrite(IN3,HIGH);digitalWrite(IN4,LOW); hiz(HIZ); }
void geri (){ digitalWrite(IN1,LOW); digitalWrite(IN2,HIGH);digitalWrite(IN3,LOW); digitalWrite(IN4,HIGH);hiz(HIZ); }
void sola (){ digitalWrite(IN1,LOW); digitalWrite(IN2,HIGH);digitalWrite(IN3,HIGH);digitalWrite(IN4,LOW); hiz(HIZ); } // sol geri, sağ ileri
void saga (){ digitalWrite(IN1,HIGH);digitalWrite(IN2,LOW); digitalWrite(IN3,LOW); digitalWrite(IN4,HIGH);hiz(HIZ); } // sol ileri, sağ geri

const char* PAGE = R"HTML(
<!DOCTYPE html><html><head>
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<style>
body{font-family:sans-serif;text-align:center;background:#15301a;color:#fff;
 user-select:none;-webkit-user-select:none;touch-action:none;margin:0;padding:14px}
h2{color:#8fbf5f;margin:8px}
.grid{display:grid;grid-template-columns:repeat(3,88px);grid-gap:12px;justify-content:center;margin-top:16px}
button{height:88px;font-size:26px;border:none;border-radius:14px;background:#2c5f2d;color:#fff}
button:active{background:#8fbf5f}
.stop{background:#a33}
.sp{margin-top:26px;font-size:18px}
input[type=range]{width:80%;height:34px}
.st{margin-top:10px;color:#8fbf5f;font-size:14px}
</style></head><body>
<h2>ÇİMBOT Kontrol</h2>
<div class="grid">
<div></div><button id="F">&#9650;</button><div></div>
<button id="L">&#9664;</button><button class="stop" id="S">&#9632;</button><button id="R">&#9654;</button>
<div></div><button id="B">&#9660;</button><div></div>
</div>
<div class="sp">Hiz: <span id="v">180</span><br><input type="range" min="0" max="255" value="180" id="sp"></div>
<div class="st" id="st">baglaniyor...</div>
<script>
var ws=new WebSocket('ws://'+location.hostname+':81/');
ws.onopen=function(){document.getElementById('st').innerText='bagli';};
ws.onclose=function(){document.getElementById('st').innerText='koptu';};
function bind(id,cmd){var b=document.getElementById(id);
 var d=function(e){e.preventDefault();ws.send(cmd);};
 var u=function(e){e.preventDefault();ws.send('S');};
 b.addEventListener('touchstart',d);b.addEventListener('touchend',u);
 b.addEventListener('mousedown',d);b.addEventListener('mouseup',u);b.addEventListener('mouseleave',u);}
bind('F','F');bind('B','B');bind('L','L');bind('R','R');
document.getElementById('S').addEventListener('click',function(){ws.send('S');});
var sp=document.getElementById('sp');
sp.addEventListener('input',function(){document.getElementById('v').innerText=sp.value;ws.send('V'+sp.value);});
</script></body></html>
)HTML";

void onWs(uint8_t num, WStype_t type, uint8_t* payload, size_t len){
  if(type==WStype_TEXT && len>0){
    char c = payload[0];
    if(c=='F') ileri();
    else if(c=='B') geri();
    else if(c=='L') sola();
    else if(c=='R') saga();
    else if(c=='S') dur();
    else if(c=='V'){ HIZ = atoi((char*)payload+1); }
  }
}

void setup(){
  Serial.begin(115200);
  pinMode(IN1,OUTPUT);pinMode(IN2,OUTPUT);pinMode(IN3,OUTPUT);pinMode(IN4,OUTPUT);
  ledcAttach(ENA,1000,8);
  ledcAttach(ENB,1000,8);
  dur();

  WiFi.softAP(ssid, password);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP()); // 192.168.4.1

  server.on("/", [](){ server.send(200,"text/html",PAGE); });
  server.begin();
  webSocket.begin();
  webSocket.onEvent(onWs);
  Serial.println("Hazir: WiFi 'CIMBOT' (12345678) -> http://192.168.4.1");
}

void loop(){
  webSocket.loop();
  server.handleClient();
}
