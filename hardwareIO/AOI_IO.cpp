#include "StdAfx.h"    
#include <winioctl.h>
#include "hardwareio\AOI_IO.h"
#include "hardwareio\pcio.hpp" // needed for TCPcio class

// Class to facilitate communication with hardware through the AOI_pcio driver
//#define PRINT_VERBOSE

AOI_IO::AOI_IO( )
{
	hDevice = NULL;						// handle to device (file handle)
	io_map = NULL;
	m_EndInterruptThreadEvt = NULL;		// handle to kill event 
	m_InterruptThreadHdl = NULL;			// handle to thread 
	m_InterruptThreadStatus = FALSE;
	m_InterruptThreadExitWith_WD_running = FALSE; // Turn off the System WD upon interrupt thread exit
}

AOI_IO::~AOI_IO( )
{	
	Close_Device_Handle();
	if (m_EndInterruptThreadEvt)
		CloseHandle(m_EndInterruptThreadEvt);
	if ( m_InterruptThreadHdl )
			CloseHandle(m_InterruptThreadHdl);
}

void 
AOI_IO::Close_Device_Handle(void)
{
	if( hDevice)
	{
		// resets the PCIO -- not sure what this is going to do for us...
		PCIO_Write(PCIO_RESET, 0x3);	// ON == 3 resets the IO-card AND the controller 
		Sleep(1);
		PCIO_Write(PCIO_RESET, 0);

		CloseHandle(hDevice);
		hDevice = NULL;
		io_map = NULL;
	}
}
#ifdef CreateWithGUIDInterface
bool
AOI_IO::GetDevicePathFromGUID()
{
    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
    SP_DEVINFO_DATA DeviceInfoData;

    ULONG size;
    int count, i, index;
    bool status = TRUE;
    TCHAR *DeviceName = NULL;
    TCHAR *DeviceLocation = NULL;

    //
    //  Retrieve the device information for all PLX devices.
    //
    hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_AOI_PCIO,
                                   NULL,
                                   NULL,
                                   DIGCF_DEVICEINTERFACE |
                                   DIGCF_PRESENT);

    //
    //  Initialize the SP_DEVICE_INTERFACE_DATA Structure.
    //
    DeviceInterfaceData.cbSize  = sizeof(SP_DEVICE_INTERFACE_DATA);


	// count device instances 
	for (count=0; 
		SetupDiEnumDeviceInterfaces(hDevInfo,NULL,&GUID_DEVINTERFACE_AOI_PCIO,count,&DeviceInterfaceData);	
		count++)
	{
	}


    //
    //  If the count is zero then there are no devices present.
    //
    if (count == 0) {
        TRACE("No AOI_PCIO devices are present and enabled in the system.\n");
        this->Status = 1;
        return FALSE;
    }

    //
    //  Initialize the appropriate data structures in preparation for
    //  the SetupDi calls.
    //
    DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    //
    //  Loop through the device list to allow user to choose
    //  a device.  If there is only one device, select it
    //  by default.
    //
    i = 0;
    while ( SetupDiEnumDeviceInterfaces(hDevInfo,NULL,(LPGUID)&GUID_DEVINTERFACE_AOI_PCIO,i,&DeviceInterfaceData) ) 
	{

        //
        // Determine the size required for the DeviceInterfaceData
        //
        SetupDiGetDeviceInterfaceDetail(hDevInfo,
                                        &DeviceInterfaceData,
                                        NULL,
                                        0,
                                        &size,
                                        NULL);

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            TRACE("SetupDiGetDeviceInterfaceDetail failed, Error: %u", GetLastError());
            this->Status = 1;
            return FALSE;
        }

        pDeviceInterfaceDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA) malloc(size);

        if (!pDeviceInterfaceDetail) {
            TRACE("Insufficient memory.\n");
            this->Status = 1;
            return FALSE;
        }

        //
        // Initialize structure and retrieve data.
        //
        pDeviceInterfaceDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        status = SetupDiGetDeviceInterfaceDetail(hDevInfo,
                                                 &DeviceInterfaceData,
                                                 pDeviceInterfaceDetail,
                                                 size,
                                                 NULL,
                                                 &DeviceInfoData);

        free(pDeviceInterfaceDetail);

        if (!status) {
            TRACE("SetupDiGetDeviceInterfaceDetail failed, Error: %u", GetLastError());
            this->Status = 1;
            return status;
        }

        //
        //  Get the Device Name
        //  Calls to SetupDiGetDeviceRegistryProperty require two consecutive
        //  calls, first to get required buffer size and second to get
        //  the data.
        //
        SetupDiGetDeviceRegistryProperty(hDevInfo,
                                        &DeviceInfoData,
                                        SPDRP_DEVICEDESC,
                                        NULL,
                                        (PBYTE)DeviceName,
                                        0,
                                        &size);

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            TRACE("SetupDiGetDeviceRegistryProperty failed, Error: %u", GetLastError());
            this->Status = 1;
            return FALSE;
        }

        DeviceName = (TCHAR*) malloc(size);
        if (!DeviceName) {
            TRACE("Insufficient memory.\n");
            this->Status = 1;
            return FALSE;
        }

        status = SetupDiGetDeviceRegistryProperty(hDevInfo,
                                                  &DeviceInfoData,
                                                  SPDRP_DEVICEDESC,
                                                  NULL,
                                                  (PBYTE)DeviceName,
                                                  size,
                                                  NULL);
        if (!status) {
            TRACE("SetupDiGetDeviceRegistryProperty failed, Error: %u",GetLastError());
            free(DeviceName);
            this->Status = 1;
            return status;
        }

        //
        //  Now retrieve the Device Location.
        //
        SetupDiGetDeviceRegistryProperty(hDevInfo,
                                         &DeviceInfoData,
                                         SPDRP_LOCATION_INFORMATION,
                                         NULL,
                                         (PBYTE)DeviceLocation,
                                         0,
                                         &size);

        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            DeviceLocation = (TCHAR*) malloc(size);

            if (DeviceLocation != NULL) {

                status = SetupDiGetDeviceRegistryProperty(hDevInfo,
                                                          &DeviceInfoData,
                                                          SPDRP_LOCATION_INFORMATION,
                                                          NULL,
                                                          (PBYTE)DeviceLocation,
                                                          size,
                                                          NULL);
                if (!status) {
                    free(DeviceLocation);
                    DeviceLocation = NULL;
                }
            }

        } else {
            DeviceLocation = NULL;
        }

        //
        // If there is more than one device, print description.
        //
    
        TRACE("%d- ", i);
    
        TRACE("%s\n", DeviceName);

        if (DeviceLocation) {
            TRACE("        %s\n", DeviceLocation);
        }

        free(DeviceName);
        DeviceName = NULL;

        if (DeviceLocation) {
            free(DeviceLocation);
            DeviceLocation = NULL;
        }

        i++; // Cycle through the available devices.
    }

    //
    //  Select device.
    //
    index = 0;


    //
    //  Get information for specific device.
    //
    status = SetupDiEnumDeviceInterfaces(hDevInfo,
                                    NULL,
                                    (LPGUID)&GUID_DEVINTERFACE_AOI_PCIO,
                                    index,
                                    &DeviceInterfaceData);

    if (!status) {
        TRACE("SetupDiEnumDeviceInterfaces failed, Error: %u", GetLastError());
        return status;
    }

    //
    // Determine the size required for the DeviceInterfaceData
    //
    SetupDiGetDeviceInterfaceDetail(hDevInfo,
                                    &DeviceInterfaceData,
                                    NULL,
                                    0,
                                    &size,
                                    NULL);

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        TRACE("SetupDiGetDeviceInterfaceDetail failed, Error: %u", GetLastError());
        this->Status = 1;
        return FALSE;
    }

    pDeviceInterfaceDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA) malloc(size);

    if (!pDeviceInterfaceDetail) {
        TRACE("Insufficient memory.\n");
        this->Status = 1;
        return FALSE;
    }

    //
    // Initialize structure and retrieve data.
    //
    pDeviceInterfaceDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

    status = SetupDiGetDeviceInterfaceDetail(hDevInfo,
                                             &DeviceInterfaceData,
                                             pDeviceInterfaceDetail,
                                             size,
                                             NULL,
                                             &DeviceInfoData);
    if (!status) {
        TRACE("SetupDiGetDeviceInterfaceDetail failed, Error: %u", GetLastError());
        this->Status = 1;
        return status;
    }

    return status;
}

