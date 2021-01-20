/*++

Module Name:

    intstack.hxx

Abstract:

    This class implements a linked list integer stack.

--*/

#pragma once


#include "bigint.hxx"




DECLARE_CLASS( INTSTACK );

DEFINE_TYPE( struct _INTNODE, INTNODE );

struct _INTNODE {
        PINTNODE    Next;
        BIG_INT     Data;
};

class INTSTACK : public OBJECT {

        public:

         
        DECLARE_CONSTRUCTOR( INTSTACK );

        VIRTUAL
        ~INTSTACK(
            );

         
        BOOLEAN
        Initialize(
            );

         
        BOOLEAN
        Push(
            IN  BIG_INT Data
            );

         
        VOID
        Pop(
            IN  ULONG   HowMany DEFAULT 1
            );

         
        BIG_INT
        Look(
            IN  ULONG   Index   DEFAULT 0
            ) CONST;

         
        ULONG
        QuerySize(
            ) CONST;


    private:

         
        VOID
        Construct (
                );

         
        VOID
        Destroy(
            );

        PINTNODE    _stack;
        ULONG       _size;

};


INLINE
ULONG
INTSTACK::QuerySize(
    ) CONST
/*++

Routine Description:

    This routine computes the number of elements in the stack.

Arguments:

    None.

Return Value:

    The number of elements in the stack.

--*/
{
    return _size;
}


