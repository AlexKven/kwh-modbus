#include "Arduino.h"
#include "modbus.h"
#include "modbusmemory.h"
#include "modbusserial.h"


class ArduinoFunctions
{
  void DelayMicroseconds(long len)
  {
    
  }

  void DigitalWrite(unsigned char pin, unsigned char value)
  {
    
  }

  void PinMode(unsigned char pin, unsigned char mode)
  {
    
  }
};

void setup() {
  // put your setup code here, to run once:
  //Serial *ser = &Serial1;
  
  ModbusMemory<ModbusSerial<HardwareSerial, ArduinoFunctions, Modbus>> slave;
  
}

void loop() {
  // put your main code here, to run repeatedly:

}
