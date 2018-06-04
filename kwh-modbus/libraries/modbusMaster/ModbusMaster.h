/**
@file
Arduino library for communicating with Modbus slaves over RS232/485 (via RTU protocol).

@defgroup setup ModbusMaster Object Instantiation/Initialization
@defgroup buffer ModbusMaster Buffer Management
@defgroup discrete Modbus Function Codes for Discrete Coils/Inputs
@defgroup register Modbus Function Codes for Holding/Input Registers
@defgroup constant Modbus Function Codes, Exception Codes
*/
/*

ModbusMaster.h - Arduino library for communicating with Modbus slaves
over RS232/485 (via RTU protocol).

Library:: ModbusMaster

Copyright:: 2009-2016 Doc Walker

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/
#pragma once

#ifndef ModbusMaster_h
#define ModbusMaster_h

#ifdef NO_ARDUINO
#include "../../mock/arduinoFunctions.h"
#include "../../mock/arduinoMacros.h"
#include <cstdint>
#include <cstddef>
#endif

#include "crc16.h"
#include "word.h"
/**
@def __MODBUSMASTER_DEBUG__ (0)
Set to 1 to enable debugging features within class:
- PIN A cycles for each byte read in the Modbus response
- PIN B cycles for each millisecond timeout during the Modbus response
*/
#define __MODBUSMASTER_DEBUG__ (0)
#define __MODBUSMASTER_DEBUG_PIN_A__ 4
#define __MODBUSMASTER_DEBUG_PIN_B__ 5



/* _____STANDARD INCLUDES____________________________________________________ */
// include types & constants of Wiring core API


/* _____UTILITY MACROS_______________________________________________________ */


/* _____PROJECT INCLUDES_____________________________________________________ */
// functions to calculate Modbus Application Data Unit CRC


// functions to manipulate words



/* _____CLASS DEFINITIONS____________________________________________________ */
/**
Arduino class library for communicating with Modbus slaves over
RS232/485 (via RTU protocol).
*/
template<typename TSerial, typename TArduinoFunctions>
class ModbusMaster
{
public:
	ModbusMaster();

	void begin(uint8_t slave, TSerial *serial)
	{
		//  txBuffer = (uint16_t*) calloc(ku8MaxBufferSize, sizeof(uint16_t));
		_u8MBSlave = slave;
		_serial = serial;
		_u8TransmitBufferIndex = 0;
		u16TransmitBufferLength = 0;

//#if __MODBUSMASTER_DEBUG__
//		pinMode(__MODBUSMASTER_DEBUG_PIN_A__, OUTPUT);
//		pinMode(__MODBUSMASTER_DEBUG_PIN_B__, OUTPUT);
//#endif
	}

	inline void idle(void(*idle)())
	{
		_idle = idle;
	}

	inline void preTransmission(void(*preTransmission)())
	{
		_preTransmission = preTransmission;
	}

	inline void postTransmission(void(*postTransmission)())
	{
		_postTransmission = postTransmission;
	}

	// Modbus exception codes
	/**
	Modbus protocol illegal function exception.

	The function code received in the query is not an allowable action for
	the server (or slave). This may be because the function code is only
	applicable to newer devices, and was not implemented in the unit
	selected. It could also indicate that the server (or slave) is in the
	wrong state to process a request of this type, for example because it is
	unconfigured and is being asked to return register values.

	@ingroup constant
	*/
	static const uint8_t ku8MBIllegalFunction = 0x01;

	/**
	Modbus protocol illegal data address exception.

	The data address received in the query is not an allowable address for
	the server (or slave). More specifically, the combination of reference
	number and transfer length is invalid. For a controller with 100
	registers, the ADU addresses the first register as 0, and the last one
	as 99. If a request is submitted with a starting register address of 96
	and a quantity of registers of 4, then this request will successfully
	operate (address-wise at least) on registers 96, 97, 98, 99. If a
	request is submitted with a starting register address of 96 and a
	quantity of registers of 5, then this request will fail with Exception
	Code 0x02 "Illegal Data Address" since it attempts to operate on
	registers 96, 97, 98, 99 and 100, and there is no register with address
	100.

	@ingroup constant
	*/
	static const uint8_t ku8MBIllegalDataAddress = 0x02;

