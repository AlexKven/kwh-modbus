#include "Arduino.h"
#include "Modbus.h"
#include "ModbusArray.h"
#include "ModbusSerial.hpp"
#include "ModbusSlave.hpp"
#include "HardwareSerial.h"
#include "AsyncAwait.hpp"


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
ModbusSlave<HardwareSerial, ArduinoFunctions, ModbusArray> slave;

void setup() {
  // put your setup code here, to run once:
  //Serial *ser = &Serial1;

  int c = __COUNTER__;
  int l = __LINE__;

  for (int i = 0; i < 15; i++)
  {
    registers[i] = 0;
  }

  Tuple<int, String, long> t;
  String j = Get<1>(t);
  

  Serial.begin(9600);
  Serial.println("Starting...");
  
  slave.config(&Serial1, new ArduinoFunctions(), 19200, 4);
  Serial.println("Slave initialized");
  slave.init(registers, 0, 15, 30);
  slave.setSlaveId(3);
}

void loop() {
//  // put your main code here, to run repeatedly:
//  Serial.println(Serial1.available());12

  bool processed;
  bool broadcast;

  slave.task(processed, broadcast);

  for (int i = 0; i < 15; i++)
  {
    Serial.print(registers[i]);
    Serial.print(" ");
  }
  Serial.println("");
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
