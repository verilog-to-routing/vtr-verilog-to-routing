/**CFile****************************************************************

  FileName    [mioFunc.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [File reading/writing for technology mapping.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mioFunc.c,v 1.4 2004/06/28 14:20:25 alanmi Exp $]

***********************************************************************/

#include "mioInt.h"
//#include "parse.h"
#include "exp.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// these symbols (and no other) can appear in the formulas
#define MIO_SYMB_AND    '*'
#define MIO_SYMB_AND2   '&'
#define MIO_SYMB_OR     '+'
#define MIO_SYMB_OR2    '|'
#define MIO_SYMB_XOR    '^'
#define MIO_SYMB_NOT    '!'
#define MIO_SYMB_AFTNOT '\''
#define MIO_SYMB_OPEN   '('
#define MIO_SYMB_CLOSE  ')'

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Registers the cube string with the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Mio_SopRegister( Mem_Flex_t * pMan, char * pName )
{
    char * pRegName;
    if ( pName == NULL ) return NULL;
    pRegName = Mem_FlexEntryFetch( pMan, strlen(pName) + 1 );
    strcpy( pRegName, pName );
    return pRegName;
}

/**Function*************************************************************

  Synopsis    [Collect the pin names in the formula.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mio_GateCollectNames( char * pFormula, char * pPinNames[] )
{
    char * Buffer;
    char * pTemp;
    int nPins, i;

    // save the formula as it was
    //strcpy( Buffer, pFormula );
    Buffer = Abc_UtilStrsav( pFormula );

    // remove the non-name symbols
    for ( pTemp = Buffer; *pTemp; pTemp++ )
        if ( *pTemp == MIO_SYMB_AND  || *pTemp == MIO_SYMB_AND2   || 
             *pTemp == MIO_SYMB_OR   || *pTemp == MIO_SYMB_OR2    || 
             *pTemp == MIO_SYMB_XOR  || 
             *pTemp == MIO_SYMB_NOT  || *pTemp == MIO_SYMB_AFTNOT ||
             *pTemp == MIO_SYMB_OPEN || *pTemp == MIO_SYMB_CLOSE )
             *pTemp = ' ';

    // save the names
    nPins = 0;
    pTemp = strtok( Buffer, " " );
    while ( pTemp )
    {
        for ( i = 0; i < nPins; i++ )
            if ( strcmp( pTemp, pPinNames[i] ) == 0 )
                break;
        if ( i == nPins )
        { // cannot find this name; save it
            pPinNames[nPins++] = Abc_UtilStrsav(pTemp);
        }
        // get the next name
        pTemp = strtok( NULL, " " );
    }
    ABC_FREE( Buffer );
    return nPins;
}

/**Function*************************************************************

  Synopsis    [Deriving the functionality of the gates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mio_GateParseFormula( Mio_Gate_t * pGate )
{
    char * pPinNames[100];
    char * pPinNamesCopy[100];
    Mio_Pin_t * pPin, ** ppPin;
    int nPins, iPin, i;

    // set the maximum delay of the gate; count pins
    pGate->dDelayMax = 0.0;
    nPins = 0;
    Mio_GateForEachPin( pGate, pPin )
    {
        // set the maximum delay of the gate
        if ( pGate->dDelayMax < pPin->dDelayBlockMax )
            pGate->dDelayMax = pPin->dDelayBlockMax;
        // count the pin
        nPins++;
    }

    // check for the gate with const function
    if ( nPins == 0 )
    {
        if ( strcmp( pGate->pForm, MIO_STRING_CONST0 ) == 0 )
        {
//            pGate->bFunc = b0;
            pGate->vExpr = Exp_Const0();
            pGate->pSop = Mio_SopRegister( (Mem_Flex_t *)pGate->pLib->pMmFlex, " 0\n" );
            pGate->uTruth = 0;
            pGate->pLib->pGate0 = pGate;
        }
        else if ( strcmp( pGate->pForm, MIO_STRING_CONST1 ) == 0 )
        {
//            pGate->bFunc = b1;
            pGate->vExpr = Exp_Const1();
            pGate->pSop = Mio_SopRegister( (Mem_Flex_t *)pGate->pLib->pMmFlex, " 1\n" );
            pGate->uTruth = ~(word)0;
            pGate->pLib->pGate1 = pGate;
        }
        else
        {
            printf( "Cannot parse formula \"%s\" of gate \"%s\".\n", pGate->pForm, pGate->pName );
            return 1;
        }
//        Cudd_Ref( pGate->bFunc );
        return 0;
    }

    // collect the names as they appear in the formula
    nPins = Mio_GateCollectNames( pGate->pForm, pPinNames );
    if ( nPins == 0 )
    {
        printf( "Cannot read formula \"%s\" of gate \"%s\".\n", pGate->pForm, pGate->pName );
        return 1;
    }

    // set the number of inputs
    pGate->nInputs = nPins;

    // consider the case when all the pins have identical pin info
    if ( strcmp( pGate->pPins->pName, "*" ) == 0 )
    {
        // get the topmost (generic) pin
        pPin = pGate->pPins;
        ABC_FREE( pPin->pName );

        // create individual pins from the generic pin
        ppPin = &pPin->pNext;
        for ( i = 1; i < nPins; i++ )
        {
            // get the new pin
            *ppPin = Mio_PinDup( pPin );
            // set its name
            (*ppPin)->pName = pPinNames[i];
            // prepare the next place in the list
            ppPin = &((*ppPin)->pNext);
        }
        *ppPin = NULL;

        // set the name of the topmost pin
        pPin->pName = pPinNames[0];
    }
    else
    {
        // reorder the variable names to appear the save way as the pins
        iPin = 0;
        Mio_GateForEachPin( pGate, pPin )
        {
            // find the pin with the name pPin->pName
            for ( i = 0; i < nPins; i++ )
            {
                if ( pPinNames[i] && strcmp( pPinNames[i], pPin->pName ) == 0 )
                {
                    // free pPinNames[i] because it is already available as pPin->pName
                    // setting pPinNames[i] to NULL is useful to make sure that
                    // this name is not assigned to two pins in the list
                    ABC_FREE( pPinNames[i] );
                    pPinNamesCopy[iPin++] = pPin->pName;
                    break;
                }
                if ( i == nPins )
                {
                    printf( "Cannot find pin name \"%s\" in the formula \"%s\" of gate \"%s\".\n", 
                        pPin->pName, pGate->pForm, pGate->pName );
                    return 1;
                }
            }
        }

        // check for the remaining names
        for ( i = 0; i < nPins; i++ )
            if ( pPinNames[i] )
            {
                printf( "Name \"%s\" appears in the formula \"%s\" of gate \"%s\" but there is no such pin.\n", 
                    pPinNames[i], pGate->pForm, pGate->pName );
                return 1;
            }

        // copy the names back
        memcpy( pPinNames, pPinNamesCopy, nPins * sizeof(char *) );
    }
/*
    // expand the manager if necessary
    if ( dd->size < nPins )
    {
        Cudd_Quit( dd );
        dd = Cudd_Init( nPins + 10, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
        Cudd_zddVarsFromBddVars( dd, 2 );
    }
    // derive formula as the BDD
    pGate->bFunc = Parse_FormulaParser( stdout, pGate->pForm, nPins, 0, pPinNames, dd, dd->vars );
    if ( pGate->bFunc == NULL )
        return 1;
    Cudd_Ref( pGate->bFunc );
    // derive cover
    pGate->pSop = Abc_ConvertBddToSop( pGate->pLib->pMmFlex, dd, pGate->bFunc, pGate->bFunc, nPins, 0, pGate->pLib->vCube, -1 );
*/

    // derive expression 
    pGate->vExpr = Mio_ParseFormula( pGate->pForm, (char **)pPinNames, nPins );
