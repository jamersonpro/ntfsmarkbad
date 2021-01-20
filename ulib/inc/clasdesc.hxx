/*++

Module Name:

        clasdesc.hxx

Abstract:

        This module contains the declaration for the CLASS_DESCRIPTOR class.
        A CLASS_DESCRIPTOR contains informatiom to help identify an object's
        type at run-time. It is a concrete class which is associated with all
        other concrete classes (i.e. there is one instance of a CLASS_DESCRIPTOR
        for every concrete class in Ulib). The most important aspect of a
        CLASS_DESCRIPTOR is that it supplies a guaranteed unique id for the class
        that it decsribes.

--*/

#pragma once

//
//      Forward references
//

DECLARE_CLASS(  CLASS_DESCRIPTOR                );


#define UNDEFINE_CLASS_DESCRIPTOR( c )  delete (PCLASS_DESCRIPTOR) c##_cd, c##_cd = 0


#define DEFINE_CLASS_DESCRIPTOR( c )                                        \
    ((( c##_cd = NEW CLASS_DESCRIPTOR ) != NULL ) &&        \
    ((( PCLASS_DESCRIPTOR ) c##_cd )->Initialize( )))

    #define CLASS_DESCRIPTOR_INITIALIZE             \
                               \
            BOOLEAN                                                         \
            Initialize (                                            \
                    )

    #define DECLARE_CD_DBG_DATA                                     \
            static char d

    #define DECLARE_CD_DBG_FUNCTIONS                        \
            void f(void){}

    #define DEFINE_CD_INLINE_DBG_FUNCTIONS          \
            inline void f(void){}

    #define DbgGetClassName( pobj )


class CLASS_DESCRIPTOR {

    friend
        BOOLEAN
        InitializeUlib(
            IN HANDLE   DllHandle,
            IN ULONG    Reason,
            IN PVOID    Reserved
        );

public:

    CLASS_DESCRIPTOR(
    );

    CLASS_DESCRIPTOR_INITIALIZE;

    CLASS_ID
        QueryClassId(
        ) CONST;

    DECLARE_CD_DBG_FUNCTIONS;

private:

    DECLARE_CD_DBG_DATA;

    CLASS_ID                _ClassID;
};


INLINE
CLASS_ID
CLASS_DESCRIPTOR::QueryClassId (
        ) CONST

/*++

Routine Description:

        Return the CLASSID for the object described by this CLASS_DESCRIPTOR.

Arguments:

        None.

Return Value:

        CLASSID - The CLASSID as maintained by this CLASS_DESCRIPTOR.

--*/

{
        return( _ClassID );
}


DEFINE_CD_INLINE_DBG_FUNCTIONS;

