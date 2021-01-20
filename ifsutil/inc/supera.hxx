/*++

Module Name:

    supera.hxx

Abstract:

    This class models the root of a file system.  This abstract class is
    currently the base class of an HPFS and a FAT super area.

--*/

#pragma once

#include "secrun.hxx"
#include "volume.hxx"
#include "ifsentry.hxx"

//

DECLARE_CLASS( SUPERAREA );
DECLARE_CLASS( NUMBER_SET );
DECLARE_CLASS( MESSAGE );
DECLARE_CLASS( WSTRING );


class SUPERAREA : public SECRUN {

public:

	VIRTUAL
	BOOLEAN
	MarkBad(
		IN __int64 firstPhysicalDriveSector,
		IN __int64 lastPhysicalDriveSector,
		IN OUT PMESSAGE Message
	) PURE;


	VIRTUAL
	~SUPERAREA(
	);

	VIRTUAL
	PVOID
	GetBuf(
	);

	 
	PIO_DP_DRIVE
	GetDrive(
	);


protected:

    DECLARE_CONSTRUCTOR(SUPERAREA);

     
    BOOLEAN
    Initialize(
        IN OUT  PMEM                Mem,
        IN OUT  PLOG_IO_DP_DRIVE    Drive,
        IN      SECTORCOUNT         NumberOfSectors,
        IN OUT  PMESSAGE            Message
    );


    PLOG_IO_DP_DRIVE    _drive;


private:

    VOID
    Construct(
    );

 
    VOID
    Destroy(
    );

};


INLINE
PVOID
SUPERAREA::GetBuf(
    )
/*++

Routine Description:

    This routine returns a pointer to the beginning of the read/write
    buffer.

Arguments:

    None.

Return Value:

    A pointer to a read/write buffer.

--*/
{
    return SECRUN::GetBuf();
}


INLINE
PIO_DP_DRIVE
SUPERAREA::GetDrive(
    )
/*++

Routine Description:

    Retrieve the drive object.

Arguments:

    N/A

Return Value:

    The drive object.

--*/
{
    return _drive;
}

