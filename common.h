#pragma once

#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_NON_CONFORMING_SWPRINTFS
#define _WIN32_WINNT 0x0601
#define DECLSPEC_DEPRECATED_DDK

#include <sdkddkver.h>


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <memory.h>


#include <ntstatus.h>

#define WIN32_NO_STATUS
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>

#define NTKERNELAPI  __declspec(dllimport)

typedef ULONG CLONG;

#include <setupapi.h>
#include <winioctl.h>
#include <ntddmmc.h>
#include <ntddcdrm.h>
#include <cfgmgr32.h>

#include "my_ntddk.h"


#define ASSERT(x) assert(x)
#define ASSERTMSG(message,x) assert((message)&&(x))
#define NOTHING do{}while(false)
#define ARGUMENT_PRESENT(ArgumentPointer)    (\
    (CHAR *)((ULONG_PTR)(ArgumentPointer)) != (CHAR *)(NULL) )



#include <ntstatus.h>

#define WIN32_NO_STATUS
//#include <windows.h>

//#include <nt.h>
//#include <ntrtl.h>
//#include <nturtl.h>
#include <devioctl.h>
#include <ntdddisk.h>
#include <bcrypt.h>


#include <ntddscsi.h>
#ifndef _NTSRB_
#define _NTSRB_
#endif
#include <scsi.h>


#include <initguid.h>
#include <diskguid.h>


//-------------------------

#define _NTDEF_
#include <WINERROR.H>


#define ARGUMENT_PRESENT(ArgumentPointer)    (\
    (CHAR *)((ULONG_PTR)(ArgumentPointer)) != (CHAR *)(NULL) )


struct sectors_range
{
    sectors_range(unsigned __int64 firstSector, unsigned __int64 lastSector)
    {
        this->firstSector = firstSector;
        this->lastSector = lastSector;
    }

    unsigned __int64 firstSector;
    unsigned __int64 lastSector;

    bool contains(unsigned __int64 sector) const
    {
        return firstSector <= sector && sector <= lastSector;
    }

    bool operator<(const sectors_range& another) const
    {
        return  (firstSector < another.firstSector) || (firstSector == another.firstSector && lastSector < another.lastSector);
    }
};


