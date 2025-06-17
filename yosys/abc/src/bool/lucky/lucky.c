/**CFile****************************************************************

  FileName    [lucky.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Semi-canonical form computation package.]

  Synopsis    [Truth table minimization procedures.]

  Author      [Jake]

  Date        [Started - August 2012]

***********************************************************************/

#include "luckyInt.h"

ABC_NAMESPACE_IMPL_START


int memCompare(word* x, word*  y, int nVars)
{
    int i;
    for(i=Kit_TruthWordNum_64bit( nVars )-1;i>=0;i--)
    {
        if(x[i]==y[i])
            continue;
        else if(x[i]>y[i])
            return 1;
        else
            return -1;
    }
    return 0;
}
///////sort Word* a///////////////////////////////////////////////////////////////////////////////////////////////////////
 int compareWords1 (const void * a, const void * b)
 {
     if( *(word*)a > *(word*)b )
         return 1;
     else
         return( *(word*)a < *(word*)b ) ? -1: 0;
     
 }
 
 void sortAndUnique1(word* a, Abc_TtStore_t* p)
 {
     int i, count=1, WordsN = p->nFuncs;
     word tempWord;
     qsort(a,(size_t)WordsN,sizeof(word),compareWords1);
     tempWord = a[0];
     for(i=1;i<WordsN;i++)  
         if(tempWord != a[i])
         {
             a[count] = a[i];
             tempWord = a[i];
             count++;
         }
     p->nFuncs = count;
}
//////////sort Word** a//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int compareWords2 (const void ** x, const void ** y)
{
    
    if(**(word**)x > **(word**)y)
        return 1;
    else if(**(word**)x < **(word**)y)
        return -1;
    else
        return 0;   
    
}
int compareWords (const void ** a, const void ** b)
{
     if( memcmp(*(word**)a,*(word**)b,sizeof(word)*1) > 0 )
         return 1;
     else
         return ( memcmp(*(word**)a,*(word**)b,sizeof(word)*1) < 0 ) ? -1: 0;       
}
int compareWords3 (const void ** x, const void ** y)
{
    return memCompare(*(word**)x, *(word**)y, 16);   
} 
void sortAndUnique(word** a, Abc_TtStore_t* p)
{
     int i, count=1, WordsPtrN = p->nFuncs;
     word* tempWordPtr;
     qsort(a,WordsPtrN,sizeof(word*),(int(*)(const void *,const void *))compareWords3);
     tempWordPtr = a[0];
     for(i=1;i<WordsPtrN;i++)   
         if(memcmp(a[i],tempWordPtr,sizeof(word)*(p->nWords)) != 0)
         {
             a[count] = a[i];
             tempWordPtr = a[i];
             count++;
         }
     p->nFuncs = count;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
     int totalCycles;
     int maxNCycles;
     int minNCycles;
     
}cycleCtr;
cycleCtr* setCycleCtrPtr()
{
     cycleCtr* x = (cycleCtr*) malloc(sizeof(cycleCtr));
     x->totalCycles=0;
     x->maxNCycles=0;
     x->minNCycles=111111111;
     return x;
}
void freeCycleCtr(cycleCtr* x)
{
    free(x);
}
word** makeArray(Abc_TtStore_t* p)
{
    int i;
    word** a;
    a = (word**)malloc(sizeof(word*)*(p->nFuncs));
    for(i=0;i<p->nFuncs;i++)
    {
        a[i] = (word*)malloc(sizeof(word)*(p->nWords));
        memcpy(a[i],p->pFuncs[i],sizeof(word)*(p->nWords));

    }
    return a;
}
void freeArray(word** a,Abc_TtStore_t* p)
{
    int i;
    for(i=0;i<p->nFuncs;i++)
        free(a[i]);
    free(a);
}

word* makeArrayB(word** a, int nFuncs)
{
    int i;
    word* b;
    b = (word*)malloc(sizeof(word)*(nFuncs));
    for(i=0;i<nFuncs;i++)
        b[i] = a[i][0];

    return b;
}
void freeArrayB(word* b)
{
    free(b);
}

