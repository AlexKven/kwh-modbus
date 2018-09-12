## Slave Register Specification ##
Each Modbus slave shall have a specific arrangement of register values visible to the master. The master will communicate to the slave by writing to these registers, and the slave will respond by modifying these registers internally, and making them available for the master to read. The Master can **actively** communicate to the slave, while the slave can only **passively** communicate to the master.

An active slave can have any slave ID greater than 1. Slave id 1 is reserved for new slaves, and is periodically polled for new slaves that are connected. Upon being connected, a new slaved will be assigned a new slave ID by the master, and will assume that new ID. No slave can have an ID of 0 because ID 0 is reserved for sending broadcasts to all slaves.

# Slave States #
Slaves will have the first register always be the current state of the slave. Below are tables of register values for each slave state.

| Register #0 Value	| Description	| Notes	|
|-------------------|---------------|-------|
| 0 | Slave is idle | Master can initiate any request |
| 1 | Slave just received a request from master | Master assigns the register to 1 to signify a request, and the slave looks for a 1 after every poll |
| 2 | Master is reading device info ||
| 3 | Master is reading data from a device ||
| 4 | Master is writing data to a device ||
| 5 | Master is reading command from a device ||
| 6 | Master is sending command to a device ||
| 7 | Master is reading message from a device ||
| 8 | Master is reading message from slave ||

### 0: Slave is Idle ###

| Register #	| Value			| Range			| Notes			|
|---------------|---------------|---------------|---------------|
| 1 |KWH Modbus major version number | 0-255 ||
| 1.5 |KWH Modbus minor version number | 0-255 ||
| 2 | Number of devices | 1-65535 | Realistically 1-10ish |
| 3 | Length of device names | 1-65535 | Reastically 4-12ish|
| 4 | Number of commands pending from devices ||
| 5 | Number of messages pending from devices ||
| 6 | Number of messages pending from slave ||

### 1: Slave Received Request from Master ###

| Register #	| Value			| Range			| Notes			|
|---------------|---------------|---------------|---------------|
| 1 | Request type | See table below | | 
| 2 | Device Number | 1-65535 | Limited by number of devices on the slave |
| 3-max | Data | Different for each request type | The makeup of the data varies by request type |

Here is a listing of each request type:

* 0: Make slave idle/abort request
	* Sets slave back to idle state, in the case of a botched request or any other instance in which a master may want to ensure the slave is idle.
* 1: Assign/reassign slave
	* This request is unique in that register 2 is not a device number, since this only deals with the slave.
	* Register 2 is the new slave ID that is to be assigned.
	* Frequently used to assign an inactive slave at address 0 to a proper address.
* 2: Read device info
	* This is for reading information about a device, like the name and type.
	* The response to this will have a state of 2.
	* The data in this respons will have the following format:
		* 0: Device type
		* 1: Data type (first 8 bits)
		* 1.5: Data length in bytes (last 8 bits)
		* 2: Number of commands waiting
		* 3: Number of messages waiting
		* 4 to 3.5+(L/2): Characters of the device name, where L is the length of the name as defined by the slave
* 3: Read device data
	* This is for reading actual data from a device, such as a power reading value at a particular time.
	* The response to this will have a state of 2.
	* The data in this response will have the following format:
		* 0 to 1: The time for the start of the request
		* 2: The number of data points being requested
		* 3 to N: TBD
* 4: Write data
	* This is for writing data from *any* device, including itself (if supported).
	* The response to this will have a state of 3.
	* The data in this 