/**CFile****************************************************************

  FileName    [abc.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abc.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef __ABC_H__
#define __ABC_H__

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "cuddInt.h"
#include "hop.h"
#include "extra.h"
#include "vec.h"
#include "stmm.h"
#include "nm.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

// network types
typedef enum { 
    ABC_NTK_NONE = 0,   // 0:  unknown
    ABC_NTK_NETLIST,    // 1:  network with PIs/POs, latches, nodes, and nets
    ABC_NTK_LOGIC,      // 2:  network with PIs/POs, latches, and nodes
    ABC_NTK_STRASH,     // 3:  structurally hashed AIG (two input AND gates with c-attributes on edges)
    ABC_NTK_OTHER       // 4:  unused
} Abc_NtkType_t;

// network functionality
typedef enum { 
    ABC_FUNC_NONE = 0,  // 0:  unknown
    ABC_FUNC_SOP,       // 1:  sum-of-products
    ABC_FUNC_BDD,       // 2:  binary decision diagrams
    ABC_FUNC_AIG,       // 3:  and-inverter graphs
    ABC_FUNC_MAP,       // 4:  standard cell library
    ABC_FUNC_BLIFMV,    // 5:  BLIF-MV node functions
    ABC_FUNC_BLACKBOX,  // 6:  black box about which nothing is known
    ABC_FUNC_OTHER      // 7:  unused
} Abc_NtkFunc_t;

// Supported type/functionality combinations:
/*------------------------------------------|
|           |  SOP  |  BDD  |  AIG  |  Map  |
|-----------|-------|-------|-------|-------|
|  Netlist  |   x   |       |   x   |   x   |
|-----------|-------|-------|-------|-------|
|  Logic    |   x   |   x   |   x   |   x   |
|-----------|-------|-------|-------|-------|
|  Strash   |       |       |   x   |       |
--------------------------------------------|*/

// object types
typedef enum { 
    ABC_OBJ_NONE = 0,   //  0:  unknown
    ABC_OBJ_CONST1,     //  1:  constant 1 node (AIG only)
    ABC_OBJ_PIO,        //  2:  inout terminal
    ABC_OBJ_PI,         //  3:  primary input terminal
    ABC_OBJ_PO,         //  4:  primary output terminal
    ABC_OBJ_BI,         //  5:  box input terminal
    ABC_OBJ_BO,         //  6:  box output terminal
    ABC_OBJ_ASSERT,     //  7:  assertion terminal
    ABC_OBJ_NET,        //  8:  net
    ABC_OBJ_NODE,       //  9:  node
    ABC_OBJ_LATCH,      // 10:  latch
    ABC_OBJ_WHITEBOX,   // 11:  box with known contents
    ABC_OBJ_BLACKBOX,   // 12:  box with unknown contents
    ABC_OBJ_NUMBER      // 13:  unused
} Abc_ObjType_t;

// latch initial values
typedef enum { 
    ABC_INIT_NONE = 0,  // 0:  unknown
    ABC_INIT_ZERO,      // 1:  zero
    ABC_INIT_ONE,       // 2:  one
    ABC_INIT_DC,        // 3:  don't-care
    ABC_INIT_OTHER      // 4:  unused
} Abc_InitType_t;

// latch type
typedef enum {
  ABC_FALLING_EDGE = 0,
  ABC_RISING_EDGE,
  ABC_ACTIVE_HIGH,
  ABC_ACTIVE_LOW,
  ABC_ASYNC
} Abc_LatchType_t;

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

//typedef int                   bool;
#ifndef __cplusplus
#ifndef bool
#define bool int
#endif
#endif

#ifndef SINT64
#define SINT64

#ifdef _WIN32
typedef signed __int64     sint64;   // compatible with MS VS 6.0
#else
typedef long long          sint64;
#endif

#endif

typedef struct Abc_Lib_t_       Abc_Lib_t;
typedef struct Abc_Ntk_t_       Abc_Ntk_t;
typedef struct Abc_Obj_t_       Abc_Obj_t;
typedef struct Abc_Aig_t_       Abc_Aig_t;
typedef struct Abc_ManTime_t_   Abc_ManTime_t;
typedef struct Abc_ManCut_t_    Abc_ManCut_t;
typedef struct Abc_Time_t_      Abc_Time_t;

struct Abc_Time_t_
{
    float             Rise;
    float             Fall;
    float             Worst;
};

struct Abc_Obj_t_ // 12 words
{
    // high-level information
    Abc_Ntk_t *       pNtk;          // the host network
    int               Id;            // the object ID
    int               TravId;        // the traversal ID (if changed, update Abc_NtkIncrementTravId)
    // internal information
    unsigned          Type    :  4;  // the object type
    unsigned          fMarkA  :  1;  // the multipurpose mark
    unsigned          fMarkB  :  1;  // the multipurpose mark
    unsigned          fMarkC  :  1;  // the multipurpose mark
    unsigned          fPhase  :  1;  // the flag to mark the phase of equivalent node
    unsigned          fExor   :  1;  // marks AIG node that is a root of EXOR
    unsigned          fPersist:  1;  // marks the persistant AIG node
    unsigned          fCompl0 :  1;  // complemented attribute of the first fanin in the AIG
    unsigned          fCompl1 :  1;  // complemented attribute of the second fanin in the AIG 
    unsigned          Level   : 20;  // the level of the node
    // connectivity
    Vec_Int_t         vFanins;       // the array of fanins
    Vec_Int_t         vFanouts;      // the array of fanouts
    // miscellaneous
    void *            pData;         // the network specific data (SOP, BDD, gate, equiv class, etc)
    Abc_Obj_t *       pNext;         // the next pointer in the hash table
    Abc_Obj_t *       pCopy;         // the copy of this object
    Hop_Obj_t *       pEquiv;        // pointer to the HAIG node
};

struct Abc_Ntk_t_ 
{
    // general information 
    Abc_NtkType_t     ntkType;       // type of the network
    Abc_NtkFunc_t     ntkFunc;       // functionality of the network
    char *            pName;         // the network name
    char *            pSpec;         // the name of the spec file if present
    Nm_Man_t *        pManName;      // name manager (stores names of objects)
    // components of the network
    Vec_Ptr_t *       vObjs;         // the array of all objects (net, nodes, latches, etc)
    Vec_Ptr_t *       vPis;          // the array of primary inputs
    Vec_Ptr_t *       vPos;          // the array of primary outputs
    Vec_Ptr_t *       vCis;          // the array of combinational inputs  (PIs, latches)
    Vec_Ptr_t *       vCos;          // the array of combinational outputs (POs, asserts, latches)
    Vec_Ptr_t *       vPios;         // the array of PIOs
    Vec_Ptr_t *       vAsserts;      // the array of assertions
    Vec_Ptr_t *       vBoxes;        // the array of boxes
    // the number of living objects
    int               nObjs;         // the number of live objs
    int nObjCounts[ABC_OBJ_NUMBER];  // the number of objects by type
    // the backup network and the step number
    Abc_Ntk_t *       pNetBackup;    // the pointer to the previous backup network
    int               iStep;         // the generation number for the given network
    // hierarchy
    Abc_Lib_t *       pDesign;
    short             fHieVisited;   // flag to mark the visited network
    short             fHiePath;      // flag to mark the network on the path
    // miscellaneous data members
    int               nTravIds;      // the unique traversal IDs of nodes
    Extra_MmFixed_t * pMmObj;        // memory manager for objects
    Extra_MmStep_t *  pMmStep;       // memory manager for arrays
    void *            pManFunc;      // functionality manager (AIG manager, BDD manager, or memory manager for SOPs)
//    Abc_Lib_t *       pVerLib;       // for structural verilog designs
    Abc_ManTime_t *   pManTime;      // the timing manager (for mapped networks) stores arrival/required times for all nodes
    void *            pManCut;       // the cut manager (for AIGs) stores information about the cuts computed for the nodes
    int               LevelMax;      // maximum number of levels
    Vec_Int_t *       vLevelsR;      // level in the reverse topological order (for AIGs)
    Vec_Ptr_t *       vSupps;        // CO support information
    int *             pModel;        // counter-example (for miters)
    Abc_Ntk_t *       pExdc;         // the EXDC network (if given)
    void *            pData;         // misc
    Abc_Ntk_t *       pCopy;
    Hop_Man_t *       pHaig;         // history AIG
    // node attributes
    Vec_Ptr_t *       vAttrs;        // managers of various node attributes (node functionality, global BDDs, etc)
};

struct Abc_Lib_t_ 
{
    char *            pName;         // the name of the library
    void *            pManFunc;      // functionality manager for the nodes
    Vec_Ptr_t *       vTops;         // the array of top-level modules
    Vec_Ptr_t *       vModules;      // the array of modules
    st_table *        tModules;      // the table hashing module names into their networks
    Abc_Lib_t *       pLibrary;      // the library used to map this design
    void *            pGenlib;       // the genlib library used to map this design
};

struct Abc_LatchInfo_t_
{
  Abc_InitType_t    InitValue;
  Abc_LatchType_t   LatchType;
  char              *pClkName;
};

typedef struct Abc_LatchInfo_t_ Abc_LatchInfo_t;

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

// maximum/minimum operators
#define ABC_MIN(a,b)      (((a) < (b))? (a) : (b))
#define ABC_MAX(a,b)      (((a) > (b))? (a) : (b))
#define ABC_ABS(a)        (((a) >= 0)?  (a) :-(a))
#define ABC_INFINITY      (100000000)

