/**CFile****************************************************************

  FileName    [wlnBlast.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Word-level network.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 23, 2018.]

  Revision    [$Id: wlnBlast.c,v 1.00 2018/09/23 00:00:00 alanmi Exp $]

***********************************************************************/

#include "wln.h"
#include "base/wlc/wlc.h"

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
void Rtl_VecExtend( Vec_Int_t * p, int nRange, int fSigned )
{
    Vec_IntFillExtra( p, nRange, fSigned ? Vec_IntEntryLast(p) : 0 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rtl_NtkBlastNode( Gia_Man_t * pNew, int Type, int nIns, Vec_Int_t * vDatas, int nRange, int fSign0, int fSign1 )
{
    extern void Wlc_BlastMinus( Gia_Man_t * pNew, int * pNum, int nNum, Vec_Int_t * vRes );
    extern int  Wlc_BlastReduction( Gia_Man_t * pNew, int * pFans, int nFans, int Type );
    extern int  Wlc_BlastLess( Gia_Man_t * pNew, int * pArg0, int * pArg1, int nBits );
    extern int  Wlc_BlastLessSigned( Gia_Man_t * pNew, int * pArg0, int * pArg1, int nBits );
    extern void Wlc_BlastShiftRight( Gia_Man_t * pNew, int * pNum, int nNum, int * pShift, int nShift, int fSticky, Vec_Int_t * vRes );
    extern void Wlc_BlastShiftLeft( Gia_Man_t * pNew, int * pNum, int nNum, int * pShift, int nShift, int fSticky, Vec_Int_t * vRes );
    extern int  Wlc_BlastAdder( Gia_Man_t * pNew, int * pAdd0, int * pAdd1, int nBits, int Carry ); // result is in pAdd0
    extern void Wlc_BlastSubtract( Gia_Man_t * pNew, int * pAdd0, int * pAdd1, int nBits, int Carry ); // result is in pAdd0
    extern int  Wlc_NtkCountConstBits( int * pArray, int nSize );
    extern void Wlc_BlastBooth( Gia_Man_t * pNew, int * pArgA, int * pArgB, int nArgA, int nArgB, Vec_Int_t * vRes, int fSigned, int fCla, Vec_Wec_t ** pvProds );
    extern void Wlc_BlastMultiplier3( Gia_Man_t * pNew, int * pArgA, int * pArgB, int nArgA, int nArgB, Vec_Int_t * vRes, int fSigned, int fCla, Vec_Wec_t ** pvProds );
    extern void Wlc_BlastZeroCondition( Gia_Man_t * pNew, int * pDiv, int nDiv, Vec_Int_t * vRes );
    extern void Wlc_BlastDividerTop( Gia_Man_t * pNew, int * pNum, int nNum, int * pDiv, int nDiv, int fQuo, Vec_Int_t * vRes, int fNonRest );
    extern void Wlc_BlastDividerSigned( Gia_Man_t * pNew, int * pNum, int nNum, int * pDiv, int nDiv, int fQuo, Vec_Int_t * vRes, int fNonRest );
    extern void Wlc_BlastPower( Gia_Man_t * pNew, int * pNum, int nNum, int * pExp, int nExp, Vec_Int_t * vTemp, Vec_Int_t * vRes );

    int k, iLit, iLit0, iLit1;
    if ( nIns == 1 )
    {
        Vec_Int_t * vArg = vDatas;
        Vec_Int_t * vRes = vDatas+3;
        assert( Vec_IntSize(vRes) == 0 );
        if ( Type == ABC_OPER_BIT_INV )   // Y = ~A    $not  
        {
            assert( Vec_IntSize(vArg) == nRange );
            Vec_IntForEachEntry( vArg, iLit, k )
                Vec_IntPush( vRes, Abc_LitNot(iLit) );
            return;
        }
        if ( Type == ABC_OPER_BIT_BUF )   // Y = +A    $pos 
        {
            assert( Vec_IntSize(vArg) == nRange );
            Vec_IntForEachEntry( vArg, iLit, k )
                Vec_IntPush( vRes, iLit );
            return;
        }
        if ( Type == ABC_OPER_ARI_MIN )   // Y = -A    $neg 
        {
            assert( Vec_IntSize(vArg) == nRange );
            Wlc_BlastMinus( pNew, Vec_IntArray(vArg), Vec_IntSize(vArg), vRes );
            return;
        }
        if ( Type == ABC_OPER_RED_AND )   // Y = &A    $reduce_and
        {
            assert( nRange == 1 );
            Vec_IntPush( vRes, Wlc_BlastReduction( pNew, Vec_IntArray(vArg), Vec_IntSize(vArg), WLC_OBJ_REDUCT_AND ) );
            for ( k = 1; k < nRange; k++ )
                Vec_IntPush( vRes, 0 );
            return;
        }
        if ( Type == ABC_OPER_RED_OR )    // Y = |A    $reduce_or     $reduce_bool 
        {
            assert( nRange == 1 );
            Vec_IntPush( vRes, Wlc_BlastReduction( pNew, Vec_IntArray(vArg), Vec_IntSize(vArg), WLC_OBJ_REDUCT_OR ) );
            for ( k = 1; k < nRange; k++ )
                Vec_IntPush( vRes, 0 );
            return;
        }
        if ( Type == ABC_OPER_RED_XOR )   // Y = ^A    $reduce_xor  
        {
            assert( nRange == 1 );
            Vec_IntPush( vRes, Wlc_BlastReduction( pNew, Vec_IntArray(vArg), Vec_IntSize(vArg), WLC_OBJ_REDUCT_XOR ) );
            for ( k = 1; k < nRange; k++ )
                Vec_IntPush( vRes, 0 );
            return;
        }
        if ( Type == ABC_OPER_RED_NXOR )  // Y = ~^A   $reduce_xnor 
        {
            assert( nRange == 1 );
            Vec_IntPush( vRes, Wlc_BlastReduction( pNew, Vec_IntArray(vArg), Vec_IntSize(vArg), WLC_OBJ_REDUCT_NXOR ) );
            for ( k = 1; k < nRange; k++ )
                Vec_IntPush( vRes, 0 );
            return;
        }
        if ( Type == ABC_OPER_LOGIC_NOT ) // Y = !A    $logic_not   
        {
            int iLit = Wlc_BlastReduction( pNew, Vec_IntArray(vArg), Vec_IntSize(vArg), WLC_OBJ_REDUCT_OR );
            assert( nRange == 1 );
            Vec_IntFill( vRes, 1, Abc_LitNot(iLit) );
            for ( k = 1; k < nRange; k++ )
                Vec_IntPush( vRes, 0 );
            return;
        }
        assert( 0 );
        return;
    }

    if ( nIns == 2 )
    {
        Vec_Int_t * vArg0 = vDatas;
        Vec_Int_t * vArg1 = vDatas+1;
        Vec_Int_t * vRes  = vDatas+3;
        int nRangeMax = Abc_MaxInt( nRange, Abc_MaxInt(Vec_IntSize(vArg0), Vec_IntSize(vArg1)) );
        int nSizeArg0 = Vec_IntSize(vArg0);
        int nSizeArg1 = Vec_IntSize(vArg1);
        Rtl_VecExtend( vArg0, nRangeMax, fSign0 );
        Rtl_VecExtend( vArg1, nRangeMax, fSign1 );
        assert( Vec_IntSize(vArg0) == Vec_IntSize(vArg1) );
        assert( Vec_IntSize(vRes) == 0 );
        if ( Type == ABC_OPER_LOGIC_AND )     // Y = A && B   $logic_and  
        {
            int iLit0 = Wlc_BlastReduction( pNew, Vec_IntArray(vArg0), Vec_IntSize(vArg0), WLC_OBJ_REDUCT_OR );
            int iLit1 = Wlc_BlastReduction( pNew, Vec_IntArray(vArg1), Vec_IntSize(vArg1), WLC_OBJ_REDUCT_OR );
            assert( 1 == nRange );
            Vec_IntFill( vRes, 1, Gia_ManHashAnd(pNew, iLit0, iLit1) );
            for ( k = 1; k < nRange; k++ )
                Vec_IntPush( vRes, 0 );
            return;
        }
        if ( Type == ABC_OPER_LOGIC_OR )      // Y = A || B   $logic_or    
        {
            int iLit0 = Wlc_BlastReduction( pNew, Vec_IntArray(vArg0), Vec_IntSize(vArg0), WLC_OBJ_REDUCT_OR );
            int iLit1 = Wlc_BlastReduction( pNew, Vec_IntArray(vArg1), Vec_IntSize(vArg1), WLC_OBJ_REDUCT_OR );
            assert( 1 == nRange );
            Vec_IntFill( vRes, 1, Gia_ManHashOr(pNew, iLit0, iLit1) );
            for ( k = 1; k < nRange; k++ )
                Vec_IntPush( vRes, 0 );
            return;
        }

        if ( Type == ABC_OPER_BIT_AND )     // Y = A & B    $and
        {
            Vec_IntForEachEntryTwo( vArg0, vArg1, iLit0, iLit1, k )
                Vec_IntPush( vRes, Gia_ManHashAnd(pNew, iLit0, iLit1) );
            Vec_IntShrink( vRes, nRange );
            return;
        }
        if ( Type == ABC_OPER_BIT_OR )      // Y = A | B    $or    
        {
            Vec_IntForEachEntryTwo( vArg0, vArg1, iLit0, iLit1, k )
                Vec_IntPush( vRes, Gia_ManHashOr(pNew, iLit0, iLit1) );
            Vec_IntShrink( vRes, nRange );
            return;
        }
        if ( Type == ABC_OPER_BIT_XOR )     // Y = A ^ B    $xor
        { 
            Vec_IntForEachEntryTwo( vArg0, vArg1, iLit0, iLit1, k )
                Vec_IntPush( vRes, Gia_ManHashXor(pNew, iLit0, iLit1) );
            Vec_IntShrink( vRes, nRange );
            return;
        }
        if ( Type == ABC_OPER_BIT_NXOR )    // Y = A ~^ B   $xnor
        {
            assert( Vec_IntSize(vArg0) == nRange );
            Vec_IntForEachEntryTwo( vArg0, vArg1, iLit0, iLit1, k )
                Vec_IntPush( vRes, Abc_LitNot(Gia_ManHashXor(pNew, iLit0, iLit1)) );
            Vec_IntShrink( vRes, nRange );
            return;
        }
/*
        if ( !strcmp(pType, "$lt") )          return ABC_OPER_COMP_LESS;     // Y = A <  B   $lt          
        if ( !strcmp(pType, "$le") )          return ABC_OPER_COMP_LESSEQU;  // Y = A <= B   $le         
        if ( !strcmp(pType, "$ge") )          return ABC_OPER_COMP_MOREEQU;  // Y = A >= B   $ge           
        if ( !strcmp(pType, "$gt") )          return ABC_OPER_COMP_MORE;     // Y = A >  B   $gt        
        if ( !strcmp(pType, "$eq") )          return ABC_OPER_COMP_EQU;      // Y = A == B   $eq         
        if ( !strcmp(pType, "$ne") )          return ABC_OPER_COMP_NOTEQU;   // Y = A != B   $ne         
*/
        if ( Type == ABC_OPER_COMP_EQU || Type == ABC_OPER_COMP_NOTEQU )
        {
            iLit = 0;
            assert( nRange == 1 );
            Vec_IntForEachEntryTwo( vArg0, vArg1, iLit0, iLit1, k )
                iLit = Gia_ManHashOr( pNew, iLit, Gia_ManHashXor(pNew, iLit0, iLit1) ); 
            Vec_IntFill( vRes, 1, Abc_LitNotCond(iLit, Type == ABC_OPER_COMP_EQU) );
            for ( k = 1; k < nRange; k++ )
                Vec_IntPush( vRes, 0 );
            return;
        }
        if ( Type == ABC_OPER_COMP_LESS || Type == ABC_OPER_COMP_LESSEQU ||
             Type == ABC_OPER_COMP_MORE || Type == ABC_OPER_COMP_MOREEQU )
        {
            int fSigned = fSign0 && fSign1;
            int fSwap   = (Type == ABC_OPER_COMP_MORE    || Type == ABC_OPER_COMP_LESSEQU);
            int fCompl  = (Type == ABC_OPER_COMP_MOREEQU || Type == ABC_OPER_COMP_LESSEQU);
            assert( Vec_IntSize(vArg0) == Vec_IntSize(vArg1) );
            assert( nRange == 1 );
            if ( fSwap ) 
                ABC_SWAP( Vec_Int_t, *vArg0, *vArg1 )
            if ( fSigned )
                iLit = Wlc_BlastLessSigned( pNew, Vec_IntArray(vArg0), Vec_IntArray(vArg1), Vec_IntSize(vArg0) );
            else
                iLit = Wlc_BlastLess( pNew, Vec_IntArray(vArg0), Vec_IntArray(vArg1), Vec_IntSize(vArg0) );
            iLit = Abc_LitNotCond( iLit, fCompl );
            Vec_IntFill( vRes, 1, iLit );
            for ( k = 1; k < nRange; k++ )
                Vec_IntPush( vRes, 0 );
            return;
        }
/*        
        if ( !strcmp(pType, "$shl") )         return ABC_OPER_SHIFT_L;       // Y = A << B   $shl        
        if ( !strcmp(pType, "$shr") )         return ABC_OPER_SHIFT_R;       // Y = A >> B   $shr        
        if ( !strcmp(pType, "$sshl") )        return ABC_OPER_SHIFT_LA;      // Y = A <<< B  $sshl      
        if ( !strcmp(pType, "$sshr") )        return ABC_OPER_SHIFT_RA;      // Y = A >>> B  $sshr  
*/
        if ( Type == ABC_OPER_SHIFT_R || Type == ABC_OPER_SHIFT_RA ||
             Type == ABC_OPER_SHIFT_L || Type == ABC_OPER_SHIFT_LA )
        {
            Vec_IntShrink( vArg1, nSizeArg1 );
            if ( Type == ABC_OPER_SHIFT_R || Type == ABC_OPER_SHIFT_RA )
                Wlc_BlastShiftRight( pNew, Vec_IntArray(vArg0), nRangeMax, Vec_IntArray(vArg1), nSizeArg1, fSign0 && Type == ABC_OPER_SHIFT_RA, vRes );
            else
                Wlc_BlastShiftLeft( pNew, Vec_IntArray(vArg0), nRangeMax, Vec_IntArray(vArg1), nSizeArg1, 0, vRes );
            Vec_IntShrink( vRes, nRange );
            return;
        }
/*
        if ( !strcmp(pType, "$add") )         return ABC_OPER_ARI_ADD;       // Y = A + B    $add         
        if ( !strcmp(pType, "$sub") )         return ABC_OPER_ARI_SUB;       // Y = A - B    $sub         
        if ( !strcmp(pType, "$mul") )         return ABC_OPER_ARI_MUL;       // Y = A * B    $mul         
        if ( !strcmp(pType, "$div") )         return ABC_OPER_ARI_DIV;       // Y = A / B    $div         
        if ( !strcmp(pType, "$mod") )         return ABC_OPER_ARI_MOD;       // Y = A % B    $mod         
        if ( !strcmp(pType, "$pow") )         return ABC_OPER_ARI_POW;       // Y = A ** B   $pow        
*/
        if ( Type == ABC_OPER_ARI_ADD || Type == ABC_OPER_ARI_SUB )
        {
            //Vec_IntPrint( vArg0 );
            //Vec_IntPrint( vArg1 );
            Vec_IntAppend( vRes, vArg0 );
            if ( Type == ABC_OPER_ARI_ADD )
                Wlc_BlastAdder( pNew, Vec_IntArray(vRes), Vec_IntArray(vArg1), nRangeMax, 0 ); // result is in pFan0 (vRes)
            else 
                Wlc_BlastSubtract( pNew, Vec_IntArray(vRes), Vec_IntArray(vArg1), nRangeMax, 1 ); // result is in pFan0 (vRes)
            Vec_IntShrink( vRes, nRange );
            return;
        }
        if ( Type == ABC_OPER_ARI_MUL )
        {
            int fBooth  = 1;
            int fCla    = 0;
            int fSigned = fSign0 && fSign1;   
            //int i, iObj;
            Vec_IntShrink( vArg0, nSizeArg0 );
            Vec_IntShrink( vArg1, nSizeArg1 );

            //printf( "Adding %d + %d + %d buffers\n", nSizeArg0, nSizeArg1, nRange ); 
            //Vec_IntForEachEntry( vArg0, iObj, i )
            //    Vec_IntWriteEntry( vArg0, i, Gia_ManAppendBuf(pNew, iObj) );
            //Vec_IntForEachEntry( vArg1, iObj, i )
            //    Vec_IntWriteEntry( vArg1, i, Gia_ManAppendBuf(pNew, iObj) );

            if ( Wlc_NtkCountConstBits(Vec_IntArray(vArg0), Vec_IntSize(vArg0)) < Wlc_NtkCountConstBits(Vec_IntArray(vArg1), Vec_IntSize(vArg1)) )
                ABC_SWAP( Vec_Int_t, *vArg0, *vArg1 )
            if ( fBooth )
                Wlc_BlastBooth( pNew, Vec_IntArray(vArg0), Vec_IntArray(vArg1), Vec_IntSize(vArg0), Vec_IntSize(vArg1), vRes, fSigned, fCla, NULL );
            else
                Wlc_BlastMultiplier3( pNew, Vec_IntArray(vArg0), Vec_IntArray(vArg1), Vec_IntSize(vArg0), Vec_IntSize(vArg1), vRes, fSigned, fCla, NULL );
            if ( nRange > Vec_IntSize(vRes) )
                Vec_IntFillExtra( vRes, nRange, fSigned ? Vec_IntEntryLast(vRes) : 0 );
            else
                Vec_IntShrink( vRes, nRange );
            assert( Vec_IntSize(vRes) == nRange );

            //Vec_IntForEachEntry( vRes, iObj, i )
            //    Vec_IntWriteEntry( vRes, i, Gia_ManAppendBuf(pNew, iObj) );
            return;
        }
        if ( Type == ABC_OPER_ARI_DIV || Type == ABC_OPER_ARI_MOD )
        {
            int fDivBy0 = 1; // correct with 1
            int fSigned = fSign0 && fSign1;
            if ( fSigned )
                Wlc_BlastDividerSigned( pNew, Vec_IntArray(vArg0), nRangeMax, Vec_IntArray(vArg1), nRangeMax, Type == ABC_OPER_ARI_DIV, vRes, 0 );
            else
                Wlc_BlastDividerTop( pNew, Vec_IntArray(vArg0), nRangeMax, Vec_IntArray(vArg1), nRangeMax, Type == ABC_OPER_ARI_DIV, vRes, 0 );
            Vec_IntShrink( vRes, nRange );
            if ( !fDivBy0 )
                Wlc_BlastZeroCondition( pNew, Vec_IntArray(vArg1), nRange, vRes );
            return;
        }
        if ( Type == ABC_OPER_ARI_POW )
        {
            Vec_Int_t * vTemp = vDatas+4;
            Vec_IntGrow( vTemp, nRangeMax );
            Vec_IntGrow( vRes,  nRangeMax );
            Vec_IntShrink( vArg1, nSizeArg1 );
            Wlc_BlastPower( pNew, Vec_IntArray(vArg0), nRangeMax, Vec_IntArray(vArg1), Vec_IntSize(vArg1), vTemp, vRes );
            Vec_IntShrink( vRes, nRange );
            return;
        }
    }

    if ( nIns == 3 )
    {
        if ( Type == ABC_OPER_SEL_NMUX )    // $mux 
        {
            Vec_Int_t * vArg0 = vDatas;
            Vec_Int_t * vArg1 = vDatas+1;
            Vec_Int_t * vArgS = vDatas+2;
            Vec_Int_t * vRes  = vDatas+3;
            int iCtrl = Vec_IntEntry(vArgS, 0);
            //Vec_IntPrint( vArg0 );
            //Vec_IntPrint( vArg1 );
            //Vec_IntPrint( vArgS );
            assert( Vec_IntSize(vArg0) == Vec_IntSize(vArg1) );
            assert( Vec_IntSize(vArg0) == nRange );
            assert( Vec_IntSize(vArgS) == 1 );
            assert( Vec_IntSize(vRes) == 0 );
            Vec_IntForEachEntryTwo( vArg0, vArg1, iLit0, iLit1, k )
                Vec_IntPush( vRes, Gia_ManHashMux(pNew, iCtrl, iLit1, iLit0) );
            return;
        }
        if ( Type == ABC_OPER_SEL_SEL )    // $pmux 
        {
            int i, k, iLit;
            Vec_Int_t * vArgA = vDatas;
            Vec_Int_t * vArgB = vDatas+1;
            Vec_Int_t * vArgS = vDatas+2;
            Vec_Int_t * vRes  = vDatas+3;
            Vec_Int_t * vTemp = vDatas+4;
            assert( Vec_IntSize(vArgA) == nRange ); // widthA = widthY
            assert( Vec_IntSize(vArgB) == Vec_IntSize(vArgA)*Vec_IntSize(vArgS) ); // widthB == widthA*widthS
            assert( Vec_IntSize(vRes)  == 0 );
            for ( i = 0; i < nRange; i++ )
            {
                int iCond = 1;
                Vec_IntClear( vTemp );
                Vec_IntForEachEntry( vArgS, iLit, k ) // iLit = S[i]
                {
                    //Vec_IntPush( vTemp, Abc_LitNot( Gia_ManHashAnd(pNew, iLit, Vec_IntEntry(vArgB, nRange*(Vec_IntSize(vArgS)-1-k)+i)) ) ); // B[widthA*k+i]
                    Vec_IntPush( vTemp, Abc_LitNot( Gia_ManHashAnd(pNew, iLit, Vec_IntEntry(vArgB, nRange*k+i)) ) ); // B[widthA*k+i]
                    iCond = Gia_ManHashAnd( pNew, iCond, Abc_LitNot(iLit) );
                }
                Vec_IntPush( vTemp, Abc_LitNot( Gia_ManHashAnd(pNew, iCond, Vec_IntEntry(vArgA, i)) ) );
                Vec_IntPush( vRes, Abc_LitNot( Gia_ManHashAndMulti(pNew, vTemp) ) );
            }
            return;
        }
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

