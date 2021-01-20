/*++

Module Name:

    drive.hxx

Abstract:

    The drive class hierarchy models the concept of a drive in various
    stages.  It looks like this:

        DRIVE
            DP_DRIVE
                IO_DP_DRIVE
                    LOG_IO_DP_DRIVE
                    PHYS_IO_DP_DRIVE

    DRIVE
    -----

    DRIVE implements a container for the drive path which is recognizable
    by the file system.  'Initialize' takes the path as an argument so
    that it can be later queried with 'GetNtDriveName'.


    DP_DRIVE
    --------

    DP_DRIVE (Drive Parameters) implements queries for the geometry of the
    drive.  'Initiliaze' queries the information from the drive.  What
    is returned is the default drive geometry for the drive.  The user
    may ask by means of 'IsSupported' if the physical device will support
    another MEDIA_TYPE.

    A protected member function called 'SetMediaType' allows the a derived
    class to set the MEDIA_TYPE to another media type which is supported
    by the device.  This method is protected because only a low-level
    format will actually change the media type.


    IO_DP_DRIVE
    -----------

    IO_DP_DRIVE implements the reading and writing of sectors as well
    as 'Lock', 'Unlock', and 'Dismount'.  The 'FormatVerifyFloppy' method
    does a low-level format.  A version of this method allows the user
    to specify a new MEDIA_TYPE for the media.


    LOG_IO_DP_DRIVE and PHYS_IO_DP_DRIVE
    ------------------------------------

    LOG_IO_DP_DRIVE models logical drive.  PHYS_IO_DP_DRIVE models a
    physical drive.  Currently both implementations just initialize
    an IO_DP_DRIVE.  The difference is in the drive path specified.
    Some drive paths are to logical drives and others are to physical
    drives.

--*/

#pragma once


#include "wstring.hxx"
#include "bigint.hxx"


//
//      Forward references
//

DECLARE_CLASS( DRIVE );
DECLARE_CLASS( DP_DRIVE );
DECLARE_CLASS( IO_DP_DRIVE );
DECLARE_CLASS( LOG_IO_DP_DRIVE );

DECLARE_CLASS( NUMBER_SET );
DECLARE_CLASS( MESSAGE );
DECLARE_CLASS( DRIVE_CACHE );

#include "ifsentry.hxx"

DEFINE_TYPE( ULONG, SECTORCOUNT );      // count of sectors
DEFINE_TYPE( ULONG, LBN );              // Logical buffer number

DEFINE_POINTER_AND_REFERENCE_TYPES( MEDIA_TYPE );
DEFINE_POINTER_AND_REFERENCE_TYPES( DISK_GEOMETRY );

struct _DRTYPE {
    MEDIA_TYPE  MediaType;
    ULONG       SectorSize;
    BIG_INT     Sectors;            // w/o hidden sectors.
    BIG_INT     HiddenSectors;
    SECTORCOUNT SectorsPerTrack;
    ULONG       Heads;
};

DEFINE_TYPE( struct _DRTYPE, DRTYPE );

class DRIVE : public OBJECT {

public:

    DECLARE_CONSTRUCTOR(DRIVE);

    VIRTUAL
    ~DRIVE(
          );
     
    BOOLEAN
    Initialize(
              IN      PCWSTRING    NtDriveName,
              IN OUT  PMESSAGE     Message DEFAULT NULL
              );

private:
     
    VOID
    Construct(
             );

    VOID
    Destroy(
           );

    DSTRING _name;
};


class DP_DRIVE : public DRIVE {

public:
     
    DECLARE_CONSTRUCTOR(DP_DRIVE);

    VIRTUAL
     
    ~DP_DRIVE(
             );
    
    BOOLEAN
    Initialize(
              IN      PCWSTRING    NtDriveName,
              IN OUT  PMESSAGE     Message         DEFAULT NULL,
              IN      BOOLEAN      IsTransient     DEFAULT FALSE
              );
     
    MEDIA_TYPE
    QueryMediaType(
                  ) CONST;

    VIRTUAL
    ULONG
    QuerySectorSize(
                   ) CONST;

    VIRTUAL
    BIG_INT
    QuerySectors(
                ) CONST;

     
    BIG_INT
    QueryHiddenSectors(
                      ) CONST;

     
     
    ULONG
    QueryAlignmentMask(
                      ) CONST;

     
    NTSTATUS
    QueryLastNtStatus(
                     ) CONST;


protected:


    // On a normal drive, _handle is a handle to the drive
    HANDLE      _handle;
    NTSTATUS    _last_status;
    PARTITION_INFORMATION_EX    _partition_info;

private:
     
    VOID
    Construct(
             );

     
    VOID
    Destroy(
           );

    STATIC
    NTSTATUS
    OpenDrive(
             IN      PCWSTRING   NtDriveName,
             IN      ACCESS_MASK DesiredAccess,
             IN      BOOLEAN     ExclusiveWrite,
             OUT     PHANDLE     Handle,
             OUT     PULONG      Alignment,
             IN OUT  PMESSAGE    Message
             );

