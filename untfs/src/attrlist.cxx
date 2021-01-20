#include "stdafx.h"

/*++

Module Name:

    attrlist.cxx

Abstract:

    This module contains the member function definitions for
    NTFS_ATTRIBUTE_LIST, which models an ATTRIBUTE_LIST Attribute
    in an NTFS File Record Segment.

    If a file has any external attributes (i.e. if it has more than
    one File Record Segment), then it will have an ATTRIBUTE_LIST
    attribute.  This attribute's value consists of a series of
    Attribute List Entries, which describe the attribute records
    in the file's File Record Segments.  There is an entry for each
    attribute record attached to the file, including the attribute
    records in the base File Record Segment, and in particular
    including the attribute records which describe the ATTRIBUTE_LIST
    attribute itself.

    An entry in the Attribute List gives the type code and name (if any)
    of the attribute, along with the LowestVcn of the attribute record
    (zero if the attribute record is Resident) and a segment reference
    (which combines an MFT VCN with a sequence number) showing where
    the attribute record may be found.

    The entries in the Attribute List are sorted first by attribute
    type code and then by name.  Note that two attributes can have the
    same type code and name only if they can be distinguished by
    value.


--*/


#include "ulib.hxx"

#include "untfs.hxx"

#include "wstring.hxx"

#include "attrlist.hxx"
#include "attrrec.hxx"
#include "upcase.hxx"

#include "message.hxx"
#include "ntfsbit.hxx"



DEFINE_CONSTRUCTOR( NTFS_ATTRIBUTE_LIST, NTFS_ATTRIBUTE );


NTFS_ATTRIBUTE_LIST::~NTFS_ATTRIBUTE_LIST(
    )
{
    Destroy();
}

VOID
NTFS_ATTRIBUTE_LIST::Construct(
    )
/*++

Routine Description:

    Worker function for object construction.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _LengthOfList = 0;
    _UpcaseTable = NULL;
}

VOID
NTFS_ATTRIBUTE_LIST::Destroy(
    )
/*++

Routine Description:

    Worker function for object destruction.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _LengthOfList = 0;
    _UpcaseTable = NULL;
}

 
BOOLEAN
NTFS_ATTRIBUTE_LIST::Initialize(
    IN OUT  PLOG_IO_DP_DRIVE    Drive,
    IN      ULONG               ClusterFactor,
    IN      PNTFS_UPCASE_TABLE  UpcaseTable
    )
/*++

Routine Description:

    This method initializes an empty attribute list.

Arguments:

    Drive           -- supplies the drive on which the attribute list resides
    ClusterFactor   -- supplies the cluster factor for that drive
    UpcaseTable     -- supplies the volume upcase table.

Return Value:

    TRUE upon successful completion.

Notes:

    UpcaseTable may be NULL if the client will never compare
    named attribute records.

--*/
{
    Destroy();

    if( !_Mem.Initialize() ||
        !NTFS_ATTRIBUTE::Initialize( Drive,
                                     ClusterFactor,
                                     NULL,
                                     0,
                                     $ATTRIBUTE_LIST ) ) {

        return FALSE;
    }

    _UpcaseTable = UpcaseTable;
    return TRUE;
}

 
BOOLEAN
NTFS_ATTRIBUTE_LIST::Initialize(
    IN OUT  PLOG_IO_DP_DRIVE        Drive,
    IN      ULONG                   ClusterFactor,
    IN      PCNTFS_ATTRIBUTE_RECORD AttributeRecord,
    IN      PNTFS_UPCASE_TABLE      UpcaseTable
    )
/*++

Routine Description:

    This method initializes an attribute list based on an
    attribute record.

Arguments:

    Drive           -- supplies the drive on which the attribute list resides
    ClusterFactor   -- supplies the cluster factor for that drive
    AttributeRecord -- supplies the attribute record describing the
                        attribute list.
    UpcaseTable     -- supplies the volume upcase table.

Return Value:

    TRUE upon successful completion.

Notes:

    This method does not read the attribute list.

    UpcaseTable may be NULL if the client will never compare
    named attribute records.

--*/
{
    Destroy();

    if( !_Mem.Initialize() ||
        !NTFS_ATTRIBUTE::Initialize( Drive,
                                     ClusterFactor,
                                     AttributeRecord ) ) {

        return FALSE;
    }

    _UpcaseTable = UpcaseTable;
    return TRUE;
}
 
