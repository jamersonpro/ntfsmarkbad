#include "stdafx.h"

/*++

Module Name:

	clasdesc.cxx

Abstract:

	This module contains the definition for the CLASS_DESCRIPTOR class. 
	CLASS_DESCRIPTOR classes are special concrete classes derived from 
	OBJECT. They are special in that a single staic object of this class
	exists for every other concrete class in the Ulib hierarchy. 
	CLASS_DESCRIPTORs allocate and maintain information that can be used
	at run-time to determine the actual type of an object.

Notes:

	The definitions for all concrete class' CLASS_DESCRIPTORs can be found
	in the file ulib.cxx.

	See the Cast member function in ulibdef.hxx to see how dynamic casting
	and CLASS_DESCRIPTORs work.

--*/


#include "ulib.hxx"

 
CLASS_DESCRIPTOR::CLASS_DESCRIPTOR (
	)
{
}

 
BOOLEAN
CLASS_DESCRIPTOR::Initialize (
	)

/*++

Routine Description:

	Initialize a CLASS_DESCRIPTOR object by initializing the class id.

Arguments:

Return Value:

    None.

--*/

{
	_ClassID = ( ULONG_PTR ) &_ClassID;
	return( TRUE );
}