    STATIC
    VOID
    DiskGeometryToDriveType(
                           IN  PCDISK_GEOMETRY DiskGeometry,
                           OUT PDRTYPE         DriveType
                           );

    STATIC
    VOID
    DiskGeometryToDriveType(
                           IN  PCDISK_GEOMETRY DiskGeometry,
                           IN  BIG_INT         NumSectors,
                           IN  BIG_INT         NumHiddenSectors,
                           OUT PDRTYPE         DriveType
                           );

    BOOLEAN
    CheckForPrimaryPartition(
                            );


    DRTYPE      _actual;
    PDRTYPE     _supported_list;
    INT         _num_supported;
    ULONG       _alignment_mask;
    
    BOOLEAN     _is_writeable;
    BOOLEAN     _is_primary_partition;
    DRIVE_TYPE  _drive_type;
};

INLINE
MEDIA_TYPE
DP_DRIVE::QueryMediaType(
                        ) CONST
/*++

Routine Description:

        This routine computes the media type.

Arguments:

    None.

Return Value:

        The media type.

--*/
{
    return _actual.MediaType;
}


INLINE
BIG_INT
DP_DRIVE::QueryHiddenSectors(
                            ) CONST
/*++

Routine Description:

    This routine computes the number of hidden sectors.

Arguments:

    None.

Return Value:

    The number of hidden sectors.

--*/
{
    return _actual.HiddenSectors;
}

INLINE
ULONG
DP_DRIVE::QueryAlignmentMask(
                            ) CONST
/*++

Routine Description:

    This routine returns the memory alignment requirement for the drive.

Arguments:

    None.

Return Value:

    The memory alignment requirement for the drive in the form of a mask.

--*/
{
    return _alignment_mask;
}


INLINE
NTSTATUS
DP_DRIVE::QueryLastNtStatus(
                           ) CONST
/*++

Routine Description:

    This routine returns the last NT status value.

Arguments:

    None.

Return Value:

    The last NT status value.

--*/
{
    return _last_status;
}



class IO_DP_DRIVE : public DP_DRIVE {

    FRIEND class DRIVE_CACHE;

public:

    VIRTUAL
    ~IO_DP_DRIVE(
                );

     
    BOOLEAN
    Read(
        IN  BIG_INT     StartingSector,
        IN  SECTORCOUNT NumberOfSectors,
        OUT PVOID       Buffer
        );

     
    BOOLEAN
    Write(
         IN  BIG_INT     StartingSector,
         IN  SECTORCOUNT NumberOfSectors,
         IN  PVOID       Buffer
         );
     
     
    BOOLEAN
    Verify(
          IN  BIG_INT StartingSector,
          IN  BIG_INT NumberOfSectors
          );

     
    BOOLEAN
    Verify(
          IN      BIG_INT     StartingSector,
          IN      BIG_INT     NumberOfSectors,
          IN OUT  PNUMBER_SET BadSectors
          );

     
    PMESSAGE
    GetMessage(
        );

     
    BOOLEAN
    Lock(
        );


     
    BOOLEAN
    Unlock(
          );


protected:

    DECLARE_CONSTRUCTOR(IO_DP_DRIVE);

     
    BOOLEAN
    Initialize(
              IN      PCWSTRING    NtDriveName,
              IN OUT  PMESSAGE     Message        DEFAULT NULL
              );


private:

    BOOLEAN         _is_locked;
    BOOLEAN         _is_exclusive_write;
    PDRIVE_CACHE    _cache;
    ULONG           _ValidBlockLengthForVerify;
    PMESSAGE        _message;

     
    VOID
    Construct(
             );

     
    VOID
    Destroy(
           );

     
    BOOLEAN
    VerifyWithRead(
                  IN  BIG_INT StartingSector,
                  IN  BIG_INT NumberOfSectors
                  );

     
    BOOLEAN
    HardRead(
            IN  BIG_INT     StartingSector,
            IN  SECTORCOUNT NumberOfSectors,
            OUT PVOID       Buffer
            );

     
    BOOLEAN
    HardWrite(
             IN  BIG_INT     StartingSector,
             IN  SECTORCOUNT NumberOfSectors,
             IN  PVOID       Buffer
             );

     
    BOOLEAN
    Dismount(
            );

};


INLINE
PMESSAGE
IO_DP_DRIVE::GetMessage(
                          )
/*++

Routine Description:

    Retrieve the message object.

Arguments:

    N/A

Return Value:

    The message object.

--*/
{
    return _message;
}

class LOG_IO_DP_DRIVE : public IO_DP_DRIVE {

public:

     
    DECLARE_CONSTRUCTOR(LOG_IO_DP_DRIVE);

    VIRTUAL
     
    ~LOG_IO_DP_DRIVE(
                    );

     
    BOOLEAN
    Initialize(
              IN      PCWSTRING    NtDriveName,
              IN OUT  PMESSAGE     Message        DEFAULT NULL
              );

private:

    VOID
    Construct(
             );


};


INLINE
VOID
LOG_IO_DP_DRIVE::Construct(
                          )
/*++

Routine Description:

    Constructor for LOG_IO_DP_DRIVE.

Arguments:

    None.

Return Value:

    None.

--*/
{
}


