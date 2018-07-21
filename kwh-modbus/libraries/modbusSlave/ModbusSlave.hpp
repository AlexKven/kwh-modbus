#include "../modbus/ModbusSerial.hpp"

template<typename TSerial, typename TSystemFunctions, typename TBase>
class ModbusSlave : public ModbusSerial<TSerial, TSystemFunctions, TBase>
{
private:
	byte  _slaveId;
};