bool
AOI_IO::GetDeviceHandleByGUID()
{
    bool status = TRUE;

    if (pDeviceInterfaceDetail == NULL) {
        status = GetDevicePathFromGUID();
    }
    if (pDeviceInterfaceDetail == NULL) {
        status = FALSE;
    }

    if (status) {

        //
        //  Get handle to device.
        //
        hDevice = CreateFile(pDeviceInterfaceDetail->DevicePath,
								GENERIC_READ|GENERIC_WRITE,
								// FILE_SHARE_READ,
								0,	// File can not be opened by an other process or thread until the handle is closed again
								NULL,
								OPEN_EXISTING,
								FILE_ATTRIBUTE_DEVICE |FILE_FLAG_OVERLAPPED,
								NULL);

        if (hDevice == INVALID_HANDLE_VALUE) 
		{
            status = FALSE;
            TRACE("CreateFile failed.  Error:%u", GetLastError());
            this->Status = 1;
        }
    }

    return status;
}
#else

/*
	This function establishes the connection to the AOI device driver by means of the shared Symbolic name.
	CreateFile() returns a file handle by which the rest of the class functions can access the device. 
	Only one instance to the device can be opened at a time, governed both by the SHARE attribute of the 
	CreateFile() call as well as the Driver code itself.
	Note that the Device is opened with the FILE_FLAG_OVERLAPPED attribute.
	This is necessary so that WIN32 IOCTRL calls made to GetNextInterrupt can execute asynchronously in order for the 
	interrupt thread not to block other IOctrl calls made from other threads on the same handle, and also so that the interrupt thread 
	can wait on multiple objects while  waiting for the GetNextInterrupt to complete. Without asynchronous handling of 
	the IOctrl calls it would not be possible to terminate the interrupt thread gracefully with an event. 
	Only the GetNextInterrupt IOCTRL call should be executing asynchronously -- all the other IOCTRL calls should execute sync
*/
bool 
AOI_IO::GetDeviceHandleBySymLink()
{
	bool statusOK = TRUE;
	char* sLinkName = DRIVER_SYMBOLIC_NAME_app;
	hDevice = CreateFile(sLinkName,
                    GENERIC_READ|GENERIC_WRITE,
                    0,	// File can NOT be opened by any other process or thread until the handle is closed again
					NULL,
                    OPEN_EXISTING,
					FILE_ATTRIBUTE_DEVICE |FILE_FLAG_OVERLAPPED, // Overlapped makes the call Async at the win32 layer of  DeviceIoControl
                    NULL);

	if (hDevice == INVALID_HANDLE_VALUE) 
	{
        hDevice = NULL;
		statusOK = FALSE;
    }

	return statusOK;
}

