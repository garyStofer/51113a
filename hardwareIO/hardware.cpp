/*
 * Module:		$RCSfile: hardware.cpp $
 *				$Revision: 1.180.1.25.1.4.2.11.1.3 $
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
 *				(c) COPYRIGHT 1999-2000, TERADYNE, INC.
 *
 * Discussion:
 *
 * Date:		August. 2, 1999
 *
 */

#include "stdafx.h"
#include <atlbase.h>
#include "resource.h"

#include "afxtempl.h"
#include "optima.h"
#include "utility.h"
#include "sysparam.h"
#include "environ.h"
#include "evmevents.h"
#include "evmgrapi.h"
#include "TCException.hpp"
#include "messages.h"

#include "registry.l"
#include "waitcursor.h"
#include "timer.h"
#include "hardwareIO\utilities.h"
#include "hardware_enum.h"
#include "hardwareIO/hardwareDlg.hpp"
#include "hardwareIO/pcio.hpp"
#include "hardwareIO/smema.hpp"
#include "conveyor/firmware.h"
#include "hardwareIO/conveyors.hpp"
#include "hardwareIO/conveyor2.hpp"
#include "hardwareIO/conveyorUMSCO.hpp"
#include "hardwareIO/conveyorMC.hpp"
#include "hardwareIO/barcode.hpp"
#include "hardwareIO/lifters.hpp"
#include "hardwareIO/tower.hpp"
#include "hardwareIO/laser.hpp"
#include "hardwareIO/beeper.hpp"
#include "hardwareIO/console.hpp"
#include "hardwareIO/brdctrl.hpp"
#include "lights.hpp"
//motors
#include "lmotors.hpp"
#include "MCmotors.hpp"
#include "ACSCmotors.hpp"

#include "hardwareIO/hardware.hpp"
#include "reminder.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int Timeout30 = 30;
//20081015, morgan hsiao,
//add an array variable m_aCycleStopDlg to record sepearte purposes use 
//the same class TCDlgCycleStop.
//the first cell is for low air(7300), 
//the second is for cycle stop(7300)
//and the last one is for prepare eject protection(7355)
//TCDlgCycleStop    m_aCycleStopDlg[3];

bool m_bSuspendCheckMotorCancel;
HARDWAREIO_CLASS void SetSuspendCheckMotorCancel(bool bSuspendCheckMotorCancel)
{
	m_bSuspendCheckMotorCancel = bSuspendCheckMotorCancel;
};

HARDWAREIO_CLASS bool GetSuspendCheckMotorCancel()
{
	return m_bSuspendCheckMotorCancel;
};

/******************************************************************************/
/*  hardwareIO CallBack function                                              */
/******************************************************************************/
void HardwareIOHandler (TCEventData *EventData,
											void *ClientData, AOI_EVENT event)
{
	TCHardwareIO *pHardware = (TCHardwareIO *)(ClientData);
	// add verification
	if (pHardware != NULL)
		pHardware->HardwareIOEventHandler(event, EventData);		
}

void
TCHardwareIO::HardwareIOEventHandler(AOI_EVENT event, TCEventData *EventData)
{
	switch (event)
	{
		// something changed on the Factory Tab
		case AOI_HW_AIRPRESSURE:
			if (EventData->m_iRet == cON)
				LowAirWarningDialog(EventData->m_Command, csdtWARNING);
			else
				KillLowAirWarningDialog();
			break;

		case AOI_CYCLESTOP_ON:
			CycleStopDialog(EventData->m_Command, csdtWARNING);
			this->GetConveyorPtr()->LockConveyor();
			break;
		case AOI_CYCLESTOP_OFF:
			KillCycleStopDialog();
			this->GetConveyorPtr()->UnlockConveyor();
			break;

		case AOI_HW_INTERLOCK:
			StartHunterKillerThread();
			InitComFailureDialog();
			Sleep(300);
			break;
		case AOI_UR_HWSTATUS:
			// this will quiet motors & conveyor during this mode change
			SetMode(SOFTWARE_ONLY);
			EndHunterKillerThread();
			SendEvent(AOI_CYCLESTOP_OFF, _T("TCHardwareIO: System CycleStop"), (int) cOFF);
			Sleep(300);
			if(m_SystemType == NT7355 || m_SystemType == NT5500)
				SetSuspendCheckMotorCancel(true);
			InitHwModules(HARDWARE_ENABLED, TRUE);
			Sleep(300);
			// move the camera head out of the way
			ParkXYMotors();
			if(m_SystemType == NT7355 || m_SystemType == NT5500)
				SetSuspendCheckMotorCancel(false);
			break;
		case AOI_CANCEL:
			break;
		case AOI_SAFETY_DETECTOR_RESET:
			SetLaser(cON);
			break;
		case AOI_PREPARE_EJECT_PROTECTION:
			if (EventData->m_iRet == cON)
				PrepareEjectProtectionDialog(EventData->m_Command, csdtPROMPT);
			else //cOFF
				KillPrepareEjectProtectionDialog();
			break;
		default:
			break;
	}
}

/******************************************************************************/
/*  hardware functions                                                        */
/******************************************************************************/
/*
 * Function:	constructor
 *
 * Description:	build this object (initialize)
 *
 * Parameters:
 *		none
 *
 * Return Values:
 *		none
 *
 * Discussion:
 *		1. initialize the SystemType to NOHARDWARE
 *		2. NULL all hardware pointers
 *		3. initialize the CriticalSection
 *
 */
// this is a warning about using 'this' in the member initialisation,
#pragma warning( disable : 4355 )
// it's OK here as we just store the pointer and don't use the class 'bpalin
TCHardwareIO::TCHardwareIO() :
	m_CBCancel(AOI_CANCEL,
			::HardwareIOHandler, _T("MechIO: Cancel"),         this),
	m_CBLowAir(AOI_HW_AIRPRESSURE,
			::HardwareIOHandler, _T("MechIO: HardwareInit"),   this),
	m_CBCycleStopOn(AOI_CYCLESTOP_ON,
			::HardwareIOHandler, _T("MechIO: CycleStopOn"),    this),
	m_CBCycleStopOff(AOI_CYCLESTOP_OFF,
			::HardwareIOHandler, _T("MechIO: CycleStopOff"),   this),
	m_CBReInitialize(AOI_HW_INTERLOCK,
			::HardwareIOHandler, _T("MechIO: HardwareReInit"), this),
	m_CBHardware(AOI_UR_HWSTATUS,
			::HardwareIOHandler, _T("MechIO: HardwareInit"),   this),
	m_CBLaserOn(AOI_SAFETY_DETECTOR_RESET,
			::HardwareIOHandler, _T("MechIO: LaserOn"),   this),
	m_CBPrepareEjectProtection(AOI_PREPARE_EJECT_PROTECTION,
			::HardwareIOHandler, _T("MechIO: PrepareEjectProtection"),   this)
{
	// default
	m_SystemType = NT7300;

	// clear hardware initialization flag
	m_AmInitializing = FALSE;

	m_pPcio = new TCPcio();
	m_pPcio->InitializeTCPcio(this);

	// clear hardware Pointers
	m_pBarcode       = NULL;
	m_pBeeper        = NULL;
	m_pConveyors     = NULL;
	m_pConveyorUMSCO = NULL;
	m_pConsole       = NULL;
	m_pConveyors     = NULL;
	m_pLaser         = NULL;
	m_pLifters       = NULL;
	m_pLightDome     = NULL;
	m_pLMotors       = NULL;
	m_pLtTower       = NULL;
	m_pSmema         = NULL;
	// initialize
	InitPointers();
	InitializeCriticalSection(&csHardware);

	// initialization status dialog
	m_pHardwareInitDlg = NULL;
	m_pCycleStopDlg = NULL;
	m_pPrepareEjectProtectionDlg = NULL;
	m_pLowAirWarningDlg = NULL;
};

/*
 * Function:	destructor
 *
 * Description:	close this object
 *
 * Parameters:
 *		none
 *
 * Return Values:
 *		none
 *
 * Discussion:
 *		1. clear the SystemType to NOHARDWARE
 *		2. delete the CriticalSection
 *		3. close the connection to the PCIO driver
 *
 */
TCHardwareIO::~TCHardwareIO()
{
	Close();
	delete(m_pPcio);
};

void
TCHardwareIO::Close()
{

	// PCIO_close() calls KillAllThreads first.
	m_pPcio->PCIO_Close(); // close the PCIO connection

	SetMode((HARDWAREMODE) NULL);
	
	RegFactoryHardware(HW_UNTESTED);

	if (m_pLMotors != NULL)// XY Motors
	{
		if(m_pLMotors->m_iMotorType == MT_MCMOTOR)
			delete ((MCMotor*) m_pLMotors);
		else if(m_pLMotors->m_iMotorType == MT_ACSCMOTOR)
			delete ((ACSCMotor*)m_pLMotors);
		else
			delete m_pLMotors;
	}
	m_pLMotors = NULL;

	if (m_pSmema != NULL)
		delete (m_pSmema);		// Board Conveyor Communications
	m_pSmema = NULL;

	if (m_pLifters != NULL)
		delete (m_pLifters);	// Board Lifters
	m_pLifters = NULL;

	if (m_pConveyors != NULL)
	{
		int type = m_pConveyors->GetConveyorType();
		switch (type)
		{
		case CC7_MC:
			delete ((CCnvMC*) m_pConveyors);
			break;
		case CC7_ATWC:
			delete ((TCCnvyr*) m_pConveyors);
			break;
		case CC7_UMSCO:
			delete ((TCCnvyrUMSCO*) m_pConveyors);
			break;
		default:
			delete (m_pConveyors);	// Conveyor
		}
	}

	m_pConveyors = NULL;

	if (m_pLightDome != NULL)
		delete (m_pLightDome);	// Dome Lights
	m_pLightDome = NULL;

	if (m_pLaser != NULL)
		delete (m_pLaser);		// system laser
	m_pLaser = NULL;

	if (m_pBarcode != NULL)
	{
		m_pBarcode->Close();
		delete (m_pBarcode);	// system barcodes
	}
	m_pBarcode = NULL;

	if (m_pConsole != NULL)
		delete (m_pConsole);	// user panel
	m_pConsole = NULL;

	if (m_pBeeper != NULL)
		delete (m_pBeeper);		// system alarm
	m_pBeeper = NULL;

	if (m_pLtTower != NULL)
		delete (m_pLtTower);	// Light Tower
	m_pLtTower = NULL;

	if(m_pBrdCtrl != NULL)
		delete (m_pBrdCtrl);	// Board Controller
	m_pBrdCtrl = NULL;

	// PCIO_Close moved to top
	// close the PCIO
	// m_pPcio->PCIO_Close(); 

	// Critical Section no longer needed
	DeleteCriticalSection(&csHardware);
};

/*
 * Function:	InitPointers(void)
 *
 * Description:	NULL hardware pointers
 *
 * Parameters:
 *		none
 *
 * Return Values:
 *		none
 *
 * Discussion:
 *		initialize all hardware pointers to NULL
 *
 */
void
TCHardwareIO::InitPointers(void)
{
	// initialize
	m_SystemInterlock     = NULL;
	m_Diagnostic          = NULL; //
	m_Laser               = NULL;
	m_LaserPolarity       = NULL;
	m_Beeper              = NULL;
	m_LoadLight           = NULL;
	m_InspectButton       = NULL;
	m_LoadButton          = NULL;
	m_Unused              = NULL;
	m_TowerYellow         = NULL;
	m_ConveyorBypass      = NULL;

	// lighttower
	m_TowerRed            = NULL;
	m_TowerGreen          = NULL;
	m_TowerBlue           = NULL;
	m_TowerWhite          = NULL;
	m_LTFlashTime         = NULL;
	m_Beeper2             = NULL;

	// fans
	m_AuxFan0             = NULL;
	m_AuxFan1             = NULL;
	m_AuxFan2             = NULL;
	m_AuxFan3             = NULL;
	m_AuxFan4             = NULL;
	m_AuxFan5             = NULL;
	m_AuxFan6             = NULL;
	m_AuxFan7             = NULL;

	// operator panel
	m_CancelButton        = NULL;
	m_EjectButton         = NULL;
	m_Opt1Button          = NULL;
	m_Opt2Button          = NULL;
	m_Opt3Button          = NULL;
	m_Aux1Button          = NULL;
	m_Aux2Button          = NULL;
	m_Aux3Button          = NULL;
	m_InspectLight        = NULL;
	m_EjectLight          = NULL;
	m_CancelLight         = NULL;
	m_Opt1Light           = NULL;
	m_Opt2Light           = NULL;
	m_Opt3Light           = NULL;
	m_Aux1Light           = NULL;
	m_Aux2Light           = NULL;
	m_Aux3Light           = NULL;

	// driver interrupts
	m_LEDrv               = NULL;
	m_TowerDrv            = NULL;
	m_V12Drv              = NULL;
	m_V24Drv              = NULL;

	// opto electronics
	m_ConveyorBypass      = NULL;
	m_AirInstalled        = NULL;
	m_LowAir              = NULL;
	m_MotorStagingInstalled = NULL;

	// master interrupt status
	m_InterruptStatus     = NULL;

	m_SystemInterlock     = NULL;
	// use this 'unused' button structure for controller diagnostics
	m_Diagnostic          = NULL;

	// laser
	m_Laser               = NULL;
	m_LaserPolarity       = NULL;

	// light dome
	m_Dome                = NULL;
};

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
TCHardwareIO::SendEvent(AOI_EVENT event, LPCTSTR Sender, int iRet, LPCTSTR Command)
{
	TCEventData msg;
	msg.m_SendersName = Sender;
	msg.m_iRet        = iRet;
	msg.m_Command     = Command;
	msg.m_event       = event;
	EMBroadcastEvent(event, &msg);
};

