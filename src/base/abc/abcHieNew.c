/**CFile****************************************************************

  FileName    [abcHieNew.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [New hierarchy manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcHieNew.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "misc/vec/vec.h"
#include "misc/util/utilNam.h"
#include "misc/extra/extra.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define AU_MAX_FANINS 0x1FFFFFFF
 
typedef enum { 
    AU_OBJ_NONE,           // 0: non-existent object
    AU_OBJ_CONST0,         // 1: constant node
    AU_OBJ_PI,             // 2: primary input
    AU_OBJ_PO,             // 3: primary output
    AU_OBJ_FAN,            // 4: box output
    AU_OBJ_FLOP,           // 5: flip-flop
    AU_OBJ_BOX,            // 6: box
    AU_OBJ_NODE,           // 7: logic node
    AU_OBJ_VOID            // 8: placeholder
} Au_Type_t;


typedef struct Au_Man_t_   Au_Man_t;
typedef struct Au_Ntk_t_   Au_Ntk_t;
typedef struct Au_Obj_t_   Au_Obj_t;

struct Au_Obj_t_ // 16 bytes
{
    unsigned               Func    : 30;       // functionality
    unsigned               Value   :  2;       // node value
    unsigned               Type    :  3;       // object type
    unsigned               nFanins : 29;       // fanin count (related to AU_MAX_FANIN_NUM)
    int                    Fanins[2];          // fanin literals
};

struct Au_Ntk_t_ 
{
    char *                 pName;              // model name
    Au_Man_t *             pMan;               // model manager
    int                    Id;                 // model ID
    // objects
    Vec_Int_t              vPis;               // primary inputs (CI id -> handle)
    Vec_Int_t              vPos;               // primary outputs (CI id -> handle)
    Vec_Int_t              vObjs;              // internal nodes (obj id -> handle)
    int                    nObjs[AU_OBJ_VOID]; // counter of objects of each type
    // memory for objects
    Vec_Ptr_t *            vChunks;            // memory pages
    Vec_Ptr_t              vPages;             // memory pages
    int                    iHandle;            // currently available ID
    int                    nObjsAlloc;         // the total number of objects allocated
    int                    nObjsUsed;          // the number of useful entries
    // object attributes
    int                    nTravIds;           // counter of traversal IDs
    Vec_Int_t              vTravIds;           // trav IDs of the objects
    Vec_Int_t              vCopies;            // object copies
    // structural hashing
    int                    nHTable;            // hash table size
    int *                  pHTable;            // hash table
    Au_Obj_t *             pConst0;            // constant node
    // statistics
    int                    fMark;
    double                 nBoxes;
    double                 nNodes;
    double                 nPorts;
    double                 nNodeAnds;
    double                 nNodeXors;
    double                 nNodeMuxs;
};

struct Au_Man_t_ 
{
    char *                 pName;              // the name of the library
    Vec_Ptr_t              vNtks;              // the array of modules
    Abc_Nam_t *            pFuncs;             // hashing functions into integers
    int                    nRefs;              // reference counter
    // statistics
    int                    nGiaObjMax;         // max number of GIA objects
    double                 nPortsC0;           // const ports
    double                 nPortsC1;           // const ports
    double                 nPortsNC;           // non-const ports
};


static inline int          Au_Var2Lit( int Var, int fCompl )             { return Var + Var + fCompl;                       }
static inline int          Au_Lit2Var( int Lit )                         { return Lit >> 1;                                 }
static inline int          Au_LitIsCompl( int Lit )                      { return Lit & 1;                                  }
static inline int          Au_LitNot( int Lit )                          { return Lit ^ 1;                                  }
static inline int          Au_LitNotCond( int Lit, int c )               { return Lit ^ (int)(c > 0);                       }
static inline int          Au_LitRegular( int Lit )                      { return Lit & ~01;                                }

static inline Au_Obj_t *   Au_Regular( Au_Obj_t * p )                    { return (Au_Obj_t *)((ABC_PTRUINT_T)(p) & ~01);   }
static inline Au_Obj_t *   Au_Not( Au_Obj_t * p )                        { return (Au_Obj_t *)((ABC_PTRUINT_T)(p) ^  01);   }
static inline Au_Obj_t *   Au_NotCond( Au_Obj_t * p, int c )             { return (Au_Obj_t *)((ABC_PTRUINT_T)(p) ^ (c));   }
static inline int          Au_IsComplement( Au_Obj_t * p )               { return (int)((ABC_PTRUINT_T)(p) & 01);           }
 
static inline char *       Au_UtilStrsav( char * s )                     { return s ? strcpy(ABC_ALLOC(char, strlen(s)+1), s) : NULL;             }

static inline char *       Au_ManName( Au_Man_t * p )                    { return p->pName;                                                       }
static inline int          Au_ManNtkNum( Au_Man_t * p )                  { return Vec_PtrSize(&p->vNtks) - 1;                                     }
static inline Au_Ntk_t *   Au_ManNtk( Au_Man_t * p, int i )              { return (Au_Ntk_t *)Vec_PtrEntry(&p->vNtks, i);                         }
static inline Au_Ntk_t *   Au_ManNtkRoot( Au_Man_t * p )                 { return Au_ManNtk( p, 1 );                                              }

static inline char *       Au_NtkName( Au_Ntk_t * p )                    { return p->pName;                                                       }
static inline Au_Man_t *   Au_NtkMan( Au_Ntk_t * p )                     { return p->pMan;                                                        }
static inline int          Au_NtkPiNum( Au_Ntk_t * p )                   { return p->nObjs[AU_OBJ_PI];                                            } 
static inline int          Au_NtkPoNum( Au_Ntk_t * p )                   { return p->nObjs[AU_OBJ_PO];                                            } 
static inline int          Au_NtkFanNum( Au_Ntk_t * p )                  { return p->nObjs[AU_OBJ_FAN];                                           } 
static inline int          Au_NtkFlopNum( Au_Ntk_t * p )                 { return p->nObjs[AU_OBJ_FLOP];                                          } 
static inline int          Au_NtkBoxNum( Au_Ntk_t * p )                  { return p->nObjs[AU_OBJ_BOX];                                           } 
static inline int          Au_NtkNodeNum( Au_Ntk_t * p )                 { return p->nObjs[AU_OBJ_NODE];                                          } 
static inline int          Au_NtkObjNumMax( Au_Ntk_t * p )               { return (Vec_PtrSize(&p->vPages) - 1) * (1 << 12) + p->iHandle;         } 
static inline int          Au_NtkObjNum( Au_Ntk_t * p )                  { return Vec_IntSize(&p->vObjs);                                         } 
static inline Au_Obj_t *   Au_NtkObj( Au_Ntk_t * p, int h )              { return (Au_Obj_t *)p->vPages.pArray[h >> 12] + (h & 0xFFF);            }

static inline Au_Obj_t *   Au_NtkPi( Au_Ntk_t * p, int i )               { return Au_NtkObj(p, Vec_IntEntry(&p->vPis, i));                        }
static inline Au_Obj_t *   Au_NtkPo( Au_Ntk_t * p, int i )               { return Au_NtkObj(p, Vec_IntEntry(&p->vPos, i));                        }
static inline Au_Obj_t *   Au_NtkObjI( Au_Ntk_t * p, int i )             { return Au_NtkObj(p, Vec_IntEntry(&p->vObjs, i));                       }

static inline int          Au_ObjIsNone( Au_Obj_t * p )                  { return p->Type == AU_OBJ_NONE;                                         } 
static inline int          Au_ObjIsConst0( Au_Obj_t * p )                { return p->Type == AU_OBJ_CONST0;                                       } 
static inline int          Au_ObjIsPi( Au_Obj_t * p )                    { return p->Type == AU_OBJ_PI;                                           } 
static inline int          Au_ObjIsPo( Au_Obj_t * p )                    { return p->Type == AU_OBJ_PO;                                           } 
static inline int          Au_ObjIsFan( Au_Obj_t * p )                   { return p->Type == AU_OBJ_FAN;                                          } 
static inline int          Au_ObjIsFlop( Au_Obj_t * p )                  { return p->Type == AU_OBJ_FLOP;                                         } 
static inline int          Au_ObjIsBox( Au_Obj_t * p )                   { return p->Type == AU_OBJ_BOX;                                          } 
static inline int          Au_ObjIsNode( Au_Obj_t * p )                  { return p->Type == AU_OBJ_NODE;                                         }
static inline int          Au_ObjIsTerm( Au_Obj_t * p )                  { return p->Type >= AU_OBJ_PI && p->Type <= AU_OBJ_FLOP;                 } 

static inline char *       Au_ObjBase( Au_Obj_t * p )                    { return (char *)p - ((ABC_PTRINT_T)p & 0x3FF);                          } 
static inline Au_Ntk_t *   Au_ObjNtk( Au_Obj_t * p )                     { return ((Au_Ntk_t **)Au_ObjBase(p))[0];                                } 
static inline int          Au_ObjId( Au_Obj_t * p )                      { return ((int *)Au_ObjBase(p))[2] | (((ABC_PTRINT_T)p & 0x3FF) >> 4);   }
static inline int          Au_ObjPioNum( Au_Obj_t * p )                  { assert(Au_ObjIsTerm(p)); return p->Fanins[p->nFanins];                 }
static inline int          Au_ObjFunc( Au_Obj_t * p )                    { return p->Func;                                                        }
static inline Au_Ntk_t *   Au_ObjModel( Au_Obj_t * p )                   { assert(Au_ObjIsFan(p)||Au_ObjIsBox(p)); return Au_ManNtk(Au_NtkMan(Au_ObjNtk(p)), p->Func); }

static inline int          Au_ObjFaninNum( Au_Obj_t * p )                { return p->nFanins;                                                     }
static inline int          Au_ObjFaninId( Au_Obj_t * p, int i )          { assert(i >= 0 && i < (int)p->nFanins && p->Fanins[i]); return Au_Lit2Var(p->Fanins[i]);     }
static inline int          Au_ObjFaninId0( Au_Obj_t * p )                { return Au_ObjFaninId(p, 0);                                                                 }
static inline int          Au_ObjFaninId1( Au_Obj_t * p )                { return Au_ObjFaninId(p, 1);                                                                 }
static inline int          Au_ObjFaninId2( Au_Obj_t * p )                { return Au_ObjFaninId(p, 2);                                                                 }
static inline Au_Obj_t *   Au_ObjFanin( Au_Obj_t * p, int i )            { return Au_NtkObj(Au_ObjNtk(p), Au_ObjFaninId(p, i));                                        }
static inline Au_Obj_t *   Au_ObjFanin0( Au_Obj_t * p )                  { return Au_ObjFanin( p, 0 );                                                                 }
static inline Au_Obj_t *   Au_ObjFanin1( Au_Obj_t * p )                  { return Au_ObjFanin( p, 1 );                                                                 }
static inline Au_Obj_t *   Au_ObjFanin2( Au_Obj_t * p )                  { return Au_ObjFanin( p, 2 );                                                                 }
static inline int          Au_ObjFaninC( Au_Obj_t * p, int i )           { assert(i >= 0 && i < (int)p->nFanins && p->Fanins[i]); return Au_LitIsCompl(p->Fanins[i]);  }
static inline int          Au_ObjFaninC0( Au_Obj_t * p )                 { return Au_ObjFaninC(p, 0);                                                                  }
static inline int          Au_ObjFaninC1( Au_Obj_t * p )                 { return Au_ObjFaninC(p, 1);                                                                  }
static inline int          Au_ObjFaninC2( Au_Obj_t * p )                 { return Au_ObjFaninC(p, 2);                                                                  }
static inline int          Au_ObjFaninLit( Au_Obj_t * p, int i )         { assert(i >= 0 && i < (int)p->nFanins && p->Fanins[i]); return p->Fanins[i];                 }
static inline void         Au_ObjSetFanin( Au_Obj_t * p, int i, int f )  { assert(f > 0 && p->Fanins[i] == 0); p->Fanins[i] = Au_Var2Lit(f,0);                         }
static inline void         Au_ObjSetFaninLit( Au_Obj_t * p, int i, int f){ assert(f >= 0 && p->Fanins[i] == 0); p->Fanins[i] = f;                                      }

static inline int          Au_BoxFanoutNum( Au_Obj_t * p )               { assert(Au_ObjIsBox(p)); return p->Fanins[p->nFanins];                                       }
static inline int          Au_BoxFanoutId( Au_Obj_t * p, int i )         { assert(i >= 0 && i < Au_BoxFanoutNum(p)); return p->Fanins[p->nFanins+1+i];                 }
static inline Au_Obj_t *   Au_BoxFanout( Au_Obj_t * p, int i )           { return Au_NtkObj(Au_ObjNtk(p), Au_BoxFanoutId(p, i));                                       }

static inline int          Au_ObjCopy( Au_Obj_t * p )                    { return Vec_IntEntry( &Au_ObjNtk(p)->vCopies, Au_ObjId(p) );                                 }
static inline void         Au_ObjSetCopy( Au_Obj_t * p, int c )          { Vec_IntWriteEntry( &Au_ObjNtk(p)->vCopies, Au_ObjId(p), c );                                }

static inline int          Au_ObjFanout( Au_Obj_t * p, int i )           { assert(p->Type == AU_OBJ_BOX && i >= 0 && i < p->Fanins[p->nFanins] && p->Fanins[i]); return p->Fanins[p->nFanins + 1 + i];             }
static inline void         Au_ObjSetFanout( Au_Obj_t * p, int i, int f ) { assert(p->Type == AU_OBJ_BOX && i >= 0 && i < p->Fanins[p->nFanins] && p->Fanins[i] == 0 && f > 0); p->Fanins[p->nFanins + 1 + i] = f;  }

static inline void         Au_NtkIncrementTravId( Au_Ntk_t * p )            { if (p->vTravIds.pArray == NULL) Vec_IntFill(&p->vTravIds, Au_NtkObjNumMax(p)+500, 0); p->nTravIds++; assert(p->nTravIds < (1<<30));  }
static inline void         Au_ObjSetTravIdCurrent( Au_Obj_t * p )           { Vec_IntSetEntry(&Au_ObjNtk(p)->vTravIds, Au_ObjId(p), Au_ObjNtk(p)->nTravIds );                        }
static inline void         Au_ObjSetTravIdPrevious( Au_Obj_t * p )          { Vec_IntSetEntry(&Au_ObjNtk(p)->vTravIds, Au_ObjId(p), Au_ObjNtk(p)->nTravIds-1 );                      }
static inline int          Au_ObjIsTravIdCurrent( Au_Obj_t * p )            { return (Vec_IntGetEntry(&Au_ObjNtk(p)->vTravIds, Au_ObjId(p)) == Au_ObjNtk(p)->nTravIds);              }
static inline int          Au_ObjIsTravIdPrevious( Au_Obj_t * p )           { return (Vec_IntGetEntry(&Au_ObjNtk(p)->vTravIds, Au_ObjId(p)) == Au_ObjNtk(p)->nTravIds-1);            }
static inline void         Au_ObjSetTravIdCurrentId( Au_Ntk_t * p, int Id ) { Vec_IntSetEntry(&p->vTravIds, Id, p->nTravIds );                                                       }
static inline int          Au_ObjIsTravIdCurrentId( Au_Ntk_t * p, int Id )  { return (Vec_IntGetEntry(&p->vTravIds, Id) == p->nTravIds);                                             }

#define Au_ManForEachNtk( p, pNtk, i )           \
    for ( i = 1; (i < Vec_PtrSize(&p->vNtks))   && (((pNtk) = Au_ManNtk(p, i)), 1); i++ ) 
#define Au_ManForEachNtkReverse( p, pNtk, i )    \
    for ( i = Vec_PtrSize(&p->vNtks) - 1;(i>=1) && (((pNtk) = Au_ManNtk(p, i)), 1); i-- ) 

#define Au_ObjForEachFaninId( pObj, hFanin, i )  \
    for ( i = 0; (i < Au_ObjFaninNum(pObj))     && (((hFanin) = Au_ObjFaninId(pObj, i)), 1); i++ )  
#define Au_BoxForEachFanoutId( pObj, hFanout, i) \
    for ( i = 0; (i < Au_BoxFanoutNum(pObj))    && (((hFanout) = Au_BoxFanoutId(pObj, i)), 1); i++ )

#define Au_ObjForEachFanin( pObj, pFanin, i )    \
    for ( i = 0; (i < Au_ObjFaninNum(pObj))     && (((pFanin) = Au_ObjFanin(pObj, i)), 1); i++ )  
#define Au_BoxForEachFanout( pObj, pFanout, i)   \
    for ( i = 0; (i < Au_BoxFanoutNum(pObj))    && (((pFanout) = Au_BoxFanout(pObj, i)), 1); i++ )

#define Au_NtkForEachPi( p, pObj, i )            \
    for ( i = 0; (i < Vec_IntSize(&p->vPis))    && (((pObj) = Au_NtkPi(p, i)), 1); i++ )
#define Au_NtkForEachPo( p, pObj, i )            \
    for ( i = 0; (i < Vec_IntSize(&p->vPos))    && (((pObj) = Au_NtkPo(p, i)), 1); i++ )
#define Au_NtkForEachObj( p, pObj, i )           \
    for ( i = 0; (i < Vec_IntSize(&p->vObjs))   && (((pObj) = Au_NtkObjI(p, i)), 1); i++ )
#define Au_NtkForEachNode( p, pObj, i )           \
    for ( i = 0; (i < Vec_IntSize(&p->vObjs))   && (((pObj) = Au_NtkObjI(p, i)), 1); i++ ) if ( !Au_ObjIsNode(pObj) ) {} else
#define Au_NtkForEachBox( p, pObj, i )           \
    for ( i = 0; (i < Vec_IntSize(&p->vObjs))   && (((pObj) = Au_NtkObjI(p, i)), 1); i++ ) if ( !Au_ObjIsBox(pObj) ) {} else



extern void Au_ManAddNtk( Au_Man_t * pMan, Au_Ntk_t * p );
extern void Au_ManFree( Au_Man_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Working with models.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Ntk_t * Au_NtkAlloc( Au_Man_t * pMan, char * pName )
{
    Au_Ntk_t * p;
    p = ABC_CALLOC( Au_Ntk_t, 1 );
    p->pName = Au_UtilStrsav( pName );
    p->vChunks = Vec_PtrAlloc( 111 );
    Vec_IntGrow( &p->vPis,  111 );
    Vec_IntGrow( &p->vPos,  111 );
    Vec_IntGrow( &p->vObjs, 1111 );
    Vec_PtrGrow( &p->vPages, 11 );
    Au_ManAddNtk( pMan, p );
    return p;
}
void Au_NtkFree( Au_Ntk_t * p )
{
    Au_ManFree( p->pMan );
    Vec_PtrFreeFree( p->vChunks );
    ABC_FREE( p->vCopies.pArray );
    ABC_FREE( p->vPages.pArray );
    ABC_FREE( p->vObjs.pArray );
    ABC_FREE( p->vPis.pArray );
    ABC_FREE( p->vPos.pArray );
    ABC_FREE( p->pHTable );
    ABC_FREE( p->pName );
    ABC_FREE( p );
}
int Au_NtkMemUsage( Au_Ntk_t * p )
{
    int Mem = sizeof(Au_Ntk_t);
    Mem += 4 * p->vPis.nCap;
    Mem += 4 * p->vPos.nCap;
    Mem += 4 * p->vObjs.nCap;
    Mem += 16 * p->nObjsAlloc;
    return Mem;
}
void Au_NtkPrintStats( Au_Ntk_t * p )
{
    printf( "%-30s:",        Au_NtkName(p) );
    printf( " i/o =%6d/%6d", Au_NtkPiNum(p), Au_NtkPoNum(p) );
    if ( Au_NtkFlopNum(p) )
        printf( "  lat =%5d",    Au_NtkFlopNum(p) );
    printf( "  nd =%6d",     Au_NtkNodeNum(p) );
//    if ( Au_NtkBoxNum(p) )
        printf( "  box =%5d",    Au_NtkBoxNum(p) );
    printf( "  obj =%7d",    Au_NtkObjNum(p) );
//    printf( "  max =%7d",    Au_NtkObjNumMax(p) );
//    printf( "  use =%7d",    p->nObjsUsed );
    printf( " %5.1f %%",     100.0 * (Au_NtkObjNumMax(p) - Au_NtkObjNum(p)) / Au_NtkObjNumMax(p) );
    printf( " %6.1f MB",     1.0 * Au_NtkMemUsage(p) / (1 << 20) );
    printf( " %5.1f %%",     100.0 * (p->nObjsAlloc - p->nObjsUsed) / p->nObjsAlloc );
    printf( "\n" );
}
void Au_NtkCleanCopy( Au_Ntk_t * p )
{
    Vec_IntFill( &p->vCopies, Au_NtkObjNumMax(p), -1 );
}
int Au_NtkNodeNumFunc( Au_Ntk_t * p, int Func )
{
    Au_Obj_t * pObj;
    int i, Counter = 0;
    if ( p->pMan && p->pMan->pFuncs )
        return 0;
    Au_NtkForEachNode( p, pObj, i )
    {
        Counter += (pObj->Func == (unsigned)Func);
//        printf( "%d ", pObj->Func );
    }
//    printf( "\n" );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Working with manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Man_t * Au_ManAlloc( char * pName )
{
    Au_Man_t * p;
    p = ABC_CALLOC( Au_Man_t, 1 );
    p->pName = Au_UtilStrsav( pName );
    Vec_PtrGrow( &p->vNtks,  111 );
    Vec_PtrPush( &p->vNtks, NULL );
    return p;
}
void Au_ManFree( Au_Man_t * p )
{
    assert( p->nRefs > 0 );
    if ( --p->nRefs > 0 )
        return;
    if ( p->pFuncs )
        Abc_NamStop( p->pFuncs );
    ABC_FREE( p->vNtks.pArray );
    ABC_FREE( p->pName );
    ABC_FREE( p );
}
void Au_ManDelete( Au_Man_t * p )
{
    Au_Ntk_t * pNtk;
    int i;
    Au_ManForEachNtk( p, pNtk, i )
        Au_NtkFree( pNtk );
}
int Au_ManFindNtk( Au_Man_t * p, char * pName )
{
    Au_Ntk_t * pNtk;
    int i;
    Au_ManForEachNtk( p, pNtk, i )
        if ( !strcmp(Au_NtkName(pNtk), pName) )
            return i;
    return -1;
}
Au_Ntk_t * Au_ManFindNtkP( Au_Man_t * p, char * pName )
{
    int iNtk = Au_ManFindNtk( p, pName );
    if ( iNtk == -1 )
        return NULL;
    return Au_ManNtk( p, iNtk );
}
void Au_ManAddNtk( Au_Man_t * pMan, Au_Ntk_t * p )
{
    assert( Au_ManFindNtk(pMan, Au_NtkName(p)) == -1 );
    p->pMan = pMan; pMan->nRefs++;
    p->Id = Vec_PtrSize( &pMan->vNtks );
    Vec_PtrPush( &pMan->vNtks, p );
}
int Au_ManMemUsage( Au_Man_t * p )
{
    Au_Ntk_t * pNtk;
    int i, Mem = 0;
    Au_ManForEachNtk( p, pNtk, i )
        Mem += 16 * pNtk->nObjsAlloc;
    return Mem;
}
int Au_ManMemUsageUseful( Au_Man_t * p )
{
    Au_Ntk_t * pNtk;
    int i, Mem = 0;
    Au_ManForEachNtk( p, pNtk, i )
        Mem += 16 * pNtk->nObjsUsed;
    return Mem;
}
void Au_ManPrintStats( Au_Man_t * p )
{
    Au_Ntk_t * pNtk;
    int i;
    if ( Vec_PtrSize(&p->vNtks) > 2 )
        printf( "Design %-13s\n", Au_ManName(p) );
    Au_ManForEachNtk( p, pNtk, i )
        Au_NtkPrintStats( pNtk );
    printf( "Different functions = %d. ", p->pFuncs ? Abc_NamObjNumMax(p->pFuncs) : 0 );
    printf( "Memory = %.1f MB",  1.0 * Au_ManMemUsage(p) / (1 << 20) );
    printf( " %5.1f %%",       100.0 * (Au_ManMemUsage(p) - Au_ManMemUsageUseful(p)) / Au_ManMemUsage(p) );
    printf( "\n" );
//    if ( p->pFuncs )
//        Abc_NamPrint( p->pFuncs );
}

void Au_ManReorderModels_rec( Au_Ntk_t * pNtk, Vec_Int_t * vOrder )
{
    Au_Ntk_t * pBoxModel;
    Au_Obj_t * pObj;
    int k;
    if ( pNtk->fMark )
        return;
    pNtk->fMark = 1;
    Au_NtkForEachBox( pNtk, pObj, k )
    {
        pBoxModel = Au_ObjModel(pObj);
        if ( pBoxModel == NULL || pBoxModel == pNtk )
            continue;
        Au_ManReorderModels_rec( pBoxModel, vOrder );
    }
    Vec_IntPush( vOrder, pNtk->Id );
}
void Au_ManReorderModels( Au_Man_t * p, Au_Ntk_t * pRoot )
{
    Vec_Ptr_t * vNtksNew;
    Vec_Int_t * vOrder, * vTemp;
    Au_Ntk_t * pNtk, * pBoxModel;
    Au_Obj_t * pBox, * pFan;
    int i, k, j, Entry;
    Au_ManForEachNtk( p, pNtk, i )
        pNtk->fMark = 0;
    // collect networks in the DFS order
    vOrder = Vec_IntAlloc( Au_ManNtkNum(p)+1 );
    Vec_IntPush( vOrder, 0 );
    Au_ManReorderModels_rec( pRoot, vOrder );
    assert( Vec_IntEntryLast(vOrder) == pRoot->Id );
    // add unconnected ones
    Vec_IntPop( vOrder );
    Au_ManForEachNtk( p, pNtk, i )
        if ( pNtk->fMark == 0 )
            Vec_IntPush( vOrder, pNtk->Id );
    Vec_IntPush( vOrder, pRoot->Id );
    assert( Vec_IntSize(vOrder) == Au_ManNtkNum(p)+1 );
    // reverse order
    vOrder->nSize--;
    vOrder->pArray++;
    Vec_IntReverseOrder( vOrder ); 
    vOrder->pArray--;
    vOrder->nSize++;
    // compute new order
    vNtksNew = Vec_PtrAlloc( Au_ManNtkNum(p)+1 );
    Vec_IntForEachEntry( vOrder, Entry, i )
        Vec_PtrPush( vNtksNew, Au_ManNtk(p, Entry) );
    // invert order
    assert( Vec_IntEntry(vOrder, 1) == pRoot->Id );
    vOrder = Vec_IntInvert( vTemp = vOrder, 0 );
    Vec_IntFree( vTemp );
    assert( Vec_IntEntry(vOrder, 1) == pRoot->Id );
    // update model numbers
    Au_ManForEachNtk( p, pNtk, i )
    {
        pNtk->Id = Vec_IntEntry( vOrder, pNtk->Id );
        Au_NtkForEachBox( pNtk, pBox, k )
        {
            pBox->Func = Vec_IntEntry( vOrder, pBox->Func );
            assert( pBox->Func > 0 );
            Au_BoxForEachFanout( pBox, pFan, j )
                pFan->Func = pBox->Func;
        }
    }
    // update
    ABC_FREE( p->vNtks.pArray );
    p->vNtks.pArray = vNtksNew->pArray;
    vNtksNew->pArray = NULL;
    Vec_PtrFree( vNtksNew );
    // verify
    Au_ManForEachNtk( p, pNtk, i )
        Au_NtkForEachBox( pNtk, pBox, k )
        {
            pBoxModel = Au_ObjModel(pBox);
            if ( pBoxModel == NULL || pBoxModel == pNtk )
                continue;
            assert( !pBox->Func || pBox->Func >= (unsigned)pNtk->Id );
            assert( Au_ObjFaninNum(pBox) == Au_NtkPiNum(pBoxModel) );
            assert( Au_BoxFanoutNum(pBox) == Au_NtkPoNum(pBoxModel) );
        }
    Vec_IntFree( vOrder );
}
void Au_ManCountThings( Au_Man_t * p )
{
    Au_Ntk_t * pNtk, * pBoxModel;
    Au_Obj_t * pBox;
    int i, k;//, clk = Abc_Clock();
    Au_ManForEachNtkReverse( p, pNtk, i )
    {
        pNtk->nBoxes = Au_NtkBoxNum(pNtk);
        pNtk->nNodes = Au_NtkNodeNum(pNtk);
        pNtk->nPorts = Au_NtkPiNum(pNtk) + Au_NtkPoNum(pNtk);
        pNtk->nNodeAnds = Au_NtkNodeNumFunc( pNtk, 1 );
        pNtk->nNodeXors = Au_NtkNodeNumFunc( pNtk, 2 );
        pNtk->nNodeMuxs = Au_NtkNodeNumFunc( pNtk, 3 );
//        assert( pNtk->nNodes == pNtk->nNodeAnds + pNtk->nNodeXors + pNtk->nNodeMuxs );
//        printf( "adding %.0f nodes of model %s\n", pNtk->nNodes, Au_NtkName(pNtk) );
        Au_NtkForEachBox( pNtk, pBox, k )
        {
            pBoxModel = Au_ObjModel(pBox);
            if ( pBoxModel == NULL || pBoxModel == pNtk )
                continue;
            assert( Au_ObjFaninNum(pBox) == Au_NtkPiNum(pBoxModel) );
            assert( Au_BoxFanoutNum(pBox) == Au_NtkPoNum(pBoxModel) );
            assert( pBoxModel->Id > pNtk->Id );
            assert( pBoxModel->nPorts > 0 );
            pNtk->nBoxes += pBoxModel->nBoxes;
            pNtk->nNodes += pBoxModel->nNodes;
            pNtk->nPorts += pBoxModel->nPorts;
            pNtk->nNodeAnds += pBoxModel->nNodeAnds;
            pNtk->nNodeXors += pBoxModel->nNodeXors;
            pNtk->nNodeMuxs += pBoxModel->nNodeMuxs;
//            printf( "    adding %.0f nodes of model %s\n", pBoxModel->nNodes, Au_NtkName(pBoxModel) );
        }
//        printf( "total %.0f nodes in model %s\n", pNtk->nNodes, Au_NtkName(pNtk) );
    }
    pNtk = Au_ManNtkRoot(p);
    printf( "Total nodes = %15.0f. Total instances = %15.0f. Total ports = %15.0f.\n", 
//    printf( "Total nodes = %.2e. Total instances = %.2e. Total ports = %.2e.\n", 
        pNtk->nNodes, pNtk->nBoxes, pNtk->nPorts );
//    printf( "Total ANDs  = %15.0f. Total XORs      = %15.0f. Total MUXes = %15.0f.\n", 
//    printf( "Total ANDs  = %.2e. Total XORs      = %.2e. Total MUXes = %.2e.  ", 
//        pNtk->nNodeAnds, pNtk->nNodeXors, pNtk->nNodeMuxs );
    printf( "Total ANDs  = %15.0f.\n", pNtk->nNodeAnds );
    printf( "Total XORs  = %15.0f.\n", pNtk->nNodeXors );
    printf( "Total MUXes = %15.0f.\n", pNtk->nNodeMuxs );
//    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
}

int Au_NtkCompareNames( Au_Ntk_t ** p1, Au_Ntk_t ** p2 )
{
    return strcmp( Au_NtkName(*p1), Au_NtkName(*p2) );
}
void Au_ManPrintBoxInfo( Au_Ntk_t * pNtk )
{
    Vec_Ptr_t * vMods;
    Au_Ntk_t * pModel, * pBoxModel;
    Au_Obj_t * pObj;
    Vec_Int_t * vCounts;
    int i, k, Num;
    if ( pNtk->pMan == NULL )
    {
        printf( "There is no hierarchy information.\n" );
        return;
    }
    vMods = &pNtk->pMan->vNtks;

/*
    vMods->nSize--;
    vMods->pArray++;
    // sort models by name
    Vec_PtrSort( vMods, (int(*)())Au_NtkCompareNames );
    // swap the first model
    Num = Vec_PtrFind( vMods, pNtk );
    assert( Num >= 0 && Num < Vec_PtrSize(vMods) );
    pBoxModel = (Au_Ntk_t *)Vec_PtrEntry(vMods, 0);
    Vec_PtrWriteEntry(vMods, 0, (Au_Ntk_t *)Vec_PtrEntry(vMods, Num) );
    Vec_PtrWriteEntry(vMods, Num, pBoxModel );
    vMods->pArray--;
    vMods->nSize++;
*/

