/**CFile****************************************************************

  FileName    [luckySwap.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Semi-canonical form computation package.]

  Synopsis    [Swapping variables in the truth table.]

  Author      [Jake]

  Date        [Started - August 2012]

***********************************************************************/

#include "luckyInt.h"


ABC_NAMESPACE_IMPL_START


static word mask0[6] = { ABC_CONST(0x5555555555555555),ABC_CONST(0x3333333333333333), ABC_CONST(0x0F0F0F0F0F0F0F0F),ABC_CONST(0x00FF00FF00FF00FF),ABC_CONST(0x0000FFFF0000FFFF), ABC_CONST(0x00000000FFFFFFFF)};  
/*
static word mask1[6] = { 0xAAAAAAAAAAAAAAAA,0xCCCCCCCCCCCCCCCC, 0xF0F0F0F0F0F0F0F0,0xFF00FF00FF00FF00,0xFFFF0000FFFF0000, 0xFFFFFFFF00000000 };
static word mask[6][2] =   {
    {0x5555555555555555,0xAAAAAAAAAAAAAAAA},
    {0x3333333333333333,0xCCCCCCCCCCCCCCCC},
    {0x0F0F0F0F0F0F0F0F,0xF0F0F0F0F0F0F0F0},
    {0x00FF00FF00FF00FF,0xFF00FF00FF00FF00},
    {0x0000FFFF0000FFFF,0xFFFF0000FFFF0000},
    {0x00000000FFFFFFFF,0xFFFFFFFF00000000}
};
*/

int Kit_TruthWordNum_64bit( int nVars )  { return nVars <= 6 ? 1 : (1 << (nVars - 6));}

int Kit_WordCountOnes_64bit(word x)
{
    x = x - ((x >> 1) & ABC_CONST(0x5555555555555555));   
    x = (x & ABC_CONST(0x3333333333333333)) + ((x >> 2) & ABC_CONST(0x3333333333333333));    
    x = (x + (x >> 4)) & ABC_CONST(0x0F0F0F0F0F0F0F0F);    
    x = x + (x >> 8);
    x = x + (x >> 16);
    x = x + (x >> 32); 
    return (int)(x & 0xFF);
}

int Kit_TruthCountOnes_64bit( word* pIn, int nVars )
{
    int w, Counter = 0;
    for ( w = Kit_TruthWordNum_64bit(nVars)-1; w >= 0; w-- )
        Counter += Kit_WordCountOnes_64bit(pIn[w]);
    return Counter;
}

void Kit_TruthCountOnesInCofs_64bit( word * pTruth, int nVars, int * pStore )
{    
    int nWords = Kit_TruthWordNum_64bit( nVars );
    int i, k, Counter;
    memset( pStore, 0, sizeof(int) * nVars );
    if ( nVars <= 6 )
    {
        if ( nVars > 0 )        
            pStore[0] = Kit_WordCountOnes_64bit( pTruth[0] & ABC_CONST(0x5555555555555555) );  
        if ( nVars > 1 )       
            pStore[1] = Kit_WordCountOnes_64bit( pTruth[0] & ABC_CONST(0x3333333333333333) );     
        if ( nVars > 2 )       
            pStore[2] = Kit_WordCountOnes_64bit( pTruth[0] & ABC_CONST(0x0F0F0F0F0F0F0F0F) );   
        if ( nVars > 3 )       
            pStore[3] = Kit_WordCountOnes_64bit( pTruth[0] & ABC_CONST(0x00FF00FF00FF00FF) );     
        if ( nVars > 4 )       
            pStore[4] = Kit_WordCountOnes_64bit( pTruth[0] & ABC_CONST(0x0000FFFF0000FFFF) ); 
        if ( nVars > 5 )       
            pStore[5] = Kit_WordCountOnes_64bit( pTruth[0] & ABC_CONST(0x00000000FFFFFFFF) );      
        return;
    }
    // nVars > 6
    // count 1's for all other variables
    for ( k = 0; k < nWords; k++ )
    {
        Counter = Kit_WordCountOnes_64bit( pTruth[k] );
        for ( i = 6; i < nVars; i++ )
            if ( (k & (1 << (i-6))) == 0)
                pStore[i] += Counter;
    }
    // count 1's for the first six variables
    for ( k = nWords/2; k>0; k-- )
    {
        pStore[0] += Kit_WordCountOnes_64bit( (pTruth[0] & ABC_CONST(0x5555555555555555)) | ((pTruth[1] & ABC_CONST(0x5555555555555555)) <<  1) );
        pStore[1] += Kit_WordCountOnes_64bit( (pTruth[0] & ABC_CONST(0x3333333333333333)) | ((pTruth[1] & ABC_CONST(0x3333333333333333)) <<  2) );
        pStore[2] += Kit_WordCountOnes_64bit( (pTruth[0] & ABC_CONST(0x0F0F0F0F0F0F0F0F)) | ((pTruth[1] & ABC_CONST(0x0F0F0F0F0F0F0F0F)) <<  4) );
        pStore[3] += Kit_WordCountOnes_64bit( (pTruth[0] & ABC_CONST(0x00FF00FF00FF00FF)) | ((pTruth[1] & ABC_CONST(0x00FF00FF00FF00FF)) <<  8) );
        pStore[4] += Kit_WordCountOnes_64bit( (pTruth[0] & ABC_CONST(0x0000FFFF0000FFFF)) | ((pTruth[1] & ABC_CONST(0x0000FFFF0000FFFF)) <<  16) );
        pStore[5] += Kit_WordCountOnes_64bit( (pTruth[0] & ABC_CONST(0x00000000FFFFFFFF)) | ((pTruth[1] & ABC_CONST(0x00000000FFFFFFFF)) <<  32) );  
        pTruth += 2;
    }
}

