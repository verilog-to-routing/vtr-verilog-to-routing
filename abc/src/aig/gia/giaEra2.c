/**CFile****************************************************************

  FileName    [gia.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: gia.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "giaAig.h"

ABC_NAMESPACE_IMPL_START

/*
    Limitations of this package:
    - no more than (1<<31)-1 state cubes and internal nodes
    - no more than MAX_VARS_NUM state variables
    - no more than MAX_CALL_NUM transitions from a state
    - cube list rebalancing happens when cube count reaches MAX_CUBE_NUM
*/

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define MAX_CALL_NUM    (1000000)  // the max number of recursive calls
#define MAX_ITEM_NUM      (1<<20)  // the number of items on a page
#define MAX_PAGE_NUM      (1<<11)  // the max number of memory pages
#define MAX_VARS_NUM      (1<<14)  // the max number of state vars allowed
#define MAX_CUBE_NUM          63   // the max number of cubes before rebalancing

// pointer to the tree node or state cube
typedef struct Gia_PtrAre_t_ Gia_PtrAre_t;
struct Gia_PtrAre_t_
{
    unsigned       nItem    : 20;  // item number (related to MAX_ITEM_NUM)
    unsigned       nPage    : 11;  // page number (related to MAX_PAGE_NUM)
    unsigned       fMark    :  1;  // user mark
};

typedef union Gia_PtrAreInt_t_ Gia_PtrAreInt_t;
union Gia_PtrAreInt_t_
{
    Gia_PtrAre_t   iGia;
    unsigned       iInt;
};

// tree nodes
typedef struct Gia_ObjAre_t_ Gia_ObjAre_t;
struct Gia_ObjAre_t_
{
    unsigned       iVar     : 14;  // variable     (related to MAX_VARS_NUM)
    unsigned       nStas0   :  6;  // cube counter (related to MAX_CUBE_NUM)
    unsigned       nStas1   :  6;  // cube counter (related to MAX_CUBE_NUM)
    unsigned       nStas2   :  6;  // cube counter (related to MAX_CUBE_NUM)
    Gia_PtrAre_t   F[3];           // branches
};

// state cube
typedef struct Gia_StaAre_t_ Gia_StaAre_t;
struct Gia_StaAre_t_
{
    Gia_PtrAre_t   iPrev;          // previous state
    Gia_PtrAre_t   iNext;          // next cube in the list
    unsigned       pData[0];       // state bits
};

// explicit state reachability manager
typedef struct Gia_ManAre_t_ Gia_ManAre_t;
struct Gia_ManAre_t_
{
    Gia_Man_t *    pAig;           // user's AIG manager
    Gia_Man_t *    pNew;           // temporary AIG manager
    unsigned **    ppObjs;         // storage for objects (MAX_PAGE_NUM pages)
    unsigned **    ppStas;         // storage for states  (MAX_PAGE_NUM pages)
//    unsigned *     pfUseless;      // to label useless cubes
//    int            nUselessAlloc;  // the number of useless alloced
    // internal flags
    int            fMiter;         // stops when a bug is discovered
    int            fStopped;       // set high when reachability is stopped
    int            fTree;          // working in the tree mode
    // internal parametesr
    int            nWords;         // the size of bit info in words
    int            nSize;          // the size of state structure in words
    int            nObjPages;      // the number of pages used for objects
    int            nStaPages;      // the number of pages used for states
    int            nObjs;          // the number of objects 
    int            nStas;          // the number of states  
    int            iStaCur;        // the next state to be explored
    Gia_PtrAre_t   Root;           // root of the tree
    Vec_Vec_t *    vCiTfos;        // storage for nodes in the CI TFOs
    Vec_Vec_t *    vCiLits;        // storage for literals of these nodes
    Vec_Int_t *    vCubesA;        // checked cubes
    Vec_Int_t *    vCubesB;        // unchecked cubes
    // deriving counter-example
    void *         pSat;           // SAT solver
    Vec_Int_t *    vSatNumCis;     // SAT variables for CIs
    Vec_Int_t *    vSatNumCos;     // SAT variables for COs 
    Vec_Int_t *    vCofVars;       // variables used to cofactor
    Vec_Int_t *    vAssumps;       // temporary storage for assumptions
    Gia_StaAre_t * pTarget;        // state that needs to be reached
    int            iOutFail;       // the number of the failed output
    // statistics
    int            nChecks;        // the number of timea cube was checked
    int            nEquals;        // total number of equal
    int            nCompares;      // the number of compares
    int            nRecCalls;      // the number of rec calls
    int            nDisjs;         // the number of disjoint cube pairs
    int            nDisjs2;        // the number of disjoint cube pairs
    int            nDisjs3;        // the number of disjoint cube pairs
    // time
    int            timeAig;        // AIG cofactoring time
    int            timeCube;       // cube checking time 
}; 

static inline Gia_PtrAre_t    Gia_Int2Ptr( unsigned n )                               { Gia_PtrAreInt_t g; g.iInt = n; return g.iGia;            }
static inline unsigned        Gia_Ptr2Int( Gia_PtrAre_t n )                           { Gia_PtrAreInt_t g; g.iGia = n; return g.iInt & 0x7fffffff; }

static inline int             Gia_ObjHasBranch0( Gia_ObjAre_t * q )                   { return !q->nStas0 && (q->F[0].nPage || q->F[0].nItem);   }
static inline int             Gia_ObjHasBranch1( Gia_ObjAre_t * q )                   { return !q->nStas1 && (q->F[1].nPage || q->F[1].nItem);   }
static inline int             Gia_ObjHasBranch2( Gia_ObjAre_t * q )                   { return !q->nStas2 && (q->F[2].nPage || q->F[2].nItem);   }

static inline Gia_ObjAre_t *  Gia_ManAreObj( Gia_ManAre_t * p, Gia_PtrAre_t n )       { return (Gia_ObjAre_t *)(p->ppObjs[n.nPage] + (n.nItem << 2));      }
static inline Gia_StaAre_t *  Gia_ManAreSta( Gia_ManAre_t * p, Gia_PtrAre_t n )       { return (Gia_StaAre_t *)(p->ppStas[n.nPage] +  n.nItem * p->nSize); }
static inline Gia_ObjAre_t *  Gia_ManAreObjInt( Gia_ManAre_t * p, int n )             { return Gia_ManAreObj( p, Gia_Int2Ptr(n) );               }
static inline Gia_StaAre_t *  Gia_ManAreStaInt( Gia_ManAre_t * p, int n )             { return Gia_ManAreSta( p, Gia_Int2Ptr(n) );               }
static inline Gia_ObjAre_t *  Gia_ManAreObjLast( Gia_ManAre_t * p )                   { return Gia_ManAreObjInt( p, p->nObjs-1 );                }
static inline Gia_StaAre_t *  Gia_ManAreStaLast( Gia_ManAre_t * p )                   { return Gia_ManAreStaInt( p, p->nStas-1 );                }

static inline Gia_ObjAre_t *  Gia_ObjNextObj0( Gia_ManAre_t * p, Gia_ObjAre_t * q )   { return Gia_ManAreObj( p, q->F[0] );                      }
static inline Gia_ObjAre_t *  Gia_ObjNextObj1( Gia_ManAre_t * p, Gia_ObjAre_t * q )   { return Gia_ManAreObj( p, q->F[1] );                      }
static inline Gia_ObjAre_t *  Gia_ObjNextObj2( Gia_ManAre_t * p, Gia_ObjAre_t * q )   { return Gia_ManAreObj( p, q->F[2] );                      }

static inline int             Gia_StaHasValue0( Gia_StaAre_t * p, int iReg )          { return Abc_InfoHasBit( p->pData,  iReg << 1 );           }
static inline int             Gia_StaHasValue1( Gia_StaAre_t * p, int iReg )          { return Abc_InfoHasBit( p->pData, (iReg << 1) + 1 );      }

static inline void            Gia_StaSetValue0( Gia_StaAre_t * p, int iReg )          { Abc_InfoSetBit( p->pData,  iReg << 1 );                  }
static inline void            Gia_StaSetValue1( Gia_StaAre_t * p, int iReg )          { Abc_InfoSetBit( p->pData, (iReg << 1) + 1 );             }