void
TCHardwareIO::RegFactoryHardware(DWORD hardware)
{
	GetSysReg()->sysv_HWTHwIO   = hardware;
	GetSysReg()->sysv_HWTDomeIO = hardware;
}

/*
 * Function:	RegistryCheck
 *
 * Description:	check the registry for hardware status
 *
 * Parameters:
 *		none
 *
 * Return Values
 *		BOOL				TRUE  = hardware available
 *							FALSE = NO hardware, software ONLY system
 *
 * Discussion:
 *		read the AOI 'HardwareStatus' key from the Registry, if SOFTWARE_ONLY,
 *		return FALSE
 *
 *		if the 'HardwareStatus' key returns HARDWARE_ENABLED, read from the
 *		Registry the status of the TeraPcio driver.  If active return TRUE
 *		if the driver is inactive, return FALSE
 *
 */
HARDWAREMODE
TCHardwareIO::RegistryCheck(void)
{
	// from the registry . . .
	// system type
	m_SystemType = (SYSTEMTYPE) (int) GetSysReg()->sysv_SystemType;
	// dome type
	m_DomeType = (DOMETYPE) (int) GetSysReg()->sysv_DomeType;

	// default hardware mode
	HARDWAREMODE hardware = SOFTWARE_ONLY;
	DWORD Hardware;

	// check for a prodcution mode license if trying to start in hardware mode
//	bool hwLicensed = AoiGetApp()->CheckLicense( LicFeatureOptimaProd ); //F404 vc6ぉㄴ쨢쬟간
	bool hwLicensed = AoiGetApp()->CheckLicense((AoiGetApp()->GetLicenseModel() + AoiGetApp()->GetIsThatModel())->Lot[1].loc);
	if( (!hwLicensed) && (GetSysReg()->sysv_HardwareStatus != SOFTWARE_ONLY) )
	{
		//AfxMessageBox( "You are not licensed to run in production mode" );
		TCException e = TCEXCEPTION(HW_NO_LICENSE);
		AfxMessageBox  ( e.getMessage() );
		SetMode(SOFTWARE_ONLY);
		return(SOFTWARE_ONLY);
	}

	// get Windows OS version
	OSVERSIONINFOEX osvi;
	BOOL bOsVersionInfoEx;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi);
	
	if (osvi.dwMajorVersion == 4)
	{ // detect Windows OS
		// determine if the TeraPcio.sys service is installed & running
		if (ReadRegistry("SYSTEM\\CurrentControlSet\\Enum\\Root\\LEGACY_TERAPCIO\\0000",
			"StatusFlags", Hardware) == ERROR_SUCCESS)
			hardware = (Hardware == 0L) ?  SOFTWARE_ONLY : HARDWARE_ENABLED;
		else
			hardware = HARDWARE_ENABLED;
	}
	else if (osvi.dwMajorVersion == 5)
	{ // OS is W2K or XP
		// No registry check.  rely on the detection of the PCIO and SC to select
		// HARDWARE_ENABLED or SOFTWARE_ONLY modes.
		hardware = HARDWARE_ENABLED;
	}
	else if (osvi.dwMajorVersion == 6)
	{
		// OS is Win 7
		// No registry check.  rely on the detection of the PCIO and SC to select
		// HARDWARE_ENABLED or SOFTWARE_ONLY modes.
		hardware = HARDWARE_ENABLED;
	}
	else // if (hardware == SOFTWARE_ONLY)
	{
		GetSysReg()->sysv_HardwareStatus = SOFTWARE_ONLY;
		SetMode(SOFTWARE_ONLY);
		return (SOFTWARE_ONLY);
	}

	// hardware mode
	DWORD startup = 0;
	switch (GetSysReg()->sysv_HardwareStatus)
	{
		case SOFTWARE_ONLY:
			// if we reached here && the registery indicates that the
			// current Optima mode is "OPERATOR", try to reengage the
			// hardware with user intervention.
			if (GetSysReg()->userv_OperatorStartupMode == SHENV_OPERATOR &&
						GetCurrentEnvironment() == SHENV_OPERATOR && hwLicensed)
			{	// operator mode
				hardware = HARDWARE_ENABLED;
				GetSysReg()->sysv_HardwareStatus = HARDWARE_ENABLED;
			}
			else
				hardware = SOFTWARE_ONLY;
			break;
		default:
		case HARDWARE_ENABLED:
			hardware = HARDWARE_ENABLED;
			break;
		case HARDWARE_FLASH:
			hardware = HARDWARE_FLASH;
			break;
	}

	SetMode(hardware);
	return (hardware);
}

/*
 * Function:	ReInitialize(HARDWAREMODE mode)
 *
 * Description:	re-initialize the hardware
 *
 * Parameters:
 *		HARDWAREMODE	mode
 *
 * Return Values:
 *		TSTATUS
 *
 * Discussion:
 *			
 *				restart the hardware in the 'new' mode
 */
TSTATUS
TCHardwareIO::ReInitialize(HARDWAREMODE mode)
{
	// close the PCIO
	m_pPcio->PCIO_Close();

	InitPointers();
	return Init(mode);
}

/*
 * Function:	Init(HARDWAREMODE mode)
 *
 * Description:	initialize the hardware
 *
 * Parameters:
 *		HARDWAREMODE	mode
 *
 * Return Values:
 *		TSTATUS
 *
 * Discussion:
 *			
 *
 */
extern HINSTANCE HardwareInstance;

TSTATUS
TCHardwareIO::Init(HARDWAREMODE mode)
{
	CString message;
	DWORD error = 0;
	SetMode(mode);
	m_MotorsInitialized   = FALSE;
	m_ConveyorInitialized = FALSE;
	m_ControllerFirmRevision = SC_SOFTWAREONLY;

	if (RegistryCheck() == SOFTWARE_ONLY ||
		GetMode()       == SOFTWARE_ONLY ||
		(error = m_pPcio->PCIO_Open(m_SystemType)) == PCIO_OPEN_ERROR)
	{
		if (GetMode() != HARDWARE_FLASH)
		{
			if (error == PCIO_OPEN_ERROR)
			{
				message.Format("System mode is \"Hardware Enabled\",\n"\
					"but the hardware is unavailable.  Reverting to\n\"Software Only\" mode");
				/*UserMessage(message, _T("AOI Alert Message"),
					MB_SYSTEMMODAL | MB_ICONEXCLAMATION | MB_OK);*/
				TCException e = TCEXCEPTION(HW_SYSTEM_REVERTING);
				AfxMessageBox( e.getMessage(), MB_SYSTEMMODAL | MB_ICONEXCLAMATION | MB_OK);

			}
			SetMode(SOFTWARE_ONLY);
			GetSysReg()->sysv_HardwareStatus = SOFTWARE_ONLY;

			RegFactoryHardware(HW_FAILED);
		}
		return TS_SUCCESS;
	}
	// PCI/O dirver & hardware connected
	// check on System Controller

	// get PCIO version
	m_PcioRevision   = m_pPcio->GetPcioRevision();

// The DeviceDrivers IOcontrol interface version can not be checked at this late point.
// the call to PCIO_Open already failed and returned with error 
// 
	m_pPcio->GetPcioDriverRevision(m_PcioDriverRevision);

	m_ControllerType = m_pPcio->GetControllerType();

	// match controller version
	if ((m_PcioRevision & PCIO_REVISION_MASK) != PCIO_REVISION )
	{
		message.Format("PCI Adapter Hardware revision:(%x) mismatch, expected revision of %x"\
			"\n\nReverting to \'Software Only\' mode.",
			m_PcioRevision & PCIO_REVISION_MASK,		
			PCIO_REVISION
			);
		UserMessage(message, _T("AOI Alert Message"),
			MB_SYSTEMMODAL | MB_ICONEXCLAMATION | MB_OK);

		SetMode(SOFTWARE_ONLY);
		RegFactoryHardware(HW_FAILED);
		return TS_SUCCESS;
	}

	int scMj = (int) m_pPcio->GetControllerFirmwareMajor();
	int scMr = (int) m_pPcio->GetControllerFirmwareMinor();
	//if (scMj == 0x0)  // F404 vc6ぉㄴ쨢쬟간
	//{
		//m_ControllerFirmRevision = SC_PREFLASH;
		//m_ControllerFirmRevision = SC_B0;
		//m_ControllerFirmwareRevision = "@.0";
		//m_ControllerFirmwareRevision = "LX";
	//}
	//else
	//{
	if (scMj == 1 && scMr < 0x9)
		m_ControllerFirmRevision = SC_A0;// Flashable, old Interlock (A.0 thru A.8)
	else if (scMj == 1 && scMr == 0x9)
		m_ControllerFirmRevision = SC_A9; // Flashable, new interlock(cyclestop)
	else // > A.9
		m_ControllerFirmRevision = SC_B0;

	if(m_pPcio->GetControllerPCA() == "@.0" && m_pPcio->GetControllerPCB() == "@.0")
		m_ControllerFirmwareRevision = "LX";
	else
		m_ControllerFirmwareRevision.Format("%c.%c", scMj + '@', scMr + '0');
	//}

	if (GetMode() != HARDWARE_FLASH)
	{
		// start PCIO interrupt thread
		if (m_pPcio->StartInterruptThread() != PCIO_SUCCESS)
		{
			SetMode(SOFTWARE_ONLY);
			RegFactoryHardware(HW_FAILED);
			return TS_SUCCESS;
		}
		// allow thread to completely startup
		while (!m_pPcio->GetInterruptThreadStatus())
			Sleep(100);
	}

	// PCIO ready, initialize controller
	if (m_pPcio->Controller_Init(GetMode()) == PCIO_INIT_ERROR)
	{
		SetMode(SOFTWARE_ONLY);
		RegFactoryHardware(HW_FAILED);
		return TS_SUCCESS;
	}

	// flag DlgFactory
	RegFactoryHardware(HW_PASSED);

	m_TowerRed            = m_pPcio->getIO_Ptrs(IO_TOWER_RED);
	m_TowerYellow         = m_pPcio->getIO_Ptrs(IO_TOWER_YEL);
	m_TowerGreen          = m_pPcio->getIO_Ptrs(IO_TOWER_GREEN);
	m_TowerBlue           = m_pPcio->getIO_Ptrs(IO_TOWER_BLUE);
	m_TowerWhite          = m_pPcio->getIO_Ptrs(IO_TOWER_WHITE);
	m_LTFlashTime         = m_pPcio->getIO_Ptrs(IO_TOWER_FLASH);
	m_Beeper              = m_pPcio->getIO_Ptrs(IO_ALARM1);
	m_Beeper2             = m_pPcio->getIO_Ptrs(IO_ALARM2);

	//fans
	m_AuxFan0             = m_pPcio->getIO_Ptrs(IO_AUXFAN0);
	m_AuxFan1             = m_pPcio->getIO_Ptrs(IO_AUXFAN1);
	m_AuxFan2             = m_pPcio->getIO_Ptrs(IO_AUXFAN2);
	m_AuxFan3             = m_pPcio->getIO_Ptrs(IO_AUXFAN3);
	m_AuxFan4             = m_pPcio->getIO_Ptrs(IO_AUXFAN4);
	m_AuxFan5             = m_pPcio->getIO_Ptrs(IO_AUXFAN5);
	m_AuxFan6             = m_pPcio->getIO_Ptrs(IO_AUXFAN6);
	m_AuxFan7             = m_pPcio->getIO_Ptrs(IO_AUXFAN7);

	// operator panel
	m_LoadButton          = m_pPcio->getIO_Ptrs(IO_LOADBUTTON);
	m_InspectButton       = m_pPcio->getIO_Ptrs(IO_INSPECTBUTTON);
	m_EjectButton         = m_pPcio->getIO_Ptrs(IO_EJECTBUTTON);
	m_CancelButton        = m_pPcio->getIO_Ptrs(IO_CANCELBUTTON);
	m_Opt1Button          = m_pPcio->getIO_Ptrs(IO_OPT1BUTTON);
	m_Opt2Button          = m_pPcio->getIO_Ptrs(IO_OPT2BUTTON);
	m_Opt3Button          = m_pPcio->getIO_Ptrs(IO_OPT3BUTTON);
	m_Aux1Button          = m_pPcio->getIO_Ptrs(IO_AUX1BUTTON);
	m_Aux2Button          = m_pPcio->getIO_Ptrs(IO_AUX2BUTTON);
	m_Aux3Button          = m_pPcio->getIO_Ptrs(IO_AUX3BUTTON);
	m_LoadLight           = m_pPcio->getIO_Ptrs(IO_LOADLITE);
	m_InspectLight        = m_pPcio->getIO_Ptrs(IO_INSPECTLITE);
	m_EjectLight          = m_pPcio->getIO_Ptrs(IO_EJECTLITE);
	m_CancelLight         = m_pPcio->getIO_Ptrs(IO_CANCELLITE);
	m_Opt1Light           = m_pPcio->getIO_Ptrs(IO_OPT1LITE);
	m_Opt2Light           = m_pPcio->getIO_Ptrs(IO_OPT2LITE);
	m_Opt3Light           = m_pPcio->getIO_Ptrs(IO_OPT3LITE);
	m_Aux1Light           = m_pPcio->getIO_Ptrs(IO_AUX1LITE);
	m_Aux2Light           = m_pPcio->getIO_Ptrs(IO_AUX1LITE);
	m_Aux3Light           = m_pPcio->getIO_Ptrs(IO_AUX1LITE);

	// driver interrupts
	m_LEDrv               = m_pPcio->getIO_Ptrs(IO_LEDDRV);
	m_TowerDrv            = m_pPcio->getIO_Ptrs(IO_TOWERDRV);
	m_V12Drv              = m_pPcio->getIO_Ptrs(IO_V12DRV);
	m_V24Drv              = m_pPcio->getIO_Ptrs(IO_V24DRV);

	// master interrupt status
	m_InterruptStatus     = m_pPcio->getIO_Ptrs(IO_INT_ALL); // all interrupts
	m_SystemInterlock     = m_pPcio->getIO_Ptrs(IO_INTERLOCK); // system interlock interrupt

	// use this 'unused' button structure for controller diagnostics
	m_Diagnostic          = m_pPcio->getIO_Ptrs(IO_DIAGNOSTIC);

	// opto electronics
	m_ConveyorBypass      = m_pPcio->getIO_Ptrs(IO_CONVYR_BYPASS);
	m_AirInstalled        = m_pPcio->getIO_Ptrs(IO_AIRINSTALLED);
	m_LowAir              = m_pPcio->getIO_Ptrs(IO_LOWAIR);
	m_MotorStagingInstalled = m_pPcio->getIO_Ptrs(IO_MOTORSTAGING_INSTALLED);

	// laser
	m_Laser               = m_pPcio->getIO_Ptrs(IO_LASER);
	m_LaserPolarity       = m_pPcio->getIO_Ptrs(IO_LASER_POLARITY);

	m_Dome                = m_pPcio->getDomePtr();

	return TS_SUCCESS;
} /* Init() */

