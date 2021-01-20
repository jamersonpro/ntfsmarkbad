/*++

Module Name:

        Object.hxx

Abstract:

        This module contains the class declaration for the OBJECT class,
        the root of the Ulib hierarchy.  OBJECT is a very important class as
        it provides default (and sometimes  ) implementations for
        helping to identify and classify all other objects at run-time. This
        capability allows CONTAINERs to manipulate OBJECTs rather than a given
        derived class of objects. OBJECTs supply the most primitive support for
        polymorphism within the library. Along with CLASS_DESCRIPTORs they supply
        the ability to safely determine (Cast) if an OBJECT is of a given class
        at run-time.

--*/

#pragma once

#include "clasdesc.hxx"

DECLARE_CLASS( OBJECT );


        #define DECLARE_OBJECT_DBG_FUNCTIONS

        #define DEFINE_OBJECT_DBG_FUNCTIONS

        #define DbgDump( pobj )

        #define DbgDumpAll( pobj )

        #define DbgDumpDeep( pobj, flag )


class OBJECT {

        public:

        VIRTUAL
        ~OBJECT(
            );

        VIRTUAL
        LONG
        Compare(
            IN PCOBJECT     Object
            ) CONST;

         
        PCCLASS_DESCRIPTOR
        GetClassDescriptor(
            ) CONST;

         
        BOOLEAN
        IsSameClass(
            IN PCOBJECT             Object
            ) CONST;

         
        BOOLEAN
        IsSameObject(
            IN PCOBJECT             Object
            ) CONST;

         
        CLASS_ID
        QueryClassId(
            ) CONST;

        DECLARE_OBJECT_DBG_FUNCTIONS

        protected:

         
        OBJECT(
            );

         
        VOID
        Construct(
            );

         
        VOID
        SetClassDescriptor(
            IN PCCLASS_DESCRIPTOR   ClassDescriptor
            );

    private:

        PCCLASS_DESCRIPTOR      _ClassDescriptor;

};


INLINE
VOID
OBJECT::Construct(
    )
{
}


INLINE
CLASS_ID
OBJECT::QueryClassId (
        ) CONST

/*++

Routine Description:

        Return the CLASSID for this object.

Arguments:

        None.

Return Value:

        CLASSID - The CLASSID as maintained by the object's CLASS_DESCRIPTOR.

Notes:

        This function used to be at the end of clasdesac.hxx with th following
        note:

        This member functions is for OBJECT, not for CLASS_DESCRIPTOR. It is
        defined here because it requires CLASS_DESCRIPTOR to be defined in
        order to make it inline.

--*/

{
        DebugPtrAssert( _ClassDescriptor );

    return _ClassDescriptor->QueryClassId();
}


INLINE
VOID
OBJECT::SetClassDescriptor (
        IN PCCLASS_DESCRIPTOR   ClassDescriptor
        )

/*++

Routine Description:

        Set the CLASS_DESCRIPTOR for a derived class.

Arguments:

        ClassDescriptor - Supplies a pointer to the derived class'
                CLASS_DECRIPTOR.

Return Value:

        None.

Notes:

        This function should only be called by the DEFINE_CONSTRUCTOR macro.

--*/

{
//        DebugPtrAssert( ClassDescriptor );
        _ClassDescriptor = ClassDescriptor;
}



INLINE
PCCLASS_DESCRIPTOR
OBJECT::GetClassDescriptor (
        ) CONST

/*++

Routine Description:

        Gain access to the object's CLASS_DESCRIPTOR.

Arguments:

        None.

Return Value:

        PCCLASS_DESCRIPTOR - A pointer to the CLASS_DESCRIPTOR asscoicated with
                this object.

--*/

{
        DebugPtrAssert( _ClassDescriptor );

        return( _ClassDescriptor );
}


INLINE
BOOLEAN
OBJECT::IsSameClass (
        IN PCOBJECT             Object
        ) CONST

/*++

Routine Description:

        Determine if this object and the supplied object are of the same class
        by checking their CLASS_IDs for equality.

Arguments:

        Object - Supplies the object to compare class types against.

Return Value:

        BOOLEAN - Returns TRUE if this object and the supplied object are of
                the same class.

--*/

{
        DebugPtrAssert( Object );

    return (BOOLEAN) (QueryClassId() == Object->QueryClassId());
}


INLINE
BOOLEAN
OBJECT::IsSameObject (
        IN PCOBJECT             Object
        ) CONST

/*++

Routine Description:

        Determine if this object and the supplied object are the same by checking
        their CLASS_IDs and then their memory location.

Arguments:

        Object - Supplies the object to compare for equivalence.

Return Value:

        BOOLEAN - Returns TRUE if this object and the supplied object are
                equivalent.

--*/

{
    return (BOOLEAN) (this == Object);
}