//    Mio_ParseFormulaTruthTest( pGate->pForm, (char **)pPinNames, nPins );
    // derive cover
    pGate->pSop = Mio_LibDeriveSop( nPins, pGate->vExpr, pGate->pLib->vCube );
    pGate->pSop = Mio_SopRegister( (Mem_Flex_t *)pGate->pLib->pMmFlex, pGate->pSop );
    // derive truth table
    if ( nPins <= 6 )
        pGate->uTruth = Exp_Truth6( nPins, pGate->vExpr, NULL );
    else if ( nPins <= 8 )
    {
        pGate->pTruth = ABC_CALLOC( word, 4 );
        Exp_Truth8( nPins, pGate->vExpr, NULL, pGate->pTruth );
    }

/*
    // verify
    if ( pGate->nInputs <= 6 )
    {
        extern word Abc_SopToTruth( char * pSop, int nInputs );
        word t2 = Abc_SopToTruth( pGate->pSop, nPins );
        if ( pGate->uTruth != t2 )
        {
            printf( "%s\n", pGate->pForm );
            Exp_Print( nPins, pGate->vExpr );
            printf( "Verification failed!\n" );
        }
    }
*/
    return 0;
}

/**Function*************************************************************

  Synopsis    [Deriving the functionality of the gates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mio_LibraryParseFormulas( Mio_Library_t * pLib )
{
    Mio_Gate_t * pGate;

    // count the gates
    pLib->nGates = 0;
    Mio_LibraryForEachGate( pLib, pGate )
        pLib->nGates++;        

    // for each gate, derive its function
    Mio_LibraryForEachGate( pLib, pGate )
        if ( Mio_GateParseFormula( pGate ) )
            return 1;
    return 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

