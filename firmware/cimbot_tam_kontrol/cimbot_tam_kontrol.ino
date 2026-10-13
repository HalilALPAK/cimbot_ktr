/*
 * ÇİMBOT - TAM KONTROL
 * Sürüş (ön/arka/sağ/sol) + Kesici motor (BIÇAK aç/kapa + hız) + TF-Luna + AUTO
 * ESP32 + L298N + 2 sürüş motoru + RS775(IRLZ44N) + TF-Luna(UART)
 *
 * GEREKLİ KÜTÜPHANE: "WebSockets" by Markus Sattler
 *
 * Pinler:
 *   Sürüş: ENA14 IN1-27 IN2-26 IN3-25 IN4-33 ENB32
 *   Kesici (MOSFET gate): GPIO13  (220Ω/100Ω üzerinden)
 *   TF-Luna: Serial2  RX=16 (TF TXD buraya), TX=17
 *
 * KULLANIM: "CIMBOT" WiFi (12345678) -> http://192.168.4.1
 *   !!! Kesiciyi ilk MISINASIZ dene, etrafi bosalt, gozluk tak !!!
 */

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

// Sürüş
#define ENA 14
#define IN1 27
#define IN2 26
#define IN3 25
#define IN4 33
#define ENB 32
// Kesici
#define CUT 13
// TF-Luna
#define TF_RX 16
#define TF_TX 17
#define ESIK 35          // cm - engel eşiği

const char* ssid = "CIMBOT";
const char* password = "12345678";
WebServer server(80);
WebSocketsServer webSocket(81);

int  HIZ = 180;          // sürüş hızı 0-255
char cmd = 'S';          // sürüş komutu
bool autoMode = false;
int  tfDist = -1;        // TF-Luna mesafe cm

// Kesici (yumuşak başlatma)
bool bladeOn = false;
int  bladeSpeed = 180;   // hedef PWM 0-255
int  cutCur = 0;         // anlık PWM (rampalı)
unsigned long lastRamp = 0, lastSend = 0;

// ---- Sürüş fonksiyonları ----
void hiz(int s){ ledcWrite(ENA,s); ledcWrite(ENB,s); }
void durDrive(){ ledcWrite(ENA,0); ledcWrite(ENB,0);
  digitalWrite(IN1,LOW);digitalWrite(IN2,LOW);digitalWrite(IN3,LOW);digitalWrite(IN4,LOW); }
void ileri(){ digitalWrite(IN1,HIGH);digitalWrite(IN2,LOW); digitalWrite(IN3,HIGH);digitalWrite(IN4,LOW); hiz(HIZ); }
void geri (){ digitalWrite(IN1,LOW); digitalWrite(IN2,HIGH);digitalWrite(IN3,LOW); digitalWrite(IN4,HIGH);hiz(HIZ); }
void sola (){ digitalWrite(IN1,LOW); digitalWrite(IN2,HIGH);digitalWrite(IN3,HIGH);digitalWrite(IN4,LOW); hiz(HIZ); }
void saga (){ digitalWrite(IN1,HIGH);digitalWrite(IN2,LOW); digitalWrite(IN3,LOW); digitalWrite(IN4,HIGH);hiz(HIZ); }

