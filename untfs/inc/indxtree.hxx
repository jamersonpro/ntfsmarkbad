/*++

Module Name:

    indxtree.hxx

Abstract:

    this module contains the declarations for the NTFS_INDEX_TREE
    class, which models index trees on an NTFS volume.

    An NTFS Index Tree consists of an index root and a set of
    index buffers.  The index root is stored as the value of
    an INDEX_ROOT attribute; the index buffers are part of the
    value of an INDEX_ALLOCATION attribute.

--*/

#pragma once

#include "hmem.hxx"
#include "intstack.hxx"

DECLARE_CLASS( LOG_IO_DP_DRIVE );
DECLARE_CLASS( WSTRING );
DECLARE_CLASS( NTFS_ATTRIBUTE );
DECLARE_CLASS( NTFS_BITMAP );
DECLARE_CLASS( NTFS_INDEX_ROOT );
DECLARE_CLASS( NTFS_INDEX_BUFFER );
DECLARE_CLASS( NTFS_FILE_RECORD_SEGMENT );
DECLARE_CLASS( NTFS_UPCASE_TABLE );

// This constant is given to FindEntry to indicate it should skip
// all matching entries.
//
CONST ULONG INDEX_SKIP = (ULONG)(-1);

typedef enum INDEX_ITERATOR_STATE {

    INDEX_ITERATOR_RESET,
    INDEX_ITERATOR_CURRENT,
    INDEX_ITERATOR_INVALID,
    INDEX_ITERATOR_DELETED,
    INDEX_ITERATOR_CORRUPT
};

typedef enum INDEX_ENTRY_TYPE {
    INDEX_ENTRY_WITH_DATA_TYPE_4 = 0,   // value is used as an index to
    INDEX_ENTRY_WITH_DATA_TYPE_8 = 1,   // IndexEntryAttributeLength array;
    INDEX_ENTRY_WITH_DATA_TYPE_12 = 2,  // these values should not be changed
    INDEX_ENTRY_WITH_DATA_TYPE_16 = 3,
    INDEX_ENTRY_WITH_DATA_TYPE,
    INDEX_ENTRY_WITH_FILE_NAME_TYPE,
    INDEX_ENTRY_GENERIC_TYPE
};

LONG
NtfsCollate(
    IN PCVOID               Value1,
    IN ULONG                Length1,
    IN PCVOID               Value2,
    IN ULONG                Length2,
    IN COLLATION_RULE       CollationRule,
    IN PNTFS_UPCASE_TABLE   UpcaseTable OPTIONAL
    );

LONG
CompareNtfsIndexEntries(
    IN PCINDEX_ENTRY        Entry1,
    IN PCINDEX_ENTRY        Entry2,
    IN COLLATION_RULE       CollationRule,
    IN PNTFS_UPCASE_TABLE   UpcaseTable OPTIONAL
    );


class NTFS_INDEX_TREE : public OBJECT {

    public:

         
        DECLARE_CONSTRUCTOR( NTFS_INDEX_TREE );

        VIRTUAL
         
        ~NTFS_INDEX_TREE(
            );

         
         
        BOOLEAN
        Initialize(
            IN OUT PLOG_IO_DP_DRIVE             Drive,
            IN     ULONG                        ClusterFactor,
            IN OUT PNTFS_BITMAP                 VolumeBitmap,
            IN     PNTFS_UPCASE_TABLE           UpcaseTable,
            IN     ULONG                        MaximumRootSize,
            IN     PNTFS_FILE_RECORD_SEGMENT    SourceFrs,
            IN     PCWSTRING                    IndexName DEFAULT NULL
            );

         
        BOOLEAN
        Initialize(
            IN     ATTRIBUTE_TYPE_CODE  IndexedAttributeType,
            IN OUT PLOG_IO_DP_DRIVE     Drive,
            IN     ULONG                ClusterFactor,
            IN OUT PNTFS_BITMAP         VolumeBitmap,
            IN     PNTFS_UPCASE_TABLE   UpcaseTable,
            IN     COLLATION_RULE       CollationRule,
            IN     ULONG                IndexBufferSize,
            IN     ULONG                MaximumRootSize,
            IN     PCWSTRING            IndexName DEFAULT NULL
            );


         
        BOOLEAN
        InsertEntry(
            IN  PCINDEX_ENTRY    NewEntry,
            IN  BOOLEAN          NoDuplicates DEFAULT TRUE,
            IN  PBOOLEAN         Duplicate DEFAULT NULL
            );



         
        BOOLEAN
        Save(
            IN OUT PNTFS_FILE_RECORD_SEGMENT TargetFrs
            );


         
        COLLATION_RULE
        QueryCollationRule(
            );

         
        ATTRIBUTE_TYPE_CODE
        QueryIndexedAttributeType(
            );

         
        UCHAR
        QueryClustersPerBuffer(
            );

         
        ULONG
        QueryBufferSize(
            );

         
        VOID
        FreeAllocation(
            );

         
        BOOLEAN
        UpdateFileName(
            IN PCFILE_NAME      Name,
            IN FILE_REFERENCE   ContainingFile
            );

         
        PCWSTRING
        GetName(
            ) CONST;

        STATIC
        BOOLEAN
        IsIndexEntryCorrupt(
            IN  PCINDEX_ENTRY           IndexEntry,
            IN  ULONG                   MaximumLength,
            IN  PMESSAGE                Message,
            IN  INDEX_ENTRY_TYPE        IndexEntryType  DEFAULT INDEX_ENTRY_GENERIC_TYPE
            );


