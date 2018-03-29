/**CFile****************************************************************

  FileNameIn  [parseCore.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Boolean formula parser.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: parseCore.c,v 1.0 2003/02/01 00:00:00 alanmi Exp $]

***********************************************************************/

/*
        Some aspects of Boolean Formula Parser:

 1) The names in the boolean formulas can be any strings of symbols
    that start with char or underscore and contain chars, digits 
    and underscores: For example: 1) a&b <+> c'&d => a + b;   
    2) a1 b2 c3' dummy' + (a2+b2')c3 dummy
 2) Constant values 0 and 1 can be used just like normal variables
 3) Any boolean operator (listed below) and parentheses can be used
    any number of times provided there are equal number of opening
    and closing parentheses.
 4) By default, absence of an operator between vars and before and 
    after parentheses is taken for AND. 
 5) Both complementation prefix and complementation suffix can be 
    used at the same time (but who needs this?)
 6) Spaces (tabs, end-of-lines) may be inserted anywhere,
    except between characters of the operations: <=>, =>, <=, <+>
 7) The stack size is defined by macro STACKSIZE and is used by the 
    stack constructor.
*/
 
////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#include "parseInt.h"

ABC_NAMESPACE_IMPL_START


// the list of operation symbols to be used in expressions
#define PARSE_SYM_OPEN    '('   // opening parenthesis
#define PARSE_SYM_CLOSE   ')'   // closing parenthesis
#define PARSE_SYM_LOWER   '['   // shifts one rank down 
#define PARSE_SYM_RAISE   ']'   // shifts one rank up
#define PARSE_SYM_CONST0  '0'   // constant 0
#define PARSE_SYM_CONST1  '1'   // constant 1
#define PARSE_SYM_NEGBEF1 '!'   // negation before the variable
#define PARSE_SYM_NEGBEF2 '~'   // negation before the variable
#define PARSE_SYM_NEGAFT  '\''  // negation after the variable
#define PARSE_SYM_AND1    '&'   // logic AND
#define PARSE_SYM_AND2    '*'   // logic AND
#define PARSE_SYM_XOR1    '<'   // logic EXOR   (the 1st symbol) 
#define PARSE_SYM_XOR2    '+'   // logic EXOR   (the 2nd symbol)
#define PARSE_SYM_XOR3    '>'   // logic EXOR   (the 3rd symbol)
#define PARSE_SYM_XOR     '^'   // logic XOR
#define PARSE_SYM_OR1     '+'   // logic OR
#define PARSE_SYM_OR2     '|'   // logic OR
#define PARSE_SYM_EQU1    '<'   // equvalence   (the 1st symbol)
#define PARSE_SYM_EQU2    '='   // equvalence   (the 2nd symbol)
#define PARSE_SYM_EQU3    '>'   // equvalence   (the 3rd symbol)
#define PARSE_SYM_FLR1    '='   // implication  (the 1st symbol)
#define PARSE_SYM_FLR2    '>'   // implication  (the 2nd symbol)
#define PARSE_SYM_FLL1    '<'   // backward imp (the 1st symbol)
#define PARSE_SYM_FLL2    '='   // backward imp (the 2nd symbol)
// PARSE_SYM_FLR1 and PARSE_SYM_FLR2 should be the same as PARSE_SYM_EQU2 and PARSE_SYM_EQU3!

// the list of opcodes (also specifying operation precedence)
#define PARSE_OPER_NEG 10  // negation
#define PARSE_OPER_AND  9  // logic AND
#define PARSE_OPER_XOR  8  // logic EXOR   (a'b | ab')   
#define PARSE_OPER_OR   7  // logic OR
#define PARSE_OPER_EQU  6  // equvalence   (a'b'| ab )
#define PARSE_OPER_FLR  5  // implication  ( a' | b )
#define PARSE_OPER_FLL  4  // backward imp ( 'b | a )
#define PARSE_OPER_MARK 1  // OpStack token standing for an opening parenthesis

// these are values of the internal Flag
#define PARSE_FLAG_START  1 // after the opening parenthesis 
#define PARSE_FLAG_VAR    2 // after operation is received
#define PARSE_FLAG_OPER   3 // after operation symbol is received
#define PARSE_FLAG_ERROR  4 // when error is detected

