#include "stdafx.h"





#include "ulib.hxx"

#include "untfs.hxx"
#include "frsstruc.hxx"
#include "mem.hxx"
#include "attrib.hxx"
#include "drive.hxx"
#include "clusrun.hxx"
#include "wstring.hxx"
#include "message.hxx"

#include "attrrec.hxx"
#include "attrlist.hxx"
#include "ntfsbit.hxx"
#include "bigint.hxx"
#include "numset.hxx"


DEFINE_CONSTRUCTOR( NTFS_FRS_STRUCTURE, OBJECT   );


 
NTFS_FRS_STRUCTURE::~NTFS_FRS_STRUCTURE(
    )
/*++

Routine Description:

    Destructor for NTFS_FRS_STRUCTURE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


VOID
NTFS_FRS_STRUCTURE::Construct(
    )
/*++

Routine Description:

    This routine initialize this class to a default state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _FrsData = NULL;
    _secrun = NULL;
    _mftdata = NULL;
    _file_number = 0;
    _cluster_factor = 0;
    _size = 0;
    _drive = NULL;
    _volume_sectors = 0;
    _upcase_table = NULL;
    _first_file_number = 0;
    _frs_count = 0;
    _frs_state = _read_status = FALSE;
    _usa_check = UpdateSequenceArrayCheckValueOk;
}



VOID
NTFS_FRS_STRUCTURE::Destroy(
    )
/*++

Routine Description:

    This routine returns this class to a default state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _FrsData = NULL;
    DELETE(_secrun);
    _mftdata = NULL;
    _file_number = 0;
    _cluster_factor = 0;
    _size = 0;
    _drive = NULL;
    _volume_sectors = 0;
    _upcase_table = NULL;
    _first_file_number = 0;
    _frs_count = 0;
    _frs_state = _read_status = FALSE;
}


 
BOOLEAN
NTFS_FRS_STRUCTURE::Initialize(
    IN OUT  PMEM                Mem,
    IN OUT  PNTFS_ATTRIBUTE     MftData,
    IN      VCN                 FileNumber,
    IN      ULONG               ClusterFactor,
    IN      BIG_INT             VolumeSectors,
    IN      ULONG               FrsSize,
    IN      PNTFS_UPCASE_TABLE  UpcaseTable
    )
/*++

Routine Description:

    This routine initializes a NTFS_FRS_STRUCTURE to a valid
    initial state.

Arguments:

    Mem             - Supplies memory for the FRS.
    MftData         - Supplies the $DATA attribute of the MFT.
    FileNumber      - Supplies the file number for this FRS.
    ClusterFactor   - Supplies the number of sectors per cluster.
    VolumeSectors   - Supplies the number of volume sectors.
    FrsSize         - Supplies the size of each frs, in bytes.
    UpcaseTable     - Supplies the volume upcase table.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

Notes:

    The client may supply NULL for the upcase table, but then
    it cannot manipulate named attributes until the ucpase
    table is set.

--*/
{
    Destroy();

    DebugAssert(Mem);
    DebugAssert(MftData);
    DebugAssert(ClusterFactor);

    _mftdata = MftData;
    _file_number = FileNumber;
    _cluster_factor = ClusterFactor;
    _drive = MftData->GetDrive();
    _size = FrsSize;
    _volume_sectors = VolumeSectors;
    _upcase_table = UpcaseTable;
    _usa_check = UpdateSequenceArrayCheckValueOk;

    DebugAssert(_drive);
    DebugAssert(_drive->QuerySectorSize());

    _FrsData = (PFILE_RECORD_SEGMENT_HEADER)
               Mem->Acquire(QuerySize(), _drive->QueryAlignmentMask());

    if (!_FrsData) {
        Destroy();
        return FALSE;
    }

    return TRUE;
}


 
BOOLEAN
NTFS_FRS_STRUCTURE::Initialize(
    IN OUT  PMEM                Mem,
    IN OUT  PNTFS_ATTRIBUTE     MftData,
    IN      VCN                 FirstFileNumber,
    IN      ULONG               FrsCount,
    IN      ULONG               ClusterFactor,
    IN      BIG_INT             VolumeSectors,
    IN      ULONG               FrsSize,
    IN      PNTFS_UPCASE_TABLE  UpcaseTable
    )
