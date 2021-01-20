/*++

Module Name:

    mftfile.hxx

Abstract:

    This module contains the declarations for the NTFS_MFT_FILE
	class.	The MFT is the root of the file system

--*/

#pragma once

#include "hmem.hxx"

#include "bitfrs.hxx"
#include "clusrun.hxx"
#include "frs.hxx"
#include "attrib.hxx"
#include "ntfsbit.hxx"
#include "mft.hxx"
#include "mftref.hxx"

DECLARE_CLASS( NTFS_MFT_FILE );


class NTFS_MFT_FILE : public NTFS_FILE_RECORD_SEGMENT {

	public:

         
        DECLARE_CONSTRUCTOR( NTFS_MFT_FILE );

		VIRTUAL
         
        ~NTFS_MFT_FILE(
			);

		 
         
        BOOLEAN
		Initialize(
			IN OUT  PLOG_IO_DP_DRIVE    Drive,
			IN      LCN                 Lcn,
			IN      ULONG               ClusterFactor,
            IN      ULONG               FrsSize,
            IN      BIG_INT             VolumeSectors,
            IN OUT  PNTFS_BITMAP        VolumeBitmap    OPTIONAL,
            IN      PNTFS_UPCASE_TABLE  UpcaseTable     OPTIONAL
			);

         
         
        BOOLEAN
        Read(
            );

        BOOLEAN
		Flush(
			);

         
        PNTFS_MASTER_FILE_TABLE
        GetMasterFileTable(
            );

	private:

		 
		VOID
		Construct(
			);

		 
		VOID
		Destroy(
            );

         
        BOOLEAN
        CheckMirrorSize(
            IN OUT PNTFS_ATTRIBUTE  MirrorDataAttribute,
            IN     BOOLEAN          Fix,
            IN OUT PNTFS_BITMAP     VolumeBitmap,
            OUT    PLCN             FirstLcn
            );

         
        BOOLEAN
        WriteMirror(
            IN OUT PNTFS_ATTRIBUTE  MirrorDataAttribute
            );



        LCN                     _FirstLcn;
        NTFS_ATTRIBUTE          _DataAttribute;
        NTFS_BITMAP             _MftBitmap;
        NTFS_MASTER_FILE_TABLE  _Mft;
        PNTFS_BITMAP            _VolumeBitmap;

        HMEM                    _MirrorMem;
        NTFS_CLUSTER_RUN        _MirrorClusterRun;

};




INLINE
PNTFS_MASTER_FILE_TABLE
NTFS_MFT_FILE::GetMasterFileTable(
    )
/*++

Routine Description:

    This routine returns a pointer to master file table.

Arguments:

    None.

Return Value:

    A pointer to the master file table.

--*/
{
    return _Mft.AreMethodsEnabled() ? &_Mft : NULL;
}

