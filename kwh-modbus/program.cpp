#include "stdio.h"
#include "iostream"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdint>
#include <cstddef>
#include "libraries/modbusSlave/modbusmemory.h"
#include "libraries/modbusSlave/modbus.h"

using namespace std;

int main()
{
	Modbus *mb = new ModbusMemory<Modbus>();
}