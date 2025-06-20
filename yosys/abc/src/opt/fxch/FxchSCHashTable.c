/**CFile****************************************************************

  FileName    [ FxchSCHashTable.c ]

  PackageName [ Fast eXtract with Cube Hashing (FXCH) ]

  Synopsis    [ Sub-cubes hash table implementation ]

  Author      [ Bruno Schmitt - boschmitt at inf.ufrgs.br ]

  Affiliation [ UFRGS ]

  Date        [ Ver. 1.0. Started - March 6, 2016. ]

  Revision    []

***********************************************************************/
#include "Fxch.h"

#if (__GNUC__ >= 8)
  #pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
static inline void MurmurHash3_x86_32 ( const void* key,
                                        int len,
                                        uint32_t seed,
                                        void* out )
{
    const uint8_t* data = (const uint8_t*)key;
    const int nblocks = len / 4;

    uint32_t h1 = seed;

    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    const uint8_t * tail;
    uint32_t k1;

    //----------
    // body

    const uint32_t * blocks = (const uint32_t *)(data + nblocks*4);
    int i;

    for(i = -nblocks; i; i++)
    {
        uint32_t k1 = blocks[i];

        k1 *= c1;
        k1 = (k1 << 15) | (k1 >> (32 - 15));
        k1 *= c2;

        h1 ^= k1;
        h1 = (h1 << 13) | (h1 >> (32 - 13));
        h1 = h1*5+0xe6546b64;
    }

    //----------
    // tail

    tail = (const uint8_t*)(data + nblocks*4);

    k1 = 0;

    switch(len & 3)
    {
        case 3: k1 ^= tail[2] << 16;
        case 2: k1 ^= tail[1] << 8;
        case 1: k1 ^= tail[0];
              k1 *= c1; k1 = (k1 << 15) | (k1 >> (32 - 15)); k1 *= c2; h1 ^= k1;
    };

    //----------
    // finalization

    h1 ^= len;

    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;

    *(uint32_t*)out = h1;
}

Fxch_SCHashTable_t* Fxch_SCHashTableCreate( Fxch_Man_t* pFxchMan,
                                            int nEntries )
{
    Fxch_SCHashTable_t* pSCHashTable = ABC_CALLOC( Fxch_SCHashTable_t, 1 );
    int nBits = Abc_Base2Log( nEntries + 1 );


    pSCHashTable->pFxchMan = pFxchMan;
    pSCHashTable->SizeMask = (1 << nBits) - 1;
    pSCHashTable->pBins = ABC_CALLOC( Fxch_SCHashTable_Entry_t, pSCHashTable->SizeMask + 1 );

    return pSCHashTable;
}

void Fxch_SCHashTableDelete( Fxch_SCHashTable_t* pSCHashTable )
{
    unsigned i;
    for ( i = 0; i <= pSCHashTable->SizeMask; i++ )
        ABC_FREE( pSCHashTable->pBins[i].vSCData );
    Vec_IntErase( &pSCHashTable->vSubCube0 );
    Vec_IntErase( &pSCHashTable->vSubCube1 );
    ABC_FREE( pSCHashTable->pBins );
    ABC_FREE( pSCHashTable );
}

static inline Fxch_SCHashTable_Entry_t* Fxch_SCHashTableBin( Fxch_SCHashTable_t* pSCHashTable,
                                                             unsigned int SubCubeID )
{
    return pSCHashTable->pBins + (SubCubeID & pSCHashTable->SizeMask);
}

