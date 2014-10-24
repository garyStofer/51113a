/*
 * Module:		$RCSfile: TCDFlash.cpp $ - diagnostic Flash class
 *				$Revision: 1.40.1.1 $
 *
 * Author:		Don White
 *
 * Affiliation:	Teradyne, Inc.
 *
 * Copyright:	This  is  an  unpublished  work.   Any  unauthorized
 *				reproduction  is  PROHIBITED.  This unpublished work
 *				is protected by Federal Copyright Law.  If this work
 *				becomes published, the following notice shall apply:
 *
 *				(c) COPYRIGHT 2001, TERADYNE, INC.
 *
 * Discussion:
 *
 * Date:		Jan. 9, 2001
 *
 */

#include "stdafx.h"
#include <afxtempl.h>

#include "timer.h"
#include "hardware_enum.h"
#include "lmotors.hpp"
#include "MCmotors.hpp"
#include "hardwareIO/hardware.hpp"
#include "serialport/serialport.h"
#include "hardwareIO/barcode.hpp"
#include "hardwareIO/conveyors.hpp"
#include "MechIO.hpp"

#include "resource.h"
#include "checkers/diagnostics.hpp"
#include "checkers/TCDiagClass.hpp"
#include "checkers/TCDFlash.hpp"

#include "hardwareIO/pcio.hpp"
#include "hardwareIO/jamexprt.h"

#include "TCException.hpp"

#include <io.h>
#include <sys/stat.h>
//#include "flash_export.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*****************************************************************************/
/*****************************************************************************/
/*  TCDFlash Class                                                           */
/*****************************************************************************/
/*****************************************************************************/
/*
 * Function:	constructor
 *
 * Description:	construct the class
 *
 * Parameters:
 *				TCDiagDlg      *Wnd		 - parent window pointer
 *				TCHardwareIO   *console  - hardwareIO object pointer
 * 
 * Return Values
 *		none
 *
 */
