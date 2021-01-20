#include "stdafx.h"

/*++

Module Name:

    ulib.cxx

Abstract:

    This module contains run-time, global support for the ULIB class library.
    This support includes:

        - creation of CLASS_DESCRIPTORs
        - Global objects
        - Ulib to Win32 API mapping functions

--*/



#include "ulib.hxx"


#include "system.hxx"
#include "array.hxx"
#include "arrayit.hxx"
#include "bitvect.hxx"
#include "message.hxx"
#include "wstring.hxx"
#include "path.hxx"

#include <locale.h>



//
//  Declare class descriptors for all classes.
//

DECLARE_CLASS(  CLASS_DESCRIPTOR        );
DECLARE_CLASS(  ARRAY                   );
DECLARE_CLASS(  ARRAY_ITERATOR          );
DECLARE_CLASS(  BITVECTOR               );
DECLARE_CLASS(  CONTAINER               );
DECLARE_CLASS(  DSTRING                 );
DECLARE_CLASS(  FSTRING                 );
DECLARE_CLASS(  HMEM                    );
DECLARE_CLASS(  ITERATOR                );
DECLARE_CLASS(  LIST                    );
DECLARE_CLASS(  LIST_ITERATOR           );
DECLARE_CLASS(  MEM                     );
DECLARE_CLASS(  MESSAGE                 );
DECLARE_CLASS(  OBJECT                  );
DECLARE_CLASS(  PATH                    );
DECLARE_CLASS(  SEQUENTIAL_CONTAINER    );
DECLARE_CLASS(  SORTABLE_CONTAINER      );
DECLARE_CLASS(  WSTRING                 );
DECLARE_CLASS(  STATIC_MEM_BLOCK_MGR    );
DECLARE_CLASS(  MEM_ALLOCATOR           );
DECLARE_CLASS(  MEM_BLOCK_MGR           );



BOOLEAN
UlibDefineClassDescriptors(
)

/*++

Routine Description:

    Defines all the class descriptors used by ULIB

Arguments:

    None.

Return Value:

    BOOLEAN - Returns TRUE if all class descriptors were succesfully
              constructed and initialized.

--*/

{
    BOOLEAN Success = TRUE;

    if (Success &&
        DEFINE_CLASS_DESCRIPTOR(ARRAY) &&
        DEFINE_CLASS_DESCRIPTOR(ARRAY_ITERATOR) &&
        DEFINE_CLASS_DESCRIPTOR(BITVECTOR) &&
        DEFINE_CLASS_DESCRIPTOR(CONTAINER) &&
        DEFINE_CLASS_DESCRIPTOR(DSTRING) &&
        DEFINE_CLASS_DESCRIPTOR(ITERATOR) &&
        DEFINE_CLASS_DESCRIPTOR(LIST) &&
        DEFINE_CLASS_DESCRIPTOR(LIST_ITERATOR) &&
        DEFINE_CLASS_DESCRIPTOR(PATH) &&
        DEFINE_CLASS_DESCRIPTOR(SEQUENTIAL_CONTAINER) &&
        DEFINE_CLASS_DESCRIPTOR(SORTABLE_CONTAINER) &&
        DEFINE_CLASS_DESCRIPTOR(WSTRING) &&
        DEFINE_CLASS_DESCRIPTOR(MESSAGE) ) {
    }
    else {
        Success = FALSE;
    }

    if (Success &&
        DEFINE_CLASS_DESCRIPTOR(FSTRING) &&
        DEFINE_CLASS_DESCRIPTOR(HMEM) &&
        DEFINE_CLASS_DESCRIPTOR(STATIC_MEM_BLOCK_MGR) &&
        DEFINE_CLASS_DESCRIPTOR(MEM_ALLOCATOR) &&
        DEFINE_CLASS_DESCRIPTOR(MEM_BLOCK_MGR) ) {
    }
    else {
        Success = FALSE;
    }

    if (Success &&
        DEFINE_CLASS_DESCRIPTOR(MEM) ) {
    }
    else {
        Success = FALSE;
    }


    if (!Success) {
        DebugPrint("Could not initialize class descriptors!");
    }
    return Success;

}

BOOLEAN
UlibUndefineClassDescriptors(
)

/*++

Routine Description:

    Defines all the class descriptors used by ULIB

Arguments:

    None.

Return Value:

    BOOLEAN - Returns TRUE if all class descriptors were successfully
              constructed and initialized.

--*/

{
    UNDEFINE_CLASS_DESCRIPTOR(ARRAY);
    UNDEFINE_CLASS_DESCRIPTOR(ARRAY_ITERATOR);
    UNDEFINE_CLASS_DESCRIPTOR(BITVECTOR);
    UNDEFINE_CLASS_DESCRIPTOR(CONTAINER);
    UNDEFINE_CLASS_DESCRIPTOR(DSTRING);
    UNDEFINE_CLASS_DESCRIPTOR(ITERATOR);
    UNDEFINE_CLASS_DESCRIPTOR(LIST);
    UNDEFINE_CLASS_DESCRIPTOR(LIST_ITERATOR);
    UNDEFINE_CLASS_DESCRIPTOR(PATH);
    UNDEFINE_CLASS_DESCRIPTOR(SEQUENTIAL_CONTAINER);
    UNDEFINE_CLASS_DESCRIPTOR(SORTABLE_CONTAINER);
    UNDEFINE_CLASS_DESCRIPTOR(WSTRING);
    UNDEFINE_CLASS_DESCRIPTOR(MESSAGE);
    UNDEFINE_CLASS_DESCRIPTOR(FSTRING);
    UNDEFINE_CLASS_DESCRIPTOR(HMEM);
    UNDEFINE_CLASS_DESCRIPTOR(STATIC_MEM_BLOCK_MGR);
    UNDEFINE_CLASS_DESCRIPTOR(MEM_ALLOCATOR);
    UNDEFINE_CLASS_DESCRIPTOR(MEM_BLOCK_MGR);
    UNDEFINE_CLASS_DESCRIPTOR(MEM);

    return TRUE;

}

