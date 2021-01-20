#include "stdafx.h"

/*++

Module Name:

    drive.cxx

Abstract:

    Provide drive methods.
    See drive.hxx for details.

--*/


#include "ulib.hxx"


#include "drive.hxx"

#include "message.hxx"
#include "numset.hxx"
#include "dcache.hxx"
#include "hmem.hxx"
#include "ifssys.hxx"



// Don't lock down more that 64K for IO.
CONST  int MaxIoSize   = 65536;

DEFINE_CONSTRUCTOR( DRIVE, OBJECT );

VOID
DRIVE::Construct (
        )
/*++

Routine Description:

    Contructor for DRIVE.

Arguments:

    None.

Return Value:

    None.

--*/
{
        // unreferenced parameters
        (void)(this);
}


DRIVE::~DRIVE(
    )
/*++

Routine Description:

    Destructor for DRIVE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


BOOLEAN
DRIVE::Initialize(
    IN      PCWSTRING    NtDriveName,
    IN OUT  PMESSAGE     Message
    )
/*++

Routine Description:

    This routine initializes a drive object.

Arguments:

    NtDriveName - Supplies an NT style drive name.
    Message     - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    Destroy();

    if (!NtDriveName) {
        Destroy();
        return FALSE;
    }

    if (!_name.Initialize(NtDriveName)) 
    {
        Destroy();
        Message ? Message->Out("Insufficient memory.") : 1;
        return FALSE;
    }

    return TRUE;
}


VOID
DRIVE::Destroy(
    )
/*++

Routine Description:

    This routine returns a DRIVE object to its initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
        // unreferenced parameters
        (void)(this);
}


DEFINE_CONSTRUCTOR( DP_DRIVE, DRIVE );

VOID
DP_DRIVE::Construct (
        )
/*++

Routine Description:

    Constructor for DP_DRIVE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    memset(&_actual, 0, sizeof(DRTYPE));
    _supported_list = NULL;
    _num_supported = 0;
    _alignment_mask = 0;
    _last_status = 0;
    _handle = 0;
    _is_writeable = FALSE;
    _is_primary_partition = FALSE;

    memset(&_partition_info, 0, sizeof(_partition_info));
}


 
DP_DRIVE::~DP_DRIVE(
    )
/*++

Routine Description:

    Destructor for DP_DRIVE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}

NTSTATUS
DP_DRIVE::OpenDrive(
    IN      PCWSTRING   NtDriveName,
    IN      ACCESS_MASK DesiredAccess,
    IN      BOOLEAN     ExclusiveWrite,
    OUT     PHANDLE     Handle,
    OUT     PULONG      Alignment,
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This method is a worker function for the Initialize methods,
    to open a volume and determine its alignment requirement.

Arguments:

    NtDriveName     - Supplies the name of the drive.
    DesiredAccess   - Supplies the access the client desires to the volume.
    ExclusiveWrite  - Supplies a flag indicating whether the client
                      wishes to exclude other write handles.
    Handle          - Receives the handle to the opened volume.
    Alignment       - Receives the alignment requirement for the volume.
    Message         - Supplies an outlet for messages.

Return Value:

    TRUE upon successful completion.


--*/
{
    UNICODE_STRING              string;
    OBJECT_ATTRIBUTES           oa;
    IO_STATUS_BLOCK             status_block;
    FILE_ALIGNMENT_INFORMATION  alignment_info;
    NTSTATUS                    Status;


    string.Length = (USHORT) NtDriveName->QueryChCount() * sizeof(WCHAR);
    string.MaximumLength = string.Length;
    string.Buffer = (PWSTR)NtDriveName->GetWSTR();

    InitializeObjectAttributes( &oa,
                                &string,
                                OBJ_CASE_INSENSITIVE,
                                0,
                                0 );

    Status = NtOpenFile(Handle,
                        DesiredAccess,
                        &oa, &status_block,
                        FILE_SHARE_READ |
                        (ExclusiveWrite ? 0 : FILE_SHARE_WRITE),
                        FILE_SYNCHRONOUS_IO_ALERT);

    if (!NT_SUCCESS(Status))
    {
        char* message = (Status == STATUS_ACCESS_DENIED) ?
            "Access denied." :
            "Cannot open volume for direct access.";
        Message ? Message->Out(message) : 1;
        return Status;
    }


    // Query the disk alignment information.

    Status = NtQueryInformationFile(*Handle,
                                    &status_block,
                                    &alignment_info,
                                    sizeof(FILE_ALIGNMENT_INFORMATION),
                                    FileAlignmentInformation);

    if (!NT_SUCCESS(Status)) 
    {
        const  char* message;
        switch( Status ) {

            case STATUS_DEVICE_BUSY:
            case STATUS_DEVICE_NOT_READY:
                message = "The device is busy.";
                break;

            case STATUS_DEVICE_OFF_LINE:
                message = "The device is offline.";
                break;

            default:
                message = "Error in IOCTL call.";
                break;
        }

        DebugPrintTrace(("Failed NtQueryInformationFile (%x)\n", Status));

        Message ? Message->Out(message) : 1;


        return Status;
    }

    *Alignment = alignment_info.AlignmentRequirement;

    //
    //  Set the ALLOW_EXTENDED_DASD_IO flag for the file handle,
    //  which ntfs format and chkdsk depend on to write the backup
    //  boot sector.  We ignore the return code from this, but we
    //  could go either way.
    //

    (VOID)NtFsControlFile( *Handle,
                           0, NULL, NULL,
                           &status_block,
                           FSCTL_ALLOW_EXTENDED_DASD_IO,
                           NULL, 0, NULL, 0);

    return Status;
}


 
BOOLEAN
DP_DRIVE::Initialize(
    IN      PCWSTRING   NtDriveName,
    IN OUT  PMESSAGE    Message,
    IN      BOOLEAN     IsTransient
    )
