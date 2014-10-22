/*
 * Module:		$RCSfile: pcio.hpp $ - DLL header file
 *				$Revision: 1.25 $
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
 * Discussion:
 *
 * Date:		October. 19, 1999
 *
 */

#if !defined(_TCPCIO_HPP__INCLUDED_)
#define _TCPCIO_HPP__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "AOI_IO.h"
#include "evmgrapi.h"
#include "tstatus.h"
#include "hardware_enum.h"
#include "controllerAddresses.hpp"

#define  Checkers_unneeded_timerFunctions 

class TCHardwareIO;

#ifdef HARDWAREIO_EXPORTS
#   define HARDWAREIO_CLASS __declspec(dllexport)
#else
#   define HARDWAREIO_CLASS __declspec(dllimport)
#endif

/*******************************************************************************/
/* I/O definitions                                                             */
/*******************************************************************************/
enum
{
	IO_UNUSED_1, IO_UNUSED_2,

	IO_TOWER_RED, IO_TOWER_YEL, IO_TOWER_GREEN, IO_TOWER_BLUE, IO_TOWER_WHITE,
	IO_TOWER_FLASH, IO_ALARM1, IO_ALARM2,
	
	IO_AUXFAN0, IO_AUXFAN1, IO_AUXFAN2, IO_AUXFAN3, 
	IO_AUXFAN4, IO_AUXFAN5, IO_AUXFAN6, IO_AUXFAN7,
	
	IO_LOADBUTTON, IO_INSPECTBUTTON, IO_EJECTBUTTON, IO_CANCELBUTTON,
	IO_OPT1BUTTON, IO_OPT2BUTTON, IO_OPT3BUTTON,
	IO_AUX1BUTTON, IO_AUX2BUTTON, IO_AUX3BUTTON,

	IO_LOADLITE, IO_INSPECTLITE, IO_EJECTLITE, IO_CANCELLITE,
	IO_OPT1LITE, IO_OPT2LITE, IO_OPT3LITE,
	IO_AUX1LITE, IO_AUX2LITE, IO_AUX3LITE,

	IO_LEDDRV, IO_TOWERDRV, IO_V12DRV, IO_V24DRV,

	IO_INT_ALL,
	
	IO_INTERLOCK, IO_DIAGNOSTIC, IO_CONVYR_BYPASS,
	IO_AIRINSTALLED, IO_LOWAIR,  IO_LASER, IO_LASER_POLARITY, IO_MOTORSTAGING_INSTALLED,

};

/*******************************************************************************/
/* TCIO_Bit                                                                    */
/*******************************************************************************/
class TCIO_Bits
{
public:
	TCIO_Bits () {};
	~TCIO_Bits() {};

	// C51140
	void Initialize (int address, int interrupt = pcioNA, int direction = pcioNA);
	void Clear(void)                     {m_address = IOBITS_INVALID;};
	void SetAddress(int address)         {m_address = address;       };
	int  GetAddress(void)                {return m_address;          };
	void SetBit(int bit)                 {m_bitposition = bit;       };
	int  GetBit(void)                    {return m_bitposition;      };
	void SetDirection(int direction)     {m_IODirection = direction; };
	int  GetDirection(void)              {return m_IODirection;      };
	void SetInterruptEnable(int enable)  {m_interruptEnable = enable;};
	int  GetInterruptEnable(void)        {return m_interruptEnable;  };

private:
	int m_address;
	int m_bitposition;
	int m_IODirection; // input/output
	int m_interruptEnable;
};

/*******************************************************************************/
/* TCPcio                                                                      */
/*******************************************************************************/
#define SC_COMFAILURE  "SCComFailure"


#ifdef Checkers_unneeded_timerFunctions
//TODO : all this timer stuff can be removed when checkers is modified to not call any timer stuff
#define LOWTIMERBASE       0x0104
#define HIGHTIMERBASE      0x0107
#define LOWTIMER1          LOWTIMERBASE
#define HIGHTIMER1         HIGHTIMERBASE
#define LOWTIMER2          LOWTIMERBASE  + 1
#define HIGHTIMER2         HIGHTIMERBASE + 1
#define LOWTIMER3          LOWTIMERBASE  + 2
#define HIGHTIMER3         HIGHTIMERBASE + 2
#define TIMER_BASE_CTRL    0x010A
#define TIMER1_CTRL        TIMER_BASE_CTRL
#define TIMER2_CTRL        TIMER_BASE_CTRL + 1
#define TIMER3_CTRL        TIMER_BASE_CTRL + 2

