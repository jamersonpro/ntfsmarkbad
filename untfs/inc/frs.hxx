/*++

Module Name:

        frs.hxx

Abstract:

        This module contains the declarations for the
        NTFS_FILE_RECORD_SEGMENT class.  This class models File
        Record Segments in the NTFS Master File Table; it is the
        object through which a file's attributes may be accessed.

--*/

#pragma once

#include "frsstruc.hxx"
#include "clusrun.hxx"
#include "array.hxx"
#include "hmem.hxx"
#include "list.hxx"
#include "iterator.hxx"

// Possible return codes for SortIndex:
//
//      NTFS_SORT_INDEX_NOT_FOUND       --  this FRS does not contain an
//                                          index with the specified name.
//      NTFS_SORT_INDEX_WELL_ORDERED    --  the index was not sorted because
//                                          it was found to be well-ordered.
//      NTFS_SORT_INDEX_BADLY_ORDERED   --  The index was found to be badly
//                                          ordered, and it was not sorted.
//      NTFS_SORT_INDEX_SORTED          --  The index was sorted and new
//                                          attributes were inserted into
//                                          the FRS.
//      NTFS_SORT_INSERT_FAILED         --  An insertion of an index entry
//                                          into the new tree failed.
//                                          (Probable cause:  out of space.)
//      NTFS_SORT_ERROR                 --  Sort failed because of an error.
//
//
typedef enum NTFS_SORT_CODE {

    NTFS_SORT_INDEX_NOT_FOUND,
    NTFS_SORT_INDEX_WELL_ORDERED,
    NTFS_SORT_INDEX_BADLY_ORDERED,
    NTFS_SORT_INDEX_SORTED,
    NTFS_SORT_INSERT_FAILED,
    NTFS_SORT_ERROR
};

// Possible return codes for VerifyAndFixQuotaDefaultId:
//
//        NTFS_QUOTA_INDEX_NOT_FOUND      --  this FRS does not contain an
//                                            index with the specified name.
//        NTFS_QUOTA_DEFAULT_ENTRY_MISSING--  the default entry was not found
//                                            in the index
//        NTFS_QUOTA_INDEX_FOUND          --  Found the default Id entry in the
//                                            index tree.
//        NTFS_QUOTA_INDEX_INSERTED       --  Inserted the default Id entry into
//                                            the index tree.
//        NTFS_QUOTA_INSERT_FAILED        --  An insertion of the default Id
//                                            entry into the index tree failed.
//                                            (Probable cause:  out of space.)
//        NTFS_QUOTA_ERROR                --  error occurred.  (Possibly out
//                                            of memory or out of space.)
//
typedef enum NTFS_QUOTA_CODE {

    NTFS_QUOTA_INDEX_NOT_FOUND,
    NTFS_QUOTA_INDEX_FOUND,
    NTFS_QUOTA_DEFAULT_ENTRY_MISSING,
    NTFS_QUOTA_INDEX_INSERTED,
    NTFS_QUOTA_INSERT_FAILED,
    NTFS_QUOTA_ERROR
};

// Possible return codes for FindSecurityIndexEntryAndValidate:
//
//        NTFS_SECURITY_INDEX_ENTRY_MISSING  --  the specified index entry key
//                                               cannot be found in the index
//        NTFS_SECURITY_INDEX_FOUND          --  the found entry contains
//                                               correct data
//        NTFS_SECURITY_INDEX_FIXED          --  the found entry contains invalid
//                                               data but is now corrected
//        NTFS_SECURITY_INDEX_DATA_ERROR     --  The index was found but the data
//                                               data in it is incorrect.
//        NTFS_SECURITY_INDEX_INSERTED       --  An index was successfully inserted
//                                               into the specified index.
//        NTFS_SECURITY_INSERT_FAILED        --  An insertion of an index entry
//                                               into the index tree failed.
//                                               (Probable cause:  out of space.)
//        NTFS_SECURITY_ERROR                --  failed because of an error.
//                                               (Probably out of memory or out
//                                               of space.)
//
typedef enum NTFS_SECURITY_CODE {
   NTFS_SECURITY_INDEX_ENTRY_MISSING,
   NTFS_SECURITY_INDEX_FOUND,
   NTFS_SECURITY_INDEX_FIXED,
   NTFS_SECURITY_INDEX_DATA_ERROR,
   NTFS_SECURITY_INDEX_INSERTED,
   NTFS_SECURITY_INSERT_FAILED,
   NTFS_SECURITY_ERROR
};

