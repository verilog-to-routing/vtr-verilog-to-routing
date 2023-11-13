/**CFile****************************************************************

  FileName    [abcSop.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Implementation of a simple SOP representation of nodes.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcSop.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"
#include "bool/kit/kit.h"

ABC_NAMESPACE_IMPL_START


/* 
    The SOPs in this package are represented using char * strings.
    For example, the SOP of the node: 

       .names c d0 d1 MUX
       01- 1
       1-1 1

    is the string: "01- 1\n1-1 1\n" where '\n' is a single char.
*/

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Registers the cube string with the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_SopRegister( Mem_Flex_t * pMan, const char * pName )
{
    char * pRegName;
    if ( pName == NULL ) return NULL;
    pRegName = Mem_FlexEntryFetch( pMan, strlen(pName) + 1 );
    strcpy( pRegName, pName );
    return pRegName;
}

/**Function*************************************************************

  Synopsis    [Creates the constant 1 cover with the given number of variables and cubes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_SopStart( Mem_Flex_t * pMan, int nCubes, int nVars )
{
    char * pSopCover, * pCube;
    int i, Length;

    Length = nCubes * (nVars + 3);
    pSopCover = Mem_FlexEntryFetch( pMan, Length + 1 );
    memset( pSopCover, '-', (size_t)Length );
    pSopCover[Length] = 0;

    for ( i = 0; i < nCubes; i++ )
    {
        pCube = pSopCover + i * (nVars + 3);
        pCube[nVars + 0] = ' ';
        pCube[nVars + 1] = '1';
        pCube[nVars + 2] = '\n';
    }
    return pSopCover;
}

/**Function*************************************************************

  Synopsis    [Creates the constant 1 cover with 0 variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_SopCreateConst1( Mem_Flex_t * pMan )
{
    return Abc_SopRegister( pMan, " 1\n" );
}

/**Function*************************************************************

  Synopsis    [Creates the constant 1 cover with 0 variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_SopCreateConst0( Mem_Flex_t * pMan )
{
    return Abc_SopRegister( pMan, " 0\n" );
}

/**Function*************************************************************

  Synopsis    [Creates the AND2 cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_SopCreateAnd2( Mem_Flex_t * pMan, int fCompl0, int fCompl1 )
{
    char Buffer[6];
    Buffer[0] = '1' - fCompl0;
    Buffer[1] = '1' - fCompl1;
    Buffer[2] = ' ';
    Buffer[3] = '1';
    Buffer[4] = '\n';
    Buffer[5] = 0;
    return Abc_SopRegister( pMan, Buffer );
}

/**Function*************************************************************

  Synopsis    [Creates the multi-input AND cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_SopCreateAnd( Mem_Flex_t * pMan, int nVars, int * pfCompl )
{
    char * pSop;
    int i;
    pSop = Abc_SopStart( pMan, 1, nVars );
    for ( i = 0; i < nVars; i++ )
        pSop[i] = '1' - (pfCompl? pfCompl[i] : 0);
    pSop[nVars + 1] = '1';
    return pSop;
}

/**Function*************************************************************

  Synopsis    [Creates the multi-input NAND cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_SopCreateNand( Mem_Flex_t * pMan, int nVars )
{
    char * pSop;
    int i;
    pSop = Abc_SopStart( pMan, 1, nVars );
    for ( i = 0; i < nVars; i++ )
        pSop[i] = '1';
    pSop[nVars + 1] = '0';
    return pSop;
}

/**Function*************************************************************

  Synopsis    [Creates the multi-input OR cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_SopCreateOr( Mem_Flex_t * pMan, int nVars, int * pfCompl )
{
    char * pSop;
    int i;
    pSop = Abc_SopStart( pMan, 1, nVars );
    for ( i = 0; i < nVars; i++ )
        pSop[i] = '0' + (pfCompl? pfCompl[i] : 0);
    pSop[nVars + 1] = '0';
    return pSop;
}

/**Function*************************************************************

  Synopsis    [Creates the multi-input OR cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_SopCreateOrMultiCube( Mem_Flex_t * pMan, int nVars, int * pfCompl )
{
    char * pSop, * pCube;
    int i;
    pSop = Abc_SopStart( pMan, nVars, nVars );
    i = 0;
    Abc_SopForEachCube( pSop, nVars, pCube )
    {
        pCube[i] = '1' - (pfCompl? pfCompl[i] : 0);
        i++;
    }
    return pSop;
}

/**Function*************************************************************

  Synopsis    [Creates the multi-input NOR cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_SopCreateNor( Mem_Flex_t * pMan, int nVars )
{
    char * pSop;
    int i;
    pSop = Abc_SopStart( pMan, 1, nVars );
    for ( i = 0; i < nVars; i++ )
        pSop[i] = '0';
    return pSop;
}

/**Function*************************************************************

  Synopsis    [Creates the multi-input XOR cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_SopCreateXor( Mem_Flex_t * pMan, int nVars )
{
    assert( nVars == 2 );
    return Abc_SopRegister(pMan, "01 1\n10 1\n");
}

/**Function*************************************************************

  Synopsis    [Creates the multi-input XOR cover (special case).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_SopCreateXorSpecial( Mem_Flex_t * pMan, int nVars )
{
    char * pSop;
    pSop = Abc_SopCreateAnd( pMan, nVars, NULL );
    pSop[nVars+1] = 'x';
    assert( pSop[nVars+2] == '\n' );
    return pSop;
}

/**Function*************************************************************

  Synopsis    [Creates the multi-input XNOR cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_SopCreateNxor( Mem_Flex_t * pMan, int nVars )
{
    assert( nVars == 2 );
    return Abc_SopRegister(pMan, "11 1\n00 1\n");
}

/**Function*************************************************************

  Synopsis    [Creates the MUX cover.]

  Description [The first input of MUX is the control. The second input
  is DATA1. The third input is DATA0.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_SopCreateMux( Mem_Flex_t * pMan )
{
    return Abc_SopRegister(pMan, "11- 1\n0-1 1\n");
}

/**Function*************************************************************

  Synopsis    [Creates the inv cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_SopCreateInv( Mem_Flex_t * pMan )
{
    return Abc_SopRegister(pMan, "0 1\n");
}

/**Function*************************************************************

  Synopsis    [Creates the buf cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_SopCreateBuf( Mem_Flex_t * pMan )
{
    return Abc_SopRegister(pMan, "1 1\n");
}

/**Function*************************************************************

  Synopsis    [Creates the arbitrary cover from the truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_SopCreateFromTruth( Mem_Flex_t * pMan, int nVars, unsigned * pTruth )
{
    char * pSop, * pCube;
    int nMints, Counter, i, k;
    if ( nVars == 0 )
        return pTruth[0] ? Abc_SopCreateConst1(pMan) : Abc_SopCreateConst0(pMan);
    // count the number of true minterms
    Counter = 0;
    nMints = (1 << nVars);
    for ( i = 0; i < nMints; i++ )
        Counter += ((pTruth[i>>5] & (1 << (i&31))) > 0);
    // SOP is not well-defined if the truth table is constant 0
    assert( Counter > 0 );
    if ( Counter == 0 )
        return NULL;
    // start the cover
    pSop = Abc_SopStart( pMan, Counter, nVars );
    // create true minterms
    Counter = 0;
    for ( i = 0; i < nMints; i++ )
        if ( (pTruth[i>>5] & (1 << (i&31))) > 0 )
        {
            pCube = pSop + Counter * (nVars + 3);
            for ( k = 0; k < nVars; k++ )
                pCube[k] = '0' + ((i & (1 << k)) > 0);
            Counter++;
        }
    return pSop;
}

/**Function*************************************************************

  Synopsis    [Creates the cover from the ISOP computed from TT.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_SopCreateFromIsop( Mem_Flex_t * pMan, int nVars, Vec_Int_t * vCover )
{
    char * pSop, * pCube;
    int i, k, Entry, Literal;
    assert( Vec_IntSize(vCover) > 0 );
    if ( Vec_IntSize(vCover) == 0 )
        return NULL;
    // start the cover
    pSop = Abc_SopStart( pMan, Vec_IntSize(vCover), nVars );
    // create cubes
    Vec_IntForEachEntry( vCover, Entry, i )
    {
        pCube = pSop + i * (nVars + 3);
        for ( k = 0; k < nVars; k++ )
        {
            Literal = 3 & (Entry >> (k << 1));
            if ( Literal == 1 )
                pCube[k] = '0';
            else if ( Literal == 2 )
                pCube[k] = '1';
            else if ( Literal != 0 )
                assert( 0 );
        }
    }
    return pSop;
}

/**Function*************************************************************

  Synopsis    [Creates the cover from the ISOP computed from TT.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_SopCreateFromTruthIsop( Mem_Flex_t * pMan, int nVars, word * pTruth, Vec_Int_t * vCover )
{
    char * pSop = NULL;
    int w, nWords  = Abc_Truth6WordNum( nVars );
    assert( nVars < 16 );

    for ( w = 0; w < nWords; w++ )
        if ( pTruth[w] )
            break;
    if ( w == nWords )
        return Abc_SopRegister( pMan, " 0\n" );

    for ( w = 0; w < nWords; w++ )
        if ( ~pTruth[w] )
            break;
    if ( w == nWords )
        return Abc_SopRegister( pMan, " 1\n" );

    {
        int RetValue = Kit_TruthIsop( (unsigned *)pTruth, nVars, vCover, 1 );
        assert( nVars > 0 );
        assert( RetValue == 0 || RetValue == 1 );
        pSop = Abc_SopCreateFromIsop( pMan, nVars, vCover );
        if ( RetValue )
            Abc_SopComplement( pSop );
    }
    return pSop;
}

/**Function*************************************************************

  Synopsis    [Creates the cover from the ISOP computed from TT.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SopToIsop( char * pSop, Vec_Int_t * vCover )
{
    char * pCube;
    int k, nVars, Entry;
    nVars = Abc_SopGetVarNum( pSop );
    assert( nVars > 0 );
    // create cubes
    Vec_IntClear( vCover );
    for ( pCube = pSop; *pCube; pCube += nVars + 3 )
    {
        Entry = 0;
        for ( k = nVars - 1; k >= 0; k-- )
            if ( pCube[k] == '0' )
                Entry = (Entry << 2) | 1;
            else if ( pCube[k] == '1' )
                Entry = (Entry << 2) | 2;
            else if ( pCube[k] == '-' )
                Entry = (Entry << 2);
            else 
                assert( 0 );
        Vec_IntPush( vCover, Entry );
    }
}

/**Function*************************************************************

  Synopsis    [Reads the number of cubes in the cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_SopGetCubeNum( char * pSop )
{
    char * pCur;
    int nCubes = 0;
    if ( pSop == NULL )
        return 0;
    for ( pCur = pSop; *pCur; pCur++ )
        nCubes += (*pCur == '\n');
    return nCubes;
}

/**Function*************************************************************

  Synopsis    [Reads the number of SOP literals in the cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_SopGetLitNum( char * pSop )
{
    char * pCur;
    int nLits = 0;
    if ( pSop == NULL )
        return 0;
    for ( pCur = pSop; *pCur; pCur++ )
    {
        nLits  -= (*pCur == '\n');
        nLits  += (*pCur == '0' || *pCur == '1');
    }
    return nLits;
}

/**Function*************************************************************

  Synopsis    [Reads the number of variables in the cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_SopGetVarNum( char * pSop )
{
    char * pCur;
    for ( pCur = pSop; *pCur != '\n'; pCur++ )
        if ( *pCur == 0 )
            return -1;
    return pCur - pSop - 2;
}

/**Function*************************************************************

  Synopsis    [Reads the phase of the cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_SopGetPhase( char * pSop )
{
    int nVars = Abc_SopGetVarNum( pSop );
    if ( pSop[nVars+1] == '0' || pSop[nVars+1] == 'n' )
        return 0;
    if ( pSop[nVars+1] == '1' || pSop[nVars+1] == 'x' )
        return 1;
    assert( 0 );
    return -1;
}

/**Function*************************************************************

  Synopsis    [Returns the i-th literal of the cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_SopGetIthCareLit( char * pSop, int i )
{
    char * pCube;
    int nVars;
    nVars = Abc_SopGetVarNum( pSop );
    Abc_SopForEachCube( pSop, nVars, pCube )
        if ( pCube[i] != '-' )
            return pCube[i] - '0';
    return -1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SopComplement( char * pSop )
{
    char * pCur;
    for ( pCur = pSop; *pCur; pCur++ )
        if ( *pCur == '\n' )
        {
            if ( *(pCur - 1) == '0' )
                *(pCur - 1) = '1';
            else if ( *(pCur - 1) == '1' )
                *(pCur - 1) = '0';
            else if ( *(pCur - 1) == 'x' )
                *(pCur - 1) = 'n';
            else if ( *(pCur - 1) == 'n' )
                *(pCur - 1) = 'x';
            else
                assert( 0 );
        }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SopComplementVar( char * pSop, int iVar )
{
    char * pCube;
    int nVars = Abc_SopGetVarNum(pSop);
    assert( iVar < nVars );
    Abc_SopForEachCube( pSop, nVars, pCube )
    {
        if ( pCube[iVar] == '0' )
            pCube[iVar] = '1';
        else if ( pCube[iVar] == '1' )
            pCube[iVar] = '0';
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_SopIsComplement( char * pSop )
{
    char * pCur;
    for ( pCur = pSop; *pCur; pCur++ )
        if ( *pCur == '\n' )
            return (int)(*(pCur - 1) == '0' || *(pCur - 1) == 'n');
    assert( 0 );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Checks if the cover is constant 0.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_SopIsConst0( char * pSop )
{
    return pSop[0] == ' ' && pSop[1] == '0';
}

/**Function*************************************************************

  Synopsis    [Checks if the cover is constant 1.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_SopIsConst1( char * pSop )
{
    return pSop[0] == ' ' && pSop[1] == '1';
}

/**Function*************************************************************

  Synopsis    [Checks if the cover is constant 1.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_SopIsBuf( char * pSop )
{
    if ( pSop[4] != 0 )
        return 0;
    if ( (pSop[0] == '1' && pSop[2] == '1') || (pSop[0] == '0' && pSop[2] == '0') )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Checks if the cover is constant 1.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_SopIsInv( char * pSop )
{
    if ( pSop[4] != 0 )
        return 0;
    if ( (pSop[0] == '0' && pSop[2] == '1') || (pSop[0] == '1' && pSop[2] == '0') )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Checks if the cover is AND with possibly complemented inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_SopIsAndType( char * pSop )
{
    char * pCur;
    if ( Abc_SopGetCubeNum(pSop) != 1 )
        return 0;
    for ( pCur = pSop; *pCur != ' '; pCur++ )
        if ( *pCur == '-' )
            return 0;
    if ( pCur[1] != '1' )
        return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Checks if the cover is OR with possibly complemented inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_SopIsOrType( char * pSop )
{
    char * pCube, * pCur;
    int nVars, nLits;
    nVars = Abc_SopGetVarNum( pSop );
    if ( nVars != Abc_SopGetCubeNum(pSop) )
        return 0;
    Abc_SopForEachCube( pSop, nVars, pCube )
    {
        // count the number of literals in the cube
        nLits = 0;
        for ( pCur = pCube; *pCur != ' '; pCur++ )
            nLits += ( *pCur != '-' );
        if ( nLits != 1 )
            return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_SopIsExorType( char * pSop )
{
    char * pCur;
    for ( pCur = pSop; *pCur; pCur++ )
        if ( *pCur == '\n' )
            return (int)(*(pCur - 1) == 'x' || *(pCur - 1) == 'n');
    assert( 0 );
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_SopCheck( char * pSop, int nFanins )
{
    char * pCubes, * pCubesOld;
    int fFound0 = 0, fFound1 = 0;

    // check the logic function of the node
    for ( pCubes = pSop; *pCubes; pCubes++ )
    {
        // get the end of the next cube
        for ( pCubesOld = pCubes; *pCubes != ' '; pCubes++ );
        // compare the distance
        if ( pCubes - pCubesOld != nFanins )
        {
            fprintf( stdout, "Abc_SopCheck: SOP has a mismatch between its cover size (%d) and its fanin number (%d).\n",
                (int)(ABC_PTRDIFF_T)(pCubes - pCubesOld), nFanins );
            return 0;
        }
        // check the output values for this cube
        pCubes++;
        if ( *pCubes == '0' )
            fFound0 = 1;
        else if ( *pCubes == '1' )
            fFound1 = 1;
        else if ( *pCubes != 'x' && *pCubes != 'n' )
        {
            fprintf( stdout, "Abc_SopCheck: SOP has a strange character (%c) in the output part of its cube.\n", *pCubes );
            return 0;
        }
        // check the last symbol (new line)
        pCubes++;
        if ( *pCubes != '\n' )
        {
            fprintf( stdout, "Abc_SopCheck: SOP has a cube without new line in the end.\n" );
            return 0;
        }
    }
    if ( fFound0 && fFound1 )
    {
        fprintf( stdout, "Abc_SopCheck: SOP has cubes in both phases.\n" );
        return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_SopCheckReadTruth( Vec_Ptr_t * vRes, char * pToken, int fHex )
{
    char * pBase; int nVars;
    int Log2 = Abc_Base2Log( strlen(pToken) );
    if ( fHex && strlen(pToken) == 1 )
        Log2 = 0;     
    if ( (1 << Log2) != (int)strlen(pToken) )
    {
        printf( "The truth table length (%d) is not power-of-2.\n", (int)strlen(pToken) );
        Vec_PtrFreeData( vRes );
        Vec_PtrShrink( vRes, 0 );
        return 0;
    }
    if ( Vec_PtrSize(vRes) == 0 )
        return 1;
    pBase = (char *)Vec_PtrEntry( vRes, 0 );
    nVars = Abc_SopGetVarNum( pBase );
    if ( nVars != Log2+2*fHex )
    {
        printf( "Truth table #1 has %d vars while truth table #%d has %d vars.\n", nVars, Vec_PtrSize(vRes)+1, Log2+2*fHex );
        Vec_PtrFreeData( vRes );
        Vec_PtrShrink( vRes, 0 );
        return 0;
    }   
    return 1;
}

/**Function*************************************************************

  Synopsis    [Derives SOP from the truth table representation.]

  Description [Truth table is expected to be in the hexadecimal notation.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_SopFromTruthBin( char * pTruth )
{
    char * pSopCover, * pCube;
    int nTruthSize, nVars, Digit, Length, Mint, i, b;
    Vec_Int_t * vMints;

    // get the number of variables
    nTruthSize = strlen(pTruth);
    nVars = Abc_Base2Log( nTruthSize );
    if ( nTruthSize != (1 << (nVars)) )
    {
        printf( "String %s does not look like a truth table of a %d-variable function.\n", pTruth, nVars );
        return NULL;
    }

    // collect the on-set minterms
    vMints = Vec_IntAlloc( 100 );
    for ( i = 0; i < nTruthSize; i++ )
    {
        if ( pTruth[i] >= '0' && pTruth[i] <= '1' )
            Digit = pTruth[i] - '0';
        else
        {
            Vec_IntFree( vMints );
            printf( "String %s does not look like a binary representation of the truth table.\n", pTruth );
            return NULL;
        }
        if ( Digit == 1 )
            Vec_IntPush( vMints, nTruthSize - 1 - i );
    }
/*    
    if ( Vec_IntSize( vMints ) == 0 || Vec_IntSize( vMints ) == nTruthSize )
    {
        Vec_IntFree( vMints );
        printf( "Cannot create constant function.\n" );
        return NULL;
    }
*/
    if ( Vec_IntSize(vMints) == 0 || Vec_IntSize(vMints) == (1 << nVars) )
    {
        pSopCover = ABC_ALLOC( char, 4 );
        pSopCover[0] = ' ';
        pSopCover[1] = '0' + (Vec_IntSize(vMints) > 0);
        pSopCover[2] = '\n';                        
        pSopCover[3] = 0;
    }
    else
    {
        // create the SOP representation of the minterms
        Length = Vec_IntSize(vMints) * (nVars + 3);
        pSopCover = ABC_ALLOC( char, Length + 1 );
        pSopCover[Length] = 0;
        Vec_IntForEachEntry( vMints, Mint, i )
        {
            pCube = pSopCover + i * (nVars + 3);
            for ( b = 0; b < nVars; b++ )
    //            if ( Mint & (1 << (nVars-1-b)) )
                if ( Mint & (1 << b) )
                    pCube[b] = '1';
                else
                    pCube[b] = '0';
            pCube[nVars + 0] = ' ';
            pCube[nVars + 1] = '1';
            pCube[nVars + 2] = '\n';
        }
    }
    Vec_IntFree( vMints );
    return pSopCover;
}
Vec_Ptr_t * Abc_SopFromTruthsBin( char * pTruth )
{
    Vec_Ptr_t * vRes = Vec_PtrAlloc( 10 );
    char * pCopy = Abc_UtilStrsav(pTruth);
    char * pToken = strtok( pCopy, " \r\n\t|" );
    while ( pToken )
    {
        if ( !Abc_SopCheckReadTruth( vRes, pToken, 0 ) )
            break;
        Vec_PtrPush( vRes, Abc_SopFromTruthBin(pToken) );
        pToken = strtok( NULL, " \r\n\t|" );
    }
    ABC_FREE( pCopy );    
    return vRes;
}

