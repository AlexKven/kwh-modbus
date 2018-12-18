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
| 4 | Master is preparing to write data to a device ||
| 6 | Master is reading command from a device ||
| 7 | Master is sending command to a device ||
| 8 | Master is reading message from a device ||
| 9 | Master is reading message from slave ||

### 0: Slave is Idle ###

| Register #	| Value			| Range			| Notes			|
|---------------|---------------|---------------|---------------|
| 1 |KWH Modbus major version number | 0-255 ||
| 1.5 |KWH Modbus minor version number | 0-255 ||
| 2 | Number of devices | 1-65535 | Realistically 1-10ish |
| 3 | Length of device names | 1-65535 | Reastically 4-12ish|
| 4 | Number of Modbus holding registers on slave | up to 65535 | |
| 5 | Number of commands pending from devices |||
| 6 | Number of messages pending from devices |||
| 7 | Number of messages pending from slave |||

### 1: Slave Received Request from Master ###

| Register #	| Value			| Range			| Notes			|
|---------------|---------------|---------------|---------------|
| 1 | Request type | See table below | |
| 2 | Device Number | 1-65535 | Limited by number of devices on the slave |
| 3 to max | Data | Different for each request type | The makeup of the data varies by request type |

### 2: Master is Reading Device Info

| Register #         | Value                  | Range                              | Notes                                                        |
| ------------------ | ---------------------- | ---------------------------------- | ------------------------------------------------------------ |
| 1                  | Device Number          | 1-65535                            |                                                              |
| 2                  | Device Type            | TBD                                |                                                              |
| 3 to 2 .5+(L/2)    | Device Name            | L bytes, each half a register long | L is length of name. Entire name is next whole number of registers |
| 3+(L/2) to 4+(L/2) | \# of Commands Pending | 0-65535                            | Use next whole number for L/2                                |
| 4+(L/2) to 5+(L/2) |                        |                                    |                                                              |

### 3: Master is reading data from a device

| Register # | Value                     | Range         | Notes                                                        |
| ---------- | ------------------------- | ------------- | ------------------------------------------------------------ |
| 1          | Request Status            | 0 to 2        | 0 = success, 1 = error: time not set, 2 = device doesn't send data |
| 2 to 3     | Data Start Time           | 0 to 2^32 - 1 | Applies to this page only and not necessarily the same as the request that the slave received. |
| 4          | Num Data Points this page | 0 to 255      | Number of data points in this page                           |
| 4.5        | data point size (bits)    | 0 to 63       |                                                              |
| 5          | Current page              | 0 to 255      |                                                              |
| 5.5        | Num Remaining Pages       | 0 to 255      |                                                              |
| 6 to X     | Data Points               | Anything      | Binary data containing sent data points, each point composed of an integer number of *bits* according to the device type |

### 4: Master is preparing to write data to device

| Register # | Value                         | Range    | Notes                                                        |
| ---------- | ----------------------------- | -------- | ------------------------------------------------------------ |
| 1          | Status (8 bits)               | 0 to 4   | 0 = success, 1 = not supported, 2 = name is too long, 3 = current time is requested, 4 = failure |
| 1.5        | Data points per page (8 bits) | 0 to 255 | Specifies how many data points the slave will expect to receive per page |

###

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
  * The data in this request will have the following format:
  	* 0: Device type
  	* 1: Data type (first 8 bits)
  	* 1.5: Data length in bytes (last 8 bits)
  	* 2: Number of commands waiting
  	* 3: Number of messages waiting
  	* 4 to 3.5+(L/2): Characters of the device name, where L is the length of the name as defined by the slave
* 3: Read device data
  * This is for reading actual data from a device, such as a power reading value at a particular time.
  * The response to this will have a state of 3.
  * The data in this request will have the following format:
    * 0 to 1: The time for the start of the request
    * 2: The number of data points being requested
    * 3: The page (0-255) requested (0 if first request)
    * 3.5: Max data points requested (0 if we don't request a max). Data points beyond this is guaranteed to be paginated by the slave
* 4: Prepare to write data
  * This is for writing data from *any* device, including itself (if supported).
  * The response to this will have a state of 4.
  * The data in this request will have the following format:
    * 0: Length of name of originating device (L)
    * 1 to 2: Start time
    * 3: Data point size (8 bits)
    * 3.5: Data point timescale (8 bits)
    * 4: Data points count
    * 5 to end: Name
* 5: Write data
  * This is to send actual data to the receiving slave.
  * You must call `4: Prepare to write data` before calling `5: Write data`.
  * This response will have a state of 0.
  * Slave will automatically calculate offset from page number, according to the value returned from `4: Prepare to write data`.
  * The data in this request will have the following format:
    * 0: Number of data points in this page (8 bits)
    * 0.5: Data point size (8 bits)
    * 1: Data point timescale
    * 1.5: Page number
    * 2 to end: Data
* 32770 (0x8002): Broadcasts time in unsigned Y2K epoch time that is used by Arduino time library
  * Goes to all slaves at once
  * Overflows in 2136
  * Applies to slave directly and not devices (though all devices have access to the time)
  * The data (beginning at 2 for broadcasts) contains the following:
    * 0 to 1: The four bytes in a unsigned 32 bit integer denoting the number of seconds since Jan 1, 2000