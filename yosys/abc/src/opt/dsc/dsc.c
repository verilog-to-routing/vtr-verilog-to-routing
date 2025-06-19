/**CFile****************************************************************

  FileName    [dsc.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Disjoint support decomposition - ICCD'15]

  Synopsis    [Disjoint-support decomposition with cofactoring and boolean difference analysis
               from V. Callegaro, F. S. Marranghello, M. G. A. Martins, R. P. Ribas and A. I. Reis,
               "Bottom-up disjoint-support decomposition based on cofactor and boolean difference analysis," ICCD'15]

  Author      [Vinicius Callegaro, Mayler G. A. Martins, Felipe S. Marranghello, Renato P. Ribas and Andre I. Reis]

  Affiliation [UFRGS - Federal University of Rio Grande do Sul - Brazil]

  Date        [Ver. 1.0. Started - October 24, 2014.]

  Revision    [$Id: dsc.h,v 1.00 2014/10/24 00:00:00 vcallegaro Exp $]

***********************************************************************/

#include "dsc.h"
#include <assert.h>
#include "misc/util/utilTruth.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
/*
    This code performs truth-table-based decomposition for 6-variable functions.
    Representation of operations:
    ! = not;
    (ab) = a and b;
    [ab] = a xor b;
*/
typedef struct Dsc_node_t_ Dsc_node_t;
struct Dsc_node_t_
{
    word *pNegCof;
    word *pPosCof;
    word *pBoolDiff;
    unsigned int on[DSC_MAX_VAR+1]; // pos cofactor spec - first element denotes the size of the array
    unsigned int off[DSC_MAX_VAR+1]; // neg cofactor spec - first element denotes the size of the array
    char exp[DSC_MAX_STR];
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

static inline void xorInPlace( word * pOut, word * pIn2, int nWords)
{
    int w;
    for ( w = 0; w < nWords; w++ )
        pOut[w] ^= pIn2[w];
}

static inline void dsc_debug_node(Dsc_node_t * pNode, int nVars, const int TRUTH_WORDS) {
    int i;
    printf("Node:\t%s\n",pNode->exp);
    printf("\tneg cof:\t");Abc_TtPrintHexRev(stdout, pNode->pNegCof, nVars);
    printf("\tpos cof:\t");Abc_TtPrintHexRev(stdout, pNode->pPosCof, nVars);
    printf("\tbool diff:\t");Abc_TtPrintHexRev(stdout, pNode->pBoolDiff, nVars);
    printf("\toff:\t");
    for (i=1;i<=(int)pNode->off[0];i++) {
        printf("%c%c", (pNode->off[i] & 1U) ? ' ' : '!', 'a'+(pNode->off[i] >> 1));
    }
    printf("\ton:\t");
    for (i=1;i<=(int)pNode->on[0];i++) {
        printf("%c%c", (pNode->on[i] & 1U) ? ' ' : '!', 'a'+(pNode->on[i] >> 1));
    }
    printf("\n");
}

static inline int dsc_and_test(Dsc_node_t *ni, Dsc_node_t *nj, const int TRUTH_WORDS, int* ci, int* cj) {
            if (Abc_TtEqual(ni->pNegCof, nj->pNegCof, TRUTH_WORDS)) {*ci=1; *cj=1; return 1;}
    else     if (Abc_TtEqual(ni->pNegCof, nj->pPosCof, TRUTH_WORDS)) {*ci=1; *cj=0; return 1;}
    else     if (Abc_TtEqual(ni->pPosCof, nj->pNegCof, TRUTH_WORDS)) {*ci=0; *cj=1; return 1;}
    else     if (Abc_TtEqual(ni->pPosCof, nj->pPosCof, TRUTH_WORDS)) {*ci=0; *cj=0; return 1;}
    return 0;
}

static inline int dsc_xor_test(Dsc_node_t *ni, Dsc_node_t *nj, const int TRUTH_WORDS) {
    return Abc_TtEqual(ni->pBoolDiff, nj->pBoolDiff, TRUTH_WORDS);
}

static inline void concat(char* target, char begin, char end, char* s1, int s1Polarity, char* s2, int s2Polarity) {
    *target++ = begin;
    //s1
    if (!s1Polarity)
        *target++ = '!';
    while (*s1 != '\0')
        *target++ = *s1++;
    // s2
    if (!s2Polarity)
        *target++ = '!';
    while (*s2 != '\0')
        *target++ = *s2++;
    // end
    *target++ = end;
    *target = '\0';
}

static inline void cubeCofactor(word * const pTruth, const unsigned int * const cubeCof, const int TRUTH_WORDS) {
    int size = cubeCof[0];
    int i;
    for (i = 1; i <= size; i++) {
        unsigned int c = cubeCof[i];
        if (c & 1U) {
            Abc_TtCofactor1(pTruth, TRUTH_WORDS, c >> 1);
        } else {
            Abc_TtCofactor0(pTruth, TRUTH_WORDS, c >> 1);
        }
    }
}

static inline void merge(unsigned int * const pOut, const unsigned int * const pIn) {
    const int elementsToCopy = pIn[0];
    int i, j;
    for (i = pOut[0]+1, j = 1; j <= elementsToCopy; i++, j++) {
        pOut[i] = pIn[j];
    }
    pOut[0] += elementsToCopy;
}

void dsc_and_group(Dsc_node_t * pOut, Dsc_node_t * ni, int niPolarity, Dsc_node_t * nj, int njPolarity, int nVars, const int TRUTH_WORDS) {
    unsigned int* xiOFF, * xiON, * xjOFF, * xjON;
    // expression
    concat(pOut->exp, '(', ')', ni->exp, niPolarity, nj->exp, njPolarity);
    // ON-OFF
    if (niPolarity) {
        xiOFF = ni->off;
        xiON = ni->on;
    } else {
        xiOFF = ni->on;
        xiON = ni->off;
    }
    if (njPolarity) {
        xjOFF = nj->off;
        xjON = nj->on;
    } else {
        xjOFF = nj->on;
        xjON = nj->off;
    }
    // creating both the new OFF specification and negative cofactor of the new group
    {
        // first element of the array represents the size of the cube-cofactor
        int xiOFFSize = xiOFF[0];
        int xjOFFSize = xjOFF[0];
        if (xiOFFSize <= xjOFFSize) {
            int i;
            pOut->off[0] = xiOFFSize; // set the number of elements
            for (i = 1; i <= xiOFFSize; i++) {
                pOut->off[i] = xiOFF[i];
            }
        } else {
            int i;
            pOut->off[0] = xjOFFSize; // set the number of elements
            for (i = 1; i <= xjOFFSize; i++) {
                pOut->off[i] = xjOFF[i];
            }
        }
        // set the negative cofactor of the new group
        pOut->pNegCof = niPolarity ? ni->pNegCof : ni->pPosCof;
    }
    // creating both new ON specification and positive cofactor of the new group
    {
        int i;
        int j;
        unsigned int xiONSize = xiON[0];
        unsigned int xjONSize = xjON[0];
        pOut->on[0] = xiONSize + xjONSize;
        for (i = 1; i <= (int)xiONSize; i++) {
            pOut->on[i] = xiON[i];
        }
        for (j = 1; j <= (int)xjONSize; j++) {
            pOut->on[i++] = xjON[j];
        }
        // set the positive cofactor of the new group
        if (xiONSize >= xjONSize) {
            pOut->pPosCof = niPolarity ? ni->pPosCof : ni->pNegCof;
            cubeCofactor(pOut->pPosCof, xjON, TRUTH_WORDS);
        } else {
            pOut->pPosCof = njPolarity ? nj->pPosCof : nj->pNegCof;
            cubeCofactor(pOut->pPosCof, xiON, TRUTH_WORDS);
        }
    }
    // set the boolean difference of the new group
    pOut->pBoolDiff = njPolarity ? nj->pNegCof : nj->pPosCof;
    xorInPlace(pOut->pBoolDiff, pOut->pPosCof, TRUTH_WORDS);
}

void dsc_xor_group(Dsc_node_t * pOut, Dsc_node_t * ni, Dsc_node_t * nj, int nVars, const int TRUTH_WORDS) {
    //
    const unsigned int * xiOFF = ni->off;
    const unsigned int * xiON = ni->on;
    const unsigned int * xjOFF = nj->off;
    const unsigned int * xjON = nj->on;
    //
    const int xiOFFSize = xiOFF[0];
    const int xiONSize = xiON[0];
    const int xjOFFSize = xjOFF[0];
    const int xjONSize = xjON[0];
    // minCubeCofs
    int minCCSize = xiOFFSize;
    int minCCPolarity = 0;
    Dsc_node_t * minCCNode = ni;
    // expression
    concat(pOut->exp, '[', ']', ni->exp, 1, nj->exp, 1);
    if (minCCSize > xiONSize) {
        minCCSize = xiONSize;
        minCCPolarity = 1;
        //minCCNode = ni;
    }
    if (minCCSize > xjOFFSize) {
        minCCSize = xjOFFSize;
        minCCPolarity = 0;
        minCCNode = nj;
    }
    if (minCCSize > xjONSize) {
        minCCSize = xjONSize;
        minCCPolarity = 1;
        minCCNode = nj;
    }
    //
    if (minCCNode == ni) {
        if (minCCPolarity) {
            // gOFF = xiON, xjON
            pOut->pNegCof = nj->pPosCof;
            cubeCofactor(pOut->pNegCof, xiON, TRUTH_WORDS);
            // gON = xiON, xjOFF
            pOut->pPosCof = nj->pNegCof;
            cubeCofactor(pOut->pPosCof, xiON, TRUTH_WORDS);
        } else {
            // gOFF = xiOFF, xjOFF
            pOut->pNegCof = nj->pNegCof;
            cubeCofactor(pOut->pNegCof, xiOFF, TRUTH_WORDS);
            // gON = xiOFF, xjON
            pOut->pPosCof = nj->pPosCof;
            cubeCofactor(pOut->pPosCof, xiOFF, TRUTH_WORDS);
        }
    }else  {
        if (minCCPolarity) {
            // gOFF = xjON, xiON
            pOut->pNegCof = ni->pPosCof;
            cubeCofactor(pOut->pNegCof, xjON, TRUTH_WORDS);
            // gON = xjON, xiOFF
            pOut->pPosCof = ni->pNegCof;
            cubeCofactor(pOut->pPosCof, xjON, TRUTH_WORDS);
        } else {
            // gOFF = xjOFF, xiOFF
            pOut->pNegCof = ni->pNegCof;
            cubeCofactor(pOut->pNegCof, xjOFF, TRUTH_WORDS);
            // gON = xjOFF, xiON
            pOut->pPosCof = ni->pPosCof;
            cubeCofactor(pOut->pPosCof, xjOFF, TRUTH_WORDS);
        }
    }
    // bool diff
    pOut->pBoolDiff = ni->pBoolDiff;
    // evaluating specs
    // off spec
    pOut->off[0] = 0;
    if ((xiOFFSize+xjOFFSize) <= (xiONSize+xjONSize)) {
        merge(pOut->off, xiOFF);
        merge(pOut->off, xjOFF);
    } else {
        merge(pOut->off, xiON);
        merge(pOut->off, xjON);
    }
    // on spec
    pOut->on[0] = 0;
    if ((xiOFFSize+xjONSize) <= (xiONSize+xjOFFSize)) {
        merge(pOut->on, xiOFF);
        merge(pOut->on, xjON);
    } else {
        merge(pOut->on, xiON);
        merge(pOut->on, xjOFF);
    }
}

/**
 * memory allocator with a capacity of storing 3*nVars
 * truth-tables for negative and positive cofactors and
 * the boolean difference for each input variable
 */
extern word * Dsc_alloc_pool(int nVars) {
    return ABC_ALLOC(word, 3 * Abc_TtWordNum(nVars) * nVars);
}

/**
 * just free the memory pool
 */
extern void Dsc_free_pool(word * pool) {
    ABC_FREE(pool);
}

/**
 * This method implements the paper proposed by V. Callegaro, F. S. Marranghello, M. G. A. Martins, R. P. Ribas and A. I. Reis,
 * entitled "Bottom-up disjoint-support decomposition based on cofactor and boolean difference analysis", presented at ICCD 2015.
 * pTruth: pointer for the truth table representing the target function.
 * nVarsInit: the number of variables of the truth table of the target function.
 * pRes: pointer for storing the resulting decomposition, whenever a decomposition can be found.
 * pool: NULL or a pointer for with a capacity of storing 3*nVars truth-tables. IF NULL, the function will allocate and free the memory of each call.
 * (the results presented on ICCD paper are running this method with NULL for the memory pool).
 * The method returns 0 if a full decomposition was found and a negative value otherwise.
 */
extern int Dsc_Decompose(word * pTruth, const int nVarsInit, char * const pRes, word *pool) {
    const int TRUTH_WORDS = Abc_TtWordNum(nVarsInit);
    const int NEED_POOL_ALLOC = (pool == NULL);

    Dsc_node_t nodes[DSC_MAX_VAR];
    Dsc_node_t *newNodes[DSC_MAX_VAR];
    Dsc_node_t *oldNodes[DSC_MAX_VAR];

    Dsc_node_t freeNodes[DSC_MAX_VAR]; // N is the maximum number of possible groups.
    int f = 0; // f represent the next free position in the freeNodes array
    int o = 0; // o stands for the number of already tested nodes
    int n = 0; // n will represent the number of current nodes (i.e. support)

    pRes[0] = '\0';
    pRes[1] = '\0';

    if (NEED_POOL_ALLOC)
        pool = ABC_ALLOC(word, 3 * TRUTH_WORDS * nVarsInit);

    // block for the node data allocation
    {
        // pointer for the next free truth word
        word *pNextTruth = pool;
        int iVar;
        for (iVar = 0; iVar < nVarsInit; iVar++) {
            // negative cofactor
            Abc_TtCofactor0p(pNextTruth, pTruth, TRUTH_WORDS, iVar);
            // dont care test
            if (!Abc_TtEqual(pNextTruth, pTruth, TRUTH_WORDS)) {
                Dsc_node_t *node = &nodes[iVar];
                node->pNegCof = pNextTruth;
                // increment next truth pointer
                pNextTruth += TRUTH_WORDS;
                // positive cofactor
                node->pPosCof = pNextTruth;
                Abc_TtCofactor1p(node->pPosCof, pTruth, TRUTH_WORDS, iVar);
                // increment next truth pointer
                pNextTruth += TRUTH_WORDS;
                // boolean difference
                node->pBoolDiff = pNextTruth;
                Abc_TtXor(node->pBoolDiff, node->pNegCof, node->pPosCof, TRUTH_WORDS, 0);
                // increment next truth pointer
                pNextTruth += TRUTH_WORDS;
                // define on spec -
                node->on[0] = 1; node->on[1] = (iVar << 1) | 1u; // lit = i*2+1, when polarity=true
                // define off spec
                node->off[0] = 1; node->off[1] = iVar << 1;// lit=i*2 otherwise
                // store the node expression
                node->exp[0] = 'a'+iVar; // TODO fix the variable names
                node->exp[1] = '\0';
                // add the node to the newNodes array
                newNodes[n++] = node;
            }
        }
    }
    //const int initialSupport = n;
    if (n == 0) {
        if (NEED_POOL_ALLOC)
            ABC_FREE(pool);
        if (Abc_TtIsConst0(pTruth, TRUTH_WORDS)) {
            { if ( pRes ) pRes[0] = '0', pRes[1] = '\0'; }
            return 0;
        } else if (Abc_TtIsConst1(pTruth, TRUTH_WORDS)) {
            { if ( pRes ) pRes[0] = '1', pRes[1] = '\0'; }
            return 0;
        } else {
            Abc_Print(-1, "ERROR. No variable in the support of f, but f isn't constant!\n");
            return -1;
        }
    }
    while (n > 0) {
        int tempN = 0;
        int i, j, iPolarity, jPolarity;
        Dsc_node_t *ni, *nj, *newNode = NULL;
        for (i = 0; i < n; i++) {
            ni = newNodes[i];
            newNode = NULL;
            j = 0;
            while (j < o) {
                nj = oldNodes[j];
                if (dsc_and_test(ni, nj, TRUTH_WORDS, &iPolarity, &jPolarity)) {
                    newNode = &freeNodes[f++];
                    dsc_and_group(newNode, ni, iPolarity, nj, jPolarity, nVarsInit, TRUTH_WORDS);
                }
                // XOR test
                if ((newNode == NULL)  && (dsc_xor_test(ni, nj, TRUTH_WORDS))) {
                    newNode = &freeNodes[f++];
                    dsc_xor_group(newNode, ni, nj, nVarsInit, TRUTH_WORDS);
                }
                if (newNode != NULL) {
                    oldNodes[j] = oldNodes[--o];
                    break;
                } else {
                    j++;
                }
            }
            if (newNode != NULL) {
                newNodes[tempN++] = newNode;
            } else {
                oldNodes[o++] = ni;
            }
        }
        n = tempN;
    }
    if (o == 1) {
        Dsc_node_t * solution = oldNodes[0];
        if (Abc_TtIsConst0(solution->pNegCof, TRUTH_WORDS) && Abc_TtIsConst1(solution->pPosCof, TRUTH_WORDS)) {
            // Direct solution found
            if ( pRes )
                strcpy( pRes, solution->exp);
            if (NEED_POOL_ALLOC)
                ABC_FREE(pool);
            return 0;
        } else if (Abc_TtIsConst1(solution->pNegCof, TRUTH_WORDS) && Abc_TtIsConst0(solution->pPosCof, TRUTH_WORDS)) {
            // Complementary solution found
            if ( pRes ) {
                pRes[0] = '!';
                strcpy( &pRes[1], solution->exp);
            }
            if (NEED_POOL_ALLOC)
                ABC_FREE(pool);
            return 0;
        } else {
            printf("DSC ERROR: Final DSC node found, but differs from target function.\n");
            if (NEED_POOL_ALLOC)
                ABC_FREE(pool);
            return -1;
        }
    }
    if (NEED_POOL_ALLOC)
        ABC_FREE(pool);
    return -1;
}


/**Function*************************************************************
  Synopsis    [DSD formula manipulation.]
  Description [Code copied from dauDsd.c but changed DAU_MAX_VAR to DSC_MAX_VAR]
***********************************************************************/
int * Dsc_ComputeMatches( char * p )
{
    static int pMatches[DSC_MAX_VAR];
    int pNested[DSC_MAX_VAR];
    int v, nNested = 0;
    for ( v = 0; p[v]; v++ )
    {
        pMatches[v] = 0;
        if ( p[v] == '(' || p[v] == '[' || p[v] == '<' || p[v] == '{' )
            pNested[nNested++] = v;
        else if ( p[v] == ')' || p[v] == ']' || p[v] == '>' || p[v] == '}' )
            pMatches[pNested[--nNested]] = v;
        assert( nNested < DSC_MAX_VAR );
    }
    assert( nNested == 0 );
    return pMatches;
}

/**Function*************************************************************
  Synopsis    [DSD formula manipulation.]
  Description [Code copied from dauDsd.c but changed DAU_MAX_VAR to DSC_MAX_VAR]
***********************************************************************/
int Dsc_CountAnds_rec( char * pStr, char ** p, int * pMatches )
{
    if ( **p == '!' )
        (*p)++;
    while ( (**p >= 'A' && **p <= 'F') || (**p >= '0' && **p <= '9') )
        (*p)++;
    if ( **p == '<' )
    {
        char * q = pStr + pMatches[*p - pStr];
        if ( *(q+1) == '{' )
            *p = q+1;
    }
    if ( **p >= 'a' && **p <= 'z' ) // var
        return 0;
    if ( **p == '(' || **p == '[' ) // and/or/xor
    {
        int Counter = 0, AddOn = (**p == '(')? 1 : 3;
        char * q = pStr + pMatches[ *p - pStr ];
        assert( *q == **p + 1 + (**p != '(') );
        for ( (*p)++; *p < q; (*p)++ )
            Counter += AddOn + Dsc_CountAnds_rec( pStr, p, pMatches );
        assert( *p == q );
        return Counter - AddOn;
    }
    if ( **p == '<' || **p == '{' ) // mux
    {
        int Counter = 3;
        char * q = pStr + pMatches[ *p - pStr ];
        assert( *q == **p + 1 + (**p != '(') );
        for ( (*p)++; *p < q; (*p)++ )
            Counter += Dsc_CountAnds_rec( pStr, p, pMatches );
        assert( *p == q );
        return Counter;
    }
    assert( 0 );
    return 0;
}
/**Function*************************************************************
  Synopsis    [DSD formula manipulation.]
  Description [Code copied from dauDsd.c but changed DAU_MAX_VAR to DSC_MAX_VAR]
***********************************************************************/
extern int Dsc_CountAnds( char * pDsd )
{
    if ( pDsd[1] == 0 )
        return 0;
    return Dsc_CountAnds_rec( pDsd, &pDsd, Dsc_ComputeMatches(pDsd) );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
