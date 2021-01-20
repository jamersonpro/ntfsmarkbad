#include "stdafx.h"

/*++

Module Name:

	bitfrs.cxx

Abstract:

	This module contains the member function definitions for
    the NTFS_BITMAP_FILE class.

--*/



#include "ulib.hxx"

#include "untfs.hxx"

#include "ntfsbit.hxx"
#include "drive.hxx"
#include "attrib.hxx"
#include "bitfrs.hxx"

DEFINE_CONSTRUCTOR( NTFS_BITMAP_FILE, NTFS_FILE_RECORD_SEGMENT   );

 
NTFS_BITMAP_FILE::~NTFS_BITMAP_FILE(
	)
{
	Destroy();
}


VOID
NTFS_BITMAP_FILE::Construct(
	)
/*++

Routine Description:

	Worker function for the construtor.

Arguments:

	None.

Return Value:

	None.

--*/
{
}


VOID
NTFS_BITMAP_FILE::Destroy(
	)
/*++

Routine Description:

	Clean up an NTFS_BITMAP_FILE object in preparation for
	destruction or reinitialization.

Arguments:

	None.

Return Value:

	None.

--*/
{
}

 
BOOLEAN
NTFS_BITMAP_FILE::Initialize(
	IN OUT  PNTFS_MASTER_FILE_TABLE	Mft
	)
/*++

Routine Description:

    This method initializes a Bitmap File object.
    The only special knowledge that it adds to the File Record Segment
    initialization is the location within the Master File Table of the
    Bitmap File.

Arguments:

	Mft 			-- Supplies the volume MasterFile Table.

Return Value:

	TRUE upon successful completion

Notes:

	This class is reinitializable.


--*/
{
    Destroy();

    return( NTFS_FILE_RECORD_SEGMENT::Initialize( BIT_MAP_FILE_NUMBER,
                                                  Mft ) );
}

