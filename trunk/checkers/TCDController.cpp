/*
 * Module:		$RCSfile: TCDController.cpp $ - diagnostic controller class
 *				$Revision: 1.9 $
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
 *				(c) COPYRIGHT 1999 - 2001, TERADYNE, INC.
 *
 * Discussion:
 *
 * Date:		January. 6, 2000
 *
 */

#include "stdafx.h"

#include "timer.h"
#include "hardwareIO/pcio.hpp"
#include "hardwareIO/hardware.hpp"

#include "resource.h"
#include "checkers/TCDRegistry.hpp"
#include "checkers/TCDiagClass.hpp"
#include "checkers/TCDController.hpp"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*****************************************************************************/
/*  memory test patterns                                                     */
/*****************************************************************************/

SHORT patterns [] =
{
	(SHORT) 0x0000, (SHORT) 0x0001, (SHORT) 0x0002, (SHORT) 0x0004,
	(SHORT) 0x0008, (SHORT) 0x0010, (SHORT) 0x0020, (SHORT) 0x0040,
	(SHORT) 0x0080, (SHORT) 0x0100, (SHORT) 0x0200, (SHORT) 0x0400,
	(SHORT) 0x0800, (SHORT) 0x1000, (SHORT) 0x2000, (SHORT) 0x4000,
	(SHORT) 0x8000, (SHORT) 0xfffe, (SHORT) 0xfffd, (SHORT) 0xfffb,
	(SHORT) 0xfff7, (SHORT) 0xffef, (SHORT) 0xffdf, (SHORT) 0xffbf,
	(SHORT) 0xff7f, (SHORT) 0xfeff, (SHORT) 0xfdff, (SHORT) 0xfbff,
	(SHORT) 0xf7ff, (SHORT) 0xefff, (SHORT) 0xdfff, (SHORT) 0xbfff,
	(SHORT) 0x7fff, (SHORT) 0xffff, (SHORT) 0xaaaa, (SHORT) 0x5555
};

/*****************************************************************************/
/*  TCDController Class                                                      */
/*****************************************************************************/

/*
 * Function:	constructor
 *
 * Description:	construct the class
 *
 * Parameters:
 *				TCViewDiags    *Wnd		 - parent window pointer
 *				TCHardwareIO   *console  - hardwareIO object pointer
 * 
 * Return Values
 *		none
 *
 */
TCDController::TCDController(TCCheckers *Wnd, TCHardwareIO * controller)
	: TCDiagClass(Wnd)
{
	m_pMainWnd       = Wnd;		// main window
	m_pHardware      = controller;
	m_pPcio          = m_pHardware->GetPcio();
	m_ControllerType = m_pHardware->GetControllerType();
	m_HardwareMode   = m_pHardware->GetMode();
	m_ControllerPF   = DIAG_PASSED;

	m_ControllerInterrupt = FALSE;
	m_pHardware->RegisterInterruptCallback(CONTROLLERDIAGNOSTIC, this,
			(PCIO_HANDLER_PTR) ControllerInterrupt);
	m_pHardware->EnableDiagnosticInterrupt();
}

/*
 * Function:	destrutor
 *
 * Description:	destroy the class
 *
 * Parameters:
 *		void
 * 
 * Return Values
 *		none
 *
 */
TCDController::~TCDController()
{
	m_pHardware->UnRegisterInterruptCallback(CONTROLLERDIAGNOSTIC);
	m_pHardware->ClearDiagnosticInterrupt();

	// indicate a PASS or FAIL
	m_DiagPF = m_ControllerPF;
}

/*
 * Function:	Create
 *
 * Description:	create the member objects
 *
 * Parameters:
 *		none
 * 
 * Return Values
 *		none
 *
 */
