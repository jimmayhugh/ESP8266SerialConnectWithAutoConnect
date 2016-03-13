/********************

teensySetupAndTestESP8266.ino

Version 0.0.1
Last Modified 03/06/2016
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


*********************/

const uint16_t BUFFER_SIZE = 512;

const uint8_t CHPD   = 14;
const uint8_t GPIO2  = 15;
const uint8_t ESPRST = 16;
const uint8_t GPIO0  = 17;
const uint8_t spCnt  = 32;
const uint8_t ipCnt  = 18;
const uint8_t upCnt  = 7;

const uint32_t baudRate = 115200;
 
char buffer[BUFFER_SIZE];
char serialPacketBuffer[2048]; //buffer to hold incoming packet,

char ssidStr[spCnt] = "";
char passStr[spCnt] = "";
char ipStr[ipCnt]   = "";
char gwStr[ipCnt]   = "";
char snStr[ipCnt]   = "";
char upStr[upCnt]   = "";
char eipStr[ipCnt]  = "";

uint8_t esp8266z = 0;
uint8_t esp8266c = 0;

bool serialUdpReady = false;
bool useSerial = false;


// Should restart Teensy 3, will also disconnect USB during restart

// From http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0337e/Cihcbadd.html
// Search for "0xE000ED0C"
// Original question http://forum.pjrc.com/threads/24304-_reboot_Teensyduino%28%29-vs-_restart_Teensyduino%28%29?p=35981#post35981

#define RESTART_ADDR       0xE000ED0C
#define READ_RESTART()     (*(volatile uint32_t *)RESTART_ADDR)
#define WRITE_RESTART(val) ((*(volatile uint32_t *)RESTART_ADDR) = (val))

// function prototypes
void softReset(void);
void resetESP8266(void);
bool read_till_eol(uint32_t);
void showSetup(void);
void getParameters(void);
bool setupWiFi(void);

void softReset(void)
{
  // 0000101111110100000000000000100
  // Assert [2]SYSRESETREQ
  WRITE_RESTART(0x5FA0004);
}  

void resetESP8266(void)
{
  // Reset The ESP-01
  Serial.println("Resetting ESP8266");  
  digitalWrite(ESPRST, LOW);
  delay(500);
  digitalWrite(ESPRST, HIGH);
  read_till_eol(5000);
  while(Serial1.available()) Serial1.read();
  Serial.println("++ready++");  
  read_till_eol(500);
  delay(5000);
}

bool read_till_eol(uint32_t timeout)
{
  uint32_t t;
  // wait for at most timeout milliseconds

  t = millis();
  uint16_t i;

  for(i = 0; i < BUFFER_SIZE; i++) buffer[i] = 0xFF;
  i = 0;

  while(millis() < t+timeout)
  {
    if(Serial1.available())
    {
      buffer[i] = Serial1.read();
      if((buffer[i] == 0x0D || buffer[i] == 0x0A))
      {
        while(i < BUFFER_SIZE)
        {
          buffer[i] = 0x00;
          i++;
        }
        return true;
      }else{
        i++;
      }
    }
  }
  return false;
}

void showSetup(void)
{
  Serial.print("ssid = ");
  Serial.print(ssidStr);
  Serial.println(";");
  Serial.print("pass = ");
  Serial.print(passStr);
  Serial.println(";");
  Serial.print("WiFi ip = ");
  Serial.print(ipStr);
  Serial.println(";");
  Serial.print("gw = ");
  Serial.print(gwStr);
  Serial.println(";");
  Serial.print("sn = ");
  Serial.print(snStr);
  Serial.println(";");
  Serial.print("up = ");
  Serial.print(upStr);
  Serial.println(";");
  Serial.print("Ethernet ip = ");
  Serial.println(eipStr);
}

