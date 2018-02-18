# OpcUaTestClient
OPC UA Client test console application, using [One-Way Automation C++ OPC UA SDK](https://github.com/onewayautomation/1WaOpcUaSdk).

## How to build on Windows.
It is assumed that source code repository base folder is at RepoBaseFolder.

> Please note that currently binaries for the OPC UA SDK are created only for VC++ 2017, 64 bit debug version.
If you need SDK binaries for other targets (not only VC++, any other target like Ubuntu or Raspberry Pi), please create an issue or contact us.

### Install prerequisites
- Install Microsoft Visual Studio 2017 (free Community edition is sufficient).
- Install Python 2.7 and add path to it into Path environment variable; 
- Install nasm (download from http://www.nasm.us, and add path to it into Path environment variable;)
### Clone and build dependencies
- Open command line console and go to the folder RepoBaseFolder/OpcUaTestClient 
- Run script **install-dependencies.cmd** This can take for a while (around 15 minutes) to pull source code and build all the dependencies.
### Build OpcUaTestClient
  Open solution RepoBaseFolder/OpcUaTestClient/OpcUatestClient.sln with Visual Studio 2017 and build the solution.
  
 >Please note that at the very first run of the built application it will take some time to create CA certificate and Application Instance Certificate.

## License and Copyright
Copyright 2018, One-Way Aautomation Inc.

This source code can be distributed and used under MIT license terms (https://opensource.org/licenses/MIT)

## Questions?
If you have questions / issues, or need binaries of the SDK for other targets, please contact us by email ravil at onewayautomation.com.
