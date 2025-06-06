/**CFile****************************************************************

  FileName    [extraBddKmap.c]

  PackageName [extra]

  Synopsis    [Visualizing the K-map.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - September 1, 2003.]

  Revision    [$Id: extraBddKmap.c,v 1.0 2003/05/21 18:03:50 alanmi Exp $]

***********************************************************************/

///      K-map visualization using pseudo graphics      ///
///       Version 1.0. Started - August 20, 2000        ///
///     Version 2.0. Added to EXTRA - July 17, 2001     ///

#include "extraBdd.h"

#ifdef WIN32
#include <windows.h>
#endif

ABC_NAMESPACE_IMPL_START


/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

// the maximum number of variables in the Karnaugh Map
#define MAXVARS 20

/*
// single line
#define SINGLE_VERTICAL     (char)179
#define SINGLE_HORIZONTAL   (char)196
#define SINGLE_TOP_LEFT     (char)218
#define SINGLE_TOP_RIGHT    (char)191
#define SINGLE_BOT_LEFT     (char)192
#define SINGLE_BOT_RIGHT    (char)217

// double line
#define DOUBLE_VERTICAL     (char)186
#define DOUBLE_HORIZONTAL   (char)205
#define DOUBLE_TOP_LEFT     (char)201
#define DOUBLE_TOP_RIGHT    (char)187
#define DOUBLE_BOT_LEFT     (char)200
#define DOUBLE_BOT_RIGHT    (char)188

// line intersections
#define SINGLES_CROSS       (char)197
#define DOUBLES_CROSS       (char)206
#define S_HOR_CROSS_D_VER   (char)215
#define S_VER_CROSS_D_HOR   (char)216

// single line joining
#define S_JOINS_S_VER_LEFT  (char)180
#define S_JOINS_S_VER_RIGHT (char)195
#define S_JOINS_S_HOR_TOP   (char)193
#define S_JOINS_S_HOR_BOT   (char)194

// double line joining
#define D_JOINS_D_VER_LEFT  (char)185
#define D_JOINS_D_VER_RIGHT (char)204
#define D_JOINS_D_HOR_TOP   (char)202
#define D_JOINS_D_HOR_BOT   (char)203

// single line joining double line
#define S_JOINS_D_VER_LEFT  (char)182
#define S_JOINS_D_VER_RIGHT (char)199
#define S_JOINS_D_HOR_TOP   (char)207
#define S_JOINS_D_HOR_BOT   (char)209
*/

// single line
#define SINGLE_VERTICAL     (char)'|'
#define SINGLE_HORIZONTAL   (char)'-'
#define SINGLE_TOP_LEFT     (char)'+'
#define SINGLE_TOP_RIGHT    (char)'+'
#define SINGLE_BOT_LEFT     (char)'+'
#define SINGLE_BOT_RIGHT    (char)'+'

// double line
#define DOUBLE_VERTICAL     (char)'|'
#define DOUBLE_HORIZONTAL   (char)'-'
#define DOUBLE_TOP_LEFT     (char)'+'
#define DOUBLE_TOP_RIGHT    (char)'+'
#define DOUBLE_BOT_LEFT     (char)'+'
#define DOUBLE_BOT_RIGHT    (char)'+'

// line intersections
#define SINGLES_CROSS       (char)'+'
#define DOUBLES_CROSS       (char)'+'
#define S_HOR_CROSS_D_VER   (char)'+'
#define S_VER_CROSS_D_HOR   (char)'+'

// single line joining
#define S_JOINS_S_VER_LEFT  (char)'+'
#define S_JOINS_S_VER_RIGHT (char)'+'
#define S_JOINS_S_HOR_TOP   (char)'+'
#define S_JOINS_S_HOR_BOT   (char)'+'

// double line joining
#define D_JOINS_D_VER_LEFT  (char)'+'
#define D_JOINS_D_VER_RIGHT (char)'+'
#define D_JOINS_D_HOR_TOP   (char)'+'
#define D_JOINS_D_HOR_BOT   (char)'+'

// single line joining double line
#define S_JOINS_D_VER_LEFT  (char)'+'
#define S_JOINS_D_VER_RIGHT (char)'+'
#define S_JOINS_D_HOR_TOP   (char)'+'
#define S_JOINS_D_HOR_BOT   (char)'+'


