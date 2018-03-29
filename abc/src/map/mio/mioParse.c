/**CFile****************************************************************

  FileName    [mioParse.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Parsing Boolean expressions.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mioParse.c,v 1.4 2004/06/28 14:20:25 alanmi Exp $]

***********************************************************************/

#include "mioInt.h"
#include "exp.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// the list of operation symbols to be used in expressions
#define MIO_EQN_SYM_OPEN    '('   // opening parenthesis
#define MIO_EQN_SYM_CLOSE   ')'   // closing parenthesis
#define MIO_EQN_SYM_CONST0  '0'   // constant 0
#define MIO_EQN_SYM_CONST1  '1'   // constant 1
#define MIO_EQN_SYM_NEG     '!'   // negation before the variable
#define MIO_EQN_SYM_NEGAFT  '\''  // negation after the variable
#define MIO_EQN_SYM_AND     '*'   // logic AND
#define MIO_EQN_SYM_AND2    '&'   // logic AND
#define MIO_EQN_SYM_XOR     '^'   // logic XOR
#define MIO_EQN_SYM_OR      '+'   // logic OR
#define MIO_EQN_SYM_OR2     '|'   // logic OR

// the list of opcodes (also specifying operation precedence)
#define MIO_EQN_OPER_NEG    10    // negation
#define MIO_EQN_OPER_AND     9    // logic AND
#define MIO_EQN_OPER_XOR     8    // logic XOR
#define MIO_EQN_OPER_OR      7    // logic OR
#define MIO_EQN_OPER_MARK    1    // OpStack token standing for an opening parenthesis

// these are values of the internal Flag
#define MIO_EQN_FLAG_START   1    // after the opening parenthesis 
#define MIO_EQN_FLAG_VAR     2    // after operation is received
#define MIO_EQN_FLAG_OPER    3    // after operation symbol is received
#define MIO_EQN_FLAG_ERROR   4    // when error is detected

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs the operation on the top entries in the stack.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Mio_ParseFormulaOper( int * pMan, int nVars, Vec_Ptr_t * pStackFn, int Oper )
{
    Vec_Int_t * gArg1, * gArg2, * gFunc;
    // perform the given operation
    gArg2 = (Vec_Int_t *)Vec_PtrPop( pStackFn );
    gArg1 = (Vec_Int_t *)Vec_PtrPop( pStackFn );
    if ( Oper == MIO_EQN_OPER_AND )
        gFunc = Exp_And( pMan, nVars, gArg1, gArg2, 0, 0 );
    else if ( Oper == MIO_EQN_OPER_OR )
        gFunc = Exp_Or( pMan, nVars, gArg1, gArg2 );
    else if ( Oper == MIO_EQN_OPER_XOR )
        gFunc = Exp_Xor( pMan, nVars, gArg1, gArg2 );
    else
        return NULL;
//    Cudd_Ref( gFunc );
//    Cudd_RecursiveDeref( dd, gArg1 );
//    Cudd_RecursiveDeref( dd, gArg2 );
    Vec_IntFree( gArg1 );
    Vec_IntFree( gArg2 );
    Vec_PtrPush( pStackFn,  gFunc );
    return gFunc;
}

