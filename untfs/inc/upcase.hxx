/*++

Module Name:

    upcase.hxx

Abstract:

    This module contains the declarations for the NTFS_UPCASE_TABLE
    class.  This class models the upcase table stored on an NTFS volume,
    which is used to upcase attribute names and file names resident
    on that volume.

--*/

#pragma once

DECLARE_CLASS( NTFS_ATTRIBUTE );
DECLARE_CLASS( NTFS_BITMAP );
DECLARE_CLASS( NTFS_UPCASE_TABLE );

#include "attrib.hxx"

// This function is used to compare two NTFS names.  Its definition
// appears in upcase.cxx.
//
LONG
 
NtfsUpcaseCompare(
    IN PCWSTR               LeftName,
    IN ULONG                LeftNameLength,
    IN PCWSTR               RightName,
    IN ULONG                RightNameLength,
    IN PCNTFS_UPCASE_TABLE  UpcaseTable,
    IN BOOLEAN              CaseSensitive
    );

class NTFS_UPCASE_TABLE : public OBJECT {

    public:

         
        DECLARE_CONSTRUCTOR( NTFS_UPCASE_TABLE );

        VIRTUAL
         
        ~NTFS_UPCASE_TABLE(
            );

         
        BOOLEAN
        Initialize(
            IN PNTFS_ATTRIBUTE Attribute
            );

         
        WCHAR
        UpperCase(
            IN WCHAR Character
            ) CONST;


        STATIC
        ULONG
        QueryDefaultLength(
            );



    private:

         
        VOID
        Construct(
            );

         
        VOID
        Destroy(
            );

        PWCHAR  _Data;
        ULONG   _Length;
};

 
INLINE
WCHAR
NTFS_UPCASE_TABLE::UpperCase(
    IN WCHAR Character
    ) CONST
/*++

Routine Description:

    This method returns the upper-case value of the supplied
    character.

Arguments:

    Character   --  Supplies the character to upcase.

Notes:

    If Character is not in the table (ie. is greater or equal
    to _Length), it upcases to itself.

--*/
{
    return( (Character < _Length) ? _Data[Character] : Character );
}


 
INLINE
ULONG
NTFS_UPCASE_TABLE::QueryDefaultLength(
    )
/*++

Routine Description:

    This method returns the length (in characters) of the
    default upcase table.

Arguments:

    None.

Return Value:

    The length (in characters) of the default upcase table.

--*/
{
    return 0x10000;
}