void Kit_TruthChangePhase_64bit( word * pInOut, int nVars, int iVar )
{
    int nWords = Kit_TruthWordNum_64bit( nVars );
    int i, Step,SizeOfBlock;
    word Temp[512];
    
    assert( iVar < nVars );
    if(iVar<=5)
    {
        for ( i = 0; i < nWords; i++ )
            pInOut[i] = ((pInOut[i] & mask0[iVar]) << (1<<(iVar))) | ((pInOut[i] & ~mask0[iVar]) >> (1<<(iVar)));
    }
    else
    {
        Step = (1 << (iVar - 6));
        SizeOfBlock = sizeof(word)*Step;
        for ( i = 0; i < nWords; i += 2*Step )
        {   
            memcpy(Temp,pInOut,(size_t)SizeOfBlock);
            memcpy(pInOut,pInOut+Step,(size_t)SizeOfBlock);
            memcpy(pInOut+Step,Temp,(size_t)SizeOfBlock);
            //          Temp = pInOut[i];
            //          pInOut[i] = pInOut[Step+i];
            //          pInOut[Step+i] = Temp;          
            pInOut += 2*Step;
        }
    }
    
}

void Kit_TruthNot_64bit(word * pIn, int nVars )
{
    int w;
    for ( w = Kit_TruthWordNum_64bit(nVars)-1; w >= 0; w-- )
        pIn[w] = ~pIn[w];
}
void Kit_TruthCopy_64bit( word * pOut, word * pIn, int nVars )
{ 
    memcpy(pOut,pIn,Kit_TruthWordNum_64bit(nVars)*sizeof(word));
}

void Kit_TruthSwapAdjacentVars_64bit( word * pInOut, int nVars, int iVar )
{
    int i, Step, Shift, SizeOfBlock;                   //
    word temp[256];                   // to make only pInOut possible
    static word PMasks[5][3] = {
        { ABC_CONST(0x9999999999999999), ABC_CONST(0x2222222222222222), ABC_CONST(0x4444444444444444) },
        { ABC_CONST(0xC3C3C3C3C3C3C3C3), ABC_CONST(0x0C0C0C0C0C0C0C0C), ABC_CONST(0x3030303030303030) },
        { ABC_CONST(0xF00FF00FF00FF00F), ABC_CONST(0x00F000F000F000F0), ABC_CONST(0x0F000F000F000F00) },
        { ABC_CONST(0xFF0000FFFF0000FF), ABC_CONST(0x0000FF000000FF00), ABC_CONST(0x00FF000000FF0000) },
        { ABC_CONST(0xFFFF00000000FFFF), ABC_CONST(0x00000000FFFF0000), ABC_CONST(0x0000FFFF00000000) }
    };
    int nWords = Kit_TruthWordNum_64bit( nVars ); 
    
    assert( iVar < nVars - 1 );
    if ( iVar < 5 )
    {
        Shift = (1 << iVar);
        for ( i = 0; i < nWords; i++ )
            pInOut[i] = (pInOut[i] & PMasks[iVar][0]) | ((pInOut[i] & PMasks[iVar][1]) << Shift) | ((pInOut[i] & PMasks[iVar][2]) >> Shift);
    }
    else if ( iVar > 5 )
    {
        Step = 1 << (iVar - 6);
        SizeOfBlock = sizeof(word)*Step;
        pInOut += 2*Step;
        for(i=2*Step; i<nWords; i+=4*Step)
        {           
            memcpy(temp,pInOut-Step,(size_t)SizeOfBlock);
            memcpy(pInOut-Step,pInOut,(size_t)SizeOfBlock);
            memcpy(pInOut,temp,(size_t)SizeOfBlock);
            pInOut += 4*Step;
        }
    }
    else // if ( iVar == 5 )
    {
        for ( i = 0; i < nWords; i += 2 )
        {
            temp[0] = pInOut[i+1] << 32;
            pInOut[i+1] ^= (temp[0] ^ pInOut[i]) >> 32;
            pInOut[i] = (pInOut[i] & 0x00000000FFFFFFFF) | temp[0];
            
        }
    }
}

