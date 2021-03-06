#include "stdafx.h"

/*++

Module Name:

    attrrec.hxx

Abstract:

    This module contains the member function definitions for
    NTFS_ATTRIBUTE_RECORD, which models NTFS attribute records.

    An Attribute Record may be a template laid over a chunk of
    memory; in that case, it does not own the memory.  It may
    also be told, upon initialization, to allocate its own memory
    and copy the supplied data.  In that case, it is also responsible
    for freeing that memory.

    Attribute Records are passed between Attributes and File
    Record Segments.  A File Record Segment can initialize
    an Attribute with a list of Attribute Records; when an
    Attribute is Set into a File Record Segment, it packages
    itself up into Attribute Records and inserts them into
    the File Record Segment.

    File Record Segments also use Attribute Records to scan
    through their list of attribute records, and to shuffle
    them around.

--*/


#include "ulib.hxx"

#include "untfs.hxx"

#include "wstring.hxx"
#include "extents.hxx"
#include "attrrec.hxx"
#include "ntfsbit.hxx"
#include "extents.hxx"
#include "upcase.hxx"


DEFINE_CONSTRUCTOR( NTFS_ATTRIBUTE_RECORD, OBJECT   );

 
NTFS_ATTRIBUTE_RECORD::~NTFS_ATTRIBUTE_RECORD(
    )
{
    Destroy();
}

VOID
NTFS_ATTRIBUTE_RECORD::Construct(
    )
/*++

Routine Description:

    This method is the private worker function for object construction.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _Data = NULL;
    _MaximumLength = 0;
    _IsOwnBuffer = FALSE;
    _DisableUnUse = FALSE;
    _Drive = NULL;
}

VOID
NTFS_ATTRIBUTE_RECORD::Destroy(
    )
/*++

Routine Description:

    This method is the private worker function for object destruction.

Arguments:

    None.

Return Value:

    None.

--*/
{
    if( _IsOwnBuffer && _Data != NULL ) {

        FREE( _Data );
    }

    _Data = NULL;
    _MaximumLength = 0;
    _IsOwnBuffer = FALSE;
    _Drive = NULL;
}

 
BOOLEAN
NTFS_ATTRIBUTE_RECORD::Initialize(
    IN      PIO_DP_DRIVE    Drive,
    IN OUT  PVOID           Data,
    IN      ULONG           MaximumLength,
    IN      BOOLEAN         MakeCopy
    )
/*++

Routine Description:

    This method initializes an NTFS_ATTRIBUTE_RECORD object,
    handing it a buffer with attribute record data.  The caller
    may also ask the object to make a private copy of the data.

Arguments:

    Drive           -- supplies the drive on which the attribute record resides
    Data            -- supplies a buffer containing the attribute
                        record data the object will own.
    MaximumLength   -- supplies the size of the buffer.
    MakeCopy        -- supplies a flag indicating whether the object
                        should copy the data to a private buffer.

Return Value:

    TRUE upon successful completion.

Notes:

    If MakeCopy is TRUE, then the object must allocate its own
    buffer and copy the attribute record data to it, in which
    case the object is also responsible for freeing that private
    buffer.  It that flag is FALSE, then the object will cache a
    pointer to the buffer supplied by the client; the client is
    responsible for making sure that buffer remains valid for
    the lifetime of the NTFS_ATTRIBUTE_RECORD object.

    This object is reinitializable.

--*/
{
    Destroy();

    if( !MakeCopy ) {

        _Data = (PATTRIBUTE_RECORD_HEADER) Data;
        _MaximumLength = MaximumLength;
        _IsOwnBuffer = FALSE;
        _Drive = Drive;

        return TRUE;

    } else {

        if( (_Data = (PATTRIBUTE_RECORD_HEADER)
                     MALLOC( (UINT) MaximumLength )) == NULL ) {

            Destroy();
            return FALSE;
        }

        _MaximumLength = MaximumLength;
        _IsOwnBuffer = TRUE;
        _Drive = Drive;
        memcpy(_Data, Data, (UINT) MaximumLength);

        return TRUE;
    }
}

 
 
