#include "stdafx.h"

/*++

Module Name:

    path.cxx

Abstract:

    This contains the implementation for all methods handling file
    path names. These are needed for use with an file i/o or the
    FILE and DIR objects.

--*/

extern "C" {
    #include <string.h>
}
#include "ulib.hxx"
#include "wstring.hxx"
#include "path.hxx"
#include "system.hxx"


typedef enum _SPECIAL_DEVICES {
    LPT,
    COM,
    CON,
    PRN,
    AUX,
    LAST_SPECIAL_DEVICE
} SPECIAL_DEVICES;


//
// Static member data.
//

STATIC PWSTRING    _SlashString;
STATIC PWSTRING    _SpecialDevices[ LAST_SPECIAL_DEVICE ];

#define     DELIMITER_STRING    "\\"
#define     DELIMITER_CHAR      ((WCHAR)'\\')

 
BOOLEAN
PATH::Initialize (
    )

/*++

Routine Description:

    Perform global initialization of the PATH class.

Arguments:

    None.

Return Value:

    BOOLEAN - Returns TRUE if global initialization was succesful

Notes:

    Global initialization should be interpreted as class (rather than object)
    initialization. This routine should be called by ALL other
    PATH::Initialize member functions.

    Current this routine:

        - constructs and initializes _SlashString, a WSTRING that
          contains a '\'

--*/

{
    STATIC BOOLEAN  fInit = FALSE;

    if (!fInit) 
    {
        if ( (( _SlashString = NEW DSTRING) != NULL)        &&
             ((_SpecialDevices[LPT] = NEW DSTRING) != NULL) &&
             ((_SpecialDevices[COM] = NEW DSTRING) != NULL) &&
             ((_SpecialDevices[CON] = NEW DSTRING) != NULL) &&
             ((_SpecialDevices[PRN] = NEW DSTRING) != NULL) &&
             ((_SpecialDevices[AUX] = NEW DSTRING) != NULL) &&
             _SlashString->Initialize( DELIMITER_STRING )   &&
             _SpecialDevices[LPT]->Initialize( "LPT" )      &&
             _SpecialDevices[COM]->Initialize( "COM" )      &&
             _SpecialDevices[CON]->Initialize( "CON" )      &&
             _SpecialDevices[PRN]->Initialize( "PRN" )      &&
             _SpecialDevices[AUX]->Initialize( "AUX" )
           ) 
        {
            return fInit = TRUE;
        }
    }

    return fInit;
}
 
 
BOOLEAN
PATH::EndsWithDelimiter (
    ) CONST

/*++

Routine Description:

    Returns TRUE if the path ends with slash

Arguments:

    None.

Return Value:

    BOOLEAN -   Returns TRUE if the path ends with a slash

--*/
{

    return ( _PathString.QueryChAt( _PathString.QueryChCount() - 1 ) == DELIMITER_CHAR );

}
 
 
CHNUM
PATH::QueryDeviceLen(
    IN  PWSTRING pString
    ) CONST

