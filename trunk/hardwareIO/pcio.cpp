/*
 * Module:		$RCSfile: pcio.cpp $ - PCIO interface
 *				$Revision: 1.88 $
 *
 * Author:		john ulshoeffer
 *
 * Affiliation:	Teradyne, Inc.
 *
 * Copyright:	This  is  an  unpublished  work.   Any  unauthorized
 *				reproduction  is  PROHIBITED.  This unpublished work
 *				is protected by Federal Copyright Law.  If this work
 *				becomes published, the following notice shall apply:
 *
 *				(c) COPYRIGHT 1999, TERADYNE, INC.
 *
 *Discussion:
 *
 *
 *
 * Date:		December. 10, 1999
 *
 */

#include "stdafx.h"
#include <winioctl.h>

#include "ast.h"
#include "timer.h"

#include "hardware_enum.h"
#include "hardwareIO\utilities.h"
#include "hardwareIO\hardware.hpp"
#include "hardwareIO\pcio.hpp"

#include <sys/stat.h>
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

TODO(" Why optimize off ??");
#pragma optimize ("", off)

/* Debug help 
void myoutput(LPCTSTR string)
{
    TCEventData msg;

	msg.m_SendersName.Format("Output to OutputWindow");
	msg.m_Command.Format("%s\r\n", string);
	EMBroadcastEvent(AOI_OUTPUT_WINDOW_CLR, &msg);
	EMBroadcastEvent(AOI_OUTPUT_WINDOW, &msg);
}
*/

#define STATUS_SUCCESS  0x00000000L

static USHORT previousPage = 0x0;

/*******************************************************************************/
/*******************************************************************************/
/* TCPcio                                                                      */
/*******************************************************************************/
/*******************************************************************************/
TCPcio::TCPcio(void)
{
	InitializeCriticalSection(&csPcio);

	// pre-made cancel event
	m_CancelEvent.m_event = AOI_CANCEL;
	m_CancelEvent.m_SendersName = _T("TCMechIO: System Controller Communications Failure");
	m_CancelEvent.m_Command = _T("");
	m_CancelEvent.m_iRet = 0;
}

/*
 * Function:	InitializeTCPcio
 *
 * Description:	build this object (initialize)
 *
 * Parameters:
 *		none
 *
 * Return Values
 *		none
 *
 * Discussion:
 *		initialize the member variables,
 *
 */
void
TCPcio::InitializeTCPcio(TCHardwareIO *hardware)
{
	m_pHardware = hardware;

	// interrupt class pointers
	for (int i = 0; i < 32; i++)
		m_ClassPtr[i]  = NULL;

//	m_SC_Status                = SC_STATUS_NORMAL;
TODO("I don't think that this should touch AOI_IO member m_InterruptThreadStatus")
	m_InterruptThreadStatus    = FALSE;
	m_LtTimeoutEnabled         = FALSE; // 

	m_CallbackThread   = NULL;
	m_LtWatchdogThread = NULL;
}

/*
 * Function:	SendEvent
 *
 * Description:	broadcast an event via the EventManagerl
 *
 * Parameters:
 *		AOI_EVENT   -  event ID
 *		LPCSTR		-  Sender's ID
 *		int			-  return value
 *		LPCSTR		-  Command string
 *
 * Return Values:
 *		TSTATUS
 *
 * Discussion:
 *
 */
void
TCPcio::SendEvent(AOI_EVENT event, LPCTSTR Sender, int iRet, LPCTSTR Command)
{
	TCEventData msg;
	msg.m_SendersName = Sender;
	msg.m_iRet        = iRet;
	msg.m_Command     = Command;
	msg.m_event       = event;
	EMBroadcastEvent(event, &msg);
}

void
TCPcio::SendCancelEvent()
{
	EMBroadcastEvent(AOI_CANCEL, &m_CancelEvent);
}

void
TCPcio::SendAlert(AOI_EVENT event)
{
	SendEvent(event, _T("TCMechIO::SendAlert()"));
}

DWORD
TCPcio::InitBits(SYSTEMTYPE type)
{
	TCIO_Bits *ioBits = &m_IoBits[0x00];

	// initialize all ioBits to NULL
	for (int i = 0; i < MAX_IO; i++)
		(ioBits++)->Clear();

	// now initialize the used ioBits
	m_IoBits[IO_UNUSED_1].Initialize(IOBITS_INVALID);		// formerly WD_TIMEOUT
	m_IoBits[IO_UNUSED_2].Initialize(IOBITS_INVALID);		// formerly PRETIMEOUT

	// Light Tower
	m_IoBits[IO_TOWER_RED].Initialize(LT_RED);
	m_IoBits[IO_TOWER_YEL].Initialize(LT_YELLOW);
	m_IoBits[IO_TOWER_GREEN].Initialize(LT_GREEN);
	m_IoBits[IO_TOWER_BLUE].Initialize(LT_BLUE);
	m_IoBits[IO_TOWER_WHITE].Initialize(LT_WHITE);
	m_IoBits[IO_TOWER_FLASH].Initialize(LT_FLASH_DURATION);
	m_IoBits[IO_ALARM1].Initialize(LT_BEEPER1);
	m_IoBits[IO_ALARM2].Initialize(LT_BEEPER2);

	// fans
	m_IoBits[IO_AUXFAN0].Initialize(AUXFAN0);	//
	m_IoBits[IO_AUXFAN1].Initialize(AUXFAN1);	//
	m_IoBits[IO_AUXFAN2].Initialize(AUXFAN2);	//
	m_IoBits[IO_AUXFAN3].Initialize(AUXFAN3);	//
	m_IoBits[IO_AUXFAN4].Initialize(AUXFAN4);	//
	m_IoBits[IO_AUXFAN5].Initialize(AUXFAN5);	//
	m_IoBits[IO_AUXFAN6].Initialize(AUXFAN6);	//
	m_IoBits[IO_AUXFAN7].Initialize(AUXFAN7);	//

	// operator panel
	m_IoBits[IO_LOADBUTTON].Initialize(LOAD_DATA, pcioDIS);	//
	m_IoBits[IO_INSPECTBUTTON].Initialize(INSPECT_DATA, pcioDIS);	//
	m_IoBits[IO_EJECTBUTTON].Initialize(EJECT_DATA, pcioDIS);	//
	m_IoBits[IO_CANCELBUTTON].Initialize(CANCEL_DATA, pcioDIS);	//
	m_IoBits[IO_OPT1BUTTON].Initialize(OPT1_DATA, pcioDIS);	//
	m_IoBits[IO_OPT2BUTTON].Initialize(OPT2_DATA, pcioDIS);	//
	m_IoBits[IO_OPT3BUTTON].Initialize(OPT3_DATA, pcioDIS);	//
	m_IoBits[IO_AUX1BUTTON].Initialize(AUX1_DATA, pcioDIS);	//
	m_IoBits[IO_AUX2BUTTON].Initialize(AUX2_DATA, pcioDIS);	//
	m_IoBits[IO_AUX3BUTTON].Initialize(AUX3_DATA, pcioDIS);	//
	m_IoBits[IO_LOADLITE].Initialize(LOAD_LED);	// 
	m_IoBits[IO_INSPECTLITE].Initialize(INSPECT_LED);	//
	m_IoBits[IO_EJECTLITE].Initialize(EJECT_LED);	//
	m_IoBits[IO_CANCELLITE].Initialize(CANCEL_LED);	//
	m_IoBits[IO_OPT1LITE].Initialize(OPT1_LED);	//
	m_IoBits[IO_OPT2LITE].Initialize(OPT2_LED);	//
	m_IoBits[IO_OPT3LITE].Initialize(OPT3_LED);	//
	m_IoBits[IO_AUX1LITE].Initialize(AUX1_LED);	//
	m_IoBits[IO_AUX2LITE].Initialize(AUX2_LED);	//
	m_IoBits[IO_AUX3LITE].Initialize(AUX3_LED);	//

	// driver interrupts
	m_IoBits[IO_LEDDRV].Initialize(LED_DRV_DATA, pcioDIS);	//
	m_IoBits[IO_TOWERDRV].Initialize(LT_DRV_DATA, pcioDIS);	//
	m_IoBits[IO_V12DRV].Initialize(V12_DRV_DATA, pcioDIS);	//
	m_IoBits[IO_V24DRV].Initialize(V24_DRV_DATA, pcioDIS);	//

	// master interrupt status
	m_IoBits[IO_INT_ALL].Initialize(ALL_STATUS);        // all interrupts
	m_IoBits[IO_INTERLOCK].Initialize(INTERLOCK_DATA);	// system interlock

	// diagnostic interrupt
	m_IoBits[IO_DIAGNOSTIC].Initialize(DIAGNOSTIC_DATA, pcioDIS);

	// opto electronics
	// ConveyorBypass
	m_IoBits[IO_CONVYR_BYPASS].Initialize(CONVYR_BYPASS_DATA, pcioDIS, pcioIN);
	// Low Air Warning
	m_IoBits[IO_AIRINSTALLED].Initialize(AIRINSTALLED_DATA, pcioDIS, pcioIN);
	m_IoBits[IO_LOWAIR].Initialize(LOWAIR_DATA, pcioDIS, pcioIN);

	//motor staging clamper installed
	m_IoBits[IO_MOTORSTAGING_INSTALLED].Initialize(MOTORSTAGING_DATA, pcioDIS, pcioIN);

	// laser power
	m_IoBits[IO_LASER].Initialize(LASER_DATA, pcioDIS, pcioOUT);
	m_IoBits[IO_LASER_POLARITY].Initialize(LASER_POLARITY, pcioDIS, pcioOUT);

	// find the number of items in the array
	for (int i = m_NumberIO = 0; i < MAX_IO; i++)
	{
		/*
		if ((ioBits++)->GetAddress() != IOBITS_INVALID)
			m_NumberIO++;
		*///用另外的方法實做,暫不改變,JulianShen,20140604
TODO("Verify the counting of m_NumberIO ");
/*
// Julien: I can't read your comment above, but this code below does not count the NumberIO as it probably should
//		 m_NumberIO is always going to either be 0 or 0x100 depending on the constant index of 0 into the m_IoBits array below. 
//       and since ndx 0 is no IO_UNUSED1 m_NumberIO  will be 0 at the end of the loop
//		 Maybe you wanted to say m_IoBits[i].GetAddress(.....) 
		instead of:
		if (m_IoBits[0x00].GetAddress() != IOBITS_INVALID)
			m_NumberIO++;
*/
		if (m_IoBits[i].GetAddress() != IOBITS_INVALID)
			m_NumberIO++;
	}

	return PCIO_SUCCESS;
}

