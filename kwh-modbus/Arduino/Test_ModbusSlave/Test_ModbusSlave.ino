#include "Arduino.h"
#include "Modbus.h"
#include "ModbusArray.h"
#include "ModbusSerial.hpp"
#include "ModbusSlave.hpp"
#include "HardwareSerial.h"


class ArduinoFunctions
{
public:
  void delayMicroseconds(long len)
  {
    delayMicroseconds(len);
  }

  void digitalWrite(unsigned char pin, unsigned char value)
  {
    digitalWrite(pin, value);
  }

  uint8_t digitalRead(unsigned char pin)
  {
    return digitalRead(pin);
  }

  void pinMode(unsigned char pin, unsigned char mode)
  {
    pinMode(pin, mode);
  }
};

void setup() {
  // put your setup code here, to run once:
  //Serial *ser = &Serial1;

  
  
  ModbusSlave<HardwareSerial, ArduinoFunctions, ModbusArray> slave;
  slave.config(&Serial1, new ArduinoFunctions(), 1200, 4);
  
}

void loop() {
  // put your main code here, to run repeatedly:

}
