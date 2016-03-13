/*
TeensyWiFiManager

This code uses the ESP8266 board in order to communicate with a Teensy
device via the serial interface.

The Teensy device can request that the ESP8266 enter AP mode to collect
configuration information, which the Teensydevice then uses to setup the ESP8266. 

Version 0.0.1
Last Modified 03/12/2016
By Jim Mayhugh https://github.com/jimmayhugh


Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

This software uses multiple libraries that are subject to additional
licenses as defined by the author of that software. It is the user's
and developer's responsibility to determine and adhere to any additional
requirements that may arise.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

http://www.esp8266.com/viewtopic.php?f=29&t=2520
https://github.com/chriscook8/esp-arduino-apboot
https://github.com/esp8266/Arduino/tree/esp8266/hardware/esp8266com/esp8266/libraries/DNSServer/examples/CaptivePortalAdvanced
*/

#include "TeensyWiFiManager.h"

WiFiManager::WiFiManager() : server(80) {}

/*
void WiFiManager::begin() {
  begin("NoNetESP");
}
*/

void WiFiManager::begin(char const *apName) {
  
  DEBUG_PRINT("");
  _apName = apName;
  start = millis();

  DEBUG_PRINT("Configuring access point... ");
  DEBUG_PRINT(_apName);

  WiFi.softAP(_apName);//TODO: add password option
  delay(500); // Without delay I've seen the IP address blank
  DEBUG_PRINT("AP IP address: ");
  DEBUG_PRINT(WiFi.softAPIP());

  /* Setup the DNS server redirecting all the domains to the apIP */  
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  /* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
  server.on("/", std::bind(&WiFiManager::handleRoot, this));
  server.on("/wifi", std::bind(&WiFiManager::handleWifi, this, true));
  server.on("/0wifi", std::bind(&WiFiManager::handleWifi, this, false));
  server.on("/wifisave", std::bind(&WiFiManager::handleWifiSave, this));
  server.on("/generate_204", std::bind(&WiFiManager::handle204, this));  //Android/Chrome OS captive portal check.
  server.on("/fwlink", std::bind(&WiFiManager::handleRoot, this));  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
  server.onNotFound (std::bind(&WiFiManager::handleNotFound, this));
  server.begin(); // Web server start
  DEBUG_PRINT("HTTP server started");
}

/*
boolean WiFiManager::autoConnect() {
  String ssid = "ESP" + String(ESP.getChipId());
  return autoConnect(ssid.c_str());
}
*/

boolean WiFiManager::autoConnect(char const *apName) {
  DEBUG_PRINT("");
  DEBUG_PRINT("AutoConnect");

  //setup AP
  WiFi.mode(WIFI_AP);

  connect = false;
  begin(apName);

  bool looping = true;
  //while(1) {
  while(timeout == 0 || millis() < start + timeout)
  {
    //DNS
    dnsServer.processNextRequest();
    //HTTP
    server.handleClient();

    if(connect)
    {
      Serial.print(server.arg("s").c_str());
      Serial.print(';');
      Serial.print(server.arg("p").c_str());
      Serial.print(';');
      Serial.print(server.arg("i").c_str());
      Serial.print(';');
      Serial.print(server.arg("g").c_str());
      Serial.print(';');
      Serial.print(server.arg("n").c_str());
      Serial.print(';');
      Serial.print(server.arg("u").c_str());
      Serial.print(';');
      Serial.println(server.arg("e").c_str());
      break;
    }
    yield();    
  }

  return  WiFi.status() == WL_CONNECTED;
}

void WiFiManager::resetSettings() {
  //need to call it only after lib has been started with autoConnect or begin
  //setEEPROMString(0, 32, "-");
  //setEEPROMString(32, 64, "-");

  DEBUG_PRINT("settings invalidated");
  WiFi.disconnect();
  delay(200);
}

void WiFiManager::setTimeout(unsigned long seconds) {
  timeout = seconds * 1000;
}

void WiFiManager::setDebugOutput(boolean debug) {
  _debug = debug;
}


/** Handle root or redirect to captive portal */
void WiFiManager::handleRoot() {
  DEBUG_PRINT("Handle root");
  if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
    return;
  }

  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.

  String head = HTTP_HEAD;
  head.replace("{v}", "Options");
  server.sendContent(head);
  server.sendContent(HTTP_SCRIPT);
  server.sendContent(HTTP_STYLE);
  server.sendContent(HTTP_HEAD_END);
  
  server.sendContent(
    "<form action=\"/wifi\" method=\"get\"><button>Configure WiFi</button></form><br/>"
  );
  server.sendContent(
    "<form action=\"/0wifi\" method=\"get\"><button>Configure WiFi (No Scan)</button></form>"
  );
  
  server.sendContent(HTTP_END);

  server.client().stop(); // Stop is needed because we sent no content length
}