/*
 * Function:	PCIO_SetDigital
 *
 * Description:	put the PCIO into DIGITAL mode, This is the default mode after a HW reset
 *
 * Parameters:
 *		none
 *
 * Return Values:
 *		none
 *
 * Discussion: This is used to communicate with the WC140 controller
 *		
 *
 */
void
TCPcio::PCIO_SetDigital(void)
{
	// digital IO
	PCIO_Write(PCIO_STRB_CTRL, D_STRB);
}

/*
 * Function:	PCIO_SetAnalog
 *
 * Description:	put the PCIO into'Analog' strobe mode
 *
 * Parameters:
 *		none
 *
 * Return Values:
 *		none
 *
 * Discussion:	This is used to command the jtag chain on the WC140 controller
 *
 */
void
TCPcio::PCIO_SetAnalog(void)
{
	// analog IO
	PCIO_Write(PCIO_STRB_CTRL, A_STRB);
}

/*
 * Function:	PCIO_Reset
 *
 * Description:	resets the PCIO to a 'quite' state
 *
 * Parameters:
 *		none
 *
 * Return Values:
 *		DWORD		error code
 *
 * Discussion:
 *		
 *
 */
tPCIO_RET
TCPcio::PCIO_Reset(void)
{
	// Reset the IO card  -- this also sends a reset down the cable to the Wc140 
	PCIO_Write(PCIO_RESET, ON);	// ON == 3 resets the IO-card AND the controller.. However it doesn't boot the controller 
	Sleep(1);
	PCIO_Write(PCIO_RESET, OFF);
		
	PCIO_SetDigital(); // digital IO -- This is redundant because that's the reset condition -- left here for clarity

TODO("Why are we getting the controller version and THEN rebooting the controller. ");

// It seems the other way around would make more sense, if any.
// rebooting the controller is probably not necessary in the first place

	// determine if controller is there
	if ( GetControllerVersion() == 0)	
		return PCIO_OPEN_ERROR;

	PCIO_Write(CONTROLLER_RESET, 0);	// This re-boots the WC140 FPGA
	Sleep(CONTROLLER_BOOT_TIME);		// During the boot the WC140 can not be reached for any IO
	InitializeLtTower(cOFF, cOFF, cOFF);

	return PCIO_SUCCESS;
}

/*
 * Function:	GetPcioRevision
 *
 * Description:	return the PCIO Revision
 *
 * Parameters:
 *		none
 *
 * Return Values:
 *		none
 *
 * Discussion:
 *		return the integer in PCIO address 0x100
 *
 */
USHORT
TCPcio::GetPcioRevision(void) 
{
	USHORT data;
	PCIO_Read(PCIO_CONFIG, &data);
	return (data & PCIO_REVISION_MASK); 
}

/*
 * Function:	GetPcioVersion
 *
 * Description:	return a string that contains the PCIO Version
 *
 * Parameters:
 *		none
 *
 * Return Values:
 *		none
 *
 * Discussion:
 *		
 *
 */
bool
TCPcio::GetDriverVersion(CString &msg) 
{
	char	bufOutput[24] = {0,0,0,};	// Output from device

	if ( !  GetDriverIOCTLVersion (bufOutput, sizeof(bufOutput)) )
	{
		msg.Format("Driver IOCTRL \'Version\' ERROR: <0x%04x>.", GetLastError());
		UserMessage(msg,  _T("AOI Alert Message"),MB_SYSTEMMODAL | MB_ICONEXCLAMATION | MB_OK);
		return false;
	}

	msg.Format("%s", bufOutput);
	
	// check the device interface control version. If they don't match we can not continue to make 
	// any device IO calls and have to close the file handle immediately
	if ( strcmp(TERAPCIO_REVISION, bufOutput) != 0)
		return false;
		
	return true;
}

/*
 * Function:	PCIO_Open
 *
 * Description:	open the PCIO driver
 *
 * Parameters:
 *		none
 *
 * Return Values:
 *		DWORD		error code
 *
 * Discussion:
 *					an attempt is made to 'open' the TeraPCIO.sys device
 *					driver.  if successful the "io_map" pointer is 'mapped'
 *
 */
