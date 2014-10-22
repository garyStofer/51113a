Oct. 10. 2014

This is a VS2010 project to test and exercise the AOI_PCIO driver and attached WC140 controller.
It is a stand alone solution since this is not to be built or distributed as part of the final product and should 
therefore not be included in the solution file Tenphase.sln.

The command line application is dependent on the device-driver interface class AOI_IO found in file AOI_IO.cpp under
folder src/hardwareIO. 

The application has a test class TCPcio which inherits from the device-driver interface class AOI_IO. The test class exercises 
the driver and attached WC140 controller and provides printf messages to the console, while TRACE messages from class AOI_IO can be 
observed on the debugger console window. 

The test class TCPcio implements the interrupt handling thread and the functions to operate the system watchdog in the same manner as 
the real TCPcio class, implemented in file pcio.cpp under hardwareIO.  

The main.cpp file uses the test class TCPcio to show the communication to/from the controller and starts/stops the interrupt 
handling thread to demonstrate the sys watchdog function etc..






