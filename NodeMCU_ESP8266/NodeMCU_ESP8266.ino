#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>
#include <MQUnifiedsensor.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebSrv.h>
#include <ArduinoJson.h>

/************************Hardware Related Macros************************************/
#define         Board                   ("ESP8266")
#define         LED_RED_PIN             (D0)
#define         LED_BLUE_PIN            (D1)
#define         I2C_SCL                 (D1)
#define         I2C_SDA                 (D2)
#define         DHT_PIN                 (D4)
#define         MQ2_PIN                 (A0)
#define         MQ7_PIN                 (A0)
/***********************Software Related Macros************************************/
#define         DHT_TYPE                (DHT11)
#define         MQ2_TYPE                ("MQ-2") 
#define         MQ7_TYPE                ("MQ-7") 
#define         Voltage_Resolution      (3.3)
#define         ADC_Bit_Resolution      (10) 
#define         RatioMQ2CleanAir        (9.83) //RS / R0 = 9.83 ppm 
#define         RatioMQ7CleanAir        (27.5) 
#define         MQ2_LPG_A               (574.25)
#define         MQ2_LPG_B               (-2.222)
#define         MQ2_H2_A                (987.99)
#define         MQ2_H2_B                (-2.162)
#define         MQ2_CO_A                (36974)
#define         MQ2_CO_B                (-3.109)
#define         MQ2_Alcohol_A           (3616.1)
#define         MQ2_Alcohol_B           (-2.675)
#define         MQ2_Propane_A           (658.71)
#define         MQ2_Propane_B           (-2.168)
#define         MQ7_CO_A                (99.042) // CO concentration
#define         MQ7_CO_B                (-1.518)
#define         MQ7_H2_A                (69.014)
#define         MQ7_H2_B                (-1.374)
#define         MQ7_LPG_A               (700000000)
#define         MQ7_LPG_B               (-7.703)
#define         MQ7_CH4_A               (60000000000000)
#define         MQ7_CH4_B               (-10.54)
#define         MQ7_Alcohol_A           (40000000000000000)
#define         MQ7_Alcohol_B           (-12.35)
/*****************************Globals***********************************************/

DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_BMP280 bme;
//MQUnifiedsensor MQ2(Board, Voltage_Resolution, ADC_Bit_Resolution, MQ2_PIN, MQ2_TYPE);
MQUnifiedsensor MQ7(Board, Voltage_Resolution, ADC_Bit_Resolution, MQ7_PIN, MQ7_TYPE);

float temperatureC = 0;
float pressure = 0;
int humidity = 0;
float ppmCO = 0;

AsyncWebServer server(80);

const char* WIFI_AP_SSID = "NodeMCU";
const IPAddress WIFI_AP_IP = IPAddress(10,0,0,10);
const IPAddress WIFI_AP_SUBNET = IPAddress(255,255,255,0);
const IPAddress WIFI_AP_GATEWAY = IPAddress(10,0,0,1);

const char* input_ssid = "input_ssid";
const char* input_pass = "input_pass";
const char* input_endpoint = "input_endpoint";
const char* input_key = "input_key";

String ssid = "";
String password = "";
String endpoint = "";
String key = "";

boolean restart = false;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>HTML Form to Input Data</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Times New Roman; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem; color: #FF0000;}
  </style>
  </head><body>
  <h2>Connection to internet data</h2> 
  <form action="/" method="POST">
    SSID: <input type="text" id="input_ssid" name="input_ssid">
    Password: <input type="text" id="input_pass" name="input_pass">
    Endpoint: <input type="text" id="input_endpoint" name="input_endpoint">
    Device key: <input type="text" id="input_key" name="input_key">
    <input type="submit" value="Submit">
  </form>
</body></html>)rawliteral";

void setupMQ(MQUnifiedsensor* sensor, float A, float B, float cleanAirRatio)
{
  sensor->setRegressionMethod(1);
  sensor->setA(A); 
  sensor->setB(B);
  sensor->init(); 

  Serial.print("Calibrating sensor please wait.");
  float calcR0 = 0;
  for(int i = 1; i<=10; i ++)
  {
    sensor->update(); // Update data, the arduino will read the voltage from the analog pin
    calcR0 += sensor->calibrate(cleanAirRatio);
    Serial.print(".");
  }
  sensor->setR0(calcR0/10);
  Serial.println("  done!.");
  
  if(isinf(calcR0)) {Serial.println("Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply"); while(1);}
  if(calcR0 == 0){Serial.println("Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply"); while(1);}
}

