/*
 * Module:		$RCSfile: TCDSti.cpp $ - diagnostic STI Lights class
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
 *				(c) COPYRIGHT 2000 - 2001, TERADYNE, INC.
 *
 * Discussion:
 *
 * Date:		January. 10, 2000
 *
 */

#include "stdafx.h"

#include "math.h"
#include "timer.h"
#include "lights.hpp"
#include "motors.hpp"
#include "lmotors.hpp"
#include "MechIO.hpp"
#include "hardwareIO/pcio.hpp"
#include "hardwareIO/hardware.hpp"
#include "camera.hpp"
#include "CDIB.H"

#include "resource.h"
#include "checkers/diagnostics.hpp"
#include "checkers/TCDRegistry.hpp"
#include "checkers/TCDiagClass.hpp"
#include "checkers/TCDLedButton.hpp"
#include "checkers/TCDSti.hpp"
#include "checkers/TCDiagSpin.hpp"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*****************************************************************************/
/*  TCDSti Class                                                             */
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
TCDSti::TCDSti(TCCheckers *Wnd, TCMechIO * sti)
	: TCDiagClass(Wnd)
{
	m_pMainWnd       = Wnd;	// main window
	m_pMechIO        = sti;	// MechIO pointer
	m_pHardware      = m_pMechIO->GetHardwarePtr();
	m_pPcio          = m_pHardware->GetPcio();
	m_SystemType     = m_pHardware->GetSystemType();
	m_HardwareMode   = m_pHardware->GetMode();
	m_ControllerType = m_pHardware->GetControllerType();
	m_DomeType       = m_pHardware->GetDomeType();

	// motors
	m_pLMotors       = m_pMechIO->GetLMotorPtr();

	m_Step           = Coarse;
	m_Camera		 = TCCamera::CreateCamera(4);
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
TCDSti::~TCDSti()
{
	while(GetWorkerThreadStatus() == TRUE)
		Sleep (200);
	
	// indicate a PASS or FAIL
	m_DiagPF = (m_StiPF == DIAG_PASSED ? DIAG_PASSED : DIAG_FAILED);
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
TCDSti::Create(void)
{
	CreateCommonButtons("Dome LEDs", FALSE);

	if (m_HardwareMode != SOFTWARE_ONLY)
	{
		// STI Lights start button
		m_Button1Caption = "Dome LEDs";
		m_StiButton.CreateButton(m_Button1Caption, CRect(90,550,180,580),
			m_pMainWnd, IDC_CHECKERS_BUTTON1);
		m_StiButton.ShowWindow(SW_SHOW);

		m_Button2Caption = "Camera 5";
		m_StiCameraButton.CreateButton(m_Button2Caption, CRect(95,87,175,117),
			m_pMainWnd, IDC_CHECKERS_BUTTON2);
		m_StiCameraButton.ShowWindow(SW_SHOW);
		m_StiCameraButton.EnableWindow(FALSE);

		m_StiMoveUpButton.Create("", BS_PUSHBUTTON | BS_OWNERDRAW,
			CRect(120,50,150,80), (CWnd *) m_pMainWnd, IDC_CHECKERS_BUTTON3);
		m_StiMoveUpButton.LoadBitmaps(IDB_CHECKERS_ARROW_UP_NP, IDB_CHECKERS_ARROW_UP_P,
			IDB_CHECKERS_ARROW_UP_NP, IDB_CHECKERS_ARROW_UP_UN);
		m_StiMoveUpButton.ShowWindow(SW_SHOW);

		m_StiMoveLeftButton.Create("", BS_PUSHBUTTON | BS_OWNERDRAW,
			CRect(60,90,85,120), (CWnd *) m_pMainWnd, IDC_CHECKERS_BUTTON4);
		m_StiMoveLeftButton.LoadBitmaps(IDB_CHECKERS_ARROW_LEFT_NP,
			IDB_CHECKERS_ARROW_LEFT_P, IDB_CHECKERS_ARROW_LEFT_NP,
			IDB_CHECKERS_ARROW_LEFT_UN);
		m_StiMoveLeftButton.ShowWindow(SW_SHOW);

		m_StiMoveDownButton.Create("", BS_PUSHBUTTON | BS_OWNERDRAW,
			CRect(120,130,150,160),	(CWnd *) m_pMainWnd, IDC_CHECKERS_BUTTON5);
		m_StiMoveDownButton.LoadBitmaps(IDB_CHECKERS_ARROW_DOWN_NP,
			IDB_CHECKERS_ARROW_DOWN_P, IDB_CHECKERS_ARROW_DOWN_NP,
			IDB_CHECKERS_ARROW_DOWN_UN);
		m_StiMoveDownButton.ShowWindow(SW_SHOW);

		m_StiMoveRightButton.Create("", BS_PUSHBUTTON | BS_OWNERDRAW,
			CRect(185,90,210,120), (CWnd *) m_pMainWnd, IDC_CHECKERS_BUTTON6);
		m_StiMoveRightButton.LoadBitmaps(IDB_CHECKERS_ARROW_RIGHT_NP,
			IDB_CHECKERS_ARROW_RIGHT_P, IDB_CHECKERS_ARROW_RIGHT_NP,
			IDB_CHECKERS_ARROW_RIGHT_UN);
		m_StiMoveRightButton.ShowWindow(SW_SHOW);

		m_StiRotateCCButton.Create("", BS_PUSHBUTTON | BS_OWNERDRAW,
			CRect(70,50,95,80), (CWnd *) m_pMainWnd, IDC_CHECKERS_BUTTON8);
		m_StiRotateCCButton.LoadBitmaps(IDB_CHECKERS_ARROW_CCW_NP,
			IDB_CHECKERS_ARROW_CCW_P, IDB_CHECKERS_ARROW_CCW_NP,
			IDB_CHECKERS_ARROW_CCW_UN);
		m_StiRotateCCButton.ShowWindow(SW_SHOW);

		m_StiRotateCButton.Create("", BS_PUSHBUTTON | BS_OWNERDRAW,
			CRect(175,50,200,80), (CWnd *) m_pMainWnd, IDC_CHECKERS_BUTTON9);
		m_StiRotateCButton.LoadBitmaps(IDB_CHECKERS_ARROW_CW_NP,
			IDB_CHECKERS_ARROW_CW_P, IDB_CHECKERS_ARROW_CW_NP,
			IDB_CHECKERS_ARROW_CW_UN);
		m_StiRotateCButton.ShowWindow(SW_SHOW);

	//debug button
	//	m_StiNextButton.CreateButton("smile", CRect(210,550,310,580),
	//	m_pMainWnd, IDCCHECKERS_BUTTON7);
	//	m_StiNextButton.ShowWindow(SW_SHOW);

		m_CamMoveBox.Create("Camera Head Control", WS_CHILD | WS_VISIBLE,
			CRect(65,25,215,45), (CWnd *) m_pMainWnd, IDC_STATIC);
		m_CamMoveBox.SetFont(&m_dlgFont);
		m_CamMoveBox.ShowWindow(SW_SHOW);

		// slider
		m_Fine.Create("Fine", WS_CHILD | WS_VISIBLE, CRect(35,162,149,188),
			(CWnd *) m_pMainWnd, IDC_STATIC);
		m_Fine.SetFont(&m_dlgFont);
		m_Fine.ShowWindow(SW_SHOW);

		m_Course.Create("Course", WS_CHILD | WS_VISIBLE, CRect(190,162,235,178),
			(CWnd *) m_pMainWnd, IDC_STATIC);
		m_Course.SetFont(&m_dlgFont);
		m_Course.ShowWindow(SW_SHOW);

		m_Slider.Create(TBS_HORZ, CRect(35,180, 235,190),
			(CWnd *) m_pMainWnd, IDC_CHECKERS_SPIN1);
		m_Slider.SetRange(0, 3);
		m_Slider.SetPos(4 - m_Step);
		m_Slider.ShowWindow(SW_SHOW);

		// fast/slow test
		m_SlowButton.CreateButton("Run Test Slower", CRect(70,480, 240,500),
			m_pMainWnd, NULL);
		m_SlowButton.ShowWindow(SW_SHOW);
		::SendMessage(m_SlowButton.m_hWnd, BM_SETCHECK, 0, 0);

		m_MoveBox.SetRect(20, 20, 250,200);
		m_LEDBox. SetRect(20,210, 250,460);

		UINT i, x, y;
		UINT X      = ((m_LEDBox.right  - m_LEDBox.left) / 2) + m_LEDBox.left;
		UINT Y      = ((m_LEDBox.bottom - m_LEDBox.top ) / 2) + m_LEDBox.top;
		UINT Radius = ((m_LEDBox.right  - m_LEDBox.left) / 2) - XBITMAP;

		// columns
		for (i = 0; i < MAX_NUM_LEDCOLS; i++)
		{
			x = (UINT) (X + Radius * cos(PI / (MAX_NUM_LEDCOLS / 2) * i - 0.75 * PI));
			y = (UINT) (Y - Radius * sin(PI / (MAX_NUM_LEDCOLS / 2) * i - 0.75 * PI));
			m_LedColButtons[i].CreateButton(CRect(x - XBITMAP/2, y - YBITMAP, x, y),
				(CWnd *) m_pMainWnd);
			m_LedColButtons[i].SetOnOff(ON);
			m_LedColButtons[i].EnableWindow(TRUE);
		}

		// rows
		int numRows = (m_DomeType == STANDARD) ? NUM_LEDROWS_STD : NUM_LEDROWS_HC1;
		for (i = 0; i < numRows; i++)
		{
			x = X + ((i % 2) ? 5 : -5);
			y = m_LEDBox.top + 25 + (i * ((XBITMAP / 2) + 4));
			m_LedRowButtons[i].CreateButton(CRect(x - 5, y, x + 5, y + 5), (CWnd *) m_pMainWnd);
			if (i == 1)
				m_LedRowButtons[i].SetOnOff(ON);
			else 
				m_LedRowButtons[i].SetOnOff(OFF);
			m_LedRowButtons[i].EnableWindow(TRUE);
		}

		// camera
		m_Camera->InitCamera (4);	// camera 5

		// picture box
		DWORD dwStyle = AFX_WS_DEFAULT_VIEW;
		dwStyle &= ~WS_BORDER;
		
		// initilaize the dib from a file
		CString Filename;
		char modfile[256];

		HINSTANCE inst = GetModuleHandle( NULL );

		if( GetModuleFileName(inst, modfile, 256) == 0 )
			return;
		Filename = modfile;
		Filename = Filename.Left( Filename.ReverseFind( '\\') + 1 ) + "initdib.bmp";
		
		CFile dummyFile;
		if( !dummyFile.Open( Filename, CFile::modeRead ) )
		{
			//AfxMessageBox("Need the initalizing picture file" );
			TCException e = TCEXCEPTION(HW_INIT_PICFILE);
			AfxMessageBox  ( e.getMessage(), MB_OK | MB_ICONEXCLAMATION );
			return;
		}

		//load dummy picture
		m_Picture.Read( &dummyFile );

		for(i = 0; i < NCAM_XPIXELS * NCAM_YPIXELS; i++ )
			m_Picture.m_lpImage[i] = (char) 0;

		// move the camera head to the reflective balls (default position)
		X          = (UINT) m_Registry->GetStiPositionX(116000);
		Y          = (UINT) m_Registry->GetStiPositionY(218000);
		m_Rotation = (UINT) m_Registry->GetStiRotation (220); // 250
		if (m_Rotation > 300 || m_Rotation < 100)
			m_Rotation = 220;

		if ((X < m_pLMotors->GetXUpperLimit()) && (Y < m_pLMotors->GetYUpperLimit()))
		{
			m_pLMotors->MoveXYAbsolute(X, Y);
			m_pLMotors->IsMoveComplete();
		}

		AlignmentSnap();
	}
	else
	{
		// instructions
		m_IBox.Create("", m_IRect, (CWnd *) m_pMainWnd, IDC_STATIC);
	}
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
TCDSti::Close(int mode)
{
	// signal the work thread to 'cancel'
	m_Continue = FALSE;

	int X = 0, Y = 0;
	try
	{ // read the motor encoders
		m_pLMotors->GetAxisPosition(X, Y);
		m_Registry->SetStiPositionX(X);
		m_Registry->SetStiPositionY(Y);
		m_pLMotors->MoveXYAbsolute(0, 0);
		m_pLMotors->IsMoveComplete();
	}
	catch (TCException& e)
	{
		int i = (int) &e;
		TRACE("error reading motor position");
	}

	m_Registry->SetStiRotation (m_Rotation);
	DomeReset();

	TCDiagClass::Close(MANUAL);
}

/*
 * Function:	Draw
 *
 * Description:	draw the "STI Lights" items
 *
 * Parameters:
 *		void
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *
 *				Upon entry, draw the control items necessary to perform
 *				the STI Lights diagnostics
 */
void
TCDSti::Draw(void) 
{
	if (m_HardwareMode == SOFTWARE_ONLY)
	{
		m_IBox.SetWindowText("System Alarm tests are unavailable in "\
			"SOFTWARE ONLY mode.");
	}
	else
	{
		CClientDC dc((CWnd *)m_pMainWnd);
//		m_pMainWnd->OnPrepareDC(&dc);

		CFont *oldFont = dc.SelectObject(&m_dlgFont);
		int oldBkMode  = dc.SetBkMode(TRANSPARENT);

		dc.DrawEdge(m_MoveBox, EDGE_ETCHED, BF_RECT);
		dc.DrawEdge(m_LEDBox,  EDGE_ETCHED, BF_RECT);

		// picture border
		dc.DrawEdge(CRect(298,18, 302+NCAM_XPIXELS, 22+NCAM_YPIXELS),  EDGE_SUNKEN, BF_RECT);
		dc.DrawEdge(CRect(294,14, 306+NCAM_XPIXELS, 26+NCAM_YPIXELS),  EDGE_RAISED, BF_RECT);

		// display the snapshot
		m_Picture.Draw(&dc, CPoint(300,20), m_Picture.GetDimensions());

		// draw alignment circles
		DrawCircles();
	}
}

// values were plotted by look at a single dome, may not be correct
int rowvalueSTD[11] = {194, 180, 167, 154, 140, 127, 112, 97, 81, 65, 51};
//int rowvalueHC1[12] = {165, 150, 145, 139, 131, 123, 115, 100, 90, 80, 60, 38};
int rowvalueHC1[12] = {162, 157, 151, 145, 138, 128, 119, 109, 90, 80, 60, 38};

/*
 * Function:	DrawCircle
 *
 * Description:	draw a LED circle at position(x, y)
 *
 * Parameters:
 *		int   xPos - x coordinate
 *		int   yPos - y coordinate
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *
 *				draw a circle at position(x, y).  put an
 *				alignment cross in it.
 */
void
TCDSti::DrawCircle(int xPos, int yPos) 
{
	CClientDC dc((CWnd *)m_pMainWnd);
//	m_pMainWnd->OnPrepareDC(&dc);

	// offsets x,y (center of display)
	int x = (NCAM_XPIXELS/2) + 300 + xPos;
	int y = (NCAM_YPIXELS/2) +  20 + yPos;
	
	// alignment circles
	CPen newPen(PS_SOLID, 1, RGB(000,240,0000));
	CPen *oldPen = dc.SelectObject(&newPen);
	CBrush *oldBrush = (CBrush *) dc.SelectStockObject(HOLLOW_BRUSH);

	int offset = 12;
	dc.Ellipse(CRect(x - offset, y - offset, x + offset, y + offset));

	dc.MoveTo(x-offset, y);
	dc.LineTo(x+offset, y);
	dc.MoveTo(x, y-offset);
	dc.LineTo(x, y+offset);

	dc.SelectObject(oldBrush);
	dc.SelectObject(oldPen);
}

/*
 * Function:	DrawCircles
 *
 * Description:	draw the alignment circles
 *
 * Parameters:
 *		none
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *
 *				1. draw a circle at the size of the radius of row1.
 *				2. draw alignment cross in the entire camera view.
 *				3. draw alignment circles for rotation alignment.
 */
void
TCDSti::DrawCircles(void) 
{
	CClientDC dc((CWnd *)m_pMainWnd);
//	m_pMainWnd->OnPrepareDC(&dc);

	CPen newPen(PS_SOLID, 1, RGB(240,000,0000));
	CPen *oldPen = dc.SelectObject(&newPen);

	CPoint center((NCAM_XPIXELS/2) + 300, (NCAM_YPIXELS/2) +  20);
	int x, y;

	// alignment cross
	dc.MoveTo(                   300, (NCAM_YPIXELS/2) + 20);
	dc.LineTo((NCAM_XPIXELS)   + 300, (NCAM_YPIXELS/2) + 20);
	dc.MoveTo((NCAM_XPIXELS/2) + 300,                    20);
	dc.LineTo((NCAM_XPIXELS/2) + 300,  NCAM_YPIXELS    + 20);
	CBrush *oldBrush = (CBrush *) dc.SelectStockObject(HOLLOW_BRUSH);

	// main alighment circle, using Row #1
	if (m_DomeType == STANDARD)
	{
		dc.Ellipse(center.x - rowvalueSTD[1],  center.y - rowvalueSTD[1],
					center.x + rowvalueSTD[1],  center.y + rowvalueSTD[1]);
	}
	else
	{ // HIGHCLEARANCE1
		dc.Ellipse(center.x - rowvalueHC1[1],  center.y - rowvalueHC1[1],
					center.x + rowvalueHC1[1],  center.y + rowvalueHC1[1]);
	}

	// alignment circles
	x = 0, y = 0;
	PositionLED(x, y, 1,  0);
	DrawCircle(x, y);
	PositionLED(x, y, 1, 10);
	DrawCircle(x, y);
	PositionLED(x, y, 1, 20);
	DrawCircle(x, y);
	PositionLED(x, y, 1, 30);
	DrawCircle(x, y);

	dc.SelectObject(oldBrush);
	dc.SelectObject(oldPen);
}

/*
 * Function:	ClearLightingModeAll
 *
 * Description:	clear all the 'lighting modes'
 *
 * Parameters:
 *		void
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *
 *				store zero's in each of the 'lighting mode' variables
 */
void
TCDSti::ClearLightingModeAll(void)
{
	for (int i = 0; i < 8; i++)
		m_LightingMode[i] = 0;
}

/*
 * Function:	ClearLightingModeRow
 *
 * Description:	clear the 'row' bit
 *
 * Parameters:
 *		void
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *
 *				store zero's in the 'lighting mode' row bit
 *				in the 'lighting mode' variable
 */
void
TCDSti::ClearLightingModeRow(int row)
{
	int light = (row > 31) ? 31 : row;

	if (light < 8)
		m_LightingMode[2] &= ~(0x1 << (light + 8));
	else
		m_LightingMode[3] &= ~(0x1 << (light - 8));
}

/*
 * Function:	ClearLightingModeRow
 *
 * Description:	clear the 'column' bit
 *
 * Parameters:
 *		void
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *
 *				store zero's in the 'lighting mode' column bit
 *				in the 'lighting mode' variable
 */
void
TCDSti::ClearLightingModeCol(int column)
{
	int light = (column > 63) ? 63 : column;
	m_LightingMode[light / 16] &= ~(0x1 << (light % 16));
}

/*
 * Function:	ClearLightingMode
 *
 * Description:	clear the 'column' & the 'row' bit
 *
 * Parameters:
 *		void
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *
 *				store zero's in the 'lighting mode' column
 *				and row bit	in the 'lighting mode' variable
 */
void
TCDSti::ClearLightingMode(int row, int column)
{
	ClearLightingModeCol(column);
	ClearLightingModeRow(row);
}

/*
 * Function:	SetLightingModeRow
 *
 * Description:	set the 'row' bit
 *
 * Parameters:
 *		void
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *
 *				set to '1', 'row' bit in the 'lighting mode'
 */
void
TCDSti::SetLightingModeRow(int row)
{
	int light = (row > 31) ? 31 : row;

	if (m_ControllerType == C51134)
	{
		if (light < 8)
			m_LightingMode[2] |= (0x1 << (light + 8));
		else
			m_LightingMode[3] |= (0x1 << (light - 8));
	}
	else
		m_LightingMode[(light / 16) + 4] |= (0x1 << (light % 16));
}

/*
 * Function:	SetLightingModeCol
 *
 * Description:	set the 'column' bit
 *
 * Parameters:
 *		void
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *
 *				set to '1', 'column' bit in the 'lighting mode'
 */
void
TCDSti::SetLightingModeCol(int column)
{
	int light = (column >63) ? 63 : column;
	m_LightingMode[light / 16] |= (0x1 << (light % 16));
}

/*
 * Function:	ClearLightingMode
 *
 * Description:	set the 'column' & the 'row' bit
 *
 * Parameters:
 *		void
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *
 *				set to '1', the 'lighting mode' column
 *				and row bit	in the 'lighting mode' variables
 */
void
TCDSti::SetLightingMode(int row, int column)
{
	SetLightingModeCol(column);
	SetLightingModeRow(row);
}

/*
 * Function:	FlashLights
 *
 * Description:	strobe the dome lights
 *
 * Parameters:
 *		void
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *
 *				set the 'begin' & 'end' sequence registers and 'RUN'
 */
void
TCDSti::FlashLights()
{
	if (m_ControllerType == C51134)
	{
		m_pPcio->Dome_ioWrite(BEGIN_SEQUENCE_134, 0x1ff);
		m_pPcio->Dome_ioWrite(END_SEQUENCE_134,   0x200);
		m_pPcio->Dome_ioWrite(RUN_134,            RUN);
	}
	else
	{
		m_pPcio->Dome_ioWrite(BEGIN_SEQUENCE_140, 0x1ff);
		m_pPcio->Dome_ioWrite(END_SEQUENCE_140,   0x200);
		m_pPcio->Dome_ioWrite(DIAG_RUN_140,       RUN);
	}
}

/*
 * Function:	WriteLightModeDefn
 *
 * Description:	store the 'lighting mode'
 *
 * Parameters:
 *		void
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *
 *				store the 'lighting mode' variables into dome memory
 */
void
TCDSti::WriteLightModeDefn(void)
{
	if (m_ControllerType == C51134)
		m_pPcio->Dome_ioWrite(DIAG_LIGHTDEFADDR_134, (USHORT *) &m_LightingMode[0], 4);
	else
		m_pPcio->Dome_ioWrite(DIAG_LIGHTDEFADDR_140, (USHORT *)&m_LightingMode[0], 8);
}

/*
 * Function:	WriteStrobeDefn
 *
 * Description:	store the 'strobe definition'
 *
 * Parameters:
 *		void
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *
 *				store the 'strobe definition' variables into dome memory
 */
void
TCDSti::WriteStrobeDefn(int LEDIntensity)
{
	SHORT def[4] = {(SHORT) 0xffff, // {15:8]lighting mode definition 255,
									// [4] camera5, [3] camera4, etc
							0x6000, // [14] strobe dome, [13] time trigger, [9:0] stobe time
							0x0000, // x strobe position lsb (time count)
							0x0000  // x strobe position msb
	};

	// add LED strobe time
	def[1] = (LEDIntensity & 0x3ff) | 0x6000; // strobe time

	if (m_ControllerType == C51134)
		m_pPcio->Dome_ioWrite(DIAG_STROBEDEFADDR_134, (USHORT *) &def[0], 4);
	else // C51140
		m_pPcio->Dome_ioWrite(DIAG_STROBEDEFADDR_140, (USHORT *) &def[0], 4);
}
 
void
TCDSti::DomeReset(void)
{
	m_pPcio->Dome_ioWrite(DIAG_RUN_140,           STOP);
	m_pPcio->Dome_ioWrite(DIAG_CAMDELAY_140,     (USHORT)  0);
	m_pPcio->Dome_ioWrite(DIAG_LIGHTDELAY_140,   (USHORT) 60); 
	m_pPcio->Dome_ioWrite(DIAG_CAMERAONTIME_140, (USHORT) 16);
}

/*
 * Function:	AlignmentSnap
 *
 * Description:	setup and take a 'snapshots'
 *
 * Parameters:
 *		void
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *
 *				get Led 'OnOff' state from TCDLedButton, write the definitions
 *				into dome memory and flash the LED's
 */
void
TCDSti::AlignmentSnap(void)
{
	DomeReset();

	GetLeds();
	SnapShot(70);
	DrawCircles();
}

/*
 * Function:	GetLeds
 *
 * Description:	get LED 'OnOff' state
 *
 * Parameters:
 *		void
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *
 *				get Led 'OnOff' state from TCDLedButton
 */
void
TCDSti::GetLeds(void)
{
	int i;

	ClearLightingModeAll();
	for(i = 0; i < MAX_NUM_LEDCOLS; i++)
	{
		if (m_LedColButtons[i].GetOnOff())
			SetLightingModeCol(i);
		else
			ClearLightingModeCol(i);
	}

	// rows
	int numRows = (m_DomeType == STANDARD) ? NUM_LEDROWS_STD : NUM_LEDROWS_HC1;
	for(i = 0; i < numRows; i++)
	{
		if (m_LedRowButtons[i].GetOnOff())
			SetLightingModeRow(i);
		else
			ClearLightingModeRow(i);
	}
}

/*
 * Function:	EnableLedButtons
 *
 * Description:	enable/disable LED buttons
 *
 * Parameters:
 *		BOOL	enable
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *
 *				enable or disable the LED buttons as per the input
 *				parameter 'enable'
 */
void
TCDSti::EnableLedButtons(BOOL enable)
{
	int i;

	// columns
	for (i = 0; i < MAX_NUM_LEDCOLS; i++)
		m_LedColButtons[i].EnableWindow(enable);

	// rows
	int numRows = (m_DomeType == STANDARD) ? NUM_LEDROWS_STD : NUM_LEDROWS_HC1;
	for (i = 0; i < numRows; i++)
		m_LedRowButtons[i].EnableWindow(enable);
}

void
TCDSti::LedSmile(void)
{
	if (m_ControllerType == C51134)
		m_pPcio->Dome_ioWrite(DIAG_LIGHTDELAY_134,   (SHORT) 1);
	else
		m_pPcio->Dome_ioWrite(DIAG_LIGHTDELAY_140,   (SHORT) 1);

	ClearLightingModeAll();
	SetLightingModeCol(1);
	SetLightingModeRow(1);
	SnapShot(150);
	SetLightingModeCol(9);
	SetLightingModeRow(9);
	FlashLights();

	DomeReset();
}

/*
 * Function:	PositionLED_STD9
 *
 * Description:	compute the relative position of STANDARD dome, row 9
 *
 * Parameters:
 *		int   &x  - the computed value in the x axis
 *		int   &y  - the computed value in the y axis
 *		int   led - odd or even column
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *
 *				row 9 on the STANDARD dome has 2 raial positions.  in the
 *				rowvalueSTD array, array[9] is the outer position and
 *				array[10] is the inner position foe this top most dome row.
 */
void
TCDSti::PositionLED_STD9(int &x, int &y, int column)
{ // top row
	int radius;
	double spacing  = SRADIAN / (double) MAX_NUM_LEDCOLS;

	double rotation;
	if (column % 2 != 0)
	{
		radius = rowvalueSTD[9];
		rotation = PI * ((double) (m_Rotation - 16) * 0.001);
	}
	else
	{
		radius = rowvalueSTD[10];
		rotation = PI * ((double) (m_Rotation + 16) * 0.001);
	}

	x = (int) (radius * sin(spacing * column - rotation));
	y = (int) (radius * cos(spacing * column - rotation));
}

/*
 * Function:	PositionLED_STD0
 *
 * Description:	compute the relative position of STANDARD dome, row 0
 *
 * Parameters:
 *		int   &x  - the computed value in the x axis
 *		int   &y  - the computed value in the y axis
 *		int   led - odd or even column
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *
 *				row 0 on the STANDARD dome has 2 LEDs per column
 */
void
TCDSti::PositionLED_STD0(int &x, int &y, int column, int led)
{ // bottom row, two LEDs
	int radius = (m_DomeType == STANDARD) ? rowvalueSTD[0] : rowvalueHC1[0];
	double spacing  = SRADIAN / (double) MAX_NUM_LEDCOLS;
	// minor rotation adjustment & the position of LED 0 (Column 0, Row 0)
	double rotation;
	if (led == 0)
		rotation = PI * ((double) (m_Rotation - 14) * 0.001);
	else
		rotation = PI * ((double) (m_Rotation + 14) * 0.001);

	x = (int) (radius * sin(spacing * column - rotation));
	y = (int) (radius * cos(spacing * column - rotation));
}

/*
 * Function:	PositionLED
 *
 * Description:	compute the relative position of the LED on either dome
 *
 * Parameters:
 *		int   &x  - the computed value in the x axis
 *		int   &y  - the computed value in the y axis
 *		int   led - odd or even column
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *
 *
 */
void
TCDSti::PositionLED(int &x, int &y, int row, int column)
{
	int radius = (m_DomeType == STANDARD) ? rowvalueSTD[row] : rowvalueHC1[row];

	double spacing  = SRADIAN / (double) MAX_NUM_LEDCOLS;
	// minor rotation adjustment & the position of LED 0 (Column 0, Row 0)
	double rotation = PI * ((double) m_Rotation * 0.001);

	x = (int) (radius * sin(spacing * column - rotation));
	y = (int) (radius * cos(spacing * column - rotation));
}

/*
 * Function:	GetLEDValue
 *
 * Description:	return the pixel value at position(x, y)
 *
 * Parameters:
 *		int   x  - x coordinate
 *		int   y  - y coordinate
 * 
 * Return Values
 *		int      - value of the pixel(s)
 *
 * Discussion:
 *
 *				using the provided x & y coordinates, get the surrounding
 *				pixel values, return the average.
 */
int
TCDSti::GetLEDValue(int x, int y)
{
	int  X   = NCAM_XPIXELS/2 + x;
	int  Y   = NCAM_YPIXELS/2 - y;
	UINT led = 0;

	led += (UINT) m_Picture.m_lpImage[((Y-1) * NCAM_XPIXELS) + (X-1)];
	led += (UINT) m_Picture.m_lpImage[((Y-1) * NCAM_XPIXELS) + (X)];
	led += (UINT) m_Picture.m_lpImage[((Y-1) * NCAM_XPIXELS) + (X+1)];
	led += (UINT) m_Picture.m_lpImage[((Y)   * NCAM_XPIXELS) + (X-1)];
	led += (UINT) m_Picture.m_lpImage[((Y)   * NCAM_XPIXELS) + (X)];
	led += (UINT) m_Picture.m_lpImage[((Y)   * NCAM_XPIXELS) + (X+1)];
	led += (UINT) m_Picture.m_lpImage[((Y+1) * NCAM_XPIXELS) + (X-1)];
	led += (UINT) m_Picture.m_lpImage[((Y+1) * NCAM_XPIXELS) + (X)];
	led += (UINT) m_Picture.m_lpImage[((Y+1) * NCAM_XPIXELS) + (X+1)];

	return (int) (led / 9);
}

/*
 * Function:	TestIt
 *
 * Description:	test the LED at dome position(row, column)
 *
 * Parameters:
 *		int   row
 *		int   column
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *
 *				create the lighting mode for this LED.  snap &
 *				display the camera picture.
 */
void
TCDSti::TestIt(int row, int column)
{
	ClearLightingModeAll();

	SetLightingModeRow(row);
//	if (column >= 42 && column <= 47) //F404 vc6版中已刪除
//	{
//		for (int col = 42; col < 47; col++)
//			SetLightingModeCol(col);
//	}
//	else
		SetLightingModeCol(column);

	SnapShot(200, 70);
}

/*
 * Function:	TestLedHC1
 *
 * Description:	test the LEDx on the High Clearence Dome #1
 *
 * Parameters:
 *		int   row
 *		int   column
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *
 *				1. determine the LED row & colume
 *				2. test it. (displays the camera picture too)
 *				3. draw a circle where we thing the LED is.
 *				4. measure the pixel value.
 *				5. if below minimum value, indicate LED failed at ROW, COLUMN
 */
void
TCDSti::TestLedHC1(int row, int column)
{
	int x = 0, y = 0, led;
	CString msg;

	switch (row)
	{
		case 8:  // ????
		case 9:
		case 10:
		case 11:
			if (column ==  3 || column ==  4 || column ==  5 || column ==  6 ||
				column == 13 || column == 14 || column == 15 || column == 16 ||
				column == 23 || column == 24 || column == 25 || column == 26 ||
				column == 33 || column == 34 || column == 35 || column == 36)
					break;
		default:
			TestIt(row, column);
			PositionLED(x, y, row, column);
			DrawCircle(x, y);
	}
	// test pixel results for LED
	if ((led = GetLEDValue(x, y)) < MIN_LEDVALUE)
	{
		msg.Format("Row %d Column %d failed at %d", row, column, led);
		DiagOutput(msg);
		m_DomePF     = DIAG_FAILED;
	}
	if (::SendMessage(m_SlowButton.m_hWnd, BM_GETCHECK, 0, 0) == TRUE)
		Sleep(200);
}

/*
 * Function:	TestLedSTD
 *
 * Description:	test the LEDx on the Standard Dome
 *
 * Parameters:
 *		int   row
 *		int   column
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *
 *				1. determine the LED row & colume
 *				2. test it. (displays the camera picture too)
 *				3. draw a circle where we thing the LED is.
 *				4. measure the pixel value.
 *				5. if below minimum value, indicate LED failed at ROW, COLUMN
 */
void
TCDSti::TestLedSTD(int row, int column)
{
	/*	pixel coordinates with in camera view
		origin ((0, 0) is the center of view.

		         |
          -X, -Y | +X,-Y	
                 |
        --------0+0--------
                 |
          -X, +Y | +X,+Y	
                 |
	*/

	int x = 0, y = 0, led;
	CString msg;

	switch (row)
	{
		case 0: //row 0 has two LEDs
			PositionLED_STD0(x, y, column, /* led */0); // x & y values are from here
			DrawCircle(x, y);                         // draw target

			// test pixel results for LED 0
			if ((led = GetLEDValue(x, y)) < MIN_LEDVALUE)
			{
				msg.Format("Row 0A, Column %d failed at %d", column, led);
				DiagOutput(msg);
				m_DomePF     = DIAG_FAILED;
			}

			TestIt(row, column);
			PositionLED_STD0(x, y, column, /* led */1);
			DrawCircle(x, y);

			// test pixel results for LED 1
			if ((led = GetLEDValue(x, y)) < MIN_LEDVALUE)
			{
				msg.Format("Row 0B, Column %d failed at %d", column, led);
				DiagOutput(msg);
				m_DomePF     = DIAG_FAILED;
			}
			break;

		case 9:
			if (column ==  3 || column ==  4 || column ==  5 || column ==  6 ||
				column == 13 || column == 14 || column == 15 || column == 16 ||
				column == 23 || column == 24 || column == 25 || column == 26 ||
				column == 33 || column == 34 || column == 35 || column == 36)
					break;

			TestIt(row, column);
			PositionLED_STD9(x, y, column);
			DrawCircle(x, y);

			// test pixel results for LED
			if ((led = GetLEDValue(x, y)) < MIN_LEDVALUE)
			{
				msg.Format("Row 9, Column %d failed at %d", column, led);
				DiagOutput(msg);
				m_DomePF     = DIAG_FAILED;
			}
			break;

		case 21:
		case 22:
		case 23:
				TestIt(row, column);
		break;

		case 7:
		case 8:
			if (column ==  3 || column ==  4 || column ==  5 || column ==  6 ||
				column == 13 || column == 14 || column == 15 || column == 16 ||
				column == 23 || column == 24 || column == 25 || column == 26 ||
				column == 33 || column == 34 || column == 35 || column == 36)
					break;
		default:

			TestIt(row, column);
			PositionLED(x, y, row, column);
			DrawCircle(x, y);
			// test pixel results for LED
			if ((led = GetLEDValue(x, y)) < MIN_LEDVALUE)
			{
				msg.Format("Row %d Column %d failed at %d", row, column, led);
				DiagOutput(msg);
				m_DomePF     = DIAG_FAILED;
			}
			break;
	}
	if (::SendMessage(m_SlowButton.m_hWnd, BM_GETCHECK, 0, 0) == TRUE)
		Sleep(200);

}

/*
 * Function:	TestSplitter
 *
 * Description:	test the beam splitter
 *
 * Parameters:
 *		none
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *				 
 */
void
TCDSti::TestSplitter(void)
{
	int column, row, domecolumns, domerows;

	DomeReset();

	if (m_SystemType == C51134)
	{

	}
	else
	{  // as handbuilt by Royster
		domecolumns = 47;
		domerows    = 22;
		// rows 22 & 23
		for (row = 21; row <= domerows; row++)
		{
			// columns 41 thru 46
			for (column = 42; column <= domecolumns; column++)
			{
				if (m_Continue == FALSE)
					break;
				TestLedSTD(row, column);
				SpectrumSleep(500);  // prevent dutycycle shutdown
			}
			if (m_Continue == FALSE)
				break;
		}
			TestLedSTD(23, 41); // center LED
			Sleep(10);
	}

	CString diag = "Dome Splitter LED Tests: Completed: ";
	DiagReport(diag, m_SplitterPF);
}

/*
 * Function:	TestOuterDome
 *
 * Description:	test the main part of the LED dome
 *
 * Parameters:
 *		none
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *				 
 */
void
TCDSti::TestOuterDome(void)
{
	int column, row, domecolumns, domerows;

	DomeReset();
	domecolumns = 40;
	domerows =  (m_DomeType == STANDARD) ? 10 : 12;

	// for each LED
	for (row = 0; row < domerows; row++)
	{
		for (column = 0; column < domecolumns; column++)
		{
			if (m_DomeType == STANDARD)
				TestLedSTD(row, column);
			else
				TestLedHC1(row, column);

			SpectrumSleep(500); //500 usec

			if (m_Continue == FALSE)
				break;
		}
		if (m_Continue == FALSE)
			break;
	}

	CString diag = "Outer Dome LED Tests: Completed: ";
	DiagReport(diag, m_DomePF);
}

/*
 * Function:	DomeTest
 *
 * Description:	Diagnostic Entry
 *
 * Parameters:
 *		none
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *				Creates a 'worker thread', which will test each LED in the Dome. 
 */
void
TCDSti::DomeTest(void)
{
	m_DomePF     = DIAG_PASSED;
	m_SplitterPF = DIAG_PASSED;

	// the worker thread to test each LED of the DOME
	InitializeWorkerThread((void *) this);
}

/*
 * Function:	WorkerLoop
 *
 * Description:	called from the base class (periodically)
 *
 * Parameters:
 *		int		count
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *				performs the diagnostic
 */
void
TCDSti::WorkerLoop(void)
{
	// if we are are ready working, discontinue;
	if (m_Continue == TRUE)
		m_Continue = FALSE;
	else
	{
		m_Continue = TRUE;
		m_StiButton.SetWindowText("Cancel");
		// disable movement
		m_StiRotateCCButton.EnableWindow(FALSE);
		m_StiRotateCButton.EnableWindow(FALSE);
		m_StiMoveUpButton.EnableWindow(FALSE);
		m_StiMoveLeftButton.EnableWindow(FALSE);
		m_StiMoveDownButton.EnableWindow(FALSE);
		m_StiMoveRightButton.EnableWindow(FALSE);
		m_Slider.EnableWindow(FALSE);
		EnableLedButtons(FALSE);

		TestOuterDome();
//		if (m_Continue == TRUE) //F404 vc6版中已刪除
//			TestSplitter();

		// report on overall results
		CString diag = "Dome LED Tests: Completed: ";
		m_DiagPF     = (m_DomePF == DIAG_PASSED && m_SplitterPF == DIAG_PASSED)
							? DIAG_PASSED : DIAG_FAILED;
		if (m_DiagPF == DIAG_PASSED)
			DiagReport(diag, DIAG_PASSED);
		else
			DiagReport(diag, DIAG_FAILED);
	}

	// redraw the screen
	GetLeds();
	SnapShot(150, 100);

	m_StiButton.SetWindowText("STI Lights");

	m_StiRotateCCButton.EnableWindow(TRUE);
	m_StiRotateCButton.EnableWindow(TRUE);
	m_StiMoveUpButton.EnableWindow(TRUE);
	m_StiMoveLeftButton.EnableWindow(TRUE);
	m_StiMoveDownButton.EnableWindow(TRUE);
	m_StiMoveRightButton.EnableWindow(TRUE);
	m_Slider.EnableWindow(TRUE);
	EnableLedButtons(TRUE);

	// kill the worker thread
	KillWorkerThread();
	m_Continue   = FALSE;
}


/****************************************************************************/
/* Camera Functions                                                         */
/****************************************************************************/
/*
 * Function:	SnapShot
 *
 * Description:	take a picture with Camer 5
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
TCDSti::SnapShot(int CameraIntensity, int LEDIntensity)
{
    m_Camera->SetExposure(CameraIntensity);
    m_Camera->SetGain(CAM_DFLT_GAIN);

	// load the lights
	WriteStrobeDefn(LEDIntensity);
	WriteLightModeDefn();;

	// get the picture
    BYTE *image = m_Camera->SnapShot();

	// build the image
	int row, col;
	for (row = 0; row < NCAM_YPIXELS; row++)
	{
		for (col = 0; col < NCAM_XPIXELS; col++)
		{
			m_Picture.m_lpImage[((NCAM_YPIXELS - 1) - row) * NCAM_XPIXELS + col] = 
				image[(row * NCAM_XPIXELS) + col];
		}
	}
	// display it
	CClientDC dc((CWnd *) m_pMainWnd);
//	m_pMainWnd->OnPrepareDC(&dc); //F404 vc6版中已刪除
	m_Picture.Draw(&dc, CPoint(300,20), m_Picture.GetDimensions());
}

/****************************************************************************/
/* Head Move Functions                                                      */
/****************************************************************************/
/*
 * Function:	MoveUp
 *
 * Description:	Move the Camera Head UP
 *
 * Parameters:
 *		void
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *
 *				1. Read the step size from the slider
 *				2. Move the Camera Head UP that amount
 *				3. Take a SnapShot with Camera 5
 *				4. Draw the LED circle
 */
void
TCDSti::MoveUp(void)
{
	STEPSIZE step = (STEPSIZE) (4 - m_Slider.GetPos());
	m_pLMotors->Step(YAxis, step, Plus);
	m_pLMotors->IsMoveComplete();
	AlignmentSnap();
}

/*
 * Function:	MoveLeft
 *
 * Description:	Move the Camera Head LEFT
 *
 * Parameters:
 *		void
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *
 *				1. Read the step size from the slider
 *				2. Move the Camera Head LEFT that amount
 *				3. Take a SnapShot with Camera 5
 *				4. Draw the LED circle
 */
void
TCDSti::MoveLeft(void)
{
	STEPSIZE step = (STEPSIZE) (4 - m_Slider.GetPos());
	m_pLMotors->Step(XAxis, step, Plus);
	m_pLMotors->IsMoveComplete();
	AlignmentSnap();
}

/*
 * Function:	MoveDown
 *
 * Description:	Move the Camera Head DOWN
 *
 * Parameters:
 *		void
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *
 *				1. Read the step size from the slider
 *				2. Move the Camera Head DOWN that amount
 *				3. Take a SnapShot with Camera 5
 *				4. Draw the LED circle
 */
void
TCDSti::MoveDown(void)
{
	STEPSIZE step = (STEPSIZE) (4 - m_Slider.GetPos());
	m_pLMotors->Step(YAxis, step, Minus);
	m_pLMotors->IsMoveComplete();
	AlignmentSnap();
}

/*
 * Function:	MoveRight
 *
 * Description:	Move the Camera Head RIGHT
 *
 * Parameters:
 *		void
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *
 *				1. Read the step size from the slider
 *				2. Move the Camera Head RIGHT that amount
 *				3. Take a SnapShot with Camera 5
 *				4. Draw the LED circle
 */
void
TCDSti::MoveRight(void)
{
	STEPSIZE step = (STEPSIZE) (4 - m_Slider.GetPos());
	m_pLMotors->Step(XAxis, step, Minus);
	m_pLMotors->IsMoveComplete();
	AlignmentSnap();
}

/*
 * Function:	RotateC
 *
 * Description:	change the rotation of the LED circles
 *
 * Parameters:
 *		void
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *
 *				1. Change the rotation value Clockwise
 *				2. Take a SnapShot with Camera 5
 *				4. Draw the new LED circle
 */
void
TCDSti::RotateC(void)
{
	m_Rotation += 3;
	CString msg;
	msg.Format("rotation = %d", m_Rotation);
	TRACE(msg);
	AlignmentSnap();
}

/*
 * Function:	RotateCC
 *
 * Description:	change the rotation of the LED circles
 *
 * Parameters:
 *		void
 * 
 * Return Values
 *		none
 *
 * Discussion:
 *
 *				1. Change the rotation value Counter-Clockwise
 *				2. Take a SnapShot with Camera 5
 *				4. Draw the new LED circle
 */
void
TCDSti::RotateCC(void)
{
	m_Rotation -= 3;
	CString msg;
	msg.Format("rotation = %d", m_Rotation);
	TRACE(msg);
	AlignmentSnap();
}