static inline Gia_StaAre_t *  Gia_StaPrev( Gia_ManAre_t * p, Gia_StaAre_t * pS )      { return Gia_ManAreSta(p, pS->iPrev);                      }
static inline Gia_StaAre_t *  Gia_StaNext( Gia_ManAre_t * p, Gia_StaAre_t * pS )      { return Gia_ManAreSta(p, pS->iNext);                      }
static inline int             Gia_StaIsGood( Gia_ManAre_t * p, Gia_StaAre_t * pS )    { return ((unsigned *)pS) != p->ppStas[0];                         }

static inline void            Gia_StaSetUnused( Gia_StaAre_t * pS )                   { pS->iPrev.fMark = 1;                                     }
static inline int             Gia_StaIsUnused( Gia_StaAre_t * pS )                    { return  pS->iPrev.fMark;                                 }
static inline int             Gia_StaIsUsed( Gia_StaAre_t * pS )                      { return !pS->iPrev.fMark;                                 }

#define Gia_ManAreForEachCubeList( p, pList, pCube )                         \
    for ( pCube = pList; Gia_StaIsGood(p, pCube); pCube = Gia_StaNext(p, pCube) )
#define Gia_ManAreForEachCubeList2( p, iList, pCube, iCube )                 \
    for ( iCube = Gia_Ptr2Int(iList), pCube = Gia_ManAreSta(p, iList);       \
          Gia_StaIsGood(p, pCube);                                           \
          iCube = Gia_Ptr2Int(pCube->iNext), pCube = Gia_StaNext(p, pCube) )
#define Gia_ManAreForEachCubeStore( p, pCube, i )                            \
    for ( i = 1; i < p->nStas && (pCube = Gia_ManAreStaInt(p, i)); i++ )
#define Gia_ManAreForEachCubeVec( vVec, p, pCube, i )                        \
    for ( i = 0; i < Vec_IntSize(vVec) && (pCube = Gia_ManAreStaInt(p, Vec_IntEntry(vVec,i))); i++ )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Count state minterms contained in a cube.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCountMintermsInCube( Gia_StaAre_t * pCube, int nVars, unsigned * pStore )
{
    unsigned Mint, Mask = 0;
    int i, m, nMints, nDashes = 0, Dashes[32];
    // count the number of dashes
    for ( i = 0; i < nVars; i++ )
    {
        if ( Gia_StaHasValue0( pCube, i ) )
            continue;
        if ( Gia_StaHasValue1( pCube, i ) )
            Mask |= (1 << i);
        else
            Dashes[nDashes++] = i;
    }
    // fill in the miterms
    nMints = (1 << nDashes);
    for ( m = 0; m < nMints; m++ )
    {
        Mint = Mask;
        for ( i = 0; i < nVars; i++ )
            if ( m & (1 << i) )
                Mint |= (1 << Dashes[i]);
        Abc_InfoSetBit( pStore, Mint );
    }
}

