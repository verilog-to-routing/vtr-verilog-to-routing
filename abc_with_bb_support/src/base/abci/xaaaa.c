/**CFile****************************************************************

  FileName    [abc_.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abc_.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"
#include "kit.h"

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
void Abc_NtkPrintMeasures( unsigned * pTruth, int nVars )
{
    unsigned uCofs[10][32];
    int i, k, nOnes;

    // total pairs
    nOnes =  Kit_TruthCountOnes( uCofs[0], nVars );
    printf( "Total = %d.\n", nOnes * ((1 << nVars) - nOnes) );

    // print measures for individual variables
    for ( i = 0; i < nVars; i++ )
    {
        Kit_TruthUniqueNew( uCofs[0], pTruth, nVars, i );
        nOnes = Kit_TruthCountOnes( uCofs[0], nVars );
        printf( "%7d ", nOnes );
    }
    printf( "\n" );

    // consider pairs
    for ( i = 0; i < nVars; i++ )
    for ( k = 0; k < nVars; k++ )
    {
        if ( i == k )
        {
            printf( "        " );
            continue;
        }
        Kit_TruthCofactor0New( uCofs[0], pTruth, nVars, i );
        Kit_TruthCofactor1New( uCofs[1], pTruth, nVars, i );

        Kit_TruthCofactor0New( uCofs[2], uCofs[0], nVars, k ); // 00
        Kit_TruthCofactor1New( uCofs[3], uCofs[0], nVars, k ); // 01
        Kit_TruthCofactor0New( uCofs[4], uCofs[1], nVars, k ); // 10
        Kit_TruthCofactor1New( uCofs[5], uCofs[1], nVars, k ); // 11

        Kit_TruthAndPhase( uCofs[6], uCofs[2], uCofs[5], nVars, 0, 1 ); // 00  & 11'
        Kit_TruthAndPhase( uCofs[7], uCofs[2], uCofs[5], nVars, 1, 0 ); // 00' & 11
        Kit_TruthAndPhase( uCofs[8], uCofs[3], uCofs[4], nVars, 0, 1 ); // 01  & 10'
        Kit_TruthAndPhase( uCofs[9], uCofs[3], uCofs[4], nVars, 1, 0 ); // 01' & 10

        nOnes = Kit_TruthCountOnes( uCofs[6], nVars ) + 
                Kit_TruthCountOnes( uCofs[7], nVars ) + 
                Kit_TruthCountOnes( uCofs[8], nVars ) + 
                Kit_TruthCountOnes( uCofs[9], nVars );

        printf( "%7d ", nOnes );
        if ( k == nVars - 1 )
            printf( "\n" );
    }
    printf( "\n" );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_Ntk4VarTable( Abc_Ntk_t * pNtk )
{
    static u4VarTts[222] = {
        0x0000, 0x0001, 0x0003, 0x0006, 0x0007, 0x000f, 0x0016, 0x0017, 0x0018, 0x0019, 
        0x001b, 0x001e, 0x001f, 0x003c, 0x003d, 0x003f, 0x0069, 0x006b, 0x006f, 0x007e, 
        0x007f, 0x00ff, 0x0116, 0x0117, 0x0118, 0x0119, 0x011a, 0x011b, 0x011e, 0x011f, 
        0x012c, 0x012d, 0x012f, 0x013c, 0x013d, 0x013e, 0x013f, 0x0168, 0x0169, 0x016a, 
        0x016b, 0x016e, 0x016f, 0x017e, 0x017f, 0x0180, 0x0181, 0x0182, 0x0183, 0x0186, 
        0x0187, 0x0189, 0x018b, 0x018f, 0x0196, 0x0197, 0x0198, 0x0199, 0x019a, 0x019b, 
        0x019e, 0x019f, 0x01a8, 0x01a9, 0x01aa, 0x01ab, 0x01ac, 0x01ad, 0x01ae, 0x01af, 
        0x01bc, 0x01bd, 0x01be, 0x01bf, 0x01e8, 0x01e9, 0x01ea, 0x01eb, 0x01ee, 0x01ef, 
        0x01fe, 0x033c, 0x033d, 0x033f, 0x0356, 0x0357, 0x0358, 0x0359, 0x035a, 0x035b, 
        0x035e, 0x035f, 0x0368, 0x0369, 0x036a, 0x036b, 0x036c, 0x036d, 0x036e, 0x036f, 
        0x037c, 0x037d, 0x037e, 0x03c0, 0x03c1, 0x03c3, 0x03c5, 0x03c6, 0x03c7, 0x03cf, 
        0x03d4, 0x03d5, 0x03d6, 0x03d7, 0x03d8, 0x03d9, 0x03db, 0x03dc, 0x03dd, 0x03de, 
        0x03fc, 0x0660, 0x0661, 0x0662, 0x0663, 0x0666, 0x0667, 0x0669, 0x066b, 0x066f, 
        0x0672, 0x0673, 0x0676, 0x0678, 0x0679, 0x067a, 0x067b, 0x067e, 0x0690, 0x0691, 
        0x0693, 0x0696, 0x0697, 0x069f, 0x06b0, 0x06b1, 0x06b2, 0x06b3, 0x06b4, 0x06b5, 
        0x06b6, 0x06b7, 0x06b9, 0x06bd, 0x06f0, 0x06f1, 0x06f2, 0x06f6, 0x06f9, 0x0776, 
        0x0778, 0x0779, 0x077a, 0x077e, 0x07b0, 0x07b1, 0x07b4, 0x07b5, 0x07b6, 0x07bc, 
        0x07e0, 0x07e1, 0x07e2, 0x07e3, 0x07e6, 0x07e9, 0x07f0, 0x07f1, 0x07f2, 0x07f8, 
        0x0ff0, 0x1668, 0x1669, 0x166a, 0x166b, 0x166e, 0x167e, 0x1681, 0x1683, 0x1686, 
        0x1687, 0x1689, 0x168b, 0x168e, 0x1696, 0x1697, 0x1698, 0x1699, 0x169a, 0x169b, 
        0x169e, 0x16a9, 0x16ac, 0x16ad, 0x16bc, 0x16e9, 0x177e, 0x178e, 0x1796, 0x1798, 
        0x179a, 0x17ac, 0x17e8, 0x18e7, 0x19e1, 0x19e3, 0x19e6, 0x1bd8, 0x1be4, 0x1ee1, 
        0x3cc3, 0x6996
    };
    int Counters[222];

    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj;
    int i;

    // set elementary truth tables
    Abc_NtkForEachPi( pNtk, pObj, i )
        pObj

    Abc_NtkForEachPo( pNtk, pObj, i )
    {
        vNodes = Abc_NtkDfsNodes( pNtk, &pObj, i );
        Vec_PtrFree( vNodes );
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


