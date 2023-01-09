#include "stdafx.h"



#include "ulib.hxx"
#include "ntfssa.hxx"
#include "message.hxx"


#include "array.hxx"
#include "arrayit.hxx"

#include "mftfile.hxx"
#include "ntfsbit.hxx"
#include "frs.hxx"
#include "wstring.hxx"
#include "indxtree.hxx"
#include "badfile.hxx"
#include "bitfrs.hxx"
#include "attrib.hxx"
#include "attrrec.hxx"
#include "mft.hxx"
#include "upcase.hxx"
#include "upfile.hxx"
#include "ifssys.hxx"


#include "path.hxx"


UCHAR NTFS_SA::_MajorVersion = NTFS_CURRENT_MAJOR_VERSION,
      NTFS_SA::_MinorVersion = NTFS_CURRENT_MINOR_VERSION;

DEFINE_CONSTRUCTOR( NTFS_SA, SUPERAREA   );


VOID
NTFS_SA::Construct (
    )
/*++

Routine Description:

    This routine sets an NTFS_SA to a default initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _cleanup_that_requires_reboot = FALSE;
    _boot_sector = NULL;
    memset(&_bpb, 0, sizeof(BIOS_PARAMETER_BLOCK));
    _boot2 = 0;
    _boot3 = 0;
    _NumberOfStages = 0;
    _cvt_zone = 0;
    _cvt_zone_size = 0;
}


VOID
NTFS_SA::Destroy(
    )
/*++

Routine Description:

    This routine returns an NTFS_SA to a default initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _cleanup_that_requires_reboot = FALSE;
    _boot_sector = NULL;
    memset(&_bpb, 0, sizeof(BIOS_PARAMETER_BLOCK));
    _boot2 = 0;
    _boot3 = 0;
    _cvt_zone = 0;
    _cvt_zone_size = 0;
}


 
NTFS_SA::~NTFS_SA(
    )
/*++

Routine Description:

    Destructor for NTFS_SA.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


 
 
BOOLEAN
NTFS_SA::Initialize(
    IN OUT  PLOG_IO_DP_DRIVE    Drive,
    IN OUT  PMESSAGE            Message,
    IN      LCN                 CvtStartZone,
    IN      BIG_INT             CvtZoneSize
    )
/*++

Routine Description:

    This routine returns an NTFS_SA to a default initial state.

Arguments:

    Drive               - Supplies the drive that this MultiSectorBuffer is on
    Message             - Supplies an outlet for messages.
    CvtStartZone        - Supplies the starting cluster of the convert zone
    CvtZoneSize         - Supplies the convert zone size in clusters

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG   num_boot_sectors;

    Destroy();

    DebugAssert(Drive);
    DebugAssert(Message);

    num_boot_sectors = max(1, BYTES_PER_BOOT_SECTOR/Drive->QuerySectorSize());

    if (!_hmem.Initialize() ||
        !SUPERAREA::Initialize(&_hmem, Drive, num_boot_sectors, Message)) 
    {
        return FALSE;
    }

    _boot_sector = (PPACKED_BOOT_SECTOR) SECRUN::GetBuf();

    _cvt_zone = CvtStartZone;
    _cvt_zone_size = CvtZoneSize;

    return TRUE;
}

 
 
BOOLEAN
NTFS_SA::Read(
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This routine reads the NTFS volume's boot sector from disk.
    If the read fails then a message will be printed and then
    we will attempt to find an alternate boot sector, looking
    first at the end of the volume and then in the middle.

Arguments:

    Message - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DebugAssert(Message);

    if (!SECRUN::Read()) 
    {
        Message->Out("The first NTFS boot sector is unreadable. Reading second NTFS boot sector instead.");

        _boot2 = _drive->QuerySectors() - 1;
        Relocate(_boot2);

        if (!SECRUN::Read() ||
            !IFS_SYSTEM::IsThisNtfs(_drive->QuerySectors(),
                                    _drive->QuerySectorSize(),
                                    (PVOID)_boot_sector)) {

            _boot2 = _drive->QuerySectors()/2;
            Relocate(_boot2);

            if (!SECRUN::Read() ||
                !IFS_SYSTEM::IsThisNtfs(_drive->QuerySectors(),
                                        _drive->QuerySectorSize(),
                                        (PVOID)_boot_sector)) 
            {
                Message->Out("All NTFS boot sectors are unreadable. Cannot continue.");

                _boot2 = 0;
                Relocate(0);
                return FALSE;
            }
        }

        Relocate(0);
    }

    UnpackBios(&_bpb, &(_boot_sector->PackedBpb));

    if (QueryVolumeSectors() < _drive->QuerySectors()) {
        _boot2 = _drive->QuerySectors() - 1;
    } else {
        _boot2 = _drive->QuerySectors() / 2;
    }

    return TRUE;
}


 
BOOLEAN
NTFS_SA::Write(
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This routine writes both of the NTFS volume's boot sector to disk.
    If the write fails on either of the boot sectors then a message
    will be printed.

Arguments:

    Message - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DebugAssert(Message);

    PackBios(&_bpb, &(_boot_sector->PackedBpb));

    if (SECRUN::Write()) {

        Relocate(_boot2);

        if (!SECRUN::Write()) {

            Message->Out("The second NTFS boot sector is unwriteable.");
            return FALSE;
        }

        Relocate(0);

    } else {

        Message->Out("The first NTFS boot sector is unwriteable.");

        Relocate(_boot2);

        if (!SECRUN::Write()) {
            Message->Out("All NTFS boot sectors are unwriteable. Cannot continue.");
            Relocate(0);
            return FALSE;
        }

        Relocate(0);
    }

    return TRUE;
}

VOID
NTFS_SA::SetVersionNumber(
    IN  UCHAR   Major,
    IN  UCHAR   Minor
    )
{
    _MajorVersion = Major;
    _MinorVersion = Minor;
}


VOID
NTFS_SA::QueryVersionNumber(
    OUT PUCHAR  Major,
    OUT PUCHAR  Minor
    )
{
    *Major = _MajorVersion;
    *Minor = _MinorVersion;
}


 
USHORT
NTFS_SA::QueryVolumeFlags(
    OUT PBOOLEAN    CorruptVolume,
    OUT PUCHAR      MajorVersion,
    OUT PUCHAR      MinorVersion
    )
/*++

Routine Description:

    This routine fetches the volume flags.

Arguments:

    CorruptVolume   - Returns whether or not a volume corruption was
                        detected.
    MajorVersion    - Returns the major file system version number.
    MinorVersion    - Returns the minor file system version number.

Return Value:

    The flags describing this volume's state 

--*/
{
    NTFS_FRS_STRUCTURE      frs;
    HMEM                    hmem;
    LCN                     cluster_number, alternate;
    ULONG                   cluster_offset, alternate_offset;
    PVOID                   p;
    NTFS_ATTRIBUTE_RECORD   attr_rec;
    PVOLUME_INFORMATION     vol_info;

    if (CorruptVolume) {
        *CorruptVolume = FALSE;
    }

    if (MajorVersion) {
        *MajorVersion = 0;
    }

    if (MinorVersion) {
        *MinorVersion = 0;
    }

    ULONG cluster_size = QueryClusterFactor() * _drive->QuerySectorSize();

    cluster_number = (VOLUME_DASD_NUMBER * QueryFrsSize())/ cluster_size +
        QueryMftStartingLcn();

    cluster_offset = (QueryMftStartingLcn()*cluster_size +
        VOLUME_DASD_NUMBER * QueryFrsSize() - cluster_number * cluster_size).GetLowPart();

    DebugAssert(cluster_offset < cluster_size);

    alternate = (VOLUME_DASD_NUMBER * QueryFrsSize())/ cluster_size +
        QueryMft2StartingLcn();

    alternate_offset = (QueryMft2StartingLcn()*cluster_size +
        VOLUME_DASD_NUMBER * QueryFrsSize() - alternate * cluster_size).GetLowPart();

    for (;;) {

        if (!hmem.Initialize() ||
            !frs.Initialize(&hmem, _drive, cluster_number,
                            QueryClusterFactor(),
                            QueryVolumeSectors(),
                            QueryFrsSize(),
                            NULL,
                            cluster_offset) ||
            !frs.Read()) {

            if (cluster_number == alternate) {
                break;
            } else {
                cluster_number = alternate;
                cluster_offset = alternate_offset;
                continue;
            }
        }

        p = NULL;
        while (p = frs.GetNextAttributeRecord(p)) 
        {
            if (!attr_rec.Initialize(GetDrive(), p)) 
            {
                // the attribute record containing the volume flags
                // is not available--this means that the volume is
                // dirty.
                //
                return VOLUME_DIRTY;
            }

#if ($VOLUME_NAME > $VOLUME_INFORMATION)
#error  Attribute type $VOLUME_NAME should be smaller than that of $VOLUME_INFORMATION
#endif

            if (attr_rec.QueryTypeCode() == $VOLUME_INFORMATION &&
                attr_rec.QueryNameLength() == 0 &&
                attr_rec.QueryRecordLength() > SIZE_OF_RESIDENT_HEADER &&
                attr_rec.QueryResidentValueLength() < attr_rec.QueryRecordLength() &&
                (attr_rec.QueryRecordLength() - attr_rec.QueryResidentValueLength()) >=
                attr_rec.QueryResidentValueOffset() &&
                attr_rec.QueryResidentValueLength() >= sizeof(VOLUME_INFORMATION) &&
                (vol_info = (PVOLUME_INFORMATION) attr_rec.GetResidentValue())) 
            {
                if (MajorVersion) 
                {
                    *MajorVersion = vol_info->MajorVersion;
                }

                if (MinorVersion) 
                {
                    *MinorVersion = vol_info->MinorVersion;
                }

                if (vol_info->MajorVersion > 3) 
                {
                    break;  // try the mirror copy
                }

                return (vol_info->VolumeFlags);
            }
        }

        // If the desired attribute wasn't found in the first
        // volume dasd file then check the mirror.

        if (cluster_number == alternate) {
            break;
        } else {
            cluster_number = alternate;
            cluster_offset = alternate_offset;
        }
    }

    if (CorruptVolume) {
        *CorruptVolume = TRUE;
    }

    return VOLUME_DIRTY;
}



