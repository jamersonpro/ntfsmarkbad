#include "stdafx.h"


/*++

Module Name:

    mftfile.hxx

Abstract:

    This module contains the member function definitions for the
    NTFS_MFT_FILE class.


Notes:

    The MFT and the Volume Bitmap:

        The Master File Table needs the bitmap to extend itself.  The
        volume bitmap can be passed in upon initialization, or it can
        be supplied (using SetVolumeBitmap) at any time.  However,
        until it is supplied, the Master File Table is unable to grow
        itself.

--*/




#include "ulib.hxx"

#include "untfs.hxx"

#include "drive.hxx"
#include "attrib.hxx"
#include "ntfsbit.hxx"
#include "mftfile.hxx"
#include "clusrun.hxx"
#include "indxtree.hxx"

#define LOGFILE_PLACEMENT_V1    1

DEFINE_CONSTRUCTOR( NTFS_MFT_FILE, NTFS_FILE_RECORD_SEGMENT   );

 
NTFS_MFT_FILE::~NTFS_MFT_FILE(
    )
{
    Destroy();
}


VOID
NTFS_MFT_FILE::Construct(
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
    _FirstLcn = 0;
    _VolumeBitmap = NULL;
}


VOID
NTFS_MFT_FILE::Destroy(
    )
/*++

Routine Description:

    Clean up an NTFS_MFT_FILE object in preparation for
    destruction or reinitialization.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _FirstLcn = 0;
    _VolumeBitmap = NULL;
}


 
BOOLEAN
NTFS_MFT_FILE::Initialize(
    IN OUT  PLOG_IO_DP_DRIVE Drive,
    IN      LCN Lcn,
    IN      ULONG ClusterFactor,
    IN      ULONG FrsSize,
    IN      BIG_INT VolumeSectors,
    IN OUT  PNTFS_BITMAP VolumeBitmap,
    IN      PNTFS_UPCASE_TABLE  UpcaseTable
    )
/*++

Routine Description:

    Initialize an NTFS_MFT_FILE object.

Arguments:

    Drive           -- supplies the Drive on which the file table resides
    Lcn             -- supplies the logical cluster number of the master
                        file table entry which describes the master file
                        table itself.
    ClusterFactor   -- supplies the number of sectors per cluster.
    FrsSize         -- supplies the number of bytes per File Record
                        Segment in this MFT.
    VolumeSectors   -- supplies the number of volume sectors.
    VolumeBitmap    -- supplies the bitmap for the volume.  This parameter
                        may be NULL.

Return Value:

    TRUE upon successful completion.

--*/
{
    ULONG MirroredClusters;
    ULONG ClusterSize;

    Destroy();

    DebugPtrAssert( Drive );

    _FirstLcn = Lcn;
    _VolumeBitmap = VolumeBitmap;

    ClusterSize = Drive->QuerySectorSize() * ClusterFactor;

    MirroredClusters = (REFLECTED_MFT_SEGMENTS * FrsSize + (ClusterSize - 1))
        / ClusterSize;

    if( !_MirrorMem.Initialize() ||
        !_MirrorClusterRun.Initialize( &_MirrorMem,
                                       Drive,
                                       0,
                                       ClusterFactor,
                                       MirroredClusters ) ) {

        DebugPrint( "Can't initialize MFT helper cluster run.\n" );
        Destroy();
        return FALSE;
    }

    if (!_Mft.Initialize(&_DataAttribute, &_MftBitmap, VolumeBitmap,
                         UpcaseTable, ClusterFactor, FrsSize,
                         Drive->QuerySectorSize(), VolumeSectors)) {

        return FALSE;
    }

    _Mft.DisableMethods();


    if (!NTFS_FILE_RECORD_SEGMENT::Initialize(Drive,
                                              Lcn,
                                              &_Mft) ) {

        Destroy();
        return FALSE;
    }



    return TRUE;
}

 
BOOLEAN
NTFS_MFT_FILE::Read(
    )
