#include <Arduino.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

WiFiManager wm;


std::unique_ptr<ESP8266WebServer> server;


void handleRoot() {
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  StreamString temp;
  temp.reserve(500);  // Preallocate a large chunk to avoid memory fragmentation
  temp.printf("\
<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>ESP8266 Demo</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello from ESP8266!</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\

  </body>\
</html>",
              hr, min % 60, sec % 60);
  server->send(200, "text/html", temp.c_str());
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server->uri();
  message += "\nMethod: ";
  message += (server->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server->args();
  message += "\n";
  for (uint8_t i = 0; i < server->args(); i++) {
    message += " " + server->argName(i) + ": " + server->arg(i) + "\n";
  }
  server->send(404, "text/plain", message);
}


void setup() {
    // Debugging on serial port
    Serial.begin(115200);

    // Start WiFi Manager
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP    
    //reset settings - wipe credentials for testing
    //wm.resetSettings();
    // WifiManager Debugging Messages
    wm.setDebugOutput(true);
    // Run WifiManager for 60 seconds (connect, connection setup), if not connected by then, start access point
    wm.setTimeout(60);
    // Set Hostname for DNS to Inselbahn, so it will show up with this name
    wm.setHostname("Inselbahn");
    //automatically connect using saved credentials if they exist
    //If connection fails it starts an access point with the specified name and runs config portal
    if(wm.autoConnect("Inselbahn","12345678")){
        Serial.println("connected...yeey :)");
    }
    else { // config portal has run into timeout, start WiFi AP
      Serial.println("offline mode");     
        WiFi.mode(WIFI_AP_STA);
        IPAddress apIP(192, 168, 4, 1);
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
        WiFi.softAP("Inselbahn_offline","12345678");
    }

  // We are either connected or running AP, start webserver now

  server.reset(new ESP8266WebServer(WiFi.localIP(), 80));
  // Define web adresses and hooks
  server->on("/", handleRoot);
  server->on("/inline", []() {
    server->send(200, "text/plain", "this works as well");
  });
  server->onNotFound(handleNotFound);

  // Start Webserver
  server->begin();
  Serial.println("HTTP server started");
  Serial.println(WiFi.localIP());
}

// ***** MAIN LOOP ******* 
void loop() {
  // put your main code here, to run repeatedly:
  server->handleClient();
}