BOOLEAN
NTFS_ATTRIBUTE_RECORD::Initialize(
    IN      PIO_DP_DRIVE    Drive,
    IN OUT  PVOID           Data
    )
/*++

Routine Description:

    This version of Initialize takes it's maximum size from the
    attribute record.

Arguments:

    Drive   - supplies the drive on which the attribute record resides
    Data    - supplies a buffer containing the attribute
                record data.

Return Value:

    TRUE upon successful completion.

--*/
{
    Destroy();

    _Data = (PATTRIBUTE_RECORD_HEADER) Data;
    _MaximumLength = _Data->RecordLength;
    _IsOwnBuffer = FALSE;
    _Drive = Drive;

    return TRUE;
}
 
BOOLEAN
NTFS_ATTRIBUTE_RECORD::CreateResidentRecord(
    IN  PCVOID              Value,
    IN  ULONG               ValueLength,
    IN  ATTRIBUTE_TYPE_CODE TypeCode,
    IN  PCWSTRING           Name,
    IN  USHORT              Flags,
    IN  UCHAR               ResidentFlags
    )
/*++

Routine Description:

    This method formats the object's buffer with a resident
    attribute record.

Arguments:

    Value           -- supplies the attribute value
    ValueLength     -- supplies the length of the value
    TypeCode        -- supplies the attribute type code
    Name            -- supplies the name of the attribute
                        (may be NULL)
    Flags           -- supplies the attribute's flags.
    ResidentFlags   -- supplies the attribute's resident flags

Return Value:

    TRUE upon successful completion.

--*/
{
    // Clear the memory first.
    memset(_Data, 0, (UINT) _MaximumLength);

    // We will arrange the attribute in the following order:
    //  Attribute Record Header
    //  Name (if any)
    //  Value

    if( _MaximumLength < SIZE_OF_RESIDENT_HEADER )  {

        DebugAbort( "Create:  buffer is too small.\n" );
        return FALSE;
    }

    _Data->TypeCode = TypeCode;
    _Data->FormCode = RESIDENT_FORM;
    _Data->Flags = Flags;

    if( Name != NULL ) {

        _Data->NameLength = (UCHAR) Name->QueryChCount();
        _Data->NameOffset = QuadAlign(SIZE_OF_RESIDENT_HEADER);

        //
        // The structure should be quad aligned already.  This check is just in case.
        //
        DebugAssert(QuadAlign(SIZE_OF_RESIDENT_HEADER) == SIZE_OF_RESIDENT_HEADER);

        _Data->Form.Resident.ValueOffset =
            QuadAlign( _Data->NameOffset +
                        _Data->NameLength * sizeof( WCHAR ) );

    } else {

        _Data->NameLength = 0;
        _Data->NameOffset = 0;

        _Data->Form.Resident.ValueOffset =
                    QuadAlign(SIZE_OF_RESIDENT_HEADER);
    }

    _Data->Form.Resident.ValueLength = ValueLength;
    _Data->Form.Resident.ResidentFlags = ResidentFlags;

    _Data->RecordLength =
        QuadAlign(_Data->Form.Resident.ValueOffset + ValueLength );

    if( _Data->RecordLength > _MaximumLength ) {

        return FALSE;
    }

    // Now that we're sure there's room, copy the name (if any)
    // and the value into their respective places.

    if( Name != NULL ) {

        Name->QueryWSTR( 0,
                         _Data->NameLength,
                         (PWSTR)((PBYTE)_Data + _Data->NameOffset),
                         _Data->NameLength,
                         FALSE );
    }

    memcpy( (PBYTE)_Data + _Data->Form.Resident.ValueOffset,
            Value,
            (UINT) ValueLength );

    return TRUE;
}

 
BOOLEAN
NTFS_ATTRIBUTE_RECORD::CreateNonresidentRecord(
    IN  PCNTFS_EXTENT_LIST  Extents,
    IN  BIG_INT             AllocatedLength,
    IN  BIG_INT             ActualLength,
    IN  BIG_INT             ValidLength,
    IN  ATTRIBUTE_TYPE_CODE TypeCode,
    IN  PCWSTRING           Name,
    IN  USHORT              Flags,
    IN  USHORT              CompressionUnit,
    IN  ULONG               ClusterSize
    )