TCDFlash::TCDFlash(TCCheckers *Wnd, TCMechIO * mechio)
	: TCDiagClass(Wnd)
{
	m_pMainWnd     = Wnd;		// main window
	m_pMechIO      = mechio;

	m_pHardware    = m_pMechIO->GetHardwarePtr();
	m_pPcio        = m_pHardware->GetPcio();
	m_pConveyor    = m_pHardware->GetConveyorPtr();
	m_pMotors      = m_pHardware->GetLMotorPtr();
	m_pBarCode     = m_pHardware->GetBarcodePtr();

	m_SystemType   = m_pHardware->GetSystemType();
	m_HardwareMode = m_pHardware->GetMode();
};

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
TCDFlash::~TCDFlash()
{
	if (m_HardwareMode != SOFTWARE_ONLY)
	{
		// wait untill worker thread before event unregister
		while (GetWorkerThreadStatus() == TRUE)
			Sleep(200); //wait until thread dies
	}
};

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
TCDFlash::Create(void)
{
	int x, y;	// temp coordinate pair.
	
	m_HardwareEnabled = TRUE;
	//find out if this is a 7300, 7350 or 5500
	SYSTEMTYPE m_SystemType = m_pHardware->GetSystemType();
	

	CreateCommonButtons(_T("Flash"), FALSE);
	m_FlashPF        = DIAG_PASSED;

	// instructions
	m_IRect.SetRect(CHECKERS_IBOX_DEFAULT_LEFT, CHECKERS_IBOX_DEFAULT_TOP -20,
					CHECKERS_IBOX_DEFAULT_RIGHT,CHECKERS_IBOX_DEFAULT_TOP +80);
	m_IBox.Create(_T(""), m_IRect, (CWnd *) m_pMainWnd, IDC_STATIC);/*
	m_IBox.SetWindowText(_T("Flash will attempt to program the controllers in "\
							"your system.\nPlease choose firmware files to program."));*/
	TCException emsg = TCEXCEPTION(HW_FLASH_MSG);
    m_IBox.SetWindowText(emsg.getMessage());

	// Create filepath edit boxes
	x = 110;
	y = m_IRect.bottom + 20;	// Note - m_IRect.bottom was adjusted in the constructor

	// SC box position
	x = 45; y = m_IRect.bottom + 15;
	TCException esystem = TCEXCEPTION(HW_FLASH_SYSTEM);/*
	m_SCBox.Create(_T(_T("System Controller:")), BS_GROUPBOX,
		(CRect) TCDiagRect(BOXLEFT,BOXTOP, BOXRIGHT,BOXBOTTOM), 
		(CWnd *) m_pMainWnd, IDC_CHECKERS_RBUTTONA);*/
	m_SCBox.Create(esystem.getMessage(), BS_GROUPBOX,
		(CRect) TCDiagRect(BOXLEFT,BOXTOP, BOXRIGHT,BOXBOTTOM), 
		(CWnd *) m_pMainWnd, IDC_CHECKERS_RBUTTONA);
	m_SCBox.SetFont(&m_dlgFont);
	m_SCBox.ShowWindow(SW_SHOW);

	// SC title
	TCException ejamfile = TCEXCEPTION(HW_FLASH_JAMFILE);/*
	m_JAM.Create(_T("JAM File:"), WS_VISIBLE | SS_CENTERIMAGE,
		(CRect) TCDiagRect(TITLELEFT,TITLETOP, TITLERIGHT,TITLEBOTTOM),
		(CWnd *) m_pMainWnd);*/
	m_JAM.Create(ejamfile.getMessage(), WS_VISIBLE | SS_CENTERIMAGE,
		(CRect) TCDiagRect(TITLELEFT,TITLETOP, TITLERIGHT,TITLEBOTTOM),
		(CWnd *) m_pMainWnd);
	m_JAM.SetFont(&m_dlgFont);

 	// SC browse button position
	m_JAMFileEdit.CreateEx(WS_EX_CLIENTEDGE, _T("EDIT"), _T(""),
		WS_CHILD | WS_VISIBLE | WS_BORDER,
		(CRect) TCDiagRect(EDITLEFT,EDITTOP, EDITRIGHT,EDITBOTTOM),
		(CWnd *) m_pMainWnd, IDC_CHECKERS_EDIT5);
	m_JAMFileEdit.SetFont(&m_dlgFont);

	TCException ejamborwse = TCEXCEPTION(HW_FLASH_JAMBORWSE);
	//m_Button5Caption = _T("JAM Browse");
	m_Button5Caption.Format("%s",ejamborwse.getMessage());
	m_BrowseJAMButton.CreateButton(m_Button5Caption,
		TCDiagRect(BUTTONLEFT, BUTTONTOP, BUTTONRIGHT, BUTTONBOTTOM), 
		m_pMainWnd, IDC_CHECKERS_BUTTON5);
	m_BrowseJAMButton.ShowWindow(SW_SHOW);

	y += 80;
	TCException emotor = TCEXCEPTION(HW_FLASH_MOTOR);/*
	m_MCBox.Create(_T("Motor Controller:"), BS_GROUPBOX,
		(CRect) TCDiagRect(BOXLEFT,BOXTOP, BOXRIGHT,BOXBOTTOM2),
		(CWnd *) m_pMainWnd, IDC_CHECKERS_RBUTTON7);*/
	m_MCBox.Create(emotor.getMessage(), BS_GROUPBOX,
		(CRect) TCDiagRect(BOXLEFT,BOXTOP, BOXRIGHT,BOXBOTTOM2),
		(CWnd *) m_pMainWnd, IDC_CHECKERS_RBUTTON7);
	m_MCBox.SetFont(&m_dlgFont);
	m_MCBox.ShowWindow(SW_SHOW);

	//dolnit:benchtop start
	int iShow = SW_SHOW;
	if(m_pMotors->m_iMotorType == MT_MCMOTOR)
	{
		TCException ecfgbro = TCEXCEPTION(HW_FLASH_CFGBRO);
		//m_Button1Caption = _T("CFG Browse");
		m_Button1Caption.Format("%s", ecfgbro.getMessage());
		m_BrowseCFGButton.CreateButton(m_Button1Caption,
			TCDiagRect(BUTTONLEFT, BUTTONTOP, BUTTONRIGHT, BUTTONBOTTOM),
			m_pMainWnd, IDC_CHECKERS_BUTTON1);
		m_BrowseCFGButton.ShowWindow( iShow );
		iShow = SW_HIDE;
	}
	else
	{
		TCException eprgbro = TCEXCEPTION(HW_FLASH_PRGBRO);
		//m_Button1Caption = _T("PRG Browse");
		m_Button1Caption.Format("%s", eprgbro.getMessage());
		m_BrowsePRGButton.CreateButton(m_Button1Caption,
			TCDiagRect(BUTTONLEFT, BUTTONTOP, BUTTONRIGHT, BUTTONBOTTOM),
			m_pMainWnd, IDC_CHECKERS_BUTTON1);
		m_BrowsePRGButton.ShowWindow( iShow );
	}
	TCException ecfgfile = TCEXCEPTION(HW_FLASH_CFGFILE);/*
	m_CFG.Create( _T("CFG File:"), WS_VISIBLE | SS_CENTERIMAGE,
		(CRect) TCDiagRect(TITLELEFT,TITLETOP, TITLERIGHT,TITLEBOTTOM),
		(CWnd *) m_pMainWnd);*/
	m_CFG.Create( ecfgfile.getMessage(), WS_VISIBLE | SS_CENTERIMAGE,
		(CRect) TCDiagRect(TITLELEFT,TITLETOP, TITLERIGHT,TITLEBOTTOM),
		(CWnd *) m_pMainWnd);
	m_CFG.SetFont(&m_dlgFont);
	m_CFG.ShowWindow( iShow == SW_HIDE ? SW_SHOW: SW_HIDE );

	m_CFGFileEdit.CreateEx(WS_EX_CLIENTEDGE,
		_T("EDIT"), _T(""), WS_CHILD | WS_VISIBLE | WS_BORDER,
		(CRect) TCDiagRect(EDITLEFT,EDITTOP, EDITRIGHT,EDITBOTTOM),
		(CWnd *) m_pMainWnd, IDC_CHECKERS_EDIT0);
	m_CFGFileEdit.SetFont(&m_dlgFont);
	m_CFGFileEdit.ShowWindow( iShow == SW_HIDE ? SW_SHOW: SW_HIDE );

	TCException eprgfile = TCEXCEPTION(HW_FLASH_PRGFILE);/*
	m_PRG.Create( _T("PRG File:"), WS_VISIBLE | SS_CENTERIMAGE,
		(CRect) TCDiagRect(TITLELEFT,TITLETOP, TITLERIGHT,TITLEBOTTOM),
		(CWnd *) m_pMainWnd);*/
	m_PRG.Create( eprgfile.getMessage(), WS_VISIBLE | SS_CENTERIMAGE,
		(CRect) TCDiagRect(TITLELEFT,TITLETOP, TITLERIGHT,TITLEBOTTOM),
		(CWnd *) m_pMainWnd);
	m_PRG.SetFont(&m_dlgFont);
	m_PRG.ShowWindow( iShow );

	m_PRGFileEdit.CreateEx(WS_EX_CLIENTEDGE,
		_T("EDIT"), _T(""), WS_CHILD | WS_VISIBLE | WS_BORDER,
		(CRect) TCDiagRect(EDITLEFT,EDITTOP, EDITRIGHT,EDITBOTTOM),
		(CWnd *) m_pMainWnd, IDC_CHECKERS_EDIT1);
	m_PRGFileEdit.SetFont(&m_dlgFont);
	m_PRGFileEdit.ShowWindow( iShow );


	y += 40;
	if(m_pMotors->m_iMotorType == MT_ACSCMOTOR)
	{
		TCException espifile = TCEXCEPTION(HW_FLASH_SPIFILE);/*
		m_SPI.Create(_T("SPI File:"), WS_VISIBLE | SS_CENTERIMAGE,
			(CRect) TCDiagRect(TITLELEFT,TITLETOP, TITLERIGHT,TITLEBOTTOM),
			(CWnd *) m_pMainWnd);*/
		m_SPI.Create(espifile.getMessage(), WS_VISIBLE | SS_CENTERIMAGE,
			(CRect) TCDiagRect(TITLELEFT,TITLETOP, TITLERIGHT,TITLEBOTTOM),
			(CWnd *) m_pMainWnd);
		m_SPI.SetFont(&m_dlgFont);
		m_SPI.ShowWindow( iShow );

		m_SETFileEdit.CreateEx(WS_EX_CLIENTEDGE,
			_T("EDIT"), _T(""), WS_CHILD | WS_VISIBLE | WS_BORDER,
			(CRect) TCDiagRect(EDITLEFT,EDITTOP, EDITRIGHT,EDITBOTTOM),
			(CWnd *) m_pMainWnd, IDC_CHECKERS_EDIT2);
		m_SETFileEdit.SetFont(&m_dlgFont);
		m_SETFileEdit.ShowWindow( iShow );

		TCException espibro = TCEXCEPTION(HW_FLASH_SPIBRO);
		//m_Button2Caption = _T("SPI Browse");
		m_Button2Caption.Format("%s",espibro.getMessage());
		m_BrowseSETButton.CreateButton(m_Button2Caption,
			TCDiagRect(BUTTONLEFT, BUTTONTOP, BUTTONRIGHT, BUTTONBOTTOM),
			m_pMainWnd, IDC_CHECKERS_BUTTON2);
		m_BrowseSETButton.ShowWindow( iShow );
	}
	else
	{
		TCException esetfile = TCEXCEPTION(HW_FLASH_SETFILE);/*
		m_SET.Create(_T("SET File:"), WS_VISIBLE | SS_CENTERIMAGE,
			(CRect) TCDiagRect(TITLELEFT,TITLETOP, TITLERIGHT,TITLEBOTTOM),
			(CWnd *) m_pMainWnd);*/
		m_SET.Create(esetfile.getMessage(), WS_VISIBLE | SS_CENTERIMAGE,
			(CRect) TCDiagRect(TITLELEFT,TITLETOP, TITLERIGHT,TITLEBOTTOM),
			(CWnd *) m_pMainWnd);
		m_SET.SetFont(&m_dlgFont);
		m_SET.ShowWindow( iShow );

		m_SETFileEdit.CreateEx(WS_EX_CLIENTEDGE,
			_T("EDIT"), _T(""), WS_CHILD | WS_VISIBLE | WS_BORDER,
			(CRect) TCDiagRect(EDITLEFT,EDITTOP, EDITRIGHT,EDITBOTTOM),
			(CWnd *) m_pMainWnd, IDC_CHECKERS_EDIT2);
		m_SETFileEdit.SetFont(&m_dlgFont);
		m_SETFileEdit.ShowWindow( iShow );

		TCException esetbro = TCEXCEPTION(HW_FLASH_SETBRO);
		//m_Button2Caption = _T("SET Browse");
		m_Button2Caption.Format("%s", esetbro.getMessage());
		m_BrowseSETButton.CreateButton(m_Button2Caption,
			TCDiagRect(BUTTONLEFT, BUTTONTOP, BUTTONRIGHT, BUTTONBOTTOM),
			m_pMainWnd, IDC_CHECKERS_BUTTON2);
		m_BrowseSETButton.ShowWindow( iShow );
	}
	//dolnit:benchtop end
	
	y += 80;
	TCException econcont = TCEXCEPTION(HW_FLASH_CONCONT);/*
	m_CVBox.Create(_T("Conveyor Controller:"), BS_GROUPBOX,
		(CRect) TCDiagRect(BOXLEFT,BOXTOP, BOXRIGHT,BOXBOTTOM),
		(CWnd *) m_pMainWnd, IDC_CHECKERS_RBUTTON8);*/
	m_CVBox.Create(econcont.getMessage(), BS_GROUPBOX,
		(CRect) TCDiagRect(BOXLEFT,BOXTOP, BOXRIGHT,BOXBOTTOM),
		(CWnd *) m_pMainWnd, IDC_CHECKERS_RBUTTON8);
	m_CVBox.SetFont(&m_dlgFont);
	m_CVBox.ShowWindow(SW_SHOW);

	m_7300.Create(_T("7300"), BS_AUTORADIOBUTTON | WS_GROUP,
		(CRect) TCDiagRect(BOXLEFT+6,BOXTOP+25, BOXLEFT+60,BOXTOP+45),
				(CWnd *) m_pMainWnd, IDC_CHECKERS_RBUTTON1);
	m_7300.SetFont(&m_dlgFont);
	m_7300.ShowWindow(SW_SHOW);
	m_7300.EnableWindow( iShow == SW_HIDE ? FALSE: TRUE );

	m_7350.Create(_T("7350"), BS_AUTORADIOBUTTON,
		(CRect) TCDiagRect(BOXLEFT+6,BOXTOP+46, BOXLEFT+60,BOXTOP+66),
				(CWnd *) m_pMainWnd, IDC_CHECKERS_RBUTTON2);
	m_7350.SetFont(&m_dlgFont);
	m_7350.ShowWindow(SW_SHOW);
	m_7350.EnableWindow( iShow == SW_HIDE ? FALSE: TRUE );

	TCException ehexfile = TCEXCEPTION(HW_FLASH_HEXFILE);/*
	m_HEX.Create(_T("HEX File:"), WS_VISIBLE | SS_CENTERIMAGE | WS_GROUP,
		(CRect) TCDiagRect(TITLELEFT,TITLETOP, TITLERIGHT,TITLEBOTTOM),
		(CWnd *) m_pMainWnd);*/
	m_HEX.Create(ehexfile.getMessage(), WS_VISIBLE | SS_CENTERIMAGE | WS_GROUP,
		(CRect) TCDiagRect(TITLELEFT,TITLETOP, TITLERIGHT,TITLEBOTTOM),
		(CWnd *) m_pMainWnd);
	m_HEX.SetFont(&m_dlgFont);

	m_HEXFileEdit.CreateEx(WS_EX_CLIENTEDGE,
		_T("EDIT"), _T(""), WS_CHILD | WS_VISIBLE | WS_BORDER | WS_GROUP,
		(CRect) TCDiagRect(EDITLEFT,EDITTOP, EDITRIGHT,EDITBOTTOM),
		(CWnd *) m_pMainWnd, IDC_CHECKERS_EDIT3);
	m_HEXFileEdit.SetFont(&m_dlgFont);
	m_HEXFileEdit.EnableWindow( iShow == SW_HIDE ? FALSE: TRUE );

	TCException ehexbro = TCEXCEPTION(HW_FLASH_HEXBRO);
	//m_Button3Caption = _T("HEX Browse");
	m_Button3Caption.Format("%s", ehexbro.getMessage());
	m_BrowseHEXButton.CreateButton(m_Button3Caption,
		TCDiagRect(BUTTONLEFT, BUTTONTOP, BUTTONRIGHT, BUTTONBOTTOM),
		m_pMainWnd, IDC_CHECKERS_BUTTON3);
	m_BrowseHEXButton.ShowWindow(SW_SHOW);
	//m_ConveyorType = CC7_ATWC;
	m_ConveyorType = m_pConveyor->GetConveyorType();
	if (m_ConveyorType == CC7_UMSCO)
	{
		TCException etxtfile = TCEXCEPTION(HW_FLASH_TXTFILE);
		TCException etxtbro = TCEXCEPTION(HW_FLASH_TXTBRO);
		SendMessage(m_7350, BM_SETCHECK, BST_CHECKED, 0);
		//m_HEX.SetWindowText(_T("TXT File:"));
		//m_BrowseHEXButton.SetWindowText(_T("TXT Browse"));
		m_HEX.SetWindowText(etxtfile.getMessage());
		m_BrowseHEXButton.SetWindowText(etxtbro.getMessage());
	}
	else if(m_ConveyorType == CC7_ATWC)
		SendMessage(m_7300, BM_SETCHECK, BST_CHECKED, 0);
	
	m_BrowseHEXButton.EnableWindow( iShow == SW_HIDE ? FALSE: TRUE );

	// barcode group
	y += 80;
	TCException ebarcodescanners = TCEXCEPTION(HW_FLASH_BARCODESCANNERS);/*
	m_BCRBox.Create(_T("Bar Code Scanners:"), BS_GROUPBOX,
		(CRect) TCDiagRect(BOXLEFT,BOXTOP, BOXRIGHT,BOXBOTTOM2),
		(CWnd *) m_pMainWnd,
		IDC_CHECKERS_RBUTTON9);*/
	m_BCRBox.Create(ebarcodescanners.getMessage(), BS_GROUPBOX,
		(CRect) TCDiagRect(BOXLEFT,BOXTOP, BOXRIGHT,BOXBOTTOM2),
		(CWnd *) m_pMainWnd,
		IDC_CHECKERS_RBUTTON9);
	m_BCRBox.SetFont(&m_dlgFont);
	m_BCRBox.ShowWindow(SW_SHOW);

	TCException ebarcodefile = TCEXCEPTION(HW_FLASH_BARCODEFILE);/*
	m_BAR.Create(_T("BAR File:"), WS_VISIBLE | SS_CENTERIMAGE,
		(CRect) TCDiagRect(TITLELEFT,TITLETOP, TITLERIGHT,TITLEBOTTOM),
		(CWnd *) m_pMainWnd);*/
	m_BAR.Create(ebarcodefile.getMessage(), WS_VISIBLE | SS_CENTERIMAGE,
		(CRect) TCDiagRect(TITLELEFT,TITLETOP, TITLERIGHT,TITLEBOTTOM),
		(CWnd *) m_pMainWnd);
	m_BAR.SetFont(&m_dlgFont);

	m_BARFileEdit.CreateEx(WS_EX_CLIENTEDGE, _T("EDIT"), _T(""),
		WS_CHILD | WS_VISIBLE | WS_BORDER,
		(CRect) TCDiagRect(EDITLEFT,EDITTOP, EDITRIGHT,EDITBOTTOM),
		(CWnd *) m_pMainWnd, IDC_CHECKERS_EDIT4);
	m_BARFileEdit.SetFont(&m_dlgFont);

	TCException ebarbro = TCEXCEPTION(HW_FLASH_BARBRO);
	//m_Button4Caption = _T("BAR Browse");
	m_Button4Caption.Format("%s",ebarbro.getMessage());
	m_BrowseBARButton.CreateButton(m_Button4Caption,
		TCDiagRect(BUTTONLEFT, BUTTONTOP, BUTTONRIGHT, BUTTONBOTTOM),
		m_pMainWnd, IDC_CHECKERS_BUTTON4);
	m_BrowseBARButton.ShowWindow(SW_SHOW);

	y += 40;
	TCException escanners = TCEXCEPTION(HW_FLASH_SCANNER);/*
	m_BarScanners.Create(_T("Scanners:"), WS_VISIBLE | SS_CENTERIMAGE,
		(CRect) TCDiagRect(TITLELEFT,TITLETOP, TITLERIGHT,TITLEBOTTOM),
		(CWnd *) m_pMainWnd);*/
	m_BarScanners.Create(escanners.getMessage(), WS_VISIBLE | SS_CENTERIMAGE,
		(CRect) TCDiagRect(TITLELEFT,TITLETOP, TITLERIGHT,TITLEBOTTOM),
		(CWnd *) m_pMainWnd);
	m_BarScanners.SetFont(&m_dlgFont);

	m_Scanners.Create(WS_CHILD | WS_VISIBLE | WS_THICKFRAME | WS_VSCROLL |
									CBS_DROPDOWNLIST |  CBS_AUTOHSCROLL,
		(CRect) TCDiagRect(EDITLEFT,EDITTOP, EDITRIGHT,EDITBOTTOM + 200),
		(CWnd *) m_pMainWnd, IDC_CHECKERS_EDIT6);
	m_Scanners.SetFont(&m_dlgFont);

	CStringArray list;
	m_pBarCode->GetScannerList(list);
	for (int i = 0; i < list.GetSize(); i++)
	{
		m_Scanners.AddString(list[i]);
	}
	m_Scanners.SetCurSel(0);

	// Create Progress Control
	y += 80;
	TCException eprogress = TCEXCEPTION(CA_GREY_PROGRESS);/*
	m_PB.Create(_T("Progress:"), WS_VISIBLE | SS_CENTERIMAGE,
		(CRect) TCDiagRect(TITLELEFT,TITLETOP, TITLERIGHT,TITLEBOTTOM),
		(CWnd *) m_pMainWnd);*/
	m_PB.Create(eprogress.getMessage(), WS_VISIBLE | SS_CENTERIMAGE,
		(CRect) TCDiagRect(TITLELEFT,TITLETOP, TITLERIGHT,TITLEBOTTOM),
		(CWnd *) m_pMainWnd);
	m_PB.SetFont(&m_dlgFont);
	m_PB.ShowWindow(SW_SHOW);
	m_ProgressCtrl.Create(WS_BORDER | WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
		(CRect) TCDiagRect(EDITLEFT,EDITTOP, EDITRIGHT,EDITBOTTOM),
		(CWnd *) m_pMainWnd, IDC_CHECKERS_PROGRESSBAR);
	// Zero the progress bar - note: default range is 0-100	m_ProgressCtrl.SetPos(0);
	m_ProgressCtrl.SetStep(1);
	m_ProgressCtrl.ShowWindow(SW_SHOW);

	// Create Program Button - below Progress Control
	TCException eprog = TCEXCEPTION(HW_FLASH_PROG);
	//m_Button6Caption = _T("Program");
	m_Button6Caption.Format("%s", eprog.getMessage());
	m_ProgramButton.CreateButton(m_Button6Caption,
		TCDiagRect(BUTTONLEFT, BUTTONTOP, BUTTONRIGHT, BUTTONBOTTOM),
		m_pMainWnd, IDC_CHECKERS_BUTTON6);
	m_ProgramButton.ShowWindow(SW_SHOW);
	m_ProgramButton.EnableWindow(FALSE);	// Disable Program button for now


	x = 50; y = 35;
	TCException erev = TCEXCEPTION(HW_FLASH_REV);/*
	m_RevisionBox.Create(_T("System Revisions:"), BS_GROUPBOX,
		(CRect) TCDiagRect(REVBOXLEFT,REVBOXTOP, REVBOXRIGHT,REVBOXBOTTOM),
		(CWnd *) m_pMainWnd,
		IDC_CHECKERS_RBUTTON9);*/
	m_RevisionBox.Create(erev.getMessage(), BS_GROUPBOX,
		(CRect) TCDiagRect(REVBOXLEFT,REVBOXTOP, REVBOXRIGHT,REVBOXBOTTOM),
		(CWnd *) m_pMainWnd,
		IDC_CHECKERS_RBUTTON9);
	m_RevisionBox.SetFont(&m_dlgFont);
	m_RevisionBox.ShowWindow(SW_SHOW);

	TCException esystemr = TCEXCEPTION(HW_FLASH_SYSTEM);/*
	m_SCRevisionTitle.Create(_T("System Controller :"), WS_VISIBLE | SS_CENTERIMAGE,
		(CRect) TCDiagRect(REVTITLELEFT,REVTITLETOP, REVTITLERIGHT,REVTITLEBOTTOM),
		(CWnd *) m_pMainWnd);*/
	m_SCRevisionTitle.Create(esystemr.getMessage(), WS_VISIBLE | SS_CENTERIMAGE,
		(CRect) TCDiagRect(REVTITLELEFT,REVTITLETOP, REVTITLERIGHT,REVTITLEBOTTOM),
		(CWnd *) m_pMainWnd);
	m_SCRevisionTitle.SetFont(&m_dlgFont);
	m_SCRevisionTitle.ShowWindow(SW_SHOW);

	m_SCRevision.Create(_T(":"), WS_VISIBLE | SS_CENTERIMAGE,
		(CRect) TCDiagRect(REVLEFT,REVTOP, REVRIGHT,REVBOTTOM),
		(CWnd *) m_pMainWnd);
	m_SCRevision.SetFont(&m_dlgFont);
	m_SCRevision.ShowWindow(SW_SHOW);

	y += 15;
	TCException econcontr = TCEXCEPTION(HW_FLASH_CONCONT);/*
	m_ConveyorRevisionTitle.Create(_T("Conveyor Controller :"), WS_VISIBLE | SS_CENTERIMAGE,
		(CRect) TCDiagRect(REVTITLELEFT,REVTITLETOP, REVTITLERIGHT,REVTITLEBOTTOM),
		(CWnd *) m_pMainWnd);*/
	m_ConveyorRevisionTitle.Create(econcontr.getMessage(), WS_VISIBLE | SS_CENTERIMAGE,
		(CRect) TCDiagRect(REVTITLELEFT,REVTITLETOP, REVTITLERIGHT,REVTITLEBOTTOM),
		(CWnd *) m_pMainWnd);
	m_ConveyorRevisionTitle.SetFont(&m_dlgFont);
	m_ConveyorRevisionTitle.ShowWindow(SW_SHOW);

	m_ConveyorRevision.Create(_T(":"), WS_VISIBLE | SS_CENTERIMAGE,
		(CRect) TCDiagRect(REVLEFT,REVTOP, REVRIGHT,REVBOTTOM),
		(CWnd *) m_pMainWnd);
	m_ConveyorRevision.SetFont(&m_dlgFont);
	m_ConveyorRevision.ShowWindow(SW_SHOW);

	FillControllerRevisions();
/*
	CStatic       m_SCRevisionTitle;
	CStatic       m_MotorRevision;
	CStatic       m_MororRevisionTitle;
	CStatic       m_ConveyorRevision;
	CStatic       m_ConveyorRevisionTitle;
*/
}

