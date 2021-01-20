#include "stdafx.h"

/*++

Module Name:

    indxtree.cxx

Abstract:

    This module contains the member function definitions for the
    NTFS_INDEX_TREE class, which models index trees on an NTFS
    volume.

    An NTFS Index Tree consists of an index root and a set of
    index buffers.  The index root is stored as the value of
    an INDEX_ROOT attribute; the index buffers are part of the
    value of an INDEX_ALLOCATION attribute.

--*/


#include "ulib.hxx"

#include "untfs.hxx"

#include "drive.hxx"

#include "attrib.hxx"
#include "frs.hxx"
#include "indxtree.hxx"
#include "indxbuff.hxx"
#include "indxroot.hxx"
#include "ntfsbit.hxx"
#include "upcase.hxx"
#include "message.hxx"


CONST USHORT IndexEntryAttributeLength[] = { 4, 8, 12, 16 };

LONG
CompareNtfsFileNames(
    IN PCFILE_NAME          Name1,
    IN PCFILE_NAME          Name2,
    IN PNTFS_UPCASE_TABLE   UpcaseTable
    )
/*++

Routine Description:

    This method compares two FILE_NAME structures according to the
    COLLATION_FILE_NAME collation rule.

Arguments:

    Name1       --  Supplies the first name to compare.
    Name2       --  Supplies the second name to compare.
    UpcaseTable --  Supplies the volume upcase table.

Returns:

    <0 if Name1 is less than Name2
    =0 if Name1 is equal to Name2
    >0 if Name1 is greater than Name2.

--*/
{
    LONG Result;

    Result = NtfsUpcaseCompare( NtfsFileNameGetName( Name1 ),
                                Name1->FileNameLength,
                                NtfsFileNameGetName( Name2 ),
                                Name2->FileNameLength,
                                UpcaseTable,
                                TRUE );

    return Result;
}

LONG
NtfsCollate(
    IN PCVOID               Value1,
    IN ULONG                Length1,
    IN PCVOID               Value2,
    IN ULONG                Length2,
    IN COLLATION_RULE       CollationRule,
    IN PNTFS_UPCASE_TABLE   UpcaseTable
    )
/*++

Routine Description:

    This function compares two values according to an NTFS
    collation rule.

Arguments:

    Value1          --  Supplies the first value.
    Length1         --  Supplies the length of the first value.
    Value2          --  Supplies the second value.
    Length2         --  Supplies the length of the second value.
    CollationRule   --  Supplies the rule used for collation.
    UpcaseTable     --  Supplies the volume upcase table.  (May be NULL
                        if the collatio rule is not COLLATION_FILE_NAME).

Return Value:

    <0 if Entry1 is less than Entry2 by CollationRule
     0 if Entry1 is equal to Entry2 by CollationRule
    >0 if Entry1 is greater than Entry2 by CollationRule

Notes:

    The upcase table is only required for comparing file names.

    If two values are compared according to an unsupported collation
    rule, they are always treated as equal.

--*/
{
    LONG result;

    switch( CollationRule ) {

    case COLLATION_BINARY :

        // Binary collation of the values.
        //
        result = memcmp( Value1,
                         Value2,
                         MIN( Length1, Length2 ) );

        if( result != 0 ) {

            return result;

        } else {

            return( Length1 - Length2 );
        }

    case COLLATION_FILE_NAME :

        return CompareNtfsFileNames( (PFILE_NAME)Value1,
                                     (PFILE_NAME)Value2,
                                     UpcaseTable );


    case COLLATION_UNICODE_STRING :

        // unsupported collation rule.
        //
        return 0;

    case COLLATION_ULONG:

        // Unsigned long collation

        DebugAssert(Length1 == sizeof(ULONG));
        DebugAssert(Length1 == sizeof(ULONG));

        if (*(ULONG*)Value1 < *(ULONG *)Value2)
            return -1;
        else if (*(ULONG*)Value1 > *(ULONG *)Value2)
            return 1;
        else
            return 0;

    case COLLATION_SID:

        // SecurityId collation

        result = memcmp(&Length1, &Length2, sizeof(Length1));
        if (result != 0)
            return result;

        result = memcmp( Value1, Value2, Length1 );
        return result;

    case COLLATION_SECURITY_HASH: {

        // Security Hash (Hash key and SecurityId) Collation

        PSECURITY_HASH_KEY HashKey1 = (PSECURITY_HASH_KEY)Value1;
        PSECURITY_HASH_KEY HashKey2 = (PSECURITY_HASH_KEY)Value2;

        DebugAssert(Length1 == sizeof(SECURITY_HASH_KEY));
        DebugAssert(Length2 == sizeof(SECURITY_HASH_KEY));

        if (HashKey1->Hash < HashKey2->Hash)
            return -1;
        else if (HashKey1->Hash > HashKey2->Hash)
            return 1;
        else if (HashKey1->SecurityId < HashKey2->SecurityId)
            return -1;
        else if (HashKey1->SecurityId > HashKey2->SecurityId)
            return 1;
        else
            return 0;
    }

    case COLLATION_ULONGS: {
        PULONG pu1, pu2;
        ULONG count;

        result = 0;

        DebugAssert( (Length1 & 3) == 0 );
        DebugAssert( (Length2 & 3) == 0 );

        count = Length1;
        if (count != Length2) {
           result = -1;
           if (count > Length2) {
               count = Length2;
               result = 1;
           }
        }

        pu1 = (PULONG)Value1;
        pu2 = (PULONG)Value2;

        while (count > 0) {
           if (*pu1 > *pu2) {
               return 1;
           } else if (*(pu1++) < *(pu2++)) {
               return -1;
           }
           count -= 4;
        }
        return result;
    }

    default:

        DebugAbort( "Unsupported collation rule.\n" );
        return 0;
    }
}




LONG
CompareNtfsIndexEntries(
    IN PCINDEX_ENTRY    Entry1,
    IN PCINDEX_ENTRY    Entry2,
    IN COLLATION_RULE   CollationRule,
    IN PNTFS_UPCASE_TABLE UpcaseTable
    )
/*++

Routine Description:

    This global function is used to compare index entries.

Arguments:

    Entry1          --  Supplies the first entry to compare.
    Entry2          --  Supplies the second entry to compare.
    CollationRule   --  Supplies the rule used for collation.
    UpcaseTable     --  Supplies the volume upcase table.

Return Value:

    <0 if Entry1 is less than Entry2 by CollationRule
     0 if Entry1 is equal to Entry2 by CollationRule
    >0 if Entry1 is greater than Entry2 by CollationRule

Notes:

    The upcase table is only required for comparing file names.

--*/
{
    return NtfsCollate( GetIndexEntryValue( Entry1 ),
                        Entry1->AttributeLength,
                        GetIndexEntryValue( Entry2 ),
                        Entry2->AttributeLength,
                        CollationRule,
                        UpcaseTable );
}



