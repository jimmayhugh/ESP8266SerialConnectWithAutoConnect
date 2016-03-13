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

#ifndef TeensyWiFiManager_h
#define TeensyWiFiManager_h

#include <ESP8266WiFi.h>

//#include <EEPROM.h>
//#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>


class WiFiManager
{
public:
    WiFiManager();
    
    void    begin(void);
    void    begin(char const *apName);
    
//    boolean autoConnect(void);
    boolean autoConnect(char const *apName);
    
    boolean hasConnected();
    
    String  beginConfigMode(void);
    void    startWebConfig();
    
    void    resetSettings();
    //for conveniennce
    String  urldecode(const char*);

    //sets timeout before webserver loop ends and exits even if there has been no setup. 
    //usefully for devices that failed to connect at some point and got stuck in a webserver loop
    //in seconds
    void    setTimeout(unsigned long seconds);
    void    setDebugOutput(boolean debug);

    //sets a custom ip /gateway /subnet configuration
//    void    setAPConfig(IPAddress ip, IPAddress gw, IPAddress sn);

    int     serverLoop();
    
//    IPAddress ip;
//    IPAddress gw;
//    IPAddress sn;
//    IPAddress dns = (8,8,8,8);
//    String ssid;
//    String pass;

private:
    DNSServer dnsServer;
    ESP8266WebServer server;

    const int WM_DONE = 0;
    const int WM_WAIT = 10;
    
    const String HTTP_404 = "HTTP/1.1 404 Not Found\r\n\r\n";
    const String HTTP_200 = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
    const String HTTP_HEAD = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"/><title>{v}</title>";
    const String HTTP_STYLE = "<style>div,input {margin-bottom: 5px;} body{width:200px;display:block;margin-left:auto;margin-right:auto;} button{padding:0.75rem 1rem;border:0;border-radius:0.317rem;background-color:#1fa3ec;color:#fff;line-height:1.5;cursor:pointer;}</style>";
    const String HTTP_SCRIPT = "<script>function c(l){document.getElementById('s').value=l.innerText||l.textContent;document.getElementById('p').focus();}</script>";
    const String HTTP_HEAD_END = "</head><body>";
    const String HTTP_ITEM = "<div><a href='#' onclick='c(this)'>{v}</a></div>";
    const String HTTP_FORM = "<form method='get' action='wifisave'><input id='s' name='s' length=32 value='GMJLinksys' placeholder='SSID'><input id='p' name='p' length=64 value='ckr7518t' placeholder='password'><input id='i' name='i' length=32  value='192.168.1.240' placeholder='WiFi ipaddress'><input id='g' name='g' length=32 value='192.168.1.1' placeholder='gateway'><input id='n' name='n' length=32 value='255.255.255.0' placeholder='netmask'><input id='u' name='u' length=6 value='2652' placeholder='udpport'><input id='e' name='e' length=32  value='192.168.1.241' placeholder='Ethernet ipaddress'><br/><button type='submit'>save</button></form>";
    const String HTTP_SAVED = "<div>Credentials Saved<br />Trying to connect ESP to network.<br />If it fails reconnect to AP to try again</div>";
    const String HTTP_END = "</body></html>";
    //const char HTTP_END[] PROGMEM = R"=====(
    //</body></html>
    //)=====";
    
    int _eepromStart;
    const char* _apName = "no-net";
    String _ssid    = "";
    String _pass    = "";
    String _ipAddr  = "";
    String _gwAddr  = "";
    String _snAddr  = "";
    String _udpPort = "";
    
    char* _ipPtr;
    uint8_t _x, _y;
    unsigned long timeout = 0;
    unsigned long start = 0;
    IPAddress _ip;
    IPAddress _gw;
    IPAddress _sn;
    
    String getEEPROMString(int start, int len);
    void setEEPROMString(int start, int len, String string);

    bool keepLooping = true;
    int status = WL_IDLE_STATUS;
    void connectWifi(String ssid, String pass);

    void handleRoot();
    void handleWifi(bool scan);
    void handleWifiSave();
    void handleNotFound();
    void handle204();

    boolean captivePortal();
    IPAddress strToAddr(String);
    
    // DNS server
    const byte DNS_PORT = 53;
    
    boolean isIp(String str);
    String toStringIp(IPAddress ip);

    boolean connect = false;
    boolean readytoConnect = false;
    boolean _debug = false;

    template <typename Generic>
    void DEBUG_PRINT(Generic text);
};



#endif
