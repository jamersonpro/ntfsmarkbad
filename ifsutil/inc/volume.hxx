/*++

Module Name:

    volume.hxx

Abstract:

    Provides volume methods.

--*/

#pragma once


#include "drive.hxx"
#include "numset.hxx"

//
//      Forward references
//

DECLARE_CLASS( HMEM );
DECLARE_CLASS( MESSAGE );
DECLARE_CLASS( SUPERAREA );
DECLARE_CLASS( VOL_LIODPDRV     );
DECLARE_CLASS( WSTRING );
DECLARE_CLASS( WSTRING );

// This number describes the minimum number of bytes in a boot sector.
#define BYTES_PER_BOOT_SECTOR   512


typedef ULONG VOLID;

#define MAXVOLNAME 11

#define AUTOCHK_TIMEOUT                 10              // 10 seconds before initiating autochk
#define MAX_AUTOCHK_TIMEOUT_VALUE       (3*24*3600)     // 3 days maximum

enum FIX_LEVEL {
    CheckOnly,
    TotalFix,
    SetupSpecial
};

enum FORMAT_ERROR_CODE {
        GeneralError,
        NoError,
        LockError
};


class VOL_LIODPDRV : public LOG_IO_DP_DRIVE {

    public:

        BOOLEAN
            MarkBad(
                IN      __int64   firstPhysicalDriveSector,
                IN      __int64   lastPhysicalDriveSector,
                IN OUT  PMESSAGE    Message
            );
	
        VIRTUAL
         
        ~VOL_LIODPDRV(
            );


         
        PSUPERAREA
        GetSa(
            );


    protected:

         
        DECLARE_CONSTRUCTOR( VOL_LIODPDRV );

         
        FORMAT_ERROR_CODE
        Initialize(
            IN      PCWSTRING           NtDriveName,
            IN      PSUPERAREA          SuperArea,
            IN OUT  PMESSAGE            Message         DEFAULT NULL
            );


    private:

         
        VOID
        Construct (
            );

         
        VOID
        Destroy(
            );

         
        PSUPERAREA  _sa;
        NUMBER_SET  _bad_sectors;

};


INLINE
PSUPERAREA
VOL_LIODPDRV::GetSa(
    )
/*++

Routine Description:

    This routine returns a pointer to the current super area.

Arguments:

    None.

Return Value:

    A pointer to the current super area.

--*/
{
        return _sa;
}

