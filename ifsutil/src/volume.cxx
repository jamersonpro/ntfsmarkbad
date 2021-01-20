#include "stdafx.h"

/*++

Module Name:

    volume.cxx

Abstract:

    Provides volume methods.

--*/


#include "ulib.hxx"


#include "volume.hxx"
#include "supera.hxx"
#include "hmem.hxx"
#include "message.hxx"

#include "ifsentry.hxx"

#include "path.hxx"


DEFINE_CONSTRUCTOR( VOL_LIODPDRV, LOG_IO_DP_DRIVE   );

VOID
VOL_LIODPDRV::Construct (
    )
/*++

Routine Description:

    Constructor for VOL_LIODPDRV.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _sa = NULL;
}

 
VOL_LIODPDRV::~VOL_LIODPDRV(
    )
/*++

Routine Description:

    Destructor for VOL_LIODPDRV.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


VOID
VOL_LIODPDRV::Destroy(
    )
/*++

Routine Description:

    This routine returns a VOL_LIODPDRV to its initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _sa = NULL;
}


 
FORMAT_ERROR_CODE
VOL_LIODPDRV::Initialize(
    IN      PCWSTRING   NtDriveName,
    IN      PSUPERAREA  SuperArea,
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This routine initializes a VOL_LIODPDRV to a valid state.

Arguments:

    NtDriveName     - Supplies the drive path for the volume.
    SuperArea       - Supplies the superarea for the volume.
    Message         - Supplies an outlet for messages.
    ExclusiveWrite  - Supplies whether or not the drive should be
                        opened for exclusive write.
    FormatMedia     - Supplies whether or not to format the media.
    MediaType       - Supplies the type of media to format to.
    FormatType      - Supplies the file system type in the event of a format
    ForceDismount   - Supplies whether the drive should be dismounted
                        and locked 

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    Destroy();

    DebugAssert(NtDriveName);
    DebugAssert(SuperArea);

    if (!LOG_IO_DP_DRIVE::Initialize(NtDriveName, Message)) 
    {
        return GeneralError;
    }

    if (!_bad_sectors.Initialize()) {
        return GeneralError;
    }

    _sa = SuperArea;


    if (QueryMediaType() == Unknown) 
    {
        Message ? Message->Out("Disk is not formatted.") : 1;
        return GeneralError;
    }


    if (QueryMediaType() != Unknown && QuerySectors() == 0) 
    {
        if (Message) {
            Message->Out("Cannot determine the number of sectors on this volume.");
        } else {
            DebugPrint("Sectors is 0");
        }
        return GeneralError;
    }

    return NoError;
}


BOOLEAN
VOL_LIODPDRV::MarkBad(
    IN      __int64   firstPhysicalDriveSector,
    IN      __int64   lastPhysicalDriveSector,
    IN OUT  PMESSAGE    Message
)
{
    if (!_sa) {
        return FALSE;
    }

    return _sa->MarkBad(firstPhysicalDriveSector, lastPhysicalDriveSector, Message);
}