void
TCDController::Create(void)
{
	CString ibox, boardNum, firmRev, PCARev, PCBRev, Sn;

	// start button
	TCException estart = TCEXCEPTION(HW_CHECKER_START);
	//m_Button1Caption = _T("Start");
	m_Button1Caption.Format("%s", estart.getMessage());
	m_StartButton.CreateButton(m_Button1Caption,
		TCDiagRect(60,170, 140,200), m_pMainWnd, IDC_CHECKERS_BUTTON1);
	m_StartButton.ShowWindow(SW_SHOW);

	// cancel button
	TCException ecancel = TCEXCEPTION(HW_CHECKER_CANCEL);
	//m_Button2Caption = _T("Cancel");
	m_Button2Caption.Format("%s", ecancel.getMessage());
	m_CancelButton.CreateButton(m_Button2Caption,
		TCDiagRect(60,210, 140,240), m_pMainWnd, IDC_CHECKERS_BUTTON2);
	m_CancelButton.ShowWindow(SW_SHOW);
	m_CancelButton.EnableWindow(FALSE);

	if (m_HardwareMode == SOFTWARE_ONLY)
	{
		ibox = _T("System Controller tests are unavailable in SOFTWARE ONLY mode.");
		boardNum = _T("<software only>");
		firmRev = _T("");
		PCARev = _T("");
		PCBRev = _T("");
		Sn = _T("");

		m_StartButton.EnableWindow(FALSE);
	}
	else
	{
		TCException emsg = TCEXCEPTION(HW_SYSTEM_MSG);/*
		ibox = _T("Click the 'Start' button to test the System "\
			"Controller.");*/
		ibox.Format("%s", emsg.getMessage());

		// board part number
		DWORD bn = m_pPcio->GetControllerBoardNumber();
		boardNum.Format(_T("%03x-%03x-%02x"), (bn >> 20) & 0xfff,
			(bn >> 8) & 0xfff, bn & 0xff);
		firmRev = m_pPcio->GetControllerFirmware();
		PCARev = m_pPcio->GetControllerPCA();
		PCBRev = m_pPcio->GetControllerPCB();
		Sn.Format("%d", m_pPcio->GetControllerSerialNumber());
	}

	// instructions
	m_IBox.Create(ibox, m_IRect, (CWnd *) m_pMainWnd, IDC_STATIC);

	// board title display
	TCException esysconbrd = TCEXCEPTION(HW_SYSTEM_SYSCONBRD);/*
	m_BoardDisplay.Create(_T("System Controller Board:"), WS_CHILD | WS_VISIBLE,
		CRect(CHECKERS_X_OFFSET + 60,CHECKERS_Y_OFFSET + 50,
				CHECKERS_X_OFFSET + 219,CHECKERS_Y_OFFSET + 69),
		(CWnd *) m_pMainWnd, IDC_STATIC);*/
	m_BoardDisplay.Create( esysconbrd.getMessage(), WS_CHILD | WS_VISIBLE,
		CRect(CHECKERS_X_OFFSET + 60,CHECKERS_Y_OFFSET + 50,
				CHECKERS_X_OFFSET + 219,CHECKERS_Y_OFFSET + 69),
		(CWnd *) m_pMainWnd, IDC_STATIC);
	m_BoardDisplay.SetFont(&m_dlgFont);
	m_BoardDisplay.ShowWindow(SW_SHOW);

	// board part number display
	m_BoardNumDisplay.Create(boardNum, WS_CHILD | WS_VISIBLE,
		CRect(CHECKERS_X_OFFSET + 220,CHECKERS_Y_OFFSET + 50,
				CHECKERS_X_OFFSET + 299,CHECKERS_Y_OFFSET + 69),
		(CWnd *) m_pMainWnd, IDC_STATIC);
	m_BoardNumDisplay.SetFont(&m_dlgFont);
	m_BoardNumDisplay.ShowWindow(SW_SHOW);

	// firmware title display
	TCException efirmrev = TCEXCEPTION(HW_SYSTEM_FIRMREV);/*
	m_FirmwareDisplay.Create(_T("Firmware Revision:"), WS_CHILD | WS_VISIBLE,
		CRect(CHECKERS_X_OFFSET + 60,CHECKERS_Y_OFFSET + 70,
				CHECKERS_X_OFFSET + 219,CHECKERS_Y_OFFSET + 89),
		(CWnd *) m_pMainWnd, IDC_STATIC);*/
	m_FirmwareDisplay.Create( efirmrev.getMessage(), WS_CHILD | WS_VISIBLE,
		CRect(CHECKERS_X_OFFSET + 60,CHECKERS_Y_OFFSET + 70,
				CHECKERS_X_OFFSET + 219,CHECKERS_Y_OFFSET + 89),
		(CWnd *) m_pMainWnd, IDC_STATIC);
	m_FirmwareDisplay.SetFont(&m_dlgFont);
	m_FirmwareDisplay.ShowWindow(SW_SHOW);

	// firmware revision display
	m_FirmwareRevDisplay.Create(firmRev, WS_CHILD | WS_VISIBLE,
		CRect(CHECKERS_X_OFFSET + 220,CHECKERS_Y_OFFSET + 70,
				CHECKERS_X_OFFSET + 299,CHECKERS_Y_OFFSET + 89),
		(CWnd *) m_pMainWnd, IDC_STATIC);
	m_FirmwareRevDisplay.SetFont(&m_dlgFont);
	m_FirmwareRevDisplay.ShowWindow(SW_SHOW);

	// PCA title display
	TCException epcarev = TCEXCEPTION(HW_SYSTEM_PCAREV);/*
	m_PCADisplay.Create(_T("PCA Revision:"), WS_CHILD | WS_VISIBLE,
		CRect(CHECKERS_X_OFFSET + 60,CHECKERS_Y_OFFSET + 90,
				CHECKERS_X_OFFSET + 219,CHECKERS_Y_OFFSET + 109),
		(CWnd *) m_pMainWnd, IDC_STATIC);*/
	m_PCADisplay.Create(epcarev.getMessage(), WS_CHILD | WS_VISIBLE,
		CRect(CHECKERS_X_OFFSET + 60,CHECKERS_Y_OFFSET + 90,
				CHECKERS_X_OFFSET + 219,CHECKERS_Y_OFFSET + 109),
		(CWnd *) m_pMainWnd, IDC_STATIC);
	m_PCADisplay.SetFont(&m_dlgFont);
	m_PCADisplay.ShowWindow(SW_SHOW);

	// PCA revision display
	m_PCARevDisplay.Create(PCARev, WS_CHILD | WS_VISIBLE,
		CRect(CHECKERS_X_OFFSET + 220,CHECKERS_Y_OFFSET + 90,
				CHECKERS_X_OFFSET + 299,CHECKERS_Y_OFFSET + 109),
		(CWnd *) m_pMainWnd, IDC_STATIC);
	m_PCARevDisplay.SetFont(&m_dlgFont);
	m_PCARevDisplay.ShowWindow(SW_SHOW);

	// PCB title display
	TCException epcbrev = TCEXCEPTION(HW_SYSTEM_PCBREV);/*
	m_PCBDisplay.Create(_T("PCB Revision:"), WS_CHILD | WS_VISIBLE,
		CRect(CHECKERS_X_OFFSET + 60,CHECKERS_Y_OFFSET + 110,
				CHECKERS_X_OFFSET + 219,CHECKERS_Y_OFFSET + 129),
		(CWnd *) m_pMainWnd, IDC_STATIC);*/
	m_PCBDisplay.Create(epcbrev.getMessage(), WS_CHILD | WS_VISIBLE,
		CRect(CHECKERS_X_OFFSET + 60,CHECKERS_Y_OFFSET + 110,
				CHECKERS_X_OFFSET + 219,CHECKERS_Y_OFFSET + 129),
		(CWnd *) m_pMainWnd, IDC_STATIC);
	m_PCBDisplay.SetFont(&m_dlgFont);
	m_PCBDisplay.ShowWindow(SW_SHOW);

	// PCB revision display
	m_PCBRevDisplay.Create(PCARev, WS_CHILD | WS_VISIBLE,
		CRect(CHECKERS_X_OFFSET + 220,CHECKERS_Y_OFFSET + 110,
				CHECKERS_X_OFFSET + 299,CHECKERS_Y_OFFSET + 129),
		(CWnd *) m_pMainWnd, IDC_STATIC);
	m_PCBRevDisplay.SetFont(&m_dlgFont);
	m_PCBRevDisplay.ShowWindow(SW_SHOW);

	// SN title display
	TCException eserial = TCEXCEPTION(HW_SYSTEM_SERIAL);/*
	m_SNDisplay.Create(_T("Serial Number:"), WS_CHILD | WS_VISIBLE,
		CRect(CHECKERS_X_OFFSET + 60,CHECKERS_Y_OFFSET + 130,
				CHECKERS_X_OFFSET + 219,CHECKERS_Y_OFFSET + 149),
		(CWnd *) m_pMainWnd, IDC_STATIC);*/
	m_SNDisplay.Create(eserial.getMessage(), WS_CHILD | WS_VISIBLE,
		CRect(CHECKERS_X_OFFSET + 60,CHECKERS_Y_OFFSET + 130,
				CHECKERS_X_OFFSET + 219,CHECKERS_Y_OFFSET + 149),
		(CWnd *) m_pMainWnd, IDC_STATIC);
	m_SNDisplay.SetFont(&m_dlgFont);
	m_SNDisplay.ShowWindow(SW_SHOW);

	// SN display
	m_SNumDisplay.Create(PCARev, WS_CHILD | WS_VISIBLE,
		CRect(CHECKERS_X_OFFSET + 220,CHECKERS_Y_OFFSET + 130,
				CHECKERS_X_OFFSET + 299,CHECKERS_Y_OFFSET + 149),
		(CWnd *) m_pMainWnd, IDC_STATIC);
	m_SNumDisplay.SetFont(&m_dlgFont);
	m_SNumDisplay.ShowWindow(SW_SHOW);
    TCException esyscon = TCEXCEPTION(HW_FLASH_SYSTEM);
	//CreateCommonButtons(_T("System Controller"), FALSE);
	CreateCommonButtons(esyscon.getMessage(), FALSE);
}

