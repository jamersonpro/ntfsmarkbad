/*++

Module Name:

    dcache.hxx

Abstract:

    This class models a general cache for reading and writing.
    The actual implementation of this base class is to not have
    any cache at all.

--*/

#pragma once

#include "bigint.hxx"
#include "drive.hxx"

DECLARE_CLASS(DRIVE_CACHE);

class DRIVE_CACHE : public OBJECT {

    public:

        DECLARE_CONSTRUCTOR( DRIVE_CACHE );

        VIRTUAL
        ~DRIVE_CACHE(
            );
         
        BOOLEAN
        Initialize(
            IN OUT  PIO_DP_DRIVE    Drive
            );

        VIRTUAL
		BOOLEAN
		Read(
			IN  BIG_INT     StartingSector,
			IN  SECTORCOUNT NumberOfSectors,
			OUT PVOID       Buffer
			);

        VIRTUAL
		BOOLEAN
		Write(
			IN  BIG_INT     StartingSector,
			IN  SECTORCOUNT NumberOfSectors,
			IN  PVOID       Buffer
            );

    private:
         
		VOID
		Construct(
			);

         
        VOID
        Destroy(
            );

        PIO_DP_DRIVE    _drive;
};