static inline int Fxch_SCHashTableEntryCompare( Fxch_SCHashTable_t* pSCHashTable,
                                                Vec_Wec_t* vCubes,
                                                Fxch_SubCube_t* pSCData0,
                                                Fxch_SubCube_t* pSCData1 )
{
    Vec_Int_t* vCube0 = Vec_WecEntry( vCubes, pSCData0->iCube ),
             * vCube1 = Vec_WecEntry( vCubes, pSCData1->iCube );

    int* pOutputID0 = Vec_IntEntryP( pSCHashTable->pFxchMan->vOutputID, pSCData0->iCube * pSCHashTable->pFxchMan->nSizeOutputID ),
       * pOutputID1 = Vec_IntEntryP( pSCHashTable->pFxchMan->vOutputID, pSCData1->iCube * pSCHashTable->pFxchMan->nSizeOutputID );
    int i, Result = 0;

    if ( !Vec_IntSize( vCube0 ) ||
         !Vec_IntSize( vCube1 ) ||
         Vec_IntEntry( vCube0, 0 ) != Vec_IntEntry( vCube1, 0 ) ||
         pSCData0->Id != pSCData1->Id )
        return 0;

    for ( i = 0; i < pSCHashTable->pFxchMan->nSizeOutputID && Result == 0; i++ )
        Result = ( pOutputID0[i] & pOutputID1[i] );

    if ( Result == 0 )
        return 0;

    Vec_IntClear( &pSCHashTable->vSubCube0 );
    Vec_IntClear( &pSCHashTable->vSubCube1 );

    if ( pSCData0->iLit1 > 0 && pSCData1->iLit1 > 0 &&
         ( Vec_IntEntry( vCube0, pSCData0->iLit0 ) == Vec_IntEntry( vCube1, pSCData1->iLit0 ) ||
           Vec_IntEntry( vCube0, pSCData0->iLit0 ) == Vec_IntEntry( vCube1, pSCData1->iLit1 ) ||
           Vec_IntEntry( vCube0, pSCData0->iLit1 ) == Vec_IntEntry( vCube1, pSCData1->iLit0 ) ||
           Vec_IntEntry( vCube0, pSCData0->iLit1 ) == Vec_IntEntry( vCube1, pSCData1->iLit1 ) ) )
        return 0;

    if ( pSCData0->iLit0 > 0 )
        Vec_IntAppendSkip( &pSCHashTable->vSubCube0, vCube0, pSCData0->iLit0 );
    else
        Vec_IntAppend( &pSCHashTable->vSubCube0, vCube0 );

    if ( pSCData1->iLit0 > 0 )
        Vec_IntAppendSkip( &pSCHashTable->vSubCube1, vCube1, pSCData1->iLit0 );
    else
        Vec_IntAppend( &pSCHashTable->vSubCube1, vCube1 );

    if ( pSCData0->iLit1 > 0)
        Vec_IntDrop( &pSCHashTable->vSubCube0,
                       pSCData0->iLit0 < pSCData0->iLit1 ? pSCData0->iLit1 - 1 : pSCData0->iLit1 );

    if ( pSCData1->iLit1 > 0 )
        Vec_IntDrop( &pSCHashTable->vSubCube1,
                       pSCData1->iLit0 < pSCData1->iLit1 ? pSCData1->iLit1 - 1 : pSCData1->iLit1 );

    return Vec_IntEqual( &pSCHashTable->vSubCube0, &pSCHashTable->vSubCube1 );
}