#endif
BOOL 	
AOI_IO::GetDriverIOCTLVersion (char *bufOutput, size_t bufSz)
{
	BOOL statusOK = 0;
	ULONG nOutput;
	
	//This IOCTL call should always execute synchronously
	if ( hDevice)
	{
		statusOK = DeviceIoControl( hDevice,PCIO_IOCTL_GetVersion, NULL,0,bufOutput,(DWORD) bufSz,&nOutput,NULL);
	}
	return statusOK;
}

BOOL
AOI_IO::GetMemMap(void)
{	
	BOOL statusOK = 0;		// set to fail
	PVOID pointers[2] = {NULL,NULL};		
	ULONG nOutput;
	
	//This IOCTL call should always execute synchronously
	if ( hDevice)
	{
		if (! (statusOK = DeviceIoControl(	hDevice,
							PCIO_IOCTL_GetMemMap, 
							NULL,
							0,
							pointers,
							sizeof( pointers),
							&nOutput,
							NULL) 
							))
		{
				TRACE("AOI_PCIO IOcontrol failed to get user space mapped address for Hardware IO\n");
		}
		else
		{
			TRACE("-> Pointer to memory mapped registers at %p\n", pointers[0]);
// !!!! careful during debugging -- debugger pre-reads *ptr if cursor enters source window or locals window is displayed,
// possibly locking up the PCI bus because of missing hardware.
			io_map = (unsigned short *) pointers[0];
	 
		}

	}
	else 
		TRACE("No handle to device driver, can not get memory mapped address\n");
	
	return statusOK;
}