void
TCDController::ControllerTest(void)
{

	if (m_HardwareMode == SOFTWARE_ONLY)
		return;

	// clear the cancel falg
	m_cancel = FALSE;
	m_StartButton .EnableWindow(FALSE);
	m_CancelButton.EnableWindow(TRUE);

	TestAddress();
	TestMemory();
	TestInterrupts();

	m_StartButton.EnableWindow(TRUE);
	m_CancelButton.EnableWindow(FALSE);

	TCException esyscon = TCEXCEPTION(HW_PCIO_COMPLETE);
	//CString diag = _T("\r\nSYSTEM CONTROLLER Checkers completed: ");
	CString diag;
	diag.Format("%s", esyscon.getMessage());
	DiagReport(diag, m_ControllerPF);
	DiagSeparator();
}

void
TCDController::TestAddress(void) 
{
	m_AddressPF = DIAG_PASSED;
	USHORT readWord;
	CString error;
	CWaitCursor wait;

	USHORT StartAddress, EndAddress; 
	USHORT address;
	// 51134 controller address range 0x1000 thru 0x1fff
	if (m_ControllerType == C51134)
	{
		StartAddress = 0x1000;
		EndAddress   = 0x1fff;
	}
	// 51140 controller address range 0x2000 thru 0x3fff
	else
	{ // C51140
		StartAddress = 0x2000;
		EndAddress   = 0x3fff;
	}

	for (address = StartAddress; address <= EndAddress; address++)
	{
		m_pPcio->Dome_ioWrite(address, &address);
		m_pPcio->Dome_ioRead(address,  &readWord);
		if (readWord != address)
		{
			error.Format(_T("System Controller Memory Address failure; "\
								"Address:0x%x, Read:0x%x"),
				address, readWord);
			DiagOutput(error);
			m_AddressPF = DIAG_FAILED;
		}

		if (m_cancel == TRUE)
		{
			m_AddressPF = DIAG_ABORTED;
			break;
		}
	}

	TCException ememoryadd = TCEXCEPTION(HW_SYSTEM_MEMORYADD);
	//CString diag = _T("System Controller Memory Address Tests: Completed: ");
	CString diag;
	diag.Format("%s", ememoryadd.getMessage());
	DiagReport(diag, m_AddressPF);
}