/*
 * Function:	Close (predestructor)
 *
 * Description:	destroy the worker thread
 *
 * Parameters:
 *		int		mode (not used)
 * 
 * Return Values
 *		none
 *
 */
void
TCDFlash::Close(int mode)
{
	TCDiagClass::Close(MANUAL);
}

/*
 * Function:	FillControllerRevisions
 *
 * Description:	
 *
 * Parameters:
 *		void
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *
 */
void
TCDFlash::FillControllerRevisions(void)
{
	m_pHardware->GetSysControllerFirmwareRevision(m_SCFirmwareRevision);
	m_SCRevision.SetWindowText(m_SCFirmwareRevision);
	m_ConveyorFirmwareRevision = m_pConveyor->GetConverorFirmwareRevision();
	if (m_ConveyorFirmwareRevision.IsEmpty() || m_ConveyorType == CC7_NONE)
		m_ConveyorFirmwareRevision = _T("Unknown");
	else if( m_ConveyorType == CC7_MC )
	{
		m_ConveyorFirmwareRevision.TrimRight();
	}
	else
	{
		m_ConveyorFirmwareRevision.TrimRight();
		m_ConveyorFirmwareRevision = m_ConveyorFirmwareRevision.Mid(5);
	}
	m_ConveyorRevision.SetWindowText(m_ConveyorFirmwareRevision);
};

/******************************************************************************/
/*  Load Board  CallBack function                                             */
/******************************************************************************/
void
FlashEvent (TCEventData *EventData, void *ClientData, AOI_EVENT event)
{

}

void
TCDFlash::DisableAllButtons(void)
{
	(m_pMotors->m_iMotorType == MT_MCMOTOR) ?
		m_BrowseCFGButton.EnableWindow(FALSE):	// Motor Controller #1
		m_BrowsePRGButton.EnableWindow(FALSE);	// Motor Controller #1

	m_BrowseSETButton.EnableWindow(FALSE);	// Motor Controller #2

	m_BrowseHEXButton.EnableWindow(FALSE);	// Conveyor Controller
	m_BrowseBARButton.EnableWindow(FALSE);	// Barcode Scanner
	m_BrowseJAMButton.EnableWindow(FALSE);	// System Controller
	m_ProgramButton.EnableWindow(FALSE);	// Start Programming

	m_pCloseButton->EnableWindow(FALSE);
};

void
TCDFlash::EnableAllButtons(void)
{
	(m_pMotors->m_iMotorType == MT_MCMOTOR) ?
		m_BrowseCFGButton.EnableWindow(TRUE):	// Motor Controller #1
		m_BrowsePRGButton.EnableWindow(TRUE);	// Motor Controller #1

	m_BrowseSETButton.EnableWindow(TRUE);	// Motor Controller #2

	m_BrowseBARButton.EnableWindow(TRUE);	// Barcode Scanner
	m_BrowseJAMButton.EnableWindow(TRUE);	// System Controller

	m_pCloseButton->EnableWindow(TRUE);
};

void
TCDFlash::ClearAllWindows(void)
{
	//clear all file fields
	m_PRGFileEdit.SetWindowText(_T(""));
	m_SETFileEdit.SetWindowText(_T(""));
	m_CFGFileEdit.SetWindowText(_T(""));
	m_HEXFileEdit.SetWindowText(_T(""));
	m_BARFileEdit.SetWindowText(_T(""));
	m_JAMFileEdit.SetWindowText(_T(""));
};

void
TCDFlash::C7300(void)
{
	m_HEX.SetWindowText(_T("HEX File:"));
	m_BrowseHEXButton.SetWindowText(_T("HEX Browse"));
	m_ConveyorType = CC7_ATWC;
};

void
TCDFlash::C7350(void)
{
	m_HEX.SetWindowText(_T("TXT File:"));
	m_BrowseHEXButton.SetWindowText(_T("TXT Browse"));
	m_ConveyorType = CC7_UMSCO;
};

void
TCDFlash::BrowsePRG(void)
{

	if(m_SystemType == NT5500 || m_SystemType == NT7355)
	{
		BrowseCFG();
		return;
	}
	char FileName[256] = {0};
	CString FormatFileName;
	static char BASED_CODE szFilter[] = _T("Prg Files (*.prg)|*.prg|All Files (*.*)|*.*||");

	CFileDialog dlgOpen(TRUE,				// give us an open file dialog
		NULL,									// no default extension
		NULL,									// no default filename
		OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY, 
		szFilter,								// our extension filter
		NULL);									// the parent window

	GetModuleFileName(NULL, FileName, 256);

	FormatFileName = FileName;
	FormatFileName.MakeLower();

	if(FormatFileName.Find(_T("bin")) != -1)
	{	
		FormatFileName.Replace(_T("bin"),_T("config"));					
#ifdef _DEBUG
		FormatFileName.Replace(_T("optimad.exe"),NULL);			
#else
		FormatFileName.Replace(_T("optima.exe"),NULL);			
#endif	
		dlgOpen.m_ofn.lpstrInitialDir = FormatFileName;
	}

	if (dlgOpen.DoModal() == IDOK)
	{
		m_PRGFileEdit.SetWindowText(dlgOpen.GetPathName());
		m_ProgramButton.EnableWindow(TRUE);
		m_ProgramButton.SetFocus();
		if (m_HardwareEnabled)
			m_IBox.SetWindowText(_T("Press 'Program' to start programming. . . "));
	}	
}

void
TCDFlash::BrowseSET(void)
{
	char FileName[256] = {0};
	CString FormatFileName;
	FormatFileName.Format("%s", (m_pMotors->m_iMotorType == MT_ACSCMOTOR) ?
		_T("Spi Files (*.spi)|*.spi|All Files (*.*)|*.*||"):
		_T("Set Files (*.set)|*.set|All Files (*.*)|*.*||") );
	static char* BASED_CODE szFilter = FormatFileName.GetBuffer(FormatFileName.GetLength());

	CFileDialog dlgOpen(TRUE,				// give us an open file dialog
		NULL,									// no default extension
		NULL,									// no default filename
		OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY, 
		szFilter,								// our extension filter
		NULL);									// the parent window

	GetModuleFileName(NULL, FileName, 256);

	FormatFileName = FileName;
	FormatFileName.MakeLower();

	if(FormatFileName.Find(_T("bin")) != -1)
	{	
		FormatFileName.Replace(_T("bin"),_T("config"));					
#ifdef _DEBUG
		FormatFileName.Replace(_T("optimad.exe"),NULL);			
#else
		FormatFileName.Replace(_T("optima.exe"),NULL);			
#endif	
		dlgOpen.m_ofn.lpstrInitialDir = FormatFileName;
	}

	if (dlgOpen.DoModal() == IDOK)
	{
		m_SETFileEdit.SetWindowText(dlgOpen.GetPathName());
		m_ProgramButton.EnableWindow(TRUE);
		m_ProgramButton.SetFocus();
		if (m_HardwareEnabled)
			m_IBox.SetWindowText(_T("Press 'Program' to start programming. . . "));
	}	
}

void
TCDFlash::BrowseCFG(void)
{
	char FileName[256] = {0};
	CString FormatFileName;
	static char BASED_CODE szFilter[] = _T("Cfg Files (*.cfg)|*.cfg|All Files (*.*)|*.*||");

	CFileDialog dlgOpen(TRUE,				// give us an open file dialog
		NULL,									// no default extension
		NULL,									// no default filename
		OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY, 
		szFilter,								// our extension filter
		NULL);									// the parent window

	GetModuleFileName(NULL, FileName, 256);

	FormatFileName = FileName;
	FormatFileName.MakeLower();

	if(FormatFileName.Find(_T("bin")) != -1)
	{	
		FormatFileName.Replace(_T("bin"),_T("config"));					
#ifdef _DEBUG
		FormatFileName.Replace(_T("optimad.exe"),NULL);			
#else
		FormatFileName.Replace(_T("optima.exe"),NULL);			
#endif	
		dlgOpen.m_ofn.lpstrInitialDir = FormatFileName;
	}

	if (dlgOpen.DoModal() == IDOK)
	{
		m_CFGFileEdit.SetWindowText(dlgOpen.GetPathName());
		m_ProgramButton.EnableWindow(TRUE);
		m_ProgramButton.SetFocus();
		if (m_HardwareEnabled)
			m_IBox.SetWindowText(_T("Press 'Program' to start programming. . . "));
	}	
}

void
TCDFlash::BrowseHEX(void)
{
	//identify the 8051 #515 or #535
	TCException e = TCEXCEPTION(HW_CHECKERS_CONVEYOR);
	//if(MessageBox(NULL,"Identify Conveyor CPU type ?", "IDENTIFY", MB_YESNO | MB_ICONWARNING) == IDYES)
	if(MessageBox(NULL,e.getMessage(), "IDENTIFY", MB_YESNO | MB_ICONWARNING) == IDYES)
	{
		COMMTIMEOUTS TimeOut;
		char received[300] = {0};
		int ret = 2;
		//set us to a minimal hardware mode
		m_pHardware->ForceHardwareFlashMode();

		// setup conveyor flash mode
		m_pConveyor->SetFlashMode(TRUE);
		//reset
		m_pConveyor->ClearSerialBufferFlash();

		// set timeouts to wait for first character
		TimeOut.ReadIntervalTimeout = 0;
		TimeOut.ReadTotalTimeoutMultiplier = 0;
		TimeOut.ReadTotalTimeoutConstant = 10000;	 
		m_pConveyor->SetComTimeoutFlash(TimeOut);					

		/*UserMessage(_T("Please press 'OK' and reset the conveyors now.\n"\
					"You will have 10 seconds after pressing 'OK' "\
					"to press reset."),
					"Reset Conveyors",	MB_OK | MB_ICONWARNING);*/
		TCException e = TCEXCEPTION(HW_SECONDS_PRESSING);
		AfxMessageBox( e.getMessage(), MB_OK | MB_ICONERROR);
	
		m_IBox.SetWindowText("Waiting. . . ");
	
		// Wait for the first character
		if (!m_pConveyor->GetComPortFlash(received,82))
			ret = 2;

		// set timeouts back to something normal
		TimeOut.ReadIntervalTimeout = 0;
		TimeOut.ReadTotalTimeoutMultiplier = 5;
		TimeOut.ReadTotalTimeoutConstant = 50;	 
		m_pConveyor->SetComTimeoutFlash(TimeOut);

		if(received[2] == 'B' || received[3] == 'B') ret = 1;
		if(received[2] == 'M' || received[3] == 'M') ret =0;

		TCException e1 = TCEXCEPTION(HW_CHECKERS_CONVEYOR1);
		TCException e2 = TCEXCEPTION(HW_CHECKERS_CONVEYOR2);
		TCException e3 = TCEXCEPTION(HW_CHECKERS_CONVEYOR3);
		switch ( ret )
		{
		case 2:
			//MessageBox(NULL,"Conveyor Reset Timeout error", "Conveyor Error", MB_OK);
			MessageBox(NULL, e1.getMessage(), "Conveyor Error", MB_OK);
			break;
		case 1:
			//MessageBox(NULL,"RECOMMEND USING CONVEYOR 535", "IDENTIFY", MB_OK);
			MessageBox(NULL, e2.getMessage(), "Conveyor Error", MB_OK);
			break;
		case 0:
			//MessageBox(NULL,"RECOMMEND USING CONVEYOR 515", "IDENTIFY", MB_OK);
			MessageBox(NULL, e3.getMessage(), "Conveyor Error", MB_OK);
			break;
		}
		// clear conveyor flash mode
		m_pConveyor->SetFlashMode(FALSE);
		//
		m_pHardware->ClearHardwareFlashMode();
	}

	char FileName[256] = {0};
	CString FormatFileName;
	static char BASED_CODE szHexFilter[] = _T("Hex Files (*.hex)|*.hex|All Files (*.*)|*.*||");
	static char BASED_CODE szTxtFilter[] = _T("Txt Files (*.txt)|*.txt|All Files (*.*)|*.*||");

	CFileDialog dlgOpen(TRUE,				// give us an open file dialog
		NULL,									// no default extension
		NULL,									// no default filename
		OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY, 
												// our extension filter
		m_ConveyorType == CC7_ATWC ? szHexFilter : szTxtFilter,
		NULL);									// the parent window

	GetModuleFileName(NULL, FileName, 256);

	FormatFileName = FileName;
	FormatFileName.MakeLower();

	if(FormatFileName.Find(_T("bin")) != -1)
	{	
		FormatFileName.Replace(_T("bin"),_T("config"));					
#ifdef _DEBUG
		FormatFileName.Replace(_T("optimad.exe"),NULL);			
#else
		FormatFileName.Replace(_T("optima.exe"),NULL);			
#endif	
		dlgOpen.m_ofn.lpstrInitialDir = FormatFileName;
	}

	if (dlgOpen.DoModal() == IDOK)
	{
		m_HEXFileEdit.SetWindowText(dlgOpen.GetPathName());
		m_ProgramButton.EnableWindow(TRUE);
		m_ProgramButton.SetFocus();
		if (m_HardwareEnabled)
			m_IBox.SetWindowText(_T("Press 'Program' to start programming. . . "));
	}	
}

