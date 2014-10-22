/*
 * Module:		$RCSfile: hardware_enum.h $
 *				$Revision: 1.1.1.1.1.6.1.2 $
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
 *	Following are the enum definitions required for all hardware controls:
 *
 *
 * Date:		August. 2, 1999
 *
 */

#if !defined(_HARDWARE_ENUM_H_)
#define _HARDWARE_ENUM_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// system modes
enum HARDWAREMODE
{
	SOFTWARE_ONLY,        // NO Hardware available
	HARDWARE_ENABLED,     // Normal
	HARDWARE_JOG_ENABLED, // Currently UNused
	DIAGNOSTICS,          // checkers
	HARDWARE_FLASH,       // Only the SC enabled (safemode)
};

// system types (NT7300 == look down systems, NT7350 == look up systems, NT5500 == BenchTop, NT7355 == Grand BenchTop)
enum SYSTEMTYPE  {NT7300 = 0, NT7350, NT5500, NT7355, NTUNKNOW};
enum CONTROLTYPE {CUNKNOWN = 0x0000, C51134 = 0x0112, C51140 = 0x0140};
enum CONVEYORCLAMPTYPE {MOTORTYPE=0, AIRTYPE, MOTORSTAGETYPE, NONETYPE};
// There is another enum for dome type in wintypes.h - they must match
enum DOMETYPE    {STANDARD, HIGHCLEARANCE1, LOWANGLE};
//enum CONVEYOR   {ONE, TWO};

#define MAXBOARDSTAGELENGTH 12.0
#define MINBOARDSTAGELENGTH  3.0

//
// CE_NONE - no conveyor extension installed
// CE_OFF  - conveyor extension present but insert mode is off
// CE_PASSTHRU - insert mode active - pass boards through machine
// CE_INSPECT - insert mode active - inspect boards
enum CONVEYOR_EXTENSION { CE_NONE, CE_OFF, CE_PASSTHRU, CE_INSPECT };
enum PRELOAD            { PL_INSERT, PL_SMEMA };


enum ONOFF       {cOFF, cONP1, cONP2, cON};
enum ALARMS      {aOFF, aALARM1, aALARM2};
enum EJECT       {LF2LF, RT2RT, RT2LF, LF2RT, FAST_L2R, FAST_R2L, LT2RT_FL,};
enum DIR         {GO_RIGHT, GO_LEFT};
//enum SENSOR      {PRELOAD, BOARDINPLACE, POSTLOAD};
enum T_LIGHT     {GREEN, RED, YELLOW, BLUE, WHITE};
enum C_LIGHT     {PASSLIGHT, LOADLIGHT, FAILLIGHT};
enum NC_LIGHT    {LOADLITE, INSPECTLITE, EJECTLITE, CANCELLITE, OPT1LITE, OPT2LITE, OPT3LITE,};
enum BUTTON      {B_INSPECT, B_LOAD, B_REJECT, B_ACCEPT, B_BYPASS, B_EJECT, B_CANCEL,
					B_OPT1, B_OPT2, B_OPT3, B_AUX1, B_AUX2, B_AUX3,};
//enum SMEMA_PORT {UPSTREAM, DOWNSTREAM};
enum SMEMA       {NO_BOARD_AVAILABLE, BOARD_AVAILABLE, BUSY, NOTBUSY};
//20070713, morgan hsiao, add new definition about DPPM response for ALERTS enum variation.
enum ALERTS      
{
	ALERT = 1, 
	ALERT_EQUIP, 
	ALERT_MAJOR, 
	ALERT_MINOR, 
	ALERT_IDLE, 
	ALERT_NORMAL, 
	ALERT_BYPASS, 
	ALERT_OFF, 
	ALERT_DPPML1, 
	ALERT_DPPML2, 
	ALERT_DPPML3
};

// laser trigger polarity
enum LASERTRIGGER {FALLINGEDGE, RISINGEDGE, };

enum             {HW_PASSED = 0L, HW_FAILED = 1L, HW_UNTESTED = 2L, };

// PCI/O interrupt enums
enum             {pcioFALLING, pcioRISING,}; // interrupt trigger edge
enum             {pcioDIS, pcioEN,};         // interrupt enable
enum             {pcioIN, pcioOUT, pcioNA,}; // port direction

enum
{
	INIT_SYSCONTROLLER,
	INIT_MOTORCONTROLLER,
	INIT_BARCODE,
	INIT_CONVEYORCONTROLLER,
	INIT_DOME,
	INIT_LASER,
};

enum
{
	IDLG_SOFTWAREONLY,
	IDLG_INITIALIZING,
	IDLG_INITCOMPLETE,
	IDLG_ERROR,
	IDLG_SHUTDOWN,
	IDLG_REPORT,
};

enum
{
	HW_INITIALIZIING,
	HW_SHUTDOWN,
};

