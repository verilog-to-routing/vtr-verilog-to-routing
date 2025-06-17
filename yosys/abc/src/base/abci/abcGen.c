/**CFile****************************************************************

  FileName    [abcGen.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Procedures to generate various type of circuits.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcGen.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "aig/miniaig/miniaig.h"

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
void Abc_WriteHalfAdder( FILE * pFile )
{
    int fNaive = 0;
    fprintf( pFile, ".model HA\n" );
    fprintf( pFile, ".inputs a b\n" ); 
    fprintf( pFile, ".outputs s cout\n" ); 
    if ( fNaive )
    {
        fprintf( pFile, ".names a b s\n" ); 
        fprintf( pFile, "10 1\n" ); 
        fprintf( pFile, "01 1\n" ); 
        fprintf( pFile, ".names a b cout\n" ); 
        fprintf( pFile, "11 1\n" ); 
    }
    else
    {
        fprintf( pFile, ".names a b cout\n" ); 
        fprintf( pFile, "11 1\n" ); 
        fprintf( pFile, ".names a b and1_\n" ); 
        fprintf( pFile, "00 1\n" ); 
        fprintf( pFile, ".names cout and1_ s\n" ); 
        fprintf( pFile, "00 1\n" ); 
    }
    fprintf( pFile, ".end\n" ); 
    fprintf( pFile, "\n" ); 
}
void Abc_WriteFullAdder( FILE * pFile )
{
    int fNaive = 0;
    fprintf( pFile, ".model FA\n" );
    fprintf( pFile, ".inputs a b cin\n" ); 
    fprintf( pFile, ".outputs s cout\n" ); 
    if ( fNaive )
    {
/*        
        fprintf( pFile, ".names a b k\n" ); 
        fprintf( pFile, "10 1\n" ); 
        fprintf( pFile, "01 1\n" ); 
        fprintf( pFile, ".names k cin s\n" ); 
        fprintf( pFile, "10 1\n" ); 
        fprintf( pFile, "01 1\n" ); 
        fprintf( pFile, ".names a b cin cout\n" ); 
        fprintf( pFile, "11- 1\n" ); 
        fprintf( pFile, "1-1 1\n" ); 
        fprintf( pFile, "-11 1\n" ); 
*/        
        fprintf( pFile, ".names a b s0\n" ); 
        fprintf( pFile, "10 1\n" ); 
        fprintf( pFile, "01 1\n" ); 
        fprintf( pFile, ".names a b c0\n" ); 
        fprintf( pFile, "11 1\n" ); 
        fprintf( pFile, ".names s0 cin s\n" ); 
        fprintf( pFile, "10 1\n" ); 
        fprintf( pFile, "01 1\n" ); 
        fprintf( pFile, ".names s0 cin c1\n" ); 
        fprintf( pFile, "11 1\n" ); 
        fprintf( pFile, ".names c0 c1 cout\n" ); 
        fprintf( pFile, "00 0\n" ); 
    }
    else
    {
        fprintf( pFile, ".names a b and1\n" ); 
        fprintf( pFile, "11 1\n" ); 
        fprintf( pFile, ".names a b and1_\n" ); 
        fprintf( pFile, "00 1\n" ); 
        fprintf( pFile, ".names and1 and1_ xor\n" ); 
        fprintf( pFile, "00 1\n" ); 

        fprintf( pFile, ".names cin xor and2\n" ); 
        fprintf( pFile, "11 1\n" ); 
        fprintf( pFile, ".names cin xor and2_\n" ); 
        fprintf( pFile, "00 1\n" ); 
        fprintf( pFile, ".names and2 and2_ s\n" ); 
        fprintf( pFile, "00 1\n" ); 

        fprintf( pFile, ".names and1 and2 cout\n" ); 
        fprintf( pFile, "00 0\n" ); 
    }
    fprintf( pFile, ".end\n" ); 
    fprintf( pFile, "\n" ); 
}
void Abc_WriteAdder( FILE * pFile, int nVars )
{
    int i, nDigits = Abc_Base10Log( nVars );

    assert( nVars > 0 );
    fprintf( pFile, ".model ADD%d\n", nVars );

    fprintf( pFile, ".inputs" );
    for ( i = 0; i < nVars; i++ )
        fprintf( pFile, " a%0*d", nDigits, i );
    for ( i = 0; i < nVars; i++ )
        fprintf( pFile, " b%0*d", nDigits, i );
    fprintf( pFile, "\n" );

    fprintf( pFile, ".outputs" );
    for ( i = 0; i <= nVars; i++ )
        fprintf( pFile, " s%0*d", nDigits, i );
    fprintf( pFile, "\n" );

    fprintf( pFile, ".names c\n" );
    if ( nVars == 1 )
        fprintf( pFile, ".subckt FA a=a0 b=b0 cin=c s=y0 cout=s1\n" );
    else
    {
        fprintf( pFile, ".subckt FA a=a%0*d b=b%0*d cin=c s=s%0*d cout=%0*d\n", nDigits, 0, nDigits, 0, nDigits, 0, nDigits, 0 );
        for ( i = 1; i < nVars-1; i++ )
            fprintf( pFile, ".subckt FA a=a%0*d b=b%0*d cin=%0*d s=s%0*d cout=%0*d\n", nDigits, i, nDigits, i, nDigits, i-1, nDigits, i, nDigits, i );
        fprintf( pFile, ".subckt FA a=a%0*d b=b%0*d cin=%0*d s=s%0*d cout=s%0*d\n", nDigits, i, nDigits, i, nDigits, i-1, nDigits, i, nDigits, i+1 );
    }
    fprintf( pFile, ".end\n" ); 
    fprintf( pFile, "\n" );
    Abc_WriteFullAdder( pFile );
}
void Abc_GenAdder( char * pFileName, int nVars )
{
    FILE * pFile;
    assert( nVars > 0 );
    pFile = fopen( pFileName, "w" );
    fprintf( pFile, "# %d-bit ripple-carry adder generated by ABC on %s\n", nVars, Extra_TimeStamp() );
    Abc_WriteAdder( pFile, nVars );
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_WriteMulti( FILE * pFile, int nVars )
{
    int i, k, nDigits = Abc_Base10Log( nVars ), nDigits2 = Abc_Base10Log( 2*nVars );

    assert( nVars > 0 );
    fprintf( pFile, ".model Multi%d\n", nVars );

    fprintf( pFile, ".inputs" );
    for ( i = 0; i < nVars; i++ )
        fprintf( pFile, " a%0*d", nDigits, i );
    for ( i = 0; i < nVars; i++ )
        fprintf( pFile, " b%0*d", nDigits, i );
    fprintf( pFile, "\n" );

    fprintf( pFile, ".outputs" );
    for ( i = 0; i < 2*nVars; i++ )
        fprintf( pFile, " m%0*d", nDigits2, i );
    fprintf( pFile, "\n" );

    for ( i = 0; i < 2*nVars; i++ )
        fprintf( pFile, ".names x%0*d_%0*d\n", nDigits, 0, nDigits2, i );
    for ( k = 0; k < nVars; k++ )
    {
        for ( i = 0; i < 2 * nVars; i++ )
            if ( i >= k && i < k + nVars )
                fprintf( pFile, ".names b%0*d a%0*d y%0*d_%0*d\n11 1\n", nDigits, k, nDigits, i-k, nDigits, k, nDigits2, i );
            else
                fprintf( pFile, ".names y%0*d_%0*d\n", nDigits, k, nDigits2, i );
        fprintf( pFile, ".subckt ADD%d", 2*nVars );
        for ( i = 0; i < 2*nVars; i++ )
            fprintf( pFile, " a%0*d=x%0*d_%0*d", nDigits2, i, nDigits, k, nDigits2, i );
        for ( i = 0; i < 2*nVars; i++ )
            fprintf( pFile, " b%0*d=y%0*d_%0*d", nDigits2, i, nDigits, k, nDigits2, i );
        for ( i = 0; i <= 2*nVars; i++ )
            fprintf( pFile, " s%0*d=x%0*d_%0*d", nDigits2, i, nDigits, k+1, nDigits2, i );
        fprintf( pFile, "\n" );
    }
    for ( i = 0; i < 2 * nVars; i++ )
        fprintf( pFile, ".names x%0*d_%0*d m%0*d\n1 1\n", nDigits, k, nDigits2, i, nDigits2, i );
    fprintf( pFile, ".end\n" ); 
    fprintf( pFile, "\n" );
    Abc_WriteAdder( pFile, 2*nVars );
}
void Abc_GenMulti( char * pFileName, int nVars )
{
    FILE * pFile;
    assert( nVars > 0 );
    pFile = fopen( pFileName, "w" );
    fprintf( pFile, "# %d-bit multiplier generated by ABC on %s\n", nVars, Extra_TimeStamp() );
    Abc_WriteMulti( pFile, nVars );
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_WriteComp( FILE * pFile )
{
    fprintf( pFile, ".model Comp\n" );
    fprintf( pFile, ".inputs a b\n" ); 
    fprintf( pFile, ".outputs x y\n" ); 
    fprintf( pFile, ".names a b x\n" ); 
    fprintf( pFile, "11 1\n" ); 
    fprintf( pFile, ".names a b y\n" ); 
    fprintf( pFile, "1- 1\n" ); 
    fprintf( pFile, "-1 1\n" ); 
    fprintf( pFile, ".end\n" ); 
    fprintf( pFile, "\n" ); 
}
void Abc_WriteLayer( FILE * pFile, int nVars, int fSkip1 )
{
    int i;
    fprintf( pFile, ".model Layer%d\n", fSkip1 );
    fprintf( pFile, ".inputs" ); 
    for ( i = 0; i < nVars; i++ )
        fprintf( pFile, " x%02d", i ); 
    fprintf( pFile, "\n" ); 
    fprintf( pFile, ".outputs" ); 
    for ( i = 0; i < nVars; i++ )
        fprintf( pFile, " y%02d", i ); 
    fprintf( pFile, "\n" ); 
    if ( fSkip1 )
    {
        fprintf( pFile, ".names x00 y00\n" ); 
        fprintf( pFile, "1 1\n" ); 
        i = 1;
    }
    else
        i = 0;
    for ( ; i + 1 < nVars; i += 2 )
        fprintf( pFile, ".subckt Comp a=x%02d b=x%02d x=y%02d y=y%02d\n", i, i+1, i, i+1 );
    if ( i < nVars )
    {
        fprintf( pFile, ".names x%02d y%02d\n", i, i ); 
        fprintf( pFile, "1 1\n" ); 
    }
    fprintf( pFile, ".end\n" ); 
    fprintf( pFile, "\n" ); 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_GenSorter( char * pFileName, int nVars )
{
    FILE * pFile;
    int i, k, Counter, nDigits;

    assert( nVars > 1 );

    pFile = fopen( pFileName, "w" );
    fprintf( pFile, "# %d-bit sorter generated by ABC on %s\n", nVars, Extra_TimeStamp() );
    fprintf( pFile, ".model Sorter%02d\n", nVars );

    fprintf( pFile, ".inputs" );
    for ( i = 0; i < nVars; i++ )
        fprintf( pFile, " x%02d", i );
    fprintf( pFile, "\n" );

    fprintf( pFile, ".outputs" );
    for ( i = 0; i < nVars; i++ )
        fprintf( pFile, " y%02d", i );
    fprintf( pFile, "\n" );

    Counter = 0;
    nDigits = Abc_Base10Log( (nVars-2)*nVars );
    if ( nVars == 2 )
        fprintf( pFile, ".subckt Comp a=x00 b=x01 x=y00 y=y01\n" );
    else
    {
        fprintf( pFile, ".subckt Layer0" );
        for ( k = 0; k < nVars; k++ )
            fprintf( pFile, " x%02d=x%02d", k, k );
        for ( k = 0; k < nVars; k++ )
            fprintf( pFile, " y%02d=%0*d", k, nDigits, Counter++ );
        fprintf( pFile, "\n" );
        Counter -= nVars;
        for ( i = 1; i < 2*nVars-2; i++ )
        {
            fprintf( pFile, ".subckt Layer%d", (i&1) );
            for ( k = 0; k < nVars; k++ )
                fprintf( pFile, " x%02d=%0*d", k, nDigits, Counter++ );
            for ( k = 0; k < nVars; k++ )
                fprintf( pFile, " y%02d=%0*d", k, nDigits, Counter++ );
            fprintf( pFile, "\n" );
            Counter -= nVars;
        }
        fprintf( pFile, ".subckt Layer%d", (i&1) );
        for ( k = 0; k < nVars; k++ )
            fprintf( pFile, " x%02d=%0*d", k, nDigits, Counter++ );
        for ( k = 0; k < nVars; k++ )
            fprintf( pFile, " y%02d=y%02d", k, k );
        fprintf( pFile, "\n" );
    }
    fprintf( pFile, ".end\n" ); 
    fprintf( pFile, "\n" );

    Abc_WriteLayer( pFile, nVars, 0 );
    Abc_WriteLayer( pFile, nVars, 1 );
    Abc_WriteComp( pFile );
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_WriteCell( FILE * pFile )
{
    fprintf( pFile, ".model cell\n" );
    fprintf( pFile, ".inputs px1 px2 py1 py2 x y\n" ); 
    fprintf( pFile, ".outputs fx fy\n" ); 
    fprintf( pFile, ".names x y a\n" ); 
    fprintf( pFile, "11 1\n" ); 
    fprintf( pFile, ".names px1 a x nx\n" ); 
    fprintf( pFile, "11- 1\n" ); 
    fprintf( pFile, "0-1 1\n" ); 
    fprintf( pFile, ".names py1 a y ny\n" ); 
    fprintf( pFile, "11- 1\n" ); 
    fprintf( pFile, "0-1 1\n" ); 
    fprintf( pFile, ".names px2 nx fx\n" ); 
    fprintf( pFile, "10 1\n" ); 
    fprintf( pFile, "01 1\n" ); 
    fprintf( pFile, ".names py2 ny fy\n" ); 
    fprintf( pFile, "10 1\n" ); 
    fprintf( pFile, "01 1\n" ); 
    fprintf( pFile, ".end\n" ); 
    fprintf( pFile, "\n" ); 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_GenMesh( char * pFileName, int nVars )
{
    FILE * pFile;
    int i, k;

    assert( nVars > 0 );

    pFile = fopen( pFileName, "w" );
    fprintf( pFile, "# %dx%d mesh generated by ABC on %s\n", nVars, nVars, Extra_TimeStamp() );
    fprintf( pFile, ".model mesh%d\n", nVars );

    for ( i = 0; i < nVars; i++ )
        for ( k = 0; k < nVars; k++ )
        {
            fprintf( pFile, ".inputs" );
            fprintf( pFile, " p%d%dx1", i, k );
            fprintf( pFile, " p%d%dx2", i, k );
            fprintf( pFile, " p%d%dy1", i, k );
            fprintf( pFile, " p%d%dy2", i, k );
            fprintf( pFile, "\n" );
        }
    fprintf( pFile, ".inputs" );
    for ( i = 0; i < nVars; i++ )
        fprintf( pFile, " v%02d v%02d", 2*i, 2*i+1 );
    fprintf( pFile, "\n" );

    fprintf( pFile, ".outputs" );
    fprintf( pFile, " fx00" );
    fprintf( pFile, "\n" );

    for ( i = 0; i < nVars; i++ ) // horizontal
        for ( k = 0; k < nVars; k++ ) // vertical
        {
            fprintf( pFile, ".subckt cell" );
            fprintf( pFile, " px1=p%d%dx1", i, k );
            fprintf( pFile, " px2=p%d%dx2", i, k );
            fprintf( pFile, " py1=p%d%dy1", i, k );
            fprintf( pFile, " py2=p%d%dy2", i, k );
            if ( k == nVars - 1 )
                fprintf( pFile, " x=v%02d", i );
            else
                fprintf( pFile, " x=fx%d%d", i, k+1 );
            if ( i == nVars - 1 )
                fprintf( pFile, " y=v%02d", nVars+k );
            else
                fprintf( pFile, " y=fy%d%d", i+1, k );
            // outputs
            fprintf( pFile, " fx=fx%d%d", i, k );
            fprintf( pFile, " fy=fy%d%d", i, k );
            fprintf( pFile, "\n" );
        }
    fprintf( pFile, ".end\n" ); 
    fprintf( pFile, "\n" );
    fprintf( pFile, "\n" );

    Abc_WriteCell( pFile );
    fclose( pFile );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_WriteKLut( FILE * pFile, int nLutSize )
{
    int i, iVar, iNext, nPars = (1 << nLutSize);
    fprintf( pFile, "\n" ); 
    fprintf( pFile, ".model lut%d\n", nLutSize );
    fprintf( pFile, ".inputs" );
    for ( i = 0; i < nPars; i++ )
        fprintf( pFile, " p%02d", i );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".inputs" );
    for ( i = 0; i < nLutSize; i++ )
        fprintf( pFile, " i%d", i );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".outputs o\n" ); 
    fprintf( pFile, ".names n01 o\n" ); 
    fprintf( pFile, "1 1\n" ); 
    // write internal MUXes
    iVar = 0;
    iNext = 2;
    for ( i = 1; i < nPars; i++ )
    {
        if ( i == iNext )
        {
            iNext *= 2;
            iVar++; 
        }
        if ( iVar == nLutSize - 1 )
            fprintf( pFile, ".names i%d p%02d p%02d n%02d\n", iVar, 2*(i-nPars/2), 2*(i-nPars/2)+1, i ); 
        else
            fprintf( pFile, ".names i%d n%02d n%02d n%02d\n", iVar, 2*i, 2*i+1, i ); 
        fprintf( pFile, "01- 1\n" ); 
        fprintf( pFile, "1-1 1\n" ); 
    }
    fprintf( pFile, ".end\n" ); 
    fprintf( pFile, "\n" ); 
}

/**Function*************************************************************

  Synopsis    [Generates structure of L K-LUTs implementing an N-var function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_GenFpga( char * pFileName, int nLutSize, int nLuts, int nVars )
{
    int fGenerateFunc = 1;
    FILE * pFile;
    int nVarsLut = (1 << nLutSize);                     // the number of LUT variables
    int nVarsLog = Abc_Base2Log( nVars + nLuts - 1 ); // the number of encoding vars
    int nVarsDeg = (1 << nVarsLog);                     // the number of LUT variables (total)
    int nParsLut = nLuts * (1 << nLutSize);             // the number of LUT params
    int nParsVar = nLuts * nLutSize * nVarsLog;         // the number of var params
    int i, j, k;

    assert( nVars > 0 );

    pFile = fopen( pFileName, "w" );
    fprintf( pFile, "# Structure with %d %d-LUTs for %d-var function generated by ABC on %s\n", nLuts, nLutSize, nVars, Extra_TimeStamp() );
    fprintf( pFile, ".model struct%dx%d_%d\n", nLuts, nLutSize, nVars );

    fprintf( pFile, ".inputs" );
    for ( i = 0; i < nParsLut; i++ )
    {
//        if ( i % (1 << nLutSize) == 0 && i != (nLuts - 1) * (1 << nLutSize) )
//            continue;
        fprintf( pFile, " pl%02d", i );
    }
    fprintf( pFile, "\n" );

    fprintf( pFile, ".inputs" );
    for ( i = 0; i < nParsVar; i++ )
        fprintf( pFile, " pv%02d", i );
    fprintf( pFile, "\n" );

    fprintf( pFile, ".inputs" );
    for ( i = 0; i < nVars; i++ )
        fprintf( pFile, " v%02d", i );
    fprintf( pFile, "\n" );

    fprintf( pFile, ".outputs" );
//    fprintf( pFile, " v%02d", nVars + nLuts - 1 );
    fprintf( pFile, " out" );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".names Gnd\n" ); 
    fprintf( pFile, " 0\n" ); 

    // generate function
    if ( fGenerateFunc )
    {
        fprintf( pFile, ".names v%02d func out\n", nVars + nLuts - 1 );
        fprintf( pFile, "00 1\n11 1\n" );
        fprintf( pFile, ".names" );
        for ( i = 0; i < nVars; i++ )
            fprintf( pFile, " v%02d", i );
        fprintf( pFile, " func\n" );
        for ( i = 0; i < nVars; i++ )
            fprintf( pFile, "1" );
        fprintf( pFile, " 1\n" );
    }
    else
        fprintf( pFile, ".names v%02d out\n1 1\n", nVars + nLuts - 1 );

    // generate LUTs
    for ( i = 0; i < nLuts; i++ )
    {
        fprintf( pFile, ".subckt lut%d", nLutSize );
        // generate config parameters
        for ( k = 0; k < nVarsLut; k++ )
            fprintf( pFile, " p%02d=pl%02d", k, i * nVarsLut + k );
        // generate the inputs
        for ( k = 0; k < nLutSize; k++ )
            fprintf( pFile, " i%d=s%02d", k, i * nLutSize + k );
        // generate the output
        fprintf( pFile, " o=v%02d", nVars + i );
        fprintf( pFile, "\n" );
    }

    // generate LUT inputs
    for ( i = 0; i < nLuts; i++ )
    {
        for ( j = 0; j < nLutSize; j++ )
        {
            fprintf( pFile, ".subckt lut%d", nVarsLog );
            // generate config parameters
            for ( k = 0; k < nVarsDeg; k++ )
            {
                if ( k < nVars + nLuts - 1 && k < nVars + i )
                    fprintf( pFile, " p%02d=v%02d", k, k );
                else
                    fprintf( pFile, " p%02d=Gnd", k );
            }
            // generate the inputs
            for ( k = 0; k < nVarsLog; k++ )
                fprintf( pFile, " i%d=pv%02d", k, (i * nLutSize + j) * nVarsLog + k );
            // generate the output
            fprintf( pFile, " o=s%02d", i * nLutSize + j );
            fprintf( pFile, "\n" );
        }
    }

    fprintf( pFile, ".end\n" ); 
    fprintf( pFile, "\n" ); 

    // generate LUTs
    Abc_WriteKLut( pFile, nLutSize );
    if ( nVarsLog != nLutSize )
        Abc_WriteKLut( pFile, nVarsLog );
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    [Generates structure of L K-LUTs implementing an N-var function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_GenOneHot( char * pFileName, int nVars )
{
    FILE * pFile;
    int i, k, Counter, nDigitsIn, nDigitsOut;
    pFile = fopen( pFileName, "w" );
    fprintf( pFile, "# One-hotness condition for %d vars generated by ABC on %s\n", nVars, Extra_TimeStamp() );
    fprintf( pFile, ".model 1hot_%dvars\n", nVars );
    fprintf( pFile, ".inputs" );
    nDigitsIn = Abc_Base10Log( nVars );
    for ( i = 0; i < nVars; i++ )
        fprintf( pFile, " i%0*d", nDigitsIn, i );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".outputs" );
    nDigitsOut = Abc_Base10Log( nVars * (nVars - 1) / 2 );
    for ( i = 0; i < nVars * (nVars - 1) / 2; i++ )
        fprintf( pFile, " o%0*d", nDigitsOut, i );
    fprintf( pFile, "\n" );
    Counter = 0;
    for ( i = 0; i < nVars; i++ )
        for ( k = i+1; k < nVars; k++ )
        {
            fprintf( pFile, ".names i%0*d i%0*d o%0*d\n", nDigitsIn, i, nDigitsIn, k, nDigitsOut, Counter ); 
            fprintf( pFile, "11 0\n" ); 
            Counter++;
        }
    fprintf( pFile, ".end\n" ); 
    fprintf( pFile, "\n" ); 
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    [Generates structure of L K-LUTs implementing an N-var function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_GenOneHotIntervals( char * pFileName, int nPis, int nRegs, Vec_Ptr_t * vOnehots )
{
    Vec_Int_t * vLine;
    FILE * pFile;
    int i, j, k, iReg1, iReg2, Counter, Counter2, nDigitsIn, nDigitsOut;
    pFile = fopen( pFileName, "w" );
    fprintf( pFile, "# One-hotness with %d vars and %d regs generated by ABC on %s\n", nPis, nRegs, Extra_TimeStamp() );
    fprintf( pFile, "# Used %d intervals of 1-hot registers: { ", Vec_PtrSize(vOnehots) );
    Counter = 0;
    Vec_PtrForEachEntry( Vec_Int_t *, vOnehots, vLine, k )
    {
        fprintf( pFile, "%d ", Vec_IntSize(vLine) );
        Counter += Vec_IntSize(vLine) * (Vec_IntSize(vLine) - 1) / 2;
    }
    fprintf( pFile, "}\n" );
    fprintf( pFile, ".model 1hot_%dvars_%dregs\n", nPis, nRegs );
    fprintf( pFile, ".inputs" );
    nDigitsIn = Abc_Base10Log( nPis+nRegs );
    for ( i = 0; i < nPis+nRegs; i++ )
        fprintf( pFile, " i%0*d", nDigitsIn, i );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".outputs" );
    nDigitsOut = Abc_Base10Log( Counter );
    for ( i = 0; i < Counter; i++ )
        fprintf( pFile, " o%0*d", nDigitsOut, i );
    fprintf( pFile, "\n" );
    Counter2 = 0;
    Vec_PtrForEachEntry( Vec_Int_t *, vOnehots, vLine, k )
    {
        Vec_IntForEachEntry( vLine, iReg1, i )
        Vec_IntForEachEntryStart( vLine, iReg2, j, i+1 )
        {
            fprintf( pFile, ".names i%0*d i%0*d o%0*d\n", nDigitsIn, nPis+iReg1, nDigitsIn, nPis+iReg2, nDigitsOut, Counter2 ); 
            fprintf( pFile, "11 0\n" ); 
            Counter2++;
        }
    }
    assert( Counter == Counter2 );
    fprintf( pFile, ".end\n" ); 
    fprintf( pFile, "\n" ); 
    fclose( pFile );
}

ABC_NAMESPACE_IMPL_END

#include "aig/aig/aig.h"

ABC_NAMESPACE_IMPL_START


/**Function*************************************************************

  Synopsis    [Generates structure of L K-LUTs implementing an N-var function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_GenRandom( char * pFileName, int nPis )
{
    FILE * pFile;
    unsigned * pTruth;
    int i, b, w, nWords = Abc_TruthWordNum( nPis );
    int nDigitsIn;
    //Aig_ManRandom( 1 );
    pTruth = ABC_ALLOC( unsigned, nWords );
    for ( w = 0; w < nWords; w++ )
        pTruth[w] = Aig_ManRandom( 0 );
    pFile = fopen( pFileName, "w" );
    fprintf( pFile, "# Random function with %d inputs generated by ABC on %s\n", nPis, Extra_TimeStamp() );
    fprintf( pFile, ".model rand%d\n", nPis );
    fprintf( pFile, ".inputs" );
    nDigitsIn = Abc_Base10Log( nPis );
    for ( i = 0; i < nPis; i++ )
        fprintf( pFile, " i%0*d", nDigitsIn, i );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".outputs f\n" );
    fprintf( pFile, ".names" );
    nDigitsIn = Abc_Base10Log( nPis );
    for ( i = 0; i < nPis; i++ )
        fprintf( pFile, " i%0*d", nDigitsIn, i );
    fprintf( pFile, " f\n" );
    for ( i = 0; i < (1<<nPis); i++ )
        if ( Abc_InfoHasBit(pTruth, i) )
        {
            for ( b = nPis-1; b >= 0; b-- )
                fprintf( pFile, "%d", (i>>b)&1 );
            fprintf( pFile, " 1\n" );
        }
    fprintf( pFile, ".end\n" ); 
    fprintf( pFile, "\n" ); 
    fclose( pFile );
    ABC_FREE( pTruth );
}


/**Function*************************************************************

  Synopsis    [Generates structure of L K-LUTs implementing an N-var function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_GenFsmCond( Vec_Str_t * vCond, int nPis, int Prob )
{
    int i, Rand;
    Vec_StrClear( vCond );
    for ( i = 0; i < nPis; i++ )
    {
        Rand = Aig_ManRandom( 0 );
        if ( Rand % 100 > Prob )
            Vec_StrPush( vCond, '-' );
        else if ( Rand & 1 )
            Vec_StrPush( vCond, '1' );
        else
            Vec_StrPush( vCond, '0' );
    }
    Vec_StrPush( vCond, '\0' );
}
void Abc_GenFsm( char * pFileName, int nPis, int nPos, int nStates, int nLines, int ProbI, int ProbO )
{
    FILE * pFile;
    Vec_Wrd_t * vStates;
    Vec_Str_t * vCond;
    int i, iState, iState2;
    int nDigits = Abc_Base10Log( nStates );
    Aig_ManRandom( 1 );
    vStates = Vec_WrdAlloc( nLines );
    vCond = Vec_StrAlloc( 1000 );
    for ( i = 0; i < nStates; )
    {
        iState = Aig_ManRandom( 0 ) % nStates;
        if ( iState == i )
            continue;
        Vec_WrdPush( vStates, ((word)i << 32) | iState );
        i++;
    }
    for (      ; i < nLines; )
    {
        iState = Aig_ManRandom( 0 ) % nStates;
        iState2 = Aig_ManRandom( 0 ) % nStates;
        if ( iState2 == iState )
            continue;
        Vec_WrdPush( vStates, ((word)iState << 32) | iState2 );
        i++;
    }
    Vec_WrdSort( vStates, 0 );
    // write the file
    pFile = fopen( pFileName, "w" );
    fprintf( pFile, "# This random FSM was generated by ABC on %s\n", Extra_TimeStamp() );
    fprintf( pFile, "# Command line was: \"genfsm -I %d -O %d -S %d -L %d -P %d -Q %d %s\"\n", nPis, nPos, nStates, nLines, ProbI, ProbO, pFileName );
    fprintf( pFile, "# FSM has %d inputs, %d outputs, %d states, and %d products\n", nPis, nPos, nStates, nLines );
    fprintf( pFile, ".i %d\n", nPis );
    fprintf( pFile, ".o %d\n", nPos );
    fprintf( pFile, ".p %d\n", nLines );
    fprintf( pFile, ".s %d\n", nStates );
    for ( i = 0; i < nLines; i++ )
    {
        Abc_GenFsmCond( vCond, nPis, ProbI );
        fprintf( pFile, "%s ", Vec_StrArray(vCond) );
        fprintf( pFile, "%0*d ", nDigits, (int)(Vec_WrdEntry(vStates, i) >> 32) );
        fprintf( pFile, "%0*d ", nDigits, (int)(Vec_WrdEntry(vStates, i)) );
        if ( nPos > 0 )
        {
            Abc_GenFsmCond( vCond, nPos, ProbO );
            fprintf( pFile, "%s", Vec_StrArray(vCond) );
        }
        fprintf( pFile, "\n" );
    }
    fprintf( pFile, ".e" );
    fprintf( pFile, "\n" );
    fclose( pFile );
    Vec_WrdFree( vStates );
    Vec_StrFree( vCond );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_AdderTree( FILE * pFile, int nArgs, int nBits )
{
    int i, k, nDigits = Abc_Base10Log( nBits ), Log2 = Abc_Base2Log( nArgs );
    assert( nArgs > 1 && nBits > 1 );
    fprintf( pFile, "module adder_tree_%d_%d (\n   ", nArgs, nBits );
    for ( i = 0; i < nBits; i++, fprintf(pFile, "\n   ") )
        for ( k = 0; k < nArgs; k++ )
            fprintf( pFile, " i%0*d_%0*d,", nDigits, k, nDigits, nBits-1-i );
    fprintf( pFile, " z\n" );
    fprintf( pFile, "  );\n" );
    for ( i = 0; i < nBits; i++ )
    {
        fprintf( pFile, "  input" );
        for ( k = 0; k < nArgs; k++ )
            fprintf( pFile, " i%0*d_%0*d%s", nDigits, k, nDigits, nBits-1-i, k==nArgs-1 ? "":"," );
        fprintf( pFile, ";\n" );
    }
    fprintf( pFile, "  output [%d:0] z;\n", nBits+Log2-1 );
    for ( i = 0; i < nArgs; i++ )
    {
        fprintf( pFile, "  wire [%d:0] t%d = {", nBits-1, i );
        for ( k = 0; k < nBits; k++ )
            fprintf( pFile, " i%0*d_%0*d%s", nDigits, i, nDigits, nBits-1-k, k==nBits-1 ? "":"," );
        fprintf( pFile, " };\n" );
    }
    for ( i = 0; i < nArgs-1; i++ )
        fprintf( pFile, "  wire [%d:0] s%d = t%d + %s%d;\n", nBits+Log2-1, i+1, i+1, i ? "s":"t", i );
    fprintf( pFile, "  assign z = s%d;\n", nArgs-1 );
    fprintf( pFile, "endmodule\n\n" ); 
}
void Abc_GenAdderTree( char * pFileName, int nArgs, int nBits )
{
    FILE * pFile = fopen( pFileName, "w" );
    fprintf( pFile, "// %d-argument %d-bit adder-tree generated by ABC on %s\n", nArgs, nBits, Extra_TimeStamp() );
    Abc_AdderTree( pFile, nArgs, nBits );
    fclose( pFile );
}


/**Function*************************************************************

  Synopsis    [Generating signed Booth multiplier.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_GenSignedBoothPP( Gia_Man_t * p, int a, int b, int c, int d, int e )
{
/*
    abc>  lutexact -I 5 -N 7 -g F335ACC0

    05 = 4'b0110( d e )
    06 = 4'b0110( c d )
    07 = 4'b0100( a 06 )
    08 = 4'b1000( b 06 )
    09 = 4'b0100( 05 07 )
    10 = 4'b0110( 08 09 )
    11 = 4'b0110( d 10 )
*/
    int n05 = Gia_ManHashXor( p, d,   e   );
    int n06 = Gia_ManHashXor( p, c,   d   );
    int n07 = Gia_ManHashAnd( p, a,   Abc_LitNot(n06) );
    int n08 = Gia_ManHashAnd( p, b,   n06 );
    int n09 = Gia_ManHashAnd( p, n05, Abc_LitNot(n07) );
    int n10 = Gia_ManHashXor( p, n08, n09 );
    int n11 = Gia_ManHashXor( p, d,   n10 );
    return n11;
}
Gia_Man_t * Abc_GenSignedBoothPPTest( int nArgA, int nArgB )
{
    Gia_Man_t * pNew; int i, iLit;
    pNew = Gia_ManStart( 1000 );
    pNew->pName = Abc_UtilStrsav( "booth" );
    for ( i = 0; i < 5; i++ )
         Gia_ManAppendCi(pNew);
    iLit = Abc_GenSignedBoothPP( pNew, 2, 4, 6, 8, 10 );
    Gia_ManAppendCo(pNew, iLit);
    return pNew;
}

/*
// parametrized implementation of signed Booth multiplier
module booth #(
     parameter N = 4      // bit-width of input a 
    ,parameter M = 4      // bit-width of input b
)(
     input  [N-1:0]   a    // input data
    ,input  [M-1:0]   b    // input data
    ,output [N+M-1:0] z    // output data
);

    localparam TT = 32'hF335ACC0;
    localparam W  = N+M+1;
    localparam L  =  (M+1)/2;

    wire [W-1:0] data1[L:0];
    wire [W-1:0] data2[L:0];

    assign data2[0] = data1[0];
    assign z = data2[L][N+M-1:0];

    wire [N+1:0] a2 = {a[N-1], a, 1'b0};
    wire [M+1:0] b2 = {b[M-1], b, 1'b0};

    genvar j;
    generate
        for ( j = 0; j < W; j = j + 1 ) begin : J
            assign data1[0][j] = (j%2 == 0 && j/2 < L) ? b2[j+2] : 1'b0;
        end
    endgenerate

    genvar k, i0, i1, i2;
    generate
        for ( k = 0; k < 2*L; k = k + 2 ) begin : K

            for ( i0 = 0; i0 < k; i0 = i0 + 1 ) begin : I0
                assign data1[k/2+1][i0] = 1'b0;
            end

            for ( i1 = 0; i1 <= N;  i1 = i1 + 1 ) begin : I1
                wire [4:0] in = {b2[k+2], b2[k+1], b2[k], a2[i1+1], a2[i1]};
                assign data1[k/2+1][k+i1] = (k > 0 && i1 == N) ? ~TT[in] : TT[in];
            end

            assign data1[k/2+1][k+N+1]  = k > 0 ? 1'b1 : data1[k/2+1][k+N];
            for ( i2 = k+N+2; i2 < W; i2 = i2 + 1 ) begin : I2
                assign data1[k/2+1][i2] = (k > 0 || i2 > k+N+2)? 1'b0 : ~data1[k/2+1][k+N];
            end

            assign data2[k/2+1] = data2[k/2] + data1[k/2+1];

        end
    endgenerate

endmodule
*/

Gia_Man_t * Abc_GenSignedBooth( int nArgN, int nArgM )
{
    int nWidth = nArgN + nArgM + 1;
    int Length = (nArgM + 1) / 2;
    int i, k, iLit;

    Vec_Int_t * vPPs  = Vec_IntAlloc( nWidth * (Length + 1) );
    Vec_Int_t * vArgN = Vec_IntAlloc( nArgN + 2 );
    Vec_Int_t * vArgM = Vec_IntAlloc( nArgM + 2 );
    int * pArgN = Vec_IntArray( vArgN );
    int * pArgM = Vec_IntArray( vArgM );

    Gia_Man_t * pTemp, * pNew; 
    pNew = Gia_ManStart( 1000 );
    pNew->pName = Abc_UtilStrsav( "booth" );

    Vec_IntPush( vArgN, 0 );
    for ( i = 0; i < nArgN; i++ )
        Vec_IntPush( vArgN, Gia_ManAppendCi(pNew) );
    Vec_IntPush( vArgN, Vec_IntEntryLast(vArgN) );

    Vec_IntPush( vArgM, 0 );
    for ( i = 0; i < nArgM; i++ )
        Vec_IntPush( vArgM, Gia_ManAppendCi(pNew) );
    Vec_IntPush( vArgM, Vec_IntEntryLast(vArgM) );

    for ( i = 0; i < nWidth; i++ )
        Vec_IntPush( vPPs, (i%2 == 0 && i/2 < Length) ? pArgM[i+2] : 0 );

    Gia_ManHashAlloc( pNew );
    for ( k = 0; k < 2*Length; k += 2 )
    {
        for ( i = 0; i < k; i++ )
            Vec_IntPush( vPPs, 0 );
        for ( i = 0; i <= nArgN; i++ )
        {
            iLit = Abc_GenSignedBoothPP( pNew, pArgN[i], pArgN[i+1], pArgM[k], pArgM[k+1], pArgM[k+2] );
            Vec_IntPush( vPPs, Abc_LitNotCond( iLit, k > 0 && i == nArgN ) );
        }
        iLit = Vec_IntEntryLast(vPPs);
        Vec_IntPush( vPPs, k > 0 ? 1 : iLit );
        for ( i = k+nArgN+2; i < nWidth; i++ )
            Vec_IntPush( vPPs, (k > 0 || i > k+nArgN+2) ? 0 : Abc_LitNot(iLit) );
    }
    Gia_ManHashStop( pNew );

    for ( k = 0; k <= Length; k++ )
        for ( i = 0; i < nArgN+nArgM; i++ )
            Gia_ManAppendCo( pNew, Vec_IntEntry(vPPs, k*(nArgN+nArgM+1) + i) );
    Vec_IntFree( vPPs );
    Vec_IntFree( vArgN );
    Vec_IntFree( vArgM );

    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}
Mini_Aig_t * Abc_GenSignedBoothMini( int nArgN, int nArgM )
{
    extern Mini_Aig_t * Gia_ManToMiniAig( Gia_Man_t * pGia );
    Gia_Man_t * pGia = Abc_GenSignedBooth( nArgN, nArgM );
    Mini_Aig_t * pMini = Gia_ManToMiniAig( pGia );
    Gia_ManStop( pGia );
    return pMini;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_WriteBoothPartialProducts( FILE * pFile, int nVars )
{
    Mini_Aig_t * p = Abc_GenSignedBoothMini( nVars, nVars );
    int i, nNodes = Mini_AigNodeNum(p);
    int nDigits   = Abc_Base10Log( nVars );
    int nDigits2  = Abc_Base10Log( 2*nVars );
    int nDigits3  = Abc_Base10Log( nNodes );
    int nOut = 0;
    fprintf( pFile, ".names pp%0*d\n", nDigits3, 0 );
    for ( i = 1; i < nNodes; i++ )
    {
        if ( Mini_AigNodeIsPi( p, i ) )
        {
            if ( i > 0 && i <= nVars )
                fprintf( pFile, ".names a%0*d pp%0*d\n1 1\n", nDigits, i-1,       nDigits3, i );
            else if ( i > nVars && i <= 2*nVars )
                fprintf( pFile, ".names b%0*d pp%0*d\n1 1\n", nDigits, i-1-nVars, nDigits3, i );
            else assert( 0 );
        }
        else if ( Mini_AigNodeIsPo( p, i ) )
        {
            int Lit = Mini_AigNodeFanin0( p, i );
            fprintf( pFile, ".names pp%0*d y%0*d_%0*d\n%d 1\n", nDigits3, Abc_Lit2Var(Lit), nDigits, nOut/(2*nVars), nDigits2, nOut%(2*nVars), !Abc_LitIsCompl(Lit) );
            nOut++;
        }
        else if ( Mini_AigNodeIsAnd( p, i ) )
        {
            int Lit0 = Mini_AigNodeFanin0( p, i );
            int Lit1 = Mini_AigNodeFanin1( p, i );
            fprintf( pFile, ".names pp%0*d pp%0*d pp%0*d\n%d%d 1\n", 
                nDigits3, Abc_Lit2Var(Lit0), nDigits3, Abc_Lit2Var(Lit1), nDigits3, i, !Abc_LitIsCompl(Lit0), !Abc_LitIsCompl(Lit1) );
        }
        else assert( 0 );
    }
    Mini_AigStop( p );
}
void Abc_WriteBooth( FILE * pFile, int nVars )
{
    int i, k, nDigits = Abc_Base10Log( nVars ), nDigits2 = Abc_Base10Log( 2*nVars );
    int Length = 1+(nVars + 1)/2;

    assert( nVars > 0 );
    fprintf( pFile, ".model Multi%d\n", nVars );

    fprintf( pFile, ".inputs" );
    for ( i = 0; i < nVars; i++ )
        fprintf( pFile, " a%0*d", nDigits, i );
    for ( i = 0; i < nVars; i++ )
        fprintf( pFile, " b%0*d", nDigits, i );
    fprintf( pFile, "\n" );

    fprintf( pFile, ".outputs" );
    for ( i = 0; i < 2*nVars; i++ )
        fprintf( pFile, " m%0*d", nDigits2, i );
    fprintf( pFile, "\n" );

    Abc_WriteBoothPartialProducts( pFile, nVars );

    for ( i = 0; i < 2*nVars; i++ )
        fprintf( pFile, ".names x%0*d_%0*d\n", nDigits, 0, nDigits2, i );
    for ( k = 0; k < Length; k++ )
    {
        fprintf( pFile, ".subckt ADD%d", 2*nVars );
        for ( i = 0; i < 2*nVars; i++ )
            fprintf( pFile, " a%0*d=x%0*d_%0*d", nDigits2, i, nDigits, k, nDigits2, i );
        for ( i = 0; i < 2*nVars; i++ )
            fprintf( pFile, " b%0*d=y%0*d_%0*d", nDigits2, i, nDigits, k, nDigits2, i );
        for ( i = 0; i <= 2*nVars; i++ )
            fprintf( pFile, " s%0*d=x%0*d_%0*d", nDigits2, i, nDigits, k+1, nDigits2, i );
        fprintf( pFile, "\n" );
    }
    for ( i = 0; i < 2 * nVars; i++ )
        fprintf( pFile, ".names x%0*d_%0*d m%0*d\n1 1\n", nDigits, k, nDigits2, i, nDigits2, i );
    fprintf( pFile, ".end\n" ); 
    fprintf( pFile, "\n" );
    Abc_WriteAdder( pFile, 2*nVars );
}
void Abc_GenBooth( char * pFileName, int nVars )
{
    FILE * pFile;
    assert( nVars > 0 );
    pFile = fopen( pFileName, "w" );
    fprintf( pFile, "# %d-bit signed Booth multiplier generated by ABC on %s\n", nVars, Extra_TimeStamp() );
    Abc_WriteBooth( pFile, nVars );
    fclose( pFile );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_GenGraph( char * pFileName, int nPis )
{
    FILE * pFile;
    int i, a, b, w, nDigitsIn, nWords = Abc_TruthWordNum( nPis*(nPis-1)/2 );
    unsigned * pTruth = ABC_CALLOC( unsigned, nWords );
    unsigned char M[10][10] = {{0}}, C[100][2] = {{0}}, nVars = 0;
    assert( nPis <= 8 );
    for ( a = 0;   a < nPis; a++ )
    for ( b = a+1; b < nPis; b++ )
        C[nVars][0] = a, C[nVars][1] = b, nVars++;
    for ( i = 0; i < (1<<nVars); i++ )
    {
        int fChanges = 1;
        for ( w = 0; w < nVars; w++ )
            M[C[w][0]][C[w][1]] = M[C[w][1]][C[w][0]] = (i >> w) & 1;
        while ( fChanges && !M[0][1] ) {
            fChanges = 0;
            for ( a = 0; a < nPis; a++ )
            for ( b = 0; b < nPis; b++ )
                if ( M[a][b] )
                    for ( w = 0; w < nPis; w++ )
                        if ( M[b][w] && !M[a][w] )
                            M[a][w] = 1, fChanges = 1;
        }
        if ( M[0][1] )
            Abc_InfoSetBit(pTruth, i);    
    }
    pFile = fopen( pFileName, "w" );
    fprintf( pFile, "# Function with %d inputs generated by ABC on %s\n", nVars, Extra_TimeStamp() );
    fprintf( pFile, ".model fun%d\n", nVars );
    fprintf( pFile, ".inputs" );
    nDigitsIn = Abc_Base10Log( nVars );
    for ( i = 0; i < nVars; i++ )
        fprintf( pFile, " i%0*d", nDigitsIn, i );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".outputs f\n" );
    fprintf( pFile, ".names" );
    nDigitsIn = Abc_Base10Log( nVars );
    for ( b = nVars-1; b >= 0; b-- )
        fprintf( pFile, " i%0*d", nDigitsIn, b );
    fprintf( pFile, " f\n" );
    for ( i = 0; i < (1<<nVars); i++ )
        if ( Abc_InfoHasBit(pTruth, i) )
        {
            for ( b = nVars-1; b >= 0; b-- )
                fprintf( pFile, "%d", (i>>b)&1 );
            fprintf( pFile, " 1\n" );
        }
    fprintf( pFile, ".end\n" ); 
    fprintf( pFile, "\n" ); 
    fclose( pFile );
    ABC_FREE( pTruth );
}

/**Function*************************************************************

  Synopsis    [Threshold function generation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_GenComp63a4( FILE * pFile )
{
    fprintf( pFile, ".model C63a\n" );
    fprintf( pFile, ".inputs x0 x1 x2 x3 x4 x5\n" );
    fprintf( pFile, ".outputs z0 z1 z2\n" );
    fprintf( pFile, ".names x1 x2 x3 x0 n10\n" );
    fprintf( pFile, "--00 1\n" );
    fprintf( pFile, "-0-0 1\n" );
    fprintf( pFile, "0--0 1\n" );
    fprintf( pFile, "000- 1\n" );
    fprintf( pFile, ".names x4 x5 n13 n10 z0\n" );
    fprintf( pFile, "--00 1\n" );
    fprintf( pFile, "-1-0 1\n" );
    fprintf( pFile, "1--0 1\n" );
    fprintf( pFile, "111- 1\n" );
    fprintf( pFile, ".names x1 x2 x3 x0 n13\n" );
    fprintf( pFile, "-110 1\n" );
    fprintf( pFile, "1-10 1\n" );
    fprintf( pFile, "11-0 1\n" );
    fprintf( pFile, "-001 1\n" );
    fprintf( pFile, "0-01 1\n" );
    fprintf( pFile, "00-1 1\n" );
    fprintf( pFile, ".names x4 x5 n13 n16 z1\n" );
    fprintf( pFile, "1-00 1\n" );
    fprintf( pFile, "0-10 1\n" );
    fprintf( pFile, "-101 1\n" );
    fprintf( pFile, "-011 1\n" );
    fprintf( pFile, ".names x1 x2 x3 x4 n16\n" );
    fprintf( pFile, "1000 1\n" );
    fprintf( pFile, "0100 1\n" );
    fprintf( pFile, "0010 1\n" );
    fprintf( pFile, "1110 1\n" );
    fprintf( pFile, "0001 1\n" );
    fprintf( pFile, "1101 1\n" );
    fprintf( pFile, "1011 1\n" );
    fprintf( pFile, "0111 1\n" );
    fprintf( pFile, ".names x5 n16 z2\n" );
    fprintf( pFile, "10 1\n" );
    fprintf( pFile, "01 1\n" );
    fprintf( pFile, ".end\n\n" );
}
void Abc_GenComp63a6( FILE * pFile )
{
    fprintf( pFile, ".model C63a\n" );
    fprintf( pFile, ".inputs x0 x1 x2 x3 x4 x5\n" );
    fprintf( pFile, ".outputs z0 z1 z2\n" );
    fprintf( pFile, ".names x1 x2 x3 x4 x5 x0 z0\n" );
    fprintf( pFile, "---111 1\n" );
    fprintf( pFile, "--1-11 1\n" );
    fprintf( pFile, "--11-1 1\n" );
    fprintf( pFile, "-1--11 1\n" );
    fprintf( pFile, "-1-1-1 1\n" );
    fprintf( pFile, "-11--1 1\n" );
    fprintf( pFile, "-1111- 1\n" );
    fprintf( pFile, "1---11 1\n" );
    fprintf( pFile, "1--1-1 1\n" );
    fprintf( pFile, "1-1--1 1\n" );
    fprintf( pFile, "1-111- 1\n" );
    fprintf( pFile, "11---1 1\n" );
    fprintf( pFile, "11-11- 1\n" );
    fprintf( pFile, "111-1- 1\n" );
    fprintf( pFile, "1111-- 1\n" );
    fprintf( pFile, ".names x1 x2 x3 x4 x5 x0 z1\n" );
    fprintf( pFile, "-00001 1\n" );
    fprintf( pFile, "-00110 1\n" );
    fprintf( pFile, "-01010 1\n" );
    fprintf( pFile, "-01100 1\n" );
    fprintf( pFile, "-10010 1\n" );
    fprintf( pFile, "-10100 1\n" );
    fprintf( pFile, "-11000 1\n" );
    fprintf( pFile, "-11111 1\n" );
    fprintf( pFile, "0-0001 1\n" );
    fprintf( pFile, "0-0110 1\n" );
    fprintf( pFile, "0-1010 1\n" );
    fprintf( pFile, "0-1100 1\n" );
    fprintf( pFile, "00-001 1\n" );
    fprintf( pFile, "00-110 1\n" );
    fprintf( pFile, "000-01 1\n" );
    fprintf( pFile, "0000-1 1\n" );
    fprintf( pFile, "1-0010 1\n" );
    fprintf( pFile, "1-0100 1\n" );
    fprintf( pFile, "1-1000 1\n" );
    fprintf( pFile, "1-1111 1\n" );
    fprintf( pFile, "11-000 1\n" );
    fprintf( pFile, "11-111 1\n" );
    fprintf( pFile, "111-11 1\n" );
    fprintf( pFile, "1111-1 1\n" );
    fprintf( pFile, ".names x1 x2 x3 x4 x5 z2\n" );
    fprintf( pFile, "00001 1\n" );
    fprintf( pFile, "00010 1\n" );
    fprintf( pFile, "00100 1\n" );
    fprintf( pFile, "00111 1\n" );
    fprintf( pFile, "01000 1\n" );
    fprintf( pFile, "01011 1\n" );
    fprintf( pFile, "01101 1\n" );
    fprintf( pFile, "01110 1\n" );
    fprintf( pFile, "10000 1\n" );
    fprintf( pFile, "10011 1\n" );
    fprintf( pFile, "10101 1\n" );
    fprintf( pFile, "10110 1\n" );
    fprintf( pFile, "11001 1\n" );
    fprintf( pFile, "11010 1\n" );
    fprintf( pFile, "11100 1\n" );
    fprintf( pFile, "11111 1\n" );
    fprintf( pFile, ".end\n\n" );
}
void Abc_GenAdder4( FILE * pFile, int nBits, int nLutSize )
{
    int i, n;
    fprintf( pFile, ".model A%02d_4x\n", nBits );
    for ( n = 0; n < 4; n++ ) {
        fprintf( pFile, ".inputs" );
        for ( i = 0; i < nBits; i++ )
            fprintf( pFile, " %c%02d", 'a'+n, i );
        fprintf( pFile, "\n" );
    }
    fprintf( pFile, ".outputs" );
    for ( i = 0; i < nBits; i++ )
        fprintf( pFile, " s%02d", i );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".names v00\n" );
    fprintf( pFile, ".names w00\n" );
    for ( i = 0; i < nBits; i++ ) {
        fprintf( pFile, ".subckt C63a" );
        fprintf( pFile, " x0=w%02d", i );
        fprintf( pFile, " x1=v%02d", i );
        fprintf( pFile, " x2=a%02d", i );
        fprintf( pFile, " x3=b%02d", i );
        fprintf( pFile, " x4=c%02d", i );
        fprintf( pFile, " x5=d%02d", i );
        fprintf( pFile, " z0=w%02d", i+1 );
        fprintf( pFile, " z1=v%02d", i+1 );
        fprintf( pFile, " z2=s%02d", i );
        fprintf( pFile, "\n" );
    }
    fprintf( pFile, ".end\n\n" );
    if ( nLutSize == 4 )
        Abc_GenComp63a4( pFile );
    else if ( nLutSize == 6 )
        Abc_GenComp63a6( pFile );
    else assert( 0 );
}
void Abc_WriteAdder2( FILE * pFile, int nVars )
{
    int i;
    assert( nVars > 0 );
    fprintf( pFile, ".model A%02d\n", nVars );
    fprintf( pFile, ".inputs c\n" );
    fprintf( pFile, ".inputs" );
    for ( i = 0; i < nVars; i++ )
        fprintf( pFile, " a%02d", i );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".inputs" );
    for ( i = 0; i < nVars; i++ )
        fprintf( pFile, " b%02d", i );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".outputs" );
    for ( i = 0; i <= nVars; i++ )
        fprintf( pFile, " s%02d", i );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".names c t00\n1 1\n" );
    for ( i = 0; i < nVars; i++ )
        fprintf( pFile, ".subckt FA a=a%02d b=b%02d cin=t%02d s=s%02d cout=t%02d\n", i, i, i, i, i+1 );
    fprintf( pFile, ".names t%02d s%02d\n1 1\n", nVars, nVars );
    fprintf( pFile, ".end\n\n" ); 
    Abc_WriteFullAdder( pFile );
}
void Abc_GenAdder4test( FILE * pFile, int nBits )
{
    int i, n;
    fprintf( pFile, ".model A%02d_4x\n", nBits );
    for ( n = 0; n < 4; n++ ) {
        fprintf( pFile, ".inputs" );
        for ( i = 0; i < nBits; i++ )
            fprintf( pFile, " %c%02d", 'a'+n, i );
        fprintf( pFile, "\n" );
    }
    fprintf( pFile, ".outputs" );
    for ( i = 0; i < nBits; i++ )
        fprintf( pFile, " o%02d", i );
    fprintf( pFile, "\n" );

    fprintf( pFile, ".names zero\n" );
    fprintf( pFile, ".subckt A%02d c=zero", nBits );
    fprintf( pFile, " \\\n" );
    for ( i = 0; i < nBits; i++ )
        fprintf( pFile, " a%0d=a%02d", i, i );
    fprintf( pFile, " \\\n" );
    for ( i = 0; i < nBits; i++ )
        fprintf( pFile, " b%0d=b%02d", i, i );
    fprintf( pFile, " \\\n" );
    for ( i = 0; i <= nBits; i++ )
        fprintf( pFile, " s%0d=t%02d", i, i );
    fprintf( pFile, "\n" );

    fprintf( pFile, ".subckt A%02d c=zero", nBits );
    fprintf( pFile, " \\\n" );
    for ( i = 0; i < nBits; i++ )
        fprintf( pFile, " a%0d=c%02d", i, i );
    fprintf( pFile, " \\\n" );
    for ( i = 0; i < nBits; i++ )
        fprintf( pFile, " b%0d=t%02d", i, i );
    fprintf( pFile, " \\\n" );
    for ( i = 0; i <= nBits; i++ )
        fprintf( pFile, " s%0d=u%02d", i, i );
    fprintf( pFile, "\n" );

    fprintf( pFile, ".subckt A%02d c=zero", nBits );
    fprintf( pFile, " \\\n" );
    for ( i = 0; i < nBits; i++ )
        fprintf( pFile, " a%0d=d%02d", i, i );
    fprintf( pFile, " \\\n" );
    for ( i = 0; i < nBits; i++ )
        fprintf( pFile, " b%0d=u%02d", i, i );
    fprintf( pFile, " \\\n" );
    for ( i = 0; i <= nBits; i++ )
        fprintf( pFile, " s%0d=o%02d", i, i );
    fprintf( pFile, "\n" );

    fprintf( pFile, ".end\n\n" );
    Abc_WriteAdder( pFile, nBits );
}
void Abc_WriteWeight( FILE * pFile, int Num, int nBits, int Weight )
{
    int i;
    fprintf( pFile, ".model W%02d\n", Num );
    fprintf( pFile, ".inputs i\n" );
    fprintf( pFile, ".outputs" );
    for ( i = 0; i < nBits; i++ )
        fprintf( pFile, " o%02d", i );
    fprintf( pFile, "\n" );
    for ( i = 0; i < nBits; i++ )
        if ( (Weight >> i) & 1 )
            fprintf( pFile, ".names i o%02d\n1 1\n", i );
        else
            fprintf( pFile, ".names o%02d\n", i );
    fprintf( pFile, ".end\n\n" );    
}

Vec_Int_t * Abc_GenTreeFindGroups( char * pTree, int iPos )
{
    Vec_Int_t * vRes = NULL;
    int Counter = 1;
    assert( pTree[iPos] == '(' );
    while ( pTree[++iPos] ) {
        if ( pTree[iPos] == '(' ) {
            if ( Counter++ == 1 ) {
                if ( vRes == NULL )
                    vRes = Vec_IntAlloc( 4 );
                Vec_IntPush( vRes, iPos );
            }
        }
        if ( pTree[iPos] == ')' )
            Counter--;
        if ( Counter == 0 )
            return vRes;
    }
    assert( 0 );
    return NULL;
}
int Abc_GenTree_rec( FILE * pFile, int nBits, char * pTree, int iPos, int * pSig, int * pUsed )
{
    Vec_Int_t * vGroups = Abc_GenTreeFindGroups( pTree, iPos );
    if ( vGroups == NULL )
        return atoi(pTree+iPos+1);
    int i, g, Group;
    Vec_IntForEachEntry( vGroups, Group, g ) {
        Group = Abc_GenTree_rec( pFile, nBits, pTree, Group, pSig, pUsed );
        Vec_IntWriteEntry( vGroups, g, Group );
    }
    if ( Vec_IntSize(vGroups) == 3 )
        Vec_IntPush(vGroups, 0);        
    if ( Vec_IntSize(vGroups) == 4 )
        fprintf( pFile, ".subckt A%02d_4x", nBits ), *pUsed = 1;
    else if ( Vec_IntSize(vGroups) == 2 )
        fprintf( pFile, ".subckt A%02d c=zero", nBits );
    else assert( 0 );
    Vec_IntForEachEntry( vGroups, Group, g ) {
        fprintf( pFile, " \\\n" );
        for ( i = 0; i < nBits; i++ )
            fprintf( pFile, " %c%02d=%02d_%02d", 'a'+g, i, Group, i );
    }
    fprintf( pFile, " \\\n" );
    for ( i = 0; i < nBits; i++ )
        fprintf( pFile, " s%02d=%02d_%02d", i, *pSig, i );
    fprintf( pFile, "\n\n" );
    return (*pSig)++;
}
void Abc_GenThreshAdder( FILE * pFile, int nBits, int A, int B, int S, int fOne )
{
    if ( A > B ) ABC_SWAP( int, A, B ); int i;
    fprintf( pFile, ".subckt A%02d c=%s", nBits, fOne ? "one" : "zero" );
    fprintf( pFile, " \\\n" );
    for ( i = 0; i < nBits; i++ )
        fprintf( pFile, " a%02d=%02d_%02d", i, A, i );
    fprintf( pFile, " \\\n" );
    for ( i = 0; i < nBits; i++ )
        fprintf( pFile, " b%02d=%02d_%02d", i, B, i );
    fprintf( pFile, " \\\n" );
    for ( i = 0; i <= nBits; i++ )
        fprintf( pFile, " s%02d=%02d_%02d", i, S, i );
    fprintf( pFile, "\n" );
}
void Abc_GenThresh( char * pFileName, int nBits, Vec_Int_t * vNums, int nLutSize, char * pArch )
{
    FILE * pFile = fopen( pFileName, "w" );  
    int c, i, k, Temp, iPrev = 1, nNums = 1, nSigs = 1, fUsed = 0;
    fprintf( pFile, "# %d-bit threshold function with %d variables generated by ABC on %s\n", 
        nBits, Vec_IntSize(vNums)-1, Extra_TimeStamp() );
    fprintf( pFile, "# Weights:" );
    Vec_IntForEachEntryStop( vNums, Temp, i, Vec_IntSize(vNums)-1 )
        fprintf( pFile, " %d", Temp );
    fprintf( pFile, "\n# Threshold: %d\n", Vec_IntEntryLast(vNums) );
    fprintf( pFile, ".model TF%d_%d\n", Vec_IntSize(vNums)-1, nBits );
    fprintf( pFile, ".inputs" );
    for ( i = 0; i < Vec_IntSize(vNums)-1; i++ )
        fprintf( pFile, " x%02d", i );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".outputs F\n" );
    for ( i = 0; i < nBits; i++ )
        fprintf( pFile, ".names %02d_%02d\n", 0, i );
    fprintf( pFile, ".names zero\n" );
    fprintf( pFile, ".names one\n 1\n" );
    Vec_IntForEachEntry( vNums, Temp, k ) {
        fprintf( pFile, ".subckt W%02d", k );
        if ( k < Vec_IntSize(vNums)-1 )
            fprintf( pFile, " i=x%02d", k );
        else
            fprintf( pFile, " i=one" );
        for ( i = 0; i < nBits; i++ )
            fprintf( pFile, " o%02d=%02d_%02d", i, nSigs, i );
        fprintf( pFile, "\n" );
        nSigs++;
    }
    fprintf( pFile, "\n" );
    if ( pArch == NULL )
    {
        Vec_IntForEachEntryStart( vNums, Temp, k, 1 ) {
            Abc_GenThreshAdder( pFile, nBits, iPrev, k+1, nSigs, k == Vec_IntSize(vNums)-1 );
            iPrev = nSigs++;
        }
        fprintf( pFile, ".names %02d_%02d F\n0 1\n", iPrev, nBits-1 );
    }
    else
    {
        Vec_Str_t * vArch = Vec_StrAlloc( 100 );
        for ( c = 0; c < strlen(pArch); c++ ) {
            if ( pArch[c] == '(' || pArch[c] == ')' ) {
                Vec_StrPush( vArch, pArch[c] );
                continue;
            }
            Temp = pArch[c] >= '0' && pArch[c] <= '9' ? pArch[c] - '0' : pArch[c] - 'A' + 10;
            assert( Temp > 0 );
            if ( Temp == 1 ) {
                if ( nNums + Temp == Vec_IntSize(vNums) )
                    Abc_GenThreshAdder( pFile, nBits, nNums, nNums+1, iPrev = nSigs++, 1 );
                else
                    iPrev = nNums++;
            }
            else {
                int kLast = 0;
                assert( nNums + Temp <= Vec_IntSize(vNums) );
                if ( nNums + Temp == Vec_IntSize(vNums) )
                    kLast = Temp++;
                iPrev = nNums++;                
                for ( k = 1; k < Temp; k++ ) {
                    Abc_GenThreshAdder( pFile, nBits, iPrev, nNums++, nSigs, k == kLast );
                    iPrev = nSigs++;
                }
                fprintf( pFile, "\n" );                
            }
            Vec_StrPrintF( vArch, "(%d)", iPrev );
        }
        Vec_StrPush( vArch, '\0' );    
        Temp = Abc_GenTree_rec( pFile, nBits, Vec_StrArray(vArch), 0, &nSigs, &fUsed );
        fprintf( pFile, ".names %02d_%02d F\n0 1\n", Temp, nBits-1 );
        Vec_StrFree( vArch );
    }
    fprintf( pFile, ".end\n\n" ); 
    Vec_IntForEachEntry( vNums, Temp, k )
        Abc_WriteWeight( pFile, k, nBits, k == Vec_IntSize(vNums)-1 ? ~Temp : Temp );
    Abc_WriteAdder2( pFile, nBits );
    if ( fUsed )
        Abc_GenAdder4( pFile, nBits, nLutSize == 4 ? 4 : 6 );
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    [Adder tree generation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

// Based on the paper: E. Demenkov, A. Kojevnikov, A. Kulikov, and G. Yaroslavtsev,
// "New upper bounds on the Boolean circuit complexity of symmetric functions".
// Information Processing Letters, Vol 110(7), March 2010, Pages 264-267.
// https://grigory.us/files/publications/2010_upper_bounds_symmetric_ipl.pdf

void Abc_WriteMDFA( FILE * pFile )
{
    fprintf( pFile, ".model MDFA\n" );
    fprintf( pFile, ".inputs z x1 y1 x2 y2\n" );
    fprintf( pFile, ".outputs s c1 c2\n" );
    fprintf( pFile, ".names x1 z g1\n" );
    fprintf( pFile, "10 1\n" );
    fprintf( pFile, "01 1\n" );
    fprintf( pFile, ".names y1 g1 g2\n" );
    fprintf( pFile, "00 0\n" );
    fprintf( pFile, ".names y1 z g3\n" );
    fprintf( pFile, "10 1\n" );
    fprintf( pFile, "01 1\n" );
    fprintf( pFile, ".names g2 g3 g4\n" );
    fprintf( pFile, "10 1\n" );
    fprintf( pFile, "01 1\n" );
    fprintf( pFile, ".names x2 g3 g5\n" );
    fprintf( pFile, "10 1\n" );
    fprintf( pFile, "01 1\n" );
    fprintf( pFile, ".names g3 y2 g6\n" );
    fprintf( pFile, "10 1\n" );
    fprintf( pFile, "01 1\n" );
    fprintf( pFile, ".names g5 y2 g7\n" );
    fprintf( pFile, "10 1\n" );
    fprintf( pFile, ".names g2 g7 g8\n" );
    fprintf( pFile, "10 1\n" );
    fprintf( pFile, "01 1\n" );
    fprintf( pFile, ".names g6 s\n" );
    fprintf( pFile, "1 1\n" );
    fprintf( pFile, ".names g4 c1\n" );
    fprintf( pFile, "1 1\n" );
    fprintf( pFile, ".names g8 c2\n" );
    fprintf( pFile, "1 1\n" );
    fprintf( pFile, ".end\n" );
}

void Abc_GenAT( char * pFileName, Vec_Int_t * vNums )
{
    word Sum = 0; int i, k, Num, nBits = 0;
    Vec_IntForEachEntry( vNums, Num, i )
        Sum += ((word)1 << i) * Num;
    while ( Sum )
        nBits++, Sum >>= 1;

    Vec_Int_t * vTemp; int nFAs = 0, nHAs = 0, nItem = 0;
    Vec_Wec_t * vItems = Vec_WecStart( nBits );
    Vec_IntForEachEntry( vNums, Num, i )
        for ( k = 0; k < Num; k++ )
            Vec_WecPush( vItems, i, nItem++ );

    FILE * pFile = fopen( pFileName, "w" );
    fprintf( pFile, "# %d-bit %d-input adder tree generated by ABC on %s\n", nBits, nItem, Extra_TimeStamp() );
    fprintf( pFile, "# Profile:" );
    Vec_IntForEachEntry( vNums, Num, i )
        fprintf( pFile, " %d", Num );
    fprintf( pFile, "\n" );

    fprintf( pFile, ".model AT%d_%d\n", nItem, nBits );
    Vec_WecForEachLevel( vItems, vTemp, i ) {
        if ( Vec_IntSize(vTemp) == 0 )
            continue;
        fprintf( pFile, ".inputs" );
        Vec_IntForEachEntry( vTemp, Num, k )
            fprintf( pFile, " %02d", Num );
        fprintf( pFile, "\n" );
    }
    fprintf( pFile, ".outputs" );
    for ( k = 0; k < nBits; k++ )
        fprintf( pFile, " o%02d", k );
    fprintf( pFile, "\n\n" );

    assert( nItem == Vec_IntSum(vNums) );
    Vec_WecForEachLevel( vItems, vTemp, i ) {
        fprintf( pFile, "# Rank %d:\n", i );
        Vec_IntForEachEntry( vTemp, Num, k ) {
            if ( Vec_IntSize(vTemp) < 2 )
                continue;
            while ( Vec_IntSize(vTemp) > 2 ) {
                int i1 = Vec_IntPop(vTemp);
                int i2 = Vec_IntPop(vTemp);
                int i3 = Vec_IntPop(vTemp);
                int i4 = nItem++;
                int i5 = nItem++;
                fprintf( pFile, ".subckt FA a=%02d b=%02d cin=%02d s=%02d cout=%02d\n", i3, i2, i1, i4, i5 ); nFAs++;
                Vec_IntPush( vTemp, i4 );
                if ( i+1 < Vec_WecSize(vItems) )
                    Vec_WecPush( vItems, i+1, i5 );
            }
            if ( Vec_IntSize(vTemp) == 2 ) {
                int i1 = Vec_IntPop(vTemp);
                int i2 = Vec_IntPop(vTemp);    
                int i4 = nItem++;
                int i5 = nItem++;                
                fprintf( pFile, ".subckt HA a=%02d b=%02d s=%02d cout=%02d\n", i2, i1, i4, i5 ); nHAs++;
                Vec_IntPush( vTemp, i4 );
                if ( i+1 < Vec_WecSize(vItems) )
                    Vec_WecPush( vItems, i+1, i5 );                
            }
            assert( Vec_IntSize(vTemp) == 1 );
        }
    }
    Vec_WecForEachLevel( vItems, vTemp, i )
        if ( Vec_IntSize(vTemp) == 0 )
            fprintf( pFile, ".names o%02d\n", i );
        else if ( Vec_IntSize(vTemp) == 1 )
            fprintf( pFile, ".names %02d o%02d\n1 1\n", Vec_IntEntry(vTemp, 0), i );
        else assert( 0 );
    fprintf( pFile, ".end\n\n" );
    Abc_WriteHalfAdder( pFile );
    Abc_WriteFullAdder( pFile );
    printf( "Created %d-bit %d-input AT with %d FAs and %d HAs.\n", nBits, Vec_IntSum(vNums), nFAs, nHAs );
    fclose( pFile );
    Vec_WecFree( vItems );
}
void Abc_GenATDual( char * pFileName, Vec_Int_t * vNums )
{
    word Sum = 0; int i, k, Num, nBits = 0, Iter = 0, fUsed = 0;
    Vec_IntForEachEntry( vNums, Num, i )
        Sum += ((word)1 << i) * Num;
    while ( Sum )
        nBits++, Sum >>= 1;

    Vec_Int_t * vTemp; int nFAs = 0, nHAs = 0, nXors = 0, nItem = 1;
    Vec_Wec_t * vItems = Vec_WecStart( nBits );
    Vec_IntForEachEntry( vNums, Num, i )
        for ( k = 0; k < Num; k++ )
            Vec_WecPush( vItems, i, nItem++ );

    FILE * pFile = fopen( pFileName, "w" );
    fprintf( pFile, "# %d-bit %d-input adder tree generated by ABC on %s\n", nBits, Vec_IntSum(vNums), Extra_TimeStamp() );
    fprintf( pFile, "# Profile:" );
    Vec_IntForEachEntry( vNums, Num, i )
        fprintf( pFile, " %d", Num );
    fprintf( pFile, "\n" );

    fprintf( pFile, ".model AT%d_%d\n", nItem, nBits );
    Vec_WecForEachLevel( vItems, vTemp, i ) {
        if ( Vec_IntSize(vTemp) == 0 )
            continue;
        fprintf( pFile, ".inputs" );
        Vec_IntForEachEntry( vTemp, Num, k )
            fprintf( pFile, " %02d", Num );
        fprintf( pFile, "\n" );
    }
    fprintf( pFile, ".outputs" );
    for ( k = 0; k < nBits; k++ )
        fprintf( pFile, " o%02d", k );
    fprintf( pFile, "\n\n" );

    fprintf( pFile, ".names %02d\n", 0 );
    while ( Vec_WecMaxLevelSize(vItems) > 2 )
    {
        fprintf( pFile, "# Iter %d:\n", Iter++ );
        Vec_Wec_t * vItems2 = Vec_WecStart( nBits );
        Vec_WecForEachLevel( vItems, vTemp, i ) {
            while ( Vec_IntSize(vTemp) > 3 ) {
                int i0 = Vec_IntEntry(vTemp, 0);  Vec_IntDrop(vTemp, 0);
                int i1 = Vec_IntEntry(vTemp, 0);  Vec_IntDrop(vTemp, 0);
                int i2 = Vec_IntEntry(vTemp, 0);  Vec_IntDrop(vTemp, 0);
                int i3 = Vec_IntEntry(vTemp, 0);  Vec_IntDrop(vTemp, 0);
                int i4 = (Vec_IntSize(vTemp) > 0 && Vec_IntEntryLast(vTemp) > 0) ? Vec_IntPop(vTemp) : 0;
                assert( (i0 < 0) == (i1 < 0) );
                assert( (i2 < 0) == (i3 < 0) );
                if ( i1 > 0 ) 
                    fprintf( pFile, ".names %02d %02d %02d\n01 1\n10 1\n", i0, i1, nItem ), i1 = nItem++, nXors++;
                else 
                    i1 = -i1, i0 = -i0;
                if ( i3 > 0 ) 
                    fprintf( pFile, ".names %02d %02d %02d\n01 1\n10 1\n", i2, i3, nItem ), i3 = nItem++, nXors++;
                else 
                    i3 = -i3, i2 = -i2;
                int o0 = nItem++;
                int o1 = nItem++;
                int o2 = nItem++;
                fprintf( pFile, ".subckt MDFA  z=%02d  x1=%02d y1=%02d x2=%02d y2=%02d  s=%02d c1=%02d c2=%02d\n", i4, i0, i1, i2, i3,  o0, o1, o2 ); nFAs += 2, fUsed = 1;
                Vec_WecPush( vItems2, i, o0 );
                if ( i+1 < Vec_WecSize(vItems2) ) {
                    Vec_WecPush( vItems2, i+1, -o1 );
                    Vec_WecPush( vItems2, i+1, -o2 );
                }                
            }
            if ( Vec_IntSize(vTemp) == 3 ) {
                int i2 = Vec_IntPop(vTemp);
                int i1 = Vec_IntPop(vTemp);
                int i0 = Vec_IntPop(vTemp);
                assert( (i0 < 0) == (i1 < 0) );
                assert( i2 > 0 );
                if ( i1 < 0 )
                    fprintf( pFile, ".names %02d %02d %02d\n01 1\n10 1\n", -i0, -i1, nItem ), i0 = -i0, i1 = nItem++, nXors++;
                int o0 = nItem++;
                int o1 = nItem++;
                fprintf( pFile, ".subckt FA a=%02d b=%02d cin=%02d s=%02d cout=%02d\n", i0, i1, i2, o0, o1 ); nFAs++;
                Vec_WecPush( vItems2, i, o0 );
                if ( i+1 < Vec_WecSize(vItems2) )
                    Vec_WecPush( vItems2, i+1, o1 );
            }
            if ( Vec_IntSize(vTemp) == 2 ) {
                int i1 = Vec_IntPop(vTemp);
                int i0 = Vec_IntPop(vTemp);
                assert( (i0 < 0) == (i1 < 0) );
                if ( i1 < 0 ) {
                    Vec_IntInsert( Vec_WecEntry(vItems2, i), 0, i1 );
                    Vec_IntInsert( Vec_WecEntry(vItems2, i), 0, i0 );
                }
                else {
                    Vec_WecPush( vItems2, i, i0 );
                    Vec_WecPush( vItems2, i, i1 );
                }
            }
            if ( Vec_IntSize(vTemp) == 1 ) {
                int i0 = Vec_IntPop(vTemp);
                assert( i0 > 0 );
                Vec_WecPush( vItems2, i, i0 );
            }
            assert( Vec_IntSize(vTemp) == 0 );
        }
        Vec_WecFree( vItems );
        vItems = vItems2;        
    }
    Vec_WecForEachLevel( vItems, vTemp, i ) {
        if ( Vec_IntSize(vTemp) == 2 ) {
            int i1 = Vec_IntPop(vTemp);
            int i0 = Vec_IntPop(vTemp);
            assert( (i0 < 0) == (i1 < 0) );
            if ( i1 < 0 )
                fprintf( pFile, ".names %02d %02d %02d\n01 1\n10 1\n", -i0, -i1, nItem ), i0 = -i0, i1 = nItem++, nXors++;
            Vec_IntPush( vTemp, i0 );
            Vec_IntPush( vTemp, i1 );
        }
        if ( Vec_IntSize(vTemp) == 1 ) {
            int i0 = Vec_IntPop(vTemp);
            assert( i0 > 0 );
            Vec_IntPush( vTemp, i0 );
            Vec_IntPush( vTemp, 0 );
        }
        if ( Vec_IntSize(vTemp) == 0 ) {
            Vec_IntPush( vTemp, 0 );
            Vec_IntPush( vTemp, 0 );
        }
        assert( Vec_IntSize(vTemp) == 2 );
    }
    int cin = 0;
    Vec_WecForEachLevel( vItems, vTemp, i ) {
        int i1 = Vec_IntPop(vTemp);
        int i0 = Vec_IntPop(vTemp);
        assert( i0 >= 0 && i1 >= 0 );
        fprintf( pFile, ".subckt FA a=%02d b=%02d cin=%02d s=o%02d cout=%02d\n", i0, i1, cin, i, nItem ); nFAs++;
        cin = nItem++;
    }
    fprintf( pFile, ".end\n\n" );
    Abc_WriteFullAdder( pFile );
    if ( fUsed )
        Abc_WriteMDFA( pFile );
    printf( "Created %d-bit %d-input AT with %d FAs, %d HAs, and %d XORs.\n", nBits, Vec_IntSum(vNums), nFAs, nHAs, nXors );
    fclose( pFile );
    Vec_WecFree( vItems );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