tPCIO_RET
TCPcio::PCIO_Open(SYSTEMTYPE type) 
{
	CString msg;
	m_SystemType = type;
		
	//stat = p.GetDeviceHandleByGUID();		 // This is an alternate way of getting a file handle of the driver -- Using a GUID --
											 // The driver provides both the GUID method and the straight symbolic link option
											 // Symbolic link is much easier to open and is the method used by the previous NT driver
	if (! GetDeviceHandleBySymLink() )		 // using the easier way
	{
		    TRACE("Device Driver failed to open! %s, Error:%u", DRIVER_SYMBOLIC_NAME_app, GetLastError());
			return PCIO_OPEN_ERROR;
	}

		
	// found driver -- Check IOcontrol Interface version 
	if ( GetDriverVersion(m_DriverRevision) == false )
	{
		// Can't continue if the driver interface version does not match 	
		TRACE0("Device Driver IOcontrol interface version did not match expected value. Failed to connect to driver!\n");
		Close_Device_Handle();
		return PCIO_OPEN_ERROR;
	}

	if ( ! GetMemMap( ) )		
	{
		Close_Device_Handle();
		return PCIO_OPEN_ERROR;
	}
	
	// reset device and reboot the WC140 controller
	if (PCIO_Reset() == PCIO_OPEN_ERROR)
	{
		if (m_pHardware->GetMode() != HARDWARE_FLASH)
		{
			Close_Device_Handle();
		}

		return PCIO_OPEN_ERROR;
	}

	// initialize IO_Bits
	InitBits(m_SystemType);
	return PCIO_SUCCESS;
}

/*
 * Function:	PCIO_Close
 *
 * Description:	open the PCIO driver
 *
 * Parameters:
 *		none
 *
 * Return Values:
 *		DWORD		error code
 *
 * Discussion:
 *					an attempt is made to 'close' the TeraPCIO.sys device
 *					driver.
 *
 */
tPCIO_RET
TCPcio::PCIO_Close(void) 
{
	TRACE0("PCIO_Close()\n");

	KillAllThreads(PCIO_ALL_THREADS);

	// close connection with driver
	Close_Device_Handle();

	return PCIO_SUCCESS;
}

USHORT
TCPcio::GetControllerVersion(void)
{ // this  needs to be changed to return the ENUM, see hardwareIO::Init
	
	PCIO_SetDigital();

	USHORT data = 0;
	PCIO_Read(CONTROL_TYPE, &data);

	USHORT eeprom = 0;
	m_ControllerSerialNumber = 0x0;
	m_ControllerRevisionPCA  = 0x0;
	m_ControllerRevisionPCB  = 0x0;

	if (data == 0x0140)
	{
		m_ControllerType = C51140;	
		PCIO_Read(CONTROL_FIRM_MAJOR, &m_ControllerFirmMajor);
		PCIO_Read(CONTROL_FRIM_MINOR, &m_ControllerFirmMinor);

		// Teradyne ID
		PCIO_ERead(CONTROL_EPROM_ID, &eeprom);
		if (eeprom != 0x5445) // 'T' 'E'
		{ // very old controller: can't be FLASHED
			//m_CanFlash             = FALSE; //F404 vc6版中已刪除
			m_CanFlash               = TRUE; // change for LX
			m_ControllerBoardNumber  = 0x05114000;
			m_ControllerSerialNumber = 0x0;
			m_ControllerRevisionPCA  = 0x0;//will show @.0
			m_ControllerRevisionPCB  = 0x0;//will show @.0
			//m_ControllerFirmMajor  = 0x0; //F404 vc6版中已刪除
			//m_ControllerFirmMinor  = 0x0; //F404 vc6版中已刪除
		}
		else
		{
			// indicate that SC can be Flashed;
			m_CanFlash              = FALSE;
			// board part number
			PCIO_ERead(CONTROL_EPROM_BD_MSB, &eeprom);
			m_ControllerBoardNumber  = eeprom << 16;
			PCIO_ERead(CONTROL_EPROM_BD_LSB, &eeprom);
			m_ControllerBoardNumber |= (eeprom & 0xffff);

			// board serial number
			PCIO_ERead(CONTROL_EPROM_SN_MSB, &eeprom);
			m_ControllerSerialNumber |= eeprom << 16;
			PCIO_ERead(CONTROL_EPROM_SN_LSB, &eeprom);
			m_ControllerSerialNumber |= (eeprom & 0xffff);

			// board ECO revision number
			PCIO_ERead(CONTROL_EPROM_RV_PCA, &eeprom);
			m_ControllerRevisionPCA = (DWORD) eeprom;
			PCIO_ERead(CONTROL_EPROM_RV_PCB, &eeprom);
			m_ControllerRevisionPCB = (DWORD) eeprom;
		}
		GetSysControllerBoardNumber();
		SetSysControllerSerialNumber();
		SetControllerFirmware();
		SetControllerPCA();
		SetControllerPCB();

	}	
	else
	{
		m_ControllerType = CUNKNOWN;
		data = 0;
	}
	return data;
}

DWORD
TCPcio::Controller_Init(HARDWAREMODE mode)
{
	DWORD id;
	// startup
//	m_SC_Status = SC_STATUS_NORMAL;

	// digital IO
	PCIO_SetDigital();
	
	// fans
	SetFans(0, ON);

	// light tower power & stuff
	InitializeLtTower(cOFF, cOFF, cOFF);

	SetLightTowerDriver(ON);
	SetLightFlashDuration(LIGHTFLASHDURATION);

	// panel lights
	SetPanelDrv(ON);

	// dome power
	SetDomePS(ON);
	
	if (mode == HARDWARE_ENABLED)
	{
		HANDLE hProcess = GetCurrentProcess();
		BOOL bResult = SetPriorityClass(hProcess, REALTIME_PRIORITY_CLASS);

		m_LtWatchdogThread = CreateThread(NULL, 0, &LtWatchdogThread, this, NULL, &id);
		Sleep(100);

		if (!m_LtWatchdogThread)
			return PCIO_INIT_ERROR;
		
		bResult = SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS);
		CloseHandle(hProcess);
	}

	return PCIO_SUCCESS;
}

/*
 * Function:	WatchdogThread
 *
 * Description:	interrupt worker thread
 *
 * Parameters:
 *		void *	ClassPtr
 *
 * Return Values:
 *		int		error code
 *
 * Discussion:
 *				
 *
 */

// System Watchdog stuff remvoed from this thread function -- Sys WD handled exclusively in the interrupt thread
DWORD WINAPI
TCPcio::LtWatchdogThread(void *arg)
{
// TODO: Raising the priority level off too many threads defeats the purpose. 
	// raise the priority of this thread
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

	// pointer to TCPcio object
	TCPcio * pPcio = (TCPcio *) arg;

	while (pPcio->m_KillThread != TRUE)
	{
TODO(" Sys WD reloaded in LtWatchdogThread every 250ms -- WHY?");
// I'm not sure why we also would want to relaod the system WD here as well
// This kind of defeats the purpose of the interrupt threads handling of the WD Pretimeout 
// However it should not cause any problems for the interrpt thread, except that it would never get 
// a PRE_TIMEOUT interrupt as long as this is running.

		pPcio->ReloadSysWatchdog(); // Feeds the watchdog by resetting the time again
	
		if (pPcio->m_pHardware->GetMode() != DIAGNOSTICS)
		{ // don't interfere with diagnostics
			// Alerts & Motors initialized
			if (pPcio->m_LtTimeoutEnabled == TRUE && pPcio->m_pHardware->MotorsInitialized())
			{
				// light tower watchdog timer
				// if the thread is alive and the AlertStatus is NORMAL, check it
				if (pPcio->m_KillThread != TRUE                               &&
					pPcio->m_pHardware->GetAlertStatus() == AOI_SYSTEM_NORMAL &&
					pPcio->m_TowerWatchDog-- <= 0)
				{ // 
					pPcio->SendEvent(AOI_SYSTEM_IDLE); //
				}
			}
		}
// TODO: 
// In order to end the thread properly this should be waiting on a event that can be signaled 
		Sleep(250); // 1/4 second loop  
	} // while running (pcio->m_KillThread != TRUE)

	CloseHandle(pPcio->m_LtWatchdogThread);
	pPcio->m_LtWatchdogThread = NULL;

	return NULL;
}