unsigned  Kit_TruthSemiCanonicize_Yasha( word* pInOut, int nVars, char * pCanonPerm )
{
    int pStore[16];
    int nWords = Kit_TruthWordNum_64bit( nVars );
    int i, Temp, fChange, nOnes;
    unsigned  uCanonPhase=0;
    assert( nVars <= 16 );
    
    nOnes = Kit_TruthCountOnes_64bit(pInOut, nVars);
    
    if ( (nOnes > nWords * 32) )
    {
        uCanonPhase |= (1 << nVars);
        Kit_TruthNot_64bit( pInOut, nVars );
        nOnes = nWords*64 - nOnes;
    }
    
    // collect the minterm counts
    Kit_TruthCountOnesInCofs_64bit( pInOut, nVars, pStore );
    
    // canonicize phase
    for ( i = 0; i < nVars; i++ )
    {
        if ( pStore[i] >= nOnes-pStore[i])
            continue;
        uCanonPhase |= (1 << i);
        pStore[i] = nOnes-pStore[i]; 
        Kit_TruthChangePhase_64bit( pInOut, nVars, i );
    }  
    
    do {
        fChange = 0;
        for ( i = 0; i < nVars-1; i++ )
        {
            if ( pStore[i] <= pStore[i+1] )
                continue;            
            fChange = 1;
            
            Temp = pCanonPerm[i];
            pCanonPerm[i] = pCanonPerm[i+1];
            pCanonPerm[i+1] = Temp;
            
            Temp = pStore[i];
            pStore[i] = pStore[i+1];
            pStore[i+1] = Temp;
            
            // if the polarity of variables is different, swap them
            if ( ((uCanonPhase & (1 << i)) > 0) != ((uCanonPhase & (1 << (i+1))) > 0) )
            {
                uCanonPhase ^= (1 << i);
                uCanonPhase ^= (1 << (i+1));
            }
            
            Kit_TruthSwapAdjacentVars_64bit( pInOut, nVars, i );            
        }
    } while ( fChange );
    return uCanonPhase;
}