void
TCHardwareIO::ForceSoftwareOnlyMode(void)
{
	SetMode(SOFTWARE_ONLY);
	GetSysReg()->sysv_HardwareStatus = SOFTWARE_ONLY;

	// reinitialize
	SendEvent(AOI_UR_HWSTATUS, _T("TCMechIO: Hardware ReInitialization"));
}

void
TCHardwareIO::ClearHardwareFlashMode(void)
{
	GetSysReg()->sysv_HardwareStatus = HARDWARE_ENABLED;

	// reinitialize
	SendEvent(AOI_UR_HWSTATUS, _T("TCMechIO: Hardware ReInitialization"));
}

void
TCHardwareIO::ForceHardwareFlashMode(void)
{
	SetMode(SOFTWARE_ONLY);
	GetSysReg()->sysv_HardwareStatus = HARDWARE_FLASH;

	// reinitialize
	SendEvent(AOI_UR_HWSTATUS, _T("TCMechIO: Hardware ReInitialization"));
}

void
TCHardwareIO::SetSystemType(SYSTEMTYPE type)
{
	m_SystemType = type;
};

SYSTEMTYPE
TCHardwareIO::GetSystemType(void)
{
	return m_SystemType;
};

CString	TCHardwareIO::SystemTypeToString( SYSTEMTYPE type )
{
	switch (type)
	{
	case NT7300:
		return "7300";
	case NT7350:
		return "7350";
	case NT5500:
		return "5500";
	default:
	case NTUNKNOW:
		return "Unknow";
	}
}

HARDWAREMODE
TCHardwareIO::GetMode(void)
{
	return m_Mode;
};

void
TCHardwareIO::SetMode(HARDWAREMODE mode)
{
	m_Mode = mode;
	if (m_Mode == SOFTWARE_ONLY)
	{
		if (m_pConveyors != NULL)
			m_pConveyors->SetOperatingMode(SOFTWARE_ONLY);
		if (m_pLMotors != NULL)
			m_pLMotors->SetOperatingMode(SOFTWARE_ONLY);
	}
};

/*
 * Function:	GetControllerType()
 *				*private*
 *
 * Description:	return the System Controller type 
 *
 * Parameters:
 *		none
 *
 * Return Values:
 *		CONTROLTYPE
 *
 * Discussion:
 *
 *
 */
CONTROLTYPE
TCHardwareIO::GetControllerType(void)
{
	return m_ControllerType;
};

/*
 * Function:	GetDomeType()
 *				*private*
 *
 * Description:	return the LED dome type 
 *
 * Parameters:
 *		none
 *
 * Return Values:
 *		DOMETYPE
 *
 * Discussion:
 *
 *
 */
DOMETYPE
TCHardwareIO::GetDomeType(void)
{
	return m_DomeType;
};


/*
 * Function:	InitHwModules()
 *				*private*
 *
 * Description:	create and initialize all hardware objects 
 *
 * Parameters:
 *		HARDWAREMODE -	HAREWARE_ENABLED
 *						SOFTWARE_ONLY
 *		BOOL		 -  FALSE == initialize
 *						TRUE  == reinitialize
 *
 * Return Values:
 *		TSTATUS
 *
 * Discussion:
 *
 *
 */
TSTATUS
TCHardwareIO::InitHwModules(HARDWAREMODE mode, BOOL reinitialize)
{
	m_ConveyorLock = FALSE;
	m_ConveyorLockThreadID = NULL;
	m_hAutoWidthThread = NULL;
	m_hInitmotorThread = NULL;

	InitializeDialog(HW_INITIALIZIING);

	if (GetSysControllerFirmwareRevision() >= SC_A9)
	{
		TRACE("WatchDog Paused\n");
		PauseSysWatchdog();
	}

    try
    {
		UADDTIMESTAMP("HW: start SysCont Initializing");
		TSTATUS hardware = (reinitialize == TRUE) ?
			ReInitialize(mode) : Init(mode);
		UADDTIMESTAMP("HW: end SysCont Initializing");
		if (hardware == TS_SUCCESS)
		{
			mode = GetMode(); // mode based on return from SysCont
			// pause the Watchdog timer if reinitializing
			if (reinitialize == TRUE &&
				GetSysControllerFirmwareRevision() >= SC_A9)
			{
				TRACE("WatchDog Paused\n");
				PauseSysWatchdog();
			}

			// indicate SC initializing
			InitializeDialogUpdate(INIT_SYSCONTROLLER, IDLG_INITIALIZING);
			m_SystemType = GetSystemType();
			// if FLASHing the SC, then initialize all othe systems
			// in SOFTWARE_ONLY mode
//			HARDWAREMODE mode = (initMode == HARDWARE_FLASH) ?
//											SOFTWARE_ONLY : GetMode(); //F404 vc6ぉㄴ쨢쬟간

			// build the hardware objects
			// user panel
			UADDTIMESTAMP("HW: start UserPanel Initializing");
			if (InitializeConsole(reinitialize) == TS_ERROR)
			{ // a condition of 'interlock' or 'cyclestop' can prevent initialization
				GetSysReg()->sysv_HardwareStatus = SOFTWARE_ONLY;
				SetMode(SOFTWARE_ONLY);
			}
			UADDTIMESTAMP("HW: end UserPanel Initializing");

			// update Initialization Dialog
			mode = GetMode();
			InitializeDialogUpdate(INIT_SYSCONTROLLER,
				mode == SOFTWARE_ONLY ? IDLG_SOFTWAREONLY : IDLG_INITCOMPLETE);

		// XYMotors
			InitializeDialogUpdate(INIT_MOTORCONTROLLER, IDLG_INITIALIZING);
			UADDTIMESTAMP("HW: start XY Motors Initializing");
			TSTATUS motors = InitializeMotors(mode, reinitialize);
			InitializeDialogUpdate(INIT_MOTORCONTROLLER,
				mode == SOFTWARE_ONLY ? IDLG_SOFTWAREONLY : motors == TS_SUCCESS ? 
												IDLG_INITCOMPLETE : IDLG_ERROR);
			UADDTIMESTAMP("HW: end XY Motors Initializing");

		// lifters
			InitializeLifters(reinitialize);
		// SMEMA
			InitializeSMEMA(reinitialize);

		// BarCode
			InitializeDialogUpdate(INIT_BARCODE, IDLG_INITIALIZING);
			TSTATUS barcode = InitializeBarcode(reinitialize);
			InitializeDialogUpdate(INIT_BARCODE,
				mode == SOFTWARE_ONLY ? IDLG_SOFTWAREONLY : barcode == TS_SUCCESS ? 
												IDLG_INITCOMPLETE : IDLG_ERROR);

		// conveyors
			InitializeDialogUpdate(INIT_CONVEYORCONTROLLER, IDLG_INITIALIZING);
			int cc;
			TSTATUS conveyor;
			UADDTIMESTAMP("HW: start Conveyors Initializing");

			CRuntimeClass * rc = m_pLMotors->GetRuntimeClass();

			if( !::strcmp(rc->m_lpszClassName, "MCMotor" ) && m_pLMotors->IsControllerExist() )
					cc = CC7_MC;
			else
			{
				if (mode == SOFTWARE_ONLY)
					cc = CC7_ATWC;
				else
				{
					if (reinitialize == TRUE)
						cc = m_pConveyors->DetermineControllerType();
					else
					{
						TCConveyors conv(this);
						cc = conv.DetermineControllerType();
					}
				}
			}

			if (cc == CC7_NONE)
			{ // use the _ATWC conveyor for SOFTWARE_ONLY
				if (reinitialize == FALSE)
					m_pConveyors = (TCConveyors *) new TCCnvyr(this);
				conveyor = m_pConveyors->InitCnvyrController(SOFTWARE_ONLY);

				// if hardware is ENABLED but no conveyor, then programming
				// station;  set maximum XYmotor speed to 0.5
				double speed =  ((double) m_MotorSpeed) / 100.0;
				m_pLMotors->SetMotorSpeed((speed > 1.0) ? 0.5 : (speed / 2.0));
			}
			else if (cc == CC7_ATWC)
			{
				conveyor = InitializeConveyor2(reinitialize);
			}
			else if (cc == CC7_MC)
			{
				conveyor = InitializeConveyorMC(reinitialize);
			}
			else
			{
				conveyor = InitializeConveyorUMSCO(reinitialize);
			}
			UADDTIMESTAMP("HW: end Conveyors Initializing");

			
		// board control
			if (mode != HARDWARE_FLASH)
				InitializeBrdCtrl(reinitialize);

			if(cc == CC7_MC && motors != TS_SUCCESS) conveyor = motors;
			InitializeDialogUpdate(INIT_CONVEYORCONTROLLER,
				mode == SOFTWARE_ONLY ?
						IDLG_SOFTWAREONLY : conveyor == TS_SUCCESS ? 
						IDLG_INITCOMPLETE : conveyor == TS_INFO    ?
						IDLG_SOFTWAREONLY :	IDLG_ERROR);

		// light tower
			InitializeLtTower(reinitialize);
		// system alarm
			InitializeBeepers(reinitialize);

			// lighting Dome
			InitializeDialogUpdate(INIT_DOME, IDLG_INITIALIZING);
			TSTATUS dome = InitializeLightDome(reinitialize);
			InitializeDialogUpdate(INIT_DOME,
				mode == SOFTWARE_ONLY ? IDLG_SOFTWAREONLY : dome == TS_SUCCESS ? 
												IDLG_INITCOMPLETE : IDLG_ERROR);
		// warp laser
			InitializeDialogUpdate(INIT_LASER, IDLG_INITIALIZING);
			TSTATUS laser = InitializeLaser(reinitialize);
			InitializeDialogUpdate(INIT_LASER,
				mode == SOFTWARE_ONLY ? IDLG_SOFTWAREONLY : laser == TS_SUCCESS ? 
												IDLG_INITCOMPLETE : IDLG_ERROR);
			// delay to show the user the conditions of initialization
			Sleep(1500);

			if (mode != HARDWARE_FLASH)
			{
				// reset the watchdog
				if (reinitialize == TRUE)
				{
					// eject the board (if loaded)    
					if(m_SystemType !=NT7355) //hugo 20090109
					SendEvent(AOI_UREJECTBOARD, _T("TCMechIO: ReInitialize EjectBoard"));

					if(GetSysControllerFirmwareRevision() >= SC_A9)
					{
						TRACE("WatchDog Renabled\n");
						ResetSysWatchdog();
						ReEnableSysWatchdog();
					}
				}
			}
		}
		SetMode(mode);
    }
	catch (TCException e)
    {/*
		UserMessage(_T("Hardware Initialization FAILED\n"\
						"reverting to 'Software Only' mode"), 
					 _T("AOI Alert Message"),
					 MB_SYSTEMMODAL | MB_ICONEXCLAMATION | MB_OK);*/
		TCException b = TCEXCEPTION(HW_HARDINI_FAILED);
		AfxMessageBox( b.getMessage(), MB_SYSTEMMODAL | MB_ICONEXCLAMATION | MB_OK);

		InitPointers();
		SetSystemType(NT7300);
		m_SystemType = NT7300;
		SetMode(SOFTWARE_ONLY);

   		InitializeMotors(SOFTWARE_ONLY);
	}

	KillInitializeDialog();
	return TS_SUCCESS;
}