DEFINE_CONSTRUCTOR( NTFS_INDEX_TREE, OBJECT   );

 
NTFS_INDEX_TREE::~NTFS_INDEX_TREE(
    )
{
    Destroy();
}

VOID
NTFS_INDEX_TREE::Construct(
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
    _Drive = NULL;
    _ClusterFactor = 0;
    _ClustersPerBuffer = 0;
    _BufferSize = 0;
    _VolumeBitmap = NULL;
    _UpcaseTable = NULL;
    _AllocationAttribute = NULL;
    _IndexAllocationBitmap = NULL;
    _IndexRoot = NULL;
    _Name = NULL;

    _IteratorState = INDEX_ITERATOR_RESET;
    _CurrentEntry = NULL;
    _CurrentBuffer = NULL;
    _CurrentKey = NULL;
    _CurrentKeyLength = 0;
}

VOID
NTFS_INDEX_TREE::Destroy(
    )
/*++

Routine Description:

    This method cleans up an NTFS_INDEX_TREE object in preparation
    for destruction or reinitialization.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _Drive = NULL;
    _ClustersPerBuffer = 0;
    _BufferSize = 0;
    _VolumeBitmap = NULL;
    _UpcaseTable = NULL;

    DELETE( _AllocationAttribute );
    DELETE( _IndexAllocationBitmap );
    DELETE( _IndexRoot );
    DELETE( _Name );

    _IteratorState = INDEX_ITERATOR_RESET;

    _CurrentEntry = NULL;
    DELETE( _CurrentBuffer );
    FREE( _CurrentKey );

    _CurrentKeyLength = 0;
}

 
BOOLEAN
NTFS_INDEX_TREE::Initialize(
    IN OUT PLOG_IO_DP_DRIVE             Drive,
    IN     ULONG                        ClusterFactor,
    IN OUT PNTFS_BITMAP                 VolumeBitmap,
    IN     PNTFS_UPCASE_TABLE           UpcaseTable,
    IN     ULONG                        MaximumRootSize,
    IN     PNTFS_FILE_RECORD_SEGMENT    SourceFrs,
    IN     PCWSTRING                    IndexName
    )
/*++

Routine Description:

    This method initializes an NTFS_INDEX_TREE based on
    attributes queried from a File Record Segment.

Arguments:

    Drive               --  supplies the drive on which the
                                index resides.
    ClusterFactor       --  supplies the cluster factor for the drive.
    VolumeBitmap        --  supplies the volume bitmap.
    MaximumRootSize     --  supplies the maximum length of the index root
    SourceFrs           --  supplies the File Record Segment that contains
                            this index.
    UpcaseTable         --  supplies the volume upcase table.
    IndexName           --  supplies the name for this index.  (May be NULL,
                            in which case the index has no name.)

Return Value:

    TRUE upon successful completion.

Notes:

    SourceFrs must have an $INDEX_ROOT attribute, or this method will
    fail.

    The index tree does not remember what File Record Segment it came
    from; it only uses the FRS as a place to get the index root and
    index allocation attributes.

    The volume upcase table is only required if the indexed attribute
    type code is $FILE_NAME.

--*/
{
    NTFS_ATTRIBUTE RootAttribute;
    NTFS_ATTRIBUTE BitmapAttribute;

    BIG_INT ValueLength;
    ULONG NumberOfBuffers;
    BOOLEAN Error;

    Destroy();

    DebugAssert(0 != ClusterFactor);

    if( !SourceFrs->QueryAttribute( &RootAttribute,
                                    &Error,
                                    $INDEX_ROOT,
                                    IndexName ) ||
        (_IndexRoot = NEW NTFS_INDEX_ROOT) == NULL ||
        !_IndexRoot->Initialize( &RootAttribute,
                                 UpcaseTable,
                                 MaximumRootSize ) ) {

        Destroy();
        return FALSE;
    }

    _Drive = Drive;
    _ClusterFactor = ClusterFactor;
    _ClustersPerBuffer = _IndexRoot->QueryClustersPerBuffer();
    _BufferSize = _IndexRoot->QueryBufferSize();
    _VolumeBitmap = VolumeBitmap;
    _UpcaseTable = UpcaseTable;

    DebugAssert(0 != _BufferSize);

    if( RootAttribute.GetName() != NULL &&
        ( (_Name = NEW DSTRING) == NULL ||
          !_Name->Initialize( RootAttribute.GetName() ) ) ) {

        Destroy();
        return FALSE;
    }

    _IndexedAttributeType = _IndexRoot->QueryIndexedAttributeType();
    _CollationRule = _IndexRoot->QueryCollationRule();

    if( SourceFrs->IsAttributePresent( $INDEX_ALLOCATION, IndexName ) ) {

        if( (_AllocationAttribute = NEW NTFS_ATTRIBUTE) == NULL ||
            !SourceFrs->QueryAttribute( _AllocationAttribute,
                                        &Error,
                                        $INDEX_ALLOCATION,
                                        IndexName ) ) {

            Destroy();
            return FALSE;
        }

        // Set (ie. initialize and read) the bitmap associated with
        // the index allocation attribute.  Note that the bitmap
        // attribute's value may be larger than necessary to cover
        // the allocation attribute because the bitmap attribute's
        // value always grows in increments of eight bytes.  However,
        // at this point, we don't care, since we only worry about
        // that when we grow the bitmap.

        _AllocationAttribute->QueryValueLength( &ValueLength );

        DebugAssert( ValueLength % _BufferSize == 0 );

        NumberOfBuffers = ValueLength.GetLowPart()/_BufferSize;


        if( (_IndexAllocationBitmap = NEW NTFS_BITMAP) == NULL ||
            !_IndexAllocationBitmap->Initialize( NumberOfBuffers, TRUE ) ||
            !SourceFrs->QueryAttribute( &BitmapAttribute,
                                        &Error,
                                        $BITMAP,
                                        IndexName ) ||
            !_IndexAllocationBitmap->Read( &BitmapAttribute ) ) {

            Destroy();
            return FALSE;
        }
    }

    // Set up the buffer to support iteration.  This buffer must be
    // big enough to hold the largest key value.  The size of an
    // index allocation buffer will suffice.

    _IteratorState = INDEX_ITERATOR_RESET;
    _CurrentKeyMaxLength = _BufferSize;

    if( (_CurrentKey = MALLOC( _CurrentKeyMaxLength )) == NULL ) {

        Destroy();
        return FALSE;
    }

    _CurrentKeyLength = 0;

    if( !_CurrentEntryTrail.Initialize() ) {

        Destroy();
        return FALSE;
    }

    return TRUE;
}

 
 