/**Function*************************************************************

  Synopsis    [Derives the AIG corresponding to the equation.]

  Description [Takes the stream to output messages, the formula, the vector
  of variable names and the AIG manager.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Mio_ParseFormula( char * pFormInit, char ** ppVarNames, int nVars )
{
    char * pFormula;
    int Man = nVars, * pMan = &Man;
    Vec_Ptr_t * pStackFn;
    Vec_Int_t * pStackOp;
    Vec_Int_t * gFunc;
    char * pTemp, * pName;
    int nParans, fFound, Flag;
    int Oper, Oper1, Oper2;
    int i, v;

    // make sure that the number of opening and closing parentheses is the same
    nParans = 0;
    for ( pTemp = pFormInit; *pTemp; pTemp++ )
        if ( *pTemp == '(' )
            nParans++;
        else if ( *pTemp == ')' )
            nParans--;
    if ( nParans != 0 )
    {
        fprintf( stdout, "Mio_ParseFormula(): Different number of opening and closing parentheses ().\n" );
        return NULL;
    }

    // copy the formula
    pFormula = ABC_ALLOC( char, strlen(pFormInit) + 3 );
    sprintf( pFormula, "(%s)", pFormInit );

    // start the stacks
    pStackFn = Vec_PtrAlloc( 100 );
    pStackOp = Vec_IntAlloc( 100 );

    Flag = MIO_EQN_FLAG_START;
    for ( pTemp = pFormula; *pTemp; pTemp++ )
    {
        switch ( *pTemp )
        {
        // skip all spaces, tabs, and end-of-lines
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            continue;
        case MIO_EQN_SYM_CONST0:
            Vec_PtrPush( pStackFn, Exp_Const0() );  // Cudd_Ref( b0 );
            if ( Flag == MIO_EQN_FLAG_VAR )
            {
                fprintf( stdout, "Mio_ParseFormula(): No operation symbol before constant 0.\n" );
                Flag = MIO_EQN_FLAG_ERROR; 
                break;
            }
            Flag = MIO_EQN_FLAG_VAR; 
            break;
        case MIO_EQN_SYM_CONST1:
            Vec_PtrPush( pStackFn, Exp_Const1() );  //  Cudd_Ref( b1 );
            if ( Flag == MIO_EQN_FLAG_VAR )
            {
                fprintf( stdout, "Mio_ParseFormula(): No operation symbol before constant 1.\n" );
                Flag = MIO_EQN_FLAG_ERROR; 
                break;
            }
            Flag = MIO_EQN_FLAG_VAR; 
            break;
        case MIO_EQN_SYM_NEG:
            if ( Flag == MIO_EQN_FLAG_VAR )
            {// if NEGBEF follows a variable, AND is assumed
                Vec_IntPush( pStackOp, MIO_EQN_OPER_AND );
                Flag = MIO_EQN_FLAG_OPER;
            }
            Vec_IntPush( pStackOp, MIO_EQN_OPER_NEG );
            break;
        case MIO_EQN_SYM_NEGAFT:
            if ( Flag != MIO_EQN_FLAG_VAR )
            {// if there is no variable before NEGAFT, it is an error
                fprintf( stdout, "Mio_ParseFormula(): No variable is specified before the negation suffix.\n" );
                Flag = MIO_EQN_FLAG_ERROR; 
                break;
            }
            else // if ( Flag == PARSE_FLAG_VAR )
                Vec_PtrPush( pStackFn, Exp_Not( (Vec_Int_t *)Vec_PtrPop(pStackFn) ) );
            break;
        case MIO_EQN_SYM_AND:
        case MIO_EQN_SYM_AND2:
        case MIO_EQN_SYM_OR:
        case MIO_EQN_SYM_OR2:
        case MIO_EQN_SYM_XOR:
            if ( Flag != MIO_EQN_FLAG_VAR )
            {
                fprintf( stdout, "Mio_ParseFormula(): There is no variable before AND, EXOR, or OR.\n" );
                Flag = MIO_EQN_FLAG_ERROR; 
                break;
            }
            if ( *pTemp == MIO_EQN_SYM_AND || *pTemp == MIO_EQN_SYM_AND2 )
                Vec_IntPush( pStackOp, MIO_EQN_OPER_AND );
            else if ( *pTemp == MIO_EQN_SYM_OR || *pTemp == MIO_EQN_SYM_OR2 )
                Vec_IntPush( pStackOp, MIO_EQN_OPER_OR );
            else //if ( *pTemp == MIO_EQN_SYM_XOR )
                Vec_IntPush( pStackOp, MIO_EQN_OPER_XOR );
            Flag = MIO_EQN_FLAG_OPER; 
            break;
        case MIO_EQN_SYM_OPEN:
            if ( Flag == MIO_EQN_FLAG_VAR )
            {
                Vec_IntPush( pStackOp, MIO_EQN_OPER_AND );
//                fprintf( stdout, "Mio_ParseFormula(): An opening parenthesis follows a var without operation sign.\n" ); 
//                Flag = MIO_EQN_FLAG_ERROR; 
//              break; 
            }
            Vec_IntPush( pStackOp, MIO_EQN_OPER_MARK );
            // after an opening bracket, it feels like starting over again
            Flag = MIO_EQN_FLAG_START; 
            break;
        case MIO_EQN_SYM_CLOSE:
            if ( Vec_IntSize( pStackOp ) != 0 )
            {
                while ( 1 )
                {
                    if ( Vec_IntSize( pStackOp ) == 0 )
                    {
                        fprintf( stdout, "Mio_ParseFormula(): There is no opening parenthesis\n" );
                        Flag = MIO_EQN_FLAG_ERROR; 
                        break;
                    }
                    Oper = Vec_IntPop( pStackOp );
                    if ( Oper == MIO_EQN_OPER_MARK )
                        break;

                    // perform the given operation
                    if ( Mio_ParseFormulaOper( pMan, nVars, pStackFn, Oper ) == NULL )
                    {
                        fprintf( stdout, "Mio_ParseFormula(): Unknown operation\n" );
                        ABC_FREE( pFormula );
                        Vec_PtrFreeP( &pStackFn );
                        Vec_IntFreeP( &pStackOp );
                        return NULL;
                    }
                }
            }
            else
            {
                fprintf( stdout, "Mio_ParseFormula(): There is no opening parenthesis\n" );
                Flag = MIO_EQN_FLAG_ERROR; 
                break;
            }
            if ( Flag != MIO_EQN_FLAG_ERROR )
                Flag = MIO_EQN_FLAG_VAR; 
            break;


        default:
            // scan the next name
            for ( i = 0; pTemp[i] && 
                         pTemp[i] != ' ' && pTemp[i] != '\t' && pTemp[i] != '\r' && pTemp[i] != '\n' &&
                         pTemp[i] != MIO_EQN_SYM_AND && pTemp[i] != MIO_EQN_SYM_AND2 && pTemp[i] != MIO_EQN_SYM_OR && pTemp[i] != MIO_EQN_SYM_OR2 && 
                         pTemp[i] != MIO_EQN_SYM_XOR && pTemp[i] != MIO_EQN_SYM_NEGAFT && pTemp[i] != MIO_EQN_SYM_CLOSE; 
                  i++ )
              {
                    if ( pTemp[i] == MIO_EQN_SYM_NEG || pTemp[i] == MIO_EQN_SYM_OPEN )
                    {
                        fprintf( stdout, "Mio_ParseFormula(): The negation sign or an opening parenthesis inside the variable name.\n" );
                        Flag = MIO_EQN_FLAG_ERROR; 
                        break;
                    }
              }
            // variable name is found
            fFound = 0;
//            Vec_PtrForEachEntry( char *, vVarNames, pName, v )
            for ( v = 0; v < nVars; v++ )
            {
                pName = ppVarNames[v];
                if ( strncmp(pTemp, pName, i) == 0 && strlen(pName) == (unsigned)i )
                {
                    pTemp += i-1;
                    fFound = 1;
                    break;
                }
            }
            if ( !fFound )
            { 
                fprintf( stdout, "Mio_ParseFormula(): The parser cannot find var \"%s\" in the input var list.\n", pTemp ); 
                Flag = MIO_EQN_FLAG_ERROR; 
                break; 
            }
/*
            if ( Flag == MIO_EQN_FLAG_VAR )
            {
                fprintf( stdout, "Mio_ParseFormula(): The variable name \"%s\" follows another var without operation sign.\n", pTemp ); 
                Flag = MIO_EQN_FLAG_ERROR; 
                break; 
            }
*/
            if ( Flag == MIO_EQN_FLAG_VAR )
                Vec_IntPush( pStackOp, MIO_EQN_OPER_AND );

            Vec_PtrPush( pStackFn, Exp_Var(v) ); // Cudd_Ref( pbVars[v] );
            Flag = MIO_EQN_FLAG_VAR; 
            break;
        }

        if ( Flag == MIO_EQN_FLAG_ERROR )
            break;      // error exit
        else if ( Flag == MIO_EQN_FLAG_START )
            continue;  //  go on parsing
        else if ( Flag == MIO_EQN_FLAG_VAR )
            while ( 1 )
            {  // check if there are negations in the OpStack     
                if ( Vec_IntSize( pStackOp ) == 0 )
                    break;
                Oper = Vec_IntPop( pStackOp );
                if ( Oper != MIO_EQN_OPER_NEG )
                {
                    Vec_IntPush( pStackOp, Oper );
                    break;
                }
                else
                {
                      Vec_PtrPush( pStackFn, Exp_Not((Vec_Int_t *)Vec_PtrPop(pStackFn)) );
                }
            }
        else // if ( Flag == MIO_EQN_FLAG_OPER )
            while ( 1 )
            {  // execute all the operations in the OpStack
               // with precedence higher or equal than the last one
                Oper1 = Vec_IntPop( pStackOp ); // the last operation
                if ( Vec_IntSize( pStackOp ) == 0 ) 
                {  // if it is the only operation, push it back
                    Vec_IntPush( pStackOp, Oper1 );
                    break;
                }
                Oper2 = Vec_IntPop( pStackOp ); // the operation before the last one
                if ( Oper2 >= Oper1 )  
                {  // if Oper2 precedence is higher or equal, execute it
                    if ( Mio_ParseFormulaOper( pMan, nVars, pStackFn, Oper2 ) == NULL )
                    {
                        fprintf( stdout, "Mio_ParseFormula(): Unknown operation\n" );
                        ABC_FREE( pFormula );
                        Vec_PtrFreeP( &pStackFn );
                        Vec_IntFreeP( &pStackOp );
                        return NULL;
                    }
                    Vec_IntPush( pStackOp,  Oper1 );     // push the last operation back
                }
                else
                {  // if Oper2 precedence is lower, push them back and done
                    Vec_IntPush( pStackOp, Oper2 );
                    Vec_IntPush( pStackOp, Oper1 );
                    break;
                }
            }
    }

    if ( Flag != MIO_EQN_FLAG_ERROR )
    {
        if ( Vec_PtrSize(pStackFn) != 0 )
        {    
            gFunc = (Vec_Int_t *)Vec_PtrPop(pStackFn);
            if ( Vec_PtrSize(pStackFn) == 0 )
                if ( Vec_IntSize( pStackOp ) == 0 )
                {
//                    Cudd_Deref( gFunc );
                    ABC_FREE( pFormula );
                    Vec_PtrFreeP( &pStackFn );
                    Vec_IntFreeP( &pStackOp );
                    return Exp_Reverse(gFunc);
                }
                else
                    fprintf( stdout, "Mio_ParseFormula(): Something is left in the operation stack\n" );
            else
                fprintf( stdout, "Mio_ParseFormula(): Something is left in the function stack\n" );
        }
        else
            fprintf( stdout, "Mio_ParseFormula(): The input string is empty\n" );
    }
    ABC_FREE( pFormula );
    Vec_PtrFreeP( &pStackFn );
    Vec_IntFreeP( &pStackOp );
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Derives the TT corresponding to the equation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Wrd_t * Mio_ParseFormulaTruth( char * pFormInit, char ** ppVarNames, int nVars )
{
    Vec_Int_t * vExpr;
    Vec_Wrd_t * vTruth;
    // derive expression
    vExpr = Mio_ParseFormula( pFormInit, ppVarNames, nVars );
    if ( vExpr == NULL )
        return NULL;
    // convert it into a truth table
    vTruth = Vec_WrdStart( Abc_Truth6WordNum(nVars) );
    Exp_Truth( nVars, vExpr, Vec_WrdArray(vTruth) );
    Vec_IntFree( vExpr );
    return vTruth;
}
void Mio_ParseFormulaTruthTest( char * pFormInit, char ** ppVarNames, int nVars )
{
    Vec_Wrd_t * vTruth;
    vTruth = Mio_ParseFormulaTruth( pFormInit, ppVarNames, nVars );
//    Kit_DsdPrintFromTruth( (unsigned *)Vec_WrdArray(vTruth), nVars ); printf( "\n" );
    Vec_WrdFree( vTruth );
}