//    Vec_PtrForEachEntry( Au_Ntk_t *, vMods, pModel, i )
//        printf( "%s\n", Au_NtkName(pModel) );

    // print models
    vCounts = Vec_IntStart( Vec_PtrSize(vMods) );
    Vec_PtrForEachEntryStart( Au_Ntk_t *, vMods, pModel, i, 1 )
    {
        if ( Au_NtkBoxNum(pModel) == 0 )
            continue;
        Vec_IntFill( vCounts, Vec_IntSize(vCounts), 0 );
        Au_NtkForEachBox( pModel, pObj, k )
        {
            pBoxModel = Au_ObjModel(pObj);
            if ( pBoxModel == NULL || pBoxModel == pModel )
                continue;
            Num = Vec_PtrFind( vMods, pBoxModel );
            assert( Num >= 0 && Num < Vec_PtrSize(vMods) );
            Vec_IntAddToEntry( vCounts, Num, 1 );
        }

//        Au_NtkPrintStats( pModel, 0, 0, 0, 0, 0, 0, 0 );
        printf( "MODULE  " );
        printf( "%-30s : ", Au_NtkName(pModel) );
        printf( "PI=%6d ", Au_NtkPiNum(pModel) );
        printf( "PO=%6d ", Au_NtkPoNum(pModel) );
        printf( "BB=%6d ", Au_NtkBoxNum(pModel) );
        printf( "ND=%6d ", Au_NtkNodeNum(pModel) ); // sans constants
//        printf( "Lev=%5d ", Au_NtkLevel(pModel) );
        printf( "\n" );

        Vec_IntForEachEntry( vCounts, Num, k )
            if ( Num )
                printf( "%15d : %s\n", Num, Au_NtkName((Au_Ntk_t *)Vec_PtrEntry(vMods, k)) );
    }
    Vec_IntFree( vCounts );
    Vec_PtrForEachEntryStart( Au_Ntk_t *, vMods, pModel, i, 1 )
    {
        if ( Au_NtkBoxNum(pModel) != 0 )
            continue;
        printf( "MODULE  " );
        printf( "%-30s : ", Au_NtkName(pModel) );
        printf( "PI=%6d ", Au_NtkPiNum(pModel) );
        printf( "PO=%6d ", Au_NtkPoNum(pModel) );
        printf( "BB=%6d ", Au_NtkBoxNum(pModel) );
        printf( "ND=%6d ", Au_NtkNodeNum(pModel) );
//        printf( "Lev=%5d ", Au_NtkLevel(pModel) );
        printf( "\n" );
    }
}
int Au_NtkCompareSign( Au_Ntk_t ** p1, Au_Ntk_t ** p2 )
{
    if ( Au_NtkPiNum(*p1) - Au_NtkPiNum(*p2) != 0 )
        return Au_NtkPiNum(*p1) - Au_NtkPiNum(*p2);
    else
        return Au_NtkPoNum(*p1) - Au_NtkPoNum(*p2);
}
void Au_ManPrintBoxInfoSorted( Au_Ntk_t * pNtk )
{
    Vec_Ptr_t * vMods, * vModsNew;
    Au_Ntk_t * pModel;
    int i;
    if ( pNtk->pMan == NULL )
    {
        printf( "There is no hierarchy information.\n" );
        return;
    }
    vMods = &pNtk->pMan->vNtks;

    vMods->nSize--;
    vMods->pArray++;
    vModsNew = Vec_PtrDup( vMods );
    vMods->pArray--;
    vMods->nSize++;

    Vec_PtrSort( vModsNew, (int(*)())Au_NtkCompareSign );
    Vec_PtrForEachEntryStart( Au_Ntk_t *, vModsNew, pModel, i, 1 )
    {
        printf( "MODULE  " );
        printf( "%-30s : ", Au_NtkName(pModel) );
        printf( "PI=%6d ", Au_NtkPiNum(pModel) );
        printf( "PO=%6d ", Au_NtkPoNum(pModel) );
        printf( "BB=%6d ", Au_NtkBoxNum(pModel) );
        printf( "ND=%6d ", Au_NtkNodeNum(pModel) );
        printf( "\n" );
    }
    Vec_PtrFree( vModsNew );
}

