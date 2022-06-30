#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <HardwareSerial.h>


const char *ssid = "Orange_Swiatlowod_095A";
const char *password = "WWZYKLF6PR4V";//dane sieci

HardwareSerial mySerial(1);


IPAddress local_IP(192, 168, 1, 29);
//aby zobaczyc sterowania
//http://192.168.1.31
//http://192.168.1.29
//http://192.168.1.29/distance
//http://192.168.1.29/slider
//skozystaj z linkow
IPAddress gateway(192, 168, 1, 2);
IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

//sterownik silnika
#define PWMA 23
#define PWMB 22
#define BIN2 5//lewy silnik - B
#define BIN1 18
#define AIN2 21//prawy silnik - A
#define AIN1 19
#define chanell 8//kanał PWN do sterownika
#define PWM_Freq  4000
#define PWM_Res 8
#define MAX_PWM 230//max do 7.5 V i ma byc 
//przy 6 V to 160obr  165 PWM
//przy 3 V to 60obr  83 PWM

//sonar
#define trigPin  4
#define echoPin  27
#define SOUND_SPEED_HALF 0.017//polowa prekosci swiatla
//pomiar v
#define trigPin2  15
#define echoPin2  14
//pomiar napiecia
#define v_baterry  36

//enkoder
#define ENC2A 35
#define ENCA 33
//UART
#define RXD2 16
#define TXD2 17

long prevT = 0;
int posPrev = 0;

int pos_i = 0;
float velocity = 0.0;
float velocity2 = 0.0;
long prevT_i = 0;
long prevT_iP = 0;
float v2Prev = 0.0;

float v1Prev = 0.0;
float v_in = 0.0;
float v_in1 = 0.0;
float v_in2 = 0.0;
float v_in3 = 0.0;
int i =0;
bool left_encoder=false;
long deltaT =0;
bool right_encoder=false;
long deltaTP =0;
int incrementP= 1;
int incrementL= 1;
long lastMillis =0;
float tab[]={0,0,0,0,0,0,0,0,0,0};
int num=0;
int a=0;
int setpoint = 0;
int setpoint2 = 0;
float deltaTC =0.0;
volatile bool interruptpi=false;
volatile bool interruptdist=false;
volatile bool interruptpomiar=false;
String Front="";
String Back="";
float v1Filt = 0.0;
float v1PrevF = 0.0;

float v2Filt = 0.0;
float v2PrevF = 0.0;



hw_timer_t * timer = NULL;
hw_timer_t * timer1 = NULL;
hw_timer_t * timer2 = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

float prevT_iC =0.0;
/* zwykły PID
float  kp=10.0;
float ki=24.0;
float kp2=10.0;
float ki2=24.0;
*/
 



/*// z samą synchronizacją kół
float kp=10.0;
float ki=24.0;
float kp2=32.0;
float ki2=200.0; 

*/


float kp=10.0;
float ki=24.0;

float kp2=10.0;
float ki2=24.0; 

float kp3=32.0;
float ki3=200.0; 
//ostatni regulator


float e=0.0;
float u=0.0;
float calka=0.0;
float dr=0.0;
float ep=0.0;

float e2=0.0;
float u2=0.0;
float calka2=0.0;
float dr2=0.0;
float ep2=0.0;
float pr1=0.0;

float e3=0.0;
float calka3=0.0;


String button = "stop";//kierunek jazdy
float Distance_Cm=0.0;
float Distance_Cm2=0.0;
float lastDistance_Cm=0.0;
float lastDistance_Cm2=0.0;
bool error_front = 0;//wykrycie kolizji
bool error_back = 0;//wykrycie kolizji
int colision_distance = 20;//dopuszczalny dystans
String pwmSliderValue = "0";

AsyncWebServer server(80);
void EncoderL();
void IRAM_ATTR check_EncoderL();
void IRAM_ATTR check_EncoderP();
void EncoderP();