	/**
	Modbus protocol illegal data value exception.

	A value contained in the query data field is not an allowable value for
	server (or slave). This indicates a fault in the structure of the
	remainder of a complex request, such as that the implied length is
	incorrect. It specifically does NOT mean that a data item submitted for
	storage in a register has a value outside the expectation of the
	application program, since the MODBUS protocol is unaware of the
	significance of any particular value of any particular register.

	@ingroup constant
	*/
	static const uint8_t ku8MBIllegalDataValue = 0x03;

	/**
	Modbus protocol slave device failure exception.

	An unrecoverable error occurred while the server (or slave) was
	attempting to perform the requested action.

	@ingroup constant
	*/
	static const uint8_t ku8MBSlaveDeviceFailure = 0x04;

	// Class-defined success/exception codes
	/**
	ModbusMaster success.

	Modbus transaction was successful; the following checks were valid:
	- slave ID
	- function code
	- response code
	- data
	- CRC

	@ingroup constant
	*/
	static const uint8_t ku8MBSuccess = 0x00;

	/**
	ModbusMaster invalid response slave ID exception.

	The slave ID in the response does not match that of the request.

	@ingroup constant
	*/
	static const uint8_t ku8MBInvalidSlaveID = 0xE0;

	/**
	ModbusMaster invalid response function exception.

	The function code in the response does not match that of the request.

	@ingroup constant
	*/
	static const uint8_t ku8MBInvalidFunction = 0xE1;

	/**
	ModbusMaster response timed out exception.

	The entire response was not received within the timeout period,
	ModbusMaster::ku8MBResponseTimeout.

	@ingroup constant
	*/
	static const uint8_t ku8MBResponseTimedOut = 0xE2;

	/**
	ModbusMaster invalid response CRC exception.

	The CRC in the response does not match the one calculated.

	@ingroup constant
	*/
	static const uint8_t ku8MBInvalidCRC = 0xE3;

	inline uint16_t getResponseBuffer(uint8_t u8Index)
	{
		if (u8Index < ku8MaxBufferSize)
		{
			return _u16ResponseBuffer[u8Index];
		}
		else
		{
			return 0xFFFF;
		}
	}

	inline void clearResponseBuffer()
	{
		uint8_t i;

		for (i = 0; i < ku8MaxBufferSize; i++)
		{
			_u16ResponseBuffer[i] = 0;
		}
	}

	inline uint8_t  setTransmitBuffer(uint8_t u8Index, uint16_t u16Value)
	{
		if (u8Index < ku8MaxBufferSize)
		{
			_u16TransmitBuffer[u8Index] = u16Value;
			return ku8MBSuccess;
		}
		else
		{
			return ku8MBIllegalDataAddress;
		}
	}

	inline void clearTransmitBuffer()
	{
		uint8_t i;

		for (i = 0; i < ku8MaxBufferSize; i++)
		{
			_u16TransmitBuffer[i] = 0;
		}
	}

	//void beginTransmission(uint16_t);
	//uint8_t requestFrom(uint16_t, uint16_t);
	//void sendBit(bool);
	//void send(uint8_t);
	//void send(uint16_t);
	//void send(uint32_t);
	//uint8_t available(void);
	//uint16_t receive(void);


	inline uint8_t  readCoils(uint16_t u16ReadAddress, uint16_t u16BitQty)
	{
		_u16ReadAddress = u16ReadAddress;
		_u16ReadQty = u16BitQty;
		return ModbusMasterTransaction(ku8MBReadCoils);
	}

	inline uint8_t  readDiscreteInputs(uint16_t u16ReadAddress,
		uint16_t u16BitQty)
	{
		_u16ReadAddress = u16ReadAddress;
		_u16ReadQty = u16BitQty;
		return ModbusMasterTransaction(ku8MBReadDiscreteInputs);
	}

	inline uint8_t  readHoldingRegisters(uint16_t u16ReadAddress,
		uint16_t u16ReadQty)
	{
		_u16ReadAddress = u16ReadAddress;
		_u16ReadQty = u16ReadQty;
		return ModbusMasterTransaction(ku8MBReadHoldingRegisters);
	}

