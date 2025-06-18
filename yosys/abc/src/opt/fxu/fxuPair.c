/**CFile****************************************************************

  FileName    [fxuPair.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Operations on cube pairs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fxuPair.c,v 1.0 2003/02/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fxuInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define MAX_PRIMES      304

static int s_Primes[MAX_PRIMES] =
{
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 
    41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 
    97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 
    157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 
    227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 
    283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 
    367, 373, 379, 383, 389, 397, 401, 409, 419, 421, 431, 433, 
    439, 443, 449, 457, 461, 463, 467, 479, 487, 491, 499, 503, 
    509, 521, 523, 541, 547, 557, 563, 569, 571, 577, 587, 593, 
    599, 601, 607, 613, 617, 619, 631, 641, 643, 647, 653, 659, 
    661, 673, 677, 683, 691, 701, 709, 719, 727, 733, 739, 743, 
    751, 757, 761, 769, 773, 787, 797, 809, 811, 821, 823, 827, 
    829, 839, 853, 857, 859, 863, 877, 881, 883, 887, 907, 911, 
    919, 929, 937, 941, 947, 953, 967, 971, 977, 983, 991, 997, 
    1009, 1013, 1019, 1021, 1031, 1033, 1039, 1049, 1051, 1061, 1063, 1069, 
    1087, 1091, 1093, 1097, 1103, 1109, 1117, 1123, 1129, 1151, 1153, 1163, 
    1171, 1181, 1187, 1193, 1201, 1213, 1217, 1223, 1229, 1231, 1237, 1249, 
    1259, 1277, 1279, 1283, 1289, 1291, 1297, 1301, 1303, 1307, 1319, 1321, 
    1327, 1361, 1367, 1373, 1381, 1399, 1409, 1423, 1427, 1429, 1433, 1439, 
    1447, 1451, 1453, 1459, 1471, 1481, 1483, 1487, 1489, 1493, 1499, 1511, 
    1523, 1531, 1543, 1549, 1553, 1559, 1567, 1571, 1579, 1583, 1597, 1601, 
    1607, 1609, 1613, 1619, 1621, 1627, 1637, 1657, 1663, 1667, 1669, 1693, 
    1697, 1699, 1709, 1721, 1723, 1733, 1741, 1747, 1753, 1759, 1777, 1783, 
    1787, 1789, 1801, 1811, 1823, 1831, 1847, 1861, 1867, 1871, 1873, 1877, 
    1879, 1889, 1901, 1907, 1913, 1931, 1933, 1949, 1951, 1973, 1979, 1987, 
    1993, 1997, 1999, 2003 
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Find the canonical permutation of two cubes in the pair.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_PairCanonicize( Fxu_Cube ** ppCube1, Fxu_Cube ** ppCube2 )
{
    Fxu_Lit * pLit1, * pLit2;
    Fxu_Cube * pCubeTemp;

    // walk through the cubes to determine 
    // the one that has higher first variable
    pLit1 = (*ppCube1)->lLits.pHead;
    pLit2 = (*ppCube2)->lLits.pHead;
    while ( 1 )
    {
        if ( pLit1->iVar == pLit2->iVar )
        {
            pLit1 = pLit1->pHNext;
            pLit2 = pLit2->pHNext;
            continue;
        }
        assert( pLit1 && pLit2 ); // this is true if the covers are SCC-free
        if ( pLit1->iVar > pLit2->iVar )
        { // swap the cubes
            pCubeTemp = *ppCube1;
            *ppCube1  = *ppCube2;
            *ppCube2  = pCubeTemp;
        }
        break;
    }
}

/**Function*************************************************************

  Synopsis    [Find the canonical permutation of two cubes in the pair.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_PairCanonicize2( Fxu_Cube ** ppCube1, Fxu_Cube ** ppCube2 )
{
    Fxu_Cube * pCubeTemp;
    // canonicize the pair by ordering the cubes
    if ( (*ppCube1)->iCube > (*ppCube2)->iCube )
    { // swap the cubes
        pCubeTemp = *ppCube1;
        *ppCube1  = *ppCube2;
        *ppCube2  = pCubeTemp;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Fxu_PairHashKeyArray( Fxu_Matrix * p, int piVarsC1[], int piVarsC2[], int nVarsC1, int nVarsC2 )
{
    int Offset1 = 100, Offset2 = 200, i;
    unsigned Key;
    // compute the hash key
    Key = 0;
    for ( i = 0; i < nVarsC1; i++ )
        Key ^= s_Primes[Offset1+i] * piVarsC1[i];
    for ( i = 0; i < nVarsC2; i++ )
        Key ^= s_Primes[Offset2+i] * piVarsC2[i];
    return Key;
}

/**Function*************************************************************

  Synopsis    [Computes the hash key of the divisor represented by the pair of cubes.]

  Description [Goes through the variables in both cubes. Skips the identical
  ones (this corresponds to making the cubes cube-free). Computes the hash 
  value of the cubes. Assigns the number of literals in the base and in the 
  cubes without base.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Fxu_PairHashKey( Fxu_Matrix * p, Fxu_Cube * pCube1, Fxu_Cube * pCube2, 
                         int * pnBase, int * pnLits1, int * pnLits2 )
{
    int Offset1 = 100, Offset2 = 200;
    int nBase, nLits1, nLits2;
    Fxu_Lit * pLit1, * pLit2;
    unsigned Key;

    // compute the hash key
    Key    = 0;
    nLits1 = 0;
    nLits2 = 0;
    nBase  = 0;
    pLit1  = pCube1->lLits.pHead;
    pLit2  = pCube2->lLits.pHead;
    while ( 1 )
    {
        if ( pLit1 && pLit2 )
        {
            if ( pLit1->iVar == pLit2->iVar )
            { // ensure cube-free
                pLit1 = pLit1->pHNext;
                pLit2 = pLit2->pHNext;
                // add this literal to the base
                nBase++;
            }
            else if ( pLit1->iVar < pLit2->iVar )
            {
                Key  ^= s_Primes[Offset1+nLits1] * pLit1->iVar;
                pLit1 = pLit1->pHNext;
                nLits1++;
            }
            else
            {
                Key  ^= s_Primes[Offset2+nLits2] * pLit2->iVar;
                pLit2 = pLit2->pHNext;
                nLits2++;
            }
        }
        else if ( pLit1 && !pLit2 )
        {
            Key  ^= s_Primes[Offset1+nLits1] * pLit1->iVar;
            pLit1 = pLit1->pHNext;
            nLits1++;
        }
        else if ( !pLit1 && pLit2 )
        {
            Key  ^= s_Primes[Offset2+nLits2] * pLit2->iVar;
            pLit2 = pLit2->pHNext;
            nLits2++;
        }
        else
            break;
    }
    *pnBase  = nBase;
    *pnLits1 = nLits1;
    *pnLits2 = nLits2;
    return Key;
}

/**Function*************************************************************

  Synopsis    [Compares the two pairs.]

  Description [Returns 1 if the divisors represented by these pairs
  are equal.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fxu_PairCompare( Fxu_Pair * pPair1, Fxu_Pair * pPair2 )
{
    Fxu_Lit * pD1C1, * pD1C2;
    Fxu_Lit * pD2C1, * pD2C2;
    int TopVar1, TopVar2;
    int Code;

    if ( pPair1->nLits1 != pPair2->nLits1 )
        return 0;
    if ( pPair1->nLits2 != pPair2->nLits2 )
        return 0;

    pD1C1 = pPair1->pCube1->lLits.pHead;
    pD1C2 = pPair1->pCube2->lLits.pHead;

    pD2C1 = pPair2->pCube1->lLits.pHead;
    pD2C2 = pPair2->pCube2->lLits.pHead;

    Code  = pD1C1? 8: 0;
    Code |= pD1C2? 4: 0;
    Code |= pD2C1? 2: 0;
    Code |= pD2C2? 1: 0;
    assert( Code == 15 );

    while ( 1 )
    {
        switch ( Code )
        {
        case 0:  // -- --      NULL   NULL     NULL   NULL 
            return 1;
        case 1:  // -- -1      NULL   NULL     NULL   pD2C2
            return 0;
        case 2:  // -- 1-      NULL   NULL     pD2C1  NULL 
            return 0;
        case 3:  // -- 11      NULL   NULL     pD2C1  pD2C2
            if ( pD2C1->iVar != pD2C2->iVar )
                return 0;
            pD2C1 = pD2C1->pHNext;
            pD2C2 = pD2C2->pHNext;
            break;
        case 4:  // -1 --      NULL   pD1C2    NULL   NULL 
            return 0;
        case 5:  // -1 -1      NULL   pD1C2    NULL   pD2C2
            if ( pD1C2->iVar != pD2C2->iVar )
                return 0;
            pD1C2 = pD1C2->pHNext;
            pD2C2 = pD2C2->pHNext;
            break;
        case 6:  // -1 1-      NULL   pD1C2    pD2C1  NULL 
            return 0;
        case 7:  // -1 11      NULL   pD1C2    pD2C1  pD2C2
            TopVar2 = Fxu_Min( pD2C1->iVar, pD2C2->iVar );
            if ( TopVar2 == pD1C2->iVar )
            {
                if ( pD2C1->iVar <= pD2C2->iVar )
                    return 0;
                pD1C2 = pD1C2->pHNext;
                pD2C2 = pD2C2->pHNext;
            }
            else if ( TopVar2 < pD1C2->iVar )
            {
                if ( pD2C1->iVar != pD2C2->iVar )
                    return 0;
                pD2C1 = pD2C1->pHNext;
                pD2C2 = pD2C2->pHNext;
            }
            else
                return 0;
            break;
        case 8:  // 1- --      pD1C1  NULL     NULL   NULL 
            return 0;
        case 9:  // 1- -1      pD1C1  NULL     NULL   pD2C2
            return 0;
        case 10: // 1- 1-      pD1C1  NULL     pD2C1  NULL 
            if ( pD1C1->iVar != pD2C1->iVar )
                return 0;
            pD1C1 = pD1C1->pHNext;
            pD2C1 = pD2C1->pHNext;
            break;
        case 11: // 1- 11      pD1C1  NULL     pD2C1  pD2C2
            TopVar2 = Fxu_Min( pD2C1->iVar, pD2C2->iVar );
            if ( TopVar2 == pD1C1->iVar )
            {
                if ( pD2C1->iVar >= pD2C2->iVar )
                    return 0;
                pD1C1 = pD1C1->pHNext;
                pD2C1 = pD2C1->pHNext;
            }
            else if ( TopVar2 < pD1C1->iVar )
            {
                if ( pD2C1->iVar != pD2C2->iVar )
                    return 0;
                pD2C1 = pD2C1->pHNext;
                pD2C2 = pD2C2->pHNext;
            }
            else
                return 0;
            break;
        case 12: // 11 --      pD1C1  pD1C2    NULL   NULL 
            if ( pD1C1->iVar != pD1C2->iVar )
                return 0;
            pD1C1 = pD1C1->pHNext;
            pD1C2 = pD1C2->pHNext;
            break;
        case 13: // 11 -1      pD1C1  pD1C2    NULL   pD2C2
            TopVar1 = Fxu_Min( pD1C1->iVar, pD1C2->iVar );
            if ( TopVar1 == pD2C2->iVar )
            {
                if ( pD1C1->iVar <= pD1C2->iVar )
                    return 0;
                pD1C2 = pD1C2->pHNext;
                pD2C2 = pD2C2->pHNext;
            }
            else if ( TopVar1 < pD2C2->iVar )
            {
                if ( pD1C1->iVar != pD1C2->iVar )
                    return 0;
                pD1C1 = pD1C1->pHNext;
                pD1C2 = pD1C2->pHNext;
            }
            else
                return 0;
            break;
        case 14: // 11 1-      pD1C1  pD1C2    pD2C1  NULL 
            TopVar1 = Fxu_Min( pD1C1->iVar, pD1C2->iVar );
            if ( TopVar1 == pD2C1->iVar )
            {
                if ( pD1C1->iVar >= pD1C2->iVar )
                    return 0;
                pD1C1 = pD1C1->pHNext;
                pD2C1 = pD2C1->pHNext;
            }
            else if ( TopVar1 < pD2C1->iVar )
            {
                if ( pD1C1->iVar != pD1C2->iVar )
                    return 0;
                pD1C1 = pD1C1->pHNext;
                pD1C2 = pD1C2->pHNext;
            }
            else
                return 0;
            break;
        case 15: // 11 11      pD1C1  pD1C2    pD2C1  pD2C2
            TopVar1 = Fxu_Min( pD1C1->iVar, pD1C2->iVar );
            TopVar2 = Fxu_Min( pD2C1->iVar, pD2C2->iVar );
            if ( TopVar1 == TopVar2 )
            {
                if ( pD1C1->iVar == pD1C2->iVar )
                {
                    if ( pD2C1->iVar != pD2C2->iVar )
                        return 0;
                    pD1C1 = pD1C1->pHNext;
                    pD1C2 = pD1C2->pHNext;
                    pD2C1 = pD2C1->pHNext;
                    pD2C2 = pD2C2->pHNext;
                }
                else
                {
                    if ( pD2C1->iVar == pD2C2->iVar )
                        return 0;
                    if ( pD1C1->iVar < pD1C2->iVar )
                    {
                        if ( pD2C1->iVar > pD2C2->iVar )
                            return 0;
                        pD1C1 = pD1C1->pHNext;
                        pD2C1 = pD2C1->pHNext;
                    }
                    else
                    {
                        if ( pD2C1->iVar < pD2C2->iVar )
                            return 0;
                        pD1C2 = pD1C2->pHNext;
                        pD2C2 = pD2C2->pHNext;
                    }
                }
            }
            else if ( TopVar1 < TopVar2 )
            {
                if ( pD1C1->iVar != pD1C2->iVar )
                    return 0;
                pD1C1 = pD1C1->pHNext;
                pD1C2 = pD1C2->pHNext;
            }
            else 
            {
                if ( pD2C1->iVar != pD2C2->iVar )
                    return 0;
                pD2C1 = pD2C1->pHNext;
                pD2C2 = pD2C2->pHNext;
            }
            break;
        default:
            assert( 0 );
            break;
        }

        Code  = pD1C1? 8: 0;
        Code |= pD1C2? 4: 0;
        Code |= pD2C1? 2: 0;
        Code |= pD2C2? 1: 0;
    }
    return 1;
}


/**Function*************************************************************

  Synopsis    [Allocates the storage for cubes pairs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_PairAllocStorage( Fxu_Var * pVar, int nCubes )
{
    int k;
//    assert( pVar->nCubes == 0 );
    pVar->nCubes  = nCubes;
    // allocate memory for all the pairs
    pVar->ppPairs    = ABC_ALLOC( Fxu_Pair **, nCubes );
    pVar->ppPairs[0] = ABC_ALLOC( Fxu_Pair *,  nCubes * nCubes );
    memset( pVar->ppPairs[0], 0, sizeof(Fxu_Pair *) * nCubes * nCubes );
    for ( k = 1; k < nCubes; k++ )
        pVar->ppPairs[k] = pVar->ppPairs[k-1] + nCubes;
}

/**Function*************************************************************

  Synopsis    [Clears all pairs associated with this cube.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_PairClearStorage( Fxu_Cube * pCube )
{
    Fxu_Var * pVar;
    int i;
    pVar = pCube->pVar;
    for ( i = 0; i < pVar->nCubes; i++ )
    {
        pVar->ppPairs[pCube->iCube][i] = NULL;
        pVar->ppPairs[i][pCube->iCube] = NULL;
    }
}

/**Function*************************************************************

  Synopsis    [Clears all pairs associated with this cube.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_PairFreeStorage( Fxu_Var * pVar )
{
    if ( pVar->ppPairs )
    {
        ABC_FREE( pVar->ppPairs[0] );
        ABC_FREE( pVar->ppPairs );
    }
}

/**Function*************************************************************

  Synopsis    [Adds the pair to storage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fxu_Pair * Fxu_PairAlloc( Fxu_Matrix * p, Fxu_Cube * pCube1, Fxu_Cube * pCube2 )
{
    Fxu_Pair * pPair;
       assert( pCube1->pVar == pCube2->pVar );
    pPair = MEM_ALLOC_FXU( p, Fxu_Pair, 1 );
    memset( pPair, 0, sizeof(Fxu_Pair) );
    pPair->pCube1 = pCube1;
    pPair->pCube2 = pCube2;
    pPair->iCube1 = pCube1->iCube;
    pPair->iCube2 = pCube2->iCube;
    return pPair;
}

/**Function*************************************************************

  Synopsis    [Adds the pair to storage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_PairAdd( Fxu_Pair * pPair )
{
    Fxu_Var * pVar;

    pVar = pPair->pCube1->pVar;
    assert( pVar == pPair->pCube2->pVar );

    pVar->ppPairs[pPair->iCube1][pPair->iCube2] = pPair;
    pVar->ppPairs[pPair->iCube2][pPair->iCube1] = pPair;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

