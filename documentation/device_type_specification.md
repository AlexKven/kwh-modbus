## Device Type Specification

KWH Modbus Devices are individual worker units that communicate with the master mediated by the slave logic, and communicate with other devices mediated by the master and slave logic. The device type is a word (16 bits). From the most significant bits to the least significant bits, the following specifies how device types are allocated:

| Bits (from MSB) | Description                                  |
| --------------- | -------------------------------------------- |
| 0 & 1           | Data flow type                               |
| 2               | Data accumulation type (data collector only) |
| 3 - 5           | Data timescale (data collector only)         |
| 6-11            | Data size (in bits) (data collector only)    |
| 12-15           | TBD                                          |

| Data flow type | Description                                       |
| -------------- | ------------------------------------------------- |
| 00             | No data flow (commands only) / Other device types |
| 01             | Data collector                                    |
| 10             | Data transmitter                                  |
| 11             | Diagnostic data receiver                          |

**Data accumulation type:** 0 = instantaneous (like temperature), 1 = accumulate (like energy usage)

| Data timescale code | Period of data   |
| ------------------- | ---------------- |
| 000                 | 250 milliseconds |
| 001                 | 1 second         |
| 010                 | 15 seconds       |
| 011                 | 1 minute         |
| 100                 | 10 minutes       |
| 101                 | 30 minutes       |
| 110                 | 1 hour           |
| 111                 | 1 day            |

## Optional Statistics

The following statistics may be obtained from the slave (if supported) depending on the accumulation type:

| **Index** | **Metric for Instantaneous** | **Metric for Accumulate** |
| --------- | ---------------------------- | ------------------------- |
| 0         | Mean/period                  | Total (Integral)/period   |
| 1         | Standard deviation/period    | Standard deviation/period |
| 2         | Lifetime minimum             | Lifetime minimum          |
| 3         | Lifetime maximum             | Lifetime maximum          |
| 4         | Minimum mean                 | Minimum total             |
| 5         | Maximum mean                 | Maximum total             |

## Other Device Types

The following statistics may be obtained from the slave (if supported) depending on the accumulation type:

| **Device Type**  | **Description**                       |
| ---------------- | ------------------------------------- |
| 0000000000000001 | Time server (master can request time) |

