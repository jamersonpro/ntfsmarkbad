/*++

Module Name:

	bitfrs.hxx

Abstract:

	This module contains the declarations for the NTFS_BITMAP_FILE
	class, which models the bitmap file for an NTFS volume.

--*/

#pragma once

#include "frs.hxx"

class NTFS_BITMAP_FILE : public NTFS_FILE_RECORD_SEGMENT {

	public:

         
        DECLARE_CONSTRUCTOR( NTFS_BITMAP_FILE );

		VIRTUAL
        ~NTFS_BITMAP_FILE(
			);

		 
         
        BOOLEAN
		Initialize(
        	IN OUT  PNTFS_MASTER_FILE_TABLE	Mft
			);

	private:

		 
		VOID
		Construct(
			);

		 
		VOID
		Destroy(
			);

};

