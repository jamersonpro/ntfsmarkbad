#include "stdafx.h"

/*++

Module Name:

    upfile.cxx

Abstract:

    This module contains the declarations for the NTFS_UPCASE_FILE
    class, which models the upcase-table file for an NTFS volume.
    This class' main purpose in life is to encapsulate the creation
    of this file.

--*/



#include "ulib.hxx"

#include "untfs.hxx"

#include "ntfsbit.hxx"
#include "drive.hxx"
#include "attrib.hxx"
#include "bitfrs.hxx"
#include "upfile.hxx"
#include "upcase.hxx"
#include "message.hxx"


DEFINE_CONSTRUCTOR( NTFS_UPCASE_FILE,
                             NTFS_FILE_RECORD_SEGMENT );

 
NTFS_UPCASE_FILE::~NTFS_UPCASE_FILE(
    )
{
    Destroy();
}


VOID
NTFS_UPCASE_FILE::Construct(
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
NTFS_UPCASE_FILE::Destroy(
    )
/*++

Routine Description:

    Clean up an NTFS_UPCASE_FILE object in preparation for
    destruction or reinitialization.

Arguments:

    None.

Return Value:

    None.

--*/
{
}


 
 
BOOLEAN
NTFS_UPCASE_FILE::Initialize(
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft
    )
/*++

Routine Description:

    This method initializes an Upcase File object.  The only special
    knowledge that it adds to the File Record Segment initialization
    is the location within the Master File Table of the Upcase table
    file.

Arguments:

    Mft             -- Supplies the volume MasterFile Table.
    UpcaseTable     -- Supplies the volume upcase table.

Return Value:

    TRUE upon successful completion

Notes:

    This class is reinitializable.


--*/
{
    Destroy();

    return( NTFS_FILE_RECORD_SEGMENT::Initialize( UPCASE_TABLE_NUMBER,
                                                  Mft ) );
}