/*++

Routine Description:

    This routine initializes a DP_DRIVE object based on the supplied drive
    path.

Arguments:

    NtDriveName     - Supplies the drive path.
    Message         - Supplies an outlet for messages.
    IsTransient     - Supplies whether or not to keep the handle to the
                        drive open beyond this method.
    ExclusiveWrite  - Supplies whether or not to open the drive for
                        exclusive write.
    FormatType      - Supplies the file system type in the event of a format

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    IO_STATUS_BLOCK                 status_block;
    DISK_GEOMETRY                   disk_geometry;
    PARTITION_INFORMATION_EX        partition_info;
    GET_LENGTH_INFORMATION          length_info;
    BOOLEAN                         partition;
    
    FILE_FS_DEVICE_INFORMATION  DeviceInfo;

    Destroy();

    if (!DRIVE::Initialize(NtDriveName, Message)) {
        Destroy();
        return FALSE;
    }

    BOOL ExclusiveWrite = false;
    _last_status = OpenDrive( NtDriveName,
                              SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                              ExclusiveWrite,
                              &_handle,
                              &_alignment_mask,
                              Message );

    if(!NT_SUCCESS(_last_status)) 
    {
        Destroy();
        DebugPrintTrace(("Can't open drive. Status returned = %x.\n", _last_status));
        return FALSE;
    }

    _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                         &status_block,
                                         IOCTL_DISK_IS_WRITABLE,
                                         NULL, 0, NULL, 0);

    _is_writeable = (_last_status != STATUS_MEDIA_WRITE_PROTECTED);


    _last_status = NtQueryVolumeInformationFile(_handle,
                                                &status_block,
                                                &DeviceInfo,
                                                sizeof(DeviceInfo),
                                                FileFsDeviceInformation);

    // this is from ::GetDriveType()

    if (!NT_SUCCESS(_last_status)) 
    {
        _drive_type = UnknownDrive;
    } else if (DeviceInfo.Characteristics & FILE_REMOTE_DEVICE) 
    {
        _drive_type = RemoteDrive;
    }
	else 
    {
        switch (DeviceInfo.DeviceType)
		{
          case FILE_DEVICE_NETWORK:
          case FILE_DEVICE_NETWORK_FILE_SYSTEM:
            _drive_type = RemoteDrive;
             break;

          case FILE_DEVICE_CD_ROM:
          case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
             _drive_type = CdRomDrive;
             break;

          case FILE_DEVICE_VIRTUAL_DISK:
             _drive_type = RamDiskDrive;
             break;

          case FILE_DEVICE_DISK:
          case FILE_DEVICE_DISK_FILE_SYSTEM:

             if ( DeviceInfo.Characteristics & FILE_REMOVABLE_MEDIA ) 
             {
                 _drive_type = RemovableDrive;
                 }
             else {
                 _drive_type = FixedDrive;
                 }
             break;

          default:
             _drive_type = UnknownDrive;
             break;
        }
    }
// #endif


    // Query the disk geometry.

    _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                         &status_block,
                                         IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                         NULL, 0, &disk_geometry,
                                         sizeof(DISK_GEOMETRY));

    if (!NT_SUCCESS(_last_status))
    {
        DebugPrintTrace(("Can't query disk geometry. Status returned = %x.\n", _last_status));
        if ((_last_status == STATUS_UNSUCCESSFUL) ||
            (_last_status == STATUS_UNRECOGNIZED_MEDIA))
        {
            disk_geometry.MediaType = Unknown;
        }
        else
        {
            Destroy();
            const char* message;
            switch (_last_status)
            {
            case STATUS_NO_MEDIA_IN_DEVICE:
                message = "Cannot open volume for direct access.";
                break;

            case STATUS_DEVICE_BUSY:
            case STATUS_DEVICE_NOT_READY:
                message = "The device is busy.";
                break;

            case STATUS_DEVICE_OFF_LINE:
                message = "The device is offline.";
                break;

            default:
                message = "Error in IOCTL call.";
                break;
            }

            Message ? Message->Out(message) : 1;

            return FALSE;
        }
    }

    if (disk_geometry.MediaType == Unknown) 
    {
        memset(&disk_geometry, 0, sizeof(DISK_GEOMETRY));
        disk_geometry.MediaType = Unknown;
    }

    partition = FALSE;

    // Try to read the partition entry.

    if (disk_geometry.MediaType == FixedMedia ||
        disk_geometry.MediaType == RemovableMedia)
    {
        _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                             &status_block,
                                             IOCTL_DISK_GET_LENGTH_INFO,
                                             NULL, 0, &length_info,
                                             sizeof(GET_LENGTH_INFORMATION));

        partition = (BOOLEAN) NT_SUCCESS(_last_status);

        if (!NT_SUCCESS(_last_status) &&
            _last_status != STATUS_INVALID_DEVICE_REQUEST) {
            DebugPrintTrace(("Can't get volume size. Status returned = %x.\n", _last_status));
            Destroy();
            Message ? Message->Out("Error reading partition table.") : 1;
            return FALSE;
        }

        if (partition) {

            _last_status = NtDeviceIoControlFile(
                               _handle, 0, NULL, NULL, &status_block,
                               IOCTL_DISK_GET_PARTITION_INFO_EX, NULL, 0,
                               &partition_info,
                               sizeof(PARTITION_INFORMATION_EX));

            if (!NT_SUCCESS(_last_status)) {
                if (_last_status != STATUS_INVALID_DEVICE_REQUEST) {
                    DebugPrintTrace(("Can't read partition entry. Status returned = %x.\n", _last_status));
                    Destroy();
                    Message ? Message->Out("Error reading partition table.") : 1;
                    return FALSE;
                }

                //
                // GET_PARTITION_INFO_EX will fail outright on an EFI Dynamic
                // Volume.  In this case, just make up the starting offset
                // so that FORMAT/CHKDSK can proceed normally.
                //

                partition_info.PartitionStyle = PARTITION_STYLE_GPT;
                partition_info.StartingOffset.QuadPart = 0x7E00;
                partition_info.PartitionLength.QuadPart = length_info.Length.QuadPart;
                partition_info.Gpt.PartitionType = PARTITION_BASIC_DATA_GUID;
            }

            if (partition_info.PartitionStyle != PARTITION_STYLE_MBR) {

                // If this is EFI, then just make up reasonable MBR values
                // so that CHKDSK/FORMAT can proceed with business as usual.

                partition_info.PartitionStyle = PARTITION_STYLE_MBR;
                if (IsEqualGUID(partition_info.Gpt.PartitionType,
                                PARTITION_BASIC_DATA_GUID)) 
                {
                    partition_info.Mbr.PartitionType = 0x7;
                }
            	else 
                {
                    partition_info.Mbr.PartitionType = 0xEE;
                }

                partition_info.Mbr.BootIndicator = FALSE;
                partition_info.Mbr.RecognizedPartition = TRUE;
                partition_info.Mbr.HiddenSectors =
                        (ULONG) (partition_info.StartingOffset.QuadPart/
                                 disk_geometry.BytesPerSector);
            }
        }
    }


    // Store the information in the class.

    if (partition) 
    {
        _partition_info = partition_info;

    	DiskGeometryToDriveType(&disk_geometry,
                                partition_info.PartitionLength/
                                disk_geometry.BytesPerSector,
                                partition_info.Mbr.HiddenSectors,
                                &_actual);
    }
	else 
    {
        DiskGeometryToDriveType(&disk_geometry, &_actual);
    }

    if (!_num_supported) 
    {
        _num_supported = 1;

        if (!(_supported_list = NEW DRTYPE[1])) 
        {
            Destroy();
            Message ? Message->Out("Insufficient memory.") : 1;
            return FALSE;
        }

        _supported_list[0] = _actual;
    }

    if (!CheckForPrimaryPartition()) 
    {
        DebugPrintTrace(("Failed CheckForPrimaryPartition (%x)\n", _last_status));
        Destroy();
        if (Message) 
        {
            switch (_last_status)
        	{
                case STATUS_NO_MEDIA_IN_DEVICE:
                    Message->Out("There is no media in the drive.");
                    break;

                case STATUS_DEVICE_BUSY:
                case STATUS_DEVICE_NOT_READY:
                    Message->Out("The device is busy.");
                    break;

                case STATUS_DEVICE_OFF_LINE:
                    Message->Out("The device is offline.");
                    break;

                default:
                    Message->Out("Error in IOCTL call.");
                    break;
            }
        }
        return FALSE;
    }

    //
    // Determine whether the media is a super-floppy; non-floppy
    // removable media which is not partitioned.  Such media will
    // have but a single partition, normal media will have at least 4.
    //

    if (disk_geometry.MediaType == RemovableMedia) 
    {
        CONST INT EntriesPerBootRecord = 4;
        CONST INT MaxLogicalVolumes = 23;
        CONST INT Length =  sizeof(DRIVE_LAYOUT_INFORMATION_EX) +
                            EntriesPerBootRecord * (MaxLogicalVolumes + 1) *
                                sizeof(PARTITION_INFORMATION_EX);

        UCHAR buf[Length];

        DRIVE_LAYOUT_INFORMATION_EX *layout_info = (DRIVE_LAYOUT_INFORMATION_EX *)buf;

        _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                             &status_block,
                                             IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                                             NULL, 0, layout_info,
                                             Length);

#if 1
        if (!NT_SUCCESS(_last_status)) 
        {
            if (_last_status == STATUS_INVALID_DEVICE_REQUEST) 
            {
            }
        	else 
            {
                DebugPrintTrace(("Failed IOCTL_DISK_GET_DRIVE_LAYOUT_EX (%x)\n", _last_status));
            }
        }
#endif

        if (!NT_SUCCESS(_last_status)) 
        {
            Destroy();

            if (Message) 
            {
                switch (_last_status) {
                    case STATUS_NO_MEDIA_IN_DEVICE:
                        Message->Out("There is no media in the drive.");
                        break;

                    case STATUS_DEVICE_BUSY:
                    case STATUS_DEVICE_NOT_READY:
                        Message->Out("The device is busy.");
                        break;

                    case STATUS_DEVICE_OFF_LINE:
                        Message->Out("The device is offline.");
                        break;

                    default:
                        Message->Out("Error in IOCTL call.");
                        break;

                }
            }
            return FALSE;
        }

    }

    if (!IsTransient) 
    {
        NtClose(_handle);
        _handle = 0;
    }

    return TRUE;

}

 
ULONG
DP_DRIVE::QuerySectorSize(
    ) CONST
/*++

Routine Description:

    This routine computes the number of bytes per sector.

Arguments:

    None.

Return Value:

    The number of bytes per sector.

--*/
{
    return _actual.SectorSize;
}


