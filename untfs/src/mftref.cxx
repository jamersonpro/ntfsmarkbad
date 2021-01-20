#include "stdafx.h"

/*++

Module Name:

    mftref.cxx

Abstract:

    This module contains the member function definitions for
    the NTFS_REFLECTED_MASTER_FILE_TABLE class.  This class
    models the backup copy of the Master File Table.

--*/


#include "ulib.hxx"

#include "untfs.hxx"

#include "drive.hxx"
#include "attrib.hxx"
#include "ntfsbit.hxx"
#include "mftref.hxx"
#include "ifssys.hxx"
#include "numset.hxx"
#include "message.hxx"


DEFINE_CONSTRUCTOR( NTFS_REFLECTED_MASTER_FILE_TABLE,
                    NTFS_FILE_RECORD_SEGMENT   );

 
NTFS_REFLECTED_MASTER_FILE_TABLE::~NTFS_REFLECTED_MASTER_FILE_TABLE(
    )
{
    Destroy();
}


VOID
NTFS_REFLECTED_MASTER_FILE_TABLE::Construct(
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
NTFS_REFLECTED_MASTER_FILE_TABLE::Destroy(
    )
/*++

Routine Description:

    Clean up an NTFS_MASTER_FILE_TABLE object in preparation for
    destruction or reinitialization.

Arguments:

    None.

Return Value:

    None.

--*/
{
}

 
BOOLEAN
NTFS_REFLECTED_MASTER_FILE_TABLE::Initialize(
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft
    )
/*++

Routine Description:

    This method initializes a Master File Table Reflection object.
    The only special knowledge that it adds to the File Record Segment
    initialization is the location within the Master File Table of the
    Master File Table Reflection.

Arguments:

    Mft             -- Supplies the volume MasterFile Table.

Return Value:

    TRUE upon successful completion

Notes:

    This class is reinitializable.


--*/
{
    return( NTFS_FILE_RECORD_SEGMENT::Initialize( MASTER_FILE_TABLE2_NUMBER,
                                                  Mft ) );
}

