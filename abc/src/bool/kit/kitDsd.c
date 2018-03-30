/**CFile****************************************************************

  FileName    [kitDsd.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Computation kit.]

  Synopsis    [Performs disjoint-support decomposition based on truth tables.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Dec 6, 2006.]

  Revision    [$Id: kitDsd.c,v 1.00 2006/12/06 00:00:00 alanmi Exp $]

***********************************************************************/

#include "kit.h"
#include "misc/extra/extra.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates the DSD manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Kit_DsdMan_t * Kit_DsdManAlloc( int nVars, int nNodes )
{
    Kit_DsdMan_t * p;
    p = ABC_ALLOC( Kit_DsdMan_t, 1 );
    memset( p, 0, sizeof(Kit_DsdMan_t) );
    p->nVars    = nVars;
    p->nWords   = Kit_TruthWordNum( p->nVars );
    p->vTtElems = Vec_PtrAllocTruthTables( p->nVars );
    p->vTtNodes = Vec_PtrAllocSimInfo( nNodes, p->nWords );
    p->dd       = Cloud_Init( 16, 14 );
    p->vTtBdds  = Vec_PtrAllocSimInfo( (1<<12), p->nWords );
    p->vNodes   = Vec_IntAlloc( 512 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Deallocates the DSD manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_DsdManFree( Kit_DsdMan_t * p )
{
    Cloud_Quit( p->dd );
    Vec_IntFree( p->vNodes );
    Vec_PtrFree( p->vTtBdds );
    Vec_PtrFree( p->vTtElems );
    Vec_PtrFree( p->vTtNodes );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Allocates the DSD node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Kit_DsdObj_t * Kit_DsdObjAlloc( Kit_DsdNtk_t * pNtk, Kit_Dsd_t Type, int nFans )
{
    Kit_DsdObj_t * pObj;
    int nSize = sizeof(Kit_DsdObj_t) + sizeof(unsigned) * (Kit_DsdObjOffset(nFans) + (Type == KIT_DSD_PRIME) * Kit_TruthWordNum(nFans));
    pObj = (Kit_DsdObj_t *)ABC_ALLOC( char, nSize );
    memset( pObj, 0, nSize );
    pObj->Id = pNtk->nVars + pNtk->nNodes;
    pObj->Type = Type;
    pObj->nFans = nFans;
    pObj->Offset = Kit_DsdObjOffset( nFans );
    // add the object
    if ( pNtk->nNodes == pNtk->nNodesAlloc )
    {
        pNtk->nNodesAlloc *= 2;
        pNtk->pNodes = ABC_REALLOC( Kit_DsdObj_t *, pNtk->pNodes, pNtk->nNodesAlloc ); 
    }
    assert( pNtk->nNodes < pNtk->nNodesAlloc );
    pNtk->pNodes[pNtk->nNodes++] = pObj;
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Deallocates the DSD node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_DsdObjFree( Kit_DsdNtk_t * p, Kit_DsdObj_t * pObj )
{
    ABC_FREE( pObj );
}

/**Function*************************************************************

  Synopsis    [Allocates the DSD network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Kit_DsdNtk_t * Kit_DsdNtkAlloc( int nVars )
{
    Kit_DsdNtk_t * pNtk;
    pNtk = ABC_ALLOC( Kit_DsdNtk_t, 1 );
    memset( pNtk, 0, sizeof(Kit_DsdNtk_t) );
    pNtk->pNodes = ABC_ALLOC( Kit_DsdObj_t *, nVars+1 );
    pNtk->nVars = nVars;
    pNtk->nNodesAlloc = nVars+1;
    pNtk->pMem = ABC_ALLOC( unsigned, 6 * Kit_TruthWordNum(nVars) );
    return pNtk;
}

/**Function*************************************************************

  Synopsis    [Deallocate the DSD network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_DsdNtkFree( Kit_DsdNtk_t * pNtk )
{
    Kit_DsdObj_t * pObj;
    unsigned i;
    Kit_DsdNtkForEachObj( pNtk, pObj, i )
        ABC_FREE( pObj );
    ABC_FREE( pNtk->pSupps );
    ABC_FREE( pNtk->pNodes );
    ABC_FREE( pNtk->pMem );
    ABC_FREE( pNtk );
}

/**Function*************************************************************

  Synopsis    [Prints the hex unsigned into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_DsdPrintHex( FILE * pFile, unsigned * pTruth, int nFans )
{
    int nDigits, Digit, k;
    nDigits = (1 << nFans) / 4;
    for ( k = nDigits - 1; k >= 0; k-- )
    {
        Digit = ((pTruth[k/8] >> ((k%8) * 4)) & 15);
        if ( Digit < 10 )
            fprintf( pFile, "%d", Digit );
        else
            fprintf( pFile, "%c", 'A' + Digit-10 );
    }
}

/**Function*************************************************************

  Synopsis    [Prints the hex unsigned into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Kit_DsdWriteHex( char * pBuff, unsigned * pTruth, int nFans )
{
    int nDigits, Digit, k;
    nDigits = (1 << nFans) / 4;
    for ( k = nDigits - 1; k >= 0; k-- )
    {
        Digit = ((pTruth[k/8] >> ((k%8) * 4)) & 15);
        if ( Digit < 10 )
            *pBuff++ = '0' + Digit;
        else
            *pBuff++ = 'A' + Digit-10;
    }
    return pBuff;
}

/**Function*************************************************************

  Synopsis    [Recursively print the DSD formula.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_DsdPrint2_rec( FILE * pFile, Kit_DsdNtk_t * pNtk, int Id )
{
    Kit_DsdObj_t * pObj;
    unsigned iLit, i;
    char Symbol;

    pObj = Kit_DsdNtkObj( pNtk, Id );
    if ( pObj == NULL )
    {
        assert( Id < pNtk->nVars );
        fprintf( pFile, "%c", 'a' + Id );
        return;
    }

    if ( pObj->Type == KIT_DSD_CONST1 )
    {
        assert( pObj->nFans == 0 );
        fprintf( pFile, "Const1" );
        return;
    }

    if ( pObj->Type == KIT_DSD_VAR )
        assert( pObj->nFans == 1 );

    if ( pObj->Type == KIT_DSD_AND )
        Symbol = '*';
    else if ( pObj->Type == KIT_DSD_XOR )
        Symbol = '+';
    else 
        Symbol = ',';

    if ( pObj->Type == KIT_DSD_PRIME )
        fprintf( pFile, "[" );
    else
        fprintf( pFile, "(" );
    Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )
    {
        if ( Abc_LitIsCompl(iLit) ) 
            fprintf( pFile, "!" );
        Kit_DsdPrint2_rec( pFile, pNtk, Abc_Lit2Var(iLit) );
        if ( i < pObj->nFans - 1 )
            fprintf( pFile, "%c", Symbol );
    }
    if ( pObj->Type == KIT_DSD_PRIME )
        fprintf( pFile, "]" );
    else
        fprintf( pFile, ")" );
}

/**Function*************************************************************

  Synopsis    [Print the DSD formula.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_DsdPrint2( FILE * pFile, Kit_DsdNtk_t * pNtk )
{
//    fprintf( pFile, "F = " );
    if ( Abc_LitIsCompl(pNtk->Root) )
        fprintf( pFile, "!" );
    Kit_DsdPrint2_rec( pFile, pNtk, Abc_Lit2Var(pNtk->Root) );
//    fprintf( pFile, "\n" );
}

/**Function*************************************************************

  Synopsis    [Recursively print the DSD formula.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_DsdPrint_rec( FILE * pFile, Kit_DsdNtk_t * pNtk, int Id )
{
    Kit_DsdObj_t * pObj;
    unsigned iLit, i;
    char Symbol;

    pObj = Kit_DsdNtkObj( pNtk, Id );
    if ( pObj == NULL )
    {
        assert( Id < pNtk->nVars );
        fprintf( pFile, "%c", 'a' + Id );
        return;
    }

    if ( pObj->Type == KIT_DSD_CONST1 )
    {
        assert( pObj->nFans == 0 );
        fprintf( pFile, "Const1" );
        return;
    }

    if ( pObj->Type == KIT_DSD_VAR )
        assert( pObj->nFans == 1 );

    if ( pObj->Type == KIT_DSD_AND )
        Symbol = '*';
    else if ( pObj->Type == KIT_DSD_XOR )
        Symbol = '+';
    else 
        Symbol = ',';

    if ( pObj->Type == KIT_DSD_PRIME )
        Kit_DsdPrintHex( pFile, Kit_DsdObjTruth(pObj), pObj->nFans );

    fprintf( pFile, "(" );
    Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )
    {
        if ( Abc_LitIsCompl(iLit) ) 
            fprintf( pFile, "!" );
        Kit_DsdPrint_rec( pFile, pNtk, Abc_Lit2Var(iLit) );
        if ( i < pObj->nFans - 1 )
            fprintf( pFile, "%c", Symbol );
    }
    fprintf( pFile, ")" );
}

/**Function*************************************************************

  Synopsis    [Print the DSD formula.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_DsdPrint( FILE * pFile, Kit_DsdNtk_t * pNtk )
{
    fprintf( pFile, "F = " );
    if ( Abc_LitIsCompl(pNtk->Root) )
        fprintf( pFile, "!" );
    Kit_DsdPrint_rec( pFile, pNtk, Abc_Lit2Var(pNtk->Root) );
//    fprintf( pFile, "\n" );
}

/**Function*************************************************************

  Synopsis    [Recursively print the DSD formula.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Kit_DsdWrite_rec( char * pBuff, Kit_DsdNtk_t * pNtk, int Id )
{
    Kit_DsdObj_t * pObj;
    unsigned iLit, i;
    char Symbol;

    pObj = Kit_DsdNtkObj( pNtk, Id );
    if ( pObj == NULL )
    {
        assert( Id < pNtk->nVars );
        *pBuff++ = 'a' + Id;
        return pBuff;
    }

    if ( pObj->Type == KIT_DSD_CONST1 )
    {
        assert( pObj->nFans == 0 );
        sprintf( pBuff, "%s", "Const1" );
        return pBuff + strlen("Const1");
    }

    if ( pObj->Type == KIT_DSD_VAR )
        assert( pObj->nFans == 1 );

    if ( pObj->Type == KIT_DSD_AND )
        Symbol = '*';
    else if ( pObj->Type == KIT_DSD_XOR )
        Symbol = '+';
    else 
        Symbol = ',';

    if ( pObj->Type == KIT_DSD_PRIME )
        pBuff = Kit_DsdWriteHex( pBuff, Kit_DsdObjTruth(pObj), pObj->nFans );

    *pBuff++ = '(';
    Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )
    {
        if ( Abc_LitIsCompl(iLit) ) 
            *pBuff++ = '!';
        pBuff = Kit_DsdWrite_rec( pBuff, pNtk, Abc_Lit2Var(iLit) );
        if ( i < pObj->nFans - 1 )
            *pBuff++ = Symbol;
    }
    *pBuff++ = ')';
    return pBuff;
}

/**Function*************************************************************

  Synopsis    [Print the DSD formula.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_DsdWrite( char * pBuff, Kit_DsdNtk_t * pNtk )
{
    if ( Abc_LitIsCompl(pNtk->Root) )
        *pBuff++ = '!';
    pBuff = Kit_DsdWrite_rec( pBuff, pNtk, Abc_Lit2Var(pNtk->Root) );
    *pBuff = 0;
}

/**Function*************************************************************

  Synopsis    [Print the DSD formula.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_DsdPrintExpanded( Kit_DsdNtk_t * pNtk )
{
    Kit_DsdNtk_t * pTemp;
    pTemp = Kit_DsdExpand( pNtk );
    Kit_DsdPrint( stdout, pTemp );
    Kit_DsdNtkFree( pTemp );
}

/**Function*************************************************************

  Synopsis    [Print the DSD formula.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_DsdPrintFromTruth( unsigned * pTruth, int nVars )
{
    Kit_DsdNtk_t * pTemp, * pTemp2;
//    pTemp = Kit_DsdDecomposeMux( pTruth, nVars, 5 );
    pTemp = Kit_DsdDecomposeMux( pTruth, nVars, 8 );
//    Kit_DsdPrintExpanded( pTemp );
    pTemp2 = Kit_DsdExpand( pTemp );
    Kit_DsdPrint( stdout, pTemp2 );
    Kit_DsdVerify( pTemp2, pTruth, nVars ); 
    Kit_DsdNtkFree( pTemp2 );
    Kit_DsdNtkFree( pTemp );
}

/**Function*************************************************************

  Synopsis    [Print the DSD formula.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_DsdPrintFromTruth2( FILE * pFile, unsigned * pTruth, int nVars )
{
    Kit_DsdNtk_t * pTemp, * pTemp2;
    pTemp = Kit_DsdDecomposeMux( pTruth, nVars, 0 );
    pTemp2 = Kit_DsdExpand( pTemp );
    Kit_DsdPrint2( pFile, pTemp2 );
    Kit_DsdVerify( pTemp2, pTruth, nVars ); 
    Kit_DsdNtkFree( pTemp2 );
    Kit_DsdNtkFree( pTemp );
}

/**Function*************************************************************

  Synopsis    [Print the DSD formula.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_DsdWriteFromTruth( char * pBuffer, unsigned * pTruth, int nVars )
{
    Kit_DsdNtk_t * pTemp, * pTemp2;
//    pTemp = Kit_DsdDecomposeMux( pTruth, nVars, 5 );
    pTemp = Kit_DsdDecomposeMux( pTruth, nVars, 8 );
//    Kit_DsdPrintExpanded( pTemp );
    pTemp2 = Kit_DsdExpand( pTemp );
    Kit_DsdWrite( pBuffer, pTemp2 );
    Kit_DsdVerify( pTemp2, pTruth, nVars ); 
    Kit_DsdNtkFree( pTemp2 );
    Kit_DsdNtkFree( pTemp );
}

/**Function*************************************************************

  Synopsis    [Derives the truth table of the DSD node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Kit_DsdTruthComputeNode_rec( Kit_DsdMan_t * p, Kit_DsdNtk_t * pNtk, int Id )
{
    Kit_DsdObj_t * pObj;
    unsigned * pTruthRes, * pTruthFans[16], * pTruthTemp;
    unsigned i, iLit, fCompl;
//    unsigned m, nMints, * pTruthPrime, * pTruthMint; 

    // get the node with this ID
    pObj = Kit_DsdNtkObj( pNtk, Id );
    pTruthRes = (unsigned *)Vec_PtrEntry( p->vTtNodes, Id );

    // special case: literal of an internal node
    if ( pObj == NULL )
    {
        assert( Id < pNtk->nVars );
        return pTruthRes;
    }

    // constant node
    if ( pObj->Type == KIT_DSD_CONST1 )
    {
        assert( pObj->nFans == 0 );
        Kit_TruthFill( pTruthRes, pNtk->nVars );
        return pTruthRes;
    }

    // elementary variable node
    if ( pObj->Type == KIT_DSD_VAR )
    {
        assert( pObj->nFans == 1 );
        iLit = pObj->pFans[0];
        pTruthFans[0] = Kit_DsdTruthComputeNode_rec( p, pNtk, Abc_Lit2Var(iLit) );
        if ( Abc_LitIsCompl(iLit) )
            Kit_TruthNot( pTruthRes, pTruthFans[0], pNtk->nVars );
        else
            Kit_TruthCopy( pTruthRes, pTruthFans[0], pNtk->nVars );
        return pTruthRes;
    }

    // collect the truth tables of the fanins
    Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )
        pTruthFans[i] = Kit_DsdTruthComputeNode_rec( p, pNtk, Abc_Lit2Var(iLit) );
    // create the truth table

    // simple gates
    if ( pObj->Type == KIT_DSD_AND )
    {
        Kit_TruthFill( pTruthRes, pNtk->nVars );
        Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )
            Kit_TruthAndPhase( pTruthRes, pTruthRes, pTruthFans[i], pNtk->nVars, 0, Abc_LitIsCompl(iLit) );
        return pTruthRes;
    }
    if ( pObj->Type == KIT_DSD_XOR )
    {
        Kit_TruthClear( pTruthRes, pNtk->nVars );
        fCompl = 0;
        Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )
        {
            Kit_TruthXor( pTruthRes, pTruthRes, pTruthFans[i], pNtk->nVars );
            fCompl ^= Abc_LitIsCompl(iLit);
        }
        if ( fCompl )
            Kit_TruthNot( pTruthRes, pTruthRes, pNtk->nVars );
        return pTruthRes;
    }
    assert( pObj->Type == KIT_DSD_PRIME );
/*
    // get the truth table of the prime node
    pTruthPrime = Kit_DsdObjTruth( pObj );
    // get storage for the temporary minterm
    pTruthMint = Vec_PtrEntry(p->vTtNodes, pNtk->nVars + pNtk->nNodes);
    // go through the minterms
    nMints = (1 << pObj->nFans);
    Kit_TruthClear( pTruthRes, pNtk->nVars );
    for ( m = 0; m < nMints; m++ )
    {
        if ( !Kit_TruthHasBit(pTruthPrime, m) )
            continue;
        Kit_TruthFill( pTruthMint, pNtk->nVars );
        Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )
            Kit_TruthAndPhase( pTruthMint, pTruthMint, pTruthFans[i], pNtk->nVars, 0, ((m & (1<<i)) == 0) ^ Abc_LitIsCompl(iLit) );
        Kit_TruthOr( pTruthRes, pTruthRes, pTruthMint, pNtk->nVars );
    }
*/
    Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )
        if ( Abc_LitIsCompl(iLit) )
            Kit_TruthNot( pTruthFans[i], pTruthFans[i], pNtk->nVars );
    pTruthTemp = Kit_TruthCompose( p->dd, Kit_DsdObjTruth(pObj), pObj->nFans, pTruthFans, pNtk->nVars, p->vTtBdds, p->vNodes );
    Kit_TruthCopy( pTruthRes, pTruthTemp, pNtk->nVars );
    return pTruthRes;
}

/**Function*************************************************************

  Synopsis    [Derives the truth table of the DSD network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Kit_DsdTruthCompute( Kit_DsdMan_t * p, Kit_DsdNtk_t * pNtk )
{
    unsigned * pTruthRes;
    int i;
    // assign elementary truth ables
    assert( pNtk->nVars <= p->nVars );
    for ( i = 0; i < (int)pNtk->nVars; i++ )
        Kit_TruthCopy( (unsigned *)Vec_PtrEntry(p->vTtNodes, i), (unsigned *)Vec_PtrEntry(p->vTtElems, i), p->nVars );
    // compute truth table for each node
    pTruthRes = Kit_DsdTruthComputeNode_rec( p, pNtk, Abc_Lit2Var(pNtk->Root) );
    // complement the truth table if needed
    if ( Abc_LitIsCompl(pNtk->Root) )
        Kit_TruthNot( pTruthRes, pTruthRes, pNtk->nVars );
    return pTruthRes;
}

/**Function*************************************************************

  Synopsis    [Derives the truth table of the DSD node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Kit_DsdTruthComputeNodeOne_rec( Kit_DsdMan_t * p, Kit_DsdNtk_t * pNtk, int Id, unsigned uSupp )
{
    Kit_DsdObj_t * pObj;
    unsigned * pTruthRes, * pTruthFans[16], * pTruthTemp;
    unsigned i, iLit, fCompl, nPartial = 0;
//    unsigned m, nMints, * pTruthPrime, * pTruthMint; 

    // get the node with this ID
    pObj = Kit_DsdNtkObj( pNtk, Id );
    pTruthRes = (unsigned *)Vec_PtrEntry( p->vTtNodes, Id );

    // special case: literal of an internal node
    if ( pObj == NULL )
    {
        assert( Id < pNtk->nVars );
        assert( !uSupp || uSupp != (uSupp & ~(1<<Id)) );
        return pTruthRes;
    }

    // constant node
    if ( pObj->Type == KIT_DSD_CONST1 )
    {
        assert( pObj->nFans == 0 );
        Kit_TruthFill( pTruthRes, pNtk->nVars );
        return pTruthRes;
    }

    // elementary variable node
    if ( pObj->Type == KIT_DSD_VAR )
    {
        assert( pObj->nFans == 1 );
        iLit = pObj->pFans[0];
        assert( Kit_DsdLitIsLeaf( pNtk, iLit ) );
        pTruthFans[0] = Kit_DsdTruthComputeNodeOne_rec( p, pNtk, Abc_Lit2Var(iLit), uSupp );
        if ( Abc_LitIsCompl(iLit) )
            Kit_TruthNot( pTruthRes, pTruthFans[0], pNtk->nVars );
        else
            Kit_TruthCopy( pTruthRes, pTruthFans[0], pNtk->nVars );
        return pTruthRes;
    }

    // collect the truth tables of the fanins
    if ( uSupp )
    {
        Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )
            if ( uSupp != (uSupp & ~Kit_DsdLitSupport(pNtk, iLit)) )
                pTruthFans[i] = Kit_DsdTruthComputeNodeOne_rec( p, pNtk, Abc_Lit2Var(iLit), uSupp );
            else
            {
                pTruthFans[i] = NULL;
                nPartial = 1;
            }
    }
    else
    {
        Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )
            pTruthFans[i] = Kit_DsdTruthComputeNodeOne_rec( p, pNtk, Abc_Lit2Var(iLit), uSupp );
    }
    // create the truth table

    // simple gates
    if ( pObj->Type == KIT_DSD_AND )
    {
        Kit_TruthFill( pTruthRes, pNtk->nVars );
        Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )
            if ( pTruthFans[i] )
                Kit_TruthAndPhase( pTruthRes, pTruthRes, pTruthFans[i], pNtk->nVars, 0, Abc_LitIsCompl(iLit) );
        return pTruthRes;
    }
    if ( pObj->Type == KIT_DSD_XOR )
    {
        Kit_TruthClear( pTruthRes, pNtk->nVars );
        fCompl = 0;
        Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )
        {
            if ( pTruthFans[i] )
            {
                Kit_TruthXor( pTruthRes, pTruthRes, pTruthFans[i], pNtk->nVars );
                fCompl ^= Abc_LitIsCompl(iLit);
            }
        }
        if ( fCompl )
            Kit_TruthNot( pTruthRes, pTruthRes, pNtk->nVars );
        return pTruthRes;
    }
    assert( pObj->Type == KIT_DSD_PRIME );

    if ( uSupp && nPartial )
    {
        // find the only non-empty component
        Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )
            if ( pTruthFans[i] )
                break;
        assert( i < pObj->nFans );
        return pTruthFans[i];
    }
/*
    // get the truth table of the prime node
    pTruthPrime = Kit_DsdObjTruth( pObj );
    // get storage for the temporary minterm
    pTruthMint = Vec_PtrEntry(p->vTtNodes, pNtk->nVars + pNtk->nNodes);
    // go through the minterms
    nMints = (1 << pObj->nFans);
    Kit_TruthClear( pTruthRes, pNtk->nVars );
    for ( m = 0; m < nMints; m++ )
    {
        if ( !Kit_TruthHasBit(pTruthPrime, m) )
            continue;
        Kit_TruthFill( pTruthMint, pNtk->nVars );
        Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )
            Kit_TruthAndPhase( pTruthMint, pTruthMint, pTruthFans[i], pNtk->nVars, 0, ((m & (1<<i)) == 0) ^ Abc_LitIsCompl(iLit) );
        Kit_TruthOr( pTruthRes, pTruthRes, pTruthMint, pNtk->nVars );
    }
*/
    Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )
        if ( Abc_LitIsCompl(iLit) )
            Kit_TruthNot( pTruthFans[i], pTruthFans[i], pNtk->nVars );
    pTruthTemp = Kit_TruthCompose( p->dd, Kit_DsdObjTruth(pObj), pObj->nFans, pTruthFans, pNtk->nVars, p->vTtBdds, p->vNodes );
    Kit_TruthCopy( pTruthRes, pTruthTemp, pNtk->nVars );
    return pTruthRes;
}

