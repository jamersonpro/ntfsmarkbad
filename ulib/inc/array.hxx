/*++

Module Name:

        array.hxx

Abstract:

        This module contains the declaration for the ARRAY class.

    ARRAY is a concrete implementation of a dynamic (i.e. growable) array
    derived from the abstract class SORTABLE_CONTAINER.

    ARRAY's support one 'writer' (i.e. one client poutting OBJECTs into
    the ARRAY) and multiple readers via an ITERATOR. It is the client's
    responsibility to synchronize multiple writers.

    The ARRAY does not contain holes, i.e. all elements of the array (up
    to QueryMemberCount() ) have objects.

--*/

#pragma once

#include "sortcnt.hxx"

//
//      Forward references
//
DECLARE_CLASS( ARRAY );
DECLARE_CLASS( ARRAY_ITERATOR );

//
// Pointer to array of POBJECTs
//
typedef POBJECT*        PPOBJECT;

//
// Default values for an ARRAY object.
//
//      - Capacity is the total number of elements that can be stored in an ARRAY
//      - CapacityIncrement is the number of elemnts that the ARRAY's Capacity
//        will be increased by when it's Capacity is exceeded
//
CONST ULONG             DefaultCapacity                         = 50;
CONST ULONG             DefaultCapacityIncrement        = 25;

//
//  Invalid index within the array
//
CONST ULONG     INVALID_INDEX               = (ULONG)(-1);


class ARRAY : public SORTABLE_CONTAINER {

    friend  ARRAY_ITERATOR;

        public:

         
        DECLARE_CONSTRUCTOR( ARRAY );

        DECLARE_CAST_MEMBER_FUNCTION( ARRAY );

        VIRTUAL
        ~ARRAY(
            );

         
        BOOLEAN
        Initialize (
                IN ULONG        Capacity                        DEFAULT DefaultCapacity,
                IN ULONG        CapacityIncrement       DEFAULT DefaultCapacityIncrement
                );

        VIRTUAL
        BOOLEAN
        DeleteAllMembers(
            );

                VIRTUAL
        POBJECT
                GetAt(
            IN  ULONG   Index
            ) CONST;

        VIRTUAL
        ULONG
        GetMemberIndex(
            IN  POBJECT Object
            ) CONST;

        VIRTUAL
        BOOLEAN
        PutAt(
            IN OUT  POBJECT Member,
            IN      ULONG   Index
            );

         
        ULONG
        QueryCapacity (
            ) CONST;

         
        ULONG
        QueryCapacityIncrement (
            ) CONST;

        VIRTUAL
         
        PITERATOR
        QueryIterator(
            ) CONST;

        VIRTUAL
        ULONG
        QueryMemberCount(
            ) CONST;

        VIRTUAL
         
        POBJECT
        Remove(
            IN OUT  PITERATOR   Position
            );

        VIRTUAL
        POBJECT
        RemoveAt(
            IN  ULONG   Index
            );

         
        ULONG
        SetCapacity (
            IN ULONG        Capacity
            );

         
        VOID
        SetCapacityIncrement (
            IN ULONG        CapacityIncrement
            );

        
        BOOLEAN
        Insert(
            IN OUT  POBJECT Member,
            IN      ULONG   Index
            );


    protected:

        STATIC
        LONG
        CompareAscDesc(
            IN  POBJECT    Object1,
            IN  POBJECT    Object2,
            IN  BOOLEAN    Ascending   DEFAULT TRUE
            );

         
        VOID
        Construct (
            );

         
        PPOBJECT
        GetObjectArray (
            );

    private:

         
        ULONG
        SetArrayCapacity(
            IN  ULONG   NumberOfElements
            );

        STATIC
        int __cdecl
        CompareAscending (
            IN const void * Object1,
            IN const void * Object2
                );

        STATIC
        int __cdecl
        CompareDescending (
            IN const void * Object1,
            IN const void * Object2
                );

        PPOBJECT    _ObjectArray;           //  Array of pointers to OBJECTs
        ULONG       _PutIndex;              //  Put Index
        ULONG       _Capacity;              //  Capacity of the array
        ULONG       _CapacityIncrement;     //  Increment

#if DBG==1
        ULONG       _IteratorCount;         //  Count of iterators
#endif

};

 
INLINE
ULONG
ARRAY::QueryCapacity (
        ) CONST

/*++

Routine Description:

        Return the current capacity (maximum number of members) of the ARRAY.

Arguments:

        None.

Return Value:

        ULONG - Current capacity.


--*/

{
        return( _Capacity );
}


 
INLINE
ULONG
ARRAY::QueryCapacityIncrement (
        ) CONST

/*++

Routine Description:

        Return the current capacity increment (realloc amount) of the ARRAY.

Arguments:

        None.

Return Value:

        ULONG - Current capacity increment.

--*/

{
        return( _CapacityIncrement );
}


 
INLINE
VOID
ARRAY::SetCapacityIncrement (
        IN ULONG        CapacityIncrement
        )

/*++

Routine Description:

        Set the capacity incement value.

Arguments:

        CapacityIncrement - Supplies the new value for the capacity increment.

Return Value:

        None.

--*/

{
        _CapacityIncrement = CapacityIncrement;
}


 
INLINE
LONG
ARRAY::CompareAscDesc(
    IN  POBJECT    Object1,
    IN  POBJECT    Object2,
    IN  BOOLEAN    Ascending
    )
/*++

Routine Description:

    Compares two object accordint to an Ascending flag

Arguments:

    Object1     -   Supplies first object
    Object2     -   Supplies second object
    Ascending   -   Supplies ascending flag

Return Value:

    LONG    -   If Ascending:
                            <0  if Object1 is less that    Object2
                                 0      if Object1 is equal to     Object2
                                >0      if Object1 is greater than Object2

                If !Ascending:
                                <0      if Object2 is less that    Object1
                                 0      if Object2 is equal to     Object1
                                >0      if Object2 is greater than Object1


--*/
{
    return ( Ascending ? CompareAscending( &Object1, &Object2 ) :
                         CompareDescending( &Object1, &Object2) );
}


 
INLINE
PPOBJECT
ARRAY::GetObjectArray (
    )
/*++

Routine Description:

    Obtains pointer to the array of objects

Arguments:

    None

Return Value:

    PPOBJECT    -   Pointer to array of objects

--*/

{
    return _ObjectArray;
}