// ---- TF-Luna oku ----
void readTFLuna(){
  while(Serial2.available() >= 9){
    if(Serial2.read()!=0x59) continue;
    if(Serial2.read()!=0x59) continue;
    uint8_t f[7]; Serial2.readBytes(f,7);
    int d=f[0]|(f[1]<<8);
    uint16_t sum=0x59+0x59; for(int i=0;i<6;i++) sum+=f[i];
    if((sum&0xFF)==f[6]) tfDist=d;
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
#rad{width:100%;height:170px;display:block}
.st{color:#8fbf5f;font-size:13px;margin:2px 0 8px}
.grid{display:grid;grid-template-columns:repeat(3,76px);grid-gap:9px;justify-content:center}
button{height:76px;font-size:22px;border:none;border-radius:14px;background:#2c5f2d;color:#fff}
button:active{background:#8fbf5f}
.stop{background:#a33}
.blade{width:100%;height:60px;margin-top:14px;font-size:20px;border:none;border-radius:14px;background:#555;color:#fff}
.blade.on{background:#d33}
.estop{width:100%;height:64px;margin-top:10px;font-size:22px;font-weight:bold;border:none;border-radius:14px;background:#b00020;color:#fff}
.modes{display:flex;gap:9px;margin-top:12px}
.modes button{flex:1;height:48px;font-size:17px;background:#22405f}
.modes button.on{background:#e7b84b;color:#222}
.sp{margin-top:12px;font-size:15px}
input[type=range]{width:85%;height:30px}
.lbl{font-size:14px;color:#cfe0c2;margin-top:10px}
</style></head><body>
<h2>ÇİMBOT</h2>
<div id="panel"><canvas id="rad"></canvas></div>
<div class="st" id="st">baglaniyor...</div>

<div class="grid">
<div></div><button id="F">&#9650;</button><div></div>
<button id="L">&#9664;</button><button class="stop" id="S">&#9632;</button><button id="R">&#9654;</button>
<div></div><button id="B">&#9660;</button><div></div>
</div>

<button class="blade" id="K">BIÇAK: KAPALI</button>
<div class="lbl">Biçak hizi: <span id="wv">180</span></div>
<input type="range" min="0" max="255" value="180" id="ws">

<div class="modes">
<button id="M" class="on">MANUEL</button>
<button id="A">AUTO</button>
</div>

<div class="sp">Sürüş hizi: <span id="v">180</span><br><input type="range" min="0" max="255" value="180" id="sp"></div>

<button class="estop" id="X">ACİL STOP</button>

<script>
var ws=new WebSocket('ws://'+location.hostname+':81/');
var dist=-1, ESIK=35, MAXCM=200, blade=false;
ws.onopen=function(){document.getElementById('st').innerText='bagli';};
ws.onclose=function(){document.getElementById('st').innerText='baglanti koptu';};
ws.onmessage=function(e){var m=e.data; if(m[0]=='D'){dist=parseInt(m.substring(1));}};

function bind(id,c){var b=document.getElementById(id);
 var d=function(e){e.preventDefault();ws.send(c);};
 var u=function(e){e.preventDefault();ws.send('S');};
 b.addEventListener('touchstart',d);b.addEventListener('touchend',u);
 b.addEventListener('mousedown',d);b.addEventListener('mouseup',u);b.addEventListener('mouseleave',u);}
bind('F','F');bind('B','B');bind('L','L');bind('R','R');
document.getElementById('S').addEventListener('click',function(){ws.send('S');});

// bicak
document.getElementById('K').addEventListener('click',function(){
 blade=!blade; ws.send('K');
 var b=document.getElementById('K');
 b.innerText='BIÇAK: '+(blade?'ACIK':'KAPALI'); b.className='blade'+(blade?' on':'');});
var wsl=document.getElementById('ws');
wsl.addEventListener('input',function(){document.getElementById('wv').innerText=wsl.value;ws.send('W'+wsl.value);});

// mod
var bM=document.getElementById('M'), bA=document.getElementById('A');
bM.addEventListener('click',function(){ws.send('M');bM.className='on';bA.className='';});
bA.addEventListener('click',function(){ws.send('A');bA.className='on';bM.className='';});

// suru hizi
var sp=document.getElementById('sp');
sp.addEventListener('input',function(){document.getElementById('v').innerText=sp.value;ws.send('V'+sp.value);});

// ACIL STOP
document.getElementById('X').addEventListener('click',function(){
 ws.send('X'); blade=false;
 var b=document.getElementById('K'); b.innerText='BIÇAK: KAPALI'; b.className='blade';
 bM.className='on'; bA.className='';});

// TF-Luna gorsel
var cv=document.getElementById('rad'), ctx=cv.getContext('2d');
function resize(){cv.width=cv.clientWidth;cv.height=cv.clientHeight;}
window.addEventListener('resize',resize); resize();
function draw(){
 var W=cv.width,H=cv.height,cx=W/2,cy=H-8;
 ctx.clearRect(0,0,W,H);
 var obj=(dist>0&&dist<ESIK), col=obj?'#ff4d4d':'#3fb0ff';
 var reach=(dist>0?Math.min(dist,MAXCM):MAXCM), len=(reach/MAXCM)*(H-24);
 ctx.strokeStyle=col; ctx.lineWidth=2;
 for(var i=0;i<9;i++){var t=-0.25+0.5*i/8;
  ctx.globalAlpha=0.35+0.4*(1-Math.abs(t)/0.25);
  ctx.beginPath();ctx.moveTo(cx,cy);ctx.lineTo(cx+Math.sin(t)*len,cy-Math.cos(t)*len);ctx.stroke();}
 ctx.globalAlpha=1;
 if(dist>0){var oy=cy-(Math.min(dist,MAXCM)/MAXCM)*(H-24);
  ctx.strokeStyle=col;ctx.lineWidth=5;ctx.beginPath();ctx.moveTo(cx-46,oy);ctx.lineTo(cx+46,oy);ctx.stroke();}
 ctx.fillStyle='#8fbf5f';ctx.beginPath();ctx.arc(cx,cy,6,0,7);ctx.fill();
 ctx.fillStyle=col;ctx.font='bold 22px sans-serif';ctx.textAlign='center';
 ctx.fillText((dist>0?dist:'--')+' cm',cx,26);
 if(obj){ctx.font='bold 13px sans-serif';ctx.fillText('ENGEL!',cx,46);}
 requestAnimationFrame(draw);
}
draw();
</script></body></html>
)HTML";

void onWs(uint8_t num, WStype_t type, uint8_t* payload, size_t len){
  if(type==WStype_TEXT && len>0){
    char c=payload[0];
    if(c=='V'){ HIZ=atoi((char*)payload+1); }
    else if(c=='W'){ bladeSpeed=atoi((char*)payload+1); }
    else if(c=='A'){ autoMode=true; }
    else if(c=='M'){ autoMode=false; cmd='S'; durDrive(); }
    else if(c=='K'){ bladeOn=!bladeOn; }           // biçak aç/kapa
    else if(c=='X'){ // ACIL STOP: her sey dursun
      autoMode=false; cmd='S'; durDrive();
      bladeOn=false; cutCur=0; ledcWrite(CUT,0);
    }
    else { cmd=c; }                                 // F/B/L/R/S
  }
}

void setup(){
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, TF_RX, TF_TX);
  pinMode(IN1,OUTPUT);pinMode(IN2,OUTPUT);pinMode(IN3,OUTPUT);pinMode(IN4,OUTPUT);
  ledcAttach(ENA,1000,8);
  ledcAttach(ENB,1000,8);
  ledcAttach(CUT,1000,8);      // kesici PWM
  durDrive(); ledcWrite(CUT,0);

  WiFi.softAP(ssid,password);
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

  bool engel = (tfDist>0 && tfDist<ESIK);

  // canli mesafe
  if(millis()-lastSend>150){ lastSend=millis(); webSocket.broadcastTXT(String("D")+tfDist); }

  // --- SÜRÜŞ ---
  if(autoMode){
    if(tfDist<=0) durDrive();
    else if(engel) saga();
    else ileri();
  } else {
    if(cmd=='F'){ if(engel) durDrive(); else ileri(); }
    else if(cmd=='B') geri();
    else if(cmd=='L') sola();
    else if(cmd=='R') saga();
    else durDrive();
  }

  // --- KESİCİ (yumuşak başlatma / rampa) ---
  int target = bladeOn ? bladeSpeed : 0;
  if(millis()-lastRamp>10){
    lastRamp=millis();
    if(cutCur < target) cutCur += 2;        // yavaş yüksel (inrush düşük)
    else if(cutCur > target) cutCur -= 6;   // hızlı düş
    if(cutCur<0) cutCur=0; if(cutCur>255) cutCur=255;
    ledcWrite(CUT, cutCur);
  }
}