void
TCDController::WritePattern(SHORT pattern) 
{
	USHORT writeWord = pattern;

	SHORT StartAddress, EndAddress; 
	SHORT address;
	// 51134 controller address range 0x1000 thru 0x1fff
	if (m_ControllerType == C51134)
	{
		StartAddress = 0x1000;
		EndAddress   = 0x1fff;
	}
	// 51140 controller address range 0x2000 thru 0x3fff
	else
	{ // C51140
		StartAddress = 0x2000;
		EndAddress   = 0x3fff;
	}

	for (address = StartAddress; address <= EndAddress; address++)
	{
		m_pPcio->Dome_ioWrite(address, &writeWord);

		if (m_cancel == TRUE)
			break;
	}
}

void
TCDController::ReadPattern(SHORT pattern) 
{
	USHORT readWord;
	CString error;

	SHORT StartAddress, EndAddress; 
	SHORT address;

	// clear flag
	m_MemoryPF = DIAG_PASSED;

	// 51134 controller address range 0x1000 thru 0x1fff
	if (m_ControllerType == C51134)
	{
		StartAddress = 0x1000;
		EndAddress   = 0x1fff;
	}
	// 51140 controller address range 0x2000 thru 0x3fff
	else
	{ // C51140
		StartAddress = 0x2000;
		EndAddress   = 0x3fff;
	}

	for (address = StartAddress; address <= EndAddress; address++)
	{
		m_pPcio->Dome_ioRead(address, &readWord);
		if (readWord != pattern)
		{
			error.Format(_T("System Controller Memory failure; Address:0x%x, "\
							"Expected:0x%x, Read:0x%x"),
				address, pattern, readWord);
			DiagOutput(error);
			m_MemoryPF = DIAG_FAILED;
		}

		if (m_cancel == TRUE)
		{
			m_MemoryPF = DIAG_ABORTED;
			break;
		}
	}
}

