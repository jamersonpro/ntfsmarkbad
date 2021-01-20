/*++


Module Name:

    bpb.hxx

Abstract:

    This module contains the declarations for packed and
    unpacked Bios Parameter Block

--*/

#pragma once


// DEFINE THIS UNCHANGED PBPB for use on NTFS drive PBootSector
typedef struct _OLD_PACKED_BIOS_PARAMETER_BLOCK {
    UCHAR  BytesPerSector[2];                       //  offset = 0x000
    UCHAR  SectorsPerCluster[1];                    //  offset = 0x002
    UCHAR  ReservedSectors[2];                      //  offset = 0x003
    UCHAR  Fats[1];                                 //  offset = 0x005
    UCHAR  RootEntries[2];                          //  offset = 0x006
    UCHAR  Sectors[2];                              //  offset = 0x008
    UCHAR  Media[1];                                //  offset = 0x00A
    UCHAR  SectorsPerFat[2];                        //  offset = 0x00B
    UCHAR  SectorsPerTrack[2];                      //  offset = 0x00D
    UCHAR  Heads[2];                                //  offset = 0x00F
    UCHAR  HiddenSectors[4];                        //  offset = 0x011
    UCHAR  LargeSectors[4];                         //  offset = 0x015
} OLD_PACKED_BIOS_PARAMETER_BLOCK;                  // sizeof = 0x019


typedef struct BIOS_PARAMETER_BLOCK {
    USHORT BytesPerSector;
    UCHAR  SectorsPerCluster;
    USHORT ReservedSectors;
    UCHAR  Fats;
    USHORT RootEntries;
    USHORT Sectors;
    UCHAR  Media;
    USHORT SectorsPerFat;
    USHORT SectorsPerTrack;
    USHORT Heads;
    ULONG  HiddenSectors;
    ULONG  LargeSectors;
    ULONG  BigSectorsPerFat;
    USHORT ExtFlags;                            
    USHORT FS_Version;                          
    ULONG  RootDirStrtClus;                     
    USHORT FSInfoSec;                           
    USHORT BkUpBootSec;  
} BIOS_PARAMETER_BLOCK;
typedef BIOS_PARAMETER_BLOCK *PBIOS_PARAMETER_BLOCK;

//
//  The following types and macros are used to help unpack the packed and
//  misaligned fields found in the Bios parameter block
//
typedef union _UCHAR1 {
    UCHAR  Uchar[1];
    UCHAR  ForceAlignment;
} UCHAR1, *PUCHAR1;

typedef union _UCHAR2 {
    UCHAR  Uchar[2];
    USHORT ForceAlignment;
} UCHAR2, *PUCHAR2;

typedef union _UCHAR4 {
    UCHAR  Uchar[4];
    ULONG  ForceAlignment;
} UCHAR4, *PUCHAR4;

#define CopyUchar1(Dst,Src) {                                \
    *((UCHAR1 *)(Dst)) = *((UNALIGNED UCHAR1 *)(Src));       \
}

#define CopyUchar2(Dst,Src) {                                \
    *((UCHAR2 *)(Dst)) = *((UNALIGNED UCHAR2 *)(Src));       \
}

#define CopyU2char(Dst,Src) {                                \
    *((UNALIGNED UCHAR2 *)(Dst)) = *((UCHAR2 *)(Src));       \
}

#define CopyUchar4(Dst,Src) {                                \
    *((UCHAR4 *)(Dst)) = *((UNALIGNED UCHAR4 *)((ULONG_PTR)(Src)));       \
}

#define CopyU4char(Dst, Src) {                               \
    *((UNALIGNED UCHAR4 *)(Dst)) = *((UCHAR4 *)(Src));       \
}


//
//  This macro takes a Packed BPB and fills in its Unpacked equivalent
//


#define UnpackBios(Bios,Pbios) {                                          \
    CopyUchar2(&((Bios)->BytesPerSector),    (Pbios)->BytesPerSector   ); \
    CopyUchar1(&((Bios)->SectorsPerCluster), (Pbios)->SectorsPerCluster); \
    CopyUchar2(&((Bios)->ReservedSectors),   (Pbios)->ReservedSectors  ); \
    CopyUchar1(&((Bios)->Fats),              (Pbios)->Fats             ); \
    CopyUchar2(&((Bios)->RootEntries),       (Pbios)->RootEntries      ); \
    CopyUchar2(&((Bios)->Sectors),           (Pbios)->Sectors          ); \
    CopyUchar1(&((Bios)->Media),             (Pbios)->Media            ); \
    CopyUchar2(&((Bios)->SectorsPerFat),     (Pbios)->SectorsPerFat    ); \
    CopyUchar2(&((Bios)->SectorsPerTrack),   (Pbios)->SectorsPerTrack  ); \
    CopyUchar2(&((Bios)->Heads),             (Pbios)->Heads            ); \
    CopyUchar4(&((Bios)->HiddenSectors),     (Pbios)->HiddenSectors    ); \
    CopyUchar4(&((Bios)->LargeSectors),      (Pbios)->LargeSectors     ); \
}


//
//  This macro takes an Unpacked BPB and fills in its Packed equivalent
//

#define PackBios(Bios,Pbios) {                                            \
    CopyU2char((Pbios)->BytesPerSector,    &((Bios)->BytesPerSector)   ); \
    CopyUchar1((Pbios)->SectorsPerCluster, &((Bios)->SectorsPerCluster)); \
    CopyU2char((Pbios)->ReservedSectors,   &((Bios)->ReservedSectors)  ); \
    CopyUchar1((Pbios)->Fats,              &((Bios)->Fats)             ); \
    CopyU2char((Pbios)->RootEntries,       &((Bios)->RootEntries)      ); \
    CopyU2char((Pbios)->Sectors,           &((Bios)->Sectors)          ); \
    CopyUchar1((Pbios)->Media,             &((Bios)->Media)            ); \
    CopyU2char((Pbios)->SectorsPerFat,     &((Bios)->SectorsPerFat)    ); \
    CopyU2char((Pbios)->SectorsPerTrack,   &((Bios)->SectorsPerTrack)  ); \
    CopyU2char((Pbios)->Heads,             &((Bios)->Heads)            ); \
    CopyU4char((Pbios)->HiddenSectors,     &((Bios)->HiddenSectors)    ); \
    CopyU4char((Pbios)->LargeSectors,      &((Bios)->LargeSectors)     ); \
}