BIG_INT
DP_DRIVE::QuerySectors(
    ) CONST
/*++

Routine Description:

    This routine computes the number sectors on the disk.  This does not
    include the hidden sectors.

Arguments:

    None.

Return Value:

    The number of sectors on the disk.

--*/
{
    return _actual.Sectors;
}


VOID
DP_DRIVE::Destroy(
        )
/*++

Routine Description:

    This routine returns a DP_DRIVE to its initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    memset(&_actual, 0, sizeof(DRTYPE));
    DELETE_ARRAY(_supported_list);
    _num_supported = 0;
    _alignment_mask = 0;
    _is_writeable = FALSE;
    _is_primary_partition = FALSE;


    if (_handle)
    {
        NtClose(_handle);
        _handle = 0;
    }

    memset(&_partition_info, 0, sizeof(_partition_info));
}


VOID
DP_DRIVE::DiskGeometryToDriveType(
    IN  PCDISK_GEOMETRY DiskGeometry,
    OUT PDRTYPE         DriveType
    )
/*++

Routine Description:

    This routine computes the drive type given the disk geometry.

Arguments:

    DiskGeometry    - Supplies the disk geometry for the drive.
    DriveType       - Returns the drive type for the drive.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DriveType->MediaType = DiskGeometry->MediaType;
    DriveType->SectorSize = DiskGeometry->BytesPerSector;
    DriveType->Sectors = DiskGeometry->Cylinders*
                         DiskGeometry->TracksPerCylinder*
                         DiskGeometry->SectorsPerTrack;
    DriveType->HiddenSectors = 0;
    DriveType->SectorsPerTrack = DiskGeometry->SectorsPerTrack;
    DriveType->Heads = DiskGeometry->TracksPerCylinder;
}


VOID
DP_DRIVE::DiskGeometryToDriveType(
    IN  PCDISK_GEOMETRY DiskGeometry,
    IN  BIG_INT         NumSectors,
    IN  BIG_INT         NumHiddenSectors,
    OUT PDRTYPE         DriveType
    )
/*++

Routine Description:

    This routine computes the drive type given the disk geometry.

Arguments:

    DiskGeometry        - Supplies the disk geometry for the drive.
    NumSectors          - Supplies the total number of non-hidden sectors on
                        the disk.
    NumHiddenSectors    - Supplies the number of hidden sectors on the disk.
    DriveType           - Returns the drive type for the drive.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DriveType->MediaType = DiskGeometry->MediaType;
    DriveType->SectorSize = DiskGeometry->BytesPerSector;
    DriveType->Sectors = NumSectors;
    DriveType->HiddenSectors = NumHiddenSectors;
    DriveType->SectorsPerTrack = DiskGeometry->SectorsPerTrack;
    DriveType->Heads = DiskGeometry->TracksPerCylinder;
}

DEFINE_CONSTRUCTOR( IO_DP_DRIVE, DP_DRIVE );

VOID
IO_DP_DRIVE::Construct (
        )

/*++

Routine Description:

    Constructor for IO_DP_DRIVE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _is_locked = FALSE;
    _is_exclusive_write = FALSE;
    _cache = NULL;
    _ValidBlockLengthForVerify = 0;
    _message = NULL;
}


VOID
IO_DP_DRIVE::Destroy(
    )
/*++

Routine Description:

    This routine returns an IO_DP_DRIVE object to its initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DELETE(_cache);

    if (_is_exclusive_write) 
    {
        Dismount();
        _is_exclusive_write = FALSE;
    }

    if (_is_locked) 
    {
        Unlock();
        _is_locked = FALSE;
    }

    _ValidBlockLengthForVerify = 0;
    _message = NULL;
}


IO_DP_DRIVE::~IO_DP_DRIVE(
    )
/*++

Routine Description:

    Destructor for IO_DP_DRIVE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


BOOLEAN
IO_DP_DRIVE::Initialize(
    IN      PCWSTRING   NtDriveName,
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This routine initializes an IO_DP_DRIVE object.

Arguments:

    NtDriveName     - Supplies the drive path.
    Message         - Supplies an outlet for messages.
    ExclusiveWrite  - Supplies whether or not to open the drive for
                        exclusive write.
    FormatType      - Supplies the file system type in the event of a format

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    Destroy();

    if (!DP_DRIVE::Initialize(NtDriveName, Message, TRUE)) 
    {
        Destroy();
        return FALSE;
    }

    _is_exclusive_write = false;

    if (!(_cache = NEW DRIVE_CACHE) ||
        !_cache->Initialize(this)) 
    {
        Destroy();
        return FALSE;
    }

    _ValidBlockLengthForVerify = 0;
    _message = Message;

    return TRUE;
}


BOOLEAN
IO_DP_DRIVE::Read(
    IN  BIG_INT     StartingSector,
    IN  SECTORCOUNT NumberOfSectors,
    OUT PVOID       Buffer
    )
/*++

Routine Description:

    This routine reads a run of sectors into the buffer pointed to by
    'Buffer'.

Arguments:

    StartingSector  - Supplies the first sector to be read.
    NumberOfSectors - Supplies the number of sectors to be read.
    Buffer          - Supplies a buffer to read the run of sectors into.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DebugAssert(_cache);
    return _cache->Read(StartingSector, NumberOfSectors, Buffer);
}


 
BOOLEAN
IO_DP_DRIVE::Write(
    BIG_INT     StartingSector,
    SECTORCOUNT NumberOfSectors,
    PVOID       Buffer
    )
/*++

Routine Description:

    This routine writes a run of sectors onto the disk from the buffer pointed
    to by 'Buffer'.  Writing is only permitted if 'Lock' was called.

Arguments:

    StartingSector      - Supplies the first sector to be written.
    NumberOfSectors     - Supplies the number of sectors to be written.
    Buffer              - Supplies the buffer to write the run of sectors from.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DebugAssert(_cache);
    return _cache->Write(StartingSector, NumberOfSectors, Buffer);
}


BOOLEAN
IO_DP_DRIVE::HardRead(
    IN  BIG_INT     StartingSector,
    IN  SECTORCOUNT NumberOfSectors,
    OUT PVOID       Buffer
    )
/*++

Routine Description:

    This routine reads a run of sectors into the buffer pointed to by
    'Buffer'.

Arguments:

    StartingSector      - Supplies the first sector to be read.
    NumberOfSectors     - Supplies the number of sectors to be read.
    Buffer              - Supplies a buffer to read the run of sectors into.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG           sector_size;
    ULONG           buffer_size;
    IO_STATUS_BLOCK status_block;
    BIG_INT         secptr;
    BIG_INT         endofrange;
    SECTORCOUNT     increment;
    PCHAR           bufptr;
    BIG_INT         byte_offset;
    BIG_INT         tmp;
    LARGE_INTEGER   l;

    DebugAssert(!(((ULONG_PTR) Buffer) & QueryAlignmentMask()));

    sector_size = QuerySectorSize();
    endofrange = StartingSector + NumberOfSectors;
    increment = MaxIoSize/sector_size;

    bufptr = (PCHAR) Buffer;
    for (secptr = StartingSector; secptr < endofrange; secptr += increment) {

        byte_offset = secptr*sector_size;

        if (secptr + increment > endofrange) {
            tmp = endofrange - secptr;
            DebugAssert(tmp.GetHighPart() == 0);
            buffer_size = sector_size*tmp.GetLowPart();

        } else {
            buffer_size = sector_size*increment;

        }

        l = byte_offset.GetLargeInteger();

        _last_status = NtReadFile(_handle, 0, NULL, NULL, &status_block,
                                  bufptr, buffer_size, &l, NULL);

        if (_last_status == STATUS_NO_MEMORY) {
            increment /= 2;
            secptr -= increment;
            continue;
        }

        if (NT_ERROR(_last_status) || status_block.Information != buffer_size) {

            if (NT_ERROR(_last_status)) {
                DebugPrintTrace(("HardRead: NtReadFile failure: %x, %I64x, %x\n",
                                 _last_status, l, buffer_size));
            } else {
                DebugPrintTrace(("HardRead: NtReadFile failure: %I64x, %x, %x\n",
                                 l, status_block.Information, buffer_size));
            }

            if (_message) {
                if (NT_ERROR(_last_status)) 
                {
                    _message->Out("Read failure with status ", _last_status, " at offset ", l.QuadPart, " for ", buffer_size, " bytes.");
                }
            	else 
                {
                    _message->Out("Incorrect read at offset ", l.QuadPart," for ", buffer_size," bytes but got ", status_block.Information," bytes.");
                }
            }

            return FALSE;
        }

        bufptr += buffer_size;
    }

    return TRUE;
}


BOOLEAN
IO_DP_DRIVE::HardWrite(
    BIG_INT     StartingSector,
    SECTORCOUNT NumberOfSectors,
    PVOID       Buffer
    )
/*++

Routine Description:

    This routine writes a run of sectors onto the disk from the buffer pointed
    to by 'Buffer'.  Writing is only permitted if 'Lock' was called.

    After writing each chunk, we read it back to make sure the write
    really succeeded.

Arguments:

    StartingSector      - Supplies the first sector to be written.
    NumberOfSectors     - Supplies the number of sectors to be written.
    Buffer              - Supplies the buffer to write the run of sectors from.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG           sector_size;
    ULONG           buffer_size;
    IO_STATUS_BLOCK status_block;
    BIG_INT         secptr;
    BIG_INT         endofrange;
    SECTORCOUNT     increment;
    PCHAR           bufptr;
    PCHAR           scratch_ptr;
    BIG_INT         byte_offset;
    BIG_INT         tmp;
    LARGE_INTEGER   l;
    CHAR            ScratchIoBuf[MaxIoSize + 511];

    DebugAssert(!(((ULONG_PTR) Buffer) & QueryAlignmentMask()));
    DebugAssert(QueryAlignmentMask() < 0x200);

    if (! ((ULONG_PTR)ScratchIoBuf & QueryAlignmentMask())) {
        scratch_ptr = ScratchIoBuf;
    } else {
        scratch_ptr = (PCHAR)((ULONG_PTR) ((PCHAR)ScratchIoBuf +
            QueryAlignmentMask()) & (~(ULONG_PTR)QueryAlignmentMask()));
    }
    DebugAssert(!(((ULONG_PTR) scratch_ptr) & QueryAlignmentMask()));

    sector_size = QuerySectorSize();
    endofrange = StartingSector + NumberOfSectors;
    increment = MaxIoSize/sector_size;

    bufptr = (PCHAR) Buffer;
    for (secptr = StartingSector; secptr < endofrange; secptr += increment) {

        byte_offset = secptr*sector_size;

        if (secptr + increment > endofrange) {
            tmp = endofrange - secptr;
            DebugAssert(tmp.GetHighPart() == 0);
            buffer_size = sector_size*tmp.GetLowPart();
        } else {
            buffer_size = sector_size*increment;
        }

        l = byte_offset.GetLargeInteger();

        _last_status = NtWriteFile(_handle, 0, NULL, NULL, &status_block,
                                   bufptr, buffer_size, &l, NULL);

        if (_last_status == STATUS_NO_MEMORY) {
            increment /= 2;
            secptr -= increment;
            continue;
        }

        if (NT_ERROR(_last_status) || status_block.Information != buffer_size) {

            if (NT_ERROR(_last_status)) {
                DebugPrintTrace(("HardWrite: NtWriteFile failure: %x, %I64x, %x\n",
                                 _last_status, l, buffer_size));
            } else {
                DebugPrintTrace(("HardWrite: NtWriteFile failure: %I64x, %x, %x\n",
                                 l, status_block.Information, buffer_size));
            }

            return FALSE;
        }

        DebugAssert(buffer_size <= MaxIoSize);

        _last_status = NtReadFile(_handle, 0, NULL, NULL, &status_block,
                                  scratch_ptr, buffer_size, &l, NULL);

        if (NT_ERROR(_last_status) || status_block.Information != buffer_size) {

            if (NT_ERROR(_last_status)) {
                DebugPrintTrace(("HardWrite: NtReadFile failure: %x, %I64x, %x\n",
                                 _last_status, l, buffer_size));
            } else {
                DebugPrintTrace(("HardWrite: NtReadFile failure: %I64x, %x, %x\n",
                                 l, status_block.Information, buffer_size));
            }

            if (_message) {
                if (NT_ERROR(_last_status))

                {
                    _message->Out("Write failure with status ", _last_status, " at offset ", l.QuadPart, " for ", buffer_size, " bytes.");
                }
                else
                {
                    _message->Out("Incorrect write at offset ", l.QuadPart, " for ", buffer_size, " bytes but wrote ", status_block.Information, " bytes.");
                }
            }

            return FALSE;
        }

        if (0 != memcmp(scratch_ptr, bufptr, buffer_size)) {

            DebugPrint("What's read back does not match what's written out\n");
            if (_message) 
            {
                _message->Out("The data written out is different from what is being read back at offset ", l.QuadPart, " for ", buffer_size, " bytes");
            }

            return FALSE;
        }

        bufptr += buffer_size;
    }

    return TRUE;
}


BOOLEAN
DP_DRIVE::CheckForPrimaryPartition(
    )
/*++

Routine Description:

    This routine checks to see if the volume is on a primary partition.
    It sets the result returned by IsPrimaryPartition routine.

Arguments:

    N/A

Return Value:

    TRUE if successfully determined if the volume is on a primary
    partition.
--*/
{
    CONST INT EntriesPerBootRecord = 4;
    CONST INT MaxLogicalVolumes = 23;
    CONST INT Info_Length =  EntriesPerBootRecord * (MaxLogicalVolumes + 1) *
                            sizeof(PARTITION_INFORMATION_EX);
    INT       Length = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + Info_Length;

    IO_STATUS_BLOCK             status_block;
    STORAGE_DEVICE_NUMBER       device_info;
    DRIVE_LAYOUT_INFORMATION_EX *layout_info, *temp;

    ULONG                       i, partition_count;

    _is_primary_partition = FALSE;

    _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                         &status_block,
                                         IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                         NULL, 0, &device_info,
                                         sizeof(STORAGE_DEVICE_NUMBER));

    if (NT_SUCCESS(_last_status) && device_info.DeviceType == FILE_DEVICE_DISK &&
        (device_info.PartitionNumber != -1 && device_info.PartitionNumber != 0)) {

        layout_info = (DRIVE_LAYOUT_INFORMATION_EX *) MALLOC(Length);

        do {

            _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                             &status_block,
                                             IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                                             NULL, 0, layout_info,
                                             Length);

            if (!NT_SUCCESS(_last_status)) {
                if ((_last_status == STATUS_BUFFER_TOO_SMALL) 
                        && (Length += Info_Length)
                        && ((temp = (DRIVE_LAYOUT_INFORMATION_EX *)REALLOC(layout_info, Length)) != NULL)) {
                    layout_info = temp;    
                } else {
                    FREE(layout_info);
                    return FALSE;
                }
            }
        }while (_last_status == STATUS_BUFFER_TOO_SMALL);

        if (layout_info->PartitionStyle == PARTITION_STYLE_MBR) {
            partition_count = min(4, layout_info->PartitionCount);

            for (i=0; i<partition_count; i++) {
                if (layout_info->PartitionEntry[i].PartitionNumber ==
                    device_info.PartitionNumber) {
                    _is_primary_partition = TRUE;
                    break;
                }
            }
        }

        FREE(layout_info);
    }

    return TRUE;
}


 
BOOLEAN
IO_DP_DRIVE::Verify(
    IN  BIG_INT StartingSector,
    IN  BIG_INT NumberOfSectors
    )