enum
{// digital interrupts
	SYSINTERLOCK,
	INSPECTBUTTON,
	LOADBUTTON,
	REJECTBUTTON,
	EJECTBUTTON,
	ACCEPTBUTTON,
	CANCELBUTTON,
	OPT1BUTTON,
	OPT2BUTTON,
	OPT3BUTTON,
	AUX1BUTTON,
	AUX2BUTTON,
	AUX3BUTTON,
	SYSINTERLOCKBYPASS,
	CONVEYORBYPASS,
	AIRINSTALLED,
	LOWAIR,
	xxCONTROLLERWATCHDOG,
	xxCONTROLLERPREWATCHDOG,
	xxTIMER1,
	xxTIMER2,
	xxTIMER3,
	CONTROLLERDIAGNOSTIC, // should be last
};

enum CameraType
{
	CT_OPTEON_7300_CLASSIC =0,//the classic camera w/ old driver
	CT_OPTEON_7300, // the classic camera
	CT_OPTEON_7301, // wafercam
	CT_OPTEON_7310, // extreme
	CT_SENTECH,
	CT_UNKNOW
};

// NT73XX & C51140
// SystemController Firmware Revision
enum { SC_SOFTWAREONLY = -2, SC_PREFLASH = -1, SC_A0, SC_A9, SC_B0};
enum { SC_STATUS_NORMAL = 0, SC_STATUS_SO, SC_STATUS_EMO,    };

#define CONTROL_DIAG_73           0x00000001 //[0]  diag_int           0x0001
#define SYS_TIMER1_73             0x00000002 //[1]  timer 1            0x0002
#define SYS_TIMER2_73             0x00000004 //[1]  timer 2            0x0004
#define SYS_TIMER3_73             0x00000005 //[1]  timer 3            0x0008
#define SYS_PRE_TIMEOUT_73        0x00000010 //[4]  pretimeout_status, 0x0010
#define SYS_TIMEOUT_73            0x00000020 //[5]  time_out_status,   0x0020
#define EXTRA_73                  0x00000040 //[6]  extra_status       0x0040
#define PS_73                     0x00000080 //[7]  ps_status,         0x0080
#define DRIVERS_73                0x00000100 //[8]  driver_status,     0x0100
#define LED_TIMEOUT_73            0x00000200 //[9]  watch_dog_status,  0x0200
#define SYSINTERLOCK_73           0x00000400 //[10] interlock_status,  0x0400
#define PANEL_73                  0x00000800 //[11] panel_status,      0x0800
#define OPTO1_73                  0x00002000 //[13] opto_one_status,   0x2000
#define OPTO2_73                  0x00004000 //[14] opto_two_status,   0x4000
#define OPTO3_73                  0x00008000 //[15] opto_three_status, 0x8000
// NT73XX & C51140 panel buttons
#define LOADBUTTON_73             0x00000001
#define INSPECTBUTTON_73          0x00000002
#define EJECTBUTTON_73            0x00000004
#define CANCELBUTTON_73           0x00000008
#define OPT1BUTTON_73             0x00000010
#define OPT2BUTTON_73             0x00000020
#define OPT3BUTTON_73             0x00000040
#define AUX1BUTTON_73             0x00000010
#define AUX2BUTTON_73             0x00000020
#define AUX3BUTTON_73             0x00000040

#define LOWAIRMESSAGE             _T("Low Air Warning")

#define LOWAIR_73                 0x00000040
#define CONVEYORBYPASS_73         0x00000080

//NT73XX & C51140 drivers
#define V24_DRV_73                0x00000001
#define V12_DRV_73                0x00000002
#define TOWER_DRV_73              0x00000003
#define PANEL_DRV_73              0x00000004
#define LIGHTFLASHDURATION        0x00000200

// button switches
enum {NOTPRESSED, PRESSED};

// barcode
enum BARCODESTATUS {BARCODE_DISABLED,
					BARCODE_ENABLED,
					BARCODE_SOFTWARE_ONLY,
					BARCODE_MANUAL};
enum { BARCODEPORT1 = 'A', BARCODEPORT2 = 'B', };
enum { _COM1, _COM2, _COM3, _COM4,             };
enum { SCANNER_NONE, SCANNER_REQUIRED,         };
enum { BC_NORMAL, BC_FREERUN, BC_MANUAL,       };
enum { BC_NOREAD = -1, BC_GOODREAD = 0,        };
enum { BC_SAVED, BC_DELETED, BC_NOCHANGE,      };

#define SCANNER_NOT_FOUND BC_NOREAD
#define PARSING_SCHEME_NOT_FOUND SCANNER_NOT_FOUND

// conveyor status change codes
#define CONVYEOR_PORT_CHANGED      0x0001
#define CONVYEOR_EJECT_CHANGED     0x0002
#define CONVYEOR_AUTOWIDTH_CHANGED 0x0004
#define CONVYEOR_SMEMA_CHANGED     0x0008
#define CONVYEOR_LIFTER_CHANGED    0x0010
#define CONVYEOR_INSERT_CHANGED    0x0020
#define CONVYEOR_ENDFINDER_CHANGED 0x0040
#define CONVEYOR_SMEMA_POLARITY_CHANGED	0x0080
#define CONVEYOR_STAGEMODE_CHANGED 0x0100
#define CONVEYOR_HALFMODE_CHANGED  0x0200