UCHAR
NTFS_SA::PostReadMultiSectorFixup(
    IN OUT  PVOID           MultiSectorBuffer,
    IN      ULONG           BufferSize,
    IN      PIO_DP_DRIVE    Drive,
    IN      ULONG           ValidSize
    )
/*++

Routine Description:

    This routine first verifies that the first element of the
    update sequence array is written at the end of every
    SEQUENCE_NUMBER_STRIDE bytes till it exceeds the given
    valid size.  If not, then this routine returns FALSE.

    Otherwise this routine swaps the following elements in the
    update sequence array into the appropriate positions in the
    multi sector buffer.

    This routine will also check to make sure that the update
    sequence array is valid and that the BufferSize is appropriate
    for this size of update sequence array.  Otherwise, this
    routine will not update the array sequence and return TRUE.

Arguments:

    MultiSectorBuffer   - Supplies the buffer to be updated.
    BufferSize          - Supplies the number of bytes in this
                            buffer.
    Drive               - Supplies the drive that this MultiSectorBuffer is on
    ValidSize           - Supplies the number of bytes that is
                          valid in this buffer

Return Value:

    UpdateSequenceArrayCheckValueOk (always non-zero)
         - If everything is ok.  If any valid sector does not
           contain the check value, the header signature will
           be changed to 'BAAD'.
    UpdateSequenceArrayMinorError
         - Same as 1 except the check value beyond ValidSize
           is incorrect.
--*/
{
    PUNTFS_MULTI_SECTOR_HEADER  pheader;
    USHORT                      i, size, offset;
    PUPDATE_SEQUENCE_NUMBER     parray, pnumber;
    UCHAR                       rtncode = UpdateSequenceArrayCheckValueOk;

    pheader = (PUNTFS_MULTI_SECTOR_HEADER) MultiSectorBuffer;
    size = pheader->UpdateSequenceArraySize;
    offset = pheader->UpdateSequenceArrayOffset;

    if (BufferSize%SEQUENCE_NUMBER_STRIDE ||
        offset%sizeof(UPDATE_SEQUENCE_NUMBER) ||
        offset + size*sizeof(UPDATE_SEQUENCE_NUMBER) > BufferSize ||
        BufferSize/SEQUENCE_NUMBER_STRIDE + 1 != size) {

        return rtncode;
    }

    parray = (PUPDATE_SEQUENCE_NUMBER) ((PCHAR) pheader + offset);

    pnumber = (PUPDATE_SEQUENCE_NUMBER)
              ((PCHAR) pheader + (SEQUENCE_NUMBER_STRIDE -
                                  sizeof(UPDATE_SEQUENCE_NUMBER)));

    for (i = 1; i < size; i++) {

        if (*pnumber != parray[0]) {
            if (ValidSize > 0) {

                DebugPrintTrace(("Incorrect USA check value at block %d.\n"
                                 "The expected value is %d but found %d\n",
                                 i, *pnumber, parray[0]));

                pheader->Signature[0] = 'B';
                pheader->Signature[1] = 'A';
                pheader->Signature[2] = 'A';
                pheader->Signature[3] = 'D';
                return rtncode;
            } else
                rtncode = UpdateSequenceArrayCheckValueMinorError;
        }

        *pnumber = parray[i];

        if (ValidSize >= SEQUENCE_NUMBER_STRIDE)
            ValidSize -= SEQUENCE_NUMBER_STRIDE;
        else
            ValidSize = 0;

        pnumber = (PUPDATE_SEQUENCE_NUMBER)
                  ((PCHAR) pnumber + SEQUENCE_NUMBER_STRIDE);
    }

    return rtncode;
}