/**Function*************************************************************

  Synopsis    [Checks if the gate's formula essentially depends on all variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mio_ParseCheckName( Mio_Gate_t * pGate, char ** ppStr )
{
    Mio_Pin_t * pPin;
    int i, iBest = -1;
    // find the longest pin name that matches substring
    char * pNameBest = NULL;
    for ( pPin = Mio_GateReadPins(pGate), i = 0; pPin; pPin = Mio_PinReadNext(pPin), i++ )
        if ( !strncmp( *ppStr, Mio_PinReadName(pPin), strlen(Mio_PinReadName(pPin)) ) )
            if ( pNameBest == NULL || strlen(pNameBest) < strlen(Mio_PinReadName(pPin)) )
                pNameBest = Mio_PinReadName(pPin), iBest = i;
    // if pin is not found, return -1
    if ( pNameBest )
        *ppStr += strlen(pNameBest) - 1;
    return iBest;
}
int Mio_ParseCheckFormula( Mio_Gate_t * pGate, char * pForm )
{
    Mio_Pin_t * pPin;
    char * pStr;
    int i, iPin, fVisit[32] = {0};
    if ( Mio_GateReadPins(pGate) == NULL || !strcmp(Mio_PinReadName(Mio_GateReadPins(pGate)), "*") )
        return 1;
/*
    // find the equality sign
    pForm = strstr( pForm, "=" );
    if ( pForm == NULL )
    {
        printf( "Skipping gate \"%s\" because formula \"%s\" has not equality sign (=).\n", pGate->pName, pForm );
        return 0;
    }
*/
//printf( "Checking gate %s\n", pGate->pName );

    for ( pStr = pForm; *pStr; pStr++ )
    {
        if ( *pStr == ' ' ||
             *pStr == MIO_EQN_SYM_OPEN ||   
             *pStr == MIO_EQN_SYM_CLOSE ||  
             *pStr == MIO_EQN_SYM_CONST0 || 
             *pStr == MIO_EQN_SYM_CONST1 || 
             *pStr == MIO_EQN_SYM_NEG ||    
             *pStr == MIO_EQN_SYM_NEGAFT || 
             *pStr == MIO_EQN_SYM_AND ||    
             *pStr == MIO_EQN_SYM_AND2 ||   
             *pStr == MIO_EQN_SYM_XOR ||    
             *pStr == MIO_EQN_SYM_OR ||     
             *pStr == MIO_EQN_SYM_OR2 
           )
           continue;
        // return the number of the pin which has this name
        iPin = Mio_ParseCheckName( pGate, &pStr );
        if ( iPin == -1 )
        {
            printf( "Skipping gate \"%s\" because substring \"%s\" does not match with a pin name.\n", pGate->pName, pStr );
            return 0;
        }
        assert( iPin < 32 );
        fVisit[iPin] = 1;
    }
    // check that all pins are used
    for ( pPin = Mio_GateReadPins(pGate), i = 0; pPin; pPin = Mio_PinReadNext(pPin), i++ )
        if ( fVisit[i] == 0 )
        {
//            printf( "Skipping gate \"%s\" because pin \"%s\" does not appear in the formula \"%s\".\n", pGate->pName, Mio_PinReadName(pPin), pForm );
            return 0;
        }
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

