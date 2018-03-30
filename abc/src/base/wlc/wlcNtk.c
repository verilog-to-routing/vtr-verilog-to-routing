/**CFile****************************************************************

  FileName    [wlcNtk.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Verilog parser.]

  Synopsis    [Network data-structure.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 22, 2014.]

  Revision    [$Id: wlcNtk.c,v 1.00 2014/09/12 00:00:00 alanmi Exp $]

***********************************************************************/

#include <math.h>
#include "wlc.h"
#include "misc/vec/vecWec.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// object types
static char * Wlc_Names[WLC_OBJ_NUMBER+1] = { 
    NULL,                  // 00: unknown
    "pi",                  // 01: primary input 
    "po",                  // 02: primary output (unused)
    "ff",                  // 03: flop output
    "bi",                  // 04: flop input (unused)
    "ff",                  // 05: flop (unused)
    "const",               // 06: constant
    "buf",                 // 07: buffer
    "mux",                 // 08: multiplexer
    ">>",                  // 09: shift right
    ">>>",                 // 10: shift right (arithmetic)
    "<<",                  // 11: shift left
    "<<<",                 // 12: shift left (arithmetic)
    "rotR",                // 13: rotate right
    "rotL",                // 14: rotate left
    "~",                   // 15: bitwise NOT
    "&",                   // 16: bitwise AND
    "|",                   // 17: bitwise OR
    "^",                   // 18: bitwise XOR
    "~&",                  // 19: bitwise NAND
    "~|",                  // 20: bitwise NOR
    "~^",                  // 21: bitwise NXOR
    "[:]",                 // 22: bit selection
    "{,}",                 // 23: bit concatenation
    "zPad",                // 24: zero padding
    "sExt",                // 25: sign extension
    "!",                   // 26: logic NOT
    "=>",                  // 27: logic implication
    "&&",                  // 28: logic AND
    "||",                  // 29: logic OR
    "^^",                  // 30: logic XOR
    "==",                  // 31: compare equal
    "!=",                  // 32: compare not equal
    "<",                   // 33: compare less
    ">",                   // 34: compare more
    "<=",                  // 35: compare less or equal
    ">=",                  // 36: compare more or equal
    "&",                   // 37: reduction AND
    "|",                   // 38: reduction OR
    "^",                   // 39: reduction XOR
    "~&",                  // 40: reduction NAND
    "~|",                  // 41: reduction NOR
    "~^",                  // 42: reduction NXOR
    "+",                   // 43: arithmetic addition
    "-",                   // 44: arithmetic subtraction
    "*",                   // 45: arithmetic multiplier
    "/",                   // 46: arithmetic division
    "%",                   // 47: arithmetic reminder
    "mod",                 // 48: arithmetic modulus
    "**",                  // 49: arithmetic power
    "-",                   // 50: arithmetic minus
    "sqrt",                // 51: integer square root
    "squar",               // 52: integer square
    "table",               // 53: bit table
    NULL                   // 54: unused
};

char * Wlc_ObjTypeName( Wlc_Obj_t * p ) { return Wlc_Names[p->Type]; }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wlc_ManSetDefaultParams( Wlc_Par_t * pPars )
{
    memset( pPars, 0, sizeof(Wlc_Par_t) );
    pPars->nBitsAdd      = ABC_INFINITY;   // adder bit-width
    pPars->nBitsMul      = ABC_INFINITY;   // multiplier bit-widht 
    pPars->nBitsMux      = ABC_INFINITY;   // MUX bit-width
    pPars->nBitsFlop     = ABC_INFINITY;   // flop bit-width
    pPars->nIterMax      =         1000;   // the max number of iterations
    pPars->nLimit        = ABC_INFINITY;   // the max number of signals
    pPars->fXorOutput    =            1;   // XOR outputs of word-level miter
    pPars->fCheckClauses =            1;   // Check clauses in the reloaded trace                    
    pPars->fPushClauses  =            0;   // Push clauses in the reloaded trace                    
    pPars->fMFFC         =            1;   // Refine the entire MFFC of a PPI 
    pPars->fPdra         =            0;   // Use pdr -nct 
    pPars->fLoadTrace    =            1;   // Load previous PDR traces 
    pPars->fProofRefine  =            0;   // Use proof-based refinement
    pPars->fHybrid       =            1;   // Use a hybrid of CBR and PBR
    pPars->fCheckCombUnsat =          0;   // Check if ABS becomes comb. unsat
    pPars->fAbs2         =            0;   // Use UFAR style createAbs
    pPars->fProofUsePPI  =            0;   // Use PPI values in PBR
    pPars->fUseBmc3      =            0;   // Run BMC3 in parallel
    pPars->fShrinkAbs    =            0;   // Shrink Abs with BMC
    pPars->fShrinkScratch=            0;   // Restart pdr from scratch after shrinking
    pPars->fVerbose      =            0;   // verbose output`
    pPars->fPdrVerbose   =            0;   // show verbose PDR output
}

