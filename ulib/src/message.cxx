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


void MESSAGE::Out(const char* str)
{
    std::cout << str << "\n";
}

void MESSAGE::Out(const char* str1, const char* str2)
{
    std::cout << str1 << str2 << "\n";
}
void MESSAGE::Out(const char* str1, const char* str2, const char* str3)
{
    std::cout << str1 << str2 << str3 << "\n";
}

void MESSAGE::Out(const char* str, LONGLONG number)
{
    std::cout << str << number << "\n";
}

void MESSAGE::Out(const char* str1, LONGLONG number, const char* str2)
{
    std::cout << str1 << number << str2 << "\n";
}

void MESSAGE::Out(const char* str1, LONGLONG number1, const char* str2, LONGLONG number2, const char* str3)
{
    std::cout << str1 << number1 << str2 << number2 << str3 << "\n";
}

void MESSAGE::Out(const char* str1, LONGLONG number1, const char* str2, LONGLONG number2, const char* str3, LONGLONG number3, const char* str4)
{
    std::cout << str1 << number1 << str2 << number2 << str3 << number3 << str4 << "\n";
}