	inline uint8_t  readInputRegisters(uint16_t u16ReadAddress,
		uint8_t u16ReadQty)
	{
		_u16ReadAddress = u16ReadAddress;
		_u16ReadQty = u16ReadQty;
		return ModbusMasterTransaction(ku8MBReadInputRegisters);
	}

	inline uint8_t  writeSingleCoil(uint16_t u16WriteAddress, uint8_t u8State)
	{
		_u16WriteAddress = u16WriteAddress;
		_u16WriteQty = (u8State ? 0xFF00 : 0x0000);
		return ModbusMasterTransaction(ku8MBWriteSingleCoil);
	}

	inline uint8_t  writeSingleRegister(uint16_t u16WriteAddress,
		uint16_t u16WriteValue)
	{
		_u16WriteAddress = u16WriteAddress;
		_u16WriteQty = 0;
		_u16TransmitBuffer[0] = u16WriteValue;
		return ModbusMasterTransaction(ku8MBWriteSingleRegister);
	}

	inline uint8_t  writeMultipleCoils(uint16_t u16WriteAddress,
		uint16_t u16BitQty)
	{
		_u16WriteAddress = u16WriteAddress;
		_u16WriteQty = u16BitQty;
		return ModbusMasterTransaction(ku8MBWriteMultipleCoils);
	}

	inline uint8_t  writeMultipleCoils()
	{
		_u16WriteQty = u16TransmitBufferLength;
		return ModbusMasterTransaction(ku8MBWriteMultipleCoils);
	}

	inline uint8_t  writeMultipleRegisters(uint16_t u16WriteAddress,
		uint16_t u16WriteQty)
	{
		_u16WriteAddress = u16WriteAddress;
		_u16WriteQty = u16WriteQty;
		return ModbusMasterTransaction(ku8MBWriteMultipleRegisters);
	}

	inline uint8_t  writeMultipleRegisters()
	{
		_u16WriteQty = _u8TransmitBufferIndex;
		return ModbusMasterTransaction(ku8MBWriteMultipleRegisters);
	}

	inline uint8_t  maskWriteRegister(uint16_t u16WriteAddress,
		uint16_t u16AndMask, uint16_t u16OrMask)
	{
		_u16WriteAddress = u16WriteAddress;
		_u16TransmitBuffer[0] = u16AndMask;
		_u16TransmitBuffer[1] = u16OrMask;
		return ModbusMasterTransaction(ku8MBMaskWriteRegister);
	}

	inline uint8_t  readWriteMultipleRegisters(uint16_t u16ReadAddress,
		uint16_t u16ReadQty, uint16_t u16WriteAddress, uint16_t u16WriteQty)
	{
		_u16ReadAddress = u16ReadAddress;
		_u16ReadQty = u16ReadQty;
		_u16WriteAddress = u16WriteAddress;
		_u16WriteQty = u16WriteQty;
		return ModbusMasterTransaction(ku8MBReadWriteMultipleRegisters);
	}

	inline uint8_t  readWriteMultipleRegisters(uint16_t u16ReadAddress,
		uint16_t u16ReadQty)
	{
		_u16ReadAddress = u16ReadAddress;
		_u16ReadQty = u16ReadQty;
		_u16WriteQty = _u8TransmitBufferIndex;
		return ModbusMasterTransaction(ku8MBReadWriteMultipleRegisters);
	}

private:
	TSerial * _serial;                                             ///< reference to serial port object
	uint8_t  _u8MBSlave;                                         ///< Modbus slave (1..255) initialized in begin()
	static const uint8_t ku8MaxBufferSize = 64;   ///< size of response/transmit buffers    
	uint16_t _u16ReadAddress;                                    ///< slave register from which to read
	uint16_t _u16ReadQty;                                        ///< quantity of words to read
	uint16_t _u16ResponseBuffer[ku8MaxBufferSize];               ///< buffer to store Modbus slave response; read via GetResponseBuffer()
	uint16_t _u16WriteAddress;                                   ///< slave register to which to write
	uint16_t _u16WriteQty;                                       ///< quantity of words to write
	uint16_t _u16TransmitBuffer[ku8MaxBufferSize];               ///< buffer containing data to transmit to Modbus slave; set via SetTransmitBuffer()
	uint16_t* txBuffer; // from Wire.h -- need to clean this up Rx
	uint8_t _u8TransmitBufferIndex;
	uint16_t u16TransmitBufferLength;
	uint16_t* rxBuffer; // from Wire.h -- need to clean this up Rx
	uint8_t _u8ResponseBufferIndex;
	uint8_t _u8ResponseBufferLength;