/**Function*************************************************************

  Synopsis    [Derives the truth table of the DSD network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Kit_DsdTruthComputeOne( Kit_DsdMan_t * p, Kit_DsdNtk_t * pNtk, unsigned uSupp )
{
    unsigned * pTruthRes;
    int i;
    // if support is specified, request that supports are available
    if ( uSupp )
        Kit_DsdGetSupports( pNtk );
    // assign elementary truth tables
    assert( pNtk->nVars <= p->nVars );
    for ( i = 0; i < (int)pNtk->nVars; i++ )
        Kit_TruthCopy( (unsigned *)Vec_PtrEntry(p->vTtNodes, i), (unsigned *)Vec_PtrEntry(p->vTtElems, i), p->nVars );
    // compute truth table for each node
    pTruthRes = Kit_DsdTruthComputeNodeOne_rec( p, pNtk, Abc_Lit2Var(pNtk->Root), uSupp );
    // complement the truth table if needed
    if ( Abc_LitIsCompl(pNtk->Root) )
        Kit_TruthNot( pTruthRes, pTruthRes, pNtk->nVars );
    return pTruthRes;
}

/**Function*************************************************************

  Synopsis    [Derives the truth table of the DSD node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Kit_DsdTruthComputeNodeTwo_rec( Kit_DsdMan_t * p, Kit_DsdNtk_t * pNtk, int Id, unsigned uSupp, int iVar, unsigned * pTruthDec )
{
    Kit_DsdObj_t * pObj;
    int pfBoundSet[16];
    unsigned * pTruthRes, * pTruthFans[16], * pTruthTemp;
    unsigned i, iLit, fCompl, nPartial, uSuppFan, uSuppCur;
//    unsigned m, nMints, * pTruthPrime, * pTruthMint; 
    assert( uSupp > 0 );

    // get the node with this ID
    pObj = Kit_DsdNtkObj( pNtk, Id );
    pTruthRes = (unsigned *)Vec_PtrEntry( p->vTtNodes, Id );
    if ( pObj == NULL )
    {
        assert( Id < pNtk->nVars );
        return pTruthRes;
    }
    assert( pObj->Type != KIT_DSD_CONST1 );
    assert( pObj->Type != KIT_DSD_VAR );

    // count the number of intersecting fanins 
    // collect the total support of the intersecting fanins
    nPartial = 0;
    uSuppFan = 0;
    Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )
    {
        uSuppCur = Kit_DsdLitSupport(pNtk, iLit);
        if ( uSupp & uSuppCur )
        {
            nPartial++;
            uSuppFan |= uSuppCur;
        }
    }

    // if there is no intersection, or full intersection, use simple procedure
    if ( nPartial == 0 || nPartial == pObj->nFans )
        return Kit_DsdTruthComputeNodeOne_rec( p, pNtk, Id, 0 );

    // if support of the component includes some other variables
    // we need to continue constructing it as usual by the two-function procedure
    if ( uSuppFan != (uSuppFan & uSupp) )
    {
        assert( nPartial == 1 );
//        return Kit_DsdTruthComputeNodeTwo_rec( p, pNtk, Id, uSupp, iVar, pTruthDec );
        Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )
        {
            if ( uSupp & Kit_DsdLitSupport(pNtk, iLit) )
                pTruthFans[i] = Kit_DsdTruthComputeNodeTwo_rec( p, pNtk, Abc_Lit2Var(iLit), uSupp, iVar, pTruthDec );
            else
                pTruthFans[i] = Kit_DsdTruthComputeNodeOne_rec( p, pNtk, Abc_Lit2Var(iLit), 0 );
        }

        // create composition/decomposition functions
        if ( pObj->Type == KIT_DSD_AND )
        {
            Kit_TruthFill( pTruthRes, pNtk->nVars );
            Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )
                Kit_TruthAndPhase( pTruthRes, pTruthRes, pTruthFans[i], pNtk->nVars, 0, Abc_LitIsCompl(iLit) );
            return pTruthRes;
        }
        if ( pObj->Type == KIT_DSD_XOR )
        {
            Kit_TruthClear( pTruthRes, pNtk->nVars );
            fCompl = 0;
            Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )
            {
                fCompl ^= Abc_LitIsCompl(iLit);
                Kit_TruthXor( pTruthRes, pTruthRes, pTruthFans[i], pNtk->nVars );
            }
            if ( fCompl )
                Kit_TruthNot( pTruthRes, pTruthRes, pNtk->nVars );
            return pTruthRes;
        }
        assert( pObj->Type == KIT_DSD_PRIME );
    }
    else
    {
        assert( uSuppFan == (uSuppFan & uSupp) );
        assert( nPartial < pObj->nFans );
        // the support of the insecting component(s) is contained in the bound-set
        // and yet there are components that are not contained in the bound set

        // solve the fanins and collect info, which components belong to the bound set
        Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )
        {
            pTruthFans[i] = Kit_DsdTruthComputeNodeOne_rec( p, pNtk, Abc_Lit2Var(iLit), 0 );
            pfBoundSet[i] = (int)((uSupp & Kit_DsdLitSupport(pNtk, iLit)) > 0);
        }

        // create composition/decomposition functions
        if ( pObj->Type == KIT_DSD_AND )
        {
            Kit_TruthIthVar( pTruthRes, pNtk->nVars, iVar );
            Kit_TruthFill( pTruthDec, pNtk->nVars );
            Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )
                if ( pfBoundSet[i] )
                    Kit_TruthAndPhase( pTruthDec, pTruthDec, pTruthFans[i], pNtk->nVars, 0, Abc_LitIsCompl(iLit) );
                else
                    Kit_TruthAndPhase( pTruthRes, pTruthRes, pTruthFans[i], pNtk->nVars, 0, Abc_LitIsCompl(iLit) );
            return pTruthRes;
        }
        if ( pObj->Type == KIT_DSD_XOR )
        {
            Kit_TruthIthVar( pTruthRes, pNtk->nVars, iVar );
            Kit_TruthClear( pTruthDec, pNtk->nVars );
            fCompl = 0;
            Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )
            {
                fCompl ^= Abc_LitIsCompl(iLit);
                if ( pfBoundSet[i] )
                    Kit_TruthXor( pTruthDec, pTruthDec, pTruthFans[i], pNtk->nVars );
                else
                    Kit_TruthXor( pTruthRes, pTruthRes, pTruthFans[i], pNtk->nVars );
            }
            if ( fCompl )
                Kit_TruthNot( pTruthRes, pTruthRes, pNtk->nVars );
            return pTruthRes;
        }
        assert( pObj->Type == KIT_DSD_PRIME );
        assert( nPartial == 1 );

        // find the only non-empty component
        Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )
            if ( pfBoundSet[i] )
                break;
        assert( i < pObj->nFans );

        // save this component as the decomposed function
        Kit_TruthCopy( pTruthDec, pTruthFans[i], pNtk->nVars );
        // set the corresponding component to be the new variable
        Kit_TruthIthVar( pTruthFans[i], pNtk->nVars, iVar );
    }
/*
    // get the truth table of the prime node
    pTruthPrime = Kit_DsdObjTruth( pObj );
    // get storage for the temporary minterm
    pTruthMint = Vec_PtrEntry(p->vTtNodes, pNtk->nVars + pNtk->nNodes);
    // go through the minterms
    nMints = (1 << pObj->nFans);
    Kit_TruthClear( pTruthRes, pNtk->nVars );
    for ( m = 0; m < nMints; m++ )
    {
        if ( !Kit_TruthHasBit(pTruthPrime, m) )
            continue;
        Kit_TruthFill( pTruthMint, pNtk->nVars );
        Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )
            Kit_TruthAndPhase( pTruthMint, pTruthMint, pTruthFans[i], pNtk->nVars, 0, ((m & (1<<i)) == 0) ^ Abc_LitIsCompl(iLit) );
        Kit_TruthOr( pTruthRes, pTruthRes, pTruthMint, pNtk->nVars );
    }
*/
//    Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )
//        assert( !Abc_LitIsCompl(iLit) );
    Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )
        if ( Abc_LitIsCompl(iLit) )
            Kit_TruthNot( pTruthFans[i], pTruthFans[i], pNtk->nVars );
    pTruthTemp = Kit_TruthCompose( p->dd, Kit_DsdObjTruth(pObj), pObj->nFans, pTruthFans, pNtk->nVars, p->vTtBdds, p->vNodes );
    Kit_TruthCopy( pTruthRes, pTruthTemp, pNtk->nVars );
    return pTruthRes;
}

