#include "main.h" 

//------------------------------------- Class TCPcio ----------------------------

TCPcio::TCPcio()
{
	m_somePrivatmember = 8;
}

TCPcio::~TCPcio()
{
	m_somePrivatmember = 0;
}

// This starts the system watchdog running. Make sure that ReloadSysWatchdog() is called before enabling
// so that the timer is loaded initially. 
// Causes first a PREWatchdog timeout interrupt, followed by a full WATCHDOG timeout interrupt if the dog 
// is not fed via a call to ReloadSysWatchdog at the time of the PREWATCHDOG.
// Should the full WATCHDOG timeout ever occur the controller hardware reboots the FPGA during which 
// time the application can not talk to it. 
// Function of the Watchdog is dependent on the interrupt thread, which provides the mechanism for automatic
// reloading when the PREWATCHDOG interrupt occurs.
void 
TCPcio::StartSysWatchdog(void)		
{
	DisableSysWatchdog();	// clears status and sets initial timeout 
	PCIO_Write(WD_MASK,  (SHORT)  0x1);
	PCIO_Write(WD_ENABLE, (SHORT) 0x1);
}

void
TCPcio::ReloadSysWatchdog(void) 
{
	PCIO_Write(WD_TIMEOUT, CONTROLLER_WATCHDOG_TIME);// This feeds the watchdog counter/timer for an other 4 seconds
	PCIO_Write(PTO_STATUS,(SHORT) 0);				 // This clears the PRE_TIMEOUT interrupt 
}
void
TCPcio::PauseSysWatchdog(void)
{
	PCIO_Write(WD_ENABLE, (SHORT) 0x0);
}

void
TCPcio::ContinueSysWatchdog(void)
{
	ReloadSysWatchdog();
	PCIO_Write(WD_ENABLE, (SHORT) 0x1);
}
void
TCPcio::DisableSysWatchdog(void )	// This stops and disables the System Watchdog timer.  
{
	PCIO_Write(WD_TIMEOUT,   (SHORT) CONTROLLER_WATCHDOG_TIME);	// so it doesn't interrupt while we are clearing it
	PCIO_Write(WD_ENABLE, (SHORT) 0x0);							
	PCIO_Write(WD_MASK,   (SHORT) 0x0);
	PCIO_Write(WD_STATUS, (SHORT) 0x0);

}
int
TCPcio::PCIO_Open()
{
	char rev[24];
	USHORT result;
	BOOL stat;
	//stat = p.GetDeviceHandleByGUID();		 // This is an alternate way of getting a file handle to the driver -- Using a GUID --
											 // The driver provides both the GUID method and the straight symbolic link option
											 // Symbolic link is much easier to open and is the method used by the previous NT driver
	stat = GetDeviceHandleBySymLink();		// using the easier way
	if ( !stat )
	{
		    printf("Could not obtain a file handle to device %s, Error:%u", DRIVER_SYMBOLIC_NAME_app, GetLastError());
			return stat;
	}

	if ((stat = GetDriverIOCTLVersion(rev, sizeof(rev))))
		printf("AOI_PCIO IOcontrol version :%s \n",rev);
	else
	{
		Close_Device_Handle();
		return stat;
	}
	 // after GetDecviceHandleBy...() we can use the IOCTRL interface, but not yet the direct memory mapped interface 
	Write_IOCtrl(PCIO_RESET,RST_ALL);	// This resets the IO card and turns all interrupts off 
	Sleep(1);							// -- this interferes with the framework enabling and disabling the interrupts
	Write_IOCtrl(PCIO_RESET,RST_NONE);
	Sleep(50);

	// This is getting the user mode mapped address pointer to memory mapped hardware IO of the adapter
	// The resulting io_map pointer is valid as long as the File handle obtained in either GetDeviceHandleBySymLink()
	// or GetDeviceHandleByGUID() remains valid, but can not be accessed after the file has been closed
	if ( !(stat =GetMemMap( )) )		
	{
		printf("AOI_PCIO  GetMemMap failed \n");
		Close_Device_Handle();
		return stat;
	}
	else
	{
		// These are read and writes via the direct user-spaced mapped hardware address range	
		PCIO_Write(0x101,0x40);		// set analog strobe
		PCIO_Read(0x100,&result);		// bit 0x40 should be set -- upper byte indicates HW revision b1, 0xb148

		PCIO_Write(0x101,0x00);		// set digital strobe
		PCIO_Read(0x100,&result);		// bit 0x40 should be cleared -- should read 0xb108
	}
	return stat;
}