// Forward references

DECLARE_CLASS( IO_DP_DRIVE );
DECLARE_CLASS( NTFS_MASTER_FILE_TABLE );
DECLARE_CLASS( NTFS_MFT_FILE );
DECLARE_CLASS( NTFS_ATTRIBUTE );
DECLARE_CLASS( WSTRING );
DECLARE_CLASS( NTFS_ATTRIBUTE_RECORD );
DECLARE_CLASS( NTFS_ATTRIBUTE_RECORD_LIST );
DECLARE_CLASS( NTFS_FILE_RECORD_SEGMENT );
DECLARE_CLASS( NTFS_ATTRIBUTE_LIST );
DECLARE_CLASS( NTFS_BITMAP );
DECLARE_CLASS( NTFS_BAD_CLUSTER_FILE );


class NTFS_FILE_RECORD_SEGMENT : public NTFS_FRS_STRUCTURE {

        public:

         
        DECLARE_CONSTRUCTOR( NTFS_FILE_RECORD_SEGMENT );

        VIRTUAL
         
        ~NTFS_FILE_RECORD_SEGMENT (
                        );

         
         
        BOOLEAN
        Initialize (
            IN      VCN             FileNumber,
            IN OUT  PNTFS_MFT_FILE  MftFile
            );

         
         
        BOOLEAN
        Initialize (
            IN      VCN                     FileNumber,
            IN OUT  PNTFS_MASTER_FILE_TABLE Mft
            );

         
         
        BOOLEAN
        Initialize(
            IN      VCN                     FirstFileNumber,
            IN      ULONG                   FrsCount,
            IN OUT  PNTFS_MASTER_FILE_TABLE Mft
           );

         
         
        BOOLEAN
        Initialize(
           );

         
         
        BOOLEAN
        Create (
            IN  PCSTANDARD_INFORMATION  StandardInformation,
            IN  USHORT                  Flags DEFAULT 0
            );

         
        BOOLEAN
        Create (
            IN  PCMFT_SEGMENT_REFERENCE BaseSegment,
            IN  USHORT                  Flags DEFAULT 0
            );



        VIRTUAL
        BOOLEAN
        Write(
            );

         
        BOOLEAN
        Flush(
            IN OUT PNTFS_BITMAP         VolumeBitmap OPTIONAL,
            IN OUT PNTFS_INDEX_TREE     ParentIndex DEFAULT NULL,
            IN     BOOLEAN              FrsIsEmpty DEFAULT FALSE
            );

         
        BOOLEAN
        AddDataAttribute(
            IN     ULONG        InitialSize,
            IN OUT PNTFS_BITMAP VolumeBitmap,
            IN     BOOLEAN      Fill DEFAULT FALSE,
            IN     CHAR         FillCharacter DEFAULT 0
            );

         
         
