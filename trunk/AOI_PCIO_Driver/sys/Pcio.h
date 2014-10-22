#pragma once
#define PLX9050_MemMapSize      (0x1000)	// This is the memory mapped space of the adapter as requested in BAR2 of the PCI configuration space
extern PUSHORT	MemIoBase;		// The memory mapped hardware address space. Only driver code can access HW through this. See below


//
// The device extension for the device object
//
typedef struct _DEVICE_EXTENSION
{
	// HW Resources
	// The following HW io pointer and associated length is moved to driver global space so that the pointer doesn't have to
	// be recovered each time in the IOCTRL callback function. Since we limit this driver to a single HW instance having a
	// single global instance of these variables is save. 
	//PUSHORT				MemIoBase;		// The memory mapped address space behind this will be MemIoLength long
	//ULONG					MemIoLength;	// The size in bytes of the memory mapped space as requested by the PCI BAR, should be equal to PLX9050_MemMapSize
	WDFQUEUE                IOCTLQueue;		// The main queue receiving all the IOCTRLS 
	WDFQUEUE				IntQueue;
	WDFINTERRUPT            Interrupt;      // Returned by InterruptCreate
	WDFDEVICE               Device;
	//WDFREQUEST				CurrentInterruptRequest;		

}  DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// This will generate the function named GetDeviceContext to be use for
// retrieving the DEVICE_EXTENSION pointer.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_EXTENSION, GetDeviceContext)

typedef struct _INTRRUPT_EXTENSION
{
	USHORT adapter_intr_status;
	USHORT controller_intr_status;

} INTERRUPT_EXTENSION, *PINTERRUPT_EXTENSION;

//
// This will generate the function named GetInterruptContext to be use for
// retrieving the INTERRUPT_EXTENSION pointer.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(INTERRUPT_EXTENSION, GetInterruptContext)

typedef struct _QUEUE_EXTENSION
{
	USHORT tmp;
} QUEUE_EXTENSION, *PQUEUE_EXTENSION;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(QUEUE_EXTENSION, GetQueueContext)


//
// Function prototypes
//
DRIVER_INITIALIZE DriverEntry;		// the "main()" of all driver things

// these are callback events
EVT_WDF_DEVICE_FILE_CREATE EvtDeviceFileCreate;
EVT_WDF_FILE_CLOSE EvtDeviceFileClose;
EVT_WDF_FILE_CLEANUP EvtDeviceFileCleanup;
EVT_WDF_DRIVER_DEVICE_ADD EvtDeviceAdd;
EVT_WDF_DRIVER_UNLOAD EvtDriverUnload;
EVT_WDF_OBJECT_CONTEXT_CLEANUP EvtDriverContextCleanup;
EVT_WDF_DEVICE_D0_ENTRY EvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT EvtDeviceD0Exit;
EVT_WDF_DEVICE_PREPARE_HARDWARE EvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE EvtDeviceReleaseHardware;

//
// WDFINTERRUPT Support
//

EVT_WDF_INTERRUPT_ISR EvtInterruptIsr;
EVT_WDF_INTERRUPT_DPC EvtInterruptDpc;
EVT_WDF_INTERRUPT_ENABLE EvtInterruptEnable;
EVT_WDF_INTERRUPT_DISABLE EvtInterruptDisable;

NTSTATUS InterruptCreate(IN PDEVICE_EXTENSION DevExt);

// File interface support
VOID 
EvtDeviceFileCreate(IN WDFDEVICE Device, IN WDFREQUEST Request, IN WDFFILEOBJECT FileObject);

VOID
EvtDeviceFileClose(_In_ WDFFILEOBJECT FileObject);

VOID
EvtDeviceFileCleanup(_In_ WDFFILEOBJECT FileObject);


#pragma warning(disable:4127) // avoid conditional expression is constant error with W4