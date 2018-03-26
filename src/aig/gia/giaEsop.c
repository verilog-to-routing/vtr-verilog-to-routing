/**CFile****************************************************************

  FileName    [giaEsop.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [ESOP computation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaEsop.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/extra/extra.h"
#include "misc/vec/vecHsh.h"
#include "misc/vec/vecWec.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Eso_Man_t_ Eso_Man_t;
struct Eso_Man_t_
{
    Gia_Man_t *     pGia;     // user's AIG
    int             nVars;    // number of variables
    int             Cube1;    // ID of const1 cube
    Vec_Wec_t *     vEsops;   // ESOP for each node
    Hsh_VecMan_t *  pHash;    // hash table for cubes
    Vec_Wec_t *     vCubes;   // cover during minimization
    // internal
    Vec_Int_t *     vCube1;   // first cube
    Vec_Int_t *     vCube2;   // second cube
    Vec_Int_t *     vCube;    // resulting cube
};

static inline Vec_Int_t * Eso_ManCube( Eso_Man_t * p, int iCube ) { assert( iCube >= 0 ); return Hsh_VecReadEntry(p->pHash, iCube); }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
    
/**Function*************************************************************

  Synopsis    [Computation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Eso_Man_t * Eso_ManAlloc( Gia_Man_t * pGia )
{
    int i, n, Id;
    Eso_Man_t * p = ABC_CALLOC( Eso_Man_t, 1 );
    p->pGia   = pGia;
    p->nVars  = Gia_ManCiNum(pGia);
    p->Cube1  = ABC_INFINITY;
    p->vEsops = Vec_WecStart( Gia_ManObjNum(pGia) );
    p->pHash  = Hsh_VecManStart( 1000 );
    p->vCubes = Vec_WecStart(Gia_ManCiNum(pGia)+1);
    p->vCube1 = Vec_IntAlloc(Gia_ManCiNum(pGia));
    p->vCube2 = Vec_IntAlloc(Gia_ManCiNum(pGia));
    p->vCube  = Vec_IntAlloc(Gia_ManCiNum(pGia));
    Gia_ManForEachCiId( pGia, Id, i )
    {
        for ( n = 0; n < 2; n++ )
        {
            Vec_IntFill( p->vCube, 1, Abc_Var2Lit(i, n) );
            Hsh_VecManAdd( p->pHash, p->vCube );
        }
        Vec_IntPush( Vec_WecEntry(p->vEsops, Id), Abc_Var2Lit(i, 0) );
    }
    return p;
}
void Eso_ManStop( Eso_Man_t * p )
{
    Vec_WecFree( p->vEsops );
    Hsh_VecManStop( p->pHash );
    Vec_WecFree( p->vCubes );
    Vec_IntFree( p->vCube1 );
    Vec_IntFree( p->vCube2 );
    Vec_IntFree( p->vCube );
    ABC_FREE( p );
}
    

/**Function*************************************************************

  Synopsis    [Printing/transforming the cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Eso_ManCoverPrint( Eso_Man_t * p, Vec_Int_t * vEsop )
{
    Vec_Str_t * vStr;
    Vec_Int_t * vCube;
    int i, k, Lit, Cube;
    if ( Vec_IntSize(vEsop) == 0 )
    {
        printf( "Const 0\n" );
        return;
    }
    vStr = Vec_StrAlloc( p->nVars + 4 );
    Vec_StrFill( vStr, p->nVars, '-' );
    Vec_StrPush( vStr, ' ' );
    Vec_StrPush( vStr, '1' );
    Vec_StrPush( vStr, '\n' );
    Vec_StrPush( vStr, '\0' );
    assert( Vec_IntSize(vEsop) > 0 );
    Vec_IntForEachEntry( vEsop, Cube, i )
    {
        if ( Cube == p->Cube1 )
            printf( "%s", Vec_StrArray(vStr) );
        else 
        {
            vCube = Eso_ManCube( p, Cube );
            Vec_IntForEachEntry( vCube, Lit, k )
                Vec_StrWriteEntry( vStr, Abc_Lit2Var(Lit), (char)(Abc_LitIsCompl(Lit)?'0':'1') );
            printf( "%s", Vec_StrArray(vStr) );
            Vec_IntForEachEntry( vCube, Lit, k )
                Vec_StrWriteEntry( vStr, Abc_Lit2Var(Lit), '-' );
        }
    }
    printf( "\n" );
    Vec_StrFree( vStr );
}
Vec_Wec_t * Eso_ManCoverDerive( Eso_Man_t * p, Vec_Ptr_t * vCover )
{
    Vec_Wec_t * vRes;
    Vec_Int_t * vEsop, * vLevel;  int i;
    vRes = Vec_WecAlloc( Vec_VecSizeSize((Vec_Vec_t *)vCover) );
    Vec_PtrForEachEntry( Vec_Int_t *, vCover, vEsop, i )
    {
        if ( Vec_IntSize(vEsop) > 0 )
        {
            int c, Cube;
            Vec_IntForEachEntry( vEsop, Cube, c )
            {
                vLevel = Vec_WecPushLevel( vRes );
                if ( Cube != p->Cube1 )
                {
                    int k, Lit;
                    Vec_Int_t * vCube = Eso_ManCube( p, Cube );
                    Vec_IntForEachEntry( vCube, Lit, k )
                        Vec_IntPush( vLevel, Lit );
                }
                Vec_IntPush( vLevel, -i-1 );
            }
        }
    }
    //assert( Abc_MaxInt(Vec_WecSize(vRes), 8) == Vec_WecCap(vRes) );
    return vRes;
}
Gia_Man_t * Eso_ManCoverConvert( Eso_Man_t * p, Vec_Ptr_t * vCover )
{
    Vec_Int_t * vEsop;
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;  int i;
    pNew = Gia_ManStart( Gia_ManObjNum(p->pGia) );
    pNew->pName = Abc_UtilStrsav( p->pGia->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pGia->pSpec );
    Gia_ManHashAlloc( pNew );
    Gia_ManConst0(p->pGia)->Value = 0;
    Gia_ManForEachCi( p->pGia, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Vec_PtrForEachEntry( Vec_Int_t *, vCover, vEsop, i )
    {
        if ( Vec_IntSize(vEsop) > 0 )
        {
            int c, Cube, iRoot = 0;
            Vec_IntForEachEntry( vEsop, Cube, c )
            {
                int k, Lit, iAnd = 1;
                if ( Cube != p->Cube1 )
                {
                    Vec_Int_t * vCube = Eso_ManCube( p, Cube );
                    Vec_IntForEachEntry( vCube, Lit, k )
                        iAnd = Gia_ManHashAnd( pNew, iAnd, Lit + 2 );
                }
                iRoot = Gia_ManHashXor( pNew, iRoot, iAnd );
            }
            Gia_ManAppendCo( pNew, iRoot );
        }
        else
            Gia_ManAppendCo( pNew, 0 );
    }
    // cleanup
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Minimization.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Eso_ManFindDistOneLitEqual( int * pCube1, int * pCube2, int nLits ) // pCube1 and pCube2 both have nLits
{
    int i, iDiff = -1;
    for ( i = 0; i < nLits; i++ )
        if ( pCube1[i] != pCube2[i] )
        {
            if ( iDiff != -1 )
                return -1;
            if ( Abc_Lit2Var(pCube1[i]) != Abc_Lit2Var(pCube2[i]) )
                return -1;
            iDiff = i;
        }
    return iDiff;
}
int Eso_ManFindDistOneLitNotEqual( int * pCube1, int * pCube2, int nLits ) // pCube1 has nLits; pCube2 has nLits + 1
{
    int i, k, iDiff = -1;
    for ( i = k = 0; i < nLits; i++, k++ )
        if ( pCube1[i] != pCube2[k] )
        {
            if ( iDiff != -1 )
                return -1;
            iDiff = i;
            i--;
        }
    if ( iDiff == -1 )
        iDiff = nLits;
    return iDiff;
}
void Eso_ManMinimizeAdd( Eso_Man_t * p, int Cube )
{
    int fMimimize = 1;
    Vec_Int_t * vCube      = (Cube == p->Cube1) ? NULL : Eso_ManCube(p, Cube); 
    int * pCube2, * pCube  = (Cube == p->Cube1) ? NULL : Vec_IntArray(vCube); 
    int Cube2, nLits       = (Cube == p->Cube1) ? 0 : Vec_IntSize(vCube);
    Vec_Int_t * vLevel     = Vec_WecEntry( p->vCubes, nLits );
    int c, k, iLit, iPlace = Vec_IntFind( vLevel, Cube );
    if ( iPlace >= 0 ) // identical found
    {
        Vec_IntDrop( vLevel, iPlace );
        return;
    }
    if ( Cube == p->Cube1 ) // simple case
    {
        assert( Vec_IntSize(vLevel) == 0 );
        Vec_IntPush( vLevel, Cube );
        return;
    }
    // look for distance-1 in next bin
    if ( fMimimize && nLits < p->nVars - 1 )
    {
        Vec_Int_t * vLevel = Vec_WecEntry( p->vCubes, nLits+1 );
        Vec_IntForEachEntry( vLevel, Cube2, c )
        {
            pCube2 = Hsh_VecReadArray( p->pHash, Cube2 ); 
            iLit = Eso_ManFindDistOneLitNotEqual( pCube, pCube2, nLits );
            if ( iLit == -1 )
                continue;
            // remove this cube
            Vec_IntDrop( vLevel, c );
            // create new cube
            Vec_IntClear( p->vCube );
            for ( k = 0; k <= nLits; k++ )
                Vec_IntPush( p->vCube, Abc_LitNotCond(pCube2[k], k == iLit) );
            Cube = Hsh_VecManAdd( p->pHash, p->vCube );
            // try to add new cube
            Eso_ManMinimizeAdd( p, Cube );
            return;
        }
    }
    // look for distance-1 in the same bin
    if ( fMimimize )
    {
        Vec_IntForEachEntry( vLevel, Cube2, c )
        {
            pCube2 = Hsh_VecReadArray( p->pHash, Cube2 ); 
            iLit = Eso_ManFindDistOneLitEqual( pCube2, pCube, nLits );
            if ( iLit == -1 )
                continue;
            // remove this cube
            Vec_IntDrop( vLevel, c );
            // create new cube
            Vec_IntClear( p->vCube );
            for ( k = 0; k < nLits; k++ )
                if ( k != iLit )
                    Vec_IntPush( p->vCube, pCube[k] );
            if ( Vec_IntSize(p->vCube) == 0 )
                Cube = p->Cube1;
            else
                Cube = Hsh_VecManAdd( p->pHash, p->vCube );
            // try to add new cube
            Eso_ManMinimizeAdd( p, Cube );
            return;
        }
    }
    assert( nLits > 0 );
    if ( fMimimize && nLits > 0 )
    {
        // look for distance-1 in the previous bin
        Vec_Int_t * vLevel = Vec_WecEntry( p->vCubes, nLits-1 );
        // check for the case of one-literal cube
        if ( nLits == 1 && Vec_IntSize(vLevel) == 1 )
        {
            Vec_IntDrop( vLevel, 0 );
            Cube = Abc_LitNot( Cube );
        }
        else
        Vec_IntForEachEntry( vLevel, Cube2, c )
        {
            pCube2 = Hsh_VecReadArray( p->pHash, Cube2 ); 
            iLit = Eso_ManFindDistOneLitNotEqual( pCube2, pCube, nLits-1 );
            if ( iLit == -1 )
                continue;
            // remove this cube
            Vec_IntDrop( vLevel, c );
            // create new cube
            Vec_IntClear( p->vCube );
            for ( k = 0; k < nLits; k++ )
                Vec_IntPush( p->vCube, Abc_LitNotCond(pCube[k], k == iLit) );
            Cube = Hsh_VecManAdd( p->pHash, p->vCube );
            // try to add new cube
            Eso_ManMinimizeAdd( p, Cube );
            return;
        }
    }
    // could not find - simply add this cube
    Vec_IntPush( vLevel, Cube );
}

void Eso_ManMinimizeCopy( Eso_Man_t * p, Vec_Int_t * vEsop )
{
    Vec_Int_t * vLevel;
    int i;
    Vec_IntClear( vEsop );
    Vec_WecForEachLevel( p->vCubes, vLevel, i )
    {
        Vec_IntAppend( vEsop, vLevel );
        if ( i > 0 )
        {
            int k, Cube;
            Vec_IntForEachEntry( vLevel, Cube, k )
                assert( Vec_IntSize(Eso_ManCube(p, Cube)) == i );
        }
        Vec_IntClear( vLevel );
    }
}

/**Function*************************************************************

  Synopsis    [Compute the produce of two covers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Eso_ManComputeAnd( Eso_Man_t * p, Vec_Int_t * vCube1, Vec_Int_t * vCube2, Vec_Int_t * vCube )
{
    int * pBeg  = vCube->pArray;
    int * pBeg1 = vCube1->pArray;
    int * pBeg2 = vCube2->pArray;
    int * pEnd1 = vCube1->pArray + vCube1->nSize;
    int * pEnd2 = vCube2->pArray + vCube2->nSize;
    while ( pBeg1 < pEnd1 && pBeg2 < pEnd2 )
    {
        if ( *pBeg1 == *pBeg2 )
            *pBeg++ = *pBeg1++, pBeg2++;
        else if ( Abc_Lit2Var(*pBeg1) == Abc_Lit2Var(*pBeg2) )
            return -1;
        else if ( *pBeg1 < *pBeg2 )
            *pBeg++ = *pBeg1++;
        else 
            *pBeg++ = *pBeg2++;
    }
    while ( pBeg1 < pEnd1 )
        *pBeg++ = *pBeg1++;
    while ( pBeg2 < pEnd2 )
        *pBeg++ = *pBeg2++;
    vCube->nSize = pBeg - vCube->pArray;
    assert( vCube->nSize <= vCube->nCap );
    assert( vCube->nSize >= vCube1->nSize );
    assert( vCube->nSize >= vCube2->nSize );
    return Hsh_VecManAdd( p->pHash, vCube );
}
void Eso_ManComputeOne( Eso_Man_t * p, Vec_Int_t * vEsop1, Vec_Int_t * vEsop2, Vec_Int_t * vEsop )
{
    Vec_Int_t vCube1, vCube2;
    int i, k, Cube1, Cube2, Cube;
    Vec_IntClear( vEsop );
    if ( Vec_IntSize(vEsop1) == 0 || Vec_IntSize(vEsop2) == 0 )
        return;
    Cube1 = Vec_IntEntry(vEsop1, 0);
    Cube2 = Vec_IntEntry(vEsop2, 0);
    Vec_IntForEachEntry( vEsop1, Cube1, i )
    {
        if ( Cube1 == p->Cube1 )
        {
            Vec_IntForEachEntry( vEsop2, Cube2, k )
                Eso_ManMinimizeAdd( p, Cube2 );
            continue;
        }
        Vec_IntForEachEntry( vEsop2, Cube2, k )
        {
            if ( Cube2 == p->Cube1 )
            {
                Eso_ManMinimizeAdd( p, Cube1 );
                continue;
            }
            vCube1 = *Hsh_VecReadEntry( p->pHash, Cube1 );
            vCube2 = *Hsh_VecReadEntry( p->pHash, Cube2 );
            Cube = Eso_ManComputeAnd( p, &vCube1, &vCube2, p->vCube );
            if ( Cube >= 0 )
                Eso_ManMinimizeAdd( p, Cube );
        }
    }
    Eso_ManMinimizeCopy( p, vEsop );
}

/**Function*************************************************************

  Synopsis    [Complements the cover if needed, or just copy it.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Eso_ManTransformOne( Eso_Man_t * p, Vec_Int_t * vEsop, int fCompl, Vec_Int_t * vRes )
{
    int i, Cube, Start = 0;
    Vec_IntClear( vRes );
    if ( fCompl )
    {
        if ( Vec_IntSize(vEsop) == 0 )
            Vec_IntPush( vRes, p->Cube1 );
        else
        {
            Cube = Vec_IntEntry(vEsop, 0);
            if ( Cube == p->Cube1 )
                Start = 1;
            else if ( Cube < 2 * p->nVars )
                Vec_IntPush( vRes, Abc_LitNot(Cube) ), Start = 1;
            else
                Vec_IntPush( vRes, p->Cube1 );
        }
    }
    Vec_IntForEachEntryStart( vEsop, Cube, i, Start )
        Vec_IntPush( vRes, Cube );
    return vRes;
}

/**Function*************************************************************

  Synopsis    [Computes ESOP from AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Eso_ManCompute( Gia_Man_t * pGia, int fVerbose, Vec_Wec_t ** pvRes )
{
    abctime clk = Abc_Clock();
    Vec_Ptr_t * vCover;
    Gia_Man_t * pNew = NULL;
    Gia_Obj_t * pObj; 
    int i, nCubes = 0, nCubesUsed = 0;
    Vec_Int_t * vEsop1, * vEsop2, * vEsop;
    Eso_Man_t * p = Eso_ManAlloc( pGia ); 
    Gia_ManForEachAnd( pGia, pObj, i )
    {
        vEsop1 = Vec_WecEntry( p->vEsops, Gia_ObjFaninId0(pObj, i) );
        vEsop2 = Vec_WecEntry( p->vEsops, Gia_ObjFaninId1(pObj, i) );
        vEsop1 = Eso_ManTransformOne( p, vEsop1, Gia_ObjFaninC0(pObj), p->vCube1 );
        vEsop2 = Eso_ManTransformOne( p, vEsop2, Gia_ObjFaninC1(pObj), p->vCube2 );
        vEsop  = Vec_WecEntry( p->vEsops, i );
        Eso_ManComputeOne( p, vEsop1, vEsop2, vEsop );
        nCubes += Vec_IntSize(vEsop);
    }
    vCover = Vec_PtrAlloc( Gia_ManCoNum(pGia) );
    Gia_ManForEachCo( pGia, pObj, i )
    {
        vEsop1 = Vec_WecEntry( p->vEsops, Gia_ObjFaninId0p(pGia, pObj) );
        vEsop1 = Eso_ManTransformOne( p, vEsop1, Gia_ObjFaninC0(pObj), p->vCube1 );
        if ( fVerbose )
            printf( "Output %3d:  ESOP has %5d cubes\n", i, Vec_IntSize(vEsop1) );
//        if ( fVerbose )
//            Eso_ManCoverPrint( p, vEsop1 );
        Vec_PtrPush( vCover, Vec_IntDup(vEsop1) );
        nCubesUsed += Vec_IntSize(vEsop1);
    }
    if ( fVerbose )
    {
        printf( "Outs = %d.  Cubes = %d.  Used = %d.  Hashed = %d. ", 
            Gia_ManCoNum(pGia), nCubes, nCubesUsed, Hsh_VecSize(p->pHash) );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    }
    if ( pvRes )
        *pvRes = Eso_ManCoverDerive( p, vCover );
    else
        pNew = Eso_ManCoverConvert( p, vCover );
    Vec_VecFree( (Vec_Vec_t *)vCover );
    Eso_ManStop( p );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

