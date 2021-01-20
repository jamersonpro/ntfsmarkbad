/*++

Module Name:

        attrlist.hxx

Abstract:

        This module contains the declarations for NTFS_ATTRIBUTE_LIST,
        which models an ATTRIBUTE_LIST Attribute in an NTFS File Record
    Segment.

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

#pragma once

#include "attrib.hxx"
#include "hmem.hxx"
#include "volume.hxx"

DECLARE_CLASS( LOG_IO_DP_DRIVE );
DECLARE_CLASS( WSTRING );
DECLARE_CLASS( NTFS_ATTRIBUTE_RECORD );
DECLARE_CLASS( NTFS_ATTRIBUTE_RECORD_LIST );
DECLARE_CLASS( NTFS_BITMAP );
DECLARE_CLASS( NTFS_UPCASE_TABLE );
DECLARE_CLASS( NTFS_ATTRIBUTE_LIST );

// This macro produces a pointer to the wide-character name of an attribute
// list entry from a pointer to an attribute list entry.

#define NameFromEntry( x ) ((PWSTR)((PBYTE)(x)+(x)->AttributeNameOffset))

// This macro produces a pointer to the attribute list entry
// following x

#define NextEntry( x ) \
    ((PATTRIBUTE_LIST_ENTRY)((PBYTE)(x) + (x)->RecordLength))

typedef struct _ATTR_LIST_CURR_ENTRY {
        PATTRIBUTE_LIST_ENTRY   CurrentEntry;
        ULONG                   CurrentOffset;
};

DEFINE_TYPE( _ATTR_LIST_CURR_ENTRY, ATTR_LIST_CURR_ENTRY );

class NTFS_ATTRIBUTE_LIST : public NTFS_ATTRIBUTE {

    public:
         
        DECLARE_CONSTRUCTOR( NTFS_ATTRIBUTE_LIST );

        VIRTUAL
        ~NTFS_ATTRIBUTE_LIST(
            );

         
        BOOLEAN
        Initialize (
            IN OUT  PLOG_IO_DP_DRIVE    Drive,
            IN      ULONG               ClusterFactor,
            IN      PNTFS_UPCASE_TABLE  UpcaseTable
            );

         
        BOOLEAN
        Initialize (
            IN OUT  PLOG_IO_DP_DRIVE        Drive,
            IN      ULONG                   ClusterFactor,
            IN      PCNTFS_ATTRIBUTE_RECORD AttributeRecord,
            IN      PNTFS_UPCASE_TABLE      UpcaseTable
            );

         
        BOOLEAN
        AddEntry(
            IN  ATTRIBUTE_TYPE_CODE     Type,
            IN  VCN                     LowestVcn,
            IN  PCMFT_SEGMENT_REFERENCE SegmentReference,
            IN  USHORT                  InstanceTag,
            IN  PCWSTRING               Name    DEFAULT NULL
            );

         
        BOOLEAN
        DeleteEntry(
            IN ULONG EntryIndex
            );

         
        BOOLEAN
        DeleteCurrentEntry(
            IN PATTR_LIST_CURR_ENTRY    Entry
            );

         
        BOOLEAN
        DeleteEntries(
            IN  ATTRIBUTE_TYPE_CODE Type,
            IN  PCWSTRING           Name DEFAULT NULL
            );

         
        BOOLEAN
        IsInList(
            IN  ATTRIBUTE_TYPE_CODE Type,
            IN  PCWSTRING           Name DEFAULT NULL
            ) CONST;


         
        BOOLEAN
        QueryNextEntry(
            IN OUT PATTR_LIST_CURR_ENTRY   CurrEntry,
               OUT PATTRIBUTE_TYPE_CODE    Type,
               OUT PVCN                    LowestVcn,
               OUT PMFT_SEGMENT_REFERENCE  SegmentReference,
               OUT PUSHORT                 InstanceTag,
               OUT PWSTRING                Name
            ) CONST;

         
        BOOLEAN
        QueryEntry(
            IN  MFT_SEGMENT_REFERENCE   SegmentReference,
            IN  USHORT                  InstanceTag,
            OUT PATTRIBUTE_TYPE_CODE    Type,
            OUT PVCN                    LowestVcn,
            OUT PWSTRING                Name
            ) CONST;

         
        PCATTRIBUTE_LIST_ENTRY
        GetNextAttributeListEntry(
            IN  PCATTRIBUTE_LIST_ENTRY  CurrentEntry
            ) CONST;

         
        BOOLEAN
        QueryExternalReference(
            IN  ATTRIBUTE_TYPE_CODE     Type,
            OUT PMFT_SEGMENT_REFERENCE  SegmentReference,
            OUT PULONG                  EntryIndex,
            IN  PCWSTRING               Name        DEFAULT NULL,
            IN  PVCN                    DesiredVcn  DEFAULT NULL,
            OUT PVCN                    StartingVcn DEFAULT NULL
            ) CONST;

         
        BOOLEAN
        QueryNextAttribute(
            IN OUT PATTRIBUTE_TYPE_CODE TypeCode,
            IN OUT PWSTRING             Name
            ) CONST;

         
        BOOLEAN
        ReadList(
            );

         
        BOOLEAN
        WriteList(
            PNTFS_BITMAP VolumeBitmap
            );


    private:

         
        VOID
        Construct (
            );

         
        VOID
        Destroy(
            );

         
        PATTRIBUTE_LIST_ENTRY
        FindEntry(
            IN  ATTRIBUTE_TYPE_CODE Type,
            IN  PCWSTRING           Name,
            IN  VCN                 LowestVcn,
            OUT PULONG              EntryOffset DEFAULT NULL,
            OUT PULONG              EntryIndex DEFAULT NULL
            ) CONST;


        HMEM                    _Mem;
        ULONG                   _LengthOfList;
        PNTFS_UPCASE_TABLE      _UpcaseTable;

};


 
INLINE
BOOLEAN
NTFS_ATTRIBUTE_LIST::WriteList(
    IN OUT PNTFS_BITMAP VolumeBitmap
    )
/*++

Routine Description:

    This method writes the list to disk.

Arguments:

    None.

Return Value:

    TRUE upon successful completion.

--*/
{
    ULONG BytesWritten;

    return( Resize( _LengthOfList, VolumeBitmap ) &&
            Write( _Mem.GetBuf(),
                   0,
                   _LengthOfList,
                   &BytesWritten,
                   VolumeBitmap ) &&
            BytesWritten == _LengthOfList );
}

