/**CFile****************************************************************

  FileName    [plaSimple.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SOP manager.]

  Synopsis    [Scalable SOP transformations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - March 18, 2015.]

  Revision    [$Id: plaSimple.c,v 1.00 2014/09/12 00:00:00 alanmi Exp $]

***********************************************************************/

#include "pla.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Dump PLA manager into a BLIF file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Pla_ManDumpPla( Pla_Man_t * p, char * pFileName )
{
    // find the number of original variables
    //int nVarsInit = Pla_ManDivNum(p) ? Vec_IntCountZero(&p->vDivs) : Pla_ManInNum(p);
    FILE * pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
        printf( "Cannot open file \"%s\" for writing.\n", pFileName );
    else
    {
        //char * pLits = "-01?";
        Vec_Str_t * vStr;
        Vec_Int_t * vCube;
        int i, k, Lit;
        // comment
        fprintf( pFile, "# PLA file written via PLA package in ABC on " );
        fprintf( pFile, "%s", Extra_TimeStamp() );
        fprintf( pFile, "\n\n" );
        // header
        fprintf( pFile, ".i %d\n", Pla_ManInNum(p) );
        fprintf( pFile, ".o %d\n", 1 );
        fprintf( pFile, ".p %d\n", Vec_WecSize(&p->vCubeLits) );
        // SOP
        vStr = Vec_StrStart( Pla_ManInNum(p) + 1 );
        Vec_WecForEachLevel( &p->vCubeLits, vCube, i )
        {
            if ( !Vec_IntSize(vCube) )
                continue;
            for ( k = 0; k < Pla_ManInNum(p); k++ )
                Vec_StrWriteEntry( vStr, k, '-' );
            Vec_IntForEachEntry( vCube, Lit, k )
                Vec_StrWriteEntry( vStr, Abc_Lit2Var(Lit), (char)(Abc_LitIsCompl(Lit) ? '0' : '1') );
            fprintf( pFile, "%s 1\n", Vec_StrArray(vStr) );
        }
        Vec_StrFree( vStr );
        fprintf( pFile, ".e\n\n" );
        fclose( pFile );
        printf( "Written file \"%s\".\n", pFileName );
    }
}
void Pla_ManDumpBlif( Pla_Man_t * p, char * pFileName )
{
    // find the number of original variables
    int nVarsInit = Pla_ManDivNum(p) ? Vec_IntCountZero(&p->vDivs) : Pla_ManInNum(p);
    FILE * pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
        printf( "Cannot open file \"%s\" for writing.\n", pFileName );
    else
    {
        //char * pLits = "-01?";
        Vec_Str_t * vStr;
        Vec_Int_t * vCube;
        int i, k, Lit, Div;
        // comment
        fprintf( pFile, "# BLIF file written via PLA package in ABC on " );
        fprintf( pFile, "%s", Extra_TimeStamp() );
        fprintf( pFile, "\n\n" );
        // header
        fprintf( pFile, ".model %s\n", Pla_ManName(p) );
        fprintf( pFile, ".inputs" );
        for ( i = 0; i < nVarsInit; i++ )
            fprintf( pFile, " i%d", i );
        fprintf( pFile, "\n" );
        fprintf( pFile, ".outputs o" );
        fprintf( pFile, "\n" );
        // SOP
        fprintf( pFile, ".names" );
        for ( i = 0; i < Pla_ManInNum(p); i++ )
            fprintf( pFile, " i%d", i );
        fprintf( pFile, " o\n" );
        vStr = Vec_StrStart( Pla_ManInNum(p) + 1 );
        Vec_WecForEachLevel( &p->vCubeLits, vCube, i )
        {
            for ( k = 0; k < Pla_ManInNum(p); k++ )
                Vec_StrWriteEntry( vStr, k, '-' );
            Vec_IntForEachEntry( vCube, Lit, k )
                Vec_StrWriteEntry( vStr, Abc_Lit2Var(Lit), (char)(Abc_LitIsCompl(Lit) ? '0' : '1') );
            fprintf( pFile, "%s 1\n", Vec_StrArray(vStr) );
        }
        Vec_StrFree( vStr );
        // divisors
        Vec_IntForEachEntryStart( &p->vDivs, Div, i, nVarsInit )
        {
            int pLits[3] = { (Div >> 2) & 0x3FF, (Div >> 12) & 0x3FF, (Div >> 22) & 0x3FF };
            fprintf( pFile, ".names" );
            fprintf( pFile, " i%d", Abc_Lit2Var(pLits[0]) );
            fprintf( pFile, " i%d", Abc_Lit2Var(pLits[1]) );
            if ( (Div & 3) == 3 ) // MUX
                fprintf( pFile, " i%d", Abc_Lit2Var(pLits[2]) );
            fprintf( pFile, " i%d\n", i );
            if ( (Div & 3) == 1 ) // AND
                fprintf( pFile, "%d%d 1\n", !Abc_LitIsCompl(pLits[0]), !Abc_LitIsCompl(pLits[1]) );
            else if ( (Div & 3) == 2 ) // XOR
            {
                assert( !Abc_LitIsCompl(pLits[0]) );
                assert( !Abc_LitIsCompl(pLits[1]) );
                fprintf( pFile, "10 1\n01 1\n" );
            }
            else if ( (Div & 3) == 3 ) // MUX
            {
                assert( !Abc_LitIsCompl(pLits[1]) );
                assert( !Abc_LitIsCompl(pLits[2]) );
                fprintf( pFile, "%d-0 1\n-11 1\n", !Abc_LitIsCompl(pLits[0]) );
            }
            else assert( 0 );
        }
        fprintf( pFile, ".end\n\n" );
        fclose( pFile );
        printf( "Written file \"%s\".\n", pFileName );
    }
}

