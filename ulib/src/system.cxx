#include "stdafx.h"

/*++

Module Name:

    system.cxx

Abstract:

    This contains the implementation for all methods communicating
    with the operating system.

--*/




#include "ulib.hxx"
#include "system.hxx"

extern "C" {
    #include <stdio.h>
    #include <string.h>
    //#include "winbasep.h"
}

#include "path.hxx"
#include "wstring.hxx"


 
BOOLEAN
SYSTEM::QueryCurrentDosDriveName(
    OUT PWSTRING    DosDriveName
    )
/*++

Routine Description:

    This routine returns the name of the current drive.

Arguments:

    DosDriveName    - Returns the name of the current drive.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PATH                path;
    PWSTRING     p;

    if (!path.Initialize( (LPWSTR)L"foo", TRUE)) {
        return FALSE;
    }

    if (!(p = path.QueryDevice())) {
        return FALSE;
    }

    if (!DosDriveName->Initialize(p)) {
        return FALSE;
    }

    DELETE(p);

    return TRUE;
}




