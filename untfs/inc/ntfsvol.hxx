/*++

Module Name:

    ntfsvol.hxx

Abstract:

    This class implements NTFS only VOLUME items.

--*/

#pragma once

#include "volume.hxx"
#include "ntfssa.hxx"

DECLARE_CLASS( NTFS_SA );
DECLARE_CLASS( NTFS_VOL );
DECLARE_CLASS( MESSAGE );

class NTFS_VOL : public VOL_LIODPDRV
{

public:

    DECLARE_CONSTRUCTOR(NTFS_VOL);

    VIRTUAL
        ~NTFS_VOL(
        );

     
        FORMAT_ERROR_CODE
        Initialize(
            IN      PCWSTRING   NtDriveName,
            IN OUT  PMESSAGE    Message         DEFAULT NULL
        );


private:

     
        VOID
        Construct(
        );

     
        VOID
        Destroy(
        );

    NTFS_SA _ntfssa;

};