TSTATUS
TCHardwareIO::ShutDown(void)
{
	// park the motors
	ParkXYMotors();

	// shutdown to a SOFTWARE_ONLY condition
	SetMode(SOFTWARE_ONLY);

	// dialog 
	InitializeDialog(HW_SHUTDOWN);

	if (GetSysControllerFirmwareRevision() >= SC_A9)
	{
		TRACE("WatchDog Paused\n");
		PauseSysWatchdog();
	}

    try
    {
		// warp laser
		InitializeDialogUpdate(INIT_LASER, IDLG_SHUTDOWN);
		TSTATUS laser = InitializeLaser(TRUE);
		InitializeDialogUpdate(INIT_LASER, IDLG_SHUTDOWN);

		// lighting Dome
		InitializeDialogUpdate(INIT_DOME, IDLG_SHUTDOWN);
		TSTATUS dome = InitializeLightDome(TRUE);
		InitializeDialogUpdate(INIT_DOME, IDLG_SHUTDOWN);

		// conveyor
		InitializeDialogUpdate(INIT_CONVEYORCONTROLLER,IDLG_SHUTDOWN);
		UADDTIMESTAMP("HW: start Conveyors ShutDown");
		TSTATUS conveyor = InitializeConveyor2(TRUE);
		UADDTIMESTAMP("HW: end Conveyors ShutDown");
		InitializeDialogUpdate(INIT_CONVEYORCONTROLLER,IDLG_SHUTDOWN);

		// XYMotors
		InitializeDialogUpdate(INIT_MOTORCONTROLLER, IDLG_SHUTDOWN);
		UADDTIMESTAMP("HW: start XY Motors ShutDown");
		TSTATUS motors = InitializeMotors(SOFTWARE_ONLY, TRUE);
		UADDTIMESTAMP("HW: end XY Motors ShutDown");
		InitializeDialogUpdate(INIT_MOTORCONTROLLER, IDLG_SHUTDOWN);

		// barcode scanners
		InitializeDialogUpdate(INIT_BARCODE, IDLG_SHUTDOWN);
		UADDTIMESTAMP("HW: start Barcode Scanners ShutDown");
		TSTATUS barcode = InitializeBarcode(TRUE);
		UADDTIMESTAMP("HW: end Barcode Scanners ShutDown");
		InitializeDialogUpdate(INIT_BARCODE, IDLG_SHUTDOWN);

		// user panel
		UADDTIMESTAMP("HW: start UserPanel ShutDown");
		InitializeConsole(TRUE);
		UADDTIMESTAMP("HW: end UserPanel ShutDown");

		// SC
		InitializeDialogUpdate(INIT_SYSCONTROLLER, IDLG_SHUTDOWN);
		UADDTIMESTAMP("HW: start SC ShutDown");
		// close the PCIO
		m_pPcio->PCIO_Close();
		InitPointers();
		UADDTIMESTAMP("HW: end SC ShutDown");
		InitializeDialogUpdate(INIT_SYSCONTROLLER, IDLG_SHUTDOWN);

		// delay to show the user the conditions of initialization
		Sleep(1500);
    }
	catch (TCException e)
    {/*
		UserMessage(_T("Hardware ShutDown FAILED."), 
						 _T("AOI Alert Message"),
						 MB_SYSTEMMODAL | MB_ICONEXCLAMATION | MB_OK);*/
		TCException b = TCEXCEPTION(HW_SHUTDOWN_FAILED);
		AfxMessageBox( b.getMessage(), MB_SYSTEMMODAL | MB_ICONEXCLAMATION | MB_OK);

		InitPointers();
		SetSystemType(NT7300);
		m_SystemType = NT7300;
		SetMode(SOFTWARE_ONLY);

   		InitializeMotors(SOFTWARE_ONLY);
	}

	KillInitializeDialog();
	return TS_SUCCESS;
}

/******************************************************************************/
/*  PCI/O communication functions                                             */
/******************************************************************************/
TCPcio *
TCHardwareIO::GetPcio()
{
	return m_pPcio;
};

int
TCHardwareIO::GetPcioRevision(void)
{
	return m_PcioRevision;
};

CString
TCHardwareIO::GetPcioDriverRevision(void)
{
	return m_PcioDriverRevision;
};

void
TCHardwareIO::RegisterInterruptCallback(int interrupt, void *classPtr, PCIO_HANDLER_PTR func)
{
	m_pPcio->RegisterAoiInterruptCallback(interrupt, classPtr, func);
};

void
TCHardwareIO::UnRegisterInterruptCallback(int interrupt)
{
	m_pPcio->UnRegisterAoiInterruptCallback(interrupt);
};

BOOL
TCHardwareIO::CanFlash()
{
	return m_pPcio->CanFlash();
};

void
TCHardwareIO::ReEnableSysWatchdog(void)
{
	m_pPcio->ContinueSysWatchdog();
};

void
TCHardwareIO::PauseSysWatchdog(void)
{
	m_pPcio->PauseSysWatchdog();
};

void
TCHardwareIO::ResetSysWatchdog(void)
{
	m_pPcio->ReloadSysWatchdog();
};

/******************************************************************************/
/*  hardware communication functions                                          */
/******************************************************************************/
/*
TSTATUS
TCHardwareIO::EnableInterrupts(void)
{
	EnterCriticalSection(&csHardware);
		m_pPcio->AoiInterruptEnable(PCIO_iENABLEALL);
	LeaveCriticalSection(&csHardware);
	TRACE("interrupts re-enabled\n");
	return(TS_SUCCESS);
};

TSTATUS
TCHardwareIO::DisableInterrupts(void)
{
	EnterCriticalSection(&csHardware);
		m_pPcio->AoiInterruptEnable(PCIO_iDISABLEALL);
	LeaveCriticalSection(&csHardware);
	return(TS_SUCCESS);
};
*/
TSTATUS
TCHardwareIO::ResetDomePosition(MOTOR_AXIS axis)
{
	if(this->m_SystemType == NT5500 || this->m_SystemType == NT7355)
	{
		int resetpositionX = (this->m_SystemType == NT7355 ? BT_XHOME : -BT_XHOME);
		int resetpositionY = BT_YHOME;
		if (axis == Both || axis == XAxis)
			SetDomeXPosition(resetpositionX);
		if (axis == Both || axis == YAxis)
			SetDomeYPosition(resetpositionY);
		int homeX,homeY;
		this->GetDomeXPosition( homeX );
		this->GetDomeYPosition( homeY );
		m_pLMotors->SetAxisPosition( homeX, homeY );
	}
	else
	{
		int resetposition = 0;
		if (axis == Both || axis == XAxis)
			SetDomeXPosition(resetposition);
		if (axis == Both || axis == YAxis)
			SetDomeYPosition(resetposition);
	}
	return(TS_SUCCESS);
}

TSTATUS
TCHardwareIO::GetDomeXPosition(int &position)
{
	EnterCriticalSection(&csHardware);
		position = m_pPcio->ReadDomeXPosition();
	LeaveCriticalSection(&csHardware);
	return(TS_SUCCESS);
}

TSTATUS
TCHardwareIO::SetDomeXPosition(int &position)
{
	EnterCriticalSection(&csHardware);
		m_pPcio->WriteDomeXPosition(position);
	LeaveCriticalSection(&csHardware);
	return(TS_SUCCESS);
}

TSTATUS
TCHardwareIO::GetDomeYPosition(int &position)
{
	EnterCriticalSection(&csHardware);
		position = m_pPcio->ReadDomeYPosition();
	LeaveCriticalSection(&csHardware);
	return(TS_SUCCESS);
}

TSTATUS
TCHardwareIO::SetDomeYPosition(int &position)
{
	EnterCriticalSection(&csHardware);
		m_pPcio->WriteDomeYPosition(position);
	LeaveCriticalSection(&csHardware);
	return(TS_SUCCESS);
}

/******************************************************************************/
/*  NT73XX SMEMA functions                                                    */
/******************************************************************************/
TSTATUS
TCHardwareIO::InitializeSMEMA(BOOL reinitialize)
{
	if (reinitialize == FALSE)
		m_pSmema = new TCSmema;

	m_pSmema->Initialize();

	return TS_SUCCESS;
}

TSTATUS
TCHardwareIO::SetOutBoardAvailable(SMEMA Avail)
{ //NO_BOARD_AVAILABLE, BOARD_AVAILABLE
	if (GetMode() == DIAGNOSTICS)
		m_pConveyors->SetDownSMEMA((Avail == NO_BOARD_AVAILABLE) ? 0 : 1);
	return(TS_SUCCESS);
};

TSTATUS
TCHardwareIO::SetInBoardBusy(SMEMA Busy)
{ // BUSY, NOTBUSY
	if (GetMode() == DIAGNOSTICS)
		m_pConveyors->SetUpSMEMA((Busy == BUSY) ? 0 : 1);
	return(TS_SUCCESS);
};

TSTATUS
TCHardwareIO::SetSmemaPass(ONOFF OnOff)
{
	if (GetMode() == DIAGNOSTICS)
		m_pConveyors->SetDownPass((OnOff) ? 1 : 0);
	return(TS_SUCCESS);
};

/******************************************************************************/
/*  user panel functions                                                      */
/******************************************************************************/
TSTATUS
TCHardwareIO::InitializeConsole(BOOL reinitialize)
{
	if (reinitialize == FALSE)
	{
		m_pConsole = new TCConsole(this);
		return m_pConsole->Initialize();
	}
	else
		return m_pConsole->ReInitialize();
}

/******************************************************************************/
/*  panel button (light) functions                                            */
/******************************************************************************/
/*
 * Functions:	SetPanelLight
 *
 * Description:	set the light to condition OnOff
 *
 * Parameters:
 *		TCIO_Bits	*iobits - light pointer
 *		ONOFF		 OnOff  - contition for light
 *
 * Return Values:
 *		TSTATUS
 *
 * Discussion:
 *				call the PCIO write function and set the Controller
 *				register for the light to condition ONOFF.
 *
 */
TSTATUS
TCHardwareIO::SetPanelLight(TCIO_Bits *iobits, ONOFF OnOff)
{
	TSTATUS ret = TS_SUCCESS;
	EnterCriticalSection(&csHardware);
		if (iobits)
			m_pPcio->Controller_ioWrite(iobits, (SHORT) OnOff);
		else
			ret = TS_ERROR;
	LeaveCriticalSection(&csHardware);

	return ret;
}

/*
 * Functions:	GetPanelLight
 *
 * Description:	get the condition of the switch
 *
 * Parameters:
 *		TCIO_Bits	*iobits - light pointer
 *		ONOFF		&OnOff  - contition
 *
 * Return Values:
 *		TSTATUS
 *
 * Discussion:
 *				call the PCIO read function and deposit the light's
 *				condition into the ONOFF variable.  
 *
 */
TSTATUS
TCHardwareIO::GetPanelLight(TCIO_Bits *iobits, ONOFF &OnOff)
{
	TSTATUS ret = TS_SUCCESS;
	EnterCriticalSection(&csHardware);
		USHORT onoff = 0;
		if (iobits)
			m_pPcio->Controller_ioRead(iobits, &onoff);
		else
			ret = TS_ERROR;
		OnOff = onoff ? cON : cOFF;
	LeaveCriticalSection(&csHardware);

	return ret;
}

/******************************************************************************/
/*  panel button (switch) functions                                           */
/******************************************************************************/
/*
 * Functions:	SetButton
 *
 * Description:	set the switch to condition OnOff
 *
 * Parameters:
 *		TCIO_Bits	*iobits - switch pointer
 *		ONOFF		 OnOff  - contition for switch (diagnostic only)
 *
 * Return Values:
 *		TSTATUS
 *
 * Discussion:
 *				call the PCIO write function and set the Controller
 *				register for the switch to condition ONOFF.  This
 *				function is for diagnostic only!
 *
 */
TSTATUS
TCHardwareIO::SetButton(TCIO_Bits *iobits, ONOFF OnOff)
{
	TSTATUS ret = TS_SUCCESS;
	EnterCriticalSection(&csHardware);
		if (iobits)
			m_pPcio->Controller_ioWrite(iobits, (SHORT) OnOff);
		else
			ret = TS_ERROR;
	LeaveCriticalSection(&csHardware);

	return ret;
}

/*
 * Functions:	GetButton
 *
 * Description:	get the condition of the switch
 *
 * Parameters:
 *		TCIO_Bits	*iobits - switch pointer
 *		ONOFF		&OnOff  - contition
 *
 * Return Values:
 *		TSTATUS
 *
 * Discussion:
 *				call the PCIO read function and deposit the switch's
 *				condition into the ONOFF variable.  
 *
 */
TSTATUS
TCHardwareIO::GetButton(TCIO_Bits *iobits, ONOFF &OnOff)
{
	TSTATUS ret = TS_SUCCESS;
	EnterCriticalSection(&csHardware);
		USHORT onoff = 0;
		if (iobits)
			m_pPcio->Controller_ioRead(iobits, &onoff);
		else
			ret = TS_ERROR;
		OnOff = onoff ? cON : cOFF;
	LeaveCriticalSection(&csHardware);

	return ret;
}

/*
 * Functions:	ClearInterrupt
 *
 * Description:	Clear the interrupt enable & the interrupt
 *
 * Parameters:
 *		TCIO_Bits	*iobits - switch pointer
 *
 * Return Values:
 *		TSTATUS
 *
 * Discussion:
 *				clear the interrupt enable for the designated system
 *				function.  Also clear the interrupt status register on
 *				the controller.
 *
 */
TSTATUS
TCHardwareIO::ClearInterrupt(TCIO_Bits *iobits)
{
	TSTATUS ret = TS_SUCCESS;
	EnterCriticalSection(&csHardware);
		if (iobits)
		{
			m_pPcio->ClearControllerInterruptStatus(iobits);
			m_pPcio->DisableController_ioInterrupt(iobits);
		}
		else
			ret = TS_ERROR;
	LeaveCriticalSection(&csHardware);

// CANT touch Interrup enables a the application level
	// reenable all other interrupts
//	EnableInterrupts();
	return ret;
};

/*
 * Functions:	EnableInterrupt
 *
 * Description:	Clear the interrupt enable & the interrupt
 *
 * Parameters:
 *		TCIO_Bits	*iobits - switch pointer
 *
 * Return Values:
 *		TSTATUS
 *
 * Discussion:
 *				set the interrupt enable for the designated system
 *				function.  Also clear the interrupt status register on
 *				the controller.
 *
 */