// same as Read_IO but direct via io_map[]
// !!! careful in debugging -- Debugger pre-reads *io_map and can cause a PCI bus to lock-up
//	when reading from non existing locations
bool
AOI_IO::PCIO_Read(USHORT index, USHORT *val )
{
	if (index == PCIO_STRB_CTRL)
	{
		TRACE("Invalid read from adapter interrupt status register\n");
		return false;
	}

	if (io_map && hDevice )		// if we don't have both we don't have both we don't have a connection to the HW
	{
		*val = io_map[index]; 
		return true;
	}
	return false;
}


bool
AOI_IO::Read_IOCtrl( USHORT index, USHORT * result )
{
	ULONG nOutput;
	OVERLAPPED ovl={0};
	DWORD err_code = 0;

	if (index == PCIO_STRB_CTRL)
	{
		TRACE("Invalid read from adapter interrupt status register\n");
		return false;
	}

	if (! hDevice)
	{
		TRACE("No handle to device driver, can not read\n");
		return false;
	}
/*
  This IOCTL call should always execute synchronously on any thread even though the file handle was opened with FILE_FLAG_OVERLAPPED
  Should it ever not execute synchronously the function returns false and ERROR_IO_PENDING is set. Then we wait for the result in 
  GetOverlappedResult() until the IO is complete. 
*/
	if (! DeviceIoControl(hDevice,PCIO_IOCTL_Read_IO, 
						&index,	sizeof(index),
						result,	sizeof( *result),
						&nOutput,
						&ovl) 
						)
	{
		if ((err_code = GetLastError()) == ERROR_IO_PENDING)
		{
			if (! GetOverlappedResult(hDevice,&ovl,&nOutput,TRUE) )
			{
				TRACE("AOI_PCIO Read-IOctl failed\n");
				return false;
			}
		}
		else
		{	// failed for some other reason 
			TRACE("AOI_PCIO Read-IOctl failed\n");
			return false;
		}
	}
#ifdef PRINT_VERBOSE
	TRACE("IO index %x read %x \n", index, *result);
#endif

	return true;
}

// same as Write_IO but direct via io_map[]
// !!! careful in debugging -- Debugger pre-reads *io_map and can cause a PCI bus to lock-up
//	when reading from non existing locations
bool
AOI_IO::PCIO_Write(USHORT index, USHORT val )
{
	// Do not ever write to the Adapter interrupt select register !!
	if ( index == PCIO_CONFIG  )
	{
		TRACE("Invalid write to adapter interrupt select register\n");
		return false;
	}

	if (io_map && hDevice )		// if we don't have both we don't have both we don't have a connection to the HW
	{
		io_map[index] = val;
		return true;
	}
	return false;
}


