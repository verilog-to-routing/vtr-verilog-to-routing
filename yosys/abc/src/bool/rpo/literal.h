/**CFile****************************************************************

  FileName    [literal.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [RPO]

  Synopsis    [Literal structure]

  Author      [Mayler G. A. Martins / Vinicius Callegaro]
  
  Affiliation [UFRGS]

  Date        [Ver. 1.0. Started - May 08, 2013.]

  Revision    [$Id: literal.h,v 1.00 2013/05/08 00:00:00 mgamartins Exp $]

 ***********************************************************************/

#ifndef ABC__bool__rpo__literal_h
#define ABC__bool__rpo__literal_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include "bool/kit/kit.h"
#include "misc/vec/vec.h"
#include "misc/util/abc_global.h"

ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

// associations
typedef enum {
    LIT_NONE = 0, // 0: unknown
    LIT_AND, // 1: AND association
    LIT_OR, // 2: OR association
    LIT_XOR // 3: XOR association (not used yet)
} Operator_t;


typedef struct Literal_t_ Literal_t;

struct Literal_t_ {
    unsigned * transition;
    unsigned * function;
    Vec_Str_t * expression;
};


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Compute the positive transition]

  Description [The inputs are a function, the number of variables and a variable index and the output is a function]
               
  SideEffects [Should this function be in kitTruth.c ?]

  SeeAlso     []
//
***********************************************************************/

static inline void Lit_TruthPositiveTransition( unsigned * pIn, unsigned * pOut, int nVars, int varIdx )
{
    unsigned * cof0 = ABC_ALLOC (unsigned, Kit_TruthWordNum(nVars) );
    unsigned * cof1 = ABC_ALLOC (unsigned, Kit_TruthWordNum(nVars) );
    unsigned * ncof0;
    Kit_TruthCofactor0New(cof0, pIn,nVars,varIdx);
    Kit_TruthCofactor1New(cof1, pIn,nVars,varIdx);
    ncof0 = ABC_ALLOC (unsigned, Kit_TruthWordNum(nVars) );
    Kit_TruthNot(ncof0,cof0,nVars);
    Kit_TruthAnd(pOut,ncof0,cof1, nVars);
    ABC_FREE(cof0);
    ABC_FREE(ncof0);
    ABC_FREE(cof1);
}


/**Function*************************************************************

  Synopsis    [Compute the negative transition]

  Description [The inputs are a function, the number of variables and a variable index and the output is a function]
               
  SideEffects [Should this function be in kitTruth.c ?]

  SeeAlso     []

***********************************************************************/

static inline void Lit_TruthNegativeTransition( unsigned * pIn, unsigned * pOut, int nVars, int varIdx )
{
    unsigned * cof0 = ABC_ALLOC (unsigned, Kit_TruthWordNum(nVars) );
    unsigned * cof1 = ABC_ALLOC (unsigned, Kit_TruthWordNum(nVars) );
    unsigned * ncof1;
    Kit_TruthCofactor0New(cof0, pIn,nVars,varIdx);
    Kit_TruthCofactor1New(cof1, pIn,nVars,varIdx);
    ncof1 = ABC_ALLOC (unsigned, Kit_TruthWordNum(nVars) );
    Kit_TruthNot(ncof1,cof1,nVars);
    Kit_TruthAnd(pOut,ncof1,cof0,nVars);
    ABC_FREE(cof0);
    ABC_FREE(cof1);
    ABC_FREE(ncof1);
}


/**Function*************************************************************

  Synopsis    [Create a literal given a polarity ]

  Description [The inputs are the function, index and its polarity and a the output is a literal]
               
  SideEffects []

  SeeAlso     []

 ***********************************************************************/

static inline Literal_t* Lit_Alloc(unsigned* pTruth, int nVars, int varIdx, char pol) {
    unsigned * transition;
    unsigned * function;
    Vec_Str_t * exp;
    Literal_t* lit;
    assert(pol == '+' || pol == '-');
    transition = ABC_ALLOC(unsigned, Kit_TruthWordNum(nVars));
    if (pol == '+') {
        Lit_TruthPositiveTransition(pTruth, transition, nVars, varIdx);
    } else {
        Lit_TruthNegativeTransition(pTruth, transition, nVars, varIdx);
    }
    if (!Kit_TruthIsConst0(transition,nVars)) {
        function = ABC_ALLOC(unsigned, Kit_TruthWordNum(nVars));
        Kit_TruthIthVar(function, nVars, varIdx);
        //Abc_Print(-2, "Allocating function %X %d %c \n", *function, varIdx, pol);
        exp = Vec_StrAlloc(5);
        if (pol == '-') {
            Kit_TruthNot(function, function, nVars);
            Vec_StrPutC(exp, '!');
        }
        Vec_StrPutC(exp, (char)('a' + varIdx));
        Vec_StrPutC(exp, '\0');
        lit = ABC_ALLOC(Literal_t, 1);
        lit->function = function;
        lit->transition = transition;
        lit->expression = exp;
        return lit;
    } else {
        ABC_FREE(transition); // free the function.
        return NULL;
    }
}