#define RUN_TIMER         0x002
#define STOP_TIMER        0x001

typedef union _AOITIME
{
	ULONG time_us;
	struct
	{ 
		USHORT low;
		USHORT high;
	} time;
} AOITIME;
// TODO: ^^^^^^^^^^^
#endif


// must match the revision embedded in the PCIO board 
#define PCIO_REVISION        0xb100
#define PCIO_REVISION_MASK   0xff00
#define CONTROLLER_BOOT_TIME 500	// TODO: Might be tweaked downward 

typedef enum
{	// driver DLL return codes
	PCIO_TIMER_ERROR = -1,
	PCIO_SUCCESS     =  0,
	PCIO_INIT_ERROR,
	PCIO_INTERRUPT_ERROR,
	PCIO_OPEN_ERROR,
	PCIO_CLOSE_ERROR,
	PCIO_READ_ERROR,
	PCIO_WRITE_ERROR,
	PCIO_DOME_ADDR_ERROR,
} tPCIO_RET;

enum
{
	PCIO_THREADS,
	PCIO_ALL_THREADS,
};

typedef void * IO_CHANNEL ;


enum {OFF = (SHORT) 0x00, ONP1 = 0x01, ONP2 = 0x02, ON = (SHORT) 0x03};

// callback prototype
typedef void (CALLBACK * PCIO_HANDLER_PTR)(void *, int);

class HARDWAREIO_CLASS TCPcio: public AOI_IO	
{
public:
	TCPcio(void);
   ~TCPcio(void) {DeleteCriticalSection(&csPcio);};
	void    InitializeTCPcio(TCHardwareIO *hardware);
	int     StartInterruptThread(void);
	void    KillAllThreads(int thread = PCIO_THREADS);		

	/******************************************************************************/
	/*  TCPcio Event functions                                                    */
	/******************************************************************************/
	void    SendEvent(AOI_EVENT event, LPCSTR Sender = "TCMechIO::SendEvent()",
								int iRet = 0, LPCSTR Command = _T(""));
	void    SendCancelEvent(void);
	void    SendAlert(AOI_EVENT event);

   // returns enum (PCIO_SUCCESS, etc)
// TODO : tPCIO_RET instead of DWORD
	tPCIO_RET PCIO_Open (SYSTEMTYPE type);
	tPCIO_RET PCIO_Close(void);
	DWORD   PCIO_ERead (SHORT addr, USHORT *data);		
	DWORD   PCIO_EWrite(SHORT addr, USHORT *data);
	DWORD   PCIO_EWriteEnable(void);
	void    PCIO_SetDigital(void);
	void    PCIO_SetAnalog (void);

	USHORT  GetPcioRevision(void);
	void    GetPcioDriverRevision(CString &revision){revision = m_DriverRevision;};

	static  DWORD WINAPI InterruptThread(void *arg);	// This is THE interrupt thread function -- hence static
	static  DWORD WINAPI DigitalCallbackThread(void *arg); // 
	static  DWORD WINAPI LtWatchdogThread(void *arg);

	void    EnableLtWatchdog(int timer);
	void    ResetLtWatchdog(int timer) {m_TowerWatchDog = timer * 4;}; // timer loop == 250ms

	void    InitializeLtTower(ONOFF red, ONOFF yellow, ONOFF green);
#ifdef Checkers_unneeded_timerFunctions
	// The following 5 functions are only used in checkers and provide no function to the operation of the machine
	// TODO: To be deleted 
	DWORD ReadTimer(UINT timer);
	DWORD   LoadTimer  (UINT timer, DWORD  time);
	DWORD   StartTimer (UINT timer, DWORD  time = 2000000);
	DWORD   StopTimer  (UINT timer);
	DWORD   AoiSleep   (UINT uSec);
	// end of to be deleted
#endif
	int     RegisterAoiInterruptCallback(int interrupt, void * ClassPtr, PCIO_HANDLER_PTR func);
	int     UnRegisterAoiInterruptCallback(int interrupt);
	
