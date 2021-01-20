#include "stdafx.h"

/*++

Module Name:

        untfs.cxx

Abstract:

        This module contains run-time, global support for the
        NTFS IFS Utilities library (UNTFS).  This support includes:

                - creation of CLASS_DESCRIPTORs
                - Global objects
--*/


#include "ulib.hxx"
#include "untfs.hxx"


DECLARE_CLASS( NTFS_ATTRIBUTE );
DECLARE_CLASS( NTFS_ATTRIBUTE_LIST );
DECLARE_CLASS( NTFS_ATTRIBUTE_RECORD );
DECLARE_CLASS( NTFS_BAD_CLUSTER_FILE );
DECLARE_CLASS( NTFS_BITMAP_FILE );
DECLARE_CLASS( NTFS_CLUSTER_RUN );
DECLARE_CLASS( NTFS_EXTENT );
DECLARE_CLASS( NTFS_EXTENT_LIST );
DECLARE_CLASS( NTFS_FILE_RECORD_SEGMENT );
DECLARE_CLASS( NTFS_FRS_STRUCTURE );
DECLARE_CLASS( NTFS_INDEX_BUFFER );
DECLARE_CLASS( NTFS_INDEX_ROOT );
DECLARE_CLASS( NTFS_INDEX_TREE );
DECLARE_CLASS( NTFS_MASTER_FILE_TABLE );
DECLARE_CLASS( NTFS_MFT_FILE );
DECLARE_CLASS( NTFS_REFLECTED_MASTER_FILE_TABLE );
DECLARE_CLASS( NTFS_BITMAP );
DECLARE_CLASS( NTFS_UPCASE_FILE );
DECLARE_CLASS( NTFS_UPCASE_TABLE );
DECLARE_CLASS( NTFS_VOL );
DECLARE_CLASS( NTFS_SA );

BOOLEAN
UntfsDefineClassDescriptors(
        )
{
        if( 
        DEFINE_CLASS_DESCRIPTOR( NTFS_ATTRIBUTE                     ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_ATTRIBUTE_LIST                ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_ATTRIBUTE_RECORD              ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_BAD_CLUSTER_FILE              ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_BITMAP_FILE                   ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_CLUSTER_RUN                   ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_EXTENT                        ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_EXTENT_LIST                   ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_FILE_RECORD_SEGMENT           ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_FRS_STRUCTURE                 ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_INDEX_BUFFER                  ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_INDEX_ROOT                    ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_INDEX_TREE                    ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_MASTER_FILE_TABLE             ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_MFT_FILE                      ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_REFLECTED_MASTER_FILE_TABLE   ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_UPCASE_FILE                   ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_UPCASE_TABLE                  ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_VOL                           ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_SA                            ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_BITMAP                        ) ) {

                return TRUE;

        } else {

                DebugPrint( "Could not initialize class descriptors!");
                return FALSE;
        }
}

BOOLEAN
UntfsUndefineClassDescriptors(
        )
{
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_ATTRIBUTE                     );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_ATTRIBUTE_LIST                );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_ATTRIBUTE_RECORD              );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_BAD_CLUSTER_FILE              );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_BITMAP_FILE                   );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_CLUSTER_RUN                   );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_EXTENT                        );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_EXTENT_LIST                   );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_FILE_RECORD_SEGMENT           );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_FRS_STRUCTURE                 );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_INDEX_BUFFER                  );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_INDEX_ROOT                    );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_INDEX_TREE                    );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_MASTER_FILE_TABLE             );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_MFT_FILE                      );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_REFLECTED_MASTER_FILE_TABLE   );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_UPCASE_FILE                   );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_UPCASE_TABLE                  );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_VOL                           );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_SA                            );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_BITMAP                        );
    return TRUE;
}
