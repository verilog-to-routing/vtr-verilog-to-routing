/**CFile****************************************************************

  FileName    [dauGia.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware unmapping.]

  Synopsis    [Coverting DSD into GIA.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: dauGia.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "dauInt.h"
#include "aig/gia/gia.h"
#include "misc/util/utilTruth.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern int Kit_TruthToGia( Gia_Man_t * pMan, unsigned * pTruth, int nVars, Vec_Int_t * vMemory, Vec_Int_t * vLeaves, int fHash );

#define DAU_DSD_MAX_VAR 12

static int m_Calls = 0;
static int m_NonDsd = 0;
static int m_Non1Step = 0;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
 
/**Function*************************************************************

  Synopsis    [Derives GIA for the truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dau_DsdToGiaCompose_rec( Gia_Man_t * pGia, word Func, int * pFanins, int nVars )
{
    int t0, t1;
    if ( Func == 0 )
        return 0;
    if ( Func == ~(word)0 )
        return 1;
    assert( nVars > 0 );
    if ( --nVars == 0 )
    {
        assert( Func == s_Truths6[0] || Func == s_Truths6Neg[0] );
        return Abc_LitNotCond( pFanins[0], (int)(Func == s_Truths6Neg[0]) );
    }
    if ( !Abc_Tt6HasVar(Func, nVars) )
        return Dau_DsdToGiaCompose_rec( pGia, Func, pFanins, nVars );
    t0 = Dau_DsdToGiaCompose_rec( pGia, Abc_Tt6Cofactor0(Func, nVars), pFanins, nVars );
    t1 = Dau_DsdToGiaCompose_rec( pGia, Abc_Tt6Cofactor1(Func, nVars), pFanins, nVars );
    if ( pGia->pMuxes )
        return Gia_ManHashMuxReal( pGia, pFanins[nVars], t1, t0 );
    else
        return Gia_ManHashMux( pGia, pFanins[nVars], t1, t0 );
}

/**Function*************************************************************

  Synopsis    [Derives GIA for the DSD formula.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dau_DsdToGia2_rec( Gia_Man_t * pGia, char * pStr, char ** p, int * pMatches, int * pLits, Vec_Int_t * vCover )
{
    int fCompl = 0;
    if ( **p == '!' )
        (*p)++, fCompl = 1;
    if ( **p >= 'a' && **p < 'a' + DAU_DSD_MAX_VAR ) // var
        return Abc_LitNotCond( pLits[**p - 'a'], fCompl );
    if ( **p == '(' ) // and/or
    {
        char * q = pStr + pMatches[ *p - pStr ];
        int Res = 1, Lit;
        assert( **p == '(' && *q == ')' );
        for ( (*p)++; *p < q; (*p)++ )
        {
            Lit = Dau_DsdToGia2_rec( pGia, pStr, p, pMatches, pLits, vCover );
            Res = Gia_ManHashAnd( pGia, Res, Lit );
        }
        assert( *p == q );
        return Abc_LitNotCond( Res, fCompl );
    }
    if ( **p == '[' ) // xor
    {
        char * q = pStr + pMatches[ *p - pStr ];
        int Res = 0, Lit;
        assert( **p == '[' && *q == ']' );
        for ( (*p)++; *p < q; (*p)++ )
        {
            Lit = Dau_DsdToGia2_rec( pGia, pStr, p, pMatches, pLits, vCover );
            if ( pGia->pMuxes )
                Res = Gia_ManHashXorReal( pGia, Res, Lit );
            else
                Res = Gia_ManHashXor( pGia, Res, Lit );
        }
        assert( *p == q );
        return Abc_LitNotCond( Res, fCompl );
    }
    if ( **p == '<' ) // mux
    {
        int nVars = 0;
        int Temp[3], * pTemp = Temp, Res;
        int Fanins[DAU_DSD_MAX_VAR], * pLits2;
        char * pOld = *p;
        char * q = pStr + pMatches[ *p - pStr ];
        // read fanins
        if ( *(q+1) == '{' )
        {
            char * q2;
            *p = q+1;
            q2 = pStr + pMatches[ *p - pStr ];
            assert( **p == '{' && *q2 == '}' );
            for ( nVars = 0, (*p)++; *p < q2; (*p)++, nVars++ )
                Fanins[nVars] = Dau_DsdToGia2_rec( pGia, pStr, p, pMatches, pLits, vCover );
            assert( *p == q2 );
            pLits2 = Fanins;
        }
        else
            pLits2 = pLits;
        // read MUX
        *p = pOld;
        q = pStr + pMatches[ *p - pStr ];
        assert( **p == '<' && *q == '>' );
        // verify internal variables
        if ( nVars )
            for ( ; pOld < q; pOld++ )
                if ( *pOld >= 'a' && *pOld <= 'z' )
                    assert( *pOld - 'a' < nVars );
        // derive MUX components
        for ( (*p)++; *p < q; (*p)++ )
            *pTemp++ = Dau_DsdToGia2_rec( pGia, pStr, p, pMatches, pLits2, vCover );
        assert( pTemp == Temp + 3 );
        assert( *p == q );
        if ( *(q+1) == '{' ) // and/or
        {
            char * q = pStr + pMatches[ ++(*p) - pStr ];
            assert( **p == '{' && *q == '}' );
            *p = q;
        }
        if ( pGia->pMuxes )
            Res = Gia_ManHashMuxReal( pGia, Temp[0], Temp[1], Temp[2] );
        else
            Res = Gia_ManHashMux( pGia, Temp[0], Temp[1], Temp[2] );
        return Abc_LitNotCond( Res, fCompl );
    }
    if ( (**p >= 'A' && **p <= 'F') || (**p >= '0' && **p <= '9') )
    {
        Vec_Int_t vLeaves;  char * q;
        word pFunc[DAU_DSD_MAX_VAR > 6 ? (1 << (DAU_DSD_MAX_VAR-6)) : 1];
        int Fanins[DAU_DSD_MAX_VAR], Res; 
        int i, nVars = Abc_TtReadHex( pFunc, *p );
        *p += Abc_TtHexDigitNum( nVars );
        q = pStr + pMatches[ *p - pStr ];
        assert( **p == '{' && *q == '}' );
        for ( i = 0, (*p)++; *p < q; (*p)++, i++ )
            Fanins[i] = Dau_DsdToGia2_rec( pGia, pStr, p, pMatches, pLits, vCover );
        assert( i == nVars );
        assert( *p == q );
//        Res = Dau_DsdToGia2Compose_rec( pGia, Func, Fanins, nVars );
        vLeaves.nCap = nVars;
        vLeaves.nSize = nVars;
        vLeaves.pArray = Fanins;        
        Res = Kit_TruthToGia( pGia, (unsigned *)pFunc, nVars, vCover, &vLeaves, 1 );
        m_Non1Step++;
        return Abc_LitNotCond( Res, fCompl );
    }
    assert( 0 );
    return 0;
}
int Dau_DsdToGia2( Gia_Man_t * pGia, char * p, int * pLits, Vec_Int_t * vCover )
{
    int Res;
    if ( *p == '0' && *(p+1) == 0 )
        Res = 0;
    else if ( *p == '1' && *(p+1) == 0 )
        Res = 1;
    else
        Res = Dau_DsdToGia2_rec( pGia, p, &p, Dau_DsdComputeMatches(p), pLits, vCover );
    assert( *++p == 0 );
    return Res;
}

/**Function*************************************************************

  Synopsis    [Derives GIA for the DSD formula.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dau_DsdAddToArray( Gia_Man_t * pGia, int * pFans, int nFans, int iFan )
{
    int i;
    pFans[nFans] = iFan;
    if ( nFans == 0 )
        return;
    for ( i = nFans; i > 0; i-- )
    {
        if ( Gia_ObjLevelId(pGia, Abc_Lit2Var(pFans[i])) <= Gia_ObjLevelId(pGia, Abc_Lit2Var(pFans[i-1])) )
            return;
        ABC_SWAP( int, pFans[i], pFans[i-1] );
    }
}
int Dau_DsdBalance( Gia_Man_t * pGia, int * pFans, int nFans, int fAnd )
{
    Gia_Obj_t * pObj;
    int iFan0, iFan1, iFan;
    if ( nFans == 1 )
        return pFans[0];
    assert( nFans > 1 );
    iFan0 = pFans[--nFans];
    iFan1 = pFans[--nFans];
    if ( Vec_IntSize(&pGia->vHTable) == 0 )
    {
        if ( fAnd )
            iFan = Gia_ManAppendAnd2( pGia, iFan0, iFan1 );
        else if ( pGia->pMuxes )
        {
            int fCompl = Abc_LitIsCompl(iFan0) ^ Abc_LitIsCompl(iFan1);
            iFan = Gia_ManAppendXorReal( pGia, Abc_LitRegular(iFan0), Abc_LitRegular(iFan1) );
            iFan = Abc_LitNotCond( iFan, fCompl );
        }
        else 
            iFan = Gia_ManAppendXor2( pGia, iFan0, iFan1 );
    }
    else
    {
        if ( fAnd )
            iFan = Gia_ManHashAnd( pGia, iFan0, iFan1 );
        else if ( pGia->pMuxes )
            iFan = Gia_ManHashXorReal( pGia, iFan0, iFan1 );
        else 
            iFan = Gia_ManHashXor( pGia, iFan0, iFan1 );
    }
    pObj = Gia_ManObj(pGia, Abc_Lit2Var(iFan));
    if ( Gia_ObjIsAnd(pObj) )
    {
        if ( fAnd )
            Gia_ObjSetAndLevel( pGia, pObj );
        else if ( pGia->pMuxes )
            Gia_ObjSetXorLevel( pGia, pObj );
        else 
        {
            if ( Gia_ObjIsAnd(Gia_ObjFanin0(pObj)) )
                Gia_ObjSetAndLevel( pGia, Gia_ObjFanin0(pObj) );
            if ( Gia_ObjIsAnd(Gia_ObjFanin1(pObj)) )
                Gia_ObjSetAndLevel( pGia, Gia_ObjFanin1(pObj) );
            Gia_ObjSetAndLevel( pGia, pObj );
        }
    }
    Dau_DsdAddToArray( pGia, pFans, nFans++, iFan );
    return Dau_DsdBalance( pGia, pFans, nFans, fAnd );
}
int Dau_DsdToGia_rec( Gia_Man_t * pGia, char * pStr, char ** p, int * pMatches, int * pLits, Vec_Int_t * vCover )
{
    int fCompl = 0;
    if ( **p == '!' )
        (*p)++, fCompl = 1;
    if ( **p >= 'a' && **p < 'a' + DAU_DSD_MAX_VAR ) // var
        return Abc_LitNotCond( pLits[**p - 'a'], fCompl );
    if ( **p == '(' ) // and/or
    {
        char * q = pStr + pMatches[ *p - pStr ];
        int pFans[DAU_DSD_MAX_VAR], nFans = 0, Fan;
        assert( **p == '(' && *q == ')' );
        for ( (*p)++; *p < q; (*p)++ )
        {
            Fan = Dau_DsdToGia_rec( pGia, pStr, p, pMatches, pLits, vCover );
            Dau_DsdAddToArray( pGia, pFans, nFans++, Fan );
        }
        Fan = Dau_DsdBalance( pGia, pFans, nFans, 1 );
        assert( *p == q );
        return Abc_LitNotCond( Fan, fCompl );
    }
    if ( **p == '[' ) // xor
    {
        char * q = pStr + pMatches[ *p - pStr ];
        int pFans[DAU_DSD_MAX_VAR], nFans = 0, Fan;
        assert( **p == '[' && *q == ']' );
        for ( (*p)++; *p < q; (*p)++ )
        {
            Fan = Dau_DsdToGia_rec( pGia, pStr, p, pMatches, pLits, vCover );
            Dau_DsdAddToArray( pGia, pFans, nFans++, Fan );
        }
        Fan = Dau_DsdBalance( pGia, pFans, nFans, 0 );
        assert( *p == q );
        return Abc_LitNotCond( Fan, fCompl );
    }
    if ( **p == '<' ) // mux
    {
        Gia_Obj_t * pObj;
        int nVars = 0;
        int Temp[3], * pTemp = Temp, Res;
        int Fanins[DAU_DSD_MAX_VAR], * pLits2;
        char * pOld = *p;
        char * q = pStr + pMatches[ *p - pStr ];
        // read fanins
        if ( *(q+1) == '{' )
        {
            char * q2;
            *p = q+1;
            q2 = pStr + pMatches[ *p - pStr ];
            assert( **p == '{' && *q2 == '}' );
            for ( nVars = 0, (*p)++; *p < q2; (*p)++, nVars++ )
                Fanins[nVars] = Dau_DsdToGia_rec( pGia, pStr, p, pMatches, pLits, vCover );
            assert( *p == q2 );
            pLits2 = Fanins;
        }
        else
            pLits2 = pLits;
        // read MUX
        *p = pOld;
        q = pStr + pMatches[ *p - pStr ];
        assert( **p == '<' && *q == '>' );
        // verify internal variables
        if ( nVars )
            for ( ; pOld < q; pOld++ )
                if ( *pOld >= 'a' && *pOld <= 'z' )
                    assert( *pOld - 'a' < nVars );
        // derive MUX components
        for ( (*p)++; *p < q; (*p)++ )
            *pTemp++ = Dau_DsdToGia_rec( pGia, pStr, p, pMatches, pLits2, vCover );
        assert( pTemp == Temp + 3 );
        assert( *p == q );
        if ( *(q+1) == '{' ) // and/or
        {
            char * q = pStr + pMatches[ ++(*p) - pStr ];
            assert( **p == '{' && *q == '}' );
            *p = q;
        }
        if ( Vec_IntSize(&pGia->vHTable) == 0 )
        {
            if ( pGia->pMuxes )
                Res = Gia_ManAppendMux( pGia, Temp[0], Temp[1], Temp[2] );
            else
                Res = Gia_ManAppendMux2( pGia, Temp[0], Temp[1], Temp[2] );
        }
        else
        {
            if ( pGia->pMuxes )
                Res = Gia_ManHashMuxReal( pGia, Temp[0], Temp[1], Temp[2] );
            else
                Res = Gia_ManHashMux( pGia, Temp[0], Temp[1], Temp[2] );
        }
        pObj = Gia_ManObj(pGia, Abc_Lit2Var(Res));
        if ( Gia_ObjIsAnd(pObj) )
        {
            if ( pGia->pMuxes && Vec_IntSize(&pGia->vHTable) )
                Gia_ObjSetMuxLevel( pGia, pObj );
            else 
            {
                if ( Gia_ObjIsAnd(Gia_ObjFanin0(pObj)) )
                    Gia_ObjSetAndLevel( pGia, Gia_ObjFanin0(pObj) );
                if ( Gia_ObjIsAnd(Gia_ObjFanin1(pObj)) )
                    Gia_ObjSetAndLevel( pGia, Gia_ObjFanin1(pObj) );
                Gia_ObjSetAndLevel( pGia, pObj );
            }
        }
        return Abc_LitNotCond( Res, fCompl );
    }
    if ( (**p >= 'A' && **p <= 'F') || (**p >= '0' && **p <= '9') )
    {
        Vec_Int_t vLeaves;  char * q;
        word pFunc[DAU_DSD_MAX_VAR > 6 ? (1 << (DAU_DSD_MAX_VAR-6)) : 1];
        int Fanins[DAU_DSD_MAX_VAR], Res, nObjOld; 
        int i, nVars = Abc_TtReadHex( pFunc, *p );
        *p += Abc_TtHexDigitNum( nVars );
        q = pStr + pMatches[ *p - pStr ];
        assert( **p == '{' && *q == '}' );
        for ( i = 0, (*p)++; *p < q; (*p)++, i++ )
            Fanins[i] = Dau_DsdToGia_rec( pGia, pStr, p, pMatches, pLits, vCover );
        assert( i == nVars );
        assert( *p == q );
        vLeaves.nCap = nVars;
        vLeaves.nSize = nVars;
        vLeaves.pArray = Fanins;      
        nObjOld = Gia_ManObjNum(pGia);
        Res = Kit_TruthToGia( pGia, (unsigned *)pFunc, nVars, vCover, &vLeaves, Vec_IntSize(&pGia->vHTable) != 0 );
//        assert( nVars <= 6 );
//        Res = Dau_DsdToGiaCompose_rec( pGia, pFunc[0], Fanins, nVars );
        for ( i = nObjOld; i < Gia_ManObjNum(pGia); i++ )
            Gia_ObjSetGateLevel( pGia, Gia_ManObj(pGia, i) );
        m_Non1Step++;
        return Abc_LitNotCond( Res, fCompl );
    }
    assert( 0 );
    return 0;
}
int Dau_DsdToGia( Gia_Man_t * pGia, char * p, int * pLits, Vec_Int_t * vCover )
{
    int Res;
    if ( *p == '0' && *(p+1) == 0 )
        Res = 0;
    else if ( *p == '1' && *(p+1) == 0 )
        Res = 1;
    else
        Res = Dau_DsdToGia_rec( pGia, p, &p, Dau_DsdComputeMatches(p), pLits, vCover );
    assert( *++p == 0 );
    return Res;
}

/**Function*************************************************************

  Synopsis    [Convert TT to GIA via DSD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dsm_ManTruthToGia( void * p, word * pTruth, Vec_Int_t * vLeaves, Vec_Int_t * vCover )
{
    int fUseMuxes = 0;
    int fDelayBalance = 1;
    Gia_Man_t * pGia = (Gia_Man_t *)p;
    int nSizeNonDec;
    char pDsd[1000];
    word pTruthCopy[DAU_MAX_WORD];
    Abc_TtCopy( pTruthCopy, pTruth, Abc_TtWordNum(Vec_IntSize(vLeaves)), 0 );
    m_Calls++;
    assert( Vec_IntSize(vLeaves) <= DAU_DSD_MAX_VAR );
    // collect delay information
    if ( fDelayBalance && fUseMuxes )
    {
        int i, iLit, pVarLevels[DAU_DSD_MAX_VAR];
        Vec_IntForEachEntry( vLeaves, iLit, i )
            pVarLevels[i] = Gia_ObjLevelId( pGia, Abc_Lit2Var(iLit) );
        nSizeNonDec = Dau_DsdDecomposeLevel( pTruthCopy, Vec_IntSize(vLeaves), fUseMuxes, 1, pDsd, pVarLevels );
    }
    else
        nSizeNonDec = Dau_DsdDecompose( pTruthCopy, Vec_IntSize(vLeaves), fUseMuxes, 1, pDsd );
    if ( nSizeNonDec )
        m_NonDsd++;
//    printf( "%s\n", pDsd );
    if ( fDelayBalance && pGia->vLevels )
        return Dau_DsdToGia( pGia, pDsd, Vec_IntArray(vLeaves), vCover );
    else
        return Dau_DsdToGia2( pGia, pDsd, Vec_IntArray(vLeaves), vCover );
}

/**Function*************************************************************

  Synopsis    [Convert TT to GIA via DSD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dsm_ManReportStats()
{
    printf( "Calls = %d. NonDSD = %d. Non1Step = %d.\n", m_Calls, m_NonDsd, m_Non1Step );
    m_Calls = m_NonDsd = m_Non1Step = 0;
}

/**Function*************************************************************

  Synopsis    [Performs structural hashing on the LUT functions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Dsm_ManDeriveGia( void * pGia, int fUseMuxes )
{
    Gia_Man_t * p = (Gia_Man_t *)pGia;
    Gia_Man_t * pNew, * pTemp;
    Vec_Int_t * vCover, * vLeaves;
    Gia_Obj_t * pObj; 
    int k, i, iLut, iVar;
    word * pTruth;
    assert( Gia_ManHasMapping(p) );   
    // create new manager
    pNew = Gia_ManStart( 6*Gia_ManObjNum(p)/5 + 100 );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    pNew->vLevels = Vec_IntStart( 6*Gia_ManObjNum(p)/5 + 100 );
    if ( fUseMuxes )
        pNew->pMuxes = ABC_CALLOC( unsigned, pNew->nObjsAlloc );
    // map primary inputs
    Gia_ManFillValue(p);
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    // iterate through nodes used in the mapping
    vLeaves = Vec_IntAlloc( 16 );
    vCover  = Vec_IntAlloc( 1 << 16 );
    Gia_ManHashStart( pNew );
    Gia_ObjComputeTruthTableStart( p, Gia_ManLutSizeMax(p) );
    Gia_ManForEachAnd( p, pObj, iLut )
    {
        if ( Gia_ObjIsBuf(pObj) )
        {
            pObj->Value = Gia_ManAppendBuf( pNew, Gia_ObjFanin0Copy(pObj) );
            continue;
        }
        if ( !Gia_ObjIsLut(p, iLut) )
            continue;
        // collect leaves
        Vec_IntClear( vLeaves );
        Gia_LutForEachFanin( p, iLut, iVar, k )
            Vec_IntPush( vLeaves, iVar );
        pTruth = Gia_ObjComputeTruthTableCut( p, Gia_ManObj(p, iLut), vLeaves );
        // collect incoming literals
        Vec_IntClear( vLeaves );
        Gia_LutForEachFanin( p, iLut, iVar, k )
            Vec_IntPush( vLeaves, Gia_ManObj(p, iVar)->Value );
        Gia_ManObj(p, iLut)->Value = Dsm_ManTruthToGia( pNew, pTruth, vLeaves, vCover );
    }
    Gia_ObjComputeTruthTableStop( p );
    Gia_ManForEachCo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    Vec_IntFree( vLeaves );
    Vec_IntFree( vCover );
/*
    Gia_ManForEachAnd( pNew, pObj, i )
    {
        int iLev  = Gia_ObjLevelId(pNew, i);
        int iLev0 = Gia_ObjLevelId(pNew, Gia_ObjFaninId0(pObj, i));
        int iLev1 = Gia_ObjLevelId(pNew, Gia_ObjFaninId1(pObj, i));
        assert( iLev == 1 + Abc_MaxInt(iLev0, iLev1) );
    }
*/
    // perform cleanup
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

