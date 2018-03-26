/**CFile****************************************************************

  FileName    [wlcStdin.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Verilog parser.]

  Synopsis    [stdin processing for STM interface.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 22, 2014.]

  Revision    [$Id: wlcStdin.c,v 1.00 2014/09/12 00:00:00 alanmi Exp $]

***********************************************************************/

#include "wlc.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Converts a bit-string into a number in a given radix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wlc_ComputeSum( char * pRes, char * pAdd, int nBits, int Radix )
{
    char Carry = 0;  int i; 
    for ( i = 0; i < nBits; i++ )
    {
        char Sum = pRes[i] + pAdd[i] + Carry;
        if ( Sum >= Radix )
        {
            Sum -= Radix;
            Carry = 1;
        }
        else 
            Carry = 0;
        assert( Sum >= 0 && Sum < Radix );
        pRes[i] = Sum;
    }
    assert( Carry == 0 );
}
Vec_Str_t * Wlc_ConvertToRadix( unsigned * pBits, int Start, int nBits, int Radix )
{
    Vec_Str_t * vRes = Vec_StrStart( nBits );
    Vec_Str_t * vSum = Vec_StrStart( nBits );
    char * pRes = Vec_StrArray( vRes );
    char * pSum = Vec_StrArray( vSum );  int i;
    assert( Radix >= 2 && Radix < 36 );
    pSum[0] = 1;
    // compute number
    for ( i = 0; i < nBits; i++ )
    {
        if ( Abc_InfoHasBit(pBits, Start + i) )
            Wlc_ComputeSum( pRes, pSum, nBits, Radix );
        if ( i < nBits - 1 )
            Wlc_ComputeSum( pSum, pSum, nBits, Radix );
    }
    Vec_StrFree( vSum );
    // remove zeros
    for ( i = nBits - 1; i >= 0; i-- )
        if ( pRes[i] )
            break;
    Vec_StrShrink( vRes, i+1 );
    // convert to chars
    for ( ; i >= 0; i-- )
    {
        if ( pRes[i] < 10 )
            pRes[i] += '0';
        else
            pRes[i] += 'a' - 10;
    }
    Vec_StrReverseOrder( vRes );
    if ( Vec_StrSize(vRes) == 0 )
        Vec_StrPush( vRes, '0' );
    Vec_StrPush( vRes, '\0' );
    return vRes;
}

