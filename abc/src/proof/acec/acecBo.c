/**CFile****************************************************************

  FileName    [acecBo.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [CEC for arithmetic circuits.]

  Synopsis    [Core procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: acecBo.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "acecInt.h"
#include "misc/vec/vecWec.h"
#include "misc/extra/extra.h"
#include "misc/util/utilTruth.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Acec_DetectBoothXorMux( Gia_Man_t * p, Gia_Obj_t * pMux, Gia_Obj_t * pXor, int pIns[3] )
{
    Gia_Obj_t * pFan0, * pFan1;
    Gia_Obj_t * pDat0, * pDat1, * pCtrl;
    if ( !Gia_ObjIsMuxType(pMux) || !Gia_ObjIsMuxType(pXor) )
        return 0;
    if ( !Gia_ObjRecognizeExor( pXor, &pFan0, &pFan1 ) )
        return 0;
    pFan0 = Gia_Regular(pFan0);
    pFan1 = Gia_Regular(pFan1);
    if ( Gia_ObjId(p, pFan0) > Gia_ObjId(p, pFan1) )
        ABC_SWAP( Gia_Obj_t *, pFan0, pFan1 );
    if ( !(pCtrl = Gia_ObjRecognizeMux( pMux, &pDat0, &pDat1 )) )
        return 0;
    pDat0 = Gia_Regular(pDat0);
    pDat1 = Gia_Regular(pDat1);
    pCtrl = Gia_Regular(pCtrl);
    if ( !Gia_ObjIsAnd(pDat0) || !Gia_ObjIsAnd(pDat1) )
        return 0;
    if ( Gia_ObjFaninId0p(p, pDat0) != Gia_ObjFaninId0p(p, pDat1) ||
         Gia_ObjFaninId1p(p, pDat0) != Gia_ObjFaninId1p(p, pDat1) )
         return 0;
    if ( Gia_ObjFaninId0p(p, pDat0) != Gia_ObjId(p, pFan0) ||
         Gia_ObjFaninId1p(p, pDat0) != Gia_ObjId(p, pFan1) )
         return 0;
    pIns[0] = Gia_ObjId(p, pFan0);
    pIns[1] = Gia_ObjId(p, pFan1);
    pIns[2] = Gia_ObjId(p, pCtrl);
    return 1;
}
int Acec_DetectBoothXorFanin( Gia_Man_t * p, Gia_Obj_t * pObj, int pIns[5] )
{
    Gia_Obj_t * pFan0, * pFan1;
    //int Id = Gia_ObjId(p, pObj);
    if ( !Gia_ObjIsAnd(pObj) )
        return 0;
    if ( !Gia_ObjFaninC0(pObj) || !Gia_ObjFaninC1(pObj) )
        return 0;
    pFan0 = Gia_ObjFanin0(pObj);
    pFan1 = Gia_ObjFanin1(pObj);
    if ( !Gia_ObjIsAnd(pFan0) || !Gia_ObjIsAnd(pFan1) )
        return 0;
    if ( Acec_DetectBoothXorMux(p, Gia_ObjFanin0(pFan0), Gia_ObjFanin0(pFan1), pIns) )
    {
        pIns[3] = Gia_ObjId(p, Gia_ObjFanin1(pFan0));
        pIns[4] = Gia_ObjId(p, Gia_ObjFanin1(pFan1));
        return 1;
    }
    if ( Acec_DetectBoothXorMux(p, Gia_ObjFanin0(pFan0), Gia_ObjFanin1(pFan1), pIns) )
    {
        pIns[3] = Gia_ObjId(p, Gia_ObjFanin1(pFan0));
        pIns[4] = Gia_ObjId(p, Gia_ObjFanin0(pFan1));
        return 1;
    }
    if ( Acec_DetectBoothXorMux(p, Gia_ObjFanin1(pFan0), Gia_ObjFanin0(pFan1), pIns) )
    {
        pIns[3] = Gia_ObjId(p, Gia_ObjFanin0(pFan0));
        pIns[4] = Gia_ObjId(p, Gia_ObjFanin1(pFan1));
        return 1;
    }
    if ( Acec_DetectBoothXorMux(p, Gia_ObjFanin1(pFan0), Gia_ObjFanin1(pFan1), pIns) )
    {
        pIns[3] = Gia_ObjId(p, Gia_ObjFanin0(pFan0));
        pIns[4] = Gia_ObjId(p, Gia_ObjFanin0(pFan1));
        return 1;
    }
    return 0;
}
int Acec_DetectBoothOne( Gia_Man_t * p, Gia_Obj_t * pObj, int pIns[5] )
{
    Gia_Obj_t * pFan0, * pFan1;
    if ( !Gia_ObjRecognizeExor( pObj, &pFan0, &pFan1 ) )
        return 0;
    pFan0 = Gia_Regular(pFan0);
    pFan1 = Gia_Regular(pFan1);
    if ( Acec_DetectBoothXorFanin( p, pFan0, pIns ) && pIns[2] == Gia_ObjId(p, pFan1) )
        return 1;
    if ( Acec_DetectBoothXorFanin( p, pFan1, pIns ) && pIns[2] == Gia_ObjId(p, pFan0) )
        return 1;
    return 0;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Acec_DetectBoothTwoXor( Gia_Man_t * p, Gia_Obj_t * pObj, int pIns[5] )
{
    Gia_Obj_t * pFan0, * pFan1;
    if ( !Gia_ObjIsAnd(pObj) )
        return 0;
    if ( Gia_ObjRecognizeExor( Gia_ObjFanin0(pObj), &pFan0, &pFan1 ) )
    {
        pIns[0] = Gia_ObjId(p, Gia_Regular(pFan0));
        pIns[1] = Gia_ObjId(p, Gia_Regular(pFan1));
        pIns[2] = -1;
        pIns[3] = 0;
        pIns[4] = Gia_ObjId(p, Gia_ObjFanin1(pObj));
        return 1;
    }
    if ( Gia_ObjRecognizeExor( Gia_ObjFanin1(pObj), &pFan0, &pFan1 ) )
    {
        pIns[0] = Gia_ObjId(p, Gia_Regular(pFan0));
        pIns[1] = Gia_ObjId(p, Gia_Regular(pFan1));
        pIns[2] = -1;
        pIns[3] = 0;
        pIns[4] = Gia_ObjId(p, Gia_ObjFanin0(pObj));
        return 1;
    }
    return 0;
}
int Acec_DetectBoothTwo( Gia_Man_t * p, Gia_Obj_t * pObj, int pIns[5] )
{
    Gia_Obj_t * pFan0, * pFan1;
    if ( !Gia_ObjRecognizeExor( pObj, &pFan0, &pFan1 ) )
        return 0;
    pFan0 = Gia_Regular(pFan0);
    pFan1 = Gia_Regular(pFan1);
    if ( Acec_DetectBoothTwoXor( p, pFan0, pIns ) )
    {
        pIns[2] = Gia_ObjId(p, pFan1);
        return 1;
    }
    if ( Acec_DetectBoothTwoXor( p, pFan1, pIns ) )
    {
        pIns[2] = Gia_ObjId(p, pFan0);
        return 1;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Acec_DetectBoothTest( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i, pIns[5];
    Gia_ManForEachAnd( p, pObj, i )
    {        
        if ( !Acec_DetectBoothOne(p, pObj, pIns) && !Acec_DetectBoothTwo(p, pObj, pIns) )
                continue;
        printf( "obj = %4d  :  b0 = %4d  b1 = %4d  b2 = %4d    a0 = %4d  a1 = %4d\n", 
            i, pIns[0], pIns[1], pIns[2], pIns[3], pIns[4] );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManResubTest4()
{
    Vec_Int_t * vRes = Vec_IntAlloc( 100 );
    unsigned T = 0xF335ACC0;
    int a, b, c;
    int i, k, f, y;
    int Count = 0;
    for ( a = 0; a < 2; a++ )
    {
        unsigned A = s_Truths5[a];
        for ( b = 0; b < 3; b++ )
        {
            unsigned B = s_Truths5[2+b];
            for ( c = 0; c < 3; c++ ) if ( c != b )
            {
                unsigned C = s_Truths5[2+c];
                Vec_IntPush( vRes, A & B &  C );
                Vec_IntPush( vRes, A & B & ~C );
            }
        }
    }
    printf( "Size = %d.\n", Vec_IntSize(vRes) );
    for ( i = 0; i < (1 << Vec_IntSize(vRes)); i++ )
    {
        unsigned F[7] = {0};
        unsigned Y[3] = {0};
        if ( Abc_TtCountOnes( (word)i ) >= 8 )
            continue;
        for ( f = k = 0; k < Vec_IntSize(vRes); k++ )
            if ( ((i >> k) & 1) )
                F[f++] = Vec_IntEntry(vRes, k);
        {
            unsigned S1 = (F[0] & F[1]) | (F[0] & F[2]) | (F[1] & F[2]);
            unsigned C1 = F[0] ^ F[1] ^ F[2];
            unsigned S2 = (F[3] & F[4]) | (F[3] & F[5]) | (F[4] & F[5]);
            unsigned C2 = F[3] ^ F[4] ^ F[5];
            unsigned S3 = (F[6] & S1) | (F[6] & S2) | (S1 & S2);
            unsigned C3 = F[6] ^ S1 ^ S2;
            unsigned S4 = (C1 & C2) | (C1 & C3) | (C2 & C3);
            unsigned C4 = C1 ^ C2 ^ C3;
            Y[0] = S3;
            Y[1] = S4;
            Y[2] = C4;
        }
        for ( y = 0; y < 3; y++ )
            if ( Y[y] == T )
                printf( "Found!\n" );
        Count++;
    }
    printf( "Tried = %d.\n", Count );
    Vec_IntFree( vRes );
}
void Gia_ManResubTest5()
{
    unsigned T = 0xF335ACC0;
    int i;
    for ( i = 0; i < 4; i++ )
    {
        unsigned x = i%2 ? Abc_Tt5Cofactor1(T, 0) : Abc_Tt5Cofactor0(T, 0);
        unsigned y = i/2 ? Abc_Tt5Cofactor1(x, 1) : Abc_Tt5Cofactor0(x, 1);
        word F = y;
        F |= F << 32;
        //Dau_DsdPrintFromTruth2( &F, 6 ); printf( "\n" );
    }
}
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