void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  interruptpi=true;
  portEXIT_CRITICAL_ISR(&timerMux);
 
}
void IRAM_ATTR onTimer1() {
  portENTER_CRITICAL_ISR(&timerMux);
  interruptpomiar=true;
  portEXIT_CRITICAL_ISR(&timerMux);
 
}
void IRAM_ATTR onTimer2() {
  portENTER_CRITICAL_ISR(&timerMux);
  interruptdist=true;
  portEXIT_CRITICAL_ISR(&timerMux);
 
}
void move(String direction)
{ // wykonanie polecenia z przegladarki
  //z uwzglednieniem kolizji
  
  if (direction == "forward" && !colision_detect()) {
   // Serial.println("Forward");
   setpoint=abs(setpoint);
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
    digitalWrite(BIN1, HIGH);
    digitalWrite(BIN2, LOW);
    setpoint2=1;
  }
  if (direction == "left") {
   //Serial.println("Left");
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
    digitalWrite(BIN1, LOW);
    digitalWrite(BIN2, HIGH);
    //setpoint=-abs(setpoint);
    //setpoint2=-1;
  }
  if (direction == "right") {
   // Serial.println("Right");
//setpoint=abs(setpoint);
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);
    digitalWrite(BIN1, HIGH);
    digitalWrite(BIN2, LOW);
    //setpoint2=-1;
  }
  if (direction == "backward"&& !colision_detect_back()) {
    //Serial.println("Backward");
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);
    digitalWrite(BIN1, LOW);
    digitalWrite(BIN2, HIGH);
    //setpoint=-abs(setpoint);
    //setpoint2=1;
  }
  if (direction == "stop") {
   // Serial.println("Stop");
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, LOW);
    digitalWrite(BIN1, LOW);
    digitalWrite(BIN2, LOW);
    e=0.0;
    u=0.0;
    calka=0.0;
    e2=0.0;
    u2=0.0;
    calka2=0.0;
    dr=0.0;
    ep=0.0;

    e3=0.0;
    
    calka3=0.0;
  }
}

bool colision_detect()
{
  //detekcja kolizji oraz zatrzymanie robota
  if (Distance_Cm > colision_distance && button == "forward" && lastDistance_Cm <= colision_distance)
  {
    lastDistance_Cm = Distance_Cm;
    error_front = 0;
    button = "forward";
    move("forward");
    return 0;

  }
  else if (Distance_Cm <= colision_distance && button == "forward")
  {
    lastDistance_Cm = Distance_Cm;
    error_front = 1;
    button = "stop";
    move("stop");
    return 1;
  }
  else
  {
    lastDistance_Cm = Distance_Cm;
    error_front = 0;
    return 0;
  }
 
}
bool colision_detect_back()
{
  //detekcja kolizji oraz zatrzymanie robota
  if (Distance_Cm2 > colision_distance && button == "backward" && lastDistance_Cm2 <= colision_distance)
  {//juz nie ma kolizji
    lastDistance_Cm2 = Distance_Cm2;
    error_back = 0;
    button = "backward";
    move("backward");
    return 0;

  }
  else if (Distance_Cm2 <= colision_distance && button == "backward")
  {
    lastDistance_Cm2 = Distance_Cm2;
    error_back = 1;
    button = "stop";
    move("stop");
    return 1;
  }
  else
  {
    lastDistance_Cm2 = Distance_Cm2;
    error_back = 0;
    return 0;
  }
 
}
String v_measure()
{
 // v_in = (analogRead(v_baterry)/4095.0)*3.3) * 2.8;//28000/100000=2.8 z dzielnika napiecia
  v_in = ((analogRead(v_baterry)/4095.0)*3.3+0.19)*4.9;
  v_in3=v_in2;
  v_in2=v_in1;
  v_in1=v_in;
  v_in=(v_in1+v_in2+v_in3)/3.0;
  
   return String(v_in, 2);
}
void Front_sensor()
{
  //obsluga HC-SR04
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Sposob proponowany przez producenta
  Distance_Cm = pulseIn(echoPin, HIGH) * SOUND_SPEED_HALF;
  
  //Serial.print("Distance (cm): ");
  //Serial.println(Distance_Cm);
  
  colision_detect();
  
  Front=String(Distance_Cm, 2);
}
void Back_sensor()
{
  //obsluga HC-SR04
  digitalWrite(trigPin2, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin2, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin2, LOW);
  // Sposob proponowany przez producenta
  Distance_Cm2 = pulseIn(echoPin2, HIGH) * SOUND_SPEED_HALF;
  
 // Serial.print("Distance (cm): ");
 // Serial.println(Distance_Cm2);
  
  colision_detect_back();
  
  Back=String(Distance_Cm2, 2);
}

