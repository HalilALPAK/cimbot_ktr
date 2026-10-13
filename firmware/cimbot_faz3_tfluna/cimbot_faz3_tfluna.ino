/*
 * ÇİMBOT - Faz 3: WiFi kontrol + TF-Luna engel algılama + AUTO (otonom) mod
 * ESP32 + L298N + 2 DC motor + TF-Luna (UART)
 *
 * GEREKLİ KÜTÜPHANE: "WebSockets" by Markus Sattler (Library Manager'dan).
 *
 * TF-Luna (UART): Kırmızı->5V | Siyah->GND | TXD(siyaha bitişik mavi)->GPIO16
 *
 * Telefon: "CIMBOT" WiFi (12345678) -> http://192.168.4.1
 *   Üstte TF-Luna ışın görüntüsü (engelde kırmızı), altta kontrol + MANUEL/AUTO.
 *   !!! İlk testte tekerlekler HAVADA olsun !!!
 */

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

#define ENA 14
#define IN1 27
#define IN2 26
#define IN3 25
#define IN4 33
#define ENB 32

#define TF_RX 16
#define TF_TX 17
#define ESIK 35    // cm

const char* ssid = "CIMBOT";
const char* password = "12345678";

WebServer server(80);
WebSocketsServer webSocket(81);

int HIZ = 180;
int tfDist = -1;
char cmd = 'S';
bool autoMode = false;
unsigned long lastSend = 0;

void hiz(int s){ ledcWrite(ENA,s); ledcWrite(ENB,s); }
void dur(){ ledcWrite(ENA,0); ledcWrite(ENB,0);
  digitalWrite(IN1,LOW);digitalWrite(IN2,LOW);digitalWrite(IN3,LOW);digitalWrite(IN4,LOW); }
void ileri(){ digitalWrite(IN1,HIGH);digitalWrite(IN2,LOW); digitalWrite(IN3,HIGH);digitalWrite(IN4,LOW); hiz(HIZ); }
void geri (){ digitalWrite(IN1,LOW); digitalWrite(IN2,HIGH);digitalWrite(IN3,LOW); digitalWrite(IN4,HIGH);hiz(HIZ); }
void sola (){ digitalWrite(IN1,LOW); digitalWrite(IN2,HIGH);digitalWrite(IN3,HIGH);digitalWrite(IN4,LOW); hiz(HIZ); }
void saga (){ digitalWrite(IN1,HIGH);digitalWrite(IN2,LOW); digitalWrite(IN3,LOW); digitalWrite(IN4,HIGH);hiz(HIZ); }

void readTFLuna(){
  while(Serial2.available() >= 9){
    if(Serial2.read() != 0x59) continue;
    if(Serial2.read() != 0x59) continue;
    uint8_t f[7]; Serial2.readBytes(f, 7);
    int d = f[0] | (f[1] << 8);
    uint16_t sum = 0x59 + 0x59;
    for(int i=0;i<6;i++) sum += f[i];
    if((sum & 0xFF) == f[6]) tfDist = d;
  }
}

const char* PAGE = R"HTML(
<!DOCTYPE html><html><head>
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<style>
*{box-sizing:border-box}
body{font-family:sans-serif;text-align:center;background:#0f2413;color:#fff;
 user-select:none;-webkit-user-select:none;touch-action:none;margin:0;padding:10px}
