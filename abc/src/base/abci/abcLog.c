/**CFile****************************************************************

  FileName    [abcLog.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Log file printing.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcLog.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "aig/gia/gia.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

/*
    Log file format (Jiang, Mon, 28 Sep 2009;  updated by Alan in Jan 2011)

    <result> <bug_free_depth>  <engine_name> <0-based_output_num> <0-based_frame>
    <INIT_STATE> : default is empty line.
    <TRACE> : default is empty line
   
    <result> is one of the following: "snl_SAT", "snl_UNSAT", "snl_UNK", "snl_ABORT".
    <bug_free_depth> is the number of timeframes exhaustively explored without counter-examples
    <0-based_output_num> only need to be given if the problem is SAT.
    <0-based_frame> only need to be given if the problem is SAT and <0-based_frame> is different from <bug_free_depth>.   
    <INIT_STATE>  : initial state
    <TRACE> : input vector

    <INIT_STATE>and <TRACE> are strings of 0/1/- ( - means don't care). The length is equivalent to #input*#<cyc>.
*/


         



////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkWriteLogFile( char * pFileName, Abc_Cex_t * pCex, int Status, int nFrames, char * pCommand )
{
    FILE * pFile;
    int i;
    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        printf( "Cannot open log file for writing \"%s\".\n" , pFileName );
        return;
    }
    // write <result>
    if ( Status == 1 )
        fprintf( pFile, "snl_UNSAT" );
    else if ( Status == 0 )
        fprintf( pFile, "snl_SAT" );
    else if ( Status == -1 )
        fprintf( pFile, "snl_UNK" );
    else 
        printf( "Abc_NtkWriteLogFile(): Cannot recognize solving status.\n" );
    fprintf( pFile, " " );
    // write <bug_free_depth>
    fprintf( pFile, "%d", nFrames );
    fprintf( pFile, " " );
    // write <engine_name>
    fprintf( pFile, "%s", pCommand ? pCommand : "unknown" );
    if ( pCex && Status == 0 )
        fprintf( pFile, " %d", pCex->iPo );
    // write <cyc>
    if ( pCex && pCex->iFrame != nFrames )
        fprintf( pFile, " %d", pCex->iFrame );
    fprintf( pFile, "\n" );
    // write <INIT_STATE>
    if ( pCex == NULL )
        fprintf( pFile, "NULL" );
    else
    {
        for ( i = 0; i < pCex->nRegs; i++ )
            fprintf( pFile, "%d", Abc_InfoHasBit(pCex->pData,i) );
    }
    fprintf( pFile, "\n" );
    // write <TRACE>
    if ( pCex == NULL )
        fprintf( pFile, "NULL" );
    else
    {
        assert( pCex->nBits - pCex->nRegs == pCex->nPis * (pCex->iFrame + 1) );
        for ( i = pCex->nRegs; i < pCex->nBits; i++ )
            fprintf( pFile, "%d", Abc_InfoHasBit(pCex->pData,i) );
    }
    fprintf( pFile, "\n" );
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkReadLogFile( char * pFileName, Abc_Cex_t ** ppCex, int * pnFrames )
{
    FILE * pFile;
    Abc_Cex_t * pCex;
    Vec_Int_t * vNums;
    char Buffer[1000], * pToken, * RetValue;
    int c, nRegs = -1, nFrames = -1, iPo = -1, Status = -1, nFrames2 = -1;
    pFile = fopen( pFileName, "r" );
    if ( pFile == NULL )
    {
        printf( "Cannot open log file for reading \"%s\".\n" , pFileName );
        return -1;
    }
    RetValue = fgets( Buffer, 1000, pFile );
    if ( !strncmp( Buffer, "snl_UNSAT", strlen("snl_UNSAT") ) )
    {
        Status = 1;
        nFrames = atoi( Buffer + strlen("snl_UNSAT") ); 
    }
    else if ( !strncmp( Buffer, "snl_SAT", strlen("snl_SAT") ) )
    {
        Status = 0;
//        nFrames = atoi( Buffer + strlen("snl_SAT") ); 
        pToken  = strtok( Buffer + strlen("snl_SAT"), " \t\n" );
        nFrames = atoi( pToken ); 
        pToken  = strtok( NULL, " \t\n" );
        pToken  = strtok( NULL, " \t\n" );
        if ( pToken != NULL )
        {
            iPo     = atoi( pToken ); 
            pToken  = strtok( NULL, " \t\n" );
            if ( pToken )
                nFrames2 = atoi( pToken ); 
        }
//        else
//            printf( "Warning! The current status is SAT but the current CEX is not given.\n"  );
    }
    else if ( !strncmp( Buffer, "snl_UNK", strlen("snl_UNK") ) )
    {
        Status = -1;
        nFrames = atoi( Buffer + strlen("snl_UNK") ); 
    }
    else
    {
        printf( "Unrecognized status.\n" );
    }
    // found regs till the new line
    vNums = Vec_IntAlloc( 100 );
    while ( (c = fgetc(pFile)) != EOF )
    {
        if ( c == '\n' )
            break;
        if ( c == '0' || c == '1' )
            Vec_IntPush( vNums, c - '0' );
    }
    nRegs = Vec_IntSize(vNums);
    // skip till the new line
    while ( (c = fgetc(pFile)) != EOF )
    {
        if ( c == '0' || c == '1' )
            Vec_IntPush( vNums, c - '0' );
    }
    fclose( pFile );
    if ( Vec_IntSize(vNums) )
    {
        int iFrameCex = (nFrames2 == -1) ? nFrames : nFrames2;
        if ( nRegs < 0 )
        {
            printf( "Cannot read register number.\n" );
            Vec_IntFree( vNums );
            return -1;
        }
        if ( Vec_IntSize(vNums)-nRegs == 0 )
        {
            printf( "Cannot read counter example.\n" );
            Vec_IntFree( vNums );
            return -1;
        }
        if ( (Vec_IntSize(vNums)-nRegs) % (iFrameCex + 1) != 0 )
        {
            printf( "Incorrect number of bits.\n" );
            Vec_IntFree( vNums );
            return -1;
        }
        pCex = Abc_CexAlloc( nRegs, (Vec_IntSize(vNums)-nRegs)/(iFrameCex + 1), iFrameCex + 1 );
        pCex->iPo    = iPo;
        pCex->iFrame = iFrameCex;
        assert( Vec_IntSize(vNums) == pCex->nBits );
        for ( c = 0; c < pCex->nBits; c++ )
            if ( Vec_IntEntry(vNums, c) )
                Abc_InfoSetBit( pCex->pData, c );
        Vec_IntFree( vNums );
        if ( ppCex )
            *ppCex = pCex;
        else
            ABC_FREE( pCex );
    }
    else
    {                                                         
        // corner case of seq circuit with no PIs             
        int iFrameCex = (nFrames2 == -1) ? nFrames : nFrames2;
        pCex = Abc_CexAlloc( 0, 0, iFrameCex + 1 );           
        pCex->iFrame = iFrameCex;                             
        pCex->iPo    = iPo;                                   
        if ( ppCex )                                          
            *ppCex = pCex;                                    
        Vec_IntFree( vNums );                                 
    }                                                         
    if ( pnFrames )
        *pnFrames = nFrames;
    return Status;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