// transforming floats into ints and back
static inline int         Abc_Float2Int( float Val )                 { return *((int *)&Val);                       }
static inline float       Abc_Int2Float( int Num )                   { return *((float *)&Num);                     }
static inline int         Abc_BitWordNum( int nBits )                { return (nBits>>5) + ((nBits&31) > 0);        }
static inline int         Abc_TruthWordNum( int nVars )              { return nVars <= 5 ? 1 : (1 << (nVars - 5));  }
static inline int         Abc_InfoHasBit( unsigned * p, int i )      { return (p[(i)>>5] & (1<<((i) & 31))) > 0;    }
static inline void        Abc_InfoSetBit( unsigned * p, int i )      { p[(i)>>5] |= (1<<((i) & 31));                }
static inline void        Abc_InfoXorBit( unsigned * p, int i )      { p[(i)>>5] ^= (1<<((i) & 31));                }
static inline unsigned    Abc_InfoRandomWord()                       { return ((((unsigned)rand()) << 24) ^ (((unsigned)rand()) << 12) ^ ((unsigned)rand())); } // #define RAND_MAX 0x7fff
static inline void        Abc_InfoRandom( unsigned * p, int nWords ) { int i; for ( i = nWords - 1; i >= 0; i-- ) p[i] = Abc_InfoRandomWord();   } 
static inline void        Abc_InfoClear( unsigned * p, int nWords )  { memset( p, 0, sizeof(unsigned) * nWords );   } 
static inline void        Abc_InfoFill( unsigned * p, int nWords )   { memset( p, 0xff, sizeof(unsigned) * nWords );} 
static inline void        Abc_InfoNot( unsigned * p, int nWords )    { int i; for ( i = nWords - 1; i >= 0; i-- ) p[i] = ~p[i];   } 
static inline int         Abc_InfoIsZero( unsigned * p, int nWords ) { int i; for ( i = nWords - 1; i >= 0; i-- ) if ( p[i] )  return 0; return 1; } 
static inline int         Abc_InfoIsOne( unsigned * p, int nWords )  { int i; for ( i = nWords - 1; i >= 0; i-- ) if ( ~p[i] ) return 0; return 1; } 
static inline void        Abc_InfoCopy( unsigned * p, unsigned * q, int nWords )   { int i; for ( i = nWords - 1; i >= 0; i-- ) p[i]  = q[i];  } 
static inline void        Abc_InfoAnd( unsigned * p, unsigned * q, int nWords )    { int i; for ( i = nWords - 1; i >= 0; i-- ) p[i] &= q[i];  } 
static inline void        Abc_InfoOr( unsigned * p, unsigned * q, int nWords )     { int i; for ( i = nWords - 1; i >= 0; i-- ) p[i] |= q[i];  } 
static inline void        Abc_InfoXor( unsigned * p, unsigned * q, int nWords )    { int i; for ( i = nWords - 1; i >= 0; i-- ) p[i] ^= q[i];  } 
static inline int         Abc_InfoIsOrOne( unsigned * p, unsigned * q, int nWords ){ int i; for ( i = nWords - 1; i >= 0; i-- ) if ( ~(p[i] | q[i]) ) return 0; return 1; } 
static inline int         Abc_InfoIsOrOne3( unsigned * p, unsigned * q, unsigned * r, int nWords ){ int i; for ( i = nWords - 1; i >= 0; i-- ) if ( ~(p[i] | q[i] | r[i]) ) return 0; return 1; } 

// checking the network type
static inline bool        Abc_NtkIsNetlist( Abc_Ntk_t * pNtk )       { return pNtk->ntkType == ABC_NTK_NETLIST;     }
static inline bool        Abc_NtkIsLogic( Abc_Ntk_t * pNtk )         { return pNtk->ntkType == ABC_NTK_LOGIC;       }
static inline bool        Abc_NtkIsStrash( Abc_Ntk_t * pNtk )        { return pNtk->ntkType == ABC_NTK_STRASH;      }

static inline bool        Abc_NtkHasSop( Abc_Ntk_t * pNtk )          { return pNtk->ntkFunc == ABC_FUNC_SOP;        }
static inline bool        Abc_NtkHasBdd( Abc_Ntk_t * pNtk )          { return pNtk->ntkFunc == ABC_FUNC_BDD;        }
static inline bool        Abc_NtkHasAig( Abc_Ntk_t * pNtk )          { return pNtk->ntkFunc == ABC_FUNC_AIG;        }
static inline bool        Abc_NtkHasMapping( Abc_Ntk_t * pNtk )      { return pNtk->ntkFunc == ABC_FUNC_MAP;        }
static inline bool        Abc_NtkHasBlifMv( Abc_Ntk_t * pNtk )       { return pNtk->ntkFunc == ABC_FUNC_BLIFMV;     }
static inline bool        Abc_NtkHasBlackbox( Abc_Ntk_t * pNtk )     { return pNtk->ntkFunc == ABC_FUNC_BLACKBOX;   }

static inline bool        Abc_NtkIsSopNetlist( Abc_Ntk_t * pNtk )    { return pNtk->ntkFunc == ABC_FUNC_SOP && pNtk->ntkType == ABC_NTK_NETLIST;  }
static inline bool        Abc_NtkIsAigNetlist( Abc_Ntk_t * pNtk )    { return pNtk->ntkFunc == ABC_FUNC_AIG && pNtk->ntkType == ABC_NTK_NETLIST;  }
static inline bool        Abc_NtkIsMappedNetlist( Abc_Ntk_t * pNtk ) { return pNtk->ntkFunc == ABC_FUNC_MAP && pNtk->ntkType == ABC_NTK_NETLIST;  }
static inline bool        Abc_NtkIsBlifMvNetlist( Abc_Ntk_t * pNtk ) { return pNtk->ntkFunc == ABC_FUNC_BLIFMV && pNtk->ntkType == ABC_NTK_NETLIST;  }
static inline bool        Abc_NtkIsSopLogic( Abc_Ntk_t * pNtk )      { return pNtk->ntkFunc == ABC_FUNC_SOP && pNtk->ntkType == ABC_NTK_LOGIC  ;  }
static inline bool        Abc_NtkIsBddLogic( Abc_Ntk_t * pNtk )      { return pNtk->ntkFunc == ABC_FUNC_BDD && pNtk->ntkType == ABC_NTK_LOGIC  ;  }
static inline bool        Abc_NtkIsAigLogic( Abc_Ntk_t * pNtk )      { return pNtk->ntkFunc == ABC_FUNC_AIG && pNtk->ntkType == ABC_NTK_LOGIC  ;  }
static inline bool        Abc_NtkIsMappedLogic( Abc_Ntk_t * pNtk )   { return pNtk->ntkFunc == ABC_FUNC_MAP && pNtk->ntkType == ABC_NTK_LOGIC  ;  }

// reading data members of the network
static inline char *      Abc_NtkName( Abc_Ntk_t * pNtk )            { return pNtk->pName;            }
static inline char *      Abc_NtkSpec( Abc_Ntk_t * pNtk )            { return pNtk->pSpec;            }
static inline int         Abc_NtkTravId( Abc_Ntk_t * pNtk )          { return pNtk->nTravIds;         }    
static inline Abc_Ntk_t * Abc_NtkExdc( Abc_Ntk_t * pNtk )            { return pNtk->pExdc;            }
static inline Abc_Ntk_t * Abc_NtkBackup( Abc_Ntk_t * pNtk )          { return pNtk->pNetBackup;       }
static inline int         Abc_NtkStep  ( Abc_Ntk_t * pNtk )          { return pNtk->iStep;            }

// setting data members of the network
static inline void        Abc_NtkSetName  ( Abc_Ntk_t * pNtk, char * pName )           { pNtk->pName      = pName;      } 
static inline void        Abc_NtkSetSpec  ( Abc_Ntk_t * pNtk, char * pName )           { pNtk->pSpec      = pName;      } 
static inline void        Abc_NtkSetBackup( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNetBackup ) { pNtk->pNetBackup = pNetBackup; }
static inline void        Abc_NtkSetStep  ( Abc_Ntk_t * pNtk, int iStep )              { pNtk->iStep      = iStep;      }

// getting the number of objects 
static inline int         Abc_NtkObjNum( Abc_Ntk_t * pNtk )          { return pNtk->nObjs;                        }
static inline int         Abc_NtkObjNumMax( Abc_Ntk_t * pNtk )       { return Vec_PtrSize(pNtk->vObjs);           }
static inline int         Abc_NtkPiNum( Abc_Ntk_t * pNtk )           { return Vec_PtrSize(pNtk->vPis);            }
static inline int         Abc_NtkPoNum( Abc_Ntk_t * pNtk )           { return Vec_PtrSize(pNtk->vPos);            }
static inline int         Abc_NtkCiNum( Abc_Ntk_t * pNtk )           { return Vec_PtrSize(pNtk->vCis);            }
static inline int         Abc_NtkCoNum( Abc_Ntk_t * pNtk )           { return Vec_PtrSize(pNtk->vCos);            }
static inline int         Abc_NtkAssertNum( Abc_Ntk_t * pNtk )       { return Vec_PtrSize(pNtk->vAsserts);        }
static inline int         Abc_NtkBoxNum( Abc_Ntk_t * pNtk )          { return Vec_PtrSize(pNtk->vBoxes);          }
static inline int         Abc_NtkBiNum( Abc_Ntk_t * pNtk )           { return pNtk->nObjCounts[ABC_OBJ_BI];       }
static inline int         Abc_NtkBoNum( Abc_Ntk_t * pNtk )           { return pNtk->nObjCounts[ABC_OBJ_BO];       }
static inline int         Abc_NtkNetNum( Abc_Ntk_t * pNtk )          { return pNtk->nObjCounts[ABC_OBJ_NET];      }
static inline int         Abc_NtkNodeNum( Abc_Ntk_t * pNtk )         { return pNtk->nObjCounts[ABC_OBJ_NODE];     }
static inline int         Abc_NtkLatchNum( Abc_Ntk_t * pNtk )        { return pNtk->nObjCounts[ABC_OBJ_LATCH];    }
static inline int         Abc_NtkWhiteboxNum( Abc_Ntk_t * pNtk )     { return pNtk->nObjCounts[ABC_OBJ_WHITEBOX]; }
static inline int         Abc_NtkBlackboxNum( Abc_Ntk_t * pNtk )     { return pNtk->nObjCounts[ABC_OBJ_BLACKBOX]; }
static inline bool        Abc_NtkIsComb( Abc_Ntk_t * pNtk )          { return Abc_NtkLatchNum(pNtk) == 0;                   }
static inline bool        Abc_NtkHasOnlyLatchBoxes(Abc_Ntk_t * pNtk ){ return Abc_NtkLatchNum(pNtk) == Abc_NtkBoxNum(pNtk); }

// creating simple objects
extern        Abc_Obj_t * Abc_NtkCreateObj( Abc_Ntk_t * pNtk, Abc_ObjType_t Type );
static inline Abc_Obj_t * Abc_NtkCreatePi( Abc_Ntk_t * pNtk )        { return Abc_NtkCreateObj( pNtk, ABC_OBJ_PI );         }
static inline Abc_Obj_t * Abc_NtkCreatePo( Abc_Ntk_t * pNtk )        { return Abc_NtkCreateObj( pNtk, ABC_OBJ_PO );         }
static inline Abc_Obj_t * Abc_NtkCreateBi( Abc_Ntk_t * pNtk )        { return Abc_NtkCreateObj( pNtk, ABC_OBJ_BI );         }
static inline Abc_Obj_t * Abc_NtkCreateBo( Abc_Ntk_t * pNtk )        { return Abc_NtkCreateObj( pNtk, ABC_OBJ_BO );         }
static inline Abc_Obj_t * Abc_NtkCreateAssert( Abc_Ntk_t * pNtk )    { return Abc_NtkCreateObj( pNtk, ABC_OBJ_ASSERT );     }
static inline Abc_Obj_t * Abc_NtkCreateNet( Abc_Ntk_t * pNtk )       { return Abc_NtkCreateObj( pNtk, ABC_OBJ_NET );        }
static inline Abc_Obj_t * Abc_NtkCreateNode( Abc_Ntk_t * pNtk )      { return Abc_NtkCreateObj( pNtk, ABC_OBJ_NODE );       }
static inline Abc_Obj_t * Abc_NtkCreateLatch( Abc_Ntk_t * pNtk )     { return Abc_NtkCreateObj( pNtk, ABC_OBJ_LATCH );      }
static inline Abc_Obj_t * Abc_NtkCreateWhitebox( Abc_Ntk_t * pNtk )  { return Abc_NtkCreateObj( pNtk, ABC_OBJ_WHITEBOX );   }
static inline Abc_Obj_t * Abc_NtkCreateBlackbox( Abc_Ntk_t * pNtk )  { return Abc_NtkCreateObj( pNtk, ABC_OBJ_BLACKBOX );   }