VOID
NTFS_SA::PreWriteMultiSectorFixup(
    IN OUT  PVOID   MultiSectorBuffer,
    IN      ULONG   BufferSize
    )
/*++

Routine Description:

    This routine first checks to see if the update sequence
    array is valid.  If it is then this routine increments the
    first element of the update sequence array.  It then
    writes the value of the first element into the buffer at
    the end of every SEQUENCE_NUMBER_STRIDE bytes while
    saving the old values of those locations in the following
    elements of the update sequence arrary.

Arguments:

    MultiSectorBuffer   - Supplies the buffer to be updated.
    BufferSize          - Supplies the number of bytes in this
                            buffer.

Return Value:

    None.

--*/
{
    PUNTFS_MULTI_SECTOR_HEADER    pheader;
    USHORT                  i, size, offset;
    PUPDATE_SEQUENCE_NUMBER parray, pnumber;

    pheader = (PUNTFS_MULTI_SECTOR_HEADER) MultiSectorBuffer;
    size = pheader->UpdateSequenceArraySize;
    offset = pheader->UpdateSequenceArrayOffset;

    if (BufferSize%SEQUENCE_NUMBER_STRIDE ||
        offset%sizeof(UPDATE_SEQUENCE_NUMBER) ||
        offset + size*sizeof(UPDATE_SEQUENCE_NUMBER) > BufferSize ||
        BufferSize/SEQUENCE_NUMBER_STRIDE + 1 != size) {

        return;
    }

    parray = (PUPDATE_SEQUENCE_NUMBER) ((PCHAR) pheader + offset);


    // Don't allow 0 or all F's to be the update character.

    do {
        parray[0]++;
    } while (parray[0] == 0 || parray[0] == (UPDATE_SEQUENCE_NUMBER) -1);


    for (i = 1; i < size; i++) {

        pnumber = (PUPDATE_SEQUENCE_NUMBER)
                  ((PCHAR) pheader + (i*SEQUENCE_NUMBER_STRIDE -
                   sizeof(UPDATE_SEQUENCE_NUMBER)));

        parray[i] = *pnumber;
        *pnumber = parray[0];
    }
}

 
BOOLEAN
NTFS_SA::Read(
    )
