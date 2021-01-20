#include "stdafx.h"

/*++

Module Name:

   frs.cxx

Abstract:

   This module contains the member function definitions for the
   NTFS_FILE_RECORD_SEGMENT class.  This class models File
   Record Segments in the NTFS Master File Table; it is the
   object through which a file's attributes may be accessed.

--*/

#include "ulib.hxx"

#include "untfs.hxx"
#include "list.hxx"
#include "numset.hxx"
#include "ifssys.hxx"

#include "drive.hxx"
#include "mft.hxx"
#include "mftfile.hxx"
#include "attrrec.hxx"
#include "attrib.hxx"
#include "attrlist.hxx"
#include "frs.hxx"
#include "badfile.hxx"
#include "ntfsbit.hxx"
#include "indxtree.hxx"


#include "indxroot.hxx"
#include "upcase.hxx"
#include "indxbuff.hxx"


DEFINE_CONSTRUCTOR(NTFS_FILE_RECORD_SEGMENT, NTFS_FRS_STRUCTURE);


 
NTFS_FILE_RECORD_SEGMENT::~NTFS_FILE_RECORD_SEGMENT (
         )
{
    Destroy();
}

VOID
NTFS_FILE_RECORD_SEGMENT::Construct (
   )
/*++

Routine Description:

   Worker method for NTFS_FILE_RECORD_SEGMENT construction.

Arguments:

   None.

Return Value:

   None.

--*/
{
    _Mft = NULL;
    _AttributeList = NULL;
    _ChildIterator = NULL;
}


inline
VOID
NTFS_FILE_RECORD_SEGMENT::Destroy2 (
    )
/*++

Routine Description:

   Worker method for NTFS_FILE_RECORD_SEGMENT partial destruction.

Arguments:

   None.

Return Value:

   None.

--*/
{
    // If the child iterator is not NULL, then the
    // list of children has been initialized.

    if( _ChildIterator != NULL ) {

        DELETE( _ChildIterator );
        _Children.DeleteAllMembers();
    }

    DELETE(_AttributeList);
    _AttributeList = NULL;
}

VOID
NTFS_FILE_RECORD_SEGMENT::Destroy (
   )
/*++

Routine Description:

   Worker method for NTFS_FILE_RECORD_SEGMENT destruction.

Arguments:

   None.

Return Value:

   None.

--*/
{
   _Mft = NULL;

   Destroy2();
}

 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::Initialize(
    IN      VCN                     FileNumber,
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft
   )
