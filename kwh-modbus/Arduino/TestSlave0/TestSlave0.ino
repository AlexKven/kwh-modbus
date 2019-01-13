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

  long micros()
  {
    return ::micros();
  }

  long millis()
  {
    return ::millis();
  }
};

class SourceDevice : public DataCollectorDevice
{
  public:
  SourceDevice()
  {
    init(false, TimeScale::sec1, 8);
  }
  
  bool readDataPoint(uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits)
  {
    dataBuffer[0] = (byte)(time % 256);
  }
};

class DestinationDevice : public Device
{
  private:
  uint32_t curStart;
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
    Serial.print("Recieving data from ");
    for (int i = 0; i < nameLength; i++)
    {
      Serial.write(name[i]);
    }
    Serial.println();
    curStart = startTime;
    outDataPointsPerPage = 10;
    return RecieveDataStatus::success;
   }
   
   RecieveDataStatus receiveDeviceData(byte dataPointsInPage, byte dataPointSize,
   TimeScale timesScale, byte pageNumber, byte* dataPoints)
   {
    for (int i = 0; i <dataPointsInPage; i++)
    {
      Serial.print("Time: ");
      Serial.print(pageNumber * 10 + i);
      Serial.print(" Data value: ");
    }
    return RecieveDataStatus::success;
   }
};

word *registers = new word[40];
typedef ModbusSlave<SoftwareSerial, ArduinoFunctions, ModbusArray> T_Modbus;
typedef Slave<ModbusSlave<SoftwareSerial, ArduinoFunctions, ModbusArray>, ArduinoFunctions> T_Slave;
T_Modbus modbus;
T_Slave slave;
SoftwareSerial mySerial(10, 11);

ArduinoFunctions functions;

int interval = 0;

void setup() {
  for (int i = 0; i < 40; i++)
  {
    registers[i] = 0;
  }

  Serial.begin(9600);
  Serial.println("Starting...");
  
  modbus.config(&mySerial, &functions, 9600, 4);
  Serial.println("Slave initialized");
  modbus.init(registers, 0, 40, 30);
  modbus.setSlaveId(1);
  slave.config(&functions, &modbus);
  
  Device* devices[2];
  devices[0] = new SourceDevice();
  devices[1] = new DestinationDevice();

  byte* names[2];
  names[0] = (byte*)"device00";
  names[1] = (byte*)"device01";

  slave.init(2, 8, 20, 20, devices, names);
}

void loop() {
//  // put your main code here, to run repeatedly:
//  Serial.println(Serial1.available());12

//  Serial.println("");
//  Serial.println("");
//  Serial.println("");
//  Serial.print("Slave ID: ");
//  Serial.println(slave.getSlaveId());

for (int i = 0; i < 1000; i++)
{
  slave.loop();
//  
//  delay(5);
}

//  for (int i = 0; i < 15; i++)
//  {
//    Serial.print(registers[i]);
//    Serial.print(" ");
//  }
//  Serial.println("");
//  delay(2000);
//
//  if (Serial1.available())
//  {
//    while (Serial1.available())
//    Serial.print(Serial1.read());
//  }
//  if (Serial.available())
//  {
//    while (Serial.available())
//    {
//      digitalWrite(4, HIGH);
//      delay(30);
//      Serial1.write(Serial.read());
//      delay(30);
//      digitalWrite(4, LOW);
//      delay(30);
//    }
//  }
}