TSTATUS
TCHardwareIO::EnableInterrupt(TCIO_Bits *iobits)
{
	TSTATUS ret = TS_SUCCESS;
	EnterCriticalSection(&csHardware);
		if (iobits)
		{
			m_pPcio->ClearControllerInterruptStatus(iobits);
			m_pPcio->EnableController_ioInterrupt(iobits);
		}
		else
			ret = TS_ERROR;
	LeaveCriticalSection(&csHardware);
	return ret;
};

/*
 * Functions:	SetInterruptPolarity
 *
 * Description:	Set the interrupt polarity
 *
 * Parameters:
 *		TCIO_Bits	*iobits  - switch pointer
 *		int			polarity - pcioFALLING, pcioRISING,

 *
 * Return Values:
 *		TSTATUS
 *
 * Discussion:
 *				set the interrupt polarity for the designated system
 *				function.
 *
 */
TSTATUS
TCHardwareIO::SetInterruptPolarity(TCIO_Bits *iobits, int polarity)
{
	EnterCriticalSection(&csHardware);
		if (iobits)
			m_pPcio->SetInterruptPolarity(iobits, polarity);
	LeaveCriticalSection(&csHardware);
	return(TS_SUCCESS);
};


/*
 * Functions:	Diagnostic Interrupt
 *
 * Description:	process Diagnostic interrupt requests
 *
 *
 * Discussion:
 *			
 *
 */
TSTATUS
TCHardwareIO::TriggerDiagnosticInterrupt(void)
{
	EnterCriticalSection(&csHardware);
		if (m_Diagnostic)
			m_pPcio->TriggerController_ioInterrupt(m_Diagnostic);
	LeaveCriticalSection(&csHardware);

	return(TS_SUCCESS);
};

/*
 * Functions:	System Interlock
 *
 * Description:	
 *
 *
 * Discussion:
 *			
 *
 */
TSTATUS
TCHardwareIO::GetInterlock(ONOFF &OnOff)
{
	EnterCriticalSection(&csHardware);
		USHORT onoff = 1;  // software only; make like Interlocks are engaged
		if (m_SystemInterlock)
			m_pPcio->Controller_ioRead(m_SystemInterlock, &onoff);
		OnOff = onoff ? cOFF : cON;
	LeaveCriticalSection(&csHardware);

	return(TS_SUCCESS);
};

/*
 * Function:	GetConveyorStatus
 *
 * Description:	get the Conveyor2 status
 *
 * Parameters:
 *
 * Return Values:
 *		USHORT	- Optima Conveyor's status bits
 *
 * Discussion:
 *				used once each board load/eject, or
 *				continuously in diagnostics
 */
USHORT
TCHardwareIO::GetConveyorStatus(void)
{
	// get conveyor status
	m_ConveyorStatus = m_pConveyors->GetConveyorStatus();
	return m_ConveyorStatus;
};

BOOL
TCHardwareIO::IsBoardPresent(void)
{
	return (BOOL) m_pConveyors->IsBoardPresent();
};

/******************************************************************************/
/*  NT73XX conveyor functions                                                 */
/******************************************************************************/
TSTATUS
TCHardwareIO::InitializeBrdCtrl(BOOL reinitialize)
{
	if (reinitialize == FALSE)
	{
		m_pBrdCtrl = new TCBrdCtrl(this);
		m_pBrdCtrl->Initialize();
	}
	else	
		m_pBrdCtrl->ReInitialize();

	return TS_SUCCESS;
}


/*
 * Functions:	GetConveyorFunction
 *
 * Description:	get the condition of the conveyor function
 *
 * Parameters:
 *		TCIO_Bits	*iobits - conveyor function pointer
 *		ONOFF		&OnOff  - contition
 *
 * Return Values:
 *		TSTATUS
 *
 * Discussion:
 *				call the PCIO read function and deposit the conveyor
 *				function's condition into the ONOFF variable.  
 *
 */
TSTATUS
TCHardwareIO::GetConveyorFunction(TCIO_Bits *iobits, ONOFF &OnOff)
{
	TSTATUS ret = TS_SUCCESS;
	EnterCriticalSection(&csHardware);
		USHORT onoff = 0;
		if (iobits)
			m_pPcio->Controller_ioRead(iobits, &onoff);
		else
			ret = TS_ERROR;
		OnOff = onoff ? cON : cOFF;
	LeaveCriticalSection(&csHardware);

	return ret;
}

TSTATUS
TCHardwareIO::GetConveyorBypass(ONOFF &OnOff)
{
	TSTATUS ret = TS_SUCCESS;
	ONOFF onoff = cOFF;

	ret = GetConveyorFunction(m_ConveyorBypass, onoff);
	OnOff = onoff ? cOFF : cON; // toggle
	//20080917, morgan hsiao
	//to prevent some of system controller has wrong value since installed
	//at 7355 or 5500, and also 735 and 5500 do not equip with inline
	//conveyor so they do not need a conveyor bypass switch
	//and related mechanism.
	if(HWGetHardwareIO()->GetSystemType() == NT5500 || HWGetHardwareIO()->GetSystemType() == NT7355)
		OnOff = cOFF;

	return ret;
};

TSTATUS
TCHardwareIO::GetLowAir(ONOFF &OnOff)
{
	TSTATUS ret = TS_SUCCESS;
	ONOFF onoff = cOFF;

	ret = GetConveyorFunction(m_LowAir, onoff);
	OnOff = onoff ? cOFF : cON;

	return ret;
};

/*
 * Function:	GetClampInstalled()
 *
 * Description:	determine if the Air Option is installed 
 *
 * Parameters:
 *		none
 *
 * Return Values:
 *		BOOL	- FALSE == not installed
 *				- TRUE  == installed
 *
 * Discussion:
 *
 *		
 */
CONVEYORCLAMPTYPE
TCHardwareIO::GetClampInstalled(void)
{
	ONOFF lowair    = cOFF;
	ONOFF installed = cOFF;
	ONOFF motorstaging = cOFF;

	if (GetMode() == SOFTWARE_ONLY)
		return MOTORTYPE;
	else
	{
		// lowair:     cON == normal, cOFF == a lowAir condition
		GetConveyorFunction(m_LowAir, lowair);
		// installed:  cON == no Air, cOFF == Air
		GetConveyorFunction(m_AirInstalled, installed);
		// MotorStaging: cON == no motor staging clamper, cOFF == motor staging 
		GetConveyorFunction(m_MotorStagingInstalled, motorstaging);

		// lowair,	installed, motorstaging
		// cON,		cON,		cON			: Motor
		// cOFF,	cON	,		cON			: AirLow
		// cON,		cOFF,		cON			: Air
		// X,		X,			cOFF		: MotorStaging
		if(motorstaging == cOFF)
			return MOTORSTAGETYPE;
		else if(lowair == cON && installed == cON)
			return MOTORTYPE;
		else
			return AIRTYPE;
	}
};

/*
 * Function:	InitializeConveyors()
 *
 * Description:	initialize the 7300 conveyor 
 *
 * Parameters:
 *		none
 *
 * Return Values:
 *		TSTATUS
 *
 * Discussion:
 *
 *		get the conveyor comport from the registry and send it
 *		to the 7300 conveyor's init-function.  If an error when
 *		reading from the registry, use 'COM2:' as default.
 *
 */
TSTATUS
TCHardwareIO::InitializeConveyor2(BOOL reinitialize)
{
	// inform user
	TCWaitCursor wait("Initializing Conveyor System");

	if (reinitialize == FALSE)
		m_pConveyors = (TCConveyors *) new TCCnvyr(this);

	TSTATUS returnCode;
	// check for the Conveyor Bypass switch
	HARDWAREMODE mode = (m_pConsole->GetConveyorBypass() == TRUE) ?
									SOFTWARE_ONLY : GetMode();
	if (reinitialize)
		returnCode = m_pConveyors->ReInitialize(mode);
	else
		returnCode = m_pConveyors->InitCnvyrController(mode);

	m_ConveyorInitialized = TRUE;

	return returnCode;
}

TSTATUS
TCHardwareIO::InitializeConveyorUMSCO(BOOL reinitialize)
{
	// inform user
	TCWaitCursor wait("Initializing Conveyor System");

	if (reinitialize == FALSE)
		m_pConveyors = new TCCnvyrUMSCO(this);

	TSTATUS returnCode;
	// check for the Conveyor Bypass switch
	HARDWAREMODE mode = (m_pConsole->GetConveyorBypass() == TRUE) ?
									SOFTWARE_ONLY : GetMode();
	if (reinitialize)
		returnCode = m_pConveyors->ReInitialize(mode);
	else
		returnCode = m_pConveyors->InitCnvyrController(mode);

	m_ConveyorInitialized = TRUE;

	return returnCode;
}

TSTATUS	
TCHardwareIO::InitializeConveyorMC(BOOL reinitialize)
{
	// inform user
	TCWaitCursor wait("Initializing Conveyor System");

	if (reinitialize == FALSE)
		m_pConveyors = (TCConveyors *) new CCnvMC(this);

	TSTATUS returnCode;

	HARDWAREMODE mode = GetMode();
	if(!m_pLMotors->IsMotorDriverPresent())
			mode = SOFTWARE_ONLY;
	if (reinitialize)
		returnCode = m_pConveyors->ReInitialize(mode);
	else
		returnCode = m_pConveyors->InitCnvyrController(mode);

	m_ConveyorInitialized = TRUE;

	return returnCode;
}

int
TCHardwareIO::ForceEjectBoard(void)
{
	// set the conveyor to NON_SMEMA
	BOOL smema = GetSmemaEnabled();
	
	SetSmemaEnabled(FALSE);

	// eject the board (sync the StateController)

	TCEventData msg;
	msg.m_SendersName = _T("TCMechIO: Force EjectBoard");
	msg.m_Command = _T("");
	msg.m_event = AOI_QUERY_EJECTFLAG;
    EMBroadcastEvent(msg.m_event, &msg);
	if(msg.m_iRet == TRUE)
		return TRUE;
	// force eject board, that event only work on watchdog on
	m_pConveyors->EjectBoard( TRUE );

	m_pConveyors->IsBoardEjected();

	//20080808, morgan, reset the board eject flag of TCSCMain.
	msg.m_event = AOI_SET_EJECTFLAG;
	msg.m_iRet = TRUE;
    EMBroadcastEvent(msg.m_event, &msg);
	// reset conveyor
	if (smema)
		SetSmemaEnabled(TRUE);

	return TRUE;
};

void TCHardwareIO::ForceLoadBoard(void)
{
	TCEventData msg;
	m_pConveyors->LoadBoard();
	//20080808, morgan, reset the board eject flag of TCSCMain.
	msg.m_event = AOI_SET_EJECTFLAG;
	msg.m_iRet = FALSE;
    EMBroadcastEvent(msg.m_event, &msg);
};

void
TCHardwareIO::ConveyorCancel(void)
{
	m_pConveyors->Cancel();
};

CString
TCHardwareIO::GetConverorFirmwareRevision(void)
{
	return m_pConveyors->GetConverorFirmwareRevision();
};

BOOL
TCHardwareIO::GetAutoWidthEnabled(void)
{
	return m_pConveyors->GetAutoWidthEnable();
};

BOOL
TCHardwareIO::GetSmemaEnabled(void)
{
	return m_pSmema->GetSmemaEnabled();
};

void
TCHardwareIO::SetSmemaEnabled(BOOL smema)
{
	m_pSmema->SetSmemaEnabled(smema);
	m_pConveyors->Cancel();
	m_pConveyors->SetSMEMAMode((SMEMA_MODE) smema);
};

BOOL
TCHardwareIO::GetLiftersEnabled(void)
{
	return m_pLifters->GetLifterStatus();
};

CLIFTER
TCHardwareIO::GetLiftersMode(void)
{
	return m_pLifters->GetLifterMode();
};

void
TCHardwareIO::SetLiftersMode(CLIFTER lifter)
{
	m_pLifters->SetLifterMode(lifter);
	ConveyorCancel();
	m_pConveyors->SetLifterMode(lifter);
};

void
TCHardwareIO::EngageLifter(BOOL direction)
{
	m_pConveyors->EnableLifter(direction);
};

BOOL
TCHardwareIO::IsAutoWidthInstalled(void)
{
	return m_pConveyors->IsAutoWidthInstalled();
};

BOOL
TCHardwareIO::IsInsertMode(void)
{
	return m_pBrdCtrl->IsInsertMode();
};

BOOL
TCHardwareIO::IsExtensionInstalled(void)
{
	return m_pBrdCtrl->IsExtensionInstalled();
};

BOOL
TCHardwareIO::IsConveyorInFastMode(void)
{	 //F404 vc6ぉㄴ쨢쬟간
	//return (IsConveyorInStagingMode() == FALSE &&
	//			(m_pBrdCtrl->GetEjectDirection() == FAST_L2R ||
	//			 m_pBrdCtrl->GetEjectDirection() == FAST_R2L) );

	return (m_pBrdCtrl->GetEjectDirection() == FAST_L2R);
};

BOOL
TCHardwareIO::IsConveyorInStagingMode(void)
{
	return m_pBrdCtrl->IsConveyorInStagingMode();
};

BOOL
TCHardwareIO::IsConveyorInHalfLoadMode(void)
{
	return m_pBrdCtrl->IsConveyorInHalfLoadMode();
};

CONVEYOR_EXTENSION
TCHardwareIO::GetConveyorExtension(void)
{
	return m_pConveyors->GetConveyorExtensionMode();
};

TSTATUS
TCHardwareIO::ConveyorAlignBoard(void)
{
	if (GetMode() == SOFTWARE_ONLY)
		return TS_SUCCESS;
	else
		return m_pBrdCtrl->ConveyorAlignBoard();
}