////////////////////////////////////////////////////////////////////////////////////////

// if highest bit in F ( all ones min term ) is one => inverse 
// if pInOnt changed(minimized) by function return 1 if not 0
// int minimalInitialFlip_propper(word* pInOut, word* pDuplicat, int  nVars)
// {
//  word oneWord=1;
//  Kit_TruthCopy_64bit( pDuplicat, pInOut, nVars );
//  Kit_TruthNot_64bit( pDuplicat, nVars );
//  if( memCompare(pDuplicat,pInOut,nVars) == -1)
//  {
//      Kit_TruthCopy_64bit(pInOut, pDuplicat, nVars );
//      return 1;
//  }
//  return 0;
// }
// int minimalFlip(word* pInOut, word* pMinimal, word* PDuplicat, int  nVars) 
// {
//  int i;
//  int blockSize = Kit_TruthWordNum_64bit( nVars )*sizeof(word);
//  memcpy(pMinimal, pInOut, blockSize);
//  memcpy(PDuplicat, pInOut, blockSize);
//  for(i=0;i<nVars;i++)
//  {
//      Kit_TruthChangePhase_64bit( pInOut, nVars, i );
//      if( memCompare(pMinimal,pInOut,nVars) == 1)
//          memcpy(pMinimal, pInOut, blockSize);
//      memcpy(pInOut,PDuplicat,blockSize);
//  }
//  memcpy(pInOut,pMinimal,blockSize);
//  if(memCompare(pMinimal,PDuplicat,nVars) == 0)
//      return 0;
//  else
//      return 1;
// }
// int minimalSwap(word* pInOut, word* pMinimal, word* PDuplicat, int  nVars) 
// {
//  int i;  
//  int blockSize = Kit_TruthWordNum_64bit( nVars )*sizeof(word);
//  memcpy(pMinimal, pInOut, blockSize);
//  memcpy(PDuplicat, pInOut, blockSize);
//  for(i=0;i<nVars-1;i++)
//  {
//      Kit_TruthSwapAdjacentVars_64bit( pInOut, nVars, i );
//      if(memCompare(pMinimal,pInOut,nVars) == 1)
//          memcpy(pMinimal, pInOut, blockSize);
//      memcpy(pInOut,PDuplicat,blockSize);
//  }
//  memcpy(pInOut,pMinimal,blockSize);
//  if(memCompare(pMinimal,PDuplicat,nVars) == 0)
//      return 0;
//  else
//      return 1;
// }
// 
// void luckyCanonicizer(word* pInOut, word* pAux, word* pAux1, int  nVars, cycleCtr* cCtr)
// {
//  int counter=1, cycles=0;   
//  assert( nVars <= 16 );
//  while(counter>0 )   //  && cycles < 10 if we wanna limit cycles
//  {
//      counter=0;
//      counter += minimalInitialFlip(pInOut, nVars);
//      counter += minimalFlip(pInOut, pAux, pAux1, nVars);
//      counter += minimalSwap(pInOut, pAux, pAux1, nVars);
//      cCtr->totalCycles++;
//      cycles++;
//  }
//  if(cycles < cCtr->minNCycles)
//      cCtr->minNCycles = cycles;
//  else if(cycles > cCtr->maxNCycles)
//      cCtr->maxNCycles = cycles;
// }
//  runs paralel F and ~F in luckyCanonicizer
// void luckyCanonicizer2(word* pInOut, word* pAux, word* pAux1, word* temp, int  nVars)
// {
//  int nWords = Kit_TruthWordNum_64bit( nVars );
//  int counter=1, nOnes;   
//  assert( nVars <= 16 );
//  nOnes = Kit_TruthCountOnes_64bit(pInOut, nVars);
//  
//     if ( (nOnes*2 == nWords * 32) )
//     { 
//      Kit_TruthCopy_64bit( temp, pInOut, nVars );
//      Kit_TruthNot_64bit( temp, nVars );
//         luckyCanonicizer1_simple(pInOut, pAux, pAux1, nVars);
//      luckyCanonicizer1_simple(temp, pAux, pAux1, nVars);
//      if( memCompare(temp,pInOut,nVars) == -1)        
//          Kit_TruthCopy_64bit(pInOut, temp, nVars );  
//      return;     
//     }
//  while(counter>0 )   //  && cycles < 10 if we wanna limit cycles
//  {
//      counter=0;
//      counter += minimalInitialFlip_propper(pInOut, pAux, nVars);
//      counter += minimalFlip1(pInOut, pAux, pAux1, nVars);
//      counter += minimalSwap1(pInOut, pAux, pAux1, nVars);
//  }
// }
// same as luckyCanonicizer + cycleCtr stutistics 
// void luckyCanonicizer1(word* pInOut, word* pAux, word* pAux1, int  nVars, cycleCtr* cCtr)
// {
//  int counter=1, cycles=0;   
//  assert( nVars <= 16 );
//  while(counter>0 )   //  && cycles < 10 if we wanna limit cycles
//  {
//      counter=0;
//      counter += minimalInitialFlip1(pInOut, nVars);
//      counter += minimalFlip1(pInOut, pAux, pAux1, nVars);
//      counter += minimalSwap1(pInOut, pAux, pAux1, nVars);
//      cCtr->totalCycles++;
//      cycles++;
//  }
//  if(cycles < cCtr->minNCycles)
//      cCtr->minNCycles = cycles;
//  else if(cycles > cCtr->maxNCycles)
//      cCtr->maxNCycles = cycles;
// }
// luckyCanonicizer 

