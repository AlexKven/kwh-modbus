#include "Arduino.h"
#include "Modbus.h"
#include "ModbusArray.h"
#include "ModbusSerial.hpp"
#include "ModbusMaster.hpp"
#include "HardwareSerial.h"


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
};

word *registers = new word[15];
ModbusMaster<HardwareSerial, ArduinoFunctions, ModbusArray> master;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Starting...");

  master.config(&Serial1, new ArduinoFunctions(), 19200, 4);
  master.init(registers, 0, 15, 30);
  Serial.println("Master initialized");
}

int i = 0;
bool done = true;

void loop() {
//  // put your main code here, to run repeatedly:
  if (done)
  {
    master.setRequest_WriteRegister(3, 3, i++);
    master.send();
    Serial.println("Master done");
    done = false;
  }
  else
  {
    done = master.receive();
  }
//
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
//  if (Serial1.available())
//  {
//    while (Serial1.available())
//    Serial.print(Serial1.read());
//  }
}