/*++

Routine Description:

    This method formats the attribute record to hold a nonresident
    attribute.

Arguments:

    Extents         -- supplies an extent list describing the
                        attribute value's disk storage.
    AllocatedLength -- supplies the allocated length of the value
    ActualLength    -- supplies the actual length of the value
    ValidLength     -- supplies the valid length of the value
    TypeCode        -- supplies the attribute type code
    Name            -- supplies the name of the attribute
                        (may be NULL)
    Flags           -- supplies the attribute's flags.
    CompressionUnit -- supplies the log in base 2 of the number of
                        clusters per compression unit.

--*/
{
    ULONG   MappingPairsLength;
    VCN     NextVcn, HighestVcn;
    USHORT  sizeOfNonResidentHeader = SIZE_OF_NONRESIDENT_HEADER;

    if (Flags & (ATTRIBUTE_FLAG_COMPRESSION_MASK |
                 ATTRIBUTE_FLAG_SPARSE)) {
        sizeOfNonResidentHeader += sizeof(BIG_INT); // TotalAllocated field
    }

    // Clear the memory first.
    memset(_Data, 0, (UINT) _MaximumLength);

    // We will arrange the attribute in the following order:
    //  Attribute Record Header
    //  Name (if any)
    //  Compressed Mapping Pairs

    if( _MaximumLength < sizeOfNonResidentHeader )   {

        DebugAbort( "Create:  buffer is too small.\n" );
        return FALSE;
    }

    _Data->TypeCode = TypeCode;
    _Data->FormCode = NONRESIDENT_FORM;
    _Data->Flags = Flags;

    if( Name != NULL ) {

        _Data->NameLength = (UCHAR) Name->QueryChCount();
        _Data->NameOffset = QuadAlign(sizeOfNonResidentHeader);

        //
        // The structure should be quad aligned already.  This check is just in case.
        //
        DebugAssert(QuadAlign(sizeOfNonResidentHeader) == sizeOfNonResidentHeader);

        _Data->Form.Nonresident.MappingPairsOffset =
            (USHORT)QuadAlign( _Data->NameOffset +
                                _Data->NameLength * sizeof( WCHAR ) );

    } else {

        _Data->NameLength = 0;
        _Data->NameOffset = 0;

        _Data->Form.Nonresident.MappingPairsOffset =
            (USHORT)QuadAlign(sizeOfNonResidentHeader);
    }

    _Data->Form.Nonresident.CompressionUnit = (UCHAR)CompressionUnit;

    _Data->Form.Nonresident.AllocatedLength =
                    AllocatedLength.GetLargeInteger();

    if (Flags & (ATTRIBUTE_FLAG_COMPRESSION_MASK |
                 ATTRIBUTE_FLAG_SPARSE)) {
        _Data->Form.Nonresident.TotalAllocated =
            (Extents->QueryClustersAllocated()*ClusterSize).GetLargeInteger();
    }
    _Data->Form.Nonresident.FileSize = ActualLength.GetLargeInteger();
    _Data->Form.Nonresident.ValidDataLength = ValidLength.GetLargeInteger();


    // Copy the name

    if( Name != NULL ) {

        if( (ULONG)(_Data->NameOffset + _Data->NameLength) > _MaximumLength ) {

            // There isn't enough room for the name.

            return FALSE;
        }

        Name->QueryWSTR( 0,
                         _Data->NameLength,
                         (PWSTR)((PBYTE)_Data + _Data->NameOffset),
                         _Data->NameLength,
                         FALSE );
    }


    if( !Extents->QueryCompressedMappingPairs(
                        (PVCN)&(_Data->Form.Nonresident.LowestVcn),
                        &NextVcn,
                        &MappingPairsLength,
                        _MaximumLength -
                          _Data->Form.Nonresident.MappingPairsOffset,
                        (PVOID)((PBYTE)_Data +
                          _Data->Form.Nonresident.MappingPairsOffset) ) ) {

        // Unable to get the compressed mapping pairs.

        DebugPrint( "Could not get compressed mapping pairs.\n" );
        return FALSE;
    }

    HighestVcn = NextVcn - 1;
    memcpy( &_Data->Form.Nonresident.HighestVcn, &HighestVcn, sizeof(VCN) );

    _Data->RecordLength =
        QuadAlign(_Data->Form.Nonresident.MappingPairsOffset +
                  MappingPairsLength );

    return TRUE;
}