// reading objects
static inline Abc_Obj_t * Abc_NtkObj( Abc_Ntk_t * pNtk, int i )      { return (Abc_Obj_t *)Vec_PtrEntry( pNtk->vObjs, i );   }
static inline Abc_Obj_t * Abc_NtkPi( Abc_Ntk_t * pNtk, int i )       { return (Abc_Obj_t *)Vec_PtrEntry( pNtk->vPis, i );    }
static inline Abc_Obj_t * Abc_NtkPo( Abc_Ntk_t * pNtk, int i )       { return (Abc_Obj_t *)Vec_PtrEntry( pNtk->vPos, i );    }
static inline Abc_Obj_t * Abc_NtkCi( Abc_Ntk_t * pNtk, int i )       { return (Abc_Obj_t *)Vec_PtrEntry( pNtk->vCis, i );    }
static inline Abc_Obj_t * Abc_NtkCo( Abc_Ntk_t * pNtk, int i )       { return (Abc_Obj_t *)Vec_PtrEntry( pNtk->vCos, i );    }
static inline Abc_Obj_t * Abc_NtkAssert( Abc_Ntk_t * pNtk, int i )   { return (Abc_Obj_t *)Vec_PtrEntry( pNtk->vAsserts, i );}
static inline Abc_Obj_t * Abc_NtkBox( Abc_Ntk_t * pNtk, int i )      { return (Abc_Obj_t *)Vec_PtrEntry( pNtk->vBoxes, i );  }

// working with complemented attributes of objects
static inline bool        Abc_ObjIsComplement( Abc_Obj_t * p )       { return (bool)((unsigned long)p & (unsigned long)01);             }
static inline Abc_Obj_t * Abc_ObjRegular( Abc_Obj_t * p )            { return (Abc_Obj_t *)((unsigned long)p & ~(unsigned long)01);     }
static inline Abc_Obj_t * Abc_ObjNot( Abc_Obj_t * p )                { return (Abc_Obj_t *)((unsigned long)p ^  (unsigned long)01);     }
static inline Abc_Obj_t * Abc_ObjNotCond( Abc_Obj_t * p, int c )     { return (Abc_Obj_t *)((unsigned long)p ^  (unsigned long)(c!=0)); }

// reading data members of the object
static inline unsigned    Abc_ObjType( Abc_Obj_t * pObj )            { return pObj->Type;               }
static inline unsigned    Abc_ObjId( Abc_Obj_t * pObj )              { return pObj->Id;                 }
static inline int         Abc_ObjTravId( Abc_Obj_t * pObj )          { return pObj->TravId;             }
static inline int         Abc_ObjLevel( Abc_Obj_t * pObj )           { return pObj->Level;              }
static inline Vec_Int_t * Abc_ObjFaninVec( Abc_Obj_t * pObj )        { return &pObj->vFanins;           }
static inline Vec_Int_t * Abc_ObjFanoutVec( Abc_Obj_t * pObj )       { return &pObj->vFanouts;          }
static inline Abc_Obj_t * Abc_ObjCopy( Abc_Obj_t * pObj )            { return pObj->pCopy;              }
static inline Abc_Ntk_t * Abc_ObjNtk( Abc_Obj_t * pObj )             { return pObj->pNtk;               }
static inline void *      Abc_ObjData( Abc_Obj_t * pObj )            { return pObj->pData;              }
static inline Hop_Obj_t * Abc_ObjEquiv( Abc_Obj_t * pObj )           { return pObj->pEquiv;             }
static inline Abc_Obj_t * Abc_ObjCopyCond( Abc_Obj_t * pObj )        { return Abc_ObjRegular(pObj)->pCopy? Abc_ObjNotCond(Abc_ObjRegular(pObj)->pCopy, Abc_ObjIsComplement(pObj)) : NULL;  }

// setting data members of the network
static inline void        Abc_ObjSetLevel( Abc_Obj_t * pObj, int Level )         { pObj->Level =  Level;    } 
static inline void        Abc_ObjSetCopy( Abc_Obj_t * pObj, Abc_Obj_t * pCopy )  { pObj->pCopy =  pCopy;    } 
static inline void        Abc_ObjSetData( Abc_Obj_t * pObj, void * pData )       { pObj->pData =  pData;    } 

// checking the object type
static inline bool        Abc_ObjIsPio( Abc_Obj_t * pObj )           { return pObj->Type == ABC_OBJ_PIO;     }
static inline bool        Abc_ObjIsPi( Abc_Obj_t * pObj )            { return pObj->Type == ABC_OBJ_PI;      }
static inline bool        Abc_ObjIsPo( Abc_Obj_t * pObj )            { return pObj->Type == ABC_OBJ_PO;      }
static inline bool        Abc_ObjIsBi( Abc_Obj_t * pObj )            { return pObj->Type == ABC_OBJ_BI;      }
static inline bool        Abc_ObjIsBo( Abc_Obj_t * pObj )            { return pObj->Type == ABC_OBJ_BO;      }
static inline bool        Abc_ObjIsAssert( Abc_Obj_t * pObj )        { return pObj->Type == ABC_OBJ_ASSERT;  }
static inline bool        Abc_ObjIsCi( Abc_Obj_t * pObj )            { return pObj->Type == ABC_OBJ_PI || pObj->Type == ABC_OBJ_BO; }
static inline bool        Abc_ObjIsCo( Abc_Obj_t * pObj )            { return pObj->Type == ABC_OBJ_PO || pObj->Type == ABC_OBJ_BI || pObj->Type == ABC_OBJ_ASSERT; }
static inline bool        Abc_ObjIsTerm( Abc_Obj_t * pObj )          { return Abc_ObjIsCi(pObj) || Abc_ObjIsCo(pObj); }
static inline bool        Abc_ObjIsNet( Abc_Obj_t * pObj )           { return pObj->Type == ABC_OBJ_NET;     }
static inline bool        Abc_ObjIsNode( Abc_Obj_t * pObj )          { return pObj->Type == ABC_OBJ_NODE;    }
static inline bool        Abc_ObjIsLatch( Abc_Obj_t * pObj )         { return pObj->Type == ABC_OBJ_LATCH;   }
static inline bool        Abc_ObjIsBox( Abc_Obj_t * pObj )           { return pObj->Type == ABC_OBJ_LATCH || pObj->Type == ABC_OBJ_WHITEBOX || pObj->Type == ABC_OBJ_BLACKBOX; }
static inline bool        Abc_ObjIsWhitebox( Abc_Obj_t * pObj )      { return pObj->Type == ABC_OBJ_WHITEBOX;}
static inline bool        Abc_ObjIsBlackbox( Abc_Obj_t * pObj )      { return pObj->Type == ABC_OBJ_BLACKBOX;}
static inline void        Abc_ObjBlackboxToWhitebox( Abc_Obj_t * pObj ) { assert( Abc_ObjIsBlackbox(pObj) ); pObj->Type = ABC_OBJ_WHITEBOX; pObj->pNtk->nObjCounts[ABC_OBJ_BLACKBOX]--; pObj->pNtk->nObjCounts[ABC_OBJ_WHITEBOX]++; }

// working with fanin/fanout edges
static inline int         Abc_ObjFaninNum( Abc_Obj_t * pObj )        { return pObj->vFanins.nSize;     }
static inline int         Abc_ObjFanoutNum( Abc_Obj_t * pObj )       { return pObj->vFanouts.nSize;    }
static inline int         Abc_ObjFaninId( Abc_Obj_t * pObj, int i)   { return pObj->vFanins.pArray[i]; }
static inline int         Abc_ObjFaninId0( Abc_Obj_t * pObj )        { return pObj->vFanins.pArray[0]; }
static inline int         Abc_ObjFaninId1( Abc_Obj_t * pObj )        { return pObj->vFanins.pArray[1]; }
static inline int         Abc_ObjFanoutEdgeNum( Abc_Obj_t * pObj, Abc_Obj_t * pFanout )  { assert( Abc_NtkHasAig(pObj->pNtk) );  if ( Abc_ObjFaninId0(pFanout) == pObj->Id ) return 0; if ( Abc_ObjFaninId1(pFanout) == pObj->Id ) return 1; assert( 0 ); return -1;  }
static inline Abc_Obj_t * Abc_ObjFanout( Abc_Obj_t * pObj, int i )   { return (Abc_Obj_t *)pObj->pNtk->vObjs->pArray[ pObj->vFanouts.pArray[i] ];  }
static inline Abc_Obj_t * Abc_ObjFanout0( Abc_Obj_t * pObj )         { return (Abc_Obj_t *)pObj->pNtk->vObjs->pArray[ pObj->vFanouts.pArray[0] ];  }
static inline Abc_Obj_t * Abc_ObjFanin( Abc_Obj_t * pObj, int i )    { return (Abc_Obj_t *)pObj->pNtk->vObjs->pArray[ pObj->vFanins.pArray[i] ];   }
static inline Abc_Obj_t * Abc_ObjFanin0( Abc_Obj_t * pObj )          { return (Abc_Obj_t *)pObj->pNtk->vObjs->pArray[ pObj->vFanins.pArray[0] ];   }
static inline Abc_Obj_t * Abc_ObjFanin1( Abc_Obj_t * pObj )          { return (Abc_Obj_t *)pObj->pNtk->vObjs->pArray[ pObj->vFanins.pArray[1] ];   }
static inline Abc_Obj_t * Abc_ObjFanin0Ntk( Abc_Obj_t * pObj )       { return (Abc_NtkIsNetlist(pObj->pNtk)? Abc_ObjFanin0(pObj)  : pObj);  }
static inline Abc_Obj_t * Abc_ObjFanout0Ntk( Abc_Obj_t * pObj )      { return (Abc_NtkIsNetlist(pObj->pNtk)? Abc_ObjFanout0(pObj) : pObj);  }
static inline bool        Abc_ObjFaninC0( Abc_Obj_t * pObj )         { return pObj->fCompl0;                                                }
static inline bool        Abc_ObjFaninC1( Abc_Obj_t * pObj )         { return pObj->fCompl1;                                                }
static inline bool        Abc_ObjFaninC( Abc_Obj_t * pObj, int i )   { assert( i >=0 && i < 2 ); return i? pObj->fCompl1 : pObj->fCompl0;   }
static inline void        Abc_ObjSetFaninC( Abc_Obj_t * pObj, int i ){ assert( i >=0 && i < 2 ); if ( i ) pObj->fCompl1 = 1; else pObj->fCompl0 = 1; }
static inline void        Abc_ObjXorFaninC( Abc_Obj_t * pObj, int i ){ assert( i >=0 && i < 2 ); if ( i ) pObj->fCompl1^= 1; else pObj->fCompl0^= 1; }
static inline Abc_Obj_t * Abc_ObjChild( Abc_Obj_t * pObj, int i )    { return Abc_ObjNotCond( Abc_ObjFanin(pObj,i), Abc_ObjFaninC(pObj,i) );}
static inline Abc_Obj_t * Abc_ObjChild0( Abc_Obj_t * pObj )          { return Abc_ObjNotCond( Abc_ObjFanin0(pObj), Abc_ObjFaninC0(pObj) );  }
static inline Abc_Obj_t * Abc_ObjChild1( Abc_Obj_t * pObj )          { return Abc_ObjNotCond( Abc_ObjFanin1(pObj), Abc_ObjFaninC1(pObj) );  }
static inline Abc_Obj_t * Abc_ObjChildCopy( Abc_Obj_t * pObj, int i ){ return Abc_ObjNotCond( Abc_ObjFanin(pObj,i)->pCopy, Abc_ObjFaninC(pObj,i) );  }
static inline Abc_Obj_t * Abc_ObjChild0Copy( Abc_Obj_t * pObj )      { return Abc_ObjNotCond( Abc_ObjFanin0(pObj)->pCopy, Abc_ObjFaninC0(pObj) );    }
static inline Abc_Obj_t * Abc_ObjChild1Copy( Abc_Obj_t * pObj )      { return Abc_ObjNotCond( Abc_ObjFanin1(pObj)->pCopy, Abc_ObjFaninC1(pObj) );    }
static inline Abc_Obj_t * Abc_ObjChild0Data( Abc_Obj_t * pObj )      { return Abc_ObjNotCond( (Abc_Obj_t *)Abc_ObjFanin0(pObj)->pData, Abc_ObjFaninC0(pObj) );    }
static inline Abc_Obj_t * Abc_ObjChild1Data( Abc_Obj_t * pObj )      { return Abc_ObjNotCond( (Abc_Obj_t *)Abc_ObjFanin1(pObj)->pData, Abc_ObjFaninC1(pObj) );    }
static inline Hop_Obj_t * Abc_ObjChild0Equiv( Abc_Obj_t * pObj )     { return Hop_NotCond( Abc_ObjFanin0(pObj)->pEquiv, Abc_ObjFaninC0(pObj) );      }
static inline Hop_Obj_t * Abc_ObjChild1Equiv( Abc_Obj_t * pObj )     { return Hop_NotCond( Abc_ObjFanin1(pObj)->pEquiv, Abc_ObjFaninC1(pObj) );      }

