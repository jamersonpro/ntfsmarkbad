/*++

Module Name:

	contain.hxx

Abstract:

	This module contains the definition for the CONTAINER class, the most
	primitive, abstract class in the container sub-hierarchy. CONTAINERs
	of all types are repositories for OBJECTs. CONTAINER is the most abstract
	in that it makes no assumptions about the ordering of it's contents.

--*/

#pragma once

DECLARE_CLASS( CONTAINER );

class CONTAINER : public OBJECT {

	public:

        VIRTUAL
        ~CONTAINER(
            );

	protected:

		DECLARE_CONSTRUCTOR( CONTAINER );

};



