/*
 * Module:		$RCSfile: hardware.hpp $
 *				$Revision: 1.67.1.3.1.2.1.6.1.1 $
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


#if !defined(_HARDWARE_IO_HPP_)
#define _HARDWARE_IO_HPP_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef HARDWAREIO_EXPORTS
#   define HARDWAREIO_CLASS __declspec(dllexport)
#else
#   define HARDWAREIO_CLASS __declspec(dllimport)
#endif


// interrupt callback prototype
typedef void (CALLBACK * PCIO_HANDLER_PTR)(void *, int);

// classes that require interrupts
class TCIO_Bits;
class TCPcio;
class TCConsole;
class TCSerialPort;
class TCBarcode;
class TCHardwareIO;
class TCDlgHardwareInit;
class TCDlgCycleStop;

class TCSmema;
class TCLifters;
class TCLtTower;
class TCLaser;
class TCBeeper;
class TCConsole;
class TCConveyors;
class TCCnvyr;
class TCCnvyrUMSCO;
class TCBrdCtrl;
class TCLights;
// Motor
class TCLMotor;
class MCMotor;

#include "tstatus.h"
#include "evmevents.h"
#include "evmgrapi.h"
#include "TCException.hpp"
#include "hardware_enum.h"

class HARDWAREIO_CLASS TCHardwareIO
{

public:
	TCHardwareIO();
   ~TCHardwareIO();


	// event handler
	void         HardwareIOEventHandler(AOI_EVENT event, TCEventData *EventData);

	// hardware initializaton
	void         Close();
	TSTATUS      Init(HARDWAREMODE mode = HARDWARE_ENABLED);
	TSTATUS      ShutDown(void);

	TSTATUS      ReInitialize(HARDWAREMODE mode = HARDWARE_ENABLED);
	void         InitPointers(void);
	TSTATUS      InitHwModules(HARDWAREMODE mode = HARDWARE_ENABLED,
						BOOL reinitialize = FALSE);
	void         GetHardwareResources(HINSTANCE &hinst);
	void         ReleaseHardwareResources(HINSTANCE &hinst);
	void         InitComFailureDialog(void);
	int          AutoWidthBoardDetectDialog(BOOL isInit = FALSE);
	int          ComFailureDialog();
	BOOL	     LowAirWarningDialog(CString message, enum CYCLE_STOP_DIALOG_TYPE type);
	BOOL		 KillLowAirWarningDialog(void);
	BOOL         CycleStopDialog(CString message, enum CYCLE_STOP_DIALOG_TYPE type);
	BOOL         KillCycleStopDialog(void);
	BOOL		 PrepareEjectProtectionDialog(CString message, enum CYCLE_STOP_DIALOG_TYPE type);
	BOOL         KillPrepareEjectProtectionDialog(void);

	BOOL         InitializeDialog(int mode);
	BOOL         KillInitializeDialog();
	void         InitializeDialogUpdate(int item, int status, LPCTSTR msg = _T(""));
	BOOL         GetInitializationFlag(void) {return m_AmInitializing;     };

	TCPcio      *GetPcio();
	int          GetPcioRevision(void);
	CString      GetPcioDriverRevision(void);
	
	void         GetSysControllerFirmwareRevision(CString &revision)
						{revision = m_ControllerFirmwareRevision;          };
	int          GetSysControllerFirmwareRevision(void)
						{return m_ControllerFirmRevision;                  };
	CString      GetConverorFirmwareRevision(void);
	BOOL         CanFlash();

	void         ReEnableSysWatchdog(void);
	void         PauseSysWatchdog(void);
	void         ResetSysWatchdog(void);

	void         SetSystemType(SYSTEMTYPE type);
	SYSTEMTYPE   GetSystemType(void);
	CString		 SystemTypeToString( SYSTEMTYPE type );
	void         SetMode(HARDWAREMODE mode);
	HARDWAREMODE GetMode(void);
	void         ForceSoftwareOnlyMode(void);
	void         ForceHardwareFlashMode(void);
	void         ClearHardwareFlashMode(void);

	CONTROLTYPE  GetControllerType(void);
	DOMETYPE     GetDomeType(void);
	BOOL         GetAutoWidthEnabled(void);

	TCConveyors *GetConveyorPtr(void)           {return m_pConveyors;      };
	//it will 2 type, TCLMotor and MCMotor
	TCLMotor	*GetLMotorPtr(void)             {return m_pLMotors;        };
 	TCLights    *GetDomeLightPtr(void)          {return m_pLightDome;      };
 	TCBarcode   *GetBarcodePtr(void)            {return m_pBarcode;        };
 	TCLaser     *GetLaserPtr(void)              {return m_pLaser;          };
 	TCConsole   *GetConsolePtr(void)            {return m_pConsole;        };
 	TCSmema     *GetSmemaPtr(void)              {return m_pSmema;          };
 	TCLifters   *GetLiftersPtr(void)            {return m_pLifters;        };
 	TCBeeper    *GetBeeperPtr(void)             {return m_pBeeper;         };
 	TCLtTower   *GetLtTowerPtr(void)            {return m_pLtTower;        };
 	TCBrdCtrl   *GetBrdCtrlPtr(void)            {return m_pBrdCtrl;        };

	// timer functions
	TSTATUS StartTimer(UINT msec);
//	void    TReturn(IO_TIMER Timer, int EventID);

	// alerts status
	void    SetAlertStatus(int alert)  {m_Alert = alert;                   };
	int     GetAlertStatus(void)       {return m_Alert;                    };

	/******************************************************************************/
	/*  Hardware Event functions                                                  */
	/******************************************************************************/
	void    SendEvent(AOI_EVENT event, LPCSTR Sender = "TCMechIO::SendEvent()",
								int iRet = 0, LPCSTR Command = _T(""));

	/******************************************************************************/
	/*  XY Table Motors functions                                                 */
	/******************************************************************************/
	TSTATUS InitializeMotors(HARDWAREMODE mode = HARDWARE_ENABLED,
						BOOL reinitialize = FALSE);
	BOOL    MotorsInitialized(void) {return m_MotorsInitialized;        };
	int     SetMotorSpeed(double speed);
	void	SetXYMotorSpeed(int speed);
	int     GetXYMotorSpeed(void);
	void	SetMotorPercentSpeed( int percentSpeed ); // overwritten by scanner
	int     GetAxisPosition(MOTOR_AXIS axis);
	int     GetAxisPosition(int &X, int &Y);
	float   MoveXYAbsolute(int XPos, int YPos);
	float   MoveXYCoordinated(int XDir, int YDir, bool wait = TRUE);
	float   MoveXYSequence(int X1, int Y1, int X2, int Y2, int X3, int Y3, bool wait = TRUE);
	float   IsMoveComplete(MOTOR_AXIS axis = Both);
	int     Step(MOTOR_AXIS axis, STEPSIZE step, DIRECTION dir);
	void    HomeXY(MOTOR_AXIS axis = Both);
	int     EngageMotors(void);
	int     DisEngageMotors(void);
	int     SetXYTargetRadius(UINT TargetRadius);
	int		GetXUpperLimit();
	int		GetXLowerLimit();
	int		GetYUpperLimit();
	int		GetYLowerLimit();
	void    ParkXYMotors(void);

	void	SetParkPosition( int x, int y );
	MOTOR_STATUS ReadLimitSwitches(void);
	int		SetEstimateMode(bool emode);

	/******************************************************************************/
	/*  NT7300 Dome functions                                                     */
	/******************************************************************************/
	TSTATUS InitializeLightDome(BOOL reinitialize = FALSE);
	TSTATUS ResetDomePosition(MOTOR_AXIS axis = Both);
	TSTATUS SetDomeXPosition(int &position);
	TSTATUS GetDomeXPosition(int &position);
	TSTATUS SetDomeYPosition(int &position);
	TSTATUS GetDomeYPosition(int &position);

	/******************************************************************************/
	/*  NT7300 Barcode functions                                                  */
	/******************************************************************************/
	TSTATUS InitializeBarcode(BOOL reinitialize = FALSE);

	/******************************************************************************/
	/*  laser functions                                                           */
	/******************************************************************************/
	TSTATUS InitializeLaser(BOOL reinitialize = FALSE);

	/******************************************************************************/
	/*  user panel functions                                                      */
	/******************************************************************************/
	TSTATUS InitializeConsole(BOOL reinitialize = FALSE);

	/******************************************************************************/
	/*  NT7300 board control functions                                            */
	/******************************************************************************/
	TSTATUS InitializeBrdCtrl(BOOL reinitialize = FALSE);

	/******************************************************************************/
	/*  NT7300 SMEMA functions                                                    */
	/******************************************************************************/
	TSTATUS InitializeSMEMA(BOOL reinitialize = FALSE);
	TSTATUS SetOutBoardAvailable(SMEMA Avail);
	TSTATUS SetInBoardBusy(SMEMA Busy); 
	TSTATUS SetSmemaPass(ONOFF OnOff);
	BOOL    GetSmemaEnabled(void);
	void    SetSmemaEnabled(BOOL smema);

	/******************************************************************************/
	/*  NT7300 conveyor lifter functions                                          */
	/******************************************************************************/
	TSTATUS InitializeLifters(BOOL reinitialize = FALSE);
	TSTATUS SetLifters(ONOFF  OnOff);

	BOOL    GetLiftersEnabled(void);
	CLIFTER GetLiftersMode(void);
	void    SetLiftersMode(CLIFTER lifter);

	// TRUE == Engage (UP)
	void    EngageLifter(BOOL direction);

	/******************************************************************************/
	/*  NT7300 conveyor functions                                                 */
	/******************************************************************************/
	BOOL    ConveyorInitialized(void)  {return m_ConveyorInitialized;             };
	TSTATUS GetConveyorFunction(TCIO_Bits *iobits, ONOFF &OnOff);
	TSTATUS InitializeConveyor2(BOOL reinitialize = FALSE);
	TSTATUS InitializeConveyorUMSCO(BOOL reinitialize);
	TSTATUS	InitializeConveyorMC(BOOL reinitialize = FALSE);
	/* 
	 * Eject to Sensor
	 */
	int     ForceEjectBoard(void);
	void	ForceLoadBoard(void);

	TSTATUS ConveyorAlignBoard(void);
	/* determine if the AutoWidth Option is installed 
	 *		BOOL -	TRUE  = AutoWidth Option Installed
	 *				FALSE = No AutoWidth Option
	 */
	BOOL    IsAutoWidthInstalled(void);
	/* determine if the conveyor is in 'insert' mode 
	 *		BOOL	TRUE  - in 'insert' mode
	 *				FALSE - NOT in 'insert' mode
	 */
	BOOL    IsInsertMode(void);
	/* determine if the conveyor has an extension
	 *		BOOL	TRUE  - has an extension
	 *				FALSE - does NOT have an extension
	 */
	BOOL    IsExtensionInstalled(void);
	/* determine if the conveyor is in FastLoad&Eject mode
	 *		BOOL	TRUE  - is in FAST mode
	 *				FALSE - is NOT in FAST mode
	 */
	BOOL    IsConveyorInFastMode(void);
	/* determine if the conveyor is in Staging mode
	 *		BOOL	TRUE  - is in STAGING mode
	 *				FALSE - is NOT in STAGING mode
	 */
	BOOL    IsConveyorInStagingMode(void);

	BOOL    IsConveyorInHalfLoadMode(void);

	TSTATUS AutoWidthBoardDetect(BOOL isInit= FALSE);
	TSTATUS HomeWidthMotor(void);
	static  DWORD WINAPI HomeWidthMotorThread(void *arg);
	HANDLE  GetAWEvent() {return m_hAutoWidthEvent;};

	static  DWORD WINAPI InitMotorThread(void *arg);

	TSTATUS SetBoardWidth(int width);
	TSTATUS SetBoardWidthRelative(int width);
	int     GetBoardWidth(void);
	TSTATUS SetBoardWidthInc(int inch_cm, int increment);
	int     GetBoardWidthInc(void) {return m_jogInc;};
	TSTATUS IsAutoWidthDone(void);
	TSTATUS SetBoardWeight(int weight = 1);

	// current Conveyor ExternsionMode
	CONVEYOR_EXTENSION GetConveyorExtension(void);

	// NT7300 conveyor/sensors
	USHORT  GetConveyorStatus(void);

	void    ConveyorCancel(void);
	BOOL    IsBoardPresent(void);

	/******************************************************************************/
	/*  panel button (lights) functions                                           */
	/******************************************************************************/
	TSTATUS SetPanelLight(TCIO_Bits *iobits, ONOFF  OnOff);
	TSTATUS GetPanelLight(TCIO_Bits *iobits, ONOFF &OnOff);
	TSTATUS SetLoadLight(ONOFF  OnOff)   {return SetPanelLight(m_LoadLight,    OnOff);};
	TSTATUS GetLoadLight(ONOFF &OnOff)   {return GetPanelLight(m_LoadLight,    OnOff);};
	TSTATUS SetLoadLite(ONOFF  OnOff)    {return SetPanelLight(m_LoadLight,    OnOff);};
	TSTATUS GetLoadLite(ONOFF &OnOff)    {return GetPanelLight(m_LoadLight,    OnOff);};
	TSTATUS SetCancelLite(ONOFF  OnOff)  {return SetPanelLight(m_CancelLight,  OnOff);};
	TSTATUS GetCancelLite(ONOFF &OnOff)  {return GetPanelLight(m_CancelLight,  OnOff);};
	TSTATUS SetInspectLite(ONOFF  OnOff) {return SetPanelLight(m_InspectLight, OnOff);};
	TSTATUS GetInspectLite(ONOFF &OnOff) {return GetPanelLight(m_InspectLight, OnOff);};
	TSTATUS SetEjectLite(ONOFF  OnOff)   {return SetPanelLight(m_EjectLight,   OnOff);};
	TSTATUS GetEjectLite(ONOFF &OnOff)   {return GetPanelLight(m_EjectLight,   OnOff);};
	TSTATUS SetOpt1Lite(ONOFF  OnOff)    {return SetPanelLight(m_Opt1Light,    OnOff);};
	TSTATUS GetOpt1Lite(ONOFF &OnOff)    {return GetPanelLight(m_Opt1Light,    OnOff);};
	TSTATUS SetOpt2Lite(ONOFF  OnOff)    {return SetPanelLight(m_Opt2Light,    OnOff);};
	TSTATUS GetOpt2Lite(ONOFF &OnOff)    {return GetPanelLight(m_Opt2Light,    OnOff);};
	TSTATUS SetOpt3Lite(ONOFF  OnOff)    {return SetPanelLight(m_Opt3Light,    OnOff);};
	TSTATUS GetOpt3Lite(ONOFF &OnOff)    {return GetPanelLight(m_Opt3Light,    OnOff);};

	// user panel switches
	TSTATUS SetButton(TCIO_Bits *iobits, ONOFF  OnOff);
	TSTATUS GetButton(TCIO_Bits *iobits, ONOFF &OnOff);

	TSTATUS SetInspectButton(ONOFF  OnOff)  {return SetButton(m_InspectButton, OnOff);};
	TSTATUS GetInspectButton(ONOFF &OnOff)  {return GetButton(m_InspectButton, OnOff);};
	TSTATUS SetCancelButton(ONOFF  OnOff)   {return SetButton(m_CancelButton,  OnOff);};
	TSTATUS GetCancelButton(ONOFF &OnOff)   {return GetButton(m_CancelButton,  OnOff);};
	TSTATUS SetLoadButton(ONOFF  OnOff)     {return SetButton(m_LoadButton,    OnOff);};
	TSTATUS GetLoadButton(ONOFF &OnOff)     {return GetButton(m_LoadButton,    OnOff);};
	TSTATUS SetEjectButton(ONOFF  OnOff)    {return SetButton(m_EjectButton,   OnOff);};
	TSTATUS GetEjectButton(ONOFF &OnOff)    {return GetButton(m_EjectButton,   OnOff);};
	TSTATUS SetOpt1Button(ONOFF  OnOff)     {return SetButton(m_Opt1Button,    OnOff);};
	TSTATUS GetOpt1Button(ONOFF &OnOff)     {return GetButton(m_Opt1Button,    OnOff);};
	TSTATUS SetOpt2Button(ONOFF  OnOff)     {return SetButton(m_Opt2Button,    OnOff);};
	TSTATUS GetOpt2Button(ONOFF &OnOff)     {return GetButton(m_Opt2Button,    OnOff);};
	TSTATUS SetOpt3Button(ONOFF  OnOff)     {return SetButton(m_Opt3Button,    OnOff);};
	TSTATUS GetOpt3Button(ONOFF &OnOff)     {return GetButton(m_Opt3Button,    OnOff);};

	// user panel switch interrupts
	void    RegisterInterruptCallback  (int interrupt, void *classPtr, PCIO_HANDLER_PTR func);
	void    UnRegisterInterruptCallback(int interrupt);

	TSTATUS SetInterruptPolarity(TCIO_Bits *iobits, int polarity);
	TSTATUS ClearInterrupt(TCIO_Bits *iobits);
	TSTATUS EnableInterrupt(TCIO_Bits *iobits);

	TSTATUS ClearInspectButtonInterrupt(void)   {return ClearInterrupt(m_InspectButton);  };
	TSTATUS EnableInspectButtonInterrupt(void)  {return EnableInterrupt(m_InspectButton); };
	TSTATUS ClearCancelButtonInterrupt(void)    {return ClearInterrupt(m_CancelButton);   };
	TSTATUS EnableCancelButtonInterrupt(void)   {return EnableInterrupt(m_CancelButton);  };
	TSTATUS ClearLoadButtonInterrupt(void)      {return ClearInterrupt(m_LoadButton);     };
	TSTATUS EnableLoadButtonInterrupt(void)     {return EnableInterrupt(m_LoadButton);    };
	TSTATUS ClearEjectButtonInterrupt(void)     {return ClearInterrupt(m_EjectButton);    };
	TSTATUS EnableEjectButtonInterrupt(void)    {return EnableInterrupt(m_EjectButton);   };
	TSTATUS ClearOpt1ButtonInterrupt(void)      {return ClearInterrupt(m_Opt1Button);     };
	TSTATUS EnableOpt1ButtonInterrupt(void)     {return EnableInterrupt(m_Opt1Button);    };
	TSTATUS Opt1ButtonInterruptPolarity(int polarity)
								{return SetInterruptPolarity(m_Opt1Button, polarity);     };
	TSTATUS ClearOpt2ButtonInterrupt(void)      {return ClearInterrupt(m_Opt2Button);     };
	TSTATUS EnableOpt2ButtonInterrupt(void)     {return EnableInterrupt(m_Opt2Button);    };
	TSTATUS Opt2ButtonInterruptPolarity(int polarity)
								{return SetInterruptPolarity(m_Opt2Button, polarity);     };
	TSTATUS ClearOpt3ButtonInterrupt(void)      {return ClearInterrupt(m_Opt3Button);     };
	TSTATUS EnableOpt3ButtonInterrupt(void)     {return EnableInterrupt(m_Opt3Button);    };
	TSTATUS Opt3ButtonInterruptPolarity(int polarity)
								{return SetInterruptPolarity(m_Opt3Button, polarity);     };
	TSTATUS GetConveyorBypass(ONOFF &OnOff);
	TSTATUS ClearConveyorBypassInterrupt(void)  {return ClearInterrupt(m_ConveyorBypass); };
	TSTATUS EnableConveyorBypassInterrupt(void) {return EnableInterrupt(m_ConveyorBypass);};
	TSTATUS ConveyorBypassInterruptPolarity(int polarity)
								{return SetInterruptPolarity(m_ConveyorBypass, polarity); };
	TSTATUS ClearLowAirInterrupt(void)          {return ClearInterrupt(m_LowAir);         };
	TSTATUS EnableLowAirInterrupt(void)         {return EnableInterrupt(m_LowAir);        };
	TSTATUS LowAirInterruptPolarity(int polarity)
								{return SetInterruptPolarity(m_LowAir, polarity);         };

	// misc system functions
	TSTATUS SetMisc(TCIO_Bits *iobits, ONOFF  OnOff);
	TSTATUS GetMisc(TCIO_Bits *iobits, ONOFF &OnOff);
	/*
	 * the AirOption low air pressure warning 
	 */
	TSTATUS GetLowAir(ONOFF &OnOff);
	/*
	 * determine if the AirOption is installed 
	 */
	CONVEYORCLAMPTYPE GetClampInstalled(void);

	// system interrupts
	TSTATUS GetInterlock(ONOFF &OnOff);
	//TSTATUS SetInterlock(ONOFF  OnOff);
	TSTATUS ClearInterlockInterrupt(void)       {return ClearInterrupt(m_SystemInterlock); };
	TSTATUS EnableInterlockInterrupt(void)      {return EnableInterrupt(m_SystemInterlock);};
	TSTATUS InterlockInterruptPolarity(int polarity)
								{return SetInterruptPolarity(m_SystemInterlock, polarity); };
	TSTATUS ClearDiagnosticInterrupt(void)      {return ClearInterrupt(m_Diagnostic);};
	TSTATUS EnableDiagnosticInterrupt(void)     {return EnableInterrupt(m_Diagnostic);};
	TSTATUS TriggerDiagnosticInterrupt(void);


	/******************************************************************************/
	/*  systrem light tower functions                                             */
	/******************************************************************************/
	TSTATUS InitializeLtTower(BOOL reinitialize = FALSE);
	void    InitializeLtTowerLights(ONOFF red, ONOFF yellow, ONOFF green);
	TSTATUS SetTower(TCIO_Bits *iobits, ONOFF  OnOff);
	TSTATUS GetTower(TCIO_Bits *iobits, ONOFF &OnOff);
	TSTATUS SetLtYellow(ONOFF  OnOff)    {return SetTower(m_TowerYellow,   OnOff);};
	TSTATUS GetLtYellow(ONOFF &OnOff)    {return GetTower(m_TowerYellow,   OnOff);};
	TSTATUS SetLtRed(ONOFF  OnOff)       {return SetTower(m_TowerRed,      OnOff);};
	TSTATUS GetLtRed(ONOFF &OnOff)       {return GetTower(m_TowerRed,      OnOff);};
	TSTATUS SetLtGreen(ONOFF  OnOff)     {return SetTower(m_TowerGreen,    OnOff);};
	TSTATUS GetLtGreen(ONOFF &OnOff)     {return GetTower(m_TowerGreen,    OnOff);};
	TSTATUS SetLtBlue(ONOFF  OnOff)      {return SetTower(m_TowerBlue,     OnOff);};
	TSTATUS GetLtBlue(ONOFF &OnOff)      {return GetTower(m_TowerBlue,     OnOff);};
	TSTATUS SetLtWhite(ONOFF  OnOff)     {return SetTower(m_TowerWhite,    OnOff);};
	TSTATUS GetLtWhite(ONOFF &OnOff)     {return GetTower(m_TowerWhite,    OnOff);};
	void    EnableLtWatchdog(int timeout);
	void    ResetLtWatchdog(int timeout);

	/******************************************************************************/
	/*  system alarm functions                                                    */
	/******************************************************************************/
	TSTATUS InitializeBeepers(BOOL reinitialize = FALSE);
	TSTATUS SetBeeper(ONOFF  OnOff)      {return SetMisc(m_Beeper,  OnOff);};
	TSTATUS GetBeeper(ONOFF &OnOff)      {return GetMisc(m_Beeper,  OnOff);};
	TSTATUS SetBeeper2(ONOFF  OnOff)     {return SetMisc(m_Beeper2, OnOff);};// C51140 only
	TSTATUS GetBeeper2(ONOFF &OnOff)     {return GetMisc(m_Beeper2, OnOff);};// C51140 only

	// opto driver XY Motor trigger
	TSTATUS XYMoveTrigger(void);

	// dome functions
	TSTATUS DomeRead (UINT address, UINT numberwords, USHORT *data);
	TSTATUS DomeRead (UINT address, USHORT *data) {return (DomeRead(address, 1, data));};
	TSTATUS DomeWrite(UINT address, UINT numberwords, USHORT *data);
	TSTATUS DomeWrite(UINT address, USHORT  data) {return (DomeWrite(address, 1, &data));};
	TSTATUS SetLaser(ONOFF  OnOff)                {return SetMisc(m_Laser, (OnOff) ? cOFF : cON);};
	TSTATUS GetLaser(ONOFF &OnOff);
	TSTATUS SetLaserTrigger(LASERTRIGGER  trigger);
	TSTATUS GetLaserTrigger(LASERTRIGGER &trigger);

	ONOFF   ONOFFStringValue(CString &value);