//czysty string
const char index_html[] PROGMEM = R"rawliteral(
<html>
  <head>
    <title>Praca inżynierska</title>
    <meta charset="UTF-8">
    <style>
      body {text-align: center;}
      table { margin-left: auto; margin-right: auto; }
.button {
    display: inline-block;
    text-align: center;
    vertical-align: middle;
    height:40px;
    width:100px;
     cursor: pointer;
    border: 0px solid #ffffff;
    border-radius: 0px;
    background: #7e6bed;
    background: -webkit-gradient(linear, left top, left bottom, from(#7e6bed), to(#0e0b57));
    background: -moz-linear-gradient(top, #7e6bed, #0e0b57);
    background: linear-gradient(to bottom, #7e6bed, #0e0b57);
    font: 18px arial;
    color: #ffffff;
   
}
.button:hover,
.button:focus {
    background: #9780ff;
    background: -webkit-gradient(linear, left top, left bottom, from(#9780ff), to(#110d68));
    background: -moz-linear-gradient(top, #9780ff, #110d68);
    background: linear-gradient(to bottom, #9780ff, #110d68);
    color: #ffffff;
}
.button:active {
    background: #4c408e;
    background: -webkit-gradient(linear, left top, left bottom, from(#4c408e), to(#0e0b57));
    background: -moz-linear-gradient(top, #4c408e, #0e0b57);
    background: linear-gradient(to bottom, #4c408e, #0e0b57);
}


      .Button2 {
  background:linear-gradient(to bottom, #ffffff 5%, #ffffff 100%);
  background-color:#ffffff;
  border-radius:22px;
  border:1px solid #000000;
  display:inline-block;
  cursor:pointer;
  color:#000000;
  font-family:Arial;
  font-size:14px;
  padding:15px 34px;
  text-decoration:none;
}
.Button2:hover {
  background:linear-gradient(to bottom, #ffffff 5%, #ffffff 100%);
  background-color:#ffffff;
}
.Button2:active {
  position:relative;
  top:1px;
}

      img {  width: auto ; height: auto ; }
      .slider1 {
        -webkit-appearance: none;
         width: 500;
         height: 30px;
         background: #e0e0d1;
        }
    .slider1::-webkit-slider-thumb {
        -webkit-appearance: none;
         width: 30px;
         height: 30px;
         cursor: pointer;
         background: #ffd100;
}
    </style>
  </head>
  <body>
    <h1>Robot mobilny wyposażony w kamerę</h1>
    
    <img src="http://192.168.1.27/stream" >
    <br>
    <br>
    <button class="button" onmousedown="toggleCheckbox('forward');" ontouchstart="toggleCheckbox('forward');" onmouseup="toggleCheckbox('stop');" ontouchend="toggleCheckbox('stop');">Przód</button>
      <br>
      <br>
    <button class="button" onmousedown="toggleCheckbox('left');" ontouchstart="toggleCheckbox('left');" onmouseup="toggleCheckbox('stop');" ontouchend="toggleCheckbox('stop');">Lewo</button>
     
    <button class="button" onmousedown="toggleCheckbox('stop');" ontouchstart="toggleCheckbox('stop');">Stop</button>

    <button class="button" onmousedown="toggleCheckbox('right');" ontouchstart="toggleCheckbox('right');" onmouseup="toggleCheckbox('stop');" ontouchend="toggleCheckbox('stop');">Prawo</button>
    <br>
    <br>
    <button class="button" onmousedown="toggleCheckbox('backward');" ontouchstart="toggleCheckbox('backward');" onmouseup="toggleCheckbox('stop');" ontouchend="toggleCheckbox('stop');">Tył</button> 
    
    <p><span id="textSliderValue">%SLIDERVALUE%</span> obr/min</p>
    <p><input type="range" onchange="updateSliderPWM(this)" id="pwmSlider" min="0" max="70" value="%SLIDERVALUE%" step="1" class="slider1"></p>
    
    <p>Odległość przód: <span id="distance">%DISTANCE%</span> cm &nbsp Odległość Tył: <span id="distance2">%DISTANCE2%</span> cm</p>
    <p>Napięcie baterii <span id="voltage">%VOLTAGE%</span> V</p>
    <button class="button2" onclick="on()">LED ON</button>
    <button class="button2" onclick="off()">LED OFF</button>
    
    <p><label>Skala szarości</label> 
<input type="checkbox" id="Check" onclick="check()"></p>
<p>Rozdzielczość:</p>

<label><input type="radio" name="res" onclick="check2()"checked> VGA</label>
<label><input type="radio" name="res" onclick="check3()"> HVGA</label>

<p>Jakość <span id="jakosc">%SLIDER%</span></p>
    <p><input type="range" onchange="updateSlider(this)" id="jakoscSlider" min="10" max="60" value="10" step="1" class="slider2"></p>
    
  </body>
   <script>

   function updateSlider(element) {
  var SliderValue = document.getElementById("jakoscSlider").value;
  document.getElementById("jakosc").innerHTML = SliderValue;
  console.log(SliderValue);
  var httpRequest = new XMLHttpRequest();
  httpRequest.open("GET", "/CAM?button=quality"+SliderValue, true);
  httpRequest.send();
}
   function check() {
  var Box = document.getElementById("Check");
  if (Box.checked == true){
    var xhr = new XMLHttpRequest();
     xhr.open("GET", "/CAM?button=Gray", true);
     xhr.send();
  } else {
    var xhr = new XMLHttpRequest();
     xhr.open("GET", "/CAM?button=No Effect", true);
     xhr.send();
  }
}
function check2() {
 
    var xhr = new XMLHttpRequest();
     xhr.open("GET", "/CAM?button=VGA", true);
     xhr.send();
  
}
function check3() {
   var xhr = new XMLHttpRequest();
     xhr.open("GET", "/CAM?button=HVGA", true);
     xhr.send();
  }

   function toggleCheckbox(x) {
     var xhr = new XMLHttpRequest();
     xhr.open("GET", "/move?button=" + x, true);
     xhr.send();
   }
    function on(x) {
     var xhr = new XMLHttpRequest();
     xhr.open("GET", "/CAM?button=on", true);
     xhr.send();
   }
   function off(x) {
     var xhr = new XMLHttpRequest();
     xhr.open("GET", "/CAM?button=off", true);
     xhr.send();
   }
   
    setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("distance").innerHTML = this.responseText;
    }
  }
  xhttp.open("GET", "/distance", true);
  xhttp.send();
}, 500 ) 
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("distance2").innerHTML = this.responseText;
    }
  }
  xhttp.open("GET", "/distance2", true);
  xhttp.send();
}, 500 ) 

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("voltage").innerHTML = this.responseText;
    }
  }
  xhttp.open("GET", "/voltage", true);
  xhttp.send();
}, 1000 ) 

function updateSliderPWM(element) {
  var pwmSliderValue = document.getElementById("pwmSlider").value;
  document.getElementById("textSliderValue").innerHTML = pwmSliderValue;
  console.log(pwmSliderValue);
  var httpRequest = new XMLHttpRequest();
  httpRequest.open("GET", "/slider?value="+pwmSliderValue, true);
  httpRequest.send();
};
  </script>
</html>
)rawliteral";

//https://developer.mozilla.org/pl/docs/Web/API/XMLHttpRequest request
// wywolanie pierwszego wejscia do przegladarki
String processor(const String& var){
  //Serial.println(var);
  if(var == "DISTANCE"){
    return Front;
  }
  if(var == "DISTANCE2"){
    return Back;
  }
   if (var == "SLIDERVALUE"){
    return pwmSliderValue;
  }
  if (var == "VOLTAGE"){
    return v_measure();
  }
  if (var == "SLIDER"){
    return "10";
  }
  return String();
}
void Wifi_rozlaczono(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.print("Wifi utracone: ");
  Serial.println(info.disconnected.reason);
  Serial.println("...");
  move("stop");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
delay(500);
Serial.print(",");
}
}
void setup(){
   WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable   detector
Serial.begin(115200);// port szeregowy

mySerial.begin(115200, SERIAL_8N1, RXD2, TXD2);//115200 bit/s

pinMode(trigPin, OUTPUT); //
pinMode(echoPin, INPUT); //
pinMode(trigPin2, OUTPUT); //
pinMode(echoPin2, INPUT); //piny do obslugi HC-SR04

pinMode(v_baterry,INPUT);
pinMode(AIN2, OUTPUT);
pinMode(AIN1, OUTPUT);
digitalWrite(AIN1, LOW);
digitalWrite(AIN2, LOW);

pinMode(BIN2, OUTPUT);
pinMode(BIN1, OUTPUT);
digitalWrite(BIN1, LOW);
digitalWrite(BIN2, LOW);//piny obslugi sterownika silnikow


ledcSetup(1, 2000, 10);//inicjalizacja kanalu pwn
ledcSetup(9, 2000, 10);//inicjalizacja kanalu pwn
ledcAttachPin(PWMA,1);
ledcAttachPin(PWMB,9);
pinMode(PWMA,OUTPUT);
pinMode(PWMB,OUTPUT);//kanaly pwn
ledcWrite(1,0);//poczatkowa predkosc
ledcWrite(9,0);//poczatkowa predkosc

/*
  // polaczenie z wifi na stalym IP
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
Serial.println("STA Failed to configure");
}
*/
Serial.print("Łaczenie z  ");
Serial.println(ssid);
WiFi.begin(ssid, password);

while (WiFi.status() != WL_CONNECTED) {
delay(500);
Serial.print(",");
}

// wyswietlenie IP wybranego przez ruter
Serial.println("");
Serial.println("Wi-fi połaczone");
Serial.println("IP adres: ");
Serial.println(WiFi.localIP());

WiFi.onEvent(Wifi_rozlaczono, SYSTEM_EVENT_STA_DISCONNECTED); 
/*
   Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
*/
  // uruchomienie asynchronicznego serwera
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
server.on("/distance", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", Front.c_str());
  });
  server.on("/distance2", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", Back.c_str());
  });
  
 server.on("/move", HTTP_GET, [] (AsyncWebServerRequest *request) {
  String inputMessage;
  
  if (request->hasParam("button")) {
    inputMessage = request->getParam("button")->value();
    button = inputMessage;
   // Serial.print(button);
    move(button);
  }
  else {
    inputMessage = "No message sent";
  }
  //Serial.println(inputMessage);
  request->send(200, "text/plain", "OK");
});
//przyciski ledu
 server.on("/CAM", HTTP_GET, [] (AsyncWebServerRequest *request) {
  String inputMessage;
  
  if (request->hasParam("button")) {
    inputMessage = request->getParam("button")->value();
    //LED = inputMessage;
   // Serial.print(button);
    
  }
  else {
    inputMessage = "No message sent";
  }
  mySerial.println(inputMessage);
  Serial.println(inputMessage);
  request->send(200, "text/plain", "OK");
});

 server.on("/slider", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessageS;
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (request->hasParam("value")) {
      inputMessageS = request->getParam("value")->value();
      pwmSliderValue = inputMessageS;
      //ledcWrite(chanell, pwmSliderValue.toInt());
      //ledcWrite(, pwmSliderValue.toInt());
      //ledcWrite(9, pwmSliderValue.toInt());
      setpoint = pwmSliderValue.toInt();
    }
    else {
      inputMessageS = "No message sent";
    }
   // Serial.println(inputMessageS);
    request->send(200, "text/plain", "OK");
  });

  server.on("/voltage", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", v_measure().c_str());
  });
  pinMode(ENCA,INPUT);
  pinMode(ENC2A,INPUT);
  attachInterrupt(ENCA,check_EncoderL,RISING);
  attachInterrupt(ENC2A,check_EncoderP,RISING);


  
  //rozpoczecie dzialania serwera
  server.begin();

  
 
 // Use 1st timer of 4 (counted from zero).
  // Set 80 divider for prescaler (see ESP32 Technical Reference Manual for more
  // info).
  timer = timerBegin(0, 80, true);//12.5 ns *80 = 1us
  timerAttachInterrupt(timer, &onTimer, true);
  // Set alarm to call onTimer function every second (value in microseconds).
  // Repeat the alarm (third parameter)
  timerAlarmWrite(timer, 1000, true);//1kHz co jedna mili s
  // Start an alarm
  timerAlarmEnable(timer);
 //timerEnd(timer);
 //timer = NULL;

  timer1 = timerBegin(1, 80, true);//12.5 ns *80 = 1us
  timerAttachInterrupt(timer1, &onTimer1, true);
  // Set alarm to call onTimer function every second (value in microseconds).
  // Repeat the alarm (third parameter)
  timerAlarmWrite(timer1, 10000, true);//co 0,01 s
  // Start an alarm
  timerAlarmEnable(timer1);

   timer2 = timerBegin(2, 80, true);//12.5 ns *80 = 1us
  timerAttachInterrupt(timer2, &onTimer2, true);
  // Set alarm to call onTimer function every second (value in microseconds).
  // Repeat the alarm (third parameter)
  timerAlarmWrite(timer2, 500000, true);//co 0,01 s
  // Start an alarm
  timerAlarmEnable(timer2);


}

