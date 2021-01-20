#include "stdafx.h"

/*++

Module Name:

        ifssys.cxx

Abstract:

    This module contains the implementation for the IFS_SYSTEM class.
    The IFS_SYSTEM class is an abstract class which offers an
    interface for communicating with the underlying operating system
    on specific IFS issues.

--*/


#include "ulib.hxx"


#include "ifssys.hxx"
#include "bigint.hxx"
#include "wstring.hxx"

#include "drive.hxx"
#include "secrun.hxx"
#include "hmem.hxx"
#include "bpb.hxx"
#include "volume.hxx"

#include "untfs2.hxx"


BOOLEAN
IFS_SYSTEM::IsThisNtfs(
    IN  BIG_INT Sectors,
    IN  ULONG   SectorSize,
    IN  PVOID   BootSectorData
    )
/*++

Routine Description:

    This routine determines whether or not the given structure
    is part of an NTFS partition.

Arguments:

    Sectors     - Supplies the number of sectors on the drive.
    SectorSize  - Supplies the number of bytes per sector.
    BootSectorData
                - Supplies an unaligned boot sector.

Return Value:

    FALSE   - The supplied boot sector is not part of an NTFS
    TRUE    - The supplied boot sector is part of an NTFS volume.

--*/
{
    PPACKED_BOOT_SECTOR BootSector = (PPACKED_BOOT_SECTOR)BootSectorData;
    BOOLEAN r;
    ULONG   checksum;
    PULONG  l;
    USHORT  reserved_sectors, root_entries, sectors, sectors_per_fat;
    USHORT  bytes_per_sector;
    ULONG   large_sectors;

    memcpy(&reserved_sectors, BootSector->PackedBpb.ReservedSectors, sizeof(USHORT));
    memcpy(&root_entries, BootSector->PackedBpb.RootEntries, sizeof(USHORT));
    memcpy(&sectors, BootSector->PackedBpb.Sectors, sizeof(USHORT));
    memcpy(&sectors_per_fat, BootSector->PackedBpb.SectorsPerFat, sizeof(USHORT));
    memcpy(&bytes_per_sector, BootSector->PackedBpb.BytesPerSector, sizeof(USHORT));
    memcpy(&large_sectors, BootSector->PackedBpb.LargeSectors, sizeof(ULONG));


    r = TRUE;

    checksum = 0;
    for (l = (PULONG) BootSector; l < (PULONG) &BootSector->Checksum; l++) {
        checksum += *l;
    }

    if (BootSector->Oem[0] != 'N' ||
        BootSector->Oem[1] != 'T' ||
        BootSector->Oem[2] != 'F' ||
        BootSector->Oem[3] != 'S' ||
        BootSector->Oem[4] != ' ' ||
        BootSector->Oem[5] != ' ' ||
        BootSector->Oem[6] != ' ' ||
        BootSector->Oem[7] != ' ' ||
        // BootSector->Checksum != checksum ||
        bytes_per_sector != SectorSize) {

        r = FALSE;

    } else if ((BootSector->PackedBpb.SectorsPerCluster[0] != 0x1) &&
               (BootSector->PackedBpb.SectorsPerCluster[0] != 0x2) &&
               (BootSector->PackedBpb.SectorsPerCluster[0] != 0x4) &&
               (BootSector->PackedBpb.SectorsPerCluster[0] != 0x8) &&
               (BootSector->PackedBpb.SectorsPerCluster[0] != 0x10) &&
               (BootSector->PackedBpb.SectorsPerCluster[0] != 0x20) &&
               (BootSector->PackedBpb.SectorsPerCluster[0] != 0x40) &&
               (BootSector->PackedBpb.SectorsPerCluster[0] != 0x80)) {

        r = FALSE;

    } else if (reserved_sectors != 0 ||
               BootSector->PackedBpb.Fats[0] != 0 ||
               root_entries != 0 ||
               sectors != 0 ||
               sectors_per_fat != 0 ||
               large_sectors != 0 ||
               BootSector->NumberSectors > Sectors ||
               BootSector->MftStartLcn >= Sectors ||
               BootSector->Mft2StartLcn >= Sectors) {

        r = FALSE;
    }

    if (!r) {
        return r;
    }

    if (BootSector->ClustersPerFileRecordSegment < 0) {

        LONG temp = LONG(BootSector->ClustersPerFileRecordSegment);

        temp = 2 << -temp;

        if (temp < 512) {
            return FALSE;
        }
    } else if ((BootSector->ClustersPerFileRecordSegment != 0x1) &&
               (BootSector->ClustersPerFileRecordSegment != 0x2) &&
               (BootSector->ClustersPerFileRecordSegment != 0x4) &&
               (BootSector->ClustersPerFileRecordSegment != 0x8) &&
               (BootSector->ClustersPerFileRecordSegment != 0x10) &&
               (BootSector->ClustersPerFileRecordSegment != 0x20) &&
               (BootSector->ClustersPerFileRecordSegment != 0x40) &&
               (BootSector->ClustersPerFileRecordSegment != 0x80)) {

        return FALSE;
    }

    if (BootSector->DefaultClustersPerIndexAllocationBuffer < 0) {

        LONG temp = LONG(BootSector->DefaultClustersPerIndexAllocationBuffer);

        temp = 2 << -temp;

        if (temp < 512) {
            return FALSE;
        }
    } else if ((BootSector->DefaultClustersPerIndexAllocationBuffer != 0x1) &&
               (BootSector->DefaultClustersPerIndexAllocationBuffer != 0x2) &&
               (BootSector->DefaultClustersPerIndexAllocationBuffer != 0x4) &&
               (BootSector->DefaultClustersPerIndexAllocationBuffer != 0x8) &&
               (BootSector->DefaultClustersPerIndexAllocationBuffer != 0x10) &&
               (BootSector->DefaultClustersPerIndexAllocationBuffer != 0x20) &&
               (BootSector->DefaultClustersPerIndexAllocationBuffer != 0x40) &&
               (BootSector->DefaultClustersPerIndexAllocationBuffer != 0x80)) {

        r = FALSE;
    }

    return r;
}