void
TCDController::TestMemory(void) 
{
	CWaitCursor wait;
	for (int patternloop = 0; patternloop < 36; patternloop++)
	{
		SHORT pattern = patterns[patternloop];
		WritePattern(pattern);
		ReadPattern(pattern);
		if (m_cancel == TRUE)
		{
			m_MemoryPF = DIAG_ABORTED;
			break;
		}
	}

	TCException ememory = TCEXCEPTION(HW_SYSTEM_MEMORY);
	//CString diag = _T("System Controller Memory Tests: Completed: ");
	CString diag;
	diag.Format("%s", ememory.getMessage());
	DiagReport(diag, m_MemoryPF);

	// set pass/fail flag
	m_ControllerPF = m_MemoryPF;
}

void
TCDController::TestInterrupts(void) 
{
	USHORT data;
	m_ControllerInterrupt = FALSE;
	m_InterruptPF         = DIAG_PASSED;
	CWaitCursor wait;

	if (m_ControllerType == C51134)
	{
		// generate the faux interrupt
		m_pHardware->EnableDiagnosticInterrupt();
		m_pHardware->DomeRead (OPTO1STATUS, &data);
		m_pHardware->DomeWrite(OPTO1STATUS, (data |= 0x20));
	}
	else
	{ // C51140
		TRACE("testing interrupts\n");
		m_pHardware->ClearDiagnosticInterrupt();
		m_pHardware->EnableDiagnosticInterrupt();
		m_pHardware->TriggerDiagnosticInterrupt();
	}


	int i;
	for (i = 0; i < 20; i++)
	{ // wait for interrupt
		if (m_ControllerInterrupt == TRUE)
			break;
		if (m_cancel == TRUE)
		{
			m_InterruptPF = DIAG_ABORTED;
			break;
		}
		Sleep(20);
	}

	if (i >= 20) // timeout, no interrupt
	{
		TRACE("interrupt test failed\n");
		m_InterruptPF = DIAG_FAILED;
	}
	TCException einterrupt = TCEXCEPTION(HW_SYSTEM_INTERRUPT);
	//CString diag = _T("System Controller Interrupt Tests: Completed: ");
	CString diag;
	diag.Format("%s", einterrupt.getMessage());
	DiagReport(diag, m_InterruptPF);

	// set pass/fail flag
	m_ControllerPF = m_InterruptPF;
}

void
TCDController::Cancel(void) 
{
	m_cancel = TRUE;
}

void CALLBACK
TCDController::ControllerInterrupt(void *classPtr, int interrupt)
{
	// pointer to TCConsole object
	TCDController * control = (TCDController *) classPtr;

	// clear interrupt
	control->m_pHardware->ClearDiagnosticInterrupt();
	// flag interrupt to test thread
	control->m_ControllerInterrupt = TRUE;
}
