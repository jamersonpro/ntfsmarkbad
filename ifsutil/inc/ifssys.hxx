/*++

Module Name:

        ifssys.hxx

Abstract:

        This module contains the definition for the IFS_SYSTEM class.
    The IFS_SYSTEM class is an abstract class which offers an
    interface for communicating with the underlying operating system
    on specific IFS issues.

--*/

#pragma once

#include "drive.hxx"



DECLARE_CLASS( WSTRING );
DECLARE_CLASS( BIG_INT );
DECLARE_CLASS( IFS_SYSTEM );

class IFS_SYSTEM {

    public:

        STATIC
        BOOLEAN
        QueryFileSystemNameIsNtfs(
            IN  PCWSTRING    NtDriveName,
            OUT PBOOL     FileSystemNameIsNtfs,
            OUT PNTSTATUS    ErrorCode DEFAULT NULL
            );


        STATIC
        BOOLEAN
        IsThisNtfs(
            IN  BIG_INT Sectors,
            IN  ULONG   SectorSize,
            IN  PVOID   BootSectorData
            );

};


extern
BOOLEAN
IfsutilDefineClassDescriptors(
);

extern
BOOLEAN
IfsutilUndefineClassDescriptors(
);