BOOLEAN
NTFS_INDEX_TREE::Initialize(
    IN      ATTRIBUTE_TYPE_CODE IndexedAttributeType,
    IN OUT  PLOG_IO_DP_DRIVE    Drive,
    IN      ULONG               ClusterFactor,
    IN OUT  PNTFS_BITMAP        VolumeBitmap,
    IN      PNTFS_UPCASE_TABLE  UpcaseTable,
    IN      COLLATION_RULE      CollationRule,
    IN      ULONG               BufferSize,
    IN      ULONG               MaximumRootSize,
    IN      PCWSTRING           IndexName
    )
/*++

Routine Description:

    This method initializes an NTFS_INDEX_TREE based on its basic
    information.  It is used when creating an index.

Arguments:

    Drive                   --  supplies the drive on which the
                                index resides.
    VolumeBitmap            --  supplies the volume bitmap
    UpcaseTable             --  supplies the volume upcase table.
    IndexedAttributeType    --  supplies the attribute type code of the
                                attribute which is used as the key for
                                this index.
    CollationRule           --  supplies the collation rule for this index.
    BufferSize              --  supplies the size of each Index Buffer in this index.
    MaximumRootSize         --  supplies the maximum length of the index root
    IndexName               --  supplies the name of this index.  (May be
                                NULL, in which case the index has no name.)

Return Value:

    TRUE upon successful completion.

    The volume upcase table is only required if the indexed attribute
    type code is $FILE_NAME.

--*/
{
    ULONG   ClusterSize;

    Destroy();

    DebugAssert(0 != ClusterFactor);
    DebugPtrAssert(Drive);

    _Drive = Drive;
    _BufferSize = BufferSize;
    _VolumeBitmap = VolumeBitmap;
    _UpcaseTable = UpcaseTable;
    _ClusterFactor = ClusterFactor;

    ClusterSize = Drive->QuerySectorSize()*ClusterFactor;

    DebugAssert(ClusterSize <= 64 * 1024);

    _ClustersPerBuffer = BufferSize / ((BufferSize < ClusterSize) ?
                                       NTFS_INDEX_BLOCK_SIZE : ClusterSize);

    if( IndexName != NULL &&
        ( (_Name = NEW DSTRING) == NULL ||
          !_Name->Initialize( IndexName ) ) ) {

        Destroy();
        return FALSE;
    }

    _IndexedAttributeType = IndexedAttributeType;
    _CollationRule = CollationRule;

    _AllocationAttribute = NULL;
    _IndexAllocationBitmap = NULL;

    if( (_IndexRoot = NEW NTFS_INDEX_ROOT) == NULL ||
        !_IndexRoot->Initialize( IndexedAttributeType,
                                 CollationRule,
                                 UpcaseTable,
                                 _ClustersPerBuffer,
                                 BufferSize,
                                 MaximumRootSize ) ) {

        Destroy();
        return FALSE;
    }


    // Set up the buffer to support iteration.  This buffer must be
    // big enough to hold the largest key value.  The size of an
    // index allocation buffer will suffice.

    _IteratorState = INDEX_ITERATOR_RESET;
    _CurrentKeyMaxLength = BufferSize;

    if( (_CurrentKey = MALLOC( _CurrentKeyMaxLength )) == NULL ) {

        Destroy();
        return FALSE;
    }

    _CurrentKeyLength = 0;

    if( !_CurrentEntryTrail.Initialize() ) {

        Destroy();
        return FALSE;
    }

    return TRUE;
}
 
BOOLEAN
NTFS_INDEX_TREE::InsertEntry(
    IN  PCINDEX_ENTRY   NewEntry,
    IN  BOOLEAN         NoDuplicates,
    IN  PBOOLEAN        Duplicate
    )
/*++

Routine Description:

    This method adds an entry to the index.

Arguments:

    NewEntry            -- supplies the new entry to add to the index.
    NoDuplicates        --  Supplies a flag which, if TRUE, indicates
                            that InsertEntry should fail if a matching
                            entry is already present in the index.

Return Value:

    TRUE upon successful completion.

--*/
{
    INTSTACK ParentTrail;

    PNTFS_INDEX_BUFFER ContainingBuffer;
    PINDEX_ENTRY FoundEntry;
    ULONG Ordinal;
    BOOLEAN Found;
    BOOLEAN Result;
    BOOLEAN dup;

    if (Duplicate == NULL)
        Duplicate = &dup;

    // First, find the spot in the tree where we want to insert the
    // new entry.
    //
    // If the client does not allow duplicates, search for the first
    // matching entry--if we find a match, refuse the insert; if we
    // don't, FindEntry will find the insertion point for us.
    //
    // If the client does allow duplicates, call FindEntry with
    // a value INDEX_SKIP, which indicates all matching entries
    // should be skipped.  Thus, the new entry will be inserted
    // after all matching entries.
    //
    Ordinal = NoDuplicates ? 0 : (INDEX_SKIP);

    Found = FindEntry( NewEntry->AttributeLength,
                       GetIndexEntryValue( NewEntry ),
                       Ordinal,
                       &FoundEntry,
                       &ContainingBuffer,
                       &ParentTrail );

    *Duplicate = Found;

    if( Found && NoDuplicates ) {

        // A matching entry already exists, and the client wants
        // to fail in that case.  So fail.
        //

        if ( ContainingBuffer )
            DELETE( ContainingBuffer );

        return FALSE;
    }

    DebugAssert( !Found );

    // Since no matching entry was found, FindEntry will
    // return a leaf entry as its insertion point.  This
    // makes this  code a lot easier, since we only need
    // to handle inserting a new leaf.
    //
    if( FoundEntry == NULL ) {

        // An error occurred trying to insert the entry.

        return FALSE;
    }

    if( ContainingBuffer == NULL ) {

        // The root is also a leaf (see comment above), so we'll
        // insert the new entry into it.

        return( InsertIntoRoot( NewEntry, FoundEntry ) );

    } else {

        // We've found a leaf buffer, so we'll insert the new
        // entry into it.

        Result = InsertIntoBuffer( ContainingBuffer,
                                   &ParentTrail,
                                   NewEntry,
                                   FoundEntry );

        DELETE( ContainingBuffer );
        return Result;
    }
}


 
BOOLEAN
NTFS_INDEX_TREE::Save(
    IN OUT PNTFS_FILE_RECORD_SEGMENT TargetFrs
    )