/*++

Routine Description:

    This routine reads this FRS for the MFT and then proceeds
    to read the MFT bitmap.  If all goes well, the internal
    data attribute and MFT bitmap will be initialized.

    This method will return TRUE if and only if the base FRS for
    this MFT is correctly read in.  The MFT allocation methods will
    be enabled by this method if and only if MFT bitmap and the
    MFT data attribute are properly read in and initialized.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    NTFS_ATTRIBUTE  MftBitmapAttribute;
    BIG_INT         DataAttributeWrittenLength;
    BIG_INT         DataAttributeAllocatedLength;
    BIG_INT         NumberOfAllocatedFileRecordSegments,
                    NumberOfWrittenFileRecordSegments;
    ULONG           RequiredBitsInBitmap, PreferredBitsInBitmap;
    BOOLEAN         Error;

    _Mft.DisableMethods();

    if (!NTFS_FILE_RECORD_SEGMENT::Read()) {
        return FALSE;
    }

    _Mft.EnableMethods();

    // Make sure that we have the Data Attribute to play with.

    if (!QueryAttribute(&_DataAttribute, &Error, $DATA)) {

        _Mft.DisableMethods();
        return TRUE;
    }


    // The length of the bitmap depends on the allocated length of
    // the data attribute.

    _DataAttribute.QueryValueLength( &DataAttributeWrittenLength,
                                     &DataAttributeAllocatedLength );

    // A quick sanity check:
    //
    if( CompareGT(DataAttributeWrittenLength, DataAttributeAllocatedLength) ) {

        DebugAbort( "UNTFS: MFT Data attribute is corrupt.\n" );
        _Mft.DisableMethods();
        return TRUE;
    }

    NumberOfWrittenFileRecordSegments = DataAttributeWrittenLength / QuerySize();
    NumberOfAllocatedFileRecordSegments = DataAttributeAllocatedLength / QuerySize();

    DebugAssert( NumberOfWrittenFileRecordSegments.GetHighPart() == 0 );
    DebugAssert( NumberOfAllocatedFileRecordSegments.GetHighPart() == 0 );

    RequiredBitsInBitmap = NumberOfWrittenFileRecordSegments.GetLowPart();
    PreferredBitsInBitmap = NumberOfAllocatedFileRecordSegments.GetLowPart();

    // Create a bitmap, and get the MFT bitmap attribute through
    // which we read it, and read the bitmap.

    if( !_MftBitmap.Initialize( RequiredBitsInBitmap, TRUE ) ||
        !QueryAttribute( &MftBitmapAttribute, &Error, $BITMAP ) ||
        !_MftBitmap.Read( &MftBitmapAttribute ) ||
        !_MftBitmap.Resize( PreferredBitsInBitmap ) ) {

        DebugAbort( "Cannot read MFT Bitmap.\n" );
        _Mft.DisableMethods();
        return TRUE;
    }

    return TRUE;
}


 
BOOLEAN
NTFS_MFT_FILE::Flush(
    )
/*++

Routine Description:

    This method flushes the MFT--re-inserts the DATA attribute (if
    necessary); writes the MFT bitmap, and writes the MFT's own
    File Record Segment.

Arguments:

    None.

Return Value:

    TRUE upon successful completion.

Notes:

    This method will also write the volume bitmap and the mft mirror.
    It will resize the $DATA attributes on the bitmap and mirror files
    and write those FRS's, if necessary.

--*/
{
    NTFS_BITMAP_FILE BitmapFile;
    NTFS_REFLECTED_MASTER_FILE_TABLE MirrorFile;
    NTFS_INDEX_TREE RootIndex;
    NTFS_FILE_RECORD_SEGMENT RootIndexFrs;
    DSTRING FileNameIndexName;

    NTFS_ATTRIBUTE MftBitmapAttribute,
                   MirrorDataAttribute,
                   VolumeBitmapAttribute;

    LCN FirstMirrorLcn;
    BIG_INT OldValidLength;
    BOOLEAN Error;

    if( !_Mft.AreMethodsEnabled() ) {

        DebugAbort( "Tried to flush the MFT before enabling it.\n" );
        return FALSE;
    }


    // Ensure that the bitmap file and mirror file's $DATA attributes
    // are the correct sizes.  This will later allow us to write these
    // two constructs without affecting their respective FRS's.

    if( !BitmapFile.Initialize( &_Mft ) ||
        !BitmapFile.Read() ||
        !BitmapFile.QueryAttribute( &VolumeBitmapAttribute,
                                    &Error,
                                    $DATA ) ||
        !_VolumeBitmap->CheckAttributeSize( &VolumeBitmapAttribute,
                                           _VolumeBitmap ) ||
        !MirrorFile.Initialize( &_Mft ) ||
        !MirrorFile.Read() ||
        !MirrorFile.QueryAttribute( &MirrorDataAttribute,
                                    &Error,
                                    $DATA ) ||
        !CheckMirrorSize( &MirrorDataAttribute,
                          TRUE,
                          _VolumeBitmap,
                          &FirstMirrorLcn ) ) {

        DebugPrint( "Cannot check size of bitmap & mirror attributes.\n" );
        return FALSE;
    }


    if( VolumeBitmapAttribute.IsStorageModified() &&
        ( !VolumeBitmapAttribute.InsertIntoFile( &BitmapFile,
                                                 _VolumeBitmap ) ||
          !BitmapFile.Flush( _VolumeBitmap ) ) ) {

        DebugPrint( "Cannot save volume bitmap attribute.\n" );
        return FALSE;
    }

    if( MirrorDataAttribute.IsStorageModified() &&
        ( !MirrorDataAttribute.InsertIntoFile( &MirrorFile, _VolumeBitmap ) ||
          !MirrorFile.Flush( _VolumeBitmap ) ) ) {

        DebugPrint( "Cannot save MFT mirror data attribute.\n" );
        return FALSE;
    }

    // Fetch the root index from its FRS.
    //
    if( !RootIndexFrs.Initialize( ROOT_FILE_NAME_INDEX_NUMBER, this ) ||
        !RootIndexFrs.Read() ||
        !FileNameIndexName.Initialize( FileNameIndexNameData ) ||
        !RootIndex.Initialize( GetDrive(),
                               QueryClusterFactor(),
                               _VolumeBitmap,
                               GetUpcaseTable(),
                               QuerySize()/2,
                               &RootIndexFrs,
                               &FileNameIndexName ) ) {

        return FALSE;
    }

    // Fetch the MFT Bitmap attribute.
    //
    if( !QueryAttribute( &MftBitmapAttribute, &Error, $BITMAP ) ) {

        DebugPrintTrace(( "UNTFS: Cannot fetch MFT bitmap attribute.\n" ));
        return FALSE;
    }

    // Write the bitmap once to make it the right size.  If
    // it grows while we're saving the MFT, we'll have to
    // write it again.
    //
    if( !_MftBitmap.Write( &MftBitmapAttribute, _VolumeBitmap ) ) {

        DebugPrintTrace(( "UNTFS: Cannot write the MFT bitmap.\n" ));
        return FALSE;
    }

    do {

        // Note that inserting an attribute into a file resets the
        // attribute's StorageModified flag.
        //
        if( MftBitmapAttribute.IsStorageModified() &&
            !MftBitmapAttribute.InsertIntoFile( this, NULL ) ) {

            DebugPrint( "UNTFS: Cannot save MFT bitmap attribute.\n" );
            return FALSE;
        }

        // Remember the bitmap size (which is equal to the number
        // of FRS's in the MFT).
        //
        OldValidLength = _DataAttribute.QueryValidDataLength();

        // Save the data attribute.
        //
        if( _DataAttribute.IsStorageModified() &&
            !_DataAttribute.InsertIntoFile(this, NULL) ) {

            DebugAbort( "UNTFS: Cannot save MFT's data attribute.\n" );
            return FALSE;
        }

        // Flush this FRS.
        //
        if( !NTFS_FILE_RECORD_SEGMENT::Flush( _VolumeBitmap, &RootIndex) ) {

            return FALSE;
        }

        // Write the bitmap again, in case it changed.
        //
        if( !_MftBitmap.Write( &MftBitmapAttribute, _VolumeBitmap ) ) {

            DebugPrintTrace(( "UNTFS: Cannot write the MFT bitmap.\n" ));
            return FALSE;
        }

        // If the MFT's Valid Data Length changed while we were
        // saving the data attribute and bitmap, we have to go
        // through this loop again.

    } while( OldValidLength != _DataAttribute.QueryValidDataLength() );

    // Save the root index:
    //
    if( !RootIndex.Save( &RootIndexFrs ) ||
        !RootIndexFrs.Flush( _VolumeBitmap ) ) {

        return FALSE;
    }

    if( !_VolumeBitmap->Write( &VolumeBitmapAttribute, NULL ) ||
        !WriteMirror( &MirrorDataAttribute ) ){

        DebugPrint( "Failed write of MFT Mirror or volume bitmap.\n" );
        return FALSE;
    }

    return TRUE;
}