#ifdef NOT_USED_and_to_be_deleted 
/*
 * Function:	cbControllerWatchdogElapsed
 *
 * Description:	watchdog interrupt entry
 *
 * Parameters:
 *		void *	ClassPtr
 *		int     interrupt
 *
 * Return Values:
 *		none
 *
 * Discussion:
 *
 */

void CALLBACK
TCPcio::cbControllerWatchdogElapsed(void *classPtr, int interrupt)
{
	// pointer to TCPcio object
	TCPcio * pPcio = (TCPcio *) classPtr;

	// disable the watchdog
	pPcio->PCIO_Write(WD_ENABLE, (SHORT) 0x0);

	// clear the interrupt
	pPcio->PCIO_Write(WD_STATUS, (SHORT) 0x0);
	pPcio->PCIO_Write(WD_MASK,   (SHORT) 0x0);

	if (pPcio->m_SC_Status != SC_STATUS_NORMAL)
	{
		// kill the WatchDog Thread
		if (pPcio->m_LtWatchdogThread != NULL)
		{
			TRACE("ControllerWatchdog: Terminating WatchDog Thread\n");

			TerminateThread(pPcio->m_LtWatchdogThread, 0);
			CloseHandle(pPcio->m_LtWatchdogThread);
			pPcio->m_LtWatchdogThread = NULL;
		}

		// set error status to "MajorError"
		pPcio->SendAlert(AOI_MAJORERROR);

		//	cancel
		pPcio->SendCancelEvent();

		UserMessage(_T("System Controller watchdog timeout"),
			_T("AOI Alert Message"), MB_SYSTEMMODAL | MB_ICONEXCLAMATION | MB_OK);
	}
}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// TODO: Which one of these two identical functions is used ??
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#pragma message (" Watchdog timeout handling confused, use one or the other ControllerWatchdogElapsed()")
void
TCPcio::ControllerWatchdogElapsed(void)
{
	DisableSysWatchdog();

	if (m_SC_Status != SC_STATUS_NORMAL)
	{
		// kill the WatchDog Thread
		if (m_LtWatchdogThread != NULL)
		{
			TRACE("ControllerWatchdog: Terminating WatchDog Thread\n");

			TerminateThread(m_LtWatchdogThread, 0);
			CloseHandle(m_LtWatchdogThread);
			m_LtWatchdogThread = NULL;
		}

		// set error status to "MajorError"
		SendAlert(AOI_MAJORERROR);

		//	cancel
		SendCancelEvent();

		MessageBox(NULL,
			"System Controller watchdog timeout",
			_T("AOI Alert Message"), MB_SYSTEMMODAL | MB_ICONEXCLAMATION | MB_OK);
	}
}

void CALLBACK
TCPcio::cbControllerPreWatchdog(void *classPtr, int interrupt)
{
	// pointer to TCPcio object
	TCPcio * pPcio = (TCPcio *) classPtr;
	pPcio->ReloadSysWatchdog();
}
#endif
void
TCPcio::EnableLtWatchdog(int timer)
{
	m_LtTimeoutEnabled = TRUE;

	// timer == # of seconds, but used in a 1/4 second loop, make X4
	m_TowerWatchDog    = timer * 4;
}

void
TCPcio::InitializeLtTower(ONOFF red, ONOFF yellow, ONOFF green)
{
	SetLightTowerRed(red);
	SetLightTowerYellow(yellow);
	SetLightTowerGreen(green);
}

/*
 * Function:	Controller_ioRead
 *
 * Description:	read the data bit from the designated IO bit
 *
 * Parameters:
 *		none
 *
 * Return Values:
 *		DWORD		error code
 *
 * Discussion:
 *					
 *
 */
DWORD
TCPcio::Controller_ioRead(TCIO_Bits *iobits, USHORT *data)
{
	// digital IO
	PCIO_SetDigital();

	PCIO_Read(iobits->GetAddress(), data);

	return PCIO_SUCCESS;
}

/*
 * Function:	PCIO_ERead	- read from EEPROM
 *
 * Description:	read the data from the designated pcio address(es)
 *
 * Parameters:
 *		SHORT	  pcio (iomapped) address
 *		SHORT	* pointer to the data buffer
 *
 * Return Values:
 *		DWORD		error code
 *
 * Discussion:
 *					
 *
 */

DWORD
TCPcio::PCIO_ERead(SHORT addr, USHORT *data) 
{
	// disable EEPROM writes
	USHORT eeprom = 0x68;
	SpectrumSleep(50);
	PCIO_Write(CONTROL_EPROM_WRITE, eeprom);

	if (addr >= 0x400 && addr < 0x440)
	{
		USHORT page = previousPage = 1;
		// write to the page register
		PCIO_Write(REG_PAGE, page);

		// read twice, allows EEPROM to respond
		SpectrumSleep(50);
		PCIO_Read(addr, &eeprom);
		SpectrumSleep(30);
		return PCIO_Read(addr, data);
	}
	else
		return PCIO_READ_ERROR;
}

/*
 * Function:	Controller_ioWrite
 *
 * Description:	write the data bit to the designated IO bit
 *
 * Parameters:
 *		DWORD	  pcio (iomapped) address
 *		SHORT	* pointer to the data buffer
 *		DWORD	  number of address (words) to written
 *
 * Return Values:
 *		DWORD		error code
 *
 * Discussion:
 *					
 *
 */
DWORD
TCPcio::Controller_ioWrite(TCIO_Bits *iobits, USHORT data)
{
	// digital IO
	PCIO_SetDigital();

	return PCIO_Write(iobits->GetAddress(), data);
}

/*
 * Function:	PCIO_EWrite	- write to EEPROM
 *
 * Description:	write the data to the designated pcio address(es)
 *
 * Parameters:
 *		SHORT	  pcio (iomapped) address
 *		SHORT	* pointer to the data buffer
 *
 * Return Values:
 *		DWORD		error code
 *
 * Discussion:
 *					
 *
 */
DWORD
TCPcio::PCIO_EWrite(SHORT addr, USHORT *data) 
{
	if (addr >= 0x400 && addr < 0x440)
	{
		USHORT page = previousPage = 1;
		// write to the page register
		PCIO_Write(REG_PAGE, page);

		// read three times, allows EEPROM to respond
		SpectrumSleep(50);
		return PCIO_Write(addr, *data);
	}
	else
		return PCIO_READ_ERROR;
}

DWORD
TCPcio::PCIO_EWriteEnable(void) 
{
	// disable EEPROM writes
	USHORT eeprom = 0x34;
	SpectrumSleep(50);
	PCIO_Write(CONTROL_EPROM_WRITE, eeprom);
	return PCIO_SUCCESS;
}

/*
 * Function:	ClearControllerInterruptStatus
 *
 * Description:	clear the Interrupt Status Register
 *
 * Parameters:
 *		DWORD	  pcio (iomapped) address
 *		SHORT	* pointer to the data buffer
 *
 * Return Values:
 *		DWORD		error code
 *
 * Discussion:
 *					
 *
 */
