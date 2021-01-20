/*++

Module Name:

        system.hxx

Abstract:

        This module contains the definition for the SYSTEM class. The SYSTEM
        class is an abstract class which offers an interface for communicating
        with the underlying operating system.

--*/

#pragma once

DECLARE_CLASS( WSTRING );

#include "message.hxx"
#include "path.hxx"
#include "ifsentry.hxx"


//
//      Exit codes
//
#define         EXIT_NORMAL                     0
#define         EXIT_NO_FILES                   1
#define         EXIT_TERMINATED                 2
#define         EXIT_MISC_ERROR                 4
#define         EXIT_READWRITE_ERROR            5



//
// Flags that can be specified to FSN_FILE::Copy()
//
#define FSN_FILE_COPY_OVERWRITE_READ_ONLY       (0x0001)
#define FSN_FILE_COPY_RESET_READ_ONLY           (0x0002)
#define FSN_FILE_COPY_RESTARTABLE               (0x0004)
#define FSN_FILE_COPY_COPY_OWNER                (0x0008)
#define FSN_FILE_COPY_COPY_ACL                  (0x0010)
#define FSN_FILE_COPY_ALLOW_DECRYPTED_DESTINATION (0x0020)


class SYSTEM : public OBJECT {

    public:

        STATIC
        BOOLEAN
        QueryCurrentDosDriveName(
            OUT PWSTRING    DosDriveName
            );

};
