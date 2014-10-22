//This is the GUID for the DeviceIntreface class which will be used by the user mode application to
//enumerate and find the device in order to utimately obtain a file handle for IO_Control calls to the driver.
// This GUID is not to be confused with  the SetupClass GUID as used in the .inf file. It is only used 
// for naming and grouping of similar devices in the Device Manager user interface. In fact the DeviceInterface 
// GUID can not be the same as the SetupClass GUID
// guidgen.exe was used to generate the GUID below and it should not be changed in the future 

//include this file both in the driver and in the  win32 user mode application 
// uncomment the define to use the GUID based driver interface 
// #define  CreateWithGUIDInterface 
// {55BDE5FD-C06F-46C6-A205-A7F41E8838A3}
DEFINE_GUID(GUID_DEVINTERFACE_AOI_PCIO ,
	0x55bde5fd, 0xc06f, 0x46c6, 0xa2, 0x5, 0xa7, 0xf4, 0x1e, 0x88, 0x38, 0xa3);

