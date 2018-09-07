## Slave Register Specification ##
Each Modbus slave shall have a specific arrangement of register values visible to the master. The master will communicate to the slave by writing to these registers, and the slave will respond by modifying these registers internally, and making them available for the master to read. The Master can **actively** communicate to the slave, while the slave can only **passively** communicate to the master.

An active slave can have any slave ID greater than 0. Slave id 0 is reserved for new slaves, and is periodically polled for new slaves that are connected. Upon being connected, a new slaved will be assigned a new slave ID by the master, and will assume that new ID.

An inactive slave will have the following registers:

| Register #	|Value			|Notes			|
|---------------|---------------|---------------|
| 0 |KWH Modbus major version number ||
| 0.5 |KWH Modbus minor version number ||
