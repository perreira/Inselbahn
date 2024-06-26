#include <Arduino.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

#define LED 2 // Internal LED pin
#define FWD 5 // Pin D1
#define BCK 4 // Pin D2

int ispeed = 100; // 0 to 255 speed for PWM output
int ipausemax = 10;
int ipausemin = 1;
int idrivetime = 30;

unsigned long previousMillis;
unsigned long currentMillis;
unsigned long interval = 200UL;

bool ledOn = false; // keep track of the led state
int mode = 0;       // 0: Pause Pier, 1: Vorwärts, 2: Pause Haltepunkt,  3: Rückwärts;

WiFiManager wm;
boolean OTA_enable;

std::unique_ptr<ESP8266WebServer> server;

// Funktion zur Steuerung des Fahrstroms über L298
void drive()
{
  if (millis() - previousMillis >= interval)
  {
    previousMillis = millis();

    switch (mode)
    {
    case 0 /* constant-expression */:
      /* Pause Pier */
      mode = 1;
      digitalWrite(LED, HIGH);
      analogWrite(FWD, 0);
      analogWrite(BCK, 0);
      interval = random(ipausemin * 1000, ipausemax * 1000); // set interval for pause (random)
      break;
    case 1 /* constant-expression */:
      /* Vorwärts */
      mode = 2;
      analogWrite(LED, 50);
      analogWrite(FWD, ispeed);
      analogWrite(BCK, 0);
      interval = idrivetime * 1000; // Set interval for next step (drive, 10s)
      break;
    case 2 /* constant-expression */:
      /* Pause Haltepunkt */
      mode = 3;
      interval = random(ipausemin * 1000, ipausemax * 1000); // set interval for pause (random)
      digitalWrite(LED, HIGH);
      analogWrite(FWD, 0);
      analogWrite(BCK, 0);
      break;
    case 3 /* constant-expression */:
      /* Rückwärts */
      mode = 0;
      analogWrite(LED, 100);
      analogWrite(FWD, 0);
      analogWrite(BCK, ispeed);
      interval = idrivetime * 1000; // Set interval for next step (drive, 10s)

      break;
    default:
      break;
    }
    // toggle
    // digitalWrite( LED, digitalRead( LED) == HIGH ? LOW : HIGH);
  }
}

// Funktionen für den Webserver
// Hauptseite
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
    <title>Inselbahn Config</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Inselbahn Config</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
    <p>Modus: %i</p>\
    <h2>Einstellungen</h2><br>\
    <form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/postplain/\">\
      Geschwindigkeit (1-255):&nbsp;<input type=\"number\" name=\"Speed\" value=\"%i\" min=\"1\" max=\"255\"><br>\
      Fahrt aktiv (25-240 s):&nbsp;<input type=\"number\" name=\"DriveTime\" value=\"%i\" min=\"25\" max=\"240\"><br>\
      Pause minimum (1-100 s):&nbsp;<input type=\"number\" name=\"PauseMin\" value=\"%i\" min=\"1\" max=\"100\"><br>\
      Pause maximum (1-100 s):&nbsp;<input type=\"number\" name=\"PauseMax\" value=\"%i\" min=\"1\" max=\"100\"><br>\
      <input type=\"submit\" value=\"Submit\"><br>\
    </form>\
  </body>\
</html>",
              hr, min % 60, sec % 60, mode, ispeed, idrivetime, ipausemin, ipausemax);
  server->send(200, "text/html", temp.c_str());
}

// Webserver: Seite nicht gefunden
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

// Webserver: Formulardaten handler
void handlePlain()
{
  if (server->method() != HTTP_POST)
  {
    server->send(405, "text/plain", "Method Not Allowed");
  }
  else
  {
    if (server->hasArg("Speed"))
    {
      ispeed = server->arg("Speed").toInt();
    }
    if (server->hasArg("DriveTime"))
    {
      idrivetime = server->arg("DriveTime").toInt();
    }
    if (server->hasArg("PauseMin"))
    {
      ipausemin = server->arg("PauseMin").toInt();
    }
    if (server->hasArg("PauseMax"))
    {
      ipausemax = server->arg("PauseMax").toInt();
    }
  }
  // Set everything to sane values if toInt failed
  if (ispeed == 0) ispeed = 100;
  if (idrivetime == 0) idrivetime = 30;
  if (ipausemax == 0) ipausemax = 10;
  if (ipausemin == 0) ipausemin = 1;
  
  // Catch pause max smaller than min and switch values
  if (ipausemax < ipausemin)
  {
    int tmp = ipausemax;
    ipausemax = ipausemin;
    ipausemin = tmp;
  }

  // Send redirect to main page
  server->sendHeader("Location", String("/"), true);
  server->send(302, "text/plain", "");
}

//
// SETUP
//
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
  ispeed = 100;

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
    ArduinoOTA.onStart([]()
                       {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type); });
    ArduinoOTA.onEnd([]()
                     { Serial.println("\nEnd"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
    ArduinoOTA.onError([](ota_error_t error)
                       {
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
  server->on("/postplain/", handlePlain);
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
  server->handleClient(); // Webserver Handler
  ArduinoOTA.handle(); // Over the air update Handler

  currentMillis = millis();
  drive(); // Fahrstromhandler
}
