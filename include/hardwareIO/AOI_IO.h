#pragma once

#ifdef CreateWithGUIDInterface
#include <windows.h>
#include <setupapi.h>
#endif

// Note: The files need to point directly to the aoi_PCIO driver includes and NOT to copies of those files. 
#include "..\AOI_PCIO_Driver\sys\aoi_adapterRegs.h"
#include "..\AOI_PCIO_Driver\sys\aoi_pcioIOctrl.h"

// Some MacroMagic for a smart TODO message 
#define _STR(x) #x
#define STR(x) _STR(x)
#define TODO(x) __pragma(message("TODO: @ ln:"STR(__LINE__) " "_STR(x) ))

class AOI_IO 
{
public:
	// these two are public only because the AOI_test application calls them direcly 
	// the regular application should use the derived class's PCIO_read and PCIO_Write functions
	bool Read_IOCtrl( USHORT, USHORT * );	// via IOctl
	bool Write_IOCtrl( USHORT, USHORT );	// via IOctl
private:
	USHORT *io_map;
#ifdef  CreateWithGUIDInterface
	bool GetDevicePathFromGUID(void );
    HDEVINFO hDevInfo;
    PSP_DEVICE_INTERFACE_DETAIL_DATA pDeviceInterfaceDetail;
#endif
	
protected:
	AOI_IO();
	~AOI_IO();
	HANDLE hDevice;
	bool GetDeviceHandleBySymLink(void);
	BOOL GetDriverIOCTLVersion (char *buf , size_t bufsz);
	BOOL GetMemMap(void);
	void Close_Device_Handle(void);
	bool PCIO_Read(USHORT index, USHORT *val );	// via *io_map
	bool PCIO_Write(USHORT index, USHORT val );	// via *io_map

	int		StartInterruptThread( class TCPcio *);
	HANDLE  m_InterruptThreadHdl;
	HANDLE  m_EndInterruptThreadEvt;
	bool	m_InterruptThreadStatus;
	bool	m_InterruptThreadExitWith_WD_running;
	void	EndInterruptThread(bool leaveSysWatchdogRunning);

#ifdef  CreateWithGUIDInterface   
	bool GetDeviceHandleByGUID(void);
#endif

};

