g++ mock/mockserialstream.cpp -D "NO_ARDUINO" -c
g++ linuxPrograms/modbusmaster.cpp libraries/modbusMaster/modbusmaster.h -D "NO_ARDUINO" -o linuxOutputs/modbusMaster.out
g++ linuxPrograms/modbusslave.cpp mock/mockserialstream.cpp noArduino/systemfunctions.cpp -D "NO_ARDUINO" -fpermissive -o linuxOutputs/modbusSlave.out
