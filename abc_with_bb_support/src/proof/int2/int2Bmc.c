/**CFile****************************************************************

  FileName    [int2Bmc.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Interpolation engine.]

  Synopsis    [BMC used inside IMC.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Dec 1, 2013.]

  Revision    [$Id: int2Bmc.c,v 1.00 2013/12/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "int2Int.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Trasnforms AIG to transition into the init state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Int2_ManDupInit( Gia_Man_t * p, int fVerbose )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj, * pObjRi, * pObjRo;
    int i, iCtrl;
    assert( Gia_ManRegNum(p) > 0 );
    pNew = Gia_ManStart( 10000 );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
    {
        if ( i == Gia_ManPiNum(p) )
            iCtrl = Gia_ManAppendCi( pNew );
        pObj->Value = Gia_ManAppendCi( pNew );
    }
    Gia_ManHashAlloc( pNew );
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachPo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManForEachRiRo( p, pObjRi, pObjRo, i )
        Gia_ManAppendCo( pNew, Gia_ManHashMux( pNew, iCtrl, pObjRo->Value, Gia_ObjFanin0Copy(pObjRi) ) );
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    // remove dangling
    pNew = Gia_ManCleanup( pTemp = pNew );
    if ( fVerbose )
        printf( "Before cleanup = %d nodes. After cleanup = %d nodes.\n", 
            Gia_ManAndNum(pTemp), Gia_ManAndNum(pNew) );
    Gia_ManStop( pTemp );
    return pNew;
}


/**Function*************************************************************

  Synopsis    [Returns 1 if AIG has transition into init state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Int2_ManCheckInit( Gia_Man_t * p )
{
    sat_solver * pSat;
    Cnf_Dat_t * pCnf;
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    Vec_Int_t * vLits;
    int i, Lit, RetValue = 0;
    assert( Gia_ManRegNum(p) > 0 );
    pNew = Jf_ManDeriveCnf( p, 0 );  
    pCnf = (Cnf_Dat_t *)pNew->pData; pNew->pData = NULL;
    pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
    if ( pSat != NULL )
    {
        vLits = Vec_IntAlloc( Gia_ManRegNum(p) );
        Gia_ManForEachRi( pNew, pObj, i )
        {
            Lit = pCnf->pVarNums[ Gia_ObjId(pNew, Gia_ObjFanin0(pObj)) ];
            Lit = Abc_Var2Lit( Lit, Gia_ObjFaninC0(pObj) );
            Vec_IntPush( vLits, Abc_LitNot(Lit) );
        }
        if ( sat_solver_solve( pSat, Vec_IntArray(vLits), Vec_IntArray(vLits) + Vec_IntSize(vLits), 0, 0, 0, 0 ) == l_True )
            RetValue = 1;
        Vec_IntFree( vLits );
        sat_solver_delete( pSat );
    }
    Cnf_DataFree( pCnf );
    Gia_ManStop( pNew );
    return RetValue;
}


/**Function*************************************************************

  Synopsis    [Creates the BMC instance in the SAT solver.]

  Description [The PIs are mapped in the natural order. The flop inputs
  are the last Gia_ManRegNum(p) variables of resulting SAT solver.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Int2_ManFrameInit( Gia_Man_t * p, int nFrames, int fVerbose )
{
    Gia_Man_t * pFrames, * pTemp;
    Gia_Obj_t * pObj;
    int i, f;
    pFrames = Gia_ManStart( 10000 );
    pFrames->pName = Abc_UtilStrsav( p->pName );
    pFrames->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    // perform structural hashing
    Gia_ManHashAlloc( pFrames );
    for ( f = 0; f < nFrames; f++ )
    {
        Gia_ManForEachPi( p, pObj, i )
            pObj->Value = Gia_ManAppendCi( pFrames );
        Gia_ManForEachRo( p, pObj, i )
            pObj->Value = f ? Gia_ObjRoToRi(p, pObj)->Value : 0;
        Gia_ManForEachAnd( p, pObj, i )
            pObj->Value = Gia_ManHashAnd( pFrames, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        Gia_ManForEachRi( p, pObj, i )
            pObj->Value = Gia_ObjFanin0Copy(pObj);
    }
    Gia_ManHashStop( pFrames );
    // create flop inputs
    Gia_ManForEachRi( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pFrames, Gia_ObjFanin0Copy(pObj) );
    // remove dangling
    pFrames = Gia_ManCleanup( pTemp = pFrames );
    if ( fVerbose )
        printf( "Before cleanup = %d nodes. After cleanup = %d nodes.\n", 
            Gia_ManAndNum(pTemp), Gia_ManAndNum(pFrames) );
    Gia_ManStop( pTemp );
    return pFrames;
}
sat_solver * Int2_ManSetupBmcSolver( Gia_Man_t * p, int nFrames )
{
    Gia_Man_t * pFrames, * pTemp;
    Cnf_Dat_t * pCnf;
    sat_solver * pSat;
    // unfold for the given number of timeframes
    pFrames = Int2_ManFrameInit( p, nFrames, 1 );
    assert( Gia_ManRegNum(pFrames) == 0 );
    // derive CNF for the timeframes
    pFrames = Jf_ManDeriveCnf( pTemp = pFrames, 0 );  Gia_ManStop( pTemp );
    pCnf = (Cnf_Dat_t *)pFrames->pData; pFrames->pData = NULL;
    // create SAT solver
    pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
    if ( pSat != NULL )
    {
        Gia_Obj_t * pObj;
        int i, nVars = sat_solver_nvars( pSat );
        sat_solver_setnvars( pSat, nVars + Gia_ManPoNum(pFrames) );
        // add clauses for the POs
        Gia_ManForEachCo( pFrames, pObj, i )
            sat_solver_add_buffer( pSat, nVars + i, pCnf->pVarNums[Gia_ObjId(pFrames, Gia_ObjFanin0(pObj))], Gia_ObjFaninC0(pObj) );
    }
    Cnf_DataFree( pCnf );
    Gia_ManStop( pFrames );
    return pSat;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Int2_ManCheckFrames( Int2_Man_t * p, int iFrame, int iObj )
{
    Vec_Int_t * vMapFrame = (Vec_Int_t *)Vec_PtrEntry(p->vMapFrames, iFrame);
    return Vec_IntEntry(vMapFrame, iObj);
}
static inline void Int2_ManWriteFrames( Int2_Man_t * p, int iFrame, int iObj, int iRes )
{
    Vec_Int_t * vMapFrame = (Vec_Int_t *)Vec_PtrEntry(p->vMapFrames, iFrame);
    assert( Vec_IntEntry(vMapFrame, iObj) == -1 );
    Vec_IntWriteEntry( vMapFrame, iObj, iRes );
}
void Int2_ManCreateFrames( Int2_Man_t * p, int iFrame, Vec_Int_t * vPrefCos )
{
    Gia_Obj_t * pObj;
    int i, Entry, iLit;
    // create storage room for unfolded IDs
    for ( i = Vec_PtrSize(p->vMapFrames); i <= iFrame; i++ )
        Vec_PtrPush( p->vMapFrames, Vec_IntStartFull( Gia_ManObjNum(p->pGia) ) );
    assert( Vec_PtrSize(p->vMapFrames) == iFrame + 1 );
    // create constant 0 node
    if ( f == 0 )
    {
        iLit = 1;
        Int2_ManWriteFrames( p, iFrame, iObj, 0 );
        sat_solver_addclause( p->pGiaPref, &iLit, &iLit + 1 );
    }
    // start the stack
    Vec_IntClear( p->vStack );
    Vec_IntForEachEntry( vPrefCos, Entry, i )
    {
        pObj = Gia_ManCo( p->pGia, Entry );
        Vec_IntPush( p->vStack, iFrame );
        Vec_IntPush( p->vStack, Gia_ObjId(p->pGia, pObj) );
    }
    // construct unfolded AIG
    while ( Vec_IntSize(p->vStack) > 0 )
    {
        int iObj   = Vec_IntPop(p->vStack);
        int iFrame = Vec_IntPop(p->vStack);
        if ( Int2_ManCheckFrames(p, iFrame, iObj) >= 0 )
            continue;
        pObj = Gia_ManObj( p->pGia, iObj );
        if ( Gia_ObjIsPi(p->pGia, pObj) )
            Int2_ManWriteFrames( p, iFrame, iObj, Gia_ManAppendCi(p->pFrames) );
        else if ( iFrame == 0 && Gia_ObjIsRo(p->pGia, iObj) )
            Int2_ManWriteFrames( p, iFrame, iObj, 0 );
        else if ( Gia_ObjIsRo(p->pGia, iObj) )
        {
            int iObjF = Gia_ObjId( p->pGia, Gia_ObjRoToRi(p->pGia, pObj) );
            int iLit = Int2_ManCheckFrames( p, iFrame-1, iObjF );
            if ( iLit >= 0 )
                Int2_ManWriteFrames( p, iFrame, iObj, iLit );
            else
            {
                Vec_IntPush( p->vStack, iFrame );
                Vec_IntPush( p->vStack, iObj );
                Vec_IntPush( p->vStack, iFrame-1 );
                Vec_IntPush( p->vStack, iObjF );
            }
        }
        else if ( Gia_ObjIsCo(pObj) )
        {
            int iObjF = Gia_ObjFaninId0(p->pGia, iObj) );
            int iLit = Int2_ManCheckFrames( p, iFrame, iObjF );
            if ( iLit >= 0 )
                Int2_ManWriteFrames( p, iFrame, iObj, Abc_LitNotCond(iLit, Gia_ObjFaninC0(pObj)) );
            else
            {
                Vec_IntPush( p->vStack, iFrame );
                Vec_IntPush( p->vStack, iObj );
                Vec_IntPush( p->vStack, iFrame );
                Vec_IntPush( p->vStack, iObjF );
            }
        }
        else if ( Gia_ObjIsAnd(pObj) )
        {
            int iObjF0 = Gia_ObjFaninId0(p->pGia, iObj) );
            int iLit0 = Int2_ManCheckFrames( p, iFrame, iObjF0 );
            int iObjF1 = Gia_ObjFaninId1(p->pGia, iObj) );
            int iLit1 = Int2_ManCheckFrames( p, iFrame, iObjF1 );
            if ( iLit0 >= 0 && iLit1 >= 0 )
            {
                Entry = Gia_ManObjNum(pFrames);
                iLit = Gia_ManHashAnd(pFrames, iLit0, iLit1);
                Int2_ManWriteFrames( p, iFrame, iObj, iLit );
                if ( Entry < Gia_ManObjNum(pFrames) )
                {
                    assert( !Abc_LitIsCompl(iLit) );
                    sat_solver_add_and( p->pGiaPref, Abc_Lit2Var(iLit), Abc_Lit2Var(iLit0), Abc_Lit2Var(iLit1), Abc_LitIsCompl(iLit0), Abc_LitIsCompl(iLit1), 0 ); 
                }
            }
            else
            {
                Vec_IntPush( p->vStack, iFrame );
                Vec_IntPush( p->vStack, iObj );
                if ( iLit0 < 0 )
                {
                    Vec_IntPush( p->vStack, iFrame );
                    Vec_IntPush( p->vStack, iObjF0 );
                }
                if ( iLit1 < 0 )
                {
                    Vec_IntPush( p->vStack, iFrame );
                    Vec_IntPush( p->vStack, iObjF1 );
                }
            }
        }
        else assert( 0 );
    }
}
int Int2_ManCheckBmc( Int2_Man_t * p, Vec_Int_t * vCube )
{
    int status;
    if ( vCube == NULL )
    {
        Gia_Obj_t * pObj;
        int i, iLit;
        Gia_ManForEachPo( p->pGia, pObj, i )
        {
            iLit = Int2_ManCheckFrames( p, 0, Gia_ObjId(p->pGia, pObj) );
            if ( iLit == 0 )
                continue;
            if ( iLit == 1 )
                return 0;
            status = sat_solver_solve( p->pSatPref, &iLit, &iLit + 1, 0, 0, 0, 0 );
            if ( status == l_False )
                continue;
            if ( status == l_True )
                return 0;
            return -1;
        }
        return 1;
    }
    status = sat_solver_solve( p->pSatPref, Vec_IntArray(vCube), Vec_IntArray(vCube) + Vec_IntSize(vCube), 0, 0, 0, 0 );
    if ( status == l_False )
        return 1;
    if ( status == l_True )
        return 0;
    return -1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