void
TCDFlash::BrowseBAR(void)
{
	char FileName[256] = {0};
	CString FormatFileName;
	static char BASED_CODE szFilter[] = _T("Bar Files (*.bar)|*.bar|All Files (*.*)|*.*||");

	CFileDialog dlgOpen(TRUE,				// give us an open file dialog
		NULL,									// no default extension
		NULL,									// no default filename
		OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY, 
		szFilter,								// our extension filter
		NULL);									// the parent window

	GetModuleFileName(NULL, FileName, 256);

	FormatFileName = FileName;
	FormatFileName.MakeLower();

	if(FormatFileName.Find(_T("bin")) != -1)
	{	
		FormatFileName.Replace(_T("bin"),_T("config"));					
#ifdef _DEBUG
		FormatFileName.Replace(_T("optimad.exe"),NULL);			
#else
		FormatFileName.Replace(_T("optima.exe"),NULL);			
#endif	
		dlgOpen.m_ofn.lpstrInitialDir = FormatFileName;
	}

	if (dlgOpen.DoModal() == IDOK)
	{
		m_BARFileEdit.SetWindowText(dlgOpen.GetPathName());
		m_ProgramButton.SetFocus();
		m_ProgramButton.EnableWindow(TRUE);
		if (m_HardwareEnabled)
			m_IBox.SetWindowText(_T("Press 'Program' to start programming. . . "));
	}	
}

void
TCDFlash::BrowseJAM(void)
{
	char FileName[256] = {0};
	CString FormatFileName;
	static char BASED_CODE szFilter[] = _T("Jam Files (*.jam)|*.jam|All Files (*.*)|*.*||");

	CFileDialog dlgOpen(TRUE,				// give us an open file dialog
		NULL,									// no default extension
		NULL,									// no default filename
		OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY, 
		szFilter,								// our extension filter
		NULL);									// the parent window

	GetModuleFileName(NULL, FileName, 256);

	FormatFileName = FileName;
	FormatFileName.MakeLower();

	if(FormatFileName.Find(_T("bin")) != -1)
	{	
		FormatFileName.Replace(_T("bin"),_T("config"));					
#ifdef _DEBUG
		FormatFileName.Replace(_T("optimad.exe"),NULL);			
#else
		FormatFileName.Replace(_T("optima.exe"),NULL);			
#endif	
		dlgOpen.m_ofn.lpstrInitialDir = FormatFileName;
	}

	if (dlgOpen.DoModal() == IDOK)
	{
		m_JAMFileEdit.SetWindowText(dlgOpen.GetPathName());
		m_ProgramButton.EnableWindow(TRUE);
		m_ProgramButton.SetFocus();
		if (m_HardwareEnabled)
			m_IBox.SetWindowText(_T("Press 'Program' to start programming. . . "));
	}	
}

void
TCDFlash::OnEditPRG(void)
{
	m_ProgramButton.EnableWindow(TRUE);	//enable the program button
}


void
TCDFlash::OnEditSET(void)
{
	m_ProgramButton.EnableWindow(TRUE);	//enable the program button
}

void
TCDFlash::OnEditCFG(void)
{
	m_ProgramButton.EnableWindow(TRUE);	//enable the program button
}

void
TCDFlash::OnEditHEX(void)
{
	m_ProgramButton.EnableWindow(TRUE);	//enable the program button
}


void
TCDFlash::OnEditBAR(void)
{
	m_ProgramButton.EnableWindow(TRUE);	//enable the program button
}


void
TCDFlash::OnEditJAM(void)
{
	m_ProgramButton.EnableWindow(TRUE);	//enable the program button
}

