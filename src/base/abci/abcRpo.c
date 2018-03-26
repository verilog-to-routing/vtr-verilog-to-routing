/**CFile****************************************************************

  FileName    [abcRpo.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Rpo package.]

  Synopsis    [Procedures for executing RPO.]

  Author      [Mayler G. A. Martins / Vinicius Callegaro]
  
  Affiliation [UFRGS]

  Date        [Ver. 1.0. Started - May 08, 2013.]

  Revision    [$Id: abcRpo.c,v 1.00 2013/05/08 00:00:00 mgamartins Exp $]

 ***********************************************************************/

#include "misc/extra/extra.h"

#include "bool/rpo/rpo.h"
#include "bool/rpo/literal.h"

ABC_NAMESPACE_IMPL_START


// data-structure to store a bunch of truth tables
typedef struct Rpo_TtStore_t_ Rpo_TtStore_t;

struct Rpo_TtStore_t_ {
    int nVars;
    int nWords;
    int nFuncs;
    word ** pFuncs;
};


// read/write/flip i-th bit of a bit string table:

static inline int Abc_TruthGetBit(word * p, int i) {
    return (int) (p[i >> 6] >> (i & 63)) & 1;
}

static inline void Abc_TruthSetBit(word * p, int i) {
    p[i >> 6] |= (((word) 1) << (i & 63));
}

static inline void Abc_TruthXorBit(word * p, int i) {
    p[i >> 6] ^= (((word) 1) << (i & 63));
}

// read/write k-th digit d of a hexadecimal number:

static inline int Abc_TruthGetHex(word * p, int k) {
    return (int) (p[k >> 4] >> ((k << 2) & 63)) & 15;
}

static inline void Abc_TruthSetHex(word * p, int k, int d) {
    p[k >> 4] |= (((word) d) << ((k << 2) & 63));
}

static inline void Abc_TruthXorHex(word * p, int k, int d) {
    p[k >> 4] ^= (((word) d) << ((k << 2) & 63));
}

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

// read one hex character

static inline int Abc_TruthReadHexDigit(char HexChar) {
    if (HexChar >= '0' && HexChar <= '9')
        return HexChar - '0';
    if (HexChar >= 'A' && HexChar <= 'F')
        return HexChar - 'A' + 10;
    if (HexChar >= 'a' && HexChar <= 'f')
        return HexChar - 'a' + 10;
    assert(0); // not a hexadecimal symbol
    return -1; // return value which makes no sense
}

// write one hex character

static inline void Abc_TruthWriteHexDigit(FILE * pFile, int HexDigit) {
    assert(HexDigit >= 0 && HexDigit < 16);
    if (HexDigit < 10)
        fprintf(pFile, "%d", HexDigit);
    else
        fprintf(pFile, "%c", 'A' + HexDigit - 10);
}

// read one truth table in hexadecimal

static void Abc_TruthReadHex(word * pTruth, char * pString, int nVars) {
    int nWords = (nVars < 7) ? 1 : (1 << (nVars - 6));
    int k, Digit, nDigits = (nVars < 7) ? (1 << (nVars-2)) : (nWords << 4);
    char EndSymbol;
    // skip the first 2 symbols if they are "0x"
    if (pString[0] == '0' && pString[1] == 'x')
        pString += 2;
    // get the last symbol
    EndSymbol = pString[nDigits];
    // the end symbol of the TT (the one immediately following hex digits)
    // should be one of the following: space, a new-line, or a zero-terminator
    // (note that on Windows symbols '\r' can be inserted before each '\n')
    assert(EndSymbol == ' ' || EndSymbol == '\n' || EndSymbol == '\r' || EndSymbol == '\0');
    // read hexadecimal digits in the reverse order
    // (the last symbol in the string is the least significant digit)
    for (k = 0; k < nDigits; k++) {
        Digit = Abc_TruthReadHexDigit(pString[nDigits - 1 - k]);
        assert(Digit >= 0 && Digit < 16);
        Abc_TruthSetHex(pTruth, k, Digit);
    }
}

// write one truth table in hexadecimal (do not add end-of-line!)

static void Abc_TruthWriteHex(FILE * pFile, word * pTruth, int nVars) {
    int nDigits, Digit, k;
    nDigits = (1 << (nVars - 2));
    for (k = 0; k < nDigits; k++) {
        Digit = Abc_TruthGetHex(pTruth, k);
        assert(Digit >= 0 && Digit < 16);
        Abc_TruthWriteHexDigit(pFile, Digit);
    }
}



/**Function*************************************************************

  Synopsis    [Allocate/Deallocate storage for truth tables..]

  Description []
               
  SideEffects []

  SeeAlso     []

 ***********************************************************************/