TSTATUS
TCHardwareIO::SetBoardWidth(int width)
{
	if (GetMode() != SOFTWARE_ONLY)
	{
		if (m_pConveyors->GetAutoWidthHomed() == FALSE && HomeWidthMotor() == TS_ERROR) // ???
			return TS_ERROR;

		if (m_pConveyors->AutoWidthBoardDetect() == TS_SUCCESS &&
					m_pConveyors->SetBoardWidth((UINT) width) == CNVYR_NOT_BUSY)
			return IsAutoWidthDone();
		return TS_ERROR;
	}
	else
		return TS_SUCCESS;
}

TSTATUS
TCHardwareIO::SetBoardWidthInc(int inch_cm, int increment)
{
	// inform user
	TCWaitCursor wait("Changing Conveyor Width");

	if (inch_cm == 0) // inches
		m_jogInc = 10000;
	else // CM
		m_jogInc = 3940;

	switch (increment)
	{
		case 1:
			m_jogInc /= 10;
			break;
		case 2:
			m_jogInc /= 100;
			break;
		case 3:
			m_jogInc /= 1000;
			break;
		default:
		case 0:
			break;
	}
	return TS_SUCCESS;
}

TSTATUS
TCHardwareIO::SetBoardWidthRelative(int width)
{
	if (GetMode() != SOFTWARE_ONLY)
	{
		if (m_pConveyors->GetAutoWidthHomed() == FALSE && HomeWidthMotor() == TS_ERROR)
			return TS_ERROR;

		if (m_pConveyors->AutoWidthBoardDetect() == TS_SUCCESS &&
					m_pConveyors->SetBoardWidthRelative(width) == CNVYR_NOT_BUSY)
			return IsAutoWidthDone();
		return TS_ERROR;
	}
	else
		return TS_SUCCESS;
}

TSTATUS
TCHardwareIO::HomeWidthMotor(void)
{
	// inform user
	TCWaitCursor wait("Auto Width Homing");

	if (GetMode() != SOFTWARE_ONLY)
	{
//		DWORD id;

		ParkXYMotors();
		if(this->m_SystemType == NT7355 || this->m_SystemType == NT5500)
			m_pConveyors->EjectBoard(TRUE);
		else
			m_pConveyors->EjectToOutput();
		
		m_pConveyors->IsBoardEjected();

		if (m_pConveyors->AutoWidthBoardDetect() == TS_SUCCESS &&
					m_pConveyors->HomeWidthMotor() == CNVYR_NOT_BUSY)
		{
			if (IsAutoWidthDone() == TS_SUCCESS)
//				SetEvent(hardware->GetAWEvent()); //F404 vc6ぉㄴ쨢쬟간
				return TS_SUCCESS;
			else
				return TS_ERROR;
		}
		// start the thread for autowidth
/*  // F404 vc6ぉㄴ쨢쬟간
		TRACE(_T("<<TCHardwareIO::HomeWidthMotor(): create worker thread\n"));
		m_hAutoWidthThread = CreateThread(NULL, 0, &HomeWidthMotorThread, this, NULL, &id);
		ASSERT(m_hAutoWidthThread);
*/
	}
/*  // F404 vc6ぉㄴ쨢쬟간
	HANDLE handles[2] = {m_hCancelEvent, m_hAutoWidthEvent};

	if (WaitForMultipleObjects(2, handles, FALSE, INFINITE) == WAIT_OBJECT_0 + 1)
		return TS_SUCCESS;
	else
		return TS_ERROR;
*/
		return TS_SUCCESS;


};

DWORD WINAPI TCHardwareIO::InitMotorThread(void *arg)
{
	TRACE(_T("<<TCHardwareIO::InitMotorThread():  begin>>\n"));
	UADDTIMESTAMP(_T("HW: Start InitMotorThread()"));

	TCHardwareIO *hardware = (TCHardwareIO *) arg;

	TSTATUS returnCode = TS_ERROR;
	hardware->m_motorreturnCode = returnCode;

	for(int i = 0; i < 30; i++)
	{
		if(Timeout30 <= 0 ) break;

		if (hardware->m_reinitialize)
		{
			returnCode = hardware->m_pLMotors->ReInitialize(hardware->m_MotorComPort.GetBuffer(0), HARDWARE_ENABLED);
		}
		else
		{
			returnCode = hardware->m_pLMotors->InitMotorController(hardware->m_MotorComPort.GetBuffer(0),
				HARDWARE_ENABLED);
		}

		if( returnCode == TS_SUCCESS) break;

		Sleep(1000);
	}

	hardware->m_motorreturnCode = returnCode;

	UADDTIMESTAMP(_T("HW: End InitMotorThread()"));
	TRACE(_T("<<TCHardwareIO::InitMotorThread():  end>>\n"));
	return NULL;

}

DWORD WINAPI
TCHardwareIO::HomeWidthMotorThread(void *arg)
{
	TRACE(_T("<<TCHardwareIO::HomeWidthMotorThread():  begin>>\n"));
	UADDTIMESTAMP(_T("HW: Start HomeWidthMotorThread()"));

	TCHardwareIO *hardware = (TCHardwareIO *) arg;
	TCConveyors  *conveyors = hardware->GetConveyorPtr();

	if (conveyors->AutoWidthBoardDetect() == TS_SUCCESS &&
				conveyors->HomeWidthMotor() == CNVYR_NOT_BUSY)
	{
		if (hardware->IsAutoWidthDone() == TS_SUCCESS)
			SetEvent(hardware->GetAWEvent());
	}

	CloseHandle(hardware->m_hAutoWidthThread);
	hardware->m_hAutoWidthThread = NULL;

	UADDTIMESTAMP(_T("HW: End HomeWidthMotorThread()"));
	TRACE(_T("<<TCHardwareIO::HomeWidthMotorThread():  end>>\n"));
	return NULL;
};

TSTATUS
TCHardwareIO::IsAutoWidthDone(void)
{   
	if (GetMode() != SOFTWARE_ONLY)
	{
		return ((m_pConveyors->IsAutoWidthDone() == TRUE) ?
													TS_SUCCESS : TS_ERROR);
	}
	else
		return TS_SUCCESS;
}

int
TCHardwareIO::GetBoardWidth(void)
{
	if (GetMode() == SOFTWARE_ONLY || m_pConveyors->GetAutoWidthHomed() == FALSE)
		return -1;
	return (int) m_pConveyors->GetBoardWidth();
};

TSTATUS
TCHardwareIO::SetBoardWeight(int weight)
{
	if (GetMode() != SOFTWARE_ONLY)
		m_pConveyors->SetBoardWeight(weight);
	return TS_SUCCESS;
}

/******************************************************************************/
/*  conveyor lifter functions                                                 */
/******************************************************************************/
TSTATUS
TCHardwareIO::InitializeLifters(BOOL reinitialize)
{
	// inform user
	TCWaitCursor wait("Initializing Center Lifters");

	if (reinitialize == FALSE)
		m_pLifters = new TCLifters(this);

	return m_pLifters->Initialize();
};

TSTATUS
TCHardwareIO::SetLifters(ONOFF OnOff)
{
	if (GetMode() != SOFTWARE_ONLY)
		m_pConveyors->EnableLifter(OnOff ? TRUE : FALSE);
	return TS_SUCCESS;
};

/******************************************************************************/
/*  systrem alarm functions                                                   */
/******************************************************************************/
TSTATUS
TCHardwareIO::InitializeBeepers(BOOL reinitialize)
{
	if (reinitialize == FALSE)
		m_pBeeper = new TCBeeper(this);
	
	m_pBeeper->Initialize();

	return TS_SUCCESS;
}

/******************************************************************************/
/*  systrem light tower functions                                             */
/******************************************************************************/
TSTATUS
TCHardwareIO::InitializeLtTower(BOOL reinitialize)
{
	if (reinitialize == FALSE)
		m_pLtTower = new TCLtTower(this);
	
	m_pLtTower->Initialize();

	return TS_SUCCESS;
}

void
TCHardwareIO::InitializeLtTowerLights(ONOFF red, ONOFF yellow, ONOFF green)
{
	m_pPcio->InitializeLtTower(red, yellow, green);
};

/*
 * Functions:	SetTower
 *
 * Description:	set the tower light to condition OnOff
 *
 * Parameters:
 *		TCIO_Bits	*iobits - light pointer
 *		ONOFF		 OnOff  - contition for light
 *
 * Return Values:
 *		TSTATUS
 *
 * Discussion:
 *				call the PCIO write function and set the Controller
 *				register for the light in the light tower to conditio
 *				ONOFF.
 *
 */
TSTATUS
TCHardwareIO::SetTower(TCIO_Bits *iobits, ONOFF OnOff)
{
	TSTATUS ret = TS_SUCCESS;
	EnterCriticalSection(&csHardware);
		if (iobits)
			m_pPcio->Controller_ioWrite(iobits, (SHORT) OnOff);
		else
			ret = TS_ERROR;
	LeaveCriticalSection(&csHardware);

	return ret;
}

/*
 * Functions:	GetTower
 *
 * Description:	get the condition of the tower light
 *
 * Parameters:
 *		TCIO_Bits	*iobits - switch pointer
 *		ONOFF		&OnOff  - contition
 *
 * Return Values:
 *		TSTATUS
 *
 * Discussion:
 *				call the PCIO read function and deposit the tower light
 *				condition into the ONOFF variable.  
 *
 */
TSTATUS
TCHardwareIO::GetTower(TCIO_Bits *iobits, ONOFF &OnOff)
{
	TSTATUS ret = TS_SUCCESS;
	EnterCriticalSection(&csHardware);
		USHORT onoff = 0;
		if (iobits)
			m_pPcio->Controller_ioRead(iobits, &onoff);
		else
			ret = TS_ERROR;
		OnOff = (ONOFF) onoff;
	LeaveCriticalSection(&csHardware);

	return ret;
}

void
TCHardwareIO::EnableLtWatchdog(int timeout)
{
	m_pPcio->EnableLtWatchdog(timeout);
};

void
TCHardwareIO::ResetLtWatchdog(int timeout)
{
	m_pPcio->ResetLtWatchdog(timeout);
};

/******************************************************************************/
/*  XY Table Motors functions                                                 */
/******************************************************************************/
/*
 * Function:	InitializeMotors()
 *
 * Description:	initialize the XY Table motors
 *				*private*
 *
 * Parameters:
 *		HARDWAREMODE -	HAREWARE_ENABLED
 *						SOFTWARE_ONLY
 *
 * Return Values:
 *		TSTATUS
 *
 * Discussion:
 *
 *		get the XY Table comport from the registry and send it
 *		to the 7300 motor's init-function.  If an error when
 *		reading from the registry, use 'COM1:' as default.
 *
 */