h2{color:#8fbf5f;margin:4px 0}
#panel{background:#0a1a0d;border-radius:16px;padding:6px;margin:6px 0}
#rad{width:100%;height:210px;display:block}
.st{color:#8fbf5f;font-size:13px;margin:2px 0 10px}
.grid{display:grid;grid-template-columns:repeat(3,80px);grid-gap:10px;justify-content:center}
button{height:80px;font-size:24px;border:none;border-radius:14px;background:#2c5f2d;color:#fff}
button:active{background:#8fbf5f}
.stop{background:#a33}
.modes{display:flex;gap:10px;margin-top:14px}
.modes button{flex:1;height:52px;font-size:18px;background:#22405f}
.modes button.on{background:#e7b84b;color:#222}
.sp{margin-top:14px;font-size:15px}
input[type=range]{width:85%;height:30px}
</style></head><body>
<h2>ÇİMBOT</h2>

<!-- ÜST: TF-Luna görselleştirme -->
<div id="panel"><canvas id="rad"></canvas></div>
<div class="st" id="st">baglaniyor...</div>

<!-- ALT: kontroller -->
<div class="grid">
<div></div><button id="F">&#9650;</button><div></div>
<button id="L">&#9664;</button><button class="stop" id="S">&#9632;</button><button id="R">&#9654;</button>
<div></div><button id="B">&#9660;</button><div></div>
</div>

<div class="modes">
<button id="M" class="on">MANUEL</button>
<button id="A">AUTO</button>
</div>

<div class="sp">Hiz: <span id="v">180</span><br><input type="range" min="0" max="255" value="180" id="sp"></div>

<script>
var ws=new WebSocket('ws://'+location.hostname+':81/');
var dist=-1, ESIK=35, MAXCM=200;
ws.onopen=function(){document.getElementById('st').innerText='bagli';};
ws.onclose=function(){document.getElementById('st').innerText='baglanti koptu';};
ws.onmessage=function(e){var m=e.data; if(m[0]=='D'){dist=parseInt(m.substring(1));}};

// yon butonlari (basili tut = hareket, birak = dur)
function bind(id,c){var b=document.getElementById(id);
 var d=function(e){e.preventDefault();ws.send(c);};
 var u=function(e){e.preventDefault();ws.send('S');};
 b.addEventListener('touchstart',d);b.addEventListener('touchend',u);
 b.addEventListener('mousedown',d);b.addEventListener('mouseup',u);b.addEventListener('mouseleave',u);}
bind('F','F');bind('B','B');bind('L','L');bind('R','R');
document.getElementById('S').addEventListener('click',function(){ws.send('S');});

// mod butonlari
var bM=document.getElementById('M'), bA=document.getElementById('A');
bM.addEventListener('click',function(){ws.send('M');bM.className='on';bA.className='';});
bA.addEventListener('click',function(){ws.send('A');bA.className='on';bM.className='';});

// hiz
var sp=document.getElementById('sp');
sp.addEventListener('input',function(){document.getElementById('v').innerText=sp.value;ws.send('V'+sp.value);});

// --- TF-Luna isin gorsellestirme ---
var cv=document.getElementById('rad'), ctx=cv.getContext('2d');
function resize(){cv.width=cv.clientWidth;cv.height=cv.clientHeight;}
window.addEventListener('resize',resize); resize();
function draw(){
 var W=cv.width,H=cv.height,cx=W/2,cy=H-8;
 ctx.clearRect(0,0,W,H);
 var obj=(dist>0 && dist<ESIK);
 var col=obj?'#ff4d4d':'#3fb0ff';
 var reach=(dist>0?Math.min(dist,MAXCM):MAXCM);
 var len=(reach/MAXCM)*(H-24);
 // isin demeti (fan)
 var rays=9, spread=0.5;
 ctx.strokeStyle=col; ctx.lineWidth=2;
 for(var i=0;i<rays;i++){
   var t=-spread/2+spread*i/(rays-1);
   ctx.globalAlpha=0.35+0.4*(1-Math.abs(t)/(spread/2));
   ctx.beginPath();ctx.moveTo(cx,cy);
   ctx.lineTo(cx+Math.sin(t)*len, cy-Math.cos(t)*len);ctx.stroke();
 }
 ctx.globalAlpha=1;
 // tespit edilen nesne cubugu
 if(dist>0){
   var oy=cy-(Math.min(dist,MAXCM)/MAXCM)*(H-24);
   ctx.strokeStyle=col; ctx.lineWidth=5;
   ctx.beginPath();ctx.moveTo(cx-46,oy);ctx.lineTo(cx+46,oy);ctx.stroke();
 }
 // sensor
 ctx.fillStyle='#8fbf5f';ctx.beginPath();ctx.arc(cx,cy,6,0,7);ctx.fill();
 // yazi
 ctx.fillStyle=col;ctx.font='bold 24px sans-serif';ctx.textAlign='center';
 ctx.fillText((dist>0?dist:'--')+' cm', cx, 28);
 if(obj){ctx.font='bold 13px sans-serif';ctx.fillText('ENGEL!', cx, 48);}
 requestAnimationFrame(draw);
}
draw();
</script></body></html>
)HTML";

void onWs(uint8_t num, WStype_t type, uint8_t* payload, size_t len){
  if(type==WStype_TEXT && len>0){
    char c = payload[0];
    if(c=='V'){ HIZ = atoi((char*)payload+1); }
    else if(c=='A'){ autoMode = true; }
    else if(c=='M'){ autoMode = false; cmd='S'; dur(); }
    else { cmd = c; }
  }
}

void setup(){
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, TF_RX, TF_TX);
  pinMode(IN1,OUTPUT);pinMode(IN2,OUTPUT);pinMode(IN3,OUTPUT);pinMode(IN4,OUTPUT);
  ledcAttach(ENA,1000,8);
  ledcAttach(ENB,1000,8);
  dur();
  WiFi.softAP(ssid, password);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
  server.on("/", [](){ server.send(200,"text/html",PAGE); });
  server.begin();
  webSocket.begin();
  webSocket.onEvent(onWs);
  Serial.println("Hazir: http://192.168.4.1");
}

void loop(){
  webSocket.loop();
  server.handleClient();
  readTFLuna();
  bool engel = (tfDist > 0 && tfDist < ESIK);
  if(millis() - lastSend > 150){
    lastSend = millis();
    webSocket.broadcastTXT(String("D") + tfDist);
  }
  if(autoMode){
    if(tfDist <= 0) dur();
    else if(engel)  saga();
    else            ileri();
  } else {
    if(cmd=='F'){ if(engel) dur(); else ileri(); }
    else if(cmd=='B') geri();
    else if(cmd=='L') sola();
    else if(cmd=='R') saga();
    else dur();
  }
}