static Rpo_TtStore_t * Abc_TruthStoreAlloc(int nVars, int nFuncs) {
    Rpo_TtStore_t * p;
    int i;
    p = (Rpo_TtStore_t *) malloc(sizeof (Rpo_TtStore_t));
    p->nVars = nVars;
    p->nWords = (nVars < 7) ? 1 : (1 << (nVars - 6));
    p->nFuncs = nFuncs;
    // alloc storage for 'nFuncs' truth tables as one chunk of memory
    p->pFuncs = (word **) malloc((sizeof (word *) + sizeof (word) * p->nWords) * p->nFuncs);
    // assign and clean the truth table storage
    p->pFuncs[0] = (word *) (p->pFuncs + p->nFuncs);
    memset(p->pFuncs[0], 0, sizeof (word) * p->nWords * p->nFuncs);
    // split it up into individual truth tables
    for (i = 1; i < p->nFuncs; i++)
        p->pFuncs[i] = p->pFuncs[i - 1] + p->nWords;
    return p;
}

static Rpo_TtStore_t * Abc_TruthStoreAlloc2(int nVars, int nFuncs, word * pBuffer) {
    Rpo_TtStore_t * p;
    int i;
    p = (Rpo_TtStore_t *) malloc(sizeof (Rpo_TtStore_t));
    p->nVars = nVars;
    p->nWords = (nVars < 7) ? 1 : (1 << (nVars - 6));
    p->nFuncs = nFuncs;
    // alloc storage for 'nFuncs' truth tables as one chunk of memory
    p->pFuncs = (word **) malloc(sizeof (word *) * p->nFuncs);
    // assign and clean the truth table storage
    p->pFuncs[0] = pBuffer;
    // split it up into individual truth tables
    for (i = 1; i < p->nFuncs; i++)
        p->pFuncs[i] = p->pFuncs[i - 1] + p->nWords;
    return p;
}

static void Abc_TtStoreFree(Rpo_TtStore_t * p, int nVarNum) {
    if (nVarNum >= 0)
        ABC_FREE(p->pFuncs[0]);
    ABC_FREE(p->pFuncs);
    ABC_FREE(p);
}

/**Function*************************************************************

  Synopsis    [Read file contents.]

  Description []
               
  SideEffects []

  SeeAlso     []

 ***********************************************************************/
extern int Abc_FileSize(char * pFileName);

/**Function*************************************************************

  Synopsis    [Read file contents.]

  Description []
               
  SideEffects []

  SeeAlso     []

 ***********************************************************************/
extern char * Abc_FileRead(char * pFileName);

/**Function*************************************************************

  Synopsis    [Determine the number of variables by reading the first line.]

  Description [Determine the number of functions by counting the lines.]
               
  SideEffects []

  SeeAlso     []

 ***********************************************************************/
extern void Abc_TruthGetParams(char * pFileName, int * pnVars, int * pnTruths);


/**Function*************************************************************

  Synopsis    [Read truth tables from file.]

  Description []
               
  SideEffects []

  SeeAlso     []

 ***********************************************************************/
static void Abc_TruthStoreRead(char * pFileName, Rpo_TtStore_t * p) {
    char * pContents;
    int i, nLines;
    pContents = Abc_FileRead(pFileName);
    if (pContents == NULL)
        return;
    // here it is assumed (without checking!) that each line of the file 
    // begins with a string of hexadecimal chars followed by space

    // the file will be read till the first empty line (pContents[i] == '\n')
    // (note that Abc_FileRead() added several empty lines at the end of the file contents)
    for (nLines = i = 0; pContents[i] != '\n';) {
        // read one line
        Abc_TruthReadHex(p->pFuncs[nLines++], &pContents[i], p->nVars);
        // skip till after the end-of-line symbol
        // (note that end-of-line symbol is also skipped)
        while (pContents[i++] != '\n');
    }
    // adjust the number of functions read 
    // (we may have allocated more storage because some lines in the file were empty)
    assert(p->nFuncs >= nLines);
    p->nFuncs = nLines;
    ABC_FREE(pContents);
}

/**Function*************************************************************

  Synopsis    [Write truth tables into file.]

  Description []
               
  SideEffects []

  SeeAlso     []

 ***********************************************************************/
static void Abc_TtStoreWrite(char * pFileName, Rpo_TtStore_t * p, int fBinary) {
    FILE * pFile;
    int i, nBytes = 8 * Abc_Truth6WordNum(p->nVars);
    pFile = fopen(pFileName, "wb");
    if (pFile == NULL) {
        printf("Cannot open file \"%s\" for writing.\n", pFileName);
        return;
    }
    for (i = 0; i < p->nFuncs; i++) {
        if (fBinary)
            fwrite(p->pFuncs[i], nBytes, 1, pFile);
        else
            Abc_TruthWriteHex(pFile, p->pFuncs[i], p->nVars), fprintf(pFile, "\n");
    }
    fclose(pFile);
}

