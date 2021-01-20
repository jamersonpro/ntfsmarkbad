/*++

Module Name:

    upfile.hxx

Abstract:

    This module contains the declarations for the NTFS_UPCASE_FILE
    class, which models the upcase-table file for an NTFS volume.
    This class' main purpose in life is to encapsulate the creation
    of this file.

--*/

#pragma once

#include "frs.hxx"

DECLARE_CLASS( NTFS_BITMAP );
DECLARE_CLASS( NTFS_UPCASE_TABLE );

class NTFS_UPCASE_FILE : public NTFS_FILE_RECORD_SEGMENT {

public:

    DECLARE_CONSTRUCTOR(NTFS_UPCASE_FILE);

    VIRTUAL
        ~NTFS_UPCASE_FILE(
        );


    BOOLEAN
        Initialize(
            IN OUT  PNTFS_MASTER_FILE_TABLE Mft
        );


private:

    VOID
        Construct(
        );

    VOID
        Destroy(
        );

};