// checking the AIG node types
static inline bool        Abc_AigNodeIsConst( Abc_Obj_t * pNode )    { assert(Abc_NtkIsStrash(Abc_ObjRegular(pNode)->pNtk));  return Abc_ObjRegular(pNode)->Type == ABC_OBJ_CONST1;       }
static inline bool        Abc_AigNodeIsAnd( Abc_Obj_t * pNode )      { assert(!Abc_ObjIsComplement(pNode)); assert(Abc_NtkIsStrash(pNode->pNtk)); return Abc_ObjFaninNum(pNode) == 2;                         }
static inline bool        Abc_AigNodeIsChoice( Abc_Obj_t * pNode )   { assert(!Abc_ObjIsComplement(pNode)); assert(Abc_NtkIsStrash(pNode->pNtk)); return pNode->pData != NULL && Abc_ObjFanoutNum(pNode) > 0; }

// handling persistent nodes
static inline int         Abc_NodeIsPersistant( Abc_Obj_t * pNode )    { assert( Abc_AigNodeIsAnd(pNode) ); return pNode->fPersist; } 
static inline void        Abc_NodeSetPersistant( Abc_Obj_t * pNode )   { assert( Abc_AigNodeIsAnd(pNode) ); pNode->fPersist = 1;    } 
static inline void        Abc_NodeClearPersistant( Abc_Obj_t * pNode ) { assert( Abc_AigNodeIsAnd(pNode) ); pNode->fPersist = 0;    } 

// working with the traversal ID
static inline void        Abc_NodeSetTravId( Abc_Obj_t * pNode, int TravId ) { pNode->TravId = TravId;                                    }
static inline void        Abc_NodeSetTravIdCurrent( Abc_Obj_t * pNode )      { pNode->TravId = pNode->pNtk->nTravIds;                     }
static inline void        Abc_NodeSetTravIdPrevious( Abc_Obj_t * pNode )     { pNode->TravId = pNode->pNtk->nTravIds - 1;                 }
static inline bool        Abc_NodeIsTravIdCurrent( Abc_Obj_t * pNode )       { return (bool)(pNode->TravId == pNode->pNtk->nTravIds);     }
static inline bool        Abc_NodeIsTravIdPrevious( Abc_Obj_t * pNode )      { return (bool)(pNode->TravId == pNode->pNtk->nTravIds - 1); }

// checking initial state of the latches
static inline void        Abc_LatchSetInitNone( Abc_Obj_t * pLatch ) { assert(Abc_ObjIsLatch(pLatch)); ((Abc_LatchInfo_t *)(pLatch->pData))->InitValue = ABC_INIT_NONE;                       }
static inline void        Abc_LatchSetInit0( Abc_Obj_t * pLatch )    { assert(Abc_ObjIsLatch(pLatch)); ((Abc_LatchInfo_t *)(pLatch->pData))->InitValue = ABC_INIT_ZERO;                       }
static inline void        Abc_LatchSetInit1( Abc_Obj_t * pLatch )    { assert(Abc_ObjIsLatch(pLatch)); ((Abc_LatchInfo_t *)(pLatch->pData))->InitValue = ABC_INIT_ONE;                        }
static inline void        Abc_LatchSetInitDc( Abc_Obj_t * pLatch )   { assert(Abc_ObjIsLatch(pLatch)); ((Abc_LatchInfo_t *)(pLatch->pData))->InitValue = ABC_INIT_DC;                         }
static inline int         Abc_LatchIsInitNone( Abc_Obj_t * pLatch )  { assert(Abc_ObjIsLatch(pLatch)); return ((Abc_LatchInfo_t *)(pLatch->pData))->InitValue == ABC_INIT_NONE;               }
static inline int         Abc_LatchIsInit0( Abc_Obj_t * pLatch )     { assert(Abc_ObjIsLatch(pLatch)); return ((Abc_LatchInfo_t *)(pLatch->pData))->InitValue == ABC_INIT_ZERO;               }
static inline int         Abc_LatchIsInit1( Abc_Obj_t * pLatch )     { assert(Abc_ObjIsLatch(pLatch)); return ((Abc_LatchInfo_t *)(pLatch->pData))->InitValue == ABC_INIT_ONE;                }
static inline int         Abc_LatchIsInitDc( Abc_Obj_t * pLatch )    { assert(Abc_ObjIsLatch(pLatch)); return ((Abc_LatchInfo_t *)(pLatch->pData))->InitValue == ABC_INIT_DC;                 }
static inline int         Abc_LatchInit( Abc_Obj_t * pLatch )        { assert(Abc_ObjIsLatch(pLatch)); return (int)((Abc_LatchInfo_t *)(pLatch->pData))->InitValue;                           }
// global BDDs of the nodes
static inline void *      Abc_NtkGlobalBdd( Abc_Ntk_t * pNtk )          { return (void *)Vec_PtrEntry(pNtk->vAttrs, VEC_ATTR_GLOBAL_BDD);             }
static inline DdManager * Abc_NtkGlobalBddMan( Abc_Ntk_t * pNtk )       { return (DdManager *)Vec_AttMan( (Vec_Att_t *)Abc_NtkGlobalBdd(pNtk) );                   }
static inline DdNode **   Abc_NtkGlobalBddArray( Abc_Ntk_t * pNtk )     { return (DdNode **)Vec_AttArray( (Vec_Att_t *)Abc_NtkGlobalBdd(pNtk) );                   }
static inline DdNode *    Abc_ObjGlobalBdd( Abc_Obj_t * pObj )          { return (DdNode *)Vec_AttEntry( (Vec_Att_t *)Abc_NtkGlobalBdd(pObj->pNtk), pObj->Id );    }
static inline void        Abc_ObjSetGlobalBdd( Abc_Obj_t * pObj, DdNode * bF )   { Vec_AttWriteEntry( (Vec_Att_t *)Abc_NtkGlobalBdd(pObj->pNtk), pObj->Id, bF );   }

// MV variables of the nodes
static inline void *      Abc_NtkMvVar( Abc_Ntk_t * pNtk )              { return Vec_PtrEntry(pNtk->vAttrs, VEC_ATTR_MVVAR);                          }
static inline void *      Abc_NtkMvVarMan( Abc_Ntk_t * pNtk )           { return Abc_NtkMvVar(pNtk)? Vec_AttMan( (Vec_Att_t *)Abc_NtkMvVar(pNtk) ) : NULL;         }
static inline void *      Abc_ObjMvVar( Abc_Obj_t * pObj )              { return Abc_NtkMvVar(pObj->pNtk)? Vec_AttEntry( (Vec_Att_t *)Abc_NtkMvVar(pObj->pNtk), pObj->Id ) : NULL; }
static inline int         Abc_ObjMvVarNum( Abc_Obj_t * pObj )           { return (Abc_NtkMvVar(pObj->pNtk) && Abc_ObjMvVar(pObj))? *((int*)Abc_ObjMvVar(pObj)) : 2;   }
static inline void        Abc_ObjSetMvVar( Abc_Obj_t * pObj, void * pV) { Vec_AttWriteEntry( (Vec_Att_t *)Abc_NtkMvVar(pObj->pNtk), pObj->Id, pV );                }

// outputs the runtime in seconds
#define PRT(a,t)  printf("%s = ", (a)); printf("%6.2f sec\n", (float)(t)/(float)(CLOCKS_PER_SEC))

////////////////////////////////////////////////////////////////////////
///                        ITERATORS                                 ///
////////////////////////////////////////////////////////////////////////

// objects of the network
#define Abc_NtkForEachObj( pNtk, pObj, i )                                                         \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vObjs)) && (((pObj) = Abc_NtkObj(pNtk, i)), 1); i++ )    \
        if ( (pObj) == NULL ) {} else
#define Abc_NtkForEachNet( pNtk, pNet, i )                                                         \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vObjs)) && (((pNet) = Abc_NtkObj(pNtk, i)), 1); i++ )    \
        if ( (pNet) == NULL || !Abc_ObjIsNet(pNet) ) {} else
#define Abc_NtkForEachNode( pNtk, pNode, i )                                                       \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vObjs)) && (((pNode) = Abc_NtkObj(pNtk, i)), 1); i++ )   \
        if ( (pNode) == NULL || !Abc_ObjIsNode(pNode) ) {} else
#define Abc_NtkForEachGate( pNtk, pNode, i )                                                       \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vObjs)) && (((pNode) = Abc_NtkObj(pNtk, i)), 1); i++ )   \
        if ( (pNode) == NULL || !Abc_ObjIsGate(pNode) ) {} else
#define Abc_AigForEachAnd( pNtk, pNode, i )                                                        \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vObjs)) && (((pNode) = Abc_NtkObj(pNtk, i)), 1); i++ )   \
        if ( (pNode) == NULL || !Abc_AigNodeIsAnd(pNode) ) {} else
// various boxes
#define Abc_NtkForEachBox( pNtk, pObj, i )                                                         \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vBoxes)) && (((pObj) = Abc_NtkBox(pNtk, i)), 1); i++ )
#define Abc_NtkForEachLatch( pNtk, pObj, i )                                                       \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vBoxes)) && (((pObj) = Abc_NtkBox(pNtk, i)), 1); i++ )   \
        if ( !Abc_ObjIsLatch(pObj) ) {} else
