/* 
	This module is a cut down C-only version of the class AOI_IO found in harwareIO.dll.
	This code is used for the standalone executable Flash.exe which uses JAMdll.dll
	to program various harware devices via the jtag chain built into the WC140 controller
*/

#include <windows.h>
#include <winioctl.h>

#include "..\AOI_PCIO_Driver\sys\aoi_adapterRegs.h"
#include "..\AOI_PCIO_Driver\sys\aoi_pcioIOctrl.h"

#include "Pciio.h"

#define PCIO_SUCCESS 0

HANDLE	   hPCIO_Device;
static unsigned short *io_map = NULL;

int pciioRead(unsigned short addr, unsigned short *data) 
{
	if (io_map) 
		*data = io_map[addr];
	return PCIO_SUCCESS;
}

int pciioWrite(unsigned short addr, unsigned short *data) 
{
	if (io_map)
		io_map[addr] = *data;
	return PCIO_SUCCESS;
}


void pciioSetDigital(void)
{
	// digital IO
	unsigned short cmd = D_STRB;
	pciioWrite(PCIO_STRB_CTRL, &cmd);
}

void pciioSetAnalog(void)
{
	// digital IO
	unsigned short cmd = A_STRB;
	pciioWrite(PCIO_STRB_CTRL, &cmd);
}

void pciioDisableInterrupts(void)
{
	// disable all interrupts
	unsigned short cmd = ALL_INT_DIS;
	pciioWrite(PCIO_INT_SEL, &cmd);
}

// Note: Return codes for this function are inconsistent with all other functions found in here.
int pciioOpen(void)
{
	hPCIO_Device = CreateFile(DRIVER_SYMBOLIC_NAME_app,
					  GENERIC_READ | GENERIC_WRITE,
					  FILE_SHARE_READ,
					  NULL,
					  OPEN_EXISTING,
					  0,
					  NULL);
	if (hPCIO_Device == INVALID_HANDLE_VALUE)
	{												// no handle (driver)
		hPCIO_Device = NULL;
		return 0;
	}
	else
	{											// found driver
		unsigned long	nOutput;				// Count written to bufOutput
		unsigned short *  pointers[2];
		// Call device IO Control interface (PCIO_IOCTL_GetMemMap) in driver
		if (!DeviceIoControl(hPCIO_Device,
							 PCIO_IOCTL_GetMemMap,
							 NULL,
							 0,
							 pointers,
							 sizeof(pointers),
							 &nOutput,
							 NULL))
		{												// iomap failure, close driver

			CloseHandle(hPCIO_Device);
			hPCIO_Device = NULL;  // iomap dead, kill 
			return 0;
		}
		else
		{												// map the device
			io_map = pointers[0];
		}
	}
	return 1;
}

void pciioClose()
{
	CloseHandle(hPCIO_Device);
	hPCIO_Device = INVALID_HANDLE_VALUE;
}

int pciioProgram()
{
	unsigned short one = 1;
	pciioWrite(0x15, &one);
	return PCIO_SUCCESS;
}