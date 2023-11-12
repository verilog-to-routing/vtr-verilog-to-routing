/**CFile****************************************************************

  FileName    [giaTim.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Procedures with hierarchy/timing manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaTim.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "giaAig.h"
#include "misc/tim/tim.h"
#include "misc/extra/extra.h"
#include "proof/cec/cec.h"
#include "proof/fra/fra.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns the number of boxes in the AIG with boxes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManBoxNum( Gia_Man_t * p )
{
    return p->pManTime ? Tim_ManBoxNum((Tim_Man_t *)p->pManTime) : 0;
}
int Gia_ManRegBoxNum( Gia_Man_t * p )
{
    return p->vRegClasses ? Vec_IntSize(p->vRegClasses) : 0;
}
int Gia_ManNonRegBoxNum( Gia_Man_t * p )
{
    return Gia_ManBoxNum(p) - Gia_ManRegBoxNum(p);
}
int Gia_ManBlackBoxNum( Gia_Man_t * p )
{
    return Tim_ManBlackBoxNum((Tim_Man_t *)p->pManTime);
}
int Gia_ManBoxCiNum( Gia_Man_t * p )
{
    return p->pManTime ? Gia_ManCiNum(p) - Tim_ManPiNum((Tim_Man_t *)p->pManTime) : 0;
}
int Gia_ManBoxCoNum( Gia_Man_t * p )
{
    return p->pManTime ? Gia_ManCoNum(p) - Tim_ManPoNum((Tim_Man_t *)p->pManTime) : 0;
}
int Gia_ManClockDomainNum( Gia_Man_t * p )
{
    int i, nDoms, Count = 0;
    if ( p->vRegClasses == NULL )
        return 0;
    nDoms = Vec_IntFindMax(p->vRegClasses);
    assert( Vec_IntCountEntry(p->vRegClasses, 0) == 0 );
    for ( i = 1; i <= nDoms; i++ )
        if ( Vec_IntCountEntry(p->vRegClasses, i) > 0 )
            Count++;
    return Count;    
}

/**Function*************************************************************

  Synopsis    [Returns one if this is a seq AIG with non-trivial boxes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManIsSeqWithBoxes( Gia_Man_t * p )
{
    return (Gia_ManRegNum(p) > 0 && Gia_ManBoxNum(p) > 0);
}

/**Function*************************************************************

  Synopsis    [Makes sure the manager is normalized.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManIsNormalized( Gia_Man_t * p )  
{
    int i, nOffset;
    nOffset = 1;
    for ( i = 0; i < Gia_ManCiNum(p); i++ )
        if ( !Gia_ObjIsCi( Gia_ManObj(p, nOffset+i) ) )
            return 0;
    nOffset = 1 + Gia_ManCiNum(p) + Gia_ManAndNum(p);
    for ( i = 0; i < Gia_ManCoNum(p); i++ )
        if ( !Gia_ObjIsCo( Gia_ManObj(p, nOffset+i) ) )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Duplicates AIG in the DFS order while putting CIs first.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupNormalize( Gia_Man_t * p, int fHashMapping )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    Gia_ManFillValue( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    if ( !Gia_ManIsSeqWithBoxes(p) )
    {
        Gia_ManForEachCi( p, pObj, i )
            pObj->Value = Gia_ManAppendCi(pNew);
    }
    else
    {
        // current CI order:  PIs + FOs + NewCIs
        // desired reorder:   PIs + NewCIs + FOs
        int nCIs = Tim_ManPiNum( (Tim_Man_t *)p->pManTime );
        int nAll = Tim_ManCiNum( (Tim_Man_t *)p->pManTime );
        int nPis = nCIs - Gia_ManRegNum(p);
        assert( nAll == Gia_ManCiNum(p) );
        assert( nPis > 0 );
        // copy PIs first
        for ( i = 0; i < nPis; i++ )
            Gia_ManCi(p, i)->Value = Gia_ManAppendCi(pNew);
       // copy new CIs second
        for ( i = nCIs; i < nAll; i++ )
            Gia_ManCi(p, i)->Value = Gia_ManAppendCi(pNew);
        // copy flops last
        for ( i = nCIs - Gia_ManRegNum(p); i < nCIs; i++ )
            Gia_ManCi(p, i)->Value = Gia_ManAppendCi(pNew);
        printf( "Warning: Shuffled CI order to be correct sequential AIG.\n" );
    }
    if ( fHashMapping ) Gia_ManHashAlloc( pNew );
    Gia_ManForEachAnd( p, pObj, i )
        if ( Gia_ObjIsBuf(pObj) )
            pObj->Value = Gia_ManAppendBuf( pNew, Gia_ObjFanin0Copy(pObj) );
        else if ( fHashMapping )
            pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else
            pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    if ( fHashMapping ) Gia_ManHashStop( pNew );
    Gia_ManForEachCo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    pNew->nConstrs = p->nConstrs;
    assert( Gia_ManIsNormalized(pNew) );
    Gia_ManDupRemapEquiv( pNew, p );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Reorders flops for sequential AIGs with boxes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupUnshuffleInputs( Gia_Man_t * p )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i, nCIs, nAll, nPis;
    // sanity checks
    assert( Gia_ManIsNormalized(p) );
    assert( Gia_ManIsSeqWithBoxes(p) );
    Gia_ManFillValue( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    // change input order
    // desired reorder:   PIs + NewCIs + FOs
    // current CI order:  PIs + FOs + NewCIs
    nCIs = Tim_ManPiNum( (Tim_Man_t *)p->pManTime );
    nAll = Tim_ManCiNum( (Tim_Man_t *)p->pManTime );
    nPis = nCIs - Gia_ManRegNum(p);
    assert( nAll == Gia_ManCiNum(p) );
    assert( nPis > 0 );
    // copy PIs first
    for ( i = 0; i < nPis; i++ )
        Gia_ManCi(p, i)->Value = Gia_ManAppendCi(pNew);
    // copy flops second
    for ( i = nAll - Gia_ManRegNum(p); i < nAll; i++ )
        Gia_ManCi(p, i)->Value = Gia_ManAppendCi(pNew);
    // copy new CIs last
    for ( i = nPis; i < nAll - Gia_ManRegNum(p); i++ )
        Gia_ManCi(p, i)->Value = Gia_ManAppendCi(pNew);
    printf( "Warning: Unshuffled CI order to be correct AIG with boxes.\n" );
    // other things
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachCo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    pNew->nConstrs = p->nConstrs;
    assert( Gia_ManIsNormalized(pNew) );
    Gia_ManDupRemapEquiv( pNew, p );
    return pNew;
}


/**Function*************************************************************

  Synopsis    [Find the ordering of AIG objects.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManOrderWithBoxes_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vNodes )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return 0;
    Gia_ObjSetTravIdCurrent(p, pObj);
    if ( Gia_ObjIsCi(pObj) )
    {
        p->iData2 = Gia_ObjCioId(pObj);
        return 1;
    }
    assert( Gia_ObjIsAnd(pObj) );
    if ( Gia_ObjIsBuf(pObj) )
    {
        if ( Gia_ManOrderWithBoxes_rec( p, Gia_ObjFanin0(pObj), vNodes ) )
            return 1;
        Vec_IntPush( vNodes, Gia_ObjId(p, pObj) );
        return 0;
    }
    if ( Gia_ObjSibl(p, Gia_ObjId(p, pObj)) )
        if ( Gia_ManOrderWithBoxes_rec( p, Gia_ObjSiblObj(p, Gia_ObjId(p, pObj)), vNodes ) )
            return 1;
    if ( Gia_ManOrderWithBoxes_rec( p, Gia_ObjFanin0(pObj), vNodes ) )
        return 1;
    if ( Gia_ManOrderWithBoxes_rec( p, Gia_ObjFanin1(pObj), vNodes ) )
        return 1;
    Vec_IntPush( vNodes, Gia_ObjId(p, pObj) );
    return 0;
}
Vec_Int_t * Gia_ManOrderWithBoxes( Gia_Man_t * p )
{
    Tim_Man_t * pManTime = (Tim_Man_t *)p->pManTime;
    Vec_Int_t * vNodes;
    Gia_Obj_t * pObj;
    int i, k, curCi, curCo;
    assert( pManTime != NULL );
    assert( Gia_ManIsNormalized( p ) );
    // start trav IDs
    Gia_ManIncrementTravId( p );
    // start the array
    vNodes = Vec_IntAlloc( Gia_ManObjNum(p) );
    // include constant
    Vec_IntPush( vNodes, 0 );
    Gia_ObjSetTravIdCurrent( p, Gia_ManConst0(p) );
    // include primary inputs
    for ( i = 0; i < Tim_ManPiNum(pManTime); i++ )
    {
        pObj = Gia_ManCi( p, i );
        Vec_IntPush( vNodes, Gia_ObjId(p, pObj) );
        Gia_ObjSetTravIdCurrent( p, pObj );
        assert( Gia_ObjId(p, pObj) == i+1 );
    }
    // for each box, include box nodes
    curCi = Tim_ManPiNum(pManTime);
    curCo = 0;
    for ( i = 0; i < Tim_ManBoxNum(pManTime); i++ )
    {
        // add internal nodes
        for ( k = 0; k < Tim_ManBoxInputNum(pManTime, i); k++ )
        {
            pObj = Gia_ManCo( p, curCo + k );
            if ( Gia_ManOrderWithBoxes_rec( p, Gia_ObjFanin0(pObj), vNodes ) )
            {
                int iCiNum  = p->iData2;
                int iBoxNum = Tim_ManBoxFindFromCiNum( pManTime, iCiNum );
                printf( "The command has to terminate. Boxes are not in a topological order.\n" );
                printf( "The following information may help debugging (numbers are 0-based):\n" );
                printf( "Input %d of BoxA %d (1stCI = %d; 1stCO = %d) has TFI with CI %d,\n", 
                    k, i, Tim_ManBoxOutputFirst(pManTime, i), Tim_ManBoxInputFirst(pManTime, i), iCiNum );
                printf( "which corresponds to output %d of BoxB %d (1stCI = %d; 1stCO = %d).\n", 
                    iCiNum - Tim_ManBoxOutputFirst(pManTime, iBoxNum), iBoxNum, 
                    Tim_ManBoxOutputFirst(pManTime, iBoxNum), Tim_ManBoxInputFirst(pManTime, iBoxNum) );
                printf( "In a correct topological order, BoxB should precede BoxA.\n" );
                Vec_IntFree( vNodes );
                p->iData2 = 0;
                return NULL;
            }
        }
        // add POs corresponding to box inputs
        for ( k = 0; k < Tim_ManBoxInputNum(pManTime, i); k++ )
        {
            pObj = Gia_ManCo( p, curCo + k );
            Vec_IntPush( vNodes, Gia_ObjId(p, pObj) );
        }
        curCo += Tim_ManBoxInputNum(pManTime, i);
        // add PIs corresponding to box outputs
        for ( k = 0; k < Tim_ManBoxOutputNum(pManTime, i); k++ )
        {
            pObj = Gia_ManCi( p, curCi + k );
            Vec_IntPush( vNodes, Gia_ObjId(p, pObj) );
            Gia_ObjSetTravIdCurrent( p, pObj );
        }
        curCi += Tim_ManBoxOutputNum(pManTime, i);
    }
    // add remaining nodes
    for ( i = Tim_ManCoNum(pManTime) - Tim_ManPoNum(pManTime); i < Tim_ManCoNum(pManTime); i++ )
    {
        pObj = Gia_ManCo( p, i );
        Gia_ManOrderWithBoxes_rec( p, Gia_ObjFanin0(pObj), vNodes );
    }
    // add POs
    for ( i = Tim_ManCoNum(pManTime) - Tim_ManPoNum(pManTime); i < Tim_ManCoNum(pManTime); i++ )
    {
        pObj = Gia_ManCo( p, i );
        Vec_IntPush( vNodes, Gia_ObjId(p, pObj) );
    }
    curCo += Tim_ManPoNum(pManTime);
    // verify counts
    assert( curCi == Gia_ManCiNum(p) );
    assert( curCo == Gia_ManCoNum(p) );
    //assert( Vec_IntSize(vNodes) == Gia_ManObjNum(p) );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Duplicates AIG according to the timing manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupUnnormalize( Gia_Man_t * p )
{
    Vec_Int_t * vNodes;
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    assert( !Gia_ManBufNum(p) );
    vNodes = Gia_ManOrderWithBoxes( p );
    if ( vNodes == NULL )
        return NULL;
    Gia_ManFillValue( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    if ( Gia_ManHasChoices(p) )
        pNew->pSibls = ABC_CALLOC( int, Gia_ManObjNum(p) );
    Gia_ManForEachObjVec( vNodes, p, pObj, i )
    {
        if ( Gia_ObjIsBuf(pObj) )
            pObj->Value = Gia_ManAppendBuf( pNew, Gia_ObjFanin0Copy(pObj) );
        else if ( Gia_ObjIsAnd(pObj) )
        {
            pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
            if ( Gia_ObjSibl(p, Gia_ObjId(p, pObj)) )
                pNew->pSibls[Abc_Lit2Var(pObj->Value)] = Abc_Lit2Var(Gia_ObjSiblObj(p, Gia_ObjId(p, pObj))->Value);        
        }
        else if ( Gia_ObjIsCi(pObj) )
            pObj->Value = Gia_ManAppendCi( pNew );
        else if ( Gia_ObjIsCo(pObj) )
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
        else if ( Gia_ObjIsConst0(pObj) )
            pObj->Value = 0;
        else assert( 0 );
    }
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    Vec_IntFree( vNodes );
    return pNew;
}


/**Function*************************************************************

  Synopsis    [Remaps the AIG from the old manager into the new manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCleanupRemap( Gia_Man_t * p, Gia_Man_t * pGia )
{
    Gia_Obj_t * pObj, * pObjGia;
    int i, iPrev;
    Gia_ManForEachObj1( p, pObj, i )
    {
        iPrev = Gia_ObjValue(pObj);
        if ( iPrev == ~0 )
            continue;
        pObjGia = Gia_ManObj( pGia, Abc_Lit2Var(iPrev) );
        if ( pObjGia->Value == ~0 )
            Gia_ObjSetValue( pObj, pObjGia->Value ); 
        else
            Gia_ObjSetValue( pObj, Abc_LitNotCond(pObjGia->Value, Abc_LitIsCompl(iPrev)) );
    }
}

/**Function*************************************************************

  Synopsis    [Computes level with boxes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManLevelWithBoxes_rec( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return 0;
    Gia_ObjSetTravIdCurrent(p, pObj);
    if ( Gia_ObjIsCi(pObj) )
        return 1;
    assert( Gia_ObjIsAnd(pObj) );
    if ( Gia_ObjSibl(p, Gia_ObjId(p, pObj)) )
        Gia_ManLevelWithBoxes_rec( p, Gia_ObjSiblObj(p, Gia_ObjId(p, pObj)) );
    if ( Gia_ManLevelWithBoxes_rec( p, Gia_ObjFanin0(pObj) ) )
        return 1;
    if ( Gia_ManLevelWithBoxes_rec( p, Gia_ObjFanin1(pObj) ) )
        return 1;
    Gia_ObjSetAndLevel( p, pObj );
    return 0;
}
int Gia_ManLevelWithBoxes( Gia_Man_t * p )
{
    int nAnd2Delay = p->nAnd2Delay ? p->nAnd2Delay : 1;
    Tim_Man_t * pManTime = (Tim_Man_t *)p->pManTime;
    Gia_Obj_t * pObj, * pObjIn;
    int i, k, j, curCi, curCo, LevelMax;
    assert( Gia_ManRegNum(p) == 0 );
    assert( Gia_ManBufNum(p) == 0 );
    // copy const and real PIs
    Gia_ManCleanLevels( p, Gia_ManObjNum(p) );
    Gia_ObjSetLevel( p, Gia_ManConst0(p), 0 );
    Gia_ManIncrementTravId( p );
    Gia_ObjSetTravIdCurrent( p, Gia_ManConst0(p) );
    for ( i = 0; i < Tim_ManPiNum(pManTime); i++ )
    {
        pObj = Gia_ManCi( p, i );
        Gia_ObjSetLevel( p, pObj, Tim_ManGetCiArrival(pManTime, i) / nAnd2Delay );
        Gia_ObjSetTravIdCurrent( p, pObj );
    }
    // create logic for each box
    curCi = Tim_ManPiNum(pManTime);
    curCo = 0;
    for ( i = 0; i < Tim_ManBoxNum(pManTime); i++ )
    {
        int nBoxInputs  = Tim_ManBoxInputNum( pManTime, i );
        int nBoxOutputs = Tim_ManBoxOutputNum( pManTime, i );
        float * pDelayTable = Tim_ManBoxDelayTable( pManTime, i );
        // compute level for TFI of box inputs
        for ( k = 0; k < nBoxInputs; k++ )
        {
            pObj = Gia_ManCo( p, curCo + k );
            if ( Gia_ManLevelWithBoxes_rec( p, Gia_ObjFanin0(pObj) ) )
            {
                printf( "Boxes are not in a topological order. Switching to level computation without boxes.\n" );
                return Gia_ManLevelNum( p );
            }
            // set box input level
            Gia_ObjSetCoLevel( p, pObj );
        }
        // compute level for box outputs
        for ( k = 0; k < nBoxOutputs; k++ )
        {
            pObj = Gia_ManCi( p, curCi + k );
            Gia_ObjSetTravIdCurrent( p, pObj );
            // evaluate delay of this output
            LevelMax = 0;
            assert( nBoxInputs == (int)pDelayTable[1] );
            for ( j = 0; j < nBoxInputs && (pObjIn = Gia_ManCo(p, curCo + j)); j++ )
                if ( (int)pDelayTable[3+k*nBoxInputs+j] != -ABC_INFINITY )
                    LevelMax = Abc_MaxInt( LevelMax, Gia_ObjLevel(p, pObjIn) + ((int)pDelayTable[3+k*nBoxInputs+j] / nAnd2Delay) );
            // set box output level
            Gia_ObjSetLevel( p, pObj, LevelMax );
        }
        curCo += nBoxInputs;
        curCi += nBoxOutputs;
    }
    // add remaining nodes
    p->nLevels = 0;
    for ( i = Tim_ManCoNum(pManTime) - Tim_ManPoNum(pManTime); i < Tim_ManCoNum(pManTime); i++ )
    {
        pObj = Gia_ManCo( p, i );
        Gia_ManLevelWithBoxes_rec( p, Gia_ObjFanin0(pObj) );
        Gia_ObjSetCoLevel( p, pObj );
        p->nLevels = Abc_MaxInt( p->nLevels, Gia_ObjLevel(p, pObj) );
    }
    curCo += Tim_ManPoNum(pManTime);
    // verify counts
    assert( curCi == Gia_ManCiNum(p) );
    assert( curCo == Gia_ManCoNum(p) );
//    printf( "Max level is %d.\n", p->nLevels );
    return p->nLevels;
}

/**Function*************************************************************

  Synopsis    [Computes level with boxes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManLutLevelWithBoxes_rec( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    int iObj, k, iFan, Level = 0;
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return 0;
    Gia_ObjSetTravIdCurrent(p, pObj);
    if ( Gia_ObjIsCi(pObj) )
        return 1;
    assert( Gia_ObjIsAnd(pObj) );
    iObj = Gia_ObjId( p, pObj );
    Gia_LutForEachFanin( p, iObj, iFan, k )
    {
        if ( Gia_ManLutLevelWithBoxes_rec( p, Gia_ManObj(p, iFan) ) )
            return 1;
        Level = Abc_MaxInt( Level, Gia_ObjLevelId(p, iFan) );
    }
    Gia_ObjSetLevelId( p, iObj, Level + 1 );
    return 0;
}
int Gia_ManLutLevelWithBoxes( Gia_Man_t * p )
{
//    int nAnd2Delay = p->nAnd2Delay ? p->nAnd2Delay : 1;
    Tim_Man_t * pManTime = (Tim_Man_t *)p->pManTime;
    Gia_Obj_t * pObj, * pObjIn;
    int i, k, j, curCi, curCo, LevelMax;
    assert( Gia_ManRegNum(p) == 0 );
    if ( pManTime == NULL )
        return Gia_ManLutLevel(p, NULL);
    // copy const and real PIs
    Gia_ManCleanLevels( p, Gia_ManObjNum(p) );
    Gia_ObjSetLevel( p, Gia_ManConst0(p), 0 );
    Gia_ManIncrementTravId( p );
    Gia_ObjSetTravIdCurrent( p, Gia_ManConst0(p) );
    for ( i = 0; i < Tim_ManPiNum(pManTime); i++ )
    {
        pObj = Gia_ManCi( p, i );
//        Gia_ObjSetLevel( p, pObj, Tim_ManGetCiArrival(pManTime, i) / nAnd2Delay );
        Gia_ObjSetLevel( p, pObj, 0 );
        Gia_ObjSetTravIdCurrent( p, pObj );
    }
    // create logic for each box
    curCi = Tim_ManPiNum(pManTime);
    curCo = 0;
    for ( i = 0; i < Tim_ManBoxNum(pManTime); i++ )
    {
        int nBoxInputs  = Tim_ManBoxInputNum( pManTime, i );
        int nBoxOutputs = Tim_ManBoxOutputNum( pManTime, i );
        float * pDelayTable = Tim_ManBoxDelayTable( pManTime, i );
        // compute level for TFI of box inputs
        for ( k = 0; k < nBoxInputs; k++ )
        {
            pObj = Gia_ManCo( p, curCo + k );
            if ( Gia_ManLutLevelWithBoxes_rec( p, Gia_ObjFanin0(pObj) ) )
            {
                printf( "Boxes are not in a topological order. Switching to level computation without boxes.\n" );
                return Gia_ManLevelNum( p );
            }
            // set box input level
            Gia_ObjSetCoLevel( p, pObj );
        }
        // compute level for box outputs
        for ( k = 0; k < nBoxOutputs; k++ )
        {
            pObj = Gia_ManCi( p, curCi + k );
            Gia_ObjSetTravIdCurrent( p, pObj );
            // evaluate delay of this output
            LevelMax = 0;
            assert( nBoxInputs == (int)pDelayTable[1] );
            for ( j = 0; j < nBoxInputs && (pObjIn = Gia_ManCo(p, curCo + j)); j++ )
                if ( (int)pDelayTable[3+k*nBoxInputs+j] != -ABC_INFINITY )
//                    LevelMax = Abc_MaxInt( LevelMax, Gia_ObjLevel(p, pObjIn) + ((int)pDelayTable[3+k*nBoxInputs+j] / nAnd2Delay) );
                    LevelMax = Abc_MaxInt( LevelMax, Gia_ObjLevel(p, pObjIn) + 1 );
            // set box output level
            Gia_ObjSetLevel( p, pObj, LevelMax );
        }
        curCo += nBoxInputs;
        curCi += nBoxOutputs;
    }
    // add remaining nodes
    p->nLevels = 0;
    for ( i = Tim_ManCoNum(pManTime) - Tim_ManPoNum(pManTime); i < Tim_ManCoNum(pManTime); i++ )
    {
        pObj = Gia_ManCo( p, i );
        Gia_ManLutLevelWithBoxes_rec( p, Gia_ObjFanin0(pObj) );
        Gia_ObjSetCoLevel( p, pObj );
        p->nLevels = Abc_MaxInt( p->nLevels, Gia_ObjLevel(p, pObj) );
    }
    curCo += Tim_ManPoNum(pManTime);
    // verify counts
    assert( curCi == Gia_ManCiNum(p) );
    assert( curCo == Gia_ManCoNum(p) );
//    printf( "Max level is %d.\n", p->nLevels );
    return p->nLevels;
}

/**Function*************************************************************

  Synopsis    [Update hierarchy/timing manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Gia_ManUpdateTimMan( Gia_Man_t * p, Vec_Int_t * vBoxPres )
{
    Tim_Man_t * pManTime = (Tim_Man_t *)p->pManTime;
    assert( pManTime != NULL );
    assert( Vec_IntSize(vBoxPres) == Tim_ManBoxNum(pManTime) );
    return Tim_ManTrim( pManTime, vBoxPres );
}
void * Gia_ManUpdateTimMan2( Gia_Man_t * p, Vec_Int_t * vBoxesLeft, int nTermsDiff )
{
    Tim_Man_t * pManTime = (Tim_Man_t *)p->pManTime;
    assert( pManTime != NULL );
    assert( Vec_IntSize(vBoxesLeft) <= Tim_ManBoxNum(pManTime) );
    return Tim_ManReduce( pManTime, vBoxesLeft, nTermsDiff );
}

/**Function*************************************************************

  Synopsis    [Update AIG of the holes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManUpdateExtraAig( void * pTime, Gia_Man_t * p, Vec_Int_t * vBoxPres )
{
    Gia_Man_t * pNew;
    Tim_Man_t * pManTime = (Tim_Man_t *)pTime;
    Vec_Int_t * vOutPres = Vec_IntAlloc( 100 );
    int i, k, curPo = 0;
    assert( Vec_IntSize(vBoxPres) == Tim_ManBoxNum(pManTime) );
    assert( Gia_ManCoNum(p) == Tim_ManCiNum(pManTime) - Tim_ManPiNum(pManTime) );
    for ( i = 0; i < Tim_ManBoxNum(pManTime); i++ )
    {
        for ( k = 0; k < Tim_ManBoxOutputNum(pManTime, i); k++ )
            Vec_IntPush( vOutPres, Vec_IntEntry(vBoxPres, i) );
        curPo += Tim_ManBoxOutputNum(pManTime, i);
    }
    assert( curPo == Gia_ManCoNum(p) );
    pNew = Gia_ManDupOutputVec( p, vOutPres );
    Vec_IntFree( vOutPres );
    return pNew;
}
Gia_Man_t * Gia_ManUpdateExtraAig2( void * pTime, Gia_Man_t * p, Vec_Int_t * vBoxesLeft )
{
    Gia_Man_t * pNew;
    Tim_Man_t * pManTime = (Tim_Man_t *)pTime;
    int nRealPis = Tim_ManPiNum(pManTime);
    Vec_Int_t * vOutsLeft = Vec_IntAlloc( 100 );
    int i, k, iBox, iOutFirst;
    assert( Vec_IntSize(vBoxesLeft) <= Tim_ManBoxNum(pManTime) );
    assert( Gia_ManCoNum(p) == Tim_ManCiNum(pManTime) - nRealPis );
    Vec_IntForEachEntry( vBoxesLeft, iBox, i )
    {
        iOutFirst = Tim_ManBoxOutputFirst(pManTime, iBox) - nRealPis;
        for ( k = 0; k < Tim_ManBoxOutputNum(pManTime, iBox); k++ )
            Vec_IntPush( vOutsLeft, iOutFirst + k );
    }
    pNew = Gia_ManDupSelectedOutputs( p, vOutsLeft );
    Vec_IntFree( vOutsLeft );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates AIG while moving the last CIs to be after PIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupMoveLast( Gia_Man_t * p, int iInsert, int nItems )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        if ( i < iInsert )
            pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachCi( p, pObj, i )
        if ( i >= Gia_ManCiNum(p) - nItems )
            pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachCi( p, pObj, i )
        if ( i >= iInsert && i < Gia_ManCiNum(p) - nItems )
            pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsCi(pObj) )
            continue;
        if ( Gia_ObjIsAnd(pObj) )
            pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else if ( Gia_ObjIsCo(pObj) )
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
        else assert( 0 );
    }
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Computes AIG with boxes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDupCollapse_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Gia_Man_t * pNew )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return;
    Gia_ObjSetTravIdCurrent(p, pObj);
    assert( Gia_ObjIsAnd(pObj) );
    if ( Gia_ObjSibl(p, Gia_ObjId(p, pObj)) )
        Gia_ManDupCollapse_rec( p, Gia_ObjSiblObj(p, Gia_ObjId(p, pObj)), pNew );
    Gia_ManDupCollapse_rec( p, Gia_ObjFanin0(pObj), pNew );
    Gia_ManDupCollapse_rec( p, Gia_ObjFanin1(pObj), pNew );
//    assert( !~pObj->Value );
    pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    if ( Gia_ObjSibl(p, Gia_ObjId(p, pObj)) )
        pNew->pSibls[Abc_Lit2Var(pObj->Value)] = Abc_Lit2Var(Gia_ObjSiblObj(p, Gia_ObjId(p, pObj))->Value);        
}
Gia_Man_t * Gia_ManDupCollapse( Gia_Man_t * p, Gia_Man_t * pBoxes, Vec_Int_t * vBoxPres, int fSeq )
{
    // this procedure assumes that sequential AIG with boxes is unshuffled to have valid boxes
    Tim_Man_t * pManTime = (Tim_Man_t *)p->pManTime;
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj, * pObjBox;
    int i, k, curCi, curCo, nBBins = 0, nBBouts = 0, nNewPis = 0;
    assert( !fSeq || p->vRegClasses );
    //assert( Gia_ManRegNum(p) == 0 );
    assert( Gia_ManCiNum(p) == Tim_ManPiNum(pManTime) + Gia_ManCoNum(pBoxes) );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    if ( Gia_ManHasChoices(p) )
        pNew->pSibls = ABC_CALLOC( int, Gia_ManObjNum(p) );
    Gia_ManHashAlloc( pNew );
    // copy const and real PIs
    Gia_ManFillValue( p );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManIncrementTravId( p );
    Gia_ObjSetTravIdCurrent( p, Gia_ManConst0(p) );
    for ( i = 0; i < Tim_ManPiNum(pManTime); i++ )
    {
        pObj = Gia_ManCi( p, i );
        pObj->Value = Gia_ManAppendCi(pNew);
        Gia_ObjSetTravIdCurrent( p, pObj );
    }
    // create logic for each box
    curCi = Tim_ManPiNum(pManTime);
    curCo = 0;
    for ( i = 0; i < Tim_ManBoxNum(pManTime); i++ )
    {
        // clean boxes
        Gia_ManIncrementTravId( pBoxes );
        Gia_ObjSetTravIdCurrent( pBoxes, Gia_ManConst0(pBoxes) );
        Gia_ManConst0(pBoxes)->Value = 0;
        // add internal nodes
        //printf( "%d ", Tim_ManBoxIsBlack(pManTime, i) );
        if ( Tim_ManBoxIsBlack(pManTime, i) )
        {
            int fSkip = (vBoxPres != NULL && !Vec_IntEntry(vBoxPres, i));
            for ( k = 0; k < Tim_ManBoxInputNum(pManTime, i); k++ )
            {
                pObj = Gia_ManCo( p, curCo + k );
                Gia_ManDupCollapse_rec( p, Gia_ObjFanin0(pObj), pNew );
                pObj->Value = fSkip ? -1 : Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
                nBBouts++;
            }
            for ( k = 0; k < Tim_ManBoxOutputNum(pManTime, i); k++ )
            {
                pObj = Gia_ManCi( p, curCi + k );
                pObj->Value = fSkip ? 0 : Gia_ManAppendCi(pNew);
                Gia_ObjSetTravIdCurrent( p, pObj );
                nBBins++;
                nNewPis += !fSkip;
            }
        }
        else
        {
            for ( k = 0; k < Tim_ManBoxInputNum(pManTime, i); k++ )
            {
                // build logic
                pObj = Gia_ManCo( p, curCo + k );
                Gia_ManDupCollapse_rec( p, Gia_ObjFanin0(pObj), pNew );
                // transfer to the PI
                pObjBox = Gia_ManCi( pBoxes, k );
                pObjBox->Value = Gia_ObjFanin0Copy(pObj);
                Gia_ObjSetTravIdCurrent( pBoxes, pObjBox );
            }
            for ( k = 0; k < Tim_ManBoxOutputNum(pManTime, i); k++ )
            {
                // build logic
                pObjBox = Gia_ManCo( pBoxes, curCi - Tim_ManPiNum(pManTime) + k );
                Gia_ManDupCollapse_rec( pBoxes, Gia_ObjFanin0(pObjBox), pNew );
                // transfer to the PI
                pObj = Gia_ManCi( p, curCi + k );
                pObj->Value = Gia_ObjFanin0Copy(pObjBox);
                Gia_ObjSetTravIdCurrent( p, pObj );
            }
        }
        curCo += Tim_ManBoxInputNum(pManTime, i);
        curCi += Tim_ManBoxOutputNum(pManTime, i);
    }
    //printf( "\n" );
    // add remaining nodes
    for ( i = Tim_ManCoNum(pManTime) - Tim_ManPoNum(pManTime); i < Tim_ManCoNum(pManTime); i++ )
    {
        pObj = Gia_ManCo( p, i );
        Gia_ManDupCollapse_rec( p, Gia_ObjFanin0(pObj), pNew );
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    }
    curCo += Tim_ManPoNum(pManTime);
    // verify counts
    assert( curCi == Gia_ManCiNum(p) );
    assert( curCo == Gia_ManCoNum(p) );
    Gia_ManSetRegNum( pNew, (fSeq && p->vRegClasses) ? Vec_IntSize(p->vRegClasses) : Gia_ManRegNum(p) );
    Gia_ManHashStop( pNew );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManCleanupRemap( p, pTemp );
    Gia_ManStop( pTemp );
    if ( nNewPis )
    {
        pNew = Gia_ManDupMoveLast( pTemp = pNew, Tim_ManPiNum(pManTime)-Gia_ManRegNum(pNew), nNewPis );
        Gia_ManCleanupRemap( p, pTemp );
        Gia_ManStop( pTemp );
    }
/*
    printf( "%d = %d - %d    Diff = %d\n", 
        Tim_ManPoNum(pManTime),   Gia_ManCoNum(pNew),  nBBouts, 
        Tim_ManPoNum(pManTime) - (Gia_ManCoNum(pNew) - nBBouts) );

    printf( "%d = %d - %d    Diff = %d\n\n", 
        Tim_ManPiNum(pManTime),   Gia_ManCiNum(pNew),  nBBins, 
        Tim_ManPiNum(pManTime) - (Gia_ManCiNum(pNew) - nBBins) );
*/
    assert( vBoxPres != NULL || Tim_ManPoNum(pManTime) == Gia_ManCoNum(pNew) - nBBouts );
    assert( vBoxPres != NULL || Tim_ManPiNum(pManTime) == Gia_ManCiNum(pNew) - nBBins );
    // implement initial state if given
    if ( fSeq && p->vRegInits && Vec_IntSum(p->vRegInits) )
    {
        char * pInit = ABC_ALLOC( char, Vec_IntSize(p->vRegInits) + 1 );
        Gia_Obj_t * pObj;
        int i;
        assert( Vec_IntSize(p->vRegInits) == Gia_ManRegNum(pNew) );
        Gia_ManForEachRo( pNew, pObj, i )
        {
            if ( Vec_IntEntry(p->vRegInits, i) == 0 )
                pInit[i] = '0';
            else if ( Vec_IntEntry(p->vRegInits, i) == 1 )
                pInit[i] = '1';
            else 
                pInit[i] = 'X';
        }
        pInit[i] = 0;
        pNew = Gia_ManDupZeroUndc( pTemp = pNew, pInit, 0, 0, 1 );
        pNew->nConstrs = pTemp->nConstrs; pTemp->nConstrs = 0;
        Gia_ManStop( pTemp );
        ABC_FREE( pInit );
    }
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Verify XAIG against its spec.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManVerifyWithBoxes( Gia_Man_t * pGia, int nBTLimit, int nTimeLim, int fSeq, int fDumpFiles, int fVerbose, char * pFileSpec )
{
    int Status   = -1;
    Gia_Man_t * pSpec, * pGia0, * pGia1, * pMiter;
    Vec_Int_t * vBoxPres = NULL;
    if ( pFileSpec == NULL && pGia->pSpec == NULL )
    {
        printf( "Spec file is not given. Use standard flow.\n" );
        return Status;
    }
    if ( Gia_ManBoxNum(pGia) && pGia->pAigExtra == NULL )
    {
        printf( "Design has no box logic. Use standard flow.\n" );
        return Status;
    }
    // read original AIG
    pSpec = Gia_AigerRead( pFileSpec ? pFileSpec : pGia->pSpec, 0, 0, 0 );
    if ( Gia_ManBoxNum(pSpec) && pSpec->pAigExtra == NULL )
    {
        Gia_ManStop( pSpec );
        printf( "Spec has no box logic. Use standard flow.\n" );
        return Status;
    }
    // prepare miter
    if ( pGia->pManTime == NULL && pSpec->pManTime == NULL )
    {
        pGia0 = Gia_ManDup( pSpec );
        pGia1 = Gia_ManDup( pGia );
    }
    else
    {
        // if timing managers have different number of black boxes,
        // it is possible that some of the boxes are swept away
        if ( pSpec->pManTime && Tim_ManBlackBoxNum((Tim_Man_t *)pSpec->pManTime) > 0 && Gia_ManBoxNum(pGia) > 0 )
        {
            // specification cannot have fewer boxes than implementation
            if ( Gia_ManBoxNum(pSpec) < Gia_ManBoxNum(pGia) )
            {
                printf( "Spec has less boxes than the design. Cannot proceed.\n" );
                return Status;
            }
            // to align the boxes, find what boxes of pSpec are dropped in pGia
            if ( Gia_ManBoxNum(pSpec) > Gia_ManBoxNum(pGia) )
            {
                vBoxPres = Tim_ManAlignTwo( (Tim_Man_t *)pSpec->pManTime, (Tim_Man_t *)pGia->pManTime );
                if ( vBoxPres == NULL )
                {
                    printf( "Boxes of spec and design cannot be aligned. Cannot proceed.\n" );
                    return Status;
                }
            }
        }
        // collapse two designs
        if ( Gia_ManBoxNum(pSpec) > 0 )
            pGia0 = Gia_ManDupCollapse( pSpec, pSpec->pAigExtra, vBoxPres, fSeq );
        else
            pGia0 = Gia_ManDup( pSpec );
        if ( Gia_ManBoxNum(pGia) > 0 )
            pGia1 = Gia_ManDupCollapse( pGia,  pGia->pAigExtra,  NULL, fSeq  );
        else
            pGia1 = Gia_ManDup( pGia );
        Vec_IntFreeP( &vBoxPres );
    }
    if ( fDumpFiles )
    {
        char pFileName0[1000], pFileName1[1000];
        char * pNameGeneric = Extra_FileNameGeneric( pFileSpec ? pFileSpec : pGia->pSpec );
        sprintf( pFileName0, "%s_spec.aig", pNameGeneric );
        sprintf( pFileName1, "%s_impl.aig", pNameGeneric );
        Gia_AigerWrite( pGia0, pFileName0, 0, 0, 0 );
        Gia_AigerWrite( pGia1, pFileName1, 0, 0, 0 );
        ABC_FREE( pNameGeneric );
        printf( "Dumped two parts of the miter into files \"%s\" and \"%s\".\n", pFileName0, pFileName1 );
    }
    // compute the miter
    if ( fSeq )
    {
        pMiter = Gia_ManMiter( pGia0, pGia1, 0, 0, 1, 0, fVerbose );
        if ( pMiter )
        {
            Aig_Man_t * pMan;
            Fra_Sec_t SecPar, * pSecPar = &SecPar;
            Fra_SecSetDefaultParams( pSecPar );
            pSecPar->fRetimeFirst = 0;
            pSecPar->nBTLimit  = nBTLimit;
            pSecPar->TimeLimit = nTimeLim;
            pSecPar->fVerbose  = fVerbose;
            pMan = Gia_ManToAig( pMiter, 0 );
            Gia_ManStop( pMiter );
            Status = Fra_FraigSec( pMan, pSecPar, NULL );
            Aig_ManStop( pMan );
        }
    }
    else
    {
        pMiter = Gia_ManMiter( pGia0, pGia1, 0, 1, 0, 0, fVerbose );
        if ( pMiter )
        {
            Cec_ParCec_t ParsCec, * pPars = &ParsCec;
            Cec_ManCecSetDefaultParams( pPars );
            pPars->nBTLimit  = nBTLimit;
            pPars->TimeLimit = nTimeLim;
            pPars->fVerbose  = fVerbose;
            Status = Cec_ManVerify( pMiter, pPars );
            if ( pPars->iOutFail >= 0 )
                Abc_Print( 1, "Verification failed for at least one output (%d).\n", pPars->iOutFail );
            Gia_ManStop( pMiter );
        }
    }
    Gia_ManStop( pGia0 );
    Gia_ManStop( pGia1 );
    Gia_ManStop( pSpec );
    return Status;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