BOOLEAN
NTFS_MFT_FILE::CheckMirrorSize(
    IN OUT PNTFS_ATTRIBUTE  MirrorDataAttribute,
    IN     BOOLEAN          Fix,
    IN OUT PNTFS_BITMAP     VolumeBitmap,
    OUT    PLCN             FirstLcn
    )
/*++

Routine Description:

    This method checks that the MFT Mirror $DATA attribute is the
    correct size and contiguous.  It can also be used to check these
    restrictions.

Arguments:

    MirrorDataAttribut  --  Supplies the MFT Mirror's $DATA attribute.
    Fix                 --  Supplies a flag which indicates that the
                            attribute should be reallocated if it is
                            the wrong size or not contiguous.
    VolumeBitmap        --  Supplies the volume bitmap (only required
                            if Fix is TRUE).
    FirstLcn            --  Receives the starting LCN of the mirror.

Return Value:

    TRUE upon successful completion.

--*/
{
    BIG_INT RunLength;
    LCN     NewStartingLcn;
    ULONG   MirroredClusters;
    ULONG   ClusterSize;

    ClusterSize = QueryClusterFactor() * _Mft.QuerySectorSize();

    MirroredClusters = (REFLECTED_MFT_SEGMENTS * QuerySize() + (ClusterSize - 1))
         / ClusterSize;

    if( MirrorDataAttribute->QueryLcnFromVcn( 0, FirstLcn, &RunLength ) &&
        *FirstLcn != 0 &&
        *FirstLcn != LCN_NOT_PRESENT &&
        RunLength >= MirroredClusters ) {

        // Everything is perfect.

        return TRUE;
    }

    // Something is not perfect.

    if( Fix &&
        VolumeBitmap->AllocateClusters( QueryVolumeSectors()/
                                            QueryClusterFactor()/
                                            2,
                                        MirroredClusters,
                                        &NewStartingLcn ) &&
        MirrorDataAttribute->Resize( 0, VolumeBitmap ) &&
        MirrorDataAttribute->AddExtent( 0,
                                        NewStartingLcn,
                                        MirroredClusters ) ) {

        // It was broken, but now it's perfect.

        *FirstLcn = NewStartingLcn;
        return TRUE;
    }

    return FALSE;
}


BOOLEAN
NTFS_MFT_FILE::WriteMirror(
    IN OUT PNTFS_ATTRIBUTE  MirrorDataAttribute
    )
/*++

Routine Description:

    This method writes the MFT Mirror.  Note that it will fail if
    the mirror's $DATA attribute is not the correct size or is
    not contiguous.

Arguments:

    MirrorDataAttribute --  Supplies the MFT Mirror's $DATA attribute.

Return Value:

    TRUE upon successful completion.

Notes:

    This method copies whatever is _on disk_ in the MFT's data attribute
    to the mirror's data attribute.  Therefore, it should only be called
    after the MFT itself has been written.

--*/
{
    LCN FirstMirrorLcn;

    if( !CheckMirrorSize( MirrorDataAttribute,
                          FALSE,
                          NULL,
                          &FirstMirrorLcn ) ) {

        return FALSE;
    }

    _MirrorClusterRun.Relocate( _FirstLcn );

    if( !_MirrorClusterRun.Read() ) {

        return FALSE;
    }

    _MirrorClusterRun.Relocate( FirstMirrorLcn );

    if( !_MirrorClusterRun.Write() ) {

        return FALSE;
    }

    return TRUE;
}
