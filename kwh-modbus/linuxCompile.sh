gcc -x c++ linuxPrograms/modbusmaster.cpp libraries/modbusMaster/modbusmaster.h -D "NO_ARDUINO" -o linuxOutputs/modbusMaster.out
gcc -x c++ linuxPrograms/modbusslave.cpp noArduino/arduinofunctions.cpp libraries/modbusSlave/modbus.cpp libraries/modbusSlave/modbusserial.h -D "NO_ARDUINO" -fpermissive -o linuxOutputs/modbusSlave.out
