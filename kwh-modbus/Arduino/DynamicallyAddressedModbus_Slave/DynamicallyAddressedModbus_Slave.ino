/**
 *  Modbus slave example 1:
 *  The purpose of this example is to link a data array
 *  from the Arduino to an external device.
 *
 *  Recommended Modbus Master: QModbus
 *  http://qmodbus.sourceforge.net/
 */

#include <ModbusRtu.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include "RegisterData.cpp"

#define UNASSIGNED 247;

// data array for modbus network sharing
RegisterData *Registers;
// = { 
//  1, // 1 = meter, 2 = transmitter (SD card or wireless)
//  0, // Newly Assigned ID (waiting for this from master)
//  0, // Previously Assigned ID (if deactivated)
//  0
//  };

uint8_t SlaveID = 247;

char* Dev1Name = const_cast<char*>("Cha_Arc1");
char* Dev2Name = const_cast<char*>("Cha_Mtr1");
char* Dev3Name = const_cast<char*>("Cha_Dev3");
char* Dev4Name = const_cast<char*>("Cha_Dev4");
char* Dev5Name = const_cast<char*>("Cha_Dev5");
char Dev1Type = 1;
char Dev2Type = 2;
char Dev3Type = 128;
char Dev4Type = 130;
char Dev5Type = 129;

bool Assigning = false;

SoftwareSerial serial(10, 11);

/**
 *  Modbus object declaration
 *  u8id : node id = 0 for master, = 1..247 for slave
 *  u8serno : serial port (use 0 for Serial)
 *  u8txenpin : 0 for RS-232 and USB-FTDI 
 *               or any pin number > 1 for RS-485
 */
Modbus slave(247,1,4); // this is slave @1 and RS-232 or USB-FTDI

void setup()
{
  Registers = new RegisterData(26);
  Serial.begin(9600);
  Serial.println("Initialized unassigned");
  slave.begin(19200); // baud-rate at 19200
  Registers->set<uint8_t>(0, 0);
}

void populateMessage()
{
  Registers->set<uint8_t>(2, Dev1Type);
  Registers->set<uint8_t>(12, Dev2Type);
  Registers->set<uint8_t>(22, Dev3Type);
  Registers->set<uint8_t>(32, Dev4Type);
  Registers->set<uint8_t>(42, Dev5Type);
  Registers->setString(3, Dev1Name, 8);
  Registers->setString(13, Dev2Name, 8);
  Registers->setString(23, Dev3Name, 8);
  Registers->setString(33, Dev4Name, 8);
  Registers->setString(43, Dev5Name, 8);
  for (int i = 0; i < 50; i++)
  {
    Serial.println(Registers->get<uint8_t>(i + 2));
  }
}

void receiveAssignment()
{
  Registers->set<uint8_t>(0, 0);
  SlaveID = Registers->get<uint8_t>(3);
  slave.setID(SlaveID);
  Serial.print("Successfully assigned slave # ");
  Serial.println(SlaveID);
}

void receiveDataRequest(uint8_t len)
{
  Serial.print("Data request received:");
  for (int i = 1; i < len; i++)
  {
    Serial.print(" ");
    Serial.print(Registers->get(i));
  }
  Serial.println("");
}

void loop()
{
  uint8_t *bytes = Registers->getArray<uint8_t>();
  slave.poll(Registers->getArray(), Registers->getLength());
  if (SlaveID == 247) //Slave is unassigned
  {
    if (Registers->get<uint8_t>(0) == 1) //About to be polled
    {
      if (!Assigning)
      {
        Assigning = true;
        populateMessage();
      }
      else if (Registers->get<uint8_t>(2) == 0) //Slave was just assigned
      {
        Assigning = false;
        receiveAssignment();
      }
    }
  }
  else
  {
    delay(250);
    Serial.println(Registers->get<uint8_t>(0));
    if (Registers->get<uint8_t>(0) == 2)
    {
      Serial.println("Something...");
      uint8_t len = Registers->get<uint8_t>(1);
      while (Registers->get<uint8_t>(0) == 2)
      {
        slave.poll(Registers->getArray(), len);
        delay(1);
      }
      receiveDataRequest(len);
    }
  }
  delay(1);
}