/*++

Routine Description:

    This method saves the index.  The root is saved as an INDEX_ROOT
    attribute in the target File Record Segment; the index allocation
    (if any) is saved as an INDEX_ALLOCATION attribute.

Arguments:

    TargetFrs   --  supplies the File Record Segment in which to save
                    the index.

Return Value:

    TRUE upon successful completion.

--*/
{
    NTFS_ATTRIBUTE RootAttribute;
    NTFS_ATTRIBUTE BitmapAttribute;

    BOOLEAN Error;

    DebugAssert( ( _IndexAllocationBitmap == NULL &&
                 _AllocationAttribute == NULL ) ||
               ( _IndexAllocationBitmap != NULL &&
                 _AllocationAttribute != NULL ) );



    // Fetch or create attributes for the Index Root and (if necessary)
    // the allocation bitmap.  If either is to be newly created, make
    // it resident with zero length (since writing it it resize it
    // appropriately).

    if( !TargetFrs->QueryAttribute( &RootAttribute,
                                    &Error,
                                    $INDEX_ROOT,
                                    _Name ) &&
        ( Error ||
          !RootAttribute.Initialize( _Drive,
                                       _ClusterFactor,
                                       NULL,
                                       0,
                                       $INDEX_ROOT,
                                       _Name ) ) ) {

        return FALSE;
    }

    if( _IndexAllocationBitmap != NULL &&
        !TargetFrs->QueryAttribute( &BitmapAttribute,
                                    &Error,
                                    $BITMAP,
                                    _Name ) &&
        ( Error ||
          !BitmapAttribute.Initialize( _Drive,
                                       _ClusterFactor,
                                       NULL,
                                       0,
                                       $BITMAP,
                                       _Name ))) {

        return FALSE;
    }

    // If this tree does not have an allocation attribute, purge
    // any existing stale allocation & bitmap attributes.
    //
    if( _AllocationAttribute == NULL &&
        (!TargetFrs->PurgeAttribute( $INDEX_ALLOCATION, _Name ) ||
         !TargetFrs->PurgeAttribute( $BITMAP, _Name )) ) {

        return FALSE;
    }


    // Now save the attributes that describe this tree.
    //
    if( !_IndexRoot->Write( &RootAttribute ) ||
        !RootAttribute.InsertIntoFile( TargetFrs, _VolumeBitmap ) ) {

        return FALSE;
    }


    if( _AllocationAttribute == NULL ) {
        return TRUE;
    }

    if( !_IndexAllocationBitmap->Write( &BitmapAttribute, _VolumeBitmap )) {
        DebugPrint("UNTFS: Could not write index allocation bitmap\n");
        return FALSE;
    }

    if( !BitmapAttribute.InsertIntoFile( TargetFrs, _VolumeBitmap )) {

        DebugPrint("UNTFS: Could not insert bitmap attribute\n");

        //  Try a second time after making sure the attribute is non-resident.
        //

        if( !BitmapAttribute.MakeNonresident( _VolumeBitmap ) ||
            !BitmapAttribute.InsertIntoFile( TargetFrs, _VolumeBitmap )) {

            DebugPrint("UNTFS: Still could not insert bitmap attr.\n");
            return FALSE;
        }
    }

    if( !_AllocationAttribute->InsertIntoFile( TargetFrs, _VolumeBitmap )) {

        DebugPrintTrace(("UNTFS: Could not insert allocation attribute\n"));
        return FALSE;
    }

    return TRUE;
}

 
 
VOID
NTFS_INDEX_TREE::FreeAllocation(
    )
/*++

Routine Description:

    This method frees the disk space associated with this index's
    Allocation Attribute.

Arguments:

    None.

Return Value:

    None.

Notes:

    This method may leave the tree in a corrupt state, since it
    truncates the allocation attribute to zero without cleaning
    up downpointers in the root.  Use with care.

--*/
{
    if( _AllocationAttribute != NULL ) {

        _AllocationAttribute->Resize( 0, _VolumeBitmap );
    }
}

 
BOOLEAN
NTFS_INDEX_TREE::UpdateFileName(
    IN PCFILE_NAME      Name,
    IN FILE_REFERENCE   FileReference
    )