/*++

Routine Description:

    This routine initializes a NTFS_FRS_STRUCTURE to a valid
    initial state in preparation for reading a block of FRS'es.

Arguments:

    Mem             - Supplies memory for the FRS.
    MftData         - Supplies the $DATA attribute of the MFT.
    FirstFileNumber - Supplies the first file number for this FRS block.
    FrsCount        - Supplies the number of FRS'es in this FRS block.
    ClusterFactor   - Supplies the number of sectors per cluster.
    VolumeSectors   - Supplies the number of volume sectors.
    FrsSize         - Supplies the size of each frs, in bytes.
    UpcaseTable     - Supplies the volume upcase table.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

Notes:

    The client may supply NULL for the upcase table, but then
    it cannot manipulate named attributes until the ucpase
    table is set.

--*/
{
    Destroy();

    DebugAssert(Mem);
    DebugAssert(MftData);
    DebugAssert(ClusterFactor);

    _mftdata = MftData;
    _file_number = FirstFileNumber;
    _first_file_number = FirstFileNumber;
    _frs_count = FrsCount;
    _frs_state = _read_status = FALSE;
    _cluster_factor = ClusterFactor;
    _drive = MftData->GetDrive();
    _size = FrsSize;
    _volume_sectors = VolumeSectors;
    _upcase_table = UpcaseTable;
    _usa_check = UpdateSequenceArrayCheckValueOk;

    DebugAssert(_drive);
    DebugAssert(_drive->QuerySectorSize());

    _FrsData = (PFILE_RECORD_SEGMENT_HEADER)
               Mem->Acquire(QuerySize()*FrsCount, _drive->QueryAlignmentMask());

    if (!_FrsData) {
        Destroy();
        return FALSE;
    }

    return TRUE;
}


 
BOOLEAN
NTFS_FRS_STRUCTURE::Initialize(
    IN OUT  PMEM                Mem,
    IN OUT  PLOG_IO_DP_DRIVE    Drive,
    IN      LCN                 StartOfFrs,
    IN      ULONG               ClusterFactor,
    IN      BIG_INT             VolumeSectors,
    IN      ULONG               FrsSize,
    IN      PNTFS_UPCASE_TABLE  UpcaseTable,
    IN      ULONG               Offset
    )
/*++

Routine Description:

    This routine initializes an NTFS_FRS_STRUCTURE to point at one
    of the low frs's.

Arguments:

    Mem             - Supplies memory for the FRS.
    Drive           - Supplies the drive.
    StartOfFrs      - Supplies the starting LCN for the frs.
    ClusterFactor   - Supplies the number of sectors per cluster.
    UpcaseTable     - Supplies the volume upcase table.
    FrsSize         - Supplies the size of frs 0 in bytes.
    Offset          - Supplies the offset in the cluster for the frs.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

Notes:

    The client may supply NULL for the upcase table; in that case,
    the FRS cannot manipulate named attributes until the upcase
    table is set.

--*/
{
    Destroy();

    DebugAssert(Mem);
    DebugAssert(Drive);

    _file_number = 0;
    _cluster_factor = ClusterFactor;
    _drive = Drive;
    _size = FrsSize;
    _volume_sectors = VolumeSectors;
    _upcase_table = UpcaseTable;
    _usa_check = UpdateSequenceArrayCheckValueOk;

    //
    // Our SECRUN will need to hold the one or more sectors occupied
    // by this frs.
    //

#define BYTES_TO_SECTORS(bytes, sector_size)  \
    (((bytes) + ((sector_size) - 1))/(sector_size))

    ULONG sectors_per_frs = BYTES_TO_SECTORS(FrsSize,
                                             Drive->QuerySectorSize());

    if (!(_secrun = NEW SECRUN) ||
        !_secrun->Initialize(Mem,
                             Drive,
                             StartOfFrs * QueryClusterFactor() +
                                Offset/Drive->QuerySectorSize(),
                             sectors_per_frs)) {

        Destroy();
        return FALSE;
    }

    _FrsData = (PFILE_RECORD_SEGMENT_HEADER)_secrun->GetBuf();

    DebugAssert(_FrsData);

    return TRUE;
}


BOOLEAN
NTFS_FRS_STRUCTURE::Read(
    )
