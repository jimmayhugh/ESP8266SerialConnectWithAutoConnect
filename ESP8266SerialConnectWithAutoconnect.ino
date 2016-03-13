/*
ESP8266SerialConnectWithAutoConnect.ino

This code uses the ESP8266 board in order to communicate with a Teensy
device via the serial interface.

The Teensy device can request that the ESP8266 enter AP mode to collect
configuration information, which the Teensydevice then uses to setup the ESP8266. 

Version 0.0.1
Last Modified 03/12/2016
By Jim Mayhugh


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

*/

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <TeensyWiFiManager.h>
#include <WiFiUdp.h>
#include <Ticker.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

const uint16_t udpDebug       = 0x0001;
const uint16_t mdnsDebug      = 0x0002;
const uint16_t ipDebug        = 0x0004;
const uint16_t dBugDebug      = 0x0008;

uint16_t setDebug = 0x0000;

const uint32_t dBugDelay = 100;
const uint16_t serStrCnt = 512, packetBufCnt = 512;
const uint8_t  WiFiStrCnt = 32;  // max string length
const uint8_t  ipStrCnt = 20;
const uint8_t  udpPortCnt = 4;
const uint8_t  macCnt = 6;

uint8_t macAddress[macCnt] = {0,0,0,0,0,0};
char serBuf[serStrCnt]  = "     "; // Serial Buffer
char ssid[WiFiStrCnt]   = "";      // your network SSID (name)
char passwd[WiFiStrCnt] = "";      // your network password
char udpBuffer[packetBufCnt];      // buffer for udp
char teensyBuffer[packetBufCnt];   // buffer tennsy

char *versionID = "Version 0.0.1 ESP8266SerialConnectWithAutoConnect";

uint16_t udpPort = 0xFFFF;         // local port to listen for UDP packets

uint16_t z = 0;
uint8_t udpAddr[5];

int16_t noBytes, packetCnt;

IPAddress staticIP(255,255,255,255);
IPAddress staticGateway(255,255,255,255);
IPAddress staticSubnet(255, 255, 255, 255);
IPAddress staticDNS(8, 8, 8, 8);
IPAddress wifiIP(255,255,255,255);

// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;

Ticker  dBug;

WiFiManager wifiManager;

bool beginLoop = false;

// Function prototypes
void clearBuffers(void);
void startDebugUpdate(void);
void dBugStatus(void);
void strToIP(uint8_t *, char *);
IPAddress strToAddr(char *);
void setSSID(void);
void setPW(void);
void setIP(void);
void setGW(void);
void setSN(void);
void setUDP(void);
void connectWiFi(void);
void processUDP(void);

void flushBuffer(void)
{
  while(Serial.available())
    Serial.read(); // flush the buffer
  z = 0;
}

void clearBuffers(void)
{
  for(uint16_t c = 0; c < packetBufCnt; c++)
  {
    udpBuffer[c] = 0xFF;
    teensyBuffer[c] = 0xFF;
  }
}

void startDebugUpdate(void)
{
  if(setDebug & dBugDebug)
    Serial.println("Enter startDebugUpdate");
  dBug.attach_ms(dBugDelay, dBugStatus);
  if(setDebug & dBugDebug)
    Serial.println("Exit startDebugUpdate");
}


void dBugStatus(void)
{
  dBug.detach();
  ESP.wdtDisable(); // disable the watchdog Timer
  dBug.attach_ms(dBugDelay, dBugStatus);
}

void strToIP(uint8_t *ipArray, char *str)
{
  ipArray[0] = (uint8_t) atoi(str);
  ipArray[1] = (uint8_t) atoi((char *) &str[4]);
  ipArray[2] = (uint8_t) atoi((char *) &str[8]);
  ipArray[3] = (uint8_t) atoi((char *) &str[12]);
}

IPAddress strToAddr(char *addrStr)
{
  IPAddress addr;
  char * p;

  addr[0] = atoi(addrStr);
  if(setDebug & ipDebug)
  {
    Serial.print("addr[0] = ");
    Serial.println(addr[0]);
  }
  p = strchr(addrStr, '.');
  addr[1] = atoi(p+1);
  if(setDebug & ipDebug)
  {
    Serial.print("addr[1] = ");
    Serial.println(addr[1]);
  }
  p = strchr(p+1, '.');
  addr[2] = atoi(p+1);
  if(setDebug & ipDebug)
  {
    Serial.print("addr[2] = ");
    Serial.println(addr[2]);
  }
  p = strchr(p+1, '.');
  addr[3] = atoi(p+1);
  if(setDebug & ipDebug)
  {
    Serial.print("addr[3] = ");
    Serial.println(addr[3]);
  }
  return addr;
}

void setSSID(void)
{
  strcpy(ssid, (char *) &serBuf[1]);
  Serial.println(ssid);
  flushBuffer();
}

void setPW(void)
{
  strcpy(passwd, (char *) &serBuf[1]);
  Serial.println(passwd);
  flushBuffer();
}

void setIP(void)
{
  staticIP = strToAddr((char *) &serBuf[1]);
  Serial.println(staticIP);
  flushBuffer();
}

void setGW(void)
{
  staticGateway = strToAddr((char *) &serBuf[1]);
  Serial.println(staticGateway);
  flushBuffer();
}

void setSN(void)
{
  staticSubnet = strToAddr((char *) &serBuf[1]);
  Serial.println(staticSubnet);
  flushBuffer();
}

void setUDP(void)
{
  udpPort = atoi( (char *) &serBuf[1]);
  Serial.println(udpPort);
  z = 0;
}

