#include "system.h"
#include "jamexprt.h"

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

CSystem::CSystem()
{
	driver = NULL;
}

CSystem::~CSystem()
{
	CloseHandle(driver);
	driver = NULL;
}

int CSystem::program(char* fname, CStatic* message, CProgressCtrl* progress)
{
	//setting up flags and data
	bool messageFlag = true, progressFlag = true, error = FALSE;
	long offset = 0L, error_line = 0L, workspace_size = 0;
	JAM_RETURN_TYPE crc_result = JAMC_SUCCESS, exec_result = JAMC_SUCCESS;
	unsigned short expected_crc = 0, actual_crc = 0;
	char key[33] = {0}, value[257] = {0};
	int exit_status = 0, arg = 0, exit_code = 0, time_delta = 0, init_count = 0;
	int* windows_nt = NULL;
	time_t start_time = 0, end_time = 0;
	char *workspace = NULL;
	char *init_list[10];
	FILE *fp = NULL;
	struct stat sbuf;
	char *exit_string = NULL;

	char *file_buffer = NULL;
	long file_length = 0L;

	char* action = "PROGRAM";

	if (message == NULL)
		messageFlag = false;
	if (progress == NULL)
		progressFlag = false;

	if (messageFlag)
		message->SetWindowText("Checking file validity. . . ");
	
	if(!isValidFileName(CString(fname)))
		return INVALID_FILENAME;
	
	init_list[0] = NULL;

	if (!initialize_jtag_hardware())
		return DRIVER_ERROR;

	if (_access(fname, 0) != 0)
		return INVALID_FILE;
	else
	{
		//get length of file
		if (stat(fname, &sbuf) == 0) 
			file_length = sbuf.st_size;

		if ((fp = fopen( fname, "rb")) == NULL)
		{
			fprintf(stderr, "Error: can't open file \"%s\"\n", fname);
			return INVALID_FILE;
		}
		else
		{
			//Read entire file into a buffer
			file_buffer = (char *) malloc((size_t) file_length);

			if (file_buffer == NULL)
				return MEMORY_ERROR;
			else
			{
				if (fread(file_buffer, 1, (size_t) file_length, fp) != (size_t) file_length)
					return INVALID_FILE;
			}

			fclose(fp);
		}

		if (exit_status == 0)
		{
			data_exchange(file_buffer, file_length);
			
			calibrate_delay();

			crc_result = jam_check_crc(file_buffer, file_length, &expected_crc, &actual_crc);
			char crc_string[200];

			switch (crc_result)
			{
				case JAMC_SUCCESS:
					break;

				case JAMC_CRC_ERROR:
					sprintf_s(crc_string, "CRC mismatch: expected %04X, actual %04X\nContinue?", expected_crc, actual_crc);
					if (MessageBox(NULL, crc_string, "CRC Error", MB_OK | MB_YESNO) == IDNO)
						return SUCCESS;
					break;

				case JAMC_UNEXPECTED_END:
					sprintf_s(crc_string, "Expected CRC not found, actual CRC value = %04X\nContinue?", actual_crc);
					if (MessageBox(NULL, crc_string, "CRC Error", MB_OK | MB_YESNO) == IDNO)
						return SUCCESS;
					break;

				default:
					sprintf_s(crc_string, "CRC function returned error code %d\n", crc_result);
					MessageBox(NULL, crc_string, "CRC Error", MB_OK | MB_ICONERROR);
					return CRC_ERROR;
					break;
			}

			if (messageFlag)
				message->SetWindowText("Programming. . . ");
	
			exec_result = jam_execute(file_buffer, file_length, workspace, workspace_size, action, init_list, &error_line, &exit_code, progress);

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
					default: exit_code = error_line + 100; break;		//exit_string = "Unknown exit code"; break;
				}

			}
			else if (exec_result == JAMC_SYNTAX_ERROR)
				exit_code = error_line + 100;
			else
				exit_code = UNKNOWN_ERROR;
		}
	}

	close_jtag_hardware();
	progress->SetPos(100);

	return (exit_code);
}

bool CSystem::isValidFileName(CString name)
{
	CString extension = name.Right(4);
	CString str(".jam");
	if (name == "" || extension.CompareNoCase(str))
		return false;
	else
		return true;
}


CString CSystem::GetRev()
{
	unsigned short FirmMajor, FirmMinor;
	CString rev;

	if (!initialize_jtag_hardware())
	{
		CString str("%d", DRIVER_ERROR);
		return str;
	}

// See comment at line 3 above
	pciioSetDigital();
	pciioRead(CONTROL_FIRM_MAJOR, &FirmMajor);
	pciioRead(CONTROL_FIRM_MINOR, &FirmMinor);

	close_jtag_hardware();
	rev.Format("%c.%X",	'@' + FirmMajor, FirmMinor);

	return rev;
}

unsigned short CSystem::Write(unsigned short address, unsigned short data)
{
	if (!initialize_jtag_hardware())
		return DRIVER_ERROR;

	pciioSetDigital();
	pciioWrite(address, &data);

	close_jtag_hardware();

	return SUCCESS;
}

unsigned short CSystem::Read(unsigned short address)
{
	unsigned short data;
	
	if (!initialize_jtag_hardware())
		return DRIVER_ERROR;

	pciioSetDigital();
	pciioRead(address, &data);

	close_jtag_hardware();

	return data;
}
