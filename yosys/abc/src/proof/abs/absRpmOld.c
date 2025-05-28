/**CFile****************************************************************

  FileName    [absRpmOld.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Old min-cut-based reparametrization.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: absRpmOld.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abs.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Specialized duplication.]

  Description [Replaces registers by PIs/POs and PIs by registers.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupIn2Ff( Gia_Man_t * p )
{
    Vec_Int_t * vPiOuts;
    Gia_Man_t * pNew; 
    Gia_Obj_t * pObj;
    int i;
    vPiOuts = Vec_IntAlloc( Gia_ManPiNum(p) );
    pNew = Gia_ManStart( Gia_ManObjNum(p) + 2 * Gia_ManPiNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManFillValue( p );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachPi( p, pObj, i )
        Vec_IntPush( vPiOuts, Gia_ManAppendCi(pNew) );
    Gia_ManForEachRo( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachPi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachPo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManForEachRi( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManForEachPi( p, pObj, i )
        Gia_ManAppendCo( pNew, Vec_IntEntry(vPiOuts, i) );
    Gia_ManSetRegNum( pNew, Gia_ManPiNum(p) );
    Vec_IntFree( vPiOuts );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Reverses the above step.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManDupFf2In_rec( Gia_Man_t * pNew, Gia_Obj_t * pObj )
{
    if ( pObj->Value != ~0 )
        return pObj->Value;
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManDupFf2In_rec( pNew, Gia_ObjFanin0(pObj) );
    Gia_ManDupFf2In_rec( pNew, Gia_ObjFanin1(pObj) );
    return pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
}

/**Function*************************************************************

  Synopsis    [Reverses the above step.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupFf2In( Gia_Man_t * p, int nFlopsOld )
{
    Gia_Man_t * pNew; 
    Gia_Obj_t * pObj;
    int i;
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManFillValue( p );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachRo( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    for ( i = Gia_ManPiNum(p) - nFlopsOld; i < Gia_ManPiNum(p); i++ )
        Gia_ManPi(p, i)->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachPo( p, pObj, i )
        Gia_ManDupFf2In_rec( pNew, Gia_ObjFanin0(pObj) );
    Gia_ManForEachPo( p, pObj, i )
        Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManSetRegNum( pNew, nFlopsOld );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Reparameterized to get rid of useless primary inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Abs_RpmPerformOld( Gia_Man_t * p, int fVerbose )
{
//    extern Aig_Man_t * Saig_ManRetimeMinArea( Aig_Man_t * p, int nMaxIters, int fForwardOnly, int fBackwardOnly, int fInitial, int fVerbose );
    Aig_Man_t * pMan, * pTemp;
    Gia_Man_t * pNew, * pTmp;
    int nFlopsOld = Gia_ManRegNum(p);
    if ( fVerbose )
    {
        printf( "Original AIG:\n" );
        Gia_ManPrintStats( p, NULL );
    }

    // perform input trimming
    pNew = Gia_ManDupTrimmed( p, 1, 0, 0, -1 );
    if ( fVerbose )
    {
        printf( "After PI trimming:\n" );
        Gia_ManPrintStats( pNew, NULL );
    }
    // transform GIA
    pNew = Gia_ManDupIn2Ff( pTmp = pNew );
    Gia_ManStop( pTmp );
    if ( fVerbose )
    {
        printf( "After PI-2-FF transformation:\n" );
        Gia_ManPrintStats( pNew, NULL );
    }

    // derive AIG
    pMan = Gia_ManToAigSimple( pNew );
    Gia_ManStop( pNew );
    // perform min-reg retiming
    pMan = Saig_ManRetimeMinArea( pTemp = pMan, 10, 0, 0, 1, 0 );
    Aig_ManStop( pTemp );
    // derive GIA
    pNew = Gia_ManFromAigSimple( pMan );
    Aig_ManStop( pMan );
    if ( fVerbose )
    {
        printf( "After min-area retiming:\n" );
        Gia_ManPrintStats( pNew, NULL );
    }

    // transform back
    pNew = Gia_ManDupFf2In( pTmp = pNew, nFlopsOld );
    Gia_ManStop( pTmp );
    if ( fVerbose )
    {
        printf( "After FF-2-PI transformation:\n" );
        Gia_ManPrintStats( pNew, NULL );
    }
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

