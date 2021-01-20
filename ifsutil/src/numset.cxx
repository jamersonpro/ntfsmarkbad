#include "stdafx.h"


#include "ulib.hxx"


#include "numset.hxx"
#include "iterator.hxx"

DEFINE_CONSTRUCTOR( NUMBER_SET, OBJECT  );

DEFINE_CONSTRUCTOR( NUMBER_EXTENT, OBJECT );

VOID
NUMBER_SET::Construct (
        )
/*++

Routine Description:

    Constructor for NUMBER_SET.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _card = 0;
    _iterator = NULL;
}


VOID
NUMBER_SET::Destroy(
    )
/*++

Routine Description:

    This routine returns the NUMBER_SET to its initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _list.DeleteAllMembers();
    _card = 0;
    DELETE(_iterator);
}


 
NUMBER_SET::~NUMBER_SET(
    )
/*++

Routine Description:

    Destructor for NUMBER_SET.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


 
BOOLEAN
NUMBER_SET::Initialize(
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

    if (!_list.Initialize() ||
        !(_iterator = _list.QueryIterator())) {

        Destroy();
        return FALSE;
    }

    return TRUE;
}

 
BOOLEAN
NUMBER_SET::Add(
    IN  BIG_INT Number
    )
/*++

Routine Description:

    This routine adds 'Number' to the set.

Arguments:

    Number  - Supplies the number to add to the set.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PNUMBER_EXTENT  p, pn;
    PNUMBER_EXTENT  new_extent;
    BIG_INT         next;

    DebugAssert(_iterator);

    _iterator->Reset();
    while (p = (PNUMBER_EXTENT) _iterator->GetPrevious()) {
        if (p->Start <= Number) {
            break;
        }
    }

    if (p) {

        next = p->Start + p->Length;

        if (Number < next) {
            return TRUE;
        }

        if (Number == next) {

            p->Length += 1;
            _card += 1;

            if (pn = (PNUMBER_EXTENT) _iterator->GetNext()) {

                if (pn->Start == Number + 1) {

                    p->Length += pn->Length;
                    pn = (PNUMBER_EXTENT) _list.Remove(_iterator);
                    DELETE(pn);
                }
            }

            return TRUE;
        }
    }

    if (p = (PNUMBER_EXTENT) _iterator->GetNext()) {

        if (Number + 1 == p->Start) {

            p->Start = Number;
            p->Length += 1;
            _card += 1;
            return TRUE;
        }
    }

    if (!(new_extent = NEW NUMBER_EXTENT)) {
        return FALSE;
    }

    new_extent->Start = Number;
    new_extent->Length = 1;

    if (!_list.Insert(new_extent, _iterator)) {
        DELETE(new_extent);
        return FALSE;
    }

    _card += 1;

    return TRUE;
}

BOOLEAN
NUMBER_SET::Add(
    IN  BIG_INT Start,
    IN  BIG_INT Length
    )
/*++

Routine Description:

    This routine adds the range of numbers to the set.

Arguments:

    Start   - Supplies the first number to add to the set.
    Length  - Supplies the length of the run to add.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    BIG_INT i, sup;
    BOOLEAN r;

    sup = Start + Length;

    r = TRUE;
    for (i = Start; i < sup; i += 1) {
        r = Add(i) && r;
    }

    return r;
}

  
BOOLEAN
NUMBER_SET::Remove(
    IN  BIG_INT     Number
    )
/*++

Routine Description:

    This routine removes a number from the number set.

Arguments:

    Number  - Supplies the number to remove.

Routine Description:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PNUMBER_EXTENT  p;
    PNUMBER_EXTENT  new_extent;
    BIG_INT         next, new_length;

    DebugAssert(_iterator);

    _iterator->Reset();
    while (p = (PNUMBER_EXTENT) _iterator->GetNext()) {
        if (p->Start > Number) {
            break;
        }
    }

    if (p = (PNUMBER_EXTENT) _iterator->GetPrevious()) {

        next = p->Start + p->Length;

        if (p->Start == Number) {
            p->Start += 1;
            p->Length -= 1;
            _card -= 1;

            if (p->Length == 0) {
                p = (PNUMBER_EXTENT) _list.Remove(_iterator);
                DELETE(p);
            }

            return TRUE;
        }

        if (Number + 1 == next) {
            p->Length -= 1;
            _card -= 1;
            return TRUE;
        }

        if (Number < next) {

            if (!(new_extent = NEW NUMBER_EXTENT)) {
                return FALSE;
            }

            _iterator->GetNext();

            if (!_list.Insert(new_extent, _iterator)) {
                DELETE(new_extent);
                return FALSE;
            }

            new_length = Number - p->Start;

            new_extent->Start = Number + 1;
            new_extent->Length = p->Length - 1 - new_length;

            p->Length = new_length;

            _card -= 1;
        }
    }

    return TRUE;
}

 
BOOLEAN
NUMBER_SET::RemoveAll(
     )
{
    PNUMBER_EXTENT  p;

    DebugAssert(_iterator);

    _iterator->Reset();
    if ((p = (PNUMBER_EXTENT) _iterator->GetNext())) 
        do {
            p = (PNUMBER_EXTENT) _list.Remove(_iterator);
            DELETE(p);
        } while ((p=(PNUMBER_EXTENT)_iterator->GetCurrent()));
    _card = 0;
    return TRUE;
}

 
BOOLEAN
NUMBER_SET::CheckAndRemove(
    IN  BIG_INT     Number,
     OUT PBOOLEAN   DoesExists
    )
/*++

Routine Description:

    This routine removes a number from the number set.

Arguments:

    Number      - Supplies the number to remove.
     DoesExists - TRUE if Number was found in the set

Routine Description:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PNUMBER_EXTENT  p;
    PNUMBER_EXTENT  new_extent;
    BIG_INT         next, new_length;

    DebugAssert(_iterator);
    DebugAssert(DoesExists);

    *DoesExists = FALSE;

    _iterator->Reset();
    while (p = (PNUMBER_EXTENT) _iterator->GetNext()) {
        if (p->Start > Number) {
            break;
        }
    }

    if (p = (PNUMBER_EXTENT) _iterator->GetPrevious()) {

        next = p->Start + p->Length;

        if (p->Start == Number) {
            p->Start += 1;
            p->Length -= 1;
            _card -= 1;
            *DoesExists = TRUE;

            if (p->Length == 0) {
                p = (PNUMBER_EXTENT) _list.Remove(_iterator);
                DELETE(p);
            }

            return TRUE;
        }

        if (Number + 1 == next) {
            p->Length -= 1;
            _card -= 1;
            *DoesExists = TRUE;
            return TRUE;
        }

        if (Number < next) {

            if (!(new_extent = NEW NUMBER_EXTENT)) {
                return FALSE;
            }

            _iterator->GetNext();

            if (!_list.Insert(new_extent, _iterator)) {
                DELETE(new_extent);
                return FALSE;
            }

            new_length = Number - p->Start;

            new_extent->Start = Number + 1;
            new_extent->Length = p->Length - 1 - new_length;

            p->Length = new_length;

            _card -= 1;
            *DoesExists = TRUE;
        }
    }

    return TRUE;
}



 
BIG_INT
NUMBER_SET::QueryNumber(
    IN  BIG_INT Index
    ) CONST
/*++

Routine Description:

    This routine returns the Nth number contained in this set.

Arguments:

    Index   - Supplies a zero-based index into the ordered set.

Return Value:

    The Nth number in this set.

--*/
{
    PNUMBER_EXTENT  p;
    BIG_INT         r;
    BIG_INT         count;

    DebugAssert(Index < _card);

    _iterator->Reset();
    count = 0;
    while (p = (PNUMBER_EXTENT) _iterator->GetNext()) {

        count += p->Length;

        if (count > Index) {
            break;
        }
    }

    DebugAssert(p);

    return p->Start + Index - (count - p->Length);
}


 
BOOLEAN
NUMBER_SET::DoesIntersectSet(
    IN  BIG_INT Start,
    IN  BIG_INT Length
    ) CONST