/** Wifi config page handler */
void WiFiManager::handleWifi(bool scan) {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.


  String head = HTTP_HEAD;
  head.replace("{v}", "Config ESP");
  server.sendContent(head);
  server.sendContent(HTTP_SCRIPT);
  server.sendContent(HTTP_STYLE);
  server.sendContent(HTTP_HEAD_END);

  if (scan) {
    int n = WiFi.scanNetworks();
    DEBUG_PRINT("Scan done");
    if (n == 0) {
      DEBUG_PRINT("No networks found");
      server.sendContent("<div>No networks found. Refresh to scan again.</div>");
    }
    else {
      for (int i = 0; i < n; ++i)
      {
        DEBUG_PRINT(WiFi.SSID(i));
        DEBUG_PRINT(WiFi.RSSI(i));
        String item = HTTP_ITEM;
        item.replace("{v}", WiFi.SSID(i));
        server.sendContent(item);
        yield();
      }
    }
  }
  
  server.sendContent(HTTP_FORM);
  server.sendContent(HTTP_END);
  server.client().stop();
  
  DEBUG_PRINT("Sent config page");  
}

/** Handle the WLAN save form and redirect to WLAN config page again */
void WiFiManager::handleWifiSave() {
  DEBUG_PRINT("WiFi save");

  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.

  String head = HTTP_HEAD;
  head.replace("{v}", "Credentials Saved");
  server.sendContent(head);
  server.sendContent(HTTP_SCRIPT);
  server.sendContent(HTTP_STYLE);
  server.sendContent(HTTP_HEAD_END);
  
  server.sendContent(HTTP_SAVED);

  server.sendContent(HTTP_END);
  server.client().stop();
  
  DEBUG_PRINT("Sent wifi save page");  
  
  //saveCredentials();
  connect = true; //signal ready to connect/reset
}

void WiFiManager::handle204() {
  DEBUG_PRINT("204 No Response");  
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send ( 204, "text/plain", "");
}


void WiFiManager::handleNotFound() {
  if (captivePortal()) { // If captive portal redirect instead of displaying the error page.
    return;
  }
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send ( 404, "text/plain", message );
}


/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean WiFiManager::captivePortal() {
  if (!isIp(server.hostHeader()) ) {
    DEBUG_PRINT("Request redirected to captive portal");
    server.sendHeader("Location", String("http://") + toStringIp(server.client().localIP()), true);
    server.send ( 302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
    server.client().stop(); // Stop is needed because we sent no content length
    return true;
  }
  return false;
}





/** Is this an IP? */
boolean WiFiManager::isIp(String str) {
  for (int i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

/** IP to String? */
String WiFiManager::toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}

IPAddress WiFiManager::strToAddr(String addrStr)
{
  IPAddress addr;
  char * p;

  addr[0] = atoi(&addrStr.c_str()[0]);
  p = strchr(addrStr.c_str(), '.');
  addr[1] = atoi(p+1);
  p = strchr(p+1, '.');
  addr[2] = atoi(p+1);
  p = strchr(p+1, '.');
  addr[3] = atoi(p+1);

  return addr;
}


template <typename Generic>
void WiFiManager::DEBUG_PRINT(Generic text) {
  if(_debug) {
    Serial.print("*JM: ");
    Serial.println(text);    
  }
}