BOOLEAN
NTFS_ATTRIBUTE_LIST::AddEntry(
    IN ATTRIBUTE_TYPE_CODE      Type,
    IN VCN                      LowestVcn,
    IN PCMFT_SEGMENT_REFERENCE  SegmentReference,
    IN USHORT                   InstanceTag,
    IN PCWSTRING                Name
    )
/*++

Routine Description:

    This adds an Attribute List Entry to the list.

Arguments:

    Type                --  supplies the attribute type code of the
                            attribute record corresponding to this entry
    LowestVcn           --  supplies the record's LowestVcn
    SegmentReference    --  supplies the location of the record
    InstanceTag         --  supplies the record's attribute instance tag.
    Name                --  supplies the name associated with the
                            record (NULL if it has no name).

Return Value:

    TRUE upon successful completion.

--*/
{
    ULONG LengthOfNewEntry;
    ULONG NewLengthOfList;
    PATTRIBUTE_LIST_ENTRY CurrentEntry;
    ULONG EntryOffset, NameLength;


    // Compute the size of the new entry and the new length of the
    // list with this entry added.
    //
    NameLength = ( Name == NULL ) ? 0 : (Name->QueryChCount());

    LengthOfNewEntry = QuadAlign( sizeof(ATTRIBUTE_LIST_ENTRY) +
                                            NameLength * sizeof(WCHAR) );

    NewLengthOfList = _LengthOfList + LengthOfNewEntry;

    // If our existing buffer isn't big enough, stretch it to
    // hold the new entry.

    if( !_Mem.Resize( NewLengthOfList ) ) {

        return FALSE;
    }

    // Scan forward to the point at which the new entry should
    // be inserted.

    CurrentEntry = FindEntry( Type, Name, LowestVcn, &EntryOffset );

    if (CurrentEntry == NULL)
        return FALSE;   // fail as there is no insertion point

    // Insert a new entry at CurrentEntry.

    memmove( (PBYTE)CurrentEntry + LengthOfNewEntry,
             (PVOID)CurrentEntry,
             _LengthOfList - EntryOffset );

    memset( (PVOID)CurrentEntry, '\0', LengthOfNewEntry );

    _LengthOfList = NewLengthOfList;

    // Fill in the new entry

    CurrentEntry->AttributeTypeCode = Type;
    CurrentEntry->RecordLength = (USHORT)LengthOfNewEntry;
    CurrentEntry->AttributeNameLength = (UCHAR)NameLength;
    CurrentEntry->LowestVcn = LowestVcn;
    CurrentEntry->SegmentReference = *SegmentReference;
    CurrentEntry->Instance = InstanceTag;
    CurrentEntry->AttributeNameOffset = FIELD_OFFSET( ATTRIBUTE_LIST_ENTRY,
                                                      AttributeName );

    if( Name != NULL ) {

        Name->QueryWSTR( 0,
                         TO_END,
                         NameFromEntry( CurrentEntry ),
                         Name->QueryChCount(),
                         FALSE );
    }

    return TRUE;
}

 
BOOLEAN
NTFS_ATTRIBUTE_LIST::DeleteEntry(
    IN ULONG EntryIndex
    )
/*++

Routine Description:

    This method deletes the nth entry from the list.

Arguments:

    EntryIndex  --  supplies the index of the entry to be deleted

Return Value:

    TRUE upon successful completion.  Note that if there are
    not enough entries, this method returns TRUE.

--*/
{
    PATTRIBUTE_LIST_ENTRY CurrentEntry;
    ULONG CurrentOffset;
    ULONG BytesToRemove;
    ULONG i;


    // Scan forward to the requested entry

    CurrentOffset = 0;
    CurrentEntry = (PATTRIBUTE_LIST_ENTRY)(_Mem.GetBuf());

    if( _LengthOfList == 0 ) {

        // The list is empty.

        return TRUE;
    }


    for( i = 0; i < EntryIndex; i++ ) {

        CurrentOffset += CurrentEntry->RecordLength;

        if( CurrentOffset >= _LengthOfList ) {

            // We ran out of entries.

            return TRUE;
        }

        CurrentEntry = NextEntry( CurrentEntry );
    }



    // Delete the entry.

    BytesToRemove = CurrentEntry->RecordLength;

    DebugAssert( CurrentOffset + BytesToRemove <= _LengthOfList );

    memmove( CurrentEntry,
             (PBYTE)CurrentEntry + BytesToRemove,
             _LengthOfList - (CurrentOffset + BytesToRemove) );

    _LengthOfList -= BytesToRemove;

    return TRUE;
}

 
BOOLEAN
NTFS_ATTRIBUTE_LIST::DeleteCurrentEntry(
    IN PATTR_LIST_CURR_ENTRY    Entry
    )