/*++

Routine Description:

    This routine simply calls the other read with the default message
    object.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    MESSAGE msg;

    return Read(&msg);
}

 
UCHAR
NTFS_SA::QueryClusterFactor(
    ) CONST
/*++

Routine Description:

    This routine returns the number of sectors per cluster.

Arguments:

    None.

Return Value:

    The number of sectors per cluster.

--*/
{
    return _bpb.SectorsPerCluster;
}


BOOLEAN
NTFS_SA::MarkBad(
    IN const std::vector<sectors_range>& physicalDriveSectorsTargets,
    IN OUT  PMESSAGE    Message
)
{
    NTFS_ATTRIBUTE BitmapAttribute;
    NTFS_MFT_FILE MftFile;
    NTFS_BITMAP_FILE BitmapFile;
    NTFS_BAD_CLUSTER_FILE BadClusterFile;
    NTFS_BITMAP VolumeBitmap;
    NTFS_UPCASE_FILE UpcaseFile;
    NTFS_ATTRIBUTE UpcaseAttribute;
    NTFS_UPCASE_TABLE UpcaseTable;
    BOOLEAN Error = FALSE;
    UCHAR Major, Minor;
    BOOLEAN CorruptVolume;


    // Lock the drive.

    if (!_drive->Lock())
    {
        Message->Out("Cannot lock the drive. The volume is still in use.");
        return FALSE;
    }

    // Determine the volume version information.
    //
    QueryVolumeFlags(&CorruptVolume, &Major, &Minor);

    if (CorruptVolume)
    {
        Message->Out("The volume is corrupt. Run CHKDSK.");
        return FALSE;
    }

    if (Major > 3 || (Major = 3 && Minor > 1))
    {
        Message->Out("Unsupported NTFS version.");
        return FALSE;
    }

    SetVersionNumber(Major, Minor);

    // Initialize and read the MFT, the Bitmap File, the Bitmap, and the
    // Bad Cluster File.
    //
    if (!VolumeBitmap.Initialize(QueryVolumeSectors() /
        ((ULONG)QueryClusterFactor()),
        FALSE, _drive, QueryClusterFactor()) ||
        !MftFile.Initialize(_drive,
            QueryMftStartingLcn(),
            QueryClusterFactor(),
            QueryFrsSize(),
            QueryVolumeSectors(),
            &VolumeBitmap,
            NULL)) 
    {
        Message->Out("Out of memory.");
        return FALSE;
    }

    if (!MftFile.Read())
    {
        //DebugPrint("NTFS_SA::RecoverFile: Cannot read MFT.\n");
        Message->Out("The volume is corrupt. Run CHKDSK.");
        return FALSE;
    }

    // Get the upcase table.
    //
    if (!UpcaseFile.Initialize(MftFile.GetMasterFileTable()) ||
        !UpcaseFile.Read() ||
        !UpcaseFile.QueryAttribute(&UpcaseAttribute, &Error, $DATA) ||
        !UpcaseTable.Initialize(&UpcaseAttribute))
    {
        //DebugPrint("UNTFS RecoverFile:Can't get the upcase table.\n");

        Message->Out("The volume is corrupt. Run CHKDSK.");
        return FALSE;
    }

    MftFile.SetUpcaseTable((PNTFS_UPCASE_TABLE)&UpcaseTable);
    MftFile.GetMasterFileTable()->SetUpcaseTable((PNTFS_UPCASE_TABLE)&UpcaseTable);


    // Initialize the Bitmap file and the Bad Cluster file, and
    // read the volume bitmap.
    //
    if (!BitmapFile.Initialize(MftFile.GetMasterFileTable()) ||
        !BadClusterFile.Initialize(MftFile.GetMasterFileTable())) 
    {
        Message->Out("Out of memory.");
        return FALSE;
    }

    if (!BitmapFile.Read() ||
        !BitmapFile.QueryAttribute(&BitmapAttribute, &Error, $DATA) ||
        !VolumeBitmap.Read(&BitmapAttribute) ||
        !BadClusterFile.Read()) 
    {

        Message->Out("The volume is corrupt. Run CHKDSK.");
        return FALSE;
    }

    NUMBER_SET BadClusterList;
    if (!BadClusterList.Initialize())
    {
        return FALSE;
    }

    if (!MarkInFreeSpace(MftFile.GetMasterFileTable(), physicalDriveSectorsTargets, &BadClusterList, &BadClusterFile, Message))
    {
        return FALSE;
    }

    if (BadClusterList.QueryCardinality() != 0)
    {
        // If any bad clusters were found, we need to flush the bad cluster
        // file and the MFT and write the bitmap.  If no bad clusters were
        // found, then these structures will be unchanged.

        ULONG badClusterCount = BadClusterList.QueryCardinality().GetLowPart();
        if (badClusterCount == 1)
            Message->Out("Adding 1 cluster to the Bad Clusters File...");
        else
            Message->Out("Adding ", badClusterCount, " clusters to the Bad Clusters File...");

        if (BadClusterFile.Add(&BadClusterList))
        {
            if (!BadClusterFile.Flush(&VolumeBitmap) ||
                !MftFile.Flush() ||
                !VolumeBitmap.Write(&BitmapAttribute, &VolumeBitmap))
            {
                Message->Out("Insufficient disk space to record bad clusters.");
                return FALSE;
            }
        }
        else 
        {
            Message->Out("Insufficient disk space to record bad clusters.");
            return FALSE;
        }
    }
    else
    {
        if (!physicalDriveSectorsTargets.empty())
        {
            Message->Out("No clusters to add to the Bad Clusters File.");
        }
    }

    return TRUE;
}


