# Welcome to KWH Modbus
## What is KWH Modbus?
KWH Modbus is (going to be) a communication protocol built on top of [Modbus](https://en.wikipedia.org/wiki/Modbus), and a multi-platform software implementation of that protocol written primarily in C++. KWH Modbus is designed to facilitate movement of data between different components of a KWH data acquisition system (DAS) over a single Modbus.

KWH Modbus software has the following goals:
* Be small enough to run on Arduino-based microcontrollers, where memory is very limited
* Be useful for any kind of data measurement, such as power usage and temperature readings
* Efficiently move data from multiple data senders (e.g., power meters, temperature sensors) to multiple data receivers (e.g., cell network transmitters, archivers) in a unidirectional manner
* Move commands between different control units (e.g., administrator control panel and power switch, so power can be turned on and off) in a bi-directional manner
* Allow plug-and-play flexibility, whereby the system remains stable and adapts when any component is added or removed (e.g., you can add a new meter to the system by connecting the fully-programmed unit to the Modbus communication bus, and KWH Modbus will automatically start collecting data from the meter without any additional configuration)

## How can I get started?
Modbus is configured as a Visual Studio solution with a C++ Linux project, and a Google Test unit test project. Microsoft's excellent Linux support makes this work very nicely :-)
1. Make sure you have the latest version of Visual Studio 2017 Community edition (the free version) installed, and **make sure during the install process you have Linux development tools installed, and make sure the Google Test Adapter for Visual Studio 2017 is checked.**
2. For the best experience, make sure you have Windows 10 installed and **enable the Windows Subsystem for Linux.** Then go to the Windows Store and download any Linux distro (the one I'm using is Debian, which is what Raspian OS is based off of, but I think any distro will work)
	* You can develop on a Linux machine as well, and you will be able to edit code in any tool you like. You will compile the code using GCC. What you won't be able to do (as far as I can tell) is work on unit tests.
	* I don't have a Mac, so if you want to use a Mac, you can figure that on your own (and put in a pull request if you want to add instructions ;-) )
	* 