int Fxch_SCHashTableInsert( Fxch_SCHashTable_t* pSCHashTable,
                            Vec_Wec_t* vCubes,
                            uint32_t SubCubeID,
                            uint32_t iCube,
                            uint32_t iLit0,
                            uint32_t iLit1,
                            char fUpdate )
{
    int iNewEntry;
    int Pairs = 0;
    uint32_t BinID;
    Fxch_SCHashTable_Entry_t* pBin;
    Fxch_SubCube_t* pNewEntry;
    int iEntry;

    MurmurHash3_x86_32( ( void* ) &SubCubeID, sizeof( int ), 0x9747b28c, &BinID);
    pBin = Fxch_SCHashTableBin( pSCHashTable, BinID );

    if ( pBin->vSCData == NULL )
    {
        pBin->vSCData = ABC_CALLOC( Fxch_SubCube_t, 16 );
        pBin->Size = 0;
        pBin->Cap = 16;
    }
    else if ( pBin->Size == pBin->Cap )
    {
        assert(pBin->Cap <= 0xAAAA);
        pBin->Cap = ( pBin->Cap >> 1 ) * 3;
        pBin->vSCData = ABC_REALLOC( Fxch_SubCube_t, pBin->vSCData, pBin->Cap );
    }

    iNewEntry = pBin->Size++;
    pBin->vSCData[iNewEntry].Id = SubCubeID;
    pBin->vSCData[iNewEntry].iCube = iCube;
    pBin->vSCData[iNewEntry].iLit0 = iLit0;
    pBin->vSCData[iNewEntry].iLit1 = iLit1;
    pSCHashTable->nEntries++;

    if ( pBin->Size == 1 )
        return 0;

    pNewEntry = &( pBin->vSCData[iNewEntry] );
    for ( iEntry = 0; iEntry < (int)pBin->Size - 1; iEntry++ )
    {
        Fxch_SubCube_t* pEntry = &( pBin->vSCData[iEntry] );
        int* pOutputID0 = Vec_IntEntryP( pSCHashTable->pFxchMan->vOutputID, pEntry->iCube * pSCHashTable->pFxchMan->nSizeOutputID );
        int* pOutputID1 = Vec_IntEntryP( pSCHashTable->pFxchMan->vOutputID, pNewEntry->iCube * pSCHashTable->pFxchMan->nSizeOutputID );
        int Result = 0;
        int Base;
        int iNewDiv = -1, i, z;

        if ( (pEntry->iLit1 != 0 && pNewEntry->iLit1 == 0) || (pEntry->iLit1 == 0 && pNewEntry->iLit1 != 0)  )
            continue;

        if ( !Fxch_SCHashTableEntryCompare( pSCHashTable, vCubes, pEntry, pNewEntry ) )
            continue;

        if ( ( pEntry->iLit0 == 0 ) || ( pNewEntry->iLit0 == 0 ) )
        {
            Vec_Int_t* vCube0 = Fxch_ManGetCube( pSCHashTable->pFxchMan, pEntry->iCube ),
                     * vCube1 = Fxch_ManGetCube( pSCHashTable->pFxchMan, pNewEntry->iCube );

            if ( Vec_IntSize( vCube0 ) > Vec_IntSize( vCube1 ) )
            {
                Vec_IntPush( pSCHashTable->pFxchMan->vSCC, pEntry->iCube );
                Vec_IntPush( pSCHashTable->pFxchMan->vSCC, pNewEntry->iCube );
            }
            else
            {
                Vec_IntPush( pSCHashTable->pFxchMan->vSCC, pNewEntry->iCube );
                Vec_IntPush( pSCHashTable->pFxchMan->vSCC, pEntry->iCube );
            }

            continue;
        }

        Base = Fxch_DivCreate( pSCHashTable->pFxchMan, pEntry, pNewEntry );

        if ( Base < 0 )
            continue;

        for ( i = 0; i < pSCHashTable->pFxchMan->nSizeOutputID; i++ )
            Result += Fxch_CountOnes( pOutputID0[i] & pOutputID1[i] );

        for ( z = 0; z < Result; z++ )
            iNewDiv = Fxch_DivAdd( pSCHashTable->pFxchMan, fUpdate, 0, Base );

        Vec_WecPush( pSCHashTable->pFxchMan->vDivCubePairs, iNewDiv, pEntry->iCube );
        Vec_WecPush( pSCHashTable->pFxchMan->vDivCubePairs, iNewDiv, pNewEntry->iCube );

        Pairs++;
    }

    return Pairs;
}