/*++

Routine Description:

    This routine verifies a run of sectors on the disk.

Arguments:

    StartingSector  - Supplies the first sector of the run to verify.
    NumberOfSectors - Supplies the number of sectors in the run to verify.

Return Value:

    FALSE   - Some of the sectors in the run are bad.
    TRUE    - All of the sectors in the run are good.

--*/
{
    VERIFY_INFORMATION  verify_info;
    IO_STATUS_BLOCK     status_block;
    BIG_INT             starting_offset;
    BIG_INT             verify_size;

    DebugAssert(QuerySectorSize());

    _last_status = STATUS_SUCCESS;

    if (!_is_exclusive_write) 
    {
        return VerifyWithRead(StartingSector, NumberOfSectors);
    }

    starting_offset = StartingSector*QuerySectorSize();
    verify_size = NumberOfSectors*QuerySectorSize();

    verify_info.StartingOffset = starting_offset.GetLargeInteger();

    // Note: norbertk Verify IOCTL is destined to go to a BIG_INT length.
    DebugAssert(verify_size.GetHighPart() == 0);
    verify_info.Length = verify_size.GetLowPart();

    _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                         &status_block, IOCTL_DISK_VERIFY,
                                         &verify_info,
                                         sizeof(VERIFY_INFORMATION),
                                         NULL, 0);

    return (BOOLEAN) NT_SUCCESS(_last_status);
}

 
BOOLEAN
IO_DP_DRIVE::Verify(
    IN      BIG_INT         StartingSector,
    IN      BIG_INT         NumberOfSectors,
    IN OUT  PNUMBER_SET     BadSectors
    )