/*++

Routine Description:

    Find length in character of drive section

Arguments:

    pString - Supplies the string to determine drive size.

Return Value:

    CHNUM - Number of characters making up drive section. If no
            drive section then the length is 0.

--*/
{
    CHNUM           Position = 0;
    CHNUM           Position1;
    SPECIAL_DEVICES Index;
    ULONG           tmp;
    INT             Pos;
    LONG            Number;


    UNREFERENCED_PARAMETER( (void)this );

    DebugPtrAssert( pString );

    if ( pString->QueryChCount() > 0 ) {

        //
        //  Check for special device
        //
        Pos = (INT)pString->QueryChCount() - 1;

        while ( (Pos >= 0) && (pString->QueryChAt( (CHNUM)Pos ) != DELIMITER_CHAR) ) {
            Pos --;
        }

        Pos++;

        for (Index = LPT; Index < LAST_SPECIAL_DEVICE;
             (tmp = (ULONG) Index, tmp++, Index = (SPECIAL_DEVICES) tmp) ) {

            if ( !pString->Stricmp( _SpecialDevices[Index],
                                    (CHNUM)Pos ) ) {

                Position = (CHNUM)Pos + _SpecialDevices[Index]->QueryChCount();

                //
                //  LPT && COM must be followed by a number;
                //
                if ( (Index == LPT) || (Index == COM) ) {
                    if ( Position >= pString->QueryChCount()) {
                        continue;
                    }
                    while ( (Position < pString->QueryChCount()) &&
                            pString->QueryNumber( &Number, Position, 1 ) ) {

                        Position++;
                    }
                }

                if (Position >= pString->QueryChCount()) {
                     return Position;
                } else if (pString->QueryChAt( Position ) == (WCHAR)':') {
                    return Position+1;
                }
            }
        }
        //
        //  Look for ':'
        //
        if ((Position = pString->Strchr((WCHAR)':')) != INVALID_CHNUM) {
            return Position + 1;
        }

        //
        // check for leading "\\"
        //
        if  ( pString->QueryChCount() > 1           &&
              pString->QueryChAt(0) == DELIMITER_CHAR &&
              pString->QueryChAt(1) == DELIMITER_CHAR) {

            //
            // the device is a machine name - find the second backslash
            // (start search after first double backsl). Note that this
            //  means that the device names if formed by the machine name
            //  and the sharepoint.
            //
            if ( ((Position  = pString->Strchr( DELIMITER_CHAR, 2 )) != INVALID_CHNUM )) {

                 Position1 = pString->Strchr( DELIMITER_CHAR, Position+1 );
                 if ( Position1 == INVALID_CHNUM ) {
                    return pString->QueryChCount();
                 }
                 return Position1;

            }

            //
            //  No backslash found, this is an invalid device
            //
            DebugAbort( "Invalid Device name" );

        }


    }

    return 0;
}
 
VOID
PATH::SetPathState(
    )

/*++

Routine Description:

    Sets the state information for the Path

Arguments:

    None.

Return Value:

    None.

--*/

{

    CHNUM chnLastSlash;
    CHNUM chnLastDot;
    CHNUM chnAcum;
    CHNUM FirstSeparator;

    //
    // Find the number of characters in the device name
    //
    chnAcum = _PathState.DeviceLen = QueryDeviceLen( &_PathString );

    //
    // Find the number of characters in the dirs portion of the path
    // by searching for the last '\'
    //
    if ( _PathString.QueryChAt( chnAcum ) == DELIMITER_CHAR ) {
        //
        //  Skip over the slash after the device name
        //
        FirstSeparator = 1;
        chnAcum++;

    } else {

        FirstSeparator = 0;
    }

    if ( chnAcum < _PathString.QueryChCount() ) {

        if (( chnLastSlash = _PathString.Strrchr( DELIMITER_CHAR, chnAcum )) != INVALID_CHNUM ) {

            //
            //  The dirs length is that character position less the length
            //  of the device
            //
            _PathState.DirsLen = chnLastSlash - _PathState.DeviceLen;
            _PathState.SeparatorLen = 1;

            chnAcum += _PathState.DirsLen;
            if ( FirstSeparator ==  0 ) {
                chnAcum++;
            }
        } else {
            //
            // There is no dirs portion of this path, but there is a name.
            //
            _PathState.DirsLen      = FirstSeparator;
            _PathState.SeparatorLen = 0;

        }
    } else {

        //
        //  There is no name portion in this path, and the dirs portion
        //  might be empty (or consist solely of the delimiter ).
        //
        _PathState.DirsLen      = FirstSeparator;
        _PathState.SeparatorLen = 0;
    }

    if ( chnAcum < _PathString.QueryChCount() ) {

        //
        // Find the number of characters in the name portion of the path
        // by searching for the last '.'
        //
        if (( chnLastDot = _PathString.Strrchr( ( WCHAR )'.',
                                                chnAcum )) != INVALID_CHNUM ) {

            _PathState.BaseLen = chnLastDot - chnAcum;

            chnAcum += _PathState.BaseLen + 1;

            _PathState.ExtLen   = _PathString.QueryChCount() - chnAcum;

            _PathState.NameLen  = _PathState.BaseLen + _PathState.ExtLen + 1;


        } else {

            //
            // There is no last '.' so the name length is the length of the
            // component from the last '\' to the end of the path (adjusted
            // for zero base) and there is no extension.
            //

            _PathState.NameLen = _PathString.QueryChCount() - chnAcum;
            _PathState.BaseLen = _PathState.NameLen;
            _PathState.ExtLen  = 0;

        }
    } else {

        //
        //  There is no name part
        //
        _PathState.NameLen = 0;
        _PathState.BaseLen = 0;
        _PathState.ExtLen  = 0;

    }


    //
    // The prefix length is the sum of the device and dirs
    //
    _PathState.PrefixLen = _PathState.DeviceLen + _PathState.DirsLen;

    //
    //  If The device refers to a drive, uppercase it. (Done for
    //  compatibility with some DOS apps ).
    //
    if ( _PathState.DeviceLen == 2 ) {
        _PathString.Strupr( 0,  1 );
    }

}
 