void printCCtrInfo(cycleCtr* cCtr, int nFuncs)
{
    printf("maxNCycles = %d\n",cCtr->maxNCycles);
    printf("minNCycles = %d\n",cCtr->minNCycles);
    printf("average NCycles = %.3f\n",cCtr->totalCycles/(double)nFuncs);
}
////////////////////////////////////////New Faster versoin/////////////////////////////////////////////////////////

// if highest bit in F ( all ones min term ) is one => inverse 
// returns: if pInOnt changed(minimized) by function return 1 if not 0
int minimalInitialFlip1(word* pInOut, int  nVars)
{
    word oneWord=1;
    if(  (pInOut[Kit_TruthWordNum_64bit( nVars ) -1]>>63) & oneWord )
    {
        Kit_TruthNot_64bit( pInOut, nVars );
        return 1;
    }
    return 0;
}

// compare F with  F1 = (F with changed phase in one of the vars).
// keeps smaller.
// same for all vars in F.
// returns: if pInOnt changed(minimized) by function return 1 if not 0
int minimalFlip1(word* pInOut, word* pMinimal, word* PDuplicat, int  nVars) 
{
    int i;
    int blockSize = Kit_TruthWordNum_64bit( nVars )*sizeof(word);
    memcpy(pMinimal, pInOut, (size_t)blockSize);
    memcpy(PDuplicat, pInOut, (size_t)blockSize);
    Kit_TruthChangePhase_64bit( pInOut, nVars, 0 );
    for(i=1;i<nVars;i++)
    {
        if( memCompare(pMinimal,pInOut,nVars) == 1)
        {
            memcpy(pMinimal, pInOut, (size_t)blockSize);
            Kit_TruthChangePhase_64bit( pInOut, nVars, i );
        }
        else
        {
            memcpy(pInOut, pMinimal, (size_t)blockSize);
            Kit_TruthChangePhase_64bit( pInOut, nVars, i );
        }
    }
    if( memCompare(pMinimal,pInOut,nVars) == -1)
        memcpy(pInOut, pMinimal, (size_t)blockSize);
    if(memcmp(pInOut,PDuplicat,(size_t)blockSize) == 0)
        return 0;
    else
        return 1;
}
// compare F with  F1 = (F with swapped two adjacent vars).
// keeps smaller.
// same for all vars in F.
// returns: if pInOnt changed(minimized) by function return 1 if not 0
int minimalSwap1(word* pInOut, word* pMinimal, word* PDuplicat, int  nVars) 
{
    int i;
    int blockSize = Kit_TruthWordNum_64bit( nVars )*sizeof(word);
    memcpy(pMinimal, pInOut, (size_t)blockSize);
    memcpy(PDuplicat, pInOut, (size_t)blockSize);
    Kit_TruthSwapAdjacentVars_64bit( pInOut, nVars, 0 );
    for(i=1;i<nVars-1;i++)
    {
        if( memCompare(pMinimal,pInOut,nVars) == 1)
        {
            memcpy(pMinimal, pInOut, (size_t)blockSize);
            Kit_TruthSwapAdjacentVars_64bit( pInOut, nVars, i );
        }
        else
        {
            memcpy(pInOut, pMinimal, (size_t)blockSize);
            Kit_TruthSwapAdjacentVars_64bit( pInOut, nVars, i );
        }
    }
    if( memCompare(pMinimal,pInOut,nVars) == -1)
        memcpy(pInOut, pMinimal, (size_t)blockSize);
    if(memcmp(pInOut,PDuplicat,(size_t)blockSize) == 0)
        return 0;
    else
        return 1;
}