bool
AOI_IO::Write_IOCtrl(USHORT index, USHORT val )
{
	ULONG nOutput;
	OVERLAPPED ovl={0};
	DWORD err_code = 0;

	struct tag_io_buf
	{
		USHORT ndx;
		USHORT val;
	} inBuf;

	// Do not ever write to the Adapter interrupt select register !!
	if ( index == PCIO_CONFIG  )
	{
		TRACE("Invalid write to adapter interrupt select register\n");
		return false;
	}

	// package the two values in one structure 
	inBuf.ndx = index;
	inBuf.val = val;
		
	if (! hDevice)
	{	
		return false;
	}

/*
  This IOCTL call should always execute synchronously on any thread even though the file handle was opened with FILE_FLAG_OVERLAPPED
  If it does not execute synchronously the function returns false and ERROR_IO_PENDING is set. Then we wait for the result in 
  GetOverlappedResult() until the IO is complete. 
*/
	if (!  DeviceIoControl(	hDevice,PCIO_IOCTL_Write_IO, 
						&inBuf,	sizeof(inBuf),
						NULL,0,
						&nOutput,
						&ovl) 
						)
	{ 
		if ((err_code = GetLastError()) == ERROR_IO_PENDING)
		{
			if (! GetOverlappedResult(hDevice,&ovl,&nOutput,TRUE) )
			{
				TRACE("AOI_PCIO Write-IOctl failed\n");
				return false;
			}
		}
		else
		{	// failed for some other reason 
			TRACE("AOI_PCIO Write-IOctl failed\n");
			return false;
		}
	}
#ifdef PRINT_VERBOSE
	TRACE("IO index %x Write %x \n", index, val);
#endif
	return true; 
}



#ifdef for_testing_purposes_only
// This code shows how interrupts reflect back to the application through the GetNextInterrupt IOCTRL call
// This example code is easier to follow since it does not have the added complexity of running in a separate thread
// This code is not to be used in the final product and only left here for instructional purposes 
VOID
AOI_IO::testInterruptReflection_NoThread( void)
{
	bool statusOK = 0;
	INTERRUPTSTATUS intstats={0,0};
	ULONG nOutput;
	

	// no driver connection or no io_map yet -- 
	if (!hDevice || ! io_map )
		return; 
	Sleep(1000);

	// disable the watchdog timer and clear the interrupt status: code sequence from pcio.cpp in hardwareIO.dll
	DisableSysWatchdog();

	// enable the watchdog timer interrupt
	StartSysWatchdog();
	
#define LOOP_CNT 6
	for (int i =0; i<=LOOP_CNT;i++)
	{
		printf(" .... waiting for next Interrupt....");
		statusOK = DeviceIoControl(	hDevice,
									PCIO_IOCTL_GetNextInterrupt, 
									NULL,
									0,
									&intstats,
									sizeof( intstats),
									&nOutput,
									NULL) 
									; 
		
		printf ("\nInt reflected with statusOK %d, int_type 0x%x, int_ext_status 0x%x\n", statusOK, intstats.type, intstats.status); 
		
		if (intstats.type & 0x10 && intstats.status & 0x10)	// the PRETIMEOUT timed out -- needed to reload the WDTimer
		{	

			if ( i < 3 )	// feed the watchdog 
			{
				ReloadSysWatchdog();
			}
			else // let the Watchdog fail 
			{
				// you can't turn off the PreTimeout interrupt by itself -- Clearing the PTO_STATUS without reloading the 
				// timer will just cause an new interrupt because the counter/timer is still beyond the pretimeout count
				// The Pre-time-out interrupt has no MASK and no Enable, so it is not possible to disable this interrupt by itself without also disabling the main watchdog function
				// However, inserting a small Sleep here will let the counter timer advance enough so that it is beyond the pretimeout count 
				// and so doesn't cause a Pre-time-out interrupt again
				Sleep(10);					// This is only for demonstrating 
				Write_IO(PTO_STATUS, 0);	// Clearing now works because the counter has moved past the Pretimer trigger count
				printf("Not reloading the watchdog -- Main Watchdog timeout should occur next\n");
							
				// or shut off wachdogging altogether 
				//Write_IO(TO_ENABLE,  0x0);
				//Write_IO(TO_MASK,    0x0);
				//Write_IO(TO_STATUS,  0x0);
				//printf("Disabling the watchdog\n");
			}
		}
		
		if (intstats.type & 0x10 && intstats.status & 0x20)	// the Watchdog timed out 
		{
			printf("Watchdog timeout happened! Waiting 500ms for the controller to recover \n");
			Sleep(CONTROLLER_BOOT_TIME);
			
			// restart watchdog again -- this will create more Prewatchdog timeouts (which we will not feed), followed by a Full timeout each
			DisableSysWatchdog();

			// enable the watchdog timer interrupt
			StartSysWatchdog();
		}

		if (intstats.type & 0x8 )
		{
			printf("2us communications timeout while talking to Controller!!! BAD!\n");
		}
	}

	// Shut off WD 
	DisableSysWatchdog();
}
#endif