void
TCDFlash::OnProgram(void)
{
	int ret, m, numTasks;
	BOOL m_bPrgStat;
	BOOL m_bSetStat;
	BOOL m_bSpiStat;
	BOOL m_bCfgStat;
	BOOL m_bHexStat;
	BOOL m_bBarStat;
	BOOL m_bJamStat;
	BOOL m_bSuccess = FALSE;
	BOOL m_bFail = FALSE;
	unsigned short data;
	CString m_Alert;

	//initial status
	m_bPrgStat = FALSE;
	m_bSetStat = FALSE;
	m_bSpiStat = FALSE;
	m_bCfgStat = FALSE;
	m_bHexStat = FALSE;
	m_bBarStat = FALSE;
	m_bJamStat = FALSE;
	numTasks   = 0;
	m_PrgFile  = _T("");
	m_SpiFile  = _T("");
	m_SetFile  = _T("");
	m_CfgFile  = _T("");
	m_HexFile  = _T("");
	m_BarFile  = _T("");
	m_JamFile  = _T("");

	//getting the files selected by the user
	m_PRGFileEdit.GetWindowText(m_PrgFile);

	(m_pMotors->m_iMotorType == MT_ACSCMOTOR) ?
		m_SETFileEdit.GetWindowText(m_SpiFile):
		m_SETFileEdit.GetWindowText(m_SetFile);

	m_CFGFileEdit.GetWindowText(m_CfgFile);
	m_HEXFileEdit.GetWindowText(m_HexFile);
	m_BARFileEdit.GetWindowText(m_BarFile);
	m_JAMFileEdit.GetWindowText(m_JamFile);

	//if all file fields are empty
	if (m_PrgFile == _T("") && m_SetFile == _T("") &&
		m_HexFile == _T("") && m_BarFile == _T("") &&
		m_JamFile == _T("") && m_CfgFile == _T("") && m_SpiFile == _T(""))
	{
		//UserMessage(_T("No files to program."), _T("Flash"), MB_OK);
		TCException e = TCEXCEPTION(HW_NOFILES_PROGRAM);
		AfxMessageBox( e.getMessage(), MB_OK);
		m_IBox.SetWindowText(_T("Please choose files to program. . ."));
		m_ProgramButton.EnableWindow(FALSE);	//disable program button
		return;
	}

	//inform user of what we are going to do. 
	m_Alert = _T("You have chosen to perform the following tasks:\n");
	
	if (m_JamFile != _T(""))
	{
		m_Alert += _T("    -Program the system controller with the "\
			"specifed JAM file.\n");
		numTasks++;
	}
	if (m_PrgFile != _T(""))
	{
		m_Alert += _T("    -Program the motor controller with the "\
			"specifed PRG file.\n");
		numTasks++;
	}
	if (m_SetFile != _T(""))
	{
		m_Alert += _T("    -Program the motor controller with the "\
			"specifed SET file.\n");
		numTasks++;
	}
	if (m_SpiFile != _T(""))
	{
		m_Alert += _T("    -Program the motor controller with the "\
			"specifed SPI file.\n");
		numTasks++;
	}
	if (m_CfgFile != _T(""))
	{
		m_Alert += _T("    -Program the motor controller with the "\
			"specifed CFG file.\n");
		numTasks++;
	}
	if (m_HexFile != _T(""))
	{
		if (m_SystemType == NT7300) 
			m_Alert += _T("    -Program the conveyor controller with "\
				"the specifed HEX file.\n");
		else
			m_Alert += _T("    -Program the Umsco conveyor controller with "\
				"the specifed TXT file.\n");
		numTasks++;
	}
	if (m_BarFile != _T(""))
	{
		m_Alert += _T("    -Program the bar code scanner with the specifed "\
			"PRG file.\n");
		numTasks++;
	}

	m_Alert += _T("Once programming starts, it cannot be cancelled.\nContinue?");

	if (UserMessage(m_Alert, _T("Flash"), MB_YESNO) == IDNO)	//continue?
	{
		m_IBox.SetWindowText(_T("Please choose files to program. . ."));	//no
		return;
	}

	// disable all buttons
	DisableAllButtons();	

	CWaitCursor cursor; //????????
	m_IBox.SetWindowText(_T("Preparing to program. . ."));

	//set us to a minimal hardware mode
	m_pHardware->ForceHardwareFlashMode();

	//program the jam file
	if (m_JamFile != _T(""))
	{
		m_IBox.SetWindowText(_T("Programming System Controller JAM File. . ."));
		m_ProgressCtrl.SetPos(0);

		do
		{	//on power up, address TO_STATUS is 1 (15:0)
			m_pPcio->PCIO_Write(WD_STATUS, short(0x0));
			//without power, address TO_STATUS is 0xffff (15:0)
			m_pPcio->PCIO_Read(WD_STATUS, &data);

			if (data == 0xffff)
			{/*
				m = UserMessage(_T("System controller is not powered or is "\
						"unprogrammed.\nTo re-establish communication, please"\
						" turn on the power and press Retry.\nTo stop programming,"\
						" press Abort.\nTo continue without checking communication, "),
						_T("press Ignore.  Board Unpowered or Unprogrammed"),
						MB_ABORTRETRYIGNORE | MB_ICONERROR);*/
				TCException e = TCEXCEPTION(HW_SYSTEM_CONTROLLER);
				m = AfxMessageBox( e.getMessage(), MB_ABORTRETRYIGNORE | MB_ICONERROR);
				switch (m)
				{
					case IDABORT: 
						m_IBox.SetWindowText(_T("Programming aborted by user."));
						return;
						break;
					case IDIGNORE: 
						data = 0x0;
						break;
					case IDRETRY: 
						continue;
						break;
					default: 
						continue;
						break;
				}
			}
		} while (data == 0xffff);

		ret = ProgramSystemController(m_JamFile);
		numTasks--;

		//process return codes
		if (ret == SUCCESS)	//success
		{
			m_Alert = _T("Please reset the system controller now.");
			m_Alert += _T("\n    -Press and release the EMO.");
			m_Alert += _T("\n    -Wait one second.");
			m_Alert += _T("\n    -Press the reset button.");
			m_Alert += _T("\n    -Click OK.");

			//UserMessage(m_Alert, _T("Reset Request"), MB_OK | MB_ICONWARNING);
			TCException e = TCEXCEPTION(HW_SYSTEM_RESETCONTROLLER);
			AfxMessageBox( e.getMessage(), MB_OK | MB_ICONWARNING);
			m_IBox.SetWindowText(_T("Waiting. . ."));
			Sleep(2000);
			//address 0x13 should read 0x1 on power up.
			m_pPcio->PCIO_Read(WD_STATUS, &data);

			while ((data & 0x1) != 0x1)
			{/*
				if (UserMessage(m_Alert, _T("Reset Request"),
						MB_OKCANCEL | MB_ICONWARNING) == IDCANCEL)*/
				if(AfxMessageBox( e.getMessage(), MB_OKCANCEL | MB_ICONWARNING) == IDCANCEL)
				{
					m_IBox.SetWindowText(_T("No reset detected. . ."));
					m_pHardware->ClearHardwareFlashMode();
					return;
				}
				m_IBox.SetWindowText(_T("Waiting. . ."));
				Sleep(2000);
				m_pPcio->PCIO_Read(0x13, &data);
			}

			m_bJamStat = TRUE;
			m_bSuccess = TRUE;
			m_IBox.SetWindowText(_T("System Controller JAM File "\
										"Successfully Programmed..."));
		}
		else
		{
			switch (ret)
			{
				case INVALID_FILENAME:
				case INVALID_FILE:
					UserMessage(_T("File Error: Specified file is invalid "\
													"or does not exist"),
						_T("System Controller"), MB_OK | MB_ICONERROR);
					break;
				case PROGRAM_NOT_VERIFIED:
					UserMessage(_T("Programming Error: Unable to verify programming"),
						_T("System Controller"), MB_OK | MB_ICONERROR);
					break;
				case MEMORY_ERROR:
					UserMessage(_T("Memory Error: Unable to allocate memory"),
						_T("System Controller"), MB_OK | MB_ICONERROR);
					break;
				case DRIVER_ERROR:
					UserMessage(_T("Driver Error: Unable to access pciio driver"),
						_T("System Controller"), MB_OK | MB_ICONERROR);
					break;
				case UNRECOGNIZED_DEVICE:
					UserMessage(_T("Device Error: Unrecognized device"),
						_T("System Controller"), MB_OK | MB_ICONERROR);
					break;
				case PROGRAM_FAILURE:
					UserMessage(_T("Programming Error: Unable to program device"),
						_T("System Controller"), MB_OK | MB_ICONERROR);
					break;
				case UNKNOWN_ERROR:
					UserMessage(_T("Programming Error: Unknown programming error"),
						_T("System Controller"), MB_OK | MB_ICONERROR);
					break;
				case CRC_ERROR:
					UserMessage(_T("CRC Error: Fatal CRC error"),
						_T("System Controller"), MB_OK | MB_ICONERROR);
					break;
				default:
					if (ret < 100)
					{
						char error[40] = _T("Error in file: line ");
						char line[20];
						ret -= 100;
						_itoa_s(ret, line, 10);
						strcat_s(error, line);
						UserMessage(error, _T("System Controller"),
							MB_OK | MB_ICONERROR);
					}
					else
						UserMessage(_T("Programming Error: Unspecified  "\
															"programming error"),
						_T("System Controller"), MB_OK | MB_ICONERROR);
			}

			m_bJamStat = FALSE;
			m_bFail = TRUE;

			if (numTasks > 0)
				if (UserMessage	(_T("An error occured while programming the "\
							"system controller jam file.\n"\
							"Continue with programming tasks?"),
					_T("Error"), MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
				{
					//clear all file fields
					ClearAllWindows();
					EnableAllButtons();

					m_pHardware->ClearHardwareFlashMode();

					return;	//quit
				}

		}
	}

	//programming the prg file
	if (m_PrgFile != _T(""))
	{
		m_IBox.SetWindowText(_T("Programming Motor Controller PRG File. . ."));
		m_ProgressCtrl.SetPos(0);

		m_IBox.SetWindowText(_T("Checking file validity. . . "));

		(m_pMotors->m_iMotorType == MT_ACSCMOTOR) ?
			ret = ProgramMotorsACSPRG(m_PrgFile):
			ret = ProgramMotorsPRG(m_PrgFile);
		
		numTasks--;

		//process return codes
		if (ret == SUCCESS)	//success
		{
			m_IBox.SetWindowText(_T("Motor Controller PRG File Successfully Programmed..."));
			m_bPrgStat = TRUE;
			m_bSuccess = TRUE;
		}
		else	//we have an error
		{
			switch (ret)
			{
				case INVALID_FILENAME:
				case FILE_DOES_NOT_EXIST:
					UserMessage(_T("File Error: Specified file is invalid or does "\
							"not exist"),
						_T("Motor Controller"), MB_OK | MB_ICONERROR);
					break;
				case COMMUNICATION_ERROR:
					UserMessage(_T("Communication Error: Unable to communicate "\
							"with Motor Controller"),
						_T("Motor Controller"), MB_OK | MB_ICONERROR);
					break;
				case COMPILE_ERROR:
					UserMessage(_T("Programming Error: Unable to compile PRG file"),
						_T("Motor Controller"), MB_OK | MB_ICONERROR);
					break;
				case INVALID_FILE:
					UserMessage(_T("Programming Error: File contains no data"),
						_T("Motor Controller"), MB_OK | MB_ICONERROR);
					break;
				case SAVE_ERROR:
					UserMessage(_T("Programming Error: Unable to save to Motor "\
							"Controller"),
						_T("Motor Controller"), MB_OK | MB_ICONERROR);
					break;
				default:
					UserMessage(_T("Programming Error: Unspecified programming error"),
						_T("Motor Controller"), MB_OK | MB_ICONERROR);
					break;
			}
			
			m_bPrgStat = FALSE;
			m_bFail = TRUE;
			
			if (numTasks > 0)
				if (UserMessage(
					_T("An error occured while programming the motor "\
						"controller prg file.\nContinue with programming tasks?"),
						_T("Error"), MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
				{
					//clear all file fields
					ClearAllWindows();
					EnableAllButtons();
					m_pHardware->ClearHardwareFlashMode();

					return;	//quit
				}
		}
	}

	//programming the set file
	if (m_SetFile != _T(""))
	{
		m_IBox.SetWindowText(_T("Programming Motor Controller SET File. . ."));
		m_ProgressCtrl.SetPos(0);
		
		m_IBox.SetWindowText(_T("Checking file validity. . . "));
		ret = ProgramMotorsSET(m_SetFile);
		numTasks--;

		//process returns codes
		if (ret == SUCCESS)	//success
		{
			m_IBox.SetWindowText(_T("Motor Controller SET File Successfully Programmed..."));
			m_bSetStat = TRUE;
			m_bSuccess = TRUE;
		}
		else	//we have an error 
		{
			if (ret == INVALID_FILENAME || ret == FILE_DOES_NOT_EXIST)
				UserMessage(_T("File Error: Specified file is invalid or does not exist"),
					_T("Motor Controller"), MB_OK | MB_ICONERROR);
			else if (ret == COMMUNICATION_ERROR)
				UserMessage(_T("Communication Error: Unable to communicate with Motor Controller"),
					_T("Motor Controller"), MB_OK | MB_ICONERROR);
			else if (ret == INVALID_FILE)
				UserMessage(_T("Programming Error: File contains no data"),
					_T("Motor Controller"), MB_OK | MB_ICONERROR);
			else if (ret == SAVE_ERROR)
				UserMessage(_T("Programming Error: Unable to save to Motor Controller"),
					_T("Motor Controller"), MB_OK | MB_ICONERROR);
			else
				UserMessage(_T("Programming Error: Unspecified programming error"),
					_T("Motor Controller"), MB_OK | MB_ICONERROR);
				
			m_bSetStat = FALSE;
			m_bFail = TRUE;

			if (numTasks > 0)
				if (UserMessage(
						_T("An error occured while programming the motor controller set file.\n"\
							"Continue with programming tasks?"), _T("Error"),
							MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
				{
					//clear all file fields
					ClearAllWindows();
					EnableAllButtons();

					m_pHardware->ClearHardwareFlashMode();

					return;	//quit
				}
		}
	}

	//programming the spi file
	if (m_SpiFile != _T(""))
	{
		m_IBox.SetWindowText(_T("Programming Motor Controller SPI File. . ."));
		m_ProgressCtrl.SetPos(0);
		
		m_IBox.SetWindowText(_T("Checking file validity. . . "));
		ret = ProgramMotorsSPI(m_SpiFile);
		numTasks--;

		//process returns codes
		if (ret == SUCCESS)	//success
		{
			m_IBox.SetWindowText(_T("Motor Controller SPI File Successfully Programmed..."));
			m_bSetStat = TRUE;
			m_bSuccess = TRUE;
		}
		else	//we have an error 
		{
			if (ret == INVALID_FILENAME || ret == FILE_DOES_NOT_EXIST)
				UserMessage(_T("File Error: Specified file is invalid or does not exist"),
					_T("Motor Controller"), MB_OK | MB_ICONERROR);
			else if (ret == COMMUNICATION_ERROR)
				UserMessage(_T("Communication Error: Unable to communicate with Motor Controller"),
					_T("Motor Controller"), MB_OK | MB_ICONERROR);
			else if (ret == INVALID_FILE)
				UserMessage(_T("Programming Error: File contains no data"),
					_T("Motor Controller"), MB_OK | MB_ICONERROR);
			else if (ret == SAVE_ERROR)
				UserMessage(_T("Programming Error: Unable to save to Motor Controller"),
					_T("Motor Controller"), MB_OK | MB_ICONERROR);
			else
				UserMessage(_T("Programming Error: Unspecified programming error"),
					_T("Motor Controller"), MB_OK | MB_ICONERROR);
				
			m_bSetStat = FALSE;
			m_bFail = TRUE;

			if (numTasks > 0)
				if (UserMessage(
						_T("An error occured while programming the motor controller set file.\n"\
							"Continue with programming tasks?"), _T("Error"),
							MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
				{
					//clear all file fields
					ClearAllWindows();
					EnableAllButtons();

					m_pHardware->ClearHardwareFlashMode();

					return;	//quit
				}
		}
	}
	//programming the cfg file
	if (m_CfgFile != _T(""))
	{
		m_IBox.SetWindowText(_T("Programming Motor Controller CFG File. . ."));
		m_ProgressCtrl.SetPos(0);

		m_IBox.SetWindowText(_T("Checking file validity. . . "));
		ret = ProgramMotorsCFG(m_CfgFile);
		numTasks--;

		//process return codes
		if (ret == SUCCESS)	//success
		{
			m_IBox.SetWindowText(_T("Motor Controller PRG File Successfully Programmed..."));
			m_bPrgStat = TRUE;
			m_bSuccess = TRUE;
		}
		else	//we have an error
		{
			switch (ret)
			{
				case INVALID_FILENAME:
				case FILE_DOES_NOT_EXIST:
					UserMessage(_T("File Error: Specified file is invalid or does "\
							"not exist"),
						_T("Motor Controller"), MB_OK | MB_ICONERROR);
					break;
				case COMMUNICATION_ERROR:
					UserMessage(_T("Communication Error: Unable to communicate "\
							"with Motor Controller"),
						_T("Motor Controller"), MB_OK | MB_ICONERROR);
					break;
				case COMPILE_ERROR:
					UserMessage(_T("Programming Error: Unable to compile CFG file"),
						_T("Motor Controller"), MB_OK | MB_ICONERROR);
					break;
				case INVALID_FILE:
					UserMessage(_T("Programming Error: File contains no data"),
						_T("Motor Controller"), MB_OK | MB_ICONERROR);
					break;
				case SAVE_ERROR:
					UserMessage(_T("Programming Error: Unable to save to Motor "\
							"Controller"),
						_T("Motor Controller"), MB_OK | MB_ICONERROR);
					break;
				default:
					UserMessage(_T("Programming Error: Unspecified programming error"),
						_T("Motor Controller"), MB_OK | MB_ICONERROR);
					break;
			}
			
			m_bCfgStat = FALSE;
			m_bFail = TRUE;
			
			if (numTasks > 0)
				if (UserMessage(
					_T("An error occured while programming the motor "\
						"controller cfg file.\nContinue with programming tasks?"),
						_T("Error"), MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
				{
					//clear all file fields
					ClearAllWindows();
					EnableAllButtons();
					m_pHardware->ClearHardwareFlashMode();

					return;	//quit
				}
		}
	}

	//programming the hex file
	if (m_HexFile != _T(""))
	{
		if (m_ConveyorType == CC7_ATWC)
		{
			m_IBox.SetWindowText(_T("Programming Conveyor Controller HEX File. . ."));
			m_ProgressCtrl.SetPos(0);
			
			ret = ProgramConveyor(m_HexFile);
			numTasks--;

			//process return codes
			if (ret == SUCCESS)
			{
				m_IBox.SetWindowText(_T("Conveyor Controller HEX File Successfully "\
					"Programmed..."));
				m_bHexStat = TRUE;
				m_bSuccess = TRUE;
			}
			else 
			{
				switch (ret)
				{
					case INVALID_FILENAME:
					case FILE_DOES_NOT_EXIST:
						UserMessage(_T("File Error: Specified file is invalid or "\
							"does not exist"),
							_T("Conveyor Controller"), MB_OK | MB_ICONERROR);
						break;
					case CONVEYOR_RESET_TIMEOUT:
						UserMessage(_T("Conveyor Reset Error: Conveyor reset not "\
							"detected"),
							_T("Conveyor Controller"), MB_OK | MB_ICONERROR);
						break;
					case PROGRAM_NOT_VERIFIED:
						UserMessage(_T("Programming Error: Unable to verify "\
							"programming"),
							_T("Conveyor Controller"), MB_OK | MB_ICONERROR);
						break;
					case INVALID_FILE:
						UserMessage(_T("Programming Error: File contains no data"),
							_T("Conveyor Controller"), MB_OK | MB_ICONERROR);
						break;
					case COMPILE_ERROR:
						UserMessage(_T("Programming Error: Unable to compile HEX file"),
						_T("Conveyor Controller"), MB_OK | MB_ICONERROR);
						break;
					default:
						UserMessage(_T("Programming Error: Unspecified "\
							"programming error"),
							_T("Conveyor Controller"), MB_OK | MB_ICONERROR);
						break;
				}

				m_bHexStat = FALSE;
				m_bFail = TRUE;

				if (numTasks > 0)
				{
					if (UserMessage(
						_T("An error occured while programming the conveyor controller "\
						"hex file.\nContinue with programming tasks?"),
						_T("Error"), MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
					{
						//clear all file fields
						ClearAllWindows();
						EnableAllButtons();

						m_pHardware->ClearHardwareFlashMode();

						return;	//quit
					}
				}
			}
		}
		else	//7350 so program an Umsco conveyor
		{
			m_IBox.SetWindowText(_T("Programming Umsco Conveyor Controller TXT File. . ."));
			m_ProgressCtrl.SetPos(0);
			
			ret = ProgramConveyorIDC(m_HexFile);
			numTasks--;

			//process return codes
			if (ret == SUCCESS)	//success
			{
				m_IBox.SetWindowText(_T("Umsco Conveyor Controller TXT File Successfully Programmed..."));
				m_bHexStat = TRUE;
				m_bSuccess = TRUE;
			}
			else 
			{
				if (ret == INVALID_FILENAME || ret == FILE_DOES_NOT_EXIST)
					UserMessage(_T("File Error: Specified file is invalid or does "\
						"not exist"), _T("Umsco Conveyor Controller"),
						MB_OK | MB_ICONERROR);
				else if (ret == CONVEYOR_RESET_TIMEOUT)
					UserMessage(_T("Conveyor Reset Error: Conveyor reset not detected"),
						_T("Umsco Conveyor Controller"), MB_OK | MB_ICONERROR);
				else if (ret == PROGRAM_NOT_VERIFIED)
					UserMessage(_T("Programming Error: Unable to verify programming"),
						_T("Umsco Conveyor Controller"), MB_OK | MB_ICONERROR);
				else if (ret == INVALID_FILE)
					UserMessage(_T("Programming Error: File contains no data"),
						_T("Umsco Conveyor Controller"), MB_OK | MB_ICONERROR);
				else if (ret == COMPILE_ERROR)
					UserMessage(_T("Programming Error: Unable to compile TXT file"),
						_T("Umsco Conveyor Controller"), MB_OK | MB_ICONERROR);
				else
					UserMessage(_T("Programming Error: Unspecified programming error"),
						_T("Umsco Conveyor Controller"), MB_OK | MB_ICONERROR);

				m_bHexStat = FALSE;
				m_bFail = TRUE;

				if (numTasks > 0)
				{
					if (UserMessage(_T("An error occured while programming the "\
						"Umsco conveyor controller txt file.\nContinue with "\
						"programming tasks?"),
						_T("Error"), MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
					{
						//clear all file fields
						ClearAllWindows();
						EnableAllButtons();

						m_pHardware->ClearHardwareFlashMode();

						return;	//quit
					}
				}
			}
		}
	}
	if (m_BarFile != _T(""))
	{
		CString ScannerName;
		m_Scanners.GetLBText(m_Scanners.GetCurSel(), ScannerName);

		if (ProgramBarCodeReader(ScannerName, m_BarFile) == SUCCESS)
		{
			m_bBarStat = TRUE;
			m_bSuccess = TRUE;
		}
		else
		{
			m_bFail    = TRUE;
			m_bBarStat = FALSE;
			UserMessage(_T("Programming Error: Unable to verify "\
				"programming"),
				_T("Bar Code Scanner"), MB_OK | MB_ICONERROR);
		}

	}

	m_pHardware->ClearHardwareFlashMode();

	m_Alert = _T("");

	if (m_bSuccess)	//at least one operation successfully
	{
		m_Alert = _T("The following tasks were completed successfully:\n");

		if (m_JamFile != _T("") && m_bJamStat == TRUE)
			m_Alert += _T("    -The system controller was programmed with the specifed JAM file.\n");
		if (m_PrgFile != _T("") && m_bPrgStat == TRUE)
			m_Alert += _T("    -The motor controller was programmed with the specifed PRG file.\n");
		if (m_SetFile != _T("") && m_bSetStat == TRUE)
			m_Alert += _T("    -The motor controller was programmed with the specifed SET file.\n");
		if (m_SpiFile != _T("") && m_bSpiStat == TRUE)
			m_Alert += _T("    -The motor controller was programmed with the specifed SPI file.\n");
		if (m_HexFile != _T("") && m_bHexStat == TRUE)
			if (m_ConveyorType == CC7_ATWC)
				m_Alert += _T("    -The conveyor controller was programmed with the specifed HEX file.\n");
			else
				m_Alert += _T("    -The Umsco conveyor controller was programmed with the specifed TXT file.\n");
		if (m_BarFile != _T("") && m_bBarStat == TRUE)
			m_Alert += _T("    -The bar code scanner was programmed with the specifed BAR file.\n");
		if (m_bFail)
			m_Alert += _T("\n");
	}

	if (m_bFail)	//at least one operation failed.
	{
		m_Alert += _T("The following tasks were not completed successfully:\n");

		if (m_JamFile != _T("") && m_bJamStat == FALSE)
			m_Alert += _T("    -The system controller was not programmed with the specifed JAM file.\n");
		if (m_PrgFile != _T("") && m_bPrgStat == FALSE)
			m_Alert += _T("    -The motor controller was not programmed with the specifed PRG file.\n");
		if (m_SetFile != _T("") && m_bSetStat == FALSE)
			m_Alert += _T("    -The motor controller was not programmed with the specifed SET file.\n");
		if (m_SpiFile != _T("") && m_bSpiStat == FALSE)
			m_Alert += _T("    -The motor controller was not programmed with the specifed SPI file.\n");
		if (m_HexFile != _T("") && m_bHexStat == FALSE)
			if (m_ConveyorType == CC7_ATWC)
				m_Alert += _T("    -The conveyor controller was not programmed with the specifed HEX file.\n");
			else
				m_Alert += _T("    -The Umsco conveyor controller was not programmed with the specifed TXT file.\n");
		if (m_BarFile != _T("") && m_bBarStat == FALSE)
			m_Alert += _T("    -The bar code scanner was not programmed with the specifed BAR file.");
	}

	FillControllerRevisions();

	UserMessage(m_Alert, _T("Programming Results"), MB_OK);
	
	m_IBox.SetWindowText(_T("Done programming. . . "));

	//clear all file fields
	ClearAllWindows();
	EnableAllButtons();
};

/*
 * Function:	GetLinesInFile
 *
 * Description:	retrieve the number of lines
 *
 * Parameters:
 *		CString	 - file name
 * 
 * Return Values
 *		int      - success code
 *					SUCCESS > 0 (number of lines in file)
 *					other   <= 0
 *
 */
int
TCDFlash::GetLinesInFile(CString fname)
{
	int lines = 0;
	char buffer[256];

	fstream	file(fname, ios::in);
	if (!file.is_open())
	{ // check that file exists
		file.close();
		return FILE_DOES_NOT_EXIST;
	}

	do
	{	// get a line from file
		file.getline(buffer,255);
		lines++;
	} while (!file.eof());
	file.close();

	return lines;
};

/*
 * Function:	ProgramConveyor
 *
 * Description:	program the ATWC conveyor controller
 *
 * Parameters:
 *		CString	 - file name
 * 
 * Return Values
 *		int      - success code
 *					SUCCESS == 0
 *					other   != 0
 *
 */
int
TCDFlash::ProgramConveyor(CString fname)
{
	COMMTIMEOUTS TimeOut;
	char received[300] = {0};

	// setup conveyor flash mode
	m_pConveyor->SetFlashMode(TRUE);

	//checking file validity
	m_IBox.SetWindowText("Checking file validity. . . ");

	//progress bar setup
	m_ProgressCtrl.SetPos(0);
	int lines = GetLinesInFile(fname);
	switch (lines)
	{
		case 0:
			return INVALID_FILE;
		case FILE_DOES_NOT_EXIST:
			return FILE_DOES_NOT_EXIST;
		default:
			m_ProgressCtrl.SetRange(0, lines);
	}

	//open fname as input stream
	fstream	fin(fname, ios::in);

	//reset
	m_pConveyor->ClearSerialBufferFlash();

	// set timeouts to wait for first character
	TimeOut.ReadIntervalTimeout = 0;
	TimeOut.ReadTotalTimeoutMultiplier = 0;
	TimeOut.ReadTotalTimeoutConstant = 10000;	 
	m_pConveyor->SetComTimeoutFlash(TimeOut);					

	UserMessage(_T("Please press 'OK' and reset the conveyors now.\n"\
					"You will have 10 seconds after pressing 'OK' "\
					"to press reset."),
				"Reset Conveyors",	MB_OK | MB_ICONWARNING);
	
	m_IBox.SetWindowText("Waiting. . . ");
	
	// Wait for the first character
	if (!m_pConveyor->GetComPortFlash(received,82))
		return CONVEYOR_RESET_TIMEOUT;

	// set timeouts back to something normal
	TimeOut.ReadIntervalTimeout = 0;
	TimeOut.ReadTotalTimeoutMultiplier = 5;
	TimeOut.ReadTotalTimeoutConstant = 50;	 
	m_pConveyor->SetComTimeoutFlash(TimeOut);

	m_IBox.SetWindowText("Entering upload mode. . . ");

	// send a space character
	m_pConveyor->SendComPortFlash(" ");
	m_pConveyor->GetComPortFlash(received,198); 
	// send one
	m_pConveyor->SendComPortFlash("1");
	
	m_IBox.SetWindowText("Deleting existing firmware. . . ");

	// set timeouts to wait for ....
	TimeOut.ReadIntervalTimeout = 1000;
	TimeOut.ReadTotalTimeoutMultiplier = 0;
	TimeOut.ReadTotalTimeoutConstant = 0;	 
	m_pConveyor->SetComTimeoutFlash(TimeOut);					

	m_pConveyor->GetComPortFlash(received,46);

	// set timeouts back to something normal
	TimeOut.ReadIntervalTimeout = 0;
	TimeOut.ReadTotalTimeoutMultiplier = 5;
	TimeOut.ReadTotalTimeoutConstant = 50;	 
	m_pConveyor->SetComTimeoutFlash(TimeOut);
	
	//program conveyors
	m_IBox.SetWindowText("Programming Conveyors. . . ");

	char filebuffer[80] = {0};
	Sleep(1000);// it's very important for 515		
	do
	{
		fin.getline(filebuffer,80);
		m_pConveyor->SendComPortFlash(filebuffer);
		m_pConveyor->GetComPortFlash(received,2);
			m_ProgressCtrl.StepIt();
	} while(!fin.eof());
	
	fin.close();
	char *verify = received + 2;

	m_IBox.SetWindowText("Waiting. . . ");
	
	// set timeouts to wait for ....
	TimeOut.ReadIntervalTimeout = 1000;
	TimeOut.ReadTotalTimeoutMultiplier = 0;
	TimeOut.ReadTotalTimeoutConstant = 2000;	 
	m_pConveyor->SetComTimeoutFlash(TimeOut);					

	m_pConveyor->GetComPortFlash(verify,34);

	// set timeouts back to something normal
	TimeOut.ReadIntervalTimeout = 0;
	TimeOut.ReadTotalTimeoutMultiplier = 60;
	TimeOut.ReadTotalTimeoutConstant = 600;	 
	m_pConveyor->SetComTimeoutFlash(TimeOut);

	//check programming
	if (strstr(received, "Error") != NULL)
		return COMPILE_ERROR;

	m_IBox.SetWindowText("Checking programming. . . ");

	char check[20] = {0};
	Sleep(1000);// this delay is very important for 515
	// clear all the input characters
	m_pConveyor->ClearSerialBufferFlash();
	m_pConveyor->SendComPortFlash("\r\r\r");
	m_pConveyor->GetComPortFlash(check,20);
	
	// clear conveyor flash mode
	m_pConveyor->SetFlashMode(FALSE);

	if(check[0] == '?' || check[1] == '?' || check[2] == '?')
	{ // verify
		m_IBox.SetWindowText("Done programming conveyors. . . ");
		return SUCCESS;
	}
	else
		return PROGRAM_NOT_VERIFIED;
};

/*
 * Function:	ProgramConveyorIDC
 *
 * Description:	program the UMSCO conveyor controller
 *
 * Parameters:
 *		CString	 - file name
 * 
 * Return Values
 *		int      - success code
 *					SUCCESS == 0
 *					other   != 0
 *
 */
int
TCDFlash::ProgramConveyorIDC(CString fname)
{
	COMMTIMEOUTS TimeOut;
	char received[300] = {0};
	char filebuffer[256] = {0};

	// setup conveyor flash mode
	m_pConveyor->SetFlashMode(TRUE);

	//checking file validity
	m_IBox.SetWindowText("Checking file validity. . . ");
	
	//progress bar setup
	m_ProgressCtrl.SetPos(0);
	int lines = GetLinesInFile(fname);
	switch (lines)
	{
		case 0:
			return INVALID_FILE;
		case FILE_DOES_NOT_EXIST:
			return FILE_DOES_NOT_EXIST;
		default:
			m_ProgressCtrl.SetRange(0, lines);
	}

	//open fname as input stream
	fstream	fin(fname, ios::in);

	//Set Z-World into "cmd pass through" (ricochet) mode
	m_pConveyor->SendComPortFlash("RIC 1");

	//program conveyors
	m_IBox.SetWindowText("Programming Conveyors. . . ");

			
	do
	{
		fin.getline(filebuffer,255);
		m_pConveyor->SendComPortFlash(filebuffer);
		m_ProgressCtrl.StepIt();
	} while (!fin.eof());

	fin.close();

	//reset
	m_pConveyor->ClearSerialBufferFlash();
	//reset the IDC
	m_pConveyor->SendComPortFlash("1RS");

	//set timeouts to wait 10 seconds for first character
	TimeOut.ReadIntervalTimeout = 0;
	TimeOut.ReadTotalTimeoutMultiplier = 0;
	TimeOut.ReadTotalTimeoutConstant = 10000;	 
	m_pConveyor->SetComTimeoutFlash(TimeOut);					

	m_IBox.SetWindowText("Waiting for reset to complete. . . ");
	
	//Wait for the first character
	if (!m_pConveyor->GetComPortFlash(received, 5))
		return CONVEYOR_RESET_TIMEOUT;
	
	//return to normal Z-world operation
	m_pConveyor->SendComPortFlash(CString("0x1B"));

	Sleep(500);

	//set timeouts back to something normal
	TimeOut.ReadIntervalTimeout = 0;
	TimeOut.ReadTotalTimeoutMultiplier = 5;
	TimeOut.ReadTotalTimeoutConstant = 50;	 
	m_pConveyor->SetComTimeoutFlash(TimeOut);

	m_pConveyor->ClearSerialBufferFlash();

	// clear conveyor flash mode
	m_pConveyor->SetFlashMode(FALSE);

	return SUCCESS;
};

/*
 * Function:	ProgramMotorsPRG
 *
 * Description:	program the XYMotor controller
 *
 * Parameters:
 *		CString	 - file name
 * 
 * Return Values
 *		int      - success code
 *					SUCCESS == 0
 *					other   != 0
 *
 */
int
TCDFlash::ProgramMotorsPRG(CString fname)
{
//set flags
	char received[200];
	char fileLine[256];

	//progress bar setup
	m_ProgressCtrl.SetPos(0);
	int lines = GetLinesInFile(fname);
	switch (lines)
	{
		case 0:
			return INVALID_FILE;
		case FILE_DOES_NOT_EXIST:
			return FILE_DOES_NOT_EXIST;
		default:
			m_ProgressCtrl.SetRange(0, lines);
	}

	//open fname as input stream
	fstream prgFile(fname, ios::in);

	//check communication with d-serv
	m_IBox.SetWindowText("Checking communication with Motor Controller. . . ");
	
	//change from binary to ASCII
	m_pMotors->SendComPort("SHT1");
	m_pMotors->SendComPort("");
	//exit any mode
	m_pMotors->SendComPort("E");
	//clear any garbage
	m_pMotors->ClearSerialBuffer();
	m_pMotors->SendComPort("");
	//Wait for response
	m_pMotors->GetComPort(received,4);
	
	//valid response is "0>"
	if (strncmp(received, "0>", 2))
		return COMMUNICATION_ERROR;

	//setting up dserv
	m_IBox.SetWindowText("Setting up Motor Controller. . . ");
	m_pMotors->ClearSerialBuffer();	

	//programming
	m_pMotors->SendComPort("");
	m_pMotors->SendComPort("E");
	m_pMotors->ClearSerialBuffer();

	//clear memory
	m_pMotors->SendComPort("CLEAR");
	//Wait for response
	m_pMotors->GetComPort(received,109);
	//confirm
	m_pMotors->SendComPort("CLEAR");
	m_pMotors->ClearSerialBuffer();

	//programming mode
	m_pMotors->SendComPort("P");
	//insert mode
	m_pMotors->SendComPort("I");
	//Wait for response
	m_pMotors->GetComPort(received,16);

	//send the file
	m_IBox.SetWindowText("Programming Motor Controller. . . ");
	m_pMotors->ClearSerialBuffer();
	
	do 
	{ //start PRG file programming
		//get a line from file
		prgFile.getline(fileLine,255);
		m_pMotors->SendComPort(fileLine);
		m_pMotors->GetComPort(received, 8);
		m_ProgressCtrl.StepIt();
	} while (!prgFile.eof());

	//compile
	m_IBox.SetWindowText("Compiling. . . ");
	//exit insert mode
	m_pMotors->SendComPort("");
	m_pMotors->GetComPort(fileLine, 8);
	m_pMotors->ClearSerialBuffer();
	//compile PRG file
	m_pMotors->SendComPort("C");
		
	//set timeouts to wait for first character
	COMMTIMEOUTS TimeOut;
	TimeOut.ReadIntervalTimeout = 0;
	TimeOut.ReadTotalTimeoutMultiplier = 0;
	TimeOut.ReadTotalTimeoutConstant = 10000;	 
	m_pMotors->SetComTimeout(TimeOut);					

	char comp[4];
	m_pMotors->GetComPort(comp, 4);

	//if it didn't compile, fileLine will be "Comp...."
	//otherwise it will be a number like 1342
	int check = atoi(comp);

	if (check == 0)
	{ //0 means fileline had no numbers
		prgFile.close();
		return COMPILE_ERROR;
	}

	//set timeouts back to something normal
	TimeOut.ReadIntervalTimeout = 0;
	TimeOut.ReadTotalTimeoutMultiplier = 5;
	TimeOut.ReadTotalTimeoutConstant = 50;	 
	m_pMotors->SetComTimeout(TimeOut);

	//save the program onto the eeprom
	m_IBox.SetWindowText("Saving. . . ");
	m_pMotors->ClearSerialBuffer();
	m_pMotors->SendComPort("SAVE");
	//get SAVE warning
	m_pMotors->GetComPort(received, 152);
	//confirm
	m_pMotors->SendComPort("SAVE");
	
	m_IBox.SetWindowText("Waiting for save to complete. . . ");
	
	//set timeouts to wait for first character
	TimeOut.ReadIntervalTimeout = 0;
	TimeOut.ReadTotalTimeoutMultiplier = 0;
	TimeOut.ReadTotalTimeoutConstant = 40000;	 
	m_pMotors->SetComTimeout(TimeOut);					

	//Wait for the first character
	m_pMotors->GetComPort(received, 4);
	if (strncmp(received, "P>", 2))
	{	
		prgFile.close();
		return SAVE_ERROR;
	}

	//set timeouts back to normal
	TimeOut.ReadIntervalTimeout = 0;
	TimeOut.ReadTotalTimeoutMultiplier = TIMEOUTMULTIPLIER;
	TimeOut.ReadTotalTimeoutConstant = TIMEOUTCONSTANT;	 
	m_pMotors->SetComTimeout(TimeOut);

	//done
	prgFile.close();
	
	//exit to direct mode
	m_pMotors->SendComPort("E");

	m_IBox.SetWindowText("Done programming Motor Controller PRG file. . . ");
	return SUCCESS;
}

/*
 * Function:	ProgramMotorsACSPRG
 *
 * Description:	program the XYMotor controller
 *
 * Parameters:
 *		CString	 - file name
 * 
 * Return Values
 *		int      - success code
 *					SUCCESS == 0
 *					other   != 0
 *
 */
#include "acsc.h"

int
TCDFlash::ProgramMotorsACSPRG(CString fname)
{
//set flags
	int err = 0;
	bool hasData[10]={false};

//check the validity of the specified file
	m_IBox.SetWindowText("Checking file validity. . . ");
	
	if (!isValidFileName(fname, ".prg"))			//check that we have a PRG file
		return INVALID_FILENAME;
	
	fstream range(fname, ios::in);		//open file name as input stream to set progress bar range
		
	if (!range.is_open())				//check that file exists
	{
		range.close();
		return FILE_DOES_NOT_EXIST;
	}

//progress bar setup
	int lines = 0;
	m_ProgressCtrl.SetRange(0, 13);		//set progress bar range

	char buffer[255];
	m_ProgressCtrl.SetPos(0);
	do
	{
		CString line;
		range.getline(buffer,255);		//get a line from file
		line.Format("%s",buffer);
		if( line.GetLength() == 2 && line.Mid(0,1) == "#" )
			hasData[atoi(line.Mid(1,1).GetBuffer(1))] = true;

		lines++;						//total lines in file
	} while (!range.eof());				//check for end of file


	range.close();			//close range file

	if (lines == 0)
		return INVALID_FILE;
	
	m_ProgressCtrl.StepIt();
//check communication with acs-mc
	m_IBox.SetWindowText("Checking communication with Motor Controller. . . ");

	char SerialNumber[256];
	int Received;
	HANDLE hAcsc = this->m_pMotors->GetPortHandle();
	if(hAcsc == ACSC_INVALID)
		return COMMUNICATION_ERROR;
	if(!acsc_GetSerialNumber(hAcsc, SerialNumber, 255, &Received, NULL))
		return COMMUNICATION_ERROR;
	m_ProgressCtrl.StepIt();
//setting up dserv
	m_IBox.SetWindowText("Setting up Motor Controller. . . ");

//programming
//send the file
	m_IBox.SetWindowText("Programming Motor Controller. . . ");

	if(!acsc_LoadBuffersFromFile( hAcsc, fname.GetBuffer(fname.GetLength()), NULL ))
		return INVALID_FILE;
	m_ProgressCtrl.StepIt();
//compile
	m_IBox.SetWindowText("Compiling. . . ");

	for(int i=0; i<= 9; i++)
	{
		if(hasData[i])
			if(!acsc_CompileBuffer( hAcsc, i, NULL ))
				return COMPILE_ERROR;

		m_ProgressCtrl.StepIt();
	}

//save the program onto the eeprom
	m_IBox.SetWindowText("Saving. . . ");
	
	m_IBox.SetWindowText("Waiting for save to complete. . . ");
	
//done
	m_IBox.SetWindowText("Done programming Motor Controller PRG file. . . ");
	return SUCCESS;	

}
/*
 * Function:	ProgramMotorsSET
 *
 * Description:	program the XYMotor controller
 *
 * Parameters:
 *		CString	 - file name
 * 
 * Return Values
 *		int      - success code
 *					SUCCESS == 0
 *					other   != 0
 *
 */
int
TCDFlash::ProgramMotorsSET(CString fname)
{
	char received[200];
	char fileLine[256];

	//check file validity
	m_IBox.SetWindowText("Checking file validity. . . ");
	
	//progress bar setup
	m_ProgressCtrl.SetPos(0);
	int lines = GetLinesInFile(fname);
	switch (lines)
	{
		case 0:
			return INVALID_FILE;
		case FILE_DOES_NOT_EXIST:
			return FILE_DOES_NOT_EXIST;
		default:
			m_ProgressCtrl.SetRange(0, lines);
	}

	//open fname as input stream
	fstream setFile(fname, ios::in);

	//check communication with d-serv
	m_IBox.SetWindowText("Checking communication with Motor Controller. . . ");
	
	//change from binary to ASCII
	m_pMotors->SendComPort("SHT1");
	m_pMotors->SendComPort("");
	//exit to direct mode
	m_pMotors->SendComPort("E");
	//clear any garbage
	m_pMotors->ClearSerialBuffer();
	m_pMotors->SendComPort("");
	//Wait for response
	m_pMotors->GetComPort(received,4);
	
	//valid response is "0>"
	if (strncmp(received, "0>", 2))
		return COMMUNICATION_ERROR;

	//setting up d-serv
	m_IBox.SetWindowText("Setting up Motor Controller. . . ");
	m_pMotors->ClearSerialBuffer();
	
	//programming
	m_pMotors->SendComPort("");
	m_pMotors->SendComPort("E");
	m_pMotors->ClearSerialBuffer();

	//send the file
	m_IBox.SetWindowText("Programming SET file. . . ");
	m_pMotors->ClearSerialBuffer();

	do
	{ //get a line from file
		setFile.getline(fileLine,255);
		m_pMotors->SendComPort(fileLine);
		m_pMotors->GetComPort(received, 4);
		m_ProgressCtrl.StepIt();
	}while (!setFile.eof());

	//save the program onto the eeprom
	m_IBox.SetWindowText("Saving. . . ");
	m_pMotors->ClearSerialBuffer();
	m_pMotors->SendComPort("SAVE");
	//get SAVE warning
	m_pMotors->GetComPort(received, 148);
	//confirm
	m_pMotors->SendComPort("SAVE");
	
	m_IBox.SetWindowText("Waiting for save to complete. . . ");

	//set timeouts to wait for first character
	COMMTIMEOUTS TimeOut;
	TimeOut.ReadIntervalTimeout = 0;
	TimeOut.ReadTotalTimeoutMultiplier = 0;
	TimeOut.ReadTotalTimeoutConstant = 40000;	 
	m_pMotors->SetComTimeout(TimeOut);					

	//Wait for the first character
	m_pMotors->GetComPort(received, 4);
	if (strncmp(received, "0>", 2))
	{	
		setFile.close();
		return SAVE_ERROR;
	}

	//set timeouts back to normal
	TimeOut.ReadIntervalTimeout = 0;
	TimeOut.ReadTotalTimeoutMultiplier = TIMEOUTMULTIPLIER;
	TimeOut.ReadTotalTimeoutConstant = TIMEOUTCONSTANT;	 
	m_pMotors->SetComTimeout(TimeOut);

	//done
	setFile.close();

	m_IBox.SetWindowText("Done programming Motor Controller SET file. . . ");
	return SUCCESS;
};
/*
 * Function:	ProgramMotorsSPI
 *
 * Description:	program the XYMotor controller
 *
 * Parameters:
 *		CString	 - file name
 * 
 * Return Values
 *		int      - success code
 *					SUCCESS == 0
 *					other   != 0
 *
 */
int
TCDFlash::ProgramMotorsSPI(CString fname)
{
//flags
	int err = 0;
//check file validity
	m_IBox.SetWindowText("Checking file validity. . . ");
	if (!isValidFileName(fname, ".spi"))			//check that we have a SPI file
		return INVALID_FILENAME;
	
	fstream range(fname, ios::in);		//open file

	if (!range.is_open())									//check that file exists
	{
		range.close();
		return FILE_DOES_NOT_EXIST;
	}
	
//progress bar setup
	int lines = 0;

	char buffer[256];
	m_ProgressCtrl.SetPos(0);

	do
	{
		CString line;
		range.getline(buffer,255);		//get a line from file
		line.Format("%s", range);
		lines++;						//total lines in file
	} while (!range.eof());				//check for end of file
	
	m_ProgressCtrl.SetRange(0, lines);			//set progress bar range

	range.close();		//close range file

	if (lines == 0)
		return INVALID_FILE;

	fstream spiFile(fname, ios::in);		//open input file

//check communication with acs-mc
	m_IBox.SetWindowText("Checking communication with Motor Controller. . . ");

	HANDLE hAcsc = this->m_pMotors->GetPortHandle();
	if(hAcsc == ACSC_INVALID)
		return COMMUNICATION_ERROR;
	
	m_IBox.SetWindowText("Checking communication with Motor Controller. . . ");

	char SerialNumber[256];
	int Received;
	if(!acsc_GetSerialNumber(hAcsc, SerialNumber, 255, &Received, NULL))
		return COMMUNICATION_ERROR;

//programming
//send the file
	m_IBox.SetWindowText("Programming SPI file. . . ");

	char fileLine[256];						//for a line of the file
	int count = 0;
	int axis_index = 0;

	int order_str = 0;//System Parameter & Axes Parameters
	CString StrOrder[3] = {"Parameter", "ACSPL+","Adjusting"};

	int suborder_str = 0;//Low Level, Amplifier, Motor, Encoder, Position Loop, Current Loop
						 //Commutation Optimization & Open Loop, Description
	CString StrSubOrder[8] = {"Low Level","Amplifier","Motor","Encoder","Position Loop","Current Loop",
		"Commutation Optimization & Open Loop", "Description"};
	do
	{
		CString line, para, value, index1, index2, tmp;
		int ival=0, from1, from2;//we don't need the to1 & to2, because we fill one by one
		double rval=0.0;
		
		m_ProgressCtrl.StepIt();

		count++;
		spiFile.getline(fileLine,255);			//get a line from file
		line.Format("%s",fileLine);
		if(line.Find("//===== File: ACSPL",0) != -1)//buffer start
		{
			order_str = 2;//ignore buffer data, it will handle on prg file!
			continue;
		}

		if(line.Find("//===== File: ADJ",0) != -1)//buffer start
		{
			axis_index = atoi(line.Mid(17,1).GetBuffer(1));
			order_str = 1;//adjusting data
			continue;
		}

		switch(order_str)
		{
		case 0:
			{
				int start = 0;
				//set parameter
				if(line.Mid(0,2).Compare("//") == 0 ) continue;	//commet
				if((start = line.Find(")=",0)) == -1) continue;	//not the parameter
		
				//get parameter's value
				int end = line.Find("(",0);
				para = line.Mid(0, end);
				value = line.Mid( start + 2, line.GetLength() - start - 2);

				//get parameter's range
				tmp = line.Mid( end + 1, start - end -1 );
				if(tmp.GetLength() == 1)
				{
					from1  = atoi( tmp.GetBuffer( tmp.GetLength() ) );
					from2  = ACSC_NONE;
				}
				else
				{
					index2 = tmp.Mid( tmp.Find(",",0) + 1, 1);
					index1 = tmp.Mid( 0,1 );
					from1  = atoi( index1.GetBuffer( index1.GetLength() ) );
					from2  = atoi( index2.GetBuffer( index2.GetLength() ) );
				}
				break;
			}
		case 1:
			{
				for(int i=0; i< 8; i++)
				{
					if(line.Find(StrSubOrder[i],0) != -1)
					{
						suborder_str = i;
						break;
					}
				}

				if(suborder_str != 0) continue;
				
				int start = 0;
				if((start = line.Find("=",0)) == -1) continue;
				
				from1 = axis_index;
				from2 = ACSC_NONE;
				para  = line.Mid( 0, start );
				value = line.Mid( start + 1, line.GetLength() - start - 1);

				break;
			}
		case 2:
			continue;
		}

		if(value.Find(".",0) != -1 )
		{
			rval = atof( value.GetBuffer( value.GetLength() ) );
			err |= !acsc_WriteReal( hAcsc, ACSC_NONE, para.GetBuffer(para.GetLength()), from1, from1, from2, from2, &rval, NULL );
		}
		else
		{
			ival = atol( value.GetBuffer( value.GetLength() ) );
			err |= !acsc_WriteInteger( hAcsc, ACSC_NONE, para.GetBuffer(para.GetLength()), from1, from1, from2, from2, &ival, NULL );
		}
		
		if(err)
		{
			err = 0;
			CString msg;
			msg.Format("Set Parameter Error on line(%d)\nDo you want to terminal processing?",count);
			TCException e = TCEXCEPTION1(HW_SETPARAMETER_TERMINAL, count);
			if( AfxMessageBox(e.getMessage(), MB_YESNO|MB_ICONSTOP) == IDYES )
				break;
		}

	}while (!spiFile.eof());

//save the program onto the eeprom
	m_IBox.SetWindowText("Saving. . . ");
	err = 0;
	char* save_cmd  = "#SAVE\r";
	err |= !acsc_Command( hAcsc, save_cmd, strlen(save_cmd), ACSC_IGNORE);
	err |= !acsc_Command( hAcsc, save_cmd, strlen(save_cmd), ACSC_IGNORE);
//done
	m_IBox.SetWindowText("Waiting for save to complete. . . ");

	if (err)
	{	
		spiFile.close();
		return SAVE_ERROR;
	}

//done
	spiFile.close();										//close sba file

	m_IBox.SetWindowText("Done programming Motor Controller SPI file. . . ");
	return SUCCESS;
};

/*
 * Function:	ProgramMotorsCFG
 *
 * Description:	program the XYMotor controller
 *
 * Parameters:
 *		CString	 - file name
 * 
 * Return Values
 *		int      - success code
 *					SUCCESS == 0
 *					other   != 0
 *
 */
int
TCDFlash::ProgramMotorsCFG(CString fname)
{
	//progress bar setup
	int err = 0;

	m_ProgressCtrl.SetPos(0);
	int lines = GetLinesInFile(fname);
	switch (lines)
	{
		case 0:
			return INVALID_FILE;
		case FILE_DOES_NOT_EXIST:
			return FILE_DOES_NOT_EXIST;
		default:
			m_ProgressCtrl.SetRange(0, lines);
	}

	//open fname as input stream
	fstream cfgFile(fname, ios::in);

	//check communication with AC SERVO
	m_IBox.SetWindowText("Checking communication with Motor Controller. . . ");
	
///////////////////////////////////////////////////
	char received[200]={0};							//for a line of the file
	unsigned char cBuff[8]={0};
	m_pMotors->ClearSerialBuffer();				//clear any garbage

	COMMTIMEOUTS TimeOut;
	TimeOut.ReadIntervalTimeout = 0;			//set timeouts back to something normal
	TimeOut.ReadTotalTimeoutMultiplier = 5;
	TimeOut.ReadTotalTimeoutConstant = 50;	 
	m_pMotors->SetComTimeout(TimeOut);

	cBuff[0] = 0x03;	//command number
	cBuff[2] = 0x18;	//mode/command
	cBuff[7] = 0;
	
	m_pMotors->CFGLockDriver( 0 );
	m_pMotors->CFGLockDriver( 1 );
	//loop
	lines = 0;
	do
	{
		lines++;								//count line

		CString tmp;
		char fileLine[10];						//for a line of the file
		cfgFile.getline(fileLine,10);			//get a line from file
		tmp = fileLine;

		cBuff[1] = m_pMotors->Hextoi( tmp.Mid(0,2) );
		cBuff[3] = m_pMotors->Hextoi( tmp.Mid(2,2) );
		cBuff[4] = m_pMotors->Hextoi( tmp.Mid(4,2) );
		cBuff[5] = m_pMotors->Hextoi( tmp.Mid(6,2) );
		if( (err = m_pMotors->CFGSendCommand(cBuff, 7, received)) != 0 )
		{
			CString countline;
			countline.Format("CFG parameter Error: %s (line %d), Interrupt Operation?",fileLine, lines);
			TCException e = TCEXCEPTION2(HW_SETPARAMETER_CFG, fileLine, lines);
			if(AfxMessageBox(e.getMessage(), MB_OKCANCEL) == IDOK)
			{
				m_pMotors->CFGUnLockDriver( cBuff[1] );
				return err;
			}
		}
		
		m_ProgressCtrl.StepIt();

	} while(!cfgFile.eof());
	//end loop
///////////////////////////////////////////////////
	cfgFile.close();
	//send the file
	m_IBox.SetWindowText("Programming Motor Controller. . . ");
	//clear any garbage
	m_pMotors->ClearSerialBuffer();

	//save the program onto the eeprom
	m_IBox.SetWindowText("Saving. . . ");

	for( int i=0; i< 2; i++)
	{
		CString msg;
		msg.Format("ID#%d Motor Controller write to EEPROM. . .",i);
		m_IBox.SetWindowText( msg );
		TimeOut.ReadIntervalTimeout = 0;			//set timeouts back to something normal
		TimeOut.ReadTotalTimeoutMultiplier = 5;
		TimeOut.ReadTotalTimeoutConstant = 50;	 
		m_pMotors->SetComTimeout(TimeOut);

		if( m_pMotors->CFGSave2EEPROM( i ) != 0 )
			return SAVE_ERROR;

		TimeOut.ReadIntervalTimeout = 0;			//set timeouts to wait for first character
		TimeOut.ReadTotalTimeoutMultiplier = 0;
		TimeOut.ReadTotalTimeoutConstant = 5000;	 
		m_pMotors->SetComTimeout(TimeOut);
		m_pMotors->GetComPort(received,20);		//Wait for response
		m_pMotors->CFGUnLockDriver( i );		//unlock the pane of Motor Driver with ID 0
	}

	//set timeouts back to normal
	TimeOut.ReadIntervalTimeout = 0;
	TimeOut.ReadTotalTimeoutMultiplier = TIMEOUTMULTIPLIER;
	TimeOut.ReadTotalTimeoutConstant = TIMEOUTCONSTANT;	 
	m_pMotors->SetComTimeout(TimeOut);
	
	m_IBox.SetWindowText("Done programming Motor Controller CFG file. . . ");
	m_IBox.SetWindowText("Please Reset Motor Controller. . . ");
	return SUCCESS;
}

int
TCDFlash::ProgramSystemController(CString m_File)
{
	//setting up flags and data
	JAM_RETURN_TYPE crc_result = JAMC_SUCCESS;
	JAM_RETURN_TYPE exec_result = JAMC_SUCCESS;
	USHORT expected_crc = 0;
	USHORT actual_crc = 0;

	BOOL error = FALSE;
	long offset = 0L;
	long error_line = 0L;
	long workspace_size = 0;
	char key[33] = {0};
	char value[257] = {0};
	int exit_status = 0;
	int arg = 0;
	int exit_code = 0;
	int time_delta = 0;
	int init_count = 0;
	int* windows_nt = NULL;
	time_t start_time = 0;
	time_t end_time = 0;
	char *workspace = NULL;
	char *init_list[10];
	FILE *fp = NULL;
	struct stat sbuf;
	char *exit_string = NULL;
	char *file_buffer = NULL;
	long file_length = 0L;
	char* action = "PROGRAM";
	char fname[256] = {0};

	strcpy_s(fname, (LPCTSTR) m_File);
	init_list[0] = NULL;

	m_IBox.SetWindowText(_T("Checking file validity. . . "));
	
	if (!initialize_jtag_hardware(m_pPcio))
		return DRIVER_ERROR;
	
	if (_access(fname, 0) != 0)
		return INVALID_FILE;
	else
	{
		//get length of file
		if (stat(fname, &sbuf) == 0) file_length = sbuf.st_size;

		if (fopen_s(&fp, fname, "rb") == NULL)
			return INVALID_FILE;	//can't open file
		else
		{
			//Read entire file into a buffer
			file_buffer = (char *) malloc((size_t) file_length);

			if (file_buffer == NULL)
				return MEMORY_ERROR;
			else
			{
				if (fread(file_buffer, 1, (size_t) file_length, fp) !=
												(size_t) file_length)
					return INVALID_FILE;
			}

			fclose(fp);
		}

		if (exit_status == 0)
		{
			data_exchange(file_buffer, file_length);
			calibrate_delay();

			crc_result = jam_check_crc(file_buffer, file_length,
											&expected_crc, &actual_crc);
			char crc_string[200];

			switch (crc_result)
			{
				case JAMC_SUCCESS:
					break;

				case JAMC_CRC_ERROR:
					sprintf_s(crc_string,
						_T("CRC mismatch: expected %04X, actual %04X\nContinue?"), expected_crc, actual_crc);
					if (UserMessage(crc_string, "CRC Error", MB_OK | MB_YESNO) == IDNO)
						return SUCCESS;
					break;

				case JAMC_UNEXPECTED_END:
					sprintf_s(crc_string,
						_T("Expected CRC not found, actual CRC value = %04X\nContinue?"), actual_crc);
					if (UserMessage(crc_string, "CRC Error", MB_OK | MB_YESNO) == IDNO)
						return SUCCESS;
					break;

				default:
					sprintf_s(crc_string,
						_T("CRC function returned error code %d\n"), crc_result);
					UserMessage(crc_string, "CRC Error", MB_OK | MB_ICONERROR);
					return CRC_ERROR;
					break;
			}

			m_IBox.SetWindowText(_T("Programming System Controller JAM file. . . "));
	
			exec_result =
				jam_execute(file_buffer, file_length, workspace, workspace_size,
							action, init_list, &error_line, &exit_code, &m_ProgressCtrl/*progress*/);

			if (exec_result == JAMC_SUCCESS)
			{
				switch (exit_code)
				{
					case 0: // "Success"
						exit_code = SUCCESS;
						break;
					case 1:	// "Illegal initialization values"
						exit_code = SUCCESS;
						break;
					case 2: // "Unrecognized device"
						exit_code = UNRECOGNIZED_DEVICE;
						break;
					case 3: // "Device revision is not supported"
						exit_code = SUCCESS;
						break;
					case 4: // "Device programming failure"
						exit_code = PROGRAM_FAILURE;
						break;
					case 5: // "Device is not blank"
						exit_code = SUCCESS;
						break;
					case 6: // "Device verify failure"
						exit_code = PROGRAM_NOT_VERIFIED;
						break;
					case 7: // "SRAM configuration failure"
						exit_code = SUCCESS;
						break;
					default: // "Unknown exit code"
						exit_code = error_line + 100;
						break;
				}

			}
			else if (exec_result == JAMC_SYNTAX_ERROR)
				exit_code = error_line + 100;
			else
				exit_code = UNKNOWN_ERROR;
		}
	}

	close_jtag_hardware();
	m_ProgressCtrl.SetPos(100);

	return (exit_code);
}

/*
 * Function:	ProgramBarCodeReader
 *
 * Description:	program the XYMotor controller
 *
 * Parameters:
 *		CString	 - file name
 *		CString	 - file name
 * 
 * Return Values
 *		int      - success code
 *					SUCCESS == 0
 *					other   != 0
 *
 */
int
TCDFlash::ProgramBarCodeReader(CString ScannerName, CString fname)
{
	//check file validity
	m_IBox.SetWindowText("Checking file validity. . . ");
	
	//progress bar setup
	m_ProgressCtrl.SetPos(0);
	int lines = GetLinesInFile(fname);
	switch (lines)
	{
		case 0:
			return INVALID_FILE;
		case FILE_DOES_NOT_EXIST:
			return FILE_DOES_NOT_EXIST;
		default:
			m_ProgressCtrl.SetRange(0, lines);
			m_ProgressCtrl.SetPos(0);
	}

	//open fname as input stream
	fstream fin(fname, ios::in);

	//check communication with d-serv
	m_IBox.SetWindowText("Checking communication with Bar Code Scanner . . . ");

	//program bar code scannners
	m_IBox.SetWindowText("Programming Bar Code Scanners. . . ");

	TC_BCScanner *scanner = m_pBarCode->GetScannerPtr(ScannerName);
	do
	{
		char filebuffer[256] = {0};
		CString check, send, response, received;
		int startResponse;

		fin.getline(filebuffer, 255);
		check.Format(_T("%s"), filebuffer);
		if (check.Left(3) == _T("\\o:"))
		{
			startResponse = check.Find(_T("\\i:"));
			if (startResponse > 0)
			{
				send = check.Mid(3, startResponse - 3);
				response = check.Mid(startResponse + 3);
			}
			else
			{
				send = check.Mid(3);
				response = _T("");
			}

			received = scanner->WriteAndWaitForResponse(send,
									response == _T("") ? FALSE : TRUE);
			if (received.Mid(0, response.GetLength()) != response)
			{
				CString message;
				message.Format(_T("Scanner failed to program\n"));//Expected: %s; Received: %s"),
						//response, received.Mid(0, response.GetLength()));
				//UserMessage(message, _T("Bar Code"), MB_ICONEXCLAMATION | MB_OK);
				TCException e = TCEXCEPTION(HW_SCANNER_VERIFYPROGRAM);
				UserMessage( e.getMessage(), _T("Optima 730"), MB_ICONEXCLAMATION | MB_OK);
				return TS_ERROR;
			}
			m_ProgressCtrl.StepIt();
		}
		else if (check.Left(3) == _T("\\v:"))
		{
			m_ProgressCtrl.SetPos(lines);
			m_IBox.SetWindowText("Verifying programming. . . ");
			startResponse = check.Find(_T("\\i:"));
			send = check.Mid(3, startResponse - 3);
			response = check.Mid(startResponse + 3);
			Sleep(700);
			received = scanner->WriteAndWaitForResponse(send,
								response == _T("") ? FALSE : TRUE);
			if (received.Mid(0, response.GetLength()) != response)
			{
				//UserMessage(_T("Scanner failed to verify program"), _T("Bar Code"), MB_OK);
				TCException e = TCEXCEPTION(HW_SCANNER_VERIFYPROGRAM);
				UserMessage( e.getMessage(), _T("Optima 730"), MB_OK);
				return TS_ERROR;
			}
			break;
		}
	} while(!fin.eof());

	return SUCCESS;
}

bool TCDFlash::isValidFileName(CString name, CString nameExt)
{
	CString extension = name.Right(4);
	
	if (name == "" || extension.CompareNoCase(nameExt))						//check for empty file name
		return false;
	else
		return true;
}