/*++

Routine Description:

    This method updates the duplicated information in a file name
    index entry.

Arguments:

    Name            --  Supplies the file name structure with the new
                        duplicated information.
    FileReference   --  Supplies the file reference for the file to
                        which this name belongs.  (Note that this is
                        the base FRS for that file, not necessarily the
                        exact FRS that contains the name.)

Return Value:

    TRUE upon successful completion.

Notes:

    This operation is meaningless on an index that is not constructed
    over the $FILE_NAME attribute.

--*/
{
    INTSTACK ParentTrail;
    PINDEX_ENTRY FoundEntry;
    PNTFS_INDEX_BUFFER ContainingBuffer = NULL;
    PFILE_NAME TargetName;
    BOOLEAN Result;

    DebugPtrAssert( Name );

    if( QueryIndexedAttributeType() != $FILE_NAME ||
        QueryCollationRule() != COLLATION_FILE_NAME ) {

        DebugAbort( "Updating file name in an index that isn't over $FILE_NAME.\n" );
        return FALSE;
    }

    // OK, find the entry that corresponds to the input.  Note that the
    // collation rule for File Names ignores everything but the actual
    // file name portion of the key value.

    if( !FindEntry( NtfsFileNameGetLength( Name ),
                    (PVOID)Name,
                    0,
                    &FoundEntry,
                    &ContainingBuffer,
                    &ParentTrail ) ) {

        // If FoundEntry is NULL, FindEntry failed because of an error;
        // otherwise, there is no matching entry in the index, which
        // means there's nothing to update.
        //

        DebugPrint( "UpdateFileName--index entry not found.\n" );
        Result = ( FoundEntry != NULL );

    } else {

        // We've found an entry.  As an extra sanity check, make sure
        // that the file reference for the found entry is the same as
        // the input file reference.

        if( memcmp( &(FoundEntry->FileReference),
                    &(FileReference),
                    sizeof( FILE_REFERENCE ) ) != 0 ) {

            DebugPrint( "File references don't match in UpdateFileName.\n" );
            Result = TRUE;

        } else {

            // Copy the duplicated information and update the file-name bits.
            //
            TargetName = (PFILE_NAME)(GetIndexEntryValue(FoundEntry));
            TargetName->Info = Name->Info;
            TargetName->Flags = Name->Flags;

            if( ContainingBuffer != NULL ) {

                // This entry is in a buffer, so we have to write the
                // buffer while we've still got it.
                //
                Result = ContainingBuffer->Write( _AllocationAttribute );

            } else {

                // This entry is in the root, so we're done.
                //
                Result = TRUE;
            }
        }
    }

    DELETE( ContainingBuffer );
    return Result;
}

 
BOOLEAN
NTFS_INDEX_TREE::IsIndexEntryCorrupt(
    IN     PCINDEX_ENTRY       IndexEntry,
    IN     ULONG               MaximumLength,
    IN OUT PMESSAGE            Message,
    IN     INDEX_ENTRY_TYPE    IndexEntryType
    )
{
    ULONG   len;

    if (sizeof(INDEX_ENTRY) > MaximumLength) 
    {
        return TRUE;
    }

    if (IndexEntry->Length != QuadAlign(IndexEntry->Length) ||
        IndexEntry->Length > MaximumLength) 
    {
        return TRUE;
    }

    len = ((IndexEntry->Flags & INDEX_ENTRY_NODE) ? sizeof(VCN) : 0) +
          ((IndexEntry->Flags & INDEX_ENTRY_END) ? 0 : IndexEntry->AttributeLength) +
          sizeof(INDEX_ENTRY);

    DebugAssert(INDEX_ENTRY_WITH_DATA_TYPE_4 == 0 &&
                INDEX_ENTRY_WITH_DATA_TYPE_8 == 1 &&
                INDEX_ENTRY_WITH_DATA_TYPE_12 == 2 &&
                INDEX_ENTRY_WITH_DATA_TYPE_16 == 3);

    switch (IndexEntryType) {
        case INDEX_ENTRY_WITH_DATA_TYPE_4:
        case INDEX_ENTRY_WITH_DATA_TYPE_8:
        case INDEX_ENTRY_WITH_DATA_TYPE_12:
        case INDEX_ENTRY_WITH_DATA_TYPE_16:
            if (!(IndexEntry->Flags & INDEX_ENTRY_END) &&
                IndexEntry->AttributeLength != IndexEntryAttributeLength[IndexEntryType]) 
            {
                return TRUE;
            }

            // fall through

        case INDEX_ENTRY_WITH_DATA_TYPE:
            if (QuadAlign(IndexEntry->DataOffset + IndexEntry->DataLength) >
                IndexEntry->Length) 
            {
                return TRUE;
            }

            len += IndexEntry->DataLength;

            // fall through

        case INDEX_ENTRY_WITH_FILE_NAME_TYPE:
            if (IndexEntry->Length != QuadAlign(len)) 
            {
                return TRUE;
            } else
                return FALSE;
    }

    if (QuadAlign(len) > IndexEntry->Length) 
    {
        return TRUE;
    }
	else
        return FALSE;
}

 
BOOLEAN
NTFS_INDEX_TREE::FindEntry(
    IN  ULONG               KeyLength,
    IN  PVOID               KeyValue,
    IN  ULONG               Ordinal,
    OUT PINDEX_ENTRY*       FoundEntry,
    OUT PNTFS_INDEX_BUFFER* ContainingBuffer,
    OUT PINTSTACK           ParentTrail
    )