DWORD
TCPcio::ClearControllerInterruptStatus(TCIO_Bits *iobits)
{

	// digital IO
	PCIO_SetDigital();

	PCIO_Write(iobits->GetAddress() + STATUS_ADDR,  0x0);

	return PCIO_SUCCESS;
}

/*
 * Function:	SetController_ioInterrupt
 *
 * Description:	write the Interrupt Enable Register
 *
 * Parameters:
 *		DWORD	  pcio (iomapped) address
 *		SHORT	* pointer to the data buffer
 *
 * Return Values:
 *		DWORD		error code
 *
 * Discussion:
 *					
 *
 */
DWORD
TCPcio::SetController_ioInterrupt(TCIO_Bits *iobits, USHORT *data)
{
	// digital IO
	PCIO_SetDigital();

	// write interrupt enable bit
	PCIO_Write(iobits->GetAddress() + ENABLE_ADDR, *data);

	return PCIO_SUCCESS;
}

/*
 * Function:	GetController_ioInterrupt
 *
 * Description:	Read the Interrupt Enable Register
 *
 * Parameters:
 *		DWORD	  pcio (iomapped) address
 *		SHORT	* pointer to the data buffer
 *
 * Return Values:
 *		DWORD		error code
 *
 * Discussion:
 *					
 *
 */
DWORD
TCPcio::GetController_ioInterrupt(TCIO_Bits *iobits, USHORT *data)
{
	USHORT rdata;

	// digital IO
	PCIO_SetDigital();

	// read interrupt enable bit
	PCIO_Read(iobits->GetAddress() + ENABLE_ADDR, &rdata);
	*data = rdata ? 1 : 0;

	return PCIO_SUCCESS;
}
	
DWORD
TCPcio::DisableController_ioInterrupt(TCIO_Bits *iobits)
{
	USHORT data = 0x0;
	return SetController_ioInterrupt(iobits, &data);
}

DWORD
TCPcio::EnableController_ioInterrupt(TCIO_Bits *iobits)
{
	USHORT data = 0x1;
	return SetController_ioInterrupt(iobits, &data);
}

DWORD
TCPcio::SetInterruptPolarity(TCIO_Bits *iobits, int polarity)
{
	// digital IO
	PCIO_SetDigital();

	PCIO_Write(iobits->GetAddress() + ACTION_ADDR, polarity);

	return PCIO_SUCCESS;
}

DWORD
TCPcio::TriggerController_ioInterrupt(TCIO_Bits *iobits)
{
	// digital IO
	PCIO_SetDigital();

	// write interrupt status bit
	PCIO_Write(iobits->GetAddress() + STATUS_ADDR, (USHORT) 0xffff);

	return PCIO_SUCCESS;
}

int
TCPcio::GetLightTowerDriver(void)
{
	USHORT onoff;
	PCIO_Read(LT_DRIVERS, &onoff);
	return (int) onoff;
}

int
TCPcio::GetLightFlashDuration(void)
{
	USHORT msec;
	PCIO_Read(LT_FLASH_DURATION, &msec);
	return (int) msec;
}

int
TCPcio::GetFans(int fan)
{
	USHORT onoff;
	PCIO_Read(AUXFAN0 + (4 * fan), &onoff);
	return (int) onoff;
}

int
TCPcio::GetPanelDrv(void)
{
	USHORT onoff;
	PCIO_Read(PANEL_LED_DRV, &onoff);
	return (int) onoff;
}

void
TCPcio::SetDomePS(int onoff, double voltage)
{ // default voltage = 10.0
	SHORT ps_voltage = (SHORT)((voltage * 228.567) - 701.833);

	if (onoff && voltage >= 5.0)
	{ // ON
		PCIO_Write(DAC_ONE,   ps_voltage);
		PCIO_Write(PS_LOADEN, (SHORT) 0);
		PCIO_Write(PS_DATA,   (SHORT) 0);
	}
	else
	{ // OFF
		PCIO_Write(DAC_ONE,   (SHORT) 0);
		PCIO_Write(PS_DATA,   (SHORT) 1);
		PCIO_Write(PS_LOADEN, (SHORT) 1);
	}
}

/*******************************************************************************/
/* DOME                                                                        */
/*******************************************************************************/


/*
 * Structure:	Dome
 *
 * Description:	contains the parameter for each IO bit
 *
 * Discussion:
 *				DOME:	int word;
 *
 */
static DOME Dome ={0};

DOME *
TCPcio::getDomePtr (void)
{
	return &Dome;
}

/*
 * Function:	Dome_ioRead
 *
 * Description:	read the data bit to the DomeController
 *
 * Parameters:
 *		DWORD	  dome controller address
 *		SHORT	* pointer to the data buffer
 *		DWORD	  number of address (words) to written
 *
 * Return Values:
 *		DWORD		error code
 *
 * Discussion:
 *					
 *
 */

DWORD
TCPcio::Dome_ioRead(SHORT address, USHORT *data, SHORT count)
{
	USHORT *dataPtr =  data;

	// digital IO
	PCIO_SetDigital();

	// dome address range (0x2000 or greater)
	if (address < 0x2000)
		return PCIO_Read(address, data);

	if (count < 1)
		count = 1;

	for (SHORT i = 0; i < count; i++)
	{
		SHORT page = (address + i) / 0x0400;
		if (page != previousPage)
		{	// write to the page register only if necessary
			PCIO_Write(REG_PAGE, page);
			previousPage = page;
		}

		// compute the ram address & set the page bit
		SHORT mAddr = ((address + i) % 0x0400) | 0x0400;
		PCIO_Read(mAddr, dataPtr++);
	}

	return PCIO_SUCCESS;
}

/*
 * Function:	Dome_ioWrite
 *
 * Description:	write the data bit to the DomeController
 *
 * Parameters:
 *		DWORD	  dome controller address
 *		SHORT	* pointer to the data buffer
 *		DWORD	  number of address (words) to written
 *
 * Return Values:
 *		DWORD		error code
 *
 * Discussion:
 *					
 *
 */
DWORD
TCPcio::Dome_ioWrite(SHORT address, USHORT data)
{
	USHORT word = data;
	return Dome_ioWrite(address, &word, 1);
}

DWORD
TCPcio::Dome_ioWrite(SHORT address, USHORT *data, SHORT count)
{
	USHORT *dataPtr = data;

	// digital IO
	PCIO_SetDigital();

	// dome address range (0x2000 or greater)
	if (address < 0x2000)
		return PCIO_Write(address, *data);

	for (SHORT i = 0; i < count; i++)
	{
		USHORT page = (address + i) / 0x0400;
		if (page != previousPage)
		{	// write to the page register only if necessary
			PCIO_Write(REG_PAGE, page);
			previousPage = page;
		}

		// compute the ram address & set the page bit
		SHORT mAddr = ((address + i) % 0x0400) | 0x0400;
		PCIO_Write(mAddr, *dataPtr);
		dataPtr++;
	}
	return PCIO_SUCCESS;
}

int
TCPcio::ReadDomeXPosition(void)
{
	int lsb, msb;

	PCIO_Read(CURRENT_X_POS_1, (USHORT *) &lsb);
	PCIO_Read(CURRENT_X_POS_2, (USHORT *) &msb);

	return ~(((msb & 0x0000ffff) << 16) | (lsb & 0x0000ffff));
}

int
TCPcio::ReadDomeYPosition(void)
{
	int lsb, msb;
	PCIO_Read(CURRENT_Y_POS_1, (USHORT *) &lsb);
	PCIO_Read(CURRENT_Y_POS_2, (USHORT *) &msb);

	return ((msb & 0x0000ffff) << 16) | (lsb & 0x0000ffff);
}

