#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebSrv.h>
#include <PID_v1.h>

#define         DHT_TYPE                (DHT11)
#define         DHT_PIN                 (D4)
#define         FAN_PIN                 (D5)
#define         SIGNAL_PIN              (D6)

#define         MIN_FAN_SPEED_PERCENT   (20) 
#define         MIN_TEMP                (20) 
#define         MAX_TEMP                (35) 

DHT dht(DHT_PIN, DHT_TYPE);

AsyncWebServer server(80);

const char* WIFI_AP_SSID = "NodeMCU";
const IPAddress WIFI_AP_IP = IPAddress(10,0,0,10);
const IPAddress WIFI_AP_SUBNET = IPAddress(255,255,255,0);
const IPAddress WIFI_AP_GATEWAY = IPAddress(10,0,0,1);

double temperatureC = 0;
int fanSpeedRpm = 0;
int fanSpeedPercent = 0;

bool automatical = true;
bool disabledFan = false;
int setFanSpeed = 0;

double KP=0.4, KI=0.4, KD=0.05;
double DUTY_CYCLE = 0;  
int MIN_DUTY_CYCLE = 15;
double TEMP_SETPOINT = 23;
PID FAN_PID(&temperatureC, &DUTY_CYCLE, &TEMP_SETPOINT, KP, KI, KD, REVERSE);

void handleRoot(AsyncWebServerRequest *request)
{
  char msg[1500];

  String autoMode = automatical ? "true" : "false";
  String disabled = disabledFan ? "true" : "false";

  snprintf(msg, 1500, "<!DOCTYPE HTML><html><head>\
    <title>HTML Form to Input Data</title>\
    <meta name='viewport' content='width=device-width, initial-scale=1'>\
    <style>\
      html {font-family: Times New Roman; display: inline-block; text-align: center;}\
      h2 {font-size: 3.0rem; color: #FF0000;}\
    </style>\
    </head><body>\
    <h2>Fan control</h2>\
    <p><span>Temperature: </span><span>%.2f</span><span> C</span></p>\
    <p><span>Current fan rpm: </span><span>%d</span><span> RPM</span></p>\
    <p><span>Current fan percent </span><span>%d</span><span> \%</span></p>\
	<form action='/' method='POST'>\
    Autmatical: <input type='checkbox' id='auto_mode' name='auto_mode' onClick='change()'></br>\
	Disable fan: <input type='checkbox' id='disabled_mode' name='disabled_mode' onClick='change()'></br>\
    Fan percent: <input type='range' id='fan_speed' name='fan_speed' min='20' max='100' value='50'><span id='fan'></span></br>\
    <input type='submit' value='Submit'>\
	</form>\
	<script type='text/javascript'>\
	function change() {\
	var checkboxAuto = document.getElementById('auto_mode')\
	var checkboxDisable = document.getElementById('disabled_mode')\
	var rangeFan = document.getElementById('fan_speed')\
	checkboxDisable.disabled = checkboxAuto.checked;\
	if (checkboxDisable.checked && checkboxAuto.checked) checkboxDisable.checked = false;\
	rangeFan.disabled = checkboxDisable.checked || checkboxAuto.checked;}\
	\
	var currentAuto = %s;\
	var currentDisable = %s;\
	document.getElementById('auto_mode').checked = currentAuto;\
	document.getElementById('disabled_mode').checked = currentDisable;\
	var slider = document.getElementById('fan_speed');\
	var output = document.getElementById('fan');\
	output.innerHTML = slider.value;\
	slider.oninput = function() {output.innerHTML = this.value;}\
	change();\
	</script></body></html>",
    temperatureC, fanSpeedRpm, fanSpeedPercent, autoMode, disabled);
  
  request->send(200, "text/html", msg);
}

void handlePostRequest(AsyncWebServerRequest *request)
{
  int params = request->params();
  for(int i=0;i<params;i++){
    AsyncWebParameter* p = request->getParam(i);
    if(p->isPost()){
      // HTTP POST auto value
      if (p->name() == "auto_mode") {
        String val = p->value().c_str();
        automatical = val == "on" ? true : false ;
        disabledFan = false;
        Serial.print("Auto mode set to: ");
        Serial.println(automatical);
      }
      // HTTP POST disable value
      if (p->name() == "disabled_mode") {
        String val = p->value().c_str();
        disabledFan = "on" ? true : false;
        automatical = false;
        Serial.print("Fan disabled set to: ");
        Serial.println(disabledFan);
      }
      // HTTP POST rpm value
      if (p->name() == "fan_speed") {
        String val = p->value().c_str();
        setFanSpeed = val.toInt();
        automatical = false;
        disabledFan = false;
        Serial.print("Fan speed set to: ");
        Serial.println(setFanSpeed);
      }
    }
  }

  handleRoot(request);
}

int getFanSpeedRpm() 
{
  int highTime = pulseIn(SIGNAL_PIN, HIGH);
  int lowTime = pulseIn(SIGNAL_PIN, LOW);
  int period = highTime + lowTime;
  
  if (period == 0) {
    return 0;
  }
  float freq = 1000000.0 / (float)period;
  return (freq * 60.0) / 2.0;
}

void setFanSpeedPercent(int p) {
  int value = (p / 100.0) * 255;
  analogWrite(FAN_PIN, value);
}

void setupWiFi()
{
  WiFi.softAPConfig(WIFI_AP_IP, WIFI_AP_GATEWAY, WIFI_AP_SUBNET);
  WiFi.softAP(WIFI_AP_SSID, NULL);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      handleRoot(request);});
  
  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request){
      handlePostRequest(request);});

  server.begin();
}


void setup() 
{
  Serial.begin(9600);

  dht.begin();

  pinMode(FAN_PIN, OUTPUT);
  pinMode(SIGNAL_PIN, INPUT);

  FAN_PID.SetMode(AUTOMATIC);
  FAN_PID.SetOutputLimits(0,255); 

  setupWiFi();
}

void controlAutomatically()
{
  if (temperatureC < MIN_TEMP)
  {
    fanSpeedPercent = 0;
  }
  else if (temperatureC > MIN_TEMP)
  {
    fanSpeedPercent = 100;
  }
  else
  {
    //fanSpeedPercent = (100 - MIN_FAN_SPEED_PERCENT) * (temperatureC - MIN_TEMP) / (MAX_TEMP - MIN_TEMP) + MIN_FAN_SPEED_PERCENT;
    FAN_PID.Compute();  
    fanSpeedPercent = (DUTY_CYCLE / 255) * 100;
  }

  setFanSpeedPercent(fanSpeedPercent);
}

void controlManual()
{
  if (disabledFan)
  {
    fanSpeedPercent = 0;
  }
  else 
  {
    fanSpeedPercent = setFanSpeed;
  }
  
  setFanSpeedPercent(fanSpeedPercent);
}

void loop() 
{
  temperatureC = dht.readTemperature();
  
  if (automatical)
  {
    controlAutomatically();
  }
  else 
  {
    controlManual();
  }

  fanSpeedRpm = getFanSpeedRpm();

  delay(10000);
}