/*++

Routine Description:

    This method deletes the current entry from the list.

Arguments:

    Entry  --  supplies the entry to delete

Return Value:

    TRUE upon successful completion.  Note that if there are
    not enough entries, this method returns TRUE.

--*/
{
    PATTRIBUTE_LIST_ENTRY CurrentEntry;
    ULONG CurrentOffset;
    ULONG BytesToRemove;

    if( _LengthOfList == 0 ) {

        // The list is empty.

        return TRUE;
    }

    CurrentEntry = Entry->CurrentEntry;
    CurrentOffset = Entry->CurrentOffset;

    if( CurrentOffset >= _LengthOfList ) {

        // We ran out of entries.

        return TRUE;
    }

    // Delete the entry.

    BytesToRemove = CurrentEntry->RecordLength;

    DebugAssert( CurrentOffset + BytesToRemove <= _LengthOfList );

    memmove( CurrentEntry,
             (PBYTE)CurrentEntry + BytesToRemove,
             _LengthOfList - (CurrentOffset + BytesToRemove) );

    _LengthOfList -= BytesToRemove;

    return TRUE;
}

 
BOOLEAN
NTFS_ATTRIBUTE_LIST::DeleteEntries(
    IN ATTRIBUTE_TYPE_CODE  Type,
    IN PCWSTRING            Name
    )
/*++

Routine Description:

    This method deletes all entries in the list which match the input.
    This is used when deleting a unique attribute, since all attribute
    records for that attribute type-code and name will be removed.

Arguments:

    Type                --  Supplies the attribute type code of the
                            entry to be deleted.
    Name                --  Supplies the name of the entry to be deleted;
                            may be NULL, in which case it is ignored.

Return Value:

    TRUE upon successful completion.

--*/
{
    PATTRIBUTE_LIST_ENTRY CurrentEntry;
    ULONG EntryOffset;
    ULONG BytesToRemove;
    ULONG NameLength;
    PWSTR NameBuffer = NULL;

    // This is slightly ugly but necessary.  NTFS attribute names
    // are collated straight, so we can't use the WSTRING name
    // comparison, which relies on the current locale.

    if( Name != NULL ) {

        NameLength = Name->QueryChCount();
        NameBuffer = Name->QueryWSTR();

        if( NameBuffer == NULL ) {

            return FALSE;
        }
    }


    // find the first matching entry.

    CurrentEntry = FindEntry( Type, Name, 0, &EntryOffset );

    if (CurrentEntry) {
        while( EntryOffset < _LengthOfList &&
               CurrentEntry->AttributeTypeCode == Type &&
               ( Name == NULL ||
                 ( NameLength == CurrentEntry->AttributeNameLength &&
                   memcmp( NameBuffer,
                           NameFromEntry( CurrentEntry ),
                           NameLength * sizeof(WCHAR) ) == 0 ) ) ) {

            // This entry matches, so we delete it.  Note that instead of
            // incrementing CurrentEntry and EntryOffset, we draw the
            // succeeding entries down to the current point.

            BytesToRemove = CurrentEntry->RecordLength;

            DebugAssert( EntryOffset + BytesToRemove <= _LengthOfList );

            memmove( CurrentEntry,
                     (PBYTE)CurrentEntry + BytesToRemove,
                     _LengthOfList - (EntryOffset + BytesToRemove) );

            _LengthOfList -= BytesToRemove;
        }
    }

    if( NameBuffer != NULL ) {

        FREE( NameBuffer );
    }

    return TRUE;
}

 
 
BOOLEAN
NTFS_ATTRIBUTE_LIST::IsInList(
    IN  ATTRIBUTE_TYPE_CODE Type,
    IN  PCWSTRING           Name
    ) CONST