        BOOLEAN
        AddFileNameAttribute(
            IN PFILE_NAME FileNameAttributeValue
            );

         
        BOOLEAN
        AddAttribute(
            IN     ATTRIBUTE_TYPE_CODE  Type,
            IN     PCWSTRING            Name    OPTIONAL,
            IN     PCVOID               Value   OPTIONAL,
            IN     ULONG                Length,
            IN OUT PNTFS_BITMAP         Bitmap  OPTIONAL,
            IN     BOOLEAN              IsIndexed DEFAULT FALSE
            );

         
        BOOLEAN
        AddEmptyAttribute(
            IN ATTRIBUTE_TYPE_CODE Type,
            IN PCWSTRING           Name DEFAULT NULL
            );

         
        BOOLEAN
        IsAttributePresent (
            IN      ATTRIBUTE_TYPE_CODE     Type,
            IN      PCWSTRING               Name            DEFAULT NULL,
            IN      BOOLEAN                 IgnoreExternal  DEFAULT FALSE
            );

         
        BOOLEAN
        QueryAttributeRecord (
            OUT PNTFS_ATTRIBUTE_RECORD  AttributeRecord,
            IN  ATTRIBUTE_TYPE_CODE     Type,
            IN  PCWSTRING               Name        DEFAULT NULL
            );

         
        BOOLEAN
        QueryAttribute (
            OUT PNTFS_ATTRIBUTE     Attribute,
            OUT PBOOLEAN            Error,
            IN  ATTRIBUTE_TYPE_CODE Type,
            IN  PCWSTRING           Name DEFAULT NULL
            );


         
        BOOLEAN
        QueryAttributeListAttribute (
            OUT PNTFS_ATTRIBUTE     Attribute,
            OUT PBOOLEAN            Error
            );

         
        BOOLEAN
        QueryFileSizes (
            OUT PBIG_INT            AllocatedLength,
            OUT PBIG_INT            FileSize,
            OUT PBOOLEAN            Error
            );


         
        BOOLEAN
        QueryAttributeByOrdinal (
            OUT PNTFS_ATTRIBUTE     Attribute,
            OUT PBOOLEAN            Error,
            IN  ATTRIBUTE_TYPE_CODE Type,
            IN  ULONG               Ordinal
            );

         
        BOOLEAN
        QueryAttributeByTag (
            OUT PNTFS_ATTRIBUTE     Attribute,
            OUT PBOOLEAN            Error,
            IN  ULONG               Tag
            );

         
        BOOLEAN
        PurgeAttribute (
            IN  ATTRIBUTE_TYPE_CODE Type,
            IN  PCWSTRING           Name    DEFAULT NULL,
            IN  BOOLEAN             IgnoreExternal DEFAULT FALSE
            );

         
        BOOLEAN
        DeleteResidentAttribute(
            IN  ATTRIBUTE_TYPE_CODE Type,
            IN  PCWSTRING           Name OPTIONAL,
            IN  PCVOID              Value,
            IN  ULONG               ValueLength,
            OUT PBOOLEAN            Deleted,
            IN  BOOLEAN             IgnoreExternal  DEFAULT FALSE
            );

         
        BOOLEAN
        DeleteResidentAttributeLocal(
            IN  ATTRIBUTE_TYPE_CODE Type,
            IN  PCWSTRING           Name OPTIONAL,
            IN  PCVOID              Value,
            IN  ULONG               ValueLength,
            OUT PBOOLEAN            Deleted,
            OUT PBOOLEAN            IsIndexed,
            OUT PUSHORT             InstanceTag
            );

        VIRTUAL
        BOOLEAN
        InsertAttributeRecord (
            IN OUT PNTFS_ATTRIBUTE_RECORD  NewRecord,
            IN     BOOLEAN                 ForceExternal DEFAULT FALSE
                        );

         
        USHORT
        QueryNextInstance(
            );

         
        VOID
        IncrementNextInstance(
            );

         
        ULONG
        QueryFreeSpace(
            );

         
        ULONG
        QueryMaximumAttributeRecordSize (
            ) CONST;

         
        BOOLEAN
        QueryDuplicatedInformation(
            OUT PDUPLICATED_INFORMATION DuplicatedInformation
            );

         
        BOOLEAN
        UpdateFileNames(
            IN     PDUPLICATED_INFORMATION  DuplicatedInformation,
            IN OUT PNTFS_INDEX_TREE         Index OPTIONAL,
            IN     BOOLEAN                  IgnoreExternal
            );

         
        BOOLEAN
        Backtrack(
            OUT PWSTRING Path
            );

         
        VOID
        SetLsn(
            IN  BIG_INT NewLsn
            );

         
        BOOLEAN
        PurgeAttributeList (
            );
        
         
        BOOLEAN
        IsAttributeListPresent(
            );

    protected:

         
        BOOLEAN
        Initialize(
            IN OUT  PLOG_IO_DP_DRIVE        Drive,
            IN      LCN                     StartOfMft,
            IN      PNTFS_MASTER_FILE_TABLE Mft
            );

    private:

         
        VOID
        Construct (
                );

        inline
         
