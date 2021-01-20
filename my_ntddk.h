#pragma once


#define MAXULONG    0xffffffff  // winnt

//
// device characteristics flags
//

#define FILE_REMOVABLE_MEDIA                        0x00000001
#define FILE_REMOTE_DEVICE                          0x00000010

typedef LONG NTSTATUS;

//
// Generic test for success on any status value (non-negative numbers
// indicate success).
//

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

//
// Generic test for error on any status value.
//

#ifndef NT_ERROR
#define NT_ERROR(Status) ((((ULONG)(Status)) >> 30) == 3)
#endif

#ifndef InitializeObjectAttributes
#define InitializeObjectAttributes( p, n, a, r, s ) { \
    (p)->Length = sizeof( OBJECT_ATTRIBUTES );          \
    (p)->RootDirectory = r;                             \
    (p)->Attributes = a;                                \
    (p)->ObjectName = n;                                \
    (p)->SecurityDescriptor = s;                        \
    (p)->SecurityQualityOfService = NULL;               \
    }
#endif

//
// Valid values for the Attributes field
//

#define OBJ_CASE_INSENSITIVE                0x00000040L

//
// Define the create/open option flags
//

#define FILE_SYNCHRONOUS_IO_ALERT               0x00000010



typedef struct _IO_STATUS_BLOCK {
#pragma warning(push)
#pragma warning(disable: 4201)
    union {
        NTSTATUS Status;
        PVOID Pointer;
    } DUMMYUNIONNAME;
#pragma warning(pop)

    ULONG_PTR Information;
} IO_STATUS_BLOCK, * PIO_STATUS_BLOCK;



typedef
VOID
(NTAPI* PIO_APC_ROUTINE) (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved
    );

typedef struct _REPARSE_DATA_BUFFER {
    ULONG  ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;


    union {
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            ULONG Flags;
            WCHAR PathBuffer[1];
        } SymbolicLinkReparseBuffer;
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            WCHAR PathBuffer[1];
        } MountPointReparseBuffer;
        struct {
            UCHAR  DataBuffer[1];
        } GenericReparseBuffer;
    } DUMMYUNIONNAME;
} REPARSE_DATA_BUFFER, * PREPARSE_DATA_BUFFER;


//================ FileAlignmentInformation ===================================

typedef struct _FILE_ALIGNMENT_INFORMATION {
    ULONG AlignmentRequirement;
} FILE_ALIGNMENT_INFORMATION, * PFILE_ALIGNMENT_INFORMATION;



typedef struct _FILE_FS_DEVICE_INFORMATION {
    DEVICE_TYPE DeviceType;
    ULONG Characteristics;
} FILE_FS_DEVICE_INFORMATION, * PFILE_FS_DEVICE_INFORMATION;


typedef enum _FSINFOCLASS {
    FileFsVolumeInformation = 1,
    FileFsLabelInformation,         // 2
    FileFsSizeInformation,          // 3
    FileFsDeviceInformation,        // 4
    FileFsAttributeInformation,     // 5
    FileFsControlInformation,       // 6
    FileFsFullSizeInformation,      // 7
    FileFsObjectIdInformation,      // 8
    FileFsDriverPathInformation,    // 9
    FileFsVolumeFlagsInformation,   // 10
    FileFsSectorSizeInformation,    // 11
    FileFsDataCopyInformation,      // 12
    FileFsMetadataSizeInformation,  // 13
    FileFsFullSizeInformationEx,    // 14
    FileFsMaximumInformation
} FS_INFORMATION_CLASS, * PFS_INFORMATION_CLASS;



//
// Define the file information class values
//
// WARNING:  The order of the following values are assumed by the I/O system.
//           Any changes made here should be reflected there as well.
//

