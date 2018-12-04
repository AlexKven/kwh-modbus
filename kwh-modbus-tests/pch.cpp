//
// pch.cpp
// Include the standard header and generate the precompiled header.
//

#include "pch.h"
#include "../kwh-modbus/libraries/modbus/Modbus.cpp"
#include "../kwh-modbus/libraries/modbus/ModbusArray.cpp"
#include "../kwh-modbus/libraries/random/Random.cpp"
#include "../kwh-modbus/noArduino/ModbusMemory.cpp"
#include "../kwh-modbus/mock/MockSerialStream.cpp"
#include "../kwh-modbus/libraries/timeManager/TimeManager.cpp"
#include "../kwh-modbus/libraries/device/DataCollectorDevice.cpp"
#include "../kwh-modbus/libraries/device/DataTransmitterDevice.cpp"
#include "../kwh-modbus/libraries/device/Device.cpp"