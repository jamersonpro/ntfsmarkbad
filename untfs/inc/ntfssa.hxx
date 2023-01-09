/*++

Module Name:

    ntfssa.hxx

Abstract:

    This class supplies the NTFS-only SUPERAREA methods.

--*/

#pragma once

#include "supera.hxx"
#include "hmem.hxx"
#include "untfs.hxx"
#include "message.hxx"
#include "ntfsbit.hxx"
#include "numset.hxx"

DECLARE_CLASS( NTFS_INDEX_TREE );

#include "indxtree.hxx"


DECLARE_CLASS( LOG_IO_DP_DRIVE );
DECLARE_CLASS( WSTRING );
DECLARE_CLASS( NTFS_MASTER_FILE_TABLE );
DECLARE_CLASS( NTFS_BITMAP );
DECLARE_CLASS( NTFS_FILE_RECORD_SEGMENT );
DECLARE_CLASS( NTFS_ATTRIBUTE );
DECLARE_CLASS( NTFS_FRS_STRUCTURE );
DECLARE_CLASS( NTFS_ATTRIBUTE_LIST );
DECLARE_CLASS( CONTAINER );
DECLARE_CLASS( SEQUENTIAL_CONTAINER );
DECLARE_CLASS( LIST );
DECLARE_CLASS( NTFS_EXTENT_LIST );
DECLARE_CLASS( NUMBER_SET );
DECLARE_CLASS( NTFS_UPCASE_TABLE );
DECLARE_CLASS( NTFS_MFT_FILE );

//
// Types of message for SynchronizeMft()
//
enum MessageMode {
        CorrectMessage = 0,
        SuppressMessage,
        UpdateMessage
};

// This global variable used by CHKDSK to compute the largest
// LSN on the volume.

extern LSN              LargestLsnEncountered;
extern LARGE_INTEGER    LargestUsnEncountered;
extern ULONG64          FrsOfLargestUsnEncountered;

CONST ULONG LsnResetThreshholdHighPart = 0x10000;

CONST UCHAR UpdateSequenceArrayCheckValueMinorError = 2;// should always be non-zero
CONST UCHAR UpdateSequenceArrayCheckValueOk = 1;        // should always be non-zero


class NTFS_SA : public SUPERAREA {

public:

    BOOLEAN
        MarkInFreeSpace(
            IN OUT  PNTFS_MASTER_FILE_TABLE Mft,
            IN      const std::vector<sectors_range>& physicalDriveSectorsTargets,
            IN OUT  PNUMBER_SET             BadClusters,
            IN      PNTFS_BAD_CLUSTER_FILE  BadClusterFile,
            IN OUT  PMESSAGE                Message
        );

    VIRTUAL
        BOOLEAN
        MarkBad(
            IN const std::vector<sectors_range>& physicalDriveSectorsTargets,
            IN OUT  PMESSAGE    Message
        );



    DECLARE_CONSTRUCTOR(NTFS_SA);

    VIRTUAL
        ~NTFS_SA(
        );



    BOOLEAN
        Initialize(
            IN OUT  PLOG_IO_DP_DRIVE    Drive,
            IN OUT  PMESSAGE            Message,
            IN      LCN                 CvtStartZone    DEFAULT 0,
            IN      BIG_INT             CvtZoneSize     DEFAULT 0
        );

    VIRTUAL
        PVOID
        GetBuf(
        );


    BOOLEAN
        Read(
        );


    BOOLEAN
        Read(
            IN OUT  PMESSAGE    Message
        );

    VIRTUAL
        BOOLEAN
        Write(
        );

    VIRTUAL
        BOOLEAN
        Write(
            IN OUT  PMESSAGE    Message
        );


    BIG_INT
        QueryVolumeSectors(
        ) CONST;




    UCHAR
        QueryClusterFactor(
        ) CONST;


    LCN
        QueryMftStartingLcn(
        ) CONST;


    LCN
        QueryMft2StartingLcn(
        ) CONST;




    ULONG
        QueryFrsSize(
        ) CONST;




    USHORT
        QueryVolumeFlags(
            OUT PBOOLEAN    CorruptVolume   DEFAULT NULL,
            OUT PUCHAR      MajorVersion    DEFAULT NULL,
            OUT PUCHAR      MinorVersion    DEFAULT NULL
        );


