/****************************************************************************/
/*																			*/
/*	Module:			jamstub.c												*/
/*																			*/
/*					Copyright (C) Altera Corporation 1997-1999				*/
/*																			*/
/*	Description:	Main source file for stand-alone JAM test utility.		*/
/*					Altered to accomodate Teradyne pciio card and System	*/
/*					Controller board. For winnt only, compiled to a DLL.	*/
/*																			*/
/****************************************************************************/

#if ( _MSC_VER >= 800 )
#pragma warning(disable:4115)
#pragma warning(disable:4201)
#pragma warning(disable:4214)
#pragma warning(disable:4514)
#endif


#include "pciio.h" // This is acut down version of AOI_IO -- only to be used for the stand alone Flash tool

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <malloc.h>
#include <time.h>
#include <conio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "jamexprt.h"

#define PGDC_IOCTL_GET_DEVICE_INFO_PP 0x00166A00L
#define PGDC_IOCTL_READ_PORT_PP       0x00166A04L
#define PGDC_IOCTL_WRITE_PORT_PP      0x0016AA08L
#define PGDC_IOCTL_PROCESS_LIST_PP    0x0016AA1CL
#define PGDC_READ_INFO                0x0a80
#define PGDC_READ_PORT                0x0a81
#define PGDC_WRITE_PORT               0x0a82
#define PGDC_PROCESS_LIST             0x0a87
#define PGDC_HDLC_NTDRIVER_VERSION    2
#define PORT_IO_BUFFER_SIZE           256

#pragma intrinsic (inp, outp)


/************************************************************************
*
*	Global variables
*/
	//return codes
	#define SUCCESS					0
	#define FILE_DOES_NOT_EXIST		1
	#define INVALID_FILENAME		2
	#define COMMUNICATION_ERROR		3
	#define PROGRAM_NOT_VERIFIED	10
	#define INVALID_FILE			11
	#define MEMORY_ERROR			12
	#define DRIVER_ERROR			13
	#define PROGRAM_ERROR			14
	#define UNRECOGNIZED_DEVICE		15
	#define PROGRAM_FAILURE			16
	#define UNKNOWN_ERROR			17

	unsigned short address = 0;	
	unsigned short tdiArr[400];
	unsigned short tmsArr[400];
	unsigned short tdoArr[1000000];
	unsigned short rtdoArr[1000000];
	int index = 0;
	int line = 0;
	FILE *fp;
	int i;


/* file buffer for JAM input file */
char *file_buffer = NULL;
long file_pointer = 0L;
long file_length = 0L;

/* delay count for one millisecond delay */
int one_ms_delay = 0;

/* delay count to reduce the maximum TCK frequency */
int tck_delay = 0;

/* serial port interface available on all platforms */
BOOL jtag_hardware_initialized = FALSE;
char *serial_port_name = NULL;
BOOL specified_com_port = FALSE;
int com_port = -1;

/* parallel port interface available on PC only */
BOOL specified_lpt_port = FALSE;
BOOL specified_lpt_addr = FALSE;
int lpt_port = 1;
int initial_lpt_ctrl = 0;
WORD lpt_addr = 0x3bc;
WORD lpt_addr_table[3] = { 0x3bc, 0x378, 0x278 };
WORD lpt_addresses_from_registry[4] = { 0 };
BOOL alternative_isp_cable = FALSE;

/* variables to manage cached I/O under Windows NT */
BOOL windows_nt = FALSE;
int port_io_count = 0;
HANDLE nt_device_handle = INVALID_HANDLE_VALUE;
struct PORT_IO_LIST_STRUCT
{
	USHORT command;
	USHORT data;
} port_io_buffer[PORT_IO_BUFFER_SIZE];
BOOL initialize_nt_driver(void);

/* function prototypes to allow forward reference */
extern void delay_loop(int count);

/*
*	This structure stores information about each available vector signal
*/
struct VECTOR_LIST_STRUCT
{
	char *signal_name;
	int  hardware_bit;
	int  vector_index;
};

/*
*	Vector signals for ByteBlaster:
*
*	tck (dclk)    = register 0, bit 0
*	tms (nconfig) = register 0, bit 1
*	tdi (data)    = register 0, bit 6
*	tdo (condone) = register 1, bit 7 (inverted!)
*	nstatus       = register 1, bit 4 (not inverted)
*/
struct VECTOR_LIST_STRUCT vector_list[] =
{
	/* add a record here for each vector signal */
	{ "**TCK**",   0, -1 },
	{ "**TMS**",   1, -1 },
	{ "**TDI**",   6, -1 },
	{ "**TDO**",   7, -1 },
	{ "TCK",       0, -1 },
	{ "TMS",       1, -1 },
	{ "TDI",       6, -1 },
	{ "TDO",       7, -1 },
	{ "DCLK",      0, -1 },
	{ "NCONFIG",   1, -1 },
	{ "DATA",      6, -1 },
	{ "CONF_DONE", 7, -1 },
	{ "NSTATUS",   4, -1 }
};