/*++

Routine Description:

Arguments:

    Type        -- supplies the type code of the attribute in question.
    Name        -- supplies the name of the attribute in question.
                    (may be NULL, in which case the attribute has no name.)
Return Value:

    TRUE if there is an entry in the attribute list with this
    type code and (if specified) name.

--*/
{
    PATTRIBUTE_LIST_ENTRY CurrentEntry;
    ULONG EntryOffset, CurrentEntryIndex;
    ULONG NameLength;
    PCWSTR NameBuffer = NULL;

    // This is slightly ugly but necessary.  NTFS attribute names
    // are collated straight, so we can't use the WSTRING name
    // comparison, which relies on the current locale.

    if( Name != NULL ) {

        NameLength = Name->QueryChCount();
        NameBuffer = Name->GetWSTR();

    } else {

        NameLength = 0;
    }


    // Find the first entry which matches this type code & name.

    CurrentEntry = FindEntry( Type, Name, 0,
                                &EntryOffset, &CurrentEntryIndex );

    if( CurrentEntry == NULL ||
        EntryOffset >= _LengthOfList ||
        CurrentEntry->AttributeTypeCode != Type ||
        NameLength != CurrentEntry->AttributeNameLength ||
        (NameLength != 0 &&
         NtfsUpcaseCompare( NameBuffer,
                            NameLength,
                            NameFromEntry( CurrentEntry ),
                            NameLength,
                            _UpcaseTable,
                            TRUE) != 0) ) {

        // We've gone too far.  There are no matching entries.

        return FALSE;
    }

    return TRUE;
}

#if 0
 
BOOLEAN
NTFS_ATTRIBUTE_LIST::QueryEntry(
    IN  ULONG                   EntryIndex,
    OUT PATTRIBUTE_TYPE_CODE    Type,
    OUT PVCN                    LowestVcn,
    OUT PMFT_SEGMENT_REFERENCE  SegmentReference,
    OUT PUSHORT                 InstanceTag,
    OUT PWSTRING                Name
    ) CONST
/*++

Routine Description:

    This method fetches the nth entry in the list.

Arguments:

    EntryIndex          --  supplies the index into the list of the
                            entry to fetch
    Type                --  receives the entry's attribute type code
    LowestVcn           --  receives the entry's LowestVcn
    SegmentReference    --  receives the entry's SegmentReference
    InstanceTag         --  receives the entry's attribute instance tag.
    Name                --  receives the entry's Name (if any)

Return Value:

    TRUE upon successful completion.

--*/
{
    ULONG i;
    PATTRIBUTE_LIST_ENTRY CurrentEntry;
    ULONG CurrentOffset;

    CurrentOffset = 0;
    CurrentEntry = (PATTRIBUTE_LIST_ENTRY)(((PNTFS_ATTRIBUTE_LIST) this)->_Mem.GetBuf());

    if( _LengthOfList == 0 ) {

        // The list is empty.

        return FALSE;
    }


    for( i = 0; i < EntryIndex; i++ ) {

        if (CurrentEntry->RecordLength == 0)
            return FALSE;

        CurrentOffset += CurrentEntry->RecordLength;

        if( CurrentOffset >= _LengthOfList ) {

            // We ran out of entries.

            return FALSE;
        }

        CurrentEntry = NextEntry( CurrentEntry );
    }

    *Type = CurrentEntry->AttributeTypeCode;
    *LowestVcn = CurrentEntry->LowestVcn;
    *SegmentReference = CurrentEntry->SegmentReference;
    *InstanceTag = CurrentEntry->Instance;

    if( !Name->Initialize( NameFromEntry( CurrentEntry ),
                           CurrentEntry->AttributeNameLength ) ) {

        return FALSE;
    }

    return TRUE;
}
#endif


BOOLEAN
NTFS_ATTRIBUTE_LIST::QueryNextEntry(
    IN OUT PATTR_LIST_CURR_ENTRY   CurrEntry,
    OUT    PATTRIBUTE_TYPE_CODE    Type,
    OUT    PVCN                    LowestVcn,
    OUT    PMFT_SEGMENT_REFERENCE  SegmentReference,
    OUT    PUSHORT                 InstanceTag,
    OUT    PWSTRING                Name
    ) CONST