DEFINE_CONSTRUCTOR( PATH, OBJECT  );

DEFINE_CAST_MEMBER_FUNCTION( PATH );

VOID
PATH::Construct (
    )

{
    _PathState.BaseLen  = 0;
    _PathState.DeviceLen= 0;
    _PathState.DirsLen  = 0;
    _PathState.ExtLen   = 0;
    _PathState.NameLen  = 0;
    _PathState.PrefixLen= 0;

    _PathBuffer[0] = 0;
    _PathString.Initialize(_PathBuffer, MAX_PATH);

#if DBG==1
    _Signature  =   PATH_SIGNATURE;
#endif

}
 
 
BOOLEAN
PATH::Initialize(
    IN PCWSTR   InitialPath,
    IN BOOLEAN  Canonicalize
    )

/*++

Routine Description:

    Initialize a PATH object with the supplied string.  No validation
    on the given path is performed unless 'Canonicalize' is set to TRUE.

Arguments:

    InitialPath - Supplies a zero terminated string
    Canonicalize- Supplies a flag, which if TRUE indicates that the PATH
        should be canoicalized at initialization time (i.e. now)

Return Value:

    BOOLEAN - Returns TRUE if the PATH was succesfully initialized.

--*/

{
    PWSTR       filepart;
    DWORD       CharsInPath;


    DebugPtrAssert( InitialPath );

    //
    // Perform global (class) initialization
    //
    if (!Initialize()) {
        DebugAbort( "Class initialization failed" );
        return FALSE;
    }

    // Avoid copies during Strcat by making this a reasonable size.
    if (!_PathString.NewBuf(MAX_PATH - 1)) {
        return FALSE;
    }

    if ( Canonicalize ) {

        if (!_PathString.NewBuf(MAX_PATH - 1) ||
            !(CharsInPath = GetFullPathName((LPWSTR) InitialPath,
                                            MAX_PATH,
                                            (LPWSTR) _PathString.GetWSTR(),
                                            &filepart)) ||
            CharsInPath > MAX_PATH) {

            return FALSE;
        }

        _PathString.SyncLength();

        SetPathState( );
#if DBG==1
        _Initialized = TRUE;
#endif
        return TRUE;

    } else if( ((PWSTRING) &_PathString)->Initialize( InitialPath )) {

        SetPathState( );
#if DBG==1
        _Initialized = TRUE;
#endif
        return TRUE;
    }

    return FALSE;
}
 
 
BOOLEAN
PATH::Initialize(
    IN PCWSTRING    InitialPath,
    IN BOOLEAN      Canonicalize

    )
{
    DebugPtrAssert( InitialPath );

    return Initialize( InitialPath->GetWSTR(), Canonicalize );
}
 
 
BOOLEAN
PATH::Initialize (
    IN PCPATH       InitialPath,
    IN BOOLEAN      Canonicalize
    )

{
    DebugPtrAssert( InitialPath );

    return Initialize( InitialPath->GetPathString()->GetWSTR(), Canonicalize );
}
 
 
PATH::~PATH (
    )

{
}
 
 
BOOLEAN
PATH::AppendBase (
    IN PCWSTRING Base,
    IN BOOLEAN          Absolute
    )

