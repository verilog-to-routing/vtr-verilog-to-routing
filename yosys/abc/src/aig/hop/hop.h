/**CFile****************************************************************

  FileName    [hop.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Minimalistic And-Inverter Graph package.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 11, 2006.]

  Revision    [$Id: hop.h,v 1.00 2006/05/11 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__aig__hop__hop_h
#define ABC__aig__hop__hop_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "misc/vec/vec.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START
 

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Hop_Man_t_            Hop_Man_t;
typedef struct Hop_Obj_t_            Hop_Obj_t;
typedef int                          Hop_Edge_t;

// object types
typedef enum { 
    AIG_NONE,                        // 0: non-existent object
    AIG_CONST1,                      // 1: constant 1 
    AIG_PI,                          // 2: primary input
    AIG_PO,                          // 3: primary output
    AIG_AND,                         // 4: AND node
    AIG_EXOR,                        // 5: EXOR node
    AIG_VOID                         // 6: unused object
} Hop_Type_t;

// the AIG node
struct Hop_Obj_t_  // 6 words
{
    union { 
        void *       pData;          // misc
        int          iData; };       // misc
    union { 
        Hop_Obj_t *  pNext;          // strashing table
        int          PioNum; };      // the number of PI/PO
    Hop_Obj_t *      pFanin0;        // fanin
    Hop_Obj_t *      pFanin1;        // fanin
    unsigned int     Type    :  3;   // object type
    unsigned int     fPhase  :  1;   // value under 000...0 pattern
    unsigned int     fMarkA  :  1;   // multipurpose mask
    unsigned int     fMarkB  :  1;   // multipurpose mask
    unsigned int     nRefs   : 26;   // reference count (level)
    int              Id;             // unique ID of the node
};

// the AIG manager
struct Hop_Man_t_
{
    // AIG nodes
    Vec_Ptr_t *      vPis;           // the array of PIs
    Vec_Ptr_t *      vPos;           // the array of POs
    Vec_Ptr_t *      vObjs;          // the array of all nodes (optional)
    Hop_Obj_t *      pConst1;        // the constant 1 node
    Hop_Obj_t        Ghost;          // the ghost node
    // AIG node counters
    int              nObjs[AIG_VOID];// the number of objects by type
    int              nCreated;       // the number of created objects
    int              nDeleted;       // the number of deleted objects
    // stuctural hash table
    Hop_Obj_t **     pTable;         // structural hash table
    int              nTableSize;     // structural hash table size
    // various data members
    void *           pData;          // the temporary data
    int              nTravIds;       // the current traversal ID
    int              fRefCount;      // enables reference counting
    int              fCatchExor;     // enables EXOR nodes
    // memory management
    Vec_Ptr_t *      vChunks;        // allocated memory pieces
    Vec_Ptr_t *      vPages;         // memory pages used by nodes
    Hop_Obj_t *      pListFree;      // the list of free nodes 
    // timing statistics
    abctime          time1;
    abctime          time2;
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////
extern void Hop_ManAddMemory( Hop_Man_t * p );

static inline int          Hop_BitWordNum( int nBits )            { return (nBits>>5) + ((nBits&31) > 0);           }
static inline int          Hop_TruthWordNum( int nVars )          { return nVars <= 5 ? 1 : (1 << (nVars - 5));     }
static inline int          Hop_InfoHasBit( unsigned * p, int i )  { return (p[(i)>>5] & (1<<((i) & 31))) > 0;       }
static inline void         Hop_InfoSetBit( unsigned * p, int i )  { p[(i)>>5] |= (1<<((i) & 31));                   }
static inline void         Hop_InfoXorBit( unsigned * p, int i )  { p[(i)>>5] ^= (1<<((i) & 31));                   }
static inline int          Hop_Base2Log( unsigned n )             { int r; if ( n < 2 ) return n; for ( r = 0, n--; n; n >>= 1, r++ ) {}; return r; }
static inline int          Hop_Base10Log( unsigned n )            { int r; if ( n < 2 ) return n; for ( r = 0, n--; n; n /= 10, r++ ) {}; return r; }

static inline Hop_Obj_t *  Hop_Regular( Hop_Obj_t * p )           { return (Hop_Obj_t *)((ABC_PTRUINT_T)(p) & ~01); }
static inline Hop_Obj_t *  Hop_Not( Hop_Obj_t * p )               { return (Hop_Obj_t *)((ABC_PTRUINT_T)(p) ^  01); }
static inline Hop_Obj_t *  Hop_NotCond( Hop_Obj_t * p, int c )    { return (Hop_Obj_t *)((ABC_PTRUINT_T)(p) ^ (c)); }
static inline int          Hop_IsComplement( Hop_Obj_t * p )      { return (int)((ABC_PTRUINT_T)(p) & 01);          }

static inline Hop_Obj_t *  Hop_ManConst0( Hop_Man_t * p )         { return Hop_Not(p->pConst1);                     }
static inline Hop_Obj_t *  Hop_ManConst1( Hop_Man_t * p )         { return p->pConst1;                              }
static inline Hop_Obj_t *  Hop_ManGhost( Hop_Man_t * p )          { return &p->Ghost;                               }
static inline Hop_Obj_t *  Hop_ManPi( Hop_Man_t * p, int i )      { return (Hop_Obj_t *)Vec_PtrEntry(p->vPis, i);   }
static inline Hop_Obj_t *  Hop_ManPo( Hop_Man_t * p, int i )      { return (Hop_Obj_t *)Vec_PtrEntry(p->vPos, i);   }
static inline Hop_Obj_t *  Hop_ManObj( Hop_Man_t * p, int i )     { return p->vObjs ? (Hop_Obj_t *)Vec_PtrEntry(p->vObjs, i) : NULL;  }

static inline Hop_Edge_t   Hop_EdgeCreate( int Id, int fCompl )            { return (Id << 1) | fCompl;             }
static inline int          Hop_EdgeId( Hop_Edge_t Edge )                   { return Edge >> 1;                      }
static inline int          Hop_EdgeIsComplement( Hop_Edge_t Edge )         { return Edge & 1;                       }
static inline Hop_Edge_t   Hop_EdgeRegular( Hop_Edge_t Edge )              { return (Edge >> 1) << 1;               }
static inline Hop_Edge_t   Hop_EdgeNot( Hop_Edge_t Edge )                  { return Edge ^ 1;                       }
static inline Hop_Edge_t   Hop_EdgeNotCond( Hop_Edge_t Edge, int fCond )   { return Edge ^ fCond;                   }

static inline int          Hop_ManPiNum( Hop_Man_t * p )          { return p->nObjs[AIG_PI];                    }
static inline int          Hop_ManPoNum( Hop_Man_t * p )          { return p->nObjs[AIG_PO];                    }
static inline int          Hop_ManAndNum( Hop_Man_t * p )         { return p->nObjs[AIG_AND];                   }
static inline int          Hop_ManExorNum( Hop_Man_t * p )        { return p->nObjs[AIG_EXOR];                  }
static inline int          Hop_ManNodeNum( Hop_Man_t * p )        { return p->nObjs[AIG_AND]+p->nObjs[AIG_EXOR];}
static inline int          Hop_ManGetCost( Hop_Man_t * p )        { return p->nObjs[AIG_AND]+3*p->nObjs[AIG_EXOR]; }
static inline int          Hop_ManObjNum( Hop_Man_t * p )         { return p->nCreated - p->nDeleted;           }

static inline Hop_Type_t   Hop_ObjType( Hop_Obj_t * pObj )        { return (Hop_Type_t)pObj->Type;               }
static inline int          Hop_ObjIsNone( Hop_Obj_t * pObj )      { return pObj->Type == AIG_NONE;   }
static inline int          Hop_ObjIsConst1( Hop_Obj_t * pObj )    { assert(!Hop_IsComplement(pObj)); return pObj->Type == AIG_CONST1; }
static inline int          Hop_ObjIsPi( Hop_Obj_t * pObj )        { return pObj->Type == AIG_PI;     }
static inline int          Hop_ObjIsPo( Hop_Obj_t * pObj )        { return pObj->Type == AIG_PO;     }
static inline int          Hop_ObjIsAnd( Hop_Obj_t * pObj )       { return pObj->Type == AIG_AND;    }
static inline int          Hop_ObjIsExor( Hop_Obj_t * pObj )      { return pObj->Type == AIG_EXOR;   }
static inline int          Hop_ObjIsNode( Hop_Obj_t * pObj )      { return pObj->Type == AIG_AND || pObj->Type == AIG_EXOR;   }
static inline int          Hop_ObjIsTerm( Hop_Obj_t * pObj )      { return pObj->Type == AIG_PI  || pObj->Type == AIG_PO || pObj->Type == AIG_CONST1; }
static inline int          Hop_ObjIsHash( Hop_Obj_t * pObj )      { return pObj->Type == AIG_AND || pObj->Type == AIG_EXOR;   }

static inline int          Hop_ObjIsMarkA( Hop_Obj_t * pObj )     { return pObj->fMarkA;  }
static inline void         Hop_ObjSetMarkA( Hop_Obj_t * pObj )    { pObj->fMarkA = 1;     }
static inline void         Hop_ObjClearMarkA( Hop_Obj_t * pObj )  { pObj->fMarkA = 0;     }
 
static inline void         Hop_ObjSetTravId( Hop_Obj_t * pObj, int TravId )                { pObj->pData = (void *)(ABC_PTRINT_T)TravId;                      }
static inline void         Hop_ObjSetTravIdCurrent( Hop_Man_t * p, Hop_Obj_t * pObj )      { pObj->pData = (void *)(ABC_PTRINT_T)p->nTravIds;                 }
static inline void         Hop_ObjSetTravIdPrevious( Hop_Man_t * p, Hop_Obj_t * pObj )     { pObj->pData = (void *)(ABC_PTRINT_T)(p->nTravIds - 1);           }
static inline int          Hop_ObjIsTravIdCurrent( Hop_Man_t * p, Hop_Obj_t * pObj )       { return (int)((int)(ABC_PTRINT_T)pObj->pData == p->nTravIds);     }
static inline int          Hop_ObjIsTravIdPrevious( Hop_Man_t * p, Hop_Obj_t * pObj )      { return (int)((int)(ABC_PTRINT_T)pObj->pData == p->nTravIds - 1); }

static inline int          Hop_ObjTravId( Hop_Obj_t * pObj )      { return (int)(ABC_PTRINT_T)pObj->pData;        }
static inline int          Hop_ObjPhase( Hop_Obj_t * pObj )       { return pObj->fPhase;                           }
static inline int          Hop_ObjRefs( Hop_Obj_t * pObj )        { return pObj->nRefs;                            }
static inline void         Hop_ObjRef( Hop_Obj_t * pObj )         { pObj->nRefs++;                                 }
static inline void         Hop_ObjDeref( Hop_Obj_t * pObj )       { assert( pObj->nRefs > 0 ); pObj->nRefs--;      }
static inline void         Hop_ObjClearRef( Hop_Obj_t * pObj )    { pObj->nRefs = 0;                               }
static inline int          Hop_ObjFaninC0( Hop_Obj_t * pObj )     { return Hop_IsComplement(pObj->pFanin0);        }
static inline int          Hop_ObjFaninC1( Hop_Obj_t * pObj )     { return Hop_IsComplement(pObj->pFanin1);        }
static inline Hop_Obj_t *  Hop_ObjFanin0( Hop_Obj_t * pObj )      { return Hop_Regular(pObj->pFanin0);             }
static inline Hop_Obj_t *  Hop_ObjFanin1( Hop_Obj_t * pObj )      { return Hop_Regular(pObj->pFanin1);             }
static inline Hop_Obj_t *  Hop_ObjChild0( Hop_Obj_t * pObj )      { return pObj->pFanin0;                          }
static inline Hop_Obj_t *  Hop_ObjChild1( Hop_Obj_t * pObj )      { return pObj->pFanin1;                          }
static inline Hop_Obj_t *  Hop_ObjChild0Copy( Hop_Obj_t * pObj )  { assert( !Hop_IsComplement(pObj) ); return Hop_ObjFanin0(pObj)? Hop_NotCond((Hop_Obj_t *)Hop_ObjFanin0(pObj)->pData, Hop_ObjFaninC0(pObj)) : NULL;  }
static inline Hop_Obj_t *  Hop_ObjChild1Copy( Hop_Obj_t * pObj )  { assert( !Hop_IsComplement(pObj) ); return Hop_ObjFanin1(pObj)? Hop_NotCond((Hop_Obj_t *)Hop_ObjFanin1(pObj)->pData, Hop_ObjFaninC1(pObj)) : NULL;  }
static inline int          Hop_ObjChild0CopyI( Hop_Obj_t * pObj ) { assert( !Hop_IsComplement(pObj) ); return Hop_ObjFanin0(pObj)? Abc_LitNotCond(Hop_ObjFanin0(pObj)->iData, Hop_ObjFaninC0(pObj)) : -1;              }
static inline int          Hop_ObjChild1CopyI( Hop_Obj_t * pObj ) { assert( !Hop_IsComplement(pObj) ); return Hop_ObjFanin1(pObj)? Abc_LitNotCond(Hop_ObjFanin1(pObj)->iData, Hop_ObjFaninC1(pObj)) : -1;              }
static inline int          Hop_ObjLevel( Hop_Obj_t * pObj )       { return pObj->nRefs;                            }
static inline int          Hop_ObjLevelNew( Hop_Obj_t * pObj )    { return 1 + Hop_ObjIsExor(pObj) + Abc_MaxInt(Hop_ObjFanin0(pObj)->nRefs, Hop_ObjFanin1(pObj)->nRefs);       }
static inline int          Hop_ObjPhaseCompl( Hop_Obj_t * pObj )  { return Hop_IsComplement(pObj)? !Hop_Regular(pObj)->fPhase : pObj->fPhase; }
static inline void         Hop_ObjClean( Hop_Obj_t * pObj )       { memset( pObj, 0, sizeof(Hop_Obj_t) ); }
static inline int          Hop_ObjWhatFanin( Hop_Obj_t * pObj, Hop_Obj_t * pFanin )    
{ 
    if ( Hop_ObjFanin0(pObj) == pFanin ) return 0; 
    if ( Hop_ObjFanin1(pObj) == pFanin ) return 1; 
    assert(0); return -1; 
}
static inline int          Hop_ObjFanoutC( Hop_Obj_t * pObj, Hop_Obj_t * pFanout )    
{ 
    if ( Hop_ObjFanin0(pFanout) == pObj ) return Hop_ObjFaninC0(pObj); 
    if ( Hop_ObjFanin1(pFanout) == pObj ) return Hop_ObjFaninC1(pObj); 
    assert(0); return -1; 
}

// create the ghost of the new node
static inline Hop_Obj_t *  Hop_ObjCreateGhost( Hop_Man_t * p, Hop_Obj_t * p0, Hop_Obj_t * p1, Hop_Type_t Type )    
{
    Hop_Obj_t * pGhost;
    assert( Type != AIG_AND || !Hop_ObjIsConst1(Hop_Regular(p0)) );
    assert( p1 == NULL || !Hop_ObjIsConst1(Hop_Regular(p1)) );
    assert( Type == AIG_PI || Hop_Regular(p0) != Hop_Regular(p1) );
    pGhost = Hop_ManGhost(p);
    pGhost->Type = Type;
    if ( Hop_Regular(p0)->Id < Hop_Regular(p1)->Id )
    {
        pGhost->pFanin0 = p0;
        pGhost->pFanin1 = p1;
    }
    else
    {
        pGhost->pFanin0 = p1;
        pGhost->pFanin1 = p0;
    }
    return pGhost;
}

// internal memory manager
static inline Hop_Obj_t * Hop_ManFetchMemory( Hop_Man_t * p )  
{ 
    Hop_Obj_t * pTemp;
    if ( p->pListFree == NULL )
        Hop_ManAddMemory( p );
    pTemp = p->pListFree;
    p->pListFree = *((Hop_Obj_t **)pTemp);
    memset( pTemp, 0, sizeof(Hop_Obj_t) ); 
    if ( p->vObjs )
    {
        assert( p->nCreated == Vec_PtrSize(p->vObjs) );
        Vec_PtrPush( p->vObjs, pTemp );
    }
    pTemp->Id = p->nCreated++;
    return pTemp;
}
static inline void Hop_ManRecycleMemory( Hop_Man_t * p, Hop_Obj_t * pEntry )
{
    pEntry->Type = AIG_NONE; // distinquishes dead node from live node
    *((Hop_Obj_t **)pEntry) = p->pListFree;
    p->pListFree = pEntry;
}


////////////////////////////////////////////////////////////////////////
///                             ITERATORS                            ///
////////////////////////////////////////////////////////////////////////

// iterator over the primary inputs
#define Hop_ManForEachPi( p, pObj, i )                                          \
    Vec_PtrForEachEntry( Hop_Obj_t *, p->vPis, pObj, i )
// iterator over the primary outputs
#define Hop_ManForEachPo( p, pObj, i )                                          \
    Vec_PtrForEachEntry( Hop_Obj_t *, p->vPos, pObj, i )
// iterator over all objects, including those currently not used
#define Hop_ManForEachNode( p, pObj, i )                                        \
    for ( i = 0; i < p->nTableSize; i++ )                                       \
        if ( ((pObj) = p->pTable[i]) == NULL ) {} else

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== hopBalance.c ========================================================*/
extern Hop_Man_t *     Hop_ManBalance( Hop_Man_t * p, int fUpdateLevel );
extern Hop_Obj_t *     Hop_NodeBalanceBuildSuper( Hop_Man_t * p, Vec_Ptr_t * vSuper, Hop_Type_t Type, int fUpdateLevel );
/*=== hopCheck.c ========================================================*/
extern int             Hop_ManCheck( Hop_Man_t * p );
/*=== hopDfs.c ==========================================================*/
extern Vec_Ptr_t *     Hop_ManDfs( Hop_Man_t * p );
extern Vec_Ptr_t *     Hop_ManDfsNode( Hop_Man_t * p, Hop_Obj_t * pNode );
extern int             Hop_ManCountLevels( Hop_Man_t * p );
extern void            Hop_ManCreateRefs( Hop_Man_t * p );
extern int             Hop_DagSize( Hop_Obj_t * pObj );
extern int             Hop_ObjFanoutCount( Hop_Obj_t * pObj, Hop_Obj_t * pPivot );
extern void            Hop_ConeUnmark_rec( Hop_Obj_t * pObj );
extern Hop_Obj_t *     Hop_Transfer( Hop_Man_t * pSour, Hop_Man_t * pDest, Hop_Obj_t * pObj, int nVars );
extern Hop_Obj_t *     Hop_Compose( Hop_Man_t * p, Hop_Obj_t * pRoot, Hop_Obj_t * pFunc, int iVar );
extern Hop_Obj_t *     Hop_Complement( Hop_Man_t * p, Hop_Obj_t * pRoot, int iVar );
extern Hop_Obj_t *     Hop_Remap( Hop_Man_t * p, Hop_Obj_t * pRoot, unsigned uSupp, int nVars );
extern Hop_Obj_t *     Hop_Permute( Hop_Man_t * p, Hop_Obj_t * pRoot, int nRootVars, int * pPermute );
/*=== hopMan.c ==========================================================*/
extern Hop_Man_t *     Hop_ManStart();
extern Hop_Man_t *     Hop_ManDup( Hop_Man_t * p );
extern void            Hop_ManStop( Hop_Man_t * p );
extern int             Hop_ManCleanup( Hop_Man_t * p );
extern void            Hop_ManPrintStats( Hop_Man_t * p );
/*=== hopMem.c ==========================================================*/
extern void            Hop_ManStartMemory( Hop_Man_t * p );
extern void            Hop_ManStopMemory( Hop_Man_t * p );
/*=== hopObj.c ==========================================================*/
extern Hop_Obj_t *     Hop_ObjCreatePi( Hop_Man_t * p );
extern Hop_Obj_t *     Hop_ObjCreatePo( Hop_Man_t * p, Hop_Obj_t * pDriver );
extern Hop_Obj_t *     Hop_ObjCreate( Hop_Man_t * p, Hop_Obj_t * pGhost );
extern void            Hop_ObjConnect( Hop_Man_t * p, Hop_Obj_t * pObj, Hop_Obj_t * pFan0, Hop_Obj_t * pFan1 );
extern void            Hop_ObjDisconnect( Hop_Man_t * p, Hop_Obj_t * pObj );
extern void            Hop_ObjDelete( Hop_Man_t * p, Hop_Obj_t * pObj );
extern void            Hop_ObjDelete_rec( Hop_Man_t * p, Hop_Obj_t * pObj );
extern Hop_Obj_t *     Hop_ObjRepr( Hop_Obj_t * pObj );
extern void            Hop_ObjCreateChoice( Hop_Obj_t * pOld, Hop_Obj_t * pNew );
/*=== hopOper.c =========================================================*/
extern Hop_Obj_t *     Hop_IthVar( Hop_Man_t * p, int i );
extern Hop_Obj_t *     Hop_Oper( Hop_Man_t * p, Hop_Obj_t * p0, Hop_Obj_t * p1, Hop_Type_t Type );
extern Hop_Obj_t *     Hop_And( Hop_Man_t * p, Hop_Obj_t * p0, Hop_Obj_t * p1 );
extern Hop_Obj_t *     Hop_Or( Hop_Man_t * p, Hop_Obj_t * p0, Hop_Obj_t * p1 );
extern Hop_Obj_t *     Hop_Exor( Hop_Man_t * p, Hop_Obj_t * p0, Hop_Obj_t * p1 );
extern Hop_Obj_t *     Hop_Mux( Hop_Man_t * p, Hop_Obj_t * pC, Hop_Obj_t * p1, Hop_Obj_t * p0 );
extern Hop_Obj_t *     Hop_Maj( Hop_Man_t * p, Hop_Obj_t * pA, Hop_Obj_t * pB, Hop_Obj_t * pC );
extern Hop_Obj_t *     Hop_Miter( Hop_Man_t * p, Vec_Ptr_t * vPairs );
extern Hop_Obj_t *     Hop_CreateAnd( Hop_Man_t * p, int nVars );
extern Hop_Obj_t *     Hop_CreateOr( Hop_Man_t * p, int nVars );
extern Hop_Obj_t *     Hop_CreateExor( Hop_Man_t * p, int nVars );
/*=== hopTable.c ========================================================*/
extern Hop_Obj_t *     Hop_TableLookup( Hop_Man_t * p, Hop_Obj_t * pGhost );
extern void            Hop_TableInsert( Hop_Man_t * p, Hop_Obj_t * pObj );
extern void            Hop_TableDelete( Hop_Man_t * p, Hop_Obj_t * pObj );
extern int             Hop_TableCountEntries( Hop_Man_t * p );
extern void            Hop_TableProfile( Hop_Man_t * p );
/*=== hopTruth.c ========================================================*/
extern unsigned *      Hop_ManConvertAigToTruth( Hop_Man_t * p, Hop_Obj_t * pRoot, int nVars, Vec_Int_t * vTruth, int fMsbFirst );
extern word            Hop_ManComputeTruth6( Hop_Man_t * p, Hop_Obj_t * pObj, int nVars );
/*=== hopUtil.c =========================================================*/
extern void            Hop_ManIncrementTravId( Hop_Man_t * p );
extern void            Hop_ManCleanData( Hop_Man_t * p );
extern void            Hop_ObjCleanData_rec( Hop_Obj_t * pObj );
extern void            Hop_ObjCollectMulti( Hop_Obj_t * pFunc, Vec_Ptr_t * vSuper );
extern int             Hop_ObjIsMuxType( Hop_Obj_t * pObj );
extern int             Hop_ObjRecognizeExor( Hop_Obj_t * pObj, Hop_Obj_t ** ppFan0, Hop_Obj_t ** ppFan1 );
extern Hop_Obj_t *     Hop_ObjRecognizeMux( Hop_Obj_t * pObj, Hop_Obj_t ** ppObjT, Hop_Obj_t ** ppObjE );
extern void            Hop_ObjPrintEqn( FILE * pFile, Hop_Obj_t * pObj, Vec_Vec_t * vLevels, int Level );
extern void            Hop_ObjPrintVerilog( FILE * pFile, Hop_Obj_t * pObj, Vec_Vec_t * vLevels, int Level, int fOnlyAnds );
extern void            Hop_ObjPrintVerbose( Hop_Obj_t * pObj, int fHaig );
extern void            Hop_ManPrintVerbose( Hop_Man_t * p, int fHaig );
extern void            Hop_ManDumpBlif( Hop_Man_t * p, char * pFileName );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