/*++

Routine Description:

    This method locates an entry (based on its key value) in
    the index tree.  If no matching entry is found, it locates
    the first leaf entry which is greater than the search value
    (i.e. the point at which the search value would be inserted
    into the tree).

Arguments:

    KeyLength           --  supplies the length, in bytes, of the
                            search value
    KeyValue            --  supplies the search value
    Ordinal             --  supplies the (zero-based) ordinal of the
                            matching entry to return.  (zero returns
                            the first matching value).

                            Note that a value of INDEX_SKIP skips
                            all matching entries.

    FoundEntry          --  Receives a pointer to the located entry
                            (NULL indicates error).
    ContainingBuffer    --  Receives a pointer to the index buffer
                            containing the returned entry (NULL if the
                            entry is in the root).
    ParentTrail         --  Receives the parent trail of ContainingBuffer
                            (ie. the VCNs of that buffer's ancestors).
                            If the entry is in the root, this object
                            may be left uninitialized.

Return Value:

    TRUE If a matching entry is found.

    FALSE if no matching entry was found.  If no error occurred, then
    *FoundEntry will point at the place in the tree where the search
    value would be inserted.

    If the method fails due to error, it returns FALSE and sets
    *FoundEntry to NULL.

    Note that if FindEntry does not find a matching entry, it will
    always return a leaf entry.

--*/
{
    PINDEX_ENTRY SearchEntry;
    VCN CurrentBufferVcn;
    PNTFS_INDEX_BUFFER CurrentBuffer=NULL;
    BOOLEAN Finished = FALSE;
    BOOLEAN Result = FALSE;
    USHORT SearchEntryLength;

    // Rig up an index-entry to pass to the index root and buffers:

    SearchEntryLength = (USHORT)QuadAlign( sizeof( INDEX_ENTRY ) + KeyLength );

    if( (SearchEntry = (PINDEX_ENTRY)MALLOC( SearchEntryLength )) == NULL ) {

        // Return the error.

        *FoundEntry = NULL;
        return FALSE;

    }

    SearchEntry->Length = SearchEntryLength;
    SearchEntry->AttributeLength = (USHORT)KeyLength;

    memcpy( GetIndexEntryValue( SearchEntry ),
            KeyValue,
            KeyLength );


    // See if the entry we want is in the index root:

    if( _IndexRoot->FindEntry( SearchEntry,
                               &Ordinal,
                               FoundEntry ) ) {

        // The desired entry is in the root.  *FoundEntry has been set
        // by the Index Root; fill in the other return parameters

        *ContainingBuffer = NULL;
        Result = TRUE;

    } else if ( *FoundEntry == NULL ) {

        // An error occurred trying to find the entry.

        *ContainingBuffer = NULL;
        Result = FALSE;

    } else if( !((*FoundEntry)->Flags & INDEX_ENTRY_NODE) ||
               GetDownpointer( *FoundEntry ) == INVALID_VCN ) {

        // The entry we want isn't in the root, and the root is a leaf,
        // so it's not in the tree.  Return the entry we did find, and
        // return 'not found' to the client.

        *ContainingBuffer = NULL;
        Result = FALSE;

    } else {

        // We didn't find the entry we want in the index root, and
        // the root is not a leaf, so we'll start looking through the
        // index allocation buffers.

        // First, we have to allocate an index allocation buffer
        // for our search.  If all goes well, we'll return this
        // buffer to the client.  Initialize the parent trail, but
        // leave it empty (indicating that we're at the root).

        if( !ParentTrail->Initialize() ||
            (CurrentBuffer = NEW NTFS_INDEX_BUFFER) == NULL ) {

            *FoundEntry = NULL;
        }

        if (_AllocationAttribute == NULL) {


            *FoundEntry = NULL;
        }

        while( *FoundEntry != NULL && !Finished ) {

            DebugAssert( ((*FoundEntry)->Flags & INDEX_ENTRY_NODE) &&
                       GetDownpointer( *FoundEntry ) != INVALID_VCN );

            CurrentBufferVcn = GetDownpointer( *FoundEntry );

            if( !CurrentBuffer->Initialize( _Drive,
                                            CurrentBufferVcn,
                                            _ClusterFactor * _Drive->QuerySectorSize(),
                                            _ClustersPerBuffer,
                                            _BufferSize,
                                            _CollationRule,
                                            _UpcaseTable ) ||
                !CurrentBuffer->Read( _AllocationAttribute ) ) {

                *FoundEntry = NULL;

            } else if( CurrentBuffer->FindEntry( SearchEntry,
                                                 &Ordinal,
                                                 FoundEntry ) ) {

                // We found the entry we want.

                Finished = TRUE;
                Result = TRUE;

            } else if ( *FoundEntry != NULL &&
                        (!((*FoundEntry)->Flags & INDEX_ENTRY_NODE) ||
                         GetDownpointer( *FoundEntry ) == INVALID_VCN) ) {

                // This buffer is a leaf, so the entry we want isn't
                // to be found.  Instead, we'll return this entry, along
                // with a result of FALSE to indicate 'not found'.

                Finished = TRUE;
                Result = FALSE;

            } else {

                // We have to recurse down another level in the tree.
                // Add the current buffer's VCN to the parent trail.

                if( !ParentTrail->Push( CurrentBufferVcn ) ) {

                    // Error.  Drop out of the loop and into the error
                    // handling.

                    *FoundEntry = NULL;
                }
            }
        }

        if( *FoundEntry == NULL ) {

            // We're returning an error, so we have to clean up.

            DELETE( CurrentBuffer );
            CurrentBuffer = NULL;
            *ContainingBuffer = NULL;
            Result = FALSE;

        } else {

            // We're returning an entry--either the one the client
            // wants or the next leaf.  Either way, it's contained
            // in the current buffer, so we need to return that, too.

            *ContainingBuffer = CurrentBuffer;
        }
    }

    FREE( SearchEntry );

    return Result;
}

 
BOOLEAN
NTFS_INDEX_TREE::InsertIntoRoot(
    PCINDEX_ENTRY   NewEntry,
    PINDEX_ENTRY    InsertionPoint
    )
/*++

Routine Description:

    This method attempts to insert an entry into the Index Root
    attribute.  If necessary, it will twiddle the index b-tree.

Arguments:

    NewEntry        --  supplies the new index entry
    InsertionPoint  --  supplies a pointer to the point in the root
                        where the entry should be inserted, if known.
                        This must be a pointer that was returned by a
                        call to _IndexRoot->FindEntry (with no intervening
                        inserts or deletes).  This parameter may be NULL.

Return Value:

    TRUE upon successful completion.

--*/
{
    NTFS_INDEX_BUFFER NewBuffer;
    INTSTACK ParentTrail;
    VCN NewBufferVcn;
    ULONG BytesToMove;
    PINDEX_ENTRY CurrentEntry;

    // Try the easy case--NTFS_INDEX_ROOT::InsertEntry will succeed
    // if there's room in the root for the new entry.

    if( _IndexRoot->InsertEntry( NewEntry, InsertionPoint ) ) {

        return TRUE;
    }

    //  We didn't get away with the easy case.  Instead, we have to
    //  push the entries that are currently in the index root down
    //  into an index allocation buffer.  Here's the plan:
    //
    //      If we don't have an allocation attribute, create one.
    //      Allocate a new index buffer.
    //      Create it as an empty buffer.  If the root is currently
    //          a leaf, this new buffer becomes a leaf; if not, not.
    //      Move all the index entries that are in the root to the
    //          new buffer
    //      Recreate the root as an empty node, and set the downpointer
    //          of its END entry to point at the new buffer.

    if( _AllocationAttribute == NULL &&
        !CreateAllocationAttribute() ) {

        // Can't create an allocation attribute.
        return FALSE;
    }


    // Allocate and initialize the new buffer.  Postpone creating it
    // until we know what to give it as an end-entry downpointer

    if( !AllocateIndexBuffer( &NewBufferVcn ) ) {

        return FALSE;
    }

    if( !NewBuffer.Initialize( _Drive,
                               NewBufferVcn,
                               _ClusterFactor * _Drive->QuerySectorSize(),
                               _ClustersPerBuffer,
                               _BufferSize,
                               _CollationRule,
                               _UpcaseTable ) ) {

        FreeIndexBuffer( NewBufferVcn );
    }


    // Now copy all the non-end entries from the index root to
    // the new buffer.

    BytesToMove = 0;

    CurrentEntry = _IndexRoot->GetFirstEntry();

    while( !(CurrentEntry->Flags & INDEX_ENTRY_END) ) {

        BytesToMove += CurrentEntry->Length;
        CurrentEntry = GetNextEntry( CurrentEntry );
    }

    // OK, now we can create the new buffer and copy the entries into
    // it.

    if( CurrentEntry->Flags & INDEX_ENTRY_NODE &&
        GetDownpointer( CurrentEntry ) != INVALID_VCN ) {

        // Give the new buffer's end entry the downpointer from the
        // root's end entry.

        NewBuffer.Create( FALSE, GetDownpointer( CurrentEntry ) );

    } else {

        // The new buffer is a leaf.

        NewBuffer.Create( TRUE, 0 );
    }

    NewBuffer.InsertClump( BytesToMove,
                           _IndexRoot->GetFirstEntry() );

    NewBuffer.Write( _AllocationAttribute );


    // Recreate the index root as an empty node.  This will wipe out the
    // old end entry, which is OK.  (If it had a downpointer, we passed
    // that value to the new buffer's end entry; if not, then it didn't
    // have any interesting information.)

    _IndexRoot->Recreate( FALSE, NewBufferVcn );

    // Set up an empty stack for the parent trail (since the new
    // buffer's parent is the root) and insert the new entry into
    // the new leaf buffer.

    return( ParentTrail.Initialize() &&
            InsertIntoBuffer( &NewBuffer, &ParentTrail, NewEntry ) );
}

 
BOOLEAN
NTFS_INDEX_TREE::InsertIntoBuffer(
    IN OUT PNTFS_INDEX_BUFFER  TargetBuffer,
    IN OUT PINTSTACK           ParentTrail,
    IN     PCINDEX_ENTRY       NewEntry,
    IN     PINDEX_ENTRY        InsertionPoint
    )