BOOLEAN
NTFS_ATTRIBUTE_RECORD::UseClusters(
    IN OUT  PNTFS_BITMAP    VolumeBitmap,
    OUT     PBIG_INT        ClusterCount
    ) CONST
/*++

Routine Description:

    This routine allocates the disk space claimed by this attribute
    record in the bitmap provided.  A check is made to verify that
    the requested disk space is free before the allocation takes
    place.  If the requested space is not available in the bitmap
    then this routine will return FALSE.

Arguments:

    VolumeBitmap    - Supplies the bitmap.
    ClusterCount    - Receives the number of clusters allocated
                      to this record.  Not set if method fails.

Return Value:

    FALSE   - The request bitmap space was not available.
    TRUE    - Success.

--*/
{
    NTFS_EXTENT_LIST    extent_list;
    ULONG               num_extents;
    ULONG               i, j;
    VCN                 next_vcn;
    LCN                 current_lcn;
    BIG_INT             run_length;

    DebugAssert(VolumeBitmap);

    if (IsResident()) {
        *ClusterCount = 0;
        return TRUE;
    }

    if (!QueryExtentList(&extent_list)) {
        return FALSE;
    }

    num_extents = extent_list.QueryNumberOfExtents();

    for (i = 0; i < num_extents; i++) {

        if (!extent_list.QueryExtent(i, &next_vcn, &current_lcn,
                                     &run_length)) {
            return FALSE;
        }

        if (current_lcn == LCN_NOT_PRESENT) {
            continue;
        }


        // Make sure that the run is free before allocating.
        // If it is not, this indicates a cross-link.

        if (!VolumeBitmap->IsFree(current_lcn, run_length)) 
        {

            DebugPrintTrace(("cross-linked run starts at 0x%I64x for 0x%I64x\n",
                             current_lcn.GetLargeInteger(),
                             run_length.GetLargeInteger()));

            // Free everything so far allocated by this routine.

            for (j = 0; j < i; j++) {

                if (!extent_list.QueryExtent(j, &next_vcn, &current_lcn,
                                             &run_length)) {

                    DebugAbort("Could not query extent");
                    return FALSE;
                }
                if (current_lcn == LCN_NOT_PRESENT) {
                    continue;
                }

                VolumeBitmap->SetFree(current_lcn, run_length);
            }

            return FALSE;
        }

        VolumeBitmap->SetAllocated(current_lcn, run_length);
    }

    *ClusterCount = extent_list.QueryClustersAllocated();
    return TRUE;
}

 
BOOLEAN
NTFS_ATTRIBUTE_RECORD::QueryName(
    OUT PWSTRING    Name
    ) CONST
