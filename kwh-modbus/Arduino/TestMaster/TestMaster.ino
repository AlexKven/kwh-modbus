#define CONSOLE_DEBUG
#define CONSOLE_INFO
#define CONSOLE_VERBOSE
#define PRINTLN(MSG) Serial.println(MSG)
#define PRINT(MSG) Serial.print(MSG)
#define WRITE(CHR) Serial.write(CHR)
#define P_TIME() Serial.print(millis()); Serial.print("ms ")

#include "Arduino.h"
#include "Modbus.h"
#include "ModbusArray.h"
#include "ModbusSerial.hpp"
#include "ModbusMaster.hpp"
#include "HardwareSerial.h"
#include "ResilientModbusMaster.hpp"
#include "TimeManager.h"
#include "Device.h"
#include "Master.hpp"
#include "DeviceDirectory.hpp"
#include "SoftwareSerial.h"

DEBUG_CATEGORY(checkForSlaves|readAndSendData|transferPendingData|sendDataToSlaves)

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

word *registers = new word[50];
typedef ResilientModbusMaster<SoftwareSerial, ArduinoFunctions, ModbusArray> T_Modbus;
typedef Master<ResilientModbusMaster<SoftwareSerial, ArduinoFunctions, ModbusArray>, ArduinoFunctions, DeviceDirectory> T_Master;

T_Modbus modbus;
T_Master master;
ArduinoFunctions functions;
DeviceDirectory directory;
SoftwareSerial mySerial(50, 51);

void setup() {
  for (int i = 0; i < 50; i++)
  {
    registers[i] = 0;
  }
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Starting...");

  modbus.config(&mySerial, &functions, 9600, 4);
  modbus.init(registers, 0, 50, 80);
  modbus.setMinTimePerTryMicros(15000);
  modbus.setMaxTimePerTryMicros(100000);
  modbus.setMaxTries(10);
  directory.init(8, 20);
  master.config(&functions, &modbus, &directory, 40, 30);
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
  master.loop();
  }