int
AOI_IO::StartInterruptThread( TCPcio *t)
{
	DWORD id;

	m_InterruptThreadStatus = FALSE;
	
	if ( m_EndInterruptThreadEvt )	// precaution to prevent lost handles 
	{
		CloseHandle(m_EndInterruptThreadEvt);
		m_EndInterruptThreadEvt = NULL;
	}

	if ( m_InterruptThreadHdl )	// precaution to prevent lost handles 
	{
		CloseHandle(m_InterruptThreadHdl);
		m_InterruptThreadHdl = NULL;
	}

	// FIRST create the Kill event for the thread 
	m_EndInterruptThreadEvt = CreateEvent(NULL,TRUE,FALSE,NULL);	// create un-named event in un-signaled state with manual reset
	
	// Create and start the thread for interrupts
	
	m_InterruptThreadHdl = CreateThread(NULL, 0,t->InterruptThread, t, NULL, &id);
	
	if (!m_InterruptThreadHdl)
	{
		TRACE("StartInterruptThread: failed to start\n");
		return PCIO_INTERRUPT_ERROR;
	}

	do
	{
		Sleep(1); // yield for the new thread to start-up
	} while (! m_InterruptThreadStatus);

	TRACE("-> Interrupt thread with ID:%d started and running\n", id);
	return PCIO_SUCCESS;
}

/* Ends the interrupt thread gracefully by signalling an event which the interrupt thread is monitoring
// Argument: leaveSysWatchdogRunning prevents the interrupt clean-up code from disabling the controller watchdog, so
// that it reboots the controller when the time is up.
// After signalling the event it checks the thread status for up to 100ms waiting for the thread to terminate
// If after this time the thread has not terminated the thread is brute-force-closed -- This is a last resort and should
// not happen under normal circumstances.   
// The interrupt thread should not use any blocking style Sleep calls, but instead should use WaitFor....Object() functions.
*/
void
AOI_IO::EndInterruptThread( bool leaveSysWatchdogRunning )
{
	int i;

	if (!m_InterruptThreadStatus || !m_InterruptThreadHdl || !m_EndInterruptThreadEvt )//thread has already been signaled and is exited 
		return;

	// leaving the system Watchdog running on exit causes a Controller reboot when the WD runs out
	m_InterruptThreadExitWith_WD_running = leaveSysWatchdogRunning; 

	// signal the interrupt thread to terminate itself
	SetEvent(m_EndInterruptThreadEvt);

// TODO: might need to adjust the interval if we see that the thread gets brute-force-closed ever 

	// wait for the thread to exit, this could also be done with waiting for the thread handle to signal -- but this seems easier
	for( i =0; i<100 ; i++ ) // give it 100ms to go away in case the thread has some lengthy process to finish 
	{  
		Sleep(1);
		if ( m_InterruptThreadStatus == FALSE)	// thread is done
			break;
	}

	if (m_InterruptThreadStatus)				// if thread for some reason did not want to die
	{
		TerminateThread(m_InterruptThreadHdl,0);	// brute force it down --- BAD  ----
		CloseHandle(m_InterruptThreadHdl);
		m_InterruptThreadHdl = NULL;
		CloseHandle(m_EndInterruptThreadEvt);
		m_EndInterruptThreadEvt = NULL;
		TRACE("Brute force kill of interrupt thread -- some resources most likely lost! "); 
	}

}

