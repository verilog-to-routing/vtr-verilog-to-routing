/**CFile****************************************************************

  FileName    [wlcSim.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Verilog parser.]

  Synopsis    [Performs sequential simulation of word-level network.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 04, 2015.]

  Revision    [$Id: wlcSim.c,v 1.00 2015/06/04 00:00:00 alanmi Exp $]

***********************************************************************/

#include "wlc.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Internal simulation APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word * Wlc_ObjSim( Gia_Man_t * p, int iObj )
{
    return Vec_WrdEntryP( p->vSims, p->nSimWords * iObj );
}
static inline void Wlc_ObjSimPi( Gia_Man_t * p, int iObj )
{
    int w;
    word * pSim = Wlc_ObjSim( p, iObj );
    for ( w = 0; w < p->nSimWords; w++ )
        pSim[w] = Gia_ManRandomW( 0 );
}
static inline void Wlc_ObjSimRo( Gia_Man_t * p, int iObj )
{
    int w;
    word * pSimRo = Wlc_ObjSim( p, iObj );
    word * pSimRi = Wlc_ObjSim( p, Gia_ObjRoToRiId(p, iObj) );
    for ( w = 0; w < p->nSimWords; w++ )
        pSimRo[w] = pSimRi[w];
}
static inline void Wlc_ObjSimCo( Gia_Man_t * p, int iObj )
{
    int w;
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    word * pSimCo  = Wlc_ObjSim( p, iObj );
    word * pSimDri = Wlc_ObjSim( p, Gia_ObjFaninId0(pObj, iObj) );
    if ( Gia_ObjFaninC0(pObj) )
        for ( w = 0; w < p->nSimWords; w++ )
            pSimCo[w] = ~pSimDri[w];
    else
        for ( w = 0; w < p->nSimWords; w++ )
            pSimCo[w] =  pSimDri[w];
}
static inline void Wlc_ObjSimAnd( Gia_Man_t * p, int iObj )
{
    int w;
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    word * pSim  = Wlc_ObjSim( p, iObj );
    word * pSim0 = Wlc_ObjSim( p, Gia_ObjFaninId0(pObj, iObj) );
    word * pSim1 = Wlc_ObjSim( p, Gia_ObjFaninId1(pObj, iObj) );
    if ( Gia_ObjFaninC0(pObj) && Gia_ObjFaninC1(pObj) )
        for ( w = 0; w < p->nSimWords; w++ )
            pSim[w] = ~pSim0[w] & ~pSim1[w];
    else if ( Gia_ObjFaninC0(pObj) && !Gia_ObjFaninC1(pObj) )
        for ( w = 0; w < p->nSimWords; w++ )
            pSim[w] = ~pSim0[w] & pSim1[w];
    else if ( !Gia_ObjFaninC0(pObj) && Gia_ObjFaninC1(pObj) )
        for ( w = 0; w < p->nSimWords; w++ )
            pSim[w] = pSim0[w] & ~pSim1[w];
    else
        for ( w = 0; w < p->nSimWords; w++ )
            pSim[w] = pSim0[w] & pSim1[w];
}