void
TCPcio::WriteDomeXPosition(int &position)
{
	SHORT word1 = (SHORT) position & 0x0000ffff;
	SHORT word2 = (SHORT) ((position & 0xffff0000) >> 16);

	PCIO_Write(CURRENT_X_POS_1, word1);
	PCIO_Write(CURRENT_X_POS_2, word2);
}

void
TCPcio::WriteDomeYPosition(int &position)
{
	SHORT word1 = (SHORT) position & 0x0000ffff;
	SHORT word2 = (SHORT)((position & 0xffff0000) >> 16);

	// add scan direction . . . .
	PCIO_Write(CURRENT_Y_POS_1, word1);
	PCIO_Write(CURRENT_Y_POS_2, word2);
}

#ifdef Checkers_unneeded_timerFunctions 
/*******************************************************************************/
/* TCPcio timer functions                                                      */
/*******************************************************************************/
/*
 * Function:	LoadTimer
 *
 * Description:	start the timer
 *
 * Parameters:
 *		UINT	timer 1, 2 or 3
 *
 * Return Values:
 *		DWORD	error code
 *
 * Discussion:
 *					
 *
 */
DWORD
TCPcio::LoadTimer(UINT timer, DWORD time) 
{ // diagnostics uses this only -- has no benefit 
	if (timer < 1 || timer > 3)
		return PCIO_TIMER_ERROR;

	AOITIME starttime;
	starttime.time_us = time;

	// lower word of timer
	PCIO_Write(LOWTIMERBASE  + (timer -1), starttime.time.low);
	// upper word of timer
	PCIO_Write(HIGHTIMERBASE + (timer -1), starttime.time.high) ;

	return PCIO_SUCCESS;
}

/*
 * Function:	StartTimer
 *
 * Description:	start the timer
 *
 * Parameters:
 *		UINT	timer 1, 2 or 3
 *
 * Return Values:
 *		DWORD	error code
 *
 * Discussion:
 *					
 *
 */
DWORD
TCPcio::StartTimer(UINT timer, DWORD time) 
{ // diagnostics uses this only -- has no benefit 
	if (timer < 1 || timer > 3)
		return PCIO_TIMER_ERROR;

	// if running,
	PCIO_Write(TIMER_BASE_CTRL + (timer -1), STOP_TIMER);

	LoadTimer(timer, time);

	// start the timer
	PCIO_Write(TIMER_BASE_CTRL + (timer -1), RUN_TIMER);

	return PCIO_SUCCESS;
}

/*
 * Function:	StopTimer
 *
 * Description:	stop the timer
 *
 * Parameters:
 *		UINT	timer 1, 2 or 3
 *
 * Return Values:
 *		DWORD	error code
 *
 * Discussion:
 *					
 *
 */
DWORD
TCPcio::StopTimer(UINT timer) 
{
	// diagnostics uses this only -- has no benefit 
	if (timer < 1 || timer > 3)
		return PCIO_TIMER_ERROR;

	// stop the timer
	PCIO_Write(TIMER_BASE_CTRL + (timer -1), STOP_TIMER);

	return PCIO_SUCCESS;
}

/*
 * Function:	ReadTimer
 *
 * Description:	stop the timer
 *
 * Parameters:
 *		UINT	timer 1, 2 or 3
 *
 * Return Values:
 *		DWORD	error code / timer
 *
 * Discussion:
 *					
 *
 */
DWORD
TCPcio::ReadTimer(UINT timer) 
{	// diagnostics uses this only -- has no benefit 
	if (timer != 1 && timer != 2 && timer != 3)
		return PCIO_TIMER_ERROR;

	USHORT low = 0;
	PCIO_Read(LOWTIMERBASE + (timer -1), &low);
	USHORT high = 0;
	PCIO_Read(HIGHTIMERBASE + (timer -1), &high);

	return((DWORD) ((high << 16) | low));
}

/*
 * Function:	AOI_Sleep
 *
 * Description:	Sleep for 'uSec' microseconds
 *
 * Parameters:
 *		UINT	uSec
 *
 * Return Values:
 *		DWORD	error code
 *
 * Discussion:
 *				uses Timer 3	
 *
 */

// this is to appease Diagnostics  simply use a SpectrumSleep 
DWORD
TCPcio::AoiSleep(UINT uSec)
{   // diagnostics uses this only -- has no benefit 
	// checkers calls this, we use a Spectrum sleep to satisfy the checkers request 
	// Timer on the IO card are never used and are not implemented therefore in the DD
	SpectrumSleep(uSec);
	
	return PCIO_SUCCESS;
}
TODO(" Remove the unnecessary timer functions in Checkers and here");
#endif 
/*
 * Function:	RegisterAoiInterruptCallback
 *
 * Description:	register the interrupt callback function (all interrupts)
 *
 * Parameters:
 *		void *	ClassPtr
 *
 * Return Values:
 *		int		error code
 *
 * Discussion:
 *				store the 'callback' function pointers	
 *
 */
int
TCPcio::RegisterAoiInterruptCallback(int interrupt, void * ClassPtr, PCIO_HANDLER_PTR func)
{
	m_ClassPtr[interrupt]          = ClassPtr;
	m_InterruptCallback[interrupt] = func;
	return NULL;
}
/*
 * Function:	UnRegisterAoiInterruptCallback
 *
 * Description:	unregister the interrupt callback function (all interrupts)
 *
 * Parameters:
 *		void *	ClassPtr
 *
 * Return Values:
 *		int		error code
 *
 * Discussion:
 *				null the 'callback' function	
 *
 */
