/*++

Module Name:

	sortcnt.hxx

Abstract:

	This module contains the declaration for the SORTABLE_CONTAINER class.
	SORTABLE_CONTAINER is an abstract classe that is derived from the abstract
	class SEQUENTIAL_CONTAINER. It not only assumes a sequence but also 
	assumes that the sequence can be changed by sorting it's contents. That
	is the contents have a relative order independent from how they were
	placed in the container.

--*/

#pragma once

#include "seqcnt.hxx"

DECLARE_CLASS( SORTABLE_CONTAINER );

class SORTABLE_CONTAINER : public SEQUENTIAL_CONTAINER {

	public:

        VIRTUAL
        ~SORTABLE_CONTAINER(
            );

		VIRTUAL
        BOOLEAN
		Put(
			IN OUT  POBJECT Member
			) PURE;

        VIRTUAL
        ULONG
        QueryMemberCount(
            ) CONST PURE;

		VIRTUAL
		PITERATOR
		QueryIterator(
			) CONST PURE;

		VIRTUAL
		POBJECT
		Remove(
			IN OUT  PITERATOR   Position
			) PURE;

	protected:

		DECLARE_CONSTRUCTOR( SORTABLE_CONTAINER );

};


