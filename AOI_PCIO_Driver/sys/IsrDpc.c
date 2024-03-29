#include "Precomp.h"
#include "IsrDpc.tmh"


/*++
Routine Description:

    Configure and create the WDFINTERRUPT object.
    This routine is called by EvtDeviceAdd callback.

Arguments:

    DevExt      Pointer to our DEVICE_EXTENSION

Return Value:

    NTSTATUS code

--*/
NTSTATUS
InterruptCreate(IN PDEVICE_EXTENSION DevExt)
{
    NTSTATUS                    status;
    WDF_INTERRUPT_CONFIG        InterruptConfig;
	WDF_OBJECT_ATTRIBUTES		attributes;				

    WDF_INTERRUPT_CONFIG_INIT( &InterruptConfig, EvtInterruptIsr, EvtInterruptDpc );

    InterruptConfig.EvtInterruptEnable  = EvtInterruptEnable;
    InterruptConfig.EvtInterruptDisable = EvtInterruptDisable;

    InterruptConfig.AutomaticSerialization = TRUE;

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, INTERRUPT_EXTENSION);		// reserve and initialize storage for the interrupt context aka InterruptExtension
    
    // Unlike WDM, framework driver should create interrupt object in EvtDeviceAdd and
    // let the framework do the resource parsing and registration of ISR with the kernel.
    // Framework connects the interrupt after invoking the EvtDeviceD0Entry callback
    // and disconnect before invoking EvtDeviceD0Exit. EvtInterruptEnable is called after
    // the interrupt interrupt is connected and EvtInterruptDisable before the interrupt is
    // disconnected.
    //
    status = WdfInterruptCreate( DevExt->Device,
                                 &InterruptConfig,
                                 &attributes,
                                 &DevExt->Interrupt );

    if( !NT_SUCCESS(status) ) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                    "WdfInterruptCreate failed: %!STATUS!", status);
    }

    return status;
}


/*++
Routine Description:

    Interrupt handler for this driver. Called at DIRQL level when the
    device or another device sharing the same interrupt line asserts
    the interrupt. The driver first checks the device to make sure whether
    this interrupt is generated by its device and if so clear the interrupt
    register to disable further generation of interrupts and queue a
    DPC to do other I/O work related to interrupt - such as reflecting information 
	about the interrupt event to the application

Arguments:

    Interrupt   - Handle to WDFINTERRUPT Object for this device.
    MessageID  - MSI message ID (always 0 in this configuration)

Return Value:

     TRUE   --  This device generated the interrupt.
     FALSE  --  This device did not generated this interrupt.

--*/
// This is the location where the 51140 controller has its interrupt status present 
#define C051140_INT_ALL_STATUS       0xc0 // From ControllerAddresses.hpp
BOOLEAN
EvtInterruptIsr( IN WDFINTERRUPT Interrupt,IN ULONG  MessageID)
{
	UNREFERENCED_PARAMETER(MessageID);
	PDEVICE_EXTENSION   devExt = GetDeviceContext(WdfInterruptGetDevice(Interrupt));
	PINTERRUPT_EXTENSION intExt = GetInterruptContext(Interrupt);

	USHORT intr_status;
	USHORT tmr_stat;  
	BOOLEAN stat;
	
    //TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INTERRUPT, "--> PLxInterruptHandler");

	intr_status = READ_REGISTER_USHORT(&MemIoBase[PCIO_INT_READ]);	 // read the inerrupt status 
	intr_status &= 0xFF;
	
	if (intr_status == 0)
		return FALSE; // Return FALSE to indicate that this device did not cause the interrupt.

	// Timers are not used in the AOI machine but it is still possible that some code starts a timer and an interrupt is generated by it 
	// Therefore this handler silently clears the interrupt and resets the timer
	
	if ((tmr_stat = (intr_status & INT_TMR_BITS)) != 0)
	{

		if (tmr_stat & INT_Tmr1)
			WRITE_REGISTER_USHORT(&MemIoBase[TMR1_CTRL], TMR_CTRL_RESET);

		if (tmr_stat & INT_Tmr2)
			WRITE_REGISTER_USHORT(&MemIoBase[TMR2_CTRL], TMR_CTRL_RESET);

		if (tmr_stat & INT_Tmr3)
			WRITE_REGISTER_USHORT(&MemIoBase[TMR3_CTRL], TMR_CTRL_RESET);
	}

	// Do we have any  interrupts  we need to send to the application ?
	// All other sources should not be enabled ever
	// should an unused interrupt arises anyway it is silently swollowed here


// TODO: should it ever proof that during a JTAG action, when the adapter is using A_Strobes to communicate, the interupt line
// from the controller fires we could try to eliminate this false intrrupt from reaching the application here by checking the 
// STROBE_A_RFLECT_BIT in the PCIO_CONFIG and act accordingly. Better yet make sure that getNextInterrupt is not pending when 
// Jtag action is perforemed ! 

	if (intr_status & (INT_2us | INT_Dig))	
	{
		// Disable interrupts here to make sure we have completed the deferred processing before we take on an other one.
		// All ints are enabled (or re-enabled) by the queueing of the GetNextInterrupt request in the
		// DeviceIOControl handler. That is the only place where ALL_INT_EN can be enabled without 
		// running the risk of either missing interrupts or permanemtly locking interrupts out.
		
		WRITE_REGISTER_USHORT(&MemIoBase[PCIO_INT_SEL], ALL_INT_DIS);
		
		intExt->adapter_intr_status = intr_status;  // Save the interrupt status for to the DPC
		
		// See note about controller watchdog timeout in ISR_DPC
		if (intr_status & INT_Dig)			// The Controller Interrupted, get the extended interrupt status from it
			intExt->controller_intr_status = READ_REGISTER_USHORT(&MemIoBase[C051140_INT_ALL_STATUS]);
		else
			intExt->controller_intr_status = 0; // there is no extended interrupt for anything else
	
		// This could fail if being called while the last DPC is still running.
		// Disabling interrupts until the DPC finished prevents this.
		stat =  WdfInterruptQueueDpcForIsr(devExt->Interrupt);
	
		// This should never happen -- coding sequence error if it does.
		if (!stat) 
			TraceEvents(TRACE_LEVEL_ERROR, DBG_INTERRUPT, "EvtInterruptIsr: Failed to queue DPC for Interrupt!");
	}
	else
	{
		TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INTERRUPT, "EvtInterruptIsr: Unreflected interrupt occured 0x%x !", intr_status);
	}
    return TRUE;		// It was ours 
}