    STATIC
        UCHAR
        PostReadMultiSectorFixup(
            IN OUT  PVOID               MultiSectorBuffer,
            IN      ULONG               BufferSize,
            IN OUT  PIO_DP_DRIVE        Drive,
            IN      ULONG               VaildSize   DEFAULT         MAXULONG
        );

    STATIC
        VOID
        PreWriteMultiSectorFixup(
            IN OUT  PVOID   MultiSectorBuffer,
            IN      ULONG   BufferSize
        );



    STATIC
        VOID
        SetVersionNumber(
            IN  UCHAR   Major,
            IN  UCHAR   Minor
        );

    STATIC
        VOID
        QueryVersionNumber(
            OUT PUCHAR  Major,
            OUT PUCHAR  Minor
        );


private:


    HMEM                    _hmem;          // memory for SECRUN
    PPACKED_BOOT_SECTOR     _boot_sector;   // packed boot sector
    BIOS_PARAMETER_BLOCK    _bpb;           // unpacked BPB
    BIG_INT                 _boot2;         // alternate boot sector
    BIG_INT                 _boot3;         // second alternate boot sector
    UCHAR                   _NumberOfStages;// minimum number of stages for chkdsk
                                            //   to go thru

    VOID
        Construct(
        );


    VOID
        Destroy(
        );

    BOOLEAN                 _cleanup_that_requires_reboot;
    LCN                     _cvt_zone;      // convert region for mft, logfile, etc.
    BIG_INT                 _cvt_zone_size; // convert region size in terms of clusters

    // This version number is used to determine what format
    // is used for the compressed mapping pairs of sparse
    // files.  Ideally, this information should be tracked
    // on a per-volume basis; however, that would require
    // extensive changes to the UNTFS class interfaces.
    //
    STATIC UCHAR            _MajorVersion, _MinorVersion;
};

INLINE
PVOID
NTFS_SA::GetBuf(
    )
/*++

Routine Description:

    This routine returns a pointer to the write buffer for the NTFS
    SUPERAREA.  This routine also packs the bios parameter block.

Arguments:

    None.

Return Value:

    A pointer to the write buffer.

--*/
{
    PackBios(&_bpb, &(_boot_sector->PackedBpb));
    return SECRUN::GetBuf();
}

INLINE
BOOLEAN
NTFS_SA::Write(
    )
/*++

Routine Description:

    This routine simply calls the other write with the default message
    object.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    MESSAGE msg;

    return Write(&msg);
}


INLINE
BIG_INT
NTFS_SA::QueryVolumeSectors(
    ) CONST
/*++

Routine Description:

    This routine returns the number of sectors on the volume as recorded in
    the boot sector.

Arguments:

    None.

Return Value:

    The number of volume sectors.

--*/
{
    return _boot_sector->NumberSectors;
}


INLINE
LCN
NTFS_SA::QueryMftStartingLcn(
    ) CONST
/*++

Routine Description:

    This routine returns the starting logical cluster number
    for the Master File Table.

Arguments:

    None.

Return Value:

    The starting LCN for the MFT.

--*/
{
    return _boot_sector->MftStartLcn;
}


INLINE
LCN
NTFS_SA::QueryMft2StartingLcn(
    ) CONST
/*++

Routine Description:

    This routine returns the starting logical cluster number
    for the mirror of the Master File Table.

Arguments:

    None.

Return Value:

    The starting LCN for the mirror of the MFT.

--*/
{
    return _boot_sector->Mft2StartLcn;
}


INLINE
ULONG
NTFS_SA::QueryFrsSize(
    ) CONST
/*++

Routine Description:

    This routine computes the number of clusters per file record segment.

Arguments:

    None.

Return Value:

    The number of clusters per file record segment.

--*/
{
    if (_boot_sector->ClustersPerFileRecordSegment < 0) {

        LONG temp = LONG(_boot_sector->ClustersPerFileRecordSegment);

        return 1 << -temp;
    }

    return _boot_sector->ClustersPerFileRecordSegment *
         _bpb.SectorsPerCluster * _drive->QuerySectorSize();
}

