/*++

Module Name:

    numset.hxx

Abstract:

    This class implements a sparse number set.  The number are
    stored in ascending order.

--*/

#pragma once

#include "bigint.hxx"
#include "list.hxx"


DECLARE_CLASS( NUMBER_SET );

class NUMBER_EXTENT : public OBJECT {

    public:

        DECLARE_CONSTRUCTOR( NUMBER_EXTENT );

        BIG_INT Start;
        BIG_INT Length;

};


DEFINE_POINTER_TYPES(NUMBER_EXTENT);

class NUMBER_SET : public OBJECT {

        public:

         
        DECLARE_CONSTRUCTOR( NUMBER_SET );

        VIRTUAL
         
        ~NUMBER_SET(
            );

         
        BOOLEAN
        Initialize(
            );

         
        BOOLEAN
        Add(
            IN  BIG_INT Number
            );
         
        BOOLEAN
        Add(
            IN  BIG_INT Start,
            IN  BIG_INT Length
            );
         
        BOOLEAN
        Remove(
            IN  BIG_INT Number
            );
        
         
        BOOLEAN
        RemoveAll(
            );

         
        BOOLEAN
        CheckAndRemove(
            IN  BIG_INT         Number,
            OUT PBOOLEAN        DoesExists
            );
         
        BIG_INT
        QueryCardinality(
            ) CONST;

         
         
        BIG_INT
        QueryNumber(
            IN  BIG_INT Index
            ) CONST;

         
        BOOLEAN
        DoesIntersectSet(
            IN  BIG_INT Start,
            IN  BIG_INT Length
            ) CONST;

         
        ULONG
        QueryNumDisjointRanges(
            ) CONST;

         
        VOID
        QueryDisjointRange(
            IN  ULONG       Index,
            OUT PBIG_INT    Start,
            OUT PBIG_INT    Length
            ) CONST;

         
        BOOLEAN
        QueryContainingRange(
            IN  BIG_INT     Number,
            OUT PBIG_INT    Start,
            OUT PBIG_INT    Length
            ) CONST;

    private:
         
        VOID
        Construct (
                );

         
        VOID
        Destroy(
            );

        LIST        _list;
        BIG_INT     _card;
        PITERATOR   _iterator;

};


INLINE
BIG_INT
NUMBER_SET::QueryCardinality(
    ) CONST
/*++

Routine Description:

    This routine computes the number of elements in the set.

Arguments:

    None.

Return Value:

    The number of elements in the set.

--*/
{
    return _card;
}


INLINE
ULONG
NUMBER_SET::QueryNumDisjointRanges(
    ) CONST
/*++

Routine Description:

    This routine computes the number of disjoint ranges contained
    in this number set.

Arguments:

    None.

Return Value:

    The number of disjoint ranges contained in this number set.

--*/
{
    return _list.QueryMemberCount();
}