#define VECTOR_SIGNAL_COUNT ((int)(sizeof(vector_list)/sizeof(vector_list[0])))

BOOL verbose = FALSE;

/************************************************************************
*
*	Customized interface functions for JAM interpreter I/O:
*
*	jam_getc()
*	jam_seek()
*	jam_jtag_io()
*	jam_message()
*	jam_delay()
*/

int jam_getc(void)
{
	int ch = EOF;

	if (file_pointer < file_length)
	{
		ch = (int) file_buffer[file_pointer++];
	}

	return (ch);
}

int jam_seek(long offset)
{
	int return_code = EOF;

	if ((offset >= 0L) && (offset < file_length))
	{
		file_pointer = offset;
		return_code = 0;
	}

	return (return_code);
}

int jam_jtag_io(int tms, int tdi, int read_tdo)
{
	unsigned short data = 0, tck_high;
	int tdo = 0;
	int count = 50;

	address = ((tms ? 1 : 0) << 1) | (tdi ? 1 : 0);
	tck_high = address | 0x4;
// see TODO comment at line 22
	pciioWrite(address, &data);
    
    if (read_tdo)
	{
		pciioRead(address, &data);
		tdo = (data & 1);

	}

	//pciioWrite(tck_high, &data);
	//pciioWrite(address, &data);
    
	return tdo;
}

void jam_message(char *message_text)
{
	puts(message_text);
	fflush(stdout);
}

void jam_export(char *key, long value)
{
	if (verbose)
	{
		//printf("Export: key = \"%s\", value = %ld\n", key, value);
		fflush(stdout);
	}
}

void jam_delay(long microseconds)
{
	delay_loop((int) (microseconds *
		((one_ms_delay / 1000L) + ((one_ms_delay % 1000L) ? 1 : 0))));
}

int jam_vector_map(int signal_count, char **signals)
{
	int matched_count = 0;

	return (matched_count);
}

int jam_vector_io(int signal_count, long *dir_vect,	long *data_vect, long *capture_vect)
{
	int matched_count = 0;

	return (matched_count);
}

int jam_set_frequency(long hertz)
{
	if (verbose)
	{
		//printf("Frequency: %ld Hz\n", hertz);
		fflush(stdout);
	}

	if (hertz == -1)
	{
		/* no frequency limit */
		tck_delay = 0;
	}
	else if (hertz == 0)
	{
		/* stop the clock */
		tck_delay = -1;
	}
	else
	{
		/* set the clock delay to the period */
		/* corresponding to the selected frequency */
		tck_delay = (one_ms_delay * 1000) / hertz;
	}

	return (0);
}

void *jam_malloc(unsigned int size)
{
	return (malloc(size));
}

void jam_free(void *ptr)
{	
	if(ptr != NULL)
		free(ptr);	
}


/************************************************************************
*
*	get_tick_count() -- Get system tick count in milliseconds
*
*	for DOS, use BIOS function _bios_timeofday()
*	for WINDOWS use GetTickCount() function
*	for UNIX use clock() system function
*/
DWORD get_tick_count(void)
{
    DWORD tick_count = 0L;

	tick_count = GetTickCount();

    return (tick_count);
}

#define DELAY_SAMPLES 10
#define DELAY_CHECK_LOOPS 10000

void calibrate_delay(void)
{
	int sample = 0;
	int count = 0;
	DWORD tick_count1 = 0L;
	DWORD tick_count2 = 0L;

	one_ms_delay = 0;

	for (sample = 0; sample < DELAY_SAMPLES; ++sample)
	{
		count = 0;
		tick_count1 = get_tick_count();

		while ((tick_count2 = get_tick_count()) == tick_count1)
		{};
		
		do 
		{ 
			delay_loop(DELAY_CHECK_LOOPS);
			count++;
		} while	((tick_count1 = get_tick_count()) == tick_count2);

		one_ms_delay += (int)((DELAY_CHECK_LOOPS * (DWORD)count) / (tick_count1 - tick_count2));
	}

	one_ms_delay /= DELAY_SAMPLES;
}

char *error_text[] =
{
	"success",
	"out of memory",
	"file access error",
	"syntax error",
	"unexpected end of file",
	"undefined symbol",
	"redefined symbol",
	"integer overflow",
	"divide by zero",
	"CRC mismatch",
	"internal error",
	"bounds error",
	"type mismatch",
	"assignment to constant object",
	"NEXT statement unexpected",
	"POP statement unexpected",
	"RETURN statement unexpected",
	"illegal symbolic name",
	"vector signal name not found",
	"unexpected statement type (phase error)",
	"illegal symbol reference (scope error)",
	"action name not found"
};

#define MAX_ERROR_CODE (int)((sizeof(error_text)/sizeof(error_text[0]))+1)

/************************************************************************/

