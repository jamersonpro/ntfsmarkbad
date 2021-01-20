/*++

Module Name:

	arrayit.hxx

Abstract:

	This module contains the declaration for the ARRAY_ITERATOR class. 
	ARRAY_ITERATOR is a concrete implementation derived from the abstarct
	ITERATOR class. It is used to 'read' (iterate) over an ARRAY object.
	It has no public constructor and therefore can only be queried from an
	ARRAY object. It is the client's responsibility to detsroy 
	ARRAY_ITERATORs when they are done. ARRAY_ITERATORs maintain the currency
	for reading an ARRAY in order.

--*/

#pragma once

#include "iterator.hxx"

//
//	Forward references
//

DECLARE_CLASS( ARRAY );
DECLARE_CLASS( ARRAY_ITERATOR );


class ARRAY_ITERATOR : public ITERATOR {

    friend ARRAY;

	public:

		VIRTUAL
		~ARRAY_ITERATOR(
			);

        VIRTUAL
        VOID
        Reset(
            );

		VIRTUAL
		POBJECT
		GetCurrent(
			);

		VIRTUAL
		POBJECT
		GetNext(
			);

		VIRTUAL
		POBJECT
		GetPrevious(
			);

         
        ULONG
        QueryCurrentIndex(
            );

	protected:

        DECLARE_CAST_MEMBER_FUNCTION( ARRAY_ITERATOR );
		DECLARE_CONSTRUCTOR( ARRAY_ITERATOR );

         
        BOOLEAN
        Initialize(
            IN OUT  PARRAY  Array
            );

         
        VOID
        Construct(
            );

    private:

        PARRAY  _Array;         //  Array
        ULONG   _CurrentIndex;  //  Current index

};

 
INLINE
ULONG
ARRAY_ITERATOR::QueryCurrentIndex(
    )
/*++

Routine Description:

    Obtains the current index

Arguments:

    None

Return Value:

    ULONG   -   The current index

--*/

{
    return _CurrentIndex;
}
