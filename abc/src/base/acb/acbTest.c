/**CFile****************************************************************

  FileName    [acbTest.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchical word-level netlist.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 21, 2015.]

  Revision    [$Id: acbTest.c,v 1.00 2014/11/29 00:00:00 alanmi Exp $]

***********************************************************************/

#include "acb.h"
#include "aig/saig/saig.h"
#include "aig/gia/giaAig.h"
#include "base/abc/abc.h"
#include "proof/fraig/fraig.h"
#include "misc/util/utilTruth.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int fForceZero = 0;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSimTry( Gia_Man_t * pF, Gia_Man_t * pG )
{
    int i, j, n, nWords = 500;
    Vec_Wrd_t * vSimsF, * vSimsG;
    Abc_Random(1);
    Vec_WrdFreeP( &pF->vSimsPi );
    Vec_WrdFreeP( &pG->vSimsPi );
    pF->vSimsPi = Vec_WrdStartRandom( Gia_ManCiNum(pF) * nWords );
    pG->vSimsPi = Vec_WrdDup( pF->vSimsPi );
    vSimsF = Gia_ManSimPatSim( pF );
    vSimsG = Gia_ManSimPatSim( pG );
    assert( Gia_ManObjNum(pF) * nWords == Vec_WrdSize(vSimsF) );
    for ( i = 0; i < Gia_ManCoNum(pF)/2; i++ )
    {
        Gia_Obj_t * pObjFb = Gia_ManCo( pF, 2*i+0 );
        Gia_Obj_t * pObjFx = Gia_ManCo( pF, 2*i+1 );
        Gia_Obj_t * pObjGb = Gia_ManCo( pG, 2*i+0 );
        Gia_Obj_t * pObjGx = Gia_ManCo( pG, 2*i+1 );
        word * pSimFb = Vec_WrdEntryP(vSimsF, Gia_ObjId(pF, pObjFb)*nWords);
        word * pSimFx = Vec_WrdEntryP(vSimsF, Gia_ObjId(pF, pObjFx)*nWords);
        word * pSimGb = Vec_WrdEntryP(vSimsG, Gia_ObjId(pG, pObjGb)*nWords);
        word * pSimGx = Vec_WrdEntryP(vSimsG, Gia_ObjId(pG, pObjGx)*nWords);

        int nBitsFx = Abc_TtCountOnesVec(pSimFx, nWords);
        int nBitsF1 = Abc_TtCountOnesVecMask(pSimFx, pSimFb, nWords, 1);
        int nBitsF0 = nWords*64 - nBitsFx - nBitsF1;
        
        int nBitsGx = Abc_TtCountOnesVec(pSimGx, nWords);
        int nBitsG1 = Abc_TtCountOnesVecMask(pSimGx, pSimGb, nWords, 1);
        int nBitsG0 = nWords*64 - nBitsGx - nBitsG1;
        
        printf( "Output %4d : ", i );

        printf( "    RF :  " );
        printf( "0 =%7.3f %%  ",    100.0*nBitsF0/64/nWords );
        printf( "1 =%7.3f %%  ",    100.0*nBitsF1/64/nWords );
        printf( "X =%7.3f %%  ",    100.0*nBitsFx/64/nWords );

        printf( "   GF :  " );
        printf( "0 =%7.3f %%  ",    100.0*nBitsG0/64/nWords );
        printf( "1 =%7.3f %%  ",    100.0*nBitsG1/64/nWords );
        printf( "X =%7.3f %%  ",    100.0*nBitsGx/64/nWords );

        printf( "\n" );
        if ( i == 20 )
            break;
    }

    printf( "\n" );
    for ( j = 0; j < 20; j++ )
    {
        for ( n = 0; n < 2; n++ )
        {
            for ( i = 0; i < Gia_ManCoNum(pF)/2; i++ )
            {
                Gia_Obj_t * pObjFb = Gia_ManCo( pF, 2*i+0 );
                Gia_Obj_t * pObjFx = Gia_ManCo( pF, 2*i+1 );
                Gia_Obj_t * pObjGb = Gia_ManCo( pG, 2*i+0 );
                Gia_Obj_t * pObjGx = Gia_ManCo( pG, 2*i+1 );
                word * pSimFb = Vec_WrdEntryP(vSimsF, Gia_ObjId(pF, pObjFb)*nWords);
                word * pSimFx = Vec_WrdEntryP(vSimsF, Gia_ObjId(pF, pObjFx)*nWords);
                word * pSimGb = Vec_WrdEntryP(vSimsG, Gia_ObjId(pG, pObjGb)*nWords);
                word * pSimGx = Vec_WrdEntryP(vSimsG, Gia_ObjId(pG, pObjGx)*nWords);
                word * pSimb = n ? pSimGb : pSimFb;
                word * pSimx = n ? pSimGx : pSimFx;
                if ( Abc_TtGetBit(pSimx, j) )
                    printf( "x" );
                else if ( Abc_TtGetBit(pSimb, j) )
                    printf( "1" );
                else
                    printf( "0" );
            }
            printf( "\n" );
        }
        printf( "\n" );
    }

    Vec_WrdFree( vSimsF );
    Vec_WrdFree( vSimsG );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDualNot( Gia_Man_t * p, int LitA[2], int LitZ[2] )
{
    LitZ[0]   = Abc_LitNot(LitA[0]);
    LitZ[1]   = LitA[1];
    
    if ( fForceZero ) LitZ[0]   = Gia_ManHashAnd( p, LitZ[0], Abc_LitNot(LitZ[1]) );
}
// computes Z = XOR(A, B) where A, B, Z belong to {0,1,x} encoded as 0=00, 1=01, x=1-
void Gia_ManDualXor2( Gia_Man_t * p, int LitA[2], int LitB[2], int LitZ[2] )
{
    LitZ[0]   = Gia_ManHashXor( p, LitA[0], LitB[0] );
    LitZ[1]   = Gia_ManHashOr( p, LitA[1], LitB[1] );
    
    if ( fForceZero ) LitZ[0]   = Gia_ManHashAnd( p, LitZ[0], Abc_LitNot(LitZ[1]) );
}
void Gia_ManDualXorN( Gia_Man_t * p, int * pLits, int n, int LitZ[2] )
{
    int i;
    LitZ[0] = 0;
    LitZ[1] = 0;
    for ( i = 0; i < n; i++ )
    {
        LitZ[0] = Gia_ManHashXor( p, LitZ[0], pLits[2*i] );
        LitZ[1] = Gia_ManHashOr ( p, LitZ[1], pLits[2*i+1] );
    }
}
// computes Z = AND(A, B) where A, B, Z belong to {0,1,x} encoded as 0=00, 1=01, z=1-
void Gia_ManDualAnd2( Gia_Man_t * p, int LitA[2], int LitB[2], int LitZ[2] )
{
    int ZeroA = Gia_ManHashAnd( p, Abc_LitNot(LitA[0]), Abc_LitNot(LitA[1]) );
    int ZeroB = Gia_ManHashAnd( p, Abc_LitNot(LitB[0]), Abc_LitNot(LitB[1]) );
    int ZeroZ = Gia_ManHashOr( p, ZeroA, ZeroB );
    LitZ[0]   = Gia_ManHashAnd( p, LitA[0], LitB[0] );
    LitZ[1]   = Gia_ManHashAnd( p, Gia_ManHashOr( p, LitA[1], LitB[1] ), Abc_LitNot(ZeroZ) );

    //LitZ[0]   = Gia_ManHashAnd( p, Gia_ManHashAnd(p, LitA[0], Abc_LitNot(LitA[1])), Gia_ManHashAnd(p, LitB[0], Abc_LitNot(LitB[1])) );
    //LitZ[1]   = Gia_ManHashAnd( p, Gia_ManHashOr(p, LitA[0], LitA[1]), Gia_ManHashOr(p, LitB[0], LitB[1]) );
    //LitZ[1]   = Gia_ManHashAnd( p, LitZ[1], Abc_LitNot(LitZ[0]) );
}
void Gia_ManDualAndN( Gia_Man_t * p, int * pLits, int n, int LitZ[2] )
{
    int i, LitZero = 0, LitOne = 0;
    LitZ[0] = 1;
    for ( i = 0; i < n; i++ )
    {
        int Lit = Gia_ManHashAnd( p, Abc_LitNot(pLits[2*i]), Abc_LitNot(pLits[2*i+1]) );
        LitZero = Gia_ManHashOr( p, LitZero, Lit );
        LitOne  = Gia_ManHashOr( p, LitOne,  pLits[2*i+1] );
        LitZ[0] = Gia_ManHashAnd( p, LitZ[0], pLits[2*i] );
    }
    LitZ[1] = Gia_ManHashAnd( p, LitOne, Abc_LitNot(LitZero) );
    
    if ( fForceZero ) LitZ[0]   = Gia_ManHashAnd( p, LitZ[0], Abc_LitNot(LitZ[1]) );
}
/*
module _DC(O, C, D);
 output O;
 input C, D;
 assign O = D ? 1'bx : C;
endmodule
*/
void Gia_ManDualDc( Gia_Man_t * p, int LitC[2], int LitD[2], int LitZ[2] )
{
    LitZ[0]   = LitC[0];
//    LitZ[0]   = Gia_ManHashMux( p, LitD[0], 0, LitC[0] );
    LitZ[1]   = Gia_ManHashOr(p, Gia_ManHashOr(p,LitD[0],LitD[1]), LitC[1] );
    
    if ( fForceZero ) LitZ[0]   = Gia_ManHashAnd( p, LitZ[0], Abc_LitNot(LitZ[1]) );
}
void Gia_ManDualMux( Gia_Man_t * p, int LitC[2], int LitT[2], int LitE[2], int LitZ[2] )
{
/*
    // total logic size: 18 nodes
    int Xnor = Gia_ManHashXor( p, Abc_LitNot(LitT[0]), LitE[0] );
    int Cond = Gia_ManHashAnd( p, Abc_LitNot(LitT[1]), Abc_LitNot(LitE[1]) );
    int pTempE[2], pTempT[2];
    pTempE[0] = Gia_ManHashMux( p, LitC[0], LitT[0], LitE[0] );
    pTempE[1] = Gia_ManHashMux( p, LitC[0], LitT[1], LitE[1] );
    //pTempT[0] = LitT[0];
    pTempT[0] = Gia_ManHashAnd( p, LitT[0], LitE[0] );
    pTempT[1] = Gia_ManHashAnd( p, Cond, Xnor );
    LitZ[0] = Gia_ManHashMux( p, LitC[1], pTempT[0], pTempE[0] );
    LitZ[1] = Gia_ManHashMux( p, LitC[1], pTempT[1], pTempE[1] );
*/
    // total logic size: 14 nodes
    int Xnor = Gia_ManHashXor( p, Abc_LitNot(LitT[0]), LitE[0] );
    int Cond = Gia_ManHashAnd( p, Abc_LitNot(LitT[1]), Abc_LitNot(LitE[1]) );
    int XVal1 = Abc_LitNot( Gia_ManHashAnd( p, Cond, Xnor ) );
    int XVal0 = Gia_ManHashMux( p, LitC[0], LitT[1], LitE[1] );
    LitZ[0] = Gia_ManHashMux( p, LitC[0], LitT[0], LitE[0] );
    LitZ[1] = Gia_ManHashMux( p, LitC[1], XVal1, XVal0 );

    if ( fForceZero ) LitZ[0]   = Gia_ManHashAnd( p, LitZ[0], Abc_LitNot(LitZ[1]) );
}
int Gia_ManDualCompare( Gia_Man_t * p, int LitF[2], int LitS[2] )
{
    int iMiter = Gia_ManHashXor( p, LitF[0], LitS[0] );
    iMiter = Gia_ManHashOr( p, LitF[1], iMiter );
    iMiter = Gia_ManHashAnd( p, Abc_LitNot(LitS[1]), iMiter );
    return iMiter;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Acb_ObjToGiaDual( Gia_Man_t * pNew, Acb_Ntk_t * p, int iObj, Vec_Int_t * vTemp, Vec_Int_t * vCopies, int pRes[2] )
{
    //char * pName = Abc_NamStr( p->pDesign->pStrs, Acb_ObjName(p, iObj) );
    int * pFanin, iFanin, k, Type;
    assert( !Acb_ObjIsCio(p, iObj) );
    Vec_IntClear( vTemp );
    Acb_ObjForEachFaninFast( p, iObj, pFanin, iFanin, k )
    {
        int * pLits = Vec_IntEntryP( vCopies, 2*iFanin );
        assert( pLits[0] >= 0 && pLits[1] >= 0 );
        Vec_IntPushTwo( vTemp, pLits[0], pLits[1] );
    }
    Type = Acb_ObjType( p, iObj );
    if ( Type == ABC_OPER_CONST_F ) 
    {
        pRes[0] = 0;
        pRes[1] = 0;
        return;
    }
    if ( Type == ABC_OPER_CONST_T ) 
    {
        pRes[0] = 1;
        pRes[1] = 0;
        return;
    }
    if ( Type == ABC_OPER_CONST_X ) 
    {
        pRes[0] = 0;
        pRes[1] = 1;
        return;
    }
    if ( Type == ABC_OPER_BIT_BUF ) 
    {
        pRes[0] = Vec_IntEntry(vTemp, 0);
        pRes[1] = Vec_IntEntry(vTemp, 1);
        return;
    }
    if ( Type == ABC_OPER_BIT_INV ) 
    {
        Gia_ManDualNot( pNew, Vec_IntArray(vTemp), pRes );
        return;
    }
    if ( Type == ABC_OPER_TRI ) 
    {
        // in the file inputs are ordered as follows:  _DC \n6_5[9] ( .O(\108 ), .C(\96 ), .D(\107 ));
        // in this code, we expect them as follows: void Gia_ManDualDc( Gia_Man_t * p, int LitC[2], int LitD[2], int LitZ[2] )
        assert( Vec_IntSize(vTemp) == 4 );
        Gia_ManDualDc( pNew, Vec_IntArray(vTemp), Vec_IntArray(vTemp) + 2, pRes );
        return;
    }
    if ( Type == ABC_OPER_BIT_MUX ) 
    {
        // in the file inputs are ordered as follows:  _HMUX \U$1 ( .O(\282 ), .I0(1'b1), .I1(\277 ), .S(\281 ));
        // in this code, we expect them as follows: void Gia_ManDualMux( Gia_Man_t * p, int LitC[2], int LitT[2], int LitE[2], int LitZ[2] )
        assert( Vec_IntSize(vTemp) == 6 );
        ABC_SWAP( int, Vec_IntArray(vTemp)[0], Vec_IntArray(vTemp)[4] );
        ABC_SWAP( int, Vec_IntArray(vTemp)[1], Vec_IntArray(vTemp)[5] );
        Gia_ManDualMux( pNew, Vec_IntArray(vTemp), Vec_IntArray(vTemp) + 2, Vec_IntArray(vTemp) + 4, pRes );
        return;
    }
    if ( Type == ABC_OPER_BIT_AND || Type == ABC_OPER_BIT_NAND )
    {
        Gia_ManDualAndN( pNew, Vec_IntArray(vTemp), Vec_IntSize(vTemp)/2, pRes );
        if ( Type == ABC_OPER_BIT_NAND )
            pRes[0] = Abc_LitNot( pRes[0] );
        return;
    }
    if ( Type == ABC_OPER_BIT_OR || Type == ABC_OPER_BIT_NOR )
    {
        int * pArray = Vec_IntArray( vTemp );
        for ( k = 0; k < Vec_IntSize(vTemp)/2; k++ )
            pArray[2*k] = Abc_LitNot( pArray[2*k] );
        Gia_ManDualAndN( pNew, pArray, Vec_IntSize(vTemp)/2, pRes );
        if ( Type == ABC_OPER_BIT_OR )
            pRes[0] = Abc_LitNot( pRes[0] );
        return;
    }
    if ( Type == ABC_OPER_BIT_XOR || Type == ABC_OPER_BIT_NXOR )
    {
        assert( Vec_IntSize(vTemp) == 4 );
        Gia_ManDualXor2( pNew, Vec_IntArray(vTemp), Vec_IntArray(vTemp) + 2, pRes );
        if ( Type == ABC_OPER_BIT_NXOR )
            pRes[0] = Abc_LitNot( pRes[0] );
        return;
    }
    assert( 0 );
}
Gia_Man_t * Acb_NtkGiaDeriveDual( Acb_Ntk_t * p )
{
    extern Vec_Int_t * Acb_NtkFindNodes2( Acb_Ntk_t * p );
    Gia_Man_t * pNew, * pOne;
    Vec_Int_t * vFanins, * vNodes;
    Vec_Int_t * vCopies = Vec_IntStartFull( 2*Acb_NtkObjNum(p) );
    int i, iObj, * pLits;
    pNew = Gia_ManStart( 5 * Acb_NtkObjNum(p) );
    pNew->pName = Abc_UtilStrsav(Acb_NtkName(p));
    Gia_ManHashAlloc( pNew );
    pLits = Vec_IntEntryP( vCopies, 0 );
    pLits[0] = 0;
    pLits[1] = 0;
    Acb_NtkForEachCi( p, iObj, i )
    {
        pLits = Vec_IntEntryP( vCopies, 2*iObj );
        pLits[0] = Gia_ManAppendCi(pNew);
        pLits[1] = 0;
    }
    vFanins = Vec_IntAlloc( 4 );
    vNodes  = Acb_NtkFindNodes2( p );
    Vec_IntForEachEntry( vNodes, iObj, i )
    {
        pLits = Vec_IntEntryP( vCopies, 2*iObj );
        Acb_ObjToGiaDual( pNew, p, iObj, vFanins, vCopies, pLits );
    }
    Vec_IntFree( vNodes );
    Vec_IntFree( vFanins );
    Acb_NtkForEachCo( p, iObj, i )
    {
        pLits = Vec_IntEntryP( vCopies, 2*Acb_ObjFanin(p, iObj, 0) );
        Gia_ManAppendCo( pNew, pLits[0] );
        Gia_ManAppendCo( pNew, pLits[1] );
    }
    Vec_IntFree( vCopies );
    pNew = Gia_ManCleanup( pOne = pNew );
    Gia_ManStop( pOne );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Acb_NtkGiaDeriveMiter( Gia_Man_t * pOne, Gia_Man_t * pTwo, int Type )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    int i;
    assert( Gia_ManCiNum(pOne) == Gia_ManCiNum(pTwo) );
    assert( Gia_ManCoNum(pOne) == Gia_ManCoNum(pTwo) );
    pNew = Gia_ManStart( Gia_ManObjNum(pOne) + Gia_ManObjNum(pTwo) + 5*Gia_ManCoNum(pOne)/2 );
    pNew->pName = Abc_UtilStrsav( "miter" );
    pNew->pSpec = NULL;
    Gia_ManHashAlloc( pNew );
    Gia_ManConst0(pOne)->Value = 0;
    Gia_ManConst0(pTwo)->Value = 0;
    Gia_ManForEachCi( pOne, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachCi( pTwo, pObj, i )
        pObj->Value = Gia_ManCi(pOne, i)->Value;
    Gia_ManForEachAnd( pOne, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachAnd( pTwo, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachCo( pOne, pObj, i )
        pObj->Value = Gia_ObjFanin0Copy(pObj);
    Gia_ManForEachCo( pTwo, pObj, i )
        pObj->Value = Gia_ObjFanin0Copy(pObj);
    if ( Type == 0 ) // only main circuit
    {
        for ( i = 0; i < Gia_ManCoNum(pOne); i += 2 )
        {
            int pLitsF[2] = { Gia_ManCo(pOne, i)->Value, Gia_ManCo(pOne, i+1)->Value };
            int pLitsS[2] = { Gia_ManCo(pTwo, i)->Value, Gia_ManCo(pTwo, i+1)->Value };
            Gia_ManAppendCo( pNew, pLitsF[0] );
            Gia_ManAppendCo( pNew, pLitsS[0] );
        }
    }
    else if ( Type == 1 ) // only shadow circuit
    {
        for ( i = 0; i < Gia_ManCoNum(pOne); i += 2 )
        {
            int pLitsF[2] = { Gia_ManCo(pOne, i)->Value, Gia_ManCo(pOne, i+1)->Value };
            int pLitsS[2] = { Gia_ManCo(pTwo, i)->Value, Gia_ManCo(pTwo, i+1)->Value };
            Gia_ManAppendCo( pNew, pLitsF[1] );
            Gia_ManAppendCo( pNew, pLitsS[1] );
        }
    }
    else // comparator of the two
    {
        for ( i = 0; i < Gia_ManCoNum(pOne); i += 2 )
        {
            int pLitsF[2] = { Gia_ManCo(pOne, i)->Value, Gia_ManCo(pOne, i+1)->Value };
            int pLitsS[2] = { Gia_ManCo(pTwo, i)->Value, Gia_ManCo(pTwo, i+1)->Value };
            Gia_ManAppendCo( pNew, Gia_ManDualCompare( pNew, pLitsF, pLitsS ) );
        }
    }
    Gia_ManHashStop( pNew );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Acb_OutputFile( char * pFileName, Acb_Ntk_t * pNtkF, int * pModel )
{
    const char * pFileName0 = pFileName? pFileName : "output";
    FILE * pFile = fopen( pFileName0, "wb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open results file \"%s\".\n", pFileName0 );
        return;
    }
    if ( pModel == NULL )
        fprintf( pFile, "EQ\n" );
    else 
    {
        /*
        NEQ
        in 1
        a 1
        b 0
        */
        int i, iObj;
        fprintf( pFile, "NEQ\n" );
        Acb_NtkForEachPi( pNtkF, iObj, i )
            fprintf( pFile, "%s %d\n", Acb_ObjNameStr(pNtkF, iObj), pModel[i] );
    }
    fclose( pFile );
    printf( "Produced output file \"%s\".\n\n", pFileName0 );
}
int * Acb_NtkSolve( Gia_Man_t * p )
{
    extern Abc_Ntk_t * Abc_NtkFromAigPhase( Aig_Man_t * pMan );
    Aig_Man_t * pMan = Gia_ManToAig( p, 0 );
    Abc_Ntk_t * pNtkTemp = Abc_NtkFromAigPhase( pMan );
    Prove_Params_t Params, * pParams = &Params;
    Prove_ParamsSetDefault( pParams );
    pParams->fUseRewriting = 1;
    pParams->fVerbose      = 0;
    Aig_ManStop( pMan );
    if ( pNtkTemp )
    {
        abctime clk = Abc_Clock();
        int RetValue = Abc_NtkIvyProve( &pNtkTemp, pParams );
        int * pModel = pNtkTemp->pModel;
        pNtkTemp->pModel = NULL;
        Abc_NtkDelete( pNtkTemp );
        printf( "The networks are %s.  ", RetValue == 1 ? "equivalent" : (RetValue == 0 ? "NOT equivalent" : "UNDECIDED") );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
        if ( RetValue == 0 )
            return pModel;
    }
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Various statistics.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Acb_NtkPrintCecStats( Acb_Ntk_t * pNtk )
{
    int iObj, nDcs = 0, nMuxes = 0;
    Acb_NtkForEachNode( pNtk, iObj )
        if ( Acb_ObjType( pNtk, iObj ) == ABC_OPER_TRI )
            nDcs++;
        else if ( Acb_ObjType( pNtk, iObj ) == ABC_OPER_BIT_MUX )
            nMuxes++;

    printf( "PI = %6d  ",  Acb_NtkCiNum(pNtk) );
    printf( "PO = %6d  ",  Acb_NtkCoNum(pNtk) );
    printf( "Obj = %6d  ", Acb_NtkObjNum(pNtk) );
    printf( "DC = %4d  ",  nDcs );
    printf( "Mux = %4d  ", nMuxes );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Changing the PI order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Acb_NtkUpdateCiOrder( Acb_Ntk_t * pNtkF, Acb_Ntk_t * pNtkG )
{
    int i, iObj;
    Vec_Int_t * vMap = Vec_IntStartFull( Acb_ManNameIdMax(pNtkG->pDesign) );
    Vec_Int_t * vOrder = Vec_IntStartFull( Acb_NtkCiNum(pNtkG) );
    Acb_NtkForEachCi( pNtkG, iObj, i )
        Vec_IntWriteEntry( vMap, Acb_ObjName(pNtkG, iObj), i );
    Acb_NtkForEachCi( pNtkF, iObj, i )
    {
        int NameIdG = Acb_ManStrId( pNtkG->pDesign, Acb_ObjNameStr(pNtkF, iObj) );
        int iPerm = NameIdG < Vec_IntSize(vMap) ? Vec_IntEntry( vMap, NameIdG ) : -1;
        if ( iPerm == -1 )
            printf( "Cannot find name \"%s\" of PI %d of F among PIs of G.\n", Acb_ObjNameStr(pNtkF, iObj), i );
        else
            Vec_IntWriteEntry( vOrder, iPerm, iObj );
    }
    Vec_IntClear( &pNtkF->vCis );
    Vec_IntAppend( &pNtkF->vCis, vOrder );
    Vec_IntFree( vOrder );
    Vec_IntFree( vMap );
}
int Acb_NtkCheckPiOrder( Acb_Ntk_t * pNtkF, Acb_Ntk_t * pNtkG )
{
    int i, nPis = Acb_NtkCiNum(pNtkF);
    for ( i = 0; i < nPis; i++ )
    {
        char * pNameF = Acb_ObjNameStr( pNtkF, Acb_NtkCi(pNtkF, i) );
        char * pNameG = Acb_ObjNameStr( pNtkG, Acb_NtkCi(pNtkG, i) );
        if ( strcmp(pNameF, pNameG) )
        {
//            printf( "PI %d has different names (%s and %s) in these networks.\n", i, pNameF, pNameG );
            printf( "Networks have different PI names. Reordering PIs of the implementation network.\n" );
            Acb_NtkUpdateCiOrder( pNtkF, pNtkG );
            break;
        }
    }
    if ( i == nPis )
        printf( "Networks have the same PI names.\n" );
    return i == nPis;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Acb_NtkRunTest( char * pFileNames[4], int fFancy, int fVerbose )
{
    extern Acb_Ntk_t * Acb_VerilogSimpleRead( char * pFileName, char * pFileNameW );
    extern void        Gia_AigerWrite( Gia_Man_t * p, char * pFileName, int fWriteSymbols, int fCompact, int fWriteNewLine );

    int fSolve = 1;
    int * pModel = NULL;
    Gia_Man_t * pGiaF = NULL;
    Gia_Man_t * pGiaG = NULL;
    Gia_Man_t * pGia  = NULL;
    Acb_Ntk_t * pNtkF = Acb_VerilogSimpleRead( pFileNames[0], NULL );
    Acb_Ntk_t * pNtkG = Acb_VerilogSimpleRead( pFileNames[1], NULL );
    if ( !pNtkF || !pNtkG )
        return;
        
    assert( Acb_NtkCiNum(pNtkF) == Acb_NtkCiNum(pNtkG) );
    assert( Acb_NtkCoNum(pNtkF) == Acb_NtkCoNum(pNtkG) );

    Acb_NtkCheckPiOrder( pNtkF, pNtkG );
    //Acb_NtkCheckPiOrder( pNtkG, pNtkF );
    Acb_NtkPrintCecStats( pNtkF );
    Acb_NtkPrintCecStats( pNtkG );

    pGiaF = Acb_NtkGiaDeriveDual( pNtkF );
    pGiaG = Acb_NtkGiaDeriveDual( pNtkG );
    pGia  = Acb_NtkGiaDeriveMiter( pGiaF, pGiaG, 2 );
    //Gia_AigerWrite( pGiaF, Extra_FileNameGenericAppend(pFileNames[1], "_f2.aig"), 0, 0, 0 );
    //Gia_AigerWrite( pGiaG, Extra_FileNameGenericAppend(pFileNames[1], "_g2.aig"), 0, 0, 0 );
    //Gia_AigerWrite( pGia,  Extra_FileNameGenericAppend(pFileNames[1], "_miter_0.aig"), 0, 0, 0 );
    //printf( "Written the miter info file \"%s\".\n", Extra_FileNameGenericAppend(pFileNames[1], "_miter_0.aig") );

    //Gia_ManPrintStats( pGia, NULL );
    //Gia_ManSimTry( pGiaF, pGiaG );

    if ( fSolve )
    {
        pModel = Acb_NtkSolve( pGia );
        Acb_OutputFile( pFileNames[2], pNtkF, pModel );
        ABC_FREE( pModel );
    }

    Gia_ManStop( pGia );
    Gia_ManStop( pGiaF );
    Gia_ManStop( pGiaG );

    Acb_ManFree( pNtkF->pDesign );
    Acb_ManFree( pNtkG->pDesign );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

