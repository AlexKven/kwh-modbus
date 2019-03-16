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
  private:
  unsigned long getMinute(unsigned long clock)
  {
    return clock / 60;
  }

  unsigned long getMinute()
  {
    return getMinute(getTimeSource()->getClock());
  }

  double WATTHOURS_PER_PULSE = 0.0026944444465999994;
  double UNITS_PER_PULSE = 1.412664914;
  
  public:
  SourceDevice()
  {
    init(false, TimeScale::min1, 24);
  }
  
  bool readDataPoint(uint32_t time, byte quarterSecondOffset, byte* dataBuffer, byte dataSizeBits)
  {
    double WATTHOURS_PER_UNIT = 0.0019073486;
    Serial.print(F("Requested data for: "));
    Serial.print(getMinute(time));
    Serial.print(F(". Current minute: "));
    Serial.println(getMinute());
    uint32_t timeDiff = getMinute() - getMinute(time);
    uint32_t result = 0;
    if (getNumMinutesStored() > timeDiff)
    {
//      unsigned long pulseDiff = getAndResetPulses();
      result = getCachedMinute(timeDiff);
      double wattHours = (double)result * WATTHOURS_PER_UNIT;

//      result = (double)pulseDiff * UNITS_PER_PULSE;
//      Serial.print(F(" Pulses: "));
//      Serial.print(pulseDiff);
      Serial.print(F(" Watthours: "));
      Serial.print(wattHours);
      Serial.print(F(" Units: "));
      Serial.println(result);
      dataBuffer[0] = (byte)(result & 0xFF);
      result >>= 8;
      dataBuffer[1] = (byte)(result & 0xFF);
      result >>= 8;
      dataBuffer[2] = (byte)(result & 0xFF);
      return true;
    }
//    return false;
//    Serial.print(F("Result: "));
//    unsigned int result = (unsigned int)(sin((float)(time % 3600) * 3.14159265358 / 1800) * 2000 + 2500);
//    Serial.println(result);
  }
};

word *registers = new word[60];
typedef ModbusSlave<SoftwareSerial, ArduinoFunctions, ModbusArray> T_Modbus;
typedef Slave<ModbusSlave<SoftwareSerial, ArduinoFunctions, ModbusArray>, ArduinoFunctions> T_Slave;
T_Modbus modbus;
T_Slave slave;

SoftwareSerial serialSource(50, 51);
ArduinoFunctions functions;

void setup() {
  for (int i = 0; i < 60; i++)
  {
    registers[i] = 0;
  }

  Serial.begin(9600);
  Serial.println(F("Starting..."));
  
  modbus.config(&serialSource, &functions, 9600, 4);
  Serial.println(F("Slave initialized"));
  modbus.init(registers, 0, 60, 80);
  modbus.setSlaveId(1);
  slave.config(&functions, &modbus);
  
  Device* devices[1];
  devices[0] = new SourceDevice();

  byte* names[1];
  names[0] = (byte*)"meter0";

  slave.init(1, 6, 15, 20, devices, names);
  
//  pinMode(4, OUTPUT);
//  digitalWrite(4, HIGH);

//  setupMeter();
}

void loop() {
  slave.loop();
//  loopMeter();
}
