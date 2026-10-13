/*
 * CIMBOT - JOYSTICK KONTROL v5
 *  - Joystick differential; ic teker DURMAZ (DONUS_GUCU ile ayarlanir)
 *  - Geri + sag/sol ; Surus hizi ; AUTO swerve ; olu adam dur
 *
 * AYAR DEGISKENLERI:
 *   DONUS_GUCU : 0-1. Dusur=iki teker de doner(genis viraj, ic teker yavas ama donmeye devam),
 *                      artir=keskin donus (yuksekse ic teker kosede durabilir). Varsayilan 0.5
 *   STEER_SIGN : donus ters gelirse -1 yap
 *
 * Pinler: Surus ENA14 IN1-27 IN2-26 IN3-25 IN4-33 ENB32 ; Kesici gate GPIO13
 *         TF-Luna Serial2 RX=16 TX=17 ; Kutuphane: "WebSockets" by Markus Sattler
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
#define CUT 13
#define TF_RX 16
#define TF_TX 17
#define ESIK 35

#define STEER_SIGN 1       // donus ters gelirse -1 yap
float DONUS_GUCU = 0.5;    // <<< BU DEGISKENI AYARLA (0-1). dusur=genis/ic teker donmeye devam, artir=keskin

const char* ssid = "CIMBOT";
const char* password = "12345678";
WebServer server(80);
WebSocketsServer webSocket(81);

int  HIZ = 180;
int  jx = 0, jy = 0;
unsigned long lastJoy = 0;
bool autoMode = false;
int  tfDist = -1;
bool bladeOn = false;
int  bladeSpeed = 180;
int  cutCur = 0;
unsigned long lastRamp = 0, lastSend = 0;

int aSt = 0; unsigned long aT = 0;
#define T_TURN 600
#define T_PASS 800
#define T_RET  600

void setLeft(int s){ s=constrain(s,-255,255);
  if(s>=0){ digitalWrite(IN1,HIGH);digitalWrite(IN2,LOW);  ledcWrite(ENA,s); }
  else    { digitalWrite(IN1,LOW); digitalWrite(IN2,HIGH); ledcWrite(ENA,-s); } }
void setRight(int s){ s=constrain(s,-255,255);
  if(s>=0){ digitalWrite(IN3,LOW); digitalWrite(IN4,HIGH); ledcWrite(ENB,s); }
  else    { digitalWrite(IN3,HIGH);digitalWrite(IN4,LOW);  ledcWrite(ENB,-s); } }
void drive(int l,int r){ setLeft(l); setRight(r); }

void applyJoy(){
  float x=(jx/100.0)*STEER_SIGN;
  float y=jy/100.0;
  float l = y - x*DONUS_GUCU;      // ic teker DONUS_GUCU kadar yavaslar ama DURMAZ
  float r = y + x*DONUS_GUCU;      // saga cekince sag teker hizli, sol yavas ama doner
  l=constrain(l,-1.0,1.0); r=constrain(r,-1.0,1.0);
  drive((int)(l*HIZ),(int)(r*HIZ));
}

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
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<style>
*{box-sizing:border-box}
body{font-family:sans-serif;text-align:center;background:#0f2413;color:#fff;
 user-select:none;-webkit-user-select:none;touch-action:none;margin:0;padding:10px}
h2{color:#8fbf5f;margin:4px 0}
#panel{background:#0a1a0d;border-radius:16px;padding:6px;margin:6px 0}
#rad{width:100%;height:150px;display:block}
.st{color:#8fbf5f;font-size:13px;margin:2px 0 6px}
#joy{display:block;margin:6px auto;touch-action:none}
.blade{width:100%;height:58px;margin-top:10px;font-size:20px;border:none;border-radius:14px;background:#555;color:#fff}
.blade.on{background:#d33}
.estop{width:100%;height:62px;margin-top:8px;font-size:22px;font-weight:bold;border:none;border-radius:14px;background:#b00020;color:#fff}
.modes{display:flex;gap:9px;margin-top:10px}
.modes button{flex:1;height:46px;font-size:17px;border:none;border-radius:14px;background:#22405f;color:#fff}
.modes button.on{background:#e7b84b;color:#222}
.sp{margin-top:10px;font-size:15px}
input[type=range]{width:85%;height:30px}
.lbl{font-size:14px;color:#cfe0c2;margin-top:8px}
</style></head><body>
<h2>CIMBOT</h2>
<div id="panel"><canvas id="rad"></canvas></div>
<div class="st" id="st">baglaniyor...</div>

<canvas id="joy" width="200" height="200"></canvas>

<button class="blade" id="K">BICAK: KAPALI</button>
<div class="lbl">Bicak hizi: <span id="wv">180</span></div>
<input type="range" min="0" max="255" value="180" id="ws">

<div class="modes">
<button id="M" class="on">MANUEL</button>
<button id="A">AUTO</button>
</div>

<div class="sp">Surus hizi: <span id="v">180</span><br><input type="range" min="0" max="255" value="180" id="sp"></div>

<button class="estop" id="X">ACIL STOP</button>

<script>
var ws=new WebSocket('ws://'+location.hostname+':81/');
var dist=-1, ESIK=35, MAXCM=200, blade=false;
ws.onopen=function(){document.getElementById('st').innerText='bagli';};
ws.onclose=function(){document.getElementById('st').innerText='baglanti koptu';};
ws.onmessage=function(e){var m=e.data; if(m[0]=='D'){dist=parseInt(m.substring(1));}};

var joy=document.getElementById('joy'), jc=joy.getContext('2d');
var jr=80, kx=0, ky=0, drag=false, lastJ=0;
function jdraw(){
 jc.clearRect(0,0,200,200);
 jc.fillStyle='#0a1a0d'; jc.beginPath(); jc.arc(100,100,jr,0,7); jc.fill();
 jc.strokeStyle='#2c5f2d'; jc.lineWidth=3; jc.stroke();
 jc.fillStyle='#8fbf5f'; jc.beginPath(); jc.arc(100+kx,100+ky,30,0,7); jc.fill();
 requestAnimationFrame(jdraw);
}
jdraw();
function jpos(e){
 var r=joy.getBoundingClientRect();
 var t=e.touches?e.touches[0]:e;
 var x=t.clientX-r.left-100, y=t.clientY-r.top-100;
 var d=Math.sqrt(x*x+y*y); if(d>jr){ x=x/d*jr; y=y/d*jr; }
 kx=x; ky=y;
 var now=Date.now();
 if(now-lastJ>60){ lastJ=now; ws.send('J'+Math.round(x/jr*100)+','+Math.round(-y/jr*100)); }
}
function jend(){ kx=0;ky=0;drag=false; ws.send('J0,0'); }
joy.addEventListener('touchstart',function(e){e.preventDefault();drag=true;jpos(e);});
joy.addEventListener('touchmove', function(e){e.preventDefault();if(drag)jpos(e);});
joy.addEventListener('touchend',  function(e){e.preventDefault();jend();});
joy.addEventListener('mousedown', function(e){drag=true;jpos(e);});
joy.addEventListener('mousemove', function(e){if(drag)jpos(e);});
joy.addEventListener('mouseup',   function(e){jend();});
joy.addEventListener('mouseleave',function(e){if(drag)jend();});

document.getElementById('K').addEventListener('click',function(){
 blade=!blade; ws.send('K');
 var b=document.getElementById('K');
 b.innerText='BICAK: '+(blade?'ACIK':'KAPALI'); b.className='blade'+(blade?' on':'');});
var wsl=document.getElementById('ws');
wsl.addEventListener('input',function(){document.getElementById('wv').innerText=wsl.value;ws.send('W'+wsl.value);});

var bM=document.getElementById('M'), bA=document.getElementById('A');
bM.addEventListener('click',function(){ws.send('M');bM.className='on';bA.className='';});
bA.addEventListener('click',function(){ws.send('A');bA.className='on';bM.className='';});

var sp=document.getElementById('sp');
sp.addEventListener('input',function(){document.getElementById('v').innerText=sp.value;ws.send('V'+sp.value);});

document.getElementById('X').addEventListener('click',function(){
 ws.send('X'); blade=false; kx=0;ky=0;
 var b=document.getElementById('K'); b.innerText='BICAK: KAPALI'; b.className='blade';
 bM.className='on'; bA.className='';});

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
 ctx.fillText((dist>0?dist:'--')+' cm',cx,24);
 if(obj){ctx.font='bold 13px sans-serif';ctx.fillText('ENGEL!',cx,44);}
 requestAnimationFrame(draw);
}
draw();
</script></body></html>
)HTML";

void onWs(uint8_t num, WStype_t type, uint8_t* payload, size_t len){
  if(type==WStype_TEXT && len>0){
    char buf[24]; int n=len<23?len:23; memcpy(buf,payload,n); buf[n]=0;
    char c=buf[0];
    if(c=='J'){ sscanf(buf+1,"%d,%d",&jx,&jy); lastJoy=millis(); }
    else if(c=='V'){ HIZ=atoi(buf+1); }
    else if(c=='W'){ bladeSpeed=atoi(buf+1); }
    else if(c=='A'){ autoMode=true; aSt=0; }
    else if(c=='M'){ autoMode=false; drive(0,0); }
    else if(c=='K'){ bladeOn=!bladeOn; }
    else if(c=='X'){ autoMode=false; drive(0,0); jx=0; jy=0; bladeOn=false; cutCur=0; ledcWrite(CUT,0); }
  }
}

void setup(){
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, TF_RX, TF_TX);
  pinMode(IN1,OUTPUT);pinMode(IN2,OUTPUT);pinMode(IN3,OUTPUT);pinMode(IN4,OUTPUT);
  ledcAttach(ENA,1000,8);
  ledcAttach(ENB,1000,8);
  ledcAttach(CUT,1000,8);
  drive(0,0); ledcWrite(CUT,0);
  WiFi.softAP(ssid,password);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
  server.on("/", [](){ server.send(200,"text/html; charset=UTF-8",PAGE); });
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
  if(millis()-lastSend>150){ lastSend=millis(); webSocket.broadcastTXT(String("D")+tfDist); }

  if(autoMode){
    switch(aSt){
      case 0:
        if(tfDist<=0) drive(0,0);
        else if(engel){ aSt=1; aT=millis(); }
        else drive(HIZ,HIZ);
        break;
      case 1:
        drive(HIZ, (int)(HIZ*0.1));
        if(millis()-aT>T_TURN){ aSt=2; aT=millis(); }
        break;
      case 2:
        if(engel){ aSt=1; aT=millis(); }
        else { drive(HIZ,HIZ); if(millis()-aT>T_PASS){ aSt=3; aT=millis(); } }
        break;
      case 3:
        drive((int)(HIZ*0.1), HIZ);
        if(millis()-aT>T_RET){ aSt=0; }
        break;
    }
  } else {
    if(millis()-lastJoy < 400) applyJoy();
    else drive(0,0);
  }

  int target = bladeOn ? bladeSpeed : 0;
  if(millis()-lastRamp>10){
    lastRamp=millis();
    if(cutCur < target) cutCur += 2;
    else if(cutCur > target) cutCur -= 6;
    if(cutCur<0) cutCur=0; if(cutCur>255) cutCur=255;
    ledcWrite(CUT, cutCur);
  }
}