#define Abc_NtkForEachLatchInput( pNtk, pObj, i )                                                  \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vBoxes)); i++ )                                          \
        if ( !(Abc_ObjIsLatch(Abc_NtkBox(pNtk, i)) && (((pObj) = Abc_ObjFanin0(Abc_NtkBox(pNtk, i))), 1)) ) {} else
#define Abc_NtkForEachLatchOutput( pNtk, pObj, i )                                                 \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vBoxes)); i++ )                                          \
        if ( !(Abc_ObjIsLatch(Abc_NtkBox(pNtk, i)) && (((pObj) = Abc_ObjFanout0(Abc_NtkBox(pNtk, i))), 1)) ) {} else
#define Abc_NtkForEachWhitebox( pNtk, pObj, i )                                                    \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vBoxes)) && (((pObj) = Abc_NtkBox(pNtk, i)), 1); i++ )   \
        if ( !Abc_ObjIsWhitebox(pObj) ) {} else
#define Abc_NtkForEachBlackbox( pNtk, pObj, i )                                                    \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vBoxes)) && (((pObj) = Abc_NtkBox(pNtk, i)), 1); i++ )   \
        if ( !Abc_ObjIsBlackbox(pObj) ) {} else
// inputs and outputs
#define Abc_NtkForEachPi( pNtk, pPi, i )                                                           \
    for ( i = 0; (i < Abc_NtkPiNum(pNtk)) && (((pPi) = Abc_NtkPi(pNtk, i)), 1); i++ )
#define Abc_NtkForEachCi( pNtk, pCi, i )                                                           \
    for ( i = 0; (i < Abc_NtkCiNum(pNtk)) && (((pCi) = Abc_NtkCi(pNtk, i)), 1); i++ )
#define Abc_NtkForEachPo( pNtk, pPo, i )                                                           \
    for ( i = 0; (i < Abc_NtkPoNum(pNtk)) && (((pPo) = Abc_NtkPo(pNtk, i)), 1); i++ )
#define Abc_NtkForEachCo( pNtk, pCo, i )                                                           \
    for ( i = 0; (i < Abc_NtkCoNum(pNtk)) && (((pCo) = Abc_NtkCo(pNtk, i)), 1); i++ )
#define Abc_NtkForEachAssert( pNtk, pObj, i )                                                      \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vAsserts)) && (((pObj) = Abc_NtkAssert(pNtk, i)), 1); i++ )
// fanin and fanouts
#define Abc_ObjForEachFanin( pObj, pFanin, i )                                                     \
    for ( i = 0; (i < Abc_ObjFaninNum(pObj)) && (((pFanin) = Abc_ObjFanin(pObj, i)), 1); i++ )
#define Abc_ObjForEachFanout( pObj, pFanout, i )                                                   \
    for ( i = 0; (i < Abc_ObjFanoutNum(pObj)) && (((pFanout) = Abc_ObjFanout(pObj, i)), 1); i++ )
// cubes and literals
#define Abc_SopForEachCube( pSop, nFanins, pCube )                                                 \
    for ( pCube = (pSop); *pCube; pCube += (nFanins) + 3 )
#define Abc_CubeForEachVar( pCube, Value, i )                                                      \
    for ( i = 0; (pCube[i] != ' ') && (Value = pCube[i]); i++ )           


////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== abcAig.c ==========================================================*/
extern Abc_Aig_t *        Abc_AigAlloc( Abc_Ntk_t * pNtk );
extern void               Abc_AigFree( Abc_Aig_t * pMan );
extern int                Abc_AigCleanup( Abc_Aig_t * pMan );
extern bool               Abc_AigCheck( Abc_Aig_t * pMan );
extern int                Abc_AigLevel( Abc_Ntk_t * pNtk );
extern Abc_Obj_t *        Abc_AigConst1( Abc_Ntk_t * pNtk );
extern Abc_Obj_t *        Abc_AigAnd( Abc_Aig_t * pMan, Abc_Obj_t * p0, Abc_Obj_t * p1 );
extern Abc_Obj_t *        Abc_AigAndLookup( Abc_Aig_t * pMan, Abc_Obj_t * p0, Abc_Obj_t * p1 );
extern Abc_Obj_t *        Abc_AigXorLookup( Abc_Aig_t * pMan, Abc_Obj_t * p0, Abc_Obj_t * p1, int * pType );
extern Abc_Obj_t *        Abc_AigMuxLookup( Abc_Aig_t * pMan, Abc_Obj_t * pC, Abc_Obj_t * pT, Abc_Obj_t * pE, int * pType );
extern Abc_Obj_t *        Abc_AigOr( Abc_Aig_t * pMan, Abc_Obj_t * p0, Abc_Obj_t * p1 );
extern Abc_Obj_t *        Abc_AigXor( Abc_Aig_t * pMan, Abc_Obj_t * p0, Abc_Obj_t * p1 );
extern Abc_Obj_t *        Abc_AigMiter( Abc_Aig_t * pMan, Vec_Ptr_t * vPairs );
extern void               Abc_AigReplace( Abc_Aig_t * pMan, Abc_Obj_t * pOld, Abc_Obj_t * pNew, bool fUpdateLevel );
extern void               Abc_AigDeleteNode( Abc_Aig_t * pMan, Abc_Obj_t * pOld );
extern void               Abc_AigRehash( Abc_Aig_t * pMan );
extern bool               Abc_AigNodeHasComplFanoutEdge( Abc_Obj_t * pNode );
extern bool               Abc_AigNodeHasComplFanoutEdgeTrav( Abc_Obj_t * pNode );
extern void               Abc_AigPrintNode( Abc_Obj_t * pNode );
extern bool               Abc_AigNodeIsAcyclic( Abc_Obj_t * pNode, Abc_Obj_t * pRoot );
extern void               Abc_AigCheckFaninOrder( Abc_Aig_t * pMan );
extern void               Abc_AigSetNodePhases( Abc_Ntk_t * pNtk );
extern Vec_Ptr_t *        Abc_AigUpdateStart( Abc_Aig_t * pMan, Vec_Ptr_t ** pvUpdatedNets );
extern void               Abc_AigUpdateStop( Abc_Aig_t * pMan );
extern void               Abc_AigUpdateReset( Abc_Aig_t * pMan );
/*=== abcAttach.c ==========================================================*/
extern int                Abc_NtkAttach( Abc_Ntk_t * pNtk );
/*=== abcBlifMv.c ==========================================================*/
extern void               Abc_NtkStartMvVars( Abc_Ntk_t * pNtk );
extern void               Abc_NtkFreeMvVars( Abc_Ntk_t * pNtk );
extern void               Abc_NtkSetMvVarValues( Abc_Obj_t * pObj, int nValues );
extern Abc_Ntk_t *        Abc_NtkStrashBlifMv( Abc_Ntk_t * pNtk );
extern Abc_Ntk_t *        Abc_NtkInsertBlifMv( Abc_Ntk_t * pNtkBase, Abc_Ntk_t * pNtkLogic );
extern int                Abc_NtkConvertToBlifMv( Abc_Ntk_t * pNtk );
extern char *             Abc_NodeConvertSopToMvSop( int nVars, Vec_Int_t * vSop0, Vec_Int_t * vSop1 );
extern int                Abc_NodeEvalMvCost( int nVars, Vec_Int_t * vSop0, Vec_Int_t * vSop1 );
/*=== abcBalance.c ==========================================================*/
extern Abc_Ntk_t *        Abc_NtkBalance( Abc_Ntk_t * pNtk, bool fDuplicate, bool fSelective, bool fUpdateLevel );
/*=== abcCheck.c ==========================================================*/
extern bool               Abc_NtkCheck( Abc_Ntk_t * pNtk );
extern bool               Abc_NtkCheckRead( Abc_Ntk_t * pNtk );
extern bool               Abc_NtkDoCheck( Abc_Ntk_t * pNtk );
extern bool               Abc_NtkCheckObj( Abc_Ntk_t * pNtk, Abc_Obj_t * pObj );
extern bool               Abc_NtkCompareSignals( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int fOnlyPis, int fComb );
extern int                Abc_NtkIsAcyclicHierarchy( Abc_Ntk_t * pNtk );
extern int                Abc_NtkCheckUniqueCiNames( Abc_Ntk_t * pNtk );
extern int                Abc_NtkCheckUniqueCoNames( Abc_Ntk_t * pNtk );
extern int                Abc_NtkCheckUniqueCioNames( Abc_Ntk_t * pNtk );
/*=== abcCollapse.c ==========================================================*/
extern Abc_Ntk_t *        Abc_NtkCollapse( Abc_Ntk_t * pNtk, int fBddSizeMax, int fDualRail, int fReorder, int fVerbose );
/*=== abcCut.c ==========================================================*/
extern void *             Abc_NodeGetCutsRecursive( void * p, Abc_Obj_t * pObj, int fDag, int fTree );
extern void *             Abc_NodeGetCuts( void * p, Abc_Obj_t * pObj, int fDag, int fTree );
extern void               Abc_NodeGetCutsSeq( void * p, Abc_Obj_t * pObj, int fFirst );
extern void *             Abc_NodeReadCuts( void * p, Abc_Obj_t * pObj );
extern void               Abc_NodeFreeCuts( void * p, Abc_Obj_t * pObj );
/*=== abcDfs.c ==========================================================*/
extern Vec_Ptr_t *        Abc_NtkDfs( Abc_Ntk_t * pNtk, int fCollectAll );
extern Vec_Ptr_t *        Abc_NtkDfsNodes( Abc_Ntk_t * pNtk, Abc_Obj_t ** ppNodes, int nNodes );
extern Vec_Ptr_t *        Abc_NtkDfsReverse( Abc_Ntk_t * pNtk );
extern Vec_Ptr_t *        Abc_NtkDfsReverseNodes( Abc_Ntk_t * pNtk, Abc_Obj_t ** ppNodes, int nNodes );
extern Vec_Ptr_t *        Abc_NtkDfsReverseNodesContained( Abc_Ntk_t * pNtk, Abc_Obj_t ** ppNodes, int nNodes );
extern Vec_Ptr_t *        Abc_NtkDfsSeq( Abc_Ntk_t * pNtk );
extern Vec_Ptr_t *        Abc_NtkDfsSeqReverse( Abc_Ntk_t * pNtk );
extern Vec_Ptr_t *        Abc_NtkDfsIter( Abc_Ntk_t * pNtk, int fCollectAll );
extern Vec_Ptr_t *        Abc_NtkDfsHie( Abc_Ntk_t * pNtk, int fCollectAll );
extern bool               Abc_NtkIsDfsOrdered( Abc_Ntk_t * pNtk );
extern Vec_Ptr_t *        Abc_NtkSupport( Abc_Ntk_t * pNtk );
extern Vec_Ptr_t *        Abc_NtkNodeSupport( Abc_Ntk_t * pNtk, Abc_Obj_t ** ppNodes, int nNodes );
extern Vec_Ptr_t *        Abc_AigDfs( Abc_Ntk_t * pNtk, int fCollectAll, int fCollectCos );
extern Vec_Vec_t *        Abc_DfsLevelized( Abc_Obj_t * pNode, bool fTfi );
extern int                Abc_NtkLevel( Abc_Ntk_t * pNtk );
extern int                Abc_NtkLevelReverse( Abc_Ntk_t * pNtk );
extern bool               Abc_NtkIsAcyclic( Abc_Ntk_t * pNtk );
extern Vec_Ptr_t *        Abc_AigGetLevelizedOrder( Abc_Ntk_t * pNtk, int fCollectCis );
/*=== abcFanio.c ==========================================================*/
extern void               Abc_ObjAddFanin( Abc_Obj_t * pObj, Abc_Obj_t * pFanin );
extern void               Abc_ObjDeleteFanin( Abc_Obj_t * pObj, Abc_Obj_t * pFanin );
extern void               Abc_ObjRemoveFanins( Abc_Obj_t * pObj );
extern void               Abc_ObjPatchFanin( Abc_Obj_t * pObj, Abc_Obj_t * pFaninOld, Abc_Obj_t * pFaninNew );
extern Abc_Obj_t *        Abc_ObjInsertBetween( Abc_Obj_t * pNodeIn, Abc_Obj_t * pNodeOut, Abc_ObjType_t Type );
extern void               Abc_ObjTransferFanout( Abc_Obj_t * pObjOld, Abc_Obj_t * pObjNew );
extern void               Abc_ObjReplace( Abc_Obj_t * pObjOld, Abc_Obj_t * pObjNew );
extern int                Abc_ObjFanoutFaninNum( Abc_Obj_t * pFanout, Abc_Obj_t * pFanin );
/*=== abcFraig.c ==========================================================*/
extern Abc_Ntk_t *        Abc_NtkFraig( Abc_Ntk_t * pNtk, void * pParams, int fAllNodes, int fExdc );
extern void *             Abc_NtkToFraig( Abc_Ntk_t * pNtk, void * pParams, int fAllNodes, int fExdc );
extern Abc_Ntk_t *        Abc_NtkFraigTrust( Abc_Ntk_t * pNtk );
extern int                Abc_NtkFraigStore( Abc_Ntk_t * pNtk );
extern Abc_Ntk_t *        Abc_NtkFraigRestore();
extern void               Abc_NtkFraigStoreClean();
/*=== abcFunc.c ==========================================================*/
extern int                Abc_NtkSopToBdd( Abc_Ntk_t * pNtk );
extern DdNode *           Abc_ConvertSopToBdd( DdManager * dd, char * pSop );
extern char *             Abc_ConvertBddToSop( Extra_MmFlex_t * pMan, DdManager * dd, DdNode * bFuncOn, DdNode * bFuncOnDc, int nFanins, int fAllPrimes, Vec_Str_t * vCube, int fMode );
extern int                Abc_NtkBddToSop( Abc_Ntk_t * pNtk, int fDirect );
extern void               Abc_NodeBddToCnf( Abc_Obj_t * pNode, Extra_MmFlex_t * pMmMan, Vec_Str_t * vCube, int fAllPrimes, char ** ppSop0, char ** ppSop1 );
extern int                Abc_CountZddCubes( DdManager * dd, DdNode * zCover );
extern void               Abc_NtkLogicMakeDirectSops( Abc_Ntk_t * pNtk );
extern int                Abc_NtkSopToAig( Abc_Ntk_t * pNtk );
extern int                Abc_NtkAigToBdd( Abc_Ntk_t * pNtk );
extern unsigned *         Abc_ConvertAigToTruth( Hop_Man_t * p, Hop_Obj_t * pRoot, int nVars, Vec_Int_t * vTruth, int fMsbFirst );
extern int                Abc_NtkMapToSop( Abc_Ntk_t * pNtk );
extern int                Abc_NtkToSop( Abc_Ntk_t * pNtk, int fDirect );
extern int                Abc_NtkToBdd( Abc_Ntk_t * pNtk );
extern int                Abc_NtkToAig( Abc_Ntk_t * pNtk );
/*=== abcHaig.c ==========================================================*/
extern int                Abc_NtkHaigStart( Abc_Ntk_t * pNtk );
extern int                Abc_NtkHaigStop( Abc_Ntk_t * pNtk );
extern Abc_Ntk_t *        Abc_NtkHaigUse( Abc_Ntk_t * pNtk );
/*=== abcHie.c ==========================================================*/
extern Abc_Ntk_t *        Abc_NtkFlattenLogicHierarchy( Abc_Ntk_t * pNtk );
extern Abc_Ntk_t *        Abc_NtkConvertBlackboxes( Abc_Ntk_t * pNtk );
extern Abc_Ntk_t *        Abc_NtkInsertNewLogic( Abc_Ntk_t * pNtkH, Abc_Ntk_t * pNtkL );
/*=== abcLatch.c ==========================================================*/
extern bool               Abc_NtkLatchIsSelfFeed( Abc_Obj_t * pLatch );
extern int                Abc_NtkCountSelfFeedLatches( Abc_Ntk_t * pNtk );
extern int                Abc_NtkRemoveSelfFeedLatches( Abc_Ntk_t * pNtk );
extern Vec_Int_t *        Abc_NtkCollectLatchValues( Abc_Ntk_t * pNtk );
extern void               Abc_NtkInsertLatchValues( Abc_Ntk_t * pNtk, Vec_Int_t * vValues );
 /*=== abcLib.c ==========================================================*/