/**Function*************************************************************

  Synopsis    [Derives the truth table of the DSD network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Kit_DsdTruthComputeTwo( Kit_DsdMan_t * p, Kit_DsdNtk_t * pNtk, unsigned uSupp, int iVar, unsigned * pTruthDec )
{
    unsigned * pTruthRes, uSuppAll;
    int i;
    assert( uSupp > 0 );
    assert( pNtk->nVars <= p->nVars );
    // compute support of all nodes
    uSuppAll = Kit_DsdGetSupports( pNtk );
    // consider special case - there is no overlap
    if ( (uSupp & uSuppAll) == 0 )
    {
        Kit_TruthClear( pTruthDec, pNtk->nVars );
        return Kit_DsdTruthCompute( p, pNtk );
    }
    // consider special case - support is fully contained
    if ( (uSupp & uSuppAll) == uSuppAll )
    {
        pTruthRes = Kit_DsdTruthCompute( p, pNtk );
        Kit_TruthCopy( pTruthDec, pTruthRes, pNtk->nVars );
        Kit_TruthIthVar( pTruthRes, pNtk->nVars, iVar );
        return pTruthRes;
    }
    // assign elementary truth tables
    for ( i = 0; i < (int)pNtk->nVars; i++ )
        Kit_TruthCopy( (unsigned *)Vec_PtrEntry(p->vTtNodes, i), (unsigned *)Vec_PtrEntry(p->vTtElems, i), p->nVars );
    // compute truth table for each node
    pTruthRes = Kit_DsdTruthComputeNodeTwo_rec( p, pNtk, Abc_Lit2Var(pNtk->Root), uSupp, iVar, pTruthDec );
    // complement the truth table if needed
    if ( Abc_LitIsCompl(pNtk->Root) )
        Kit_TruthNot( pTruthRes, pTruthRes, pNtk->nVars );
    return pTruthRes;
}

/**Function*************************************************************

  Synopsis    [Derives the truth table of the DSD network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_DsdTruth( Kit_DsdNtk_t * pNtk, unsigned * pTruthRes )
{
    Kit_DsdMan_t * p;
    unsigned * pTruth;
    p = Kit_DsdManAlloc( pNtk->nVars, Kit_DsdNtkObjNum(pNtk) );
    pTruth = Kit_DsdTruthCompute( p, pNtk );
    Kit_TruthCopy( pTruthRes, pTruth, pNtk->nVars );
    Kit_DsdManFree( p );
}

/**Function*************************************************************

  Synopsis    [Derives the truth table of the DSD network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_DsdTruthPartialTwo( Kit_DsdMan_t * p, Kit_DsdNtk_t * pNtk, unsigned uSupp, int iVar, unsigned * pTruthCo, unsigned * pTruthDec )
{
    unsigned * pTruth = Kit_DsdTruthComputeTwo( p, pNtk, uSupp, iVar, pTruthDec );
    if ( pTruthCo )
        Kit_TruthCopy( pTruthCo, pTruth, pNtk->nVars );
}

/**Function*************************************************************

  Synopsis    [Derives the truth table of the DSD network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_DsdTruthPartial( Kit_DsdMan_t * p, Kit_DsdNtk_t * pNtk, unsigned * pTruthRes, unsigned uSupp )
{
    unsigned * pTruth = Kit_DsdTruthComputeOne( p, pNtk, uSupp );
    Kit_TruthCopy( pTruthRes, pTruth, pNtk->nVars );
/*
    // verification
    {
        // compute the same function using different procedure
        unsigned * pTruthTemp = Vec_PtrEntry(p->vTtNodes, pNtk->nVars + pNtk->nNodes + 1);
        pNtk->pSupps = NULL;
        Kit_DsdTruthComputeTwo( p, pNtk, uSupp, -1, pTruthTemp );
//        if ( !Kit_TruthIsEqual( pTruthTemp, pTruthRes, pNtk->nVars ) )
        if ( !Kit_TruthIsEqualWithPhase( pTruthTemp, pTruthRes, pNtk->nVars ) )
        {
            printf( "Verification FAILED!\n" );
            Kit_DsdPrint( stdout, pNtk );
            Kit_DsdPrintFromTruth( pTruthRes, pNtk->nVars );
            Kit_DsdPrintFromTruth( pTruthTemp, pNtk->nVars );
        }
//        else
//            printf( "Verification successful.\n" );
    }
*/
}