/*++

Routine Description:

    This routine reads the FRS in from disk.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG           bytes;
    BOOLEAN         r;
    PIO_DP_DRIVE    drive;

    DebugAssert(_mftdata || _secrun);

    if (_mftdata) {
        r = _mftdata->Read(_FrsData,
                           _file_number*QuerySize(),
                           QuerySize(),
                           &bytes) &&
            bytes == QuerySize();

        drive = _mftdata->GetDrive();
    } else {
        r = _secrun->Read();
        drive = _secrun->GetDrive();
    }

    _read_status = r &&
                   (_usa_check =
                    NTFS_SA::PostReadMultiSectorFixup(_FrsData,
                                                      QuerySize(),
                                                      drive,
                                                      _FrsData->FirstFreeByte));

    return _read_status;
}

 
BOOLEAN
NTFS_FRS_STRUCTURE::Write(
    )
/*++

Routine Description:

    This routine writes the FRS to disk.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG   bytes;
    BOOLEAN r;

    DebugAssert(_mftdata || _secrun);

    NTFS_SA::PreWriteMultiSectorFixup(_FrsData, QuerySize());

    if (_mftdata) {

        r = _mftdata->Write(_FrsData,
                            _file_number*QuerySize(),
                            QuerySize(),
                            &bytes,
                            NULL) &&
            bytes == QuerySize();

    } else {
        r = _secrun->Write();
    }

    NTFS_SA::PostReadMultiSectorFixup(_FrsData, QuerySize(), NULL);

    return r;
}


 
PVOID
NTFS_FRS_STRUCTURE::GetNextAttributeRecord(
    IN      PCVOID      AttributeRecord,
    IN OUT  PMESSAGE    Message,
    OUT     PBOOLEAN    ErrorsFound
    )
/*++

Routine Description:

    This routine gets the next attribute record in the file record
    segment assuming that 'AttributeRecord' points to a valid
    attribute record.  If NULL is given as the first argument then
    the first attribute record is returned.

Arguments:

    AttributeRecord - Supplies a pointer to the current attribute record.
    Message         - Supplies an outlet for error processing.
    ErrorsFound     - Supplies whether or not errors were found and
                        corrected in the FRS.

Return Value:

    A pointer to the next attribute record or NULL if there are no more.

--*/
{
    PATTRIBUTE_RECORD_HEADER    p;
    PCHAR                       q;
    PCHAR                       next_frs;
    ULONG                       bytes_free;
    BOOLEAN                     error;

    DebugAssert(_FrsData);

    if (ErrorsFound) {
        *ErrorsFound = FALSE;
    }

    next_frs = (PCHAR) _FrsData + QuerySize();

    if (!AttributeRecord) {

        // Make sure the FirstAttributeOffset field will give us a properly
        // aligned pointer.  If not, bail.
        //

        if (_FrsData->FirstAttributeOffset % 4 != 0) {

            return NULL;
        }

        AttributeRecord = (PCHAR) _FrsData + _FrsData->FirstAttributeOffset;

        p = (PATTRIBUTE_RECORD_HEADER) AttributeRecord;
        q = (PCHAR) AttributeRecord;

        if (q + QuadAlign(sizeof(ATTRIBUTE_TYPE_CODE)) > next_frs) {

            // There is no way to correct this error.
            // The FRS is totally hosed, this will also be detected
            // by VerifyAndFix.  I can't really say *ErrorsFound = TRUE
            // because the error was not corrected.  I also cannot
            // update the firstfreebyte and bytesfree fields.

            return NULL;
        }

        if (p->TypeCode != $END) {

            error = FALSE;
            if (q + sizeof(ATTRIBUTE_TYPE_CODE) + sizeof(ULONG) > next_frs) {

                error = TRUE;
            } else if (!p->RecordLength) {

                error = TRUE;
            } else if (!IsQuadAligned(p->RecordLength)) {

                error = TRUE;
            } else if (q + p->RecordLength + sizeof(ATTRIBUTE_TYPE_CODE) > next_frs) {

                error = TRUE;
            }

            if (error) {

                p->TypeCode = $END;

                bytes_free = (ULONG)(next_frs - q) -
                                QuadAlign(sizeof(ATTRIBUTE_TYPE_CODE));

                _FrsData->FirstFreeByte = QuerySize() - bytes_free;

                if (ErrorsFound) {
                    *ErrorsFound = TRUE;
                }

                if (Message) 
                {
                    Message->OutIncorrectStructure();
                }

                return NULL;
            }
        }

    } else {

        // Assume that the attribute record passed in is good.

        p = (PATTRIBUTE_RECORD_HEADER) AttributeRecord;
        q = (PCHAR) AttributeRecord;

        q += p->RecordLength;
        p = (PATTRIBUTE_RECORD_HEADER) q;
    }


    if (p->TypeCode == $END) 
    {
        // Update the bytes free and first free fields.

        bytes_free = (ULONG)(next_frs - q) - QuadAlign(sizeof(ATTRIBUTE_TYPE_CODE));

        if (_FrsData->FirstFreeByte + bytes_free != QuerySize()) 
        {
            _FrsData->FirstFreeByte = QuerySize() - bytes_free;

            if (ErrorsFound) 
            {
                *ErrorsFound = TRUE;
            }

            if (Message) 
            {
                Message->OutIncorrectStructure();
            }
        }

        return NULL;
    }


    // Make sure the attribute record is good.

    error = FALSE;
    if (q + sizeof(ATTRIBUTE_TYPE_CODE) + sizeof(ULONG) > next_frs) {

        error = TRUE;
    } else if (!p->RecordLength) {

        error = TRUE;
    } else if (!IsQuadAligned(p->RecordLength)) {

        error = TRUE;
    } else if (q + p->RecordLength + QuadAlign(sizeof(ATTRIBUTE_TYPE_CODE)) > next_frs) {

        error = TRUE;
    }

    if (error) {

        p->TypeCode = $END;

        bytes_free = (ULONG)(next_frs - q) - QuadAlign(sizeof(ATTRIBUTE_TYPE_CODE));

        _FrsData->FirstFreeByte = QuerySize() - bytes_free;

        if (ErrorsFound) {
            *ErrorsFound = TRUE;
        }

    	return NULL;
    }

    return p;
}


