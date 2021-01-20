#include "stdafx.h"

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

#include "ulib.hxx"

#include "untfs.hxx"

#include "drive.hxx"
#include "numset.hxx"

#include "ntfsbit.hxx"
#include "mft.hxx"
#include "attrrec.hxx"
#include "attrib.hxx"

#include "badfile.hxx"
#include "ifssys.hxx"
#include "message.hxx"


#define BadfileDataNameData "$Bad"


DEFINE_CONSTRUCTOR( NTFS_BAD_CLUSTER_FILE, NTFS_FILE_RECORD_SEGMENT   );

 
NTFS_BAD_CLUSTER_FILE::~NTFS_BAD_CLUSTER_FILE(
    )
{
    Destroy();
}

VOID
NTFS_BAD_CLUSTER_FILE::Construct(
    )
/*++

Routine Description:

    Worker method for NTFS_BAD_CLUSTER_FILE construction.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _DataAttribute = NULL;
}

VOID
NTFS_BAD_CLUSTER_FILE::Destroy(
    )
/*++

Routine Description:

    Worker method for NTFS_BAD_CLUSTER_FILE destruction.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DELETE( _DataAttribute );
}

 
 
BOOLEAN
NTFS_BAD_CLUSTER_FILE::Initialize(
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft
    )
/*++

Routine Description:

    This method initializes an NTFS_BAD_CLUSTER_FILE object.

Arguments:

    Mft             -- Supplies the volume MasterFile Table.

--*/
{
    Destroy();

    return( NTFS_FILE_RECORD_SEGMENT::
                Initialize( BAD_CLUSTER_FILE_NUMBER,
                            Mft ) );
}

  
BOOLEAN
NTFS_BAD_CLUSTER_FILE::Add(
    IN  LCN Lcn
    )
/*++

Routine Description:

    This method adds a cluster to the Bad Cluster List.  Note that it
    does not mark it as used in the volume bitmap.

Arguments:

    Lcn -- supplies the LCN of the bad cluster

Return Value:

    TRUE upon successful completion.

--*/
{
    return( AddRun( Lcn, 1 ) );
}


 
BOOLEAN
NTFS_BAD_CLUSTER_FILE::Add(
    IN  PCNUMBER_SET    ClustersToAdd
    )
/*++

Routine Description:

    This method adds a set of clusters to the Bad Cluster List.  Note
    that it does not mark them as used in the volume bitmap.

Arguments:

    BadClusters --  Supplies the clusters to be added to the
                    bad cluster file.

Return Value:

    TRUE upon successful completion.

--*/
{
    BIG_INT NumberOfClustersToAdd;
    LCN CurrentLcn;
    ULONG i;

    NumberOfClustersToAdd = ClustersToAdd->QueryCardinality();

    for( i = 0; i < NumberOfClustersToAdd; i++ ) 
    {
        CurrentLcn = ClustersToAdd->QueryNumber(i);

        if( !IsInList( CurrentLcn ) &&
            !Add( CurrentLcn ) ) 
        {
            return FALSE;
        }
    }

    return TRUE;
}


 
BOOLEAN
NTFS_BAD_CLUSTER_FILE::AddRun(
    IN  LCN     Lcn,
    IN  BIG_INT RunLength
    )
/*++

Routine Description:

    This method adds a run of clusters to the Bad Cluster List.  Note
    that it does not mark these clusters as used in the volume bitmap.

Arguments:

    Lcn         -- supplies the LCN of the first cluster in the run.
    RunLength   -- supplies the number of clusters in the run.

Return Value:

    TRUE upon successful completion.

Notes:

    If LCN is in the range of the volume but the run extends past
    the end of the volume, then the run is truncated.

    If LCN or the RunLength is negative, the run is ignored.  (The
    method succeeds without doing anything in this case.)

--*/
{
    DSTRING DataAttributeName;
    BIG_INT num_clusters;
    BOOLEAN Error;

    num_clusters = QueryVolumeSectors()/QueryClusterFactor();

    if( Lcn < 0             ||
        Lcn >= num_clusters ||
        RunLength < 0 ) 
    {
        return TRUE;
    }

    if (Lcn + RunLength > num_clusters) 
    {
        RunLength = num_clusters - Lcn;
    }

    if( _DataAttribute == NULL &&
        ( !DataAttributeName.Initialize( BadfileDataNameData ) ||
          (_DataAttribute = NEW NTFS_ATTRIBUTE) == NULL ||
          !QueryAttribute( _DataAttribute,
                           &Error,
                           $DATA,
                           &DataAttributeName ) ) ) 
    {
        DELETE( _DataAttribute );
        return FALSE;
    }

    return( _DataAttribute->AddExtent( Lcn, Lcn, RunLength ) );
}

 
BOOLEAN
NTFS_BAD_CLUSTER_FILE::IsInList(
    IN LCN Lcn
    )
/*++

Routine Description:

    This method determines whether a particular LCN is in the bad
    cluster list.

Arguments:

    Lcn --  supplies the LCN in question.

Return Value:

    TRUE if the specified LCN is in the list of bad clusters.

Notes:

    This method cannot be CONST because it may need to fetch the
    data attribute.

--*/
{
    DSTRING DataAttributeName;
    LCN QueriedLcn;
    BOOLEAN Error;

    if( _DataAttribute == NULL &&
        (!DataAttributeName.Initialize( BadfileDataNameData ) ||
          (_DataAttribute = NEW NTFS_ATTRIBUTE) == NULL ||
          !QueryAttribute( _DataAttribute,
                           &Error,
                           $DATA,
                           &DataAttributeName ) ) ) 
    {
        DELETE( _DataAttribute );
        return FALSE;
    }

    if( !_DataAttribute->QueryLcnFromVcn( Lcn, &QueriedLcn ) ||
        QueriedLcn == LCN_NOT_PRESENT ) 
    {
        return FALSE;
    }

    return TRUE;
}


 
BOOLEAN
NTFS_BAD_CLUSTER_FILE::Flush(
    IN OUT  PNTFS_BITMAP        Bitmap,
    IN OUT  PNTFS_INDEX_TREE    ParentIndex
    )
/*++

Routine Description:

    Write the modified bad cluster list to disk.

Arguments:

    Bitmap  -- supplies the volume bitmap.  (May be NULL).

Return Value:

    TRUE upon successful completion.

--*/
{
    if( _DataAttribute != NULL &&
        _DataAttribute->IsStorageModified() &&
        !_DataAttribute->InsertIntoFile( this, Bitmap ) ) {

        return FALSE;
    }

    return( NTFS_FILE_RECORD_SEGMENT::Flush( Bitmap, ParentIndex ) );
}

