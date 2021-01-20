#include "stdafx.h"



#include "ulib.hxx"


#include "supera.hxx"
#include "message.hxx"

#include "ifssys.hxx"


DEFINE_CONSTRUCTOR( SUPERAREA, SECRUN   );

 
SUPERAREA::~SUPERAREA(
    )
/*++

Routine Description:

    Destructor for SUPERAREA.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


VOID
SUPERAREA::Construct(
        )
/*++

Routine Description:

        Constructor for SUPERAREA.

Arguments:

        None.

Return Value:

        None.

--*/
{
    _drive = NULL;
}


VOID
SUPERAREA::Destroy(
    )
/*++

Routine Description:

    This routine returns the object to its initial state freeing up
    any memory in the process.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _drive = NULL;
}


 
BOOLEAN
SUPERAREA::Initialize(
    IN OUT  PMEM                Mem,
    IN OUT  PLOG_IO_DP_DRIVE    Drive,
    IN      SECTORCOUNT         NumberOfSectors,
    IN OUT  PMESSAGE            Message
)
/*++

Routine Description:

    This routine initializes the SUPERAREA for the given drive.

Arguments:

    Mem             - Supplies necessary memory for the underlying sector run.
    Drive           - Supplies the drive where the superarea resides.
    NumberOfSectors - Supplies the number of sectors in the superarea.
    Message         - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    Destroy();

    DebugAssert(Mem);
    DebugAssert(Drive);
    DebugAssert(NumberOfSectors);

    if (!SECRUN::Initialize(Mem, Drive, 0, NumberOfSectors))
    {
        Message->Out("Insufficient memory.");
        return FALSE;
    }

    _drive = Drive;

    return TRUE;
}