/*++

Routine Description:

    This routine computes whether or not the range specified intersects
    the current number set.  This routine will return FALSE if the
    intersection is empty, TRUE otherwise.

Arguments:

    Start   - Supplies the start of the range.
    Length  - Supplies the length of the range.

Return Value:

    FALSE   - The specified range does not intersect the number set.
    TRUE    - The specified range makes a non-empty intersection with
                the number set.

--*/
{
    PNUMBER_EXTENT  p;
    BIG_INT         pnext, next;

    DebugAssert(_iterator);

    if (Length == 0) {
        return FALSE;
    }

    next = Start + Length;

    _iterator->Reset();
    while (p = (PNUMBER_EXTENT) _iterator->GetNext()) {

        pnext = p->Start + p->Length;

        if (Start >= p->Start) {

            if (Start < pnext) {
                return TRUE;
            }
        } else {

            if (next > p->Start) {
                return TRUE;
            }
        }
    }

    return FALSE;
}


 
VOID
NUMBER_SET::QueryDisjointRange(
    IN  ULONG       Index,
    OUT PBIG_INT    Start,
    OUT PBIG_INT    Length
    ) CONST
/*++

Routine Description:

    This routine returns the 'Index'th disjoint range.  (This is zero
    based).

Arguments:

    Index   - Supplies the index of the disjoint range.
    Start   - Returns the start of the disjoint range.
    Length  - Returns the length of the disjoint range.

Return Value:

    None.

--*/
{
    ULONG           i;
    PNUMBER_EXTENT  p=NULL;

    DebugAssert(_iterator);

    _iterator->Reset();
    for (i = 0; i <= Index; i++) {
        p = (PNUMBER_EXTENT) _iterator->GetNext();
    }

    DebugAssert(p);
    DebugAssert(Start);
    DebugAssert(Length);

    *Start = p->Start;
    *Length = p->Length;
}


 
BOOLEAN
NUMBER_SET::QueryContainingRange(
    IN  BIG_INT     Number,
    OUT PBIG_INT    Start,
    OUT PBIG_INT    Length
    ) CONST
/*++

Routine Description:

    This routine returns the range that contains the given number.

Arguments:

    Number  - Supplies the number.
    Start   - Returns the start of the range.
    Length  - Returns the length of the range.

Return Value:

    FALSE   - The given number was not in the set.
    TRUE    - Success.

--*/
{
    PNUMBER_EXTENT  p;

    DebugAssert(_iterator);

    _iterator->Reset();
    while (p = (PNUMBER_EXTENT) _iterator->GetPrevious()) {
        if (p->Start <= Number) {
            break;
        }
    }

    if (!p || Number >= p->Start + p->Length) {
        return FALSE;
    }

    *Start = p->Start;
    *Length = p->Length;

    return TRUE;
}
