#include "stdio.h"
#include "iostream"
#include "Libraries/modbus-slave/ModbusSerial/ModbusSerial.h"

using namespace std;

void delayMicrosecond(unsigned long length)
{

}

void pinMode(unsigned char pin, unsigned char mode) {}

class MockSerial
{
public:
	void begin(int baud)
	{

	}

	bool operator!() 
	{
		return true;
	}
};

int main()
{
	Modbus *mb = new Modbus();
	cout << "Hello World!" << endl;
	int test;
	cin >> test;

	ModbusSerial<MockSerial, pinMode, delayMicrosecond, pinMode> modbus;
	modbus.config(new MockSerial(), 5, 5);
}