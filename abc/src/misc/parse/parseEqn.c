/**CFile****************************************************************

  FileNameIn  [parseEqn.c]

  PackageName [ABC: Logic synthesis and verification system.]

  Synopsis    [Boolean formula parser.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - December 18, 2006.]

  Revision    [$Id: parseEqn.c,v 1.0 2006/12/18 00:00:00 alanmi Exp $]

***********************************************************************/

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#include "parseInt.h"

ABC_NAMESPACE_IMPL_START


// the list of operation symbols to be used in expressions
#define PARSE_EQN_SYM_OPEN    '('   // opening parenthesis
#define PARSE_EQN_SYM_CLOSE   ')'   // closing parenthesis
#define PARSE_EQN_SYM_CONST0  '0'   // constant 0
#define PARSE_EQN_SYM_CONST1  '1'   // constant 1
#define PARSE_EQN_SYM_NEG     '!'   // negation before the variable
#define PARSE_EQN_SYM_AND     '*'   // logic AND
#define PARSE_EQN_SYM_OR      '+'   // logic OR

// the list of opcodes (also specifying operation precedence)
#define PARSE_EQN_OPER_NEG    10    // negation
#define PARSE_EQN_OPER_AND     9    // logic AND
#define PARSE_EQN_OPER_OR      7    // logic OR
#define PARSE_EQN_OPER_MARK    1    // OpStack token standing for an opening parenthesis

// these are values of the internal Flag
#define PARSE_EQN_FLAG_START   1    // after the opening parenthesis 
#define PARSE_EQN_FLAG_VAR     2    // after operation is received
#define PARSE_EQN_FLAG_OPER    3    // after operation symbol is received
#define PARSE_EQN_FLAG_ERROR   4    // when error is detected

#define PARSE_EQN_STACKSIZE 1000