/**Function*************************************************************

  Synopsis    [Count state minterms contains in the used cubes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManCountMinterms( Gia_ManAre_t * p )
{
    Gia_StaAre_t * pCube;
    unsigned * pMemory;
    int i, nMemSize, Counter = 0;
    if ( Gia_ManRegNum(p->pAig) > 30 )
        return -1;
    nMemSize = Abc_BitWordNum( 1 << Gia_ManRegNum(p->pAig) );
    pMemory  = ABC_CALLOC( unsigned, nMemSize );
    Gia_ManAreForEachCubeStore( p, pCube, i )
        if ( Gia_StaIsUsed(pCube) )
            Gia_ManCountMintermsInCube( pCube, Gia_ManRegNum(p->pAig), pMemory );
    for ( i = 0; i < nMemSize; i++ )
        Counter += Gia_WordCountOnes( pMemory[i] );
    ABC_FREE( pMemory );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Derives the TFO of one CI.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManDeriveCiTfo_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vRes )
{
    if ( Gia_ObjIsCi(pObj) )
        return pObj->fMark0;
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return pObj->fMark0;
    Gia_ObjSetTravIdCurrent(p, pObj);
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManDeriveCiTfo_rec( p, Gia_ObjFanin0(pObj), vRes );
    Gia_ManDeriveCiTfo_rec( p, Gia_ObjFanin1(pObj), vRes );
    pObj->fMark0 = Gia_ObjFanin0(pObj)->fMark0 | Gia_ObjFanin1(pObj)->fMark0;
    if ( pObj->fMark0 )
        Vec_IntPush( vRes, Gia_ObjId(p, pObj) );
    return pObj->fMark0;
}


/**Function*************************************************************

  Synopsis    [Derives the TFO of one CI.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManDeriveCiTfoOne( Gia_Man_t * p, Gia_Obj_t * pPivot )
{
    Vec_Int_t * vRes;
    Gia_Obj_t * pObj;
    int i;
    assert( pPivot->fMark0 == 0 );
    pPivot->fMark0 = 1;
    vRes = Vec_IntAlloc( 100 );
    Vec_IntPush( vRes, Gia_ObjId(p, pPivot) );
    Gia_ManIncrementTravId( p );
    Gia_ObjSetTravIdCurrent( p, Gia_ManConst0(p) );
    Gia_ManForEachCo( p, pObj, i )
    {
        Gia_ManDeriveCiTfo_rec( p, Gia_ObjFanin0(pObj), vRes );
        if ( Gia_ObjFanin0(pObj)->fMark0 )
            Vec_IntPush( vRes, Gia_ObjId(p, pObj) );
    }
    pPivot->fMark0 = 0;
    return vRes;
}

/**Function*************************************************************

  Synopsis    [Derives the TFO of each CI.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Vec_t * Gia_ManDeriveCiTfo( Gia_Man_t * p )
{
    Vec_Ptr_t * vRes;
    Gia_Obj_t * pPivot;
    int i;
    Gia_ManCleanMark0( p );
    Gia_ManIncrementTravId( p );
    vRes = Vec_PtrAlloc( Gia_ManCiNum(p) );
    Gia_ManForEachCi( p, pPivot, i )
        Vec_PtrPush( vRes, Gia_ManDeriveCiTfoOne(p, pPivot) );
    Gia_ManCleanMark0( p );
    return (Vec_Vec_t *)vRes;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if states are equal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_StaAreEqual( Gia_StaAre_t * p1, Gia_StaAre_t * p2, int nWords )
{
    int w;
    for ( w = 0; w < nWords; w++ )
        if ( p1->pData[w] != p2->pData[w] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if states are disjoint.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_StaAreDisjoint( Gia_StaAre_t * p1, Gia_StaAre_t * p2, int nWords )
{
    int w;
    for ( w = 0; w < nWords; w++ )
        if ( ((p1->pData[w] ^ p2->pData[w]) >> 1) & (p1->pData[w] ^ p2->pData[w]) & 0x55555555 )
            return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if cube p1 contains cube p2.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_StaAreContain( Gia_StaAre_t * p1, Gia_StaAre_t * p2, int nWords )
{
    int w;
    for ( w = 0; w < nWords; w++ )
        if ( (p1->pData[w] | p2->pData[w]) != p2->pData[w] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns the number of dashes in p1 that are non-dashes in p2.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_StaAreDashNum( Gia_StaAre_t * p1, Gia_StaAre_t * p2, int nWords )
{
    int w, Counter = 0;
    for ( w = 0; w < nWords; w++ )
        Counter += Gia_WordCountOnes( (~(p1->pData[w] ^ (p1->pData[w] >> 1))) & (p2->pData[w] ^ (p2->pData[w] >> 1)) & 0x55555555 );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Returns the number of a variable for sharping the cube.]

  Description [Counts the number of variables that have dash in p1 and 
  non-dash in p2. If there is exactly one such variable, returns its index.
  Otherwise returns -1.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_StaAreSharpVar( Gia_StaAre_t * p1, Gia_StaAre_t * p2, int nWords )
{
    unsigned Word;
    int w, iVar = -1;
    for ( w = 0; w < nWords; w++ )
    {
        Word = (~(p1->pData[w] ^ (p1->pData[w] >> 1))) & (p2->pData[w] ^ (p2->pData[w] >> 1)) & 0x55555555;
        if ( Word == 0 )
            continue;
        if ( !Gia_WordHasOneBit(Word) )
            return -1;
        // has exactly one bit
        if ( iVar >= 0 )
            return -1;
        // the first variable of this type
        iVar = 16 * w + Gia_WordFindFirstBit( Word ) / 2;
    }
    return iVar;
}

/**Function*************************************************************

  Synopsis    [Returns the number of a variable for merging the cubes.]

  Description [If there is exactly one such variable, returns its index.
  Otherwise returns -1.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_StaAreDisjointVar( Gia_StaAre_t * p1, Gia_StaAre_t * p2, int nWords )
{
    unsigned Word;
    int w, iVar = -1;
    for ( w = 0; w < nWords; w++ )
    {
        Word = (p1->pData[w] ^ p2->pData[w]) & ((p1->pData[w] ^ p2->pData[w]) >> 1) & 0x55555555;
        if ( Word == 0 )
            continue;
        if ( !Gia_WordHasOneBit(Word) )
            return -1;
        // has exactly one bit
        if ( iVar >= 0 )
            return -1;
        // the first variable of this type
        iVar = 16 * w + Gia_WordFindFirstBit( Word ) / 2;
    }
    return iVar;
}

/**Function*************************************************************

  Synopsis    [Creates reachability manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_ManAre_t * Gia_ManAreCreate( Gia_Man_t * pAig )
{
    Gia_ManAre_t * p;
    assert( sizeof(Gia_ObjAre_t) == 16 );
    p = ABC_CALLOC( Gia_ManAre_t, 1 );
    p->pAig       = pAig;
    p->nWords     = Abc_BitWordNum( 2 * Gia_ManRegNum(pAig) );
    p->nSize      = sizeof(Gia_StaAre_t)/4 + p->nWords;
    p->ppObjs     = ABC_CALLOC( unsigned *, MAX_PAGE_NUM );
    p->ppStas     = ABC_CALLOC( unsigned *, MAX_PAGE_NUM );
    p->vCiTfos    = Gia_ManDeriveCiTfo( pAig );
    p->vCiLits    = Vec_VecDupInt( p->vCiTfos );
    p->vCubesA    = Vec_IntAlloc( 100 );
    p->vCubesB    = Vec_IntAlloc( 100 );
    p->iOutFail   = -1;
    return p;
}

/**Function*************************************************************

  Synopsis    [Deletes reachability manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManAreFree( Gia_ManAre_t * p )
{
    int i;
    Gia_ManStop( p->pAig );
    if ( p->pNew )
        Gia_ManStop( p->pNew );
    Vec_IntFree( p->vCubesA );
    Vec_IntFree( p->vCubesB );
    Vec_VecFree( p->vCiTfos );
    Vec_VecFree( p->vCiLits );
    for ( i = 0; i < p->nObjPages; i++ )
        ABC_FREE( p->ppObjs[i] );
    ABC_FREE( p->ppObjs );
    for ( i = 0; i < p->nStaPages; i++ )
        ABC_FREE( p->ppStas[i] );
    ABC_FREE( p->ppStas );
//    ABC_FREE( p->pfUseless );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Returns new object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Gia_ObjAre_t * Gia_ManAreCreateObj( Gia_ManAre_t * p )
{
    if ( p->nObjs == p->nObjPages * MAX_ITEM_NUM )
    {
        if ( p->nObjPages == MAX_PAGE_NUM )
        {
            printf( "ERA manager has run out of memory after allocating 2B internal nodes.\n" );
            return NULL;
        }
        p->ppObjs[p->nObjPages++] = ABC_CALLOC( unsigned, MAX_ITEM_NUM * 4 );
        if ( p->nObjs == 0 )
            p->nObjs = 1;
    }
    return Gia_ManAreObjInt( p, p->nObjs++ );
}

/**Function*************************************************************

  Synopsis    [Returns new state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Gia_StaAre_t * Gia_ManAreCreateSta( Gia_ManAre_t * p )
{
    if ( p->nStas == p->nStaPages * MAX_ITEM_NUM )
    {
        if ( p->nStaPages == MAX_PAGE_NUM )
        {
            printf( "ERA manager has run out of memory after allocating 2B state cubes.\n" );
            return NULL;
        }
        if ( p->ppStas[p->nStaPages] == NULL )
            p->ppStas[p->nStaPages] = ABC_CALLOC( unsigned, MAX_ITEM_NUM * p->nSize );
        p->nStaPages++;
        if ( p->nStas == 0 )
        {
            p->nStas = 1;
//            p->nUselessAlloc = (1 << 18);
//            p->pfUseless = ABC_CALLOC( unsigned, p->nUselessAlloc );
        }
//        if ( p->nStas == p->nUselessAlloc * 32 )
//        {
//            p->nUselessAlloc *= 2;
//            p->pfUseless = ABC_REALLOC( unsigned, p->pfUseless, p->nUselessAlloc );
//            memset( p->pfUseless + p->nUselessAlloc/2, 0, sizeof(unsigned) * p->nUselessAlloc/2 );
//        }
    }
    return Gia_ManAreStaInt( p, p->nStas++ );
}

/**Function*************************************************************

  Synopsis    [Recycles new state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManAreRycycleSta( Gia_ManAre_t * p, Gia_StaAre_t * pSta )
{
    memset( pSta, 0, p->nSize << 2 );
    if ( pSta == Gia_ManAreStaLast(p) )
    {
        p->nStas--;
        if ( p->nStas == (p->nStaPages-1) * MAX_ITEM_NUM )
            p->nStaPages--;
    }
    else
    {
//        Gia_StaSetUnused( pSta );
    }
}

/**Function*************************************************************

  Synopsis    [Creates new state state from the latch values.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Gia_StaAre_t * Gia_ManAreCreateStaNew( Gia_ManAre_t * p )
{
    Gia_StaAre_t * pSta;
    Gia_Obj_t * pObj;
    int i;
    pSta = Gia_ManAreCreateSta( p );
    Gia_ManForEachRi( p->pAig, pObj, i )
    {
        if ( pObj->Value == 0 )
            Gia_StaSetValue0( pSta, i );
        else if ( pObj->Value == 1 )
            Gia_StaSetValue1( pSta, i );
    }
    return pSta;
}

/**Function*************************************************************

  Synopsis    [Creates new state state with latch init values.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Gia_StaAre_t * Gia_ManAreCreateStaInit( Gia_ManAre_t * p )
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachRi( p->pAig, pObj, i )
        pObj->Value = 0;
    return Gia_ManAreCreateStaNew( p );
}


/**Function*************************************************************

  Synopsis    [Prints the state cube.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManArePrintCube( Gia_ManAre_t * p, Gia_StaAre_t * pSta )
{
    Gia_Obj_t * pObj;
    int i, Count0 = 0, Count1 = 0, Count2 = 0;
    printf( "%4d %4d :  ", p->iStaCur, p->nStas-1 );
    printf( "Prev %4d   ", Gia_Ptr2Int(pSta->iPrev) );
    printf( "%p   ", pSta );
    Gia_ManForEachRi( p->pAig, pObj, i )
    {
        if ( Gia_StaHasValue0(pSta, i) )
            printf( "0" ), Count0++;
        else if ( Gia_StaHasValue1(pSta, i) )
            printf( "1" ), Count1++;
        else
            printf( "-" ), Count2++;
    }
    printf( "  0 =%3d", Count0 );
    printf( "  1 =%3d", Count1 );
    printf( "  - =%3d", Count2 );
    printf( "\n" );
}
 
/**Function*************************************************************

  Synopsis    [Counts the depth of state transitions leading ot this state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManAreDepth( Gia_ManAre_t * p, int iState )
{
    Gia_StaAre_t * pSta;
    int Counter = 0;
    for ( pSta = Gia_ManAreStaInt(p, iState); Gia_StaIsGood(p, pSta); pSta = Gia_StaPrev(p, pSta) )
        Counter++;
    return Counter;
} 

/**Function*************************************************************

  Synopsis    [Counts the number of cubes in the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_ManAreListCountListUsed( Gia_ManAre_t * p, Gia_PtrAre_t Root )
{
    Gia_StaAre_t * pCube;
    int Counter = 0;
    Gia_ManAreForEachCubeList( p, Gia_ManAreSta(p, Root), pCube )
        Counter += Gia_StaIsUsed(pCube);
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts the number of used cubes in the tree.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManAreListCountUsed_rec( Gia_ManAre_t * p, Gia_PtrAre_t Root, int fTree )
{
    Gia_ObjAre_t * pObj;
    if ( !fTree )
        return Gia_ManAreListCountListUsed( p, Root );
    pObj = Gia_ManAreObj(p, Root);
    return Gia_ManAreListCountUsed_rec( p, pObj->F[0], Gia_ObjHasBranch0(pObj) ) +
           Gia_ManAreListCountUsed_rec( p, pObj->F[1], Gia_ObjHasBranch1(pObj) ) +
           Gia_ManAreListCountUsed_rec( p, pObj->F[2], Gia_ObjHasBranch2(pObj) );
}

/**Function*************************************************************

  Synopsis    [Counts the number of used cubes in the tree.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_ManAreListCountUsed( Gia_ManAre_t * p )
{
    return Gia_ManAreListCountUsed_rec( p, p->Root, p->fTree );
}


/**Function*************************************************************

  Synopsis    [Prints used cubes in the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_ManArePrintListUsed( Gia_ManAre_t * p, Gia_PtrAre_t Root )
{
    Gia_StaAre_t * pCube;
    Gia_ManAreForEachCubeList( p, Gia_ManAreSta(p, Root), pCube )
        if ( Gia_StaIsUsed(pCube) )
            Gia_ManArePrintCube( p, pCube );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Prints used cubes in the tree.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManArePrintUsed_rec( Gia_ManAre_t * p, Gia_PtrAre_t Root, int fTree )
{
    Gia_ObjAre_t * pObj;
    if ( !fTree )
        return Gia_ManArePrintListUsed( p, Root );
    pObj = Gia_ManAreObj(p, Root);
    return Gia_ManArePrintUsed_rec( p, pObj->F[0], Gia_ObjHasBranch0(pObj) ) +
           Gia_ManArePrintUsed_rec( p, pObj->F[1], Gia_ObjHasBranch1(pObj) ) +
           Gia_ManArePrintUsed_rec( p, pObj->F[2], Gia_ObjHasBranch2(pObj) );
}

/**Function*************************************************************

  Synopsis    [Prints used cubes in the tree.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_ManArePrintUsed( Gia_ManAre_t * p )
{
    return Gia_ManArePrintUsed_rec( p, p->Root, p->fTree );
}


/**Function*************************************************************

  Synopsis    [Best var has max weight.]

  Description [Weight is defined as the number of 0/1-lits minus the 
  absolute value of the diff between the number of 0-lits and 1-lits.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManAreFindBestVar( Gia_ManAre_t * p, Gia_PtrAre_t List )
{
    Gia_StaAre_t * pCube;
    int Count0, Count1, Count2;
    int iVarThis, iVarBest = -1, WeightThis, WeightBest = -1;
    for ( iVarThis = 0; iVarThis < Gia_ManRegNum(p->pAig); iVarThis++ )
    {
        Count0 = Count1 = Count2 = 0;
        Gia_ManAreForEachCubeList( p, Gia_ManAreSta(p, List), pCube )
        {
            if ( Gia_StaIsUnused(pCube) )
                continue;
            if ( Gia_StaHasValue0(pCube, iVarThis) )
                Count0++;
            else if ( Gia_StaHasValue1(pCube, iVarThis) )
                Count1++;
            else
                Count2++;
        }
//        printf( "%4d : %5d  %5d  %5d   Weight = %5d\n", iVarThis, Count0, Count1, Count2, 
//            Count0 + Count1 - (Count0 > Count1 ? Count0 - Count1 : Count1 - Count0) );
        if ( (!Count0 && !Count1) || (!Count0 && !Count2) || (!Count1 && !Count2) )
            continue;
        WeightThis = Count0 + Count1 - (Count0 > Count1 ? Count0 - Count1 : Count1 - Count0);
        if ( WeightBest < WeightThis ) 
        {
            WeightBest = WeightThis;
            iVarBest = iVarThis;
        }
    }
    if ( iVarBest == -1 )
    {
        Gia_ManArePrintListUsed( p, List );
        printf( "Error: Best variable not found!!!\n" );
    }
    assert( iVarBest != -1 );
    return iVarBest;
}
 
/**Function*************************************************************

  Synopsis    [Rebalances the tree when cubes exceed the limit.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManAreRebalance( Gia_ManAre_t * p, Gia_PtrAre_t * pRoot )
{
    Gia_ObjAre_t * pNode;
    Gia_StaAre_t * pCube;
    Gia_PtrAre_t iCube, iNext;
    assert( pRoot->nItem || pRoot->nPage );
    pNode = Gia_ManAreCreateObj( p );
    pNode->iVar = Gia_ManAreFindBestVar( p, *pRoot );
    for ( iCube = *pRoot, pCube = Gia_ManAreSta(p, iCube), iNext = pCube->iNext; 
          Gia_StaIsGood(p, pCube); 
          iCube = iNext,  pCube = Gia_ManAreSta(p, iCube), iNext = pCube->iNext )
    {
        if ( Gia_StaIsUnused(pCube) )
            continue;
        if ( Gia_StaHasValue0(pCube, pNode->iVar) )
            pCube->iNext = pNode->F[0], pNode->F[0] = iCube, pNode->nStas0++;
        else if ( Gia_StaHasValue1(pCube, pNode->iVar) )
            pCube->iNext = pNode->F[1], pNode->F[1] = iCube, pNode->nStas1++;
        else
            pCube->iNext = pNode->F[2], pNode->F[2] = iCube, pNode->nStas2++;
    }
    *pRoot = Gia_Int2Ptr(p->nObjs - 1);
    assert( pNode == Gia_ManAreObj(p, *pRoot) );
    p->fTree = 1;
}
 
/**Function*************************************************************

  Synopsis    [Compresses the list by removing unused cubes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManAreCompress( Gia_ManAre_t * p, Gia_PtrAre_t * pRoot )
{
    Gia_StaAre_t * pCube;
    Gia_PtrAre_t iList = *pRoot;
    Gia_PtrAre_t iCube, iNext;
    assert( pRoot->nItem || pRoot->nPage );
    pRoot->nItem = 0; 
    pRoot->nPage = 0;
    for ( iCube = iList, pCube = Gia_ManAreSta(p, iCube), iNext = pCube->iNext; 
          Gia_StaIsGood(p, pCube); 
          iCube = iNext, pCube = Gia_ManAreSta(p, iCube), iNext = pCube->iNext )
    {
        if ( Gia_StaIsUnused(pCube) )
            continue;
        pCube->iNext = *pRoot;
        *pRoot = iCube;
    }
}


/**Function*************************************************************

  Synopsis    [Checks if the state exists in the list.]

  Description [The state may be sharped.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_ManAreCubeCheckList( Gia_ManAre_t * p, Gia_PtrAre_t * pRoot, Gia_StaAre_t * pSta )
{
    int fVerbose = 0;
    Gia_StaAre_t * pCube;
    int iVar;
if ( fVerbose )
{
printf( "Trying cube: " );
Gia_ManArePrintCube( p, pSta );
}
    Gia_ManAreForEachCubeList( p, Gia_ManAreSta(p, *pRoot), pCube )
    {
        p->nChecks++;
        if ( Gia_StaIsUnused( pCube ) )
            continue;
        if ( Gia_StaAreDisjoint( pSta, pCube, p->nWords ) )
            continue;
        if ( Gia_StaAreContain( pCube, pSta, p->nWords ) )
        {
if ( fVerbose )
{
printf( "Contained in " );
Gia_ManArePrintCube( p, pCube );
}
            Gia_ManAreRycycleSta( p, pSta );
            return 0;
        }
        if ( Gia_StaAreContain( pSta, pCube, p->nWords ) )
        {
if ( fVerbose )
{
printf( "Contains     " );
Gia_ManArePrintCube( p, pCube );
}
            Gia_StaSetUnused( pCube );
            continue;
        }
        iVar = Gia_StaAreSharpVar( pSta, pCube, p->nWords );
        if ( iVar == -1 )
            continue;
if ( fVerbose )
{
printf( "Sharped by   " );
Gia_ManArePrintCube( p, pCube );
Gia_ManArePrintCube( p, pSta );
}
//        printf( "%d  %d\n", Gia_StaAreDashNum( pSta, pCube, p->nWords ), Gia_StaAreSharpVar( pSta, pCube, p->nWords ) );
        assert( !Gia_StaHasValue0(pSta, iVar) && !Gia_StaHasValue1(pSta, iVar) );
        assert(  Gia_StaHasValue0(pCube, iVar) ^  Gia_StaHasValue1(pCube, iVar) );
        if ( Gia_StaHasValue0(pCube, iVar) )
            Gia_StaSetValue1( pSta, iVar );
        else
            Gia_StaSetValue0( pSta, iVar );
//        return Gia_ManAreCubeCheckList( p, pRoot, pSta );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Adds new state to the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManAreCubeAddToList( Gia_ManAre_t * p, Gia_PtrAre_t * pRoot, Gia_StaAre_t * pSta )
{
    int fVerbose = 0;
    pSta->iNext = *pRoot;
    *pRoot = Gia_Int2Ptr( p->nStas - 1 );
    assert( pSta == Gia_ManAreSta(p, *pRoot) );
if ( fVerbose )
{
printf( "Adding cube: " );
Gia_ManArePrintCube( p, pSta );
//printf( "\n" );
}
}
 
/**Function*************************************************************

  Synopsis    [Checks if the cube like this exists in the tree.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManAreCubeCheckTree_rec( Gia_ManAre_t * p, Gia_ObjAre_t * pObj, Gia_StaAre_t * pSta )
{
    int RetValue;
    if ( Gia_StaHasValue0(pSta, pObj->iVar) )
    {
        if ( Gia_ObjHasBranch0(pObj) )
            RetValue = Gia_ManAreCubeCheckTree_rec( p, Gia_ObjNextObj0(p, pObj), pSta );
        else
            RetValue = Gia_ManAreCubeCheckList( p, pObj->F, pSta );
        if ( RetValue == 0 )
            return 0;
    }
    else if ( Gia_StaHasValue1(pSta, pObj->iVar) )
    {
        if ( Gia_ObjHasBranch1(pObj) )
            RetValue = Gia_ManAreCubeCheckTree_rec( p, Gia_ObjNextObj1(p, pObj), pSta );
        else
            RetValue = Gia_ManAreCubeCheckList( p, pObj->F + 1, pSta );
        if ( RetValue == 0 )
            return 0;
    }
    if ( Gia_ObjHasBranch2(pObj) )
        return Gia_ManAreCubeCheckTree_rec( p, Gia_ObjNextObj2(p, pObj), pSta );
    return Gia_ManAreCubeCheckList( p, pObj->F + 2, pSta );
}

/**Function*************************************************************

  Synopsis    [Adds new cube to the tree.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManAreCubeAddToTree_rec( Gia_ManAre_t * p, Gia_ObjAre_t * pObj, Gia_StaAre_t * pSta )
{
    if ( Gia_StaHasValue0(pSta, pObj->iVar) )
    {
        if ( Gia_ObjHasBranch0(pObj) )
            Gia_ManAreCubeAddToTree_rec( p, Gia_ObjNextObj0(p, pObj), pSta );
        else
        {
            Gia_ManAreCubeAddToList( p, pObj->F, pSta );
            if ( ++pObj->nStas0 == MAX_CUBE_NUM )
            {
                pObj->nStas0 = Gia_ManAreListCountListUsed( p, pObj->F[0] );
                if ( pObj->nStas0 < MAX_CUBE_NUM/2 )
                    Gia_ManAreCompress( p, pObj->F );
                else
                {
                    Gia_ManAreRebalance( p, pObj->F );
                    pObj->nStas0 = 0;
                }
            }
        }
    }
    else if ( Gia_StaHasValue1(pSta, pObj->iVar) )
    {
        if ( Gia_ObjHasBranch1(pObj) )
            Gia_ManAreCubeAddToTree_rec( p, Gia_ObjNextObj1(p, pObj), pSta );
        else
        {
            Gia_ManAreCubeAddToList( p, pObj->F+1, pSta );
            if ( ++pObj->nStas1 == MAX_CUBE_NUM )
            {
                pObj->nStas1 = Gia_ManAreListCountListUsed( p, pObj->F[1] );
                if ( pObj->nStas1 < MAX_CUBE_NUM/2 )
                    Gia_ManAreCompress( p, pObj->F+1 );
                else
                {
                    Gia_ManAreRebalance( p, pObj->F+1 );
                    pObj->nStas1 = 0;
                }
            }
        }
    }
    else
    {
        if ( Gia_ObjHasBranch2(pObj) )
            Gia_ManAreCubeAddToTree_rec( p, Gia_ObjNextObj2(p, pObj), pSta );
        else
        {
            Gia_ManAreCubeAddToList( p, pObj->F+2, pSta );
            if ( ++pObj->nStas2 == MAX_CUBE_NUM )
            {
                pObj->nStas2 = Gia_ManAreListCountListUsed( p, pObj->F[2] );
                if ( pObj->nStas2 < MAX_CUBE_NUM/2 )
                    Gia_ManAreCompress( p, pObj->F+2 );
                else
                {
                    Gia_ManAreRebalance( p, pObj->F+2 );
                    pObj->nStas2 = 0;
                }
            }
        }
    }
}



/**Function*************************************************************

  Synopsis    [Collects overlapping cubes in the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_ManAreCubeCollectList( Gia_ManAre_t * p, Gia_PtrAre_t * pRoot, Gia_StaAre_t * pSta )
{
    Gia_StaAre_t * pCube;
    int iCube;
    Gia_ManAreForEachCubeList2( p, *pRoot, pCube, iCube )
    {
        if ( Gia_StaIsUnused( pCube ) )
            continue;
        if ( Gia_StaAreDisjoint( pSta, pCube, p->nWords ) )
        {
/*
            int iVar;
            p->nDisjs++;
            iVar = Gia_StaAreDisjointVar( pSta, pCube, p->nWords );
            if ( iVar >= 0 )
            {
                p->nDisjs2++;
                if ( iCube > p->iStaCur )
                    p->nDisjs3++;
            }
*/
            continue;
        }
