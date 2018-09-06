#include <Time.h>
#include <TimeLib.h>

/**
 *  Modbus master example 1:
 *  The purpose of this example is to query an array of data
 *  from an external Modbus slave device. 
 *  The link media can be USB or RS232.
 *
 *  Recommended Modbus slave: 
 *  diagslave http://www.modbusdriver.com/diagslave.html
 *
 *  In a Linux box, run 
 *  "./diagslave /dev/ttyUSB0 -b 19200 -d 8 -s 1 -p none -m rtu -a 1"
 *   This is:
 *    serial port /dev/ttyUSB0 at 19200 baud 8N1
 *    RTU mode and address @1
 */

#include <ModbusRtu.h>
#include <EEPROM.h>
#include "RegisterData.cpp"

const int UNASSIGNED = 247;
const int MAX_DEVICES_PER_SLAVE = 5;
const int MAX_RESPONSE_SIZE = 120;
const int MAX_TRANSMISSION_SIZE = 120;

enum
{
  WAIT = 0,
  CHECK_NEW_DEVICES = 1,
  CHECK_NEW_DEVICES_WAIT = 2,
  ASSIGN_NEW_DEVICE = 3,
  ASSIGN_NEW_DEVICE_WAIT = 4,
  REQUEST_DATA = 5,
  REQUEST_DATA_WAIT = 6,
  RECEIVE_DATA = 7,
  RECEIVE_DATA_WAIT = 8
};

// data array for modbus network sharing
RegisterData *Transmission;
RegisterData *Response;
uint8_t** DevNameList;
uint8_t State;
bool SlaveIDsExhausted = false;
int32_t LastCheckNewDevicesTime; //Every 2000
int32_t LastAcquireDataTime; //Every 60000 (if time is known)
int32_t LastCheckCommandsTime; //Every 1000
int32_t LastRequestDateTimeTime; //Every 86400000 or 15000 (if time is unknown)

SoftwareSerial serial(10, 11);

/**
 *  Modbus object declaration
 *  u8id : node id = 0 for master, = 1..247 for slave
 *  u8serno : serial port (use 0 for Serial)
 *  u8txenpin : 0 for RS-232 and USB-FTDI 
 *               or any pin number > 1 for RS-485
 */
Modbus master(0,1,4); // this is master and RS-232 or USB-FTDI

/**
 * This is an structe which contains a query to an slave device
 */
modbus_t telegram;

void setup()
{
  Transmission = new RegisterData(MAX_TRANSMISSION_SIZE);
  Response = new RegisterData(MAX_RESPONSE_SIZE);
  DevNameList = new uint8_t*[MAX_DEVICES_PER_SLAVE];
  InitializeDeviceDirectory();
  
  Serial.begin(9600);
  master.begin( 19200 );
  
  int32_t curTime = millis();
  LastCheckNewDevicesTime = curTime - 1000;
  
  State = WAIT;
}

void sendPreMessage(uint8_t code, uint8_t size, uint8_t recipient)
{
  Transmission->set<uint8_t>(0, code);
  Transmission->set<uint8_t>(1, size);
  telegram.u8id = recipient; // slave address
  telegram.u8fct = 16; // function code (this one is registers write)
  telegram.u16RegAdd = 0; // start address in slave
  telegram.u16CoilsNo = 1; // number of elements (coils or registers) to write
  telegram.au16reg = Transmission->getArray(); // pointer to a memory array in the Arduino
  master.query(telegram);
  do
  {
    master.poll();
  } while (master.getState() != COM_IDLE);
  delay(2000);
}