   private:

         
        VOID
        Construct(
            );

         
        VOID
        Destroy(
            );

         
        BOOLEAN
        FindEntry(
            IN  ULONG               KeyLength,
            IN  PVOID               KeyValue,
            IN  ULONG               Ordinal,
            OUT PINDEX_ENTRY*       FoundEntry,
            OUT PNTFS_INDEX_BUFFER* ContainingBuffer,
            OUT PINTSTACK           ParentTrail
            );


         
        BOOLEAN
        InsertIntoRoot(
            IN PCINDEX_ENTRY   NewEntry,
            IN PINDEX_ENTRY    InsertionPoint DEFAULT NULL
            );

         
        BOOLEAN
        InsertIntoBuffer(
            IN OUT  PNTFS_INDEX_BUFFER  TargetBuffer,
            IN OUT  PINTSTACK           ParentTrail,
            IN      PCINDEX_ENTRY       NewEntry,
            IN      PINDEX_ENTRY        InsertionPoint DEFAULT NULL
            );

	
        BOOLEAN
        AllocateIndexBuffer(
            OUT PVCN    NewBufferVcn
            );

         
        VOID
        FreeIndexBuffer(
            IN VCN BufferVcn
            );

         
        ULONG
        QueryMaximumEntrySize(
            ) CONST;

         
        BOOLEAN
        CreateAllocationAttribute(
            );


        PLOG_IO_DP_DRIVE    _Drive;
        ULONG               _ClusterFactor;
        ULONG               _ClustersPerBuffer;
        ULONG               _BufferSize;
        PNTFS_BITMAP        _VolumeBitmap;
        PNTFS_ATTRIBUTE     _AllocationAttribute;
        PNTFS_INDEX_ROOT    _IndexRoot;
        PNTFS_BITMAP        _IndexAllocationBitmap;

        PWSTRING            _Name;
        ATTRIBUTE_TYPE_CODE _IndexedAttributeType;
        COLLATION_RULE      _CollationRule;

        PNTFS_UPCASE_TABLE  _UpcaseTable;


        // Iterator state information:
        //
        // Each index tree has a single iterator associated with it.
        // This iterator oscillates among the following states:
        //
        //  INDEX_ITERATOR_RESET    --  the iterator is at the beginning
        //                              of the index, and the next call
        //                              to GetNext will return the first
        //                              entry in the index.
        //  INDEX_ITERATOR_CURRENT  --  _CurrentEntry points at the current
        //                              entry. _IsCurrentEntryInRoot is
        //                              TRUE if that entry is in the index
        //                              root, otherwise _CurrentEntryBuffer
        //                              points to the buffer that contains
        //                              the entry, and _CurrentEntryTrail
        //                              contains the parent trail of that
        //                              buffer.  In either case,
        //                              _CurrentKeyOrdinal gives the
        //                              ordinal of the current entry
        //                              (ie. it is the nth entry for
        //                              the current key).
        //  INDEX_ITERATOR_INVALID  --  _CurrentEntry has been invalidated,
        //                              and the tree must relocate the
        //                              current entry.  _CurrentKey,
        //                              _CurrentKeyLength, and
        //                              _CurrentKeyOrdinal give the entry
        //                              information to locate the current
        //                              entry.
        //  INDEX_ITERATOR_DELETED  --  Differs from INDEX_ITERATOR_INVALID
        //                              only in that _CurrentKey,
        //                              _CurrentKeyLength, and
        //                              _CurrentKeyOrdinal describe the
        //                              next entry, rather than the current.
        //  INDEX_ITERATOR_CORRUPT  --  The iterator (or the tree itself)
        //                              has become corrupt; any attempt to
        //                              use it will return error.
        //
        // Since the iterator is very closely coupled to the index
        // tree, it is built into this class, rather than being maintained
        // as a separate object.

        INDEX_ITERATOR_STATE    _IteratorState;

        BOOLEAN                 _IsCurrentEntryInRoot;
        PINDEX_ENTRY            _CurrentEntry;
        PNTFS_INDEX_BUFFER      _CurrentBuffer;
        INTSTACK                _CurrentEntryTrail;

        ULONG                   _CurrentKeyOrdinal;
        PVOID                   _CurrentKey;
        ULONG                   _CurrentKeyLength;
        ULONG                   _CurrentKeyMaxLength;

};


 
INLINE
 
COLLATION_RULE
NTFS_INDEX_TREE::QueryCollationRule(
    )
/*++

Routine Description:

    This method returns the collation rule by which this
    index tree is ordered.

Arguments:

    None.

Return Value:

    The index tree's collation rule.

--*/
{
    return _CollationRule;
}

 
INLINE
 
ATTRIBUTE_TYPE_CODE
NTFS_INDEX_TREE::QueryIndexedAttributeType(
    )
/*++

Routine Description:

    This method returns the type code of the attribute which is
    indexed by this index tree.

Arguments:

    None.

Return Value:
--*/
{
    return _IndexedAttributeType;
}

 
INLINE
 
UCHAR
NTFS_INDEX_TREE::QueryClustersPerBuffer(
    )
/*++

Routine Description:

    This method returns the number of clusters in each allocation
    buffer in this index tree.

Arguments:

    None.

Return Value:

    The number of clusters per allocation buffer in this tree.

--*/
{
    return (UCHAR)_ClustersPerBuffer;
}
 
INLINE
 
ULONG
NTFS_INDEX_TREE::QueryBufferSize(
    )
/*++

Routine Description:

    This method returns the size of each allocation
    buffer in this index tree.

Arguments:

    None.

Return Value:

    The number of bytes per allocation buffer in this tree.

--*/
{
    return _BufferSize;
}


INLINE
PCWSTRING
NTFS_INDEX_TREE::GetName(
    ) CONST
/*++

Routine Description:

    This method returns the name of the index.

Arguments:

    None.

Return Value:

    The name of the index.

--*/
{
    return _Name;
}