void connectWiFi(void)
{
  
  WiFi.begin(ssid, passwd);

  // Wait for connect to AP
  int tries=0;

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    yield();
    tries++;
    if (tries > 30)
    {
      tries = 0;
      Serial.print(".");
    }
  }
  Serial.println();

  WiFi.config(staticIP, staticGateway, staticSubnet, staticDNS);

  wifiIP = WiFi.localIP();
  
  if(Udp.begin(udpPort))
  {
    Serial.print(WiFi.localIP());
    Serial.print(":");
    Serial.println(udpPort);
  }else{
    Serial.print(WiFi.localIP());
    Serial.print(":");
    Serial.println("0xFFF");
  }
  flushBuffer();
}

void autoConnectWiFi(void)
{
  wifiManager.autoConnect("AutoConnectAP");
  flushBuffer();
}

void processUDP(void)
{
  if(setDebug & udpDebug)
  {
    Serial.print(millis() / 1000);
    Serial.print(":Packet of ");
    Serial.print(noBytes);
    Serial.print(" received from ");
    Serial.print(Udp.remoteIP());
    Serial.print(":");
    Serial.println(Udp.remotePort(), HEX);
  }
  
  // We've received a packet, read the data from it
  Udp.read(udpBuffer,noBytes); // read the packet into the buffer

  Serial.print(udpBuffer); // send the UDP Packet to the Teensy3.x via serial

  // display the packet contents in HEX
  if(setDebug & udpDebug)
  {
    for (int i = 1;i <= noBytes; i++)
    {
      Serial.print(udpBuffer[i-1]);
      if (i % 32 == 0)
      {
        Serial.println();
      }
    } // end for
    Serial.println();
  }
  z = 0;
  while(1)  // get the response from the Teensy, and send it back to the requestor
  {
    while(Serial.available())
    {
      teensyBuffer[z] = Serial.read();
      if(setDebug & udpDebug)
      {
        Serial.print("0x");
        if(teensyBuffer[z] < 0x10) Serial.print("0");
        Serial.print(teensyBuffer[z], HEX);
        Serial.print(", ");
      }
      if( (teensyBuffer[z] == 0x0A) ||
          (teensyBuffer[z] == 0x0D) ||
          (teensyBuffer[z] == 0x00) ||
          (z >= packetBufCnt))
      {
        teensyBuffer[z] = 0x00;
        if(setDebug & udpDebug)
        {
          Serial.println((char *) teensyBuffer);
        }
        break;
      }
      z++;
    }
    if(teensyBuffer[z] == 0x00 && z > 0) break;
  }
  packetCnt = strlen(teensyBuffer);
  Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
  Udp.write(teensyBuffer, packetCnt);
  Udp.write("\n");
  Udp.endPacket();
  
  clearBuffers();
}

void setup(void)
{

  Serial.begin(115200);
  delay(1000);
  
  ESP.wdtDisable(); // disable the watchdog Timer

  startDebugUpdate();

  clearBuffers();
  
  Serial.println("+ready+");
  
  if(setDebug & ipDebug)
  {
    Serial.print("wifiIP = ");
    Serial.print(wifiIP);
    Serial.print(":");
    Serial.println(udpPort, HEX);
  }

  while(beginLoop == false)
  {
    for(uint16_t x = 0; x < serStrCnt; x++) serBuf[x] = ' ';
    while(1)
    {
      while(Serial.available())
      {
        serBuf[z] = Serial.read();
        if(setDebug & ipDebug)
        {
          Serial.print("0x");
          if(serBuf[z] < 0x10) Serial.print("0");
          Serial.print(serBuf[z], HEX);
          Serial.print(", ");
        }
        if( (serBuf[z] == 0x0A) || (serBuf[z] == 0x0D) || (serBuf[z] == 0x00) || (z >= serStrCnt))
        {
          serBuf[z] = 0x00;
          if(setDebug & ipDebug)
          {
            Serial.println((char *) serBuf);
          }
          break;
        }
        z++;
      }
      if(serBuf[z] == 0x00) break;
    }
    switch(serBuf[0])
    {
      case 'A': // connect WiFi - returns wifi ip address
      case 'a':
      {
        autoConnectWiFi();
        break;
      }
      case 'B': // start loop();
      case 'b':
      {
        beginLoop = true;
        break;
      }
      case 'C': // connect WiFi - returns wifi ip address
      case 'c':
      {
        connectWiFi();
        break;
      }
      case 'G': // set gateway - returns gateway address
      case 'g':
      {
        setGW();
        break;
      }
      case 'I': // set static IP - returns static IP Address
      case 'i':
      {
        setIP();
        break;
      }
      case 'N': //set subnet address - returns subnet address 
      case 'n':
      {
        setSN();
        break;
      }
      case 'P': // sets wifi password - returns password
      case 'p':
      {
        setPW();
        break;
      }
      case 'S': // set SSID - returns SSID
      case 's':
      {
        setSSID();
        break;
      }
      case 'U': // set UDP port - returns UDP port and starts UDP
      case 'u':
      {
        setUDP();
        break;
      }
      case 'V': // send version Information
      case 'v':
      {
        Serial.println(versionID);
        flushBuffer();
        break;
      }
      default:
      {
        for(uint16_t x = 0; x < serStrCnt; x++) serBuf[x] = ' ';
        break;
      }
    }
  }

//  dBug.detach();
}

void loop(void)
{
  noBytes = Udp.parsePacket();
  if(noBytes) processUDP();
  delay(100);
  yield();
}


