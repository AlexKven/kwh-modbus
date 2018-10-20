## Device Type Specification

KWH Modbus Devices are individual worker units that communicate with the master mediated by the slave logic, and communicate with other devices mediated by the master and slave logic. The device type is a word (16 bits). From the most significant bits to the least significant bits, the following specifies how device types are allocated:

| Bits (from MSB) | Description            |
| --------------- | ---------------------- |
| 0 & 1           | Data flow type         |
| 2               | Data accumulation type |
| 3 - 5           | Data timescale         |
| 6-15            | TBD                    |

| Data flow type | Description                  |
| -------------- | ---------------------------- |
| 00             | No data flow (commands only) |
| 01             | Data collector               |
| 10             | Data receiver                |
| 11             | Diagnostic data receiver     |

**Data accumulation type:** 0 = instantaneous (like temperature), 1 = accumulate (like energy usage)