int
TCPcio::UnRegisterAoiInterruptCallback(int interrupt)
{
	m_ClassPtr[interrupt]          = (void *) NULL;
	m_InterruptCallback[interrupt] = (PCIO_HANDLER_PTR) NULL;
	return NULL;
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

/*
 * Function:	StartInterruptThread
 *
 * Description:	Enable of Diaable PCI/O interrupts
 *
 * Parameters:
 *		none
 *
 * Return Values:
 *		int		error code
 *
 * Discussion:
 *				creates a worker thread to 'wait' on PCI/O interrupts	
 *
 */
int
TCPcio::StartInterruptThread(void)
{
	 return AOI_IO::StartInterruptThread(this);
}

/*
 * Function:	InterruptThread
 *
 * Description:	interrupt worker thread
 *
 * Parameters:
 *		void *	ClassPtr
 *
 * Return Values:
 *		int		error code
 *
 * Discussion:
 *				wait on PCI/O interrupts
 *
 */

DWORD WINAPI
TCPcio::InterruptThread(void *arg) // entry for interrupt thread
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

	for (int i = 0; i < pPcio->m_NumberIO; i++)
	{ // clear all potential interrupts in the controller itself
		if (pPcio->m_IoBits[i].GetInterruptEnable() != pcioNA)
		{
			pPcio->PCIO_Write(pPcio->m_IoBits[i].GetAddress() + ENABLE_ADDR, (SHORT) 0x00);
			pPcio->PCIO_Write(pPcio->m_IoBits[i].GetAddress() + STATUS_ADDR, (SHORT) 0x00);
		}
	}

	pPcio->m_InterruptThreadStatus = TRUE; // indicating that the thread is off and running

TODO("once the WD functionality has been established this ifdef can be enabled "); 
//#ifndef _DEBUG   
	// Having the system watchdog enabled while the Visual Studio debugger stops the application is not desired,
	// it makes harware debugginng impossible when the system controller re-boots after its timeout 
	pPcio->StartSysWatchdog();	 // enables interrupts on controller and lets it run
#pragma message("Caution while debugging -- Watchdog Timer function is enabled! Stopping in debugger will reboot WC140 in <4seconds" )
//#else
//#pragma message( "Caution -- Watchdog Timer function is not running in DEBUG build!!" )
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
		//						not ready to operate, while booting for example.
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
			{	// This should not ever happen -- coding sequence error if it does.. i.e. Device IO call with closed handle 
				CString errorMsg;
				errorMsg.Format("PCIO Interrupt failure (0x%04x)",	err_code);
				MessageBox(NULL, errorMsg, _T("AOI Alert Message"),	MB_SYSTEMMODAL | MB_ICONSTOP | MB_OK);
				goto thread_exit;
			}
		}
		else
		{	
			// This should not happen in this scope with the file handle opened using FILE_FLAG_OVERLAPPED
			// It means that the DeviceIoControl executed syncronously and was therefore blocking. 
			// If it does, we can't properly terminate this thread because we are not waiting for Multiple objects above
			// abd can only poll the m_hKillInterruptThreadEvt once after we come out of the blocking  DeviceIoControl().
			TRACE("get-next-interrupt DeviceIoControl call executed syncronously unexpectedly !!\n");

			if ( WaitForSingleObject(pPcio->m_EndInterruptThreadEvt,0) == WAIT_OBJECT_0)	// poll m_hKillInterruptThreadEvt
				goto thread_exit;
		}

		/* If we get here we have a valid GetNextInterrupt response to process */

		// This is the 2us comm timeout the adapter generated because somebody is talking to WC140 but its not handshaking.
		// This happens if the cable became disconnected, the controller FPGA 'locked up, power to WC140 went off, or the controller is reset and rebooting
		// A controller boot happens automatically on a watchdog timeout -- hardware logic does it.

		if (interrupt.type & INT_2us)		 
		{ 
			TRACE("System Communication Failure\n");
			// flag all motor & conveyor controls to ignore system commands
			pPcio->m_pHardware->SetMode(SOFTWARE_ONLY);
		
			// This terminates the m_CallbackThread and mControllerWatchdogThread and set mKillThread to true
			pPcio->KillAllThreads(PCIO_THREADS);  // kills all except this thread -- we simply exit gracefully
			
			// CANCEL
			pPcio->SendCancelEvent();

// HUH, how would a 2us timeout automatically be an EMO event? Does EMO shut off power and therefore the WC140 fails to communicate??
// This might have been a work-around in the prior code for the failure to wait for a rebooting WC140 after WD timeout.
// Prior code caused the 2us timeout by re-reading the WC140 interrupt status while it was booting instead of using the 
// status provided by the Device IO control return. The AOI_IO driver supresses a false 2us interrupt when the Sys WD  has timed 
// out and the WC140 is rebooting and is only reporting SYS_TIMEOUT_73 in interrupt.status.

			//			pPcio->m_SC_Status = SC_STATUS_EMO;
TODO("This is confusing to me, What does interlock tripped have to do with a 2us timeout ?");
			if (pPcio->m_pHardware->GetInitializationFlag() == FALSE)
			{ 
				pPcio->SendEvent(AOI_HW_INTERLOCK, _T("TCMechIO: Interlock Tripped"),1, SC_COMFAILURE);
				// allow this thread to die, it will be restarted when the hardware is reinitialized
				goto thread_exit;
			}
			else
			{ // we were initializing
TODO("Int2us handling doesn't make sense regarding InitializationFlag");
//  I don't understand why we would continue this thread in this situation after terminating the other two threads above
//	and calling SendCancelEvent. 
//	If this is somehow an expected situation during  InitializationFlag==TRUE then this check should 
//	be done before we send Cancel and Kill

				TRACE("System Communication Failure (initialization)\n");
			}

		} // 2us communication timeout

TODO("This was else if(), But it is possible that both a timeout and controller interrupt could be present here at the same time. "); 
//      else if (interrupt.type & INT_Dig)		
		if (interrupt.type & INT_Dig)			// digital interrupt from System controller 
		{
TODO("The code in DigitalCallBackthread is using m_Interrupt to evaluate the nature of the interrpt-- see problem descrition below");
			pPcio->m_Interrupt= interrupt.status;
			// The Watchdog needs feeding NOW !
			if (interrupt.status & SYS_PRE_TIMEOUT_73)  
			{ 
				pPcio->ReloadSysWatchdog(); // Feeds the watchdog by resetting the time again to n seconds and clearing the PRE Time-out interrupt 
				TRACE("I fed the dog\n");
			}
			// The actual Watchdog timed out and is rebooting the controller now 
			// This takes about 200ms and we need to wait 500ms before we start talking to it again
			// again, or else we get continued 2us time-outs on every io 

			else if (interrupt.status & SYS_TIMEOUT_73)			
			{
				TRACE("Watchdog timeout happened! Waiting 500ms for the controller to recover \n");
			
				// This Wait type allows the thread to be canceled during the sleep as well
				WaitForSingleObject(pPcio->m_EndInterruptThreadEvt,CONTROLLER_BOOT_TIME); 

// TODO: I don't understand what the test below signifies. This code was inlined from TCPcio::ControllerWatchdogElapsed(void)
// Why would we only process the catastrophic timeout and controller reboot event when m_SC_Status != SC_STATUS_NORMAL,
// i.e. pPcio->m_SC_Status == SC_STATUS_EMO
// 
				//                     == SC_STATUS_EMO
//				if (pPcio->m_SC_Status != SC_STATUS_NORMAL)  // This really is: 'if (pPcio->m_SC_Status == SC_STATUS_EMO)' 
TODO("Condition m_SC_Status removed for WD timedout handling-- WC140 rebooted and we can't continue"); 
				{
					pPcio->KillAllThreads(PCIO_THREADS);  // kills all except this thread -- we simply exit gracefully
				
// TODO: do we need to flag all motor & conveyor controls to ignore system commands ??
					// pPcio->m_pHardware->SetMode(SOFTWARE_ONLY);

					// set error status to "MajorError"
					pPcio->SendAlert(AOI_MAJORERROR);

					//	cancel
					pPcio->SendCancelEvent();

					MessageBox(NULL,
						"System Controller watchdog timeout",
						_T("AOI Alert Message"), MB_SYSTEMMODAL | MB_ICONEXCLAMATION | MB_OK);

					goto thread_exit;
				}
			}
			else // Process all the rest of the interrupts in a seperate thread
			{
// TODO: The code in DigitalCallBackthread is using m_Interrupt set above to evaluate the nature of the interrpt-- 
//		 This is problematic since it is possible that a new interrupt will occur while the processing thread is still running on the
//		 last one and the last status will be overwritten -- loosing interrupts 
//		 A queue for the interrupt status seems to be in order.

				// process the controller interrupts via a new thread 
				if (pPcio->m_ClassPtr[0] != NULL) // at least one 'callback'
				{

					DWORD id;
					// start the thread to process this interrupt
					pPcio->m_CallbackThread = CreateThread(NULL, 0,	&DigitalCallbackThread, pPcio, NULL, &id);
				}
			}
		}// digital interrupt 
		else
		{	// This should not ever happen -- coding error if it does.
			// No other interrupt sources should be reflected up to here.
			CString msg;
			msg.Format("Unknown PCI/O Interrupt: <0x%x>",interrupt.type);
			MessageBox(NULL, msg, _T("AOI Alert Message"),MB_SYSTEMMODAL | MB_ICONSTOP | MB_OK);
		}
	} 

	thread_exit:
	TRACE("Interrupt thread exited gracefully \n");
		
	if (!pPcio->m_InterruptThreadExitWith_WD_running)
		pPcio->DisableSysWatchdog(); 	// The WD must be disabled before exiting this thread or else it will reboot the WC140 if it was enabled

	CloseHandle(hIOCompletionEvt);
	CloseHandle(pPcio->m_EndInterruptThreadEvt);
	pPcio->m_EndInterruptThreadEvt = NULL;
	CloseHandle(pPcio->m_InterruptThreadHdl);
	pPcio->m_InterruptThreadHdl = NULL;
	pPcio->m_InterruptThreadStatus = FALSE;		// This indicates the thread has completed.
	return err_code;
}

