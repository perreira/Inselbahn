#include <Arduino.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

#define LED 2 // Internal LED pin
#define FWD 5 // Pin D1
#define BCK 4 // Pin D2

#define DRIVETIME 30000 // time the car takes to drive the full line
#define PAUSEMIN 1000 // min pause time at station
#define PAUSEMAX 10000  // max pause time at station
#define SPEED 100 // 0 to 255 Speed for PWM output

unsigned long previousMillis;
unsigned long currentMillis;
unsigned long interval = 200UL;

bool ledOn = false; // keep track of the led state
int mode = 0; // 0: Pause Pier, 1: Vorwärts, 2: Pause Haltepunkt,  3: Rückwärts;

WiFiManager wm;
boolean OTA_enable;

std::unique_ptr<ESP8266WebServer> server;

void drive()
{
   if( millis() - previousMillis >= interval) 
  {
    previousMillis = millis();
    

    switch (mode)
    {
    case 0/* constant-expression */:
      /* Pause Pier */
      mode = 1;
      digitalWrite(LED, HIGH);
      analogWrite(FWD, 0);
      analogWrite(BCK, 0);
      interval = random( PAUSEMIN, PAUSEMAX);   // set interval for pause (random)
      break;
    case 1/* constant-expression */:
      /* Vorwärts */
      mode = 2;
      analogWrite(LED, 50);
      analogWrite(FWD, SPEED);
      analogWrite(BCK, 0);
      interval = DRIVETIME; // Set interval for next step (drive, 10s)
      break;
    case 2/* constant-expression */:
      /* Pause Haltepunkt */
      mode = 3;
      interval = random( PAUSEMIN, PAUSEMAX);   // set interval for pause (random)
      digitalWrite(LED, HIGH);
      analogWrite(FWD, 0);
      analogWrite(BCK, 0);
      break;
    case 3/* constant-expression */:
      /* Rückwärts */
      mode = 0;
      analogWrite(LED, 100);
      analogWrite(FWD, 0);
      analogWrite(BCK, SPEED);
      interval = DRIVETIME; // Set interval for next step (drive, 10s)

      break;    
    default:
      break;
    }
    // toggle
    //digitalWrite( LED, digitalRead( LED) == HIGH ? LOW : HIGH);
  }
}

void handleRoot()
{
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  StreamString temp;
  temp.reserve(500); // Preallocate a large chunk to avoid memory fragmentation
  temp.printf("\
<html>\
  <head>\
    <meta http-equiv='refresh' content='1'/>\
    <title>ESP8266 Demo</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello from ESP8266!</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
    <p>Mode %i</p>\
  </body>\
</html>",
              hr, min % 60, sec % 60, mode);
  server->send(200, "text/html", temp.c_str());
}

void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server->uri();
  message += "\nMethod: ";
  message += (server->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server->args();
  message += "\n";
  for (uint8_t i = 0; i < server->args(); i++)
  {
    message += " " + server->argName(i) + ": " + server->arg(i) + "\n";
  }
  server->send(404, "text/plain", message);
}

void setup()
{
  // Debugging on serial port
  Serial.begin(115200);

  // Define PINS
  pinMode(LED, OUTPUT);
  pinMode(FWD, OUTPUT);
  pinMode(BCK, OUTPUT);

  digitalWrite(LED, LOW); // turn led off
  ledOn = false;

  // Start WiFi Manager
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  // reset settings - wipe credentials for testing
  // wm.resetSettings();
  //  WifiManager Debugging Messages
  wm.setDebugOutput(true);
  // Run WifiManager for 60 seconds (connect, connection setup), if not connected by then, start access point
  wm.setTimeout(60);
  // Set Hostname for DNS to Inselbahn, so it will show up with this name
  wm.setHostname("Inselbahn");
  // automatically connect using saved credentials if they exist
  // If connection fails it starts an access point with the specified name and runs config portal
  if (wm.autoConnect("Inselbahn", "12345678"))
  {
    Serial.println("connected...yeey :)");
    OTA_enable = true;
    ArduinoOTA.onStart([](){
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type); });
    ArduinoOTA.onEnd([](){ Serial.println("\nEnd"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
    ArduinoOTA.onError([](ota_error_t error){
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    } });
    ArduinoOTA.begin();
  }
  else
  { // config portal has run into timeout, start WiFi AP
    Serial.println("offline mode");
    WiFi.mode(WIFI_AP_STA);
    IPAddress apIP(192, 168, 4, 1);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP("Inselbahn_offline", "12345678");
    OTA_enable = false;
  }

  // We are either connected or running AP, start webserver now

  server.reset(new ESP8266WebServer(WiFi.localIP(), 80));
  // Define web adresses and hooks
  server->on("/", handleRoot);
  server->on("/inline", [](){ server->send(200, "text/plain", "this works as well"); });
  server->onNotFound(handleNotFound);

  // Start Webserver
  server->begin();
  Serial.println("HTTP server started");
  Serial.println(WiFi.localIP());
}

// ***** MAIN LOOP *******
void loop()
{
  // put your main code here, to run repeatedly:
  server->handleClient();
  ArduinoOTA.handle();

  currentMillis = millis();
  drive();


}