int Fxch_SCHashTableRemove( Fxch_SCHashTable_t* pSCHashTable,
                            Vec_Wec_t* vCubes,
                            uint32_t SubCubeID,
                            uint32_t iCube,
                            uint32_t iLit0,
                            uint32_t iLit1,
                            char fUpdate )
{
    int iEntry;
    int Pairs = 0;
    uint32_t BinID;
    Fxch_SCHashTable_Entry_t* pBin;
    Fxch_SubCube_t* pEntry;
    int idx;

    MurmurHash3_x86_32( ( void* ) &SubCubeID, sizeof( int ), 0x9747b28c, &BinID);

    pBin = Fxch_SCHashTableBin( pSCHashTable, BinID );

    if ( pBin->Size == 1 )
    {
        pBin->Size = 0;
        return 0;
    }

    for ( iEntry = 0; iEntry < (int)pBin->Size; iEntry++ )
        if ( pBin->vSCData[iEntry].iCube == iCube )
            break;

    assert( ( iEntry != (int)pBin->Size ) && ( pBin->Size != 0 ) );

    pEntry = &( pBin->vSCData[iEntry] );
    for ( idx = 0; idx < (int)pBin->Size; idx++ )
    if ( idx != iEntry )
    {
        int Base,
            iDiv = -1;

        int i, z,
            iCube0,
            iCube1;

        Fxch_SubCube_t* pNextEntry = &( pBin->vSCData[idx] );
        Vec_Int_t* vDivCubePairs;
        int* pOutputID0 = Vec_IntEntryP( pSCHashTable->pFxchMan->vOutputID, pEntry->iCube * pSCHashTable->pFxchMan->nSizeOutputID );
        int* pOutputID1 = Vec_IntEntryP( pSCHashTable->pFxchMan->vOutputID, pNextEntry->iCube * pSCHashTable->pFxchMan->nSizeOutputID );
        int Result = 0;

        if ( (pEntry->iLit1 != 0 && pNextEntry->iLit1 == 0) || (pEntry->iLit1 == 0 && pNextEntry->iLit1 != 0)  )
            continue;

        if ( !Fxch_SCHashTableEntryCompare( pSCHashTable, vCubes, pEntry, pNextEntry )
             || pEntry->iLit0 == 0
             || pNextEntry->iLit0 == 0 )
            continue;

        Base = Fxch_DivCreate( pSCHashTable->pFxchMan, pNextEntry, pEntry );

        if ( Base < 0 )
            continue;

        for ( i = 0; i < pSCHashTable->pFxchMan->nSizeOutputID; i++ )
            Result += Fxch_CountOnes( pOutputID0[i] & pOutputID1[i] );

        for ( z = 0; z < Result; z++ )
            iDiv = Fxch_DivRemove( pSCHashTable->pFxchMan, fUpdate, 0, Base );

        vDivCubePairs = Vec_WecEntry( pSCHashTable->pFxchMan->vDivCubePairs, iDiv );
        Vec_IntForEachEntryDouble( vDivCubePairs, iCube0, iCube1, i )
            if ( ( iCube0 == (int)pNextEntry->iCube && iCube1 == (int)pEntry->iCube )  ||
                 ( iCube0 == (int)pEntry->iCube && iCube1 == (int)pNextEntry->iCube ) )
            {
                Vec_IntDrop( vDivCubePairs, i+1 );
                Vec_IntDrop( vDivCubePairs, i );
            }
        if ( Vec_IntSize( vDivCubePairs ) == 0 )
            Vec_IntErase( vDivCubePairs );

        Pairs++;
    }

    memmove(pBin->vSCData + iEntry, pBin->vSCData + iEntry + 1, (pBin->Size - iEntry - 1) * sizeof(*pBin->vSCData));
    pBin->Size -= 1;

    return Pairs;
}

unsigned int Fxch_SCHashTableMemory( Fxch_SCHashTable_t* pHashTable )
{
    unsigned int Memory = sizeof ( Fxch_SCHashTable_t );

    Memory += sizeof( Fxch_SubCube_t ) * ( pHashTable->SizeMask + 1 );

    return Memory;
}

void Fxch_SCHashTablePrint( Fxch_SCHashTable_t* pHashTable )
{
    int Memory;
    printf( "SubCube Hash Table at %p\n", ( void* )pHashTable );
    printf("%20s %20s\n", "nEntries",
                          "Memory Usage (MB)" );

    Memory = Fxch_SCHashTableMemory( pHashTable );
    printf("%20d %18.2f\n", pHashTable->nEntries,
                            ( ( double ) Memory / 1048576 ) );
}
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END