// other symbols
#define UNDERSCORE          (char)95
//#define SYMBOL_ZERO       (char)248   // degree sign
//#define SYMBOL_ZERO         (char)'o'
#ifdef WIN32
#define SYMBOL_ZERO         (char)'0'
#else
#define SYMBOL_ZERO         (char)' '
#endif
#define SYMBOL_ONE          (char)'1'
#define SYMBOL_DC           (char)'-'
#define SYMBOL_OVERLAP      (char)'?'

// full cells and half cells
#define CELL_FREE           (char)32
#define CELL_FULL           (char)219
#define HALF_UPPER          (char)223
#define HALF_LOWER          (char)220
#define HALF_LEFT           (char)221
#define HALF_RIGHT          (char)222


/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

// the array of BDD variables used internally
static DdNode * s_XVars[MAXVARS];

// flag which determines where the horizontal variable names are printed
static int fHorizontalVarNamesPrintedAbove = 1;

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

// Oleg's way of generating the gray code
static int GrayCode( int BinCode );
static int BinCode ( int GrayCode );

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Prints the K-map of the function.]

  Description [If the pointer to the array of variables XVars is NULL,
               fSuppType determines how the support will be determined.
               fSuppType == 0 -- takes the first nVars of the manager
               fSuppType == 1 -- takes the topmost nVars of the manager
               fSuppType == 2 -- determines support from the on-set and the offset
               ]

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Extra_PrintKMap( 
  FILE * Output,  /* the output stream */
  DdManager * dd, 
  DdNode * OnSet, 
  DdNode * OffSet, 
  int nVars, 
  DdNode ** XVars, 
  int fSuppType, /* the flag which determines how support is computed */
  char ** pVarNames )
{
    int fPrintTruth = 1;
    int d, p, n, s, v, h, w;
    int nVarsVer;
    int nVarsHor;
    int nCellsVer;
    int nCellsHor;
    int nSkipSpaces;

    // make sure that on-set and off-set do not overlap
    if ( !Cudd_bddLeq( dd, OnSet, Cudd_Not(OffSet) ) )
    {
        fprintf( Output, "PrintKMap(): The on-set and the off-set overlap\n" );
        return;
    }
    if ( nVars == 0 )
        { printf( "Function is constant %d.\n", !Cudd_IsComplement(OnSet) ); return; }

    // print truth table for debugging
    if ( fPrintTruth )
    {
        DdNode * bCube, * bPart;
        printf( "Truth table: " );
        if ( nVars == 0 )
            printf( "Constant" );
        else if ( nVars == 1 )
            printf( "1-var function" );
        else
        {
//            printf( "0x" );
            for ( d = (1<<(nVars-2)) - 1; d >= 0; d-- )
            {
                int Value = 0;
                for ( s = 0; s < 4; s++ )
                {
                    bCube = Extra_bddBitsToCube( dd, 4*d+s, nVars, dd->vars, 0 );   Cudd_Ref( bCube );
                    bPart = Cudd_Cofactor( dd, OnSet, bCube );                      Cudd_Ref( bPart );
                    Value |= ((int)(bPart == b1) << s);
                    Cudd_RecursiveDeref( dd, bPart );
                    Cudd_RecursiveDeref( dd, bCube );
                }
                if ( Value < 10 )
                    fprintf( stdout, "%d", Value );
                else
                    fprintf( stdout, "%c", 'a' + Value-10 );
            }
        }
        printf( "\n" );
    }


/*
    if ( OnSet == b1 )
    {
        fprintf( Output, "PrintKMap(): Constant 1\n" );
        return;
    }
    if ( OffSet == b1 )
    {
        fprintf( Output, "PrintKMap(): Constant 0\n" );
        return;
    }
*/
    if ( nVars < 0 || nVars > MAXVARS )
    {
        fprintf( Output, "PrintKMap(): The number of variables is less than zero or more than %d\n", MAXVARS );
        return;
    }

    // determine the support if it is not given
    if ( XVars == NULL )
    {
        if ( fSuppType == 0 )
        {   // assume that the support includes the first nVars of the manager
            assert( nVars );
            for ( v = 0; v < nVars; v++ )
                s_XVars[v] = Cudd_bddIthVar( dd, v );
        }
        else if ( fSuppType == 1 )
        {   // assume that the support includes the topmost nVars of the manager
            assert( nVars );
            for ( v = 0; v < nVars; v++ )
                s_XVars[v] = Cudd_bddIthVar( dd, dd->invperm[v] );
        }
        else // determine the support
        {
            DdNode * SuppOn, * SuppOff, * Supp;
            int cVars = 0;
            DdNode * TempSupp;

            // determine support
            SuppOn = Cudd_Support( dd, OnSet );         Cudd_Ref( SuppOn );
            SuppOff = Cudd_Support( dd, OffSet );       Cudd_Ref( SuppOff );
            Supp = Cudd_bddAnd( dd, SuppOn, SuppOff );  Cudd_Ref( Supp );
            Cudd_RecursiveDeref( dd, SuppOn );
            Cudd_RecursiveDeref( dd, SuppOff );

            nVars = Cudd_SupportSize( dd, Supp );
            if ( nVars > MAXVARS )
            {
                fprintf( Output, "PrintKMap(): The number of variables is more than %d\n", MAXVARS );
                Cudd_RecursiveDeref( dd, Supp );
                return;
            }

            // assign variables
            for ( TempSupp = Supp; TempSupp != dd->one; TempSupp = Cudd_T(TempSupp), cVars++ )
                s_XVars[cVars] = Cudd_bddIthVar( dd, TempSupp->index );

            Cudd_RecursiveDeref( dd, TempSupp );
        }
    }
    else
    {
        // copy variables
        assert( XVars );
        for ( v = 0; v < nVars; v++ )
            s_XVars[v] = XVars[v];
    }

    ////////////////////////////////////////////////////////////////////
    // determine the Karnaugh map parameters
    nVarsVer = nVars/2;
    nVarsHor = nVars - nVarsVer;

    nCellsVer = (1<<nVarsVer);
    nCellsHor = (1<<nVarsHor);
    nSkipSpaces = nVarsVer + 1;

    ////////////////////////////////////////////////////////////////////
    // print variable names
    fprintf( Output, "\n" );
    for ( w = 0; w < nVarsVer; w++ )
        if ( pVarNames == NULL )
            fprintf( Output, "%c", 'a'+nVarsHor+w );
        else
            fprintf( Output, " %s", pVarNames[nVarsHor+w] );

    if ( fHorizontalVarNamesPrintedAbove )
    {
        fprintf( Output, " \\ " );
        for ( w = 0; w < nVarsHor; w++ )
            if ( pVarNames == NULL )
                fprintf( Output, "%c", 'a'+w );
            else
                fprintf( Output, "%s ", pVarNames[w] );
    }
    fprintf( Output, "\n" );

    if ( fHorizontalVarNamesPrintedAbove )
    {
        ////////////////////////////////////////////////////////////////////
        // print horizontal digits
        for ( d = 0; d < nVarsHor; d++ )
        {
            for ( p = 0; p < nSkipSpaces + 2; p++, fprintf( Output, " " ) );
            for ( n = 0; n < nCellsHor; n++ )
                if ( GrayCode(n) & (1<<(nVarsHor-1-d)) )
                    fprintf( Output, "1   " );
                else
                    fprintf( Output, "0   " );
            fprintf( Output, "\n" );
        }
    }

    ////////////////////////////////////////////////////////////////////
    // print the upper line
    for ( p = 0; p < nSkipSpaces; p++, fprintf( Output, " " ) );
    fprintf( Output, "%c", DOUBLE_TOP_LEFT );
    for ( s = 0; s < nCellsHor; s++ )
    {
        fprintf( Output, "%c", DOUBLE_HORIZONTAL );
        fprintf( Output, "%c", DOUBLE_HORIZONTAL );
        fprintf( Output, "%c", DOUBLE_HORIZONTAL );
        if ( s != nCellsHor-1 )
        {
            if ( s&1 )
                fprintf( Output, "%c", D_JOINS_D_HOR_BOT );
            else
                fprintf( Output, "%c", S_JOINS_D_HOR_BOT );
        }
    }
    fprintf( Output, "%c", DOUBLE_TOP_RIGHT );
    fprintf( Output, "\n" );

    ////////////////////////////////////////////////////////////////////
    // print the map
    for ( v = 0; v < nCellsVer; v++ )
    {
        DdNode * CubeVerBDD;

        // print horizontal digits
//      for ( p = 0; p < nSkipSpaces; p++, fprintf( Output, " " ) );
        for ( n = 0; n < nVarsVer; n++ )
            if ( GrayCode(v) & (1<<(nVarsVer-1-n)) )
                fprintf( Output, "1" );
            else
                fprintf( Output, "0" );
        fprintf( Output, " " );

        // find vertical cube
        CubeVerBDD = Extra_bddBitsToCube( dd, GrayCode(v), nVarsVer, s_XVars+nVarsHor, 1 );    Cudd_Ref( CubeVerBDD );

        // print text line
        fprintf( Output, "%c", DOUBLE_VERTICAL );
        for ( h = 0; h < nCellsHor; h++ )
        {
            DdNode * CubeHorBDD, * Prod, * ValueOnSet, * ValueOffSet;

            fprintf( Output, " " );
//          fprintf( Output, "x" );
            ///////////////////////////////////////////////////////////////
            // determine what should be printed
            CubeHorBDD  = Extra_bddBitsToCube( dd, GrayCode(h), nVarsHor, s_XVars, 1 );    Cudd_Ref( CubeHorBDD );
            Prod = Cudd_bddAnd( dd, CubeHorBDD, CubeVerBDD );                   Cudd_Ref( Prod );
            Cudd_RecursiveDeref( dd, CubeHorBDD );

            ValueOnSet  = Cudd_Cofactor( dd, OnSet, Prod );                     Cudd_Ref( ValueOnSet );
            ValueOffSet = Cudd_Cofactor( dd, OffSet, Prod );                    Cudd_Ref( ValueOffSet );
            Cudd_RecursiveDeref( dd, Prod );

#ifdef WIN32
            {
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            char Symb = 0, Color = 0;
            if ( ValueOnSet == b1 && ValueOffSet == b0 )
                Symb = SYMBOL_ONE,     Color = 14;  // yellow
            else if ( ValueOnSet == b0 && ValueOffSet == b1 )
                Symb = SYMBOL_ZERO,    Color = 11;  // blue
            else if ( ValueOnSet == b0 && ValueOffSet == b0 ) 
                Symb = SYMBOL_DC,      Color = 10;  // green
            else if ( ValueOnSet == b1 && ValueOffSet == b1 ) 
                Symb = SYMBOL_OVERLAP, Color = 12;  // red
            else
                assert(0);
            SetConsoleTextAttribute( hConsole, Color );
            fprintf( Output, "%c", Symb );
            SetConsoleTextAttribute( hConsole, 7 );
            }
#else
            {
            if ( ValueOnSet == b1 && ValueOffSet == b0 )
                fprintf( Output, "%c", SYMBOL_ONE );
            else if ( ValueOnSet == b0 && ValueOffSet == b1 )
                fprintf( Output, "%c", SYMBOL_ZERO );
            else if ( ValueOnSet == b0 && ValueOffSet == b0 ) 
                fprintf( Output, "%c", SYMBOL_DC );
            else if ( ValueOnSet == b1 && ValueOffSet == b1 ) 
                fprintf( Output, "%c", SYMBOL_OVERLAP );
            else
                assert(0);
            }
#endif

            Cudd_RecursiveDeref( dd, ValueOnSet );
            Cudd_RecursiveDeref( dd, ValueOffSet );
            ///////////////////////////////////////////////////////////////
            fprintf( Output, " " );

            if ( h != nCellsHor-1 )
            {
                if ( h&1 )
                    fprintf( Output, "%c", DOUBLE_VERTICAL );
                else
                    fprintf( Output, "%c", SINGLE_VERTICAL );
            }
        }
        fprintf( Output, "%c", DOUBLE_VERTICAL );
        fprintf( Output, "\n" );

        Cudd_RecursiveDeref( dd, CubeVerBDD );

        if ( v != nCellsVer-1 )
        // print separator line
        {
            for ( p = 0; p < nSkipSpaces; p++, fprintf( Output, " " ) );
            if ( v&1 )
            {
                fprintf( Output, "%c", D_JOINS_D_VER_RIGHT );
                for ( s = 0; s < nCellsHor; s++ )
                {
                    fprintf( Output, "%c", DOUBLE_HORIZONTAL );
                    fprintf( Output, "%c", DOUBLE_HORIZONTAL );
                    fprintf( Output, "%c", DOUBLE_HORIZONTAL );
                    if ( s != nCellsHor-1 )
                    {
                        if ( s&1 )
                            fprintf( Output, "%c", DOUBLES_CROSS );
                        else
                            fprintf( Output, "%c", S_VER_CROSS_D_HOR );
                    }
                }
                fprintf( Output, "%c", D_JOINS_D_VER_LEFT );
            }
            else
            {
                fprintf( Output, "%c", S_JOINS_D_VER_RIGHT );
                for ( s = 0; s < nCellsHor; s++ )
                {
                    fprintf( Output, "%c", SINGLE_HORIZONTAL );
                    fprintf( Output, "%c", SINGLE_HORIZONTAL );
                    fprintf( Output, "%c", SINGLE_HORIZONTAL );
                    if ( s != nCellsHor-1 )
                    {
                        if ( s&1 )
                            fprintf( Output, "%c", S_HOR_CROSS_D_VER );
                        else
                            fprintf( Output, "%c", SINGLES_CROSS );
                    }
                }
                fprintf( Output, "%c", S_JOINS_D_VER_LEFT );
            }
            fprintf( Output, "\n" );
        }
    }
    
    ////////////////////////////////////////////////////////////////////
    // print the lower line
    for ( p = 0; p < nSkipSpaces; p++, fprintf( Output, " " ) );
    fprintf( Output, "%c", DOUBLE_BOT_LEFT );
    for ( s = 0; s < nCellsHor; s++ )
    {
        fprintf( Output, "%c", DOUBLE_HORIZONTAL );
        fprintf( Output, "%c", DOUBLE_HORIZONTAL );
        fprintf( Output, "%c", DOUBLE_HORIZONTAL );
        if ( s != nCellsHor-1 )
        {
            if ( s&1 )
                fprintf( Output, "%c", D_JOINS_D_HOR_TOP );
            else
                fprintf( Output, "%c", S_JOINS_D_HOR_TOP );
        }
    }
    fprintf( Output, "%c", DOUBLE_BOT_RIGHT );
    fprintf( Output, "\n" );

    if ( !fHorizontalVarNamesPrintedAbove )
    {
        ////////////////////////////////////////////////////////////////////
        // print horizontal digits
        for ( d = 0; d < nVarsHor; d++ )
        {
            for ( p = 0; p < nSkipSpaces + 2; p++, fprintf( Output, " " ) );
            for ( n = 0; n < nCellsHor; n++ )
                if ( GrayCode(n) & (1<<(nVarsHor-1-d)) )
                    fprintf( Output, "1   " );
                else
                    fprintf( Output, "0   " );

            /////////////////////////////////
            fprintf( Output, "%c", (char)('a'+d) );
            /////////////////////////////////
            fprintf( Output, "\n" );
        }
    }
}



