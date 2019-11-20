/**CFile****************************************************************

  FileName    [bdcCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Truth-table-based bi-decomposition engine.]

  Synopsis    [The gateway to bi-decomposition.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 30, 2007.]

  Revision    [$Id: bdcCore.c,v 1.00 2007/01/30 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bdcInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocate resynthesis manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Bdc_Man_t * Bdc_ManAlloc( Bdc_Par_t * pPars )
{
    Bdc_Man_t * p;
    unsigned * pData;
    int i, k, nBits;
    p = ALLOC( Bdc_Man_t, 1 );
    memset( p, 0, sizeof(Bdc_Man_t) );
    assert( pPars->nVarsMax > 3 && pPars->nVarsMax < 16 );
    p->pPars = pPars;
    p->nWords = Kit_TruthWordNum( pPars->nVarsMax );
    p->nDivsLimit = 200;
    p->nNodesLimit = 0; // will be set later
    // memory
    p->vMemory = Vec_IntStart( 1 << 16 );
    // internal nodes
    p->nNodesAlloc = 512;
    p->pNodes = ALLOC( Bdc_Fun_t, p->nNodesAlloc );
    // set up hash table
    p->nTableSize = (1 << p->pPars->nVarsMax);
    p->pTable = ALLOC( Bdc_Fun_t *, p->nTableSize );
    memset( p->pTable, 0, sizeof(Bdc_Fun_t *) * p->nTableSize );
    p->vSpots = Vec_IntAlloc( 256 );
    // truth tables
    p->vTruths = Vec_PtrAllocSimInfo( pPars->nVarsMax + 5, p->nWords );
    // set elementary truth tables
    nBits = (1 << pPars->nVarsMax);
    Kit_TruthFill( Vec_PtrEntry(p->vTruths, 0), p->nVars );
    for ( k = 0; k < pPars->nVarsMax; k++ )
    {
        pData = Vec_PtrEntry( p->vTruths, k+1 );
        Kit_TruthClear( pData, p->nVars );
        for ( i = 0; i < nBits; i++ )
            if ( i & (1 << k) )
                pData[i>>5] |= (1 << (i&31));
    }
    p->puTemp1 = Vec_PtrEntry( p->vTruths, pPars->nVarsMax + 1 );
    p->puTemp2 = Vec_PtrEntry( p->vTruths, pPars->nVarsMax + 2 );
    p->puTemp3 = Vec_PtrEntry( p->vTruths, pPars->nVarsMax + 3 );
    p->puTemp4 = Vec_PtrEntry( p->vTruths, pPars->nVarsMax + 4 );
    // start the internal ISFs
    p->pIsfOL = &p->IsfOL;  Bdc_IsfStart( p, p->pIsfOL );
    p->pIsfOR = &p->IsfOR;  Bdc_IsfStart( p, p->pIsfOR );
    p->pIsfAL = &p->IsfAL;  Bdc_IsfStart( p, p->pIsfAL );
    p->pIsfAR = &p->IsfAR;  Bdc_IsfStart( p, p->pIsfAR );   
    return p;
}

/**Function*************************************************************

  Synopsis    [Deallocate resynthesis manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bdc_ManFree( Bdc_Man_t * p )
{
    Vec_IntFree( p->vMemory );
    Vec_IntFree( p->vSpots );
    Vec_PtrFree( p->vTruths );
    free( p->pNodes );
    free( p->pTable );
    free( p );
}

/**Function*************************************************************

  Synopsis    [Clears the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bdc_ManPrepare( Bdc_Man_t * p, Vec_Ptr_t * vDivs )
{
    unsigned * puTruth;
    Bdc_Fun_t * pNode;
    int i;
    Bdc_TableClear( p );
    Vec_IntClear( p->vMemory );
    // add constant 1 and elementary vars
    p->nNodes = p->nNodesNew = 0;
    for ( i = 0; i <= p->pPars->nVarsMax; i++ )
    {
        pNode = Bdc_FunNew( p );
        pNode->Type = BDC_TYPE_PI;
        pNode->puFunc = Vec_PtrEntry( p->vTruths, i );
        pNode->uSupp = i? (1 << (i-1)) : 0;
        Bdc_TableAdd( p, pNode );
    }
    // add the divisors
    Vec_PtrForEachEntry( vDivs, puTruth, i )
    {
        pNode = Bdc_FunNew( p );
        pNode->Type = BDC_TYPE_PI;
        pNode->puFunc = puTruth;
        pNode->uSupp = Kit_TruthSupport( puTruth, p->nVars );
        Bdc_TableAdd( p, pNode );
        if ( i == p->nDivsLimit )
            break;
    }
}

/**Function*************************************************************

  Synopsis    [Performs decomposition of one function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bdc_ManDecompose( Bdc_Man_t * p, unsigned * puFunc, unsigned * puCare, int nVars, Vec_Ptr_t * vDivs, int nNodesMax )
{
    Bdc_Isf_t Isf, * pIsf = &Isf;
    // set current manager parameters
    p->nVars = nVars;
    p->nWords = Kit_TruthWordNum( nVars );
    Bdc_ManPrepare( p, vDivs );
    p->nNodesLimit = (p->nNodes + nNodesMax < p->nNodesAlloc)? p->nNodes + nNodesMax : p->nNodesAlloc;
    // copy the function
    Bdc_IsfStart( p, pIsf );
    Bdc_IsfClean( pIsf );
    pIsf->uSupp = Kit_TruthSupport( puFunc, p->nVars ) | Kit_TruthSupport( puCare, p->nVars );
    Kit_TruthAnd( pIsf->puOn, puCare, puFunc, p->nVars );
    Kit_TruthSharp( pIsf->puOff, puCare, puFunc, p->nVars );
    // call decomposition
    Bdc_SuppMinimize( p, pIsf );
    p->pRoot = Bdc_ManDecompose_rec( p, pIsf );
    if ( p->pRoot == NULL )
        return -1;
    return p->nNodesNew;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