// if highest bit in F ( all ones min term ) is one => inverse 
// returns: if pInOnt changed(minimized) by function return 1 if not 0
int minimalInitialFlip(word* pInOut, int  nVars, unsigned* p_uCanonPhase)
{
    word oneWord=1;
    if(  (pInOut[Kit_TruthWordNum_64bit( nVars ) -1]>>63) & oneWord )
    {
        Kit_TruthNot_64bit( pInOut, nVars );
        *p_uCanonPhase ^= (1 << nVars);
        return 1;
    }
    return 0;
}

// compare F with  F1 = (F with changed phase in one of the vars).
// keeps smaller.
// same for all vars in F.
// returns: if pInOnt changed(minimized) by function return 1 if not 0
int minimalFlip(word* pInOut, word* pMinimal, word* PDuplicat, int  nVars, unsigned* p_uCanonPhase)
{
    int i;
    unsigned minTemp = *p_uCanonPhase;
    int blockSize = Kit_TruthWordNum_64bit( nVars )*sizeof(word);
    memcpy(pMinimal, pInOut, (size_t)blockSize);
    memcpy(PDuplicat, pInOut, (size_t)blockSize);       //////////////need one more unsigned!!!!!!!!!!!!!
    Kit_TruthChangePhase_64bit( pInOut, nVars, 0 );
    *p_uCanonPhase ^= (unsigned)1;
    for(i=1;i<nVars;i++)
    {
        if( memCompare(pMinimal,pInOut,nVars) == 1)
        {
            memcpy(pMinimal, pInOut, (size_t)blockSize);
            minTemp = *p_uCanonPhase;
        }
        else
        {
            memcpy(pInOut, pMinimal, (size_t)blockSize);
            *p_uCanonPhase = minTemp;
        }
        Kit_TruthChangePhase_64bit( pInOut, nVars, i );
        *p_uCanonPhase ^= (1 << i);
    }
    if( memCompare(pMinimal,pInOut,nVars) == -1)
    {
        memcpy(pInOut, pMinimal, (size_t)blockSize);
        *p_uCanonPhase = minTemp;
    }
    if(memcmp(pInOut,PDuplicat,(size_t)blockSize) == 0) 
        return 0;
    else
        return 1;
}

// swaps iVar and iVar+1 elements in pCanonPerm ant p_uCanonPhase
void swapInfoAdjacentVars(int iVar, char * pCanonPerm, unsigned* p_uCanonPhase)
{
    char Temp = pCanonPerm[iVar];
    pCanonPerm[iVar] = pCanonPerm[iVar+1];
    pCanonPerm[iVar+1] = Temp;
    
    // if the polarity of variables is different, swap them
    if ( ((*p_uCanonPhase & (1 << iVar)) > 0) != ((*p_uCanonPhase & (1 << (iVar+1))) > 0) )
    {
        *p_uCanonPhase ^= (1 << iVar);
        *p_uCanonPhase ^= (1 << (iVar+1));
    }
            
}


