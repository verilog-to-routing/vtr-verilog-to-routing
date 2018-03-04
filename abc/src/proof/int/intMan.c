/**CFile****************************************************************

  FileName    [intMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Interpolation engine.]

  Synopsis    [Interpolation manager procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 24, 2008.]

  Revision    [$Id: intMan.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "intInt.h"
#include "aig/ioa/ioa.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates the interpolation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Inter_Man_t * Inter_ManCreate( Aig_Man_t * pAig, Inter_ManParams_t * pPars )
{
    Inter_Man_t * p;
    // create interpolation manager
    p = ABC_ALLOC( Inter_Man_t, 1 );
    memset( p, 0, sizeof(Inter_Man_t) );
    p->vVarsAB = Vec_IntAlloc( Aig_ManRegNum(pAig) );
    p->nConfLimit = pPars->nBTLimit;
    p->fVerbose = pPars->fVerbose;
    p->pFileName = pPars->pFileName;
    p->pAig = pAig;
    if ( pPars->fDropInvar )
        p->vInters = Vec_PtrAlloc( 100 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Cleans the interpolation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Inter_ManClean( Inter_Man_t * p )
{
    if ( p->vInters )
    {
        Aig_Man_t * pMan;
        int i;
        Vec_PtrForEachEntry( Aig_Man_t *, p->vInters, pMan, i )
            Aig_ManStop( pMan );
        Vec_PtrClear( p->vInters );
    }
    if ( p->pCnfInter )
        Cnf_DataFree( p->pCnfInter );
    if ( p->pCnfFrames )
        Cnf_DataFree( p->pCnfFrames );
    if ( p->pInter )
        Aig_ManStop( p->pInter );
    if ( p->pFrames )
        Aig_ManStop( p->pFrames );
}

/**Function*************************************************************

  Synopsis    [Writes interpolant into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Inter_ManInterDump( Inter_Man_t * p, int fProved )
{
    char * pFileName = p->pFileName ? p->pFileName : (char *)"invar.aig";
    Aig_Man_t * pMan;
    pMan = Aig_ManDupArray( p->vInters );
    Ioa_WriteAiger( pMan, pFileName, 0, 0 );
    Aig_ManStop( pMan );
    if ( fProved )
        printf( "Inductive invariant is dumped into file \"%s\".\n", pFileName );
    else
        printf( "Interpolants are dumped into file \"%s\".\n", pFileName );
}

/**Function*************************************************************

  Synopsis    [Frees the interpolation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Inter_ManStop( Inter_Man_t * p, int fProved )
{
    if ( p->fVerbose )
    {
        p->timeOther = p->timeTotal-p->timeRwr-p->timeCnf-p->timeSat-p->timeInt-p->timeEqu;
        printf( "Runtime statistics:\n" );
        ABC_PRTP( "Rewriting  ", p->timeRwr,   p->timeTotal );
        ABC_PRTP( "CNF mapping", p->timeCnf,   p->timeTotal );
        ABC_PRTP( "SAT solving", p->timeSat,   p->timeTotal );
        ABC_PRTP( "Interpol   ", p->timeInt,   p->timeTotal );
        ABC_PRTP( "Containment", p->timeEqu,   p->timeTotal );
        ABC_PRTP( "Other      ", p->timeOther, p->timeTotal );
        ABC_PRTP( "TOTAL      ", p->timeTotal, p->timeTotal );
    }

    if ( p->vInters )
        Inter_ManInterDump( p, fProved );

    if ( p->pCnfAig )
        Cnf_DataFree( p->pCnfAig );
    if ( p->pAigTrans )
        Aig_ManStop( p->pAigTrans );
    if ( p->pInterNew )
        Aig_ManStop( p->pInterNew );
    Inter_ManClean( p );
    Vec_PtrFreeP( &p->vInters );
    Vec_IntFreeP( &p->vVarsAB );
    ABC_FREE( p );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

