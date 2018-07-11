cmake_minimum_required (VERSION 2.6)
project (kwh-modbus CXX)
include_directories ("${PROJECT_SOURCE_DIR}/Libraries/Modbus")
# add_subdirectory (Libraries/Modbus)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNO_ARDUINO")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/linuxOutputs/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/linuxOutputs/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/linuxOutputs/bin)
 
# add the executable
add_executable (kwh-modbus program.cpp)
target_link_libraries (kwh-modbus Modbus)
target_link_libraries (kwh-modbus ModbusSerial)
target_link_libraries (kwh-modbus ModbusArray)
SET_TARGET_PROPERTIES(ModbusSerial PROPERTIES LINKER_LANGUAGE CXX)