// compare F with  F1 = (F with swapped two adjacent vars).
// keeps smaller.
// same for all vars in F.
// returns: if pInOnt changed(minimized) by function return 1 if not 0


/*
// this version is buggy and is fixed below
int minimalSwap(word* pInOut, word* pMinimal, word* PDuplicat, int  nVars, char * pCanonPerm, char * tempArray, unsigned* p_uCanonPhase) 
{
    int i;  
    int blockSizeWord = Kit_TruthWordNum_64bit( nVars )*sizeof(word);
    int blockSizeChar = nVars *sizeof(char);
    memcpy(pMinimal, pInOut, blockSizeWord);
    memcpy(PDuplicat, pInOut, blockSizeWord);
    memcpy(tempArray, pCanonPerm, blockSizeChar);  
    Kit_TruthSwapAdjacentVars_64bit( pInOut, nVars, 0 );
    swapInfoAdjacentVars(0, pCanonPerm, p_uCanonPhase);
    for(i=1;i<nVars-1;i++)
    {
        if( memCompare(pMinimal,pInOut,nVars) == 1)
        {
            memcpy(pMinimal, pInOut, blockSizeWord);
            memcpy(tempArray, pCanonPerm, blockSizeChar);
        }
        else
        {
            memcpy(pInOut, pMinimal, blockSizeWord);
            memcpy(pCanonPerm, tempArray, blockSizeChar);   
        }
        Kit_TruthSwapAdjacentVars_64bit( pInOut, nVars, i );
        swapInfoAdjacentVars(i, pCanonPerm, p_uCanonPhase);
    }
    if( memCompare(pMinimal,pInOut,nVars) == -1)
    {
        memcpy(pInOut, pMinimal, blockSizeWord);
        memcpy(pCanonPerm, tempArray, blockSizeChar);
    }
    if(memcmp(pInOut,PDuplicat,blockSizeWord) == 0)
        return 0;
    else
        return 1;
}
*/

int minimalSwap(word* pInOut, word* pMinimal, word* PDuplicat, int  nVars, char * pCanonPerm, char * tempArray, unsigned* p_uCanonPhase) 
{
    int i;  
    int blockSizeWord = Kit_TruthWordNum_64bit( nVars )*sizeof(word);
    int blockSizeChar = nVars *sizeof(char);
    unsigned TempuCanonPhase = *p_uCanonPhase;
    memcpy(pMinimal, pInOut, (size_t)blockSizeWord);
    memcpy(PDuplicat, pInOut, (size_t)blockSizeWord);
    memcpy(tempArray, pCanonPerm, (size_t)blockSizeChar);  
    Kit_TruthSwapAdjacentVars_64bit( pInOut, nVars, 0 );
    swapInfoAdjacentVars(0, pCanonPerm, p_uCanonPhase);
    for(i=1;i<nVars-1;i++)
    {
        if( memCompare(pMinimal,pInOut,nVars) == 1)
        {
            memcpy(pMinimal, pInOut, (size_t)blockSizeWord);
            memcpy(tempArray, pCanonPerm, (size_t)blockSizeChar);
            TempuCanonPhase = *p_uCanonPhase;
            
        }
        else
        {
            memcpy(pInOut, pMinimal, (size_t)blockSizeWord);
            memcpy(pCanonPerm, tempArray, (size_t)blockSizeChar);
            *p_uCanonPhase = TempuCanonPhase;
        }
        Kit_TruthSwapAdjacentVars_64bit( pInOut, nVars, i );
        swapInfoAdjacentVars(i, pCanonPerm, p_uCanonPhase);
    }
    if( memCompare(pMinimal,pInOut,nVars) == -1)
    {
        memcpy(pInOut, pMinimal, (size_t)blockSizeWord);
        memcpy(pCanonPerm, tempArray, (size_t)blockSizeChar);
        *p_uCanonPhase = TempuCanonPhase;
    }
    if(memcmp(pInOut,PDuplicat,(size_t)blockSizeWord) == 0)
        return 0;
    else
        return 1;
}