/*++

Routine Description:

    This method fetches the next entry in the list.

Arguments:

    NextEntry           --  supplies the pointer to the entry
    Type                --  receives the entry's attribute type code
    LowestVcn           --  receives the entry's LowestVcn
    SegmentReference    --  receives the entry's SegmentReference
    InstanceTag         --  receives the entry's attribute instance tag.
    Name                --  receives the entry's Name (if any)

Return Value:

    TRUE upon successful completion.

--*/
{
    PATTRIBUTE_LIST_ENTRY CurrentEntry;
    ULONG                 CurrentOffset;

    DebugPtrAssert(CurrEntry);

    if (CurrEntry->CurrentEntry == NULL) {

        CurrentEntry = (PATTRIBUTE_LIST_ENTRY)(((PNTFS_ATTRIBUTE_LIST) this)->_Mem.GetBuf());
        CurrEntry->CurrentEntry = CurrentEntry;
        CurrEntry->CurrentOffset = 0;

        *Type = CurrentEntry->AttributeTypeCode;
        *LowestVcn = CurrentEntry->LowestVcn;
        *SegmentReference = CurrentEntry->SegmentReference;
        *InstanceTag = CurrentEntry->Instance;

        if( !Name->Initialize( NameFromEntry( CurrentEntry ),
                               CurrentEntry->AttributeNameLength ) ) {

            return FALSE;
        }

        return TRUE;
    } else {

        CurrentEntry = CurrEntry->CurrentEntry;
        CurrentOffset = CurrEntry->CurrentOffset;
    }

    if( _LengthOfList == 0 ) {

        // The list is empty.

        return FALSE;
    }

    if ( CurrentEntry->RecordLength == 0 ) {

        // something is not right or it's the end

        return FALSE;
    }

    CurrentOffset += CurrentEntry->RecordLength;

    if( CurrentOffset >= _LengthOfList ) {

        // it's the end

        return FALSE;
    }

    CurrentEntry = NextEntry( CurrentEntry );

    CurrEntry->CurrentEntry = CurrentEntry;
    CurrEntry->CurrentOffset = CurrentOffset;

    *Type = CurrentEntry->AttributeTypeCode;
    *LowestVcn = CurrentEntry->LowestVcn;
    *SegmentReference = CurrentEntry->SegmentReference;
    *InstanceTag = CurrentEntry->Instance;

    if( !Name->Initialize( NameFromEntry( CurrentEntry ),
                           CurrentEntry->AttributeNameLength ) ) {

        return FALSE;
    }

    return TRUE;
}


BOOLEAN
NTFS_ATTRIBUTE_LIST::QueryEntry(
    IN  MFT_SEGMENT_REFERENCE   SegmentReference,
    IN  USHORT                  InstanceTag,
    OUT PATTRIBUTE_TYPE_CODE    Type,
    OUT PVCN                    LowestVcn,
    OUT PWSTRING                Name
    ) CONST
/*++

Routine Description:

    This routine returns the type, lowestvcn, and name of the attribute
    list entry with the given segment reference and instance tag.

Arguments:

    SegmentReference    - Supplies the entry's segment reference.
    InstanceTag         - Supplies the entry's instance tag.
    Type                - Returns the entry's type code.
    LowestVcn           - Returns the entry's lowest vcn.
    Name                - Returns the entry's name.

Return Value:

    FALSE   - An entry with the given segment reference and instance was
                not found.
    TRUE    - Success.

--*/
{
    ULONG i;
    PATTRIBUTE_LIST_ENTRY CurrentEntry;
    ULONG CurrentOffset;

    CurrentOffset = 0;
    CurrentEntry = (PATTRIBUTE_LIST_ENTRY)(_Mem.GetBuf());

    if( _LengthOfList == 0 ) {

        // The list is empty.

        return FALSE;
    }


    for( i = 0; ; i++ ) {

        if (CurrentEntry->Instance == InstanceTag &&
            CurrentEntry->SegmentReference == SegmentReference) {

            break;
        }

        CurrentOffset += CurrentEntry->RecordLength;

        if( CurrentOffset >= _LengthOfList ) {

            // We ran out of entries.

            return FALSE;
        }

        CurrentEntry = NextEntry( CurrentEntry );
    }

    *Type = CurrentEntry->AttributeTypeCode;
    *LowestVcn = CurrentEntry->LowestVcn;

    if( !Name->Initialize( NameFromEntry( CurrentEntry ),
                           CurrentEntry->AttributeNameLength ) ) {

        return FALSE;
    }

    return TRUE;
}


 
PCATTRIBUTE_LIST_ENTRY
NTFS_ATTRIBUTE_LIST::GetNextAttributeListEntry(
    IN  PCATTRIBUTE_LIST_ENTRY  CurrentEntry
    ) CONST