/**Function********************************************************************

  Synopsis    [Prints the K-map of the relation.]

  Description [Assumes that the relation depends the first nXVars of XVars and 
  the first nYVars of YVars. Draws X and Y vars and vertical and horizontal vars.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Extra_PrintKMapRelation( 
  FILE * Output,  /* the output stream */
  DdManager * dd, 
  DdNode * OnSet, 
  DdNode * OffSet, 
  int nXVars, 
  int nYVars, 
  DdNode ** XVars, 
  DdNode ** YVars ) /* the flag which determines how support is computed */
{
    int d, p, n, s, v, h, w;
    int nVars;
    int nVarsVer;
    int nVarsHor;
    int nCellsVer;
    int nCellsHor;
    int nSkipSpaces;

    // make sure that on-set and off-set do not overlap
    if ( !Cudd_bddLeq( dd, OnSet, Cudd_Not(OffSet) ) )
    {
        fprintf( Output, "PrintKMap(): The on-set and the off-set overlap\n" );
        return;
    }

    if ( OnSet == b1 )
    {
        fprintf( Output, "PrintKMap(): Constant 1\n" );
        return;
    }
    if ( OffSet == b1 )
    {
        fprintf( Output, "PrintKMap(): Constant 0\n" );
        return;
    }

    nVars = nXVars + nYVars;
    if ( nVars < 0 || nVars > MAXVARS )
    {
        fprintf( Output, "PrintKMap(): The number of variables is less than zero or more than %d\n", MAXVARS );
        return;
    }


    ////////////////////////////////////////////////////////////////////
    // determine the Karnaugh map parameters
    nVarsVer = nXVars;
    nVarsHor = nYVars;
    nCellsVer = (1<<nVarsVer);
    nCellsHor = (1<<nVarsHor);
    nSkipSpaces = nVarsVer + 1;

    ////////////////////////////////////////////////////////////////////
    // print variable names
    fprintf( Output, "\n" );
    for ( w = 0; w < nVarsVer; w++ )
        fprintf( Output, "%c", 'a'+nVarsHor+w );
    if ( fHorizontalVarNamesPrintedAbove )
    {
        fprintf( Output, " \\ " );
        for ( w = 0; w < nVarsHor; w++ )
            fprintf( Output, "%c", 'a'+w );
    }
    fprintf( Output, "\n" );

    if ( fHorizontalVarNamesPrintedAbove )
    {
        ////////////////////////////////////////////////////////////////////
        // print horizontal digits
        for ( d = 0; d < nVarsHor; d++ )
        {
            for ( p = 0; p < nSkipSpaces + 2; p++, fprintf( Output, " " ) );
            for ( n = 0; n < nCellsHor; n++ )
                if ( GrayCode(n) & (1<<(nVarsHor-1-d)) )
                    fprintf( Output, "1   " );
                else
                    fprintf( Output, "0   " );
            fprintf( Output, "\n" );
        }
    }

    ////////////////////////////////////////////////////////////////////
    // print the upper line
    for ( p = 0; p < nSkipSpaces; p++, fprintf( Output, " " ) );
    fprintf( Output, "%c", DOUBLE_TOP_LEFT );
    for ( s = 0; s < nCellsHor; s++ )
    {
        fprintf( Output, "%c", DOUBLE_HORIZONTAL );
        fprintf( Output, "%c", DOUBLE_HORIZONTAL );
        fprintf( Output, "%c", DOUBLE_HORIZONTAL );
        if ( s != nCellsHor-1 )
        {
            if ( s&1 )
                fprintf( Output, "%c", D_JOINS_D_HOR_BOT );
            else
                fprintf( Output, "%c", S_JOINS_D_HOR_BOT );
        }
    }
    fprintf( Output, "%c", DOUBLE_TOP_RIGHT );
    fprintf( Output, "\n" );

    ////////////////////////////////////////////////////////////////////
    // print the map
    for ( v = 0; v < nCellsVer; v++ )
    {
        DdNode * CubeVerBDD;

        // print horizontal digits
//      for ( p = 0; p < nSkipSpaces; p++, fprintf( Output, " " ) );
        for ( n = 0; n < nVarsVer; n++ )
            if ( GrayCode(v) & (1<<(nVarsVer-1-n)) )
                fprintf( Output, "1" );
            else
                fprintf( Output, "0" );
        fprintf( Output, " " );

        // find vertical cube
//      CubeVerBDD = Extra_bddBitsToCube( dd, GrayCode(v), nVarsVer, s_XVars+nVarsHor );    Cudd_Ref( CubeVerBDD );
        CubeVerBDD = Extra_bddBitsToCube( dd, GrayCode(v), nXVars, XVars, 1 );                 Cudd_Ref( CubeVerBDD );

        // print text line
        fprintf( Output, "%c", DOUBLE_VERTICAL );
        for ( h = 0; h < nCellsHor; h++ )
        {
            DdNode * CubeHorBDD, * Prod, * ValueOnSet, * ValueOffSet;

            fprintf( Output, " " );
//          fprintf( Output, "x" );
            ///////////////////////////////////////////////////////////////
            // determine what should be printed
//          CubeHorBDD  = Extra_bddBitsToCube( dd, GrayCode(h), nVarsHor, s_XVars );    Cudd_Ref( CubeHorBDD );
            CubeHorBDD  = Extra_bddBitsToCube( dd, GrayCode(h), nYVars, YVars, 1 );        Cudd_Ref( CubeHorBDD );
            Prod = Cudd_bddAnd( dd, CubeHorBDD, CubeVerBDD );                           Cudd_Ref( Prod );
            Cudd_RecursiveDeref( dd, CubeHorBDD );

            ValueOnSet  = Cudd_Cofactor( dd, OnSet, Prod );                     Cudd_Ref( ValueOnSet );
            ValueOffSet = Cudd_Cofactor( dd, OffSet, Prod );                    Cudd_Ref( ValueOffSet );
            Cudd_RecursiveDeref( dd, Prod );

            if ( ValueOnSet == b1 && ValueOffSet == b0 )
                fprintf( Output, "%c", SYMBOL_ONE );
            else if ( ValueOnSet == b0 && ValueOffSet == b1 )
                fprintf( Output, "%c", SYMBOL_ZERO );
            else if ( ValueOnSet == b0 && ValueOffSet == b0 ) 
                fprintf( Output, "%c", SYMBOL_DC );
            else if ( ValueOnSet == b1 && ValueOffSet == b1 ) 
                fprintf( Output, "%c", SYMBOL_OVERLAP );
            else
                assert(0);

            Cudd_RecursiveDeref( dd, ValueOnSet );
            Cudd_RecursiveDeref( dd, ValueOffSet );
            ///////////////////////////////////////////////////////////////
            fprintf( Output, " " );

            if ( h != nCellsHor-1 )
            {
                if ( h&1 )
                    fprintf( Output, "%c", DOUBLE_VERTICAL );
                else
                    fprintf( Output, "%c", SINGLE_VERTICAL );
            }
        }
        fprintf( Output, "%c", DOUBLE_VERTICAL );
        fprintf( Output, "\n" );

        Cudd_RecursiveDeref( dd, CubeVerBDD );

        if ( v != nCellsVer-1 )
        // print separator line
        {
            for ( p = 0; p < nSkipSpaces; p++, fprintf( Output, " " ) );
            if ( v&1 )
            {
                fprintf( Output, "%c", D_JOINS_D_VER_RIGHT );
                for ( s = 0; s < nCellsHor; s++ )
                {
                    fprintf( Output, "%c", DOUBLE_HORIZONTAL );
                    fprintf( Output, "%c", DOUBLE_HORIZONTAL );
                    fprintf( Output, "%c", DOUBLE_HORIZONTAL );
                    if ( s != nCellsHor-1 )
                    {
                        if ( s&1 )
                            fprintf( Output, "%c", DOUBLES_CROSS );
                        else
                            fprintf( Output, "%c", S_VER_CROSS_D_HOR );
                    }
                }
                fprintf( Output, "%c", D_JOINS_D_VER_LEFT );
            }
            else
            {
                fprintf( Output, "%c", S_JOINS_D_VER_RIGHT );
                for ( s = 0; s < nCellsHor; s++ )
                {
                    fprintf( Output, "%c", SINGLE_HORIZONTAL );
                    fprintf( Output, "%c", SINGLE_HORIZONTAL );
                    fprintf( Output, "%c", SINGLE_HORIZONTAL );
                    if ( s != nCellsHor-1 )
                    {
                        if ( s&1 )
                            fprintf( Output, "%c", S_HOR_CROSS_D_VER );
                        else
                            fprintf( Output, "%c", SINGLES_CROSS );
                    }
                }
                fprintf( Output, "%c", S_JOINS_D_VER_LEFT );
            }
            fprintf( Output, "\n" );
        }
    }
    
    ////////////////////////////////////////////////////////////////////
    // print the lower line
    for ( p = 0; p < nSkipSpaces; p++, fprintf( Output, " " ) );
    fprintf( Output, "%c", DOUBLE_BOT_LEFT );
    for ( s = 0; s < nCellsHor; s++ )
    {
        fprintf( Output, "%c", DOUBLE_HORIZONTAL );
        fprintf( Output, "%c", DOUBLE_HORIZONTAL );
        fprintf( Output, "%c", DOUBLE_HORIZONTAL );
        if ( s != nCellsHor-1 )
        {
            if ( s&1 )
                fprintf( Output, "%c", D_JOINS_D_HOR_TOP );
            else
                fprintf( Output, "%c", S_JOINS_D_HOR_TOP );
        }
    }
    fprintf( Output, "%c", DOUBLE_BOT_RIGHT );
    fprintf( Output, "\n" );

    if ( !fHorizontalVarNamesPrintedAbove )
    {
        ////////////////////////////////////////////////////////////////////
        // print horizontal digits
        for ( d = 0; d < nVarsHor; d++ )
        {
            for ( p = 0; p < nSkipSpaces + 2; p++, fprintf( Output, " " ) );
            for ( n = 0; n < nCellsHor; n++ )
                if ( GrayCode(n) & (1<<(nVarsHor-1-d)) )
                    fprintf( Output, "1   " );
                else
                    fprintf( Output, "0   " );

            /////////////////////////////////
            fprintf( Output, "%c", (char)('a'+d) );
            /////////////////////////////////
            fprintf( Output, "\n" );
        }
    }
}



/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int GrayCode ( int BinCode )
{
  return BinCode ^ ( BinCode >> 1 );
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int BinCode ( int GrayCode )
{
  int bc = GrayCode;
  while( GrayCode >>= 1 ) bc ^= GrayCode;
  return bc;
}


ABC_NAMESPACE_IMPL_END