//conveyor clamp position
#define CONVEYOR_CLP_POS_MAX	200
#define CONVEYOR_CLP_POS_MIN	50
// auto width
enum AW_HDIR {AW_IN, AW_OUT};
#define AW_MIN  3750
#define AW_MAX 24125

// lifters
enum CLIFTER {NOLIFTERS, LIFTER1, LIFTER2, BOTHLIFTERS};


//motor thingies
enum	MOTORDRIVERTYPE { MT_LMOTOR = 0, MT_MCMOTOR, MT_ACSCMOTOR, MT_ACSCMOTOR2 };
enum	MOTORCOILTYPE	{ MS_TBC = 0, MS_NONE, MS_LEA, MS_PTR, MS_LEB, MS_LZM };
enum	MOTORCOILPOS	{ MP_LEA = 6000, MP_LEB = 8000, MP_LZM = 4000, MP_LEAO = 12000};
#define MOTORCOILSTRING(index)	index == 0 ? _T("TBC") : index == 1 ? _T("NONE")\
				: index == 2 ? _T("LEA") : index == 3 ? _T("PTR") \
				: index == 4 ? _T("LEB") : _T("LZM")
#define COILPOSTOL 750
#define ONMOTOR_HOMERANGE(type, pos) type == MS_LEA ? ((((pos <= (MP_LEA + COILPOSTOL)) && ( pos >= (MP_LEA - COILPOSTOL))) ? TRUE\
									:((pos >= MP_LEAO) ? TRUE : FALSE)))\
								    :type == MS_LEB ? (((pos <= (MP_LEB + COILPOSTOL)) && ( pos >= (MP_LEB - COILPOSTOL))) ? TRUE : FALSE)\
									:type == MS_LZM ? (((pos <= (MP_LZM + COILPOSTOL)) && ( pos >= (MP_LZM - COILPOSTOL))) ? TRUE : FALSE)\
									:FALSE//unknow type
enum	MOTOR_AXIS {XAxis, YAxis, Both};
enum	VARIABLE {Velocity, Position, PWM, Error};
enum	FOV {PointThreeInch, PointFourInch, PointSixInch, PointSevenInch, OneInch};
enum	STEPSIZE {NoMove, ExtraCoarse, Coarse, Fine, ExtraFine};
enum	DIRECTION {Minus = -1,Plus = 1};
#define MOTOR_MINSPEED  50
#define MOTOR_MAXSPEED 150
#define MAX_MOTOR_ERROR 25  // microns
#define MOTOR_NOT_BUSY 1
#define MOTOR_BUSY	0
//struct type define
struct MOTOR_POS 
{
	int XPosition;
	int YPosition;
};

enum MOVE_TYPE {Relative, Absolute, Sequence, Coordinated};

enum MCDRIVER_FIRMWARE_VERSION
{
	MC_A_0 = 35,
	MC_A_1 = 36,
	MC_MAX = 999
};
#define MCDRIVER_FIRMWARE_VERSION_STRING(index)	index == 35 ? _T("MC_A_0") : index == 36 ? _T("MC_A_1")\
				: _T("MC_MAX")

typedef struct _DOME
{
	int word;
} DOME;


union MOTOR_STATUS {
	struct
	{
		unsigned short X_Plus	: 1 ;
		unsigned short X_Minus	: 1	;
		unsigned short Y_Plus	: 1	;
		unsigned short Y_Minus	: 1	;
		unsigned short E_Stop	: 1	;
		unsigned short Unused2	: 1	;
		unsigned short Unused3	: 1	;
		unsigned short Unused4	: 1	;
	} status;

	unsigned char Status;
};

union ConveyorState    // 16 bits
{
    struct          // caller's definition
    {
        unsigned int StopSensor		: 1;
		unsigned int PassThru		: 1;
		unsigned int Error			: 1;
		unsigned int LeftSensor		: 1;

		unsigned int MiddleSensor	: 1;
		unsigned int RightSensor	: 1;
		unsigned int SMEMABrdAvIn	: 1;
		unsigned int SMEMAMRdyIn	: 1;

		unsigned int SMEMAMRdyOut	: 1;
		unsigned int SMEMABrdAvOut	: 1;
		unsigned int ExtSensor		: 1;
		unsigned int Lifter1		: 1; // 1 == going right; 0 == not

		unsigned int Lifter2		: 1; // 1 == going left;  0 == not
		unsigned int BoardClamps	: 1;
		unsigned int AutoWidth		: 1; // 1 == moving; 0 == not
		unsigned int Conveyor		: 1;
        
    } convstate;

    unsigned short cstate; 
};

//20080115, morgan hsiao,
//add the two enumeration is for TCDlgCycleStop attributes setting.
enum CYCLE_STOP_DIALOG_TYPE { csdtWARNING = 0, csdtPROMPT, csdtBLINK };


#endif // !defined(_HARDWARE_ENUM_H_)

