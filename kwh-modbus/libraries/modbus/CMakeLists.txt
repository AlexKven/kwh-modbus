set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNO_ARDUINO")
add_library(Modbus modbus.cpp)
add_library(ModbusArray modbusarray.cpp)
add_library(ModbusSerial modbusserial.h)