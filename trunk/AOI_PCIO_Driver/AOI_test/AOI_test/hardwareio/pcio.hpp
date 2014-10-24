#pragma once
#include "hardwareio\AOI_IO.h"
#include "hardware_enum.h"
#include "hardwareio\controllerAddresses.hpp"

#define CONTROLLER_BOOT_TIME 500	// TODO: Might be tweaked downward 


class TCPcio: public AOI_IO	
{
public:
	TCPcio();
	~TCPcio();
	int PCIO_Open(void);
	void PCIO_Close(void);
	bool PCIO_Read(USHORT index, USHORT *val );	
	bool PCIO_Write(USHORT index, USHORT val );	
	int  StartInterruptThread(void);
	void KillAllThreads(int thread );	
	void PauseSysWatchdog(void);
	void ContinueSysWatchdog(void);
	
	static DWORD WINAPI InterruptThread(void *arg);
protected:
	
private:
	
	void StartSysWatchdog(void);
	void ReloadSysWatchdog(void);
	void DisableSysWatchdog(void );

	int m_somePrivatmember;
};