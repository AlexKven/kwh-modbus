#define CONSOLE_DEBUG
#define CONSOLE_INFO
#define CONSOLE_VERBOSE
#define PRINTLN(MSG) Serial.println(MSG)
#define PRINT(MSG) Serial.print(MSG)
#define WRITE(CHR) Serial.write(CHR)
#define P_TIME() Serial.print(millis()); Serial.print("ms ")

#include "Arduino.h"
#include "BitFunctions.hpp"
#include "Modbus.h"
#include "ModbusArray.h"
#include "ModbusSerial.hpp"
#include "ModbusSlave.hpp"
#include "TimeManager.h"
#include "Device.h"
#include "DataCollectorDevice.h"
#include "Slave.hpp"
#include "HardwareSerial.h"
#include "SoftwareSerial.h"

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <TimeLib.h>

DEBUG_CATEGORY_ALL

int getMemAllocation()
{
  int *dummy = new int();
  delete dummy;
  return (int)dummy;
}

class ArduinoFunctions
{
public:
  void delayMicroseconds(long len)
  {
    ::delayMicroseconds(len);
  }

  void digitalWrite(unsigned char pin, unsigned char value)
  {
    ::digitalWrite(pin, value);
  }

  uint8_t digitalRead(unsigned char pin)
  {
    return ::digitalRead(pin);
  }

  void pinMode(unsigned char pin, unsigned char mode)
  {
    ::pinMode(pin, mode);
  }


  uint64_t long micros()
  {
    static uint32_t low32_micros, high32_micros = 0;
    uint32_t new_low32_micros = ::micros();
    if (new_low32_micros < low32_micros) high32_micros++;
    low32_micros = new_low32_micros;
    return (uint64_t)high32_micros << 32 | (uint64_t)low32_micros;
  }

  uint64_t long millis()
  {
    static uint32_t low32_millis, high32_millis = 0;
    uint32_t new_low32_millis = ::millis();
    if (new_low32_millis < low32_millis) high32_millis++;
    low32_millis = new_low32_millis;
    return (uint64_t)high32_millis << 32 | (uint64_t)low32_millis;
  }
};

class SourceDevice : public DataCollectorDevice
{
  public:
  SourceDevice()
  {
    init(false, TimeScale::min1, 24);
  }
  
  bool readDataPoint(uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits)
  {
    Serial.print(F("Result: "));
    unsigned int result = (unsigned int)(sin((float)(time % 3600) * 3.14159265358 / 1800) * 2000 + 2500);
    Serial.println(result);
    dataBuffer[0] = (byte)(result & 0xFF);
    result >>= 8;
    dataBuffer[1] = (byte)(result & 0xFF);
    result >>= 8;
    dataBuffer[2] = (byte)(result & 0xFF);
  }
};


EthernetClient client;
EthernetUDP Udp;
IPAddress timeServer(128, 138, 140, 44);
const int NTP_PACKET_SIZE= 48;

class DestinationDevice : public Device
{
  private:
  uint32_t curStart;
  char* dataString = "#STA:54321;TM:02/09/2019,19:45:00;C:15;V:7.19;PU01:00000000;PU02:00000000;PU03:00000000;PU04:00000000;DI:333300;DO:0000;#";
  
  void applyIntToCharArray(char* arr, int startIndex, long num, int places)
  {
    for (int i = 0; i < places; i++)
    {
      int index = places - i - 1;
      arr[startIndex + index] = (num % 10) + '0';
      num /= 10;
    }
  }
  public:
  word getType()
  {
    return 2 << 14;
  }

  RecieveDataStatus prepareReceiveData(word nameLength, byte* name, uint32_t startTime,
   byte dataPointSize, TimeScale dataTimeScale, word dataPointsCount, byte &outDataPointsPerPage)
   {
    Serial.println();
    Serial.println();
    Serial.print(F("Recieving data from "));
    for (int i = 0; i < nameLength; i++)
    {
      Serial.write(name[i]);
    }
    curStart = startTime;
    setTime(946684800 + curStart);
    Serial.println();
    Serial.print("Time: ");
    Serial.print(monthStr(month()));
    Serial.print(" ");
    Serial.print(day());
    Serial.print(" ");
    Serial.print(year());
    Serial.print(" ");
    Serial.print(hour());
    Serial.print(":");
    Serial.print(minute());
    Serial.print(":");
    Serial.println(second());
    Serial.print(F("Data point size: "));
    Serial.println(dataPointSize);
    
    outDataPointsPerPage = 4;
    return RecieveDataStatus::success;
   }
   