BOOLEAN
NTFS_SA::MarkInFreeSpace(
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft,
    IN      const std::vector<sectors_range>& physicalDriveSectorsTargets,
    IN OUT  PNUMBER_SET             BadClusters,
    IN      PNTFS_BAD_CLUSTER_FILE  BadClusterFile,
    IN OUT  PMESSAGE                Message
)
/*++

Routine Description:

    This routine verifies all of the unused clusters on the disk.
    It adds any that are bad to the given bad cluster list.

Arguments:

    Mft         - Supplies the master file table.
    BadClusters - Supplies the current list of bad clusters.
    Message     - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PLOG_IO_DP_DRIVE    drive;
    PNTFS_BITMAP        bitmap;
    ULONG               cluster_factor;

    Message->Out("Scanning free space...");

    drive = Mft->GetDataAttribute()->GetDrive();

    BIG_INT firstDriveSector = drive->QueryHiddenSectors();
    BIG_INT lastDriveSector = firstDriveSector + drive->QuerySectors() - 1;

    Message->Out("First volume sector: ", firstDriveSector.GetQuadPart());
    Message->Out("Last volume sector: ", lastDriveSector.GetQuadPart());

    bitmap = Mft->GetVolumeBitmap();
    cluster_factor = Mft->QueryClusterFactor();
    
    BIG_INT skippedAlreadyBadClusters = 0;
    BIG_INT skippedInUseClusters = 0;
    BIG_INT markedClusters = 0;

    BIG_INT lastProcessedCluster = -1;

    for (std::vector<sectors_range>::const_iterator physicalDriveSectorsPair = physicalDriveSectorsTargets.begin(); physicalDriveSectorsPair != physicalDriveSectorsTargets.end(); ++physicalDriveSectorsPair)
    {
        BIG_INT firstPhysicalDriveSectorToMark = (unsigned __int64)physicalDriveSectorsPair->firstSector;
        if (firstPhysicalDriveSectorToMark < firstDriveSector) firstPhysicalDriveSectorToMark = firstDriveSector;
        if (firstPhysicalDriveSectorToMark > lastDriveSector) continue;

    	BIG_INT lastPhysicalDriveSectorToMark = (unsigned __int64)physicalDriveSectorsPair->lastSector;
        if (lastPhysicalDriveSectorToMark > lastDriveSector) lastPhysicalDriveSectorToMark = lastDriveSector;
        if (lastPhysicalDriveSectorToMark < firstDriveSector) continue;

        if (firstPhysicalDriveSectorToMark > lastPhysicalDriveSectorToMark) continue;

        BIG_INT firstSectorToMark = (firstPhysicalDriveSectorToMark - firstDriveSector) / cluster_factor;
        BIG_INT lastSectorToMark = (lastPhysicalDriveSectorToMark - firstDriveSector) / cluster_factor;
        for (BIG_INT clusterNumber = firstSectorToMark; clusterNumber <= lastSectorToMark; ++clusterNumber)
        {
            if (clusterNumber == lastProcessedCluster) continue; 

            lastProcessedCluster = clusterNumber;

            if (BadClusterFile->IsInList(clusterNumber))
            {
                ++skippedAlreadyBadClusters;
                continue;
            }

            if (!bitmap->IsFree(clusterNumber, 1))
            {
                ++skippedInUseClusters;
                continue;
            }

            // Add the bad clusters to the bad cluster list.
            if (!BadClusters->Add(clusterNumber))
            {
                Message->Out("An unspecified error occurred.");
                return FALSE;
            }

            // Mark the bad clusters as allocated in the bitmap.
            bitmap->SetAllocated(clusterNumber, 1);

            ++markedClusters;
        }
    }

    if (!physicalDriveSectorsTargets.empty())
    {
        Message->Out("The number of clusters skipped since they already marked bad: ", skippedAlreadyBadClusters.GetQuadPart());
        Message->Out("The number of clusters skipped since they are in use: ", skippedInUseClusters.GetQuadPart());
        Message->Out("The number of selected clusters: ", markedClusters.GetQuadPart());
    }

    return TRUE;
}