/*++

Routine Description:

    This routine fetches the next attribute list entry structure.

Arguments:

    CurrentEntry    - Supplies the current attribute list entry.
                        Supplying NULL as the current entry specifies that
                        you want the first entry in the list.

Return Value:

    The next attribute list entry or NULL if the current entry
    is at the end of the list.

--*/
{
    ULONG   CurrentOffset;

    if (!_LengthOfList) {
        return NULL;
    }

    if (!CurrentEntry) {
        return (PCATTRIBUTE_LIST_ENTRY) _Mem.GetBuf();
    }

    CurrentOffset = (ULONG)((PCHAR) CurrentEntry - (PCHAR) _Mem.GetBuf());

    if (CurrentOffset + CurrentEntry->RecordLength >= _LengthOfList) {
        return NULL;
    }

    return (PCATTRIBUTE_LIST_ENTRY) ((PCHAR) CurrentEntry + CurrentEntry->RecordLength);
}


 
BOOLEAN
NTFS_ATTRIBUTE_LIST::QueryExternalReference(
    IN  ATTRIBUTE_TYPE_CODE     Type,
    OUT PMFT_SEGMENT_REFERENCE  SegmentReference,
    OUT PULONG                  EntryIndex,
    IN  PCWSTRING               Name,
    IN  PVCN                    DesiredVcn,
    OUT PVCN                    StartingVcn
    ) CONST
/*++

Routine Description:

    This method fetches an entry from the list based on a type code,
    name (optional), and VCN.

Arguments:

    Type                --  supplies the attribute type code to search for.
    SegmentReference    --  receives the entry's SegmentReference
    EntryIndex          --  receives the entry's index into the list
    Name                --  supplies the entry's name.  If this pointer
                            is NULL, attribute names are ignored.
    DesiredVcn          --  supplies a pointer to the VCN we're interested
                            in.  (Note that this pointer may be NULL if
                            the caller just wants the first entry for this
                            type & name.)
    StartingVcn         --  receives the LowestVcn of the entry found;
                            if this pointer is NULL, that information is
                            not returned.


Return Value:

    TRUE if a matching entry is found.

Notes:

    A client who wishes to find all the entries for a particular
    attribute can take advantage of the fact that the list is sorted
    by type code and name.  Thus, the client finds the first matching
    entry (using QueryExternalReference), and then queries successive
    entries by index until one doesn't match.

--*/
{
    PATTRIBUTE_LIST_ENTRY CurrentEntry, PreviousEntry=NULL;
    ULONG EntryOffset, CurrentEntryIndex;
    ULONG NameLength;
    PWSTR NameBuffer = NULL;

    // This is slightly ugly but necessary.  NTFS attribute names
    // are collated straight, so we can't use the WSTRING name
    // comparison, which relies on the current locale.

    if( Name != NULL ) {

        NameLength = Name->QueryChCount();
        NameBuffer = Name->QueryWSTR();

        if( NameBuffer == NULL ) {

            return FALSE;
        }
    }


    // The search algorithm for this method is slightly different than
    // the other methods for this class.  Instead of the first matching
    // entry, we want the last matching entry which has a LowestVcn field
    // less than or equal to *DesiredVcn.  (If DesiredVcn is NULL, we can
    // just return the first entry we find.)

    CurrentEntry = FindEntry( Type, Name, 0,
                                &EntryOffset, &CurrentEntryIndex );

    if( CurrentEntry == NULL ||
        EntryOffset >= _LengthOfList ||
        CurrentEntry->AttributeTypeCode != Type ||
        ( Name != NULL &&
          ( NameLength != CurrentEntry->AttributeNameLength ||
            memcmp( NameBuffer,
                    NameFromEntry( CurrentEntry ),
                    NameLength * sizeof(WCHAR) ) != 0 ) ) ||
        ( DesiredVcn != NULL &&
          CurrentEntry->LowestVcn > *DesiredVcn ) ) {

        // We've gone too far.  There are no matching entries.

        if( NameBuffer != NULL ) {

            FREE( NameBuffer );
        }

        return FALSE;
    }

    if( DesiredVcn != NULL ) {

        // The caller specified a particular VCN, so we have to find the
        // entry that contains it.  We do this by scanning forward until
        // we find an entry that is beyond what we want, and then backing
        // up one.  Since we passed the test above, we know that the
        // loop below will execute at least once, so PreviousEntry is
        // sure to get set.

        while( EntryOffset < _LengthOfList &&
               CurrentEntry->AttributeTypeCode == Type &&
               ( Name == NULL ||
                 ( NameLength == CurrentEntry->AttributeNameLength &&
                   memcmp( NameBuffer,
                           NameFromEntry( CurrentEntry ),
                           NameLength * sizeof(WCHAR) ) == 0 ) ) &&
               CurrentEntry->LowestVcn <= *DesiredVcn ) {

            PreviousEntry = CurrentEntry;
            CurrentEntryIndex += 1;
            EntryOffset += CurrentEntry->RecordLength;
            CurrentEntry = NextEntry( CurrentEntry );
        }

        // Now back up one, to the entry we really want:

        CurrentEntry = PreviousEntry;
        CurrentEntryIndex -= 1;
    }

    // Fill in the output parameters.

    memcpy( SegmentReference,
            &CurrentEntry->SegmentReference,
            sizeof( MFT_SEGMENT_REFERENCE ) );

    *EntryIndex = CurrentEntryIndex;

    if( StartingVcn != NULL ) {

        *StartingVcn = CurrentEntry->LowestVcn;
    }

    if( NameBuffer != NULL ) {

        FREE( NameBuffer );
    }

    return TRUE;
}

 
BOOLEAN
NTFS_ATTRIBUTE_LIST::QueryNextAttribute(
    IN OUT PATTRIBUTE_TYPE_CODE TypeCode,
    IN OUT PWSTRING             Name
    ) CONST