//        p->nCompares++;
//        p->nEquals += Gia_StaAreEqual( pSta, pCube, p->nWords );
        if ( iCube <= p->iStaCur )
            Vec_IntPush( p->vCubesA, iCube );
        else
            Vec_IntPush( p->vCubesB, iCube );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Collects overlapping cubes in the tree.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManAreCubeCollectTree_rec( Gia_ManAre_t * p, Gia_ObjAre_t * pObj, Gia_StaAre_t * pSta )
{
    int RetValue;
    if ( Gia_StaHasValue0(pSta, pObj->iVar) )
    {
        if ( Gia_ObjHasBranch0(pObj) )
            RetValue = Gia_ManAreCubeCollectTree_rec( p, Gia_ObjNextObj0(p, pObj), pSta );
        else
            RetValue = Gia_ManAreCubeCollectList( p, pObj->F, pSta );
        if ( RetValue == 0 )
            return 0;
    }
    else if ( Gia_StaHasValue1(pSta, pObj->iVar) )
    {
        if ( Gia_ObjHasBranch1(pObj) )
            RetValue = Gia_ManAreCubeCollectTree_rec( p, Gia_ObjNextObj1(p, pObj), pSta );
        else
            RetValue = Gia_ManAreCubeCollectList( p, pObj->F + 1, pSta );
        if ( RetValue == 0 )
            return 0;
    }
    if ( Gia_ObjHasBranch2(pObj) )
        return Gia_ManAreCubeCollectTree_rec( p, Gia_ObjNextObj2(p, pObj), pSta );
    return Gia_ManAreCubeCollectList( p, pObj->F + 2, pSta );
}
 