        VOID
        Destroy2 (
                );

         
        VOID
        Destroy (
                );

         
        BOOLEAN
        Create (
            IN  USHORT  Flags DEFAULT 0
            );

         
        BOOLEAN
        SetupAttributeList(
            );

         
        BOOLEAN
        CreateAttributeList(
            OUT PNTFS_ATTRIBUTE_LIST    AttributeList
            );

         
        BOOLEAN
        SaveAttributeList(
            PNTFS_BITMAP VolumeBitmap
            );

         
        BOOLEAN
        InsertExternalAttributeRecord(
            IN  PNTFS_ATTRIBUTE_RECORD NewRecord
            );

         
        BOOLEAN
        BacktrackWorker(
            IN OUT PWSTRING Path
            );

         
        PNTFS_FILE_RECORD_SEGMENT
        SetupChild(
            IN VCN  FileNumber
            );

         
        BOOLEAN
        AddChild(
            PNTFS_FILE_RECORD_SEGMENT ChildFrs
            );

         
        PNTFS_FILE_RECORD_SEGMENT
        GetChild(
            VCN FileNumber
            );

         
        VOID
        DeleteChild(
            VCN FileNumber
            );

        HMEM                        _Mem;
        LIST                        _Children;
        PITERATOR                   _ChildIterator;
                PNTFS_MASTER_FILE_TABLE         _Mft;
        PNTFS_ATTRIBUTE_LIST        _AttributeList;

};

 
INLINE
USHORT
NTFS_FILE_RECORD_SEGMENT::QueryNextInstance(
    )
/*++

Routine Description:

    This method fetches the current value of the FRS'
    NextAttributeInstance field.

Arguments:

    None.

Return Value:

    The current value of the FRS' NextAttributeInstance field.

--*/
{
    return _FrsData->NextAttributeInstance;
}

 
INLINE
VOID
NTFS_FILE_RECORD_SEGMENT::IncrementNextInstance(
    )
/*++

Routine Description:

    This method increments the NextAttributeInstance field of
    the File Record Segment.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _FrsData->NextAttributeInstance++;
}


 
INLINE
ULONG
NTFS_FILE_RECORD_SEGMENT::QueryFreeSpace(
    )
/*++

Routine Description:

    This method returns the amount of free space following the
    last Attribute Record in the File Record Segment.

Arguments:

    None.

Return Value:

    The amount of free space.

Notes:

    This method assumes that the FRS is consistent.

--*/
{
    return _FrsData->BytesAvailable - _FrsData->FirstFreeByte;
}

 
INLINE
ULONG
NTFS_FILE_RECORD_SEGMENT::QueryMaximumAttributeRecordSize (
    ) CONST
/*++

Routine Description:

    This method returns the size of the largest attribute record
    the File Record Segment will accept.  Note that this is the
    largest record it will ever accept, not what it can currently
    accept.

Arguments:

    None.

Return Value:

    The size of the largest attribute record a File Record Segment
    of this size can accept.

--*/
{
    ULONG temp;

    //
    // Take a precaution to make sure this routine never returns a
    // "negative" number.
    //

    temp =  _FrsData->FirstAttributeOffset + QuadAlign(sizeof(ATTRIBUTE_TYPE_CODE));

    if (temp > QuerySize()) {

        return QuerySize();

    }

    return QuerySize() - temp;
}

 
INLINE
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::AddEmptyAttribute(
    IN ATTRIBUTE_TYPE_CODE Type,
    IN PCWSTRING           Name
    )
/*++

Routine Description:

    This method adds an empty, non-indexed, resident attribute of
    the specified type to the FRS.

Arguments:

    Type    --  Supplies the attribute's  type code.
    Name    --  Supplies the attribute's name.  May be NULL, in which
                case the attribute has no name.

Return Value:

    TRUE upon successful completion.

--*/
{
    NTFS_ATTRIBUTE Attribute;

    return( Attribute.Initialize( GetDrive(),
                                  QueryClusterFactor(),
                                  NULL,
                                  0,
                                  Type,
                                  Name,
                                  0 ) &&
            Attribute.InsertIntoFile( this, NULL ) );

}

INLINE
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::IsAttributeListPresent(
    )
/*++

Routine Description:

    This method checks for the presence of an attribute list.

Arguments:

    N/A

Return Value:

    TRUE if there is an attribute list.

--*/
{
    return (_AttributeList != NULL) ||
           IsAttributePresent($ATTRIBUTE_LIST, NULL, TRUE);
}

