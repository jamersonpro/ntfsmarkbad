#include "stdafx.h"






#include "ulib.hxx"


#include "intstack.hxx"


DEFINE_CONSTRUCTOR( INTSTACK, OBJECT   );

VOID
INTSTACK::Construct (
        )
/*++

Routine Description:

    Constructor for INTSTACK.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _stack = NULL;
    _size = 0;
}


 
INTSTACK::~INTSTACK(
    )
/*++

Routine Description:

    Destructor for INTSTACK.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


 
BOOLEAN
INTSTACK::Initialize(
    )
/*++

Routine Description:

    This routine initializes the stack for new input.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    Destroy();
    return TRUE;
}


 
BOOLEAN
INTSTACK::Push(
    IN  BIG_INT Data
    )
/*++

Routine Description:

    This routine pushes 'Data' on the stack.

Arguments:

    Data    - Supplies the integer to push on the stack.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PINTNODE    p;

    p = _stack;
        if (!(_stack = NEW INTNODE)) {
        _stack = p;
        return FALSE;
    }

    _stack->Next = p;
    _stack->Data = Data;
    _size++;

    return TRUE;
}


VOID
INTSTACK::Pop(
    IN  ULONG   HowMany
    )
/*++

Routine Description:

    This routine attempts to remove 'HowMany' elements from the top of
    the stack.  If there are not that many to remove then all that
    can be removed, will be removed and FALSE will be returned.

Arguments:

    HowMany - Supplies the number of elements to remove from the top of the
                stack.

Return Value:

    None.

--*/
{
    PINTNODE    p;

    for (; HowMany; HowMany--) {

        DebugAssert(_stack);

        p = _stack->Next;
                DELETE( _stack );
        _stack = p;
        _size--;
    }
}


BIG_INT
INTSTACK::Look(
    IN  ULONG   Index
    ) CONST
/*++

Routine Description:

    This routine returns the 'Index'th element of the stack.  Index 0 denotes
    the top of the stack.  Index 1 denotes one element from the top of the
    stack and so on.  If the stack is smaller than the element requested then
    this routine will return 0.  This is not a limitation since 'QuerySize'
    will return the depth of the stack.

Arguments:

    Index   - Supplies the index of the data requested.

Return Value:

    The value of the stack element at position 'Index' or 0.

--*/
{
    PINTNODE    p;

    p = _stack;
    for (; Index; Index--) {
        p = p ? p->Next : NULL;
    }

    if (!p) {
        return 0;
    }

    return p->Data;
}


VOID
INTSTACK::Destroy(
    )
/*++

Routine Description:

    This routine returns the INTSTACK to its initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PINTNODE    p;

    while (_stack) {
        p = _stack->Next;
                DELETE( _stack );
        _stack = p;
    }
    _size = 0;
}