void loop()
{
  uint8_t newID;
  uint8_t devSlaveID;
  uint8_t devType;
  int32_t curTime = millis();
  switch(State)
  {
  case WAIT: 
      if (curTime - LastCheckNewDevicesTime >= 8000)
      {
        State = CHECK_NEW_DEVICES;
        LastCheckNewDevicesTime = curTime;
      }
      else if (curTime - LastAcquireDataTime >= 6000)
      {
        State = REQUEST_DATA;
        LastAcquireDataTime = curTime;
      }
    break;
  case CHECK_NEW_DEVICES: 
    Serial.println("Checking for new devices...");
    sendPreMessage(1, 0, UNASSIGNED);
    
    Response->set(0, 0);
    
    telegram.u8id = UNASSIGNED; // slave address
    telegram.u8fct = 3; // function code (this one is registers read)
    telegram.u16RegAdd = 1; // start address in slave
    telegram.u16CoilsNo = MAX_DEVICES_PER_SLAVE * 5; // number of elements (coils or registers) to read
    telegram.au16reg = Response->getArray(); // pointer to a memory array in the Arduino

    master.setTimeOut(50);
    master.query( telegram ); // send query (only once)
    State = CHECK_NEW_DEVICES_WAIT;
    break;
  case CHECK_NEW_DEVICES_WAIT:
    master.poll(); // check incoming messages
    if (master.getState() == COM_IDLE)
    {
      if (Response->get(0) == 0)
      {
        if (master.getTimeOutState())
        {
          Serial.println("No new devices found.");
          State = WAIT;
        }
      }
      else
      {
        bool found = FindDeviceDetails(Response->getArray<uint8_t>() + 1, &devType, &newID);
        if (!found)
        {
          newID = FindFreeSlaveID();
          if (newID == 0)
          {
            SlaveIDsExhausted = true;
            Serial.println("All available slave IDs have been assigned, not accepting new slaves.");
            State = WAIT;
            break;
          }
        }
        int numDevices = 0;
        while (numDevices < MAX_DEVICES_PER_SLAVE && Response->get<uint8_t>(numDevices * 10) != 0)
          numDevices++;
        Transmission->set<uint8_t>(0, 0);
        Transmission->set<uint8_t>(1, newID);
        Serial.print("Found unassigned slave with ");
        Serial.print(numDevices);
        Serial.println(" devices.");
        if (found)
        {
          Serial.print("This device is already associated with SlaveID #");
          Serial.println(newID);
        }
        for (int i = 0; i < numDevices; i++)
        {
          DevNameList[i] = Response->getArray<uint8_t>() + i * 10 + 1;
          uint8_t curDevType = Response->get<uint8_t>(i * 10);
          Serial.print("Device type: ");
          Serial.println(curDevType);
          Serial.print("Device name: ");
          for (int j = 0; j < 8; j++)
            Serial.write(Response->get<char>(i * 10 + 1 + j));
          Serial.println("");
          if (!FindDeviceDetails(DevNameList[i], &devType, &devSlaveID))
            AddToDeviceDirectory(DevNameList[i], curDevType, newID);
          else if (devType != curDevType || devSlaveID != newID)
          {
            UpdateItemInDeviceDirectory(DevNameList[i], curDevType, newID);
            Serial.print("Updated device to have type ");
            Serial.print(curDevType);
            Serial.print(" and slaveID ");
            Serial.println(newID);
          }
        }
        int numDeleted = DeleteDevicesForSlaveNotInList(DevNameList, numDevices, newID);
        if (numDeleted > 0)
        {
          SlaveIDsExhausted = (FindFreeSlaveID() == 0);
          Serial.print("Deleted devices no longer associated with slave: ");
          Serial.println(numDeleted);
        }
        State = ASSIGN_NEW_DEVICE;
      }
    }
    break;
  case ASSIGN_NEW_DEVICE:
    Serial.println("Responding...");
    telegram.u8id = UNASSIGNED; // slave address
    telegram.u8fct = 16; // function code (this one is registers write)
    telegram.u16RegAdd = 1; // start address in slave
    telegram.u16CoilsNo = 1; // number of elements (coils or registers) to write
    telegram.au16reg = Transmission->getArray(); // pointer to a memory array in the Arduino
    
    master.setTimeOut(200);
    master.query( telegram ); // send query (only once)
    State = ASSIGN_NEW_DEVICE_WAIT;
    break;
  case ASSIGN_NEW_DEVICE_WAIT:
    master.poll(); // check incoming messages
    if (master.getState() == COM_IDLE)
    {
      State = WAIT;
      Serial.print("Attempted to assign slave # ");
      Serial.println(Transmission->get<uint8_t>(1));
    }
    break;
  case REQUEST_DATA: 
    Serial.println("Requesting data...");
    sendPreMessage(2, 4, 0);

    Transmission->set<uint16_t>(0, 0);
    Transmission->set<uint16_t>(1, 1);
    Transmission->set<uint16_t>(2, 1);
    Transmission->set<uint16_t>(3, 3);
    
    telegram.u8id = 0; // slave address
    telegram.u8fct = 16; // function code (this one is registers write)
    telegram.u16RegAdd = 0; // start address in slave
    telegram.u16CoilsNo = 4; // number of elements (coils or registers) to write
    telegram.au16reg = Transmission->getArray(); // pointer to a memory array in the Arduino
    
    master.setTimeOut(200);
    master.query( telegram ); // send query (only once)
    State = REQUEST_DATA_WAIT;
    break;
  case REQUEST_DATA_WAIT: 
    master.poll(); // check incoming messages
    if (master.getState() == COM_IDLE)
    {
      State = WAIT;
      Serial.print("Requested data.");
    }
    break;
  }
  
  delay(1);
}