/**Function*************************************************************

  Synopsis    [Counts the number of blocks of the given number of inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Kit_DsdCountLuts_rec( Kit_DsdNtk_t * pNtk, int nLutSize, int Id, int * pCounter )
{
    Kit_DsdObj_t * pObj;
    unsigned iLit, i, Res0, Res1;
    pObj = Kit_DsdNtkObj( pNtk, Id );
    if ( pObj == NULL )
        return 0;
    if ( pObj->Type == KIT_DSD_AND || pObj->Type == KIT_DSD_XOR )
    {
        assert( pObj->nFans == 2 );
        Res0 = Kit_DsdCountLuts_rec( pNtk, nLutSize, Abc_Lit2Var(pObj->pFans[0]), pCounter );
        Res1 = Kit_DsdCountLuts_rec( pNtk, nLutSize, Abc_Lit2Var(pObj->pFans[1]), pCounter );
        if ( Res0 == 0 && Res1 > 0 )
            return Res1 - 1;
        if ( Res0 > 0 && Res1 == 0 )
            return Res0 - 1;
        (*pCounter)++;
        return nLutSize - 2;
    }
    assert( pObj->Type == KIT_DSD_PRIME );
    if ( (int)pObj->nFans > nLutSize ) //+ 1 )
    {
        *pCounter = 1000;
        return 0;
    }
    Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )
        Kit_DsdCountLuts_rec( pNtk, nLutSize, Abc_Lit2Var(iLit), pCounter );
    (*pCounter)++;
//    if ( (int)pObj->nFans == nLutSize + 1 )
//        (*pCounter)++;
    return nLutSize - pObj->nFans;
}

/**Function*************************************************************

  Synopsis    [Counts the number of blocks of the given number of inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Kit_DsdCountLuts( Kit_DsdNtk_t * pNtk, int nLutSize )
{
    int Counter = 0;
    if ( Kit_DsdNtkRoot(pNtk)->Type == KIT_DSD_CONST1 )
        return 0;
    if ( Kit_DsdNtkRoot(pNtk)->Type == KIT_DSD_VAR )
        return 0;
    Kit_DsdCountLuts_rec( pNtk, nLutSize, Abc_Lit2Var(pNtk->Root), &Counter );
    if ( Counter >= 1000 )
        return -1;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Returns the size of the largest non-DSD block.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Kit_DsdNonDsdSizeMax( Kit_DsdNtk_t * pNtk )
{
    Kit_DsdObj_t * pObj;
    unsigned i, nSizeMax = 0;
    Kit_DsdNtkForEachObj( pNtk, pObj, i )
    {
        if ( pObj->Type != KIT_DSD_PRIME )
            continue;
        if ( nSizeMax < pObj->nFans )
            nSizeMax = pObj->nFans;
    }
    return nSizeMax;
}

/**Function*************************************************************

  Synopsis    [Returns the largest non-DSD block.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Kit_DsdObj_t * Kit_DsdNonDsdPrimeMax( Kit_DsdNtk_t * pNtk )
{
    Kit_DsdObj_t * pObj, * pObjMax = NULL;
    unsigned i, nSizeMax = 0;
    Kit_DsdNtkForEachObj( pNtk, pObj, i )
    {
        if ( pObj->Type != KIT_DSD_PRIME )
            continue;
        if ( nSizeMax < pObj->nFans )
        {
            nSizeMax = pObj->nFans;
            pObjMax = pObj;
        }
    }
    return pObjMax;
}

/**Function*************************************************************

  Synopsis    [Finds the union of supports of the non-DSD blocks.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Kit_DsdNonDsdSupports( Kit_DsdNtk_t * pNtk )
{
    Kit_DsdObj_t * pObj;
    unsigned i, uSupport = 0;
//    ABC_FREE( pNtk->pSupps );
    Kit_DsdGetSupports( pNtk );
    Kit_DsdNtkForEachObj( pNtk, pObj, i )
    {
        if ( pObj->Type != KIT_DSD_PRIME )
            continue;
        uSupport |= Kit_DsdLitSupport( pNtk, Abc_Var2Lit(pObj->Id,0) );
    }
    return uSupport;
}


/**Function*************************************************************

  Synopsis    [Expands the node.]

  Description [Returns the new literal.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_DsdExpandCollectAnd_rec( Kit_DsdNtk_t * p, unsigned iLit, unsigned * piLitsNew, int * nLitsNew )
{
    Kit_DsdObj_t * pObj;
    unsigned i, iLitFanin;
    // check the end of the supergate
    pObj = Kit_DsdNtkObj( p, Abc_Lit2Var(iLit) );
    if ( Abc_LitIsCompl(iLit) || Abc_Lit2Var(iLit) < p->nVars || pObj->Type != KIT_DSD_AND )
    {
        piLitsNew[(*nLitsNew)++] = iLit;
        return;
    }
    // iterate through the fanins
    Kit_DsdObjForEachFanin( p, pObj, iLitFanin, i )
        Kit_DsdExpandCollectAnd_rec( p, iLitFanin, piLitsNew, nLitsNew );
}

/**Function*************************************************************

  Synopsis    [Expands the node.]

  Description [Returns the new literal.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_DsdExpandCollectXor_rec( Kit_DsdNtk_t * p, unsigned iLit, unsigned * piLitsNew, int * nLitsNew )
{
    Kit_DsdObj_t * pObj;
    unsigned i, iLitFanin;
    // check the end of the supergate
    pObj = Kit_DsdNtkObj( p, Abc_Lit2Var(iLit) );
    if ( Abc_Lit2Var(iLit) < p->nVars || pObj->Type != KIT_DSD_XOR )
    {
        piLitsNew[(*nLitsNew)++] = iLit;
        return;
    }
    // iterate through the fanins
    pObj = Kit_DsdNtkObj( p, Abc_Lit2Var(iLit) );
    Kit_DsdObjForEachFanin( p, pObj, iLitFanin, i )
        Kit_DsdExpandCollectXor_rec( p, iLitFanin, piLitsNew, nLitsNew );
    // if the literal was complemented, pass the complemented attribute somewhere
    if ( Abc_LitIsCompl(iLit) )
        piLitsNew[0] = Abc_LitNot( piLitsNew[0] );
}

/**Function*************************************************************

  Synopsis    [Expands the node.]

  Description [Returns the new literal.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Kit_DsdExpandNode_rec( Kit_DsdNtk_t * pNew, Kit_DsdNtk_t * p, int iLit )
{
    unsigned * pTruth, * pTruthNew;
    unsigned i, iLitFanin, piLitsNew[16], nLitsNew = 0;
    Kit_DsdObj_t * pObj, * pObjNew;

    // consider the case of simple gate
    pObj = Kit_DsdNtkObj( p, Abc_Lit2Var(iLit) );
    if ( pObj == NULL )
        return iLit;
    if ( pObj->Type == KIT_DSD_AND )
    {
        Kit_DsdExpandCollectAnd_rec( p, Abc_LitRegular(iLit), piLitsNew, (int *)&nLitsNew );
        pObjNew = Kit_DsdObjAlloc( pNew, KIT_DSD_AND, nLitsNew );
        for ( i = 0; i < pObjNew->nFans; i++ )
            pObjNew->pFans[i] = Kit_DsdExpandNode_rec( pNew, p, piLitsNew[i] );
        return Abc_Var2Lit( pObjNew->Id, Abc_LitIsCompl(iLit) );
    }
    if ( pObj->Type == KIT_DSD_XOR )
    {
        int fCompl = Abc_LitIsCompl(iLit);
        Kit_DsdExpandCollectXor_rec( p, Abc_LitRegular(iLit), piLitsNew, (int *)&nLitsNew );
        pObjNew = Kit_DsdObjAlloc( pNew, KIT_DSD_XOR, nLitsNew );
        for ( i = 0; i < pObjNew->nFans; i++ )
        {
            pObjNew->pFans[i] = Kit_DsdExpandNode_rec( pNew, p, Abc_LitRegular(piLitsNew[i]) );
            fCompl ^= Abc_LitIsCompl(piLitsNew[i]);
        }
        return Abc_Var2Lit( pObjNew->Id, fCompl );
    }
    assert( pObj->Type == KIT_DSD_PRIME );

    // create new PRIME node
    pObjNew = Kit_DsdObjAlloc( pNew, KIT_DSD_PRIME, pObj->nFans );
    // copy the truth table
    pTruth = Kit_DsdObjTruth( pObj );
    pTruthNew = Kit_DsdObjTruth( pObjNew );
    Kit_TruthCopy( pTruthNew, pTruth, pObj->nFans );
    // create fanins
    Kit_DsdObjForEachFanin( pNtk, pObj, iLitFanin, i )
    {
        pObjNew->pFans[i] = Kit_DsdExpandNode_rec( pNew, p, iLitFanin );
        // complement the corresponding inputs of the truth table
        if ( Abc_LitIsCompl(pObjNew->pFans[i]) )
        {
            pObjNew->pFans[i] = Abc_LitRegular(pObjNew->pFans[i]);
            Kit_TruthChangePhase( pTruthNew, pObjNew->nFans, i );
        }
    }

    if ( pObj->nFans == 3 && 
        (pTruthNew[0] == 0xCACACACA || pTruthNew[0] == 0xC5C5C5C5 || 
         pTruthNew[0] == 0x3A3A3A3A || pTruthNew[0] == 0x35353535) )
    {
        // translate into regular MUXes
        if ( pTruthNew[0] == 0xC5C5C5C5 )
            pObjNew->pFans[0] = Abc_LitNot(pObjNew->pFans[0]);
        else if ( pTruthNew[0] == 0x3A3A3A3A )
            pObjNew->pFans[1] = Abc_LitNot(pObjNew->pFans[1]);
        else if ( pTruthNew[0] == 0x35353535 )
        {
            pObjNew->pFans[0] = Abc_LitNot(pObjNew->pFans[0]);
            pObjNew->pFans[1] = Abc_LitNot(pObjNew->pFans[1]);
        }
        pTruthNew[0] = 0xCACACACA;
        // resolve the complemented control input
        if ( Abc_LitIsCompl(pObjNew->pFans[2]) )
        {
            unsigned char Temp = pObjNew->pFans[0];
            pObjNew->pFans[0] = pObjNew->pFans[1];
            pObjNew->pFans[1] = Temp;
            pObjNew->pFans[2] = Abc_LitNot(pObjNew->pFans[2]);
        }
        // resolve the complemented true input
        if ( Abc_LitIsCompl(pObjNew->pFans[1]) )
        {
            iLit = Abc_LitNot(iLit);
            pObjNew->pFans[0] = Abc_LitNot(pObjNew->pFans[0]);
            pObjNew->pFans[1] = Abc_LitNot(pObjNew->pFans[1]);
        }
        return Abc_Var2Lit( pObjNew->Id, Abc_LitIsCompl(iLit) );
    }
    else
    {
        // if the incoming phase is complemented, absorb it into the prime node
        if ( Abc_LitIsCompl(iLit) )
            Kit_TruthNot( pTruthNew, pTruthNew, pObj->nFans );
        return Abc_Var2Lit( pObjNew->Id, 0 );
    }
}

/**Function*************************************************************

  Synopsis    [Expands the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Kit_DsdNtk_t * Kit_DsdExpand( Kit_DsdNtk_t * p )
{
    Kit_DsdNtk_t * pNew;
    Kit_DsdObj_t * pObjNew;
    assert( p->nVars <= 16 );
    // create a new network
    pNew = Kit_DsdNtkAlloc( p->nVars );
    // consider simple special cases
    if ( Kit_DsdNtkRoot(p)->Type == KIT_DSD_CONST1 )
    {
        pObjNew = Kit_DsdObjAlloc( pNew, KIT_DSD_CONST1, 0 );
        pNew->Root = Abc_Var2Lit( pObjNew->Id, Abc_LitIsCompl(p->Root) );
        return pNew;
    }
    if ( Kit_DsdNtkRoot(p)->Type == KIT_DSD_VAR )
    {
        pObjNew = Kit_DsdObjAlloc( pNew, KIT_DSD_VAR, 1 );
        pObjNew->pFans[0] = Kit_DsdNtkRoot(p)->pFans[0];
        pNew->Root = Abc_Var2Lit( pObjNew->Id, Abc_LitIsCompl(p->Root) );
        return pNew;
    }
    // convert the root node
    pNew->Root = Kit_DsdExpandNode_rec( pNew, p, p->Root );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Sorts the literals by their support.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_DsdCompSort( int pPrios[], unsigned uSupps[], unsigned short * piLits, int nVars, unsigned piLitsRes[] )
{
    int nSuppSizes[16], Priority[16], pOrder[16];
    int i, k, iVarBest, SuppMax, PrioMax;
    // compute support sizes and priorities of the components
    for ( i = 0; i < nVars; i++ )
    {
        assert( uSupps[i] );
        pOrder[i] = i;
        Priority[i] = KIT_INFINITY;
        for ( k = 0; k < 16; k++ )
            if ( uSupps[i] & (1 << k) )
                Priority[i] = KIT_MIN( Priority[i], pPrios[k] );
        assert( Priority[i] != 16 );
        nSuppSizes[i] = Kit_WordCountOnes(uSupps[i]);
    }
    // sort the components by pririty
    Extra_BubbleSort( pOrder, Priority, nVars, 0 );
    // find the component by with largest size and lowest priority
    iVarBest = -1;
    SuppMax  = 0;
    PrioMax  = 0;
    for ( i = 0; i < nVars; i++ )
    {
        if ( SuppMax < nSuppSizes[i] || (SuppMax == nSuppSizes[i] && PrioMax < Priority[i]) )
        {
            SuppMax  = nSuppSizes[i];
            PrioMax  = Priority[i];
            iVarBest = i;
        }
    }
    assert( iVarBest != -1 );
    // copy the resulting literals
    k = 0;
    piLitsRes[k++] = piLits[iVarBest];
    for ( i = 0; i < nVars; i++ )
    {
        if ( pOrder[i] == iVarBest )
            continue;
        piLitsRes[k++] = piLits[pOrder[i]];
    }
    assert( k == nVars );
}

/**Function*************************************************************

  Synopsis    [Shrinks multi-input nodes.]

  Description [Takes the array of variable priorities pPrios.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Kit_DsdShrink_rec( Kit_DsdNtk_t * pNew, Kit_DsdNtk_t * p, int iLit, int pPrios[] )
{
    Kit_DsdObj_t * pObj;
    Kit_DsdObj_t * pObjNew = NULL; // Suppress "might be used uninitialized"
    unsigned * pTruth, * pTruthNew;
    unsigned i, piLitsNew[16], uSupps[16];
    int iLitFanin, iLitNew;

    // consider the case of simple gate
    pObj = Kit_DsdNtkObj( p, Abc_Lit2Var(iLit) );
    if ( pObj == NULL )
        return iLit;
    if ( pObj->Type == KIT_DSD_AND )
    {
        // get the supports
        Kit_DsdObjForEachFanin( p, pObj, iLitFanin, i )
            uSupps[i] = Kit_DsdLitSupport( p, iLitFanin );
        // put the largest component last
        // sort other components in the decreasing order of priority of their vars
        Kit_DsdCompSort( pPrios, uSupps, pObj->pFans, pObj->nFans, piLitsNew );
        // construct the two-input node network
        iLitNew = Kit_DsdShrink_rec( pNew, p, piLitsNew[0], pPrios );
        for ( i = 1; i < pObj->nFans; i++ )
        {
            pObjNew = Kit_DsdObjAlloc( pNew, KIT_DSD_AND, 2 );
            pObjNew->pFans[0] = Kit_DsdShrink_rec( pNew, p, piLitsNew[i], pPrios );
            pObjNew->pFans[1] = iLitNew;
            iLitNew = Abc_Var2Lit( pObjNew->Id, 0 );
        }
        return Abc_Var2Lit( pObjNew->Id, Abc_LitIsCompl(iLit) );
    }
    if ( pObj->Type == KIT_DSD_XOR )
    {
        // get the supports
        Kit_DsdObjForEachFanin( p, pObj, iLitFanin, i )
        {
            assert( !Abc_LitIsCompl(iLitFanin) );
            uSupps[i] = Kit_DsdLitSupport( p, iLitFanin );
        }
        // put the largest component last
        // sort other components in the decreasing order of priority of their vars
        Kit_DsdCompSort( pPrios, uSupps, pObj->pFans, pObj->nFans, piLitsNew );
        // construct the two-input node network
        iLitNew = Kit_DsdShrink_rec( pNew, p, piLitsNew[0], pPrios );
        for ( i = 1; i < pObj->nFans; i++ )
        {
            pObjNew = Kit_DsdObjAlloc( pNew, KIT_DSD_XOR, 2 );
            pObjNew->pFans[0] = Kit_DsdShrink_rec( pNew, p, piLitsNew[i], pPrios );
            pObjNew->pFans[1] = iLitNew;
            iLitNew = Abc_Var2Lit( pObjNew->Id, 0 );
        }
        return Abc_Var2Lit( pObjNew->Id, Abc_LitIsCompl(iLit) );
    }
    assert( pObj->Type == KIT_DSD_PRIME );

    // create new PRIME node
    pObjNew = Kit_DsdObjAlloc( pNew, KIT_DSD_PRIME, pObj->nFans );
    // copy the truth table
    pTruth = Kit_DsdObjTruth( pObj );
    pTruthNew = Kit_DsdObjTruth( pObjNew );
    Kit_TruthCopy( pTruthNew, pTruth, pObj->nFans );
    // create fanins
    Kit_DsdObjForEachFanin( pNtk, pObj, iLitFanin, i )
    {
        pObjNew->pFans[i] = Kit_DsdShrink_rec( pNew, p, iLitFanin, pPrios );
        // complement the corresponding inputs of the truth table
        if ( Abc_LitIsCompl(pObjNew->pFans[i]) )
        {
            pObjNew->pFans[i] = Abc_LitRegular(pObjNew->pFans[i]);
            Kit_TruthChangePhase( pTruthNew, pObjNew->nFans, i );
        }
    }
    // if the incoming phase is complemented, absorb it into the prime node
    if ( Abc_LitIsCompl(iLit) )
        Kit_TruthNot( pTruthNew, pTruthNew, pObj->nFans );
    return Abc_Var2Lit( pObjNew->Id, 0 );
}

/**Function*************************************************************

  Synopsis    [Shrinks the network.]

  Description [Transforms the network to have two-input nodes so that the
  higher-ordered nodes were decomposed out first.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Kit_DsdNtk_t * Kit_DsdShrink( Kit_DsdNtk_t * p, int pPrios[] )
{
    Kit_DsdNtk_t * pNew;
    Kit_DsdObj_t * pObjNew;
    assert( p->nVars <= 16 );
    // create a new network
    pNew = Kit_DsdNtkAlloc( p->nVars );
    // consider simple special cases
    if ( Kit_DsdNtkRoot(p)->Type == KIT_DSD_CONST1 )
    {
        pObjNew = Kit_DsdObjAlloc( pNew, KIT_DSD_CONST1, 0 );
        pNew->Root = Abc_Var2Lit( pObjNew->Id, Abc_LitIsCompl(p->Root) );
        return pNew;
    }
    if ( Kit_DsdNtkRoot(p)->Type == KIT_DSD_VAR )
    {
        pObjNew = Kit_DsdObjAlloc( pNew, KIT_DSD_VAR, 1 );
        pObjNew->pFans[0] = Kit_DsdNtkRoot(p)->pFans[0];
        pNew->Root = Abc_Var2Lit( pObjNew->Id, Abc_LitIsCompl(p->Root) );
        return pNew;
    }
    // convert the root node
    pNew->Root = Kit_DsdShrink_rec( pNew, p, p->Root, pPrios );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Rotates the network.]

  Description [Transforms prime nodes to have the fanin with the
  highest frequency of supports go first.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_DsdRotate( Kit_DsdNtk_t * p, int pFreqs[] )
{
    Kit_DsdObj_t * pObj;
    unsigned * pIn, * pOut, * pTemp, k;
    int i, v, Temp, uSuppFanin, iFaninLit, WeightMax, FaninMax, nSwaps;
    int Weights[16];
    // go through the prime nodes
    Kit_DsdNtkForEachObj( p, pObj, i )
    {
        if ( pObj->Type != KIT_DSD_PRIME )
            continue;
        // count the fanin frequencies
        Kit_DsdObjForEachFanin( p, pObj, iFaninLit, k )
        {
            uSuppFanin = Kit_DsdLitSupport( p, iFaninLit );
            Weights[k] = 0;
            for ( v = 0; v < 16; v++ )
                if ( uSuppFanin & (1 << v) )
                    Weights[k] += pFreqs[v] - 1;
        }
        // find the most frequent fanin
        WeightMax = 0; 
        FaninMax = -1;
        for ( k = 0; k < pObj->nFans; k++ )
            if ( WeightMax < Weights[k] )
            {
                WeightMax = Weights[k];
                FaninMax = k;
            }
        // no need to reorder if there are no frequent fanins
        if ( FaninMax == -1 )
            continue;
        // move the fanins number k to the first place
        nSwaps = 0;
        pIn = Kit_DsdObjTruth(pObj);
        pOut = p->pMem;
//        for ( v = FaninMax; v < ((int)pObj->nFans)-1; v++ )
        for ( v = FaninMax-1; v >= 0; v-- )
        {
            // swap the fanins
            Temp = pObj->pFans[v];
            pObj->pFans[v] = pObj->pFans[v+1];
            pObj->pFans[v+1] = Temp;
            // swap the truth table variables
            Kit_TruthSwapAdjacentVars( pOut, pIn, pObj->nFans, v );
            pTemp = pIn; pIn = pOut; pOut = pTemp;
            nSwaps++;
        }
        if ( nSwaps & 1 )
            Kit_TruthCopy( pOut, pIn, pObj->nFans );
    }    
}

/**Function*************************************************************

  Synopsis    [Compute the support.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Kit_DsdGetSupports_rec( Kit_DsdNtk_t * p, int iLit )
{
    Kit_DsdObj_t * pObj;
    unsigned uSupport, k;
    int iFaninLit;
    pObj = Kit_DsdNtkObj( p, Abc_Lit2Var(iLit) );
    if ( pObj == NULL )
        return Kit_DsdLitSupport( p, iLit );
    uSupport = 0;
    Kit_DsdObjForEachFanin( p, pObj, iFaninLit, k )
        uSupport |= Kit_DsdGetSupports_rec( p, iFaninLit );
    p->pSupps[pObj->Id - p->nVars] = uSupport;
    assert( uSupport <= 0xFFFF );
    return uSupport;
}

/**Function*************************************************************

  Synopsis    [Compute the support.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Kit_DsdGetSupports( Kit_DsdNtk_t * p )
{
    Kit_DsdObj_t * pRoot;
    unsigned uSupport;
    assert( p->pSupps == NULL );
    p->pSupps = ABC_ALLOC( unsigned, p->nNodes );
    // consider simple special cases
    pRoot = Kit_DsdNtkRoot(p);
    if ( pRoot->Type == KIT_DSD_CONST1 )
    {
        assert( p->nNodes == 1 );
        uSupport = p->pSupps[0] = 0;
    }
    if ( pRoot->Type == KIT_DSD_VAR )
    {
        assert( p->nNodes == 1 );
        uSupport = p->pSupps[0] = Kit_DsdLitSupport( p, pRoot->pFans[0] );
    }
    else
        uSupport = Kit_DsdGetSupports_rec( p, p->Root );
    assert( uSupport <= 0xFFFF );
    return uSupport;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if there is a component with more than 3 inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Kit_DsdFindLargeBox_rec( Kit_DsdNtk_t * pNtk, int Id, int Size )
{
    Kit_DsdObj_t * pObj;
    unsigned iLit, i, RetValue;
    pObj = Kit_DsdNtkObj( pNtk, Id );
    if ( pObj == NULL )
        return 0;
    if ( pObj->Type == KIT_DSD_PRIME && (int)pObj->nFans > Size )
        return 1;
    RetValue = 0;
    Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )
        RetValue |= Kit_DsdFindLargeBox_rec( pNtk, Abc_Lit2Var(iLit), Size );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if there is a component with more than 3 inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Kit_DsdFindLargeBox( Kit_DsdNtk_t * pNtk, int Size )
{
    return Kit_DsdFindLargeBox_rec( pNtk, Abc_Lit2Var(pNtk->Root), Size );
}

/**Function*************************************************************

  Synopsis    [Returns 1 if there is a component with more than 3 inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Kit_DsdCountAigNodes_rec( Kit_DsdNtk_t * pNtk, int Id )
{
    Kit_DsdObj_t * pObj;
    unsigned iLit, i, RetValue = 0;
    pObj = Kit_DsdNtkObj( pNtk, Id );
    if ( pObj == NULL )
        return 0;
    if ( pObj->Type == KIT_DSD_CONST1 || pObj->Type == KIT_DSD_VAR )
        return 0;
    if ( pObj->nFans < 2 ) // why this happens? - need to figure out
        return 0; 
    assert( pObj->nFans > 1 );
    if ( pObj->Type == KIT_DSD_AND )
        RetValue = ((int)pObj->nFans - 1);
    else if ( pObj->Type == KIT_DSD_XOR )
        RetValue = ((int)pObj->nFans - 1) * 3;
    else if ( pObj->Type == KIT_DSD_PRIME )
    {
        // assuming MUX decomposition
        assert( (int)pObj->nFans == 3 );
        RetValue = 3;
    }
    else assert( 0 );
    Kit_DsdObjForEachFanin( pNtk, pObj, iLit, i )
        RetValue += Kit_DsdCountAigNodes_rec( pNtk, Abc_Lit2Var(iLit) );
    return RetValue;
}


/**Function*************************************************************

  Synopsis    [Returns 1 if there is a component with more than 3 inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Kit_DsdCountAigNodes2( Kit_DsdNtk_t * pNtk )
{
    return Kit_DsdCountAigNodes_rec( pNtk, Abc_Lit2Var(pNtk->Root) );
}

/**Function*************************************************************

  Synopsis    [Returns 1 if there is a component with more than 3 inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Kit_DsdCountAigNodes( Kit_DsdNtk_t * pNtk )
{
    Kit_DsdObj_t * pObj;
    int i, Counter = 0;
    for ( i = 0; i < pNtk->nNodes; i++ )
    {
        pObj = pNtk->pNodes[i];
        if ( pObj->Type == KIT_DSD_AND )
            Counter += ((int)pObj->nFans - 1);
        else if ( pObj->Type == KIT_DSD_XOR )
            Counter += ((int)pObj->nFans - 1) * 3;
        else if ( pObj->Type == KIT_DSD_PRIME ) // assuming MUX decomposition
            Counter += 3;
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the non-DSD 4-var func is implementable with two 3-LUTs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Kit_DsdRootNodeHasCommonVars( Kit_DsdObj_t * pObj0, Kit_DsdObj_t * pObj1 )
{
    unsigned i, k;
    for ( i = 0; i < pObj0->nFans; i++ )
    {
        if ( Abc_Lit2Var(pObj0->pFans[i]) >= 4 )
            continue;
        for ( k = 0; k < pObj1->nFans; k++ )
            if ( Abc_Lit2Var(pObj0->pFans[i]) == Abc_Lit2Var(pObj1->pFans[k]) )
                return 1;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the non-DSD 4-var func is implementable with two 3-LUTs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Kit_DsdCheckVar4Dec2( Kit_DsdNtk_t * pNtk0, Kit_DsdNtk_t * pNtk1 )
{
    assert( pNtk0->nVars == 4 );
    assert( pNtk1->nVars == 4 );
    if ( Kit_DsdFindLargeBox(pNtk0, 2) )
        return 0;
    if ( Kit_DsdFindLargeBox(pNtk1, 2) )
        return 0;
    return Kit_DsdRootNodeHasCommonVars( Kit_DsdNtkRoot(pNtk0), Kit_DsdNtkRoot(pNtk1) );
}

/**Function*************************************************************

  Synopsis    [Performs decomposition of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_DsdDecompose_rec( Kit_DsdNtk_t * pNtk, Kit_DsdObj_t * pObj, unsigned uSupp, unsigned short * pPar, int nDecMux )
{
    Kit_DsdObj_t * pRes, * pRes0, * pRes1;
    int nWords = Kit_TruthWordNum(pObj->nFans);
    unsigned * pTruth = Kit_DsdObjTruth(pObj);
    unsigned * pCofs2[2] = { pNtk->pMem, pNtk->pMem + nWords };
    unsigned * pCofs4[2][2] = { {pNtk->pMem + 2 * nWords, pNtk->pMem + 3 * nWords}, {pNtk->pMem + 4 * nWords, pNtk->pMem + 5 * nWords} };
    int i, iLit0, iLit1, nFans0, nFans1, nPairs;
    int fEquals[2][2], fOppos, fPairs[4][4];
    unsigned j, k, nFansNew, uSupp0, uSupp1;

    assert( pObj->nFans > 0 );
    assert( pObj->Type == KIT_DSD_PRIME );
    assert( uSupp == (uSupp0 = (unsigned)Kit_TruthSupport(pTruth, pObj->nFans)) );

    // compress the truth table
    if ( uSupp != Kit_BitMask(pObj->nFans) )
    {
        nFansNew = Kit_WordCountOnes(uSupp);
        Kit_TruthShrink( pNtk->pMem, pTruth, nFansNew, pObj->nFans, uSupp, 1 );
        for ( j = k = 0; j < pObj->nFans; j++ )
            if ( uSupp & (1 << j) )
                pObj->pFans[k++] = pObj->pFans[j];
        assert( k == nFansNew );
        pObj->nFans = k;
        uSupp = Kit_BitMask(pObj->nFans);
    }

    // consider the single variable case
    if ( pObj->nFans == 1 )
    {
        pObj->Type = KIT_DSD_NONE;
        if ( pTruth[0] == 0x55555555 )
            pObj->pFans[0] = Abc_LitNot(pObj->pFans[0]);
        else
            assert( pTruth[0] == 0xAAAAAAAA );
        // update the parent pointer
        *pPar = Abc_LitNotCond( pObj->pFans[0], Abc_LitIsCompl(*pPar) );
        return;
    }

    // decompose the output
    if ( !pObj->fMark )
    for ( i = pObj->nFans - 1; i >= 0; i-- )
    {
        // get the two-variable cofactors
        Kit_TruthCofactor0New( pCofs2[0], pTruth, pObj->nFans, i );
        Kit_TruthCofactor1New( pCofs2[1], pTruth, pObj->nFans, i );
//        assert( !Kit_TruthVarInSupport( pCofs2[0], pObj->nFans, i) );
//        assert( !Kit_TruthVarInSupport( pCofs2[1], pObj->nFans, i) );
        // get the constant cofs
        fEquals[0][0] = Kit_TruthIsConst0( pCofs2[0], pObj->nFans );
        fEquals[0][1] = Kit_TruthIsConst0( pCofs2[1], pObj->nFans );
        fEquals[1][0] = Kit_TruthIsConst1( pCofs2[0], pObj->nFans );
        fEquals[1][1] = Kit_TruthIsConst1( pCofs2[1], pObj->nFans );
        fOppos        = Kit_TruthIsOpposite( pCofs2[0], pCofs2[1], pObj->nFans );
        assert( !Kit_TruthIsEqual(pCofs2[0], pCofs2[1], pObj->nFans) );
        if ( fEquals[0][0] + fEquals[0][1] + fEquals[1][0] + fEquals[1][1] + fOppos == 0 )
        {
            // check the MUX decomposition
            uSupp0 = Kit_TruthSupport( pCofs2[0], pObj->nFans );
            uSupp1 = Kit_TruthSupport( pCofs2[1], pObj->nFans );
            assert( uSupp == (uSupp0 | uSupp1 | (1<<i)) );
            if ( uSupp0 & uSupp1 )
                continue;
            // perform MUX decomposition
            pRes0 = Kit_DsdObjAlloc( pNtk, KIT_DSD_PRIME, pObj->nFans );
            pRes1 = Kit_DsdObjAlloc( pNtk, KIT_DSD_PRIME, pObj->nFans );
            for ( k = 0; k < pObj->nFans; k++ )
            {
                pRes0->pFans[k] = (uSupp0 & (1 << k))? pObj->pFans[k] : 127;
                pRes1->pFans[k] = (uSupp1 & (1 << k))? pObj->pFans[k] : 127;
            }
            Kit_TruthCopy( Kit_DsdObjTruth(pRes0), pCofs2[0], pObj->nFans );        
            Kit_TruthCopy( Kit_DsdObjTruth(pRes1), pCofs2[1], pObj->nFans ); 
            // update the current one
            assert( pObj->Type == KIT_DSD_PRIME );
            pTruth[0] = 0xCACACACA;
            pObj->nFans = 3;
            pObj->pFans[2] = pObj->pFans[i];
            pObj->pFans[0] = 2*pRes0->Id; pRes0->nRefs++;
            pObj->pFans[1] = 2*pRes1->Id; pRes1->nRefs++;
            // call recursively
            Kit_DsdDecompose_rec( pNtk, pRes0, uSupp0, pObj->pFans + 0, nDecMux );
            Kit_DsdDecompose_rec( pNtk, pRes1, uSupp1, pObj->pFans + 1, nDecMux );
            return;
        }

        // create the new node
        pRes = Kit_DsdObjAlloc( pNtk, KIT_DSD_AND, 2 );
        pRes->nRefs++;
        pRes->nFans = 2;
        pRes->pFans[0] = pObj->pFans[i];  pObj->pFans[i] = 127;  uSupp &= ~(1 << i);
        pRes->pFans[1] = 2*pObj->Id;
        // update the parent pointer
        *pPar = Abc_LitNotCond( 2 * pRes->Id, Abc_LitIsCompl(*pPar) );
        // consider different decompositions
        if ( fEquals[0][0] )
        {
            Kit_TruthCopy( pTruth, pCofs2[1], pObj->nFans );
        }
        else if ( fEquals[0][1] )
        {
            pRes->pFans[0] = Abc_LitNot(pRes->pFans[0]);
            Kit_TruthCopy( pTruth, pCofs2[0], pObj->nFans );
        }
        else if ( fEquals[1][0] )
        {
            *pPar = Abc_LitNot(*pPar);
            pRes->pFans[1] = Abc_LitNot(pRes->pFans[1]);
            Kit_TruthCopy( pTruth, pCofs2[1], pObj->nFans );
        }
        else if ( fEquals[1][1] )
        {
            *pPar = Abc_LitNot(*pPar);
            pRes->pFans[0] = Abc_LitNot(pRes->pFans[0]);  
            pRes->pFans[1] = Abc_LitNot(pRes->pFans[1]);
            Kit_TruthCopy( pTruth, pCofs2[0], pObj->nFans );
        }
        else if ( fOppos )
        {
            pRes->Type = KIT_DSD_XOR;
            Kit_TruthCopy( pTruth, pCofs2[0], pObj->nFans );
        }
        else
            assert( 0 );
        // decompose the remainder
        assert( Kit_DsdObjTruth(pObj) == pTruth );
        Kit_DsdDecompose_rec( pNtk, pObj, uSupp, pRes->pFans + 1, nDecMux );
        return;
    }
    pObj->fMark = 1;

    // decompose the input
    for ( i = pObj->nFans - 1; i >= 0; i-- )
    {
        assert( Kit_TruthVarInSupport( pTruth, pObj->nFans, i ) );
        // get the single variale cofactors
        Kit_TruthCofactor0New( pCofs2[0], pTruth, pObj->nFans, i );
        Kit_TruthCofactor1New( pCofs2[1], pTruth, pObj->nFans, i );
        // check the existence of MUX decomposition
        uSupp0 = Kit_TruthSupport( pCofs2[0], pObj->nFans );
        uSupp1 = Kit_TruthSupport( pCofs2[1], pObj->nFans );
        assert( uSupp == (uSupp0 | uSupp1 | (1<<i)) );
        // if one of the cofs is a constant, it is time to check the output again
        if ( uSupp0 == 0 || uSupp1 == 0 )
        {
            pObj->fMark = 0;
            Kit_DsdDecompose_rec( pNtk, pObj, uSupp, pPar, nDecMux );
            return;
        }
        assert( uSupp0 && uSupp1 );
        // get the number of unique variables
        nFans0 = Kit_WordCountOnes( uSupp0 & ~uSupp1 );
        nFans1 = Kit_WordCountOnes( uSupp1 & ~uSupp0 );
        if ( nFans0 == 1 && nFans1 == 1 )
        {
            // get the cofactors w.r.t. the unique variables
            iLit0 = Kit_WordFindFirstBit( uSupp0 & ~uSupp1 );
            iLit1 = Kit_WordFindFirstBit( uSupp1 & ~uSupp0 );
            // get four cofactors                                        
            Kit_TruthCofactor0New( pCofs4[0][0], pCofs2[0], pObj->nFans, iLit0 );
            Kit_TruthCofactor1New( pCofs4[0][1], pCofs2[0], pObj->nFans, iLit0 );
            Kit_TruthCofactor0New( pCofs4[1][0], pCofs2[1], pObj->nFans, iLit1 );
            Kit_TruthCofactor1New( pCofs4[1][1], pCofs2[1], pObj->nFans, iLit1 );
            // check existence conditions
            fEquals[0][0] = Kit_TruthIsEqual( pCofs4[0][0], pCofs4[1][0], pObj->nFans );
            fEquals[0][1] = Kit_TruthIsEqual( pCofs4[0][1], pCofs4[1][1], pObj->nFans );
            fEquals[1][0] = Kit_TruthIsEqual( pCofs4[0][0], pCofs4[1][1], pObj->nFans );
            fEquals[1][1] = Kit_TruthIsEqual( pCofs4[0][1], pCofs4[1][0], pObj->nFans );
            if ( (fEquals[0][0] && fEquals[0][1]) || (fEquals[1][0] && fEquals[1][1]) )
            {
                // construct the MUX
                pRes = Kit_DsdObjAlloc( pNtk, KIT_DSD_PRIME, 3 );
                Kit_DsdObjTruth(pRes)[0] = 0xCACACACA;
                pRes->nRefs++;
                pRes->nFans = 3;
                pRes->pFans[0] = pObj->pFans[iLit0]; pObj->pFans[iLit0] = 127;  uSupp &= ~(1 << iLit0);
                pRes->pFans[1] = pObj->pFans[iLit1]; pObj->pFans[iLit1] = 127;  uSupp &= ~(1 << iLit1);
                pRes->pFans[2] = pObj->pFans[i];     pObj->pFans[i] = 2 * pRes->Id; // remains in support
                // update the node
//                if ( fEquals[0][0] && fEquals[0][1] )
//                    Kit_TruthMuxVar( pTruth, pCofs4[0][0], pCofs4[0][1], pObj->nFans, i );
//                else
//                    Kit_TruthMuxVar( pTruth, pCofs4[0][1], pCofs4[0][0], pObj->nFans, i );
                Kit_TruthMuxVar( pTruth, pCofs4[1][0], pCofs4[1][1], pObj->nFans, i );
                if ( fEquals[1][0] && fEquals[1][1] )
                    pRes->pFans[0] = Abc_LitNot(pRes->pFans[0]);
                // decompose the remainder
                Kit_DsdDecompose_rec( pNtk, pObj, uSupp, pPar, nDecMux );
                return;
            }
        }

        // try other inputs
        for ( k = i+1; k < pObj->nFans; k++ )
        {
            // get four cofactors                                                ik
            Kit_TruthCofactor0New( pCofs4[0][0], pCofs2[0], pObj->nFans, k ); // 00 
            Kit_TruthCofactor1New( pCofs4[0][1], pCofs2[0], pObj->nFans, k ); // 01 
            Kit_TruthCofactor0New( pCofs4[1][0], pCofs2[1], pObj->nFans, k ); // 10 
            Kit_TruthCofactor1New( pCofs4[1][1], pCofs2[1], pObj->nFans, k ); // 11 
            // compare equal pairs
            fPairs[0][1] = fPairs[1][0] = Kit_TruthIsEqual( pCofs4[0][0], pCofs4[0][1], pObj->nFans );
            fPairs[0][2] = fPairs[2][0] = Kit_TruthIsEqual( pCofs4[0][0], pCofs4[1][0], pObj->nFans );
            fPairs[0][3] = fPairs[3][0] = Kit_TruthIsEqual( pCofs4[0][0], pCofs4[1][1], pObj->nFans );
            fPairs[1][2] = fPairs[2][1] = Kit_TruthIsEqual( pCofs4[0][1], pCofs4[1][0], pObj->nFans );
            fPairs[1][3] = fPairs[3][1] = Kit_TruthIsEqual( pCofs4[0][1], pCofs4[1][1], pObj->nFans );
            fPairs[2][3] = fPairs[3][2] = Kit_TruthIsEqual( pCofs4[1][0], pCofs4[1][1], pObj->nFans );
            nPairs = fPairs[0][1] + fPairs[0][2] + fPairs[0][3] + fPairs[1][2] + fPairs[1][3] + fPairs[2][3];
            if ( nPairs != 3 && nPairs != 2 )
                continue;

            // decomposition exists
            pRes = Kit_DsdObjAlloc( pNtk, KIT_DSD_AND, 2 );
            pRes->nRefs++;
            pRes->nFans = 2;
            pRes->pFans[0] = pObj->pFans[k]; pObj->pFans[k] = 2 * pRes->Id;  // remains in support
            pRes->pFans[1] = pObj->pFans[i]; pObj->pFans[i] = 127;       uSupp &= ~(1 << i);
            if ( !fPairs[0][1] && !fPairs[0][2] && !fPairs[0][3] ) // 00
            {
                pRes->pFans[0] = Abc_LitNot(pRes->pFans[0]);  
                pRes->pFans[1] = Abc_LitNot(pRes->pFans[1]);
                Kit_TruthMuxVar( pTruth, pCofs4[1][1], pCofs4[0][0], pObj->nFans, k );
            }
            else if ( !fPairs[1][0] && !fPairs[1][2] && !fPairs[1][3] ) // 01
            {
                pRes->pFans[1] = Abc_LitNot(pRes->pFans[1]);  
                Kit_TruthMuxVar( pTruth, pCofs4[0][0], pCofs4[0][1], pObj->nFans, k );
            }
            else if ( !fPairs[2][0] && !fPairs[2][1] && !fPairs[2][3] ) // 10
            {
                pRes->pFans[0] = Abc_LitNot(pRes->pFans[0]);  
                Kit_TruthMuxVar( pTruth, pCofs4[0][0], pCofs4[1][0], pObj->nFans, k );
            }
            else if ( !fPairs[3][0] && !fPairs[3][1] && !fPairs[3][2] ) // 11
            {
//                unsigned uSupp0 = Kit_TruthSupport(pCofs4[0][0], pObj->nFans);
//                unsigned uSupp1 = Kit_TruthSupport(pCofs4[1][1], pObj->nFans);
//                unsigned uSupp;
//                Extra_PrintBinary( stdout, &uSupp0, pObj->nFans ); printf( "\n" );
//                Extra_PrintBinary( stdout, &uSupp1, pObj->nFans ); printf( "\n" );
                Kit_TruthMuxVar( pTruth, pCofs4[0][0], pCofs4[1][1], pObj->nFans, k );
//                uSupp = Kit_TruthSupport(pTruth, pObj->nFans);
//                Extra_PrintBinary( stdout, &uSupp, pObj->nFans ); printf( "\n" ); printf( "\n" );
            }
            else
            {
                assert( fPairs[0][3] && fPairs[1][2] );
                pRes->Type = KIT_DSD_XOR;;
                Kit_TruthMuxVar( pTruth, pCofs4[0][0], pCofs4[0][1], pObj->nFans, k );
            }
            // decompose the remainder
            Kit_DsdDecompose_rec( pNtk, pObj, uSupp, pPar, nDecMux );
            return;
        }
    }

    // if all decomposition methods failed and we are still above the limit, perform MUX-decomposition
    if ( nDecMux > 0 && (int)pObj->nFans > nDecMux )
    {
        int iBestVar = Kit_TruthBestCofVar( pTruth, pObj->nFans, pCofs2[0], pCofs2[1] );
        uSupp0 = Kit_TruthSupport( pCofs2[0], pObj->nFans );
        uSupp1 = Kit_TruthSupport( pCofs2[1], pObj->nFans );
        // perform MUX decomposition
        pRes0 = Kit_DsdObjAlloc( pNtk, KIT_DSD_PRIME, pObj->nFans );
        pRes1 = Kit_DsdObjAlloc( pNtk, KIT_DSD_PRIME, pObj->nFans );
        for ( k = 0; k < pObj->nFans; k++ )
            pRes0->pFans[k] = pRes1->pFans[k] = pObj->pFans[k];
        Kit_TruthCopy( Kit_DsdObjTruth(pRes0), pCofs2[0], pObj->nFans );        
        Kit_TruthCopy( Kit_DsdObjTruth(pRes1), pCofs2[1], pObj->nFans ); 
        // update the current one
        assert( pObj->Type == KIT_DSD_PRIME );
        pTruth[0] = 0xCACACACA;
        pObj->nFans = 3;
        pObj->pFans[2] = pObj->pFans[iBestVar];
        pObj->pFans[0] = 2*pRes0->Id; pRes0->nRefs++;
        pObj->pFans[1] = 2*pRes1->Id; pRes1->nRefs++;
        // call recursively
        Kit_DsdDecompose_rec( pNtk, pRes0, uSupp0, pObj->pFans + 0, nDecMux );
        Kit_DsdDecompose_rec( pNtk, pRes1, uSupp1, pObj->pFans + 1, nDecMux );
    }

}

/**Function*************************************************************

  Synopsis    [Performs decomposition of the truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Kit_DsdNtk_t * Kit_DsdDecomposeInt( unsigned * pTruth, int nVars, int nDecMux )
{
    Kit_DsdNtk_t * pNtk;
    Kit_DsdObj_t * pObj;
    unsigned uSupp;
    int i, nVarsReal;
    assert( nVars <= 16 );
    pNtk = Kit_DsdNtkAlloc( nVars );
    pNtk->Root = Abc_Var2Lit( pNtk->nVars, 0 );
    // create the first node
    pObj = Kit_DsdObjAlloc( pNtk, KIT_DSD_PRIME, nVars );
    assert( pNtk->pNodes[0] == pObj );
    for ( i = 0; i < nVars; i++ )
       pObj->pFans[i] = Abc_Var2Lit( i, 0 );
    Kit_TruthCopy( Kit_DsdObjTruth(pObj), pTruth, nVars );
    uSupp = Kit_TruthSupport( pTruth, nVars );
    // consider special cases
    nVarsReal = Kit_WordCountOnes( uSupp );
    if ( nVarsReal == 0 )
    {
        pObj->Type = KIT_DSD_CONST1;
        pObj->nFans = 0;
        if ( pTruth[0] == 0 )
             pNtk->Root = Abc_LitNot(pNtk->Root);
        return pNtk;
    }
    if ( nVarsReal == 1 )
    {
        pObj->Type = KIT_DSD_VAR;
        pObj->nFans = 1;
        pObj->pFans[0] = Abc_Var2Lit( Kit_WordFindFirstBit(uSupp), (pTruth[0] & 1) );
        return pNtk;
    }
    Kit_DsdDecompose_rec( pNtk, pNtk->pNodes[0], uSupp, &pNtk->Root, nDecMux );
    return pNtk;
}

/**Function*************************************************************

  Synopsis    [Performs decomposition of the truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Kit_DsdNtk_t * Kit_DsdDecompose( unsigned * pTruth, int nVars )
{
    return Kit_DsdDecomposeInt( pTruth, nVars, 0 );
}

/**Function*************************************************************

  Synopsis    [Performs decomposition of the truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Kit_DsdNtk_t * Kit_DsdDecomposeExpand( unsigned * pTruth, int nVars )
{
    Kit_DsdNtk_t * pNtk, * pTemp;
    pNtk = Kit_DsdDecomposeInt( pTruth, nVars, 0 );
    pNtk = Kit_DsdExpand( pTemp = pNtk );      
    Kit_DsdNtkFree( pTemp );
    return pNtk;
}

/**Function*************************************************************

  Synopsis    [Performs decomposition of the truth table.]

  Description [Uses MUXes to break-down large prime nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Kit_DsdNtk_t *  Kit_DsdDecomposeMux( unsigned * pTruth, int nVars, int nDecMux )
{
/*
    Kit_DsdNtk_t * pNew;
    Kit_DsdObj_t * pObjNew;
    assert( nVars <= 16 );
    // create a new network
    pNew = Kit_DsdNtkAlloc( nVars );
    // consider simple special cases
    if ( nVars == 0 )
    {
        pObjNew = Kit_DsdObjAlloc( pNew, KIT_DSD_CONST1, 0 );
        pNew->Root = Abc_Var2Lit( pObjNew->Id, (int)(pTruth[0] == 0) );
        return pNew;
    }
    if ( nVars == 1 )
    {
        pObjNew = Kit_DsdObjAlloc( pNew, KIT_DSD_VAR, 1 );
        pObjNew->pFans[0] = Abc_Var2Lit( 0, 0 );
        pNew->Root = Abc_Var2Lit( pObjNew->Id, (int)(pTruth[0] != 0xAAAAAAAA) );
        return pNew;
    }
*/
    return Kit_DsdDecomposeInt( pTruth, nVars, nDecMux );
}