void loop() {
  if(left_encoder==true)
  {
  EncoderL();
  }
  
  if(abs(velocity)>90&&button!="stop")velocity = float(setpoint);
  if(abs(velocity)<5&button=="stop")velocity = 0;
  v1Prev = velocity;

  if(right_encoder==true)
  {
  EncoderP();
  }
  
  if(abs(velocity2)>90&&button!="stop")velocity2 = velocity;
  if(abs(velocity2)<5&button=="stop")velocity2 = 0;
  v1Prev = velocity;
  v2Prev = velocity2;

  if(button!="stop"&&interruptpi==true)
  {

  
  v1Filt= velocity;
  v1PrevF = velocity;
  
  
  e=float(setpoint)-v1Filt;
  deltaTC =0.001;
  calka=calka+e*deltaTC;
  u=kp*e+ki*calka;//+kd*dr;
  ep=e;
  if(u>920)u=920;
  if(u<=0)u=0;
  //u=240;
  ledcWrite(9, abs(u));

///////////////////////////////2 kolo
  v2Filt =velocity2;
  v2PrevF = velocity2;

   
   //e2=v1Filt-v2Filt;
   e2=float(setpoint)-v2Filt;
   calka2=calka2+e2*deltaTC;
   u2=kp2*e2+ki2*calka2;
  
  if(u2>920)u2=920;
  if(u2<=0)u2=0;
  
  /////ostatnie sterowanie
  
  e3=v1Filt-v2Filt;

   calka3=calka3+e3*deltaTC;
   u2=u2+kp3*e3+ki3*calka3;
   
  if(u2>920)u2=920;
  if(u2<=0)u2=0;
  
  //u2=240;
  ledcWrite(1, abs(u2));
  //////////////////////////////////////
  
  interruptpi=false;
  }

  if(interruptpomiar==true)
  {
    /*
  Serial.print(velocity);
  Serial.print(" ");
  Serial.print(velocity2);
  Serial.print(" ");
  Serial.print(setpoint);
  
  Serial.println();
  */
   
  interruptpomiar=false;
  }///do wykresow
  
   if(interruptdist==true)
  {
  Serial.println();
  Front_sensor();
  Back_sensor();
   
  interruptdist=false;
  }//do sensorow

  
}


  
  

