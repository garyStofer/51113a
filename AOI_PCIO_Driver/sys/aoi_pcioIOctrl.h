//
// Define control codes for Pcio driver
//

#ifndef __terapcioioctl__h_
#define __terapcioioctl__h_

// In order to communicate with the driver a handle needs to be obtained from a file 
// create call to the driver. The driver grants exclusive access, that means only one process
// can have an open handle to the driver and therefore be in control of the HW. To release the 
// HW resources call Close. The system calls Close upon termination of the process.

// After an application has gotten a handle to the driver it can obtain the Hardware access address
// and an address for shared event memory via the IOCTL GetMemMap. (USHORT *) OutBuff[0] returns 
// the user mode mapped hardware address.

// Alternatively the application can call .._Read_IO CTL and .._Write_IO CTL_CODE to read and write to the
// harware registers without the user mode mapped address pointer. Using discrete IOCTRL calls to communicate 
// with the driver, which in term then communicates with the hardware is inherently safer, albeit a bit slower.

// To ensure that the application does not hook up with an old driver, possibly still left in the target machine,
// The symbolic name had to be changed. 
// Unfortunately the application code did not use the TERAPCIO_REVISION below  at the approriate point when 
// establishing connection with the driver as it should have to check the driver interface version, and so 
// only a change in symbolic name can now be used to prevent an old application from using the new driver
#ifndef  CreateWithGUIDInterface	// see aoi_GUIDInterface.h
#define DRIVER_SYMBOLIC_NAME_app	"\\\\.\\AOIPcioDevice0"				// This is the name the application uses in the CreateFile call.
#define DRIVER_SYMBOLIC_NAME_driver  L"\\DosDevices\\AOIPcioDevice0"	// This is the name the driver uses to register a device resulting in the above name.
#else
#include "aoi_GUIDInterface.h"
#endif
// NOTE: In addition to the Symbolic name connection the driver also supports a GUID based connection -- See the AOI_test application 
// for an example of how to establish a connection via GUID. It is a lot more complicated than directly via the Symbolic name above


// This is a version string for the IOCTL interface only -- formerly A.0 --
#define TERAPCIO_REVISION "B.0"		

 
#define IOCTL_BASE 0x800
// Definition of IOCTL call IDs : 

// This gets the version string for the IOCTL interface -- see above 
#define PCIO_IOCTL_GetVersion CTL_CODE(FILE_DEVICE_UNKNOWN,IOCTL_BASE+0, METHOD_BUFFERED, FILE_READ_ACCESS)

// This gets the memory interface addresses from the driver.  (PVOID) OutBuf[0] contains the 
// access address for the memory mapped hardware range.
#define PCIO_IOCTL_GetMemMap CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_BASE+1, METHOD_BUFFERED, FILE_READ_ACCESS)

// The following IOCTL can only be called by a single thread, one at a time. The IOCTL will
// block until an interrupt occurs, then the driver completes the call with the interrupt status in the out buffer.
// Interrupts from the 3 timers inside the adapter are not reflected, but internally cleared should they arise.   
// The only interrrupts that are ever reflected are the adapter 2us communcation timeout and the interrupt from the WC140
// All other interrpt sources are either not enabled or managed inside the driver completely.
// Interrupts are only active when a GetNextInterrupt  IOCTRL call is pending.
// !!!! The user application is not allowed to ever write to the PCIO_INT_SEL (0x100) register !!!!
//																			   METHOD_OUT_DIRECT
#define PCIO_IOCTL_GetNextInterrupt CTL_CODE(FILE_DEVICE_UNKNOWN,IOCTL_BASE+2, METHOD_BUFFERED, FILE_READ_ACCESS)

// These two can be used instead of the GetMemMap call above to read/write single values from/to the hardware.
// This method is slower since this creates a call to the driver which does checking  on the validity of the address
// but is also safer because of that.
#define PCIO_IOCTL_Read_IO CTL_CODE (FILE_DEVICE_UNKNOWN, IOCTL_BASE+4, METHOD_BUFFERED, FILE_READ_ACCESS)
#define PCIO_IOCTL_Write_IO CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_BASE+5, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// This is a shared structure between the Driver and application by wich the interrupt status
// is conveied during a GetNextInterrpt call.
typedef struct _InterruptStatus
{
	USHORT type;
	USHORT status;
} INTERRUPTSTATUS;

#endif