/**Function*************************************************************

  Synopsis    [Performs decomposition of the truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Kit_DsdTestCofs( Kit_DsdNtk_t * pNtk, unsigned * pTruthInit )
{
    Kit_DsdNtk_t * pNtk0, * pNtk1, * pTemp;
//    Kit_DsdObj_t * pRoot;
    unsigned * pCofs2[2] = { pNtk->pMem, pNtk->pMem + Kit_TruthWordNum(pNtk->nVars) };
    unsigned i, * pTruth;
    int fVerbose = 1;
    int RetValue = 0;

    pTruth = pTruthInit;
//    pRoot = Kit_DsdNtkRoot(pNtk);
//    pTruth = Kit_DsdObjTruth(pRoot);
//    assert( pRoot->nFans == pNtk->nVars );

    if ( fVerbose )
    {
        printf( "Function: " );
//        Extra_PrintBinary( stdout, pTruth, (1 << pNtk->nVars) ); 
        Extra_PrintHexadecimal( stdout, pTruth, pNtk->nVars ); 
        printf( "\n" );
        Kit_DsdPrint( stdout, pNtk ), printf( "\n" );
    }
    for ( i = 0; i < pNtk->nVars; i++ )
    {
        Kit_TruthCofactor0New( pCofs2[0], pTruth, pNtk->nVars, i );
        pNtk0 = Kit_DsdDecompose( pCofs2[0], pNtk->nVars );
        pNtk0 = Kit_DsdExpand( pTemp = pNtk0 );
        Kit_DsdNtkFree( pTemp );

        if ( fVerbose )
        {
            printf( "Cof%d0: ", i );
            Kit_DsdPrint( stdout, pNtk0 ), printf( "\n" );
        }

        Kit_TruthCofactor1New( pCofs2[1], pTruth, pNtk->nVars, i );
        pNtk1 = Kit_DsdDecompose( pCofs2[1], pNtk->nVars );
        pNtk1 = Kit_DsdExpand( pTemp = pNtk1 );
        Kit_DsdNtkFree( pTemp );

        if ( fVerbose )
        {
            printf( "Cof%d1: ", i );
            Kit_DsdPrint( stdout, pNtk1 ), printf( "\n" );
        }

//        if ( Kit_DsdCheckVar4Dec2( pNtk0, pNtk1 ) )
//            RetValue = 1;

        Kit_DsdNtkFree( pNtk0 );
        Kit_DsdNtkFree( pNtk1 );
    }
    if ( fVerbose )
        printf( "\n" );

    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Performs decomposition of the truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Kit_DsdEval( unsigned * pTruth, int nVars, int nLutSize )
{
    Kit_DsdMan_t * p;
    Kit_DsdNtk_t * pNtk;
    unsigned * pTruthC;
    int Result;

    // decompose the function
    pNtk = Kit_DsdDecompose( pTruth, nVars );
    Result = Kit_DsdCountLuts( pNtk, nLutSize );
//    printf( "\n" );
//    Kit_DsdPrint( stdout, pNtk );
//    printf( "Eval = %d.\n", Result );

    // recompute the truth table
    p = Kit_DsdManAlloc( nVars, Kit_DsdNtkObjNum(pNtk) );
    pTruthC = Kit_DsdTruthCompute( p, pNtk );
    if ( !Kit_TruthIsEqual( pTruth, pTruthC, nVars ) )
        printf( "Verification failed.\n" );
    Kit_DsdManFree( p );

    Kit_DsdNtkFree( pNtk );
    return Result;
}

/**Function*************************************************************

  Synopsis    [Performs decomposition of the truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_DsdVerify( Kit_DsdNtk_t * pNtk, unsigned * pTruth, int nVars )
{
    Kit_DsdMan_t * p;
    unsigned * pTruthC;
    p = Kit_DsdManAlloc( nVars, Kit_DsdNtkObjNum(pNtk)+2 );
    pTruthC = Kit_DsdTruthCompute( p, pNtk );
    if ( !Extra_TruthIsEqual( pTruth, pTruthC, nVars ) )
        printf( "Verification failed.\n" );
    Kit_DsdManFree( p );
}

/**Function*************************************************************

  Synopsis    [Performs decomposition of the truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_DsdTest( unsigned * pTruth, int nVars )
{
    Kit_DsdMan_t * p;
    unsigned * pTruthC;
    Kit_DsdNtk_t * pNtk, * pTemp;
    pNtk = Kit_DsdDecompose( pTruth, nVars );

//    if ( Kit_DsdFindLargeBox(pNtk, Abc_Lit2Var(pNtk->Root)) )
//        Kit_DsdPrint( stdout, pNtk );

//    if ( Kit_DsdNtkRoot(pNtk)->nFans == (unsigned)nVars && nVars == 6 )

//    printf( "\n" );
//    Kit_DsdPrint( stdout, pNtk );

    pNtk = Kit_DsdExpand( pTemp = pNtk );
    Kit_DsdNtkFree( pTemp );

    Kit_DsdPrint( stdout, pNtk ), printf( "\n" );

//    if ( Kit_DsdFindLargeBox(pNtk, Abc_Lit2Var(pNtk->Root)) )
//        Kit_DsdTestCofs( pNtk, pTruth );

    // recompute the truth table
    p = Kit_DsdManAlloc( nVars, Kit_DsdNtkObjNum(pNtk) );
    pTruthC = Kit_DsdTruthCompute( p, pNtk );
//    Extra_PrintBinary( stdout, pTruth, 1 << nVars ); printf( "\n" );
//    Extra_PrintBinary( stdout, pTruthC, 1 << nVars ); printf( "\n" );
    if ( Extra_TruthIsEqual( pTruth, pTruthC, nVars ) )
    {
//        printf( "Verification is okay.\n" );
    }
    else
        printf( "Verification failed.\n" );
    Kit_DsdManFree( p );


    Kit_DsdNtkFree( pNtk );
}

/**Function*************************************************************

  Synopsis    [Performs decomposition of the truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_DsdPrecompute4Vars()
{
    Kit_DsdMan_t * p;
    Kit_DsdNtk_t * pNtk, * pTemp;
    FILE * pFile;
    unsigned uTruth;
    unsigned * pTruthC;
    char Buffer[256];
    int i, RetValue;
    int Counter1 = 0, Counter2 = 0;

    pFile = fopen( "5npn/npn4.txt", "r" );
    for ( i = 0; fgets( Buffer, 100, pFile ); i++ )
    {
        Buffer[6] = 0;
        Extra_ReadHexadecimal( &uTruth, Buffer+2, 4 );
        uTruth = ((uTruth & 0xffff) << 16) | (uTruth & 0xffff);
        pNtk = Kit_DsdDecompose( &uTruth, 4 );

        pNtk = Kit_DsdExpand( pTemp = pNtk );
        Kit_DsdNtkFree( pTemp );


        if ( Kit_DsdFindLargeBox(pNtk, 3) )
        {
//            RetValue = 0;
            RetValue = Kit_DsdTestCofs( pNtk, &uTruth );
            printf( "\n" );
            printf( "%3d : Non-DSD function  %s  %s\n", i, Buffer + 2, RetValue? "implementable" : "" );
            Kit_DsdPrint( stdout, pNtk ), printf( "\n" );

            Counter1++;
            Counter2 += RetValue;
        }

/*
        printf( "%3d : Function  %s   ", i, Buffer + 2 );
        if ( !Kit_DsdFindLargeBox(pNtk, 3) )
            Kit_DsdPrint( stdout, pNtk );
        else
            printf( "\n" );
*/

        p = Kit_DsdManAlloc( 4, Kit_DsdNtkObjNum(pNtk) );
        pTruthC = Kit_DsdTruthCompute( p, pNtk );
        if ( !Extra_TruthIsEqual( &uTruth, pTruthC, 4 ) )
            printf( "Verification failed.\n" );
        Kit_DsdManFree( p );

        Kit_DsdNtkFree( pNtk );
    }
    fclose( pFile );
    printf( "non-DSD = %d   implementable = %d\n", Counter1, Counter2 );
}