/**Function*************************************************************

  Synopsis    [Working with models.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Wlc_Ntk_t * Wlc_NtkAlloc( char * pName, int nObjsAlloc )
{
    Wlc_Ntk_t * p;
    p = ABC_CALLOC( Wlc_Ntk_t, 1 );
    p->pName = pName ? Extra_FileNameGeneric( pName ) : NULL;
    Vec_IntGrow( &p->vPis, 111 );
    Vec_IntGrow( &p->vPos, 111 );
    Vec_IntGrow( &p->vCis, 111 );
    Vec_IntGrow( &p->vCos, 111 );
    Vec_IntGrow( &p->vFfs, 111 );
    p->pMemFanin = Mem_FlexStart();
    p->nObjsAlloc = nObjsAlloc;  
    p->pObjs = ABC_CALLOC( Wlc_Obj_t, p->nObjsAlloc );
    p->iObj = 1;
    return p;
}
void Wlc_ObjSetCi( Wlc_Ntk_t * p, Wlc_Obj_t * pObj )
{
    assert( Wlc_ObjIsCi(pObj) );
    assert( Wlc_ObjFaninNum(pObj) == 0 );
    if ( Wlc_NtkPiNum(p) == Wlc_NtkCiNum(p) || pObj->Type != WLC_OBJ_PI )
    {
        pObj->Fanins[1] = Vec_IntSize(&p->vCis);
        Vec_IntPush( &p->vCis, Wlc_ObjId(p, pObj) );
    }
    else // insert in the array of CI at the end of PIs
    {
        Wlc_Obj_t * pTemp; int i;
        Vec_IntInsert( &p->vCis, Wlc_NtkPiNum(p), Wlc_ObjId(p, pObj) );
        // other CI IDs are invalidated... naive fix!
        Wlc_NtkForEachCi( p, pTemp, i )
            pTemp->Fanins[1] = i;
    }
    if ( pObj->Type == WLC_OBJ_PI )
        Vec_IntPush( &p->vPis, Wlc_ObjId(p, pObj) );
}
void Wlc_ObjSetCo( Wlc_Ntk_t * p, Wlc_Obj_t * pObj, int fFlopInput )
{
//    pObj->Fanins[1] = Vec_IntSize(&p->vCos);
    Vec_IntPush( &p->vCos, Wlc_ObjId(p, pObj) );
    if ( !fFlopInput )
        Vec_IntPush( &p->vPos, Wlc_ObjId(p, pObj) );
    if ( fFlopInput )
        pObj->fIsFi = 1;
    else
        pObj->fIsPo = 1;
}
int Wlc_ObjAlloc( Wlc_Ntk_t * p, int Type, int Signed, int End, int Beg )
{
    Wlc_Obj_t * pObj;
    assert( Type != WLC_OBJ_PO && Type != WLC_OBJ_FI );
    if ( p->iObj == p->nObjsAlloc )
    {
        p->pObjs = ABC_REALLOC( Wlc_Obj_t, p->pObjs, 2 * p->nObjsAlloc );
        memset( p->pObjs + p->nObjsAlloc, 0, sizeof(Wlc_Obj_t) * p->nObjsAlloc );
        p->nObjsAlloc *= 2;
    }
    pObj = Wlc_NtkObj( p, p->iObj );
    pObj->Type   = Type;
    pObj->Signed = Signed;
    pObj->End    = End;
    pObj->Beg    = Beg;
    if ( Wlc_ObjIsCi(pObj) )
        Wlc_ObjSetCi( p, pObj );
    p->nObjs[Type]++;
    return p->iObj++;
}
int Wlc_ObjCreate( Wlc_Ntk_t * p, int Type, int Signed, int End, int Beg, Vec_Int_t * vFanins )
{
    int iFaninNew = Wlc_ObjAlloc( p, Type, Signed, End, Beg );
    Wlc_ObjAddFanins( p, Wlc_NtkObj(p, iFaninNew), vFanins );
    return iFaninNew;
}
char * Wlc_ObjName( Wlc_Ntk_t * p, int iObj )
{
    static char Buffer[100];
    if ( Wlc_NtkHasNameId(p) && Wlc_ObjNameId(p, iObj) )
        return Abc_NamStr( p->pManName, Wlc_ObjNameId(p, iObj) );
    sprintf( Buffer, "n%d", iObj );
    return Buffer;
}                          
void Wlc_ObjUpdateType( Wlc_Ntk_t * p, Wlc_Obj_t * pObj, int Type )
{
    assert( pObj->Type == WLC_OBJ_NONE );
    p->nObjs[pObj->Type]--;
    pObj->Type = Type;
    p->nObjs[pObj->Type]++;
}
void Wlc_ObjAddFanins( Wlc_Ntk_t * p, Wlc_Obj_t * pObj, Vec_Int_t * vFanins )
{
    assert( pObj->nFanins == 0 );
    pObj->nFanins = Vec_IntSize(vFanins);
    // special treatment of CONST, SELECT and TABLE
    if ( pObj->Type == WLC_OBJ_CONST )
        pObj->nFanins = 0;
    else if ( pObj->Type == WLC_OBJ_BIT_SELECT || pObj->Type == WLC_OBJ_TABLE )
        pObj->nFanins = 1;
    if ( Wlc_ObjHasArray(pObj) )
        pObj->pFanins[0] = (int *)Mem_FlexEntryFetch( p->pMemFanin, Vec_IntSize(vFanins) * sizeof(int) );
    memcpy( Wlc_ObjFanins(pObj), Vec_IntArray(vFanins), sizeof(int) * Vec_IntSize(vFanins) );
}
void Wlc_NtkFree( Wlc_Ntk_t * p )
{
    if ( p->pManName )
        Abc_NamStop( p->pManName );
    if ( p->pMemFanin )
        Mem_FlexStop( p->pMemFanin, 0 );
    if ( p->pMemTable )
        Mem_FlexStop( p->pMemTable, 0 );
    Vec_PtrFreeP( &p->vTables );
    ABC_FREE( p->vPis.pArray );
    ABC_FREE( p->vPos.pArray );
    ABC_FREE( p->vCis.pArray );
    ABC_FREE( p->vCos.pArray );
    ABC_FREE( p->vFfs.pArray );
    Vec_IntFreeP( &p->vInits );
    ABC_FREE( p->vTravIds.pArray );
    ABC_FREE( p->vNameIds.pArray );
    ABC_FREE( p->vValues.pArray );
    ABC_FREE( p->vCopies.pArray );
    ABC_FREE( p->vBits.pArray );
    ABC_FREE( p->vLevels.pArray );
    ABC_FREE( p->vRefs.pArray );
    ABC_FREE( p->pInits );
    ABC_FREE( p->pObjs );
    ABC_FREE( p->pName );
    ABC_FREE( p->pSpec );
    ABC_FREE( p );
}
int Wlc_NtkMemUsage( Wlc_Ntk_t * p )
{
    int Mem = sizeof(Wlc_Ntk_t);
    Mem += 4 * p->vPis.nCap;
    Mem += 4 * p->vPos.nCap;
    Mem += 4 * p->vCis.nCap;
    Mem += 4 * p->vCos.nCap;
    Mem += 4 * p->vFfs.nCap;
    Mem += sizeof(Wlc_Obj_t) * p->nObjsAlloc;
    Mem += Abc_NamMemUsed(p->pManName);
    Mem += Mem_FlexReadMemUsage(p->pMemFanin);
    return Mem;
}

/**Function*************************************************************

  Synopsis    [Assigns object levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Wlc_NtkCreateLevels( Wlc_Ntk_t * p )
{
    Wlc_Obj_t * pObj; 
    int i, k, iFanin, Level, LevelMax = 0;
    Vec_IntFill( &p->vLevels, Wlc_NtkObjNumMax(p), 0 );
    Wlc_NtkForEachObj( p, pObj, i )
    {
        Level = 0;
        Wlc_ObjForEachFanin( pObj, iFanin, k )
            Level = Abc_MaxInt( Level, Wlc_ObjLevelId(p, iFanin) + 1 );
        Vec_IntWriteEntry( &p->vLevels, i, Level );
        LevelMax = Abc_MaxInt( LevelMax, Level );
    }
    return LevelMax;
}
int Wlc_NtkCreateLevelsRev( Wlc_Ntk_t * p )
{
    Wlc_Obj_t * pObj; 
    int i, k, iFanin, Level, LevelMax = 0;
    Vec_IntFill( &p->vLevels, Wlc_NtkObjNumMax(p), 0 );
    Wlc_NtkForEachObjReverse( p, pObj, i )
    {
        if ( Wlc_ObjIsCi(pObj) )
            continue;
        Level = Wlc_ObjLevel(p, pObj) + 1;
        Wlc_ObjForEachFanin( pObj, iFanin, k )
            Vec_IntUpdateEntry( &p->vLevels, iFanin, Level );
        LevelMax = Abc_MaxInt( LevelMax, Level );
    }
    // reverse the values
    Wlc_NtkForEachObj( p, pObj, i )
        Vec_IntWriteEntry( &p->vLevels, i, LevelMax - Wlc_ObjLevelId(p, i) );
    Wlc_NtkForEachCi( p, pObj, i )
        Vec_IntWriteEntry( &p->vLevels, Wlc_ObjId(p, pObj), 0 );
    return LevelMax;
}

/**Function*************************************************************

  Synopsis    [Collects statistics for each side of the miter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wlc_NtkCollectStats( Wlc_Ntk_t * p, int nObjs[2][WLC_OBJ_NUMBER] )
{
    Wlc_Obj_t * pObj;
    int n, i;
    if ( Wlc_NtkPoNum(p) != 2 )
        return;
    for ( n = 0; n < 2; n++ )
    {
        Wlc_NtkMarkCone( p, n, 1, 1, 0 );
        Wlc_NtkForEachObj( p, pObj, i )
            if ( pObj->Mark )
                nObjs[n][pObj->Type]++;
    }
    Wlc_NtkCleanMarks( p );
}
int Wlc_NtkCountRealPis( Wlc_Ntk_t * p )
{
    Wlc_Obj_t * pObj;
    int i, Count = 0;
    Wlc_NtkMarkCone( p, -1, -1, 1, 0 );
    Wlc_NtkForEachPi( p, pObj, i )
        Count += pObj->Mark;
    Wlc_NtkCleanMarks( p );
    return Count;
}

/**Function*************************************************************

  Synopsis    [Prints distribution of operator types.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_WrdSelectSortCost2( word * pArray, int nSize, word * pCosts )
{
    int i, j, best_i;
    for ( i = 0; i < nSize-1; i++ )
    {
        best_i = i;
        for ( j = i+1; j < nSize; j++ )
            if ( pCosts[j] < pCosts[best_i] )
                best_i = j;
        ABC_SWAP( word, pArray[i], pArray[best_i] );
        ABC_SWAP( word, pCosts[i], pCosts[best_i] );
    }
}
static inline word Wlc_NtkPrintDistribMakeSign( int s, int s0, int s1 )
{
    return ((word)s1 << 42) | ((word)s0 << 21) | (word)s;
}
static inline void Wlc_NtkPrintDistribFromSign( word sss, int * s, int * s0, int * s1 )
{
    *s1 =  (int)(sss >> 42);  *s0 = (int)(sss >> 21) & 0x1FFFFF;  *s  =  (int)sss & 0x1FFFFF;
}
static inline void Wlc_NtkPrintDistribAddOne( Vec_Ptr_t * vTypes, Vec_Ptr_t * vOccurs, int Type, word Sign )
{
    Vec_Wrd_t * vType  = (Vec_Wrd_t *)Vec_PtrEntry( vTypes, Type );
    Vec_Wrd_t * vOccur = (Vec_Wrd_t *)Vec_PtrEntry( vOccurs, Type );
    word Entry; int i;
    Vec_WrdForEachEntry( vType, Entry, i )
        if ( Entry == Sign )
        {
            Vec_WrdAddToEntry( vOccur, i, 1 );
            return;
        }
    Vec_WrdPush( vType, Sign );
    Vec_WrdPush( vOccur, 1 );
}
void Wlc_NtkPrintDistribSortOne( Vec_Ptr_t * vTypes, Vec_Ptr_t * vOccurs, int Type )
{
    Vec_Wrd_t * vType  = (Vec_Wrd_t *)Vec_PtrEntry( vTypes, Type );
    Vec_Wrd_t * vOccur = (Vec_Wrd_t *)Vec_PtrEntry( vOccurs, Type );
    Vec_WrdSelectSortCost2( Vec_WrdArray(vType), Vec_WrdSize(vType), Vec_WrdArray(vOccur) );
    Vec_WrdReverseOrder( vType );
    Vec_WrdReverseOrder( vOccur );
}
void Wlc_NtkPrintDistrib( Wlc_Ntk_t * p, int fTwoSides, int fVerbose )
{
    int nObjs[2][WLC_OBJ_NUMBER] = {{0}}; // counter of objects of each type
    Wlc_Obj_t * pObj, * pObjRange = NULL; int nCountRange = 0;
    Vec_Ptr_t * vTypes, * vOccurs;
    Vec_Int_t * vAnds = Vec_IntStart( WLC_OBJ_NUMBER );
    word Sign;
    int i, k, s, s0, s1;
    if ( Wlc_NtkPoNum(p) != 2 )
        fTwoSides = 0;
    if ( fTwoSides )
        Wlc_NtkCollectStats( p, nObjs );
    // allocate statistics arrays
    vTypes  = Vec_PtrStart( WLC_OBJ_NUMBER );
    vOccurs = Vec_PtrStart( WLC_OBJ_NUMBER );
    for ( i = 0; i < WLC_OBJ_NUMBER; i++ )
        Vec_PtrWriteEntry( vTypes, i, Vec_WrdAlloc(16) );
    for ( i = 0; i < WLC_OBJ_NUMBER; i++ )
        Vec_PtrWriteEntry( vOccurs, i, Vec_WrdAlloc(16) );
    // add nodes
    Wlc_NtkForEachObj( p, pObj, i )
    {
//        char * pName = Wlc_ObjName(p, i);
        if ( Wlc_ObjSign(pObj) > 0x1FFFFF )
            printf( "Object %6d has range %d, which is reduced to %d in the statistics.\n", 
                i, Wlc_ObjRange(pObj), Wlc_ObjRange(pObj) & 0xFFFFF );
        if ( pObj->Beg )
        {
            if ( pObjRange == NULL )
                pObjRange = pObj;
            nCountRange++;
        }
        // 0-input types
        if ( Wlc_ObjIsCi(pObj) || pObj->Type == WLC_OBJ_CONST || pObj->Type == WLC_OBJ_BIT_CONCAT )
            Sign = Wlc_NtkPrintDistribMakeSign( Wlc_ObjSign(pObj), 0, 0 );
        // 1-input types
        else if ( pObj->Type == WLC_OBJ_BUF    || pObj->Type == WLC_OBJ_BIT_SELECT  || pObj->Type == WLC_OBJ_TABLE ||
             pObj->Type == WLC_OBJ_BIT_ZEROPAD || pObj->Type == WLC_OBJ_BIT_SIGNEXT || 
             pObj->Type == WLC_OBJ_BIT_NOT     || pObj->Type == WLC_OBJ_LOGIC_NOT   || pObj->Type == WLC_OBJ_ARI_MINUS )
            Sign = Wlc_NtkPrintDistribMakeSign( Wlc_ObjSign(pObj), Wlc_ObjSign(Wlc_ObjFanin0(p, pObj)), 0 );
        // 2-input types (including MUX)
        else if ( Wlc_ObjFaninNum(pObj) == 1 )
            Sign = Wlc_NtkPrintDistribMakeSign( Wlc_ObjSign(pObj), Wlc_ObjSign(Wlc_ObjFanin0(p, pObj)), 0 );
        else
        {
            assert( Wlc_ObjFaninNum(pObj) >= 2 );
            Sign = Wlc_NtkPrintDistribMakeSign( Wlc_ObjSign(pObj), Wlc_ObjSign(Wlc_ObjFanin0(p, pObj)), Wlc_ObjSign(Wlc_ObjFanin1(p, pObj)) );
        }
        // add to storage
        Wlc_NtkPrintDistribAddOne( vTypes, vOccurs, pObj->Type, Sign );
        // count the number of AIG nodes
        if ( pObj->Type == WLC_OBJ_MUX )
            Vec_IntAddToEntry( vAnds, WLC_OBJ_MUX,      3 * Wlc_ObjRange(pObj) * (Wlc_ObjFaninNum(pObj) - 2) );
        else if ( pObj->Type == WLC_OBJ_SHIFT_R )      
            Vec_IntAddToEntry( vAnds, WLC_OBJ_SHIFT_R,  Abc_MinInt(Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)), Abc_Base2Log(Wlc_ObjRange(pObj))) * 3 );
        else if ( pObj->Type == WLC_OBJ_SHIFT_RA )     
            Vec_IntAddToEntry( vAnds, WLC_OBJ_SHIFT_RA, Wlc_ObjRange(pObj) * Abc_MinInt(Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)), Abc_Base2Log(Wlc_ObjRange(pObj))) * 3 );
        else if ( pObj->Type == WLC_OBJ_SHIFT_L )      
            Vec_IntAddToEntry( vAnds, WLC_OBJ_SHIFT_L,  Wlc_ObjRange(pObj) * Abc_MinInt(Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)), Abc_Base2Log(Wlc_ObjRange(pObj))) * 3 );
        else if ( pObj->Type == WLC_OBJ_SHIFT_LA )     
            Vec_IntAddToEntry( vAnds, WLC_OBJ_SHIFT_LA, Wlc_ObjRange(pObj) * Abc_MinInt(Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)), Abc_Base2Log(Wlc_ObjRange(pObj))) * 3 );
        else if ( pObj->Type == WLC_OBJ_ROTATE_R )     
            Vec_IntAddToEntry( vAnds, WLC_OBJ_ROTATE_R, Wlc_ObjRange(pObj) * Abc_MinInt(Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)), Abc_Base2Log(Wlc_ObjRange(pObj))) * 3 );
        else if ( pObj->Type == WLC_OBJ_ROTATE_L )     
            Vec_IntAddToEntry( vAnds, WLC_OBJ_ROTATE_L, Wlc_ObjRange(pObj) * Abc_MinInt(Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)), Abc_Base2Log(Wlc_ObjRange(pObj))) * 3 );
        else if ( pObj->Type == WLC_OBJ_BIT_NOT )     
            Vec_IntAddToEntry( vAnds, WLC_OBJ_BIT_NOT, 0 );
        else if ( pObj->Type == WLC_OBJ_BIT_AND )      
            Vec_IntAddToEntry( vAnds, WLC_OBJ_BIT_AND,      Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) );
        else if ( pObj->Type == WLC_OBJ_BIT_OR )       
            Vec_IntAddToEntry( vAnds, WLC_OBJ_BIT_OR,       Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) );
        else if ( pObj->Type == WLC_OBJ_BIT_XOR )      
            Vec_IntAddToEntry( vAnds, WLC_OBJ_BIT_XOR,  3 * Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) );
        else if ( pObj->Type == WLC_OBJ_BIT_NAND )      
            Vec_IntAddToEntry( vAnds, WLC_OBJ_BIT_NAND,     Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) );
        else if ( pObj->Type == WLC_OBJ_BIT_NOR )       
            Vec_IntAddToEntry( vAnds, WLC_OBJ_BIT_NOR,      Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) );
        else if ( pObj->Type == WLC_OBJ_BIT_NXOR )      
            Vec_IntAddToEntry( vAnds, WLC_OBJ_BIT_NXOR, 3 * Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) );
        else if ( pObj->Type == WLC_OBJ_BIT_SELECT )   
            Vec_IntAddToEntry( vAnds, WLC_OBJ_BIT_SELECT, 0 );
        else if ( pObj->Type == WLC_OBJ_BIT_CONCAT )   
            Vec_IntAddToEntry( vAnds, WLC_OBJ_BIT_CONCAT, 0 );
        else if ( pObj->Type == WLC_OBJ_BIT_ZEROPAD )  
            Vec_IntAddToEntry( vAnds, WLC_OBJ_BIT_ZEROPAD, 0 );
        else if ( pObj->Type == WLC_OBJ_BIT_SIGNEXT )  
            Vec_IntAddToEntry( vAnds, WLC_OBJ_BIT_SIGNEXT, 0 );
        else if ( pObj->Type == WLC_OBJ_LOGIC_NOT )    
            Vec_IntAddToEntry( vAnds, WLC_OBJ_LOGIC_NOT,        Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) - 1 );
        else if ( pObj->Type == WLC_OBJ_LOGIC_IMPL )    
            Vec_IntAddToEntry( vAnds, WLC_OBJ_LOGIC_IMPL,       Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) + Wlc_ObjRange(Wlc_ObjFanin1(p, pObj)) - 1 );
        else if ( pObj->Type == WLC_OBJ_LOGIC_AND )    
            Vec_IntAddToEntry( vAnds, WLC_OBJ_LOGIC_AND,        Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) + Wlc_ObjRange(Wlc_ObjFanin1(p, pObj)) - 1 );
        else if ( pObj->Type == WLC_OBJ_LOGIC_OR )     
            Vec_IntAddToEntry( vAnds, WLC_OBJ_LOGIC_OR,         Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) + Wlc_ObjRange(Wlc_ObjFanin1(p, pObj)) - 1 );
        else if ( pObj->Type == WLC_OBJ_LOGIC_XOR )    
            Vec_IntAddToEntry( vAnds, WLC_OBJ_LOGIC_XOR,        Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) + Wlc_ObjRange(Wlc_ObjFanin1(p, pObj)) + 1 );
        else if ( pObj->Type == WLC_OBJ_COMP_EQU )     
            Vec_IntAddToEntry( vAnds, WLC_OBJ_COMP_EQU,     4 * Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) - 1 );
        else if ( pObj->Type == WLC_OBJ_COMP_NOTEQU )  
            Vec_IntAddToEntry( vAnds, WLC_OBJ_COMP_NOTEQU,  4 * Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) - 1 );
        else if ( pObj->Type == WLC_OBJ_COMP_LESS )    
            Vec_IntAddToEntry( vAnds, WLC_OBJ_COMP_LESS,    6 * Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) - 6 );
        else if ( pObj->Type == WLC_OBJ_COMP_MORE )    
            Vec_IntAddToEntry( vAnds, WLC_OBJ_COMP_MORE,    6 * Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) - 6 );
        else if ( pObj->Type == WLC_OBJ_COMP_LESSEQU ) 
            Vec_IntAddToEntry( vAnds, WLC_OBJ_COMP_LESSEQU, 6 * Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) - 6 );
        else if ( pObj->Type == WLC_OBJ_COMP_MOREEQU ) 
            Vec_IntAddToEntry( vAnds, WLC_OBJ_COMP_MOREEQU, 6 * Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) - 6 );
        else if ( pObj->Type == WLC_OBJ_REDUCT_AND )   
            Vec_IntAddToEntry( vAnds, WLC_OBJ_REDUCT_AND,       Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) - 1 );
        else if ( pObj->Type == WLC_OBJ_REDUCT_OR )    
            Vec_IntAddToEntry( vAnds, WLC_OBJ_REDUCT_OR,        Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) - 1 );
        else if ( pObj->Type == WLC_OBJ_REDUCT_XOR )   
            Vec_IntAddToEntry( vAnds, WLC_OBJ_REDUCT_XOR,   3 * Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) - 3 );
        else if ( pObj->Type == WLC_OBJ_REDUCT_NAND )   
            Vec_IntAddToEntry( vAnds, WLC_OBJ_REDUCT_NAND,      Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) - 1 );
        else if ( pObj->Type == WLC_OBJ_REDUCT_NOR )    
            Vec_IntAddToEntry( vAnds, WLC_OBJ_REDUCT_NOR,       Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) - 1 );
        else if ( pObj->Type == WLC_OBJ_REDUCT_NXOR )   
            Vec_IntAddToEntry( vAnds, WLC_OBJ_REDUCT_NXOR,  3 * Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) - 3 );
        else if ( pObj->Type == WLC_OBJ_ARI_ADD )       
            Vec_IntAddToEntry( vAnds, WLC_OBJ_ARI_ADD,      9 * Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) );
        else if ( pObj->Type == WLC_OBJ_ARI_SUB )       
            Vec_IntAddToEntry( vAnds, WLC_OBJ_ARI_SUB,      9 * Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) );
        else if ( pObj->Type == WLC_OBJ_ARI_MULTI )    
            Vec_IntAddToEntry( vAnds, WLC_OBJ_ARI_MULTI,    9 * Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) * Wlc_ObjRange(Wlc_ObjFanin1(p, pObj)) );
        else if ( pObj->Type == WLC_OBJ_ARI_DIVIDE )   
            Vec_IntAddToEntry( vAnds, WLC_OBJ_ARI_DIVIDE,  13 * Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) * Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) - 19 * Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) + 10 );
        else if ( pObj->Type == WLC_OBJ_ARI_REM )  
            Vec_IntAddToEntry( vAnds, WLC_OBJ_ARI_REM,     13 * Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) * Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) - 7 * Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) - 2  );
        else if ( pObj->Type == WLC_OBJ_ARI_MODULUS )  
            Vec_IntAddToEntry( vAnds, WLC_OBJ_ARI_MODULUS, 13 * Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) * Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) - 7 * Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) - 2  );
        else if ( pObj->Type == WLC_OBJ_ARI_POWER ) 
            Vec_IntAddToEntry( vAnds, WLC_OBJ_ARI_POWER,   10 * (int)pow((double)Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)),(double)Wlc_ObjRange(Wlc_ObjFanin0(p, pObj))) );
        else if ( pObj->Type == WLC_OBJ_ARI_MINUS )   
            Vec_IntAddToEntry( vAnds, WLC_OBJ_ARI_MINUS,    4 * Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) );
        else if ( pObj->Type == WLC_OBJ_ARI_SQRT )    
            Vec_IntAddToEntry( vAnds, WLC_OBJ_ARI_SQRT,    11 * Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) * Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) / 8 + 5 * Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) / 2 - 5  );
        else if ( pObj->Type == WLC_OBJ_ARI_SQUARE )    
            Vec_IntAddToEntry( vAnds, WLC_OBJ_ARI_SQUARE,   5 * Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) * Wlc_ObjRange(Wlc_ObjFanin1(p, pObj))  );
    }
    if ( nCountRange && Vec_IntSize(&p->vNameIds) > 0 )
    {
        printf( "Warning: %d objects of the design have non-zero-based ranges.\n", nCountRange );
        printf( "In particular, object %6d with name \"%s\" has range %d=[%d:%d]\n", Wlc_ObjId(p, pObjRange), 
            Abc_NamStr(p->pManName, Wlc_ObjNameId(p, Wlc_ObjId(p, pObjRange))), Wlc_ObjRange(pObjRange), pObjRange->End, pObjRange->Beg );
    }
    // print by occurrence
    printf( "ID  :  name  occurrence%s    and2 (occurrence)<output_range>=<input_range>.<input_range> ...\n", fTwoSides ? "     Left Share Right":"" );
    for ( i = 0; i < WLC_OBJ_NUMBER; i++ )
    {
        Vec_Wrd_t * vType  = (Vec_Wrd_t *)Vec_PtrEntry( vTypes, i );
        Vec_Wrd_t * vOccur = (Vec_Wrd_t *)Vec_PtrEntry( vOccurs, i );
        if ( p->nObjs[i] == 0 )
            continue;
        printf( "%2d  :  %-8s  %6d", i, Wlc_Names[i], p->nObjs[i] );
        if ( fTwoSides )
        {
            int nTotal = i == WLC_OBJ_PI ? Wlc_NtkCountRealPis(p) : p->nObjs[i];
            printf( "   " );
            printf( "%6d", nObjs[0][i] );
            printf( "%6d", nObjs[0][i]+nObjs[1][i]-nTotal );
            printf( "%6d", nObjs[1][i] );
        }
        printf( "%8d ", Vec_IntEntry(vAnds, i) );
        // sort by occurence
        Wlc_NtkPrintDistribSortOne( vTypes, vOccurs, i );
        Vec_WrdForEachEntry( vType, Sign, k )
        {
            Wlc_NtkPrintDistribFromSign( Sign, &s, &s0, &s1 );
            if ( ((k % 6) == 5 && s1) || ((k % 8) == 7 && !s1) )
            {
                printf( "\n                                " );
                if ( fTwoSides )
                    printf( "                     " );
            }
            printf( "(%d)", (int)Vec_WrdEntry( vOccur, k ) );
            printf( "%s%d",      Abc_LitIsCompl(s)?"-":"",  Abc_Lit2Var(s) );
            if ( s0 )
                printf( "=%s%d", Abc_LitIsCompl(s0)?"-":"", Abc_Lit2Var(s0) );
            if ( s1 )
                printf( ".%s%d", Abc_LitIsCompl(s1)?"-":"", Abc_Lit2Var(s1) );
            printf( " " );
        }
        printf( "\n" );
    }
    Vec_VecFree( (Vec_Vec_t *)vTypes );
    Vec_VecFree( (Vec_Vec_t *)vOccurs );
    Vec_IntFree( vAnds );
}
void Wlc_NtkPrintNode( Wlc_Ntk_t * p, Wlc_Obj_t * pObj )
{
    printf( "%8d  :  ", Wlc_ObjId(p, pObj) );
    printf( "%3d%s",    Wlc_ObjRange(pObj), Wlc_ObjIsSigned(pObj) ? "s" : " " );
    if ( pObj->Type == WLC_OBJ_CONST )
        printf( "                          " );
    else
    {
        printf( " = %3d%s  %5s  ",  Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)), Wlc_ObjIsSigned(Wlc_ObjFanin0(p, pObj)) ? "s" : " ", Wlc_Names[(int)pObj->Type] );
        if ( Wlc_ObjFaninNum(pObj) > 1 )
            printf( "%3d%s ",       Wlc_ObjRange(Wlc_ObjFanin1(p, pObj)), Wlc_ObjIsSigned(Wlc_ObjFanin1(p, pObj)) ? "s" : " " );
        else
            printf( "     " );
        if ( Wlc_ObjFaninNum(pObj) > 2 )
            printf( "%3d%s ",       Wlc_ObjRange(Wlc_ObjFanin2(p, pObj)), Wlc_ObjIsSigned(Wlc_ObjFanin2(p, pObj)) ? "s" : " " );
        else
            printf( "     " );
    }
    printf( " :    " );
    printf( "%-12s",   Wlc_ObjName(p, Wlc_ObjId(p, pObj)) );
    if ( pObj->Type == WLC_OBJ_CONST )
    {
        printf( " =  %d\'%sh", Wlc_ObjRange(pObj), Wlc_ObjIsSigned(pObj) ? "s":"" );
        if ( pObj->fXConst )
        {
            int k;
            for ( k = 0; k < (Wlc_ObjRange(pObj) + 3) / 4; k++ )
                printf( "x" );
        }
        else
            Abc_TtPrintHexArrayRev( stdout, (word *)Wlc_ObjConstValue(pObj), (Wlc_ObjRange(pObj) + 3) / 4 );
    }
    else
    {
        printf( " =  %-12s  %5s  ", Wlc_ObjName(p, Wlc_ObjFaninId0(pObj)), Wlc_Names[(int)pObj->Type] );
        if ( Wlc_ObjFaninNum(pObj) > 1 )
            printf( "%-12s ",       Wlc_ObjName(p, Wlc_ObjFaninId1(pObj)) );
        else
            printf( "             " );
        if ( Wlc_ObjFaninNum(pObj) > 2 )
            printf( "%-12s ",       Wlc_ObjName(p, Wlc_ObjFaninId2(pObj)) );
    }
    printf( "\n" );
}
void Wlc_NtkPrintNodeArray( Wlc_Ntk_t * p, Vec_Int_t * vArray )
{
    Wlc_Obj_t * pObj; 
    int i;
    Wlc_NtkForEachObjVec( vArray, p, pObj, i )
        Wlc_NtkPrintNode( p, pObj );
}
void Wlc_NtkPrintNodes( Wlc_Ntk_t * p, int Type )
{
    Wlc_Obj_t * pObj; 
    int i, Counter = 0;
    printf( "Operation %s\n", Wlc_Names[Type] );
    Wlc_NtkForEachObj( p, pObj, i )
    {
        if ( (int)pObj->Type != Type )
            continue;
        printf( "%8d  :", Counter++ );
        Wlc_NtkPrintNode( p, pObj );
    }
}
void Wlc_NtkPrintStats( Wlc_Ntk_t * p, int fDistrib, int fTwoSides, int fVerbose )
{
    int i;
    printf( "%-20s : ",        p->pName );
    printf( "PI = %4d  ",      Wlc_NtkCountRealPis(p) ); //Wlc_NtkPiNum(p) );
    printf( "PO = %4d  ",      Wlc_NtkPoNum(p) );
    printf( "FF = %4d  ",      Wlc_NtkFfNum(p) );
    printf( "Obj = %6d  ",     Wlc_NtkObjNum(p) - Wlc_NtkPiNum(p) - Wlc_NtkPoNum(p) - Wlc_NtkFfNum(p) );
    printf( "Mem = %.3f MB",   1.0*Wlc_NtkMemUsage(p)/(1<<20) );
    printf( "\n" );
    if ( fDistrib )
    {
        Wlc_NtkPrintDistrib( p, fTwoSides, fVerbose );
        return;
    }
    if ( !fVerbose )
        return;
    printf( "Node type statistics:\n" );
    for ( i = 1; i < WLC_OBJ_NUMBER; i++ )
    {
        if ( !p->nObjs[i] )
            continue;
        if ( p->nAnds[0] && p->nAnds[i] )
            printf( "%2d  :  %-8s  %6d  %7.2f %%\n", i, Wlc_Names[i], p->nObjs[i], 100.0*p->nAnds[i]/p->nAnds[0] );
        else
            printf( "%2d  :  %-8s  %6d\n", i, Wlc_Names[i], p->nObjs[i] );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wlc_NtkTransferNames( Wlc_Ntk_t * pNew, Wlc_Ntk_t * p )
{
    int i;
    assert( !Wlc_NtkHasCopy(pNew)   && Wlc_NtkHasCopy(p)   );
    assert( !Wlc_NtkHasNameId(pNew) && Wlc_NtkHasNameId(p) );
    assert( pNew->pManName  == NULL && p->pManName != NULL );
    Wlc_NtkCleanNameId( pNew );
    for ( i = 0; i < p->nObjsAlloc; i++ )
        if ( Wlc_ObjCopy(p, i) && i < Vec_IntSize(&p->vNameIds) && Wlc_ObjNameId(p, i) )
            Wlc_ObjSetNameId( pNew, Wlc_ObjCopy(p, i), Wlc_ObjNameId(p, i) );
    pNew->pManName = p->pManName; 
    p->pManName = NULL;
    Vec_IntErase( &p->vNameIds );
    // transfer table
    pNew->pMemTable = p->pMemTable;  p->pMemTable = NULL;
    pNew->vTables = p->vTables;      p->vTables = NULL;
}
char * Wlc_NtkNewName( Wlc_Ntk_t * p, int iCoId, int fSeq )
{
    static char pBuffer[1000];
    sprintf( pBuffer, "%s_o%d_%s", p->pName, iCoId, fSeq ? "seq": "comb" );
    return pBuffer;
}

/**Function*************************************************************

  Synopsis    [Reduce init vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Wlc_ReduceMarkedInitVec( Wlc_Ntk_t * p, Vec_Int_t * vInit )
{
    Vec_Int_t * vInitNew = Vec_IntDup( vInit );
    Wlc_Obj_t * pObj; int i, k = 0;
    assert( Vec_IntSize(vInit) == Wlc_NtkCiNum(p) - Wlc_NtkPiNum(p) );
    Wlc_NtkForEachCi( p, pObj, i )
        if ( !Wlc_ObjIsPi(pObj) && pObj->Mark )
            Vec_IntWriteEntry( vInitNew, k++, Vec_IntEntry(vInit, i - Wlc_NtkPiNum(p)) );
    Vec_IntShrink( vInitNew, k );
    return vInitNew;
}
char * Wlc_ReduceMarkedInitStr( Wlc_Ntk_t * p, char * pInit )
{
    char * pInitNew = Abc_UtilStrsav( pInit );
    Wlc_Obj_t * pObj; int i, b, nBits = 0, k = 0;
    Wlc_NtkForEachCi( p, pObj, i )
    {
        if ( !Wlc_ObjIsPi(pObj) && pObj->Mark )
            for ( b = 0; b < Wlc_ObjRange(pObj); b++ )
                pInitNew[k++] = pInitNew[nBits+b];
        if ( !Wlc_ObjIsPi(pObj) )
            nBits += Wlc_ObjRange(pObj);
    }
    pInitNew[k] = '\0';
    assert( nBits == (int)strlen(pInit) );
    return pInitNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates the network in a topological order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wlc_ObjCollectCopyFanins( Wlc_Ntk_t * p, int iObj, Vec_Int_t * vFanins )
{
    int i, iFanin;
    Wlc_Obj_t * pObj = Wlc_NtkObj( p, iObj );
    Vec_IntClear( vFanins );
    Wlc_ObjForEachFanin( pObj, iFanin, i )
        Vec_IntPush( vFanins, Wlc_ObjCopy(p, iFanin) );
    // special treatment of CONST and SELECT
    if ( pObj->Type == WLC_OBJ_CONST )
    {
        int * pInts = Wlc_ObjConstValue( pObj );
        int nInts = Abc_BitWordNum( Wlc_ObjRange(pObj) );
        for ( i = 0; i < nInts; i++ )
            Vec_IntPush( vFanins, pInts[i] );
    }
    else if ( pObj->Type == WLC_OBJ_BIT_SELECT )
    {
        assert( Vec_IntSize(vFanins) == 1 );
        Vec_IntPushTwo( vFanins, Wlc_ObjRangeEnd(pObj), Wlc_ObjRangeBeg(pObj) );
    }
    else if ( pObj->Type == WLC_OBJ_TABLE )
    {
        assert( Vec_IntSize(vFanins) == 1 );
        Vec_IntPush( vFanins, pObj->Fanins[1] );
    }
}
int Wlc_ObjDup( Wlc_Ntk_t * pNew, Wlc_Ntk_t * p, int iObj, Vec_Int_t * vFanins )
{
    Wlc_Obj_t * pObj = Wlc_NtkObj( p, iObj );
    int iFaninNew = Wlc_ObjAlloc( pNew, pObj->Type, Wlc_ObjIsSigned(pObj), pObj->End, pObj->Beg );
    Wlc_Obj_t * pObjNew = Wlc_NtkObj(pNew, iFaninNew);
    Wlc_ObjCollectCopyFanins( p, iObj, vFanins );
    Wlc_ObjAddFanins( pNew, pObjNew, vFanins );
    Wlc_ObjSetCopy( p, iObj, iFaninNew );
    pObjNew->fXConst = pObj->fXConst;
    return iFaninNew;
}
void Wlc_NtkDupDfs_rec( Wlc_Ntk_t * pNew, Wlc_Ntk_t * p, int iObj, Vec_Int_t * vFanins )
{
    Wlc_Obj_t * pObj;
    int i, iFanin;
    if ( Wlc_ObjCopy(p, iObj) )
        return;
    pObj = Wlc_NtkObj( p, iObj );
    Wlc_ObjForEachFanin( pObj, iFanin, i )
        Wlc_NtkDupDfs_rec( pNew, p, iFanin, vFanins );
    Wlc_ObjDup( pNew, p, iObj, vFanins );
}

Wlc_Ntk_t * Wlc_NtkDupDfsSimple( Wlc_Ntk_t * p )
{
    Wlc_Ntk_t * pNew;
    Wlc_Obj_t * pObj;
    Vec_Int_t * vFanins;
    int i;
    Wlc_NtkCleanCopy( p );
    vFanins = Vec_IntAlloc( 100 );
    pNew = Wlc_NtkAlloc( p->pName, p->nObjsAlloc );
    pNew->fSmtLib = p->fSmtLib;
    Wlc_NtkForEachCi( p, pObj, i )
        Wlc_ObjDup( pNew, p, Wlc_ObjId(p, pObj), vFanins );
    Wlc_NtkForEachCo( p, pObj, i )
        Wlc_NtkDupDfs_rec( pNew, p, Wlc_ObjId(p, pObj), vFanins );
    Wlc_NtkForEachCo( p, pObj, i )
        Wlc_ObjSetCo( pNew, Wlc_ObjCopyObj(pNew, p, pObj), pObj->fIsFi );
    if ( p->vInits )
    pNew->vInits = Vec_IntDup( p->vInits );
    if ( p->pInits )
    pNew->pInits = Abc_UtilStrsav( p->pInits );
    Vec_IntFree( vFanins );
    if ( p->pSpec )
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    return pNew;
}

Wlc_Ntk_t * Wlc_NtkDupDfs( Wlc_Ntk_t * p, int fMarked, int fSeq )
{
    Wlc_Ntk_t * pNew;
    Wlc_Obj_t * pObj;
    Vec_Int_t * vFanins;
    int i;
    vFanins = Vec_IntAlloc( 100 );
    Wlc_NtkCleanCopy( p );
    pNew = Wlc_NtkAlloc( p->pName, p->nObjsAlloc );
    pNew->fSmtLib = p->fSmtLib;
    Wlc_NtkForEachCi( p, pObj, i )
        if ( !fMarked || pObj->Mark )
        {
            unsigned Type = pObj->Type;
            if ( !fSeq ) pObj->Type = WLC_OBJ_PI;
            Wlc_ObjDup( pNew, p, Wlc_ObjId(p, pObj), vFanins );
            pObj->Type = Type;
        }
    Wlc_NtkForEachCo( p, pObj, i )
        if ( !fMarked || pObj->Mark )
            Wlc_NtkDupDfs_rec( pNew, p, Wlc_ObjId(p, pObj), vFanins );
    Wlc_NtkForEachCo( p, pObj, i )
        if ( !fMarked || pObj->Mark )
            Wlc_ObjSetCo( pNew, Wlc_ObjCopyObj(pNew, p, pObj), fSeq ? pObj->fIsFi : 0 );
    Vec_IntFree( vFanins );
    if ( fSeq )
    {
        if ( fMarked )
        {
            if ( p->vInits )
                pNew->vInits = Wlc_ReduceMarkedInitVec( p, p->vInits );
            if ( p->pInits )
                pNew->pInits = Wlc_ReduceMarkedInitStr( p, p->pInits );
        }
        else
        {
            if ( p->vInits )
                pNew->vInits = Vec_IntDup( p->vInits );
            if ( p->pInits )
                pNew->pInits = Abc_UtilStrsav( p->pInits );
        }
    }
    if ( p->pSpec )
        pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Wlc_NtkTransferNames( pNew, p );
    return pNew;
}
Wlc_Ntk_t * Wlc_NtkDupDfsAbs( Wlc_Ntk_t * p, Vec_Int_t * vPisOld, Vec_Int_t * vPisNew, Vec_Int_t * vFlops )
{
    Wlc_Ntk_t * pNew;
    Wlc_Obj_t * pObj;
    Vec_Int_t * vFanins;
    int i;
    Wlc_NtkCleanCopy( p );
    pNew = Wlc_NtkAlloc( p->pName, p->nObjsAlloc );
    pNew->fSmtLib = p->fSmtLib;

    // duplicate marked PIs
    vFanins = Vec_IntAlloc( 100 );
    Wlc_NtkForEachObjVec( vPisOld, p, pObj, i )
    {
        assert( Wlc_ObjIsPi(pObj) );
        Wlc_ObjDup( pNew, p, Wlc_ObjId(p, pObj), vFanins );
    }
    // duplicate additional PIs
    Wlc_NtkForEachObjVec( vPisNew, p, pObj, i )
    {
        unsigned Type = pObj->Type;
        int nFanins = Wlc_ObjFaninNum(pObj);
        assert( !Wlc_ObjIsPi(pObj) );
        pObj->Type = WLC_OBJ_PI;
        pObj->nFanins = 0;
        Wlc_ObjDup( pNew, p, Wlc_ObjId(p, pObj), vFanins );
        pObj->Type = Type;
        pObj->nFanins = (unsigned)nFanins;
    }
    // duplicate flop outputs
    Wlc_NtkForEachObjVec( vFlops, p, pObj, i )
    {
        assert( !Wlc_ObjIsPi(pObj) && Wlc_ObjIsCi(pObj) );
        Wlc_ObjDup( pNew, p, Wlc_ObjId(p, pObj), vFanins );
    }

    // duplicate logic cones of primary outputs
    Wlc_NtkForEachPo( p, pObj, i )
        Wlc_NtkDupDfs_rec( pNew, p, Wlc_ObjId(p, pObj), vFanins );
    // duplidate logic cone of flop inputs
    Wlc_NtkForEachObjVec( vFlops, p, pObj, i )
        Wlc_NtkDupDfs_rec( pNew, p, Wlc_ObjId(p, Wlc_ObjFo2Fi(p, pObj)), vFanins );

    // duplicate POs
    Wlc_NtkForEachPo( p, pObj, i )
        Wlc_ObjSetCo( pNew, Wlc_ObjCopyObj(pNew, p, pObj), 0 );
    // duplicate flop inputs 
    Wlc_NtkForEachObjVec( vFlops, p, pObj, i )
        Wlc_ObjSetCo( pNew, Wlc_ObjCopyObj(pNew, p, Wlc_ObjFo2Fi(p, pObj)), 1 );
    Vec_IntFree( vFanins );

    // mark flop outputs
    Wlc_NtkForEachObjVec( vFlops, p, pObj, i )
        pObj->Mark = 1;
    if ( p->vInits )
        pNew->vInits = Wlc_ReduceMarkedInitVec( p, p->vInits );
    if ( p->pInits )
        pNew->pInits = Wlc_ReduceMarkedInitStr( p, p->pInits );
    Wlc_NtkCleanMarks( p );

    if ( p->pSpec )
        pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    //Wlc_NtkTransferNames( pNew, p );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Select the cone of the given output.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wlc_NtkCleanMarks( Wlc_Ntk_t * p )
{
    Wlc_Obj_t * pObj;
    int i;
    Wlc_NtkForEachObj( p, pObj, i )
        pObj->Mark = 0;
}
int Wlc_NtkCountMarked( Wlc_Ntk_t * p, int * pnPis, int * pnFos, int * pnAdders, int * pnMults )
{
    Wlc_Obj_t * pObj;
    int i, nNodes = 0;
    *pnPis = *pnFos = *pnAdders = *pnMults = 0;
    Wlc_NtkForEachObj( p, pObj, i )
    {
        if ( !pObj->Mark )
            continue;
        if ( Wlc_ObjIsPi(pObj) )
            (*pnPis)++;
        else if ( Wlc_ObjIsCi(pObj) )
            (*pnFos)++;
        else if ( pObj->Mark )
        {
            nNodes++;
            if ( pObj->Type == WLC_OBJ_ARI_ADD || pObj->Type == WLC_OBJ_ARI_SUB )
                (*pnAdders)++;
            else if ( pObj->Type == WLC_OBJ_ARI_MULTI )
                (*pnMults)++;
        }
    }
    return nNodes;
}
void Wlc_NtkMarkCone_rec( Wlc_Ntk_t * p, Wlc_Obj_t * pObj, Vec_Int_t * vFlops )
{
    int i, iFanin;
    if ( pObj->Mark )
        return;
    pObj->Mark = 1;
    if ( Wlc_ObjIsCi(pObj) )
    {
        if ( !Wlc_ObjIsPi(pObj) )
            Vec_IntPush( vFlops, Wlc_ObjCiId(pObj) );
        return;
    }
    Wlc_ObjForEachFanin( pObj, iFanin, i )
        Wlc_NtkMarkCone_rec( p, Wlc_NtkObj(p, iFanin), vFlops );
}
void Wlc_NtkMarkCone( Wlc_Ntk_t * p, int iCoId, int Range, int fSeq, int fAllPis )
{
    Vec_Int_t * vFlops;
    Wlc_Obj_t * pObj;
    int i, CiId, CoId;
    Wlc_NtkCleanMarks( p );
    if ( fAllPis )
    Wlc_NtkForEachPi( p, pObj, i )
        pObj->Mark = 1;
    vFlops = Vec_IntAlloc( 100 );
    Wlc_NtkForEachCo( p, pObj, i )
        if ( iCoId == -1 || (i >= iCoId && i < iCoId + Range) )
            Wlc_NtkMarkCone_rec( p, pObj, vFlops );
    if ( fSeq )
    Vec_IntForEachEntry( vFlops, CiId, i )
    {
        CoId = Wlc_NtkPoNum(p) + CiId - Wlc_NtkPiNum(p);
        Wlc_NtkMarkCone_rec( p, Wlc_NtkCo(p, CoId), vFlops );
    }
    Vec_IntFree( vFlops );
}
void Wlc_NtkProfileCones( Wlc_Ntk_t * p )
{
    Wlc_Obj_t * pObj; 
    int i, nPis, nFos, nNodes, nAdders, nMults;
    Wlc_NtkForEachCo( p, pObj, i )
    {
        Wlc_NtkMarkCone( p, i, 1, 0, 0 );
        nNodes = Wlc_NtkCountMarked( p, &nPis, &nFos, &nAdders, &nMults );
        printf( "Cone %5d : ", i );
        printf( "PI = %4d  ", nPis );
        printf( "FO = %4d  ", nFos );
        printf( "Node = %6d  ", nNodes );
        printf( "Add/Sub = %4d  ", nAdders );
        printf( "Mult = %4d  ", nMults );
        printf( "\n" );
    }
    Wlc_NtkCleanMarks( p );
}

/**Function*************************************************************

  Synopsis    [Duplicates the network by copying each node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Wlc_Ntk_t * Wlc_NtkDupSingleNodes( Wlc_Ntk_t * p )
{
    Wlc_Ntk_t * pNew;
    Vec_Int_t * vFanins;
    Wlc_Obj_t * pObj, * pObjNew;
    Wlc_Obj_t * pFanin, * pFaninNew;
    int i, k, iFanin, iFaninNew, iObjNew, Count = 0;
    // count objects
    Wlc_NtkForEachObj( p, pObj, i )
        if ( !Wlc_ObjIsCi(pObj) )
            Count += 1 + Wlc_ObjFaninNum(pObj);
    // copy objects
    Wlc_NtkCleanCopy( p );
    vFanins = Vec_IntAlloc( 100 );
    pNew = Wlc_NtkAlloc( p->pName, p->nObjsAlloc );
    pNew->fSmtLib = p->fSmtLib;
    Wlc_NtkForEachObj( p, pObj, i )
    {
        if ( Wlc_ObjIsCi(pObj) )
            continue;
        if ( pObj->Type == WLC_OBJ_ARI_MULTI )
            continue;
        if ( pObj->Type == WLC_OBJ_MUX && Wlc_ObjFaninNum(pObj) > 3 )
            continue;
        // create CIs for the fanins
        Wlc_ObjForEachFanin( pObj, iFanin, k )
        {
            pFanin = Wlc_NtkObj(p, iFanin);
            iFaninNew = Wlc_ObjAlloc( pNew, WLC_OBJ_PI, pFanin->Signed, pFanin->End, pFanin->Beg );
            pFaninNew = Wlc_NtkObj(pNew, iFaninNew);
            Wlc_ObjSetCopy( p, iFanin, iFaninNew );
            //Wlc_ObjSetCi( pNew, pFaninNew );
        }
        // create object
        iObjNew = Wlc_ObjDup( pNew, p, i, vFanins );
        pObjNew = Wlc_NtkObj(pNew, iObjNew);
        pObjNew->fIsPo = 1;
        Vec_IntPush( &pNew->vPos, iObjNew );
    }
    Vec_IntFree( vFanins );
    Wlc_NtkTransferNames( pNew, p );
    if ( p->pSpec )
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Creates short names for all objects.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wlc_NtkShortNames( Wlc_Ntk_t * p )
{
    Wlc_Obj_t * pObj;
    char pBuffer[100];
    int nDigits, NameId, fFound, i;
    int nFlops = Wlc_NtkCoNum(p) - Wlc_NtkPoNum(p);
    nDigits = Abc_Base10Log( nFlops );
    Wlc_NtkForEachCo( p, pObj, i )
    {
        if ( Wlc_ObjIsPo(pObj) )
            continue;
        sprintf( pBuffer, "%s%0*d", "fi", nDigits, i - Wlc_NtkPoNum(p) );
        NameId = Abc_NamStrFindOrAdd( p->pManName, pBuffer, &fFound );
        Wlc_ObjSetNameId( p, Wlc_ObjId(p, pObj), NameId );
    }
    Wlc_NtkForEachCi( p, pObj, i )
    {
        if ( Wlc_ObjIsPi(pObj) )
            continue;
        sprintf( pBuffer, "%s%0*d", "fo", nDigits, i - Wlc_NtkPiNum(p) );
        NameId = Abc_NamStrFindOrAdd( p->pManName, pBuffer, &fFound );
        Wlc_ObjSetNameId( p, Wlc_ObjId(p, pObj), NameId );
    }
    nDigits = Abc_Base10Log( Wlc_NtkPoNum(p) );
    Wlc_NtkForEachPo( p, pObj, i )
    {
        sprintf( pBuffer, "%s%0*d", "po", nDigits, i );
        NameId = Abc_NamStrFindOrAdd( p->pManName, pBuffer, &fFound );
        Wlc_ObjSetNameId( p, Wlc_ObjId(p, pObj), NameId );
    }
    nDigits = Abc_Base10Log( Wlc_NtkPiNum(p) );
    Wlc_NtkForEachPi( p, pObj, i )
    {
        sprintf( pBuffer, "%s%0*d", "pi", nDigits, i );
        NameId = Abc_NamStrFindOrAdd( p->pManName, pBuffer, &fFound );
        Wlc_ObjSetNameId( p, Wlc_ObjId(p, pObj), NameId );
    }
    nDigits = Abc_Base10Log( Wlc_NtkObjNum(p) );
    Wlc_NtkForEachObj( p, pObj, i )
    {
        if ( Wlc_ObjIsCi(pObj) || Wlc_ObjIsCo(pObj) )
            continue;
        sprintf( pBuffer, "%s%0*d", "n", nDigits, i );
        NameId = Abc_NamStrFindOrAdd( p->pManName, pBuffer, &fFound );
        Wlc_ObjSetNameId( p, Wlc_ObjId(p, pObj), NameId );
    }
}

/**Function*************************************************************

  Synopsis    [Count the number of flops initialized to DC value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Wlc_NtkDcFlopNum( Wlc_Ntk_t * p )
{
    int i, nFlops, Count = 0;
    if ( p->pInits == NULL )
        return 0;
    nFlops = strlen(p->pInits);
    for ( i = 0; i < nFlops; i++ )
        Count += (p->pInits[i] == 'x' || p->pInits[i] == 'X');
    return Count;
}

/**Function*************************************************************

  Synopsis    [Create references.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wlc_NtkSetRefs( Wlc_Ntk_t * p )
{
    Wlc_Obj_t * pObj; int i, k, Fanin;
    Vec_IntFill( &p->vRefs, Wlc_NtkObjNumMax(p), 0 );
    Wlc_NtkForEachObj( p, pObj, i )
        Wlc_ObjForEachFanin( pObj, Fanin, k )
            Vec_IntAddToEntry( &p->vRefs, Fanin, 1 );
    Wlc_NtkForEachCo( p, pObj, i )
        Vec_IntAddToEntry( &p->vRefs, Wlc_ObjId(p, pObj), 1 );
}

/**Function*************************************************************

  Synopsis    [This procedure simply count the number of PPI bits.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Wlc_NtkCountObjBits( Wlc_Ntk_t * p, Vec_Int_t * vPisNew )
{
    Wlc_Obj_t * pObj; 
    int i, Count = 0;
    Wlc_NtkForEachObjVec( vPisNew, p, pObj, i )
        Count += Wlc_ObjRange(pObj);
    return Count;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