/*++

Routine Description:

    This method attempts to insert an entry into an Index
    Allocation Buffer.  If necessary, it will split the buffer.

Arguments:

    TargetBuffer    --  supplies the buffer that will receive the
                        new entry.
    ParentTrail     --  supplies the parent trail (ie. stack of VCNs
                        of all buffers between here and root) of the
                        target buffer.  If this stack is empty, then
                        the parent of the buffer is the root.
    NewEntry        --  supplies the new index entry
    InsertionPoint  --  supplies a pointer to the point in the root
                        where the entry should be inserted, if known.
                        This must be a pointer that was returned by a
                        call to TargetBuffer->FindEntry (with no
                        intervening inserts or deletes).  This parameter
                        may be NULL.

Return Value:

    TRUE upon successful completion.

Notes:

    This method may consume ParentTrail.  The client should not rely
    on the state of ParentTrail after this method returns.

--*/
{
    PINDEX_ENTRY PromotionBuffer;
    PINDEX_ENTRY SplitPoint;
    NTFS_INDEX_BUFFER NewBuffer, ParentBuffer;
    VCN NewBufferVcn, ParentVcn;
    ULONG BytesToCopy, BytesToRemove;
    BOOLEAN Result;
    int CompareResult;

    // Try the easy way first--NTFS_INDEX_BUFFER will succeed if
    // there's enough room in the buffer to accept this entry.

    if( TargetBuffer->InsertEntry( NewEntry, InsertionPoint ) ) {

        return( TargetBuffer->Write( _AllocationAttribute ) );
    }

    //  We didn't get away with the easy case; instead, we have to
    //  split this index buffer.

    //  Allocate a new index allocation buffer.

    if( !AllocateIndexBuffer( &NewBufferVcn ) ) {

        return FALSE;
    }

    if( !NewBuffer.Initialize( _Drive,
                               NewBufferVcn,
                               _ClusterFactor * _Drive->QuerySectorSize(),
                               _ClustersPerBuffer,
                               _BufferSize,
                               _CollationRule,
                               _UpcaseTable ) ) {

        FreeIndexBuffer( NewBufferVcn );
        return FALSE;
    }

    // Find the split point in the buffer we want to split.  This
    // entry will be promoted into the parent; the entries after it
    // stay in this buffer, while the entries before it go into the
    // new buffer.  The new buffer will become the child of the promoted
    // entry.

    SplitPoint = TargetBuffer->FindSplitPoint();

    PromotionBuffer = (PINDEX_ENTRY)MALLOC( TargetBuffer->QuerySize() );

    if( PromotionBuffer == NULL ) {

        FreeIndexBuffer( NewBufferVcn );
        return FALSE;
    }

    memcpy( PromotionBuffer,
            SplitPoint,
            SplitPoint->Length );

    if( TargetBuffer->IsLeaf() ) {

        PromotionBuffer->Flags |= INDEX_ENTRY_NODE;
        PromotionBuffer->Length += sizeof(VCN);
        NewBuffer.Create( TRUE, 0 );

    } else {

        NewBuffer.Create( FALSE, GetDownpointer(PromotionBuffer) );
    }

    GetDownpointer( PromotionBuffer ) = NewBufferVcn;


    // OK, copy all the entries before the split point into the
    // new buffer.

    BytesToCopy = (ULONG)((PBYTE)SplitPoint - (PBYTE)(TargetBuffer->GetFirstEntry()));

    NewBuffer.InsertClump( BytesToCopy, TargetBuffer->GetFirstEntry() );


    //  Now shift the remaining entries down, and adjust the target
    //  buffer's FirstFreeByte field by the number of bytes we moved
    //  to the new buffer.

    BytesToRemove = BytesToCopy + SplitPoint->Length;

    TargetBuffer->RemoveClump( BytesToRemove );


    // Now we decide which buffer gets the new entry, and insert it.
    // If it's less than the promoted entry, it goes in the new buffer;
    // otherwise, it goes in the original buffer.

    CompareResult = CompareNtfsIndexEntries( NewEntry,
                                             PromotionBuffer,
                                             _CollationRule,
                                             _UpcaseTable );

    //
    // Either of the buffer should now be large enough for the new entry
    //

    if( CompareResult < 0 ) {

        if (!NewBuffer.InsertEntry( NewEntry )) {
            FREE(PromotionBuffer);
            DebugAbort("Unable to insert the new entry into the new buffer.\n");
            return FALSE;
        }

    } else {

        if (!TargetBuffer->InsertEntry( NewEntry )) {
            FREE(PromotionBuffer);
            DebugAbort("Unable to insert the new entry into the target buffer.\n");
            return FALSE;
        }
    }

    if (!TargetBuffer->Write( _AllocationAttribute ) ||
        !NewBuffer.Write( _AllocationAttribute )) {
        FREE(PromotionBuffer);
        DebugAbort("Unable to write out the contents of the buffers\n");
        return FALSE;
    }

    // OK, we've finished splitting everybody, so we are ready to
    // insert the promoted entry into the parent.

    if( ParentTrail->QuerySize() == 0 ) {

        // The parent of the target buffer is the root.

        Result = InsertIntoRoot( PromotionBuffer );

    } else {

        // The target buffer's parent is another buffer, and its
        // VCN is on top of the ParentTrail stack.  Get that VCN,
        // and then pop the stack so we can pass it to the parent
        // buffer.  (Popping it makes it the parent trail of the
        // parent buffer.)

        ParentVcn = ParentTrail->Look();
        ParentTrail->Pop();

        Result = ( ParentBuffer.Initialize( _Drive,
                                            ParentVcn,
                                            _ClusterFactor * _Drive->QuerySectorSize(),
                                            _ClustersPerBuffer,
                                            _BufferSize,
                                            _CollationRule,
                                            _UpcaseTable ) &&
                   ParentBuffer.Read( _AllocationAttribute ) &&
                   InsertIntoBuffer( &ParentBuffer,
                                     ParentTrail,
                                     PromotionBuffer ) );
    }

    FREE( PromotionBuffer );
    return Result;
}



 
BOOLEAN
NTFS_INDEX_TREE::AllocateIndexBuffer(
    OUT PVCN    NewBufferVcn
    )
