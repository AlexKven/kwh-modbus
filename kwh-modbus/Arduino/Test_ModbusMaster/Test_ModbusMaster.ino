#include "Arduino.h"
#include "Modbus.h"
#include "ModbusArray.h"
#include "ModbusSerial.hpp"
#include "ModbusMaster.hpp"
#include "HardwareSerial.h"
#include "ResilientModbusMaster.hpp"
#include "Master.hpp"
#include "DeviceDirectory.hpp"


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

  unsigned long micros()
  {
    return ::micros();
  }

  unsigned long millis()
  {
    return ::millis();
  }
};

word *registers = new word[20];
typedef ResilientModbusMaster<HardwareSerial, ArduinoFunctions, ModbusArray> T_Modbus;
typedef Master<ResilientModbusMaster<HardwareSerial, ArduinoFunctions, ModbusArray>, ArduinoFunctions, DeviceDirectory<byte*>> T_Master;

T_Modbus modbus;
T_Master master;
ArduinoFunctions functions;
DeviceDirectory<byte*> directory;

void setup() {
  for (int i = 0; i < 20; i++)
  {
    registers[i] = 0;
  }
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Starting...");

  modbus.config(&Serial1, &functions, 9600, 4);
  modbus.init(registers, 0, 20, 30);
  modbus.setMaxTimePerTryMicros(1000000);
  modbus.setMaxTries(15);
  directory.init(8, 20);
  master.config(&functions, &modbus, &directory);
  Serial.println("Master initialized");
  Serial.print("Initial Memory: ");
  Serial.println(getMemAllocation());
}

int i = 0;
bool done = true;
int interval = 0;

int getMemAllocation()
{
  int *dummy = new int();
  delete dummy;
  return (int)dummy;
}

void loop() {
//  Serial.println("loop start");
  Serial.println("");
  Serial.println("");
  Serial.println("");

  T_Master::checkForNewSlaves_Task task0(&T_Master::checkForNewSlaves, &master);
  T_Master::processNewSlave_Task task1(&T_Master::processNewSlave, &master, false);

  while (!task0())
  {
//    delay(20);
  }
  while (!task1())
  {
//    delay(20);
  }
//  master.task();
  if (directory.isEmpty())
  {
    Serial.println("Directory is empty.");
//  Serial.print("current Memory: ");
//  Serial.println(getMemAllocation());
  }
  else
  {
    int row = 0;
    byte devName[8];
    byte devSlaveId;
    word devType;
    do
    {
      row = directory.findNextDevice(devName, devSlaveId, devType, row);
      Serial.println("");
      Serial.print("Device ");
      for (int i = 0; i < 8; i++)
        Serial.write(devName[i]);
      Serial.println(":");
      Serial.print("Slave ID: ");
      Serial.println(devSlaveId);
      Serial.print("Device type: ");
      Serial.println(devType);
    } while (row != -1);
  }

  delay(500);
}
