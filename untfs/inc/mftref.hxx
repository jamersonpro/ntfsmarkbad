/*++

Module Name:

        mftref.hxx

Abstract:

        This module contains the declarations for the
    NTFS_REFLECTED_MASTER_FILE_TABLE class.  This
    class models the backup copy of the Master File
    Table.

--*/

#pragma once

#include "frs.hxx"

DECLARE_CLASS( NTFS_MASTER_FILE_TABLE );

class NTFS_REFLECTED_MASTER_FILE_TABLE : public NTFS_FILE_RECORD_SEGMENT {

        public:

                 
            DECLARE_CONSTRUCTOR( NTFS_REFLECTED_MASTER_FILE_TABLE );

            VIRTUAL
             
            ~NTFS_REFLECTED_MASTER_FILE_TABLE(
                    );

             
     
            BOOLEAN
            Initialize(
                    IN OUT  PNTFS_MASTER_FILE_TABLE Mft
                    );


        private:

                 
                VOID
                Construct(
                        );

                 
                VOID
                Destroy(
                        );

};

