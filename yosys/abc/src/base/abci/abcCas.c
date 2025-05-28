/**CFile****************************************************************

  FileName    [abcCas.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Decomposition of shared BDDs into LUT cascade.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcCas.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "bool/kit/kit.h"
#include "aig/miniaig/miniaig.h"
#include "misc/util/utilTruth.h"

#ifdef ABC_USE_CUDD
#include "bdd/extrab/extraBdd.h"
//#include "bdd/extrab/extraLutCas.h"
#endif

ABC_NAMESPACE_IMPL_START


/* 
    This LUT cascade synthesis algorithm is described in the paper:
    A. Mishchenko and T. Sasao, "Encoding of Boolean functions and its 
    application to LUT cascade synthesis", Proc. IWLS '02, pp. 115-120.
    http://www.eecs.berkeley.edu/~alanmi/publications/2002/iwls02_enc.pdf
*/

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#ifdef ABC_USE_CUDD

extern int Abc_CascadeExperiment( char * pFileGeneric, DdManager * dd, DdNode ** pOutputs, int nInputs, int nOutputs, int nLutSize, int fCheck, int fVerbose );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkCascade( Abc_Ntk_t * pNtk, int nLutSize, int fCheck, int fVerbose )
{
    DdManager * dd;
    DdNode ** ppOutputs;
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pNode;
    char * pFileGeneric;
    int fBddSizeMax = 500000;
    int i, fReorder = 1;
    abctime clk = Abc_Clock();

    assert( Abc_NtkIsStrash(pNtk) );
    // compute the global BDDs
    if ( Abc_NtkBuildGlobalBdds(pNtk, fBddSizeMax, 1, fReorder, 0, fVerbose) == NULL )
        return NULL;

    if ( fVerbose )
    {
        DdManager * dd = (DdManager *)Abc_NtkGlobalBddMan( pNtk );
        printf( "Shared BDD size = %6d nodes.  ", Cudd_ReadKeys(dd) - Cudd_ReadDead(dd) );
        ABC_PRT( "BDD construction time", Abc_Clock() - clk );
    }

    // collect global BDDs
    dd = (DdManager *)Abc_NtkGlobalBddMan( pNtk );
    ppOutputs = ABC_ALLOC( DdNode *, Abc_NtkCoNum(pNtk) );
    Abc_NtkForEachCo( pNtk, pNode, i )
        ppOutputs[i] = (DdNode *)Abc_ObjGlobalBdd(pNode);

    // call the decomposition
    pFileGeneric = Extra_FileNameGeneric( pNtk->pSpec );
    if ( !Abc_CascadeExperiment( pFileGeneric, dd, ppOutputs, Abc_NtkCiNum(pNtk), Abc_NtkCoNum(pNtk), nLutSize, fCheck, fVerbose ) )
    {
        // the LUT size is too small
    }

    // for now, duplicate the network
    pNtkNew = Abc_NtkDup( pNtk );

    // cleanup
    Abc_NtkFreeGlobalBdds( pNtk, 1 );
    ABC_FREE( ppOutputs );
    ABC_FREE( pFileGeneric );

//    if ( pNtk->pExdc )
//        pNtkNew->pExdc = Abc_NtkDup( pNtk->pExdc );
    // make sure that everything is okay
    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        printf( "Abc_NtkCollapse: The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}

#else

Abc_Ntk_t * Abc_NtkCascade( Abc_Ntk_t * pNtk, int nLutSize, int fCheck, int fVerbose ) { return NULL; }
word * Abc_LutCascade( Mini_Aig_t * p, int nLutSize, int nLuts, int nRails, int nIters, int fVerbose ) { return NULL; }

#endif


/*
    The decomposed structure of the LUT cascade is represented as an array of 64-bit integers (words).
    The first word in the record is the number of LUT info blocks in the record, which follow one by one.
    Each LUT info block contains the following:
    - the number of words in this block
    - the number of fanins
    - the list of fanins
    - the variable ID of the output (should be a LUT counter starting with the number of variables)
    - truth tables (one word for 6 vars or less; more words as needed for more than 6 vars)
    For a 6-input node, the LUT info block takes 10 words (block size, fanin count, 6 fanins, output ID, truth table).
    For a 4-input node, the LUT info block takes  8 words (block size, fanin count, 4 fanins, output ID, truth table).
    If the LUT cascade contains a 6-LUT followed by a 4-LUT, the record contains 1+10+8=19 words.
*/

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
word * Abc_LutCascadeGenTest()
{
    word * pLuts = ABC_CALLOC( word, 20 ); int i;
    // node count    
    pLuts[0] = 2;
    // first node
    pLuts[1+0] = 10;
    pLuts[1+1] =  6;
    for ( i = 0; i < 6; i++ )
        pLuts[1+2+i] = i;
    pLuts[1+8] = 9;
    pLuts[1+9] = ABC_CONST(0x8000000000000000);
    // second node    
    pLuts[11+0] = 8;
    pLuts[11+1] = 4;
    for ( i = 0; i < 4; i++ )
        pLuts[11+2+i] = i ? i + 5 : 9;
    pLuts[11+6] = 10;
    pLuts[11+7] = ABC_CONST(0xFFFEFFFEFFFEFFFE);
    return pLuts;
}
void Abc_LutCascadePrintLut( word * pLuts, int n, int i )
{
    word nIns   = pLuts[i+1];
    word * pIns = pLuts+i+2;
    word * pT   = pLuts+i+2+nIns+1;
    printf( "LUT%d : ", n );
    printf( "%c = F( ", 'a'+(int)pIns[nIns] );
    for ( int k = 0; k < nIns; k++ )
        printf( "%c ", 'a'+(int)pIns[k] );
    printf( ")  " );
    Abc_TtPrintHexRev( stdout, pT, nIns );
    printf( "\n" );
}
void Abc_LutCascadePrint( word * pLuts )
{
    int n, i;
    printf( "The LUT cascade contains %d LUTs:\n", (int)pLuts[0] );
    for ( n = 0, i = 1; n < pLuts[0]; n++, i += pLuts[i] )
        Abc_LutCascadePrintLut( pLuts, n, i );
}
void Abc_LutCascadeGenOne( Vec_Wrd_t * vRes, int nIns, int * pIns, int Out, word * p )
{
    int w, nWords = Abc_TtWordNum(nIns);
    //int iStart = Vec_WrdSize(vRes);
    Vec_WrdAddToEntry(vRes, 0, 1);
    Vec_WrdPush( vRes, 3+nIns+nWords );
    Vec_WrdPush( vRes, nIns );
    for ( w = 0; w < nIns; w++ )
        Vec_WrdPush( vRes, pIns[w] );
    Vec_WrdPush( vRes, Out );
    if ( nIns < 6 )
        Vec_WrdPush( vRes, p ? p[0] : Abc_Tt6Stretch(Abc_Random(0), nIns) );
    else
        for ( w = 0; w < nWords; w++ )
            Vec_WrdPush( vRes, p ? p[w] : Abc_RandomW(0) );
    //printf("Adding LUT: "); Abc_LutCascadePrintLut( Vec_WrdArray(vRes), 0, iStart );
}
word * Abc_LutCascadeGen( int nVars, int nLutSize, int nRails, int nShared )
{
    assert( nLutSize - nRails - nShared > 0 );
    Vec_Wrd_t * vRes    = Vec_WrdStart( 1 );
    Vec_Int_t * vFanins = Vec_IntAlloc( nLutSize );
    Vec_Int_t * vVars   = Vec_IntStartNatural( nVars );
    Vec_Str_t * vGuide  = Vec_StrAlloc( 100 );
    Abc_Random(1);
    int i, c = 0, Obj, iVarNext = nVars, iVarPrev = -1;
    while ( Vec_IntSize(vVars) > nLutSize ) {
        if ( Vec_WrdSize(vRes) > 1 )
            for ( i = 0; i < nRails; i++ )
                Vec_IntPop( vVars );
        Vec_StrPush( vGuide, '0'+c++ );
        Vec_IntClear( vFanins );
        for ( i = 0; i < nShared; i++ ) {
            int Index = -1;
            do Index = Abc_Random(0) % Vec_IntSize(vVars);
            while ( Vec_IntFind(vFanins, Vec_IntEntry(vVars, Index)) >= 0 );
            Vec_IntPush( vFanins, Vec_IntEntry(vVars, Index) );
            Vec_StrPush( vGuide, 'A'+Vec_IntEntry(vVars, Index) );
        }
        for ( i = 0; i < nLutSize - nRails - nShared; i++ ) {
            int Index = -1;
            do Index = Abc_Random(0) % Vec_IntSize(vVars);
            while ( Vec_IntFind(vFanins, Vec_IntEntry(vVars, Index)) >= 0 );
            Vec_IntPush( vFanins, Vec_IntEntry(vVars, Index) );
            Vec_StrPush( vGuide, 'a'+Vec_IntEntry(vVars, Index) );
            Vec_IntDrop( vVars, Index );
        }        
        if ( Vec_WrdSize(vRes) == 1 ) {
            for ( i = 0; i < nRails; i++ ) {
                int Index = -1;
                do Index = Abc_Random(0) % Vec_IntSize(vVars);
                while ( Vec_IntFind(vFanins, Vec_IntEntry(vVars, Index)) >= 0 );
                Vec_IntPush( vFanins, Vec_IntEntry(vVars, Index) );
                Vec_StrPush( vGuide, 'a'+Vec_IntEntry(vVars, Index) );
                Vec_IntDrop( vVars, Index );
            }
        }
        else {
            assert( iVarPrev > 0 );
            for ( i = 0; i < nRails; i++ ) {
                Vec_IntPush( vFanins, iVarPrev+i );
                Vec_StrPush( vGuide, 'a'+iVarPrev+i );
            }
        }
        iVarPrev = iVarNext;
        assert( Vec_IntSize(vFanins) == nLutSize );
        for ( i = 0; i < nRails; i++ ) {
            Abc_LutCascadeGenOne( vRes, Vec_IntSize(vFanins), Vec_IntArray(vFanins), iVarNext++, NULL );
            Vec_IntPush( vVars, iVarPrev+i );
        }
    }
    assert( Vec_IntSize(vVars) <= nLutSize );
    Abc_LutCascadeGenOne( vRes, Vec_IntSize(vVars), Vec_IntArray(vVars), iVarNext, NULL );
    Vec_StrPush( vGuide, '0'+c++ );
    Vec_IntForEachEntry( vVars, Obj, i )
        Vec_StrPush( vGuide, 'a'+Obj );
    Vec_StrPush( vGuide, '\0' );
    
    printf( "Generated %d-LUT cascade for a %d-var function with %d rails and %d shared vars (node = %d, level = %d).\n", 
        nLutSize, nVars, nRails, nShared, (int)Vec_WrdEntry(vRes, 0), c );
    printf( "Structural info: %s\n", Vec_StrArray(vGuide) );
    Vec_StrFree( vGuide );

    word * pRes = Vec_WrdReleaseArray(vRes);
    Vec_WrdFree( vRes );
    return pRes;
}
word * Abc_LutCascadeTruth( word * pLuts, int nVars )
{
    int nWords = Abc_TtWordNum(nVars);
    Vec_Wrd_t * vFuncs = Vec_WrdStartTruthTables6( nVars );
    Vec_WrdFillExtra( vFuncs, nWords*(nVars+pLuts[0]+1), (word)0 );
    word * pCube = Vec_WrdEntryP( vFuncs, nWords*(nVars+pLuts[0]) );
    int n, i, m, v, iLastLut = -1;
    for ( n = 0, i = 1; n < pLuts[0]; n++, i += pLuts[i] ) 
    {
        word   nIns = pLuts[i+1];
        word * pIns = pLuts+i+2;
        word * pT   = pLuts+i+2+nIns+1;
        assert( pLuts[i] == 3+nIns+Abc_TtWordNum(nIns) );
        assert( pIns[nIns] < nVars+pLuts[0] );
        word * pIn[30], * pOut = Vec_WrdEntryP( vFuncs, nWords*pIns[nIns] );
        for ( v = 0; v < nIns; v++ )
            pIn[v] = Vec_WrdEntryP( vFuncs, nWords*pIns[v] );
        for ( m = 0; m < (1<<nIns); m++ ) {
            if ( !Abc_TtGetBit(pT, m) )
                continue;
            Abc_TtFill(pCube, nWords);
            for ( v = 0; v < nIns; v++ )
                Abc_TtAndCompl(pCube, pCube, 0, pIn[v], !((m>>v)&1), nWords);
            Abc_TtOr(pOut, pOut, pCube, nWords);
        }
        iLastLut = pIns[nIns];
    }      
    word * pRes = Vec_WrdReleaseArray(vFuncs);
    Abc_TtCopy( pRes, pRes + nWords*iLastLut, nWords, 0 );
    Vec_WrdFree( vFuncs );
    return pRes; 
}
int Abc_LutCascadeCount( word * pLuts )
{
    return (int)pLuts[0];
}
word * Abc_LutCascadeTest( Mini_Aig_t * p, int nLutSize, int fVerbose )
{
    word * pLuts = Abc_LutCascadeGenTest();
    Abc_LutCascadePrint( pLuts );
    return pLuts;
}


/**Function*************************************************************

  Synopsis    [LUT cascade decomposition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

// computes permutation masks for the current stage
int Abc_TtGetGuide( char * pGuide, int Iter, Vec_Int_t * vVarIDs, int fShared )
{
    int i, Res = 0, Count = 0;
    for ( i = 0; pGuide[i]; i++ )
        if ( pGuide[i] >= '0' && pGuide[i] <= '9' ) {
            if ( Count++ == Iter )
                break;
        }
    assert( i < strlen(pGuide) );
    assert( pGuide[i] == '0'+Iter );
    for ( i++; pGuide[i]; i++ ) 
    {
        char Char = pGuide[i];
        if ( Char >= '0' && Char <= '9' )
            break;
        if ( fShared && Char >= 'a' && Char <= 'z' )
            continue;
        int Value = -1;
        if ( Char >= 'a' && Char <= 'z' )
            Value = Char - 'a';
        else if ( Char >= 'A' && Char <= 'Z' )
            Value = Char - 'A';
        else assert( 0 );
        int iPlace = Vec_IntFind(vVarIDs, Value);
        assert( iPlace >= 0 );
        assert( ((Res >> iPlace) & 1) == 0 );
        Res |= 1 << iPlace;
    }
    return Res;
}

// moves variables in the mask to be the last ones in the order
void Abc_TtPermuteMask( word * p, int nVars, int Mask, Vec_Int_t * vPerm )
{
    assert( !vPerm || nVars == Vec_IntSize(vPerm) );
    int v, i, iLast = nVars-1, nWords = Abc_TtWordNum(nVars);
    int * pPerm = vPerm ? Vec_IntArray(vPerm) : NULL;
    if ( 0 && vPerm ) {
        printf( "Beg: " );
        for ( v = 0; v < nVars; v++ )
            printf( "%c ", 'a'+Vec_IntEntry(vPerm, v) );
        printf( "\n" );
        printf( "Bit:  " );
        for ( v = 0; v < nVars; v++ )
            printf( "%c ", (Mask >> v) & 1 ? '1' : '-' );
        printf( "\n" );
    }
    for ( v = nVars-1; v >= 0; v-- ) {
        if ( ((Mask >> v) & 1) == 0 )
            continue;
        if ( v == iLast ) {
            iLast--;
            continue;
        }
        assert( v < iLast );
        for ( i = v; i < iLast; i++ ) {
            Abc_TtSwapAdjacent( p, nWords, i );
            if ( pPerm ) ABC_SWAP( int, pPerm[i], pPerm[i+1] )
        }
        iLast--;
    }
    if ( 0 && vPerm ) {
        printf( "End:  " );
        for ( v = 0; v < nVars; v++ )
            printf( "%c ", 'a'+Vec_IntEntry(vPerm, v) );
        printf( "\n" );
    }
}


// checks if the given function exists in storage
// if the function does not exist, adds it to storage
// returns the number of the function in storage
int Abc_LutCascadeLookup( word * pStore, int nFuncs, word * pFunc, int nWords )
{
    int i;
    for ( i = 0; i < nFuncs; i++ )
        if ( Abc_TtEqual( pStore+i*nWords, pFunc, nWords ) )
            return i;
    Abc_TtCopy( pStore+i*nWords, pFunc, nWords, 0 );
    assert( i == nFuncs );
    return i;
}
void Abc_LutCascadeDerive( word * p, int nVars, int nBVars, int Myu, word * pRem, word * pDec, int nStep )
{
    int nFVars  = nVars-nBVars;  assert( nFVars >= 6 );
    int nEVars  = Abc_Base2Log(Myu);
    int nFWords = Abc_TtWordNum(nFVars);
    int m, e, iFunc, nFuncs = 0;
    //printf( "Decomposition pattern with %d BS vars and %d FS vars: ", nBVars, nFVars );
    for ( m = 0; m < (1 << nBVars); m++ ) {
        iFunc = Abc_LutCascadeLookup( pRem, nFuncs, p+m*nFWords, nFWords );
        //printf( "%x", iFunc );
        nFuncs = Abc_MaxInt( nFuncs, iFunc+1 );
        for ( e = 0; e < nEVars; e++ )
            if ( (iFunc >> e) & 1 )
                Abc_TtSetBit(pDec+e*nStep, m); 
    }
    //printf( "\n" );
    assert( nFuncs <= Myu );
    iFunc = nFuncs-1;
    for ( m = nFuncs; m < (1 << nEVars); m++ )
        Abc_TtCopy( pRem+m*nFWords, pRem+iFunc*nFWords, nFWords, 0 );
    if ( nBVars < 6 )
        for ( e = 0; e < nEVars; e++ )
            pDec[e*nStep] = Abc_Tt6Stretch( pDec[e*nStep], nBVars );
}

// performs decomposition of one stage
static inline int Abc_LutCascadeDecStage( char * pGuide, int Iter, Vec_Wrd_t * vFuncs[3], Vec_Int_t * vVarIDs, int nRVars, int nRails, int nLutSize, int fVerbose, Vec_Wrd_t * vCas )
{
    extern word Abc_TtFindBVarsSVars( word * p, int nVars, int nRVars, int nRails, int nLutSize, int fVerbose );
    assert( Vec_IntSize(vVarIDs) > nLutSize );
    assert( Vec_IntSize(vVarIDs) <= 24 );
    word Guide = pGuide ? 0 : Abc_TtFindBVarsSVars( Vec_WrdArray(vFuncs[0]), Vec_IntSize(vVarIDs), nRVars, nRails, nLutSize, fVerbose );
    if ( !pGuide && !Guide ) {
        if ( fVerbose )
            printf( "The function is not decomposable with %d rails.\n", nRails );
        Vec_IntClear( vVarIDs );
        return -1;
    }
    int m, Myu = pGuide ? 1 << nRails : (Guide >> 48) & 0xFF;
    int nEVars = Abc_Base2Log(Myu);
    int nVars  = Vec_IntSize(vVarIDs);
    int mBVars = pGuide ? Abc_TtGetGuide(pGuide, Iter, vVarIDs, 0) : Guide & 0xFFFFFF;
    int nBVars = __builtin_popcount(mBVars);
    assert( nBVars <= nLutSize );
    Abc_TtPermuteMask( Vec_WrdArray(vFuncs[0]), nVars, mBVars, vVarIDs );
    int mSVars = pGuide ? Abc_TtGetGuide(pGuide, Iter, vVarIDs, 1) : (Guide >> 24) & 0xFFFFFF;
    int nSVars = __builtin_popcount(mSVars);
    Abc_TtPermuteMask( Vec_WrdArray(vFuncs[0]), nVars, mSVars, vVarIDs );
    // prepare function (nVars -> nAVars+nVars)
    int nFVars = nVars - nBVars;
    int nAVars = nFVars >= 6 ? 0 : 6-nFVars;
    int nWordsNew = Abc_TtWordNum(nVars+nAVars);
    Vec_WrdFillExtra( vFuncs[0], nWordsNew, 0 );
    word * pFunc = Vec_WrdArray(vFuncs[0]);
    Abc_TtStretch6( pFunc, nVars, nVars+nAVars );
    Abc_TtPermuteMask( pFunc, nVars+nAVars, (1 << nVars)-1, NULL );
    // prepare remainder function (nAVars+nFVars+nEVars+nSVars)
    int nWordsRem = Abc_TtWordNum(nAVars+nFVars+nEVars+nSVars);
    Vec_WrdFill( vFuncs[1], nWordsRem, 0 );
    word * pRem = Vec_WrdArray(vFuncs[1]);
    // prepare decomposed functions (nBVars+nZVars) * nEVars
    int nUVars     = nBVars - nSVars;
    int nZVars     = nUVars >= 6 ? 0 : 6-nUVars;
    int nWordsDec  = Abc_TtWordNum(nBVars+nZVars);
    int nWordsStep = Abc_TtWordNum(nUVars+nZVars);
    Vec_WrdFill( vFuncs[2], nWordsDec*nEVars, 0 );
    word * pDec = Vec_WrdArray(vFuncs[2]);
    int nSMints = 1 << nSVars;
    for ( m = 0; m < nSMints; m++ ) 
        Abc_LutCascadeDerive(pFunc+m*nWordsNew/nSMints, nVars+nAVars-nSVars, nBVars-nSVars, Myu, 
            pRem+m*nWordsRem/nSMints, pDec+m*nWordsStep, nWordsDec );
    Abc_TtPermuteMask( pRem, nAVars+nFVars+nEVars+nSVars, (1 << nAVars)-1, NULL );
    Abc_TtPermuteMask( pRem, nFVars+nEVars+nSVars, ((1 << nEVars)-1) << nFVars, NULL );
    for ( m = 0; m < nEVars; m++ )
        Abc_TtPermuteMask( pDec+m*nWordsDec, nUVars+nZVars+nSVars, ((1 << nZVars)-1) << nUVars, NULL );
    for ( m = 0; m < nEVars; m++ )
        Abc_LutCascadeGenOne( vCas, nBVars, Vec_IntArray(vVarIDs)+nVars-nBVars, Vec_WrdEntry(vCas,0), pDec+m*nWordsDec );
    Abc_TtCopy( pFunc, pRem, Abc_TtWordNum(nFVars+nSVars+nEVars), 0 );
    assert( nEVars < nUVars );
    for ( m = 0; m < nSVars; m++ )
        Vec_IntWriteEntry(vVarIDs, nFVars+m, Vec_IntEntry(vVarIDs, nVars-nSVars+m) );
    for ( m = 0; m < nEVars; m++ )
        Vec_IntWriteEntry(vVarIDs, nFVars+nSVars+m, Vec_WrdEntry(vCas,0)-nEVars+m );
    Vec_IntShrink( vVarIDs, nFVars+nSVars+nEVars );
    return nEVars;
}
word * Abc_LutCascadeDec( char * pGuide, word * pTruth, int nVarsOrig, Vec_Int_t * vVarIDs, int nRails, int nLutSize, int fVerbose )
{
    word * pRes = NULL; int i, nRVars = 0, nVars = Vec_IntSize(vVarIDs);
    Vec_Wrd_t * vFuncs[3] = { Vec_WrdStart(Abc_TtWordNum(nVars)), Vec_WrdAlloc(0), Vec_WrdAlloc(0) };
    Abc_TtCopy( Vec_WrdArray(vFuncs[0]), pTruth, Abc_TtWordNum(nVars), 0 );
    Vec_Wrd_t * vCas = Vec_WrdAlloc( 100 ); Vec_WrdPush( vCas, nVarsOrig );
    for ( i = 0; Vec_IntSize(vVarIDs) > nLutSize; i++ ) {
        nRVars = Abc_LutCascadeDecStage( pGuide, i, vFuncs, vVarIDs, nRVars, nRails, nLutSize, fVerbose, vCas );
        if ( nRVars == -1 )
            break;
    }
    if ( nRVars != -1 && Vec_IntSize(vVarIDs) > 0 ) {
        Abc_LutCascadeGenOne( vCas, Vec_IntSize(vVarIDs), Vec_IntArray(vVarIDs), Vec_WrdEntry(vCas, 0), Vec_WrdArray(vFuncs[0]) );
        Vec_WrdAddToEntry( vCas, 0, -nVarsOrig );
        pRes = Vec_WrdReleaseArray(vCas);
    }
    Vec_WrdFree( vCas );
    for ( i = 0; i < 3; i++ )
        Vec_WrdFree( vFuncs[i] );
    return pRes;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkLutCascadeDeriveSop( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pNodeNew, word * pT, int nIns, Vec_Int_t * vCover )
{
    int RetValue = Kit_TruthIsop( (unsigned *)pT, nIns, vCover, 1 );
    assert( RetValue == 0 || RetValue == 1 );
    if ( Vec_IntSize(vCover) == 0 || (Vec_IntSize(vCover) == 1 && Vec_IntEntry(vCover,0) == 0) ) {
        assert( RetValue == 0 );
        pNodeNew->pData = Abc_SopCreateAnd( (Mem_Flex_t *)pNtkNew->pManFunc, nIns, NULL );
        return (Vec_IntSize(vCover) == 0) ? Abc_NtkCreateNodeConst0(pNtkNew) : Abc_NtkCreateNodeConst1(pNtkNew);
    }
    else {
        char * pSop = Abc_SopCreateFromIsop( (Mem_Flex_t *)pNtkNew->pManFunc, nIns, vCover );
        if ( RetValue ) Abc_SopComplement( (char *)pSop );
        pNodeNew->pData = pSop;
        return pNodeNew;
    }    
}
Abc_Ntk_t * Abc_NtkLutCascadeFromLuts( word * pLuts, int nVars, Abc_Ntk_t * pNtk, int nLutSize, int fVerbose )
{
    Abc_Ntk_t * pNtkNew = NULL; 
    Abc_Obj_t * pObj; int Id; char pName[2] = {0};
    Vec_Ptr_t * vCopy = Vec_PtrStart( nVars + pLuts[0] + 1000 );
    if ( pNtk ) {
        pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_LOGIC, ABC_FUNC_SOP );
    }
    else {
        pNtkNew = Abc_NtkAlloc( ABC_NTK_LOGIC, ABC_FUNC_SOP , 0 );
        pNtkNew->pName = Extra_UtilStrsav("cas");
        //pNtkNew->pSpec = Extra_UtilStrsav(pNtk->pSpec);        
        for ( Id = 0; Id < nVars; Id++ ) {
            pObj = Abc_NtkCreatePi(pNtkNew);
            pName[0] = 'a' + Id;
            Abc_ObjAssignName( pObj, pName, NULL );
        }
        pObj = Abc_NtkCreatePo(pNtkNew);
        Abc_ObjAssignName( pObj, "Out", NULL );
    }
    Abc_NtkForEachCi( pNtkNew, pObj, Id )
        Vec_PtrWriteEntry( vCopy, Id, pObj );
    Vec_Int_t * vCover = Vec_IntAlloc( 1000 );
    word n, i, k, iLastLut = -1;
    for ( n = 0, i = 1; n < pLuts[0]; n++, i += pLuts[i] ) 
    {
        word nIns   = pLuts[i+1];
        word * pIns = pLuts+i+2;
        word * pT   = pLuts+i+2+nIns+1;
        Abc_Obj_t * pNodeNew = Abc_NtkCreateNode( pNtkNew );
        for ( k = 0; k < nIns; k++ )
            Abc_ObjAddFanin( pNodeNew, (Abc_Obj_t *)Vec_PtrEntry(vCopy, pIns[k]) );
        Abc_Obj_t * pObjNew = Abc_NtkLutCascadeDeriveSop( pNtkNew, pNodeNew, pT, nIns, vCover );
        Vec_PtrWriteEntry( vCopy, pIns[nIns], pObjNew );        
        iLastLut = pIns[nIns];
    }
    Vec_IntFree( vCover );
    assert( Abc_NtkCoNum(pNtkNew) == 1 );
    Abc_ObjAddFanin( Abc_NtkCo(pNtkNew, 0), (Abc_Obj_t *)Vec_PtrEntry(vCopy, iLastLut) );
    Vec_PtrFree( vCopy ); 
    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        printf( "Abc_NtkLutCascadeFromLuts: The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;    
}
Abc_Ntk_t * Abc_NtkLutCascade( Abc_Ntk_t * pNtk, int nLutSize, int nLuts, int nRails, int nIters, int fVerbose )
{
    extern Gia_Man_t *  Abc_NtkStrashToGia( Abc_Ntk_t * pNtk );
    extern Mini_Aig_t * Gia_ManToMiniAig( Gia_Man_t * pGia );
    extern word *       Abc_LutCascade( Mini_Aig_t * p, int nLutSize, int nLuts, int nRails, int nIters, int fVerbose );    
    Gia_Man_t * pGia  = Abc_NtkStrashToGia( pNtk );
    Mini_Aig_t * pM   = Gia_ManToMiniAig( pGia );
    //word * pLuts      = Abc_LutCascade( pM, nLutSize, nLuts, nRails, nIters, fVerbose );
    word * pLuts      = Abc_LutCascadeTest( pM, nLutSize, 0 );
    Abc_Ntk_t * pNew  = pLuts ? Abc_NtkLutCascadeFromLuts( pLuts, Abc_NtkCiNum(pNtk), pNtk, nLutSize, fVerbose ) : NULL;
    ABC_FREE( pLuts );
    Mini_AigStop( pM );
    Gia_ManStop( pGia );
    return pNew;
}
Abc_Ntk_t * Abc_NtkLutCascade2( Abc_Ntk_t * pNtk, int nLutSize, int nLuts, int nRails, int nIters, int fVerbose, char * pGuide )
{
    extern Gia_Man_t *  Abc_NtkStrashToGia( Abc_Ntk_t * pNtk );
    extern word *       Abc_LutCascade2( word * p, int nVars, int nLutSize, int nLuts, int nRails, int nIters, int fVerbose );
    int nWords        = Abc_TtWordNum(Abc_NtkCiNum(pNtk));
    word * pCopy      = ABC_ALLOC( word, nWords );
    Gia_Man_t * pGia  = Abc_NtkStrashToGia( pNtk );
    Abc_Ntk_t * pNew  = NULL;
    Abc_Random(1);
    for ( int Iter = 0; Iter < nIters; Iter++ ) {
        word * pTruth1    = Gia_ObjComputeTruthTable( pGia, Gia_ManCo(pGia, 0) );
        Abc_TtCopy( pCopy, pTruth1, nWords, 0 );

        int nVars = -1;
        Vec_Int_t * vVarIDs = Vec_IntStartNatural( Abc_NtkCiNum(pNtk) );
        Abc_TtMinimumBase( pTruth1, Vec_IntArray(vVarIDs), Abc_NtkCiNum(pNtk), &nVars );
        Vec_IntShrink( vVarIDs, nVars );
        if ( fVerbose ) {
            if ( Abc_NtkCiNum(pNtk) != nVars )
                printf( "The support of the function is reduced from %d to %d variables.\n", Abc_NtkCiNum(pNtk), nVars );
            printf( "Decomposing %d-var function into %d-rail cascade of %d-LUTs", nVars, nRails, nLutSize );
            if ( pGuide )
                printf( " using structural info: %s", pGuide );
            printf( ".\n" );
        }

        word * pLuts = Abc_LutCascadeDec( pGuide, pTruth1, Abc_NtkCiNum(pNtk), vVarIDs, nRails, nLutSize, fVerbose );
        pNew = pLuts ? Abc_NtkLutCascadeFromLuts( pLuts, Abc_NtkCiNum(pNtk), pNtk, nLutSize, fVerbose ) : NULL;
        Vec_IntFree( vVarIDs );
        
        if ( pLuts ) {
            if ( fVerbose )
                Abc_LutCascadePrint( pLuts );
            word * pTruth2 = Abc_LutCascadeTruth( pLuts, Abc_NtkCiNum(pNtk) );
            if ( !Abc_TtEqual(pCopy, pTruth2, nWords) ) {
                printf( "Verification FAILED.\n" );
                printf("Function before: "); Abc_TtPrintHexRev( stdout, pCopy,   Abc_NtkCiNum(pNtk) ); printf( "\n" );
                printf("Function after:  "); Abc_TtPrintHexRev( stdout, pTruth2, Abc_NtkCiNum(pNtk) ); printf( "\n" );
            }
            else if ( fVerbose )
                printf( "Verification passed.\n" );
            ABC_FREE( pLuts );
            ABC_FREE( pTruth2 );
            break;
        }
        //ABC_FREE( pTruth1 );
    }
    ABC_FREE( pCopy );
    Gia_ManStop( pGia );
    return pNew;
}
Abc_Ntk_t * Abc_NtkLutCascadeGen( int nLutSize, int nStages, int nRails, int nShared, int fVerbose )
{
    int nVars = nStages * nLutSize - (nStages-1) * (nRails + nShared);
    word * pLuts = Abc_LutCascadeGen( nVars, nLutSize, nRails, nShared );
    Abc_Ntk_t * pNew = Abc_NtkLutCascadeFromLuts( pLuts, nVars, NULL, nLutSize, fVerbose );
    Abc_LutCascadePrint( pLuts );
    if ( fVerbose ) {
        word * pTruth = Abc_LutCascadeTruth( pLuts, nVars );
        if ( nVars <= 10 ) {
            printf( "Function: "); Abc_TtPrintHexRev( stdout, pTruth, nVars ); printf( "\n" );
        }
        ABC_FREE( pTruth );
    }
    ABC_FREE( pLuts );
    return pNew;
}


/**Function*************************************************************

  Synopsis    [Structural LUT cascade mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

typedef struct Abc_LutCas_t_  Abc_LutCas_t;
struct Abc_LutCas_t_
{
    // mapped network
    Abc_Ntk_t *        pNtk;
    // parameters
    int                nLutSize;
    int                nLutsMax;
    int                nIters;
    int                fDelayLut;
    int                fDelayRoute;
    int                fDelayDirect;
    int                fVerbose;
    // internal data
    int                DelayMax;
    Vec_Int_t *        vTime[2];   // timing info
    Vec_Int_t *        vCrits[2];  // critical terminals
    Vec_Int_t *        vPath[2];   // direct connections
    Vec_Wec_t *        vStack;     // processing queue
    Vec_Int_t *        vZeroSlack; // zero-slack nodes
    Vec_Int_t *        vCands;     // direct edge candidates
    Vec_Int_t *        vTrace;     // modification trace
    Vec_Int_t *        vTraceBest; // modification trace
};

Abc_LutCas_t * Abc_LutCasAlloc( Abc_Ntk_t * pNtk, int nLutsMax, int nIters, int fDelayLut, int fDelayRoute, int fDelayDirect, int fVerbose )
{
    Abc_LutCas_t * p = ABC_ALLOC( Abc_LutCas_t, 1 );
    p->pNtk         = pNtk;
    p->nLutSize     = 6;
    p->nLutsMax     = nLutsMax;
    p->nIters       = nIters;
    p->fDelayLut    = fDelayLut;
    p->fDelayRoute  = fDelayRoute;
    p->fDelayDirect = fDelayDirect;
    p->fVerbose     = fVerbose;
    p->vTime[0]   = Vec_IntStart( Abc_NtkObjNumMax(pNtk) );
    p->vTime[1]   = Vec_IntStart( Abc_NtkObjNumMax(pNtk) );
    p->vCrits[0]  = Vec_IntAlloc(1000);
    p->vCrits[1]  = Vec_IntAlloc(1000);
    p->vPath[0]   = Vec_IntStart( Abc_NtkObjNumMax(pNtk) );
    p->vPath[1]   = Vec_IntStart( Abc_NtkObjNumMax(pNtk) );
    p->vStack     = Vec_WecStart( Abc_NtkLevel(pNtk) + 1 );
    p->vZeroSlack = Vec_IntAlloc( 1000 );
    p->vCands     = Vec_IntAlloc( 1000 );
    p->vTrace     = Vec_IntAlloc( 1000 );
    p->vTraceBest = Vec_IntAlloc( 1000 );
    return p;
}
void Abc_LutCasFree( Abc_LutCas_t * p )
{
    Vec_IntFree( p->vTime[0]   );
    Vec_IntFree( p->vTime[1]   );
    Vec_IntFree( p->vCrits[0]  );
    Vec_IntFree( p->vCrits[1]  );
    Vec_IntFree( p->vPath[0]   );
    Vec_IntFree( p->vPath[1]   );
    Vec_WecFree( p->vStack     );
    Vec_IntFree( p->vZeroSlack );
    Vec_IntFree( p->vCands     );
    Vec_IntFree( p->vTrace     );
    Vec_IntFree( p->vTraceBest );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Structural LUT cascade mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkFindPathTimeD_rec( Abc_LutCas_t * p, Abc_Obj_t * pObj )
{
    if ( !Abc_ObjIsNode(pObj) || !Abc_ObjFaninNum(pObj) )
        return 0;
    if ( Vec_IntEntry(p->vTime[0], pObj->Id) > 0 )
        return Vec_IntEntry(p->vTime[0], pObj->Id);    
    Abc_Obj_t * pNext; int i, Delay, DelayMax = 0;
    Abc_ObjForEachFanin( pObj, pNext, i ) {
        Delay  = Abc_NtkFindPathTimeD_rec( p, pNext );
        Delay += Vec_IntEntry(p->vPath[0], pObj->Id) == pNext->Id ? p->fDelayDirect : p->fDelayRoute;
        DelayMax = Abc_MaxInt( DelayMax, Delay + p->fDelayLut );
    }
    Vec_IntWriteEntry( p->vTime[0], pObj->Id, DelayMax );
    return DelayMax;
}
int Abc_NtkFindPathTimeD( Abc_LutCas_t * p )
{
    Abc_Obj_t * pObj; int i, Delay, DelayMax = 0;
    Vec_IntFill( p->vTime[0], Abc_NtkObjNumMax(p->pNtk), 0 );
    Abc_NtkForEachCo( p->pNtk, pObj, i ) {
        Delay = Abc_NtkFindPathTimeD_rec( p, Abc_ObjFanin0(pObj) );
        DelayMax = Abc_MaxInt( DelayMax, Delay + p->fDelayRoute );
    }
    Vec_IntClear( p->vCrits[0] );
    Abc_NtkForEachCo( p->pNtk, pObj, i )
        if ( DelayMax == Vec_IntEntry(p->vTime[0], Abc_ObjFaninId0(pObj)) + p->fDelayRoute )
            Vec_IntPush( p->vCrits[0], pObj->Id );
    return DelayMax;
}
int Abc_NtkFindPathTimeR_rec( Abc_LutCas_t * p, Abc_Obj_t * pObj )
{
    if ( Abc_ObjIsCo(pObj) )
        return 0;
    if ( Vec_IntEntry(p->vTime[1], pObj->Id) > 0 )
        return Vec_IntEntry(p->vTime[1], pObj->Id) + (Abc_ObjIsNode(pObj) ? p->fDelayLut : 0);
    Abc_Obj_t * pNext; int i; float Delay, DelayMax = 0;
    Abc_ObjForEachFanout( pObj, pNext, i ) {
        Delay  = Abc_NtkFindPathTimeR_rec( p, pNext );
        Delay += Vec_IntEntry(p->vPath[0], pNext->Id) == pObj->Id ? p->fDelayDirect : p->fDelayRoute;
        DelayMax = Abc_MaxInt( DelayMax, Delay );
    }
    Vec_IntWriteEntry( p->vTime[1], pObj->Id, DelayMax );
    return DelayMax + (Abc_ObjIsNode(pObj) ? p->fDelayLut : 0);
}
int Abc_NtkFindPathTimeR( Abc_LutCas_t * p )
{
    Abc_Obj_t * pObj; int i; int Delay, DelayMax = 0;
    Vec_IntFill( p->vTime[1], Abc_NtkObjNumMax(p->pNtk), 0 );
    Abc_NtkForEachCi( p->pNtk, pObj, i ) {        
        Delay = Abc_NtkFindPathTimeR_rec( p, pObj );
        DelayMax = Abc_MaxInt( DelayMax, Delay );
    }
    Vec_IntClear( p->vCrits[1] );
    Abc_NtkForEachCi( p->pNtk, pObj, i )
        if ( DelayMax == Vec_IntEntry(p->vTime[1], pObj->Id) )
            Vec_IntPush( p->vCrits[1], pObj->Id );
    return DelayMax;
}
void Abc_NtkFindCriticalEdges( Abc_LutCas_t * p )
{
    Abc_Obj_t * pObj, * pFanin; int i, k;
    Vec_IntClear( p->vCands );
    Abc_NtkForEachNode( p->pNtk, pObj, i ) {
        if ( Vec_IntEntry(p->vPath[0], pObj->Id) )
            continue;
        if ( Vec_IntEntry(p->vTime[0], pObj->Id) + Vec_IntEntry(p->vTime[1], pObj->Id) < p->DelayMax )
            continue;
        assert( Vec_IntEntry(p->vTime[0], pObj->Id) + Vec_IntEntry(p->vTime[1], pObj->Id == p->DelayMax) );
        Abc_ObjForEachFanin( pObj, pFanin, k )
            if ( Abc_ObjIsNode(pFanin) && !Vec_IntEntry(p->vPath[1], pFanin->Id) && 
                 Vec_IntEntry(p->vTime[0], pFanin->Id) + p->fDelayRoute + p->fDelayLut == Vec_IntEntry(p->vTime[0], pObj->Id) )
                Vec_IntPushTwo( p->vCands, pObj->Id, pFanin->Id );
    }    
}
int Abc_NtkFindTiming( Abc_LutCas_t * p )
{
    int Delay0 = Abc_NtkFindPathTimeD( p );
    int Delay1 = Abc_NtkFindPathTimeR( p );
    assert( Delay0 == Delay1 );
    p->DelayMax = Delay0;
    Abc_NtkFindCriticalEdges( p );
    return Delay0;
}

int Abc_NtkUpdateNodeD( Abc_LutCas_t * p, Abc_Obj_t * pObj )
{
    Abc_Obj_t * pNext; int i; int Delay, DelayMax = 0;
    Abc_ObjForEachFanin( pObj, pNext, i ) {
        Delay  = Vec_IntEntry( p->vTime[0], pNext->Id );
        Delay += Vec_IntEntry(p->vPath[0], pObj->Id) == pNext->Id ? p->fDelayDirect : p->fDelayRoute;
        DelayMax = Abc_MaxInt( DelayMax, Delay + p->fDelayLut );
    }
    int DelayOld = Vec_IntEntry( p->vTime[0], pObj->Id );
    Vec_IntWriteEntry( p->vTime[0], pObj->Id, DelayMax );
    assert( DelayOld >= DelayMax );
    return DelayOld > DelayMax;
}
int Abc_NtkUpdateNodeR( Abc_LutCas_t * p, Abc_Obj_t * pObj )
{
    Abc_Obj_t * pNext; int i; float Delay, DelayMax = 0;
    Abc_ObjForEachFanout( pObj, pNext, i ) {
        Delay  = Vec_IntEntry(p->vTime[1], pNext->Id) + (Abc_ObjIsNode(pNext) ? p->fDelayLut : 0);
        Delay += Vec_IntEntry(p->vPath[0], pNext->Id) == pObj->Id ? p->fDelayDirect : p->fDelayRoute;
        DelayMax = Abc_MaxInt( DelayMax, Delay );
    }
    int DelayOld = Vec_IntEntry( p->vTime[1], pObj->Id );
    Vec_IntWriteEntry( p->vTime[1], pObj->Id, DelayMax );
    assert( DelayOld >= DelayMax );
    return DelayOld > DelayMax;
}
int Abc_NtkUpdateTiming( Abc_LutCas_t * p, int Node, int Fanin )
{
    Abc_Obj_t * pNode  = Abc_NtkObj( p->pNtk, Node );
    Abc_Obj_t * pFanin = Abc_NtkObj( p->pNtk, Fanin );
    Abc_Obj_t * pNext, * pTemp; Vec_Int_t * vLevel; int i, k, j;
    assert( Abc_ObjIsNode(pNode) && Abc_ObjIsNode(pFanin) );
    Vec_WecForEachLevel( p->vStack, vLevel, i )
        Vec_IntClear( vLevel );
    Abc_NtkIncrementTravId( p->pNtk );
    Abc_NodeSetTravIdCurrentId( p->pNtk, Node );
    Abc_NodeSetTravIdCurrentId( p->pNtk, Fanin );
    Vec_WecPush( p->vStack, pNode->Level, Node );
    Vec_WecPush( p->vStack, pFanin->Level, Fanin );
    Vec_WecForEachLevelStart( p->vStack, vLevel, i, pNode->Level )
        Abc_NtkForEachObjVec( vLevel, p->pNtk, pTemp, k ) {
            if ( !Abc_NtkUpdateNodeD(p, pTemp) )
                continue;
            Abc_ObjForEachFanout( pTemp, pNext, j ) {
                if ( Abc_NodeIsTravIdCurrent(pNext) || Abc_ObjIsCo(pNext) ) 
                    continue;
                Abc_NodeSetTravIdCurrent( pNext );
                Vec_WecPush( p->vStack, pNext->Level, pNext->Id );                    
            }
        }
    Vec_WecForEachLevelReverseStartStop( p->vStack, vLevel, i, pFanin->Level+1, 0 )
        Abc_NtkForEachObjVec( vLevel, p->pNtk, pTemp, k ) {
            if ( !Abc_NtkUpdateNodeR(p, pTemp) )
                continue;
            Abc_ObjForEachFanin( pTemp, pNext, j ) {
                if ( Abc_NodeIsTravIdCurrent(pNext) ) 
                    continue;
                Abc_NodeSetTravIdCurrent( pNext );
                Vec_WecPush( p->vStack, pNext->Level, pNext->Id );                    
            }
        }
    j = 0;
    Abc_NtkForEachObjVec( p->vCrits[0], p->pNtk, pTemp, i )
        if ( Vec_IntEntry(p->vTime[0], Abc_ObjFaninId0(pTemp)) + p->fDelayRoute == p->DelayMax )
            Vec_IntWriteEntry( p->vCrits[0], j++, pTemp->Id );
    Vec_IntShrink( p->vCrits[0], j );
    j = 0;
    Abc_NtkForEachObjVec( p->vCrits[1], p->pNtk, pTemp, i )
        if ( Vec_IntEntry(p->vTime[1], pTemp->Id) == p->DelayMax )
            Vec_IntWriteEntry( p->vCrits[1], j++, pTemp->Id );
    Vec_IntShrink( p->vCrits[1], j );
    if ( Vec_IntSize(p->vCrits[0]) && Vec_IntSize(p->vCrits[1]) ) {
        int j = 0, Node2, Fanin2;
        Vec_IntForEachEntryDouble( p->vCands, Node2, Fanin2, i )
            if ( !Vec_IntEntry(p->vPath[0], Node2) && !Vec_IntEntry(p->vPath[1], Fanin2) && 
                 Vec_IntEntry(p->vTime[0], Node2) + Vec_IntEntry(p->vTime[1], Node2) == p->DelayMax && 
                 Vec_IntEntry(p->vTime[0], Fanin2) + p->fDelayRoute + p->fDelayLut == Vec_IntEntry(p->vTime[0], Node2) )
                    Vec_IntWriteEntry( p->vCands, j++, Node2 ), Vec_IntWriteEntry( p->vCands, j++, Fanin2 );
        Vec_IntShrink( p->vCands, j );
        return p->DelayMax;
    }
    int DelayOld = p->DelayMax;
    Abc_NtkFindTiming(p);
    assert( DelayOld > p->DelayMax );
    return p->DelayMax;
}

/**Function*************************************************************

  Synopsis    [Structural LUT cascade mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkAddEdges( Abc_LutCas_t * p )
{
    int nEdgesMax = 10000;
    Vec_IntClear( p->vTrace );
    Vec_IntFill( p->vPath[0], Vec_IntSize(p->vPath[0]), 0 );
    Vec_IntFill( p->vPath[1], Vec_IntSize(p->vPath[1]), 0 );
    Abc_NtkFindTiming( p );
    if ( p->fVerbose )
        printf( "Start : %d\n", p->DelayMax );
    int i, LastChange = 0;
    for ( i = 0; i < nEdgesMax; i++ ) 
    {
        float DelayPrev = p->DelayMax;
        if ( Vec_IntSize(p->vCands) == 0 )
            break;
        int Index = rand() % Vec_IntSize(p->vCands)/2;
        int Node  = Vec_IntEntry( p->vCands, 2*Index );
        int Fanin = Vec_IntEntry( p->vCands, 2*Index+1 );
        assert( Vec_IntEntry( p->vPath[0], Node ) == 0 );
        Vec_IntWriteEntry( p->vPath[0], Node, Fanin );
        assert( Vec_IntEntry( p->vPath[1], Fanin ) == 0 );
        Vec_IntWriteEntry( p->vPath[1], Fanin, Node );
        Vec_IntPushTwo( p->vTrace, Node, Fanin );
        //Abc_NtkFindTiming( p );
        Abc_NtkUpdateTiming( p, Node, Fanin );
        assert( DelayPrev >= p->DelayMax );
        if ( DelayPrev > p->DelayMax )
            LastChange = i+1;
        DelayPrev = p->DelayMax;
        if ( p->fVerbose )
            printf( "%5d : %d : %4d -> %4d\n", i, p->DelayMax, Fanin, Node );
    }
    Vec_IntShrink( p->vTrace, 2*LastChange );
    return p->DelayMax;
}
Vec_Wec_t * Abc_NtkProfileCascades( Abc_Ntk_t * pNtk, Vec_Int_t * vTrace )
{
    Vec_Wec_t * vCasc = Vec_WecAlloc( 100 );
    Vec_Int_t * vMap  = Vec_IntStart( Abc_NtkObjNumMax(pNtk) );
    Vec_Int_t * vPath = Vec_IntStart( Abc_NtkObjNumMax(pNtk) );
    Vec_Int_t * vCounts = Vec_IntStart( Abc_NtkObjNumMax(pNtk) );
    Abc_Obj_t * pObj; int i, Node, Fanin, Count, nCascs = 0;
    Vec_IntForEachEntryDouble( vTrace, Node, Fanin, i ) {
        assert( Vec_IntEntry(vPath, Node) == 0 );
        Vec_IntWriteEntry( vPath, Node, Fanin );
        Vec_IntWriteEntry( vMap, Fanin, 1 );
    }
    Abc_NtkForEachNode( pNtk, pObj, i ) {
        if ( Vec_IntEntry(vMap, pObj->Id) )
            continue;
        if ( Vec_IntEntry(vPath, pObj->Id) == 0 )
            continue;
        Vec_Int_t * vLevel = Vec_WecPushLevel( vCasc );
        Node = pObj->Id;
        Vec_IntPush( vLevel, Node );
        while ( (Node = Vec_IntEntry(vPath, Node)) )
            Vec_IntPush( vLevel, Node );
        Vec_IntAddToEntry( vCounts, Vec_IntSize(vLevel), 1 );
    }
    printf( "Cascades: " );
    Vec_IntForEachEntry( vCounts, Count, i )
        if ( Count )
            printf( "%d=%d ", i, Count ), nCascs += Count;
    printf( "\n" );
    Vec_IntFree( vMap );
    Vec_IntFree( vPath );
    Vec_IntFree( vCounts );
    return vCasc;
}
void Abc_LutCasAssignNames( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkNew, Vec_Wec_t * vCascs )
{
    Abc_Obj_t * pObj; Vec_Int_t * vLevel; int i, k; char pName[100];
    Vec_Int_t * vMap = Vec_IntStart( Abc_NtkObjNumMax(pNtk) );
    Abc_NtkForEachCo( pNtkNew, pObj, i )
        Vec_IntWriteEntry( vMap, Abc_ObjFaninId0(pObj), pObj->Id );
    Vec_WecForEachLevel( vCascs, vLevel, i ) {
        Abc_NtkForEachObjVec( vLevel, pNtk, pObj, k ) {
            assert( Abc_ObjIsNode(pObj) );
            sprintf( pName, "c%d_n%d", i, k );
            if ( Vec_IntEntry(vMap, pObj->pCopy->Id) == 0 )
                Abc_ObjAssignName( Abc_NtkObj(pNtkNew, pObj->pCopy->Id), pName, NULL );
            else {
                Nm_ManDeleteIdName( pNtkNew->pManName, Vec_IntEntry(vMap, pObj->pCopy->Id) );
                Abc_ObjAssignName( Abc_NtkObj(pNtkNew, Vec_IntEntry(vMap, pObj->pCopy->Id)), pName, NULL );
            }
        }
    }
    Vec_IntFree( vMap );
}
void Abc_NtkLutCascadeDumpResults( char * pDumpFile, char * pTest, int Nodes, int Edges, int Levs, int DelStart, int DelStop, float DelRatio, int EdgesUsed, float EdgeRatio, int Cascs, float AveLength, abctime time )
{
    FILE * pTable = fopen( pDumpFile, "a+" );
    fprintf( pTable, "%s,", pTest+28 );
    fprintf( pTable, "%d,", Nodes );
    fprintf( pTable, "%d,", Edges );
    fprintf( pTable, "%d,", Levs );
    fprintf( pTable, "%d,", DelStart );
    fprintf( pTable, "%d,", DelStop );
    fprintf( pTable, "%.2f,", DelRatio );
    fprintf( pTable, "%d,", EdgesUsed );
    fprintf( pTable, "%.2f,", EdgeRatio );
    fprintf( pTable, "%d,",   Cascs );
    fprintf( pTable, "%.1f,", AveLength );
    fprintf( pTable, "%.2f,", 1.0*((double)(time))/((double)CLOCKS_PER_SEC) );
    fprintf( pTable, "\n" );
    fclose( pTable );
}

Abc_Ntk_t * Abc_NtkLutCascadeMap( Abc_Ntk_t * pNtk, int nLutsMax, int nIters, int fDelayLut, int fDelayRoute, int fDelayDirect, int fVerbose )
{
    abctime clk = Abc_Clock();
    Abc_Ntk_t * pNtkNew = NULL;
    Abc_LutCas_t * p = Abc_LutCasAlloc( pNtk, nLutsMax, nIters, fDelayLut, fDelayRoute, fDelayDirect, fVerbose );
    int i, IterBest = 0, DelayStart = Abc_NtkFindTiming( p ),  DelayBest = DelayStart, nEdges = Abc_NtkGetTotalFanins(pNtk);
    //printf( "Delays: LUT (%d) Route (%d) Direct (%d).  Iters = %d.  LUTs = %d.\n", fDelayLut, fDelayRoute, fDelayDirect, nIters, Abc_NtkNodeNum(pNtk) );
    Vec_IntFill( p->vTraceBest, Abc_NtkNodeNum(pNtk), 0 );
    for ( i = 0; i < nIters; i++ )
    {
        if ( fVerbose )
            printf( "ITERATION %2d:\n", i );
        float Delay = Abc_NtkAddEdges( p );
        if ( DelayBest < Delay || (DelayBest == Delay && Vec_IntSize(p->vTraceBest) <= Vec_IntSize(p->vTrace)) )
            continue;
        IterBest = i;
        DelayBest = Delay;
        ABC_SWAP( Vec_Int_t *, p->vTrace, p->vTraceBest );
    }
    printf( "Delay reduction %d -> %d (-%.2f %%) is found after iter %d with %d edges (%.2f %%). ", 
        DelayStart, DelayBest, 100.0*(DelayStart - DelayBest)/DelayStart, IterBest, Vec_IntSize(p->vTraceBest)/2, 50.0*Vec_IntSize(p->vTraceBest)/nEdges );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    Vec_Wec_t * vCascs = Abc_NtkProfileCascades( p->pNtk, p->vTraceBest );
//    Abc_NtkLutCascadeDumpResults( "stats.csv", pNtk->pName, Abc_NtkNodeNum(pNtk), nEdges, Abc_NtkLevel(pNtk), DelayStart, DelayBest, 100.0*(DelayStart - DelayBest)/DelayStart,
//        Vec_IntSize(p->vTraceBest)/2, 50.0*Vec_IntSize(p->vTraceBest)/nEdges, Vec_WecSize(vCascs), 0.5*Vec_IntSize(p->vTraceBest)/Abc_MaxInt(1, Vec_WecSize(vCascs)), Abc_Clock() - clk );
    Abc_LutCasFree( p );
    pNtkNew = Abc_NtkDup( pNtk );
    Abc_LutCasAssignNames( pNtk, pNtkNew, vCascs );
    Vec_WecFree( vCascs );
    return pNtkNew;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Wrd_t * Abc_NtkLutCasReadTruths( char * pFileName, int nVarsOrig )
{
    Vec_Wrd_t * vTruths = NULL;
    int nWords = Abc_TtWordNum(nVarsOrig);
    int nFileSize = Gia_FileSize( pFileName );
    FILE * pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL ) { printf("Cannot open file \"%s\" for reading.\n", pFileName); return NULL; }
    word * pTruth = ABC_ALLOC( word, 2*nWords );
    char * pToken, * pLine = ABC_ALLOC( char, nFileSize );
    for ( int i = 0; fgets(pLine, nFileSize, pFile); i++ ) {
         pToken = strtok(pLine, " ,\n\r\r");
        if ( pToken == NULL )
            continue;
        if ( pToken[0] == '0' && pToken[1] == 'x' )
            pToken += 2;
       if ( strlen(pToken) != (1 << (nVarsOrig-2)) ) {
            printf( "Line %d has truth table of size %d while expected size is %d.\n", i, (int)strlen(pToken), 1 << (nVarsOrig-2) );
            Vec_WrdFreeP( &vTruths );
            break;
        }
        if ( !Abc_TtReadHex( pTruth, pToken ) ) {
            printf( "Line %d has truth table that cannot be read correctly (%s).\n", i, pToken );
            Vec_WrdFreeP( &vTruths );
            break;
        }        
        if ( vTruths == NULL )
            vTruths = Vec_WrdAlloc( 10000 );
        for ( int w = 0; w < nWords; w++ )
            Vec_WrdPush( vTruths, pTruth[w] );
    }
    ABC_FREE( pTruth );
    ABC_FREE( pLine );
    fclose( pFile );
    return vTruths;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkLutCascadeFile( char * pFileName, int nVarsOrig, int nLutSize, int nLuts, int nRails, int nIters, int fVerbose, int fVeryVerbose )
{
    abctime clkStart = Abc_Clock();   
    int i, Sum = 0, nTotalLuts = 0, nWords = Abc_TtWordNum(nVarsOrig);
    Vec_Wrd_t * vTruths = NULL;
    if ( strstr(pFileName, ".txt") )
        vTruths = Abc_NtkLutCasReadTruths( pFileName, nVarsOrig );
    else
        vTruths = Vec_WrdReadBin( pFileName, 0 );
    if ( vTruths == NULL )
        return;

    int nFuncs = Vec_WrdSize(vTruths)/nWords;
    if ( Vec_WrdSize(vTruths) != nFuncs * nWords ) {
        printf( "The files size (%d bytes) does not match the truth table size (%d) for the given number of functions (%d).\n", 8*Vec_WrdSize(vTruths), 8*nWords, nFuncs );
        Vec_WrdFree( vTruths );
        return;
    }

    printf( "Considering %d functions having %d variables from file \"%s\".\n", nFuncs, nVarsOrig, pFileName );
    word * pCopy = ABC_ALLOC( word, nWords );
    int Iter = 0, LutStats[100] = {0};
    Abc_Random(1);
    for ( i = 0; i < nFuncs; i++ )
    {
        word * pTruth = Vec_WrdEntryP( vTruths, i*nWords );
        Abc_TtCopy( pCopy, pTruth, nWords, 0 );

        if ( fVeryVerbose )
            printf( "\n" );
        if ( fVerbose || fVeryVerbose )
            printf( "Function %3d : ", i );
        if ( fVeryVerbose )
            Abc_TtPrintHexRev( stdout, pTruth, nVarsOrig ), printf( "\n" );
        //continue;

        int nVars = -1;
        Vec_Int_t * vVarIDs = Vec_IntStartNatural( nVarsOrig );
        Abc_TtMinimumBase( pTruth, Vec_IntArray(vVarIDs), nVarsOrig, &nVars );
        Vec_IntShrink( vVarIDs, nVars );
        if ( fVeryVerbose ) {
            if ( nVarsOrig != nVars )
                printf( "The support of the function is reduced from %d to %d variables.\n", nVarsOrig, nVars );
            printf( "Decomposing %d-var function into %d-rail cascade of %d-LUTs.\n", nVars, nRails, nLutSize );
        }
        
        word * pLuts = Abc_LutCascadeDec( NULL, pTruth, nVarsOrig, vVarIDs, nRails, nLutSize, fVeryVerbose );
        Vec_IntFree( vVarIDs );
        if ( pLuts == NULL ) {
            if ( ++Iter < nIters ) {
                i--;
                continue;
            }
            Iter = 0;
            if ( fVerbose || fVeryVerbose )
                printf( "Not decomposable.\n" );
            continue;
        }
        Iter = 0;
        Sum++;
        nTotalLuts += Abc_LutCascadeCount(pLuts);
        LutStats[Abc_LutCascadeCount(pLuts)]++;
        word * pTruth2 = Abc_LutCascadeTruth( pLuts, nVarsOrig );
        if ( fVeryVerbose )
            Abc_LutCascadePrint( pLuts );
        if ( fVerbose || fVeryVerbose )
            printf( "Decomposition exists.  " );
        if ( !Abc_TtEqual(pCopy, pTruth2, nWords) ) {
            printf( "Verification FAILED for function %d.\n", i );
            printf( "Before: " ); Abc_TtPrintHexRev( stdout, pCopy,   nVarsOrig ), printf( "\n" );
            printf( "After:  " ); Abc_TtPrintHexRev( stdout, pTruth2, nVarsOrig ), printf( "\n" );
        }
        else if ( fVerbose || fVeryVerbose )
            printf( "Verification passed.\n" );
        ABC_FREE( pTruth2 );
        ABC_FREE( pLuts );
    }
    ABC_FREE( pCopy );
    Vec_WrdFree( vTruths );
    printf( "Statistics for %d-rail LUT cascade:\n", nRails );
    for ( i = 0; i < 100; i++ )
        if ( LutStats[i] )
            printf( "    %d LUT6 : Function count = %8d (%6.2f %%)\n", i, LutStats[i], 100.0*LutStats[i]/nFuncs );
    printf( "Non-decomp : Function count = %8d (%6.2f %%)\n", nFuncs-Sum, 100.0*(nFuncs-Sum)/Abc_MaxInt(1, nFuncs) );
    printf( "Finished %d functions (%.2f LUTs / function; %.2f functions / sec).  ", 
        nFuncs, 1.0*nTotalLuts/Sum, 1.0*nFuncs/(((double)(Abc_Clock() - clkStart))/((double)CLOCKS_PER_SEC)) );
    Abc_PrintTime( 0, "Total time", Abc_Clock() - clkStart );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Vec_WrdWriteTruthHex( char * pFileName, Vec_Wrd_t * vTruths, int nVars )
{
    FILE * pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL ) { printf("Cannot open file \"%s\" for reading.\n", pFileName); return; }
    int i, nWords = Abc_TtWordNum(nVars), nFuncs = Vec_WrdSize(vTruths)/nWords;
    assert( nFuncs * nWords == Vec_WrdSize(vTruths) );
    for ( i = 0; i < nFuncs; i++ )
        Abc_TtPrintHexRev( pFile, Vec_WrdEntryP(vTruths, i*nWords), nVars ), fprintf( pFile, "\n" );
    fclose( pFile );
}
void Abc_NtkSuppMinFile( char * pFileName )
{
    int fError = 0, nFileSize = Gia_FileSize( pFileName );
    FILE * pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL ) { printf("Cannot open file \"%s\" for reading.\n", pFileName); return; }
    Vec_Wrd_t ** pvTruths = ABC_CALLOC( Vec_Wrd_t *, 32 );
    char * pToken, * pLine = ABC_ALLOC( char, nFileSize );
    word * pTruth = ABC_ALLOC( word, nFileSize/16 );
    int Len, nVars = -1, nWords = -1, nFuncs = 0, nSuppSums[2] = {0};
    for ( int i = 0; fgets(pLine, nFileSize, pFile); i++ ) {
         pToken = strtok(pLine, " ,\n\r\r");
        if ( pToken == NULL )
            continue;
        if ( pToken[0] == '0' && pToken[1] == 'x' )
            pToken += 2;
        Len = strlen(pToken);
        nVars = Abc_Base2Log(Len);
        if ( Len != (1 << nVars) ) {
            printf( "The number of hex characters (%d) in the truth table listed in line %d is not a degree of 2.\n", Len, i );
            fError = 1;
            break;
        }
        nVars += 2;
        if ( !Abc_TtReadHex( pTruth, pToken ) ) {
            printf( "Line %d has truth table that cannot be read correctly (%s).\n", i, pToken );
            fError = 1;
            break;
        }
        nSuppSums[0] += nVars;
        Abc_TtMinimumBase( pTruth, NULL, nVars, &nVars );
        nSuppSums[1] += nVars;
        if ( pvTruths[nVars] == NULL )
            pvTruths[nVars] = Vec_WrdAlloc( 10000 );
        nWords = Abc_TtWordNum(nVars);
        for ( int w = 0; w < nWords; w++ )
            Vec_WrdPush( pvTruths[nVars], pTruth[w] );
        nFuncs++;
    }
    ABC_FREE( pTruth );
    ABC_FREE( pLine );
    fclose( pFile );
    if ( fError ) {
        for ( nVars = 0; nVars < 32; nVars++ )
            if ( pvTruths[nVars] )
                Vec_WrdFreeP( &pvTruths[nVars] );
        ABC_FREE( pvTruths );
        return;
    }
    // dump the resulting truth tables
    printf( "Read and support-minimized %d functions. Total support reduction %d -> %d (%.2f %%).\n", 
        nFuncs, nSuppSums[0], nSuppSums[1], 100.0*(nSuppSums[0]-nSuppSums[1])/Abc_MaxInt(nSuppSums[0], 1) );
    printf( "The resulting function statistics:\n" );
    for ( nVars = 0; nVars < 32; nVars++ ) {
        if ( pvTruths[nVars] == NULL )
            continue;
        char pFileName2[1000];
        sprintf( pFileName2, "%s_%02d.txt", Extra_FileNameGeneric(pFileName), nVars );
        Vec_WrdWriteTruthHex( pFileName2, pvTruths[nVars], nVars );
        printf( "Support size %2d : Dumped %6d truth tables into file \"%s\".\n", 
            nVars, Vec_WrdSize(pvTruths[nVars])/Abc_TtWordNum(nVars), pFileName2 );
        Vec_WrdFreeP( &pvTruths[nVars] );
    }
    ABC_FREE( pvTruths );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkRandFile( char * pFileName, int nVars, int nFuncs, int nMints )
{
    int i, k, nWords = Abc_TtWordNum(nVars);
    Vec_Wrd_t * vTruths = Vec_WrdStart( nWords * nFuncs );
    Abc_Random(1);
    for ( i = 0; i < nFuncs; i++ ) {
        word * pTruth = Vec_WrdEntryP(vTruths, i*nWords);
        if ( nMints == 0 )
            for ( k = 0; k < nWords; k++ )
                pTruth[k] = Abc_RandomW(0);
        else {
            for ( k = 0; k < nMints; k++ ) {
                int iMint = 0;
                do iMint = Abc_Random(0) % (1 << nVars);
                while ( Abc_TtGetBit(pTruth, iMint) );
                Abc_TtSetBit( pTruth, iMint );
            }
        }
    }
    Vec_WrdWriteTruthHex( pFileName, vTruths, nVars );
    if ( nMints )
        printf( "Generated %d random %d-variable functions with %d positive minterms and dumped them into file \"%s\".\n", 
            nFuncs, nVars, nMints, pFileName );
    else
        printf( "Generated %d random %d-variable functions and dumped them into file \"%s\".\n", 
            nFuncs, nVars, pFileName );    
    Vec_WrdFree( vTruths );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