#define BOOTBLKSECTORS 4
typedef int DSKPACKEDBOOTSECT;



BOOLEAN
IFS_SYSTEM::QueryFileSystemNameIsNtfs(
    IN  PCWSTRING    NtDriveName,
    OUT PBOOL     FileSystemNameIsNtfs,
    OUT PNTSTATUS    ErrorCode 
)
/*++

Routine Description:

    This routine computes the file system name for the drive specified.

Arguments:

    NtDriveName     - Supplies an NT style drive name.
    FileSystemName  - Returns the file system name for the drive.
    ErrorCode       - Receives an error code (if the method fails).
                        Note that this may be NULL, in which case the
                        exact error is not reported.
    FileSystemNameAndVersion
                    - Returns the file system name and version for the drive.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    LOG_IO_DP_DRIVE drive;
    HMEM            bootsec_hmem;
    SECRUN          bootsec;
    HMEM            super_hmem;
    SECRUN          super_secrun;
    HMEM            spare_hmem;
    SECRUN          spare;
    BOOLEAN         could_be_fat;
    BOOLEAN         could_be_hpfs;
    BOOLEAN         could_be_ntfs;
    BOOLEAN         could_be_ofs;
    ULONG           num_boot_sectors;
    BOOLEAN         first_read_failed = FALSE;

    DSTRING         fs_name_version;

    if (ErrorCode) 
    {
        *ErrorCode = 0;
    }
    *FileSystemNameIsNtfs = false;


    if (!drive.Initialize(NtDriveName))
    {
        if (ErrorCode) 
        {
            *ErrorCode = drive.QueryLastNtStatus();
        }
        return FALSE;
    }

    could_be_fat = could_be_hpfs = could_be_ntfs = could_be_ofs = TRUE;


    if (drive.QueryMediaType() == Unknown)
    {
        *FileSystemNameIsNtfs = false;
        return true;
    }

    num_boot_sectors = max(1, BYTES_PER_BOOT_SECTOR/drive.QuerySectorSize());

    if (!bootsec_hmem.Initialize() ||
        !bootsec.Initialize(&bootsec_hmem, &drive, 0, num_boot_sectors)) 
    {
        *FileSystemNameIsNtfs = false;
        return true;
    }

    if (!bootsec.Read()) {

        could_be_fat = could_be_hpfs = FALSE;
        first_read_failed = TRUE;

        bootsec.Relocate(drive.QuerySectors());

        if (!bootsec.Read()) {

            bootsec.Relocate(drive.QuerySectors()/2);

            if (!bootsec.Read()) 
            {
                could_be_ntfs = FALSE;
            }
        }
    }

    if (could_be_ntfs &&
        IsThisNtfs(drive.QuerySectors(),
            drive.QuerySectorSize(),
            (PPACKED_BOOT_SECTOR)bootsec.GetBuf()))
    {
        *FileSystemNameIsNtfs = true;
        return true;
    }

    *FileSystemNameIsNtfs = false;
    return true;
}


DECLARE_CLASS(DP_DRIVE);
DECLARE_CLASS(DRIVE);
DECLARE_CLASS(DRIVE_CACHE);
DECLARE_CLASS(INTSTACK);
DECLARE_CLASS(IO_DP_DRIVE);
DECLARE_CLASS(LOG_IO_DP_DRIVE);
DECLARE_CLASS(NUMBER_EXTENT);
DECLARE_CLASS(NUMBER_SET);
DECLARE_CLASS(SECRUN);
DECLARE_CLASS(SUPERAREA);
DECLARE_CLASS(VOL_LIODPDRV);


BOOLEAN
IfsutilDefineClassDescriptors(
)
{
    if (DEFINE_CLASS_DESCRIPTOR(DP_DRIVE) &&
        DEFINE_CLASS_DESCRIPTOR(DRIVE) &&
        DEFINE_CLASS_DESCRIPTOR(DRIVE_CACHE) &&
        DEFINE_CLASS_DESCRIPTOR(INTSTACK) &&
        DEFINE_CLASS_DESCRIPTOR(IO_DP_DRIVE) &&
        DEFINE_CLASS_DESCRIPTOR(LOG_IO_DP_DRIVE) &&
        DEFINE_CLASS_DESCRIPTOR(NUMBER_EXTENT) &&
        DEFINE_CLASS_DESCRIPTOR(NUMBER_SET) &&
        DEFINE_CLASS_DESCRIPTOR(SECRUN) &&
        DEFINE_CLASS_DESCRIPTOR(SUPERAREA) &&
        DEFINE_CLASS_DESCRIPTOR(VOL_LIODPDRV)) {

        return TRUE;

    }
    else {

        DebugPrint("Could not initialize class descriptors!");
        return FALSE;
    }
}

BOOLEAN
IfsutilUndefineClassDescriptors(
)
{
    UNDEFINE_CLASS_DESCRIPTOR(DP_DRIVE);
    UNDEFINE_CLASS_DESCRIPTOR(DRIVE);
    UNDEFINE_CLASS_DESCRIPTOR(DRIVE_CACHE);
    UNDEFINE_CLASS_DESCRIPTOR(INTSTACK);
    UNDEFINE_CLASS_DESCRIPTOR(IO_DP_DRIVE);
    UNDEFINE_CLASS_DESCRIPTOR(LOG_IO_DP_DRIVE);
    UNDEFINE_CLASS_DESCRIPTOR(NUMBER_EXTENT);
    UNDEFINE_CLASS_DESCRIPTOR(NUMBER_SET);
    UNDEFINE_CLASS_DESCRIPTOR(SECRUN);
    UNDEFINE_CLASS_DESCRIPTOR(SUPERAREA);
    UNDEFINE_CLASS_DESCRIPTOR(VOL_LIODPDRV);
    return TRUE;
}