	BOOL    GetInterruptThreadStatus(void)        {return m_InterruptThreadStatus; };

	// returns the pointer to the IO(opto22) structure
	TCIO_Bits  *getIO_Ptrs(void)                  {return &m_IoBits[0];            };
	TCIO_Bits  *getIO_Ptrs(int index)             {return &m_IoBits[index];        };

	// read the SC firmware & EEPROM
	
	BOOL        CanFlash()                        {return m_CanFlash;              };
	CONTROLTYPE GetControllerType(void)           {return m_ControllerType;        };
	// firmware revision
	SHORT       GetControllerFirmwareMajor(void)  {return m_ControllerFirmMajor;   };
	SHORT       GetControllerFirmwareMinor(void)  {return m_ControllerFirmMinor;   };
	void        SetControllerFirmware(void)
					{m_ControllerFirmware.Format("%c.%c",
						'@' + m_ControllerFirmMajor, '0' + m_ControllerFirmMinor); };
	CString     GetControllerFirmware(void)       {return m_ControllerFirmware;    };
	// PCA revision
	void        SetControllerPCA(void)
					{m_SC_PCA_Number.Format("%c.0", '@' + m_ControllerRevisionPCA);};
	CString     GetControllerPCA(void)            {return m_SC_PCA_Number;         };
	// PCB revision
	void        SetControllerPCB(void)
					{m_SC_PCB_Number.Format("%c.0", '@' + m_ControllerRevisionPCB);};
	CString     GetControllerPCB(void)            {return m_SC_PCB_Number;         };
	// assembly number
	void        SetSysControllerBoardNumber(void)
					{m_SC_BoardNumber.Format("%03x-%03x-%02x",
						(m_ControllerBoardNumber >> 20) & 0x0fff,
						(m_ControllerBoardNumber >>  8) & 0x0fff,
						(m_ControllerBoardNumber	  ) & 0x00ff);};
	CString     GetSysControllerBoardNumber(void) {return m_SC_BoardNumber;        };
	DWORD       GetControllerBoardNumber(void)    {return m_ControllerBoardNumber; };
	// assembly serial number
	void        SetSysControllerSerialNumber(void)
					{m_SC_SerialNumber.Format("%08x", m_ControllerSerialNumber);   };
	CString     GetSysControllerSerialNumber(void){return m_SC_SerialNumber;       };
	DWORD       GetControllerSerialNumber(void)   {return m_ControllerSerialNumber;};

	DWORD       Controller_Init      (HARDWAREMODE mode);
	DWORD       Controller_ioRead             (TCIO_Bits *iobits, USHORT *data);
	DWORD       Controller_ioWrite            (TCIO_Bits *iobits, USHORT  data);
	DWORD       DisableController_ioInterrupt (TCIO_Bits *iobits);
	DWORD       EnableController_ioInterrupt  (TCIO_Bits *iobits);
	DWORD       ClearControllerInterruptStatus(TCIO_Bits *iobits);
	DWORD       TriggerController_ioInterrupt (TCIO_Bits *iobits);
	DWORD       SetInterruptPolarity          (TCIO_Bits *iobits, int polarity);

	// returns the pointer to the Dome structure
	DOME       *getDomePtr  (void);
	DWORD       Dome_ioRead (SHORT address, USHORT *data, SHORT count = 1);
	DWORD       Dome_ioWrite(SHORT address, USHORT *data, SHORT count = 1);
	DWORD       Dome_ioWrite(SHORT address, USHORT  data);
	int         ReadDomeXPosition(void);
	int         ReadDomeYPosition(void);
	void        WriteDomeXPosition(int &position);
	void        WriteDomeYPosition(int &position);