/**Function*************************************************************

  Synopsis    [Performs simulation of a word-level network.]

  Description [Returns vRes, a 2D array of simulation information for 
  the output of each bit of each object listed in vNodes. In particular, 
  Vec_Ptr_t * vSimObj = (Vec_Ptr_t *)Vec_PtrEntry(vRes, iObj) and
  Vec_Ptr_t * vSimObjBit = (Vec_Ptr_t *)Vec_PtrEntry(vSimObj, iBit)
  are arrays containing the simulation info for each object (vSimObj) 
  and for each output bit of this object (vSimObjBit). Alternatively,
  Vec_Ptr_t * vSimObjBit = Vec_VecEntryEntry( (Vec_Vec_t *)vRes, iObj, iBit ).
  The output bitwidth of an object is Wlc_ObjRange( Wlc_NtkObj(pNtk, iObj) ).
  Simulation information is binary data constaining the given number (nWords)
  of 64-bit machine words for the given number (nFrames) of consecutive 
  timeframes.  The total number of timeframes is nWords * nFrames for 
  each bit of each object.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wlc_NtkDeleteSim( Vec_Ptr_t * p )
{
    word * pInfo; int i, k;
    Vec_Vec_t * vVec = (Vec_Vec_t *)p;
    Vec_VecForEachEntry( word *, vVec, pInfo, i, k )
        ABC_FREE( pInfo );
    Vec_VecFree( vVec );
}
Vec_Ptr_t * Wlc_NtkSimulate( Wlc_Ntk_t * p, Vec_Int_t * vNodes, int nWords, int nFrames )
{
    Gia_Obj_t * pObj; 
    Vec_Ptr_t * vOne, * vRes;
    Gia_Man_t * pGia = Wlc_NtkBitBlast( p, NULL );
    Wlc_Obj_t * pWlcObj;
    int f, i, k, w, nBits, Counter = 0;
    // allocate simulation info for one timeframe
    Vec_WrdFreeP( &pGia->vSims );
    pGia->vSims = Vec_WrdStart( Gia_ManObjNum(pGia) * nWords );
    pGia->nSimWords = nWords;
    // allocate resulting simulation info
    vRes = Vec_PtrAlloc( Vec_IntSize(vNodes) );
    Wlc_NtkForEachObjVec( vNodes, p, pWlcObj, i )
    {
        nBits = Wlc_ObjRange(pWlcObj);
        vOne = Vec_PtrAlloc( nBits );
        for ( k = 0; k < nBits; k++ )
            Vec_PtrPush( vOne, ABC_CALLOC(word, nWords * nFrames) );
        Vec_PtrPush( vRes, vOne ); 
    }
    // perform simulation (const0 and flop outputs are already initialized)
    Gia_ManRandomW( 1 );
    for ( f = 0; f < nFrames; f++ )
    {
        Gia_ManForEachObj1( pGia, pObj, i )
        {
            if ( Gia_ObjIsAnd(pObj) )
                Wlc_ObjSimAnd( pGia, i );
            else if ( Gia_ObjIsCo(pObj) )
                Wlc_ObjSimCo( pGia, i );
            else if ( Gia_ObjIsPi(pGia, pObj) )
                Wlc_ObjSimPi( pGia, i );
            else if ( Gia_ObjIsRo(pGia, pObj) )
                Wlc_ObjSimRo( pGia, i );
        }
        // collect simulation data
        Wlc_NtkForEachObjVec( vNodes, p, pWlcObj, i )
        {
            int nBits = Wlc_ObjRange(pWlcObj);
            int iFirst = Vec_IntEntry( &p->vCopies, Wlc_ObjId(p, pWlcObj) );
            for ( k = 0; k < nBits; k++ )
            {
                int iLit = Vec_IntEntry( &p->vBits, iFirst + k );
                word * pInfo = (word*)Vec_VecEntryEntry( (Vec_Vec_t *)vRes, i, k );
                if ( iLit == -1 )
                {
                    Counter++;
                    for ( w = 0; w < nWords; w++ )
                        pInfo[f * nWords + w] = 0;
                }
                else
                {
                    word * pInfoObj = Wlc_ObjSim( pGia, Abc_Lit2Var(iLit) );
                    for ( w = 0; w < nWords; w++ )
                        pInfo[f * nWords + w] = Abc_LitIsCompl(iLit) ? ~pInfoObj[w] : pInfoObj[w];
                }
            }
        }
        if ( f == 0 && Counter )
            printf( "Replaced %d dangling internal bits with constant 0.\n", Counter );
    }
    Vec_WrdFreeP( &pGia->vSims );
    pGia->nSimWords = 0;
    Gia_ManStop( pGia );
    return vRes;
}

/**Function*************************************************************

  Synopsis    [Testing procedure.]

  Description [This testing procedure assumes that the WLC network has 
  one node, which is a multiplier. It simulates the node and checks the 
  word-level interpretation of the bit-level simulation info to make sure 
  that it indeed represents multiplication.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wlc_NtkSimulatePrint( Wlc_Ntk_t * p, Vec_Int_t * vNodes, Vec_Ptr_t * vRes, int nWords, int nFrames )
{
    Wlc_Obj_t * pWlcObj; 
    int f, w, b, i, k, iPat = 0;
    for ( f = 0; f < nFrames; f++, printf("\n") )
      for ( w = 0; w < nWords; w++ )
        for ( b = 0; b < 64; b++, iPat++, printf("\n") )
        {
            Wlc_NtkForEachObjVec( vNodes, p, pWlcObj, i )
            {
                int nBits = Wlc_ObjRange(pWlcObj);
                for ( k = nBits-1; k >= 0; k-- )
                {
                    word * pInfo = (word*)Vec_VecEntryEntry( (Vec_Vec_t *)vRes, i, k );
                    printf( "%d", Abc_InfoHasBit((unsigned *)pInfo, iPat) );
                }
                printf( " " );
            }
        }
}
void Wlc_NtkSimulateTest( Wlc_Ntk_t * p )
{
    int nWords = 2;
    int nFrames = 2;
    Vec_Ptr_t * vRes;
    Vec_Int_t * vNodes = Vec_IntAlloc( 3 );
    Vec_IntPush( vNodes, 1 );
    Vec_IntPush( vNodes, 2 );
    Vec_IntPush( vNodes, 3 );
    vRes = Wlc_NtkSimulate( p, vNodes, nWords, nFrames );
    Wlc_NtkSimulatePrint( p, vNodes, vRes, nWords, nFrames );
    Wlc_NtkDeleteSim( vRes );
    Vec_IntFree( vNodes );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

