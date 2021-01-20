/*++

Module Name:

        badfile.hxx

Abstract:

        This module contains the declarations for the NTFS_BAD_CLUSTER_FILE
        class, which models the bad cluster file for an NTFS volume.

        The DATA attribute of the bad cluster file is a non-resident
        attribute to which bad clusters are allocated.  It is stored
        as a sparse file with LCN = VCN.

--*/

#pragma once

#include "frs.hxx"

DECLARE_CLASS( IO_DP_DRIVE );
DECLARE_CLASS( NTFS_ATTRIBUTE);
DECLARE_CLASS( NTFS_MASTER_FILE_TABLE );
DECLARE_CLASS( NTFS_BITMAP );
DECLARE_CLASS( NUMBER_SET );

class NTFS_BAD_CLUSTER_FILE : public NTFS_FILE_RECORD_SEGMENT {

public:

    DECLARE_CONSTRUCTOR(NTFS_BAD_CLUSTER_FILE);


    VIRTUAL
    ~NTFS_BAD_CLUSTER_FILE(
    );
 
    BOOLEAN
    Initialize(
        IN OUT  PNTFS_MASTER_FILE_TABLE Mft
    );

 
    BOOLEAN
    Add(
        IN LCN Lcn
    );

 
    BOOLEAN
    Add(
        IN PCNUMBER_SET ClustersToAdd
    );

 
    BOOLEAN
    AddRun(
        IN LCN          Lcn,
        IN BIG_INT      RunLength
    );

 
    BOOLEAN
    IsInList(
        IN LCN Lcn
    );

 
    BOOLEAN
    Flush(
        IN OUT  PNTFS_BITMAP        Bitmap,
        IN OUT  PNTFS_INDEX_TREE    ParentIndex DEFAULT NULL
    );

private:

    VOID
    Construct(
    );

 
    VOID
    Destroy(
    );

    PNTFS_ATTRIBUTE _DataAttribute;
};