VOID
NTFS_FRS_STRUCTURE::DeleteAttributeRecord(
    IN OUT  PVOID   AttributeRecord
    )
/*++

Routine Description:

    This routine removes the pointed to attribute record from the
    file record segment.  The pointer passed in will point to
    the next attribute record

Arguments:

    AttributeRecord - Supplies a valid pointer to an attribute record.

Return Value:

    None.

--*/
{
    PATTRIBUTE_RECORD_HEADER    p;
    PCHAR                       end;
    PCHAR                       frs_end;

    DebugAssert(AttributeRecord);

    p = (PATTRIBUTE_RECORD_HEADER) AttributeRecord;

    DebugAssert(p->TypeCode != $END);

    end = ((PCHAR) p) + p->RecordLength;
    frs_end = ((PCHAR) _FrsData) + QuerySize();

    DebugAssert(end < frs_end);

    memmove(p, end, (unsigned int)(frs_end - end));

    // This loop is here to straighten out the attribute records.
    p = NULL;
    while (p = (PATTRIBUTE_RECORD_HEADER) GetNextAttributeRecord(p)) {
    }
}


BOOLEAN
NTFS_FRS_STRUCTURE::QueryAttributeList(
    OUT PNTFS_ATTRIBUTE_LIST    AttributeList
    )
/*++

Routine Description:

    This method fetches the Attribute List Attribute from this
    File Record Segment.

Arguments:

    AttributeList   - Returns A pointer to the Attribute List Attribute.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PATTRIBUTE_RECORD_HEADER    prec;
    NTFS_ATTRIBUTE_RECORD       record;

    return (prec = (PATTRIBUTE_RECORD_HEADER) GetAttributeList()) &&
           record.Initialize(GetDrive(), prec) &&
           AttributeList->Initialize(GetDrive(), QueryClusterFactor(),
                                     &record,
                                     GetUpcaseTable());
}

 
PVOID
NTFS_FRS_STRUCTURE::GetAttribute(
    IN  ULONG   TypeCode
    )
/*++

Routine Description:

    This routine returns a pointer to the unnamed attribute with the
    given type code or NULL if this attribute does not exist.

Arguments:

    TypeCode    - Supplies the type code of the attribute to search for.

Return Value:

    A pointer to an attribute or NULL if there none was found.

--*/
{
    PATTRIBUTE_RECORD_HEADER    prec;

    prec = NULL;
    while (prec = (PATTRIBUTE_RECORD_HEADER) GetNextAttributeRecord(prec)) {

        if (prec->TypeCode == TypeCode &&
            prec->NameLength == 0) {
            break;
        }
    }

    return prec;
}


 
PVOID
NTFS_FRS_STRUCTURE::GetAttributeList(
    )
/*++

Routine Description:

    This routine returns a pointer to the attribute list or NULL if
    there is no attribute list.

Arguments:

    None.

Return Value:

    A pointer to the attribute list or NULL if there is no attribute list.

--*/
{
    return GetAttribute($ATTRIBUTE_LIST);
}