typedef enum _FILE_INFORMATION_CLASS {
    FileDirectoryInformation = 1,
    FileFullDirectoryInformation,                   // 2
    FileBothDirectoryInformation,                   // 3
    FileBasicInformation,                           // 4
    FileStandardInformation,                        // 5
    FileInternalInformation,                        // 6
    FileEaInformation,                              // 7
    FileAccessInformation,                          // 8
    FileNameInformation,                            // 9
    FileRenameInformation,                          // 10
    FileLinkInformation,                            // 11
    FileNamesInformation,                           // 12
    FileDispositionInformation,                     // 13
    FilePositionInformation,                        // 14
    FileFullEaInformation,                          // 15
    FileModeInformation,                            // 16
    FileAlignmentInformation,                       // 17
    FileAllInformation,                             // 18
    FileAllocationInformation,                      // 19
    FileEndOfFileInformation,                       // 20
    FileAlternateNameInformation,                   // 21
    FileStreamInformation,                          // 22
    FilePipeInformation,                            // 23
    FilePipeLocalInformation,                       // 24
    FilePipeRemoteInformation,                      // 25
    FileMailslotQueryInformation,                   // 26
    FileMailslotSetInformation,                     // 27
    FileCompressionInformation,                     // 28
    FileObjectIdInformation,                        // 29
    FileCompletionInformation,                      // 30
    FileMoveClusterInformation,                     // 31
    FileQuotaInformation,                           // 32
    FileReparsePointInformation,                    // 33
    FileNetworkOpenInformation,                     // 34
    FileAttributeTagInformation,                    // 35
    FileTrackingInformation,                        // 36
    FileIdBothDirectoryInformation,                 // 37
    FileIdFullDirectoryInformation,                 // 38
    FileValidDataLengthInformation,                 // 39
    FileShortNameInformation,                       // 40
    FileIoCompletionNotificationInformation,        // 41
    FileIoStatusBlockRangeInformation,              // 42
    FileIoPriorityHintInformation,                  // 43
    FileSfioReserveInformation,                     // 44
    FileSfioVolumeInformation,                      // 45
    FileHardLinkInformation,                        // 46
    FileProcessIdsUsingFileInformation,             // 47
    FileNormalizedNameInformation,                  // 48
    FileNetworkPhysicalNameInformation,             // 49
    FileIdGlobalTxDirectoryInformation,             // 50
    FileIsRemoteDeviceInformation,                  // 51
    FileUnusedInformation,                          // 52
    FileNumaNodeInformation,                        // 53
    FileStandardLinkInformation,                    // 54
    FileRemoteProtocolInformation,                  // 55

    //
    //  These are special versions of these operations (defined earlier)
    //  which can be used by kernel mode drivers only to bypass security
    //  access checks for Rename and HardLink operations.  These operations
    //  are only recognized by the IOManager, a file system should never
    //  receive these.
    //

    FileRenameInformationBypassAccessCheck,         // 56
    FileLinkInformationBypassAccessCheck,           // 57

    //
    // End of special information classes reserved for IOManager.
    //

    FileVolumeNameInformation,                      // 58
    FileIdInformation,                              // 59
    FileIdExtdDirectoryInformation,                 // 60
    FileReplaceCompletionInformation,               // 61
    FileHardLinkFullIdInformation,                  // 62
    FileIdExtdBothDirectoryInformation,             // 63
    FileDispositionInformationEx,                   // 64
    FileRenameInformationEx,                        // 65
    FileRenameInformationExBypassAccessCheck,       // 66
    FileDesiredStorageClassInformation,             // 67
    FileStatInformation,                            // 68
    FileMemoryPartitionInformation,                 // 69
    FileStatLxInformation,                          // 70
    FileCaseSensitiveInformation,                   // 71
    FileLinkInformationEx,                          // 72
    FileLinkInformationExBypassAccessCheck,         // 73
    FileStorageReserveIdInformation,                // 74
    FileCaseSensitiveInformationForceAccessCheck,   // 75

    FileMaximumInformation
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;



//
// Interrupt Request Level (IRQL)
//
typedef UCHAR KIRQL;



//
// These data structures and type definitions are needed for compilation and
// use of the internal Windows APIs defined in this header.
//


typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING;
typedef UNICODE_STRING* PUNICODE_STRING;


typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;
    PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES;
typedef OBJECT_ATTRIBUTES* POBJECT_ATTRIBUTES;


//
// These functions are intended for use by internal core Windows components
// since these functions may change between Windows releases.
//

#define RtlMoveMemory(Destination,Source,Length) memmove((Destination),(Source),(Length))




#ifdef __cplusplus
extern "C" {
#endif



NTSYSCALLAPI
NTSTATUS
NTAPI
NtReadFile(
    IN  HANDLE FileHandle,
    IN  HANDLE Event,
    IN  PIO_APC_ROUTINE ApcRoutine,
    IN  PVOID ApcContext,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN  PLARGE_INTEGER ByteOffset,
    IN  PULONG Key);



NTSYSAPI
NTSTATUS
NTAPI
RtlOemToUnicodeN(
    OUT PWCH UnicodeString,
    IN ULONG MaxBytesInUnicodeString,
    OUT PULONG BytesInUnicodeString,
    IN PCCH OemString,
    IN ULONG BytesInOemString
);


NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToOemN(
    OUT PCHAR OemString,
    IN ULONG MaxBytesInOemString,
    OUT PULONG BytesInOemString,
    IN PCWCH UnicodeString,
    IN ULONG BytesInUnicodeString
);


NTSYSAPI
NTSTATUS
NTAPI
RtlDecompressBuffer(
    OUT USHORT CompressionFormat,
    OUT PUCHAR UncompressedBuffer,
    OUT ULONG UncompressedBufferSize,
    IN PUCHAR CompressedBuffer,
    OUT ULONG CompressedBufferSize,
    OUT PULONG FinalUncompressedSize
);


NTSYSAPI
NTSTATUS
NTAPI
NtFsControlFile(
    OUT HANDLE FileHandle,
    IN HANDLE Event,
    IN PIO_APC_ROUTINE ApcRoutine,
    IN PVOID ApcContext,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT ULONG FsControlCode,
    IN PVOID InputBuffer,
    OUT ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    OUT ULONG OutputBufferLength
);



NTSYSAPI
NTSTATUS
NTAPI
NtQueryVolumeInformationFile(
    OUT HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FsInformation,
    OUT ULONG Length,
    OUT FS_INFORMATION_CLASS FsInformationClass
);


NTSYSAPI
NTSTATUS
NTAPI
NtWriteFile(
    OUT HANDLE FileHandle,
    IN HANDLE Event,
    IN PIO_APC_ROUTINE ApcRoutine,
    IN PVOID ApcContext,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    OUT ULONG Length,
    IN PLARGE_INTEGER ByteOffset,
    IN PULONG Key
);




NTSYSAPI
NTSTATUS
NTAPI
NtQueryInformationFile(
    OUT HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    OUT ULONG Length,
    OUT FILE_INFORMATION_CLASS FileInformationClass
);


NTSYSAPI
VOID
NTAPI
RtlRaiseStatus(
    IN NTSTATUS Status
);




NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetThreadExecutionState(
    IN EXECUTION_STATE esFlags,               // ES_xxx flags
    OUT EXECUTION_STATE *PreviousFlags
    );


//
// use the Win32 API instead
//     CloseHandle
//
NTSTATUS
    NTAPI
    NtClose(
        IN HANDLE Handle
    );


//
// use the Win32 API instead
//     CreateFile
//
NTSTATUS
    NTAPI
    NtOpenFile(
        OUT PHANDLE FileHandle,
        IN ACCESS_MASK DesiredAccess,
        IN POBJECT_ATTRIBUTES ObjectAttributes,
        OUT PIO_STATUS_BLOCK IoStatusBlock,
        IN ULONG ShareAccess,
        IN ULONG OpenOptions
    );




//
// use the Win32 API instead
//     DeviceIoControl
//
NTSTATUS
    NTAPI
    NtDeviceIoControlFile(
        IN HANDLE FileHandle,
        IN HANDLE Event OPTIONAL,
        IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
        IN PVOID ApcContext OPTIONAL,
        OUT PIO_STATUS_BLOCK IoStatusBlock,
        IN ULONG IoControlCode,
        IN PVOID InputBuffer OPTIONAL,
        IN ULONG InputBufferLength,
        OUT PVOID OutputBuffer OPTIONAL,
        IN ULONG OutputBufferLength
    );

#ifdef __cplusplus
}
#endif







