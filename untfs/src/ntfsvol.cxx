#include "stdafx.h"






#include "ulib.hxx"
#include "ntfsvol.hxx"

#include "bitfrs.hxx"
#include "mft.hxx"
#include "upfile.hxx"
#include "upcase.hxx"

#include "message.hxx"
#include "mftfile.hxx"

#include "wstring.hxx"


DEFINE_CONSTRUCTOR( NTFS_VOL, VOL_LIODPDRV );


VOID
NTFS_VOL::Construct (
    )

/*++

Routine Description:

    Constructor for NTFS_VOL.

Arguments:

    None.

Return Value:

    None.

--*/
{
    // unreferenced parameters
    (void)(this);
}


VOID
NTFS_VOL::Destroy(
    )
/*++

Routine Description:

    This routine returns a NTFS_VOL object to its initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    // unreferenced parameters
    (void)(this);
}


NTFS_VOL::~NTFS_VOL(
    )
/*++

Routine Description:

    Destructor for NTFS_VOL.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}

FORMAT_ERROR_CODE
NTFS_VOL::Initialize(
    IN      PCWSTRING   NtDriveName,
    IN OUT  PMESSAGE    Message  
    )
/*++

Routine Description:

    This routine initializes a NTFS_VOL object.

Arguments:

    NtDriveName     - Supplies the drive path for the volume.
    Message         - Supplies an outlet for messages.
    ExclusiveWrite  - Supplies whether or not the drive should be
                        opened for exclusive write.
    FormatMedia     - Supplies whether or not to format the media.
    MediaType       - Supplies the type of media to format to.
    ForceDismount   - Supplies whether the volume should be dismounted
                        and locked

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    MESSAGE             msg;
    FORMAT_ERROR_CODE   errcode;

    Destroy();

    errcode = VOL_LIODPDRV::Initialize(NtDriveName, &_ntfssa, Message);

    if (errcode != NoError)
        return errcode;

    if (!Message) 
    {
        Message = &msg;
    }

    if (!_ntfssa.Initialize(this, Message)) {
        return GeneralError;
    }

    if (!_ntfssa.Read(Message)) {
        return GeneralError;
    }

    return NoError;
}


