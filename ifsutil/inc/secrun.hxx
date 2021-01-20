/*++

Module Name:

    secrun.hxx

Abstract:

    This class models a run of sectors.  It is able to read in a run of
    sectors into memory and write a run of sectors in memory onto disk.

--*/

#pragma once


#include "drive.hxx"
#include "mem.hxx"




DECLARE_CLASS( SECRUN );

class SECRUN : public OBJECT {

    public:

         
        DECLARE_CONSTRUCTOR( SECRUN );

        VIRTUAL
         
        ~SECRUN(
                );

         
         
        BOOLEAN
        Initialize(
            IN OUT  PMEM            Mem,
            IN OUT  PIO_DP_DRIVE    Drive,
            IN      BIG_INT         StartSector,
            IN      SECTORCOUNT     NumSectors
            );

         
        VOID
        Relocate(
            IN  BIG_INT NewStartSector
            );

        VIRTUAL
         
        BOOLEAN
        Read(
            );

        VIRTUAL
         
        BOOLEAN
        Write(
            );

         
        PVOID
        GetBuf(
            );

         
        BIG_INT
        QueryStartSector(
            ) CONST;

         
        LBN
        QueryStartLbn(
            ) CONST;

         
        SECTORCOUNT
        QueryLength(
            ) CONST;

         
         
        PIO_DP_DRIVE
        GetDrive(
            );

    private:

         
        VOID
        Construct (
            );

         
        VOID
        Destroy(
            );

        PVOID           _buf;
        PIO_DP_DRIVE    _drive;
        BIG_INT         _start_sector;
        SECTORCOUNT     _num_sectors;

};

INLINE
VOID
SECRUN::Relocate(
    IN  BIG_INT NewStartSector
    )
/*++

Routine Description:

    This routine relocates the secrun.

Arguments:

    NewStartSector  - Supplies the new starting sector.

Return Value:

    None.

--*/
{
    _start_sector = NewStartSector;
}


INLINE
PVOID
SECRUN::GetBuf(
    )
/*++

Routine Description:

    This routine returns a pointer to the beginning of the read/write
    buffer.

Arguments:

    None.

Return Value:

    A pointer to a read/write buffer.

--*/
{
    return _buf;
}


INLINE
BIG_INT
SECRUN::QueryStartSector(
    ) CONST
/*++

Routine Description:

    This routine returns the starting sector number for the run of sectors.

Arguments:

    None.

Return Value:

    The starting sector number for the run of sectors.

--*/
{
    return _start_sector;
}


INLINE
LBN
SECRUN::QueryStartLbn(
    ) CONST
/*++

Routine Description:

    This routine returns the starting LBN for the run of sectors.

Arguments:

    None.

Return Value:

    The starting LBN for the run of sectors.

--*/
{
    DebugAssert(_start_sector.GetHighPart() == 0);
    return _start_sector.GetLowPart();
}


INLINE
SECTORCOUNT
SECRUN::QueryLength(
    ) CONST
/*++

Routine Description:

    This routine computes the number of sectors in this sector run.

Arguments:

    None.

Return Value:

    The number of sectors in this sector run.

--*/
{
    return _num_sectors;
}


INLINE
PIO_DP_DRIVE
SECRUN::GetDrive(
    )
/*++

Routine Description:

    This routine returns a pointer to the drive object.

Arguments:

    None.

Return Value:

    A pointer to the drive object.

--*/
{
    return _drive;
}