/*++

Routine Description:

    Append the supplied name to the end of this PATH.

Arguments:

    Base        - Supplies the string to be appended.
    Absolute    - Supplies a flag which if TRUE means that the path must
                  be absolute.

Return Value:

    BOOLEAN - Returns TRUE if the '\' and the supplied string was succesfully
        appended.

--*/

{
    BOOLEAN     AppendedSlash = FALSE;

    DebugPtrAssert( Base );
    DebugAssert( _Initialized );

    //
    //  If the path does not consist of only a drive letter followed by a
    //  colon, we might need to add a '\'
    //
    if ( _PathString.QueryChCount() > 0 ) {
        if ( !(( _PathState.DeviceLen == _PathString.QueryChCount()) &&
              ( _PathString.QueryChAt( _PathState.DeviceLen - 1) == (WCHAR)':')) ||
              Absolute ) {


            if ( _PathString.QueryChAt( _PathString.QueryChCount() - 1 ) != (WCHAR)'\\' ) {

                if ( !_PathString.Strcat( _SlashString )) {
                    return FALSE;
                }

                AppendedSlash = TRUE;
            }
        }
    }

    //
    //  Append the base
    //
    if ( _PathString.Strcat( Base )) {
        SetPathState();
        return TRUE;
    }

    //
    //  Could not append base, remove the slash if we appended it
    //
    if ( AppendedSlash ) {
        TruncateBase();
    }

    return FALSE;
}
  
 
PPATH
PATH::QueryFullPath(
    ) CONST

/*++

Routine Description:

Arguments:

    None.

Return Value:

    PPATH

--*/

{


    REGISTER PPATH      pFullPath;
    REGISTER PWSTRING   pFullPathString;

    DebugAssert( _Initialized );

    //
    // If the full path name string for this PATH can not be queried
    // or a new PATH, representing the full path, can not be constructed
    // return NULL.
    //

    if ((( pFullPathString = QueryFullPathString( )) == NULL )  ||
        (( pFullPath = NEW PATH ) == NULL )) {

        return NULL;
    }

    //
    // If the new, full path, can not be initialized, delete it.
    //

    if( ! ( pFullPath->Initialize( pFullPathString ))) {

        DELETE( pFullPath );
    }

    //
    // Delete the full path string and return a pointer to the new, full path
    // (note that the pointer may be NULL).
    //

    DELETE( pFullPathString );

    return pFullPath ;
}
 
 
PWSTRING
PATH::QueryFullPathString (
    ) CONST
{

    LPWSTR      pszName;

    PWSTRING    pwcFullPathString;
    WSTR         szBufferSrc[ MAX_PATH ];
    WSTR         szBufferTrg[ MAX_PATH ];

    DebugAssert( _Initialized );

    if( (pwcFullPathString = NEW DSTRING ()) != NULL ) {

        if ( _PathString.QueryWSTR( 0, TO_END, szBufferSrc, MAX_PATH ) ) {

            if (GetFullPathName( szBufferSrc,MAX_PATH,szBufferTrg,&pszName)) {

                if (pwcFullPathString->Initialize(szBufferTrg)) {

                    return pwcFullPathString;

                }
            }
        }
    }

    DELETE( pwcFullPathString );

    return NULL;

}
 
 
BOOLEAN
PATH::SetDevice (
    IN PCWSTRING NewDevice
    )
{
    DebugAssert( _Initialized );
    DebugPtrAssert( NewDevice );

    if (_PathState.DeviceLen) {
        if (!_PathString.Replace(QueryDeviceStart(), _PathState.DeviceLen,
                                 NewDevice)) {

            return FALSE;
        }
    } else {
        if (!_PathString.Strcat(NewDevice)) {
            return FALSE;
        }
    }

    SetPathState();

    return TRUE;
}
 
 
BOOLEAN
PATH::SetBase (
    IN PCWSTRING NewBase
    )
{
    DebugAssert( _Initialized );
    DebugPtrAssert( NewBase );

    if (_PathState.BaseLen) {

        if (!_PathString.Replace(QueryBaseStart(), _PathState.BaseLen,
                                 NewBase)) {

            return FALSE;
        }

    } else {

        if (!_PathString.Strcat(NewBase)) {
            return FALSE;
        }
    }

    SetPathState();

    return TRUE;
}
 
