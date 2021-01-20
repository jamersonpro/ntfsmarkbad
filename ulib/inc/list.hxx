/*++

Module Name:

	list.hxx

Abstract:

    This class is an implementation of the SEQUENTIAL_CONTAINER
    protocol.  The specific implementation is that of a doubly
    linked list.

--*/

#pragma once

#include "seqcnt.hxx"
#include "membmgr.hxx"


struct OBJECT_LIST_NODE {
    OBJECT_LIST_NODE*   next;
    OBJECT_LIST_NODE*   prev;
    POBJECT             data;
};

DEFINE_POINTER_AND_REFERENCE_TYPES( OBJECT_LIST_NODE );

DECLARE_CLASS( LIST );

class LIST : public SEQUENTIAL_CONTAINER {

    FRIEND class LIST_ITERATOR;

    public:

        DECLARE_CONSTRUCTOR( LIST );

        VIRTUAL
         
        ~LIST(
            );

         
        BOOLEAN
        Initialize(
            );

        VIRTUAL
        ULONG
        QueryMemberCount(
            ) CONST;

		VIRTUAL
         
        BOOLEAN
		Put(
            IN OUT  POBJECT Member
			);

		VIRTUAL
		POBJECT
		Remove(
			IN OUT  PITERATOR   Position
			);

		VIRTUAL
         
        PITERATOR
		QueryIterator(
            ) CONST;

         
        BOOLEAN
        Insert(
            IN OUT  POBJECT     Member,
            IN OUT  PITERATOR   Position
            );

    private:

        POBJECT_LIST_NODE   _head;
        POBJECT_LIST_NODE   _tail;
        ULONG               _count;
        MEM_BLOCK_MGR       _mem_block_mgr;

         
        VOID
        Construct(
            );

         
        VOID
        Destroy(
            );

};