void setupBME()
{
  if (!bme.begin(0x76)) 
  { 
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }
}

bool initWiFi()
{
  if(ssid==""){
    Serial.println("Undefined SSID");
    return false;
  }

  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);

  delay(20000);
  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect.");
    return false;
  }

  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  return true;
}

void handleSensorsPage(AsyncWebServerRequest *request)
{
  char msg[1500];

  snprintf(msg, 1500, "\
    <!DOCTYPE HTML><html><head>\
    <title>HTML Form to Input Data</title>\
    <meta name='viewport' content='width=device-width, initial-scale=1'>\
    <style>\
      html {font-family: Times New Roman; display: inline-block; text-align: center;}\
      h2 {font-size: 3.0rem; color: #FF0000;}\
    </style>\
    </head><body>\
    <h2>Sensors data</h2>\
    <p><span>Temperatur: </span><span>%.2f</span><span> C</span></p>\
    <p><span>Humidity: </span><span>%d</span><span> %</span></p>\
    <p><span>Pressure: </span><span>%.2f</span><span> hPA</span></p>\
    <p><span>CO content: </span><span>%.2f</span><span> PPM</span></p>\
    </body></html>",
    temperatureC, humidity, pressure, ppmCO);
  
  request->send(200, "text/html", msg);
}

void setupWiFi()
{
  if(initWiFi())
  {
    //Used after logging ESP to network
  } 
  else
  {
    Serial.println("Setting AP (Access Point)");

    WiFi.softAPConfig(WIFI_AP_IP, WIFI_AP_GATEWAY, WIFI_AP_SUBNET);
    WiFi.softAP(WIFI_AP_SSID, NULL);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/html", index_html);});

    server.on("/sensors", HTTP_GET, [](AsyncWebServerRequest *request){
      handleSensorsPage(request);});

    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == input_ssid) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
          }
          // HTTP POST pass value
          if (p->name() == input_pass) {
            password = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(password);
          }
          // HTTP POST endpoint value
          if (p->name() == input_endpoint) {
            endpoint = p->value().c_str();
            Serial.print("Endpoint set to: ");
            Serial.println(endpoint);
          }
          // HTTP POST key value
          if (p->name() == input_key) {
            key = p->value().c_str();
            Serial.print("Key set to: ");
            Serial.println(key);
          }
        }
      }

      restart = true;
      request->send(200, "text/plain", "Done. ESP will restart, connect to your router");
    });

    server.begin();
  }
}

void setup() 
{
  Serial.begin(9600);

  // pinMode(LED_BLUE_PIN, OUTPUT);
  // digitalWrite(LED_BLUE_PIN, LOW);
  
  dht.begin();
  
  setupBME();

  pinMode(MQ7_PIN, INPUT);
  setupMQ(&MQ7, MQ7_CO_A, MQ7_CO_B, RatioMQ7CleanAir);

  setupWiFi();
}

float readMQData(MQUnifiedsensor* sensor, float A, float B)
{
  sensor->setA(A);
  sensor->setB(B);
  sensor->update();
  return sensor->readSensor();
}

String serializeData()
{
  DynamicJsonDocument data(1024);

  humidity = dht.readHumidity();
  data["humidity"] = humidity;

  temperatureC = dht.readTemperature();
  data["temperatureC"] = temperatureC;

  pressure = bme.readPressure();
  data["pressure"] = pressure;

  ppmCO = readMQData(&MQ7, MQ7_CO_A, MQ7_CO_B);
  data["ppmCO"] = ppmCO;
  data["ppmAlcohol"] = readMQData(&MQ7, MQ7_Alcohol_A, MQ7_Alcohol_B);
  data["ppmCH4"] = readMQData(&MQ7, MQ7_CH4_A, MQ7_CH4_B);
  data["ppmH2"] = readMQData(&MQ7, MQ7_H2_A, MQ7_H2_B);
  data["ppmLPG"] = readMQData(&MQ7, MQ7_LPG_A, MQ7_LPG_B);

  String output = "";
  serializeJson(data, output);

  return output;
}

void sendData()
{
  String data = serializeData();
  Serial.println(data);

  if(WiFi.status()== WL_CONNECTED)
  {
    WiFiClient client;
    HTTPClient http;

    http.begin(client, endpoint);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-API-Key", key);
    int httpResponseCode = http.POST(data);
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
        
    // Free resources
    http.end();
  }
}

void loop()
{
  sendData();

  if(restart)
  {
    initWiFi();
    restart = false;
  }
  
  delay(2000);
}