/**Function*************************************************************

  Synopsis    [Checks if the cube like this exists in the tree.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManAreCubeCheckTree( Gia_ManAre_t * p, Gia_StaAre_t * pSta )
{
    Gia_StaAre_t * pCube;
    int i, iVar;
    assert( p->fTree );
    Vec_IntClear( p->vCubesA );
    Vec_IntClear( p->vCubesB );
    Gia_ManAreCubeCollectTree_rec( p, Gia_ManAreObj(p, p->Root), pSta );
//    if ( p->nStas > 3000 )
//        printf( "%d %d  \n", Vec_IntSize(p->vCubesA), Vec_IntSize(p->vCubesB) );
//    Vec_IntSort( p->vCubesA, 0 );
//    Vec_IntSort( p->vCubesB, 0 );
    Gia_ManAreForEachCubeVec( p->vCubesA, p, pCube, i )
    {
        if ( Gia_StaIsUnused( pCube ) )
            continue;
        if ( Gia_StaAreDisjoint( pSta, pCube, p->nWords ) )
            continue;
        if ( Gia_StaAreContain( pCube, pSta, p->nWords ) )
        {
            Gia_ManAreRycycleSta( p, pSta );
            return 0;
        }
        if ( Gia_StaAreContain( pSta, pCube, p->nWords ) )
        {
            Gia_StaSetUnused( pCube );
            continue;
        }
        iVar = Gia_StaAreSharpVar( pSta, pCube, p->nWords );
        if ( iVar == -1 )
            continue;
        assert( !Gia_StaHasValue0(pSta, iVar) && !Gia_StaHasValue1(pSta, iVar) );
        assert(  Gia_StaHasValue0(pCube, iVar) ^  Gia_StaHasValue1(pCube, iVar) );
        if ( Gia_StaHasValue0(pCube, iVar) )
            Gia_StaSetValue1( pSta, iVar );
        else
            Gia_StaSetValue0( pSta, iVar );
        return Gia_ManAreCubeCheckTree( p, pSta );
    }
    Gia_ManAreForEachCubeVec( p->vCubesB, p, pCube, i )
    {
        if ( Gia_StaIsUnused( pCube ) )
            continue;
        if ( Gia_StaAreDisjoint( pSta, pCube, p->nWords ) )
            continue;
        if ( Gia_StaAreContain( pCube, pSta, p->nWords ) )
        {
            Gia_ManAreRycycleSta( p, pSta );
            return 0;
        }
        if ( Gia_StaAreContain( pSta, pCube, p->nWords ) )
        {
            Gia_StaSetUnused( pCube );
            continue;
        }
        iVar = Gia_StaAreSharpVar( pSta, pCube, p->nWords );
        if ( iVar == -1 )
            continue;
        assert( !Gia_StaHasValue0(pSta, iVar) && !Gia_StaHasValue1(pSta, iVar) );
        assert(  Gia_StaHasValue0(pCube, iVar) ^  Gia_StaHasValue1(pCube, iVar) );
        if ( Gia_StaHasValue0(pCube, iVar) )
            Gia_StaSetValue1( pSta, iVar );
        else
            Gia_StaSetValue0( pSta, iVar );
        return Gia_ManAreCubeCheckTree( p, pSta );
    }
/*
    if ( p->nStas > 3000 )
    {
printf( "Trying cube:       " );
Gia_ManArePrintCube( p, pSta );
    Gia_ManAreForEachCubeVec( p->vCubesA, p, pCube, i )
    {
printf( "aaaaaaaaaaaa %5d ", Vec_IntEntry(p->vCubesA,i) );
Gia_ManArePrintCube( p, pCube );
    }
    Gia_ManAreForEachCubeVec( p->vCubesB, p, pCube, i )
    {
printf( "bbbbbbbbbbbb %5d ", Vec_IntEntry(p->vCubesB,i) );
Gia_ManArePrintCube( p, pCube );
    }
    }
*/
    return 1;
}

