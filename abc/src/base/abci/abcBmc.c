/**CFile****************************************************************

  FileName    [abcBmc.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Performs bounded model check.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcBmc.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "aig/ivy/ivy.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern Ivy_Man_t * Abc_NtkIvyBefore( Abc_Ntk_t * pNtk, int fSeq, int fUseDc );

static void Abc_NtkBmcReport( Ivy_Man_t * pMan, Ivy_Man_t * pFrames, Ivy_Man_t * pFraig, Vec_Ptr_t * vMapping, int nFrames );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkBmc( Abc_Ntk_t * pNtk, int nFrames, int fInit, int fVerbose )
{
    Ivy_FraigParams_t Params, * pParams = &Params; 
    Ivy_Man_t * pMan, * pFrames, * pFraig;
    Vec_Ptr_t * vMapping;
    // convert to IVY manager
    pMan = Abc_NtkIvyBefore( pNtk, 0, 0 );
    // generate timeframes
    pFrames = Ivy_ManFrames( pMan, Abc_NtkLatchNum(pNtk), nFrames, fInit, &vMapping );
    // fraig the timeframes
    Ivy_FraigParamsDefault( pParams );
    pParams->nBTLimitNode = ABC_INFINITY;
    pParams->fVerbose = 0;
    pParams->fProve = 0;
    pFraig = Ivy_FraigPerform( pFrames, pParams );
printf( "Frames have %6d nodes.  ", Ivy_ManNodeNum(pFrames) );
printf( "Fraig has %6d nodes.\n", Ivy_ManNodeNum(pFraig) );
    // report the classes
//    if ( fVerbose )
//        Abc_NtkBmcReport( pMan, pFrames, pFraig, vMapping, nFrames );
    // free stuff
    Vec_PtrFree( vMapping );
    Ivy_ManStop( pFraig );
    Ivy_ManStop( pFrames );
    Ivy_ManStop( pMan );
}
 
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkBmcReport( Ivy_Man_t * pMan, Ivy_Man_t * pFrames, Ivy_Man_t * pFraig, Vec_Ptr_t * vMapping, int nFrames )
{
    Ivy_Obj_t * pFirst1, * pFirst2 = NULL, * pFirst3 = NULL;
    int i, f, nIdMax, Prev2, Prev3;
    nIdMax = Ivy_ManObjIdMax(pMan);
    // check what is the number of nodes in each frame
    Prev2 = Prev3 = 0;
    for ( f = 0; f < nFrames; f++ )
    {
        Ivy_ManForEachNode( pMan, pFirst1, i )
        {
            pFirst2 = Ivy_Regular( (Ivy_Obj_t *)Vec_PtrEntry(vMapping, f * nIdMax + pFirst1->Id) );
            if ( Ivy_ObjIsConst1(pFirst2) || pFirst2->Type == 0 )
                continue;
            pFirst3 = Ivy_Regular( pFirst2->pEquiv );
            if ( Ivy_ObjIsConst1(pFirst3) || pFirst3->Type == 0 )
                continue;
            break;
        }
        assert(pFirst2);
        assert(pFirst3);
        if ( f )
            printf( "Frame %3d :  Strash = %5d  Fraig = %5d\n", f, pFirst2->Id - Prev2, pFirst3->Id - Prev3 );
        Prev2 = pFirst2->Id;
        Prev3 = pFirst3->Id;   
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

