/*++

Module Name:

    ifsentry.hxx

Abstract:

    Contains prototypes for entry points to the IFS
    utility DLLs.

--*/

#pragma once

//
// Definition of DRIVE_TYPE
//
enum DRIVE_TYPE {
    UnknownDrive,
    RemovableDrive,
    FixedDrive,
    RemoteDrive,
    CdRomDrive,
    RamDiskDrive
};