/*++

Routine Description:

   Initialize a File Record Segment object.  Note that this will not
   cause the FRS to be read.

Arguments:

   FileNumber     -- Supplies the FRS's cluster number within the MFT.
    Mft             -- Supplies the volume MasterFile Table.

Return Value:

   TRUE upon successful completion

Notes:

   This class is reinitializable.

--*/
{
   Destroy();

   DebugAssert(Mft);

   _Mft = Mft;

    if( !Mft->GetDataAttribute()    ||
        !_Mem.Initialize()          ||
        !_Children.Initialize()     ||
        (_ChildIterator = _Children.QueryIterator()) == NULL ||
        !NTFS_FRS_STRUCTURE::Initialize(&_Mem,
                                        Mft->GetDataAttribute(),
                                        FileNumber,
                                        Mft->QueryClusterFactor(),
                                        Mft->QueryVolumeSectors(),
                                        Mft->QueryFrsSize(),
                                        Mft->GetUpcaseTable() ) ) {
        Destroy();
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
NTFS_FILE_RECORD_SEGMENT::Initialize(
    IN      VCN                     FirstFileNumber,
    IN      ULONG                   FrsCount,
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft
   )
/*++

Routine Description:

   Initialize a File Record Segment object.  Note that this will not
   cause the FRS to be read.  This is meant to use together with
   Initialize(), ReadSet(), and ReadNext().

Arguments:

   FirstFileNumber - Supplies the first file number for this FRS block.
   FrsCount        - Supplies the number of FRS'es in this FRS block.
   Mft             - Supplies the volume MasterFile Table.

Return Value:

   TRUE upon successful completion

Notes:

   This class is reinitializable.

--*/
{
   Destroy();

   DebugAssert(Mft);

   _Mft = Mft;

   if( !Mft->GetDataAttribute()    ||
       !_Mem.Initialize()          ||
       !_Children.Initialize()     ||
       (_ChildIterator = _Children.QueryIterator()) == NULL ||
       !NTFS_FRS_STRUCTURE::Initialize(&_Mem,
                                       Mft->GetDataAttribute(),
                                       FirstFileNumber,
                                       FrsCount,
                                       Mft->QueryClusterFactor(),
                                       Mft->QueryVolumeSectors(),
                                       Mft->QueryFrsSize(),
                                       Mft->GetUpcaseTable() ) ) {
       Destroy();
       return FALSE;
   }

   return TRUE;
}


 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::Initialize(
   )
/*++

Routine Description:

   Partially initialize a File Record Segment object.  Note that this
   will not cause the FRS to be read.  This is meant to use together
   with SetFrsData() or ReadFrsData().

Arguments:

Return Value:

   TRUE upon successful completion

Notes:

   This class is reinitializable.

--*/
{
    Destroy2();

    if (!_Children.Initialize() ||
        (_ChildIterator = _Children.QueryIterator()) == NULL) {
        Destroy();
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
NTFS_FILE_RECORD_SEGMENT::Initialize(
    IN      VCN                 FileNumber,
    IN OUT  PNTFS_MFT_FILE      MftFile
    )
/*++

Routine Description:

    This routine initializes this class to a valid initial state.

Arguments:

    FileNumber  - Supplies the file number for this FRS.
    MftFile     - Supplies the MFT file.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

Notes:

    The upcase-table may be NULL; in this case, attributes with
    names cannot be manipulated until the upcase table is set.

--*/
{
    return Initialize(FileNumber,
                      MftFile->GetMasterFileTable());
}


 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::Initialize(
    IN OUT  PLOG_IO_DP_DRIVE        Drive,
    IN      LCN                     StartOfMft,
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft
   )
/*++

Routine Description:

   Initialize a File Record Segment object.  Note that this will not
   cause the FRS to be read.

Arguments:

    Drive           - Supplies the drive object.
    StartOfMft      - Supplies the starting cluster for the MFT.
    Mft             - Supplies the master file table.

Return Value:

   TRUE upon successful completion

Notes:

   This class is reinitializable.

--*/
{
    Destroy();

    DebugAssert(Mft);

    _Mft = Mft;
    _AttributeList = NULL;

    return _Children.Initialize() &&
           (_ChildIterator = _Children.QueryIterator()) != NULL &&
           _Mem.Initialize() &&
           NTFS_FRS_STRUCTURE::Initialize(&_Mem,
                                          Drive,
                                          StartOfMft,
                                          Mft->QueryClusterFactor(),
                                          Mft->QueryVolumeSectors(),
                                          Mft->QueryFrsSize(),
                                          Mft->GetUpcaseTable());
}

 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::Create (
    IN USHORT   Flags
   )
/*++

Routine Description:

   Create   (i.e. Format) a file record segment.  This is a private
   worker method which is called by the other Create methods.  It
   creates a File Record Segment which has no attribute records.

Arguments:

    Flags   --  Supplies the FILE_xxx flags which should be set in this
                File Record Set.  (Note that FILE_RECORD_SEGMENT_IN_USE
                will also be set, whether or not is is specified.)

Return value:

   TRUE upon successful completion.

--*/
{
    PATTRIBUTE_TYPE_CODE FirstAttribute;

    DebugPtrAssert( _FrsData );

    memset( _FrsData, 0, (UINT) QuerySize() );

    _FrsData->Lsn.LowPart = 0;
    _FrsData->Lsn.HighPart = 0;

    _FrsData->SequenceNumber = (USHORT) max(QueryFileNumber().GetLowPart(),1);
    _FrsData->ReferenceCount = 0;
    _FrsData->Flags = FILE_RECORD_SEGMENT_IN_USE | Flags;
    _FrsData->BytesAvailable = QuerySize();
    _FrsData->NextAttributeInstance = 0;

    memset( &_FrsData->BaseFileRecordSegment,
            0,
            sizeof(MFT_SEGMENT_REFERENCE) );


    // Write the 'FILE' signature in the MultiSectorHeader.

    memcpy( _FrsData->MultiSectorHeader.Signature,
            "FILE",
            4 );


    // Compute the number of Update Sequence Numbers in the
    // update array.  This number is (see ntos\inc\cache.h):
    //
    //      n/SEQUENCE_NUMBER_STRIDE + 1
    //
    // where n is the number of bytes in the protected structure
    // (in this case, a cluster).

    _FrsData->MultiSectorHeader.UpdateSequenceArraySize =
            (USHORT)(QuerySize()/SEQUENCE_NUMBER_STRIDE + 1);

    // The update sequence array starts at the field
    // UpdateArrayForCreateOnly.  (In other words, create locates
    // it using this field, all other methods locate it using
    // the offset that Create computes.)
    //
    _FrsData->MultiSectorHeader.UpdateSequenceArrayOffset =
        FIELD_OFFSET( FILE_RECORD_SEGMENT_HEADER,
                      UpdateArrayForCreateOnly );

    _FrsData->FirstAttributeOffset =
      QuadAlign( _FrsData->MultiSectorHeader.UpdateSequenceArrayOffset +
                   _FrsData->MultiSectorHeader.UpdateSequenceArraySize *
                        sizeof( UPDATE_SEQUENCE_NUMBER ) );

    _FrsData->SegmentNumberHighPart = (USHORT)QueryFileNumber().GetHighPart();
    _FrsData->SegmentNumberLowPart = QueryFileNumber().GetLowPart();

    // Make sure that the offset of the first attribute is in range:
    //
    if( _FrsData->FirstAttributeOffset + sizeof(ULONG) > QuerySize() ) {

        return FALSE;
    }

    // Put an END attribute at the first-attribute offset.  (Note
    // that this attribute doesn't have an attribute header, it just
    // consists of an Attribute Type Code of $END.)
    //
    FirstAttribute = (PATTRIBUTE_TYPE_CODE)
                     ((PBYTE)_FrsData +
                      _FrsData->FirstAttributeOffset);

    *FirstAttribute = $END;

    // The first free byte comes after the END attribute, which
    // consists of a single ATTRIBUTE_TYPE_CODE.
    //
    _FrsData->FirstFreeByte = _FrsData->FirstAttributeOffset +
                                QuadAlign( sizeof( ATTRIBUTE_TYPE_CODE ) );

    return TRUE;
}


 
 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::Create (
    IN  PCSTANDARD_INFORMATION  StandardInformation,
    IN  USHORT                  Flags
   )
/*++

Routine Description:

   Create   (i.e. Format) a base file record segment.  The base
   file record segment is the primary FRS for a file; it contains
   any resident attributes (and hence any indexed attributes) of the
   file and the External Attributes List, if any.

   Note that Create will not cause the FRS to be written, only to
   be formatted in memory.

Arguments:

   StandardInformation --  supplies the standard information for the
                     file record segment.
    Flags               --  Supplies the FILE_xxx flags which should
                            be set in this File Record Set.  (Note
                            that FILE_RECORD_SEGMENT_IN_USE will also
                            be set, whether or not is is specified.)


Return value:

   TRUE upon successful completion.

--*/
{
   NTFS_ATTRIBUTE StandardInfoAttribute;

    DebugPtrAssert( _FrsData );

    if( Create( Flags ) &&
      StandardInfoAttribute.Initialize( GetDrive(),
                                          QueryClusterFactor(),
                                          StandardInformation,
                                sizeof( STANDARD_INFORMATION ),
                                $STANDARD_INFORMATION ) &&
        StandardInfoAttribute.InsertIntoFile( this, NULL ) ) {

        return TRUE;

   } else {

        return FALSE;
    }
}

 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::Create (
    IN PCMFT_SEGMENT_REFERENCE  BaseSegment,
    IN USHORT                   Flags
   )
/*++

Routine Description:

   Create (i.e. Format) a secondary file record segment.  A secondary
   file record segment contains external attributes; it is referenced
   in the file's External Attributes List, which is in the file's
   base file record segment.

Arguments:

    BaseSegment         --  supplies a reference to the base file
                     record segment for this file.
    Flags               --  Supplies the FILE_xxx flags which should
                            be set in this File Record Set.  (Note
                            that FILE_RECORD_SEGMENT_IN_USE will also
                            be set, whether or not is is specified.)

Return value:

   TRUE upon successful completion.

--*/
{
   DebugPtrAssert( _FrsData );

    if( Create( Flags ) ) {

        memcpy( &_FrsData->BaseFileRecordSegment,
                BaseSegment,
                sizeof(MFT_SEGMENT_REFERENCE) );

      return TRUE;

   } else {

      return FALSE;
   }
}


 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::AddDataAttribute(
    IN     ULONG        InitialSize,
    IN OUT PNTFS_BITMAP VolumeBitmap,
    IN     BOOLEAN      Fill,
    IN     CHAR         FillCharacter
    )
/*++

Routine Description:

    This method adds a nonresident data attribute to the File Record Segment.

Arguments:

    InitialSize     --  Supplies the number of bytes to allocate for
                        the data attribute.
    VolumeBitmap    --  Supplies the volume bitmap.
    Fill            --  Supplies a flag which, if TRUE, indicates that
                        the value of the attribute should be set to
                        the supplied fill character.
    FillCharacter   --  Supplies a fill character.

Return Value:

    TRUE upon successful completion.

--*/
{
    NTFS_ATTRIBUTE   DataAttribute;
    NTFS_EXTENT_LIST Extents;

    // Initialize the data attribute [type is $DATA, no name] with an
    // empty extent list, and then resize it to the desired size.

    if( !Extents.Initialize( 0, 0 ) ||
        !DataAttribute.Initialize( GetDrive(),
                                   QueryClusterFactor(),
                                   &Extents,
                                   0,
                                   0,
                                   $DATA,
                                   NULL ) ||
        !DataAttribute.Resize( InitialSize, VolumeBitmap ) ) {

        return FALSE;
    }

    if( Fill &&
        !DataAttribute.Fill( 0, FillCharacter ) ) {

        return FALSE;
    }

    // Insert the data attribute into this File Record Segment.  If that
    // operation fails, we need to free up the space allocated by
    // resizing the attribute back to zero.

    if( !DataAttribute.InsertIntoFile( this, VolumeBitmap ) ) {

        DataAttribute.Resize( 0, VolumeBitmap );
        return FALSE;
    }

    return TRUE;
}


 
 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::AddFileNameAttribute(
    IN PFILE_NAME   FileName
    )
/*++

Routine Description:

    This method adds a File Name attribute to the File Record Segment.
    Note that it assumes that the File Name is indexed.

Arguments:

    FileName    --  Supplies the value of the File Name attribute, which
                    is a FILE_NAME structure.

Return Value:

    TRUE upon successful completion.

--*/
{
    NTFS_ATTRIBUTE FileNameAttribute;

    // Initialize a file-name attribute and insert it into this FRS.

    if( FileNameAttribute.Initialize( GetDrive(),
                                      QueryClusterFactor(),
                                      FileName,
                                      NtfsFileNameGetLength( FileName ),
                                      $FILE_NAME ) ) {

        FileNameAttribute.SetIsIndexed();

        return( FileNameAttribute.InsertIntoFile( this, NULL ) );

    } else {

        return FALSE;
    }
}

 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::AddAttribute(
    IN ATTRIBUTE_TYPE_CODE  Type,
    IN PCWSTRING            Name,
    IN PCVOID               Value,
    IN ULONG                Length,
    IN OUT PNTFS_BITMAP     Bitmap,
    IN BOOLEAN              IsIndexed
    )
/*++

Routine Description:

    This method adds an attribute of the specified type to the file.

Arguments:

    Type        --  Supplies the type of the attribute.
    Name        --  Supplies the name of the attribute--may be NULL,
                    which is interpreted as no name.
    Value       --  Supplies a pointer to the attribute's value.  May
                    be NULL if the attribute value length is zero.
    Length      --  Supplies the length of the attribute value.
    Bitmap      --  Supplies the volume bitmap.  May be NULL,
                    in which case the attribute cannot be made
                    nonresident.
    IsIndexed   --  Supplies a flag which indicates whether the
                    attribute is indexed.

Return Value:

    TRUE upon successful completion.

--*/
{
    NTFS_ATTRIBUTE Attribute;

    // Initialize a resident attribute with the desired
    // characteristics.  If it is to be indexed, mark
    // it as such.
    //
    if( !Attribute.Initialize( GetDrive(),
                               QueryClusterFactor(),
                               Value,
                               Length,
                               Type,
                               Name ) ) {

        return FALSE;
    }

    if( IsIndexed ) {

        Attribute.SetIsIndexed();
    }

    // Insert the attribute into the file.  If it cannot be inserted
    // as a resident attribute, make it non-resident and try again.
    //
    if( Attribute.InsertIntoFile( this, NULL ) ) {

        // Success!
        //
        return TRUE;
    }

    // Couldn't insert it as a resident attribute; if it isn't
    // indexed, make it nonresident and try again.  Note that
    // we can't make it nonresident if the client did not provide
    // a bitmap, and that indexed attributes cannot be made
    // nonresident.
    //
    if( !IsIndexed &&
        Bitmap != NULL &&
        Attribute.MakeNonresident( Bitmap ) &&
        Attribute.InsertIntoFile( this, Bitmap ) ) {

        // Second time lucky.
        //
        return TRUE;
    }

    // Can't insert this attribute into this FRS.  If the attribute
    // is nonresident, truncate it to zero length to free up the
    // space allocated to it.
    //
    if( !Attribute.IsResident() ) {

        DebugPtrAssert( Bitmap );
        Attribute.Resize( 0, Bitmap );
    }

    // return failure.
    //
    return FALSE;
}
 
 

 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::IsAttributePresent (
   IN  ATTRIBUTE_TYPE_CODE Type,
   IN  PCWSTRING           Name,
   IN  BOOLEAN             IgnoreExternal
   )
/*++

Routine Description:

   This function determines whether a specified attribute is present
   in the attributes associated with the File Record Segment.

Arguments:

   Type         -- supplies the type code of the attribute in question.
   Name         -- supplies the name of the attribute in question.
                  (may be NULL, in which case the attribute has no name.)
   IgnoreExternal -- supplies a flag indicating that the FRS should
               not look for external attributes.

Return Value:

   TRUE if the File Record Segment has an attribute which matches
   the type (and name, if given).

Notes:

   This method assumes that the file record segment is consistent,
   and that it has been read.

   Note that we can determine what attribute records are present,
   and whether they are unique, without reading the child FRS's,
   since the information we need is in the Attribute List.

   The NoExternal flag is provided mainly to allow us to check for
   the presence of the ATTRIBUTES_LIST attribute without falling
   into infinite recursion.

--*/
{
   ULONG CurrentRecordOffset;
   NTFS_ATTRIBUTE_RECORD CurrentRecord;
   NTFS_ATTRIBUTE AttributeList;
   BOOLEAN Found = FALSE;

    DebugPtrAssert( _FrsData );

    if( !IgnoreExternal &&
        Type != $ATTRIBUTE_LIST &&
        (_AttributeList != NULL ||
         IsAttributePresent( $ATTRIBUTE_LIST, NULL, TRUE )) ) {

      // This File Record Segment has an ATTRIBUTE_LIST attribute,
      // and the caller wants to include external attributes, so
      // we can traverse that list.

        if( !SetupAttributeList() ) {

            return FALSE;
        }

        return( _AttributeList->IsInList( Type, Name ) );

    } else {

      // Either the caller has asked us to ignore external
        // attributes or there is no ATTRIBUTE_LIST attribute,
        // or we're looking for the attribute list itself,
      // so we'll go through the list of attribute records
      // in this File Record Segment.

      CurrentRecordOffset = _FrsData->FirstAttributeOffset;

      while( CurrentRecordOffset < QuerySize() &&
             CurrentRecord.Initialize( GetDrive(),
                                       (PBYTE)_FrsData + CurrentRecordOffset,
                                       QuerySize() - CurrentRecordOffset ) &&
             CurrentRecord.QueryTypeCode() != $END ) {

         if( CurrentRecord.IsMatch( Type, Name ) ) {

                Found = TRUE;
                break;
         }

            // If this record has a zero length, then this FRS
            // is corrupt.  Otherwise, just go on to the next
            // attribute record.
            //
            if( CurrentRecord.QueryRecordLength() == 0 ) {
                Found = FALSE;
                break;
            }

         CurrentRecordOffset += CurrentRecord.QueryRecordLength();
      }
   }

   return Found;
}

 
 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::QueryAttribute (
    OUT PNTFS_ATTRIBUTE     Attribute,
    OUT PBOOLEAN            Error,
   IN  ATTRIBUTE_TYPE_CODE  Type,
   IN  PCWSTRING            Name
   )
/*++

Routine Description:

   This function fetches an attribute associated with the
   File Record Segment.

   This method cannot be used to fetch the ATTRIBUTE_LIST attribute.

Arguments:

    Attribute   --  Receives (ie. is initialized to) the attribute.  Note
                    that this parameter may be uninitialized on entry, and
                    may be left uninitialized if this method fails.
    Error       --  Receives TRUE if the method fails because of an error.
    Type        --  Supplies the type of the desired attribute
    Name        --  Supplies the name of the desired attribute (NULL if
                    the attribute has no name).

Return Value:

    TRUE upon successful completion.

Notes:

    If the method returns TRUE, *Error should be ignored.  If it
    returns FALSE, *Error will be set to TRUE if the failure resulted
    from an error (out of memory, corrupt structure); otherwise, the
    caller may assume that the attribute is not present.

    This method will check both internal and external attributes,
   reading child File Record Segments as necessary to access their
   attribute records.

--*/
{
    MFT_SEGMENT_REFERENCE   SegmentReference;
    NTFS_ATTRIBUTE_RECORD   Record;
    PNTFS_FILE_RECORD_SEGMENT ChildFrs = NULL;
    VCN                     TargetFileNumber;
    ULONG                   Index;
    ATTRIBUTE_TYPE_CODE     FetchType;
    VCN                     LowestVcn;
    DSTRING                 FetchName;
    USHORT                  Instance;
    PNTFS_EXTENT_LIST       backup_extent_list = NULL;

    DebugPtrAssert( Attribute );
    DebugPtrAssert( Error );

    DebugPtrAssert( _FrsData );

    // Assume innocent until proven guilty:

    *Error = FALSE;

   // This method cannot be used to fetch the ATTRIBUTE_LIST
   // attribute.

   if( Type == $ATTRIBUTE_LIST ) {

        *Error = TRUE;
        return FALSE;
   }

    if( !IsAttributePresent( Type, Name, FALSE ) ) {

      // there is no matching attribute.

        return FALSE;
    }

    // Now that we've determined that the attribute is present,
    // this method can only fail because of an error.

    *Error = TRUE;


    if( !SetupAttributeList() ) {

        return FALSE;
    }


    // Get the TargetFileNumber.

    if ( _AttributeList ) {

        if (!_AttributeList->QueryExternalReference( Type,
                                                     &SegmentReference,
                                                     &Index,
                                                     Name )) {

            return FALSE;
        }

        // We've found the first entry in the Attribute List
        // for this attribute.  We'll use that entry to
        // initialize the attribute object.  But first,
        // we have to find it...

        TargetFileNumber.Set( SegmentReference.LowPart,
                              (LONG) SegmentReference.HighPart );

    } else {

        TargetFileNumber = QueryFileNumber();
    }


    // Get the first attribute record.

    if ( TargetFileNumber == QueryFileNumber() ) {

        if (!QueryAttributeRecord(&Record, Type, Name)) {

            return FALSE;
        }

    } else {

        // The record we want is in a child record segment.
        // Fetch the child.  (Note that SetupChild will construct
        // an FRS for the child and read it, if it's not already
        // in the list of children.)

        if( (ChildFrs = SetupChild( TargetFileNumber )) == NULL ) {

            return FALSE;
        }

        if (!ChildFrs->QueryAttributeRecord(&Record, Type, Name)) {

            return FALSE;
        }
    }


    // Initialize the Attribute with the first attribute record.

    if ( !Attribute->Initialize( GetDrive(),
                                 QueryClusterFactor(),
                                 &Record) ) {
        return FALSE;
    }

    if( Attribute->IsResident() ) {

        // A resident attribute can only have one attribute record;
        // since we've found it, we can return now.
        //
        *Error = FALSE;
        return TRUE;
    }

    // Add any other attribute records to the attribute.


    if (_AttributeList) {

        ATTR_LIST_CURR_ENTRY    Entry;
        ULONG                   i = 0;

        Entry.CurrentEntry = NULL;
        while (_AttributeList->QueryNextEntry(&Entry,
                                              &FetchType,
                                              &LowestVcn,
                                              &SegmentReference,
                                              &Instance,
                                              &FetchName)) {
            if (i++ == Index) {
                while (_AttributeList->QueryNextEntry(&Entry,
                                                      &FetchType,
                                                      &LowestVcn,
                                                      &SegmentReference,
                                                      &Instance,
                                                      &FetchName) &&
                       FetchType == Type &&
                       ((!Name && !FetchName.QueryChCount()) ||
                        (Name && !Name->Strcmp(&FetchName)))) {

                    TargetFileNumber.Set( SegmentReference.LowPart,
                                          (LONG) SegmentReference.HighPart );

                    // Get attribute record from file record segment.

                    if ( TargetFileNumber == QueryFileNumber() ) {

                        if (!QueryAttributeRecord(&Record, Type, Name)) {

                            DELETE(backup_extent_list);

                            return FALSE;
                        }

                    } else {

                        // The record we want is in a child record segment.
                        // Fetch the child.  (Note that SetupChild will construct
                        // an FRS for the child and read it, if it's not already
                        // in the list of children.)
                        //
                        if( (ChildFrs = SetupChild( TargetFileNumber )) == NULL ) {

                            DELETE(backup_extent_list);

                            return FALSE;

                        }

                        if (!ChildFrs->QueryAttributeRecord(&Record, Type, Name)) {

                            DELETE(backup_extent_list);

                            return FALSE;
                        }
                    }


                    // Add attribute record to attribute.

                    if (!Attribute->AddAttributeRecord(&Record, &backup_extent_list)) {

                        DELETE(backup_extent_list);
                        DebugAbort("Couldn't do an Add attribute record.");
                        return FALSE;
                    }
                }
                break;
            }
        }
    }

    *Error = FALSE;
    DELETE(backup_extent_list);
    return TRUE;
}

 
 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::QueryFileSizes (
    OUT PBIG_INT            AllocatedLength,
    OUT PBIG_INT            FileSize,
    OUT PBOOLEAN            Error
   )
/*++

Routine Description:

   This function fetches the allocated length and file size of a
   file associated with a base File Record Segment.

Arguments:

    AllocatedLength -- receives the allocated length of the file.
    FileSize        -- receives the valid length of the file.
    Error           -- receives TRUE if the method fails because of an error.

Return Value:

    TRUE upon successful completion.

Notes:

    If the method returns TRUE, *Error should be ignored.  If it
    returns FALSE, *Error will be set to TRUE if the failure resulted
    from an error (out of memory, corrupt structure); otherwise, the
    caller may assume that the attribute is not present.

    This method will check both internal and external attributes,
   reading child File Record Segments as necessary to access their
   attribute records.

    Please note that the file size and allocated length of the file
   is entire in the base frs or the first child frs.  There is no
   need to read in all the child frs in order to compute the desired
   values.

--*/
{
    NTFS_ATTRIBUTE Attribute;
    BIG_INT TotalAllocated;
    MFT_SEGMENT_REFERENCE SegmentReference;
    NTFS_ATTRIBUTE_RECORD Record;
    PNTFS_FILE_RECORD_SEGMENT ChildFrs = NULL;
    VCN TargetFileNumber;
    ULONG Index;
    VCN LowestVcn;
    DSTRING FetchName;

    DebugPtrAssert( Error );

    DebugPtrAssert( _FrsData );

    // Assume innocent until proven guilty:

    *Error = FALSE;

    // This method cannot be used to fetch the ATTRIBUTE_LIST
    // attribute.

    if( !IsAttributePresent( $DATA, NULL, FALSE ) ) {

        // there is no matching attribute.

        return FALSE;
    }

    // Now that we've determined that the attribute is present,
    // this method can only fail because of an error.

    *Error = TRUE;


    if( !SetupAttributeList() ) {

        return FALSE;
    }


    // Get the TargetFileNumber.

    if ( _AttributeList ) {

        if (!_AttributeList->QueryExternalReference( $DATA,
                                                     &SegmentReference,
                                                     &Index,
                                                     NULL )) {

            return FALSE;
        }

        // We've found the first entry in the Attribute List
        // for this attribute.  We'll use that entry to
        // initialize the attribute object.  But first,
        // we have to find it...

        TargetFileNumber.Set( SegmentReference.LowPart,
                              (LONG) SegmentReference.HighPart );

    } else {

        TargetFileNumber = QueryFileNumber();
    }


    // Get the first attribute record.

    if ( TargetFileNumber == QueryFileNumber() ) {

        if (!QueryAttributeRecord(&Record, $DATA, NULL)) {

            return FALSE;
        }

    } else {

        // The record we want is in a child record segment.
        // Fetch the child.  (Note that SetupChild will construct
        // an FRS for the child and read it, if it's not already
        // in the list of children.)

        if( (ChildFrs = SetupChild( TargetFileNumber )) == NULL ) {

            return FALSE;
        }

        if (!ChildFrs->QueryAttributeRecord(&Record, $DATA, NULL)) {

            return FALSE;
        }
    }

    Record.QueryValueLength(FileSize,
                            AllocatedLength,
                            NULL,
                            &TotalAllocated);

    // for uncompressed file, the total allocated length does
    // not exist

    if (Record.IsResident()) {

        *AllocatedLength = QuadAlign(AllocatedLength->GetLowPart());

    } else if((Record.QueryFlags() & (ATTRIBUTE_FLAG_COMPRESSION_MASK |
                                      ATTRIBUTE_FLAG_SPARSE))) {

        // for compressed file, the allocated length is the
        // total allocated length
        *AllocatedLength = TotalAllocated;

    }

    *Error = FALSE;
    return TRUE;
}

 
 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::QueryAttributeByOrdinal (
    OUT PNTFS_ATTRIBUTE     Attribute,
    OUT PBOOLEAN            Error,
    IN  ATTRIBUTE_TYPE_CODE Type,
    IN  ULONG               Ordinal
    )
/*++

Routine Description:

    This method returns the n-th attribute of the specified
    type.  Note that it ignores attribute names (but, of course,
    the client can query the name of the returned attribute from
    that attribute).

Arguments:

    Attribute   --  Receives (ie. is initialized to) the attribute.  Note
                    that this parameter may be uninitialized on entry, and
                    may be left uninitialized if this method fails.
    Error       --  Receives TRUE if the method fails because of an error.
    Type        --  Supplies the type of the desired attribute
    Ordinal     --  Supplies the (zero-based) ordinal number of the
                    attribute to return.

Return Value:

    TRUE upon successful completion.

    If this method succeeds, the client should ignore *Error.

    If this method fails because of error, *Error will be set; if
    this method returns FALSE and *Error is FALSE, the client may
    assume that there is no such matching attribute.

--*/
{
    NTFS_ATTRIBUTE_RECORD CurrentRecord;
    MFT_SEGMENT_REFERENCE SegmentReference, NextSegmentReference;
    DSTRING Name;
    VCN LowestVcn;
    VCN TargetFileNumber;
    ATTRIBUTE_TYPE_CODE CurrentType;

    PVOID CurrentRecordData;
    PNTFS_FILE_RECORD_SEGMENT TargetFrs = NULL;

    USHORT InstanceTag, NextInstanceTag;


    DebugPtrAssert( Attribute );
    DebugPtrAssert( Error );

    // If the attribute list is present, force it into memory.
    //
    if( !SetupAttributeList() ) {

        *Error = TRUE;
        return FALSE;
    }


    if( _AttributeList != NULL ) {

        ATTR_LIST_CURR_ENTRY    Entry;

        // Spin through the attribute list until we find the nth
        // entry with the requested type and a LowestVcn of zero.
        //
        Entry.CurrentEntry = NULL;
        while( TRUE ) {

            if( !_AttributeList->QueryNextEntry( &Entry,
                                                 &CurrentType,
                                                 &LowestVcn,
                                                 &SegmentReference,
                                                 &InstanceTag,
                                                 &Name ) ) {

                // Out of entries in the attribute list, so there
                // is no matching attribute type.

                *Error = FALSE;
                return FALSE;
            }

            if( CurrentType == Type && LowestVcn == 0 ) {

                // This entry has the desired type code;  check
                // to see if we want to skip it or grab it.
                //
                if( Ordinal == 0 ) {

                    // Found the entry we want.
                    //
                    break;

                } else {

                    // Skip this one.
                    //
                    Ordinal--;
                }
            }
        }

        // Now we have an entry for the attribute we want.  If there's
        // only one entry for this attribute, we can initialize an
        // attribute with that record and return; if there are multiple
        //
        // If the next entry does not have a LowestVcn of zero, it
        // is another entry for this attribute.  Note that we need
        // SegmentReference and Instance tag for later use.
        //

        if( _AttributeList->QueryNextEntry( &Entry,
                                            &CurrentType,
                                            &LowestVcn,
                                            &NextSegmentReference,
                                            &NextInstanceTag,
                                            &Name ) &&
            CurrentType == Type &&
            LowestVcn != 0 ) {

            // This is a multi-record attribute, which means it is
            // uniquely identified by Type and Name.
            //
            return( QueryAttribute( Attribute,
                                    Error,
                                    Type,
                                    &Name ) );

        } else {

            // There are no more entries for this attribute,
            // so we can initialize an the attribute with
            // this record.  We'll let QueryAttributeByTag
            // do the work for us.
            //
            TargetFileNumber.Set( SegmentReference.LowPart,
                                  (LONG) SegmentReference.HighPart );

            if( TargetFileNumber == QueryFileNumber() ) {

                // The record is in this FRS.
                //
                TargetFrs = this;

            } else {

                // The record we want is in a child FRS; get that
                // child and squeeze the attribute out of it.
                //
                if( (TargetFrs = SetupChild( TargetFileNumber)) == NULL ) {

                    // Something is wrong--we can't get the child.
                    //

                    *Error = TRUE;
                    return FALSE;
                }
            }


            if( !TargetFrs->QueryAttributeByTag( Attribute,
                                                 Error,
                                                 InstanceTag ) ) {

                // We know the attribute is there, but we can't
                // get it.
                //
                *Error = TRUE;
                return FALSE;

            } else {

                return TRUE;
            }
        }

    } else {

        // This File Record Segment does not have an attribute list,
        // so we only have to grope through this FRS.

        // First, skip over attribute records with a type-code
        // less than the one we're looking for:

        CurrentRecordData = NULL;
        *Error = FALSE;

        do {

            if( (CurrentRecordData =
                  GetNextAttributeRecord( CurrentRecordData,
                                          NULL,
                                          Error )) == NULL ||
                *Error ) {

                // No more, or an error was found.
                return FALSE;
            }

            if( !CurrentRecord.Initialize( GetDrive(), CurrentRecordData ) ) {

                *Error = TRUE;
                return FALSE;
            }

        } while( CurrentRecord.QueryTypeCode() != $END &&
                 CurrentRecord.QueryTypeCode() < Type );

        if( CurrentRecord.QueryTypeCode() == $END ||
            CurrentRecord.QueryTypeCode() > Type ) {

            // There are no attributes of the specified type in
            // this FRS.  Note that *Error has already been set
            // to FALSE.

            return FALSE;
        }

        // Now step through the attributes of this type code to
        // find the one we want.

        while( Ordinal != 0 ) {

            // In determining the ordinal of an attribute, we only
            // count attribute records which are resident or have
            // a LowestVcn of zero.

            if( CurrentRecord.IsResident() ||
                CurrentRecord.QueryLowestVcn() == 0 ) {

                Ordinal--;
            }

            if( (CurrentRecordData =
                 GetNextAttributeRecord( CurrentRecordData,
                                         NULL,
                                         Error )) == NULL ||
                *Error ) {

                // No more, or an error was found.
                return FALSE;
            }

            if( !CurrentRecord.Initialize( GetDrive(), CurrentRecordData ) ) {

                *Error = TRUE;
                return FALSE;
            }

            if( CurrentRecord.QueryTypeCode() == $END ||
                CurrentRecord.QueryTypeCode() > Type ) {

                // Ran out of matching records.

                return FALSE;
            }
        }

        // We've found our baby.  Initialize the attribute and return.

        if( CurrentRecord.IsResident() ) {

            // Since this attribute record is resident, it is
            // the only attribute record for this attribute, and
            // so it suffices to initialize the attribute with
            // this record.

            if( Attribute->Initialize( GetDrive(),
                                       QueryClusterFactor(),
                                       &CurrentRecord ) ) {

                // Everything is just fine.

                return TRUE;

            } else {

                // Foiled at the last minute by some dastardly error.

                *Error = TRUE;
                return FALSE;
            }

        } else {

            // Since there may be other attribute records associated
            // with this attribute, we have to invoke QueryAttribute
            // to do our work for us.  Get the name from the attribute
            // record, and then query this FRS for the attribute with
            // the requested type and that name.

            if( !CurrentRecord.QueryName( &Name ) ) {

                *Error = TRUE;
                return FALSE;
            }

            return( QueryAttribute( Attribute,
                                    Error,
                                    Type,
                                    &Name ) );
        }

    }
}


 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::QueryAttributeByTag (
    OUT PNTFS_ATTRIBUTE     Attribute,
    OUT PBOOLEAN            Error,
    IN  ULONG               Tag
    )
/*++

Routine Description:

    This method initializes an attribute based on the single attribute
    record with the specified instance tag.  Note that it only examines
    records in this File Record Segment; it does not look at child FRS's.

Arguments:

    Attribute   --  Receives (ie. is initialized to) the attribute
                    in question.  Note that this parameter may be
                    uninitialized on entry, and may be left in that
                    state if this method fails.
    Error       --  Receives TRUE if the method fails because of
                    an error.
    Tag         --  Supplies the attribute record instance tag of
                    the desired record.

--*/
{
    NTFS_ATTRIBUTE_RECORD CurrentRecord;
    PVOID CurrentRecordData = NULL;
    BOOLEAN Found = FALSE;


    while( !Found ) {

        CurrentRecordData = GetNextAttributeRecord( CurrentRecordData );

        if( CurrentRecordData == NULL ) {

            // No more records.
            //
            *Error = FALSE;
            return FALSE;
        }

        if( !CurrentRecord.Initialize( GetDrive(), CurrentRecordData ) ) {

            // Error initializing object.
            //
            *Error = TRUE;
            return FALSE;
        }

        if( CurrentRecord.QueryInstanceTag() == Tag ) {

            Found = TRUE;
        }
    }

    if( !Attribute->Initialize( GetDrive(),
                                QueryClusterFactor(),
                                &CurrentRecord ) ) {

        *Error = TRUE;
        return FALSE;
    }

    return TRUE;
}


 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::PurgeAttribute (
   IN  ATTRIBUTE_TYPE_CODE Type,
   IN  PCWSTRING           Name,
    IN  BOOLEAN             IgnoreExternal
   )
/*++

Routine Description:

   This method removes all attribute records for the given attribute
   type and name from the File Record Segment and its children.

Arguments:

   Type  -- supplies the type of the attribute to purge
   Name  -- supplies the name of the attribute to purge (NULL if
            the attribute has no name).
    IgnoreExternal -- supplies a flag that, if TRUE, indicates we
                      should ignore the attribute list and only delete
                      matching records found in this File Record Segment

Return Value:

   TRUE upon successful completion.

--*/
{
   NTFS_ATTRIBUTE_RECORD CurrentRecord;
    MFT_SEGMENT_REFERENCE SegmentReference;
    PNTFS_FILE_RECORD_SEGMENT ChildFrs;
    VCN TargetFileNumber;
    ULONG EntryIndex;
   ULONG CurrentRecordOffset;
   ULONG NextRecordOffset;

   if( !IgnoreExternal &&
        Type != $ATTRIBUTE_LIST &&
        (_AttributeList != NULL ||
       IsAttributePresent($ATTRIBUTE_LIST, NULL, TRUE) ) ) {

        // This File Record Segment has an attribute list, so
        // we should consult it.

        if( !SetupAttributeList() ) {

            return FALSE;
        }


        while( _AttributeList->QueryExternalReference( Type,
                                                       &SegmentReference,
                                                       &EntryIndex,
                                                       Name ) ) {

            TargetFileNumber.Set( SegmentReference.LowPart,
                                (LONG) SegmentReference.HighPart );

            if( TargetFileNumber == QueryFileNumber() ) {

                // The record we want to delete is in this File
                // Record Segment.  The easiest way to get at it
                // is to recurse back into this function with
                // IgnoreExternal equal to TRUE (which prevents
                // further recursion).

                PurgeAttribute( Type, Name, TRUE );

            } else {

                // The record we want to delete is in a child record
                // segment.  Note that SetupChild will construct and
                // read a new FRS if the child isn't already in the list.

                if( (ChildFrs = SetupChild( TargetFileNumber )) == NULL ) {

                    return FALSE;
                }

                // Now we've got the child; purge any matching attributes
                // from it.  (It's OK if we purge more than one; we'll
                // catch up on later iterations.)

                if( !ChildFrs->PurgeAttribute( Type, Name ) ) {

                    return FALSE;
                }
            }

            _AttributeList->DeleteEntry( EntryIndex );
        }

   } else {

      // Either there is no Attribute List, or we're deleting
      // the Attribute List itself, or we've been instructed to
        // ignore it, so we can just go through this File Record
        // Segment and blow away any matching records we find.

        CurrentRecordOffset = _FrsData->FirstAttributeOffset;

      while( CurrentRecordOffset < QuerySize() &&
             CurrentRecord.Initialize( GetDrive(),
                                       (PBYTE)_FrsData + CurrentRecordOffset,
                                       QuerySize() - CurrentRecordOffset ) &&
              CurrentRecord.QueryTypeCode() != $END ) {

            if( CurrentRecord.QueryRecordLength() == 0 ) {

                return FALSE;
            }

         if( CurrentRecord.IsMatch( Type, Name ) ) {

            // This record matches, so away it goes!

            NextRecordOffset =CurrentRecordOffset +
                           CurrentRecord.QueryRecordLength();

            DebugAssert( NextRecordOffset < QuerySize() );

                _FrsData->FirstFreeByte -= CurrentRecord.QueryRecordLength();

            memmove( (PBYTE)_FrsData + CurrentRecordOffset,
                   (PBYTE)_FrsData + NextRecordOffset,
                   (UINT) (QuerySize() - NextRecordOffset) );



            // Note that, since we've brought the next record to
            // CurrentRecordOffset, there's no need to adjust
            // CurrentRecordOffset.

         } else {

            // This record doesn't match, so we won't purge it.

            CurrentRecordOffset += CurrentRecord.QueryRecordLength();
         }
      }
   }

   return TRUE;
}

 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::DeleteResidentAttribute(
    IN  ATTRIBUTE_TYPE_CODE Type,
    IN  PCWSTRING           Name,
    IN  PCVOID              Value,
    IN  ULONG               ValueLength,
    OUT PBOOLEAN            Deleted,
    IN  BOOLEAN             IgnoreExternal
    )
/*++

Routine Description:

    This method will delete any attribute record associated with the
    File Record Segment which represents a resident attribute of the
    specified type and name with a value equal to the supplied value.

Arguments:

    Type            --  Supplies the attribute type code.
    Name            --  Supplies the attribute name.  May be NULL, which
                        indicates that the attribute has no name.
    Value           --  Supplies the value of the attribute to delete.
    ValueLength     --  Supplies the length of the value.
    IgnoreExternal  --  Supplies a flag which indicates, if TRUE,
                        that this method should only examine records
                        in this FRS (ie. it should ignore external
                        attributes).
    Deleted         --  Receives TRUE if the method found and deleted
                        a matching record.

Return Value:

    TRUE upon successful completion.

--*/
{
    DSTRING CurrentName;
    NTFS_ATTRIBUTE_RECORD CurrentRecord;
    MFT_SEGMENT_REFERENCE SegmentReference, FoundSegmentReference;

    PNTFS_FILE_RECORD_SEGMENT ChildFrs;

    ATTRIBUTE_TYPE_CODE CurrentType;
    VCN LowestVcn, TargetFileNumber;
    USHORT Instance, FoundInstance;
    BOOLEAN IsIndexed;

    DebugPtrAssert( Value );
    DebugPtrAssert( Deleted );

    *Deleted = FALSE;

    if( !SetupAttributeList() ) {

        return FALSE;
    }

    if( !IgnoreExternal &&
        _AttributeList != NULL ) {

        // First, traverse the list to force all the children into
        // memory.
        //

        ATTR_LIST_CURR_ENTRY        Entry;

        Entry.CurrentEntry = NULL;
        while (_AttributeList->QueryNextEntry( &Entry,
                                               &CurrentType,
                                               &LowestVcn,
                                               &SegmentReference,
                                               &Instance,
                                               &CurrentName )) {

            TargetFileNumber.Set( SegmentReference.LowPart,
                                  (LONG)SegmentReference.HighPart );

            if( TargetFileNumber != QueryFileNumber() &&
                SetupChild( TargetFileNumber ) == NULL ) {

                return FALSE;
            }
        }

        // Now traverse the list of children, trying to delete
        // the record in question.
        //
        _ChildIterator->Reset();

        while( (ChildFrs = (PNTFS_FILE_RECORD_SEGMENT)
                           _ChildIterator->GetNext()) != NULL ) {

            if( !ChildFrs->DeleteResidentAttributeLocal( Type,
                                                         Name,
                                                         Value,
                                                         ValueLength,
                                                         Deleted,
                                                         &IsIndexed,
                                                         &Instance ) ) {

                return FALSE;
            }

            if( *Deleted ) {

                // We found our victim.  Remember the segment reference
                // and instance tag, so we can delete the attribute list
                // entry.
                //
                FoundSegmentReference = ChildFrs->QuerySegmentReference();
                FoundInstance = Instance;

                break;
            }
        }

        if( !*Deleted ) {

            // We didn't find the target attribute record in any
            // of the children; see if it's in this FRS itself.
            //
            if( !DeleteResidentAttributeLocal( Type,
                                               Name,
                                               Value,
                                               ValueLength,
                                               Deleted,
                                               &IsIndexed,
                                               &Instance ) ) {

                return FALSE;
            }

            if( *Deleted ) {

                // Found it in this FRS.
                //
                FoundSegmentReference = QuerySegmentReference();
                FoundInstance = Instance;
            }
        }

        if( *Deleted ) {

            // We found and deleted a matching attribute record.
            // Find the corresponding entry in the attribute list
            // and delete it.  Note that the Segment Reference and
            // Instance Tag are sufficient to identify that entry.
            //

            ATTR_LIST_CURR_ENTRY    Entry;

            Entry.CurrentEntry = NULL;
            while (_AttributeList->QueryNextEntry( &Entry,
                                                   &CurrentType,
                                                   &LowestVcn,
                                                   &SegmentReference,
                                                   &Instance,
                                                   &CurrentName )) {

                if( SegmentReference == FoundSegmentReference &&
                    Instance == FoundInstance ) {

                    _AttributeList->DeleteCurrentEntry( &Entry );
                    break;
                }
            }
        }

    } else {

        // This FRS does not have an attribute list (or the client
        // wants to ignore it), so we just have to examine the records
        // in this FRS.
        //
        if( !DeleteResidentAttributeLocal( Type,
                                           Name,
                                           Value,
                                           ValueLength,
                                           Deleted,
                                           &IsIndexed,
                                           &Instance ) ) {

            return FALSE;
        }

    }

    // If we successfully deleted an indexed attribute record
    // from a Base File Record Segment, we need to adjust the
    // reference count.
    //
    if( *Deleted && IsBase() && IsIndexed ) {
        SetReferenceCount(QueryReferenceCount() - 1);
    }

    return TRUE;
}

 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::DeleteResidentAttributeLocal(
    IN  ATTRIBUTE_TYPE_CODE Type,
    IN  PCWSTRING           Name OPTIONAL,
    IN  PCVOID              Value,
    IN  ULONG               ValueLength,
    OUT PBOOLEAN            Deleted,
    OUT PBOOLEAN            IsIndexed,
    OUT PUSHORT             InstanceTag
    )
/*++

Routine Description:

    This method deletes a resident attribute from the FRS.  Note
    that it will not affect external attributes.

Arguments:

    Type            --  Supplies the attribute type code.
    Name            --  Supplies the attribute name.  May be NULL, which
                        indicates that the attribute has no name.
    Value           --  Supplies the value of the attribute to delete.
    ValueLength     --  Supplies the length of the value.
    Deleted         --  Receives TRUE if the method found and deleted
                        a matching record.
    IsIndexed       --  Receives TRUE if the deleted record was indexed.
                        If no matching record was found, *Deleted is
                        FALSE and *IsIndexed is undefined.
    InstanceTag     --  Receives the instance tag of the deleted record;
                        if no matching record was found, *Deleted is
                        FALSE and *InstanceTag is undefined.

Return Value:

    TRUE upon successful completion.


--*/
{
    NTFS_ATTRIBUTE_RECORD CurrentRecord;
    PVOID CurrentRecordData;
    PCWSTR NameBuffer = NULL;
    ULONG NameLength;       //  Length of Name in characters

    DebugPtrAssert( Value );
    DebugPtrAssert( Deleted );

    *Deleted = FALSE;

    // Get the search name into a WSTR buffer, for
    // easier comparison:
    //
    if( Name == NULL ) {

        NameLength = 0;

    } else {

        NameLength = Name->QueryChCount();
        NameBuffer = Name->GetWSTR();
    }

    // Go through the attribute records in this FRS until
    // we find one that matches.  If we find one, it is
    // unique (unless the FRS is corrupt), so we delete
    // it and return.

    CurrentRecordData = NULL;

    while( (CurrentRecordData =
            GetNextAttributeRecord( CurrentRecordData )) != NULL ) {

        if( !CurrentRecord.Initialize( GetDrive(), CurrentRecordData ) ) {

            return FALSE;
        }

        // This record matches if the type codes are the same,
        // the record is resident, the names are the same length
        // and compare exactly, and the values are the same length
        // and compare exactly.
        //
        if( CurrentRecord.QueryTypeCode() == Type &&
            CurrentRecord.IsResident() &&
            CurrentRecord.QueryNameLength() == NameLength &&
            memcmp( NameBuffer,
                    CurrentRecord.GetName(),
                    NameLength * sizeof(WCHAR) ) == 0 &&
            CurrentRecord.QueryResidentValueLength() == ValueLength &&
            memcmp( CurrentRecord.GetResidentValue(),
                    Value,
                    ValueLength ) == 0 ) {

            // This is the record we want to delete.
            //
            *IsIndexed = CurrentRecord.IsIndexed();
            *InstanceTag = CurrentRecord.QueryInstanceTag();

            DeleteAttributeRecord( CurrentRecordData );
            *Deleted = TRUE;

            return TRUE;
        }
    }

    return TRUE;
}


 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::SetupAttributeList(
    )
/*++

Routine Description:

    This method makes sure that the attribute list, if present,
    has been properly set up.  If the attribute list is present
    but cannot be initialized, this method returns FALSE.

Arguments:

    None.

Return Value:

    TRUE upon successful completion.

--*/
{
    if ( _AttributeList == NULL &&
         IsAttributePresent( $ATTRIBUTE_LIST, NULL, TRUE ) ) {

        if (!(_AttributeList = NEW NTFS_ATTRIBUTE_LIST) ||
            !QueryAttributeList(_AttributeList) ||
            !_AttributeList->ReadList()) {

            //  This File Record Segment has an Attribute List, and I
            //  can't get it.  Return failure.

            DELETE(_AttributeList);
            return FALSE;
        }
    }

    return TRUE;
}

 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::CreateAttributeList(
    OUT PNTFS_ATTRIBUTE_LIST    AttributeList
    )
/*++

Routine Description:

    This method generates an Attribute List Attribute for this
    File Record Segment.

Arguments:

    AttributeList   - Returns a newly-create Attribute List.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

Notes:

    It is an error to create an Attribute List Attribute for a
    File Record Segment that already has one.

--*/
{
    NTFS_ATTRIBUTE_RECORD CurrentRecord;
    MFT_SEGMENT_REFERENCE SegmentReference;
    ULONG CurrentRecordOffset;
    DSTRING CurrentName;

    // Construct an attribute list object:

    if( !AttributeList->Initialize( GetDrive(),
                                    QueryClusterFactor(),
                                    GetUpcaseTable() ) ) {

        return FALSE;
    }

    // Walk through the record entries in this File Record Segment,
    // adding an entry to the Attribute List for each record we find.
    // Note that we can use the same Segment Reference for every entry,
    // since they are all in this File Record Segment.

    SegmentReference = QuerySegmentReference();

    CurrentRecordOffset = _FrsData->FirstAttributeOffset;

    while( CurrentRecordOffset < QuerySize() &&
           CurrentRecord.Initialize( GetDrive(),
                                     (PBYTE)_FrsData + CurrentRecordOffset,
                                     QuerySize() - CurrentRecordOffset ) &&
           CurrentRecord.QueryTypeCode() != $END ) {

        if( CurrentRecord.QueryRecordLength() == 0 ) {

            // Corrupt FRS.
            //
            return FALSE;
        }

        if (!CurrentRecord.QueryName(&CurrentName) ||
            !AttributeList->AddEntry( CurrentRecord.QueryTypeCode(),
                                      CurrentRecord.QueryLowestVcn(),
                                      &SegmentReference,
                                      CurrentRecord.QueryInstanceTag(),
                                      &CurrentName ) ) {

            return FALSE;
        }

        CurrentRecordOffset += CurrentRecord.QueryRecordLength();
    }

    return TRUE;
}


PVOID
GetBiggestLocalAttributeRecord(
    IN  PNTFS_FRS_STRUCTURE Frs
    )
/*++

Routine Description:

    This routine returns the biggest attribute record in this
    file record segment.

Arguments:

    Frs - Supplies a Frs.

Return Value:

    A pointer to the biggest attribute record or NULL.

--*/
{
    PATTRIBUTE_RECORD_HEADER    pattr, biggest;

    biggest = NULL;
    while (biggest = (PATTRIBUTE_RECORD_HEADER)
                     Frs->GetNextAttributeRecord(biggest)) {

        if (biggest->TypeCode != $STANDARD_INFORMATION) {
            break;
        }
    }

    if (!biggest) {
        return NULL;
    }

    pattr = biggest;
    while (pattr = (PATTRIBUTE_RECORD_HEADER)
                   Frs->GetNextAttributeRecord(pattr)) {

        if (pattr->TypeCode != $STANDARD_INFORMATION &&
            pattr->RecordLength > biggest->RecordLength) {
            biggest = pattr;
        }
    }

    return biggest;
}

 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::SaveAttributeList(
    PNTFS_BITMAP VolumeBitmap
    )
/*++

Routine Description:

    This method instructs the File Record Segment to save its
    attribute record list.

Arguments:

    VolumeBitmap    -- supplies the volume bitmap.

Notes:

    If _AttributesList is NULL, then the File Record Segment has not
    modified its attribute list (if it has one) and therefore does
    not need to save it.

    If this method fails, it will leave the File Record Segment in
    a consistent state (it will leave the old attribute list, if any,
    in place).

--*/
{
    NTFS_ATTRIBUTE_RECORD OldAttributeListRecord;
    NTFS_ATTRIBUTE_RECORD TemporaryRecord;
    BOOLEAN InsertSucceeded, OldPresent;
    PATTRIBUTE_RECORD_HEADER Biggest;
    ATTRIBUTE_TYPE_CODE type;
    VCN lowest_vcn;
    MFT_SEGMENT_REFERENCE seg_ref;
    USHORT instance;
    DSTRING name, tmp_name;
    BOOLEAN found_entry;


    if( _AttributeList == NULL ) {

        return TRUE;
    }

    // Note that there is no attribute list entry for the
    // attribute list's own attribute record.
    //
    // Write the attribute list.
    //
    if( !_AttributeList->WriteList( VolumeBitmap ) ) {

        return FALSE;
    }

    // We'll hedge our bets by squirreling away a copy of the old
    // attribute list's attribute record, if it exists.  Note that
    // this is a two-step process; first, we query the attribute
    // list's record from the File Record Segment; however, since
    // this record's data is actually owned by the File Record Segment,
    // we need to copy it into a record that has its own data.

    if( IsAttributePresent( $ATTRIBUTE_LIST, NULL, TRUE ) ) {

        OldPresent = TRUE;

        if( !QueryAttributeRecord(&TemporaryRecord, $ATTRIBUTE_LIST) ||
            !OldAttributeListRecord.Initialize( GetDrive(),
                                                (PVOID) TemporaryRecord.GetData(),
                                                TemporaryRecord.QueryRecordLength(),
                                                TRUE ) ) {

            // This File Record Segment has an attribute list record,
            // but we weren't able to copy it to a safe place.  Stop
            // right here, instead of going forward into a mess from
            // which we can't recover.

            return FALSE;
        }

    } else {

        OldPresent = FALSE;
    }


    // Now that we have our own copy of the old record (if it existed),
    // we can safely delete it from the File Record Segment.  We could
    // rely on _AttributeList->InsertIntoFile to do that for us, but
    // that would complicate our error handling further down.

    if( !PurgeAttribute( $ATTRIBUTE_LIST ) ) {

        return FALSE;
    }


    // Now we try every trick we've got to insert the new attribute
    // list into the File Record Segment.

    InsertSucceeded = FALSE;

    while( !InsertSucceeded ) {

        InsertSucceeded = _AttributeList->InsertIntoFile( this,
                                                          VolumeBitmap );

        if( !InsertSucceeded ) {

            // We weren't able to insert the attribute.  Try different
            // strategems to jam it in.   First, if the attribute list
            // is resident, we can make it non-resident.

            if( _AttributeList->IsResident() ) {

                if( !_AttributeList->MakeNonresident( VolumeBitmap ) ) {

                    // We failed trying to make the attribute list
                    // nonresident.  Give up.

                    break;
                }

            } else {

                // It's nonresident, so we have to move to our next
                // contingency plan:  start moving records out of the
                // base File Record Segment and into children.

                // Find the biggest attribute record in the base
                // and then eliminate it from the attribute list
                // and the base file record segment but save it.

                Biggest = (PATTRIBUTE_RECORD_HEADER)
                          GetBiggestLocalAttributeRecord(this);
                if (!Biggest ||
                    !TemporaryRecord.Initialize(GetDrive(),
                                                Biggest,
                                                Biggest->RecordLength,
                                                TRUE)) {

                    // Serious problems.
                    break;
                }

                found_entry = FALSE;

                ATTR_LIST_CURR_ENTRY    entry;

                entry.CurrentEntry = NULL;
                while (_AttributeList->QueryNextEntry(&entry,
                                                      &type,
                                                      &lowest_vcn,
                                                      &seg_ref,
                                                      &instance,
                                                      &name)) {

                    if (type == TemporaryRecord.QueryTypeCode() &&
                        lowest_vcn == TemporaryRecord.QueryLowestVcn() &&
                        seg_ref == QuerySegmentReference() &&
                        instance == TemporaryRecord.QueryInstanceTag() &&
                        TemporaryRecord.QueryName(&tmp_name) &&
                        !tmp_name.Strcmp(&name)) {

                        _AttributeList->DeleteCurrentEntry(&entry);
                        found_entry = TRUE;
                        break;
                    }
                }

                if (found_entry) {
                    DeleteAttributeRecord(Biggest);
                } else {
                    DebugAbort("Could not find attribute list entry for big");
                    break;
                }

                // Now just pull an insert external on this record to
                // finish the task.

                if (!InsertExternalAttributeRecord(&TemporaryRecord) ||
                    !_AttributeList->WriteList(VolumeBitmap)) {

                    // Out of memory or out of disk space.
                    break;
                }
            }
        }
    }


    if( !InsertSucceeded && OldPresent ) {

        // Since we were unable to insert the new attribute
        // record, we reinsert the old one.  Note that we can
        // be sure that there's room for it, since we haven't
        // added anything to the File Record Segment since we
        // deleted this record.

        InsertAttributeRecord( &OldAttributeListRecord );
    }

    // For good or ill, we're done.

    return( InsertSucceeded );
}

 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::QueryAttributeRecord (
    OUT PNTFS_ATTRIBUTE_RECORD  AttributeRecord,
    IN  ATTRIBUTE_TYPE_CODE     Type,
    IN  PCWSTRING               Name
   )
/*++

Routine Description:

    This method finds an attribute record in the FRS.  Note that it only
    searches this FRS, it will not look for external attribute records.

Arguments:

    AttributeRecord -- returns the Attribute Record object.
    Type            -- supplies the type of the desired attribute record
    Name            -- supplies the name of the desired attribute (NULL if
                           the attribute has no name).

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG CurrentRecordOffset;
    BOOLEAN Found = FALSE;

    DebugPtrAssert( _FrsData );
    DebugPtrAssert( AttributeRecord );

    // Spin through the records in this File Record Segment
    // looking for a match.  If we find one, set the Found
    // flag and break out.

    CurrentRecordOffset = _FrsData->FirstAttributeOffset;

    while( CurrentRecordOffset < QuerySize() &&
           AttributeRecord->Initialize( GetDrive(),
                                        (PBYTE)_FrsData + CurrentRecordOffset,
                                        QuerySize() - CurrentRecordOffset ) &&
           AttributeRecord->QueryTypeCode() != $END ) {

        if( AttributeRecord->IsMatch( Type, Name ) ) {

            Found = TRUE;
            break;
        }

        // Go on to the next record

        CurrentRecordOffset += AttributeRecord->QueryRecordLength();
    }

    // If Found is TRUE, then CurrentRecord is the attribute
    // record we want, so we'll return it to the caller.

    return Found;
}


 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::InsertAttributeRecord (
    IN  PNTFS_ATTRIBUTE_RECORD  NewRecord,
    IN  BOOLEAN                 ForceExternal
   )
/*++

Routine Description:

   This method inserts an attribute record into the File Record
   Segment.

   If the File Record Segment has room, it will just insert the
   record.  If not, it may make the attribute external, or it
   may make room for the record by making other attributes nonresident
   or external.

   The Attribute List attribute and the Standard Information attribute
   cannot be made external.

   This method is virtual because classes which derive from
   File Record Segment may have preferences about how they manage
   their attribute records.

Arguments:

   AttributeRecord   -- supplies the attribute record to insert

   ForceExternal     -- supplies a flag telling whether to force
                        this record into a child File Record Segment;
                        if this flag is TRUE, we force this record
                        to be external.

Return Value:

    TRUE upon successful completion.

Notes:

    This method may also update the Instance tag in the attribute record.

--*/
{
    NTFS_ATTRIBUTE_RECORD CurrentRecord;
    MFT_SEGMENT_REFERENCE SegmentReference, CheckSegment;
    DSTRING               NewName, CheckName;
    NTFS_ATTRIBUTE_RECORD TempRecord;
    PATTRIBUTE_RECORD_HEADER EvicteeData, TempRecordData;
    DSTRING NewRecordName;
    VCN CheckVcn;
    ULONG CurrentRecordOffset;
    ULONG FreeSpace;
    ATTRIBUTE_TYPE_CODE CheckType;
    BOOLEAN Result, MatchPresent;
    USHORT CheckInstance;

    // The reservation for the attribute list is enought for
    // a nonresident attribute with no name and two worst-case
    // extents.  Since this reservation is only required for
    // FRS 0, this should be plenty.
    //
    CONST int AttributeListReservation = SIZE_OF_NONRESIDENT_HEADER +
                                     3 * ( 1 + 2 * sizeof(VCN) );


   DebugPtrAssert( _FrsData );
    DebugPtrAssert( NewRecord );

    // The utilities will never insert any non-$DATA records into
    // the reserved MFT-overflow FRS when it is being used as
    // a child of the MFT.
    //
    if( !IsBase() &&
        QueryFileNumber() == MFT_OVERFLOW_FRS_NUMBER &&
        NewRecord->QueryTypeCode() != $DATA ) {

        return FALSE;
    }

    // If this File Record Segment has an attribute list, we have
    // to make sure that we've read it.

    if( !SetupAttributeList() ) {

        return FALSE;
    }

    // Determine whether an attribute record of the same type and
    // name is already present in this precise FRS (ie. in this
    // FRS and not one of its children).  If this is the case,
    // then if this
    //
    if( !NewRecord->QueryName( &NewRecordName ) ) {

        DebugPrint( "UNTFS: Can't get name from attribute record.\n" );
        return FALSE;
    }

    MatchPresent = IsAttributePresent( NewRecord->QueryTypeCode(),
                                       &NewRecordName,
                                       TRUE );

    // Determine how much space is available in the File Record Segment.
   // If there's enough, find the correct insertion point for this
    // attribute record and put it there.
    //
    FreeSpace = QueryFreeSpace();

    if( QueryFileNumber() == MASTER_FILE_TABLE_NUMBER &&
        IsBase() &&
        NewRecord->QueryTypeCode() != $ATTRIBUTE_LIST ) {

        // Reserve space for the Attribute List.
        //
        if ( FreeSpace < AttributeListReservation ) {

            FreeSpace = 0;

        } else{

            FreeSpace -= AttributeListReservation;
        }
    }

    // The new record can go in this FRS if the client has not
    // requested that it be made external, if there's sufficient
    // room for it, and if there is no collision with other records
    // for this attribute.
    //
   if( !ForceExternal &&
        NewRecord->QueryRecordLength() <= FreeSpace &&
        ( NewRecord->IsResident() || !MatchPresent ) ) {

        // There's enough free space for the new record, so it'll
        // go in this FRS.  Attach the current Next Instance tag
        // to this attribute record and increment the tag.
        //
        NewRecord->SetInstanceTag( QueryNextInstance() );
        IncrementNextInstance();

        // Scan through the list of records to find the point
      // at which we should insert it.
        //
      CurrentRecordOffset = _FrsData->FirstAttributeOffset;

      while( CurrentRecordOffset < QuerySize() &&
             CurrentRecord.Initialize( GetDrive(),
                                       (PBYTE)_FrsData + CurrentRecordOffset,
                                       QuerySize() - CurrentRecordOffset ) &&
             CurrentRecord.QueryTypeCode() != $END &&
             CompareAttributeRecords( &CurrentRecord,
                                        NewRecord,
                                        GetUpcaseTable() ) <= 0 ) {

            if( CurrentRecord.QueryRecordLength() == 0 ) {

                // Corrupt FRS.
                //
                return FALSE;
            }

         // Go on to the next record

         CurrentRecordOffset += CurrentRecord.QueryRecordLength();
      }

      // We want to insert the new record at CurrentRecordOffset;
      // make room for it there and copy it in.

      memmove( (PBYTE)_FrsData + CurrentRecordOffset +
                           NewRecord->QueryRecordLength(),
                 (PBYTE)_FrsData + CurrentRecordOffset,
                 _FrsData->FirstFreeByte - CurrentRecordOffset);

        _FrsData->FirstFreeByte += NewRecord->QueryRecordLength();

      memcpy( (PBYTE)_FrsData + CurrentRecordOffset,
            NewRecord->GetData(),
            (UINT) NewRecord->QueryRecordLength() );

        // if this File Record Segment has an attribute list, then
        // we add an entry to it for the attribute record we just added.
        // Note that we can safely assume that an attribute list is
        // present if and only if _AttributeList is non-NULL because
        // we made sure we had read it at the beginning of this method.

        if( _AttributeList != NULL &&
            NewRecord->QueryTypeCode() != $ATTRIBUTE_LIST ) {

            SegmentReference = QuerySegmentReference();

            if (!NewRecord->QueryName(&NewName) ||
                !_AttributeList->AddEntry(NewRecord->QueryTypeCode(),
                                          NewRecord->QueryLowestVcn(),
                                          &SegmentReference,
                                          NewRecord->QueryInstanceTag(),
                                          &NewName)) {

                // So near and yet so far.  We have to back all the
                // way out because we couldn't add an entry for this
                // record to the attribute list.

                memmove( (PBYTE)_FrsData + CurrentRecordOffset,
                         (PBYTE)_FrsData + CurrentRecordOffset +
                                            NewRecord->QueryRecordLength(),
                         _FrsData->FirstFreeByte -
                            (CurrentRecordOffset +
                             NewRecord->QueryRecordLength()) );

                _FrsData->FirstFreeByte -= NewRecord->QueryRecordLength();

                return FALSE;
            }
        }

        Result = TRUE;

    } else if( QueryFileNumber() == MASTER_FILE_TABLE_NUMBER &&
               NewRecord->QueryTypeCode() == $DATA &&
               NewRecord->QueryLowestVcn() == 0 ) {

        // The first chunk of the MFT's $DATA attribute must
        // be in the base File Record Segment.  Evict as many
        // other records as possible.
        //
        if( (TempRecordData = (PATTRIBUTE_RECORD_HEADER)
                              MALLOC( QuerySize() )) == NULL ) {

            DebugPrint( "UNTFS: Can't allocate memory for temporary attribute record.\n" );
            return FALSE;
        }

        EvicteeData = (PATTRIBUTE_RECORD_HEADER)GetNextAttributeRecord( NULL );

        while( EvicteeData != NULL &&
               EvicteeData->TypeCode != $END ) {

            // If this is not an attribute record we can evict,
            // then skip over it; otherwise, move it to a child
            // FRS.  Note that we don't check for the first chunk
            // of the $DATA attribute becuase, after all, that's
            // what we're inserting.
            //
            if( EvicteeData->TypeCode == $STANDARD_INFORMATION ||
                EvicteeData->TypeCode == $ATTRIBUTE_LIST ) {

                EvicteeData = (PATTRIBUTE_RECORD_HEADER)
                              GetNextAttributeRecord( EvicteeData );

            } else {

                // Copy this record to the temporary buffer and
                // delete it from the FRS.
                //
                memcpy( TempRecordData,
                        EvicteeData,
                        EvicteeData->RecordLength );

                if( !TempRecord.Initialize( GetDrive(), TempRecordData ) ) {

                    FREE( TempRecordData );
                    return FALSE;
                }

                DeleteAttributeRecord( EvicteeData );

                // Delete the attribute list entry that corresponds
                // to this attribute record.
                //
                if( _AttributeList != NULL ) {

                    ATTR_LIST_CURR_ENTRY    Entry;

                    Entry.CurrentEntry = NULL;
                    while( _AttributeList->QueryNextEntry( &Entry,
                                                           &CheckType,
                                                           &CheckVcn,
                                                           &CheckSegment,
                                                           &CheckInstance,
                                                           &CheckName ) ) {

                        // It's enough to establish a match if the
                        // segment reference and instance tag match,
                        // but we'll throw in a check for the type
                        // code just to be safe.
                        //
                        if( CheckType == TempRecord.QueryTypeCode() &&
                            CheckInstance == TempRecord.QueryInstanceTag() &&
                            CheckSegment == QuerySegmentReference() ) {

                            _AttributeList->DeleteCurrentEntry( &Entry );
                            break;
                        }
                    }
                }

                // Insert this record into a child FRS.
                //
                if( !InsertExternalAttributeRecord( &TempRecord ) ) {

                    FREE( TempRecordData );
                    return FALSE;
                }

                // No need to advance EvicteeData, since
                // DeleteAttributeRecord will bring the next
                // one down to us.
            }

        }

        FREE( TempRecordData );

        // OK, we've made as much free space as possible.  Recompute
        // the free space; if it's enough, insert the record recursively.
        // Since we only recurse if there's enough free space, and
        // we only fall into this branch if there isn't enough space,
        // we won't get infinite recursion.
        //
        FreeSpace = QueryFreeSpace();

        if( FreeSpace < AttributeListReservation ) {

            FreeSpace = 0;

        } else {

            FreeSpace -= AttributeListReservation;
        }

        if( FreeSpace > NewRecord->QueryRecordLength() ) {

            // It'll fit--go ahead and insert it.
            //
            return( InsertAttributeRecord( NewRecord, FALSE ) );

        } else {

            return FALSE;

        }

    } else {

        // This attribute will be external.  Call the private worker
        // method.

        Result = InsertExternalAttributeRecord( NewRecord );
    }

    //  If this is a base file record segment and we've successfully
    //  inserted an indexed attribute, increment the reference count.

    if( Result &&
        IsBase() &&
        NewRecord->IsResident() &&
        (NewRecord->QueryResidentFlags() & RESIDENT_FORM_INDEXED ) ) {

        _FrsData->ReferenceCount += 1;
    }

    return Result;
}

 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::InsertExternalAttributeRecord(
    IN  PNTFS_ATTRIBUTE_RECORD NewRecord
    )
/*++

Routine Description:

    This method adds an external attribute record to the File Record
    Segment.  It is a private worker for InsertAttributeRecord.  (It
    can also be used to move attribute records out of the base segment).

    Note that it is nonvirtual--once the File Record Segment has decided
    to make the attribute record external, it's done the same way for
    all types of File Record Segments.

Arguments:

   AttributeRecord   -- supplies the attribute record to insert

Return Value:

    TRUE upon successful completion.

Notes:

    The Attribute List and Standard Information attributes cannot
    be made external; this method enforces that restriction.


--*/
{
    PNTFS_FILE_RECORD_SEGMENT ChildFrs;
    MFT_SEGMENT_REFERENCE SegmentReference;
    VCN ChildFileNumber;
    ATTRIBUTE_TYPE_CODE TypeCode;
    VCN LowestVcn;
    DSTRING Name;
    USHORT Instance;
    BOOLEAN IsMft, IsMftData;
    ATTR_LIST_CURR_ENTRY    Entry;

    ULONG cluster_size = QueryClusterFactor() * GetDrive()->QuerySectorSize();


    IsMft = ( QueryFileNumber() == MASTER_FILE_TABLE_NUMBER );
    IsMftData = IsMft && ( NewRecord->QueryTypeCode() == $DATA );

    // the standard information and attribute list attributes cannot
    // be made external.

    if( NewRecord->QueryTypeCode() == $STANDARD_INFORMATION ||
        NewRecord->QueryTypeCode() == $ATTRIBUTE_LIST ) {

        return FALSE;
    }

    // The Log File cannot have any external attributes.
    //
    if( !IsBase() &&
        QueryFileNumber() == LOG_FILE_NUMBER ) {

        return FALSE;
    }

    // Note that only Base File Record Segments can have attribute lists.

    if( !IsBase() || !SetupAttributeList() ) {

        return FALSE;
    }

    // OK, at this point, _AttributeList is NULL if and only if
    // the File Record Segment has no attribute list.  If the File
    // Record Segment has no attribute list, we need to create one.

    if (_AttributeList == NULL) {

        if (!(_AttributeList = NEW NTFS_ATTRIBUTE_LIST) ||
            !CreateAttributeList(_AttributeList)) {

            // We can't create an attribute list for this File Record
            // Segment.

            DELETE(_AttributeList);
            return FALSE;
        }
    }

    // Since this is a somewhat rare case, we can indulge ourselves
    // a bit.  Go through the Attribute List and force all the
    // children of this File Record Segment into memory.

    Entry.CurrentEntry = NULL;
    while (_AttributeList->QueryNextEntry( &Entry,
                                           &TypeCode,
                                           &LowestVcn,
                                           &SegmentReference,
                                           &Instance,
                                           &Name )) {

        ChildFileNumber.Set( SegmentReference.LowPart,
                             (LONG) SegmentReference.HighPart );

        if( ChildFileNumber != QueryFileNumber() &&
            SetupChild( ChildFileNumber ) == NULL ) {

            // Error.
            return FALSE;
        }
    }


    // Now go down the list of children and see if any of them
    // will accept this record.  Note that if this is the MFT
    // $DATA attribute, we must check each child before we try
    // to use it to make sure we don't break the bootstrap.
    // we much check each child
    //
    _ChildIterator->Reset();

    while( (ChildFrs =
            (PNTFS_FILE_RECORD_SEGMENT)_ChildIterator->GetNext()) != NULL ) {

        // The MFT requires special handling.  Records for the data
        // attribute must be inserted into child FRS' in a way that
        // preserves the MFT's bootstrapping, ie. the starting VCN of
        // the record must be greater than the VCN (file number times
        // clusters per FRS) of the child.  In addition, $DATA records
        // cannot share a child FRS with any other attribute records.
        //
        if( IsMft ) {

            if (IsMftData &&
                ( ChildFrs->GetNextAttributeRecord( NULL ) != NULL ||
                  NewRecord->QueryLowestVcn() * cluster_size <=
                        ChildFrs->QueryFileNumber() * QuerySize())) {

                // Either this child FRS is not empty, or
                // inserting this record into it will break
                // the MFT's bootstrapping.  Either way,
                // it can't accept this record.
                //
                continue;
            }

            if( ChildFrs->IsAttributePresent( $DATA ) ) {

                // This child FRS already has a $DATA attribute
                // record, so it can't accept any other records.
                //
                continue;
            }
        }

        // This child FRS is eligible to hold this attribute record.
        //
        if( ChildFrs->InsertAttributeRecord( NewRecord ) ) {

            // Success!  Add an appropriate entry to the
            // attribute list.
            //
            SegmentReference = ChildFrs->QuerySegmentReference();

            if( !NewRecord->QueryName( &Name ) ||
                !_AttributeList->AddEntry( NewRecord->QueryTypeCode(),
                                           NewRecord->QueryLowestVcn(),
                                           &SegmentReference,
                                           NewRecord->QueryInstanceTag(),
                                           &Name ) ) {

                DebugPrintTrace(( "UNTFS: Can't add entry to attribute list." ));
                return FALSE;
            }

            return TRUE;
        }
    }

    // We have to allocate a new child File Record Segment.  If
    // This is the MFT File, see if we can grab the reserved FRS.
    //
    if( IsMftData &&
        GetChild( MFT_OVERFLOW_FRS_NUMBER ) == NULL ) {

        // This FRS is the MFT, and it hasn't already grabbed
        // the reserved FRS--grab it now.
        //
        ChildFileNumber = MFT_OVERFLOW_FRS_NUMBER;

    } else if( !_Mft->AllocateFileRecordSegment( &ChildFileNumber, IsMftData ) ) {

        // Can't get a new child File Record Segment.

        return FALSE;
    }

    // Set up the Segment Reference to refer to the Base File
    // Record Segment, ie. this File Record Segment.
    //
    SegmentReference = QuerySegmentReference();

    // Construct the new child File Record Segment and insert
    // the attribute record into it.  Again, make sure we don't
    // violate the MFT bootstrapping requirements.  (Note that
    // we don't have to check the sharing rule for the MFT data
    // attribute because this is a new FRS--it can't contain
    // any conflicting records.
    //
    if( (ChildFrs = NEW NTFS_FILE_RECORD_SEGMENT) == NULL ||
        !ChildFrs->Initialize( ChildFileNumber, _Mft )    ||
        !ChildFrs->Create( &SegmentReference )            ||
        ( IsMftData &&
          NewRecord->QueryLowestVcn() * cluster_size <=
            ChildFrs->QueryFileNumber() * QuerySize() ) ||
        !ChildFrs->InsertAttributeRecord( NewRecord )   ||
        !AddChild( ChildFrs ) ) {

        // That didn't do us any good at all.  Clean up the child
        // and return our failure.

        DELETE( ChildFrs );
        _Mft->FreeFileRecordSegment( ChildFileNumber );

        return FALSE;
    }

    // Note that the Child File Record Segment has passed into the
    // keeping of the children list, so we don't delete it.
    //
    // Add an entry to the attribute list for the new record.
    //
    SegmentReference = ChildFrs->QuerySegmentReference();

    if( !NewRecord->QueryName( &Name ) ||
        !_AttributeList->AddEntry( NewRecord->QueryTypeCode(),
                                   NewRecord->QueryLowestVcn(),
                                   &SegmentReference,
                                   NewRecord->QueryInstanceTag(),
                                   &Name ) ) {

        DebugPrintTrace(( "UNTFS: Can't add entry to attribute list." ));
        return FALSE;
    }

    return TRUE;
}

 
 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::Write(
   )
/*++

Routine Description:

    This method writes the File Record Segment.  It does not affect the
    attribute list or the child record segments.

Arguments:

    None.

Return Value:

   TRUE upon successful completion.

--*/
{
   return( NTFS_FRS_STRUCTURE::Write() );
}

 
 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::Flush(
    IN OUT PNTFS_BITMAP     VolumeBitmap,
    IN OUT PNTFS_INDEX_TREE ParentIndex,
    IN     BOOLEAN          FrsIsEmpty
    )
/*++

Routine Description:

    This method is used to commit a file to disk.  It saves the attribute
    list, writes any child record segments that have been brought into
    memory, and writes the File Record Segment itself.

Arguments:

    VolumeBitmap    --  Supplies the volume bitmap.  This parameter may
                        be NULL, in which case non-resident attributes
                        cannot be resized.
    ParentIndex     --  Supplies the directory which indexes this FRS
                        over $FILE_NAME.  May be NULL.

Return Value:

    TRUE upon successful completion.

--*/
{
    DUPLICATED_INFORMATION      DuplicatedInformation;
    PNTFS_FILE_RECORD_SEGMENT   CurrentChild, ChildToDelete;

    // If this is a child FRS, just write it.
    //
    if( !IsBase() ) {

        return( Write() );
    }

    // This FRS is a Base File Record Segment--it may have
    // an attribute list and children, and we have to update
    // the file-name information in the parent index.
    //

    if( !FrsIsEmpty && _AttributeList != NULL &&
        !SaveAttributeList( VolumeBitmap ) ) 
    {
        return FALSE;
    }

    // Update the file name attributes:

    if ( !FrsIsEmpty )
        if( !QueryDuplicatedInformation( &DuplicatedInformation ) ||
            !UpdateFileNames( &DuplicatedInformation, ParentIndex, FALSE ) ) {

            DebugAbort( "Can't update file names in Flush.\n" );
            return FALSE;
        }

    // Flush all the children.  If a child is empty, mark it as
    // unused.
    //
    _ChildIterator->Reset();

    while( (CurrentChild = (PNTFS_FILE_RECORD_SEGMENT)
                           _ChildIterator->GetNext()) != NULL ) {

        if( !CurrentChild->GetNextAttributeRecord( NULL ) ) {


            // This child has no attribute records--we don't
            // need it anymore.
            //
            CurrentChild->ClearInUse();
            CurrentChild->Write();
            _Mft->FreeFileRecordSegment( CurrentChild->QueryFileNumber() );
            ChildToDelete = (PNTFS_FILE_RECORD_SEGMENT)_Children.Remove(_ChildIterator);
            DebugAssert(ChildToDelete == CurrentChild);
            DELETE(ChildToDelete);
            CurrentChild = NULL;
            _ChildIterator->Reset();

        } else if( !CurrentChild->Flush( VolumeBitmap ) ) {

            return FALSE;
        }
    }

    return( Write() );
}

 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::QueryDuplicatedInformation(
    OUT PDUPLICATED_INFORMATION DuplicatedInformation
    )
/*++

Routine Description:

    This method queries the FRS for the information which is
    duplicated in File Name attributes.

Arguments:

    DuplicatedInformation   --  Receives the duplicated information.

Return Value:

    TRUE upon successful completion.


--*/
{
    NTFS_ATTRIBUTE          Attribute;
    STANDARD_INFORMATION    StandardInformation;
    EA_INFORMATION          EaInformation;
    REPARSE_DATA_BUFFER     reparse_point;
    ULONG                   BytesRead;
    BOOLEAN                 Error;

    // Start with a clean slate:
    //
    memset( DuplicatedInformation, 0, sizeof( DUPLICATED_INFORMATION ) );

    // Most of the duplicated information comes from the
    // Standard Information attribute.
    //
    if( !QueryAttribute( &Attribute,
                         &Error,
                         $STANDARD_INFORMATION ) ||
        !Attribute.Read( &StandardInformation,
                         0,
                         sizeof( STANDARD_INFORMATION ),
                         &BytesRead ) ||
        BytesRead != sizeof( STANDARD_INFORMATION ) ) {

        DebugPrintTrace(( "Can't fetch standard information.\n" ));
        return FALSE;
    }

    DuplicatedInformation->CreationTime =
                        StandardInformation.CreationTime;
    DuplicatedInformation->LastModificationTime =
                        StandardInformation.LastModificationTime;
    DuplicatedInformation->LastChangeTime =
                        StandardInformation.LastChangeTime;
    DuplicatedInformation->LastAccessTime  =
                        StandardInformation.LastAccessTime;

    DuplicatedInformation->FileAttributes = StandardInformation.FileAttributes;

    if( _FrsData->Flags & FILE_FILE_NAME_INDEX_PRESENT ) {

        DuplicatedInformation->FileAttributes |= DUP_FILE_NAME_INDEX_PRESENT;
    }

    if( _FrsData->Flags & FILE_VIEW_INDEX_PRESENT ) {

        DuplicatedInformation->FileAttributes |= DUP_VIEW_INDEX_PRESENT;
    }

    // We also need one field from the EA_INFORMATION attribute
    // or the REPARSE_POINT attribute.
    //
    if( !QueryAttribute( &Attribute, &Error, $EA_INFORMATION )) {

        if( Error ) {

            // The Ea Information attribute is present, but we
            // couldn't get it.  Bail out.
            //
            DebugAbort( "Error fetching Ea Information attribute.\n" );
            return FALSE;

        } else if (!QueryAttribute( &Attribute, &Error, $REPARSE_POINT )) {

            if( Error ) {

                // The Reparse Point attribute is present, but we
                // couldn't get it.  Bail out.
                //
                DebugAbort( "Error fetching Reparse Point attribute.\n" );
                return FALSE;
            } else {

                // The Ea Information attribute is not present and there
                // is no Reparse Point, which means this file has no EAs.

                DuplicatedInformation->PackedEaSize = 0;
            }
        } else if ( !Attribute.Read( &reparse_point,
                                     0,
                                     FIELD_OFFSET(_REPARSE_DATA_BUFFER,
                                                  GenericReparseBuffer.DataBuffer),
                                     &BytesRead ) ||
                    BytesRead != FIELD_OFFSET(_REPARSE_DATA_BUFFER,
                                              GenericReparseBuffer.DataBuffer)) {

            // Couldn't read the Reparse Point data.
            //
            DebugAbort( "Can't read Reparse Point.\n" );
            return FALSE;

        } else {

            // We've got the Reparse Point.
            //
            DuplicatedInformation->ReparsePointTag = reparse_point.ReparseTag;
        }

    } else if( !Attribute.Read( &EaInformation,
                                0,
                                sizeof( EA_INFORMATION ),
                                &BytesRead ) ||
                BytesRead != sizeof( EA_INFORMATION ) ) {

        // Couldn't read the Ea Information data.
        //
        DebugAbort( "Can't read EA Information.\n" );
        return FALSE;

    } else {

        // We've got the Ea Information.
        //
        DuplicatedInformation->PackedEaSize = EaInformation.PackedEaSize;
    }


    // Now we grope the size of the unnamed $DATA attribute and we
    // are done.
    //

    if( !QueryFileSizes( &(DuplicatedInformation->AllocatedLength),
                         &(DuplicatedInformation->FileSize),
                         &Error ) ) {

        if( Error ) {

            // The Data attribute is present, but we couldn't get it.
            //
            DebugAbort( "Error fetching $DATA attribute.\n" );
            return FALSE;

        } else {

            // This file has no unnamed $DATA attribute.
            //
            DuplicatedInformation->AllocatedLength = 0;
            DuplicatedInformation->FileSize = 0;
        }

    }

    return TRUE;
}

 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::UpdateFileNames(
    IN     PDUPLICATED_INFORMATION  DuplicatedInformation,
    IN OUT PNTFS_INDEX_TREE         Index,
    IN     BOOLEAN                  IgnoreExternal
    )
/*++

Routine Description:

    This method propagates

Arguments:

    DuplicatedInformation   --  Supplies the duplicated information which
                                is to be propagated into the File Names.
    Index                   --  Supplies the index for the directory which
                                contains this FRS.  It may be NULL, in which
                                case changes are not propagated to the index.
    IgnoreExternal          --  Supplies a flag which, if TRUE, indicates
                                that this method should only update file
                                names in this FRS, and not in its children.

Return Value:

    TRUE upon successful completion.

Notes:

    This method only propagates changes to the supplied index; thus,
    if a file is indexed by more than one directory, changes can
    only be propagated to one directory.  However, all entries for
    this file in the supplied index will be updated.

--*/
{
    NTFS_ATTRIBUTE_RECORD CurrentRecord;
    DSTRING Name;
    MFT_SEGMENT_REFERENCE SegmentReference;

    PNTFS_FILE_RECORD_SEGMENT ChildFrs;
    LCN TargetFileNumber;
    VCN LowestVcn;
    ATTRIBUTE_TYPE_CODE CurrentType;
    PFILE_NAME CurrentName;
    PVOID CurrentRecordData;
    ULONG ValueLength;
    USHORT Tag;

    // Make sure that the supplied index, if any, is constructed
    // over the FILE_NAME attribute:

    if( Index != NULL &&
        Index->QueryIndexedAttributeType() != $FILE_NAME ) {

        DebugAbort( "Updating file names in an index that's not over $FILE_NAME.\n" );
        return FALSE;
    }

    // Force the attribute list, if we have one, into memory.
    //
    if( !SetupAttributeList() ) {

        return FALSE;
    }

    if( !IgnoreExternal &&
        _AttributeList != NULL ) {

        ATTR_LIST_CURR_ENTRY        Entry;

        Entry.CurrentEntry = NULL;
        while (_AttributeList->QueryNextEntry( &Entry,
                                               &CurrentType,
                                               &LowestVcn,
                                               &SegmentReference,
                                               &Tag,
                                               &Name )) {

            if( CurrentType == $FILE_NAME ) {

                // The current entry represents a record for a
                // File Name attribute, which we may wish to
                // update.  Figure out what FRS it's in:

                TargetFileNumber.Set( SegmentReference.LowPart,
                                        (LONG) SegmentReference.HighPart );

                if( TargetFileNumber == QueryFileNumber() ) {

                    // This file name is in the base FRS, ie. in
                    // this FRS.  Call this method recursively,
                    // with IgnoreExternal TRUE to limit the
                    // recursion.

                    if( !UpdateFileNames( DuplicatedInformation,
                                          Index,
                                          TRUE ) ) {
                        return FALSE;
                    }

                } else {

                    // This attribute is in a child FRS.  Get the
                    // child and update any file names in it.  Note
                    // Note that we can ignore external names in
                    // the child, since child FRS's cannot have
                    // children.  (Note also that this will preemptively
                    // update any other names in that child, which is
                    // fine.
                    //
                    if( (ChildFrs = SetupChild( TargetFileNumber )) == NULL ||
                        !ChildFrs->UpdateFileNames( DuplicatedInformation,
                                                    Index,
                                                    TRUE ) ) {

                        return FALSE;
                    }
                }
            }
        }

    } else {

        // Crawl through the records in this FRS looking for File Names.
        //
        CurrentRecordData = NULL;

        while( (CurrentRecordData =
                GetNextAttributeRecord( CurrentRecordData )) != NULL ) {

            // Note that GetNextAttributeRecord always returns a
            // structurally-sound attribute record, which we can
            // pass directly to this flavor of Initialize, which
            // in turn will always succeed.  It is also important
            // that this flavor of initialize will cause the
            // attribute record to use directly the data in the
            // File Record Segment, so we can twiddle it there.
            //
            if( !CurrentRecord.Initialize( GetDrive(), CurrentRecordData ) ) {

                DebugAbort( "Can't initialize attribute record.\n" );
                return FALSE;
            }

            if( CurrentRecord.QueryTypeCode() == $FILE_NAME ) {

                // It's a file name.

                CurrentName = (PFILE_NAME)
                           (CurrentRecord.GetResidentValue() );

                ValueLength = CurrentRecord.QueryResidentValueLength();

                // Perform sanity checks--the attribute must be resident,
                // big enough to be a File Name, and big enough to hold
                // the name it claims to be.
                //
                if( CurrentName == NULL ||
                    ValueLength < sizeof( FILE_NAME ) ||
                    ValueLength < (ULONG)FIELD_OFFSET( FILE_NAME, FileName ) +
                                  CurrentName->FileNameLength ) {

                    DebugAbort( "Corrupt file name.\n" );
                    return FALSE;
                }

                // OK, it's a valid file name.  Update the duplicated
                // information and propagate duplicated information and
                // file name bits back to the index entry.
                //
                memcpy( &(CurrentName->Info),
                        DuplicatedInformation,
                        sizeof( DUPLICATED_INFORMATION ) );

                // Update the corresponding entry in the index.
                //

                SegmentReference = ( IsBase() ) ?
                        QuerySegmentReference() :
                        QueryBaseFileRecordSegment();


                if( Index != NULL &&
                    !Index->UpdateFileName( CurrentName,
                                            SegmentReference ) ) {

                    return FALSE;
                }
            }
        }
    }

    return TRUE;
}


 
VOID
NTFS_FILE_RECORD_SEGMENT::SetLsn(
    IN  BIG_INT NewLsn
    )
/*++

Routine Description:

    This method sets the Lsn for the File Record Segment
    and any of its children which are in memory.

Arguments:

    NewLsn  --  Supplies the new LSN for this File Record
                Segment and any available children.

Return Value:

    None.

--*/
{
    PNTFS_FILE_RECORD_SEGMENT   CurrentChild;

    _FrsData->Lsn = NewLsn.GetLargeInteger();

    _ChildIterator->Reset();

    while( (CurrentChild = (PNTFS_FILE_RECORD_SEGMENT)
                           _ChildIterator->GetNext()) != NULL ) {

        CurrentChild->SetLsn( NewLsn );
    }
}

 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::Backtrack(
    OUT PWSTRING Path
    )
/*++

Routine Description:

    This function finds a path from the root to this FRS.  Note
    that it does not detect cycles, and may enter into an infinite
    loop if there is a cycle in the logical directory structure.

    Note that the client must read this FRS before calling Backtrack.

Arguments:

    Path    --  Receives the path.

Return Value:

    TRUE upon successful completion.

--*/
{
    // First, check to see if this is the Root
    //
    if( QueryFileNumber() == ROOT_FILE_NAME_INDEX_NUMBER ) {

        return( Path->Initialize( "\\" ) );
    }

    // Initialize the path to the empty string and then pass it
    // to the worker routine.
    //
    return( Path->Initialize( "" ) &&
            BacktrackWorker( Path ) );
}

 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::BacktrackWorker(
            IN OUT PWSTRING Path
            )
/*++

Routine Description:

    This member function is a private worker routine for Backtrack;
    it performs the actual work of constructing the path from the
    root to this FRS.

Arguments:

    Path    --  Receives the path to this FRS.

Return Value:

    TRUE upon successful completion.

--*/
{
    NTFS_ATTRIBUTE FileNameAttribute;
    NTFS_FILE_RECORD_SEGMENT ParentFrs;
    VCN ParentFileNumber;
    PCFILE_NAME FileName;
    DSTRING FileNameString, Backslash;
    BOOLEAN Error;

    // Short circuit if this is the root--it doesn't add
    // anything to the path.
    //
    if( QueryFileNumber() ==  ROOT_FILE_NAME_INDEX_NUMBER ) {

        return TRUE;
    }

    // Extract a name from the FRS.  Any name will do.
    //
    if( !QueryAttribute( &FileNameAttribute, &Error, $FILE_NAME ) ||
        !FileNameAttribute.IsResident() ) {

        return FALSE;
    }

    FileName = (PCFILE_NAME)FileNameAttribute.GetResidentValue();

    ParentFileNumber.Set( FileName->ParentDirectory.LowPart,
                          (LONG)FileName->ParentDirectory.HighPart );

    // If the parent FRS is not the root, initialize and read it,
    // and then recurse into it.
    //
    if( ParentFileNumber != ROOT_FILE_NAME_INDEX_NUMBER &&
        (!ParentFrs.Initialize( ParentFileNumber, _Mft ) ||
         !ParentFrs.Read() ||
         !ParentFrs.Backtrack( Path ) ) ) {

        return FALSE;
    }

    // Now add this FRS's name to the path.
    //
    if( !Backslash.Initialize( "\\" ) ||
        !FileNameString.Initialize( NtfsFileNameGetName( FileName ),
                                    FileName->FileNameLength ) ||
        !Path->Strcat( &Backslash ) ||
        !Path->Strcat( &FileNameString ) ) {

        return FALSE;
    }

    return TRUE;
}

 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::AddChild(
    PNTFS_FILE_RECORD_SEGMENT ChildFrs
    )
/*++

Routine Description:

    This method adds a child File Record Segment to the list of
    children.  Management of that File Record Segment then passes
    to the list.

Arguments:

    ChildFrs    -- supplies a pointer to the child File Record Segment.

Return Value:

    TRUE upon successful completion.

--*/
{
    _ChildIterator->Reset();

    return( _Children.Insert( ChildFrs, _ChildIterator ) );
}


 
PNTFS_FILE_RECORD_SEGMENT
NTFS_FILE_RECORD_SEGMENT::GetChild(
    VCN FileNumber
    )
/*++

Routine Description:

    This method finds a File Record Segment in the list of children
    based on its File Number.

Arguments:

    FileNumber  -- supplies the file number of the desired child.

Return Value:

    A pointer to the desired File Record Segment.  Note that this
    object belongs to the child list, and should not be deleted by
    the client.

    NULL if the desired child could not be found in the list.

--*/
{
    PNTFS_FILE_RECORD_SEGMENT CurrentChild;

    // Spin through the list of children until we run out or find
    // one with the appropriate VCN.

    _ChildIterator->Reset();

    while( (CurrentChild =
            (PNTFS_FILE_RECORD_SEGMENT)_ChildIterator->GetNext()) != NULL &&
           CurrentChild->QueryFileNumber() != FileNumber );

    // If there is a matching child in the list, CurrentChild now points
    // at it; otherwise, CurrentChild is NULL.

    return CurrentChild;
}

 
PNTFS_FILE_RECORD_SEGMENT
NTFS_FILE_RECORD_SEGMENT::SetupChild(
    IN VCN  FileNumber
    )
/*++

Routine Description:

    This method sets up a child FRS.  If the desired child is already
    in the list, it is returned; otherwise, it is allocated and read
    and added to the list, and the returned.

Arguments:

    FileNumber  --  Supplies the file number of the desired child.

Return Value:

    A pointer to the child FRS, or NULL to indicate error.

--*/
{
    PNTFS_FILE_RECORD_SEGMENT ChildFrs;
    PNTFS_MFT_FILE MftFile;

    if( (ChildFrs = GetChild( FileNumber )) != NULL ) {

        // The child is already in the list.
        //
        return ChildFrs;
    }

    // Allocate a new FRS object, initialize it to be the
    // child we want, read it in, and add it to the list.
    //
    if( (ChildFrs = NEW NTFS_FILE_RECORD_SEGMENT) == NULL ) {

        return NULL;
    }

    if( _Mft != NULL ) {

        // This is an ordinary, run-of-the-mill File Record
        // Segment, so just initialize the child with the
        // same MFT as this object was initialized with.
        //
        if( !ChildFrs->Initialize( FileNumber, _Mft ) ||
            !ChildFrs->Read() ||
            !AddChild( ChildFrs ) ) {

            DELETE( ChildFrs );
            return NULL;
        }

    } else {

        // This File Record Segment is really the
        // MFT file itself, so we have to do some
        // arcane gesticulation.  Since we know this
        // is really an NTFS_MFT_FILE object, we'll
        // dynamically cast it to that class, and
        // then pass it in as the NTFS_MFT_FILE to
        // initialize the child.
        //
        if( QueryClassId() != NTFS_MFT_FILE_cd->QueryClassId() ) {

            DELETE( ChildFrs );
            return FALSE;
        }

        MftFile = (PNTFS_MFT_FILE)( this );

        if( !ChildFrs->Initialize( FileNumber, MftFile ) ||
            !ChildFrs->Read() ||
            !AddChild( ChildFrs ) ) {

            DELETE( ChildFrs );
            return FALSE;
        }
    }

    return ChildFrs;
}

 
VOID
NTFS_FILE_RECORD_SEGMENT::DeleteChild(
    VCN FileNumber
    )
/*++

Routine Description:

    This method removes a File Record Segment from the list of
    children based on its File Number.

Arguments:

    FileNumber  -- supplies the file number of the child to delete.

Return Value:

    None.

Notes:

    Since the list manages the File Record Segments which it has been
    given, it deletes the File Record Segment in question.

    This method assumes that only one matching child exists (since the
    same File Record Segment should not appear twice in the list).

--*/
{
    PNTFS_FILE_RECORD_SEGMENT CurrentChild, ChildToDelete;

    // Spin through the list of children until we run out or find
    // one with the appropriate VCN.

    _ChildIterator->Reset();

    while( (CurrentChild =
            (PNTFS_FILE_RECORD_SEGMENT)_ChildIterator->GetNext()) != NULL &&
           CurrentChild->QueryFileNumber() != FileNumber );

    // If there is a matching child in the list, the iterator's current
    // state points at it and CurrentChild is non-NULL.

    if( CurrentChild != NULL ) {

        // A matching child was found; remove it from the list
        // and delete it.

        ChildToDelete = (PNTFS_FILE_RECORD_SEGMENT)
                        _Children.Remove( _ChildIterator );

        DebugAssert( ChildToDelete == CurrentChild );

        DELETE( ChildToDelete );
    }
}

 
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::QueryAttributeListAttribute (
    OUT PNTFS_ATTRIBUTE     AttrList,
    OUT PBOOLEAN            Error
)
/*++

Routine Description:

   This function fetches the attribute list associated with the
   File Record Segment.

Arguments:

    AttrList    --  Receives (ie. is initialized to) the attribute.  Note
                    that this parameter may be uninitialized on entry, and
                    may be left uninitialized if this method fails.
    Error       --  Receives TRUE if the method fails because of an error.

Return Value:

    TRUE upon successful completion.

Notes:

    If the method returns TRUE, *Error should be ignored.  If it
    returns FALSE, *Error will be set to TRUE if the failure resulted
    from an error (out of memory, corrupt structure); otherwise, the
    caller may assume that the attribute is not present.

    This method assumes the attribute list is local to the File Record Segment.

--*/
{
    NTFS_ATTRIBUTE_RECORD Record;

    DebugPtrAssert( AttrList );
    DebugPtrAssert( Error );

    DebugPtrAssert( _FrsData );

    // Assume innocent until proven guilty:

    *Error = FALSE;

    if( !IsAttributePresent( $ATTRIBUTE_LIST, NULL, TRUE ) ) {

      // there is no matching attribute.

        return FALSE;
    }

    // Now that we've determined that the attribute is present,
    // this method can only fail because of an error.

    *Error = TRUE;


    if( !SetupAttributeList() ) {

        return FALSE;
    }

    // Get the first attribute record.

    if (!QueryAttributeRecord(&Record, $ATTRIBUTE_LIST)) {

        return FALSE;
    }

    // Initialize the Attribute with the first attribute record.

    if ( !AttrList->Initialize( GetDrive(),
                                QueryClusterFactor(),
                                &Record ) ) {

        return FALSE;
    }

    *Error = FALSE;
    return TRUE;
}

BOOLEAN
NTFS_FILE_RECORD_SEGMENT::PurgeAttributeList (
)
/*++

Routine Description:

   This function purges the attribute list off the File Record Segment.

Arguments:

    N/A

Return Value:

    TRUE upon successful completion.

Notes:
   It is the user's responsibility to make sure that all segment reference
   in the attribute list points back to the base frs.

--*/
{
    DELETE(_AttributeList);
    return PurgeAttribute($ATTRIBUTE_LIST, NULL, TRUE);
}
