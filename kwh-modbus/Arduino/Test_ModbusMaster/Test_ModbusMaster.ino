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

word *registers = new word[15];
ResilientModbusMaster<HardwareSerial, ArduinoFunctions, ModbusArray> master;
Master<ResilientModbusMaster<HardwareSerial, ArduinoFunctions, ModbusArray>, ArduinoFunctions, DeviceDirectory<byte*>> mstr;
ArduinoFunctions funcions;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Starting...");

  //master.config(&Serial1, &funcions, 19200, 4);
  master.init(registers, 0, 15, 30);
  master.setMaxTimePerTryMicros(100000);
  master.setMaxTries(15);
  Serial.println("Master initialized");
  Serial.print("Initial Memory: ");
  Serial.println(getMemAllocation());
}

int i = 0;
bool done = true;

int getMemAllocation()
{
  int *dummy = new int();
  delete dummy;
  return (int)dummy;
}

void loop() {
  mstr.task();
//  // put your main code here, to run repeatedly:
  if (done)
  {
    if (Serial.available())
    {
      while (Serial.available())
      {
        Serial.read();
      }
      Serial.flush();
      master.reset();
      master.setRequest_WriteRegister(3, 3, i++);
      Serial.println("Master reset");
      done = false;
    }
  }
  else
  {
    done = master.work();
    Serial.print("Status: ");
    Serial.println(master.getStatus());
  }
//

//  if (Serial1.available())
//  {
//    while (Serial1.available())
//    Serial.print(Serial1.read());
//  }
}
