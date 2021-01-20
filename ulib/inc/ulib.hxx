/*++

Module Name:

        Ulib.hxx

Abstract:


--*/

#pragma once

#include "../../common.h"




//
//  Intrinsic functions
//
#if DBG==0

    #pragma intrinsic( memset, memcpy, memcmp )

#endif

//
// Here's the scoop...ntdef.h defined NULL to be ( PVOID ) 0.
// Cfront barfs on this if you try and assign NULL to any other pointer type.
// This leaves two options (a) cast all NULL assignments or (b) define NULL
// to be zero which is what C++ expects.
//

#if defined( NULL )

        #undef NULL

#endif
#define NULL    ( 0 )

//
// Make sure const is not defined.
//
#if defined( const )
    #undef const
#endif

#include "ulibdef.hxx"
#include "object.hxx"
#include "clasdesc.hxx"


DECLARE_CLASS( PATH );

extern
BOOLEAN
UlibDefineClassDescriptors();


extern
BOOLEAN
UlibUndefineClassDescriptors();