/*++

Routine Description:

    This method returns the name of the attribute.

Arguments:

    Name    - Returns the name of the attribute.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    if (FIELD_OFFSET(ATTRIBUTE_RECORD_HEADER, Flags) >= _MaximumLength ||
        ULONG(_Data->NameOffset + _Data->NameLength) > _MaximumLength ||
        _Data->NameLength == 0) {

        return Name->Initialize( "" );

    } else {

        return Name->Initialize((PWSTR)((PBYTE)_Data + _Data->NameOffset),
                                _Data->NameLength);

    }
}

 
VOID
NTFS_ATTRIBUTE_RECORD::QueryValueLength(
    OUT PBIG_INT ValueLength,
    OUT PBIG_INT AllocatedLength,
    OUT PBIG_INT ValidLength,
    OUT PBIG_INT TotalAllocated
    ) CONST
/*++

Routine Description:

    This method returns the actual, allocated, valid, and
    total allocated lengths
    of the attribute value associated with this record.

    If the attribute is resident, these values are all
    the length of the resident value, except total allocated,
    which is meaningless.

    If the attribute is nonresident, these four values are only
    meaningful if the LowestVcn of this attribute record is 0.
    Additionally, TotalAllocated is only valid for compressed
    attributes.

Arguments:

    ValueLength     -- receives the actual length of the value.
    AllocatedLength -- receives the allocated size of the value.
                        (may be NULL if the caller doesn't care)
    ValidLength     -- receives the valid length of the value.
                        (may be NULL if the caller doesn't care)
    TotalAllocated  -- receives the total allocated length of the
                        value (may be NULL).

Return Value:

    None.

--*/
{
    DebugPtrAssert( _Data );

    if( _Data->FormCode == RESIDENT_FORM ) {

        *ValueLength = _Data->Form.Resident.ValueLength;

        if( AllocatedLength != NULL ) {

            *AllocatedLength = _Data->Form.Resident.ValueLength;
        }

        if( ValidLength != NULL ) {

            *ValidLength = _Data->Form.Resident.ValueLength;
        }

        if (TotalAllocated != NULL ) {

            // no such value for resident attributes

            *TotalAllocated = 0;
        }

    } else {

        DebugAssert( _Data->FormCode == NONRESIDENT_FORM );

        *ValueLength = _Data->Form.Nonresident.FileSize;

        if( AllocatedLength != NULL ) {

            *AllocatedLength = _Data->Form.Nonresident.AllocatedLength;
        }

        if( ValidLength != NULL ) {

            *ValidLength = _Data->Form.Nonresident.ValidDataLength;
        }

        if (TotalAllocated != NULL) {
            if ((_Data->Flags & (ATTRIBUTE_FLAG_COMPRESSION_MASK |
                                 ATTRIBUTE_FLAG_SPARSE)) != 0) {
                *TotalAllocated = _Data->Form.Nonresident.TotalAllocated;
            } else {
                *TotalAllocated = 0;
            }
        }

    }
}

VOID
NTFS_ATTRIBUTE_RECORD::SetTotalAllocated(
    IN BIG_INT TotalAllocated
    )
/*++

Routine Description:

    Set the "TotalAllocated" field in the attribute record.  If the
    attribute record doesn't have a total allocated field because
    the attribute isn't compressed or because it's resident, this
    method has no effect.

Arguments:

    TotalAllocated - the new value.

Return Value:

    None.

--*/
{
    DebugPtrAssert( _Data );

    if( _Data->FormCode == RESIDENT_FORM ) {

        // no such value for resident attributes; ignore

        return;

    }

    DebugAssert( _Data->FormCode == NONRESIDENT_FORM );

    if ((_Data->Flags & (ATTRIBUTE_FLAG_COMPRESSION_MASK |
                         ATTRIBUTE_FLAG_SPARSE)) != 0) {
        _Data->Form.Nonresident.TotalAllocated =
            TotalAllocated.GetLargeInteger();
    }
}

 
 
BOOLEAN
NTFS_ATTRIBUTE_RECORD::QueryExtentList(
    OUT PNTFS_EXTENT_LIST   ExtentList
    ) CONST