private:

	SYSTEMTYPE    m_SystemType;  // none, NT7300,  NT7350, NT5500, NT7355
	HARDWAREMODE  m_Mode;

	// hardware dialogs
	TCDlgHardwareInit  *m_pHardwareInitDlg;
	TCDlgCycleStop	   *m_pCycleStopDlg, *m_pPrepareEjectProtectionDlg, *m_pLowAirWarningDlg;
	HINSTANCE          m_hOldIns;
	HINSTANCE          m_hOldInsCycleStop;

	// callback handles
	TCEventHandler     m_CBCancel;
	TCEventHandler     m_CBCycleStopOn;
	TCEventHandler     m_CBCycleStopOff;
	TCEventHandler     m_CBHardware;
	TCEventHandler     m_CBReInitialize;
	TCEventHandler     m_CBLowAir;
	TCEventHandler     m_CBLaserOn;
	TCEventHandler     m_CBPrepareEjectProtection;

	// hardware objects
	TCLMotor     *m_pLMotors;
	TCConveyors  *m_pConveyors;
	TCCnvyrUMSCO *m_pConveyorUMSCO;
	TCLights     *m_pLightDome;
	TCBarcode    *m_pBarcode;
	TCLaser      *m_pLaser;
	TCConsole    *m_pConsole;
	TCSmema      *m_pSmema;
	TCLifters    *m_pLifters;
	TCBeeper     *m_pBeeper;
	TCLtTower    *m_pLtTower;
	TCBrdCtrl    *m_pBrdCtrl;

	HARDWAREMODE RegistryCheck(void);
	void RegFactoryHardware(DWORD hardware);

	void    StartHunterKillerThread(void);
	void    EndHunterKillerThread(void);
	static  DWORD WINAPI HunterKillerThread(void *arg);
	HANDLE  m_KillerThread;
	BOOL    m_ThreadDie;

	HANDLE  m_hAutoWidthThread;
	HANDLE  m_hCancelEvent;
	HANDLE  m_hAutoWidthEvent;
	HANDLE  m_hInitmotorThread;

	// critical section object
	CRITICAL_SECTION csHardware;

	// PCIO object
	TCPcio     *m_pPcio;

	// PCIO ControlBit pointers        // OPTO22 bit position
	TCIO_Bits  *m_SystemInterlock;     // 05
	TCIO_Bits  *m_Diagnostic;          // 
	TCIO_Bits  *m_Laser;               // 09     
	TCIO_Bits  *m_Beeper;              // 10
	TCIO_Bits  *m_LoadLight;           // 12
	TCIO_Bits  *m_Conveyor;            // 14
	TCIO_Bits  *m_InspectButton;       // 17
	TCIO_Bits  *m_LoadButton;          // 18
	TCIO_Bits  *m_RightSensor;         // 21
	TCIO_Bits  *m_LeftSensor;          // 22
	TCIO_Bits  *m_Unused;              // 23
	TCIO_Bits  *m_CenterSensor;        // 24
	TCIO_Bits  *m_SmemaPass;           // 11
	TCIO_Bits  *m_TowerYellow;         // 13
	TCIO_Bits  *m_ConveyorBypass;      // 17

	// additional C51140 controls
	TCIO_Bits  *m_LaserPolarity;
	TCIO_Bits  *m_TowerRed;
	TCIO_Bits  *m_TowerGreen;
	TCIO_Bits  *m_TowerBlue;
	TCIO_Bits  *m_TowerWhite;
	TCIO_Bits  *m_LTFlashTime;
	TCIO_Bits  *m_Beeper2;