/**Function*************************************************************

  Synopsis    [Transforms truth table into an SOP manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Pla_ManExpendDirNum( word * pOn, int nBits, int iMint, int * pVars )
{
    int v, nVars = 0;
    for ( v = 0; v < nBits; v++ )
        if ( Pla_TtGetBit(pOn, iMint ^ (1 << v)) )
            pVars[nVars++] = v;
    return nVars;
}
void Pla_PrintBinary( word * pT, int nBits )
{
    int v;
    for ( v = 0; v < nBits; v++ )
        printf( "%d", Pla_TtGetBit(pT, v) );
    printf( "\n" );
}
Vec_Wrd_t * Pla_ManFxMinimize( word * pOn, int nVars )
{
    int i, v, iMint, iVar, nMints = (1 << nVars);
    int nWords = Abc_Bit6WordNum( nMints );
    Vec_Wrd_t * vCubes = Vec_WrdAlloc( 1000 );
    word * pDc = ABC_CALLOC( word, nWords );
    int Count[32] = {0};
    int Cubes[32] = {0};
    Vec_Int_t * vStore = Vec_IntAlloc( 1000 );

    // count the number of expansion directions
    for ( i = 0; i < nMints; i++ )
        if ( Pla_TtGetBit(pOn, i) && !Pla_TtGetBit(pDc, i) )
        {
            int pDirs[32], nDirs = Pla_ManExpendDirNum(pOn, nVars, i, pDirs);
            Count[nDirs]++;
            if ( nDirs == 0 )
            {
                Pla_TtSetBit(pDc, i);
                Cubes[0]++;
                // save
                Vec_IntPushTwo( vStore, i, -1 );
                continue;
            }
            if ( nDirs == 1 )
            {
                //Pla_PrintBinary( (word *)&i, nVars );
                //printf( " %d \n", pDirs[0] );

                Pla_TtSetBit(pDc, i);
                Pla_TtSetBit(pDc, i ^ (1 << pDirs[0]));
                Cubes[1]++;
                // save
                Vec_IntPushTwo( vStore, i, pDirs[0] );
                continue;
            }
            if ( nDirs == 2 && Pla_TtGetBit(pOn, i ^ (1 << pDirs[0]) ^ (1 << pDirs[1])) )
            {
                assert( 0 );
                Pla_TtSetBit(pDc, i);
                Pla_TtSetBit(pDc, i ^ (1 << pDirs[0]));
                Pla_TtSetBit(pDc, i ^ (1 << pDirs[1]));
                Pla_TtSetBit(pDc, i ^ (1 << pDirs[0]) ^ (1 << pDirs[1]));
                Cubes[2]++;
                continue;
            }
        }

    // go through the remaining cubes
    for ( i = 0; i < nMints; i++ )
        if ( Pla_TtGetBit(pOn, i) && !Pla_TtGetBit(pDc, i) )
        {
            int pDirs[32], nDirs = Pla_ManExpendDirNum(pOn, nVars, i, pDirs);
            // find direction, which is not taken
            for ( v = 0; v < nDirs; v++ )
                if ( Pla_TtGetBit(pOn, i ^ (1 << pDirs[v])) && !Pla_TtGetBit(pDc, i ^ (1 << pDirs[v])) )
                    break;
            // if there is no open directions, use any one
            if ( v == nDirs )
                v = 0;
            // mark this one
            Pla_TtSetBit(pDc, i);
            Pla_TtSetBit(pDc, i ^ (1 << pDirs[v]));
            Cubes[10]++;
            // save
            Vec_IntPushTwo( vStore, i, pDirs[v] );
            continue;
        }

    printf( "\n" );
    printf( "Truth = %d. ", Pla_TtCountOnes(pOn, nWords) );
    printf( "Cover = %d. ", Pla_TtCountOnes(pDc, nWords) );
    printf( "\n" );

    printf( "Count: " );
    for ( i = 0; i < 16; i++ )
        if ( Count[i] )
            printf( "%d=%d ", i, Count[i] );
    printf( "\n" );

    printf( "Cubes: " );
    for ( i = 0; i < 16; i++ )
        if ( Cubes[i] )
            printf( "%d=%d ", i, Cubes[i] );
    printf( "\n" );

/*
    // extract cubes one at a time
    for ( i = 0; i < nMints; i++ )
        if ( Pla_TtGetBit(pOn, i) )
        {
            word Cube = 0;
            for ( v = 0; v < nVars; v++ )
                if ( (i >> v) & 1 )
                    Pla_CubeSetLit( &Cube, v, PLA_LIT_ONE );
                else
                    Pla_CubeSetLit( &Cube, v, PLA_LIT_ZERO );
            Vec_WrdPush( vCubes, Cube );
        }
*/
    Vec_IntForEachEntryDouble( vStore, iMint, iVar, i )
    {
        word Cube = 0;
        for ( v = 0; v < nVars; v++ )
        {
            if ( v == iVar )
                continue;
            if ( (iMint >> v) & 1 )
                Pla_CubeSetLit( &Cube, v, PLA_LIT_ONE );
            else
                Pla_CubeSetLit( &Cube, v, PLA_LIT_ZERO );
        }
        Vec_WrdPush( vCubes, Cube );
    }
    Vec_IntFree( vStore );

    // collect the minterms
    ABC_FREE( pDc );
    return vCubes;
}
Pla_Man_t * Pla_ManFxPrepare( int nVars )
{
    Pla_Man_t * p; char Buffer[1000]; 
    Vec_Bit_t * vFunc = Pla_ManPrimesTable( nVars );
    Vec_Wrd_t * vSop = Pla_ManFxMinimize( (word *)Vec_BitArray(vFunc), nVars );
    word Cube, * pCube = &Cube; int i, k, Lit;
    sprintf( Buffer, "primes%02d", nVars );
    p = Pla_ManAlloc( Buffer, nVars, 1, Vec_WrdSize(vSop) );
    Vec_WecInit( &p->vCubeLits, Pla_ManCubeNum(p) );
    Vec_WecInit( &p->vOccurs, 2*Pla_ManInNum(p) );
    Vec_WrdForEachEntry( vSop, Cube, i )
        Pla_CubeForEachLit( nVars, pCube, Lit, k )
            if ( Lit != PLA_LIT_DASH )
            {
                Lit = Abc_Var2Lit( k, Lit == PLA_LIT_ZERO );
                Vec_WecPush( &p->vCubeLits, i, Lit );
                Vec_WecPush( &p->vOccurs, Lit, i );
            }
    Vec_BitFree( vFunc );
    Vec_WrdFree( vSop );
    return p;
}
int Pla_ManFxPerformSimple( int nVars )
{
    char Buffer[100];
    Pla_Man_t * p = Pla_ManFxPrepare( nVars );
    sprintf( Buffer, "primesmin%02d.pla", nVars );
    Pla_ManDumpPla( p, Buffer );
    Pla_ManFree( p );
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