#define STACKSIZE 1000

static DdNode * Parse_ParserPerformTopOp( DdManager * dd, Parse_StackFn_t * pStackFn, int Oper );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Derives the BDD corresponding to the formula in language L.]

  Description [Takes the stream to output messages, the formula, the number
  variables and the rank in the formula. The array of variable names is also 
  given. The BDD manager and the elementary 0-rank variable are the last two
  arguments. The manager should have at least as many variables as 
  nVars * (nRanks + 1). The 0-rank variables should have numbers larger 
  than the variables of other ranks.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Parse_FormulaParser( FILE * pOutput, char * pFormulaInit, int nVars, int nRanks, 
      char * ppVarNames[], DdManager * dd, DdNode * pbVars[] )
{
    char * pFormula;
    Parse_StackFn_t * pStackFn;
    Parse_StackOp_t * pStackOp;
    DdNode * bFunc, * bTemp;
    char * pTemp;
    int nParans, fFound, Flag;
    int Oper, Oper1, Oper2;
    int i, fLower;
    int v = -1; // Suppress "might be used uninitialized"

    // make sure that the number of vars and ranks is correct
    if ( nVars * (nRanks + 1) > dd->size )
    {
        printf( "Parse_FormulaParser(): The BDD manager does not have enough variables.\n" );
        return NULL;
    }

    // make sure that the number of opening and closing parentheses is the same
    nParans = 0;
    for ( pTemp = pFormulaInit; *pTemp; pTemp++ )
        if ( *pTemp == '(' )
            nParans++;
        else if ( *pTemp == ')' )
            nParans--;
    if ( nParans != 0 )
    {
        fprintf( pOutput, "Parse_FormulaParser(): Different number of opening and closing parentheses ().\n" );
        return NULL;
    }

    nParans = 0;
    for ( pTemp = pFormulaInit; *pTemp; pTemp++ )
        if ( *pTemp == '[' )
            nParans++;
        else if ( *pTemp == ']' )
            nParans--;
    if ( nParans != 0 )
    {
        fprintf( pOutput, "Parse_FormulaParser(): Different number of opening and closing brackets [].\n" );
        return NULL;
    }

    // copy the formula
    pFormula = ABC_ALLOC( char, strlen(pFormulaInit) + 3 );
    sprintf( pFormula, "(%s)", pFormulaInit );

    // start the stacks
    pStackFn = Parse_StackFnStart( STACKSIZE );
    pStackOp = Parse_StackOpStart( STACKSIZE );

    Flag = PARSE_FLAG_START;
    fLower = 0;
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

        // treat Constant 0 as a variable
        case PARSE_SYM_CONST0:
            Parse_StackFnPush( pStackFn, b0 );   Cudd_Ref( b0 );
            if ( Flag == PARSE_FLAG_VAR )
            {
                fprintf( pOutput, "Parse_FormulaParser(): No operation symbol before constant 0.\n" );
                Flag = PARSE_FLAG_ERROR; 
                break;
            }
            Flag = PARSE_FLAG_VAR; 
            break;

        // the same for Constant 1
        case PARSE_SYM_CONST1:
            Parse_StackFnPush( pStackFn, b1 );    Cudd_Ref( b1 );
            if ( Flag == PARSE_FLAG_VAR )
            {
                fprintf( pOutput, "Parse_FormulaParser(): No operation symbol before constant 1.\n" );
                Flag = PARSE_FLAG_ERROR; 
                break;
            }
            Flag = PARSE_FLAG_VAR; 
            break;

        case PARSE_SYM_NEGBEF1:
        case PARSE_SYM_NEGBEF2:
            if ( Flag == PARSE_FLAG_VAR )
            {// if NEGBEF follows a variable, AND is assumed
                Parse_StackOpPush( pStackOp, PARSE_OPER_AND );
                Flag = PARSE_FLAG_OPER;
            }
            Parse_StackOpPush( pStackOp, PARSE_OPER_NEG );
            break;

        case PARSE_SYM_NEGAFT:
            if ( Flag != PARSE_FLAG_VAR )
            {// if there is no variable before NEGAFT, it is an error
                fprintf( pOutput, "Parse_FormulaParser(): No variable is specified before the negation suffix.\n" );
                Flag = PARSE_FLAG_ERROR; 
                break;
            }
            else // if ( Flag == PARSE_FLAG_VAR )
                Parse_StackFnPush( pStackFn, Cudd_Not( Parse_StackFnPop(pStackFn) ) );
            break;

        case PARSE_SYM_AND1:
        case PARSE_SYM_AND2:
        case PARSE_SYM_OR1:
        case PARSE_SYM_OR2:
        case PARSE_SYM_XOR:
            if ( Flag != PARSE_FLAG_VAR )
            {
                fprintf( pOutput, "Parse_FormulaParser(): There is no variable before AND, EXOR, or OR.\n" );
                Flag = PARSE_FLAG_ERROR; 
                break;
            }
            if ( *pTemp == PARSE_SYM_AND1 || *pTemp == PARSE_SYM_AND2 )
                Parse_StackOpPush( pStackOp, PARSE_OPER_AND );
            else if ( *pTemp == PARSE_SYM_OR1 || *pTemp == PARSE_SYM_OR2 )
                Parse_StackOpPush( pStackOp, PARSE_OPER_OR );
            else //if ( Str[Pos] == PARSE_SYM_XOR )
                Parse_StackOpPush( pStackOp, PARSE_OPER_XOR );
            Flag = PARSE_FLAG_OPER; 
            break;

        case PARSE_SYM_EQU1:
            if ( Flag != PARSE_FLAG_VAR )
            { 
                fprintf( pOutput, "Parse_FormulaParser(): There is no variable before Equivalence or Implication\n" );
                Flag = PARSE_FLAG_ERROR; break;
            }
            if ( pTemp[1] == PARSE_SYM_EQU2 )
            { // check what is the next symbol in the string
                pTemp++; 
                if ( pTemp[1] == PARSE_SYM_EQU3 )
                {   
                    pTemp++; 
                    Parse_StackOpPush( pStackOp, PARSE_OPER_EQU ); 
                }
                else
                {   
                    Parse_StackOpPush( pStackOp, PARSE_OPER_FLL ); 
                }
            }
            else if ( pTemp[1] == PARSE_SYM_XOR2 )
            {
                pTemp++; 
                if ( pTemp[1] == PARSE_SYM_XOR3 )
                {   
                    pTemp++; 
                    Parse_StackOpPush( pStackOp, PARSE_OPER_XOR ); 
                }
                else
                {   
                    fprintf( pOutput, "Parse_FormulaParser(): Wrong symbol after \"%c%c\"\n", PARSE_SYM_EQU1, PARSE_SYM_XOR2 );
                    Flag = PARSE_FLAG_ERROR; 
                    break;
                }
            }
            else
            {
                fprintf( pOutput, "Parse_FormulaParser(): Wrong symbol after \"%c\"\n", PARSE_SYM_EQU1 );
                Flag = PARSE_FLAG_ERROR; 
                break;
            }
            Flag = PARSE_FLAG_OPER; 
            break;

        case PARSE_SYM_EQU2:
            if ( Flag != PARSE_FLAG_VAR )
            {
                fprintf( pOutput, "Parse_FormulaParser(): There is no variable before Reverse Implication\n" );
                Flag = PARSE_FLAG_ERROR; 
                break;
            }
            if ( pTemp[1] == PARSE_SYM_EQU3 )
            {   
                pTemp++; 
                Parse_StackOpPush( pStackOp, PARSE_OPER_FLR ); 
            }
            else
            {
                fprintf( pOutput, "Parse_FormulaParser(): Wrong symbol after \"%c\"\n", PARSE_SYM_EQU2 );
                Flag = PARSE_FLAG_ERROR; 
                break;
            }
            Flag = PARSE_FLAG_OPER; 
            break;

        case PARSE_SYM_LOWER:
        case PARSE_SYM_OPEN:
            if ( Flag == PARSE_FLAG_VAR )
                Parse_StackOpPush( pStackOp, PARSE_OPER_AND );
            Parse_StackOpPush( pStackOp, PARSE_OPER_MARK );
            // after an opening bracket, it feels like starting over again
            Flag = PARSE_FLAG_START; 
            break;

        case PARSE_SYM_RAISE:
            fLower = 1;
        case PARSE_SYM_CLOSE:
            if ( !Parse_StackOpIsEmpty( pStackOp ) )
            {
                while ( 1 )
                {
                    if ( Parse_StackOpIsEmpty( pStackOp ) )
                    {
                        fprintf( pOutput, "Parse_FormulaParser(): There is no opening parenthesis\n" );
                        Flag = PARSE_FLAG_ERROR; 
                        break;
                    }
                    Oper = Parse_StackOpPop( pStackOp );
                    if ( Oper == PARSE_OPER_MARK )
                        break;

                    // perform the given operation
                    if ( Parse_ParserPerformTopOp( dd, pStackFn, Oper ) == NULL )
                    {
                        fprintf( pOutput, "Parse_FormulaParser(): Unknown operation\n" );
                        ABC_FREE( pFormula );
                        return NULL;
                    }
                }

                if ( fLower )
                {
                    bFunc = (DdNode *)Parse_StackFnPop( pStackFn );
                    bFunc = Extra_bddMove( dd, bTemp = bFunc, -nVars );   Cudd_Ref( bFunc );
                    Cudd_RecursiveDeref( dd, bTemp );
                    Parse_StackFnPush( pStackFn, bFunc );
                }
            }
            else
            {
                fprintf( pOutput, "Parse_FormulaParser(): There is no opening parenthesis\n" );
                Flag = PARSE_FLAG_ERROR; 
                break;
            }
            if ( Flag != PARSE_FLAG_ERROR )
                Flag = PARSE_FLAG_VAR; 
            fLower = 0;
            break;


        default:
            // scan the next name
/*
            fFound = 0;
            for ( i = 0; pTemp[i] && pTemp[i] != ' ' && pTemp[i] != '\t' && pTemp[i] != '\r' && pTemp[i] != '\n'; i++ )
            {
                for ( v = 0; v < nVars; v++ )
                    if ( strncmp( pTemp, ppVarNames[v], i+1 ) == 0 && strlen(ppVarNames[v]) == (unsigned)(i+1) )
                    {
                        pTemp += i;
                        fFound = 1;
                        break;
                    }
                if ( fFound )
                    break;
            }
*/ 
            // bug fix by SV (9/11/08)
            fFound = 0;
            for ( i = 0; pTemp[i] && pTemp[i] != ' ' && pTemp[i] != '\t' && pTemp[i] != '\r' && pTemp[i] != '\n' && 
                         pTemp[i] != PARSE_SYM_AND1 && pTemp[i] != PARSE_SYM_AND2 && pTemp[i] != PARSE_SYM_XOR1 &&
                         pTemp[i] != PARSE_SYM_XOR2 && pTemp[i] != PARSE_SYM_XOR3 && pTemp[i] != PARSE_SYM_XOR  &&
                         pTemp[i] != PARSE_SYM_OR1  && pTemp[i] != PARSE_SYM_OR2  && pTemp[i] != PARSE_SYM_CLOSE &&
                         pTemp[i] != PARSE_SYM_NEGAFT;
                  i++ )
            {}
            for ( v = 0; v < nVars; v++ ) 
            {
                if ( strncmp( pTemp, ppVarNames[v], i ) == 0 && strlen(ppVarNames[v]) == (unsigned)(i) )
                {
                    pTemp += i-1;
                    fFound = 1;
                    break;
                }
            }

            if ( !fFound )
            { 
                fprintf( pOutput, "Parse_FormulaParser(): The parser cannot find var \"%s\" in the input var list.\n", pTemp ); 
                Flag = PARSE_FLAG_ERROR; 
                break; 
            }
            // assume operation AND, if vars follow one another
            if ( Flag == PARSE_FLAG_VAR )
                Parse_StackOpPush( pStackOp, PARSE_OPER_AND );
            Parse_StackFnPush( pStackFn, pbVars[v] );  Cudd_Ref( pbVars[v] );
            Flag = PARSE_FLAG_VAR; 
            break;
        }

        if ( Flag == PARSE_FLAG_ERROR )
            break;      // error exit
        else if ( Flag == PARSE_FLAG_START )
            continue;  //  go on parsing
        else if ( Flag == PARSE_FLAG_VAR )
            while ( 1 )
            {  // check if there are negations in the OpStack     
                if ( Parse_StackOpIsEmpty(pStackOp) )
                    break;
                Oper = Parse_StackOpPop( pStackOp );
                if ( Oper != PARSE_OPER_NEG )
                {
                    Parse_StackOpPush( pStackOp, Oper );
                    break;
                }
                else
                {
                      Parse_StackFnPush( pStackFn, Cudd_Not(Parse_StackFnPop(pStackFn)) );
                }
            }
        else // if ( Flag == PARSE_FLAG_OPER )
            while ( 1 )
            {  // execute all the operations in the OpStack
               // with precedence higher or equal than the last one
                Oper1 = Parse_StackOpPop( pStackOp ); // the last operation
                if ( Parse_StackOpIsEmpty(pStackOp) ) 
                {  // if it is the only operation, push it back
                    Parse_StackOpPush( pStackOp, Oper1 );
                    break;
                }
                Oper2 = Parse_StackOpPop( pStackOp ); // the operation before the last one
                if ( Oper2 >= Oper1 )  
                {  // if Oper2 precedence is higher or equal, execute it
//                    Parse_StackPush( pStackFn,  Operation( FunStack.Pop(), FunStack.Pop(), Oper2 ) );
                    if ( Parse_ParserPerformTopOp( dd, pStackFn, Oper2 ) == NULL )
                    {
                        fprintf( pOutput, "Parse_FormulaParser(): Unknown operation\n" );
                        ABC_FREE( pFormula );
                        return NULL;
                    }
                    Parse_StackOpPush( pStackOp,  Oper1 );     // push the last operation back
                }
                else
                {  // if Oper2 precedence is lower, push them back and done
                    Parse_StackOpPush( pStackOp, Oper2 );
                    Parse_StackOpPush( pStackOp, Oper1 );
                    break;
                }
            }
    }

    if ( Flag != PARSE_FLAG_ERROR )
    {
        if ( !Parse_StackFnIsEmpty(pStackFn) )
        {    
            bFunc = (DdNode *)Parse_StackFnPop(pStackFn);
            if ( Parse_StackFnIsEmpty(pStackFn) )
                if ( Parse_StackOpIsEmpty(pStackOp) )
                {
                    Parse_StackFnFree(pStackFn);
                    Parse_StackOpFree(pStackOp);
                    Cudd_Deref( bFunc );
                    ABC_FREE( pFormula );
                    return bFunc;
                }
                else
                    fprintf( pOutput, "Parse_FormulaParser(): Something is left in the operation stack\n" );
            else
                fprintf( pOutput, "Parse_FormulaParser(): Something is left in the function stack\n" );
        }
        else
            fprintf( pOutput, "Parse_FormulaParser(): The input string is empty\n" );
    }
    ABC_FREE( pFormula );
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Performs the operation on the top entries in the stack.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Parse_ParserPerformTopOp( DdManager * dd, Parse_StackFn_t * pStackFn, int Oper )
{
    DdNode * bArg1, * bArg2, * bFunc;
    // perform the given operation
    bArg2 = (DdNode *)Parse_StackFnPop( pStackFn );
    bArg1 = (DdNode *)Parse_StackFnPop( pStackFn );
    if ( Oper == PARSE_OPER_AND )
        bFunc = Cudd_bddAnd( dd, bArg1, bArg2 );
    else if ( Oper == PARSE_OPER_XOR )
        bFunc = Cudd_bddXor( dd, bArg1, bArg2 );
    else if ( Oper == PARSE_OPER_OR )
        bFunc = Cudd_bddOr( dd, bArg1, bArg2 );
    else if ( Oper == PARSE_OPER_EQU )
        bFunc = Cudd_bddXnor( dd, bArg1, bArg2 );
    else if ( Oper == PARSE_OPER_FLR )
        bFunc = Cudd_bddOr( dd, Cudd_Not(bArg1), bArg2 );
    else if ( Oper == PARSE_OPER_FLL )
        bFunc = Cudd_bddOr( dd, Cudd_Not(bArg2), bArg1 );
    else
        return NULL;
    Cudd_Ref( bFunc );
    Cudd_RecursiveDeref( dd, bArg1 );
    Cudd_RecursiveDeref( dd, bArg2 );
    Parse_StackFnPush( pStackFn,  bFunc );
    return bFunc;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
ABC_NAMESPACE_IMPL_END