//	TCIO_Bits  *m_Tower24v;

	TCIO_Bits  *m_AuxFan0;
	TCIO_Bits  *m_AuxFan1;
	TCIO_Bits  *m_AuxFan2;
	TCIO_Bits  *m_AuxFan3;
	TCIO_Bits  *m_AuxFan4;
	TCIO_Bits  *m_AuxFan5;
	TCIO_Bits  *m_AuxFan6;
	TCIO_Bits  *m_AuxFan7;

	TCIO_Bits  *m_CancelButton;
	TCIO_Bits  *m_EjectButton;
	TCIO_Bits  *m_Opt1Button;
	TCIO_Bits  *m_Opt2Button;
	TCIO_Bits  *m_Opt3Button;
	TCIO_Bits  *m_Aux1Button;
	TCIO_Bits  *m_Aux2Button;
	TCIO_Bits  *m_Aux3Button;
	TCIO_Bits  *m_InspectLight;
	TCIO_Bits  *m_EjectLight;
	TCIO_Bits  *m_CancelLight;
	TCIO_Bits  *m_Opt1Light;
	TCIO_Bits  *m_Opt2Light;
	TCIO_Bits  *m_Opt3Light;
	TCIO_Bits  *m_Aux1Light;
	TCIO_Bits  *m_Aux2Light;
	TCIO_Bits  *m_Aux3Light;

	// driver interrupts
	TCIO_Bits  *m_LEDrv;
	TCIO_Bits  *m_TowerDrv;
	TCIO_Bits  *m_V12Drv;
	TCIO_Bits  *m_V24Drv;

	// master interrupt status
	TCIO_Bits  *m_InterruptStatus;

	// low air
	TCIO_Bits  *m_LowAir;
	TCIO_Bits  *m_AirInstalled;
	TCIO_Bits  *m_MotorStagingInstalled;

	BOOL        m_AmInitializing;

	// dome
	DOME       *m_Dome;                    // DomeLights pointer
	DOMETYPE    m_DomeType;                // Dome Type; Standard, High Clearance, etc.
	CONTROLTYPE m_ControllerType;          // System Controller type
	int         m_ControllerFirmRevision;  // System Controller Version
	CString     m_ControllerFirmwareRevision;
	int         m_PcioRevision;
	CString     m_PcioDriverRevision;

	// timer hardware pointers
