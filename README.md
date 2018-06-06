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
2. If you're using Windows 10, make sure you **enable the Windows Subsystem for Linux.** Then go to the Windows Store and download any Linux distro (the one I'm using is Debian, which is what Raspian OS is based off of, but I think any distro will work). [Here is a link that has instructions on the Linux and Visual Studio setup required.](https://blogs.msdn.microsoft.com/vcblog/2017/02/08/targeting-windows-subsystem-for-linux-from-visual-studio/)
	* You can develop on a Linux machine as well, and you will be able to edit code in any tool you like. You will compile the code using GCC. What you won't be able to do (as far as I can tell) is work on unit tests.
	* I don't have a Mac, so if you want to use a Mac, you can figure that on your own (and put in a pull request if you want to add instructions ;-) )
	* To compile Linux programs, run linuxCompile.sh (use the WSL if you're on Windows 10) and it will automatically create Linux programs in the linuxPrograms folder.
3. This also compiles on Arduino:
	1. First, open an Arduino project in the kwh-modbus/Arduino folder
	2. Arduino has a weird compiler that has a hard time with just including random .h files. As a result, the source code for the project needs to be in the Arduino libraries folder for it to compile propertly in Arduino. In the kwh-modbus folder, there are two scripts that you can use that will put the code in the proper place for Arduino. Use the .bat file if you're on Windows and the .sh file if you're on Linux: ![enter image description here](https://github.com/AlexKven/kwh-modbus/raw/documentation/documentation/images/arduino_script.png)
(note: the .bat file really just runs the .sh file using the WSL)
	3. After you've done that, open the Arduino IDE, and include every .h file at the top of your sketch. Then click "verify" (what Arduino IDE calls compile), and if it is successful, then everything worked.:![enter image description here](https://github.com/AlexKven/kwh-modbus/raw/documentation/documentation/images/arduino_verify.PNG)	
	4. For now, you need to run this script every time you make a change you want to compile to Arduino. And if you delete a file, you need to delete it manually from the Arduino libraries folder.





