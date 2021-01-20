/*++

Module Name:

    mftinfo.hxx

Abstract:

    This module contains the declarations for the NTFS_MFT_INFO
    class, which stores extracted information from the NTFS MFT.

--*/

#pragma once

#include "membmgr2.hxx"

DECLARE_CLASS( NTFS_UPCASE_TABLE );

typedef UCHAR   FILE_NAME_SIGNATURE[4];
typedef UCHAR   DUP_INFO_SIGNATURE[4];

DEFINE_POINTER_AND_REFERENCE_TYPES( FILE_NAME_SIGNATURE );
DEFINE_POINTER_AND_REFERENCE_TYPES( DUP_INFO_SIGNATURE );

struct _NTFS_FILE_NAME_INFO {
    FILE_NAME_SIGNATURE         Signature;
    UCHAR                       Flags;
    UCHAR                       Reserved[3];
};

struct _NTFS_FRS_INFO {
    MFT_SEGMENT_REFERENCE       SegmentReference;
    DUP_INFO_SIGNATURE          DupInfoSignature;
    USHORT                      NumberOfFileNames;
    USHORT                      Reserved;
    struct _NTFS_FILE_NAME_INFO FileNameInfo[1];
};

DEFINE_TYPE( _NTFS_FRS_INFO, NTFS_FRS_INFO );

class NTFS_MFT_INFO : public OBJECT {

    public:

        DECLARE_CONSTRUCTOR( NTFS_MFT_INFO );

        VIRTUAL
        ~NTFS_MFT_INFO(
            );


   private:

         
        VOID
        Construct(
            );

         
        VOID
        Destroy(
            );



        VCN                             _min_file_number;
        VCN                             _max_file_number;

        PVOID                           *_mft_info;
        STATIC PNTFS_UPCASE_TABLE       _upcase_table;
        STATIC UCHAR                    _major;
        STATIC UCHAR                    _minor;
        MEM_ALLOCATOR                   _mem_mgr;
        ULONG64                         _max_mem_use;
        ULONG                           _num_of_files;
};