/**Function*************************************************************

  Synopsis    [Report results.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wlc_NtkReport( Wlc_Ntk_t * p, Abc_Cex_t * pCex, char * pName, int Radix )
{
    Vec_Str_t * vNum;
    int i, NameId, Name, Start = -1, nBits = -1;
    assert( pCex->nRegs == 0 );
    // get the name ID
    NameId = Abc_NamStrFind( p->pManName, pName );
    if ( NameId <= 0 )
    {
        printf( "Cannot find \"%s\" among nodes of the network.\n", pName );
        return;
    }
    // get the primary input
    Vec_IntForEachEntryTriple( &p->vValues, Name, Start, nBits, i )
        if ( Name == NameId )
            break;
    // find the PI
    if ( i == Vec_IntSize(&p->vValues) )
    {
        printf( "Cannot find \"%s\" among primary inputs of the network.\n", pName );
        return;
    }
    // print value
    assert( Radix == 2 || Radix == 10 || Radix == 16 );
    vNum = Wlc_ConvertToRadix( pCex->pData, Start, nBits, Radix );
    printf( "((%s %s%s))\n", pName, Radix == 16 ? "#x" : (Radix == 2 ? "#b" : ""), Vec_StrArray(vNum) );
    Vec_StrFree( vNum );
}

/**Function********************************************************************

  Synopsis    [Reads stdin and stops when "(check-sat)" is observed.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
static inline int Wlc_StdinCollectStop( Vec_Str_t * vInput, char * pLine, int LineSize )
{
    char * pStr = Vec_StrArray(vInput) + Vec_StrSize(vInput) - LineSize;
    if ( Vec_StrSize(vInput) < LineSize )
        return 0;
    return !strncmp( pStr, pLine, LineSize );
}
Vec_Str_t * Wlc_StdinCollectProblem( char * pDir )
{
    int c, DirSize = strlen(pDir);
    Vec_Str_t * vInput = Vec_StrAlloc( 1000 );
    while ( (c = fgetc(stdin)) != EOF )
    {
        Vec_StrPush( vInput, (char)c );
        if ( c == ')' && Wlc_StdinCollectStop(vInput, pDir, DirSize) )
            break;
    }
    Vec_StrPush( vInput, '\0' );
    return vInput;
}
Vec_Str_t * Wlc_StdinCollectQuery()
{
    int c, Count = 0, fSomeInput = 0;
    Vec_Str_t * vInput = Vec_StrAlloc( 1000 );
    while ( (c = fgetc(stdin)) != EOF )
    {
        Vec_StrPush( vInput, (char)c );
        if ( c == '(' )
            Count++, fSomeInput = 1;
        else if ( c == ')' )
            Count--;
        if ( Count == 0 && fSomeInput )
            break;
    }
    if ( c == EOF )
        Vec_StrFreeP( &vInput );
    else
        Vec_StrPush( vInput, '\0' );
    return vInput;
}
int Wlc_StdinProcessSmt( Abc_Frame_t * pAbc, char * pCmd )
{
    // collect stdin until (check-sat)
    Vec_Str_t * vInput = Wlc_StdinCollectProblem( "(check-sat)" );
    // parse input
    Wlc_Ntk_t * pNtk = Wlc_ReadSmtBuffer( "top", Vec_StrArray(vInput), Vec_StrArray(vInput) + Vec_StrSize(vInput), 0, 0 );
    Vec_StrFree( vInput );
    // install current network
    Wlc_SetNtk( pAbc, pNtk );
    // execute command
    if ( Cmd_CommandExecute(pAbc, pCmd) )
    {
        Abc_Print( 1, "Something did not work out with the command \"%s\".\n", pCmd );
        return 0;
    }
    // solver finished
    if ( Abc_FrameReadProbStatus(pAbc) == -1 )
        printf( "undecided\n" );
    else if ( Abc_FrameReadProbStatus(pAbc) == 1 )
        printf( "unsat\n" );
    else if ( Abc_FrameReadProbStatus(pAbc) == 0 )
        printf( "sat\n" );
    else assert( 0 );
    fflush( stdout );
    // wait for stdin for give directions
    while ( (vInput = Wlc_StdinCollectQuery()) != NULL )
    {
        char * pName = strtok( Vec_StrArray(vInput), " \n\t\r()" );
        // check directive 
        if ( strcmp(pName, "get-value") )
        {
            Abc_Print( 1, "ABC is expecting \"get-value\" in a follow-up input of the satisfiable problem.\n" );
            Vec_StrFree( vInput );
            return 0;
        }
        // check status
        if ( Abc_FrameReadProbStatus(pAbc) != 0 )
        {
            Abc_Print( 1, "ABC received a follow-up input for a problem that is not known to be satisfiable.\n" );
            Vec_StrFree( vInput );
            return 0;
        }
        // get the variable number
        pName = strtok( NULL, "() \n\t\r" );
        // get the counter-example
        if ( Abc_FrameReadCex(pAbc) == NULL )
        {
            Abc_Print( 1, "ABC does not have a counter-example available to process a \"get-value\" request.\n" );
            Vec_StrFree( vInput );
            return 0;
        }
        // report value of this variable
        Wlc_NtkReport( (Wlc_Ntk_t *)pAbc->pAbcWlc, (Abc_Cex_t *)Abc_FrameReadCex(pAbc), pName, 16 );
        Vec_StrFree( vInput );
        fflush( stdout );
    }
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

