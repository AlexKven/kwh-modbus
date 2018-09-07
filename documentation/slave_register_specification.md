## Slave Register Specification ##
Each Modbus slave shall have a specific arrangement of register values visible to the master. The master will communicate to the slave by writing to these registers, and the slave will respond by modifying these registers internally, and making them available for the master to read. The Master can **actively** communicate to the slave, while the slave can only **passively** communicate to the master.

An active slave can have any slave ID greater than 0. Slave id 0 is reserved for new slaves, and is periodically polled for new slaves that are connected. Upon being connected, a new slaved will be assigned a new slave ID by the master, and will assume that new ID.

# Inactive Slaves #
An inactive slave will have the following registers:

| Register #	| Value			| Range			| Notes			|
|---------------|---------------|---------------|---------------|
| 0 |KWH Modbus major version number | 0-255 ||
| 0.5 |KWH Modbus minor version number | 0-255 ||
| 1 | Number of slaves | 1-65535 | Realistically 1-10ish |
| 2 | Length of slave legnth | 1-65535 | Reastically 4-12ish|

# Active Slaves #
Active slaves will have the first register always be the current state of the slave:

| Register #0 Value	| Description	| Notes	|
|-------------------|---------------|-------|
| 0 | Slave is idle | Master can initiate any request |
| 1 | Slave just received a request from master | Master assigns the register to 1 to signify a request, and the slave looks for a 1 after every poll |
| 2 | Master is reading data from a device ||
| 3 | Master is writing data to a device ||
| 4 | Master is reading command from a device ||
| 5 | Master is sending command to a device ||
| 6 | Master is reading message from a device ||

Below are tables of register values for each slave state.

### 0: Slave is Idle ###

| Register #	| Value			| Range			| Notes			|
|---------------|---------------|---------------|---------------|
| 1 |KWH Modbus major version number | 0-255 ||
| 1.5 |KWH Modbus minor version number | 0-255 ||
| 2 | Number of slaves | 1-65535 | Realistically 1-10ish |
| 3 | Length of slave legnth | 1-65535 | Reastically 4-12ish|
| 4 | Number of commands pending from devices ||
| 5 | Number of messages pending from devices ||