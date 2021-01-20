/*++

Module Name:

    hmem.hxx

Abstract:

    The class HMEM is an implementation of the class MEM which uses the
    memory resources of the heap.

    After the first call to Acquire that succeeds, all successive calls
    will return the same memory that was returned by the first call
    provided that the size requested is within the bounds of the first call.
    The common buffer which was created upon the first successful call to
    Acquire will be available along with its size by calling GetBuf and
    QuerySize.

    Calling Destroy will put the object back in its initial state thus
    invalidating any pointers to its memory and enabling future calls
    to Acquire to succeed regardless of the size specicified.

--*/

#pragma once

#include "mem.hxx"

DECLARE_CLASS( HMEM );


class HMEM : public MEM {

	public:

         
        DECLARE_CONSTRUCTOR( HMEM );

        VIRTUAL
        ~HMEM(
            );

         
        BOOLEAN
        Initialize(
            );

        VIRTUAL
        PVOID
        Acquire(
            IN  ULONG   Size,
            IN  ULONG   AlignmentMask   DEFAULT 0
            );

         
        PVOID
        GetBuf(
            ) CONST;

         
        ULONG
        QuerySize(
            ) CONST;

         
        BOOLEAN
        Resize(
            IN  ULONG   NewSize,
            IN  ULONG   AlignmentMask   DEFAULT 0
            );

    private:

		 
		VOID
		Construct (
			);

         
        VOID
        Destroy(
            );

        ULONG   _size;
        PVOID   _real_buf;
        PVOID   _buf;

};


INLINE
PVOID
HMEM::GetBuf(
    ) CONST
/*++

Routine Description:

    This routine returns the memory that was previously 'Acquired'.

Arguments:

    None.

Return Value:

    A pointer to the beginning of the memory buffer.

--*/
{
    return _buf;
}


INLINE
ULONG
HMEM::QuerySize(
    ) CONST
/*++

Routine Description:

    This routine returns the size of the memory that was previously
    'Acquired'.

Arguments:

    None.

Return Value:

    The size of the memory returned by 'GetBuf'.

--*/
{
    return _size;
}