//////////////// functions below just for Alan if he want to double check my program//////////////////////////////////
/////////////////You need swap_ij function or analogical one//////////////////////////////////////////////////////////
/*
void swapAndFlip(word* pAfter, int nVars, int iVarInPosition, int jVar, char * pCanonPerm, unsigned* pUCanonPhase)
{
    int Temp;
    swap_ij(pAfter, nVars, iVarInPosition, jVar);
    
    Temp = pCanonPerm[iVarInPosition];
    pCanonPerm[iVarInPosition] = pCanonPerm[jVar];
    pCanonPerm[jVar] = Temp;
    
    if ( ((*pUCanonPhase & (1 << iVarInPosition)) > 0) != ((*pUCanonPhase & (1 << jVar)) > 0) )
    {
        *pUCanonPhase ^= (1 << iVarInPosition);
        *pUCanonPhase ^= (1 << jVar);
    }
    if(*pUCanonPhase>>iVarInPosition & (unsigned)1 == 1)
        Kit_TruthChangePhase_64bit( pAfter, nVars, iVarInPosition );
    
}
int luckyCheck(word* pAfter, word* pBefore, int nVars, char * pCanonPerm, unsigned uCanonPhase)
{
    int i,j;
    char tempChar;
    for(j=0;j<nVars;j++)
    {
        tempChar = 'a'+ j;
        for(i=j;i<nVars;i++)
        {
            if(tempChar != pCanonPerm[i])
                continue;
            swapAndFlip(pAfter , nVars, j, i, pCanonPerm, &uCanonPhase);
            break;
        }
    }
    if(uCanonPhase>>nVars & (unsigned)1 == 1)
        Kit_TruthNot_64bit(pAfter, nVars );
    if(memcmp(pAfter, pBefore, Kit_TruthWordNum_64bit( nVars )*sizeof(word)) == 0)
        return 0;
    else
        return 1;
}
*/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void luckyCanonicizer(word* pInOut, word* pAux, word* pAux1, int  nVars, char * pCanonPerm, char * tempArray, unsigned* p_uCanonPhase)
{
    int counter=1;
    assert( nVars <= 16 );
    while(counter>0 )   //  && cycles < 10 if we wanna limit cycles
    {
        counter=0;
        counter += minimalInitialFlip(pInOut, nVars, p_uCanonPhase);
        counter += minimalFlip(pInOut, pAux, pAux1, nVars, p_uCanonPhase);
        counter += minimalSwap(pInOut, pAux, pAux1, nVars, pCanonPerm, tempArray, p_uCanonPhase);   
    }
}
// tries to find minimal F till F at the beginning of the loop is the same as at the end - irreducible 
unsigned luckyCanonicizer1_simple(word* pInOut, word* pAux, word* pAux1, int  nVars, char * pCanonPerm, unsigned CanonPhase)
{
    int counter=1;
    assert( nVars <= 16 );
    while(counter>0 )   //  && cycles < 10 if we wanna limit cycles
    {
        counter=0;
        counter += minimalInitialFlip1(pInOut, nVars);
        counter += minimalFlip1(pInOut, pAux, pAux1, nVars);
        counter += minimalSwap1(pInOut, pAux, pAux1, nVars);    
    }
    return CanonPhase;
}

void luckyCanonicizer_final(word* pInOut, word* pAux, word* pAux1, int  nVars)
{
    Kit_TruthSemiCanonicize_Yasha_simple( pInOut, nVars, NULL );
    luckyCanonicizer1_simple(pInOut, pAux, pAux1, nVars, NULL, 0);
}