BOOLEAN
PATH::SetExt (
    IN PCWSTRING NewExt
    )
{
    DebugAssert( _Initialized );
    DebugPtrAssert( NewExt );

    if (_PathState.ExtLen) {

        if (!_PathString.Replace(QueryExtStart(), _PathState.ExtLen,
                                 NewExt)) {

            return FALSE;
        }

    } else {

        if (!_PathString.Strcat(NewExt)) {
            return FALSE;
        }
    }

    SetPathState();

    return TRUE;
}
 
 
BOOLEAN
PATH::TruncateBase (
    )

/*++

Routine Description:

    This routine truncates the path after the prefix portion.

Arguments:

    None.

Return Value:

    BOOLEAN - Returns TRUE if the base existed and was succesfully removed.

--*/

{
    DebugAssert( _Initialized );

    // If this is the root then the prefix len will include the \.
    // If not, then it won't.  Either way.  Truncate this string to
    // the prefix length.

    _PathString.Truncate( _PathState.PrefixLen );

    SetPathState();

    return TRUE;
}


 
 
PPATH
PATH::QueryPath(
    ) CONST

/*++

Routine Description:

Arguments:

    None.

Return Value:

    PPATH

--*/

{


    REGISTER PPATH      pPath;

    DebugAssert( _Initialized );

    if (( pPath = NEW PATH ) == NULL ) {
        return NULL;
    }

    //
    // If the new path can not be initialized, delete it.
    //
    if( ! ( pPath->Initialize( GetPathString()->GetWSTR(), FALSE ))) {

        DELETE( pPath );
    }

    return pPath ;
}
 

BOOLEAN
PATH::ExpandWildCards(
    IN  OUT PWSTRING    pStr1,
    IN  OUT PWSTRING    pStr2
    )
/*++

Routine Description:

    Expands any wildcards in string 2 to match the equivalent characters in
    string 1.  Used by QueryWildCardExpansion().

Arguments:

    Str1    - A pointer to the 'base' string
    Str2    - A pointer to the string to be expanded

Return Value:

    TRUE if expansion was successful.

--*/
{
    CHNUM       idx;


    UNREFERENCED_PARAMETER( (void)this);

    // Deal with the * wild card first...
    //
    // Note: This method will ignore, even remove, any characters after the
    //       '*' in string 2.  This is to comform with the behavior of Dos...
    //
    if( ( idx = pStr2->Strchr( '*' ) ) != INVALID_CHNUM ) {
        if( idx > pStr1->QueryChCount() ) {
            return( FALSE );
        }
        if( idx == pStr1->QueryChCount() ) {
            pStr2->Truncate( idx );
        } else {
            pStr2->Replace( idx, TO_END, pStr1, idx, TO_END );
        }
    }

    // Now convert any '?' in the base
    while( ( idx = pStr2->Strchr( '?' ) ) != INVALID_CHNUM ) {
        // Make sure that the wild card is within the limits of the
        // base string...
        if( idx >= pStr1->QueryChCount() ) {
            return( FALSE );
        }
        pStr2->SetChAt( pStr1->QueryChAt( idx ), idx );
    }
    return( TRUE );
}
 

 
PWSTRING
PATH::QueryRoot (
    )

/*++

Routine Description:

    Returns a string that contains the canonicalized name of the root
    directory (device name followed by "\").

    QueryRoot returns NULL if there is no device component part of
    this path.  In other words it may be necessary to canonicalize
    the path before having access to the Root.

Arguments:

    none

Return Value:

    Pointer to a WSTRING that contains the root directory in its
    canonicalized form.

--*/

{
    PWSTRING    Root;

    if( _PathState.DeviceLen == 0 ) {
        return( NULL );
    }
    Root = NEW( DSTRING );

    if (Root == NULL) {
        DebugPrint("ULIB: Out of memory\n");
        return NULL;
    }

    if( !Root->Initialize( &_PathString, 0, _PathState.DeviceLen ) ) {
        DELETE( Root );
        return( NULL );
    }

    Root->Strcat( _SlashString );
    return( Root );
}

 