/*++

Routine Description:

    DPC callback for ISR. Please note that on a multiprocessor system,
    you could have more than one DPCs running simultaneously on
    multiple processors. So if you are accessing any global resources
    make sure to synchronize the accesses with a spinlock.

Arguments:

    Interrupt  - Handle to WDFINTERRUPT Object for this device.
    Device    - WDFDEVICE object passed to InterruptCreate

Return Value:

--*/


_Use_decl_annotations_
VOID
EvtInterruptDpc(WDFINTERRUPT Interrupt, WDFOBJECT  Device)
{
	PDEVICE_EXTENSION   devExt = GetDeviceContext(Device);
	PINTERRUPT_EXTENSION intExt = GetInterruptContext(Interrupt);
	WDFREQUEST InteruptRequest;
	INTERRUPTSTATUS *OutBuf = NULL;
	NTSTATUS status;
	size_t size;
	USHORT Istat;
	LARGE_INTEGER sleep;

	sleep.QuadPart = WDF_REL_TIMEOUT_IN_MS(500);

	// Note about Controller watchdog timeout:
	// If the watchdog ever times out the controller goes into an automatic reboot which takes between 200 and 500ms
	// during wich time the controller is incommunicado.  This means that the following controller status read from 
	// C051140_INT_ALL_STATUS will timeout by the 2us IO adapter comm timeout protection and the read will return 
	// 0xFFFF. 
	// The 2us timeout registers a interrupt, wich would be reflected to the app on the following GetNextInterrupt call.
	// See special procesing below to suppress the extra 2us interrupt and wait for the controller to boot before returing 
	// the interrpt info back to the application.

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_DPC, "EvtInterruptDpc");

	// This checks the IntQueue to get the Request object that was forwarded from the IOCTRL GetNextInterrupt to this manual queue 
	// If there is a request in the Queue it gets dequeued and completed with the interrupt type and status set back to the application 
	// if there is no request in the queue which could happen if the application enabled interrupts but did not get around to call the GetNextInterrrpt IOCTRL yet 
	// or left interrupts on and stopped listening to interrupts this function will swallow the interrupt.
	
	if ((status = WdfIoQueueRetrieveNextRequest(devExt->IntQueue, &InteruptRequest)) == STATUS_SUCCESS)		
	{
		if (InteruptRequest)
		{
			size = sizeof(INTERRUPTSTATUS);
			if ((status = WdfRequestRetrieveOutputBuffer(InteruptRequest, size, &OutBuf, NULL) )== STATUS_SUCCESS)
			{
				// Do I really need the spin lock here ? It seems that by the fact that interrupts are turned off in the ISR there is no 
				// chance of anything being able to modify intExt->xyz anyhow 
				// WdfInterruptAcquireLock(Interrupt);     // Acquire this device's InterruptSpinLock to get exclusive access to intExt->

				// detect a rebooting controller due to a WD timeout 
				if (intExt->adapter_intr_status & INT_Dig && intExt->controller_intr_status == 0xFFFF) // Controller is not talking
				{	// re-read the adapters interrupt status again to check and clear the 2us interrupt caused 
					// by the Controller beeing incommunicado
					Istat = READ_REGISTER_USHORT(&MemIoBase[PCIO_INT_READ]);
					
					if (Istat & INT_2us)// we have evidence that the controller was not talking and therefore it read 0xFFFF
					{
						TraceEvents(TRACE_LEVEL_ERROR, DBG_DPC, "ISR_DPC:Controller Watchdog reboot detected!");
						intExt->controller_intr_status = 0x20;
						// Note: Sleeping in here to pass the time for the controller to boot does not work
						// The application thread has to sleep before accessing the controller again
					}
				}

				OutBuf->type = intExt->adapter_intr_status;
				OutBuf->status = intExt->controller_intr_status;

				// WdfInterruptReleaseLock(Interrupt);
				WdfRequestCompleteWithInformation(InteruptRequest, STATUS_SUCCESS, size);
				TraceEvents(TRACE_LEVEL_ERROR, DBG_DPC, "ISR_DPC interrupt reflected - type: 0x%x, extStatus 0x%x", intExt->adapter_intr_status, intExt->controller_intr_status);
			}
			else // this is a coding problem if this ever happens
				TraceEvents(TRACE_LEVEL_ERROR, DBG_DPC, "ISR_DPC:Failed to reflect interrupt-output buffer too small");
		}
		else
			TraceEvents(TRACE_LEVEL_ERROR, DBG_DPC, "ISR_DPC:Failed to reflect interrupt-No getNextInterrupt request in queue");
	}
	else
	{
		TraceEvents(TRACE_LEVEL_ERROR, DBG_DPC, "ICR_DPC:Failed to reflect interrupt-No getNextInterrupt request in queue");
		TraceEvents(TRACE_LEVEL_ERROR, DBG_DPC, "... interrupt type: 0x%x, extStatus 0x%x", intExt->adapter_intr_status, intExt->controller_intr_status);
	}

	// do not enable interrupts here, DeviceIOControl handler for getNextInterrupt will re-enable all interrupt sources
	// when it gets a new request for an interrupt reflection.
    return;
}