/**Function*************************************************************

  Synopsis    [Derives SOP from the truth table representation.]

  Description [Truth table is expected to be in the hexadecimal notation.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_SopFromTruthHex( char * pTruth )
{
    char * pSopCover, * pCube;
    int nTruthSize, nVars, Digit, Length, Mint, i, b;
    Vec_Int_t * vMints;

    // get the number of variables
    nTruthSize = strlen(pTruth);
    nVars = (nTruthSize < 2) ? 2 : Abc_Base2Log(nTruthSize) + 2;
    if ( nTruthSize != (1 << (nVars-2)) )
    {
        printf( "String %s does not look like a truth table of a %d-variable function.\n", pTruth, nVars );
        return NULL;
    }

    // collect the on-set minterms
    vMints = Vec_IntAlloc( 100 );
    for ( i = 0; i < nTruthSize; i++ )
    {
        if ( pTruth[i] >= '0' && pTruth[i] <= '9' )
            Digit = pTruth[i] - '0';
        else if ( pTruth[i] >= 'a' && pTruth[i] <= 'f' )
            Digit = 10 + pTruth[i] - 'a';
        else if ( pTruth[i] >= 'A' && pTruth[i] <= 'F' )
            Digit = 10 + pTruth[i] - 'A';
        else
        {
            printf( "String %s does not look like a hexadecimal representation of the truth table.\n", pTruth );
            return NULL;
        }
        for ( b = 0; b < 4; b++ )
            if ( Digit & (1 << b) )
                Vec_IntPush( vMints, 4*(nTruthSize-1-i)+b );
    }

    // create the SOP representation of the minterms
    if ( Vec_IntSize(vMints) == 0 || Vec_IntSize(vMints) == (1 << nVars) )
    {
        pSopCover = ABC_ALLOC( char, 4 );
        pSopCover[0] = ' ';
        pSopCover[1] = '0' + (Vec_IntSize(vMints) > 0);
        pSopCover[2] = '\n';                        
        pSopCover[3] = 0;
    }
    else
    {
        Length = Vec_IntSize(vMints) * (nVars + 3);
        pSopCover = ABC_ALLOC( char, Length + 1 );
        pSopCover[Length] = 0;
        Vec_IntForEachEntry( vMints, Mint, i )
        {
            pCube = pSopCover + i * (nVars + 3);
            for ( b = 0; b < nVars; b++ )
    //            if ( Mint & (1 << (nVars-1-b)) )
                if ( Mint & (1 << b) )
                    pCube[b] = '1';
                else
                    pCube[b] = '0';
            pCube[nVars + 0] = ' ';
            pCube[nVars + 1] = '1';
            pCube[nVars + 2] = '\n';
        }
    }

    Vec_IntFree( vMints );
    return pSopCover;
}
Vec_Ptr_t * Abc_SopFromTruthsHex( char * pTruth )
{
    Vec_Ptr_t * vRes = Vec_PtrAlloc( 10 );
    char * pCopy = Abc_UtilStrsav(pTruth);
    char * pToken = strtok( pCopy, " \r\n\t|" );
    while ( pToken )
    {
        if ( !Abc_SopCheckReadTruth( vRes, pToken, 1 ) )
            break;
        Vec_PtrPush( vRes, Abc_SopFromTruthHex(pToken) );
        pToken = strtok( NULL, " \r\n\t|" );
    }
    ABC_FREE( pCopy );
    return vRes;
}

/**Function*************************************************************

  Synopsis    [Creates one encoder node.]

  Description [Produces MV-SOP for BLIF-MV representation.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_SopEncoderPos( Mem_Flex_t * pMan, int iValue, int nValues )
{
    char Buffer[32];
    assert( iValue < nValues );
    sprintf( Buffer, "d0\n%d 1\n", iValue );
    return Abc_SopRegister( pMan, Buffer );
}

/**Function*************************************************************

  Synopsis    [Creates one encoder node.]

  Description [Produces MV-SOP for BLIF-MV representation.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_SopEncoderLog( Mem_Flex_t * pMan, int iBit, int nValues )
{
    char * pResult;
    Vec_Str_t * vSop;
    int v, Counter, fFirst = 1, nBits = Abc_Base2Log(nValues);
    assert( iBit < nBits );
    // count the number of literals
    Counter = 0;
    for ( v = 0; v < nValues; v++ )
        Counter += ( (v & (1 << iBit)) > 0 );
    // create the cover
    vSop = Vec_StrAlloc( 100 );
    Vec_StrPrintStr( vSop, "d0\n" );
    if ( Counter > 1 )
        Vec_StrPrintStr( vSop, "(" );
    for ( v = 0; v < nValues; v++ )
        if ( v & (1 << iBit) )
        {
            if ( fFirst )
                fFirst = 0;
            else
                Vec_StrPush( vSop, ',' );
            Vec_StrPrintNum( vSop, v );
        }
    if ( Counter > 1 )
        Vec_StrPrintStr( vSop, ")" );
    Vec_StrPrintStr( vSop, " 1\n" );
    Vec_StrPush( vSop, 0 );
    pResult = Abc_SopRegister( pMan, Vec_StrArray(vSop) );
    Vec_StrFree( vSop );
    return pResult;
}

/**Function*************************************************************

  Synopsis    [Creates the decoder node.]

  Description [Produces MV-SOP for BLIF-MV representation.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_SopDecoderPos( Mem_Flex_t * pMan, int nValues )
{
    char * pResult;
    Vec_Str_t * vSop;
    int i, k;
    assert( nValues > 1 );
    vSop = Vec_StrAlloc( 100 );
    for ( i = 0; i < nValues; i++ )
    {
        for ( k = 0; k < nValues; k++ )
        {
            if ( k == i )
                Vec_StrPrintStr( vSop, "1 " );
            else
                Vec_StrPrintStr( vSop, "- " );
        }
        Vec_StrPrintNum( vSop, i );
        Vec_StrPush( vSop, '\n' );
    }
    Vec_StrPush( vSop, 0 );
    pResult = Abc_SopRegister( pMan, Vec_StrArray(vSop) );
    Vec_StrFree( vSop );
    return pResult;
}

/**Function*************************************************************

  Synopsis    [Creates the decover node.]

  Description [Produces MV-SOP for BLIF-MV representation.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_SopDecoderLog( Mem_Flex_t * pMan, int nValues )
{
    char * pResult;
    Vec_Str_t * vSop;
    int i, b, nBits = Abc_Base2Log(nValues);
    assert( nValues > 1 && nValues <= (1<<nBits) );
    vSop = Vec_StrAlloc( 100 );
    for ( i = 0; i < nValues; i++ )
    {
        for ( b = 0; b < nBits; b++ )
        {
            Vec_StrPrintNum( vSop, (int)((i & (1 << b)) > 0) );
            Vec_StrPush( vSop, ' ' );
        }
        Vec_StrPrintNum( vSop, i );
        Vec_StrPush( vSop, '\n' );
    }
    Vec_StrPush( vSop, 0 );
    pResult = Abc_SopRegister( pMan, Vec_StrArray(vSop) );
    Vec_StrFree( vSop );
    return pResult;
}

/**Function*************************************************************

  Synopsis    [Computes truth table of the node.]
 
  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
word Abc_SopToTruth( char * pSop, int nInputs )
{
    static word Truth[8] = {
        ABC_CONST(0xAAAAAAAAAAAAAAAA),
        ABC_CONST(0xCCCCCCCCCCCCCCCC),
        ABC_CONST(0xF0F0F0F0F0F0F0F0),
        ABC_CONST(0xFF00FF00FF00FF00),
        ABC_CONST(0xFFFF0000FFFF0000),
        ABC_CONST(0xFFFFFFFF00000000),
        ABC_CONST(0x0000000000000000),
        ABC_CONST(0xFFFFFFFFFFFFFFFF)
    };
    word Cube, Result = 0;
    int v, lit = 0;
    int nVars = Abc_SopGetVarNum(pSop);
    assert( nVars >= 0 && nVars <= 6 );
    assert( nVars == nInputs );
    do {
        Cube = Truth[7];
        for ( v = 0; v < nVars; v++, lit++ )
        {
            if ( pSop[lit] == '1' )
                Cube &=  Truth[v];
            else if ( pSop[lit] == '0' )
                Cube &= ~Truth[v];
            else if ( pSop[lit] != '-' )
                assert( 0 );
        }
        Result |= Cube;
        assert( pSop[lit] == ' ' );
        lit++;
        lit++;
        assert( pSop[lit] == '\n' );
        lit++;
    } while ( pSop[lit] );
    if ( Abc_SopIsComplement(pSop) )
        Result = ~Result;
    return Result;
}

/**Function*************************************************************

  Synopsis    [Computes truth table of the node.]
 
  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SopToTruth7( char * pSop, int nInputs, word r[2] )
{
    static word Truth[7][2] = {
        {ABC_CONST(0xAAAAAAAAAAAAAAAA),ABC_CONST(0xAAAAAAAAAAAAAAAA)},
        {ABC_CONST(0xCCCCCCCCCCCCCCCC),ABC_CONST(0xCCCCCCCCCCCCCCCC)},
        {ABC_CONST(0xF0F0F0F0F0F0F0F0),ABC_CONST(0xF0F0F0F0F0F0F0F0)},
        {ABC_CONST(0xFF00FF00FF00FF00),ABC_CONST(0xFF00FF00FF00FF00)},
        {ABC_CONST(0xFFFF0000FFFF0000),ABC_CONST(0xFFFF0000FFFF0000)},
        {ABC_CONST(0xFFFFFFFF00000000),ABC_CONST(0xFFFFFFFF00000000)},
        {ABC_CONST(0x0000000000000000),ABC_CONST(0xFFFFFFFFFFFFFFFF)},
    };
    word Cube[2];
    int v, lit = 0;
    int nVars = Abc_SopGetVarNum(pSop);
    assert( nVars >= 0 && nVars <= 7 );
    assert( nVars == nInputs );
    r[0] = r[1] = 0;
    do {
        Cube[0] = Cube[1] = ~(word)0;
        for ( v = 0; v < nVars; v++, lit++ )
        {
            if ( pSop[lit] == '1' )
            {
                Cube[0] &=  Truth[v][0];
                Cube[1] &=  Truth[v][1];
            }
            else if ( pSop[lit] == '0' )
            {
                Cube[0] &= ~Truth[v][0];
                Cube[1] &= ~Truth[v][1];
            }
            else if ( pSop[lit] != '-' )
                assert( 0 );
        }
        r[0] |= Cube[0];
        r[1] |= Cube[1];
        assert( pSop[lit] == ' ' );
        lit++;
        lit++;
        assert( pSop[lit] == '\n' );
        lit++;
    } while ( pSop[lit] );
    if ( Abc_SopIsComplement(pSop) )
    {
        r[0] = ~r[0];
        r[1] = ~r[1];
    }
}

/**Function*************************************************************

  Synopsis    [Computes truth table of the node.]
 
  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SopToTruthBig( char * pSop, int nInputs, word ** pVars, word * pCube, word * pRes )
{
    int nVars = Abc_SopGetVarNum(pSop);
    int nWords = nVars <= 6 ? 1 : 1 << (nVars-6);
    int v, i, lit = 0;
    assert( nVars >= 0 && nVars <= 16 );
    assert( nVars == nInputs );
    for ( i = 0; i < nWords; i++ )
        pRes[i] = 0;
    do {
        for ( i = 0; i < nWords; i++ )
            pCube[i] = ~(word)0;
        for ( v = 0; v < nVars; v++, lit++ )
        {
            if ( pSop[lit] == '1' )
            {
                for ( i = 0; i < nWords; i++ )
                    pCube[i] &= pVars[v][i];
            }
            else if ( pSop[lit] == '0' )
            {
                for ( i = 0; i < nWords; i++ )
                    pCube[i] &= ~pVars[v][i];
            }
            else if ( pSop[lit] != '-' )
                assert( 0 );
        }
        for ( i = 0; i < nWords; i++ )
            pRes[i] |= pCube[i];
        assert( pSop[lit] == ' ' );
        lit++;
        lit++;
        assert( pSop[lit] == '\n' );
        lit++;
    } while ( pSop[lit] );
    if ( Abc_SopIsComplement(pSop) )
    {
        for ( i = 0; i < nWords; i++ )
            pRes[i] = ~pRes[i];
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