/*++

Routine Description:

    This method determines the type code and name of the first
    attribute list which is strictly greater than the supplied
    type and name.

Arguments:

    TypeCode    --  supplies the current attribute type code.  Receives
                    the type code of the next attribute.  A returned type
                    code of $END indicates that there are no more attributes.
    Name        --  supplies the current name.  Receives the name of the
                    next attribute.


Return Value:

    TRUE upon successful completion.

Notes:

    This method is useful for iterating through the non-indexed
    attributes of a file, since there can only be one non-indexed
    attribute with a given type code and name in the file.  However,
    it offers no way of dealing with indexed attributes, which may
    be distinguished only by value.

--*/
{
    PATTRIBUTE_LIST_ENTRY CurrentEntry;
    ULONG CurrentEntryOffset, CurrentEntryIndex;

    // Use FindEntry to get to the entry we want.  Note that we use
    // a LowestVcn of -1, to skip over all matching entries.

    if( (CurrentEntry = FindEntry( *TypeCode,
                                   Name,
                                   -1,
                                   &CurrentEntryOffset,
                                   &CurrentEntryIndex )) == NULL ) {

        // An error occurred searching the list.
        return FALSE;
    }

    if( CurrentEntryOffset >= _LengthOfList ) {

        // This is the end of the list; there are no more entries.

        *TypeCode = $END;

        return Name->Initialize("");
    }

    // OK, we have the entry we want.  Copy its type and name (if any).

    *TypeCode = CurrentEntry->AttributeTypeCode;

    if( !Name->Initialize( NameFromEntry(CurrentEntry),
                           CurrentEntry->AttributeNameLength ) ) {

        return FALSE;
    }

    return TRUE;
}


 
BOOLEAN
NTFS_ATTRIBUTE_LIST::ReadList(
    )
/*++

Routine Description:

    This method reads the list into the object's private buffer.

Arguments:

    None.

Return Value:

    TRUE upon successful completion.

--*/
{
    BIG_INT ValueLength;
    ULONG BytesRead;

    // Determine the length of the list.

    QueryValueLength( &ValueLength );

    DebugAssert( ValueLength.GetHighPart() == 0 );

    _LengthOfList = ValueLength.GetLowPart();

    // Initialize our MEM object and use it to get the correct
    // amount of memory.

    if( !_Mem.Initialize() ||
        !_Mem.Acquire( (LONG)_LengthOfList ) ) {

        return FALSE;
    }

    // Read the attribute's value into our buffer.

    return( Read( _Mem.GetBuf(), 0, _LengthOfList, &BytesRead) &&
            BytesRead == _LengthOfList );
}


 
PATTRIBUTE_LIST_ENTRY
NTFS_ATTRIBUTE_LIST::FindEntry(
    IN  ATTRIBUTE_TYPE_CODE Type,
    IN  PCWSTRING           Name,
    IN  VCN                 LowestVcn,
    OUT PULONG              EntryOffset,
    OUT PULONG              EntryIndex
    ) CONST
