Oct. 13. 2014

This folder contains the device driver for the PCI adapter and a windows command line tool to verify and exercise 
the device driver with an attached WC140 controller.

The driver code is self contained and resides in sub folder 'sys' and 'Package'.

The test application resides in sub folder "AOI_test" and relies on a class AOI_IO found in ../../hardwareIO. 
It has it's own .sln file (VS2010) and doe not need to be built other than to exercise the driver and attached 
WC140 in a stand alone environment.

The driver has it's own solution file because it needs to be built with VisualStudio 2013 (VS12) and 
Windows Driver Foundation 8.1 ( WDF8.1) included in VS2013.

The 'sys' folder contains all the source files (.c, .h, .rc and .inx) of the driver. The build of the driver sends
the output to folder 'Win32\Win7Debug' for a win32 with Windows7 debug mode target installation.  

The project has multiple build configurations of which currently only Win7 Debug and Win7 Release are tested.
Building for a higher target OS means that it will not install on a lesser target OS.   

The Package folder contains no source files per say, but rather a project that packages the driver .sys, the .inf 
and the WDF co-installer dll and signs it with a security certificate as specified in the project file. For
the debug version the aoi_pcio.pdb file is also included so that a trace viewer/logger (traceview.exe) can
be attached to the running driver. Tracing is built into WDF and is available for Release builds as well, but the .pdb
would have to be manually moved to the target machine.

The Package project takes its inputs file from 'Win32\Win7Debug' and produces the signed driver in folder
'Win32\Win7Debug\Package'. This folder then ultimately contains the final driver distributable.

Currently both 32bit Debug and Release versions are signed with a test certificate.
 