   RecieveDataStatus receiveDeviceData(byte dataPointsInPage, byte dataPointSize,
   TimeScale timesScale, byte pageNumber, byte* dataPoints)
   {
    byte timePeriod = 1;
    if (timesScale == TimeScale::ms250)
      timePeriod = 4;
    for (int i = 0; i <dataPointsInPage; i++)
    {
      setTime(946684800 + curStart + i * 60);
      uint32_t value = dataPoints[i * 3] + dataPoints[i * 3 + 1] * 256 + dataPoints[i * 3 + 2] * 256 * 256;
      Serial.print(" Pg: ");
      Serial.print(pageNumber);
      Serial.print(" Pt: ");
      Serial.print(i);
      Serial.print(" Val: ");
      Serial.println(value);
      
      if (year() == 2019)
      {
        applyIntToCharArray(dataString, 14, month(), 2);
        applyIntToCharArray(dataString, 17, day(), 2);
        applyIntToCharArray(dataString, 20, year(), 4);
        applyIntToCharArray(dataString, 25, hour(), 2);
        applyIntToCharArray(dataString, 28, minute(), 2);
        applyIntToCharArray(dataString, 51, value, 8);

        if (value == 0xFFFFFFFF)
        {
          Serial.println("'No Data' value");
        }
        else
        {
           client.println(dataString);
           client.println();
        }
      }
//      delay(100);
    }
    return RecieveDataStatus::success;
   }
};

class TimeServerDevice : public Device
{
private:
  byte packetBuffer[NTP_PACKET_SIZE];
  unsigned int localPort = 8888;
  unsigned long millisLastTimeRequest = 0;
  unsigned long lastTimeResult = 0;
  
public:
  word getType()
  {
    return 1;
  }

  void sendNTPpacket(IPAddress& address)
  {
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    packetBuffer[0] = 0b11100011;
    packetBuffer[1] = 0;
    packetBuffer[2] = 6;
    packetBuffer[3] = 0xEC;
    packetBuffer[12]  = 49;
    packetBuffer[13]  = 0x4E;
    packetBuffer[14]  = 49;
    packetBuffer[15]  = 52;                 
    Udp.beginPacket(address, 123);
    Udp.write(packetBuffer,NTP_PACKET_SIZE);
    Udp.endPacket();
  }

  bool getTimeAndDate(unsigned long &result) {
    unsigned long elapsed = millis() - millisLastTimeRequest;
    result = 0;
    if (millisLastTimeRequest == 0 || elapsed > 900000)
    {
      Serial.println("bar");
      lastTimeResult = 0;
       millisLastTimeRequest = millis();
       Udp.begin(localPort);
       sendNTPpacket(timeServer);
       return false;
    }
    if (lastTimeResult == 0)
    {
     if (Udp.parsePacket()){
        Serial.println("foo");
       auto sze = Udp.read(packetBuffer,NTP_PACKET_SIZE);  // read the packet into the buffer
       Serial.print("Packet size: ");
       Serial.println(sze);
       unsigned long highWord, lowWord;
       highWord = word(packetBuffer[40], packetBuffer[41]);
       lowWord = word(packetBuffer[42], packetBuffer[43]); 
       result = highWord << 16 | lowWord;
       result = result - 2208988800 - 946684800;
       Serial.println(result);
       lastTimeResult = result;
     }
    }
    Serial.println(lastTimeResult);
    result = lastTimeResult + (elapsed / 1000);
     return true;
   return false;
}

  uint32_t masterRequestTime()
  {
    Serial.println("Master requested time!");
    unsigned long result = 0;
    getTimeAndDate(result);
    Serial.print("Time is ");
    Serial.println(result);
    return result;
  }
};

SoftwareSerial serial(50, 51);
word *registers = new word[60];
typedef ModbusSlave<HardwareSerial, ArduinoFunctions, ModbusArray> T_Modbus;
typedef Slave<ModbusSlave<HardwareSerial, ArduinoFunctions, ModbusArray>, ArduinoFunctions> T_Slave;
T_Modbus modbus;
T_Slave slave;

ArduinoFunctions functions;

int interval = 0;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
char server[] = "kwhstg.org";
IPAddress ip(192, 168, 0, 177);
IPAddress myDns(192, 168, 0, 1);

void setup() {

  

  Serial.begin(9600);
  Serial.println(F("Starting..."));

  
    // start the Ethernet connection:
  Serial.println(F("Initialize Ethernet:"));
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed");
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println(F("Shield not found"));
    }
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println(F("Ethernet cable not connected."));
    }
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip, myDns);
  } else {
    Serial.print("  assigned IP ");
    Serial.println(Ethernet.localIP());
  }
  
  // if you get a connection, report back via serial:
  if (client.connect(server, 11002)) {
    Serial.print(F("connected to "));
    Serial.println(client.remoteIP());
  } else {
    // if you didn't get a connection to the server:
    Serial.println(F("connection failed"));
  }

  
  
  for (int i = 0; i < 60; i++)
  {
    registers[i] = 0;
  }
  
  modbus.config(&Serial1, &functions, 9600, 4);
  Serial.println(F("Slave initialized"));
  modbus.init(registers, 0, 60, 80);
  modbus.setSlaveId(1);
  slave.config(&functions, &modbus);
  
  Device* devices[2];
//  devices[0] = new SourceDevice();
  devices[0] = new DestinationDevice();
  devices[1] = new TimeServerDevice();

  byte* names[3];
//  names[0] = (byte*)"meter0";
  names[0] = (byte*)"destin";
  names[1] = (byte*)"tmesrc";

  slave.init(2, 6, 15, 20, devices, names);

}

unsigned long lastDisplay = 0;

void loop() {
  slave.loop();
  if (millis() - lastDisplay >= 1000)
  {
    lastDisplay = millis();
  }
}