/*++

Routine Description:

    This routine computes which sectors in the given range are bad
    and adds these bad sectors to the bad sectors list.

Arguments:

    StartingSector  - Supplies the starting sector.
    NumberOfSectors - Supplies the number of sectors.
    BadSectors      - Supplies the bad sectors list.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG       MaxSectorsInVerify = 512;

    ULONG       MaxDiskHits;
    BIG_INT     half;
    PBIG_INT    starts;
    PBIG_INT    run_lengths=NULL;
    ULONG       i, n;
    BIG_INT     num_sectors;

    if (NumberOfSectors == 0) {
        return TRUE;
    }

    if (NumberOfSectors.GetHighPart() != 0) {
        DebugPrint("Number of sectors to verify exceeded 32 bits\n");
        return FALSE;
    }


    //
    // Check to see if block length has been set
    //
    if (!_ValidBlockLengthForVerify) 
    {
        num_sectors = min(NumberOfSectors, MaxSectorsInVerify);

        while (!Verify(StartingSector, num_sectors)) 
        {
            if (QueryLastNtStatus() == STATUS_INVALID_BLOCK_LENGTH ||
                QueryLastNtStatus() == STATUS_INVALID_DEVICE_REQUEST) 
            {
                if (num_sectors == 1) {
                    DebugPrint("Number of sectors to verify mysteriously down to 1\n");
                    return FALSE;
                }
                num_sectors = num_sectors / 2;
            } else
                break;
        }

        DebugAssert(num_sectors.GetHighPart() == 0);

        if (QueryLastNtStatus() == STATUS_SUCCESS) {

            if (!(NumberOfSectors < MaxSectorsInVerify &&
                  NumberOfSectors == num_sectors))
                MaxSectorsInVerify = _ValidBlockLengthForVerify = num_sectors.GetLowPart();


            if (num_sectors == NumberOfSectors)
                return TRUE;    // done, return
            else {
                //
                // skip over sectors that has been verified
                //
                StartingSector += num_sectors;
                NumberOfSectors -= num_sectors;
            }
        }
    } else
        MaxSectorsInVerify = _ValidBlockLengthForVerify;


    // Allow 20 retries so that a single bad sector in this region
    // will be found accurately.

    MaxDiskHits = (20 + NumberOfSectors/MaxSectorsInVerify + 1).GetLowPart();

    if (!(starts = NEW BIG_INT[MaxDiskHits]) ||
        !(run_lengths = NEW BIG_INT[MaxDiskHits])) {

        DELETE_ARRAY(starts);
        DELETE_ARRAY(run_lengths);
        return FALSE;
    }

    num_sectors = NumberOfSectors;
    for (i = 0; num_sectors > 0; i++) {
        starts[i] = StartingSector + i*MaxSectorsInVerify;
        if (MaxSectorsInVerify > num_sectors) {
            run_lengths[i] = num_sectors;
        } else {
            run_lengths[i] = MaxSectorsInVerify;
        }
        num_sectors -= run_lengths[i];
    }

    n = i;

    for (i = 0; i < n; i++) {

        if (!Verify(starts[i], run_lengths[i])) {

            if (QueryLastNtStatus() == STATUS_NO_MEDIA_IN_DEVICE) {
                DELETE_ARRAY(starts);
                DELETE_ARRAY(run_lengths);
                return FALSE;
            }

            DebugAssert(QueryLastNtStatus() != STATUS_INVALID_BLOCK_LENGTH &&
                        QueryLastNtStatus() != STATUS_INVALID_DEVICE_REQUEST);

            if (BadSectors == NULL) {
                DELETE_ARRAY(starts);
                DELETE_ARRAY(run_lengths);
                return FALSE;
            }

            if (n + 2 > MaxDiskHits) {

                if (!BadSectors->Add(starts[i], run_lengths[i])) {
                    DELETE_ARRAY(starts);
                    DELETE_ARRAY(run_lengths);
                    return FALSE;
                }

            } else {

                if (run_lengths[i] == 1) {

                    if (!BadSectors->Add(starts[i])) {
                        DELETE_ARRAY(starts);
                        DELETE_ARRAY(run_lengths);
                        return FALSE;
                    }

                } else {

                    half = run_lengths[i]/2;

                    starts[n] = starts[i];
                    run_lengths[n] = half;
                    starts[n + 1] = starts[i] + half;
                    run_lengths[n + 1] = run_lengths[i] - half;

                    n += 2;
                }
            }
        }
    }


    DELETE_ARRAY(starts);
    DELETE_ARRAY(run_lengths);

    return TRUE;
}


BOOLEAN
IO_DP_DRIVE::VerifyWithRead(
    IN  BIG_INT StartingSector,
    IN  BIG_INT NumberOfSectors
    )
/*++

Routine Description:

    This routine verifies the usability of the given range of sectors
    using read.

Arguments:

    StartingSector      - Supplies the starting sector of the verify.
    Number OfSectors    - Supplies the number of sectors to verify.

Return Value:

    FALSE   - At least one of the sectors in the given range was unreadable.
    TRUE    - All of the sectors in the given range are readable.

--*/
{
    HMEM    hmem;
    ULONG   grab;
    BIG_INT i;

    if (!hmem.Initialize() ||
        !hmem.Acquire(MaxIoSize, QueryAlignmentMask())) {

        return FALSE;
    }

    grab = MaxIoSize/QuerySectorSize();
    for (i = 0; i < NumberOfSectors; i += grab) {

        if (NumberOfSectors - i < grab) {
            grab = (NumberOfSectors - i).GetLowPart();
        }

        if (!HardRead(StartingSector + i, grab, hmem.GetBuf())) {
            return FALSE;
        }
    }

    return TRUE;
}


