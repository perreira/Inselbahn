# Inselbahn

ESP8266 Code für die Steuerung eines analogen Pendelzuges auf einer kleinen Modellbahn-Anlage, hier eine Inselbahnstrecke auf einem kleinen Ikea-Regal.

## Features

* Wenn WiFi vorhanden: verbinden mit WiFi, Webseite zur Steuerung
* Wenn kein WiFi vorhanden: WiFi Accesspoint aufmachen, Webseite zur Steuerung
* OTA Updates
* Geschwindigkeit, Pausen und Zeit in der der Fahrstrom aktiv ist können per WebUI eingestellt werden
* Der Zug pendelt in zufälligen Zeitabständen hin und her.
* Licht kann ein und ausgeschalten werden
* eventuell noch Sound (ein und ausschaltbar)

## Hardware

* L298H Motortreiber
* Wemos D1 mini (ESP8266)

### Anschlüsse

* L298H: 12V / GND an 12V Netztteil
* L298H: 5V / GND an 5V und GND von D1 mini
* Ausgänge D1, D2 an PWD Pins von L298H Ausgang 1
* Enable Ausgang 1 auf L298 mit Jumper auf dauernd an

## Changelog

* 16.06.24: Einstellungen über Webseite hinzugefügt
* 16.06.24: OTA Update hinzugefügt, IO aktiviert
* 15.06.24: Initialer Commit, Webserver und Grundstruktur