	// Modbus function codes for bit access
	static const uint8_t ku8MBReadCoils = 0x01; ///< Modbus function 0x01 Read Coils
	static const uint8_t ku8MBReadDiscreteInputs = 0x02; ///< Modbus function 0x02 Read Discrete Inputs
	static const uint8_t ku8MBWriteSingleCoil = 0x05; ///< Modbus function 0x05 Write Single Coil
	static const uint8_t ku8MBWriteMultipleCoils = 0x0F; ///< Modbus function 0x0F Write Multiple Coils

														 // Modbus function codes for 16 bit access
	static const uint8_t ku8MBReadHoldingRegisters = 0x03; ///< Modbus function 0x03 Read Holding Registers
	static const uint8_t ku8MBReadInputRegisters = 0x04; ///< Modbus function 0x04 Read Input Registers
	static const uint8_t ku8MBWriteSingleRegister = 0x06; ///< Modbus function 0x06 Write Single Register
	static const uint8_t ku8MBWriteMultipleRegisters = 0x10; ///< Modbus function 0x10 Write Multiple Registers
	static const uint8_t ku8MBMaskWriteRegister = 0x16; ///< Modbus function 0x16 Mask Write Register
	static const uint8_t ku8MBReadWriteMultipleRegisters = 0x17; ///< Modbus function 0x17 Read Write Multiple Registers

																 // Modbus timeout [milliseconds]
	static const uint16_t ku16MBResponseTimeout = 2000; ///< Modbus timeout [milliseconds]

