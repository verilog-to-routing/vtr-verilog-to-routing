/**CFile****************************************************************

  FileName    [AbcGlucoseCmd.cpp]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT solver Glucose 3.0 by Gilles Audemard and Laurent Simon.]

  Synopsis    [Interface to Glucose.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 6, 2017.]

  Revision    [$Id: AbcGlucoseCmd.cpp,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig/gia/gia.h"
#include "base/main/mainInt.h"

#include "sat/glucose/AbcGlucose.h"


ABC_NAMESPACE_HEADER_START

extern void Glucose_Init( Abc_Frame_t *pAbc );
extern void Glucose_End( Abc_Frame_t * pAbc );

ABC_NAMESPACE_HEADER_END

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Abc_CommandGlucose( Abc_Frame_t * pAbc, int argc, char ** argv );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void Glucose_Init(Abc_Frame_t *pAbc)
{
    Cmd_CommandAdd( pAbc, "ABC9", "&glucose",  Abc_CommandGlucose,  0 );
}

void Glucose_End( Abc_Frame_t * pAbc )
{
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_CommandGlucose( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    int c       = 0;
    int pre     = 1;
    int verb    = 0;
    int nConfls = 0;
    int fDumpCnf = 0;

    Glucose_Pars pPars;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "Cpdvh" ) ) != EOF )
    {
        switch ( c )
        {
            case 'C':
                if ( globalUtilOptind >= argc )
                {
                    Abc_Print( -1, "Command line switch \"-C\" should be followed by an integer.\n" );
                    goto usage;
                }
                nConfls = atoi(argv[globalUtilOptind]);
                globalUtilOptind++;
                if ( nConfls < 0 )
                    goto usage;
                break;
            case 'p':
                pre ^= 1;
                break;
            case 'd':
                fDumpCnf ^= 1;
                break;
            case 'v':
                verb ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }

    pPars = Glucose_CreatePars(pre,verb,0,nConfls);

    if ( argc == globalUtilOptind + 1 )
    {
        Glucose_SolveCnf( argv[globalUtilOptind], &pPars, fDumpCnf );
        return 0;
    }

    if ( pAbc->pGia == NULL )
    {
        Abc_Print( -1, "Abc_CommandGlucose(): There is no AIG.\n" );
        return 1;
    }
    
    if ( Glucose_SolveAig( pAbc->pGia, &pPars ) == 10 )
        Abc_FrameReplaceCex( pAbc, &pAbc->pGia->pCexComb );

    return 0;
    
usage:
    Abc_Print( -2, "usage: &glucose [-C num] [-pdvh] <file.cnf>\n" );
    Abc_Print( -2, "\t             run Glucose 3.0 by Gilles Audemard and Laurent Simon\n" );
    Abc_Print( -2, "\t-C num     : conflict limit [default = %d]\n",  nConfls );
    Abc_Print( -2, "\t-p         : enable preprocessing [default = %d]\n",pre);
    Abc_Print( -2, "\t-d         : enable dumping CNF after proprocessing [default = %d]\n",fDumpCnf);
    Abc_Print( -2, "\t-v         : verbosity [default = %d]\n",verb);
    Abc_Print( -2, "\t-h         : print the command usage\n");
    Abc_Print( -2, "\t<file.cnf> : (optional) CNF file to solve\n");
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END