void getParameters(void)
{
  bool parametersReady = false;
  char delim[] = ";"; 
//  char *result = NULL;
    
  // request autoconnect
  Serial.print("Requesting Autoconnect:");
  Serial1.print("a");
  Serial1.println(upStr);

  while(parametersReady == false)
  {
    if(Serial1.available())
    {
      serialPacketBuffer[esp8266c] = Serial1.read();
      Serial.print(serialPacketBuffer[esp8266c]);
      if((serialPacketBuffer[esp8266c] == 0x0D || serialPacketBuffer[esp8266c] == 0x0A || serialPacketBuffer[esp8266c] == 0x00))
      {
        serialPacketBuffer[esp8266c] = 0x00;
        Serial.println();
        Serial.println("ready to process serialUDP");
        parametersReady = true;
      }else{
        esp8266c++;
      }
    }
  }
//  result = strtok( serialPacketBuffer, delim );

  char *ssidStrPtr = strtok( serialPacketBuffer, delim );
  strcpy(ssidStr, ssidStrPtr);
  char *passStrPtr = strtok( NULL, delim );
  strcpy(passStr, passStrPtr);
  char *ipStrPtr   = strtok( NULL, delim );
  strcpy(ipStr, ipStrPtr);
  char *gwStrPtr   = strtok( NULL, delim );
  strcpy(gwStr, gwStrPtr);
  char *snStrPtr   = strtok( NULL, delim );
  strcpy(snStr, snStrPtr);
  char *upStrPtr   = strtok( NULL, delim );
  strcpy(upStr, upStrPtr);
  char *eipStrPtr  = strtok( NULL, delim );
  strcpy(eipStr, eipStrPtr);
  Serial.println("Autoconnet Result:");
  showSetup();
}

bool setupWiFi(void)
{
//  send SSID
//  showSetup();
  Serial.print("Sending SSID:");
  Serial1.print("s");
  Serial1.println(ssidStr);

  read_till_eol(4000) ? Serial.println(buffer) : Serial.println("FAILED");
  delay(100);
  while(Serial1.available()) Serial1.read();

  // send password
  Serial.print("Sending password:");
  Serial1.print("p");
  Serial1.println(passStr);

  read_till_eol(4000) ? Serial.println(buffer) : Serial.println("FAILED");
  delay(100);
  while(Serial1.available()) Serial1.read();

  // send staticIP
  Serial.print("Sending staticIP:");
  Serial1.print("i");
  Serial1.println(ipStr);

  read_till_eol(4000) ? Serial.println(buffer) : Serial.println("FAILED");
  delay(100);
  while(Serial1.available()) Serial1.read();

  // send gateway address
  Serial.print("Sending gateway:");
  Serial1.print("g");
  Serial1.println(gwStr);

  read_till_eol(4000) ? Serial.println(buffer) : Serial.println("FAILED");
  delay(100);
  while(Serial1.available()) Serial1.read();

  // send subnet address
  Serial.print("Sending subnet:");
  Serial1.print("n");
  Serial1.println(snStr);

  read_till_eol(4000) ? Serial.println(buffer) : Serial.println("FAILED");
  delay(100);
  while(Serial1.available()) Serial1.read();

  // send udpPort
  Serial.print("Sending udpPort:");
  Serial1.print("u");
  Serial1.println(upStr);

  read_till_eol(4000) ? Serial.println(buffer) : Serial.println("FAILED");
  delay(100);
  while(Serial1.available()) Serial1.read();

  bool retry = true;
  // start WiFi
  while(1)
  {
    Serial.print("Starting Wifi:");
    Serial1.println("c");
    if(read_till_eol(10000))
    {
      if(read_till_eol(4000)) Serial.println(buffer);
      Serial.println("Sending begin loop()");
      Serial1.println("b");
      serialUdpReady = true;
      return serialUdpReady;
    }else{
      Serial.println("FAILED - Reset and Retry");
      serialUdpReady = false;
      return serialUdpReady;
    }
    delay(100);
    while(Serial1.available()) Serial1.read();
  }
}

void setup(void)
{
  pinMode(CHPD , INPUT_PULLUP);
  pinMode(GPIO2 , INPUT_PULLUP);
  pinMode(GPIO0 , INPUT_PULLUP);
  pinMode(ESPRST , OUTPUT);

  digitalWrite(ESPRST, HIGH);
  delay(1000);

  Serial.begin(baudRate);   // Teensy USB Serial Port  
  delay(500);

  Serial1.begin(baudRate);  // Teensy Hardware Serial port 1   (pins 0 and 1)
  delay(500);
  

  resetESP8266();    
  getParameters();
  while(serialUdpReady == false)
  {
    resetESP8266();
    delay(100);
    if(read_till_eol(1000)) Serial.println(buffer);
    Serial.println("begin");  
    if(read_till_eol(1000)) Serial.println(buffer);
    serialUdpReady = setupWiFi();
  }
}

void loop(void)
{
  if(read_till_eol(1000))
  {
    Serial.println(buffer);
    while(Serial1.available()) Serial1.read();
    Serial1.print("OK\n");
  }
}