/**Function*************************************************************

  Synopsis    [Returns the set of cofactoring variables.]

  Description [If there is no DSD components returns 0.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Kit_DsdCofactoringGetVars( Kit_DsdNtk_t ** ppNtk, int nSize, int * pVars )
{
    Kit_DsdObj_t * pObj;
    unsigned m;
    int i, k, v, Var, nVars, iFaninLit;
    // go through all the networks
    nVars = 0;
    for ( i = 0; i < nSize; i++ )
    {
        // go through the prime objects of each networks
        Kit_DsdNtkForEachObj( ppNtk[i], pObj, k )     
        {
            if ( pObj->Type != KIT_DSD_PRIME )
                continue;
            if ( pObj->nFans == 3 )
                continue;
            // collect direct fanin variables
            Kit_DsdObjForEachFanin( ppNtk[i], pObj, iFaninLit, m )
            {
                if ( !Kit_DsdLitIsLeaf(ppNtk[i], iFaninLit) )
                    continue;
                // add it to the array
                Var = Abc_Lit2Var( iFaninLit );
                for ( v = 0; v < nVars; v++ )
                    if ( pVars[v] == Var )
                        break;
                if ( v == nVars )
                    pVars[nVars++] = Var;
            }
        }
    }
    return nVars;
}

/**Function*************************************************************

  Synopsis    [Canonical decomposition into completely DSD-structure.]

  Description [Returns the number of cofactoring steps. Also returns
  the cofactoring variables in pVars.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Kit_DsdCofactoring( unsigned * pTruth, int nVars, int * pCofVars, int nLimit, int fVerbose )
{
    Kit_DsdNtk_t * ppNtks[5][16] = {
      {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
      {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
    };
    Kit_DsdNtk_t * pTemp;
    unsigned * ppCofs[5][16];
    int pTryVars[16], nTryVars;
    int nPrimeSizeMin, nPrimeSizeMax, nPrimeSizeCur;
    int nSuppSizeMin, nSuppSizeMax, iVarBest;
    int i, k, v, nStep, nSize, nMemSize;
    assert( nLimit < 5 );

    // allocate storage for cofactors
    nMemSize = Kit_TruthWordNum(nVars);
    ppCofs[0][0] = ABC_ALLOC( unsigned, 80 * nMemSize );
    nSize = 0;
    for ( i = 0; i <  5; i++ )
    for ( k = 0; k < 16; k++ )
        ppCofs[i][k] = ppCofs[0][0] + nMemSize * nSize++;
    assert( nSize == 80 );

    // copy the function
    Kit_TruthCopy( ppCofs[0][0], pTruth, nVars );
    ppNtks[0][0] = Kit_DsdDecompose( ppCofs[0][0], nVars );

    if ( fVerbose )
        printf( "\nProcessing prime function with %d support variables:\n", nVars );

    // perform recursive cofactoring
    for ( nStep = 0; nStep < nLimit; nStep++ )
    {
        nSize = (1 << nStep);
        // find the variables to use in the cofactoring step
        nTryVars = Kit_DsdCofactoringGetVars( ppNtks[nStep], nSize, pTryVars );
        if ( nTryVars == 0 )
            break;
        // cofactor w.r.t. the above variables
        iVarBest = -1;
        nPrimeSizeMin = 10000;
        nSuppSizeMin  = 10000;
        for ( v = 0; v < nTryVars; v++ )
        {
            nPrimeSizeMax = 0;
            nSuppSizeMax = 0;
            for ( i = 0; i < nSize; i++ )
            {
                // cofactor and decompose cofactors          
                Kit_TruthCofactor0New( ppCofs[nStep+1][2*i+0], ppCofs[nStep][i], nVars, pTryVars[v] );
                Kit_TruthCofactor1New( ppCofs[nStep+1][2*i+1], ppCofs[nStep][i], nVars, pTryVars[v] );
                ppNtks[nStep+1][2*i+0] = Kit_DsdDecompose( ppCofs[nStep+1][2*i+0], nVars );
                ppNtks[nStep+1][2*i+1] = Kit_DsdDecompose( ppCofs[nStep+1][2*i+1], nVars );
                // compute the largest non-decomp block
                nPrimeSizeCur  = Kit_DsdNonDsdSizeMax(ppNtks[nStep+1][2*i+0]);
                nPrimeSizeMax  = KIT_MAX( nPrimeSizeMax, nPrimeSizeCur );
                nPrimeSizeCur  = Kit_DsdNonDsdSizeMax(ppNtks[nStep+1][2*i+1]);
                nPrimeSizeMax  = KIT_MAX( nPrimeSizeMax, nPrimeSizeCur );
                // compute the sum total of supports
                nSuppSizeMax += Kit_TruthSupportSize( ppCofs[nStep+1][2*i+0], nVars );
                nSuppSizeMax += Kit_TruthSupportSize( ppCofs[nStep+1][2*i+1], nVars );
                // free the networks
                Kit_DsdNtkFree( ppNtks[nStep+1][2*i+0] );
                Kit_DsdNtkFree( ppNtks[nStep+1][2*i+1] );
            }
            // find the min max support size of the prime component
            if ( nPrimeSizeMin > nPrimeSizeMax || (nPrimeSizeMin == nPrimeSizeMax && nSuppSizeMin > nSuppSizeMax) )
            {
                nPrimeSizeMin = nPrimeSizeMax;
                nSuppSizeMin  = nSuppSizeMax;
                iVarBest      = pTryVars[v];
            }
        }
        assert( iVarBest != -1 );
        // save the variable
        if ( pCofVars )
            pCofVars[nStep] = iVarBest;
        // cofactor w.r.t. the best
        for ( i = 0; i < nSize; i++ )
        {
            Kit_TruthCofactor0New( ppCofs[nStep+1][2*i+0], ppCofs[nStep][i], nVars, iVarBest );
            Kit_TruthCofactor1New( ppCofs[nStep+1][2*i+1], ppCofs[nStep][i], nVars, iVarBest );
            ppNtks[nStep+1][2*i+0] = Kit_DsdDecompose( ppCofs[nStep+1][2*i+0], nVars );
            ppNtks[nStep+1][2*i+1] = Kit_DsdDecompose( ppCofs[nStep+1][2*i+1], nVars );
            if ( fVerbose )
            {
                ppNtks[nStep+1][2*i+0] = Kit_DsdExpand( pTemp = ppNtks[nStep+1][2*i+0] );
                Kit_DsdNtkFree( pTemp );
                ppNtks[nStep+1][2*i+1] = Kit_DsdExpand( pTemp = ppNtks[nStep+1][2*i+1] );
                Kit_DsdNtkFree( pTemp );

                printf( "Cof%d%d: ", nStep+1, 2*i+0 );
                Kit_DsdPrint( stdout, ppNtks[nStep+1][2*i+0] ), printf( "\n" );
                printf( "Cof%d%d: ", nStep+1, 2*i+1 );
                Kit_DsdPrint( stdout, ppNtks[nStep+1][2*i+1] ), printf( "\n" );
            }
        }
    }

    // free the networks
    for ( i = 0; i <  5; i++ )
    for ( k = 0; k < 16; k++ )
        if ( ppNtks[i][k] )
            Kit_DsdNtkFree( ppNtks[i][k] );
    ABC_FREE( ppCofs[0][0] );

    assert( nStep <= nLimit );
    return nStep;
}

/**Function*************************************************************

  Synopsis    [Canonical decomposition into completely DSD-structure.]

  Description [Returns the number of cofactoring steps. Also returns
  the cofactoring variables in pVars.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_DsdPrintCofactors( unsigned * pTruth, int nVars, int nCofLevel, int fVerbose )
{
    Kit_DsdNtk_t * ppNtks[32] = {0}, * pTemp;
    unsigned * ppCofs[5][16];
    int piCofVar[5];
    int nPrimeSizeMax, nPrimeSizeCur, nSuppSizeMax;
    int i, k, v1, v2, v3, v4, s, nSteps, nSize, nMemSize;
    assert( nCofLevel < 5 );

    // print the function
    ppNtks[0] = Kit_DsdDecompose( pTruth, nVars );
    ppNtks[0] = Kit_DsdExpand( pTemp = ppNtks[0] );
    Kit_DsdNtkFree( pTemp );
    if ( fVerbose )
        Kit_DsdPrint( stdout, ppNtks[0] ), printf( "\n" );
    Kit_DsdNtkFree( ppNtks[0] );

    // allocate storage for cofactors
    nMemSize = Kit_TruthWordNum(nVars);
    ppCofs[0][0] = ABC_ALLOC( unsigned, 80 * nMemSize );
    nSize = 0;
    for ( i = 0; i <  5; i++ )
    for ( k = 0; k < 16; k++ )
        ppCofs[i][k] = ppCofs[0][0] + nMemSize * nSize++;
    assert( nSize == 80 );

    // copy the function
    Kit_TruthCopy( ppCofs[0][0], pTruth, nVars );

    if ( nCofLevel == 1 )
    for ( v1 = 0; v1 < nVars; v1++ )
    {
        nSteps = 0;
        piCofVar[nSteps++] = v1;

        printf( "    Variables { " );
        for ( i = 0; i < nSteps; i++ )
            printf( "%c ", 'a' + piCofVar[i] );
        printf( "}\n" );

        // single cofactors
        for ( s = 1; s <= nSteps; s++ )
        {
            for ( k = 0; k < s; k++ )
            {
                nSize = (1 << k);
                for ( i = 0; i < nSize; i++ )
                {
                    Kit_TruthCofactor0New( ppCofs[k+1][2*i+0], ppCofs[k][i], nVars, piCofVar[k] );
                    Kit_TruthCofactor1New( ppCofs[k+1][2*i+1], ppCofs[k][i], nVars, piCofVar[k] );
                }
            }
        }
        // compute DSD networks
        nSize = (1 << nSteps);
        nPrimeSizeMax = 0;
        nSuppSizeMax = 0;
        for ( i = 0; i < nSize; i++ )
        {
            ppNtks[i] = Kit_DsdDecompose( ppCofs[nSteps][i], nVars );
            ppNtks[i] = Kit_DsdExpand( pTemp = ppNtks[i] );
            Kit_DsdNtkFree( pTemp );
            if ( fVerbose )
            {
                printf( "Cof%d%d: ", nSteps, i );
                Kit_DsdPrint( stdout, ppNtks[i] ), printf( "\n" );
            }
            // compute the largest non-decomp block
            nPrimeSizeCur  = Kit_DsdNonDsdSizeMax(ppNtks[i]);
            nPrimeSizeMax  = KIT_MAX( nPrimeSizeMax, nPrimeSizeCur );
            Kit_DsdNtkFree( ppNtks[i] );
            nSuppSizeMax += Kit_TruthSupportSize( ppCofs[nSteps][i], nVars );
        }
        printf( "Max = %2d. Supps = %2d.\n", nPrimeSizeMax, nSuppSizeMax );
    }

    if ( nCofLevel == 2 )
    for ( v1 = 0; v1 < nVars; v1++ )
    for ( v2 = v1+1; v2 < nVars; v2++ )
    {
        nSteps = 0;
        piCofVar[nSteps++] = v1;
        piCofVar[nSteps++] = v2;

        printf( "    Variables { " );
        for ( i = 0; i < nSteps; i++ )
            printf( "%c ", 'a' + piCofVar[i] );
        printf( "}\n" );

        // single cofactors
        for ( s = 1; s <= nSteps; s++ )
        {
            for ( k = 0; k < s; k++ )
            {
                nSize = (1 << k);
                for ( i = 0; i < nSize; i++ )
                {
                    Kit_TruthCofactor0New( ppCofs[k+1][2*i+0], ppCofs[k][i], nVars, piCofVar[k] );
                    Kit_TruthCofactor1New( ppCofs[k+1][2*i+1], ppCofs[k][i], nVars, piCofVar[k] );
                }
            }
        }
        // compute DSD networks
        nSize = (1 << nSteps);
        nPrimeSizeMax = 0;
        nSuppSizeMax = 0;
        for ( i = 0; i < nSize; i++ )
        {
            ppNtks[i] = Kit_DsdDecompose( ppCofs[nSteps][i], nVars );
            ppNtks[i] = Kit_DsdExpand( pTemp = ppNtks[i] );
            Kit_DsdNtkFree( pTemp );
            if ( fVerbose )
            {
                printf( "Cof%d%d: ", nSteps, i );
                Kit_DsdPrint( stdout, ppNtks[i] ), printf( "\n" );
            }
            // compute the largest non-decomp block
            nPrimeSizeCur  = Kit_DsdNonDsdSizeMax(ppNtks[i]);
            nPrimeSizeMax  = KIT_MAX( nPrimeSizeMax, nPrimeSizeCur );
            Kit_DsdNtkFree( ppNtks[i] );
            nSuppSizeMax += Kit_TruthSupportSize( ppCofs[nSteps][i], nVars );
        }
        printf( "Max = %2d. Supps = %2d.\n", nPrimeSizeMax, nSuppSizeMax );
    }

    if ( nCofLevel == 3 )
    for ( v1 = 0; v1 < nVars; v1++ )
    for ( v2 = v1+1; v2 < nVars; v2++ )
    for ( v3 = v2+1; v3 < nVars; v3++ )
    {
        nSteps = 0;
        piCofVar[nSteps++] = v1;
        piCofVar[nSteps++] = v2;
        piCofVar[nSteps++] = v3;

        printf( "    Variables { " );
        for ( i = 0; i < nSteps; i++ )
            printf( "%c ", 'a' + piCofVar[i] );
        printf( "}\n" );

        // single cofactors
        for ( s = 1; s <= nSteps; s++ )
        {
            for ( k = 0; k < s; k++ )
            {
                nSize = (1 << k);
                for ( i = 0; i < nSize; i++ )
                {
                    Kit_TruthCofactor0New( ppCofs[k+1][2*i+0], ppCofs[k][i], nVars, piCofVar[k] );
                    Kit_TruthCofactor1New( ppCofs[k+1][2*i+1], ppCofs[k][i], nVars, piCofVar[k] );
                }
            }
        }
        // compute DSD networks
        nSize = (1 << nSteps);
        nPrimeSizeMax = 0;
        nSuppSizeMax = 0;
        for ( i = 0; i < nSize; i++ )
        {
            ppNtks[i] = Kit_DsdDecompose( ppCofs[nSteps][i], nVars );
            ppNtks[i] = Kit_DsdExpand( pTemp = ppNtks[i] );
            Kit_DsdNtkFree( pTemp );
            if ( fVerbose )
            {
                printf( "Cof%d%d: ", nSteps, i );
                Kit_DsdPrint( stdout, ppNtks[i] ), printf( "\n" );
            }
            // compute the largest non-decomp block
            nPrimeSizeCur  = Kit_DsdNonDsdSizeMax(ppNtks[i]);
            nPrimeSizeMax  = KIT_MAX( nPrimeSizeMax, nPrimeSizeCur );
            Kit_DsdNtkFree( ppNtks[i] );
            nSuppSizeMax += Kit_TruthSupportSize( ppCofs[nSteps][i], nVars );
        }
        printf( "Max = %2d. Supps = %2d.\n", nPrimeSizeMax, nSuppSizeMax );
    }

    if ( nCofLevel == 4 )
    for ( v1 = 0; v1 < nVars; v1++ )
    for ( v2 = v1+1; v2 < nVars; v2++ )
    for ( v3 = v2+1; v3 < nVars; v3++ )
    for ( v4 = v3+1; v4 < nVars; v4++ )
    {
        nSteps = 0;
        piCofVar[nSteps++] = v1;
        piCofVar[nSteps++] = v2;
        piCofVar[nSteps++] = v3;
        piCofVar[nSteps++] = v4;

        printf( "    Variables { " );
        for ( i = 0; i < nSteps; i++ )
            printf( "%c ", 'a' + piCofVar[i] );
        printf( "}\n" );

        // single cofactors
        for ( s = 1; s <= nSteps; s++ )
        {
            for ( k = 0; k < s; k++ )
            {
                nSize = (1 << k);
                for ( i = 0; i < nSize; i++ )
                {
                    Kit_TruthCofactor0New( ppCofs[k+1][2*i+0], ppCofs[k][i], nVars, piCofVar[k] );
                    Kit_TruthCofactor1New( ppCofs[k+1][2*i+1], ppCofs[k][i], nVars, piCofVar[k] );
                }
            }
        }
        // compute DSD networks
        nSize = (1 << nSteps);
        nPrimeSizeMax = 0;
        nSuppSizeMax = 0;
        for ( i = 0; i < nSize; i++ )
        {
            ppNtks[i] = Kit_DsdDecompose( ppCofs[nSteps][i], nVars );
            ppNtks[i] = Kit_DsdExpand( pTemp = ppNtks[i] );
            Kit_DsdNtkFree( pTemp );
            if ( fVerbose )
            {
                printf( "Cof%d%d: ", nSteps, i );
                Kit_DsdPrint( stdout, ppNtks[i] ), printf( "\n" );
            }
            // compute the largest non-decomp block
            nPrimeSizeCur  = Kit_DsdNonDsdSizeMax(ppNtks[i]);
            nPrimeSizeMax  = KIT_MAX( nPrimeSizeMax, nPrimeSizeCur );
            Kit_DsdNtkFree( ppNtks[i] );
            nSuppSizeMax += Kit_TruthSupportSize( ppCofs[nSteps][i], nVars );
        }
        printf( "Max = %2d. Supps = %2d.\n", nPrimeSizeMax, nSuppSizeMax );
    }


    ABC_FREE( ppCofs[0][0] );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char ** Kit_DsdNpn4ClassNames()
{
    static const char * pNames[222] = {
        "F = 0",                     /*   0 */ 
        "F = (!d*(!c*(!b*!a)))",     /*   1 */
        "F = (!d*(!c*!b))",          /*   2 */
        "F = (!d*(!c*(b+a)))",       /*   3 */
        "F = (!d*(!c*!(b*a)))",      /*   4 */
        "F = (!d*!c)",               /*   5 */
        "F = (!d*16(a,b,c))",        /*   6 */
        "F = (!d*17(a,b,c))",        /*   7 */
        "F = (!d*18(a,b,c))",        /*   8 */
        "F = (!d*19(a,b,c))",        /*   9 */
        "F = (!d*CA(!b,!c,a))",      /*  10 */
        "F = (!d*(c+!(!b*!a)))",     /*  11 */
        "F = (!d*!(c*!(!b*!a)))",    /*  12 */
        "F = (!d*(c+b))",            /*  13 */
        "F = (!d*3D(a,b,c))",        /*  14 */
        "F = (!d*!(c*b))",           /*  15 */
        "F = (!d*(c+(b+!a)))",       /*  16 */
        "F = (!d*6B(a,b,c))",        /*  17 */
        "F = (!d*!(c*!(b+a)))",      /*  18 */
        "F = (!d*7E(a,b,c))",        /*  19 */
        "F = (!d*!(c*(b*a)))",       /*  20 */
        "F = (!d)",                  /*  21 */
        "F = 0116(a,b,c,d)",         /*  22 */
        "F = 0117(a,b,c,d)",         /*  23 */
        "F = 0118(a,b,c,d)",         /*  24 */
        "F = 0119(a,b,c,d)",         /*  25 */
        "F = 011A(a,b,c,d)",         /*  26 */
        "F = 011B(a,b,c,d)",         /*  27 */
        "F = 29((!b*!a),c,d)",       /*  28 */
        "F = 2B((!b*!a),c,d)",       /*  29 */
        "F = 012C(a,b,c,d)",         /*  30 */
        "F = 012D(a,b,c,d)",         /*  31 */
        "F = 012F(a,b,c,d)",         /*  32 */
        "F = 013C(a,b,c,d)",         /*  33 */
        "F = 013D(a,b,c,d)",         /*  34 */
        "F = 013E(a,b,c,d)",         /*  35 */
        "F = 013F(a,b,c,d)",         /*  36 */
        "F = 0168(a,b,c,d)",         /*  37 */
        "F = 0169(a,b,c,d)",         /*  38 */
        "F = 016A(a,b,c,d)",         /*  39 */
        "F = 016B(a,b,c,d)",         /*  40 */
        "F = 016E(a,b,c,d)",         /*  41 */
        "F = 016F(a,b,c,d)",         /*  42 */
        "F = 017E(a,b,c,d)",         /*  43 */
        "F = 017F(a,b,c,d)",         /*  44 */
        "F = 0180(a,b,c,d)",         /*  45 */
        "F = 0181(a,b,c,d)",         /*  46 */
        "F = 0182(a,b,c,d)",         /*  47 */
        "F = 0183(a,b,c,d)",         /*  48 */
        "F = 0186(a,b,c,d)",         /*  49 */
        "F = 0187(a,b,c,d)",         /*  50 */
        "F = 0189(a,b,c,d)",         /*  51 */
        "F = 018B(a,b,c,d)",         /*  52 */
        "F = 018F(a,b,c,d)",         /*  53 */
        "F = 0196(a,b,c,d)",         /*  54 */
        "F = 0197(a,b,c,d)",         /*  55 */
        "F = 0198(a,b,c,d)",         /*  56 */
        "F = 0199(a,b,c,d)",         /*  57 */
        "F = 019A(a,b,c,d)",         /*  58 */
        "F = 019B(a,b,c,d)",         /*  59 */
        "F = 019E(a,b,c,d)",         /*  60 */
        "F = 019F(a,b,c,d)",         /*  61 */
        "F = 42(a,(!c*!b),d)",       /*  62 */
        "F = 46(a,(!c*!b),d)",       /*  63 */
        "F = 4A(a,(!c*!b),d)",       /*  64 */
        "F = CA((!c*!b),!d,a)",      /*  65 */
        "F = 01AC(a,b,c,d)",         /*  66 */
        "F = 01AD(a,b,c,d)",         /*  67 */
        "F = 01AE(a,b,c,d)",         /*  68 */
        "F = 01AF(a,b,c,d)",         /*  69 */
        "F = 01BC(a,b,c,d)",         /*  70 */
        "F = 01BD(a,b,c,d)",         /*  71 */
        "F = 01BE(a,b,c,d)",         /*  72 */
        "F = 01BF(a,b,c,d)",         /*  73 */
        "F = 01E8(a,b,c,d)",         /*  74 */
        "F = 01E9(a,b,c,d)",         /*  75 */
        "F = 01EA(a,b,c,d)",         /*  76 */
        "F = 01EB(a,b,c,d)",         /*  77 */
        "F = 25((!b*!a),c,d)",       /*  78 */
        "F = !CA(d,c,(!b*!a))",      /*  79 */
        "F = (d+!(!c*(!b*!a)))",     /*  80 */
        "F = 16(b,c,d)",             /*  81 */
        "F = 033D(a,b,c,d)",         /*  82 */
        "F = 17(b,c,d)",             /*  83 */
        "F = ((!d*!a)+(!c*!b))",     /*  84 */
        "F = !(!(!c*!b)*!(!d*!a))",  /*  85 */
        "F = 0358(a,b,c,d)",         /*  86 */
        "F = 0359(a,b,c,d)",         /*  87 */
        "F = 035A(a,b,c,d)",         /*  88 */
        "F = 035B(a,b,c,d)",         /*  89 */
        "F = 035E(a,b,c,d)",         /*  90 */
        "F = 035F(a,b,c,d)",         /*  91 */
        "F = 0368(a,b,c,d)",         /*  92 */
        "F = 0369(a,b,c,d)",         /*  93 */
        "F = 036A(a,b,c,d)",         /*  94 */
        "F = 036B(a,b,c,d)",         /*  95 */
        "F = 036C(a,b,c,d)",         /*  96 */
        "F = 036D(a,b,c,d)",         /*  97 */
        "F = 036E(a,b,c,d)",         /*  98 */
        "F = 036F(a,b,c,d)",         /*  99 */
        "F = 037C(a,b,c,d)",         /* 100 */
        "F = 037D(a,b,c,d)",         /* 101 */
        "F = 037E(a,b,c,d)",         /* 102 */
        "F = 18(b,c,d)",             /* 103 */
        "F = 03C1(a,b,c,d)",         /* 104 */
        "F = 19(b,c,d)",             /* 105 */
        "F = 03C5(a,b,c,d)",         /* 106 */
        "F = 03C6(a,b,c,d)",         /* 107 */
        "F = 03C7(a,b,c,d)",         /* 108 */
        "F = CA(!c,!d,b)",           /* 109 */
        "F = 03D4(a,b,c,d)",         /* 110 */
        "F = 03D5(a,b,c,d)",         /* 111 */
        "F = 03D6(a,b,c,d)",         /* 112 */
        "F = 03D7(a,b,c,d)",         /* 113 */
        "F = 03D8(a,b,c,d)",         /* 114 */
        "F = 03D9(a,b,c,d)",         /* 115 */
        "F = 03DB(a,b,c,d)",         /* 116 */
        "F = 03DC(a,b,c,d)",         /* 117 */
        "F = 03DD(a,b,c,d)",         /* 118 */
        "F = 03DE(a,b,c,d)",         /* 119 */
        "F = (d+!(!c*!b))",          /* 120 */
        "F = ((d+c)*(b+a))",         /* 121 */
        "F = 0661(a,b,c,d)",         /* 122 */
        "F = 0662(a,b,c,d)",         /* 123 */
        "F = 0663(a,b,c,d)",         /* 124 */
        "F = (!(d*c)*(b+a))",        /* 125 */
        "F = 0667(a,b,c,d)",         /* 126 */
        "F = 29((b+a),c,d)",         /* 127 */
        "F = 066B(a,b,c,d)",         /* 128 */
        "F = 2B((b+a),c,d)",         /* 129 */
        "F = 0672(a,b,c,d)",         /* 130 */
        "F = 0673(a,b,c,d)",         /* 131 */
        "F = 0676(a,b,c,d)",         /* 132 */
        "F = 0678(a,b,c,d)",         /* 133 */
        "F = 0679(a,b,c,d)",         /* 134 */
        "F = 067A(a,b,c,d)",         /* 135 */
        "F = 067B(a,b,c,d)",         /* 136 */
        "F = 067E(a,b,c,d)",         /* 137 */
        "F = 24((b+a),c,d)",         /* 138 */
        "F = 0691(a,b,c,d)",         /* 139 */
        "F = 0693(a,b,c,d)",         /* 140 */
        "F = 26((b+a),c,d)",         /* 141 */
        "F = 0697(a,b,c,d)",         /* 142 */
        "F = !CA(d,c,(b+a))",        /* 143 */
        "F = 06B0(a,b,c,d)",         /* 144 */
        "F = 06B1(a,b,c,d)",         /* 145 */
        "F = 06B2(a,b,c,d)",         /* 146 */
        "F = 06B3(a,b,c,d)",         /* 147 */
        "F = 06B4(a,b,c,d)",         /* 148 */
        "F = 06B5(a,b,c,d)",         /* 149 */
        "F = 06B6(a,b,c,d)",         /* 150 */
        "F = 06B7(a,b,c,d)",         /* 151 */
        "F = 06B9(a,b,c,d)",         /* 152 */
        "F = 06BD(a,b,c,d)",         /* 153 */
        "F = 2C((b+a),c,d)",         /* 154 */
        "F = 06F1(a,b,c,d)",         /* 155 */
        "F = 06F2(a,b,c,d)",         /* 156 */
        "F = CA((b+a),!d,c)",        /* 157 */
        "F = (d+!(!c*!(b+!a)))",     /* 158 */
        "F = 0776(a,b,c,d)",         /* 159 */
        "F = 16((b*a),c,d)",         /* 160 */
        "F = 0779(a,b,c,d)",         /* 161 */
        "F = 077A(a,b,c,d)",         /* 162 */
        "F = 077E(a,b,c,d)",         /* 163 */
        "F = 07B0(a,b,c,d)",         /* 164 */
        "F = 07B1(a,b,c,d)",         /* 165 */
        "F = 07B4(a,b,c,d)",         /* 166 */
        "F = 07B5(a,b,c,d)",         /* 167 */
        "F = 07B6(a,b,c,d)",         /* 168 */
        "F = 07BC(a,b,c,d)",         /* 169 */
        "F = 07E0(a,b,c,d)",         /* 170 */
        "F = 07E1(a,b,c,d)",         /* 171 */
        "F = 07E2(a,b,c,d)",         /* 172 */
        "F = 07E3(a,b,c,d)",         /* 173 */
        "F = 07E6(a,b,c,d)",         /* 174 */
        "F = 07E9(a,b,c,d)",         /* 175 */
        "F = 1C((b*a),c,d)",         /* 176 */
        "F = 07F1(a,b,c,d)",         /* 177 */
        "F = 07F2(a,b,c,d)",         /* 178 */
        "F = (d+!(!c*!(b*a)))",      /* 179 */
        "F = (d+c)",                 /* 180 */
        "F = 1668(a,b,c,d)",         /* 181 */
        "F = 1669(a,b,c,d)",         /* 182 */
        "F = 166A(a,b,c,d)",         /* 183 */
        "F = 166B(a,b,c,d)",         /* 184 */
        "F = 166E(a,b,c,d)",         /* 185 */
        "F = 167E(a,b,c,d)",         /* 186 */
        "F = 1681(a,b,c,d)",         /* 187 */
        "F = 1683(a,b,c,d)",         /* 188 */
        "F = 1686(a,b,c,d)",         /* 189 */
        "F = 1687(a,b,c,d)",         /* 190 */
        "F = 1689(a,b,c,d)",         /* 191 */
        "F = 168B(a,b,c,d)",         /* 192 */
        "F = 168E(a,b,c,d)",         /* 193 */
        "F = 1696(a,b,c,d)",         /* 194 */
        "F = 1697(a,b,c,d)",         /* 195 */
        "F = 1698(a,b,c,d)",         /* 196 */
        "F = 1699(a,b,c,d)",         /* 197 */
        "F = 169A(a,b,c,d)",         /* 198 */
        "F = 169B(a,b,c,d)",         /* 199 */
        "F = 169E(a,b,c,d)",         /* 200 */
        "F = 16A9(a,b,c,d)",         /* 201 */
        "F = 16AC(a,b,c,d)",         /* 202 */
        "F = 16AD(a,b,c,d)",         /* 203 */
        "F = 16BC(a,b,c,d)",         /* 204 */
        "F = (d+E9(a,b,c))",         /* 205 */
        "F = 177E(a,b,c,d)",         /* 206 */
        "F = 178E(a,b,c,d)",         /* 207 */
        "F = 1796(a,b,c,d)",         /* 208 */
        "F = 1798(a,b,c,d)",         /* 209 */
        "F = 179A(a,b,c,d)",         /* 210 */
        "F = 17AC(a,b,c,d)",         /* 211 */
        "F = (d+E8(a,b,c))",         /* 212 */
        "F = (d+E7(a,b,c))",         /* 213 */
        "F = 19E1(a,b,c,d)",         /* 214 */
        "F = 19E3(a,b,c,d)",         /* 215 */
        "F = (d+E6(a,b,c))",         /* 216 */
        "F = 1BD8(a,b,c,d)",         /* 217 */
        "F = (d+CA(b,c,a))",         /* 218 */
        "F = (d+(c+(!b*!a)))",       /* 219 */
        "F = (d+(c+!b))",            /* 220 */
        "F = (d+(c+(b+a)))"          /* 221 */
    };
    return (char **)pNames;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

