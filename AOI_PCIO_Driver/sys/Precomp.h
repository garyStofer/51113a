#define WIN9X_COMPAT_SPINLOCK
#include <ntddk.h>
#pragma warning(disable:4201)  // nameless struct/union warning

#include <stdarg.h>
#include <wdf.h>

#pragma warning(default:4201)


#include <initguid.h> // required for GUID definitions
#include <wdmguid.h>  // required for WMILIB_CONTEXT

#include "aoi_adapterRegs.h"
#include "aoi_GUIDInterface.h"
#include "aoi_pcioIOctrl.h"
#include "Pcio.h"
#include "trace.h"