// this procedure calls internal canoniziers
// it returns canonical phase (as return value) and canonical permutation (in pCanonPerm)
unsigned Kit_TruthSemiCanonicize_new_internal( word * pInOut, int nVars, char * pCanonPerm )
{
    word pAux[1024], pAux1[1024];
    char tempArray[16];
    unsigned CanonPhase;
    assert( nVars <= 16 );
    CanonPhase = Kit_TruthSemiCanonicize_Yasha( pInOut, nVars, pCanonPerm );
    luckyCanonicizer(pInOut, pAux, pAux1, nVars, pCanonPerm, tempArray, &CanonPhase);
    return CanonPhase;
}

// this procedure is translates truth table from 'unsingeds' into 'words' 
unsigned Kit_TruthSemiCanonicize_new( unsigned * pInOut, unsigned * pAux, int nVars, char * pCanonPerm )
{
    unsigned Result;
    if ( nVars < 6 )
    {
         word Temp = ((word)pInOut[0] << 32) | (word)pInOut[0];
         Result = Kit_TruthSemiCanonicize_new_internal( &Temp, nVars, pCanonPerm );
         pInOut[0] = (unsigned)Temp;
    }
    else
         Result = Kit_TruthSemiCanonicize_new_internal( (word *)pInOut, nVars, pCanonPerm );
    return Result;
}



// compile main() procedure only if running outside of ABC environment
#ifndef _RUNNING_ABC_

int main () 
{
//  char * pFileInput  = "nonDSDfunc06var1M.txt";
//  char * pFileInput1 = "partDSDfunc06var1M.txt";
//  char * pFileInput2 = "fullDSDfunc06var1M.txt";

//  char * pFileInput  = "nonDSDfunc10var100K.txt";
//  char * pFileInput1 = "partDSDfunc10var100K.txt";
//  char * pFileInput2 = "fullDSDfunc10var100K.txt";

//  char * pFileInput = "partDSDfunc12var100K.txt";
//  char * pFileInput  = "nonDSDfunc12var100K.txt";
//  char * pFileInput1 = "partDSDfunc12var100K.txt";
//  char * pFileInput2 = "fullDSDfunc12var100K.txt";

//  char * pFileInput  = "nonDSDfunc14var10K.txt";
//  char * pFileInput1 = "partDSDfunc14var10K.txt";
//  char * pFileInput2 = "fullDSDfunc14var10K.txt";

    char * pFileInput  = "nonDSDfunc16var10K.txt";
    char * pFileInput1 = "partDSDfunc16var10K.txt";
    char * pFileInput2 = "fullDSDfunc16var10K.txt";

    int i, j, tempNF;
    char** charArray;
    word** a, ** b;
    Abc_TtStore_t* p;
    word * pAux, * pAux1;
    int * pStore;
//  cycleCtr* cCtr;
    charArray = (char**)malloc(sizeof(char*)*3);

    charArray[0] = pFileInput;
    charArray[1] = pFileInput1;
    charArray[2] = pFileInput2;
    for(j=0;j<3;j++)
    {
        p = setTtStore(charArray[j]);   
//      p = setTtStore(pFileInput); 
        a = makeArray(p);
        b = makeArray(p);   
//      cCtr = setCycleCtrPtr();

        pAux = (word*)malloc(sizeof(word)*(p->nWords));
        pAux1 = (word*)malloc(sizeof(word)*(p->nWords));    
        pStore = (int*)malloc(sizeof(int)*(p->nVars));
        printf("In %s Fs at start = %d\n",charArray[j],p->nFuncs);
        
        tempNF = p->nFuncs;

        TimePrint("start");     
        for(i=0;i<p->nFuncs;i++)        
            luckyCanonicizer_final(a[i], pAux, pAux1, p->nVars, pStore);        
        TimePrint("done with A");

        sortAndUnique(a, p);
        printf("F left in A final = %d\n",p->nFuncs);
        freeArray(a,p);
        TimePrint("Done with sort");
        

// delete data-structures   
        free(pAux);
        free(pAux1);    
        free(pStore);
//      freeCycleCtr(cCtr);
        Abc_TruthStoreFree( p );
    }
    return 0;
}

#endif



ABC_NAMESPACE_IMPL_END