	// The following 5 functions deal with the system watchdog timer on the WC140 controller 
	void    StartSysWatchdog(void);  // activates the watchdog -- If it times out without reloading it will reset the WC140
	void    ReloadSysWatchdog(void); // Feeds the watchdog so that it doesn't time out --
	void    DisableSysWatchdog(void );// Stops the WD and disables the automatic reboot of WC140
	void    PauseSysWatchdog(void);	 	
	void    ContinueSysWatchdog(void);

private:
	tPCIO_RET	PCIO_Reset(void);
	USHORT      GetControllerVersion(void);
	DWORD       InitBits(SYSTEMTYPE type);
	bool        GetDriverVersion          (CString &msg);
	DWORD       SetController_ioInterrupt (TCIO_Bits *iobits, USHORT *data);
	DWORD       GetController_ioInterrupt (TCIO_Bits *iobits, USHORT *data);
	void        ClearAllInterrupt_140(void);

	void        SetLightTowerDriver(int onoff)
						{PCIO_Write(LT_DRIVERS,          (USHORT) onoff);};
	int         GetLightTowerDriver(void);
	void        SetLightFlashDuration(int msec)
						{PCIO_Write(LT_FLASH_DURATION,   (USHORT) msec); };
	int         GetLightFlashDuration(void);
	void        SetLightTowerGreen(int onoff)
						{PCIO_Write(LT_GREEN,            (USHORT) onoff);};
	void        SetLightTowerRed(int onoff)
						{PCIO_Write(LT_RED,              (USHORT) onoff);};
	void        SetLightTowerYellow(int onoff)
						{PCIO_Write(LT_YELLOW,           (USHORT) onoff);};
	void        SetFans(int fan, int onoff)
						{PCIO_Write(AUXFAN0 + (4 * fan), (USHORT) onoff);};
	int         GetFans(int fan);
	void        SetPanelDrv(int onoff)
						{PCIO_Write(PANEL_LED_DRV,       (USHORT) onoff);};
	int         GetPanelDrv(void);
	void        SetDomePS(int onoff, double voltage = 11.0);

	// group interrupt status
	int         GetPanelInterrupt(void);
	int         GetOpto1Interrupt(void);

	// hardware pointer
	TCHardwareIO     *m_pHardware;

	// callback storage
	PCIO_HANDLER_PTR  m_InterruptCallback[32];
	void             *m_ClassPtr[32]; // 

	// critical section object
	CRITICAL_SECTION  csPcio;

	CString     m_DriverRevision;
	SYSTEMTYPE  m_SystemType;
	CONTROLTYPE m_ControllerType;
	USHORT      m_ControllerFirmMajor;
	USHORT      m_ControllerFirmMinor;
	CString     m_ControllerFirmware;
	BOOL        m_CanFlash;

	// eeprom info
	DWORD       m_ControllerBoardNumber;
	DWORD       m_ControllerSerialNumber;
	DWORD       m_ControllerRevisionPCA;
	DWORD       m_ControllerRevisionPCB;

	CString     m_SC_BoardNumber;
	CString     m_SC_SerialNumber;
	CString     m_SC_PCA_Number;
	CString     m_SC_PCB_Number;
	
	TCIO_Bits   m_IoBits[MAX_IO];
	int         m_NumberIO;

//	int         m_SC_Status;
	int         m_TowerWatchDog;
	int         m_LtTimeoutEnabled; 

	HANDLE      m_CallbackThread;
	HANDLE      m_LtWatchdogThread;
	TCEventData m_CancelEvent;
	BOOL        m_KillThread;	// This is a variable that DigitalCallbackThread and WatchdogThread poll to see if they should exit -- Poor solution --  Needs Even and WaitForSingleObject() solution

//TODO:		
	// m_interrupt is a copy of the 'interrupt.status' of the WC140 as obtained from the device upon the completion of a GetNextInterrupt request.  
	// it is used in DigitalCallbackThread()
	// This doesn't seem to be save since an other interrupt could occur while the thread handling a prior interrupt is still running. 
	// it appears that a queue is necessary 
	int         m_Interrupt;
};

#endif // !defined(_TCPCIO_HPP__INCLUDED_)

