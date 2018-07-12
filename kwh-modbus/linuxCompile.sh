g++ mock/MockSerialStream.cpp -D "NO_ARDUINO" -c
g++ linuxPrograms/ModbusMaster.cpp libraries/modbusMaster/ModbusMaster.h -D "NO_ARDUINO" -o linuxOutputs/ModbusMaster.out
g++ linuxPrograms/ModbusSlave.cpp libraries/modbus/Modbus.cpp noArduino/ModbusMemory.cpp mock/MockSerialStream.cpp noArduino/SystemFunctions.cpp -D "NO_ARDUINO" -fpermissive -o linuxOutputs/ModbusSlave.out