unsigned  Kit_TruthSemiCanonicize_Yasha1( word* pInOut, int nVars, char * pCanonPerm, int * pStore )
{
    int nWords = Kit_TruthWordNum_64bit( nVars );
    int i, fChange, nOnes;
    int Temp;
    unsigned  uCanonPhase=0;
    assert( nVars <= 16 );
    
    nOnes = Kit_TruthCountOnes_64bit(pInOut, nVars);
    if ( nOnes == nWords * 32 )
        uCanonPhase |= (1 << (nVars+2));
    
    else if ( (nOnes > nWords * 32) )
    {
        uCanonPhase |= (1 << nVars);
        Kit_TruthNot_64bit( pInOut, nVars );
        nOnes = nWords*64 - nOnes;
    }
    
    // collect the minterm counts
    Kit_TruthCountOnesInCofs_64bit( pInOut, nVars, pStore );
    
    // canonicize phase
    for ( i = 0; i < nVars; i++ )
    {
        if (  2*pStore[i]  == nOnes)
        {
            uCanonPhase |= (1 << (nVars+1));
            continue;
        }
        if ( pStore[i] > nOnes-pStore[i])
            continue;
        uCanonPhase |= (1 << i);
        pStore[i] = nOnes-pStore[i]; 
        Kit_TruthChangePhase_64bit( pInOut, nVars, i );
    }  
    
    do {
        fChange = 0;
        for ( i = 0; i < nVars-1; i++ )
        {
            if ( pStore[i] <= pStore[i+1] )
                continue;            
            fChange = 1;
            
            Temp = pCanonPerm[i];
            pCanonPerm[i] = pCanonPerm[i+1];
            pCanonPerm[i+1] = Temp;
            
            Temp = pStore[i];
            pStore[i] = pStore[i+1];
            pStore[i+1] = Temp;
            
            // if the polarity of variables is different, swap them
            if ( ((uCanonPhase & (1 << i)) > 0) != ((uCanonPhase & (1 << (i+1))) > 0) )
            {
                uCanonPhase ^= (1 << i);
                uCanonPhase ^= (1 << (i+1));
            }
            
            Kit_TruthSwapAdjacentVars_64bit( pInOut, nVars, i );            
        }
    } while ( fChange );
    return uCanonPhase;
}


// unsigned  Kit_TruthSemiCanonicize_Yasha_simple( word* pInOut, int nVars, char * pCanonPerm )
// {
//     unsigned uCanonPhase = 0;
//     int pStore[16];
//     int nWords = Kit_TruthWordNum_64bit( nVars );
//     int i, Temp, fChange, nOnes; 
//  assert( nVars <= 16 );
//  
//     nOnes = Kit_TruthCountOnes_64bit(pInOut, nVars);
//  
//     if ( (nOnes > nWords * 32) )
//     { 
//         Kit_TruthNot_64bit( pInOut, nVars );
//      nOnes = nWords*64 - nOnes;
//     }
//  
//     // collect the minterm counts
//     Kit_TruthCountOnesInCofs_64bit( pInOut, nVars, pStore );
//  
//     // canonicize phase
//     for ( i = 0; i < nVars; i++ )
//     {
//         if ( pStore[i] >= nOnes-pStore[i])
//             continue;        
//         pStore[i] = nOnes-pStore[i]; 
//         Kit_TruthChangePhase_64bit( pInOut, nVars, i );
//     }  
//  
//     do {
//         fChange = 0;
//         for ( i = 0; i < nVars-1; i++ )
//         {
//             if ( pStore[i] <= pStore[i+1] )
//                 continue;            
//             fChange = 1;         
//          
//             Temp = pStore[i];
//             pStore[i] = pStore[i+1];
//             pStore[i+1] = Temp;
//          
//             Kit_TruthSwapAdjacentVars_64bit( pInOut, nVars, i );             
//         }
//     } while ( fChange ); 
//     return uCanonPhase;
// }

void  Kit_TruthSemiCanonicize_Yasha_simple( word* pInOut, int nVars, int * pStore )
{
    int nWords = Kit_TruthWordNum_64bit( nVars );
    int i, Temp, fChange, nOnes; 
    assert( nVars <= 16 );
    
    nOnes = Kit_TruthCountOnes_64bit(pInOut, nVars);
    
    if ( (nOnes > nWords * 32) )
    { 
        Kit_TruthNot_64bit( pInOut, nVars );
        nOnes = nWords*64 - nOnes;
    }
    
    // collect the minterm counts
    Kit_TruthCountOnesInCofs_64bit( pInOut, nVars, pStore );
    
    // canonicize phase
    for ( i = 0; i < nVars; i++ )
    {
        if ( pStore[i] >= nOnes-pStore[i])
            continue;        
        pStore[i] = nOnes-pStore[i]; 
        Kit_TruthChangePhase_64bit( pInOut, nVars, i );
    }  
    
    do {
        fChange = 0;
        for ( i = 0; i < nVars-1; i++ )
        {
            if ( pStore[i] <= pStore[i+1] )
                continue;            
            fChange = 1;            
            
            Temp = pStore[i];
            pStore[i] = pStore[i+1];
            pStore[i+1] = Temp;
            
            Kit_TruthSwapAdjacentVars_64bit( pInOut, nVars, i );            
        }
    } while ( fChange ); 
}


ABC_NAMESPACE_IMPL_END
