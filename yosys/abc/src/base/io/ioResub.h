/**CFile****************************************************************

  FileName    [ioResub.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: ioResub.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__base__io__ioResub_h
#define ABC__base__io__ioResub_h


ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Abc_RData_t_ Abc_RData_t;
struct Abc_RData_t_
{
    int         nIns;       // the number of inputs
    int         nOuts;      // the number of outputs
    int         nPats;      // the number of patterns
    int         nSimWords;  // the number of words needed to store the patterns
    Vec_Wrd_t * vSimsIn;    // input simulation signatures
    Vec_Wrd_t * vSimsOut;   // output simulation signatures
    Vec_Int_t * vDivs;      // divisors
    Vec_Int_t * vSol;       // solution
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

static inline int Abc_RDataGetIn ( Abc_RData_t * p, int i, int b ) { return Abc_InfoHasBit((unsigned *)Vec_WrdEntryP(p->vSimsIn,  i*p->nSimWords), b); }
static inline int Abc_RDataGetOut( Abc_RData_t * p, int i, int b ) { return Abc_InfoHasBit((unsigned *)Vec_WrdEntryP(p->vSimsOut, i*p->nSimWords), b); }

static inline void Abc_RDataSetIn ( Abc_RData_t * p, int i, int b ) { Abc_InfoSetBit((unsigned *)Vec_WrdEntryP(p->vSimsIn,  i*p->nSimWords), b); }
static inline void Abc_RDataSetOut( Abc_RData_t * p, int i, int b ) { Abc_InfoSetBit((unsigned *)Vec_WrdEntryP(p->vSimsOut, i*p->nSimWords), b); }

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

extern void Extra_BitMatrixTransposeP( Vec_Wrd_t * vSimsIn, int nWordsIn, Vec_Wrd_t * vSimsOut, int nWordsOut );

/**Function*************************************************************

  Synopsis    [File interface to read/write resub data.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Abc_RData_t * Abc_RDataStart( int nIns, int nOuts, int nPats )
{
    Abc_RData_t * p = ABC_CALLOC( Abc_RData_t, 1 );
    p->nIns  = nIns;
    p->nOuts = nOuts;
    p->nPats = nPats;
    p->nSimWords = Abc_Bit6WordNum(nPats);
    p->vSimsIn   = Vec_WrdStart( p->nIns * p->nSimWords );
    p->vSimsOut  = Vec_WrdStart( 2*p->nOuts * p->nSimWords ); 
    p->vDivs     = Vec_IntAlloc( 16 );
    p->vSol      = Vec_IntAlloc( 16 );
    return p;
}
static inline void Abc_RDataStop( Abc_RData_t * p )
{
    Vec_IntFree( p->vSol );
    Vec_IntFree( p->vDivs );
    Vec_WrdFree( p->vSimsIn );
    Vec_WrdFree( p->vSimsOut );
    ABC_FREE( p );
}
static inline int Abc_ReadPlaResubParams( char * pFileName, int * pnIns, int * pnOuts, int * pnPats )
{
    FILE * pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL ) {
        printf( "Cannot open file \"%s\" for reading.\n", pFileName );
        return 0;
    }
    int nLineSize = 1000000, iLine = 0;
    char * pBuffer = ABC_ALLOC( char, nLineSize ); 
    *pnIns = *pnOuts = *pnPats = 0;
    while ( fgets( pBuffer, nLineSize, pFile ) != NULL ) {
        iLine += (pBuffer[0] == '0' || pBuffer[0] == '1' || pBuffer[0] == '-');
        if ( pBuffer[0] != '.' )
            continue;
        if ( pBuffer[1] == 'i' )
            *pnIns = atoi(pBuffer+2);
        else if ( pBuffer[1] == 'o' )
            *pnOuts = atoi(pBuffer+2);
        else if ( pBuffer[1] == 'p' )
            *pnPats = atoi(pBuffer+2);
        else if ( pBuffer[1] == 'e' )
            break;
    }
    if ( *pnPats == 0 )
        *pnPats = iLine;
    else if ( *pnPats != iLine )
        printf( "The number of lines in the file (%d) does not match the number listed in .p (%d).\n", iLine, *pnPats );
    fclose(pFile);
    free( pBuffer );
    return 1;
}
static inline int Abc_ReadPlaResubData( Abc_RData_t * p, char * pFileName )
{
    FILE * pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL ) {
        printf( "Cannot open file \"%s\" for reading.\n", pFileName );
        return 0;
    }
    int i, iLine = 0, nDashes = 0, MaxLineSize = p->nIns+p->nOuts+10000;
    char * pTemp, * pBuffer = ABC_ALLOC( char, MaxLineSize );
    while ( fgets( pBuffer, MaxLineSize, pFile ) != NULL ) {
        if ( pBuffer[0] == '0' || pBuffer[0] == '1' || pBuffer[0] == '-' ) {
            for ( pTemp = pBuffer, i = 0; *pTemp; pTemp++ ) {
                if ( i < p->nIns ) { // input part
                    nDashes += (*pTemp == '-');
                    if ( *pTemp == '1' )
                        Abc_InfoSetBit( (unsigned *)Vec_WrdEntryP(p->vSimsIn, i*p->nSimWords), iLine );
                }
                else { // output part
                    if ( *pTemp == '0' )
                        Abc_InfoSetBit( (unsigned *)Vec_WrdEntryP(p->vSimsOut, (2*(i-p->nIns)+0)*p->nSimWords), iLine );
                    else if ( *pTemp == '1' )
                        Abc_InfoSetBit( (unsigned *)Vec_WrdEntryP(p->vSimsOut, (2*(i-p->nIns)+1)*p->nSimWords), iLine );
                    //else if ( *pTemp == '-' ) {
                    //    Abc_InfoSetBit( (unsigned *)Vec_WrdEntryP(p->vSimsOut, (2*(i-p->nIns)+0)*p->nSimWords), iLine );
                    //    Abc_InfoSetBit( (unsigned *)Vec_WrdEntryP(p->vSimsOut, (2*(i-p->nIns)+1)*p->nSimWords), iLine );
                    //}
                }
                i += (*pTemp == '0' || *pTemp == '1' || *pTemp == '-');
            }
            assert( i == p->nIns + p->nOuts );
            iLine++;
        }
        if ( pBuffer[0] == '.' && (pBuffer[1] == 's' || pBuffer[1] == 'a') ) {
            Vec_Int_t * vArray = pBuffer[1] == 'a' ? p->vSol : p->vDivs;
            if ( Vec_IntSize(vArray) > 0 )
                continue;
            char * pTemp = strtok( pBuffer+2, " \r\n\t" );
            do Vec_IntPush( vArray, atoi(pTemp) );
            while ( (pTemp = strtok( NULL, " \r\n\t" )) );
        }
    }
    if ( nDashes )
        printf( "Several (%d) don't-care literals in the input part are replaced by zeros \"%s\" \n", nDashes, pFileName );
    assert ( iLine == p->nPats );
    ABC_FREE(pBuffer);
    fclose(pFile);
    return 1;
}
static inline Abc_RData_t * Abc_ReadPla( char * pFileName )
{    
    int nIns, nOuts, nPats;
    int RetValue = Abc_ReadPlaResubParams( pFileName, &nIns, &nOuts, &nPats );
    if ( !RetValue ) return NULL;
    Abc_RData_t * p = Abc_RDataStart( nIns, nOuts, nPats );
    RetValue = Abc_ReadPlaResubData( p, pFileName );
    return p;
}
static inline void Abc_WritePla( Abc_RData_t * p, char * pFileName, int fRel )
{
    FILE * pFile = fopen( pFileName, "wb" ); int i, k;
    if ( pFile == NULL ) {
        printf( "Cannot open file \"%s\" for writing.\n", pFileName );
        return;
    }
    assert(  fRel || Vec_WrdSize(p->vSimsOut) == 2*p->nOuts*p->nSimWords );
    assert( !fRel || Vec_WrdSize(p->vSimsOut) == (1 << p->nOuts)*p->nSimWords );
    fprintf( pFile, ".i %d\n", p->nIns );
    fprintf( pFile, ".o %d\n", p->nOuts );
    if ( fRel == 2 ) {
        int n, iLine = 0, Count = 0; word Entry;
        Vec_WrdForEachEntry( p->vSimsOut, Entry, i )
            Count += Abc_TtCountOnes(Entry);
        fprintf( pFile, ".p %d\n", Count );
        for ( i = 0; i < p->nPats; i++ ) 
        for ( n = 0; n < (1 << p->nOuts); n++ ) {
            if ( !Abc_InfoHasBit((unsigned *)Vec_WrdEntryP(p->vSimsOut, n*p->nSimWords), i) )
                continue;
            for ( k = 0; k < p->nIns; k++ )
                fprintf( pFile, "%d", Abc_InfoHasBit((unsigned *)Vec_WrdEntryP(p->vSimsIn, k*p->nSimWords), i) );
            fprintf( pFile, " ");
            for ( k = 0; k < p->nOuts; k++ )
                fprintf( pFile, "%d", (n >> k) & 1 );
            fprintf( pFile, "\n");
            iLine++;
        }
        assert( iLine == Count );
    }
    else {
        fprintf( pFile, ".p %d\n", p->nPats );
        for ( i = 0; i < p->nPats; i++ ) {
            for ( k = 0; k < p->nIns; k++ )
                fprintf( pFile, "%d", Abc_InfoHasBit((unsigned *)Vec_WrdEntryP(p->vSimsIn, k*p->nSimWords), i) );
            fprintf( pFile, " ");
            if ( !fRel ) { // multi-output function
                for ( k = 0; k < p->nOuts; k++ ) {
                    int Val0 = Abc_InfoHasBit((unsigned *)Vec_WrdEntryP(p->vSimsOut, (2*k+0)*p->nSimWords), i);
                    int Val1 = Abc_InfoHasBit((unsigned *)Vec_WrdEntryP(p->vSimsOut, (2*k+1)*p->nSimWords), i);
                    char Val = (Val0 && Val1) ? '-' : Val1 ? '1' : '0';
                    fprintf( pFile, "%c", Val );
                }
            }
            else { // relation
                for ( k = 0; k < (1 << p->nOuts); k++ )
                    fprintf( pFile, "%d", Abc_InfoHasBit((unsigned *)Vec_WrdEntryP(p->vSimsOut, k*p->nSimWords), i) );
            }
            fprintf( pFile, "\n");
        }
    }
    fprintf( pFile, ".e\n" );
    fclose(pFile);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_RDataIsRel( Abc_RData_t * p )
{
    assert( p->nIns < 64 );
    Vec_Wrd_t * vTransIn = Vec_WrdStart( 64*p->nSimWords );
    Extra_BitMatrixTransposeP( p->vSimsIn, p->nSimWords, vTransIn, 64*p->nSimWords );
    Vec_WrdShrink( vTransIn, p->nPats );
    Vec_WrdUniqify( vTransIn );
    int Value = Vec_WrdSize(vTransIn) < p->nPats;
    Vec_WrdFree( vTransIn );
    return Value;
}
static inline Abc_RData_t * Abc_RData2Rel( Abc_RData_t * p )
{
    assert( p->nIns < 64 );
    assert( p->nOuts < 32 );
    int w;
    Vec_Wrd_t * vSimsIn2  = Vec_WrdStart( 64*p->nSimWords );
    Vec_Wrd_t * vSimsOut2 = Vec_WrdStart( 64*p->nSimWords );
    for ( w = 0; w < p->nIns; w++ )
        Abc_TtCopy( Vec_WrdEntryP(vSimsIn2,  w*p->nSimWords), Vec_WrdEntryP(p->vSimsIn, w*p->nSimWords), p->nSimWords, 0 );
    for ( w = 0; w < p->nOuts; w++ )
        Abc_TtCopy( Vec_WrdEntryP(vSimsOut2, w*p->nSimWords), Vec_WrdEntryP(p->vSimsOut, (2*w+1)*p->nSimWords), p->nSimWords, 0 );
    Vec_Wrd_t * vTransIn  = Vec_WrdStart( 64*p->nSimWords );
    Vec_Wrd_t * vTransOut = Vec_WrdStart( 64*p->nSimWords );
    Extra_BitMatrixTransposeP( vSimsIn2,  p->nSimWords, vTransIn,  1 );
    Extra_BitMatrixTransposeP( vSimsOut2, p->nSimWords, vTransOut, 1 );
    Vec_WrdShrink( vTransIn,  p->nPats );
    Vec_WrdShrink( vTransOut, p->nPats );
    Vec_Wrd_t * vTransInCopy = Vec_WrdDup(vTransIn); 
    Vec_WrdUniqify( vTransInCopy );
//    if ( Vec_WrdSize(vTransInCopy) == p->nPats )
//        printf( "This resub problem is not a relation.\n" );
    // create the relation
    Abc_RData_t * pNew = Abc_RDataStart( p->nIns, 1 << (p->nOuts-1), Vec_WrdSize(vTransInCopy) );
    pNew->nOuts = p->nOuts;
    int i, k, n, iLine = 0; word Entry, Entry2;
    Vec_WrdForEachEntry( vTransInCopy, Entry, i ) {
        for ( n = 0; n < p->nIns; n++ )
            if ( (Entry >> n) & 1 )
                Abc_InfoSetBit( (unsigned *)Vec_WrdEntryP(pNew->vSimsIn, n*pNew->nSimWords), iLine );
        Vec_WrdForEachEntry( vTransIn, Entry2, k ) {
            if ( Entry != Entry2 )
                continue;
            Entry2 = Vec_WrdEntry( vTransOut, k );
            assert( Entry2 < (1 << p->nOuts) );
            Abc_InfoSetBit( (unsigned *)Vec_WrdEntryP(pNew->vSimsOut, Entry2*pNew->nSimWords), iLine );
        }
        iLine++;
    }
    assert( iLine == pNew->nPats );
    Vec_WrdFree( vTransOut );
    Vec_WrdFree( vTransInCopy );
    Vec_WrdFree( vTransIn );
    Vec_WrdFree( vSimsIn2 );
    Vec_WrdFree( vSimsOut2 );
    return pNew;    
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
/*
void Abc_ReadPlaTest( char * pFileName )
{
    Abc_RData_t * p = Abc_ReadPla( pFileName );
    Abc_WritePla( p, "resub_out.pla", 0 );
    Abc_RData_t * p2 = Abc_RData2Rel( p );
    Abc_WritePla( p2, "resub_out1.pla", 1 );
    Abc_WritePla( p2, "resub_out2.pla", 2 );
    Abc_RDataStop( p2 );
    Abc_RDataStop( p );
}
*/

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