void IRAM_ATTR check_EncoderL(){
  //a++;
  ///if(a==2)
  //{
  long currT = micros();
  deltaT = ((currT - prevT_i));
  prevT_i = currT;
  left_encoder=true;
  //a=0;
  //}
}

void  EncoderL(){
  // Read encoder B when ENCA rises
  /*
  if(button=="forward"||button=="right"){
    // If B is high, increment forward
    incrementL = 1;
  }
   if(button=="backward"||button=="left"){
    // Otherwise, increment backward
    incrementL = -1;
  }
  */
  //pos_i = pos_i + increment;
  //long currT = micros();
  //long deltaT = ((currT - prevT_i));
  velocity = ((1000000.0)/(float(deltaT)))/(960.0/60.0);//idealny sposob
  //velocity = ((increment*1000000.0)/(float(deltaT)))/(192.0/60.0);//idealny sposob
  //prevT_i = currT;

  
  left_encoder=false;
}
void IRAM_ATTR check_EncoderP(){
  //a++;
  ///if(a==2)
  //{
  long currT = micros();
  deltaTP = ((currT - prevT_iP));
  prevT_iP = currT;
  right_encoder=true;
  //a=0;
  //}
}

void  EncoderP(){
  // Read encoder B when ENCA rises
  /*
  if(button=="forward"||button=="left"){
    // If B is high, increment forward
    incrementP = 1;
  }
   if(button=="backward"||button=="right"){
    // Otherwise, increment backward
    incrementP = -1;
  }
  */
  velocity2 = ((1000000.0)/(float(deltaTP)))/(960.0/60.0);//idealny sposob
 
  
  right_encoder=false;
}