BOOLEAN
IO_DP_DRIVE::Lock(
    )
/*++

Routine Description:

    This routine locks the drive.  If the drive is already locked then
    this routine will do nothing.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    IO_STATUS_BLOCK status_block;

    if (_is_locked) 
    {
        return TRUE;
    }

    _last_status = NtFsControlFile( _handle,
                                    0, NULL, NULL,
                                    &status_block,
                                    FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0);

    _is_locked = (BOOLEAN) NT_SUCCESS(_last_status);

    if (_is_locked) 
    {
        _is_exclusive_write = TRUE;
    }
	else 
    {
        DebugPrintTrace(("Unable to lock the volume (%x)\n", _last_status));
    }

    return _is_locked;
}


BOOLEAN
IO_DP_DRIVE::Unlock(
    )
/*++

Routine Description:

    This routine unlocks the drive.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    IO_STATUS_BLOCK status_block;

    _last_status = NtFsControlFile( _handle,
                                      0, NULL, NULL,
                                      &status_block,
                                      FSCTL_UNLOCK_VOLUME,
                                      NULL, 0, NULL, 0);

    return NT_SUCCESS(_last_status);
}


BOOLEAN
IO_DP_DRIVE::Dismount(
    )
/*++

Routine Description:

    This routine dismounts the drive.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    IO_STATUS_BLOCK status_block;

    if( !NT_SUCCESS(_last_status = NtFsControlFile( _handle,
                                    0, NULL, NULL,
                                    &status_block,
                                    FSCTL_DISMOUNT_VOLUME,
                                    NULL, 0, NULL, 0)) ) {
        return FALSE;
    }

    return TRUE;
}



DEFINE_CONSTRUCTOR( LOG_IO_DP_DRIVE, IO_DP_DRIVE   );


 
LOG_IO_DP_DRIVE::~LOG_IO_DP_DRIVE(
    )
/*++

Routine Description:

    Destructor for LOG_IO_DP_DRIVE.

Arguments:

    None.

Return Value:

    None.

--*/
{
}


 
BOOLEAN
LOG_IO_DP_DRIVE::Initialize(
    IN      PCWSTRING   NtDriveName,
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This routine initializes a LOG_IO_DP_DRIVE object.

Arguments:

    NtDriveName     - Supplies the path of the drive object.
    Message         - Supplies an outlet for messages.
    ExclusiveWrite  - Supplies whether or not to open the drive for
                        exclusive write.
    FormatType      - Supplies the file system type in the event of a format

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    return IO_DP_DRIVE::Initialize(NtDriveName, Message);
}