int jam(char* filename, char* action)
{
	BOOL error = FALSE;
	long offset = 0L;
	long error_line = 0L;
	JAM_RETURN_TYPE crc_result = JAMC_SUCCESS;
	JAM_RETURN_TYPE exec_result = JAMC_SUCCESS;
	unsigned short expected_crc = 0;
	unsigned short actual_crc = 0;
	char key[33] = {0};
	char value[257] = {0};
	int exit_status = 0;
	int arg = 0;
	int exit_code = 0;
	time_t start_time = 0;
	time_t end_time = 0;
	int time_delta = 0;
	char *workspace = NULL;
	char *init_list[10];
	int init_count = 0;
	FILE *fp = NULL;
	struct stat sbuf;
	long workspace_size = 0;
	char *exit_string = NULL;

	verbose = FALSE;

	init_list[0] = NULL;

	if (!jtag_hardware_initialized)
	{
		if (!initialize_jtag_hardware())
			return DRIVER_ERROR;
	}

	if (_access(filename, 0) != 0)
	{
		//fprintf(stderr, "Error: can't access file \"%s\"\n", filename);
		return INVALID_FILE;
	}
	else
	{
		//get length of file
		if (stat(filename, &sbuf) == 0) file_length = sbuf.st_size;

		fopen_s(&fp, filename, "rb");
		if (fp == NULL)
		{
			//fprintf(stderr, "Error: can't open file \"%s\"\n", filename);
			return INVALID_FILE;
		}
		else
		{
			//Read entire file into a buffer
			file_buffer = (char *) malloc((size_t) file_length);

			if (file_buffer == NULL)
			{
				//fprintf(stderr, "Error: can't allocate memory (%d Kbytes)\n",
					//(int) (file_length / 1024L));
				return MEMORY_ERROR;
			}
			else
			{
				if (fread(file_buffer, 1, (size_t) file_length, fp) !=
					(size_t) file_length)
				{
					//fprintf(stderr, "Error reading file \"%s\"\n", filename);
					return INVALID_FILE;
				}
			}

			fclose(fp);
		}

		if (exit_status == 0)
		{
			windows_nt = !(GetVersion() & 0x80000000);

			calibrate_delay();

			crc_result = jam_check_crc(file_buffer, file_length, &expected_crc, &actual_crc);

			switch (crc_result)
			{
				case JAMC_SUCCESS:
					//printf("CRC matched: CRC value = %04X\n", actual_crc);
					break;

				case JAMC_CRC_ERROR:
					//printf("CRC mismatch: expected %04X, actual %04X\n", expected_crc, actual_crc);
					break;

				case JAMC_UNEXPECTED_END:
					//printf("Expected CRC not found, actual CRC value = %04X\n",	actual_crc);
					break;

				default:
					//printf("CRC function returned error code %d\n", crc_result);
					break;
			}

			exec_result = jam_execute(file_buffer, file_length, workspace, workspace_size, action, init_list, &error_line, &exit_code, NULL);

			if (exec_result == JAMC_SUCCESS)
			{
				switch (exit_code)
				{
					case 0:  exit_code = SUCCESS; break;				//exit_string = "Success"; break;
					case 1:  exit_code = SUCCESS; break;				//exit_string = "Illegal initialization values"; break;
					case 2:  exit_code = UNRECOGNIZED_DEVICE; break;	//exit_string = "Unrecognized device"; break;
					case 3:  exit_code = SUCCESS; break;				//exit_string = "Device revision is not supported"; break;
					case 4:  exit_code = PROGRAM_FAILURE; break;		//exit_string = "Device programming failure"; break;
					case 5:  exit_code = SUCCESS; break;				//exit_string = "Device is not blank"; break;
					case 6:  exit_code = PROGRAM_NOT_VERIFIED; break;	//exit_string = "Device verify failure"; break;
					case 7:  exit_code = SUCCESS; break;				//exit_string = "SRAM configuration failure"; break;
					default: exit_code = SUCCESS; break;				//exit_string = "Unknown exit code"; break;
				}

			}
			else if (exec_result < MAX_ERROR_CODE)
			{
				exit_code = (error_line += 100);
			}
			else
				exit_code = UNKNOWN_ERROR;
		}
	}

	if (jtag_hardware_initialized) close_jtag_hardware();

	return (exit_code);
}

int initialize_jtag_hardware()
{
	if (!jtag_hardware_initialized)
	{
		if (!pciioOpen())
			return FALSE;
		//pciioDisableInterrupts();
		pciioSetAnalog();
		pciioDisableInterrupts();
		jtag_hardware_initialized = TRUE;
	}
	return TRUE;
}

void close_jtag_hardware()
{
	pciioClose();
	jtag_hardware_initialized = FALSE;
}

#if !defined (DEBUG)
#pragma optimize ("ceglt", off)
#endif

void delay_loop(int count)
{
	unsigned short data = 0;
	
	while (count != 0)
	{
		pciioWrite(address, &data);
		count--;
	}
}

void data_exchange(char* buffer, long length)
{
	file_buffer = buffer;
	file_length = length;
	windows_nt = !(GetVersion() & 0x80000000);
}