TSTATUS
TCHardwareIO::InitializeMotors(HARDWAREMODE mode, BOOL reinitialize)
{
	TCWaitCursor wait("Initializing XYMotors");

	long licSysType = (AoiGetApp()->GetLicenseModel() + AoiGetApp()->GetIsThatModel())->lModelNumber;
	CString msg;
	CString lsType;
	lsType.Format("%d",licSysType);

	//if (reinitialize == FALSE)
	{
		// detect the system type
		//20080917, morgan hsiao
		//move the statements of reading dongle and recognizing what type of 
		//dongle are to the beginning of InitializMotors, this can avoid
		//allocating an object MCMotor when inserted dongle is for 7300
		if(reinitialize)
		switch( m_pLMotors->m_iMotorType )
		{
		case MT_LMOTOR:
			delete (TCLMotor*)m_pLMotors;
			break;
		case MT_MCMOTOR:
			delete (MCMotor*) m_pLMotors;
			break;
		case MT_ACSCMOTOR:
		case MT_ACSCMOTOR2:
			delete (ACSCMotor*) m_pLMotors;
			break;
		}
		m_pLMotors = NULL;
		reinitialize = FALSE;
		if(licSysType == 5500001 || licSysType == 7355001)
			m_pLMotors = new MCMotor();
		else if(licSysType == 7300001 || licSysType == 7300002)//if(!m_pLMotors->IsControllerExist())
		{
			delete (MCMotor*) m_pLMotors;
			m_pLMotors = NULL;

			if(mode == SOFTWARE_ONLY)
			{
				m_pLMotors = new TCLMotor();
			}
			else
			{
				m_pLMotors = new TCLMotor();
				if(!m_pLMotors->IsControllerExist())
				{
					delete (TCLMotor*)m_pLMotors;
					m_pLMotors = NULL;
					m_pLMotors = new ACSCMotor();
				}
			}
		}
		else
		{
			//ex: 7200 or offline application
		}
	}
		
	switch (licSysType)
	{
	case 7300001L:
	case 7300002L:
		if(m_SystemType == NT5500 || m_SystemType == NT7355)
		{
			msg.Format("You are licensed on %s-%s, it not allow to run the %s in production mode.\n Switching to %s-%s",
				lsType.Mid(0,4),lsType.Mid(6,1), SystemTypeToString( m_SystemType), lsType.Mid(0,4),lsType.Mid(6,1));
			AfxMessageBox( msg );
			m_SystemType = NT7300;
			GetSysReg()->sysv_SystemType = (int) m_SystemType;

			if(mode != SOFTWARE_ONLY && m_pLMotors->IsControllerExist())
			{
				mode = SOFTWARE_ONLY;
				SetMode(mode);
			}
		}

		break;
	case 5500001:
		if(m_SystemType != NT5500 && mode != SOFTWARE_ONLY)
		{
			msg.Format("You are licensed on %s-%s, it not allow to run the %s in production mode.\n Switching to %s-%s",
				lsType.Mid(0,4),lsType.Mid(6,1), SystemTypeToString( m_SystemType), lsType.Mid(0,4),lsType.Mid(6,1));
			AfxMessageBox( msg );
			m_SystemType = NT5500;
			GetSysReg()->sysv_SystemType = (int) m_SystemType;

			if(mode != SOFTWARE_ONLY && !m_pLMotors->IsControllerExist())
			{
				mode = SOFTWARE_ONLY;
				SetMode(mode);
			}
		}
		MCSetMCMotorParameters(m_SystemType);

		break;
	case 7355001:
		if(m_SystemType != NT7355 && mode != SOFTWARE_ONLY)
		{
			msg.Format("You are licensed on %s-%s, it not allow to run the %s in production mode.\n Switching to %s-%s",
				lsType.Mid(0,4),lsType.Mid(6,1), SystemTypeToString( m_SystemType), lsType.Mid(0,4),lsType.Mid(6,1));
			AfxMessageBox( msg );
			m_SystemType = NT7355;
			GetSysReg()->sysv_SystemType = (int) m_SystemType;

			if(mode != SOFTWARE_ONLY && !m_pLMotors->IsControllerExist())
			{
				mode = SOFTWARE_ONLY;
				SetMode(mode);
			}
		}
		MCSetMCMotorParameters(m_SystemType);

		break;
	default:
			msg.Format("You are licensed on %s-%s, it not allow to run the %s in production mode.\n Switching to %s-%s",
				lsType.Mid(0,4),lsType.Mid(6,1), SystemTypeToString( m_SystemType), lsType.Mid(0,4),lsType.Mid(6,1));
		AfxMessageBox( msg );
		mode = SOFTWARE_ONLY;
		SetMode(mode);
		break;
	}


	double fieldofview = 0.7f;
	m_MotorFOV         = PointSevenInch;
	CString fov;

	if (Read73Registry("Cameras", "FOV", fov) == ERROR_SUCCESS)
	{ // no registry key, force 0.7
		fieldofview = atof(fov);
		if (fieldofview > 0.29 && fieldofview < 0.31)
			m_MotorFOV = PointThreeInch;
		else if (fieldofview > 0.39 && fieldofview < 0.41)
			m_MotorFOV = PointFourInch;
		else if (fieldofview > 0.59 && fieldofview < 0.61)
			m_MotorFOV = PointSixInch;
		else if (fieldofview > 0.69 && fieldofview < 0.71)
			m_MotorFOV = PointSevenInch;
		else if (fieldofview > 0.99 && fieldofview < 1.01)
			m_MotorFOV = OneInch;
		else // default
			m_MotorFOV = PointSevenInch;
	}

// remove when new registry is complete, see below(* see here *)
	if (Read73Registry("Factory", "MotorPercentSpeed", m_MotorSpeed) !=
		ERROR_SUCCESS)
	{ // no registry key, force COM 1
		m_MotorSpeed = 95;
	}

	// initialize
	m_MotorComPort.Format ("COM%d:", GetSysReg()->sysv_MotorControllerPort +1);
	DWORD motors;
	TSTATUS returnCode;
	if(m_pLMotors->m_iMotorType == MT_ACSCMOTOR && mode == HARDWARE_ENABLED)
	{
		DWORD id;
		Timeout30 = 35;
		m_reinitialize = reinitialize;
		m_hInitmotorThread = ::CreateThread( NULL, 0, InitMotorThread, this, 0, &id);

		//waiting the worest case 30sec for controller power on
		while( Timeout30-- > 0 )
		{
			Sleep(1000);
			//if(m_motorreturnCode == TS_SUCCESS) //F404 vc6ぉㄴ쨢쬟간
			if( WaitForSingleObject( m_hInitmotorThread, 1000 ) == WAIT_OBJECT_0 )
			{
				CloseHandle(m_hInitmotorThread);
				m_hInitmotorThread = NULL;
				returnCode = TS_SUCCESS;
				break;
			}
			else
			{
				TCException e = TCEXCEPTION1(HW_INITIALIZING_SEC, Timeout30);
				//msg.Format("Initializing...%.2d sec\r\n", Timeout30); //F404 vc6ぉㄴ쨢쬟간
				msg.Format("%s",e.getMessage());
				InitializeDialogUpdate(INIT_MOTORCONTROLLER, IDLG_REPORT, (LPCTSTR)msg);
			}
		}
	}
	else
	{
		if (reinitialize)
			returnCode = m_pLMotors->ReInitialize(m_MotorComPort.GetBuffer(0), mode);
		else
			returnCode = m_pLMotors->InitMotorController(m_MotorComPort.GetBuffer(0),
				mode);
	}

// * see here *
//	m_MotorSpeed = GetSysReg()->sysv_MotorPercentSpeed; //F404 vc6ぉㄴ쨢쬟간
	if (mode != HARDWARE_FLASH)
	{
		if (returnCode == TS_SUCCESS)
		{
			m_pLMotors->SetFOV((FOV) m_MotorFOV,
				(double)((double) m_MotorSpeed / 100.0));

			if(m_SystemType == NT7355)
				((MCMotor*)m_pLMotors)->PrepareHome();
				
			TCException e = TCEXCEPTION(HW_INITIALIZING_HOME);
			InitializeDialogUpdate(INIT_MOTORCONTROLLER, IDLG_REPORT, e.getMessage());
			//InitializeDialogUpdate(INIT_MOTORCONTROLLER, IDLG_REPORT, "Initializing...Home"); //F404 vc6ぉㄴ쨢쬟간
			m_pLMotors->HomeXY(Both);

			if (m_pLMotors->GetMotorError() == FALSE)
				motors = HW_PASSED;
			else
			{
				returnCode = TS_ERROR;
				motors = HW_FAILED;
			}
		}
		else
		{
			m_pLMotors->SetFOV((FOV) m_MotorFOV,
				(double)((double) m_MotorSpeed / 100.0));
			motors = HW_FAILED;
		}
	}

	m_MotorComPort.ReleaseBuffer();
	GetSysReg()->sysv_HWTMotors = motors;

	InitializeDialogUpdate(INIT_MOTORCONTROLLER, IDLG_REPORT, "Initializing...Reset Dome");

	ResetDomePosition();

	m_MotorsInitialized = TRUE;

	return returnCode;
}

/*
 * Function:	SetMotorPercentSpeed( int percentSpeed )
 *
 * Description:	set the motors to a Percentage of the normal FOV speed.overwritten by scanner
 *
 * Parameters:
 *		int -	percentSpeed
 *
 * Return Values:
 *		none
 *
 * Discussion:
 *
 */

void 
TCHardwareIO::SetMotorPercentSpeed( int percentSpeed )
{
	m_MotorSpeed = percentSpeed;
	m_pLMotors->SetFOV((FOV) m_MotorFOV,
				(double)((double) m_MotorSpeed / 100.0));
}

/*
 * Function:	SetMotorSpeed()
 *
 * Description:	set the motors to a Percentage of the normal FOV speed
 *
 * Parameters:
 *		double -	speedFator (0.0 - 2.0)
 *
 * Return Values:
 *		MOTOR_NOT_BUSY
 *
 * Discussion:
 *
 */
int
TCHardwareIO::SetMotorSpeed(double speed)
{	
	return m_pLMotors->SetMotorSpeed(speed);	
};

/*
 * Function:	SetXYMotorSpeed()
 *
 * Description: set the motors to a speed in microns/sec
 *
 * Parameters:	int speed (speed in microns/sec)
 *
 * Return Values: none
 *
 */
void
TCHardwareIO::SetXYMotorSpeed(int speed)
{
	m_pLMotors->SetXYMotorSpeed(speed);
};

/*
 * Function:	HomeXY(), et al.
 *
 *
 * Discussion:
 *
 *		if the system mode id not SOFTWARE_ONLY, all motor
 *		commands are sent to the Motor object.
 *
 *		It is possible of the 'm_Mode' to be SOFTWARE_ONLY if
 *		the EMO has been 'tripped'
 */
void
TCHardwareIO::HomeXY(MOTOR_AXIS axis)
{
	if (GetMode() != SOFTWARE_ONLY)
		m_pLMotors->HomeXY(axis);

	ResetDomePosition(axis);
}

int
TCHardwareIO::GetXYMotorSpeed(void)
{
	return m_pLMotors->GetXYMotorSpeed();
};

int
TCHardwareIO::GetAxisPosition(MOTOR_AXIS axis)
{
	if (GetMode() != SOFTWARE_ONLY)
		return m_pLMotors->GetAxisPosition(axis);
	else
		return MOTOR_NOT_BUSY;
};

void
TCHardwareIO::SetParkPosition(int x, int y)
{
	m_pLMotors->SetParkPosition( x, y );
}

int
TCHardwareIO::GetAxisPosition(int &X, int &Y)
{
	BOOL speedup = FALSE;

	if (GetMode() != SOFTWARE_ONLY)
	{
		speedup = (GetSysReg()->sysv_SUFlag == 1234567) ? TRUE : FALSE;
		if(speedup == TRUE)
		{
			GetDomeXPosition(X);
			GetDomeYPosition(Y);
			return TRUE;
		}
		else
			return m_pLMotors->GetAxisPosition(X, Y);
	}
	else
	{
		X = X_PARK;
		Y = Y_PARK;
		return MOTOR_NOT_BUSY;
	}
};

#define PARKMARGIN 75000
void
TCHardwareIO::ParkXYMotors(void)
{
	if (GetMode() != SOFTWARE_ONLY)
	{
		if( m_SystemType == NT5500 || m_SystemType == NT7355)
			return;
		SendEvent(AOI_PARK_MOTORS, _T("TCMechIO: Move Camerahead"));
		this->m_pLMotors->IsMoveComplete();
	}
};

float
TCHardwareIO::MoveXYAbsolute(int XPos, int YPos)
{
	return m_pLMotors->MoveXYAbsolute(XPos, YPos);	
};

float
TCHardwareIO::MoveXYCoordinated(int XDir, int YDir, bool wait)
{
	return m_pLMotors->MoveXYCoordinated(XDir, YDir, wait);	
};

float
TCHardwareIO::MoveXYSequence(int X1, int Y1, int X2, int Y2, int X3, int Y3, bool wait)
{
	return m_pLMotors->MoveXYSequence(X1,Y1, X2,Y2, X3,Y3, wait);
};

float
TCHardwareIO::IsMoveComplete(MOTOR_AXIS axis)
{
	return m_pLMotors->IsMoveComplete(axis);
};

int
TCHardwareIO::Step(MOTOR_AXIS axis, STEPSIZE step, DIRECTION dir)
{
	if (GetMode() != SOFTWARE_ONLY)
		return m_pLMotors->Step(axis, step, dir);
	else
		return MOTOR_NOT_BUSY;
};

int
TCHardwareIO::EngageMotors(void)
{
	if (GetMode() != SOFTWARE_ONLY)
		return m_pLMotors->EngageMotors();
	else
		return MOTOR_NOT_BUSY;
};

int
TCHardwareIO::DisEngageMotors(void)
{
	if (GetMode() != SOFTWARE_ONLY)
		return m_pLMotors->DisEngageMotors();
	else
		return MOTOR_NOT_BUSY;
};

int
TCHardwareIO::SetXYTargetRadius(UINT TargetRadius)
{
	if (GetMode() != SOFTWARE_ONLY)
		return m_pLMotors->SetXYTargetRadius(TargetRadius);
	else
		return MOTOR_NOT_BUSY;
};

MOTOR_STATUS
TCHardwareIO::ReadLimitSwitches(void)
{
	if (GetMode() != SOFTWARE_ONLY)
		return m_pLMotors->GetMotorStatus();
	else
	{
		MOTOR_STATUS m_status;
		m_status.Status = 0;

		return m_status;
	}
};

int 
TCHardwareIO::SetEstimateMode(bool emode)
{
	return m_pLMotors->SetEstimateMode(emode);	
};

int
TCHardwareIO::GetXUpperLimit()
{
	return m_pLMotors->GetXUpperLimit();
};

int
TCHardwareIO::GetXLowerLimit()
{
	return m_pLMotors->GetXLowerLimit();
};

int
TCHardwareIO::GetYUpperLimit()
{
	return m_pLMotors->GetYUpperLimit();
};

int
TCHardwareIO::GetYLowerLimit()
{
	return m_pLMotors->GetYLowerLimit();
};

/******************************************************************************/
/*  laser functions                                                           */
/******************************************************************************/
TSTATUS
TCHardwareIO::InitializeLaser(BOOL reinitialize)
{
	if (reinitialize == FALSE)
		m_pLaser = new TCLaser(this);

	return m_pLaser->Initialize();
};

TSTATUS
TCHardwareIO::GetLaser(ONOFF &OnOff)
{
	TSTATUS ret;
	ONOFF onoff;
	ret =  GetMisc(m_Laser, onoff);
	if (GetMode() != SOFTWARE_ONLY) // NT73XX '0' == ON
		OnOff = (onoff) ? cOFF : cON;
	return ret;
};

TSTATUS
TCHardwareIO::SetLaserTrigger(LASERTRIGGER Trigger)
{
	TSTATUS ret = TS_SUCCESS;
	EnterCriticalSection(&csHardware);
		if (m_LaserPolarity)
			m_pPcio->Controller_ioWrite(m_LaserPolarity, (SHORT) Trigger);
		else
			ret = TS_ERROR;
	LeaveCriticalSection(&csHardware);

	return ret;
};