static Hop_Obj_t * Parse_ParserPerformTopOp( Hop_Man_t * pMan, Parse_StackFn_t * pStackFn, int Oper );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Derives the AIG corresponding to the equation.]

  Description [Takes the stream to output messages, the formula, the vector
  of variable names and the AIG manager.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Parse_FormulaParserEqn( FILE * pOutput, char * pFormInit, Vec_Ptr_t * vVarNames, Hop_Man_t * pMan )
{
    char * pFormula;
    Parse_StackFn_t * pStackFn;
    Parse_StackOp_t * pStackOp;
    Hop_Obj_t * gFunc;
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
        fprintf( pOutput, "Parse_FormulaParserEqn(): Different number of opening and closing parentheses ().\n" );
        return NULL;
    }

    // copy the formula
    pFormula = ABC_ALLOC( char, strlen(pFormInit) + 3 );
    sprintf( pFormula, "(%s)", pFormInit );

    // start the stacks
    pStackFn = Parse_StackFnStart( PARSE_EQN_STACKSIZE );
    pStackOp = Parse_StackOpStart( PARSE_EQN_STACKSIZE );

    Flag = PARSE_EQN_FLAG_START;
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
		case PARSE_EQN_SYM_CONST0:
		    Parse_StackFnPush( pStackFn, Hop_ManConst0(pMan) );  // Cudd_Ref( b0 );
			if ( Flag == PARSE_EQN_FLAG_VAR )
			{
				fprintf( pOutput, "Parse_FormulaParserEqn(): No operation symbol before constant 0.\n" );
				Flag = PARSE_EQN_FLAG_ERROR; 
                break;
			}
            Flag = PARSE_EQN_FLAG_VAR; 
            break;
		case PARSE_EQN_SYM_CONST1:
		    Parse_StackFnPush( pStackFn, Hop_ManConst1(pMan) );  //  Cudd_Ref( b1 );
			if ( Flag == PARSE_EQN_FLAG_VAR )
			{
				fprintf( pOutput, "Parse_FormulaParserEqn(): No operation symbol before constant 1.\n" );
				Flag = PARSE_EQN_FLAG_ERROR; 
                break;
			}
            Flag = PARSE_EQN_FLAG_VAR; 
            break;
		case PARSE_EQN_SYM_NEG:
			if ( Flag == PARSE_EQN_FLAG_VAR )
			{// if NEGBEF follows a variable, AND is assumed
				Parse_StackOpPush( pStackOp, PARSE_EQN_OPER_AND );
				Flag = PARSE_EQN_FLAG_OPER;
			}
    		Parse_StackOpPush( pStackOp, PARSE_EQN_OPER_NEG );
			break;
        case PARSE_EQN_SYM_AND:
        case PARSE_EQN_SYM_OR:
			if ( Flag != PARSE_EQN_FLAG_VAR )
			{
				fprintf( pOutput, "Parse_FormulaParserEqn(): There is no variable before AND, EXOR, or OR.\n" );
				Flag = PARSE_EQN_FLAG_ERROR; 
                break;
			}
			if ( *pTemp == PARSE_EQN_SYM_AND )
				Parse_StackOpPush( pStackOp, PARSE_EQN_OPER_AND );
			else //if ( *pTemp == PARSE_EQN_SYM_OR )
				Parse_StackOpPush( pStackOp, PARSE_EQN_OPER_OR );
			Flag = PARSE_EQN_FLAG_OPER; 
            break;
		case PARSE_EQN_SYM_OPEN:
			if ( Flag == PARSE_EQN_FLAG_VAR )
            {
//				Parse_StackOpPush( pStackOp, PARSE_EQN_OPER_AND );
				fprintf( pOutput, "Parse_FormulaParserEqn(): An opening parenthesis follows a var without operation sign.\n" ); 
				Flag = PARSE_EQN_FLAG_ERROR; 
                break; 
            }
			Parse_StackOpPush( pStackOp, PARSE_EQN_OPER_MARK );
			// after an opening bracket, it feels like starting over again
			Flag = PARSE_EQN_FLAG_START; 
            break;
		case PARSE_EQN_SYM_CLOSE:
			if ( !Parse_StackOpIsEmpty( pStackOp ) )
            {
				while ( 1 )
			    {
				    if ( Parse_StackOpIsEmpty( pStackOp ) )
					{
						fprintf( pOutput, "Parse_FormulaParserEqn(): There is no opening parenthesis\n" );
						Flag = PARSE_EQN_FLAG_ERROR; 
                        break;
					}
					Oper = Parse_StackOpPop( pStackOp );
					if ( Oper == PARSE_EQN_OPER_MARK )
						break;

                    // perform the given operation
                    if ( Parse_ParserPerformTopOp( pMan, pStackFn, Oper ) == NULL )
	                {
                        Parse_StackFnFree( pStackFn );
                        Parse_StackOpFree( pStackOp );
		                fprintf( pOutput, "Parse_FormulaParserEqn(): Unknown operation\n" );
                        ABC_FREE( pFormula );
		                return NULL;
	                }
			    }
            }
		    else
			{
				fprintf( pOutput, "Parse_FormulaParserEqn(): There is no opening parenthesis\n" );
				Flag = PARSE_EQN_FLAG_ERROR; 
                break;
			}
			if ( Flag != PARSE_EQN_FLAG_ERROR )
			    Flag = PARSE_EQN_FLAG_VAR; 
			break;


		default:
            // scan the next name
            for ( i = 0; pTemp[i] && 
                         pTemp[i] != ' ' && pTemp[i] != '\t' && pTemp[i] != '\r' && pTemp[i] != '\n' &&
                         pTemp[i] != PARSE_EQN_SYM_AND && pTemp[i] != PARSE_EQN_SYM_OR && pTemp[i] != PARSE_EQN_SYM_CLOSE; i++ )
              {
				    if ( pTemp[i] == PARSE_EQN_SYM_NEG || pTemp[i] == PARSE_EQN_SYM_OPEN )
				    {
					    fprintf( pOutput, "Parse_FormulaParserEqn(): The negation sign or an opening parenthesis inside the variable name.\n" );
					    Flag = PARSE_EQN_FLAG_ERROR; 
                        break;
				    }
              }
            // variable name is found
            fFound = 0;
            Vec_PtrForEachEntry( char *, vVarNames, pName, v )
                if ( strncmp(pTemp, pName, i) == 0 && strlen(pName) == (unsigned)i )
                {
                    pTemp += i-1;
                    fFound = 1;
                    break;
                }
            if ( !fFound )
			{ 
				fprintf( pOutput, "Parse_FormulaParserEqn(): The parser cannot find var \"%s\" in the input var list.\n", pTemp ); 
				Flag = PARSE_EQN_FLAG_ERROR; 
                break; 
			}
			if ( Flag == PARSE_EQN_FLAG_VAR )
            {
				fprintf( pOutput, "Parse_FormulaParserEqn(): The variable name \"%s\" follows another var without operation sign.\n", pTemp ); 
				Flag = PARSE_EQN_FLAG_ERROR; 
                break; 
            }
			Parse_StackFnPush( pStackFn, Hop_IthVar( pMan, v ) ); // Cudd_Ref( pbVars[v] );
            Flag = PARSE_EQN_FLAG_VAR; 
            break;
	    }

		if ( Flag == PARSE_EQN_FLAG_ERROR )
			break;      // error exit
		else if ( Flag == PARSE_EQN_FLAG_START )
			continue;  //  go on parsing
		else if ( Flag == PARSE_EQN_FLAG_VAR )
			while ( 1 )
			{  // check if there are negations in the OpStack     
				if ( Parse_StackOpIsEmpty(pStackOp) )
					break;
                Oper = Parse_StackOpPop( pStackOp );
				if ( Oper != PARSE_EQN_OPER_NEG )
                {
					Parse_StackOpPush( pStackOp, Oper );
					break;
                }
				else
				{
      				Parse_StackFnPush( pStackFn, Hop_Not((Hop_Obj_t *)Parse_StackFnPop(pStackFn)) );
				}
			}
		else // if ( Flag == PARSE_EQN_FLAG_OPER )
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
                    if ( Parse_ParserPerformTopOp( pMan, pStackFn, Oper2 ) == NULL )
	                {
		                fprintf( pOutput, "Parse_FormulaParserEqn(): Unknown operation\n" );
                        ABC_FREE( pFormula );
                        Parse_StackFnFree( pStackFn );
                        Parse_StackOpFree( pStackOp );
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

	if ( Flag != PARSE_EQN_FLAG_ERROR )
    {
		if ( !Parse_StackFnIsEmpty(pStackFn) )
	    {	
			gFunc = (Hop_Obj_t *)Parse_StackFnPop(pStackFn);
			if ( Parse_StackFnIsEmpty(pStackFn) )
				if ( Parse_StackOpIsEmpty(pStackOp) )
                {
                    Parse_StackFnFree(pStackFn);
                    Parse_StackOpFree(pStackOp);
//                    Cudd_Deref( gFunc );
                    ABC_FREE( pFormula );
					return gFunc;
                }
				else
					fprintf( pOutput, "Parse_FormulaParserEqn(): Something is left in the operation stack\n" );
			else
				fprintf( pOutput, "Parse_FormulaParserEqn(): Something is left in the function stack\n" );
	    }
	    else
			fprintf( pOutput, "Parse_FormulaParserEqn(): The input string is empty\n" );
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
Hop_Obj_t * Parse_ParserPerformTopOp( Hop_Man_t * pMan, Parse_StackFn_t * pStackFn, int Oper )
{
    Hop_Obj_t * gArg1, * gArg2, * gFunc;
    // perform the given operation
    gArg2 = (Hop_Obj_t *)Parse_StackFnPop( pStackFn );
    gArg1 = (Hop_Obj_t *)Parse_StackFnPop( pStackFn );
	if ( Oper == PARSE_EQN_OPER_AND )
		gFunc = Hop_And( pMan, gArg1, gArg2 );
	else if ( Oper == PARSE_EQN_OPER_OR )
		gFunc = Hop_Or( pMan, gArg1, gArg2 );
	else
		return NULL;
//    Cudd_Ref( gFunc );
//    Cudd_RecursiveDeref( dd, gArg1 );
//    Cudd_RecursiveDeref( dd, gArg2 );
	Parse_StackFnPush( pStackFn,  gFunc );
    return gFunc;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
ABC_NAMESPACE_IMPL_END