/**Function*************************************************************

  Synopsis    [Read truth tables from input file and write them into output file.]

  Description []
               
  SideEffects []

  SeeAlso     []

 ***********************************************************************/
static Rpo_TtStore_t * Abc_TtStoreLoad(char * pFileName, int nVarNum) {
    Rpo_TtStore_t * p;
    if (nVarNum < 0) {
        int nVars, nTruths;
        // figure out how many truth table and how many variables
        Abc_TruthGetParams(pFileName, &nVars, &nTruths);
        if (nVars < 2 || nVars > 16 || nTruths == 0)
            return NULL;
        // allocate data-structure
        p = Abc_TruthStoreAlloc(nVars, nTruths);
        // read info from file
        Abc_TruthStoreRead(pFileName, p);
    } else {
        char * pBuffer;
        int nFileSize = Abc_FileSize(pFileName);
        int nBytes = (1 << (nVarNum - 3)); // why mishchencko put -3? ###
        int nTruths = nFileSize / nBytes;
        //Abc_Print(-2,"nFileSize=%d,nTruths=%d\n",nFileSize, nTruths);
        if (nFileSize == -1)
            return NULL;
        assert(nVarNum >= 6);
        if (nFileSize % nBytes != 0)
            Abc_Print(0, "The file size (%d) is divided by the truth table size (%d) with remainder (%d).\n",
                nFileSize, nBytes, nFileSize % nBytes);
        // read file contents
        pBuffer = Abc_FileRead(pFileName);
        // allocate data-structure
        p = Abc_TruthStoreAlloc2(nVarNum, nTruths, (word *) pBuffer);
    }
    return p;
}


/**Function*************************************************************

  Synopsis    [Apply decomposition to the truth table.]

  Description [Returns the number of AIG nodes.]
               
  SideEffects []

  SeeAlso     []

 ***********************************************************************/
void Abc_TruthRpoPerform(Rpo_TtStore_t * p, int nThreshold, int fVerbose) {
    clock_t clk = clock();
    int i;
    int rpoCount = 0;
    Literal_t* lit;
    float percent;
    for (i = 0; i < p->nFuncs; i++) {
//        if(i>1000) {
//            continue;
//        }
////        
//        if(i!= 2196 ) { //5886
//            continue;
//        }
        if(fVerbose) {
            Abc_Print(-2,"%d: ", i+1);
        }
            
        lit = Rpo_Factorize((unsigned *) p->pFuncs[i], p->nVars, nThreshold, fVerbose);
        if (lit != NULL) {
            if(fVerbose) {
                Abc_Print(-2, "Solution : %s\n", lit->expression->pArray);
                Abc_Print(-2, "\n\n");
            }
            Lit_Free(lit);
            rpoCount++;
        } else {
            if(fVerbose) {
                Abc_Print(-2, "null\n");
                Abc_Print(-2, "\n\n");
            }
        }
    }
    percent = (rpoCount * 100.0) / p->nFuncs;
    Abc_Print(-2,"%d of %d (%.2f %%) functions are RPO.\n", rpoCount,p->nFuncs,percent);
    Abc_PrintTime(1, "Time", clock() - clk);
}

/**Function*************************************************************

  Synopsis    [Apply decomposition to truth tables.]

  Description []
               
  SideEffects []

  SeeAlso     []

 ***********************************************************************/
void Abc_TruthRpoTest(char * pFileName, int nVarNum, int nThreshold, int fVerbose) {
    Rpo_TtStore_t * p;

    // allocate data-structure
//    if (fVerbose) {
//        Abc_Print(-2, "Number of variables = %d\n", nVarNum);
//    }
    p = Abc_TtStoreLoad(pFileName, nVarNum);

    if (fVerbose) {
        Abc_Print(-2, "Number of variables = %d\n", p->nVars);
    }
    // consider functions from the file
    Abc_TruthRpoPerform(p, nThreshold, fVerbose);

    // delete data-structure
    Abc_TtStoreFree(p, nVarNum);
    //    printf( "Finished decomposing truth tables from file \"%s\".\n", pFileName );
}

/**Function*************************************************************

  Synopsis    [Testbench for decomposition algorithms.]

  Description []
               
  SideEffects []

  SeeAlso     []

 ***********************************************************************/
int Abc_RpoTest(char * pFileName, int nVarNum,int nThreshold, int fVerbose) {
    if (fVerbose) {
        printf("Using truth tables from file \"%s\"...\n", pFileName);
    }
    Abc_TruthRpoTest(pFileName, nVarNum, nThreshold, fVerbose);
    fflush(stdout);
    return 0;
}


/////////////////////ert truth table to ///////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END


