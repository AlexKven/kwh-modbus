# Directory structure of the Project
Here is a summary of the important files and folders in the repository. Files shown that aren't in the repo are usually Visual Studio build outputs that I don't anticipate we will be using very much.
## The root directory:
![Root directory](https://github.com/AlexKven/kwh-modbus/raw/documentation/documentation/images/directory_structure_root.png)
1. The documentation for the project (like what you're reading now)
2. The folder that contains all the source code for the project itself
3. Unit tests for individual components, run on Windows 10 with Visual Studio
4. The gitignore for the project, which describes files that Git should not look at (e.g., you don't need compiled programs in the GitHub repository)
5. The Visual Studio solution
6. The markdown file that is displayed on the front page of the repo on GitHub

## The kwh-modbus directory:
![kwh-modbus directory](https://github.com/AlexKven/kwh-modbus/raw/documentation/documentation/images/directory_structure_kwh-modbus.png)


7. This folder contains the Arduino sketches for the project
8. Contains interfaces, which define a set of functionality that a class can implement. Classes that implement an interface can be swapped out with a **mock** (#12) for testing
9. Contains code that runs on all platforms, divided up into components
10. This contains the Linux programs that are compiled via command line or the compile script (does not contain Visual Studio compiled programs)
11. Contains the source code for individual Linux programs for a task. Each program is generally a single .cpp file with a main() function
12. Contains mocks, which are fake versions of interfaces or classes that are used for unit testing. The component being tested uses the mock object for its dependencies instead of real versions of the class
13. Contains versions of classes and functions used on Arduino that can be compiled to Linux and Windows instead.
14. Visual Studio project file for the main project (I.e., everything but unit tests)
15. I'm not sure what this is, but it's used by Visual Studio
16. Script that compiles all Linux programs, and needs to be run on Linux (or the Windows Subsystem for Linux on Windows 10)
17. Copies all the libraries over to the Arduino libraries folder on Windows, so it can be used by the Arduino IDE
18. Same as 17, but copies the libraries to the Arduino folder on Linux, and runs on Linux