bool TCPcio::PCIO_Read(USHORT index, USHORT *val )
{
	return AOI_IO::PCIO_Read(index,val);
	// or possbly 
	// return AOI_IO::Read_IOCtrl(index,val);
}

bool TCPcio::PCIO_Write(USHORT index, USHORT val )
{
	return AOI_IO::PCIO_Write(index,val);
	// or possbly 
	//return AOI_IO::Write_IOCtrl(index,val);
}

void
TCPcio::PCIO_Close()
{
	Close_Device_Handle();
}

int
TCPcio::StartInterruptThread(void)
{	
	return AOI_IO::StartInterruptThread(this);
}
void
TCPcio::KillAllThreads(  int leave_WD_running )	// function argument used for WD disabling
{

	// EndInterruptThread( false);	// true: leaves the WD running will reboot WC140 after timeout
	if (leave_WD_running ) 
		printf("End Interupt thread with WD-Running -- Will reboot controller soon\n");
	else
		printf("End Interupt thread with WD disabled -- Will not reboot controller\n");
		
	EndInterruptThread(  leave_WD_running !=0 );	// true: leaves the WD running will reboot WC140 after timeout
}

DWORD WINAPI

TCPcio::InterruptThread(void *arg)
{ 	
	DWORD err_code = 0;
	OVERLAPPED ovl;
	HANDLE hIOCompletionEvt = NULL;
	HANDLE wait_objects[2];	// handle list for waitForMultipleObjects()
	
	// pointer to TCPcio object
	TCPcio * pPcio = (TCPcio *) arg;

	hIOCompletionEvt = CreateEvent(NULL,TRUE,FALSE,NULL);	// create un-named event in un-signaled state with manual reset
	wait_objects[0] = hIOCompletionEvt;
	wait_objects[1] = pPcio->m_EndInterruptThreadEvt;

	// raise the priority of this thread
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

	pPcio->m_InterruptThreadStatus = TRUE; // indicating that the thread is off and running

//#ifndef _DEBUG   
	// Having the system watchdog enabled while the Visual Studio debugger stops the application is not desired,
	// it makes harware debugginng impossible when the system controller re-boots after its timeout 
	
	// start system watchdog
	pPcio->StartSysWatchdog();	 // enables interrupts on controller and lets it run
//#else
//#pragma message( "Watchdog Timer function is not running in DEBUG build!!")
//#endif

	while(1)	// forever or until a killThread event is sent
	{
		INTERRUPTSTATUS interrupt;	// IOcontrol buffer which contains the status of the interrupt 
		ULONG	nOutput;            // Count written to bufOutput
		
		// Call the device driver with a request for the next interrupt status. 
		// This call does not return back until an interrupt happened, then it returns with the type 
		// and extended interrupt status as read from the  WC140 controller. Type indicates what  
		// source generated the interrupt. If the source was the system controller, status indicates
		// what on the system controller caused the interrupt. 

		// Hardware interrupt at the IO card level are managed by the device driver in order to synchronize and serialize.
		// Interrupts are only enabled during the time the GetNextInterrupt IOCTL is pending and are off everywhere else.
		// The application code MUST ABSOLUTELY NOT interfere with this mechanism and can therefore not write to the 
		// hardware interrupt enable register of the adapter.

		// Only the events of two interrupt sources are reflected back to here. 
		// They are:	
		//		2usComm timeout -- this is generated by the IO adapter and happens when the controller is not responding to
		//						a read or write within 2us.This can happen because the controller is not plugged in or is 
		//						not ready to operate  while booting for example.
		//		Digital_int	   -- this is generated by the system controller and the extended status is provided.
		// All other interrupts are either not enabled or are dealt with locally in the driver code.

		interrupt.type = interrupt.status = 0;
		memset(&ovl,0,sizeof(ovl));		// Must clear each time around otherwise an Invalid handle might be present inside ovl
		ovl.hEvent = hIOCompletionEvt; // Without it's own Event the file handle is used as the event which also gets signaled on every other Device IOCtrol 


		// Post the request for another interrupt status -- This executes async if the file handle is opened with FILE_FLAG_OVERLAPPED
		// If the fh is not in async mode this DeviceIoControl call, executed from a secondary thread, would block any other calls
		// on any thread using the same fh
	
		if (!DeviceIoControl(pPcio->hDevice, PCIO_IOCTL_GetNextInterrupt, 
							 NULL,0,
							 &interrupt,sizeof(INTERRUPTSTATUS),
							 &nOutput,	// nuber of bytes returned is not needed -- always returns the same 
							 &ovl)	)	// the hEvent is sent through here, so that it can be signalled when IO is complete 
		{
			if ((err_code = GetLastError()) == ERROR_IO_PENDING)	// ERROR_IO_PENDING is returned for async IO. This is NOT an error.  
			{ 
				// This IO is marked asyncronous and pending, we now block on it here until the IO is completed
				switch ( WaitForMultipleObjects(2,wait_objects,FALSE,INFINITE ))
				{
					case WAIT_OBJECT_0+0:  // the IOControl is completed, we can now go and process the interrupt data
						break;

					case WAIT_OBJECT_0+1:  // the m_hKillInterruptThreadEvt signaled
						goto thread_exit;
				}
			}
			else
			{	// this should not happen 
				printf("GetNextInterrupt request failed err: %d !! thread exits\n",err_code);
				goto thread_exit;
			}
		}
		else
		{	
			// This should not happen in this scope with the file handle opened using FILE_FLAG_OVERLAPPED
			// It means that the DeviceIoControl executed syncronously and was therefore blocking. 
			// If it does we can't properly terminate this thread, except by checking the m_hKillInterruptThreadEvt
			// once after an interrupt occured.
			printf("get-next-interrupt returned Syncronously unexpectedly !!\n");

			if ( WaitForSingleObject(pPcio->m_EndInterruptThreadEvt,0) == WAIT_OBJECT_0)	// poll m_hKillInterruptThreadEvt
				goto thread_exit;
		}

		/* If we got here we have a valid GetNextInterrupt response to process */
				
		// This is the 2us comm timeout the adapter generated because the controller was not handshaking
		// This happens if the cable became disconnected or the controller is reset and therefore booting 
		// controller booting happens automatically on a watchdog timeout -- hardware logic does it

		if (interrupt.type & INT_2us)		 
		{ 
			printf("2us communications timeout while talking to Controller!!! BAD!\n");
		} // 2us communication timeout
 
		if (interrupt.type & INT_Dig)			// digital interrupt from System controller 
		{
			// The Watchdog needs feeding NOW !
			if (interrupt.status & SYS_PRE_TIMEOUT_73)  
			{ 
				pPcio->ReloadSysWatchdog(); // Feeds the watchdog by resetting the time again to n seconds and clearing the PRE Time-out interrupt 
				printf("I fed the dog\n");
			}
			// The actual Watchdog timed out and is rebooting the controller now 
			// This takes about 200ms and we need to wait 500ms before we start talking to it again
			// again, or else we get continued 2us time-outs on every io 
			else if (interrupt.status & SYS_TIMEOUT_73)			
			{
				printf("Watchdog timeout happened! Waiting 500ms for the controller to recover \n");
			
				// This Sleep allows the thread to be canceled during the sleep
				if ( WaitForSingleObject(pPcio->m_EndInterruptThreadEvt,CONTROLLER_BOOT_TIME) == WAIT_OBJECT_0)	// poll m_hKillInterruptThreadEvt
					goto thread_exit;	// since we are closing the interrupt handling anyways its OK to exit before the boot time is up

				// re-enable again for this testing
				pPcio->StartSysWatchdog();
			}
			else
			{
				printf("Other Controller interrupt happened 0x%x - 0x%x Not handled and not cleared in this example code\n",interrupt.type,interrupt.status);
			}
		}// digital interrupt 
		else
		{	
			printf( "Unexpected interrupt reflected to application 0x%x - 0x%x !! Unhandled and uncleared in this example code\n",interrupt.type,interrupt.status);
		}
	} 
	
thread_exit:
	printf("Interrupt thread exited gracefully because KillThread was set\n");


	if (!pPcio->m_InterruptThreadExitWith_WD_running)
		pPcio->DisableSysWatchdog(); 	// The WD must be disabled before exiting this thread or else it will reboot the WC140

	CloseHandle(hIOCompletionEvt);
	CloseHandle(pPcio->m_EndInterruptThreadEvt);
	pPcio->m_EndInterruptThreadEvt = NULL;
	CloseHandle(pPcio->m_InterruptThreadHdl);
	pPcio->m_InterruptThreadHdl = NULL;
	pPcio->m_InterruptThreadStatus = FALSE;		// This indicates the thread has completed.
	return err_code;
}