//	IO_TIMER            *m_pTimer1;
//	IO_TIMER             m_Timer;

	// conveyor parameters
	DWORD         m_ConveyorSpeed;
	BOOL          m_ConveyorInitialized;
	BOOL          m_AutoWidthEnabled;
	BOOL          m_AutoWidthHomed;
	AW_HDIR       m_AutoWidthHomeDirection;
	int           m_jogInc;
	BOOL          m_ConveyorLock;
	DWORD         m_ConveyorLockThreadID;
	USHORT        m_ConveyorStatus;

	CONVEYOR_EXTENSION
                  m_ExtensionMode;

	// XT motor parameters
	int           m_MotorFOV;
	DWORD         m_MotorSpeed;
	BOOL          m_MotorsInitialized;

	int           m_Alert;

	// serial ports
	CString       m_MotorComPort;
	CString       m_ConveyorComPort;
	CString       m_BarcodeA_ComPort;
	CString       m_BarcodeB_ComPort;

	//only for motor
	int		m_reinitialize;
	TSTATUS m_motorreturnCode;
};

HARDWAREIO_CLASS void SetSuspendCheckMotorCancel(bool bSuspendCheckMotorCancel);
HARDWAREIO_CLASS bool GetSuspendCheckMotorCancel();

#endif // !defined(_HARDWARE_IO_HPP_)
