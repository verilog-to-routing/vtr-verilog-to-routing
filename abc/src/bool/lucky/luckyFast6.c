/**CFile****************************************************************

  FileName    [luckyFast6.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Semi-canonical form computation package.]

  Synopsis    [Truth table minimization procedures for 6 vars.]

  Author      [Jake]

  Date        [Started - September 2012]

***********************************************************************/

#include "luckyInt.h"

ABC_NAMESPACE_IMPL_START

void resetPCanonPermArray_6Vars(char* x)
{
    x[0]='a';
    x[1]='b';
    x[2]='c';
    x[3]='d';
    x[4]='e';
    x[5]='f';
}
void resetPCanonPermArray(char* x, int nVars)
{
    int i;
    for(i=0;i<nVars;i++)
        x[i] = 'a'+i;
}



 word Abc_allFlip(word x, unsigned* pCanonPhase)
{
    if(  (x>>63) )
    {
        (* pCanonPhase) ^=(1<<6);
        return ~x;
    }
    else 
        return x;
    
}

unsigned adjustInfoAfterSwap(char* pCanonPerm, unsigned uCanonPhase, int iVar, unsigned info)
{   
    if(info<4)
        return (uCanonPhase ^= (info << iVar));
    else
    {
        char temp;
        uCanonPhase ^= ((info-4) << iVar);
        temp=pCanonPerm[iVar];
        pCanonPerm[iVar]=pCanonPerm[iVar+1];
        pCanonPerm[iVar+1]=temp;
        if ( ((uCanonPhase & (1 << iVar)) > 0) != ((uCanonPhase & (1 << (iVar+1))) > 0) )
        {
            uCanonPhase ^= (1 << iVar);
            uCanonPhase ^= (1 << (iVar+1));
        }
        return uCanonPhase; 
    }


}

word Extra_Truth6SwapAdjacent( word t, int iVar )
{
    // variable swapping code
    static word PMasks[5][3] = {
        { ABC_CONST(0x9999999999999999), ABC_CONST(0x2222222222222222), ABC_CONST(0x4444444444444444) },
        { ABC_CONST(0xC3C3C3C3C3C3C3C3), ABC_CONST(0x0C0C0C0C0C0C0C0C), ABC_CONST(0x3030303030303030) },
        { ABC_CONST(0xF00FF00FF00FF00F), ABC_CONST(0x00F000F000F000F0), ABC_CONST(0x0F000F000F000F00) },
        { ABC_CONST(0xFF0000FFFF0000FF), ABC_CONST(0x0000FF000000FF00), ABC_CONST(0x00FF000000FF0000) },
        { ABC_CONST(0xFFFF00000000FFFF), ABC_CONST(0x00000000FFFF0000), ABC_CONST(0x0000FFFF00000000) }
    };
    assert( iVar < 5 );
    return (t & PMasks[iVar][0]) | ((t & PMasks[iVar][1]) << (1 << iVar)) | ((t & PMasks[iVar][2]) >> (1 << iVar));
}
word Extra_Truth6ChangePhase( word t, int iVar)
{
    // elementary truth tables
    static word Truth6[6] = {
        ABC_CONST(0xAAAAAAAAAAAAAAAA),
        ABC_CONST(0xCCCCCCCCCCCCCCCC),
        ABC_CONST(0xF0F0F0F0F0F0F0F0),
        ABC_CONST(0xFF00FF00FF00FF00),
        ABC_CONST(0xFFFF0000FFFF0000),
        ABC_CONST(0xFFFFFFFF00000000)
    };
    assert( iVar < 6 );
    return ((t & ~Truth6[iVar]) << (1 << iVar)) | ((t & Truth6[iVar]) >> (1 << iVar));
}

word Extra_Truth6MinimumRoundOne( word t, int iVar, char* pCanonPerm, unsigned* pCanonPhase )
{
    word tCur, tMin = t; // ab 
    unsigned info =0;
    assert( iVar >= 0 && iVar < 5 );
    
    tCur = Extra_Truth6ChangePhase( t, iVar );    // !a b
    if(tCur<tMin)
    {
        info = 1;
        tMin = tCur;
    }
    tCur = Extra_Truth6ChangePhase( t, iVar+1 );  // a !b
    if(tCur<tMin)
    {
        info = 2;
        tMin = tCur;
    }
    tCur = Extra_Truth6ChangePhase( tCur, iVar ); // !a !b
    if(tCur<tMin)
    {
        info = 3;
        tMin = tCur;
    }
    
    t    = Extra_Truth6SwapAdjacent( t, iVar );   // b a
    if(t<tMin)
    {
        info = 4;
        tMin = t;
    }
    
    tCur = Extra_Truth6ChangePhase( t, iVar );    // !b a
    if(tCur<tMin)
    {
        info = 6;
        tMin = tCur;
    }
    tCur = Extra_Truth6ChangePhase( t, iVar+1 );  // b !a
    if(tCur<tMin)
    {
        info = 5;
        tMin = tCur;
    }
    tCur = Extra_Truth6ChangePhase( tCur, iVar ); // !b !a
    if(tCur<tMin)
    {
        (* pCanonPhase) = adjustInfoAfterSwap(pCanonPerm, * pCanonPhase, iVar, 7);
        return tCur;
    }
    else
    {
        (* pCanonPhase) = adjustInfoAfterSwap(pCanonPerm, * pCanonPhase, iVar, info);
        return tMin;
    }
}

