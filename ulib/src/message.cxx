#include "stdafx.h"

#include <iostream>   // std::cout

#include "ulib.hxx"
#include "message.hxx"

#include "hmem.hxx"

DEFINE_CONSTRUCTOR(MESSAGE, OBJECT );


MESSAGE::~MESSAGE(
    )
/*++

Routine Description:

    Destructor for MESSAGE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


VOID
MESSAGE::Construct(
    )
/*++

Routine Description:

    This routine initializes the object to a default initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
}


VOID
MESSAGE::Destroy(
    )
/*++

Routine Description:

    This routine returns the object to a default initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
}


 
BOOLEAN
MESSAGE::Initialize(
    )
/*++

Routine Description:

    This routine initializes the class to a valid initial state.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    Destroy();
    return TRUE;
}