void
TCPcio::KillAllThreads(int thread)
{
/*
	This is called from three places:

 1.) from within the interrupt thread upon a 2us communnication timeout interrupt event, but
     the thread argument is not set to PCIO_ALL_THREADS, so the interrupt thread is not forcefully 
	 terminated but instead exists gracefully when this function returns to it.


 2.) from harware.cpp TCHardwareIO::Close() with PCIO_ALL_THREADS. This causes the
     the Interrupt thread to be exited.  

  3.)  from PCIO_Close() 
*/
TODO("Item 2 and 3 seem to do the same thing --  see  TODO comment in TCHardwareIO::Close()");

	m_KillThread = TRUE;	// This affects CallBackThread and LtWatchdogThread only

TODO("Since the LT Watchdog thread sleeps for 250ms at a time it's very unlikely that the thread can exit cleanly");
//  An implementation using an event and WaitForSingleObject instead of a sleep is needed here.
//	Sleep(500);	// Must yield for longer than any sleep in threads to give the other threads some time to exit 
// TODO: better yet wait on thread handles to exit cleanly 

	if (m_CallbackThread != NULL)
	{
		TerminateThread(m_CallbackThread, 0);		// It is generally a bad idea to kill a thread in cold blood 
		CloseHandle(m_CallbackThread);
		m_CallbackThread = NULL;
	}

	if (m_LtWatchdogThread != NULL)
	{
		TerminateThread(m_LtWatchdogThread, 0);		// It is generally a bad idea to kill a thread 
		CloseHandle(m_LtWatchdogThread);
		m_LtWatchdogThread = NULL;
	}

	if (thread == PCIO_ALL_THREADS )
	{
TODO("if you want the WC140 to reboot everytime the harware is closed, change to true below");
		EndInterruptThread( false);	// false: stops the WD function so that WC140  will not reboot
		//EndInterruptThread( true);// true: leaves the WD running will reboot WC140 after timeout
	}

}

DWORD WINAPI
TCPcio::DigitalCallbackThread(void *arg)
{ // entry for callback thread
	// pointer to TCPcio object
	TCPcio * pPcio = (TCPcio *) arg;

	// raise the priority of this thread
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

	// system interlock
	if (pPcio->m_Interrupt & SYSINTERLOCK_73)
		pPcio->m_InterruptCallback[SYSINTERLOCK](pPcio->m_ClassPtr[SYSINTERLOCK],
			pPcio->m_Interrupt);

	// system conveyor bypass
	if (pPcio->m_Interrupt & OPTO1_73)
	{
		int opto1interrupt;
		if ((opto1interrupt = pPcio->GetOpto1Interrupt()) == 0x00)
			pPcio->ClearAllInterrupt_140();
		else
		{
			if (opto1interrupt & CONVEYORBYPASS_73)
				pPcio->m_InterruptCallback[CONVEYORBYPASS](pPcio->m_ClassPtr[CONVEYORBYPASS],
					opto1interrupt);
			if (opto1interrupt & LOWAIR_73)
				pPcio->m_InterruptCallback[LOWAIR](pPcio->m_ClassPtr[LOWAIR],
					opto1interrupt);
		}
	}

	if (pPcio->m_Interrupt & PANEL_73)
	{
		int panelinterrupt;
		if ((panelinterrupt = pPcio->GetPanelInterrupt()) == 0x00)
			pPcio->ClearAllInterrupt_140();
		else
		{ // test for panel interrupts

			if (panelinterrupt & CANCELBUTTON_73)
				pPcio->m_InterruptCallback[CANCELBUTTON](pPcio->m_ClassPtr[CANCELBUTTON],
					panelinterrupt);

			if (panelinterrupt & LOADBUTTON_73)
				pPcio->m_InterruptCallback[LOADBUTTON](pPcio->m_ClassPtr[LOADBUTTON],
					panelinterrupt);

			if (panelinterrupt & INSPECTBUTTON_73)
				pPcio->m_InterruptCallback[INSPECTBUTTON](pPcio->m_ClassPtr[INSPECTBUTTON],
					panelinterrupt);

			if (panelinterrupt & EJECTBUTTON_73)
				pPcio->m_InterruptCallback[EJECTBUTTON](pPcio->m_ClassPtr[EJECTBUTTON],
					panelinterrupt);

			if (panelinterrupt & OPT1BUTTON_73)
				pPcio->m_InterruptCallback[OPT1BUTTON](pPcio->m_ClassPtr[OPT1BUTTON],
					panelinterrupt);

			if (panelinterrupt & OPT2BUTTON_73)
				pPcio->m_InterruptCallback[OPT2BUTTON](pPcio->m_ClassPtr[OPT2BUTTON],
					panelinterrupt);

			if (panelinterrupt & OPT3BUTTON_73)
				pPcio->m_InterruptCallback[OPT3BUTTON](pPcio->m_ClassPtr[OPT3BUTTON],
					panelinterrupt);
		}
	}

	if (pPcio->m_ClassPtr[CONTROLLERDIAGNOSTIC] != NULL)
		if (pPcio->m_Interrupt & 0x00000001)
			pPcio->m_InterruptCallback[CONTROLLERDIAGNOSTIC](pPcio->m_ClassPtr[CONTROLLERDIAGNOSTIC],
					pPcio->m_Interrupt);

	// close
	CloseHandle(pPcio->m_CallbackThread);
	pPcio->m_CallbackThread = NULL;

	return NULL;
}

void
TCPcio::ClearAllInterrupt_140(void)
{ // clear interrupt 'status' w/o clearing the 'enables'
	for (int i = 0; i < m_NumberIO; i++)
		if (m_IoBits[i].GetInterruptEnable() != pcioNA)
			PCIO_Write(m_IoBits[i].GetAddress() + STATUS_ADDR, (SHORT) 0x00);
}

int
TCPcio::GetPanelInterrupt(void)
{
	USHORT interrupt;
	PCIO_Read(PANEL_STATUS, &interrupt);
	return interrupt;
}

int
TCPcio::GetOpto1Interrupt(void)
{
	USHORT interrupt;
	PCIO_Read(OPTO_15_0_STATUS, &interrupt);
	return interrupt;
}

/*******************************************************************************/
/* TCIO_Bit                                                                    */
/*******************************************************************************/

/*
 * Function:	Initialize
 *
 * Description:	initialize this object
 *
 * Parameters:
 *		int		port      -  0/1
 *		int		bit       -  0-15
 *		int		direction - input/output
 *		int		interrupt - bit enable/disable interrupt
 *
 * Return Values
 *		None
 *
 * Discussion:
 *		initialize the member variables,
 *
 */
void
TCIO_Bits::Initialize(int address, int interrupt, int direction)
{
	m_address         = address;
	m_IODirection     = direction;
	m_interruptEnable = interrupt;
}

#pragma optimize ("", on)