/**Function*************************************************************

  Synopsis    [Group 2 literals using AND or OR]

  Description []
               
  SideEffects []

  SeeAlso     []

 ***********************************************************************/

static inline Literal_t* Lit_GroupLiterals(Literal_t* lit1, Literal_t* lit2, Operator_t op, int nVars) {
    unsigned * newFunction = ABC_ALLOC(unsigned, Kit_TruthWordNum(nVars));
    unsigned * newTransition = ABC_ALLOC(unsigned, Kit_TruthWordNum(nVars));
    Vec_Str_t * newExp = Vec_StrAlloc(lit1->expression->nSize + lit2->expression->nSize + 3);
    Literal_t* newLiteral;
    char opChar = '%';
    switch (op) {
        case LIT_AND:
        {
            //Abc_Print(-2, "Grouping AND %X %X \n", *lit1->function, *lit2->function);
            Kit_TruthAnd(newFunction, lit1->function, lit2->function, nVars);
            opChar = '*';
            break;
        }
        case LIT_OR:
        {
            //Abc_Print(-2, "Grouping OR %X %X \n", *lit1->function, *lit2->function);
            Kit_TruthOr(newFunction, lit1->function, lit2->function, nVars);
            opChar = '+';
            break;
        }
        default:
        {
            Abc_Print(-2, "Lit_GroupLiterals with op not defined.");
            //TODO Call ABC Abort
        }
    }

    Kit_TruthOr(newTransition, lit1->transition, lit2->transition, nVars);
    // create a new expression.
    Vec_StrPutC(newExp, '(');
    Vec_StrAppend(newExp, lit1->expression->pArray);
    //Vec_StrPop(newExp); // pop the \0
    Vec_StrPutC(newExp, opChar);
    Vec_StrAppend(newExp, lit2->expression->pArray);
    //Vec_StrPop(newExp); // pop the \0
    Vec_StrPutC(newExp, ')');
    Vec_StrPutC(newExp, '\0');

    newLiteral = ABC_ALLOC(Literal_t, 1);
    newLiteral->function = newFunction;
    newLiteral->transition = newTransition;
    newLiteral->expression = newExp;
    return newLiteral;
}


/**Function*************************************************************

  Synopsis    [Create a const literal ]

  Description []
               
  SideEffects []

  SeeAlso     []

 ***********************************************************************/

static inline Literal_t* Lit_CreateLiteralConst(unsigned* pTruth, int nVars, int constant) {
    Vec_Str_t * exp = Vec_StrAlloc(3);
    Literal_t* lit;
    Vec_StrPutC(exp, (char)('0' + constant));
    Vec_StrPutC(exp, '\0');
    lit = ABC_ALLOC(Literal_t, 1);
    lit->expression = exp;
    lit->function = pTruth;
    lit->transition = pTruth; // wrong but no effect ###
    return lit;
}

static inline Literal_t* Lit_Copy(Literal_t* lit, int nVars) {
    Literal_t* newLit = ABC_ALLOC(Literal_t,1);
    newLit->function = ABC_ALLOC(unsigned, Kit_TruthWordNum(nVars));
    Kit_TruthCopy(newLit->function,lit->function,nVars);
    newLit->transition = ABC_ALLOC(unsigned, Kit_TruthWordNum(nVars));
    Kit_TruthCopy(newLit->transition,lit->transition,nVars);
    newLit->expression = Vec_StrDup(lit->expression);
//    Abc_Print(-2,"Copying: %s\n",newLit->expression->pArray); 
    return newLit;
}

static inline void Lit_PrintTT(unsigned* tt, int nVars) {
    int w;
    for(w=nVars-1; w>=0; w--) {
            Abc_Print(-2, "%08X", tt[w]);
    }
}

static inline void Lit_PrintExp(Literal_t* lit) {
    Abc_Print(-2, "%s", lit->expression->pArray);
}

/**Function*************************************************************

  Synopsis    [Delete the literal ]

  Description []
               
  SideEffects []

  SeeAlso     []

 ***********************************************************************/

static inline void Lit_Free(Literal_t * lit) {
    if(lit == NULL) {
        return;
    }
//    Abc_Print(-2,"Freeing: %s\n",lit->expression->pArray); 
    ABC_FREE(lit->function);
    ABC_FREE(lit->transition);
    Vec_StrFree(lit->expression);
    ABC_FREE(lit);
}

ABC_NAMESPACE_HEADER_END

#endif    /* LITERAL_H */