/*++

Routine Description:

    This method finds the first entry in the list which matches
    its input or, if there is no match, the first entry that
    would come after it (i.e. the place it would be if it were there).

Arguments:

    Type        -- supplies the attribute type code to find
    Name        -- supplies the name to find (may be NULL, in which case it
                    is ignored)
    LowestVcn   -- supplies the VCN to find.  A value of -1 indicates
                    we should skip all entries for this type and name.
    EntryOffset -- receives the offset into the list of the
                    returned pointer.  (May be NULL, in which case
                    this value is not returned.)
    EntryIndex  -- receives the index into the list of the returned
                    entry.  (May be NULL, in which case this value
                    is not returned.


Return Value:

    A pointer to the first entry in the list which matches the input.
    If there is no match, this method returns the next entry (i.e. the
    point at which a matching entry should be inserted).

    NULL is returned to indicate end of entry.

--*/
{
    PATTRIBUTE_LIST_ENTRY CurrentEntry;
    ULONG CurrentOffset, CurrentIndex;
    ULONG NameLength;
    PWSTR NameBuffer = NULL;

    // This is slightly ugly but necessary.  NTFS attribute names
    // are collated straight, so we can't use the WSTRING name
    // comparison, which relies on the current locale.

    if( Name != NULL ) {

        NameLength = Name->QueryChCount();
        NameBuffer = Name->QueryWSTR();

        if( NameBuffer == NULL ) {

            return NULL;
        }
    }

    // Start at the beginning of the list.

    CurrentEntry = (PATTRIBUTE_LIST_ENTRY)(((PNTFS_ATTRIBUTE_LIST) this)->_Mem.GetBuf());
    CurrentOffset = 0;
    CurrentIndex = 0;

    // Scan forward to the first entry which has a type code
    // greater than or equal to the type code we want.

    while( CurrentOffset < _LengthOfList &&
           Type > CurrentEntry->AttributeTypeCode ) {

        CurrentIndex += 1;
        CurrentOffset += CurrentEntry->RecordLength;
        CurrentEntry = NextEntry( CurrentEntry );
    }


    // CurrentEntry now points at the first entry with an attribute
    // type code greater than or equal to the one we're seeking.
    // Within the group of entries with the same type code, the
    // entries are sorted first by name and then by LowestVcn.

    if( Name != NULL ) {

        // The caller specified a name name, so we need to scan
        // through the entries with this attribute type code for
        // the first entry with a name greater than or equal to
        // that name.

        while( CurrentOffset < _LengthOfList &&
               Type == CurrentEntry->AttributeTypeCode &&
               NtfsUpcaseCompare( NameBuffer,
                                  NameLength,
                                  NameFromEntry( CurrentEntry ),
                                  CurrentEntry->AttributeNameLength,
                                  _UpcaseTable,
                                  TRUE ) > 0 ) {

            CurrentIndex += 1;
            CurrentOffset += CurrentEntry->RecordLength;
            CurrentEntry = NextEntry( CurrentEntry );
        }

        // Now scan forward by LowestVcn through the attributes with
        // this type code and name.  Note that a search value of -1
        // for LowestVcn indicates we should skip all matching entries.

        while( CurrentOffset < _LengthOfList &&
               Type == CurrentEntry->AttributeTypeCode &&
               ( NameLength == CurrentEntry->AttributeNameLength &&
                 memcmp( NameBuffer,
                         NameFromEntry( CurrentEntry ),
                         NameLength * sizeof(WCHAR) ) == 0 ) &&
               ( (LowestVcn == -1) ||
                 (LowestVcn > CurrentEntry->LowestVcn) ) ) {

            CurrentIndex += 1;
            CurrentOffset += CurrentEntry->RecordLength;
            CurrentEntry = NextEntry( CurrentEntry );
        }

    } else {

        // The caller did not specify a name, so we only examine
        // entries without names.  These come before entries with
        // that same attribute type code that have names.  Scan
        // forward by LowestVcn through the entries that have this
        // attribute type code and no name.  Note that a search value
        // of -1 for LowestVcn indicates that we should skip all matching
        // entries.

        while( CurrentOffset < _LengthOfList &&
               Type == CurrentEntry->AttributeTypeCode &&
               CurrentEntry->AttributeNameLength == 0 &&
               ( (LowestVcn == -1) ||
                 (LowestVcn > CurrentEntry->LowestVcn) ) ) {

            CurrentIndex += 1;
            CurrentOffset += CurrentEntry->RecordLength;
            CurrentEntry = NextEntry( CurrentEntry );
        }
    }

    if( EntryOffset != NULL ) {

        *EntryOffset = CurrentOffset;
    }

    if( EntryIndex != NULL ) {

        *EntryIndex = CurrentIndex;
    }

    if( NameBuffer != NULL ) {

        FREE( NameBuffer );
    }

    return CurrentEntry;
}