int Au_NtkCheckRecursive( Au_Ntk_t * pNtk )
{
    Vec_Ptr_t * vMods;
    Au_Ntk_t * pModel;
    Au_Obj_t * pObj;
    int i, k, RetValue = 0;

    if ( pNtk->pMan == NULL )
    {
        printf( "There is no hierarchy information.\n" );
        return RetValue;
    }

    vMods = &pNtk->pMan->vNtks;
    Vec_PtrForEachEntryStart( Au_Ntk_t *, vMods, pModel, i, 1 )
    {
        Au_NtkForEachObj( pModel, pObj, k )
            if ( Au_ObjIsBox(pObj) && Au_ObjModel(pObj) == pModel )
            {
                printf( "WARNING: Model \"%s\" contains a recursive definition.\n", Au_NtkName(pModel) );
                RetValue = 1;
                break;
            }
    }
    return RetValue;
}

// count the number of support variables
int Au_ObjSuppSize_rec( Au_Ntk_t * p, int Id )
{
    Au_Obj_t * pObj;
    int i, iFanin, Counter = 0;
    if ( Au_ObjIsTravIdCurrentId(p, Id) )
        return 0;
    Au_ObjSetTravIdCurrentId(p, Id);
    pObj = Au_NtkObj( p, Id );
    if ( Au_ObjIsPi(pObj) )
        return 1;
    assert( Au_ObjIsNode(pObj) || Au_ObjIsBox(pObj) || Au_ObjIsFan(pObj) );
    Au_ObjForEachFaninId( pObj, iFanin, i )
        Counter += Au_ObjSuppSize_rec( p, iFanin );
    return Counter;
}
int Au_ObjSuppSize( Au_Obj_t * pObj )
{
    Au_Ntk_t * p = Au_ObjNtk(pObj);
    Au_NtkIncrementTravId( p );
    return Au_ObjSuppSize_rec( p, Au_ObjId(pObj) );
}
/*
// this version is 50% slower than above
int Au_ObjSuppSize_rec( Au_Obj_t * pObj )
{
    Au_Obj_t * pFanin;
    int i, Counter = 0;
    if ( Au_ObjIsTravIdCurrent(pObj) )
        return 0;
    Au_ObjSetTravIdCurrent(pObj);
    if ( Au_ObjIsPi(pObj) )
        return 1;
    assert( Au_ObjIsNode(pObj) || Au_ObjIsBox(pObj) || Au_ObjIsFan(pObj) );
    Au_ObjForEachFanin( pObj, pFanin, i )
        Counter += Au_ObjSuppSize_rec( pFanin );
    return Counter;
}
int Au_ObjSuppSize( Au_Obj_t * pObj )
{
    Au_NtkIncrementTravId( Au_ObjNtk(pObj) );
    return Au_ObjSuppSize_rec( pObj );
}
*/
int Au_NtkSuppSizeTest( Au_Ntk_t * p )
{
    Au_Obj_t * pObj;
    int i, Counter = 0;
    Au_NtkForEachObj( p, pObj, i )
        if ( Au_ObjIsNode(pObj) )
            Counter += (Au_ObjSuppSize(pObj) <= 16);
    printf( "Nodes with small support %d (out of %d)\n", Counter, Au_NtkNodeNum(p) );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Returns memory for the next object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Au_NtkInsertHeader( Au_Ntk_t * p )
{
    Au_Obj_t * pMem = (Au_Obj_t *)Vec_PtrEntryLast( &p->vPages );
    assert( (((ABC_PTRINT_T)(pMem + p->iHandle) & 0x3FF) >> 4) == 0 );
    ((Au_Ntk_t **)(pMem + p->iHandle))[0] = p;
    ((int *)(pMem + p->iHandle))[2] = ((Vec_PtrSize(&p->vPages) - 1) << 12) | (p->iHandle & 0xFC0);
    p->iHandle++;
}
int Au_NtkAllocObj( Au_Ntk_t * p, int nFanins, int Type )
{
    Au_Obj_t * pMem, * pObj, * pTemp;
    int nObjInt = ((2+nFanins) >> 2) + (((2+nFanins) & 3) > 0);
    int Id, nObjIntReal = nObjInt;
    if ( nObjInt > 63 )
        nObjInt = 63 + 64 * (((nObjInt-63) >> 6) + (((nObjInt-63) & 63) > 0));
    if ( Vec_PtrSize(&p->vPages) == 0 || p->iHandle + nObjInt > (1 << 12) )
    {
        if ( nObjInt + 64 > (1 << 12) )
            pMem = ABC_CALLOC( Au_Obj_t, nObjInt + 64 ), p->nObjsAlloc += nObjInt + 64;
        else
            pMem = ABC_CALLOC( Au_Obj_t, (1 << 12) + 64 ), p->nObjsAlloc += (1 << 12) + 64;
        Vec_PtrPush( p->vChunks, pMem );
        if ( ((ABC_PTRINT_T)pMem & 0xF) )
            pMem = (Au_Obj_t *)((char *)pMem + 16 - ((ABC_PTRINT_T)pMem & 0xF));
        assert( ((ABC_PTRINT_T)pMem & 0xF) == 0 );
        p->iHandle = (((ABC_PTRINT_T)pMem & 0x3FF) >> 4);
        if ( p->iHandle )
        {
            pMem += 64 - (p->iHandle & 63);
            p->iHandle = 0; 
        }
        Vec_PtrPush( &p->vPages, pMem );
        Au_NtkInsertHeader( p );
    }
    else
    {
        pMem = (Au_Obj_t *)Vec_PtrEntryLast( &p->vPages );
        if ( (p->iHandle & 63) == 0 || nObjInt > (64 - (p->iHandle & 63)) )
        {
            if ( p->iHandle & 63 )
                p->iHandle += 64 - (p->iHandle & 63); 
            Au_NtkInsertHeader( p );
        }
        if ( p->iHandle + nObjInt > (1 << 12) )
            return Au_NtkAllocObj( p, nFanins, Type );
    }
    pObj = pMem + p->iHandle;
    assert( *((int *)pObj) == 0 );
    pObj->nFanins = nFanins;
    p->nObjs[pObj->Type = Type]++;
    if ( Type == AU_OBJ_PI )
    {
        Au_ObjSetFaninLit( pObj, 0, Vec_IntSize(&p->vPis) );
        Vec_IntPush( &p->vPis, Au_ObjId(pObj) );
    }
    else if ( Type == AU_OBJ_PO )
    {
        Au_ObjSetFaninLit( pObj, 1, Vec_IntSize(&p->vPos) );
        Vec_IntPush( &p->vPos, Au_ObjId(pObj) );
    }
    p->iHandle += nObjInt;
    p->nObjsUsed += nObjIntReal;

    Id = Au_ObjId(pObj);
    Vec_IntPush( &p->vObjs, Id );
    pTemp = Au_NtkObj( p, Id );
    assert( pTemp == pObj );
    return Id;
}
int Au_NtkCreateConst0( Au_Ntk_t * pNtk )
{
    return Au_NtkAllocObj( pNtk, 0, AU_OBJ_CONST0 );
}
int Au_NtkCreatePi( Au_Ntk_t * pNtk )
{
    return Au_NtkAllocObj( pNtk, 0, AU_OBJ_PI );
}
int Au_NtkCreatePo( Au_Ntk_t * pNtk, int iFanin )
{
    int Id = Au_NtkAllocObj( pNtk, 1, AU_OBJ_PO );
    if ( iFanin )
        Au_ObjSetFaninLit( Au_NtkObj(pNtk, Id), 0, iFanin );
    return Id;
}
int Au_NtkCreateFan( Au_Ntk_t * pNtk, int iFanin, int iFanout, int iModel )
{
    int Id = Au_NtkAllocObj( pNtk, 1, AU_OBJ_FAN );
    Au_Obj_t * p = Au_NtkObj( pNtk, Id );
    if ( iFanin )
        Au_ObjSetFaninLit( p, 0, iFanin );
    Au_ObjSetFaninLit( p, 1, iFanout );
    p->Func = iModel;
    return Id;
}
int Au_NtkCreateNode( Au_Ntk_t * pNtk, Vec_Int_t * vFanins, int iFunc )
{
    int i, iFanin;
    int Id = Au_NtkAllocObj( pNtk, Vec_IntSize(vFanins), AU_OBJ_NODE );
    Au_Obj_t * p = Au_NtkObj( pNtk, Id );
    Vec_IntForEachEntry( vFanins, iFanin, i )
        Au_ObjSetFaninLit( p, i, iFanin );
    p->Func = iFunc;
    return Id;
}
int Au_NtkCreateBox( Au_Ntk_t * pNtk, Vec_Int_t * vFanins, int nFanouts, int iModel )
{
    int i, iFanin, nFanins = Vec_IntSize(vFanins);
    int Id = Au_NtkAllocObj( pNtk, nFanins + 1 + nFanouts, AU_OBJ_BOX );
    Au_Obj_t * p = Au_NtkObj( pNtk, Id );
    Vec_IntForEachEntry( vFanins, iFanin, i )
        Au_ObjSetFaninLit( p, i, iFanin );
    Au_ObjSetFaninLit( p, nFanins, nFanouts );
    for ( i = 0; i < nFanouts; i++ )
        Au_ObjSetFaninLit( p, nFanins + 1 + i, Au_NtkCreateFan(pNtk, Au_Var2Lit(Id,0), i, iModel) );
    p->nFanins = nFanins;
    p->Func = iModel;
    assert( iModel > 0 );
    return Id;
}

/*
 * 0/1 would denote false/true respectively.
 * Signals would be even numbers, and negation would be handled by xor with 1.
 * The output signal for each gate or subckt could be implicitly generated just use the next signal number.
 * For ranges, we could use "start:cnt" to denote the sequence "start, start+2, ..., start + 2*(cnt- 1)".
    - "cnt" seems more intuitive when signals are restricted to even numbers.
 * We'd have subckts and specialized gates .and, .xor, and .mux.

Here is a small example:

.model test
.inputs 3 # Inputs 2 4 6
.subckt and3 3 1 2:3 # 8 is implicit output
.outputs 1 8
.end

.model and3
.inputs 3 # Inputs 2 4 6
.and 2 4 # 8 output
.and 6 8 # 10 output
.outputs 1 10
.end
*/

/**Function*************************************************************

  Synopsis    [Reads one entry.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Au_NtkRemapNum( Vec_Int_t * vNum2Obj, int Num )
{
    return Au_Var2Lit(Vec_IntEntry(vNum2Obj, Au_Lit2Var(Num)), Au_LitIsCompl(Num));
}
/**Function*************************************************************

  Synopsis    [Reads one entry.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Au_NtkParseCBlifNum( Vec_Int_t * vFanins, char * pToken, Vec_Int_t * vNum2Obj )
{
    char * pCur;
    int Num1, Num2, i;
    assert( pToken[0] >= '0' && pToken[0] <= '9' );
    Num1 = atoi( pToken );
    for ( pCur = pToken; *pCur; pCur++ )
        if ( *pCur == ':' )
        {
            Num2 = atoi( pCur+1 );
            for ( i = 0; i < Num2; i++ )
                Vec_IntPush( vFanins, Au_NtkRemapNum(vNum2Obj, Num1 + i + i) );
            return;
        }
        else if ( *pCur == '*' )
        {
            Num2 = atoi( pCur+1 );
            for ( i = 0; i < Num2; i++ )
                Vec_IntPush( vFanins, Au_NtkRemapNum(vNum2Obj, Num1) );
            return;
        }
    assert( *pCur == 0 );
    Vec_IntPush( vFanins, Au_NtkRemapNum(vNum2Obj, Num1) );
}

/**Function*************************************************************

  Synopsis    [Parses CBLIF file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Ntk_t * Au_NtkParseCBlif( char * pFileName )
{
    FILE * pFile;
    Au_Man_t * pMan;
    Au_Ntk_t * pRoot = NULL;
    Au_Obj_t * pBox, * pFan;
    char * pBuffer, * pCur;
    Vec_Int_t * vLines, * vNum2Obj, * vFanins;
    int i, k, j, Id, nInputs, nOutputs;
    int Line, Num, Func;
    // read the file
    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\".\n", pFileName );
        return NULL;
    }
    pBuffer = Extra_FileRead( pFile );
    fclose( pFile );
    // split into lines
    vLines = Vec_IntAlloc( 1000 );
    Vec_IntPush( vLines, 0 );
    for ( pCur = pBuffer; *pCur; pCur++ )
        if ( *pCur == '\n' )
        {
            *pCur = 0;
            Vec_IntPush( vLines, pCur - pBuffer + 1 );
        }
    // start the manager
    pMan = Au_ManAlloc( pFileName );
    // parse the lines
    vNum2Obj = Vec_IntAlloc( 1000 );
    vFanins = Vec_IntAlloc( 1000 );
    Vec_IntForEachEntry( vLines, Line, i )
    {
        pCur = strtok( pBuffer + Line, " \t\r" );
        if ( pCur == NULL || *pCur == '#' )
            continue;
        if ( *pCur != '.' )
        {
            printf( "Cannot read directive in line %d: \"%s\".\n", i, pBuffer + Line );
            continue;
        }
        Vec_IntClear( vFanins );
        if ( !strcmp(pCur, ".and") )
        {
            for ( k = 0; k < 2; k++ )
            {
                pCur = strtok( NULL, " \t\r" );
                Num  = atoi( pCur );
                Vec_IntPush( vFanins, Au_NtkRemapNum(vNum2Obj, Num) );
            }
            Id = Au_NtkCreateNode( pRoot, vFanins, 1 );
            Vec_IntPush( vNum2Obj, Id );
        }
        else if ( !strcmp(pCur, ".xor") )
        {
            for ( k = 0; k < 2; k++ )
            {
                pCur = strtok( NULL, " \t\r" );
                Num  = atoi( pCur );
                Vec_IntPush( vFanins, Au_NtkRemapNum(vNum2Obj, Num) );
            }
            Id = Au_NtkCreateNode( pRoot, vFanins, 2 );
            Vec_IntPush( vNum2Obj, Id );
        }
        else if ( !strcmp(pCur, ".mux") )
        {
            for ( k = 0; k < 3; k++ )
            {
                pCur = strtok( NULL, " \t\r" );
                Num  = atoi( pCur );
                Vec_IntPush( vFanins, Au_NtkRemapNum(vNum2Obj, Num) );
            }
            Id = Au_NtkCreateNode( pRoot, vFanins, 3 );
            Vec_IntPush( vNum2Obj, Id );
        }
        else if ( !strcmp(pCur, ".subckt") )
        {
            pCur = strtok( NULL, " \t\r" );
            Func = pCur - pBuffer;
            pCur = strtok( NULL, " \t\r" );
            nInputs = atoi( pCur );
            pCur = strtok( NULL, " \t\r" );
            nOutputs = atoi( pCur );
            while ( 1 )
            {
                pCur = strtok( NULL, " \t\r" );
                if ( pCur == NULL || *pCur == '#' )
                    break;
                Au_NtkParseCBlifNum( vFanins, pCur, vNum2Obj );
            }
            assert( Vec_IntSize(vFanins) == nInputs );
            Id = Au_NtkCreateBox( pRoot, vFanins, nOutputs, Func );
            pBox = Au_NtkObj( pRoot, Id );
            Au_BoxForEachFanoutId( pBox, Num, k )
                Vec_IntPush( vNum2Obj, Num );
        }
        else if ( !strcmp(pCur, ".model") )
        {
            pCur  = strtok( NULL, " \t\r" );
            pRoot = Au_NtkAlloc( pMan, pCur );
            Id    = Au_NtkCreateConst0( pRoot );
            Vec_IntClear( vNum2Obj );
            Vec_IntPush( vNum2Obj, Id );
        }
        else if ( !strcmp(pCur, ".inputs") )
        {
            pCur = strtok( NULL, " \t\r" );
            Num  = atoi( pCur );
            for ( k = 0; k < Num; k++ )
                Vec_IntPush( vNum2Obj, Au_NtkCreatePi(pRoot) );
        }
        else if ( !strcmp(pCur, ".outputs") )
        {
            pCur = strtok( NULL, " \t\r" );
            nOutputs = atoi( pCur );
            while ( 1 )
            {
                pCur = strtok( NULL, " \t\r" );
                if ( pCur == NULL || *pCur == '#' )
                    break; 
                Au_NtkParseCBlifNum( vFanins, pCur, vNum2Obj );
            }
            assert( Vec_IntSize(vFanins) == nOutputs );
            Vec_IntForEachEntry( vFanins, Num, k )
                Au_NtkCreatePo( pRoot, Num );
        }
        else if ( strcmp(pCur, ".end") )
            printf( "Unknown directive in line %d: \"%s\".\n", i, pBuffer + Line );
    }
    Vec_IntFree( vFanins );
    Vec_IntFree( vNum2Obj );
    Vec_IntFree( vLines );
    // set pointers to models
    Au_ManForEachNtk( pMan, pRoot, i )
        Au_NtkForEachBox( pRoot, pBox, k )
        {
            pBox->Func = Au_ManFindNtk( pMan, pBuffer + pBox->Func );
            assert( pBox->Func > 0 );
            Au_BoxForEachFanout( pBox, pFan, j )
                pFan->Func = pBox->Func;
        }
    ABC_FREE( pBuffer );
    // order models in topological order
    pRoot = Au_ManNtkRoot( pMan );
    Au_ManReorderModels( pMan, pRoot );
    return pRoot;
}

ABC_NAMESPACE_IMPL_END

#include "abc.h"
#include "aig/gia/gia.h"

ABC_NAMESPACE_IMPL_START

extern Vec_Ptr_t * Abc_NtkDfsBoxes( Abc_Ntk_t * pNtk );
extern int Abc_NtkDeriveFlatGiaSop( Gia_Man_t * pGia, int * gFanins, char * pSop );
extern int Abc_NtkCheckRecursive( Abc_Ntk_t * pNtk );

/**Function*************************************************************

  Synopsis    [Flattens the logic hierarchy of the netlist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_NtkDeriveFlatGia_rec( Gia_Man_t * pGia, Au_Ntk_t * p )
{ 
    Au_Obj_t * pObj, * pTerm;
    int i, k, Lit = 0;
    Au_NtkForEachPi( p, pTerm, i )
        assert( Au_ObjCopy(pTerm) >= 0 );
    if ( strcmp(Au_NtkName(p), "ref_egcd") == 0 )
    {
        printf( "Replacing one instance of recursive model \"%s\" by a black box.\n", "ref_egcd" );
        Au_NtkForEachPo( p, pTerm, i )
            Au_ObjSetCopy( pTerm, Gia_ManAppendCi(pGia) );
        return;
    }
    Au_NtkForEachObj( p, pObj, i )
    {
        if ( Au_ObjIsNode(pObj) )
        {
            if ( p->pMan->pFuncs )
            {
                int gFanins[16];
                char * pSop = Abc_NamStr( p->pMan->pFuncs, pObj->Func );
                assert( Au_ObjFaninNum(pObj) <= 16 );
                assert( Au_ObjFaninNum(pObj) == Abc_SopGetVarNum(pSop) );
                Au_ObjForEachFanin( pObj, pTerm, k )
                {
                    gFanins[k] = Au_ObjCopy(pTerm);
                    assert( gFanins[k] >= 0 );
                }
                Lit = Abc_NtkDeriveFlatGiaSop( pGia, gFanins, pSop );
            }
            else
            {
                int Lit0, Lit1, Lit2;
                assert( pObj->Func >= 1 && pObj->Func <= 3 );
                Lit0 = Abc_LitNotCond( Au_ObjCopy(Au_ObjFanin0(pObj)), Au_ObjFaninC0(pObj) );
                Lit1 = Abc_LitNotCond( Au_ObjCopy(Au_ObjFanin1(pObj)), Au_ObjFaninC1(pObj) );
                if ( pObj->Func == 1 )
                    Lit = Gia_ManHashAnd( pGia, Lit0, Lit1 );
                else if ( pObj->Func == 2 )
                    Lit = Gia_ManHashXor( pGia, Lit0, Lit1 );
                else if ( pObj->Func == 3 )
                {
                    Lit2 = Abc_LitNotCond( Au_ObjCopy(Au_ObjFanin2(pObj)), Au_ObjFaninC2(pObj) );
                    Lit = Gia_ManHashMux( pGia, Lit0, Lit1, Lit2 );
                }
                else assert( 0 ); 
            } 
            assert( Lit >= 0 );
            Au_ObjSetCopy( pObj, Lit );
        }
        else if ( Au_ObjIsBox(pObj) )
        {
            Au_Ntk_t * pModel = Au_ObjModel(pObj);
            Au_NtkCleanCopy( pModel );
            // check the match between the number of actual and formal parameters
            assert( Au_ObjFaninNum(pObj) == Au_NtkPiNum(pModel) );
            assert( Au_BoxFanoutNum(pObj) == Au_NtkPoNum(pModel) );
            // assign PIs
            Au_ObjForEachFanin( pObj, pTerm, k )
                Au_ObjSetCopy( Au_NtkPi(pModel, k), Au_ObjCopy(pTerm) );
            // call recursively
            Au_NtkDeriveFlatGia_rec( pGia, pModel );
            // assign POs
            Au_BoxForEachFanout( pObj, pTerm, k )
                Au_ObjSetCopy( pTerm, Au_ObjCopy(Au_NtkPo(pModel, k)) );
        }
        else if ( Au_ObjIsConst0(pObj) )
            Au_ObjSetCopy( pObj, 0 );
            
    }
    Au_NtkForEachPo( p, pTerm, i )
    {
        Lit = Abc_LitNotCond( Au_ObjCopy(Au_ObjFanin0(pTerm)), Au_ObjFaninC0(pTerm) );
        Au_ObjSetCopy( pTerm, Lit );
    }
    Au_NtkForEachPo( p, pTerm, i )
        assert( Au_ObjCopy(pTerm) >= 0 );
//    p->pMan->nGiaObjMax = Abc_MaxInt( p->pMan->nGiaObjMax, Gia_ManObjNum(pGia) );
}

/**Function*************************************************************

  Synopsis    [Flattens the logic hierarchy of the netlist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Au_NtkDeriveFlatGia( Au_Ntk_t * p )
{
    Gia_Man_t * pTemp, * pGia = NULL;
    Au_Obj_t * pTerm;
    int i;
    printf( "Collapsing model \"%s\"...\n", Au_NtkName(p) );
    Au_NtkCleanCopy( p );
    // start the network
    pGia = Gia_ManStart( (1<<16) );
    pGia->pName = Abc_UtilStrsav( Au_NtkName(p) );
//    pGia->pSpec = Abc_UtilStrsav( Au_NtkSpec(p) );
    Gia_ManHashAlloc( pGia );
    Gia_ManFlipVerbose( pGia );
    // create PIs
    Au_NtkForEachPi( p, pTerm, i )
        Au_ObjSetCopy( pTerm, Gia_ManAppendCi(pGia) );
    // recursively flatten hierarchy
    Au_NtkDeriveFlatGia_rec( pGia, p );
    // create POs
    Au_NtkForEachPo( p, pTerm, i )
        Gia_ManAppendCo( pGia, Au_ObjCopy(pTerm) );
    // prepare return value
//    Gia_ManHashProfile( pGia );
    Gia_ManHashStop( pGia );
    Gia_ManSetRegNum( pGia, 0 );
    pGia = Gia_ManCleanup( pTemp = pGia );
    Gia_ManStop( pTemp );
    return pGia;
}


// ternary simulation
#define AU_VAL0   1
#define AU_VAL1   2
#define AU_VALX   3

static inline void Au_ObjSetXsim( Au_Obj_t * pObj, int Value )  { pObj->Value = Value;  }
static inline int  Au_ObjGetXsim( Au_Obj_t * pObj )             { return pObj->Value;   }
static inline int  Au_XsimInv( int Value )   
{ 
    if ( Value == AU_VAL0 )
        return AU_VAL1;
    if ( Value == AU_VAL1 )
        return AU_VAL0;
    assert( Value == AU_VALX );       
    return AU_VALX;
}
static inline int  Au_XsimAnd( int Value0, int Value1 )   
{ 
    if ( Value0 == AU_VAL0 || Value1 == AU_VAL0 )
        return AU_VAL0;
    if ( Value0 == AU_VALX || Value1 == AU_VALX )
        return AU_VALX;
    assert( Value0 == AU_VAL1 && Value1 == AU_VAL1 );
    return AU_VAL1;
}
static inline int  Au_XsimXor( int Value0, int Value1 )   
{ 
    if ( Value0 == AU_VALX || Value1 == AU_VALX )
        return AU_VALX;
    if ( (Value0 == AU_VAL0) == (Value1 == AU_VAL0) )
        return AU_VAL0;
    return AU_VAL1;
}
static inline int  Au_XsimMux( int ValueC, int Value1, int Value0 )   
{ 
    if ( ValueC == AU_VAL0 )
        return Value0;
    if ( ValueC == AU_VAL1 )
        return Value1;
    if ( Value0 == AU_VAL0 && Value1 == AU_VAL0 )
        return AU_VAL0;
    if ( Value0 == AU_VAL1 && Value1 == AU_VAL1 )
        return AU_VAL1;
    return AU_VALX;
}
static inline int  Au_ObjGetXsimFan0( Au_Obj_t * pObj )       
{ 
    int Value = Au_ObjGetXsim( Au_ObjFanin0(pObj) );
    return Au_ObjFaninC0(pObj) ? Au_XsimInv(Value) : Value;
}
static inline int  Au_ObjGetXsimFan1( Au_Obj_t * pObj )       
{ 
    int Value = Au_ObjGetXsim( Au_ObjFanin1(pObj) );
    return Au_ObjFaninC1(pObj) ? Au_XsimInv(Value) : Value;
}
static inline int  Au_ObjGetXsimFan2( Au_Obj_t * pObj )       
{ 
    int Value = Au_ObjGetXsim( Au_ObjFanin2(pObj) );
    return Au_ObjFaninC2(pObj) ? Au_XsimInv(Value) : Value;
}

/**Function*************************************************************

  Synopsis    [Flattens the logic hierarchy of the netlist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_NtkTerSimulate_rec( Au_Ntk_t * p )
{ 
    Au_Obj_t * pObj = NULL, * pTerm;
    int i, k;
    Au_NtkForEachPi( p, pTerm, i )
    {
        assert( Au_ObjGetXsim(pTerm) > 0 );
        if ( Au_ObjGetXsim(pTerm) == AU_VALX )
            p->pMan->nPortsNC++;
        else if ( Au_ObjGetXsim(pTerm) == AU_VAL0 )
            p->pMan->nPortsC0++;
        else
            p->pMan->nPortsC1++;
    }
    if ( strcmp(Au_NtkName(p), "ref_egcd") == 0 )
    {
        printf( "Replacing one instance of recursive model \"%s\" by a black box.\n", "ref_egcd" );
        Au_NtkForEachPo( p, pTerm, i )
            Au_ObjSetXsim( pTerm, AU_VALX );
        return;
    }
    Au_NtkForEachObj( p, pObj, i )
    {
        if ( Au_ObjIsNode(pObj) )
        {
            if ( pObj->Func == 1 )
                Au_ObjSetXsim( pObj, Au_XsimAnd(Au_ObjGetXsimFan0(pObj), Au_ObjGetXsimFan1(pObj)) );
            else if ( pObj->Func == 2 )
                Au_ObjSetXsim( pObj, Au_XsimXor(Au_ObjGetXsimFan0(pObj), Au_ObjGetXsimFan1(pObj)) );
            else if ( pObj->Func == 3 )
                Au_ObjSetXsim( pObj, Au_XsimMux(Au_ObjGetXsimFan0(pObj), Au_ObjGetXsimFan1(pObj), Au_ObjGetXsimFan2(pObj)) );
            else assert( 0 );
        }
        else if ( Au_ObjIsBox(pObj) )
        {
            Au_Ntk_t * pModel = Au_ObjModel(pObj);
            // check the match between the number of actual and formal parameters
            assert( Au_ObjFaninNum(pObj) == Au_NtkPiNum(pModel) );
            assert( Au_BoxFanoutNum(pObj) == Au_NtkPoNum(pModel) );
            // assign PIs
            Au_ObjForEachFanin( pObj, pTerm, k )
                Au_ObjSetXsim( Au_NtkPi(pModel, k), Au_ObjGetXsim(pTerm) );
            // call recursively
            Au_NtkTerSimulate_rec( pModel );
            // assign POs
            Au_BoxForEachFanout( pObj, pTerm, k )
                Au_ObjSetXsim( pTerm, Au_ObjGetXsim(Au_NtkPo(pModel, k)) );
        }
        else if ( Au_ObjIsConst0(pObj) )
            Au_ObjSetXsim( pObj, AU_VAL0 );
            
    }
    Au_NtkForEachPo( p, pTerm, i )
        Au_ObjSetXsim( pTerm, Au_ObjGetXsimFan0(pObj) );
    Au_NtkForEachPo( p, pTerm, i )
    {
        assert( Au_ObjGetXsim(pTerm) > 0 );
        if ( Au_ObjGetXsim(pTerm) == AU_VALX )
            p->pMan->nPortsNC++;
        else if ( Au_ObjGetXsim(pTerm) == AU_VAL0 )
            p->pMan->nPortsC0++;
        else
            p->pMan->nPortsC1++;
    }
}

/**Function*************************************************************

  Synopsis    [Flattens the logic hierarchy of the netlist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_NtkTerSimulate( Au_Ntk_t * p )
{
    Au_Obj_t * pTerm;
    int i, Counter[2] = {0};
    assert( p->pMan->pFuncs == NULL );
    printf( "Collapsing model \"%s\"...\n", Au_NtkName(p) );
    // create PIs
    Au_NtkForEachPi( p, pTerm, i )
        Au_ObjSetXsim( pTerm, AU_VALX );
    // recursively flatten hierarchy
    p->pMan->nPortsC0 = 0;
    p->pMan->nPortsC1 = 0;
    p->pMan->nPortsNC = 0;
    Au_NtkTerSimulate_rec( p );
    // analyze outputs
    Au_NtkForEachPo( p, pTerm, i )
        if ( Au_ObjGetXsim(pTerm) == AU_VAL0 )
            Counter[0]++;
        else if ( Au_ObjGetXsim(pTerm) == AU_VAL1 )
            Counter[1]++;
    // print results
    printf( "Const0 outputs =%15d. Const1 outputs =%15d.  Total outputs =%15d.\n", 
        Counter[0], Counter[1], Au_NtkPoNum(p) );
    printf( "Const0 ports =  %.0f. Const1  ports =  %.0f. Non-const ports=  %.0f.  Total ports =  %.0f.\n", 
        p->pMan->nPortsC0, p->pMan->nPortsC1, p->pMan->nPortsNC, p->pMan->nPortsC0 + p->pMan->nPortsC1 + p->pMan->nPortsNC );
}


/**Function*************************************************************

  Synopsis    [Duplicates ABC network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Ntk_t * Au_NtkDerive( Au_Man_t * pMan, Abc_Ntk_t * pNtk, Vec_Ptr_t * vOrder )
{
    Au_Ntk_t * p;
    Au_Obj_t * pAuObj;
    Abc_Obj_t * pObj, * pTerm;
//    Vec_Ptr_t * vOrder;
    Vec_Int_t * vFanins;
    int i, k, iFunc;
    assert( Abc_NtkIsNetlist(pNtk) );
    Abc_NtkCleanCopy( pNtk );
    p = Au_NtkAlloc( pMan, Abc_NtkName(pNtk) );
    // copy PIs
    Abc_NtkForEachPi( pNtk, pTerm, i )
        Abc_ObjFanout0(pTerm)->iTemp = Au_NtkCreatePi(p);
    // copy nodes and boxes
    vFanins = Vec_IntAlloc( 100 );
//    vOrder = Abc_NtkDfsBoxes( pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, vOrder, pObj, i )
    {
        Vec_IntClear( vFanins );
        if ( Abc_ObjIsNode(pObj) )
        {
            Abc_ObjForEachFanin( pObj, pTerm, k )
                Vec_IntPush( vFanins, Au_Var2Lit(pTerm->iTemp, 0) );
            iFunc = Abc_NamStrFindOrAdd( pMan->pFuncs, (char *)pObj->pData, NULL );
            Abc_ObjFanout0(pObj)->iTemp = Au_NtkCreateNode(p, vFanins, iFunc);
            continue;
        }
        assert( Abc_ObjIsBox(pObj) );
        Abc_ObjForEachFanin( pObj, pTerm, k )
            Vec_IntPush( vFanins, Au_Var2Lit(Abc_ObjFanin0(pTerm)->iTemp, 0) );
        pObj->iTemp = Au_NtkCreateBox(p, vFanins, Abc_ObjFanoutNum(pObj), ((Abc_Ntk_t *)pObj->pData)->iStep );
        pAuObj = Au_NtkObj(p, pObj->iTemp);
        Abc_ObjForEachFanout( pObj, pTerm, k )
            Abc_ObjFanout0(pTerm)->iTemp = Au_ObjFanout(pAuObj, k);
    }
//    Vec_PtrFree( vOrder );
    Vec_IntFree( vFanins );
    // copy POs
    Abc_NtkForEachPo( pNtk, pTerm, i )
        Au_NtkCreatePo( p, Au_Var2Lit(Abc_ObjFanin0(pTerm)->iTemp, 0) );
//    Au_NtkPrintStats( p );
    return p;
}

Gia_Man_t * Au_ManDeriveTest( Abc_Ntk_t * pRoot )
{
    extern Vec_Ptr_t * Abc_NtkCollectHie( Abc_Ntk_t * pNtk );

//    char * pModelName = NULL;
    char * pModelName = "path_0_r_x_lhs";
    Gia_Man_t * pGia = NULL;
    Vec_Ptr_t * vOrder, * vModels;
    Abc_Ntk_t * pMod;
    Au_Man_t * pMan;
    Au_Ntk_t * pNtk = NULL;
    abctime clk1, clk2 = 0, clk3 = 0, clk = Abc_Clock();
    int i;

    clk1 = Abc_Clock();
    pMan = Au_ManAlloc( pRoot->pDesign ? pRoot->pDesign->pName : pRoot->pName );
    pMan->pFuncs = Abc_NamStart( 100, 16 );
    clk2 += Abc_Clock() - clk1;

    vModels = Abc_NtkCollectHie( pRoot );
    Vec_PtrForEachEntry( Abc_Ntk_t *, vModels, pMod, i )
    {
        vOrder = Abc_NtkDfsBoxes( pMod );

        clk1 = Abc_Clock();
        pNtk = Au_NtkDerive( pMan, pMod, vOrder );
        pMod->iStep = pNtk->Id;
        pMod->pData = pNtk;
        clk2 += Abc_Clock() - clk1;

        Vec_PtrFree( vOrder );
    }
    Vec_PtrFree( vModels );
    // order models in topological order
    Au_ManReorderModels( pMan, pNtk );

    // print statistics
    Au_ManPrintStats( pMan );
    Au_ManCountThings( pNtk->pMan );

    // select network
    if ( pModelName )
    {
        pNtk = Au_ManFindNtkP( pMan, pModelName );
        if ( pNtk == NULL )
            printf( "Could not find module \"%s\".\n", pModelName );
    }
    if ( pNtk == NULL )
        pNtk = (Au_Ntk_t *)pRoot->pData;

  
//    if ( !Abc_NtkCheckRecursive(pRoot) )
    {
        clk1 = Abc_Clock();
        pGia = Au_NtkDeriveFlatGia( pNtk );
        clk3 = Abc_Clock() - clk1;
//        printf( "GIA objects max = %d.\n", pMan->nGiaObjMax );
    }

//    clk1 = Abc_Clock();
//    Au_NtkSuppSizeTest( (Au_Ntk_t *)pRoot->pData );
//    clk4 = Abc_Clock() - clk1;

    clk1 = Abc_Clock();
    Au_ManDelete( pMan );
    clk2 += Abc_Clock() - clk1;
    
    Abc_PrintTime( 1, "Time all ", Abc_Clock() - clk );
    Abc_PrintTime( 1, "Time new ", clk2 );
    Abc_PrintTime( 1, "Time GIA ", clk3 );
//    Abc_PrintTime( 1, "Time supp", clk4 );
    return pGia;
}

/**Function*************************************************************

  Synopsis    [Performs hierarchical equivalence checking.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Abc_NtkHieCecTest2( char * pFileName, char * pModelName, int fVerbose )
{
    int fSimulation = 0;
    Gia_Man_t * pGia = NULL;
    Au_Ntk_t * pNtk, * pNtkClp = NULL;
    abctime clk1 = 0, clk = Abc_Clock();

    // read hierarchical netlist
    pNtk = Au_NtkParseCBlif( pFileName );
    if ( pNtk == NULL )
    {
        printf( "Reading CBLIF file has failed.\n" );
        return NULL;
    }
    if ( pNtk->pMan == NULL || pNtk->pMan->vNtks.pArray == NULL )
    {
        printf( "There is no hierarchy information.\n" );
        Au_NtkFree( pNtk );
        return NULL;
    }
    Abc_PrintTime( 1, "Reading file", Abc_Clock() - clk );

    if ( fVerbose )
    {
        Au_ManPrintBoxInfo( pNtk );
//    Au_ManPrintBoxInfoSorted( pNtk );
        Au_ManPrintStats( pNtk->pMan );
    }
    Au_ManCountThings( pNtk->pMan );

    // select network
    if ( pModelName )
        pNtkClp = Au_ManFindNtkP( pNtk->pMan, pModelName );
    if ( pNtkClp == NULL )
        pNtkClp = pNtk;

    // check if the model is recursive
    Au_NtkCheckRecursive( pNtkClp );

    // collapse
    clk1 = Abc_Clock();
    if ( fSimulation )
    {
        Au_NtkTerSimulate( pNtkClp );
        Abc_PrintTime( 1, "Time sim ", Abc_Clock() - clk1 );
    }
    else
    {
        pGia = Au_NtkDeriveFlatGia( pNtkClp );
        Abc_PrintTime( 1, "Time GIA ", Abc_Clock() - clk1 );
    }

    // delete
    Au_ManDelete( pNtk->pMan );
    Abc_PrintTime( 1, "Time all ", Abc_Clock() - clk );
    return pGia;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

