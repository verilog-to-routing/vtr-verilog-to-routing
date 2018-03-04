/**CFile****************************************************************

  FileName    [lucky.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Semi-canonical form computation package.]

  Synopsis    [External declarations.]

  Author      [Jake]

  Date        [Started - August 2012]

***********************************************************************/

#ifndef ABC__bool__lucky__LUCKY_H_
#define ABC__bool__lucky__LUCKY_H_


ABC_NAMESPACE_HEADER_START

typedef struct
{
    int varN;
    int* swapArray;
    int swapCtr;
    int totalSwaps;
    int* flipArray;
    int flipCtr;
    int totalFlips; 
}permInfo;

extern unsigned Kit_TruthSemiCanonicize_new( unsigned * pInOut, unsigned * pAux, int nVars, char * pCanonPerm );
extern unsigned luckyCanonicizer_final_fast( word * pInOut, int nVars, char * pCanonPerm );
extern unsigned luckyCanonicizer_final_fast1( word * pInOut, int nVars, char * pCanonPerm );
extern void resetPCanonPermArray(char* x, int nVars); 
extern permInfo* setPermInfoPtr(int var);
extern void freePermInfoPtr(permInfo* x);
extern void simpleMinimal(word* x, word* pAux,word* minimal, permInfo* pi, int nVars);

ABC_NAMESPACE_HEADER_END

#endif /* LUCKY_H_ */