/*++

Routine Description:

Arguments:

    None.

Return Value:

    A pointer to the extent list.  A return value of NULL indicates
    that the attribute is resident or that an error occurred processing
    the compressed mapping pairs.  (Clients should use IsResident to
    determine whether the attribute value is resident.)

--*/
{
    DebugPtrAssert( _Data );

    if( _Data->FormCode == NONRESIDENT_FORM &&
        ExtentList->Initialize( _Data->Form.Nonresident.LowestVcn,
                                (PVOID)((PBYTE)_Data +
                                    _Data->Form.Nonresident.MappingPairsOffset),
                                 _MaximumLength -
                                    _Data->Form.Nonresident.
                                        MappingPairsOffset ) ) {

        return TRUE;

    } else {

        return FALSE;
    }
}

 
BOOLEAN
NTFS_ATTRIBUTE_RECORD::IsMatch(
    IN  ATTRIBUTE_TYPE_CODE Type,
    IN  PCWSTRING           Name,
    IN  PCVOID              Value,
    IN  ULONG               ValueLength
    ) CONST
/*++

Routine Description:

    This method determines whether the attribute record matches the
    parameters given.

Arguments:

    Type        --  Supplies the type code of the attribute.  This
                        is the primary key, and must always be present.
    Name        --  Supplies a name to match.  A name of NULL is the
                        same as specifying the null string.
    Value       --  Supplies the value to match.  If this argument is
                        null, any value matches.  Only resident
                        attributes can be checked for value matches.
    ValueLength --  Supplies the length of the value (if any).

Notes:

    Value matching is not supported for nonresident attribute values;
    if a Value parameter is supplied, then no non-resident attribute
    records will match.

--*/
{
    DSTRING RecordName;

    DebugPtrAssert( _Data );

    if( Type != _Data->TypeCode ) {

        return FALSE;
    }

    if( Name != NULL ) {

        if( !RecordName.Initialize((PWSTR)((PBYTE)_Data + _Data->NameOffset),
                                   _Data->NameLength ) ) {

            return FALSE;
        }

        if( Name->Strcmp( &RecordName ) != 0 ) {

            return FALSE;
        }
    } else if (_Data->NameLength) {
        return FALSE;
    }

    if( Value != NULL &&
        ( _Data->FormCode != RESIDENT_FORM ||
          ValueLength != _Data->Form.Resident.ValueLength ||
          memcmp( Value,
                  (PBYTE)_Data + _Data->Form.Resident.ValueOffset,
                  (UINT) ValueLength ) ) ) {

        return FALSE;
    }

    return TRUE;
}

 
LONG
CompareAttributeRecords(
    IN  PCNTFS_ATTRIBUTE_RECORD Left,
    IN  PCNTFS_ATTRIBUTE_RECORD Right,
    IN  PCNTFS_UPCASE_TABLE     UpcaseTable
    )
/*++

Routine Description:

    This method compares two attribute records to determine their
    correct ordering in the File Record Segment.

Arguments:

    Left        --  Supplies the left-hand operand of the comparison.
    Right       --  Supplies the right-hand operand of the comparison.
    UpcaseTable --  Supplies the upcase table for the volume.
                    If this parameter is NULL, name comparison
                    cannot be performed.

Return Value:

    <0 if Left is less than Right
     0 if Left equals Right
    >0 if Left is greater than Right.

Notes:

    Attribute records are ordered first by type code and then
    by name.  An attribute record without a name is less than
    any attribute record of the same type with a name.

    Name comparision is first done case-insensitive; if the names
    are equal by that metric, a case-sensitive comparision is made.

    The UpcaseTable parameter may be omitted if either or both names
    are zero-length, or if they are identical (including case).
    Otherwise, it must be supplied.

--*/
{
    ULONG Result;

    // First, compare the type codes:
    //
    Result = Left->QueryTypeCode() - Right->QueryTypeCode();

    if( Result != 0 ) {

        return Result;
    }

    // They have the same type code, so we have to compare the names.
    // Pass in TRUE for the IsAttribute parameter, to indicate that
    // we are comparing attribute names.
    //
    return( NtfsUpcaseCompare( Left->GetName(),
                               Left->QueryNameLength(),
                               Right->GetName(),
                               Right->QueryNameLength(),
                               UpcaseTable,
                               TRUE ) );
}