														// master function that conducts Modbus transactions
	inline uint8_t ModbusMasterTransaction(uint8_t u8MBFunction)
	{
		uint8_t u8ModbusADU[256];
		uint8_t u8ModbusADUSize = 0;
		uint8_t i, u8Qty;
		uint16_t u16CRC;
		uint32_t u32StartTime;
		uint8_t u8BytesLeft = 8;
		uint8_t u8MBStatus = ku8MBSuccess;

		// assemble Modbus Request Application Data Unit
		u8ModbusADU[u8ModbusADUSize++] = _u8MBSlave;
		u8ModbusADU[u8ModbusADUSize++] = u8MBFunction;

		switch (u8MBFunction)
		{
		case ku8MBReadCoils:
		case ku8MBReadDiscreteInputs:
		case ku8MBReadInputRegisters:
		case ku8MBReadHoldingRegisters:
		case ku8MBReadWriteMultipleRegisters:
			u8ModbusADU[u8ModbusADUSize++] = highByte(_u16ReadAddress);
			u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16ReadAddress);
			u8ModbusADU[u8ModbusADUSize++] = highByte(_u16ReadQty);
			u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16ReadQty);
			break;
		}

		switch (u8MBFunction)
		{
		case ku8MBWriteSingleCoil:
		case ku8MBMaskWriteRegister:
		case ku8MBWriteMultipleCoils:
		case ku8MBWriteSingleRegister:
		case ku8MBWriteMultipleRegisters:
		case ku8MBReadWriteMultipleRegisters:
			u8ModbusADU[u8ModbusADUSize++] = highByte(_u16WriteAddress);
			u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16WriteAddress);
			break;
		}

		switch (u8MBFunction)
		{
		case ku8MBWriteSingleCoil:
			u8ModbusADU[u8ModbusADUSize++] = highByte(_u16WriteQty);
			u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16WriteQty);
			break;

		case ku8MBWriteSingleRegister:
			u8ModbusADU[u8ModbusADUSize++] = highByte(_u16TransmitBuffer[0]);
			u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16TransmitBuffer[0]);
			break;

		case ku8MBWriteMultipleCoils:
			u8ModbusADU[u8ModbusADUSize++] = highByte(_u16WriteQty);
			u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16WriteQty);
			u8Qty = (_u16WriteQty % 8) ? ((_u16WriteQty >> 3) + 1) : (_u16WriteQty >> 3);
			u8ModbusADU[u8ModbusADUSize++] = u8Qty;
			for (i = 0; i < u8Qty; i++)
			{
				switch (i % 2)
				{
				case 0: // i is even
					u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16TransmitBuffer[i >> 1]);
					break;

				case 1: // i is odd
					u8ModbusADU[u8ModbusADUSize++] = highByte(_u16TransmitBuffer[i >> 1]);
					break;
				}
			}
			break;

		case ku8MBWriteMultipleRegisters:
		case ku8MBReadWriteMultipleRegisters:
			u8ModbusADU[u8ModbusADUSize++] = highByte(_u16WriteQty);
			u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16WriteQty);
			u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16WriteQty << 1);

			for (i = 0; i < lowByte(_u16WriteQty); i++)
			{
				u8ModbusADU[u8ModbusADUSize++] = highByte(_u16TransmitBuffer[i]);
				u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16TransmitBuffer[i]);
			}
			break;

		case ku8MBMaskWriteRegister:
			u8ModbusADU[u8ModbusADUSize++] = highByte(_u16TransmitBuffer[0]);
			u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16TransmitBuffer[0]);
			u8ModbusADU[u8ModbusADUSize++] = highByte(_u16TransmitBuffer[1]);
			u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16TransmitBuffer[1]);
			break;
		}

		// append CRC
		u16CRC = 0xFFFF;
		for (i = 0; i < u8ModbusADUSize; i++)
		{
			u16CRC = crc16_update(u16CRC, u8ModbusADU[i]);
		}
		u8ModbusADU[u8ModbusADUSize++] = lowByte(u16CRC);
		u8ModbusADU[u8ModbusADUSize++] = highByte(u16CRC);
		u8ModbusADU[u8ModbusADUSize] = 0;

		// flush receive buffer before transmitting request
		while (_serial->read() != -1);

		// transmit request
		if (_preTransmission)
		{
			_preTransmission();
		}
		for (i = 0; i < u8ModbusADUSize; i++)
		{
			_serial->write(u8ModbusADU[i]);
		}

		u8ModbusADUSize = 0;
		_serial->flush();    // flush transmit buffer
		if (_postTransmission)
		{
			_postTransmission();
		}

		// loop until we run out of time or bytes, or an error occurs
		u32StartTime = TArduinoFunctions::Millis();
		while (u8BytesLeft && !u8MBStatus)
		{
			if (_serial->available())
			{
#if __MODBUSMASTER_DEBUG__
				digitalWrite(__MODBUSMASTER_DEBUG_PIN_A__, true);
#endif
				u8ModbusADU[u8ModbusADUSize++] = _serial->read();
				u8BytesLeft--;
#if __MODBUSMASTER_DEBUG__
				digitalWrite(__MODBUSMASTER_DEBUG_PIN_A__, false);
#endif
			}
			else
			{
#if __MODBUSMASTER_DEBUG__
				digitalWrite(__MODBUSMASTER_DEBUG_PIN_B__, true);
#endif
				if (_idle)
				{
					_idle();
				}
#if __MODBUSMASTER_DEBUG__
				digitalWrite(__MODBUSMASTER_DEBUG_PIN_B__, false);
#endif
			}

			// evaluate slave ID, function code once enough bytes have been read
			if (u8ModbusADUSize == 5)
			{
				// verify response is for correct Modbus slave
				if (u8ModbusADU[0] != _u8MBSlave)
				{
					u8MBStatus = ku8MBInvalidSlaveID;
					break;
				}

				// verify response is for correct Modbus function code (mask exception bit 7)
				if ((u8ModbusADU[1] & 0x7F) != u8MBFunction)
				{
					u8MBStatus = ku8MBInvalidFunction;
					break;
				}

				// check whether Modbus exception occurred; return Modbus Exception Code
				if (bitRead((unsigned char)u8ModbusADU[1], (unsigned char)7))
				{
					u8MBStatus = u8ModbusADU[2];
					break;
				}

				// evaluate returned Modbus function code
				switch (u8ModbusADU[1])
				{
				case ku8MBReadCoils:
				case ku8MBReadDiscreteInputs:
				case ku8MBReadInputRegisters:
				case ku8MBReadHoldingRegisters:
				case ku8MBReadWriteMultipleRegisters:
					u8BytesLeft = u8ModbusADU[2];
					break;

				case ku8MBWriteSingleCoil:
				case ku8MBWriteMultipleCoils:
				case ku8MBWriteSingleRegister:
				case ku8MBWriteMultipleRegisters:
					u8BytesLeft = 3;
					break;

				case ku8MBMaskWriteRegister:
					u8BytesLeft = 5;
					break;
				}
			}
			if ((TArduinoFunctions::Millis() - u32StartTime) > ku16MBResponseTimeout)
			{
				u8MBStatus = ku8MBResponseTimedOut;
			}
		}

		// verify response is large enough to inspect further
		if (!u8MBStatus && u8ModbusADUSize >= 5)
		{
			// calculate CRC
			u16CRC = 0xFFFF;
			for (i = 0; i < (u8ModbusADUSize - 2); i++)
			{
				u16CRC = crc16_update(u16CRC, u8ModbusADU[i]);
			}

			// verify CRC
			if (!u8MBStatus && (lowByte(u16CRC) != u8ModbusADU[u8ModbusADUSize - 2] ||
				highByte(u16CRC) != u8ModbusADU[u8ModbusADUSize - 1]))
			{
				u8MBStatus = ku8MBInvalidCRC;
			}
		}

		// disassemble ADU into words
		if (!u8MBStatus)
		{
			// evaluate returned Modbus function code
			switch (u8ModbusADU[1])
			{
			case ku8MBReadCoils:
			case ku8MBReadDiscreteInputs:
				// load bytes into word; response bytes are ordered L, H, L, H, ...
				for (i = 0; i < (u8ModbusADU[2] >> 1); i++)
				{
					if (i < ku8MaxBufferSize)
					{
						_u16ResponseBuffer[i] = word(u8ModbusADU[2 * i + 4], u8ModbusADU[2 * i + 3]);
					}

					_u8ResponseBufferLength = i;
				}

				// in the event of an odd number of bytes, load last byte into zero-padded word
				if (u8ModbusADU[2] % 2)
				{
					if (i < ku8MaxBufferSize)
					{
						_u16ResponseBuffer[i] = word(0, u8ModbusADU[2 * i + 3]);
					}

					_u8ResponseBufferLength = i + 1;
				}
				break;

			case ku8MBReadInputRegisters:
			case ku8MBReadHoldingRegisters:
			case ku8MBReadWriteMultipleRegisters:
				// load bytes into word; response bytes are ordered H, L, H, L, ...
				for (i = 0; i < (u8ModbusADU[2] >> 1); i++)
				{
					if (i < ku8MaxBufferSize)
					{
						_u16ResponseBuffer[i] = word(u8ModbusADU[2 * i + 3], u8ModbusADU[2 * i + 4]);
					}

					_u8ResponseBufferLength = i;
				}
				break;
			}
		}

		_u8TransmitBufferIndex = 0;
		u16TransmitBufferLength = 0;
		_u8ResponseBufferIndex = 0;
		return u8MBStatus;
	}

	// idle callback function; gets called during idle time between TX and RX
	void(*_idle)();
	// preTransmission callback function; gets called before writing a Modbus message
	void(*_preTransmission)();
	// postTransmission callback function; gets called after a Modbus message has been sent
	void(*_postTransmission)();
};
#endif

/**
@example examples/Basic/Basic.pde
@example examples/PhoenixContact_nanoLC/PhoenixContact_nanoLC.pde
@example examples/RS485_HalfDuplex/RS485_HalfDuplex.ino
*/

template<typename TSerial, typename TArduinoFunctions>
inline ModbusMaster<TSerial, TArduinoFunctions>::ModbusMaster()
{
}