/*++

Routine Description:

    Called by the framework at DIRQL immediately after registering the ISR with the kernel
    by calling IoConnectInterrupt.

Return Value:

    NTSTATUS
--*/
NTSTATUS
EvtInterruptEnable( IN WDFINTERRUPT Interrupt,IN WDFDEVICE    Device)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INTERRUPT,"EvtInterruptEnable: Interrupt 0x%p, Device 0x%p\n", Interrupt, Device);

	WRITE_REGISTER_USHORT(&MemIoBase[PCIO_INT_SEL], ADAPTER_INT_EN);  // The application level takes care of enabling the secondary CABLE_INT level

	return STATUS_SUCCESS;
}


/*++

Routine Description:

    Called by the framework at DIRQL before De-registering the ISR with the kernel
    by calling IoDisconnectInterrupt.

Return Value:

    NTSTATUS
--*/
NTSTATUS
EvtInterruptDisable(IN WDFINTERRUPT Interrupt,IN WDFDEVICE    Device)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INTERRUPT, "EvtInterruptDisable: Interrupt 0x%p, Device 0x%p\n", Interrupt, Device);

	WRITE_REGISTER_USHORT(&MemIoBase[PCIO_INT_SEL], ALL_INT_DIS);

    return STATUS_SUCCESS;
}