word Extra_Truth6MinimumRoundOne_noEBFC( word t, int iVar,  char* pCanonPerm, unsigned* pCanonPhase)
{
    word tMin;     
    assert( iVar >= 0 && iVar < 5 );  
    
    tMin = Extra_Truth6SwapAdjacent( t, iVar );   // b a
    if(t<tMin)
        return t;
    else
    {
        (* pCanonPhase) = adjustInfoAfterSwap(pCanonPerm, * pCanonPhase, iVar, 4);
        return tMin;
    }
}


// this function finds minimal for all TIED(and tied only) iVars 
//it finds tied vars based on rearranged  Store info - group of tied vars has the same bit count in Store
word Extra_Truth6MinimumRoundMany( word t, int* pStore, char* pCanonPerm, unsigned* pCanonPhase )
{
    int i, bitInfoTemp;
    word tMin0, tMin=t;
    do
    {
        bitInfoTemp = pStore[0];
        tMin0 = tMin;
        for ( i = 0; i < 5; i++ )
        {
            if(bitInfoTemp == pStore[i+1])          
                tMin = Extra_Truth6MinimumRoundOne( tMin, i, pCanonPerm, pCanonPhase );         
            else
                bitInfoTemp = pStore[i+1];
        }
    }while ( tMin0 != tMin );
    return tMin;
}

word Extra_Truth6MinimumRoundMany_noEBFC( word t, int* pStore, char* pCanonPerm, unsigned* pCanonPhase )
{
    int i, bitInfoTemp;
    word tMin0, tMin=t;
    do
    {
        bitInfoTemp = pStore[0];
        tMin0 = tMin;
        for ( i = 0; i < 5; i++ )
        {
            if(bitInfoTemp == pStore[i+1])          
                tMin = Extra_Truth6MinimumRoundOne_noEBFC( tMin, i, pCanonPerm, pCanonPhase );         
            else
                bitInfoTemp = pStore[i+1];
        } 
    }while ( tMin0 != tMin );
    return tMin;
}
word Extra_Truth6MinimumRoundMany1( word t, int* pStore, char* pCanonPerm, unsigned* pCanonPhase )
{
    word tMin0, tMin=t;
    char pCanonPerm1[16];
    unsigned uCanonPhase1;
    switch ((* pCanonPhase) >> 7)
    {
    case 0 :
        {

            return Extra_Truth6MinimumRoundMany_noEBFC( t, pStore, pCanonPerm, pCanonPhase);
        }
    case 1 :
        {            
            return Extra_Truth6MinimumRoundMany( t, pStore, pCanonPerm, pCanonPhase);
        }
    case 2 :
        { 
            uCanonPhase1 = *pCanonPhase;
            uCanonPhase1 ^= (1 << 6);
            memcpy(pCanonPerm1,pCanonPerm,sizeof(char)*16);
            tMin0 = Extra_Truth6MinimumRoundMany_noEBFC( t, pStore, pCanonPerm, pCanonPhase);
            tMin =  Extra_Truth6MinimumRoundMany_noEBFC( ~t, pStore, pCanonPerm1, &uCanonPhase1);
            if(tMin0 <=tMin)
                return tMin0;
            else
            {
                *pCanonPhase = uCanonPhase1;
                memcpy(pCanonPerm,pCanonPerm1,sizeof(char)*16);
                return tMin;
            }
        }
    case 3 :
        {
            uCanonPhase1 = *pCanonPhase;
            uCanonPhase1 ^= (1 << 6);
            memcpy(pCanonPerm1,pCanonPerm,sizeof(char)*16);
            tMin0 = Extra_Truth6MinimumRoundMany( t, pStore, pCanonPerm, pCanonPhase);
            tMin =  Extra_Truth6MinimumRoundMany( ~t, pStore, pCanonPerm1, &uCanonPhase1);
            if(tMin0 <=tMin)
                return tMin0;
            else
            {
                *pCanonPhase = uCanonPhase1;
                memcpy(pCanonPerm,pCanonPerm1,sizeof(char)*16);
                return tMin;
            }
        }
    }
    return Extra_Truth6MinimumRoundMany( t, pStore, pCanonPerm, pCanonPhase);
}

word luckyCanonicizer_final_fast_6Vars(word InOut, int* pStore, char* pCanonPerm, unsigned* pCanonPhase)
{
    (* pCanonPhase) = Kit_TruthSemiCanonicize_Yasha1( &InOut, 6, pCanonPerm, pStore);
    return Extra_Truth6MinimumRoundMany1(InOut, pStore, pCanonPerm, pCanonPhase); 
}
word luckyCanonicizer_final_fast_6Vars1(word InOut, int* pStore, char* pCanonPerm, unsigned* pCanonPhase )
{
    (* pCanonPhase) = Kit_TruthSemiCanonicize_Yasha1( &InOut, 6, pCanonPerm, pStore);
    InOut = Extra_Truth6MinimumRoundMany1(InOut, pStore, pCanonPerm, pCanonPhase);
    Kit_TruthChangePhase_64bit( &InOut, 6, 5 );
    Kit_TruthChangePhase_64bit( &InOut, 6, 4 );
    Kit_TruthChangePhase_64bit( &InOut, 6, 3 );
    Kit_TruthChangePhase_64bit( &InOut, 6, 2 );
    Kit_TruthChangePhase_64bit( &InOut, 6, 1 );        
    Kit_TruthChangePhase_64bit( &InOut, 6, 0 );
    (*pCanonPhase) ^= 0x3F;
    return Extra_Truth6MinimumRoundMany1(InOut, pStore, pCanonPerm, pCanonPhase); 
}


ABC_NAMESPACE_IMPL_END