/*++

Routine Description:

    This method allocates an index allocation buffer from the index
    allocation attribute.  It first checks the bitmap, to see if any
    are free; if there are none free in the bitmap, it adds a new
    index buffer to the end of the allocation attribute.

Arguments:

    NewBuffer   -- receives the VCN of the new buffer.

Return Value:

    TRUE upon successful completion.

--*/
{
    BIG_INT ValueLength;
    VCN NewBufferNumber;
    ULONG NumberOfBuffers;


    DebugPtrAssert( _AllocationAttribute != NULL &&
                  _IndexAllocationBitmap != NULL );

    _AllocationAttribute->QueryValueLength( &ValueLength );

    DebugAssert( ValueLength % _BufferSize == 0 );

    NumberOfBuffers = ValueLength.GetLowPart()/_BufferSize;

    // First, check the bitmap.  Allocate as close to the beginning
    // as possible (hence use 0 for the NearHere parameter).

    if( _IndexAllocationBitmap->AllocateClusters( 0,
                                                  1,
                                                  &NewBufferNumber ) ) {

        //  Found a free one in the bitmap--return it.

        DebugPrint( "Buffer allocated from index allocation bitmap.\n" );

        if (0 == _ClustersPerBuffer) {
            *NewBufferVcn = NewBufferNumber * (_BufferSize / 512) ;
        } else {
            *NewBufferVcn = NewBufferNumber * _ClustersPerBuffer;
        }
        return TRUE;
    }


    //  There are no free buffers in the index allocation attribute,
    //  so I have to add one.

    NewBufferNumber = NumberOfBuffers;
    NumberOfBuffers += 1;

    //  Grow the allocation attribute by one buffer:

    if( !_AllocationAttribute->Resize( ValueLength + _BufferSize, _VolumeBitmap ) ) {

        return FALSE;
    }


    //  Grow the index allocation bitmap (if necessary) to cover the
    //  current size of the index allocation attributes.

    if( !_IndexAllocationBitmap->Resize( NumberOfBuffers ) ) {

        //  Couldn't resize the bitmap--truncate the allocation attribute
        //  back to its original size and return failure.

        _AllocationAttribute->Resize( ValueLength, _VolumeBitmap );
        return FALSE;
    }

    //  Mark the new buffer as allocated and return it.

    _IndexAllocationBitmap->SetAllocated( NewBufferNumber, 1 );

    if (0 == _ClustersPerBuffer) {

        // The buffers are indexed by their block offset, where each block
        // in the allocation is 512 bytes.
        //

        *NewBufferVcn = NewBufferNumber * (_BufferSize / NTFS_INDEX_BLOCK_SIZE);

    } else {
        *NewBufferVcn = NewBufferNumber * _ClustersPerBuffer;
    }

    return TRUE;
}


 
VOID
NTFS_INDEX_TREE::FreeIndexBuffer(
    IN VCN BufferVcn
    )
/*++

Routine Description:

    This method adds a buffer, identified by VCN, to the free
    buffer list.

Arguments:

    BufferVcn   --  supplies the VCN of the buffer to free.

Return Value:

    None.

--*/
{
    if (0 == _ClustersPerBuffer) {
        _IndexAllocationBitmap->SetFree( BufferVcn, _BufferSize/512 );
    } else {
        _IndexAllocationBitmap->SetFree( BufferVcn/_ClustersPerBuffer, 1 );
    }
}


 
ULONG
NTFS_INDEX_TREE::QueryMaximumEntrySize(
    ) CONST
/*++

Routine Description:

    This method returns the maximum size buffer needed to hold an
    index entry from this index.

Arguments

    None.

Return Value:

    None.

Notes:

    The maximum entry size must be less than the buffer size for
    the allocation buffers in the tree (since an entry must fit
    into a buffer), so we'll return the index allocation buffer size.

--*/
{
    return( _BufferSize );
}


 
BOOLEAN
NTFS_INDEX_TREE::CreateAllocationAttribute(
    )
/*++

Routine Description:

    This method creates an allocation attribute.  This attribute is
    an empty, nonresident attribute.  This method also creates an
    index allocation bitmap associated with this index allocation
    attribute.

Arguments:

    None.

Return value:

    TRUE upon successful completion.  Note that if this method succeeds,
    the private member data _AllocationAttribute is set to point at the
    newly-created attribute and _IndexAllocationBitmap is set to point
    at the newly-created bitmap.

--*/
{
    PNTFS_ATTRIBUTE NewAttribute;
    PNTFS_BITMAP NewBitmap;
    NTFS_EXTENT_LIST Extents;

    DebugAssert(0 != _ClusterFactor);


    // Create an empty extent list.

    if( !Extents.Initialize( (ULONG)0, (ULONG)0 ) ) {

        return FALSE;
    }


    // Construct an index allocation attribute and initialize
    // it with this extent list.

    if( (NewAttribute = NEW NTFS_ATTRIBUTE) == NULL ||
        !NewAttribute->Initialize( _Drive,
                                   _ClusterFactor,
                                   &Extents,
                                   0,
                                   0,
                                   $INDEX_ALLOCATION,
                                   _Name ) ) {

        DebugPrint( "CreateAllocationAttribute--Cannot create index allocation attribute.\n" );

        DELETE( NewAttribute );
        return FALSE;
    }

    // Create a new bitmap.  Initialize it to cover zero allocation units,
    // and indicate that it is growable.

    if( (NewBitmap = NEW NTFS_BITMAP) == NULL ||
        !NewBitmap->Initialize( 0, TRUE ) ) {

        DebugPrint( "CreateAllocationAttribute--Cannot create index allocation bitmap.\n" );

        DELETE( NewAttribute );
        DELETE( NewBitmap );
        return FALSE;
    }

    _AllocationAttribute = NewAttribute;
    _IndexAllocationBitmap = NewBitmap;

    return TRUE;
}