/**Function*************************************************************

  Synopsis    [Processes the new cube.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_ManAreCubeProcess( Gia_ManAre_t * p, Gia_StaAre_t * pSta )
{
    int RetValue;
    p->nChecks = 0;
    if ( !p->fTree && p->nStas == MAX_CUBE_NUM )
        Gia_ManAreRebalance( p, &p->Root );
    if ( p->fTree )
    {
//        RetValue = Gia_ManAreCubeCheckTree_rec( p, Gia_ManAreObj(p, p->Root), pSta );
        RetValue = Gia_ManAreCubeCheckTree( p, pSta );
        if ( RetValue )
            Gia_ManAreCubeAddToTree_rec( p, Gia_ManAreObj(p, p->Root), pSta );
    }
    else
    {
        RetValue = Gia_ManAreCubeCheckList( p, &p->Root, pSta );
        if ( RetValue )
            Gia_ManAreCubeAddToList( p, &p->Root, pSta );
    }
//    printf( "%d ", p->nChecks );
    return RetValue;
}


/**Function*************************************************************

  Synopsis    [Returns the most used CI, or NULL if condition is met.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManAreMostUsedPi_rec( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return;
    Gia_ObjSetTravIdCurrent(p, pObj);
    if ( Gia_ObjIsCi(pObj) )
    {
        pObj->Value++;
        return;
    }
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManAreMostUsedPi_rec( p, Gia_ObjFanin0(pObj) );
    Gia_ManAreMostUsedPi_rec( p, Gia_ObjFanin1(pObj) );
}

/**Function*************************************************************

  Synopsis    [Returns the most used CI, or NULL if condition is met.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Gia_Obj_t * Gia_ManAreMostUsedPi( Gia_ManAre_t * p )
{
    Gia_Obj_t * pObj, * pObjMax = NULL;
    int i;
    // clean CI counters
    Gia_ManForEachCi( p->pNew, pObj, i )
        pObj->Value = 0;
    // traverse from each register output
    Gia_ManForEachRi( p->pAig, pObj, i )
    {
        if ( pObj->Value <= 1 )
            continue;
        Gia_ManIncrementTravId( p->pNew );
        Gia_ManAreMostUsedPi_rec( p->pNew, Gia_ManObj(p->pNew, Abc_Lit2Var(pObj->Value)) );
    }
    // check the CI counters
    Gia_ManForEachCi( p->pNew, pObj, i )
        if ( pObjMax == NULL || pObjMax->Value < pObj->Value )
            pObjMax = pObj;
    // return the result
    return pObjMax->Value > 1 ? pObjMax : NULL;
}

/**Function*************************************************************

  Synopsis    [Counts maximum support of primary outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManCheckPOs_rec( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return 0;
    Gia_ObjSetTravIdCurrent(p, pObj);
    if ( Gia_ObjIsCi(pObj) )
        return 1;
    assert( Gia_ObjIsAnd(pObj) );
    return Gia_ManCheckPOs_rec( p, Gia_ObjFanin0(pObj) ) +
           Gia_ManCheckPOs_rec( p, Gia_ObjFanin1(pObj) );
}

/**Function*************************************************************

  Synopsis    [Counts maximum support of primary outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_ManCheckPOs( Gia_ManAre_t * p )
{
    Gia_Obj_t * pObj, * pObjNew;
    int i, CountCur, CountMax = 0;
    Gia_ManForEachPo( p->pAig, pObj, i )
    {
        pObjNew = Gia_ManObj( p->pNew, Abc_Lit2Var(pObj->Value) );
        if ( Gia_ObjIsConst0(pObjNew) )
            CountCur = 0;
        else 
        {
            Gia_ManIncrementTravId( p->pNew );
            CountCur = Gia_ManCheckPOs_rec( p->pNew, pObjNew );
        }
        CountMax = Abc_MaxInt( CountMax, CountCur );
    }
    return CountMax;
}

/**Function*************************************************************

  Synopsis    [Returns the status of the primary outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_ManCheckPOstatus( Gia_ManAre_t * p )
{
    Gia_Obj_t * pObj, * pObjNew;
    int i;
    Gia_ManForEachPo( p->pAig, pObj, i )
    {
        pObjNew = Gia_ManObj( p->pNew, Abc_Lit2Var(pObj->Value) );
        if ( Gia_ObjIsConst0(pObjNew) )
        {
            if ( Abc_LitIsCompl(pObj->Value) )
            {
                p->iOutFail = i;
                return 1;
            }
        }
        else 
        {
            p->iOutFail = i;
//            printf( "To fix later:  PO may be assertable.\n" );
            return 1;
        }
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Derives next state cubes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManAreDeriveNexts_rec( Gia_ManAre_t * p, Gia_PtrAre_t Sta )
{
    Gia_Obj_t * pPivot;
    Vec_Int_t * vLits, * vTfos;
    Gia_Obj_t * pObj;
    int i;
    abctime clk;
    if ( ++p->nRecCalls == MAX_CALL_NUM )
        return 0;
    if ( (pPivot = Gia_ManAreMostUsedPi(p)) == NULL )
    {
        Gia_StaAre_t * pNew;
        clk = Abc_Clock();
        pNew = Gia_ManAreCreateStaNew( p );
        pNew->iPrev = Sta;
        p->fStopped = (p->fMiter && (Gia_ManCheckPOstatus(p) & 1));
        if ( p->fStopped )
        {
            assert( p->pTarget == NULL );
            p->pTarget = pNew;
            return 1;
        }
        Gia_ManAreCubeProcess( p, pNew );
        p->timeCube += Abc_Clock() - clk;
        return p->fStopped;
    }
    // remember values in the cone and perform update
    vTfos = Vec_VecEntryInt( p->vCiTfos, Gia_ObjCioId(pPivot) );
    vLits = Vec_VecEntryInt( p->vCiLits, Gia_ObjCioId(pPivot) );
    assert( Vec_IntSize(vTfos) == Vec_IntSize(vLits) );
    Gia_ManForEachObjVec( vTfos, p->pAig, pObj, i )
    {
        Vec_IntWriteEntry( vLits, i, pObj->Value );
        if ( Gia_ObjIsAnd(pObj) )
            pObj->Value = Gia_ManHashAnd( p->pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else if ( Gia_ObjIsCo(pObj) )
            pObj->Value = Gia_ObjFanin0Copy(pObj);
        else 
        {
            assert( Gia_ObjIsCi(pObj) );
            pObj->Value = 0;
        }
    }
    if ( Gia_ManAreDeriveNexts_rec( p, Sta ) )
        return 1;
    // compute different values
    Gia_ManForEachObjVec( vTfos, p->pAig, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
            pObj->Value = Gia_ManHashAnd( p->pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else if ( Gia_ObjIsCo(pObj) )
            pObj->Value = Gia_ObjFanin0Copy(pObj);
        else 
        {
            assert( Gia_ObjIsCi(pObj) );
            pObj->Value = 1;
        }
    }
    if ( Gia_ManAreDeriveNexts_rec( p, Sta ) )
        return 1;
    // reset the original values
    Gia_ManForEachObjVec( vTfos, p->pAig, pObj, i )
        pObj->Value = Vec_IntEntry( vLits, i );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Derives next state cubes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManAreDeriveNexts( Gia_ManAre_t * p, Gia_PtrAre_t Sta )
{
    Gia_StaAre_t * pSta;
    Gia_Obj_t * pObj;
    int i, RetValue;
    abctime clk = Abc_Clock();
    pSta = Gia_ManAreSta( p, Sta );
    if ( Gia_StaIsUnused(pSta) )
        return 0;
    // recycle the manager
    if ( p->pNew && Gia_ManObjNum(p->pNew) > 1000000 )
    {
        Gia_ManStop( p->pNew );
        p->pNew = NULL;
    }
    // allocate the manager
    if ( p->pNew == NULL )
    {
        p->pNew = Gia_ManStart( 10 * Gia_ManObjNum(p->pAig) );
        Gia_ManIncrementTravId( p->pNew );
        Gia_ManHashAlloc( p->pNew );
        Gia_ManConst0(p->pAig)->Value = 0;
        Gia_ManForEachCi( p->pAig, pObj, i )
            pObj->Value = Gia_ManAppendCi(p->pNew);
    }
    Gia_ManForEachRo( p->pAig, pObj, i )
    {
        if ( Gia_StaHasValue0( pSta, i ) )
            pObj->Value = 0;
        else if ( Gia_StaHasValue1( pSta, i ) )
            pObj->Value = 1;
        else // don't-care literal
            pObj->Value = Abc_Var2Lit( Gia_ObjId( p->pNew, Gia_ManCi(p->pNew, Gia_ObjCioId(pObj)) ), 0 );
    }
    Gia_ManForEachAnd( p->pAig, pObj, i )
        pObj->Value = Gia_ManHashAnd( p->pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachCo( p->pAig, pObj, i )
        pObj->Value = Gia_ObjFanin0Copy(pObj);

    // perform case-splitting
    p->nRecCalls = 0;
    RetValue = Gia_ManAreDeriveNexts_rec( p, Sta );
    if ( p->nRecCalls >= MAX_CALL_NUM )
    {
        printf( "Exceeded the limit on the number of transitions from a state cube (%d).\n", MAX_CALL_NUM );
        p->fStopped = 1;
    }
//    printf( "%d ", p->nRecCalls );
//printf( "%d ", Gia_ManObjNum(p->pNew) );
    p->timeAig += Abc_Clock() - clk;
    return RetValue;
}

 
/**Function*************************************************************

  Synopsis    [Prints the report]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManArePrintReport( Gia_ManAre_t * p, abctime Time, int fFinal )
{
    printf( "States =%10d. Reached =%10d. R = %5.3f. Depth =%6d. Mem =%9.2f MB.  ", 
        p->iStaCur, p->nStas, 1.0*p->iStaCur/p->nStas, Gia_ManAreDepth(p, p->iStaCur), 
        (sizeof(Gia_ManAre_t) + 4.0*Gia_ManRegNum(p->pAig) + 8.0*MAX_PAGE_NUM + 
         4.0*p->nStaPages*p->nSize*MAX_ITEM_NUM + 16.0*p->nObjPages*MAX_ITEM_NUM)/(1<<20) );
    if ( fFinal )
    {
        ABC_PRT( "Time", Abc_Clock() - Time );
    }
    else
    {
        ABC_PRTr( "Time", Abc_Clock() - Time );
    }
}
 
/**Function*************************************************************

  Synopsis    [Performs explicit reachability.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManArePerform( Gia_Man_t * pAig, int nStatesMax, int fMiter, int fVerbose )
{
//    extern Gia_Man_t * Gia_ManCompress2( Gia_Man_t * p, int fUpdateLevel, int fVerbose );
    extern Abc_Cex_t * Gia_ManAreDeriveCex( Gia_ManAre_t * p, Gia_StaAre_t * pLast );
    Gia_ManAre_t * p;
    abctime clk = Abc_Clock();
    int RetValue = 1;
    if ( Gia_ManRegNum(pAig) > MAX_VARS_NUM )
    {
        printf( "Currently can only handle circuit with up to %d registers.\n", MAX_VARS_NUM );
        return -1;
    }
    ABC_FREE( pAig->pCexSeq );
//    p = Gia_ManAreCreate( Gia_ManCompress2(pAig, 0, 0) );
    p = Gia_ManAreCreate( Gia_ManDup(pAig) );
    p->fMiter = fMiter;
    Gia_ManAreCubeProcess( p, Gia_ManAreCreateStaInit(p) );
    for ( p->iStaCur = 1; p->iStaCur < p->nStas; p->iStaCur++ )
    {
//        printf( "Explored state %d. Total cubes %d.\n", p->iStaCur, p->nStas-1 );
        if ( Gia_ManAreDeriveNexts( p, Gia_Int2Ptr(p->iStaCur) ) || p->nStas > nStatesMax )
            pAig->pCexSeq = Gia_ManAreDeriveCex( p, p->pTarget );
        if ( p->fStopped )
        {
            RetValue = -1;
            break;
        }
        if ( fVerbose )//&& p->iStaCur % 5000 == 0 )
            Gia_ManArePrintReport( p, clk, 0 );
    }
    Gia_ManArePrintReport( p, clk, 1 );
    printf( "%s after finding %d state cubes (%d not contained) with depth %d.  ", 
        p->fStopped ? "Stopped" : "Completed", 
        p->nStas, Gia_ManAreListCountUsed(p), 
        Gia_ManAreDepth(p, p->iStaCur-1) );
    ABC_PRT( "Time", Abc_Clock() - clk );
    if ( pAig->pCexSeq != NULL )
        Abc_Print( 1, "Output %d of miter \"%s\" was asserted in frame %d.\n", 
            p->iStaCur, pAig->pName, Gia_ManAreDepth(p, p->iStaCur)-1 );
    if ( fVerbose )
    {
        ABC_PRTP( "Cofactoring", p->timeAig - p->timeCube,    Abc_Clock() - clk );
        ABC_PRTP( "Containment", p->timeCube,                 Abc_Clock() - clk );
        ABC_PRTP( "Other      ", Abc_Clock() - clk - p->timeAig,  Abc_Clock() - clk );
        ABC_PRTP( "TOTAL      ", Abc_Clock() - clk,               Abc_Clock() - clk );
    }
    if ( Gia_ManRegNum(pAig) <= 30 )
    {
        clk = Abc_Clock();
        printf( "The number of unique state minterms in computed state cubes is %d.   ", Gia_ManCountMinterms(p) );
        ABC_PRT( "Time", Abc_Clock() - clk );
    }
//    printf( "Compares = %d.  Equals = %d.  Disj = %d.  Disj2 = %d.  Disj3 = %d.\n", 
//        p->nCompares, p->nEquals, p->nDisjs, p->nDisjs2, p->nDisjs3 );
//    Gia_ManAreFindBestVar( p, Gia_ManAreSta(p, p->Root) );
//    Gia_ManArePrintUsed( p );
    Gia_ManAreFree( p );
    // verify
    if ( pAig->pCexSeq )
    {
        if ( !Gia_ManVerifyCex( pAig, pAig->pCexSeq, 0 ) )
            printf( "Generated counter-example is INVALID.                       \n" );
        else
            printf( "Generated counter-example verified correctly.               \n" );
        return 0;
    }
    return RetValue;
}

ABC_NAMESPACE_IMPL_END

#include "giaAig.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satSolver.h"

ABC_NAMESPACE_IMPL_START


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManAreDeriveCexSatStart( Gia_ManAre_t * p )
{
    Aig_Man_t * pAig2;
    Cnf_Dat_t * pCnf;
    assert( p->pSat == NULL );
    pAig2 = Gia_ManToAig( p->pAig, 0 );
    Aig_ManSetRegNum( pAig2, 0 );
    pCnf = Cnf_Derive( pAig2, Gia_ManCoNum(p->pAig) );
    p->pSat = Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
    p->vSatNumCis = Cnf_DataCollectCiSatNums( pCnf, pAig2 );
    p->vSatNumCos = Cnf_DataCollectCoSatNums( pCnf, pAig2 );
    Cnf_DataFree( pCnf );
    Aig_ManStop( pAig2 );
    p->vAssumps = Vec_IntAlloc( 100 );
    p->vCofVars = Vec_IntAlloc( 100 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManAreDeriveCexSatStop( Gia_ManAre_t * p )
{
    assert( p->pSat != NULL );
    assert( p->pTarget != NULL );
    sat_solver_delete( (sat_solver *)p->pSat );
    Vec_IntFree( p->vSatNumCis );
    Vec_IntFree( p->vSatNumCos );
    Vec_IntFree( p->vAssumps );
    Vec_IntFree( p->vCofVars );
    p->pTarget = NULL;
    p->pSat = NULL;
}

/**Function*************************************************************

  Synopsis    [Computes satisfying assignment in one timeframe.]

  Description [Returns the vector of integers represeting PIO ids
  of the primary inputs that should be 1 in the counter-example.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManAreDeriveCexSat( Gia_ManAre_t * p, Gia_StaAre_t * pCur, Gia_StaAre_t * pNext, int iOutFailed )
{
    int i, status;
    // make assuptions
    Vec_IntClear( p->vAssumps );
    for ( i = 0; i < Gia_ManRegNum(p->pAig); i++ )
    {
        if ( Gia_StaHasValue0(pCur, i) )
            Vec_IntPush( p->vAssumps, Abc_Var2Lit( Vec_IntEntry(p->vSatNumCis, Gia_ManPiNum(p->pAig)+i), 1 ) );
        else if ( Gia_StaHasValue1(pCur, i) )
            Vec_IntPush( p->vAssumps, Abc_Var2Lit( Vec_IntEntry(p->vSatNumCis, Gia_ManPiNum(p->pAig)+i), 0 ) );
    }
    if ( pNext )
    for ( i = 0; i < Gia_ManRegNum(p->pAig); i++ )
    {
        if ( Gia_StaHasValue0(pNext, i) )
            Vec_IntPush( p->vAssumps, Abc_Var2Lit( Vec_IntEntry(p->vSatNumCos, Gia_ManPoNum(p->pAig)+i), 1 ) );
        else if ( Gia_StaHasValue1(pNext, i) )
            Vec_IntPush( p->vAssumps, Abc_Var2Lit( Vec_IntEntry(p->vSatNumCos, Gia_ManPoNum(p->pAig)+i), 0 ) );
    }
    if ( iOutFailed >= 0 )
    {
        assert( iOutFailed < Gia_ManPoNum(p->pAig) );
        Vec_IntPush( p->vAssumps, Abc_Var2Lit( Vec_IntEntry(p->vSatNumCos, iOutFailed), 0 ) );
    }
    // solve SAT
    status = sat_solver_solve( (sat_solver *)p->pSat, (int *)Vec_IntArray(p->vAssumps), (int *)Vec_IntArray(p->vAssumps) + Vec_IntSize(p->vAssumps), 
        (ABC_INT64_T)1000000, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
    if ( status != l_True )
    {
        printf( "SAT problem is not satisfiable. Failure...\n" );
        return;
    }
    assert( status == l_True );
    // check the model
    Vec_IntClear( p->vCofVars );
    for ( i = 0; i < Gia_ManPiNum(p->pAig); i++ )
    {
        if ( sat_solver_var_value( (sat_solver *)p->pSat, Vec_IntEntry(p->vSatNumCis, i) ) )
            Vec_IntPush( p->vCofVars, i );
    }
    // write the current state
    for ( i = 0; i < Gia_ManRegNum(p->pAig); i++ )
    {
        if ( Gia_StaHasValue0(pCur, i) )
            assert( sat_solver_var_value( (sat_solver *)p->pSat, Vec_IntEntry(p->vSatNumCis, Gia_ManPiNum(p->pAig)+i) ) == 0 );
        else if ( Gia_StaHasValue1(pCur, i) )
            assert( sat_solver_var_value( (sat_solver *)p->pSat, Vec_IntEntry(p->vSatNumCis, Gia_ManPiNum(p->pAig)+i) ) == 1 );
        // set don't-care value
        if ( sat_solver_var_value( (sat_solver *)p->pSat, Vec_IntEntry(p->vSatNumCis, Gia_ManPiNum(p->pAig)+i) ) == 0 )
            Gia_StaSetValue0( pCur, i );
        else
            Gia_StaSetValue1( pCur, i );
    }
}

/**Function*************************************************************

  Synopsis    [Returns the status of the cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Gia_ManAreDeriveCex( Gia_ManAre_t * p, Gia_StaAre_t * pLast )
{
    Abc_Cex_t * pCex;
    Vec_Ptr_t * vStates;
    Gia_StaAre_t * pSta, * pPrev;
    int Var, i, v;
    assert( p->iOutFail >= 0 );
    Gia_ManAreDeriveCexSatStart( p );
    // compute the trace
    vStates = Vec_PtrAlloc( 1000 );
    for ( pSta = pLast; Gia_StaIsGood(p, pSta); pSta = Gia_StaPrev(p, pSta) )
        if ( pSta != pLast )
            Vec_PtrPush( vStates, pSta );
    assert( Vec_PtrSize(vStates) >= 1 );
    // start the counter-example
    pCex = Abc_CexAlloc( Gia_ManRegNum(p->pAig), Gia_ManPiNum(p->pAig), Vec_PtrSize(vStates) );
    pCex->iFrame = Vec_PtrSize(vStates)-1;
    pCex->iPo = p->iOutFail;
    // compute states
    pPrev = NULL;
    Vec_PtrForEachEntry( Gia_StaAre_t *, vStates, pSta, i )
    {
        Gia_ManAreDeriveCexSat( p, pSta, pPrev, (i == 0) ? p->iOutFail : -1 ); 
        pPrev = pSta;
        // create the counter-example
        Vec_IntForEachEntry( p->vCofVars, Var, v )
        {
            assert( Var < Gia_ManPiNum(p->pAig) );
            Abc_InfoSetBit( pCex->pData, Gia_ManRegNum(p->pAig) + (Vec_PtrSize(vStates)-1-i) * Gia_ManPiNum(p->pAig) + Var );
        }
    }
    // free temporary things
    Vec_PtrFree( vStates );
    Gia_ManAreDeriveCexSatStop( p );
    return pCex;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

