/*++

Module Name:

    membmgr.hxx

Abstract:

    This class offers two classes for the management of fixed
    size blocks of memory.  The first class STATIC_MEM_BLOCK_MGR
    allows the user to allocate and free fixed size memory blocks
    up to the specified limit that the class was initialized to.

    The second class MEM_BLOCK_MGR offers a scheme for memory block
    management that will grow as the clients needs increase.

--*/

#pragma once


#include "array.hxx"
#include "bitvect.hxx"


DECLARE_CLASS( STATIC_MEM_BLOCK_MGR );

class STATIC_MEM_BLOCK_MGR : public OBJECT {

    public:

        DECLARE_CONSTRUCTOR( STATIC_MEM_BLOCK_MGR );

        VIRTUAL
        ~STATIC_MEM_BLOCK_MGR(
            );

         
        BOOLEAN
        Initialize(
            IN  ULONG   MemBlockSize,
            IN  ULONG   NumBlocks       DEFAULT 128
            );

         
        PVOID
        Alloc(
            );

         
        BOOLEAN
        Free(
            OUT PVOID   MemBlock
            );

         
        ULONG
        QueryBlockSize(
            ) CONST;

         
        ULONG
        QueryNumBlocks(
            ) CONST;

    private:

         
        VOID
        Construct(
            );

         
        VOID
        Destroy(
            );

        PCHAR       _heap;
        ULONG       _num_blocks;
        ULONG       _block_size;
        ULONG       _num_allocated;
        ULONG       _next_alloc;
        BITVECTOR   _bitvector;

};


INLINE
ULONG
STATIC_MEM_BLOCK_MGR::QueryBlockSize(
    ) CONST
/*++

Routine Description:

    This routine return the number of bytes in a block returned
    by this object.

Arguments:

    None.

Return Value:

    The number of bytes per block.

--*/
{
    return _block_size;
}


INLINE
ULONG
STATIC_MEM_BLOCK_MGR::QueryNumBlocks(
    ) CONST
/*++

Routine Description:

    This routine return the number of blocks contained
    by this object.

Arguments:

    None.

Return Value:

    The number of blocks.

--*/
{
    return _num_blocks;
}



DECLARE_CLASS( MEM_BLOCK_MGR );

class MEM_BLOCK_MGR : public OBJECT {

    public:

         
        DECLARE_CONSTRUCTOR( MEM_BLOCK_MGR );

        VIRTUAL
         
        ~MEM_BLOCK_MGR(
            );

         
         
        BOOLEAN
        Initialize(
            IN  ULONG   MemBlockSize,
            IN  ULONG   InitialNumBlocks    DEFAULT 128
            );

         
         
        PVOID
        Alloc(
            );

         
         
        BOOLEAN
        Free(
            OUT PVOID   MemBlock
            );

         
        ULONG
        QueryBlockSize(
            ) CONST;

    private:

         
        VOID
        Construct(
            );

         
        VOID
        Destroy(
            );

        PSTATIC_MEM_BLOCK_MGR       _static_mem_list[32];

};


INLINE
ULONG
MEM_BLOCK_MGR::QueryBlockSize(
    ) CONST
/*++

Routine Description:

    This routine return the number of bytes in a block returned
    by this object.

Arguments:

    None.

Return Value:

    The number of bytes per block.

--*/
{
    return _static_mem_list[0] ? _static_mem_list[0]->QueryBlockSize() : 0;
}