TSTATUS
TCHardwareIO::GetLaserTrigger(LASERTRIGGER &Trigger)
{
	TSTATUS ret = TS_SUCCESS;
	EnterCriticalSection(&csHardware);
		USHORT trigger = 0;
		if (m_LaserPolarity)
			m_pPcio->Controller_ioRead(m_LaserPolarity, &trigger);
		else
			ret = TS_ERROR;
		Trigger = trigger ? RISINGEDGE : FALLINGEDGE;
	LeaveCriticalSection(&csHardware);

	return ret;
};

/******************************************************************************/
/*  barcode functions                                                         */
/******************************************************************************/
TSTATUS
TCHardwareIO::InitializeBarcode(BOOL reinitialize)
{
	if (reinitialize == FALSE)
	{
		m_pBarcode = new TCBarcode(this);
		return m_pBarcode->Initialize();
	}
	else
	{
		m_pBarcode->Close();
		return m_pBarcode->Initialize();
	}
}

/******************************************************************************/
/*  lighting dome functions                                                   */
/******************************************************************************/
TSTATUS
TCHardwareIO::InitializeLightDome(BOOL reinitialize)
{
	if (reinitialize == FALSE)
		m_pLightDome = new TCLights(this);

	m_pLightDome->Initialize();
	return TS_SUCCESS;
}

TSTATUS
TCHardwareIO::DomeRead(UINT address, UINT numberwords, USHORT *data)
{
	// if (software only system)
	if (m_Dome == NULL)
		return TS_SUCCESS;

	UINT domeAddress = address;

	EnterCriticalSection(&csHardware);

	DWORD returnValue =
		m_pPcio->Dome_ioRead((SHORT) domeAddress,  data, (SHORT) numberwords);

	LeaveCriticalSection(&csHardware);

	if (returnValue != PCIO_SUCCESS)
		return(TS_ERROR);
	else
		return(TS_SUCCESS);
};

TSTATUS
TCHardwareIO::DomeWrite(UINT address, UINT numberwords, USHORT *data)
{
	// if (software only system)
	if (m_Dome == NULL)
		return TS_SUCCESS;

	UINT domeAddress = address;

	EnterCriticalSection(&csHardware);

	DWORD returnValue =
		m_pPcio->Dome_ioWrite((SHORT) domeAddress,  data, (SHORT) numberwords);

	LeaveCriticalSection(&csHardware);

	if (returnValue != PCIO_SUCCESS)
		return(TS_ERROR);
	else
		return(TS_SUCCESS);
};

ONOFF
TCHardwareIO::ONOFFStringValue(CString &value)
{
	if (value == "P1")
		return cONP1;
	else if (value == "P2")
		return cONP2;
	else if (value == "On")
		return cON;
	else
		return cOFF;
}

/******************************************************************************/
/*  misc Set/Get Functions                                                    */
/******************************************************************************/
/*
 * Functions:	SetMisc
 *
 * Description:	set the switch to condition OnOff
 *
 * Parameters:
 *		TCIO_Bits	*iobits - misc system function pointer
 *		ONOFF		 OnOff  - contition for misc system function
 *
 * Return Values:
 *		TSTATUS
 *
 * Discussion:
 *				call the PCIO write function and set the Controller
 *				register for the misc system function to condition ONOFF.
 *
 */
TSTATUS
TCHardwareIO::SetMisc(TCIO_Bits *iobits, ONOFF OnOff)
{
	TSTATUS ret = TS_SUCCESS;
	EnterCriticalSection(&csHardware);
		if (iobits)
			m_pPcio->Controller_ioWrite(iobits, (SHORT) OnOff);
		else
			ret = TS_ERROR;
	LeaveCriticalSection(&csHardware);

	return ret;
}

/*
 * Functions:	GetMisc
 *
 * Description:	get the condition of the misc system function
 *
 * Parameters:
 *		TCIO_Bits	*iobits - misc system function pointer
 *		ONOFF		&OnOff  - contition
 *
 * Return Values:
 *		TSTATUS
 *
 * Discussion:
 *				call the PCIO read function and deposit the misc
 *				system function's condition into the ONOFF variable.  
 *
 */
TSTATUS
TCHardwareIO::GetMisc(TCIO_Bits *iobits, ONOFF &OnOff)
{
	TSTATUS ret = TS_SUCCESS;
	EnterCriticalSection(&csHardware);
		USHORT onoff = 0;
		if (iobits)
			m_pPcio->Controller_ioRead(iobits, &onoff);
		else
			ret = TS_ERROR;
		OnOff = onoff ? cON : cOFF;
	LeaveCriticalSection(&csHardware);

	return ret;
}

/******************************************************************************/
/*  Initialization functions                                                  */
/******************************************************************************/
#define MOTORS    "XY_Motor Controller"
#define CONVEYORS "Conveyor Controller"

void
TCHardwareIO::GetHardwareResources(HINSTANCE &hinst)
{
	// use harware resources for initialization message
	hinst = AfxGetResourceHandle();
	AfxSetResourceHandle(HardwareInstance);
};

void
TCHardwareIO::ReleaseHardwareResources(HINSTANCE &hinst)
{
	// restore old resources
	AfxSetResourceHandle(hinst);
};

void
TCHardwareIO::InitComFailureDialog(void)
{
	// this will quiet motors & conveyor during this mode change
	SetMode(SOFTWARE_ONLY);

	if (ComFailureDialog() == IDOK)
		GetSysReg()->sysv_HardwareStatus = HARDWARE_ENABLED;
	else
		GetSysReg()->sysv_HardwareStatus = SOFTWARE_ONLY;

	// Delay, incase the user hits the "Reinitialize" button
	// immediately after restoring system power.  The motors
	// and conveyors need a little time to reset on power-up.
	Sleep(2000);
	// reinitialize event
	SendEvent(AOI_UR_HWSTATUS, _T("TCMechIO: Hardware ReInitialization"));
};

int TCHardwareIO::AutoWidthBoardDetectDialog( BOOL isInit )
{
	HINSTANCE hinst;
	// use harware resources for initialization message
	GetHardwareResources(hinst);
	TCDlgAWPanelDetect dlg(isInit);

	int ret = dlg.DoModal();

	ReleaseHardwareResources(hinst);
	return ret;
}

int TCHardwareIO::ComFailureDialog(void)
{
	HINSTANCE hinst;
	// use harware resources for initialization message
	GetHardwareResources(hinst);
	TCDlgComFailure dlg;

	int ret = dlg.DoModal();

	ReleaseHardwareResources(hinst);
	return ret;
}

BOOL TCHardwareIO::LowAirWarningDialog(CString message, enum CYCLE_STOP_DIALOG_TYPE type)
{
	// use harware resources for initialization message
	GetHardwareResources(m_hOldInsCycleStop);

	BOOL ret = TRUE;

	if (message.GetLength() > 0)
	{
		if (m_pLowAirWarningDlg == NULL)
		{
			m_pLowAirWarningDlg = new TCDlgCycleStop;
			//20081015, morgan hsiao,
			//add a parameter for display type selection.
			m_pLowAirWarningDlg->SetMessage(message, type);
			ret = m_pLowAirWarningDlg->Create(TCDlgCycleStop::IDD);
		}
	}
	else
	{

		m_pLowAirWarningDlg = NULL;
	}

	// restore old resources
	ReleaseHardwareResources(m_hOldInsCycleStop);

	return ret;
}

BOOL TCHardwareIO::KillLowAirWarningDialog(void)
{
	BOOL ret = TRUE;

	if (m_pLowAirWarningDlg != NULL)
	{
		ret = m_pLowAirWarningDlg->DestroyWindow();
		delete m_pLowAirWarningDlg;
	}

	m_pLowAirWarningDlg = NULL;

	return ret;
}

BOOL TCHardwareIO::CycleStopDialog(CString message, enum CYCLE_STOP_DIALOG_TYPE type)
{
	// use harware resources for initialization message
	GetHardwareResources(m_hOldInsCycleStop);

	BOOL ret = TRUE;

	if (message.GetLength() > 0)
	{
		if (m_pCycleStopDlg == NULL)
		{
			m_pCycleStopDlg = new TCDlgCycleStop;
			//20081015, morgan hsiao,
			//add a parameter for display type selection.
			m_pCycleStopDlg->SetMessage(message, type);
			ret = m_pCycleStopDlg->Create(TCDlgCycleStop::IDD);
		}
	}
	else
	{

		m_pCycleStopDlg = NULL;
	}

	// restore old resources
	ReleaseHardwareResources(m_hOldInsCycleStop);

	return ret;
}

BOOL TCHardwareIO::KillCycleStopDialog(void)
{
	BOOL ret = TRUE;

	if (m_pCycleStopDlg != NULL)
	{
		ret = m_pCycleStopDlg->DestroyWindow();
		delete m_pCycleStopDlg;
	}

	m_pCycleStopDlg = NULL;

	return ret;
}

BOOL TCHardwareIO::PrepareEjectProtectionDialog(CString message, enum CYCLE_STOP_DIALOG_TYPE type)
{
	// use harware resources for initialization message
	GetHardwareResources(m_hOldInsCycleStop);

	BOOL ret = TRUE;

	if (message.GetLength() > 0)
	{
		if (m_pPrepareEjectProtectionDlg == NULL)
		{
			m_pPrepareEjectProtectionDlg = new TCDlgCycleStop;
			//20081015, morgan hsiao,
			//add a parameter for display type selection.
			m_pPrepareEjectProtectionDlg->SetMessage(message, type);
			ret = m_pPrepareEjectProtectionDlg->Create(TCDlgCycleStop::IDD);
		}
	}
	else
	{

		m_pPrepareEjectProtectionDlg = NULL;
	}

	// restore old resources
	ReleaseHardwareResources(m_hOldInsCycleStop);

	return ret;
}

BOOL TCHardwareIO::KillPrepareEjectProtectionDialog(void)
{
	BOOL ret = TRUE;

	if (m_pPrepareEjectProtectionDlg != NULL)
	{
		ret = m_pPrepareEjectProtectionDlg->DestroyWindow();
		delete m_pPrepareEjectProtectionDlg;
	}

	m_pPrepareEjectProtectionDlg = NULL;

	return ret;
}


BOOL
TCHardwareIO::InitializeDialog(int mode)
{
	// starting hardware initialization
	m_AmInitializing = TRUE;

	// use harware resources for initialization message
	GetHardwareResources(m_hOldIns);

	BOOL ret = TRUE;
	if (m_pHardwareInitDlg == NULL)
	{
		m_pHardwareInitDlg = new TCDlgHardwareInit;
		ret = m_pHardwareInitDlg->Create(TCDlgHardwareInit::IDD);
		if (mode == HW_SHUTDOWN)
			m_pHardwareInitDlg->SetShutDown();
	}
	return ret;
}

BOOL
TCHardwareIO::KillInitializeDialog(void)
{
	BOOL ret = TRUE;

	if (m_pHardwareInitDlg != NULL)
	{
		// check if a initialization failure occurred
		if (m_pHardwareInitDlg->GetFailure() == TRUE)
			Sleep(4000);

		ret = m_pHardwareInitDlg->DestroyWindow();
		delete m_pHardwareInitDlg;

		// restore old resources
		ReleaseHardwareResources(m_hOldIns);

		// leaving hardware initialization
		m_AmInitializing = FALSE;

	}
	m_pHardwareInitDlg = NULL;

	return ret;
}

void
TCHardwareIO::InitializeDialogUpdate(int item, int status, LPCTSTR msg)
{
	m_pHardwareInitDlg->UpdateMessage(item, status, msg);
	if(msg == _T(""))
		m_pHardwareInitDlg->StepProgress();
}

/******************************************************************************/
/*  HunterKiller functions                                                  */
/******************************************************************************/
/*
 * Function:	StartHunterKillerThread
 *
 * Description:	seek and destroy various dialogs boxes
 *
 * Parameters:
 *		none
 *
 * Return Values:
 *		int		error code
 *
 * Discussion:
 *				creates a thread to find all dialog boxes during
 *				a hardware power failure/re-initialization occurrence
 *
 *				it seeks all dialogs by their caption, if found, this
 *				function calls 'enddialog' for each.
 *
 *				thread ends after re-initialization.
 *
 */
void
TCHardwareIO::StartHunterKillerThread(void)
{
	// initialize
	m_ThreadDie    = FALSE;
	m_KillerThread = NULL;

	DWORD id;
	// start the thread for interrupts
	m_KillerThread = CreateThread(NULL, 0, &HunterKillerThread, this, NULL, &id);
}

void
TCHardwareIO::EndHunterKillerThread(void)
{
	m_ThreadDie    = TRUE;
}

/*
 * Function:	HunterKillerThread
 *
 * Description:	worker thread
 *
 * Parameters:
 *		void *	ClassPtr
 *
 * Return Values:
 *		int		error code
 *
 * Discussion:
 *				seek and destroy
 *
 */
DWORD WINAPI
TCHardwareIO::HunterKillerThread(void *arg)
{ // entry for thread
	HWND hDlg;
	int i = 1;
	TCHardwareIO *pMe = (TCHardwareIO *) arg;

	while (pMe->m_ThreadDie == FALSE)
	{	// motors
		hDlg =  FindWindow(NULL, "Aoi Exception Report");
		if (hDlg != NULL)
			EndDialog(hDlg, i);
		// autowidth
		hDlg =  FindWindow(NULL, "Conveyor AutoWidth");
		if (hDlg != NULL)
			EndDialog(hDlg, i);
		// user abort
		/*	// F404 vc6ぉㄴ쨢쬟간
		hDlg =  FindWindow(NULL, "Optima 7300");
		if (hDlg != NULL)
			EndDialog(hDlg, i);*/
		Sleep(50); // be courteous
	}

	CloseHandle(pMe->m_KillerThread);
	pMe->m_KillerThread = NULL;
	return NULL;
}