extern Abc_Lib_t *        Abc_LibCreate( char * pName );
extern void               Abc_LibFree( Abc_Lib_t * pLib, Abc_Ntk_t * pNtk );
extern void               Abc_LibPrint( Abc_Lib_t * pLib );
extern int                Abc_LibAddModel( Abc_Lib_t * pLib, Abc_Ntk_t * pNtk );
extern Abc_Ntk_t *        Abc_LibFindModelByName( Abc_Lib_t * pLib, char * pName );
extern int                Abc_LibFindTopLevelModels( Abc_Lib_t * pLib );
extern Abc_Ntk_t *        Abc_LibDeriveRoot( Abc_Lib_t * pLib );
/*=== abcMiter.c ==========================================================*/
extern int                Abc_NtkMinimumBase( Abc_Ntk_t * pNtk );
extern int                Abc_NodeMinimumBase( Abc_Obj_t * pNode );
extern int                Abc_NtkRemoveDupFanins( Abc_Ntk_t * pNtk );
extern int                Abc_NodeRemoveDupFanins( Abc_Obj_t * pNode );
/*=== abcMiter.c ==========================================================*/
extern Abc_Ntk_t *        Abc_NtkMiter( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int fComb, int nPartSize );
extern void               Abc_NtkMiterAddCone( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkMiter, Abc_Obj_t * pNode );
extern Abc_Ntk_t *        Abc_NtkMiterAnd( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int fOr, int fCompl2 );
extern Abc_Ntk_t *        Abc_NtkMiterCofactor( Abc_Ntk_t * pNtk, Vec_Int_t * vPiValues );
extern Abc_Ntk_t *        Abc_NtkMiterForCofactors( Abc_Ntk_t * pNtk, int Out, int In1, int In2 );
extern Abc_Ntk_t *        Abc_NtkMiterQuantify( Abc_Ntk_t * pNtk, int In, int fExist );
extern Abc_Ntk_t *        Abc_NtkMiterQuantifyPis( Abc_Ntk_t * pNtk );
extern int                Abc_NtkMiterIsConstant( Abc_Ntk_t * pMiter );
extern void               Abc_NtkMiterReport( Abc_Ntk_t * pMiter );
extern Abc_Ntk_t *        Abc_NtkFrames( Abc_Ntk_t * pNtk, int nFrames, int fInitial );
/*=== abcObj.c ==========================================================*/
extern Abc_Obj_t *        Abc_ObjAlloc( Abc_Ntk_t * pNtk, Abc_ObjType_t Type );
extern void               Abc_ObjRecycle( Abc_Obj_t * pObj );
extern Abc_Obj_t *        Abc_NtkCreateObj( Abc_Ntk_t * pNtk, Abc_ObjType_t Type );
extern void               Abc_NtkDeleteObj( Abc_Obj_t * pObj );
extern void               Abc_NtkDeleteObj_rec( Abc_Obj_t * pObj, int fOnlyNodes );
extern void               Abc_NtkDeleteAll_rec( Abc_Obj_t * pObj );
extern Abc_Obj_t *        Abc_NtkDupObj( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pObj, int fCopyName );
extern Abc_Obj_t *        Abc_NtkDupBox( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pBox, int fCopyName );
extern Abc_Obj_t *        Abc_NtkCloneObj( Abc_Obj_t * pNode );
extern Abc_Obj_t *        Abc_NtkFindNode( Abc_Ntk_t * pNtk, char * pName );
extern Abc_Obj_t *        Abc_NtkFindNet( Abc_Ntk_t * pNtk, char * pName );
extern Abc_Obj_t *        Abc_NtkFindCi( Abc_Ntk_t * pNtk, char * pName );
extern Abc_Obj_t *        Abc_NtkFindCo( Abc_Ntk_t * pNtk, char * pName );
extern Abc_Obj_t *        Abc_NtkFindOrCreateNet( Abc_Ntk_t * pNtk, char * pName );
extern Abc_Obj_t *        Abc_NtkCreateNodeConst0( Abc_Ntk_t * pNtk );
extern Abc_Obj_t *        Abc_NtkCreateNodeConst1( Abc_Ntk_t * pNtk );
extern Abc_Obj_t *        Abc_NtkCreateNodeInv( Abc_Ntk_t * pNtk, Abc_Obj_t * pFanin );
extern Abc_Obj_t *        Abc_NtkCreateNodeBuf( Abc_Ntk_t * pNtk, Abc_Obj_t * pFanin );
extern Abc_Obj_t *        Abc_NtkCreateNodeAnd( Abc_Ntk_t * pNtk, Vec_Ptr_t * vFanins );
extern Abc_Obj_t *        Abc_NtkCreateNodeOr( Abc_Ntk_t * pNtk, Vec_Ptr_t * vFanins );
extern Abc_Obj_t *        Abc_NtkCreateNodeExor( Abc_Ntk_t * pNtk, Vec_Ptr_t * vFanins );
extern Abc_Obj_t *        Abc_NtkCreateNodeMux( Abc_Ntk_t * pNtk, Abc_Obj_t * pNodeC, Abc_Obj_t * pNode1, Abc_Obj_t * pNode0 );
extern bool               Abc_NodeIsConst( Abc_Obj_t * pNode );
extern bool               Abc_NodeIsConst0( Abc_Obj_t * pNode );    
extern bool               Abc_NodeIsConst1( Abc_Obj_t * pNode );    
extern bool               Abc_NodeIsBuf( Abc_Obj_t * pNode );    
extern bool               Abc_NodeIsInv( Abc_Obj_t * pNode );    
extern void               Abc_NodeComplement( Abc_Obj_t * pNode );
/*=== abcNames.c ====================================================*/
extern char *             Abc_ObjName( Abc_Obj_t * pNode );
extern char *             Abc_ObjAssignName( Abc_Obj_t * pObj, char * pName, char * pSuffix );
extern char *             Abc_ObjNameSuffix( Abc_Obj_t * pObj, char * pSuffix );
extern char *             Abc_ObjNameDummy( char * pPrefix, int Num, int nDigits );
extern void               Abc_NtkTrasferNames( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkNew );
extern void               Abc_NtkTrasferNamesNoLatches( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkNew );
extern Vec_Ptr_t *        Abc_NodeGetFaninNames( Abc_Obj_t * pNode );
extern Vec_Ptr_t *        Abc_NodeGetFakeNames( int nNames );
extern void               Abc_NodeFreeNames( Vec_Ptr_t * vNames );
extern char **            Abc_NtkCollectCioNames( Abc_Ntk_t * pNtk, int fCollectCos );
extern int                Abc_NodeCompareNames( Abc_Obj_t ** pp1, Abc_Obj_t ** pp2 );
extern void               Abc_NtkOrderObjsByName( Abc_Ntk_t * pNtk, int fComb );
extern void               Abc_NtkAddDummyPiNames( Abc_Ntk_t * pNtk );
extern void               Abc_NtkAddDummyPoNames( Abc_Ntk_t * pNtk );
extern void               Abc_NtkAddDummyAssertNames( Abc_Ntk_t * pNtk );
extern void               Abc_NtkAddDummyBoxNames( Abc_Ntk_t * pNtk );
extern void               Abc_NtkShortNames( Abc_Ntk_t * pNtk );
/*=== abcNetlist.c ==========================================================*/
extern Abc_Ntk_t *        Abc_NtkToLogic( Abc_Ntk_t * pNtk );
extern Abc_Ntk_t *        Abc_NtkToNetlist( Abc_Ntk_t * pNtk );
extern Abc_Ntk_t *        Abc_NtkToNetlistBench( Abc_Ntk_t * pNtk );
/*=== abcNtbdd.c ==========================================================*/
extern Abc_Ntk_t *        Abc_NtkDeriveFromBdd( DdManager * dd, DdNode * bFunc, char * pNamePo, Vec_Ptr_t * vNamesPi );
extern Abc_Ntk_t *        Abc_NtkBddToMuxes( Abc_Ntk_t * pNtk );
extern DdManager *        Abc_NtkBuildGlobalBdds( Abc_Ntk_t * pNtk, int fBddSizeMax, int fDropInternal, int fReorder, int fVerbose );
extern DdManager *        Abc_NtkFreeGlobalBdds( Abc_Ntk_t * pNtk, int fFreeMan );
extern int                Abc_NtkSizeOfGlobalBdds( Abc_Ntk_t * pNtk );
/*=== abcNtk.c ==========================================================*/
extern Abc_Ntk_t *        Abc_NtkAlloc( Abc_NtkType_t Type, Abc_NtkFunc_t Func, int fUseMemMan );
extern Abc_Ntk_t *        Abc_NtkStartFrom( Abc_Ntk_t * pNtk, Abc_NtkType_t Type, Abc_NtkFunc_t Func );
extern Abc_Ntk_t *        Abc_NtkStartFromNoLatches( Abc_Ntk_t * pNtk, Abc_NtkType_t Type, Abc_NtkFunc_t Func );
extern void               Abc_NtkFinalize( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkNew );
extern Abc_Ntk_t *        Abc_NtkStartRead( char * pName );
extern void               Abc_NtkFinalizeRead( Abc_Ntk_t * pNtk );
extern Abc_Ntk_t *        Abc_NtkDup( Abc_Ntk_t * pNtk );
extern Abc_Ntk_t *        Abc_NtkCreateCone( Abc_Ntk_t * pNtk, Abc_Obj_t * pNode, char * pNodeName, int fUseAllCis );
extern Abc_Ntk_t *        Abc_NtkCreateConeArray( Abc_Ntk_t * pNtk, Vec_Ptr_t * vRoots, int fUseAllCis );
extern Abc_Ntk_t *        Abc_NtkCreateMffc( Abc_Ntk_t * pNtk, Abc_Obj_t * pNode, char * pNodeName );
extern Abc_Ntk_t *        Abc_NtkCreateTarget( Abc_Ntk_t * pNtk, Vec_Ptr_t * vRoots, Vec_Int_t * vValues );
extern Abc_Ntk_t *        Abc_NtkCreateFromNode( Abc_Ntk_t * pNtk, Abc_Obj_t * pNode );
extern Abc_Ntk_t *        Abc_NtkCreateWithNode( char * pSop );
extern void               Abc_NtkDelete( Abc_Ntk_t * pNtk );
extern void               Abc_NtkFixNonDrivenNets( Abc_Ntk_t * pNtk );
extern void               Abc_NtkMakeComb( Abc_Ntk_t * pNtk );
/*=== abcPrint.c ==========================================================*/
extern void               Abc_NtkPrintStats( FILE * pFile, Abc_Ntk_t * pNtk, int fFactored );
extern void               Abc_NtkPrintIo( FILE * pFile, Abc_Ntk_t * pNtk );
extern void               Abc_NtkPrintLatch( FILE * pFile, Abc_Ntk_t * pNtk );
extern void               Abc_NtkPrintFanio( FILE * pFile, Abc_Ntk_t * pNtk );
extern void               Abc_NodePrintFanio( FILE * pFile, Abc_Obj_t * pNode );
extern void               Abc_NtkPrintFactor( FILE * pFile, Abc_Ntk_t * pNtk, int fUseRealNames );
extern void               Abc_NodePrintFactor( FILE * pFile, Abc_Obj_t * pNode, int fUseRealNames );
extern void               Abc_NtkPrintLevel( FILE * pFile, Abc_Ntk_t * pNtk, int fProfile, int fListNodes );
extern void               Abc_NodePrintLevel( FILE * pFile, Abc_Obj_t * pNode );
extern void               Abc_NtkPrintSkews( FILE * pFile, Abc_Ntk_t * pNtk, int fPrintAll );
extern void               Abc_ObjPrint( FILE * pFile, Abc_Obj_t * pObj );
/*=== abcProve.c ==========================================================*/
extern int                Abc_NtkMiterProve( Abc_Ntk_t ** ppNtk, void * pParams );
extern int                Abc_NtkIvyProve( Abc_Ntk_t ** ppNtk, void * pPars );
/*=== abcRec.c ==========================================================*/
extern void               Abc_NtkRecStart( Abc_Ntk_t * pNtk, int nVars, int nCuts );
extern void               Abc_NtkRecStop();
extern void               Abc_NtkRecAdd( Abc_Ntk_t * pNtk );
extern void               Abc_NtkRecPs();
extern void               Abc_NtkRecFilter( int iVar, int iPlus );
extern Abc_Ntk_t *        Abc_NtkRecUse();
extern int                Abc_NtkRecIsRunning();
extern int                Abc_NtkRecVarNum();
extern Vec_Int_t *        Abc_NtkRecMemory();
extern int                Abc_NtkRecStrashNode( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pObj, unsigned * pTruth, int nVars );
/*=== abcReconv.c ==========================================================*/
extern Abc_ManCut_t *     Abc_NtkManCutStart( int nNodeSizeMax, int nConeSizeMax, int nNodeFanStop, int nConeFanStop );
extern void               Abc_NtkManCutStop( Abc_ManCut_t * p );
extern Vec_Ptr_t *        Abc_NtkManCutReadCutLarge( Abc_ManCut_t * p );
extern Vec_Ptr_t *        Abc_NtkManCutReadCutSmall( Abc_ManCut_t * p );
extern Vec_Ptr_t *        Abc_NtkManCutReadVisited( Abc_ManCut_t * p );
extern Vec_Ptr_t *        Abc_NodeFindCut( Abc_ManCut_t * p, Abc_Obj_t * pRoot, bool fContain );
extern void               Abc_NodeConeCollect( Abc_Obj_t ** ppRoots, int nRoots, Vec_Ptr_t * vFanins, Vec_Ptr_t * vVisited, int fIncludeFanins );
extern DdNode *           Abc_NodeConeBdd( DdManager * dd, DdNode ** pbVars, Abc_Obj_t * pNode, Vec_Ptr_t * vFanins, Vec_Ptr_t * vVisited );
extern DdNode *           Abc_NodeConeDcs( DdManager * dd, DdNode ** pbVarsX, DdNode ** pbVarsY, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vRoots, Vec_Ptr_t * vVisited );
extern Vec_Ptr_t *        Abc_NodeCollectTfoCands( Abc_ManCut_t * p, Abc_Obj_t * pRoot, Vec_Ptr_t * vFanins, int LevelMax );
/*=== abcRefs.c ==========================================================*/
extern int                Abc_NodeMffcSize( Abc_Obj_t * pNode );
extern int                Abc_NodeMffcSizeSupp( Abc_Obj_t * pNode );
extern int                Abc_NodeMffcSizeStop( Abc_Obj_t * pNode );
extern int                Abc_NodeMffcLabelAig( Abc_Obj_t * pNode );
extern int                Abc_NodeMffcLabel( Abc_Obj_t * pNode );
extern void               Abc_NodeMffsConeSupp( Abc_Obj_t * pNode, Vec_Ptr_t * vCone, Vec_Ptr_t * vSupp );
extern int                Abc_NodeDeref_rec( Abc_Obj_t * pNode );
extern int                Abc_NodeRef_rec( Abc_Obj_t * pNode );
/*=== abcRefactor.c ==========================================================*/
extern int                Abc_NtkRefactor( Abc_Ntk_t * pNtk, int nNodeSizeMax, int nConeSizeMax, bool fUpdateLevel, bool fUseZeros, bool fUseDcs, bool fVerbose );
/*=== abcRewrite.c ==========================================================*/
extern int                Abc_NtkRewrite( Abc_Ntk_t * pNtk, int fUpdateLevel, int fUseZeros, int fVerbose, int fVeryVerbose, int fPlaceEnable );
/*=== abcSat.c ==========================================================*/
extern int                Abc_NtkMiterSat( Abc_Ntk_t * pNtk, sint64 nConfLimit, sint64 nInsLimit, int fVerbose, sint64 * pNumConfs, sint64 * pNumInspects );
extern void *             Abc_NtkMiterSatCreate( Abc_Ntk_t * pNtk, int fAllPrimes );
/*=== abcSop.c ==========================================================*/
extern char *             Abc_SopRegister( Extra_MmFlex_t * pMan, char * pName );
extern char *             Abc_SopStart( Extra_MmFlex_t * pMan, int nCubes, int nVars );
extern char *             Abc_SopCreateConst0( Extra_MmFlex_t * pMan );
extern char *             Abc_SopCreateConst1( Extra_MmFlex_t * pMan );
extern char *             Abc_SopCreateAnd2( Extra_MmFlex_t * pMan, int fCompl0, int fCompl1 );
extern char *             Abc_SopCreateAnd( Extra_MmFlex_t * pMan, int nVars, int * pfCompl );
extern char *             Abc_SopCreateNand( Extra_MmFlex_t * pMan, int nVars );
extern char *             Abc_SopCreateOr( Extra_MmFlex_t * pMan, int nVars, int * pfCompl );
extern char *             Abc_SopCreateOrMultiCube( Extra_MmFlex_t * pMan, int nVars, int * pfCompl );
extern char *             Abc_SopCreateNor( Extra_MmFlex_t * pMan, int nVars );
extern char *             Abc_SopCreateXor( Extra_MmFlex_t * pMan, int nVars );
extern char *             Abc_SopCreateXorSpecial( Extra_MmFlex_t * pMan, int nVars );
extern char *             Abc_SopCreateNxor( Extra_MmFlex_t * pMan, int nVars );
extern char *             Abc_SopCreateMux( Extra_MmFlex_t * pMan );
extern char *             Abc_SopCreateInv( Extra_MmFlex_t * pMan );
extern char *             Abc_SopCreateBuf( Extra_MmFlex_t * pMan );
extern char *             Abc_SopCreateFromTruth( Extra_MmFlex_t * pMan, int nVars, unsigned * pTruth );
extern char *             Abc_SopCreateFromIsop( Extra_MmFlex_t * pMan, int nVars, Vec_Int_t * vCover );
extern int                Abc_SopGetCubeNum( char * pSop );
extern int                Abc_SopGetLitNum( char * pSop );
extern int                Abc_SopGetVarNum( char * pSop );
extern int                Abc_SopGetPhase( char * pSop );
extern int                Abc_SopGetIthCareLit( char * pSop, int i );
extern void               Abc_SopComplement( char * pSop );
extern bool               Abc_SopIsComplement( char * pSop );
extern bool               Abc_SopIsConst0( char * pSop );
extern bool               Abc_SopIsConst1( char * pSop );
extern bool               Abc_SopIsBuf( char * pSop );
extern bool               Abc_SopIsInv( char * pSop );
extern bool               Abc_SopIsAndType( char * pSop );
extern bool               Abc_SopIsOrType( char * pSop );
extern int                Abc_SopIsExorType( char * pSop );
extern bool               Abc_SopCheck( char * pSop, int nFanins );
extern char *             Abc_SopFromTruthBin( char * pTruth );
extern char *             Abc_SopFromTruthHex( char * pTruth );
extern char *             Abc_SopEncoderPos( Extra_MmFlex_t * pMan, int iValue, int nValues );
extern char *             Abc_SopEncoderLog( Extra_MmFlex_t * pMan, int iBit, int nValues );
extern char *             Abc_SopDecoderPos( Extra_MmFlex_t * pMan, int nValues );
extern char *             Abc_SopDecoderLog( Extra_MmFlex_t * pMan, int nValues );
/*=== abcStrash.c ==========================================================*/
extern Abc_Ntk_t *        Abc_NtkStrash( Abc_Ntk_t * pNtk, int fAllNodes, int fCleanup, int fRecord );
extern Abc_Obj_t *        Abc_NodeStrash( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pNode, int fRecord );
extern int                Abc_NtkAppend( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int fAddPos );
extern Abc_Ntk_t *        Abc_NtkTopmost( Abc_Ntk_t * pNtk, int nLevels );
/*=== abcSweep.c ==========================================================*/
extern int                Abc_NtkSweep( Abc_Ntk_t * pNtk, int fVerbose );
extern int                Abc_NtkCleanup( Abc_Ntk_t * pNtk, int fVerbose );
extern int                Abc_NtkCleanupSeq( Abc_Ntk_t * pNtk, int fLatchSweep, int fAutoSweep, int fVerbose );
/*=== abcTiming.c ==========================================================*/
extern Abc_Time_t *       Abc_NodeReadArrival( Abc_Obj_t * pNode );
extern Abc_Time_t *       Abc_NodeReadRequired( Abc_Obj_t * pNode );
extern Abc_Time_t *       Abc_NtkReadDefaultArrival( Abc_Ntk_t * pNtk );
extern Abc_Time_t *       Abc_NtkReadDefaultRequired( Abc_Ntk_t * pNtk );
extern void               Abc_NtkTimeSetDefaultArrival( Abc_Ntk_t * pNtk, float Rise, float Fall );
extern void               Abc_NtkTimeSetDefaultRequired( Abc_Ntk_t * pNtk, float Rise, float Fall );
extern void               Abc_NtkTimeSetArrival( Abc_Ntk_t * pNtk, int ObjId, float Rise, float Fall );
extern void               Abc_NtkTimeSetRequired( Abc_Ntk_t * pNtk, int ObjId, float Rise, float Fall );
extern void               Abc_NtkTimeInitialize( Abc_Ntk_t * pNtk );
extern void               Abc_ManTimeStop( Abc_ManTime_t * p );
extern void               Abc_ManTimeDup( Abc_Ntk_t * pNtkOld, Abc_Ntk_t * pNtkNew );
extern void               Abc_NtkSetNodeLevelsArrival( Abc_Ntk_t * pNtk );
extern float *            Abc_NtkGetCiArrivalFloats( Abc_Ntk_t * pNtk );
extern Abc_Time_t *       Abc_NtkGetCiArrivalTimes( Abc_Ntk_t * pNtk );
extern float              Abc_NtkDelayTrace( Abc_Ntk_t * pNtk );
extern int                Abc_ObjLevelNew( Abc_Obj_t * pObj );
extern int                Abc_ObjReverseLevelNew( Abc_Obj_t * pObj );
extern int                Abc_ObjRequiredLevel( Abc_Obj_t * pObj );
extern int                Abc_ObjReverseLevel( Abc_Obj_t * pObj );
extern void               Abc_ObjSetReverseLevel( Abc_Obj_t * pObj, int LevelR );
extern void               Abc_NtkStartReverseLevels( Abc_Ntk_t * pNtk, int nMaxLevelIncrease );
extern void               Abc_NtkStopReverseLevels( Abc_Ntk_t * pNtk );
extern void               Abc_NtkUpdateLevel( Abc_Obj_t * pObjNew, Vec_Vec_t * vLevels );
extern void               Abc_NtkUpdateReverseLevel( Abc_Obj_t * pObjNew, Vec_Vec_t * vLevels );
extern void               Abc_NtkUpdate( Abc_Obj_t * pObj, Abc_Obj_t * pObjNew, Vec_Vec_t * vLevels );
/*=== abcUtil.c ==========================================================*/
extern void *             Abc_NtkAttrFree( Abc_Ntk_t * pNtk, int Attr, int fFreeMan );
extern void               Abc_NtkIncrementTravId( Abc_Ntk_t * pNtk );
extern void               Abc_NtkOrderCisCos( Abc_Ntk_t * pNtk );
extern int                Abc_NtkGetCubeNum( Abc_Ntk_t * pNtk );
extern int                Abc_NtkGetLitNum( Abc_Ntk_t * pNtk );
extern int                Abc_NtkGetLitFactNum( Abc_Ntk_t * pNtk );
extern int                Abc_NtkGetBddNodeNum( Abc_Ntk_t * pNtk );
extern int                Abc_NtkGetAigNodeNum( Abc_Ntk_t * pNtk );
extern int                Abc_NtkGetClauseNum( Abc_Ntk_t * pNtk );
extern double             Abc_NtkGetMappedArea( Abc_Ntk_t * pNtk );
extern int                Abc_NtkGetExorNum( Abc_Ntk_t * pNtk );
extern int                Abc_NtkGetMuxNum( Abc_Ntk_t * pNtk );
extern int                Abc_NtkGetChoiceNum( Abc_Ntk_t * pNtk );
extern int                Abc_NtkGetFaninMax( Abc_Ntk_t * pNtk );
extern int                Abc_NtkGetTotalFanins( Abc_Ntk_t * pNtk );
extern void               Abc_NtkCleanCopy( Abc_Ntk_t * pNtk );
extern void               Abc_NtkCleanData( Abc_Ntk_t * pNtk );
extern void               Abc_NtkCleanEquiv( Abc_Ntk_t * pNtk );
extern int                Abc_NtkCountCopy( Abc_Ntk_t * pNtk );
extern Vec_Ptr_t *        Abc_NtkSaveCopy( Abc_Ntk_t * pNtk );
extern void               Abc_NtkLoadCopy( Abc_Ntk_t * pNtk, Vec_Ptr_t * vCopies );
extern void               Abc_NtkCleanNext( Abc_Ntk_t * pNtk );
extern void               Abc_NtkCleanMarkA( Abc_Ntk_t * pNtk );
extern Abc_Obj_t *        Abc_NodeFindCoFanout( Abc_Obj_t * pNode );
extern Abc_Obj_t *        Abc_NodeFindNonCoFanout( Abc_Obj_t * pNode );
extern Abc_Obj_t *        Abc_NodeHasUniqueCoFanout( Abc_Obj_t * pNode );
extern bool               Abc_NtkLogicHasSimpleCos( Abc_Ntk_t * pNtk );
extern int                Abc_NtkLogicMakeSimpleCos( Abc_Ntk_t * pNtk, bool fDuplicate );
extern void               Abc_VecObjPushUniqueOrderByLevel( Vec_Ptr_t * p, Abc_Obj_t * pNode );
extern bool               Abc_NodeIsExorType( Abc_Obj_t * pNode );
extern bool               Abc_NodeIsMuxType( Abc_Obj_t * pNode );
extern bool               Abc_NodeIsMuxControlType( Abc_Obj_t * pNode );
extern Abc_Obj_t *        Abc_NodeRecognizeMux( Abc_Obj_t * pNode, Abc_Obj_t ** ppNodeT, Abc_Obj_t ** ppNodeE );
extern int                Abc_NtkPrepareTwoNtks( FILE * pErr, Abc_Ntk_t * pNtk, char ** argv, int argc, Abc_Ntk_t ** ppNtk1, Abc_Ntk_t ** ppNtk2, int * pfDelete1, int * pfDelete2 );
extern void               Abc_NodeCollectFanins( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes );
extern void               Abc_NodeCollectFanouts( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes );
extern Vec_Ptr_t *        Abc_NtkCollectLatches( Abc_Ntk_t * pNtk );
extern int                Abc_NodeCompareLevelsIncrease( Abc_Obj_t ** pp1, Abc_Obj_t ** pp2 );
extern int                Abc_NodeCompareLevelsDecrease( Abc_Obj_t ** pp1, Abc_Obj_t ** pp2 );
extern Vec_Int_t *        Abc_NtkFanoutCounts( Abc_Ntk_t * pNtk );
extern Vec_Ptr_t *        Abc_NtkCollectObjects( Abc_Ntk_t * pNtk );
extern Vec_Int_t *        Abc_NtkGetCiIds( Abc_Ntk_t * pNtk );
extern void               Abc_NtkReassignIds( Abc_Ntk_t * pNtk );
extern int                Abc_ObjPointerCompare( void ** pp1, void ** pp2 );
extern void               Abc_NtkTransferCopy( Abc_Ntk_t * pNtk );
/*=== abcVerify.c ==========================================================*/
extern int *              Abc_NtkVerifyGetCleanModel( Abc_Ntk_t * pNtk, int nFrames );
extern int *              Abc_NtkVerifySimulatePattern( Abc_Ntk_t * pNtk, int * pModel );

#ifdef __cplusplus
}
#endif

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
