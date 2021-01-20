/*++

Module Name:

        ulibdef.hxx

Abstract:

        This module contains primitive support for the ULIB class hierarchy
        and it's clients.  This support includes:

                - type definitions
                - manifest constants
                - debugging support
                - memory leakage support
                - external references

--*/

#pragma once


#pragma warning(disable:4091)  // Symbol not defined

//
// Macros for defining types:
//
//  - pointers (Ptype)
//  - pointers to constants (PCtype)
//  - references (Rtype)
//  - references to constants (RCtype)
//

#define DEFINE_POINTER_TYPES( type )                    \
        typedef type*           P##type;                    \
        typedef const type*     PC##type

#define DEFINE_REFERENCE_TYPES( type )                  \
        typedef type&           R##type;                    \
        typedef const type&     RC##type

#define DEFINE_POINTER_AND_REFERENCE_TYPES( type )      \
        DEFINE_POINTER_TYPES( type );                       \
        DEFINE_REFERENCE_TYPES( type )

#define DEFINE_TYPE( basetype, newtype )                                \
        typedef basetype newtype;                                                       \
        DEFINE_POINTER_AND_REFERENCE_TYPES( newtype )

#define DECLARE_CLASS( c )                                      \
        class c;                                \
        DEFINE_POINTER_AND_REFERENCE_TYPES( c );\
        extern PCCLASS_DESCRIPTOR c##_cd

//
// Primitive types.
//

DEFINE_TYPE( unsigned char,     UCHAR   );
DEFINE_TYPE( unsigned short,USHORT      );
DEFINE_TYPE( unsigned long,     ULONG   );

DEFINE_TYPE( char,                      CCHAR   );
DEFINE_TYPE( short,                     CSHORT  );
DEFINE_TYPE( ULONG,                     CLONG   );

DEFINE_TYPE( short,                     SSHORT  );
DEFINE_TYPE( long,                      SLONG   );

DEFINE_TYPE( UCHAR,             BYTE    );
DEFINE_TYPE( char,                      STR     );
DEFINE_TYPE( UCHAR,                     BOOLEAN );
DEFINE_TYPE( int,                       INT     );
DEFINE_TYPE( unsigned int,      UINT    );

#if !defined(_NATIVE_WCHAR_T_DEFINED)
DEFINE_TYPE(USHORT,                     WCHAR   );
#else
typedef wchar_t WCHAR;
#endif

typedef WCHAR *LPWCH;      // pwc
typedef WCHAR *LPWSTR;     // pwsz, 0x0000 terminated UNICODE strings only

DEFINE_POINTER_AND_REFERENCE_TYPES( WCHAR       );

DEFINE_TYPE( WCHAR,                                     WSTR    );
//DEFINE_TYPE( struct tagLC_ID,         LC_ID   );

//
// Augmented (beyond standard headers) VOID pointer types
//

DEFINE_POINTER_TYPES( VOID );

//
// Ulib specific, primitive types
//

DEFINE_TYPE( STR,       CLASS_NAME );
DEFINE_TYPE( ULONG_PTR, CLASS_ID );


//
// Member and non-member function/data modifiers
//

#define CONST       const
#define PURE        = 0
#define STATIC      static
#define VIRTUAL     virtual
#define INLINE      inline
#define FRIEND      friend

//
// Argument modifiers
//

#define DEFAULT     =
#define IN
#define OPTIONAL
#define OUT
#define INOUT
#define REGISTER        register

#if !defined(max)
    #define max(a,b)  (((a) > (b)) ? (a) : (b) )
#endif

#if !defined(min)
    #define min(a,b)  (((a) < (b)) ? (a) : (b) )
#endif



//
// Cast (beProtocol) support
//
// If the ID of the passed object is equal to the ID in this class'
// CLASS_DESCRIPTOR, then the object is of this type and the Cast succeeds
// (i.e. returns Object) otherwise the Cast fails (i.e. returns NULL)
//


#define DECLARE_CAST_MEMBER_FUNCTION( type )                        \
        STATIC                                                      \
        P##type                                                     \
        Cast (                                                      \
            PCOBJECT    Object                                      \
        )


#define DEFINE_CAST_MEMBER_FUNCTION( type )                     \
            P##type                                             \
            type::Cast (                                        \
                PCOBJECT    Object                              \
        )                                                       \
        {                                                       \
            if( Object && ( Object->QueryClassId( ) ==          \
                            type##_cd->QueryClassId( ))) {      \
                    return(( P##type ) Object );                \
            } else {                                            \
                    return NULL;                                \
            }                                                   \
        }


//
// Constructor support
//

//
// All classes have CLASS_DESCRIPTORS which are static and named
// after the class appended with the suffix _cd. They are passed stored in
// OBJECT and are set by the SetClassDescriptor function. The Construct
// For debugging purposes the class' name is stored in the CLASS_DESCRIPTOR.
// The Construct member function gas a no-op implementation in OBJECT and
// could be overloaded as a private member in any derived class.
//

#define DECLARE_CONSTRUCTOR( c )                                \
                                                       \
        c (                                                     \
        )

#define DEFINE_CONSTRUCTOR( d, b )                              \
        PCCLASS_DESCRIPTOR d##_cd;                              \
        d::d (                                                  \
                ) : b( )                                        \
        {                                                       \
                SetClassDescriptor( ##d##_cd );                 \
                Construct( );                                   \
        }



//
// Debug support.
//
// Use the Debug macros to invoke the following debugging functions.
//
//  DebugAbort( str )         - Print a message and abort.
//  DebugAssert( exp )        - Assert that an expression is TRUE. Abort if FALSE.
//  DebugChkHeap( )           - Validate the heap. Abort if invalid.
//  DebugPrint( str )         - Print a string including location (file/line)
//  DebugPrintTrace(fmt, ...) - Printf.
//


#define DebugAbort( str )
#define DebugAssert( exp )
#define DebugCheckHeap( )
#define DebugPrint( str )
#define DebugPrintTrace( M )
#define DebugPtrAssert( ptr )


//
// DELETE macro that NULLifizes the pointer to the deleted object.
//

// Undo any previous definitions of DELETE.
#undef DELETE

// #define DELETE(x) FREE(x), x = NULL;

#define DELETE( x )             \
        delete x, x = NULL

#define DELETE_ARRAY( x )             \
        delete [] x, x = NULL

#define NEW new

#define DumpStats




#define MALLOC(bytes) ((PVOID) LocalAlloc(0, bytes))

#define CALLOC(nitems, size) ((PVOID) LocalAlloc(LMEM_ZEROINIT, nitems*size))

#define REALLOC(x, size) ((PVOID) LocalReAlloc(x, size, LMEM_MOVEABLE))

#define FREE(x) ((x) ? (LocalFree(x), (x) = NULL) : 0)



