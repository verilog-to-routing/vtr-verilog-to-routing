/**CFile****************************************************************

  FileName    [abc.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abc.h,v 1.1 2008/05/14 22:13:11 wudenni Exp $]

***********************************************************************/

#ifndef ABC__base__abc__abc_h
#define ABC__base__abc__abc_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "misc/vec/vec.h"
#include "aig/hop/hop.h"
#include "aig/gia/gia.h"
#include "misc/st/st.h"
#include "misc/st/stmm.h"
#include "misc/nm/nm.h"
#include "misc/mem/mem.h"
#include "misc/util/utilCex.h"
#include "misc/extra/extra.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START


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
    ABC_OBJ_PI,         //  2:  primary input terminal
    ABC_OBJ_PO,         //  3:  primary output terminal
    ABC_OBJ_BI,         //  4:  box input terminal
    ABC_OBJ_BO,         //  5:  box output terminal
    ABC_OBJ_NET,        //  6:  net
    ABC_OBJ_NODE,       //  7:  node
    ABC_OBJ_LATCH,      //  8:  latch
    ABC_OBJ_WHITEBOX,   //  9:  box with known contents
    ABC_OBJ_BLACKBOX,   // 10:  box with unknown contents
    ABC_OBJ_NUMBER      // 11:  unused
} Abc_ObjType_t;

// latch initial values
typedef enum { 
    ABC_INIT_NONE = 0,  // 0:  unknown
    ABC_INIT_ZERO,      // 1:  zero
    ABC_INIT_ONE,       // 2:  one
    ABC_INIT_DC,        // 3:  don't-care
    ABC_INIT_OTHER      // 4:  unused
} Abc_InitType_t;

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Abc_Des_t_       Abc_Des_t;
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
};

struct Abc_Obj_t_     // 48/72 bytes (32-bits/64-bits)
{
    Abc_Ntk_t *       pNtk;          // the host network
    Abc_Obj_t *       pNext;         // the next pointer in the hash table
    int               Id;            // the object ID
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
    Vec_Int_t         vFanins;       // the array of fanins
    Vec_Int_t         vFanouts;      // the array of fanouts
    union { void *    pData;         // the network specific data
      int             iData; };      // (SOP, BDD, gate, equiv class, etc)
    union { void *    pTemp;         // temporary store for user's data
      Abc_Obj_t *     pCopy;         // the copy of this object
      int             iTemp;
      float           dTemp; };
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
    Vec_Ptr_t *       vBoxes;        // the array of boxes
    Vec_Ptr_t *       vLtlProperties;
    // the number of living objects
    int nObjCounts[ABC_OBJ_NUMBER];  // the number of objects by type
    int               nObjs;         // the number of live objs
    int               nConstrs;      // the number of constraints
    int               nBarBufs;      // the number of barrier buffers
    int               nBarBufs2;     // the number of barrier buffers
    // the backup network and the step number
    Abc_Ntk_t *       pNetBackup;    // the pointer to the previous backup network
    int               iStep;         // the generation number for the given network
    // hierarchy
    Abc_Des_t *       pDesign;       // design (hierarchical networks only)     
    Abc_Ntk_t *       pAltView;      // alternative structural view of the network
    int               fHieVisited;   // flag to mark the visited network
    int               fHiePath;      // flag to mark the network on the path
    int               Id;            // model ID
    double            dTemp;         // temporary value
    // miscellaneous data members
    int               nTravIds;      // the unique traversal IDs of nodes
    Vec_Int_t         vTravIds;      // trav IDs of the objects
    Mem_Fixed_t *     pMmObj;        // memory manager for objects
    Mem_Step_t *      pMmStep;       // memory manager for arrays
    void *            pManFunc;      // functionality manager (AIG manager, BDD manager, or memory manager for SOPs)
    Abc_ManTime_t *   pManTime;      // the timing manager (for mapped networks) stores arrival/required times for all nodes
    void *            pManCut;       // the cut manager (for AIGs) stores information about the cuts computed for the nodes
    float             AndGateDelay;  // an average estimated delay of one AND gate
    int               LevelMax;      // maximum number of levels
    Vec_Int_t *       vLevelsR;      // level in the reverse topological order (for AIGs)
    Vec_Ptr_t *       vSupps;        // CO support information
    int *             pModel;        // counter-example (for miters)
    Abc_Cex_t *       pSeqModel;     // counter-example (for sequential miters)
    Vec_Ptr_t *       vSeqModelVec;  // vector of counter-examples (for sequential miters)
    Abc_Ntk_t *       pExdc;         // the EXDC network (if given)
    void *            pExcare;       // the EXDC network (if given)
    void *            pData;         // misc
    Abc_Ntk_t *       pCopy;         // copy of this network
    void *            pBSMan;        // application manager
    void *            pSCLib;        // SC library
    Vec_Int_t *       vGates;        // SC library gates
    Vec_Int_t *       vPhases;       // fanins phases in the mapped netlist
    char *            pWLoadUsed;    // wire load model used
    float *           pLutTimes;     // arrivals/requireds/slacks using LUT-delay model
    Vec_Ptr_t *       vOnehots;      // names of one-hot-encoded registers
    Vec_Int_t *       vObjPerm;      // permutation saved
    Vec_Int_t *       vTopo;
    Vec_Ptr_t *       vAttrs;        // managers of various node attributes (node functionality, global BDDs, etc)
    Vec_Int_t *       vNameIds;      // name IDs
    Vec_Int_t *       vFins;         // obj/type info
};

struct Abc_Des_t_ 
{
    char *            pName;         // the name of the library
    void *            pManFunc;      // functionality manager for the nodes
    Vec_Ptr_t *       vTops;         // the array of top-level modules
    Vec_Ptr_t *       vModules;      // the array of modules
    st__table *        tModules;      // the table hashing module names into their networks
    Abc_Des_t *       pLibrary;      // the library used to map this design
    void *            pGenlib;       // the genlib library used to map this design
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

// transforming floats into ints and back
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
static inline int         Abc_NtkIsNetlist( Abc_Ntk_t * pNtk )       { return pNtk->ntkType == ABC_NTK_NETLIST;     }
static inline int         Abc_NtkIsLogic( Abc_Ntk_t * pNtk )         { return pNtk->ntkType == ABC_NTK_LOGIC;       }
static inline int         Abc_NtkIsStrash( Abc_Ntk_t * pNtk )        { return pNtk->ntkType == ABC_NTK_STRASH;      }

static inline int         Abc_NtkHasSop( Abc_Ntk_t * pNtk )          { return pNtk->ntkFunc == ABC_FUNC_SOP;        }
static inline int         Abc_NtkHasBdd( Abc_Ntk_t * pNtk )          { return pNtk->ntkFunc == ABC_FUNC_BDD;        }
static inline int         Abc_NtkHasAig( Abc_Ntk_t * pNtk )          { return pNtk->ntkFunc == ABC_FUNC_AIG;        }
static inline int         Abc_NtkHasMapping( Abc_Ntk_t * pNtk )      { return pNtk->ntkFunc == ABC_FUNC_MAP;        }
static inline int         Abc_NtkHasBlifMv( Abc_Ntk_t * pNtk )       { return pNtk->ntkFunc == ABC_FUNC_BLIFMV;     }
static inline int         Abc_NtkHasBlackbox( Abc_Ntk_t * pNtk )     { return pNtk->ntkFunc == ABC_FUNC_BLACKBOX;   }

static inline int         Abc_NtkIsSopNetlist( Abc_Ntk_t * pNtk )    { return pNtk->ntkFunc == ABC_FUNC_SOP && pNtk->ntkType == ABC_NTK_NETLIST;  }
static inline int         Abc_NtkIsBddNetlist( Abc_Ntk_t * pNtk )    { return pNtk->ntkFunc == ABC_FUNC_BDD && pNtk->ntkType == ABC_NTK_NETLIST;  }
static inline int         Abc_NtkIsAigNetlist( Abc_Ntk_t * pNtk )    { return pNtk->ntkFunc == ABC_FUNC_AIG && pNtk->ntkType == ABC_NTK_NETLIST;  }
static inline int         Abc_NtkIsMappedNetlist( Abc_Ntk_t * pNtk ) { return pNtk->ntkFunc == ABC_FUNC_MAP && pNtk->ntkType == ABC_NTK_NETLIST;  }
static inline int         Abc_NtkIsBlifMvNetlist( Abc_Ntk_t * pNtk ) { return pNtk->ntkFunc == ABC_FUNC_BLIFMV && pNtk->ntkType == ABC_NTK_NETLIST;  }
static inline int         Abc_NtkIsSopLogic( Abc_Ntk_t * pNtk )      { return pNtk->ntkFunc == ABC_FUNC_SOP && pNtk->ntkType == ABC_NTK_LOGIC  ;  }
static inline int         Abc_NtkIsBddLogic( Abc_Ntk_t * pNtk )      { return pNtk->ntkFunc == ABC_FUNC_BDD && pNtk->ntkType == ABC_NTK_LOGIC  ;  }
static inline int         Abc_NtkIsAigLogic( Abc_Ntk_t * pNtk )      { return pNtk->ntkFunc == ABC_FUNC_AIG && pNtk->ntkType == ABC_NTK_LOGIC  ;  }
static inline int         Abc_NtkIsMappedLogic( Abc_Ntk_t * pNtk )   { return pNtk->ntkFunc == ABC_FUNC_MAP && pNtk->ntkType == ABC_NTK_LOGIC  ;  }

// reading data members of the network
static inline char *      Abc_NtkName( Abc_Ntk_t * pNtk )            { return pNtk->pName;            }
static inline char *      Abc_NtkSpec( Abc_Ntk_t * pNtk )            { return pNtk->pSpec;            }
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
static inline int         Abc_NtkBoxNum( Abc_Ntk_t * pNtk )          { return Vec_PtrSize(pNtk->vBoxes);          }
static inline int         Abc_NtkBiNum( Abc_Ntk_t * pNtk )           { return pNtk->nObjCounts[ABC_OBJ_BI];       }
static inline int         Abc_NtkBoNum( Abc_Ntk_t * pNtk )           { return pNtk->nObjCounts[ABC_OBJ_BO];       }
static inline int         Abc_NtkNetNum( Abc_Ntk_t * pNtk )          { return pNtk->nObjCounts[ABC_OBJ_NET];      }
static inline int         Abc_NtkNodeNum( Abc_Ntk_t * pNtk )         { return pNtk->nObjCounts[ABC_OBJ_NODE];     }
static inline int         Abc_NtkLatchNum( Abc_Ntk_t * pNtk )        { return pNtk->nObjCounts[ABC_OBJ_LATCH];    }
static inline int         Abc_NtkWhiteboxNum( Abc_Ntk_t * pNtk )     { return pNtk->nObjCounts[ABC_OBJ_WHITEBOX]; }
static inline int         Abc_NtkBlackboxNum( Abc_Ntk_t * pNtk )     { return pNtk->nObjCounts[ABC_OBJ_BLACKBOX]; }
static inline int         Abc_NtkIsComb( Abc_Ntk_t * pNtk )          { return Abc_NtkLatchNum(pNtk) == 0;                   }
static inline int         Abc_NtkHasOnlyLatchBoxes(Abc_Ntk_t * pNtk ){ return Abc_NtkLatchNum(pNtk) == Abc_NtkBoxNum(pNtk); }
static inline int         Abc_NtkConstrNum( Abc_Ntk_t * pNtk )       { return pNtk->nConstrs;                     }

// creating simple objects
extern ABC_DLL Abc_Obj_t * Abc_NtkCreateObj( Abc_Ntk_t * pNtk, Abc_ObjType_t Type );
static inline Abc_Obj_t * Abc_NtkCreatePi( Abc_Ntk_t * pNtk )        { return Abc_NtkCreateObj( pNtk, ABC_OBJ_PI );         }
static inline Abc_Obj_t * Abc_NtkCreatePo( Abc_Ntk_t * pNtk )        { return Abc_NtkCreateObj( pNtk, ABC_OBJ_PO );         }
static inline Abc_Obj_t * Abc_NtkCreateBi( Abc_Ntk_t * pNtk )        { return Abc_NtkCreateObj( pNtk, ABC_OBJ_BI );         }
static inline Abc_Obj_t * Abc_NtkCreateBo( Abc_Ntk_t * pNtk )        { return Abc_NtkCreateObj( pNtk, ABC_OBJ_BO );         }
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
static inline Abc_Obj_t * Abc_NtkBox( Abc_Ntk_t * pNtk, int i )      { return (Abc_Obj_t *)Vec_PtrEntry( pNtk->vBoxes, i );  }

// working with complemented attributes of objects
static inline int         Abc_ObjIsComplement( Abc_Obj_t * p )       { return (int )((ABC_PTRUINT_T)p & (ABC_PTRUINT_T)01);             }
static inline Abc_Obj_t * Abc_ObjRegular( Abc_Obj_t * p )            { return (Abc_Obj_t *)((ABC_PTRUINT_T)p & ~(ABC_PTRUINT_T)01);     }
static inline Abc_Obj_t * Abc_ObjNot( Abc_Obj_t * p )                { return (Abc_Obj_t *)((ABC_PTRUINT_T)p ^  (ABC_PTRUINT_T)01);     }
static inline Abc_Obj_t * Abc_ObjNotCond( Abc_Obj_t * p, int c )     { return (Abc_Obj_t *)((ABC_PTRUINT_T)p ^  (ABC_PTRUINT_T)(c!=0)); }

// reading data members of the object
static inline unsigned    Abc_ObjType( Abc_Obj_t * pObj )            { return pObj->Type;               }
static inline unsigned    Abc_ObjId( Abc_Obj_t * pObj )              { return pObj->Id;                 }
static inline int         Abc_ObjLevel( Abc_Obj_t * pObj )           { return pObj->Level;              }
static inline Vec_Int_t * Abc_ObjFaninVec( Abc_Obj_t * pObj )        { return &pObj->vFanins;           }
static inline Vec_Int_t * Abc_ObjFanoutVec( Abc_Obj_t * pObj )       { return &pObj->vFanouts;          }
static inline Abc_Obj_t * Abc_ObjCopy( Abc_Obj_t * pObj )            { return pObj->pCopy;              }
static inline Abc_Ntk_t * Abc_ObjNtk( Abc_Obj_t * pObj )             { return pObj->pNtk;               }
static inline Abc_Ntk_t * Abc_ObjModel( Abc_Obj_t * pObj )           { assert( pObj->Type == ABC_OBJ_WHITEBOX ); return (Abc_Ntk_t *)pObj->pData;   }
static inline void *      Abc_ObjData( Abc_Obj_t * pObj )            { return pObj->pData;              }
static inline Abc_Obj_t * Abc_ObjEquiv( Abc_Obj_t * pObj )           { return (Abc_Obj_t *)pObj->pData; }
static inline Abc_Obj_t * Abc_ObjCopyCond( Abc_Obj_t * pObj )        { return Abc_ObjRegular(pObj)->pCopy? Abc_ObjNotCond(Abc_ObjRegular(pObj)->pCopy, Abc_ObjIsComplement(pObj)) : NULL;  }

// setting data members of the network
static inline void        Abc_ObjSetLevel( Abc_Obj_t * pObj, int Level )         { pObj->Level =  Level;    } 
static inline void        Abc_ObjSetCopy( Abc_Obj_t * pObj, Abc_Obj_t * pCopy )  { pObj->pCopy =  pCopy;    } 
static inline void        Abc_ObjSetData( Abc_Obj_t * pObj, void * pData )       { pObj->pData =  pData;    } 

// checking the object type
static inline int         Abc_ObjIsNone( Abc_Obj_t * pObj )          { return pObj->Type == ABC_OBJ_NONE;    }
static inline int         Abc_ObjIsPi( Abc_Obj_t * pObj )            { return pObj->Type == ABC_OBJ_PI;      }
static inline int         Abc_ObjIsPo( Abc_Obj_t * pObj )            { return pObj->Type == ABC_OBJ_PO;      }
static inline int         Abc_ObjIsBi( Abc_Obj_t * pObj )            { return pObj->Type == ABC_OBJ_BI;      }
static inline int         Abc_ObjIsBo( Abc_Obj_t * pObj )            { return pObj->Type == ABC_OBJ_BO;      }
static inline int         Abc_ObjIsCi( Abc_Obj_t * pObj )            { return pObj->Type == ABC_OBJ_PI || pObj->Type == ABC_OBJ_BO; }
static inline int         Abc_ObjIsCo( Abc_Obj_t * pObj )            { return pObj->Type == ABC_OBJ_PO || pObj->Type == ABC_OBJ_BI; }
static inline int         Abc_ObjIsTerm( Abc_Obj_t * pObj )          { return Abc_ObjIsCi(pObj) || Abc_ObjIsCo(pObj); }
static inline int         Abc_ObjIsNet( Abc_Obj_t * pObj )           { return pObj->Type == ABC_OBJ_NET;     }
static inline int         Abc_ObjIsNode( Abc_Obj_t * pObj )          { return pObj->Type == ABC_OBJ_NODE;    }
static inline int         Abc_ObjIsLatch( Abc_Obj_t * pObj )         { return pObj->Type == ABC_OBJ_LATCH;   }
static inline int         Abc_ObjIsBox( Abc_Obj_t * pObj )           { return pObj->Type == ABC_OBJ_LATCH || pObj->Type == ABC_OBJ_WHITEBOX || pObj->Type == ABC_OBJ_BLACKBOX; }
static inline int         Abc_ObjIsWhitebox( Abc_Obj_t * pObj )      { return pObj->Type == ABC_OBJ_WHITEBOX;}
static inline int         Abc_ObjIsBlackbox( Abc_Obj_t * pObj )      { return pObj->Type == ABC_OBJ_BLACKBOX;}
static inline int         Abc_ObjIsBarBuf( Abc_Obj_t * pObj )        { return Abc_NtkHasMapping(pObj->pNtk) && Abc_ObjIsNode(pObj) && Vec_IntSize(&pObj->vFanins) == 1 && pObj->pData == NULL;  }
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
static inline int         Abc_ObjFaninC0( Abc_Obj_t * pObj )         { return pObj->fCompl0;                                                }
static inline int         Abc_ObjFaninC1( Abc_Obj_t * pObj )         { return pObj->fCompl1;                                                }
static inline int         Abc_ObjFaninC( Abc_Obj_t * pObj, int i )   { assert( i >=0 && i < 2 ); return i? pObj->fCompl1 : pObj->fCompl0;   }
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
static inline Abc_Obj_t * Abc_ObjFromLit( Abc_Ntk_t * p, int iLit )  { return Abc_ObjNotCond( Abc_NtkObj(p, Abc_Lit2Var(iLit)), Abc_LitIsCompl(iLit) );           }
static inline int         Abc_ObjToLit( Abc_Obj_t * p )              { return Abc_Var2Lit( Abc_ObjId(Abc_ObjRegular(p)), Abc_ObjIsComplement(p) );                }
static inline int         Abc_ObjFaninPhase( Abc_Obj_t * p, int i )  { assert(p->pNtk->vPhases); assert( i >= 0 && i < Abc_ObjFaninNum(p) ); return (Vec_IntEntry(p->pNtk->vPhases, Abc_ObjId(p)) >> i) & 1;  } 
static inline void        Abc_ObjFaninFlipPhase( Abc_Obj_t * p,int i){ assert(p->pNtk->vPhases); assert( i >= 0 && i < Abc_ObjFaninNum(p) ); *Vec_IntEntryP(p->pNtk->vPhases, Abc_ObjId(p)) ^= (1 << i);      } 

// checking the AIG node types
static inline int         Abc_AigNodeIsConst( Abc_Obj_t * pNode )    { assert(Abc_NtkIsStrash(Abc_ObjRegular(pNode)->pNtk));  return Abc_ObjRegular(pNode)->Type == ABC_OBJ_CONST1;       }
static inline int         Abc_AigNodeIsAnd( Abc_Obj_t * pNode )      { assert(!Abc_ObjIsComplement(pNode)); assert(Abc_NtkIsStrash(pNode->pNtk)); return Abc_ObjFaninNum(pNode) == 2;                         }
static inline int         Abc_AigNodeIsChoice( Abc_Obj_t * pNode )   { assert(!Abc_ObjIsComplement(pNode)); assert(Abc_NtkIsStrash(pNode->pNtk)); return pNode->pData != NULL && Abc_ObjFanoutNum(pNode) > 0; }

// handling persistent nodes
static inline int         Abc_NodeIsPersistant( Abc_Obj_t * pNode )    { assert( Abc_AigNodeIsAnd(pNode) ); return pNode->fPersist; } 
static inline void        Abc_NodeSetPersistant( Abc_Obj_t * pNode )   { assert( Abc_AigNodeIsAnd(pNode) ); pNode->fPersist = 1;    } 
static inline void        Abc_NodeClearPersistant( Abc_Obj_t * pNode ) { assert( Abc_AigNodeIsAnd(pNode) ); pNode->fPersist = 0;    } 

// working with the traversal ID
static inline void        Abc_NtkIncrementTravId( Abc_Ntk_t * p )           { if (!p->vTravIds.pArray) Vec_IntFill(&p->vTravIds, Abc_NtkObjNumMax(p)+500, 0); p->nTravIds++; assert(p->nTravIds < (1<<30));  }
static inline int         Abc_NodeTravId( Abc_Obj_t * p )                   { return Vec_IntGetEntry(&Abc_ObjNtk(p)->vTravIds, Abc_ObjId(p));                       }
static inline void        Abc_NodeSetTravId( Abc_Obj_t * p, int TravId )    { Vec_IntSetEntry(&Abc_ObjNtk(p)->vTravIds, Abc_ObjId(p), TravId );                     }
static inline void        Abc_NodeSetTravIdCurrent( Abc_Obj_t * p )         { Abc_NodeSetTravId( p, Abc_ObjNtk(p)->nTravIds );                                      }
static inline void        Abc_NodeSetTravIdPrevious( Abc_Obj_t * p )        { Abc_NodeSetTravId( p, Abc_ObjNtk(p)->nTravIds-1 );                                    }
static inline int         Abc_NodeIsTravIdCurrent( Abc_Obj_t * p )          { return (Abc_NodeTravId(p) == Abc_ObjNtk(p)->nTravIds);                                }
static inline int         Abc_NodeIsTravIdPrevious( Abc_Obj_t * p )         { return (Abc_NodeTravId(p) == Abc_ObjNtk(p)->nTravIds-1);                              }
static inline void        Abc_NodeSetTravIdCurrentId( Abc_Ntk_t * p, int i) { Vec_IntSetEntry(&p->vTravIds, i, p->nTravIds );                                       }
static inline int         Abc_NodeIsTravIdCurrentId( Abc_Ntk_t * p, int i)  { return (Vec_IntGetEntry(&p->vTravIds, i) == p->nTravIds);                             }

// checking initial state of the latches
static inline void        Abc_LatchSetInitNone( Abc_Obj_t * pLatch ) { assert(Abc_ObjIsLatch(pLatch)); pLatch->pData = (void *)ABC_INIT_NONE;                       }
static inline void        Abc_LatchSetInit0( Abc_Obj_t * pLatch )    { assert(Abc_ObjIsLatch(pLatch)); pLatch->pData = (void *)ABC_INIT_ZERO;                       }
static inline void        Abc_LatchSetInit1( Abc_Obj_t * pLatch )    { assert(Abc_ObjIsLatch(pLatch)); pLatch->pData = (void *)ABC_INIT_ONE;                        }
static inline void        Abc_LatchSetInitDc( Abc_Obj_t * pLatch )   { assert(Abc_ObjIsLatch(pLatch)); pLatch->pData = (void *)ABC_INIT_DC;                         }
static inline int         Abc_LatchIsInitNone( Abc_Obj_t * pLatch )  { assert(Abc_ObjIsLatch(pLatch)); return pLatch->pData == (void *)ABC_INIT_NONE;               }
static inline int         Abc_LatchIsInit0( Abc_Obj_t * pLatch )     { assert(Abc_ObjIsLatch(pLatch)); return pLatch->pData == (void *)ABC_INIT_ZERO;               }
static inline int         Abc_LatchIsInit1( Abc_Obj_t * pLatch )     { assert(Abc_ObjIsLatch(pLatch)); return pLatch->pData == (void *)ABC_INIT_ONE;                }
static inline int         Abc_LatchIsInitDc( Abc_Obj_t * pLatch )    { assert(Abc_ObjIsLatch(pLatch)); return pLatch->pData == (void *)ABC_INIT_DC;                 }
static inline int         Abc_LatchInit( Abc_Obj_t * pLatch )        { assert(Abc_ObjIsLatch(pLatch)); return (int)(ABC_PTRINT_T)pLatch->pData;                     }

// global BDDs of the nodes
static inline void *      Abc_NtkGlobalBdd( Abc_Ntk_t * pNtk )          { return Vec_PtrEntry(pNtk->vAttrs, VEC_ATTR_GLOBAL_BDD);                                   }
static inline void *      Abc_NtkGlobalBddMan( Abc_Ntk_t * pNtk )       { return Vec_AttMan( (Vec_Att_t *)Abc_NtkGlobalBdd(pNtk) );                                 }
static inline void **     Abc_NtkGlobalBddArray( Abc_Ntk_t * pNtk )     { return Vec_AttArray( (Vec_Att_t *)Abc_NtkGlobalBdd(pNtk) );                               }
static inline void *      Abc_ObjGlobalBdd( Abc_Obj_t * pObj )          { return Vec_AttEntry( (Vec_Att_t *)Abc_NtkGlobalBdd(pObj->pNtk), pObj->Id );               }
static inline void        Abc_ObjSetGlobalBdd( Abc_Obj_t * pObj, void * bF )   { Vec_AttWriteEntry( (Vec_Att_t *)Abc_NtkGlobalBdd(pObj->pNtk), pObj->Id, bF );      }

// MV variables of the nodes
static inline void *      Abc_NtkMvVar( Abc_Ntk_t * pNtk )              { return Vec_PtrEntry(pNtk->vAttrs, VEC_ATTR_MVVAR);                                        }
static inline void *      Abc_NtkMvVarMan( Abc_Ntk_t * pNtk )           { return Abc_NtkMvVar(pNtk)? Vec_AttMan( (Vec_Att_t *)Abc_NtkMvVar(pNtk) ) : NULL;          }
static inline void *      Abc_ObjMvVar( Abc_Obj_t * pObj )              { return Abc_NtkMvVar(pObj->pNtk)? Vec_AttEntry( (Vec_Att_t *)Abc_NtkMvVar(pObj->pNtk), pObj->Id ) : NULL; }
static inline int         Abc_ObjMvVarNum( Abc_Obj_t * pObj )           { return (Abc_NtkMvVar(pObj->pNtk) && Abc_ObjMvVar(pObj))? *((int*)Abc_ObjMvVar(pObj)) : 2; }
static inline void        Abc_ObjSetMvVar( Abc_Obj_t * pObj, void * pV) { Vec_AttWriteEntry( (Vec_Att_t *)Abc_NtkMvVar(pObj->pNtk), pObj->Id, pV );                 }

////////////////////////////////////////////////////////////////////////
///                        ITERATORS                                 ///
////////////////////////////////////////////////////////////////////////

// objects of the network
#define Abc_NtkForEachObj( pNtk, pObj, i )                                                         \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vObjs)) && (((pObj) = Abc_NtkObj(pNtk, i)), 1); i++ )    \
        if ( (pObj) == NULL ) {} else
#define Abc_NtkForEachObjReverse( pNtk, pNode, i )                                                 \
    for ( i = Vec_PtrSize((pNtk)->vObjs) - 1; (i >= 0) && (((pNode) = Abc_NtkObj(pNtk, i)), 1); i-- ) \
        if ( (pNode) == NULL ) {} else
#define Abc_NtkForEachObjVec( vIds, pNtk, pObj, i )                                                \
    for ( i = 0; i < Vec_IntSize(vIds) && (((pObj) = Abc_NtkObj(pNtk, Vec_IntEntry(vIds,i))), 1); i++ ) \
        if ( (pObj) == NULL ) {} else
#define Abc_NtkForEachObjVecStart( vIds, pNtk, pObj, i, Start )                                    \
    for ( i = Start; i < Vec_IntSize(vIds) && (((pObj) = Abc_NtkObj(pNtk, Vec_IntEntry(vIds,i))), 1); i++ ) \
        if ( (pObj) == NULL ) {} else
#define Abc_NtkForEachNet( pNtk, pNet, i )                                                         \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vObjs)) && (((pNet) = Abc_NtkObj(pNtk, i)), 1); i++ )    \
        if ( (pNet) == NULL || !Abc_ObjIsNet(pNet) ) {} else
#define Abc_NtkForEachNode( pNtk, pNode, i )                                                       \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vObjs)) && (((pNode) = Abc_NtkObj(pNtk, i)), 1); i++ )   \
        if ( (pNode) == NULL || !Abc_ObjIsNode(pNode) ) {} else
#define Abc_NtkForEachNodeNotBarBuf( pNtk, pNode, i )                                              \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vObjs)) && (((pNode) = Abc_NtkObj(pNtk, i)), 1); i++ )   \
        if ( (pNode) == NULL || !Abc_ObjIsNode(pNode) || Abc_ObjIsBarBuf(pNode) ) {} else
#define Abc_NtkForEachNode1( pNtk, pNode, i )                                                      \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vObjs)) && (((pNode) = Abc_NtkObj(pNtk, i)), 1); i++ )   \
        if ( (pNode) == NULL || !Abc_ObjIsNode(pNode) || !Abc_ObjFaninNum(pNode) ) {} else
#define Abc_NtkForEachNodeNotBarBuf1( pNtk, pNode, i )                                             \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vObjs)) && (((pNode) = Abc_NtkObj(pNtk, i)), 1); i++ )   \
        if ( (pNode) == NULL || !Abc_ObjIsNode(pNode) || !Abc_ObjFaninNum(pNode) || Abc_ObjIsBarBuf(pNode) ) {} else
#define Abc_NtkForEachNodeReverse( pNtk, pNode, i )                                                \
    for ( i = Vec_PtrSize((pNtk)->vObjs) - 1; (i >= 0) && (((pNode) = Abc_NtkObj(pNtk, i)), 1); i-- ) \
        if ( (pNode) == NULL || !Abc_ObjIsNode(pNode) ) {} else
#define Abc_NtkForEachNodeReverse1( pNtk, pNode, i )                                               \
    for ( i = Vec_PtrSize((pNtk)->vObjs) - 1; (i >= 0) && (((pNode) = Abc_NtkObj(pNtk, i)), 1); i-- ) \
        if ( (pNode) == NULL || !Abc_ObjIsNode(pNode) || !Abc_ObjFaninNum(pNode) ) {} else
#define Abc_NtkForEachBarBuf( pNtk, pNode, i )                                                     \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vObjs)) && (((pNode) = Abc_NtkObj(pNtk, i)), 1); i++ )   \
        if ( (pNode) == NULL || !Abc_ObjIsBarBuf(pNode) ) {} else
#define Abc_NtkForEachGate( pNtk, pNode, i )                                                       \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vObjs)) && (((pNode) = Abc_NtkObj(pNtk, i)), 1); i++ )   \
        if ( (pNode) == NULL || !Abc_ObjIsGate(pNode) ) {} else
#define Abc_AigForEachAnd( pNtk, pNode, i )                                                        \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vObjs)) && (((pNode) = Abc_NtkObj(pNtk, i)), 1); i++ )   \
        if ( (pNode) == NULL || !Abc_AigNodeIsAnd(pNode) ) {} else
#define Abc_NtkForEachNodeCi( pNtk, pNode, i )                                                       \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vObjs)) && (((pNode) = Abc_NtkObj(pNtk, i)), 1); i++ )   \
        if ( (pNode) == NULL || (!Abc_ObjIsNode(pNode) && !Abc_ObjIsCi(pNode)) ) {} else
#define Abc_NtkForEachNodeCo( pNtk, pNode, i )                                                       \
    for ( i = 0; (i < Vec_PtrSize((pNtk)->vObjs)) && (((pNode) = Abc_NtkObj(pNtk, i)), 1); i++ )   \
        if ( (pNode) == NULL || (!Abc_ObjIsNode(pNode) && !Abc_ObjIsCo(pNode)) ) {} else
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
#define Abc_NtkForEachLiPo( pNtk, pCo, i )                                                         \
    for ( i = 0; (i < Abc_NtkCoNum(pNtk)) && (((pCo) = Abc_NtkCo(pNtk, i < pNtk->nBarBufs ? Abc_NtkCoNum(pNtk) - pNtk->nBarBufs + i : i - pNtk->nBarBufs)), 1); i++ )
// fanin and fanouts
#define Abc_ObjForEachFanin( pObj, pFanin, i )                                                     \
    for ( i = 0; (i < Abc_ObjFaninNum(pObj)) && (((pFanin) = Abc_ObjFanin(pObj, i)), 1); i++ )
#define Abc_ObjForEachFanout( pObj, pFanout, i )                                                   \
    for ( i = 0; (i < Abc_ObjFanoutNum(pObj)) && (((pFanout) = Abc_ObjFanout(pObj, i)), 1); i++ )
#define Abc_ObjForEachFaninId( pObj, iFanin, i )                                                   \
    for ( i = 0; (i < Abc_ObjFaninNum(pObj)) && (((iFanin) = Abc_ObjFaninId(pObj, i)), 1); i++ )
#define Abc_ObjForEachFanoutId( pObj, iFanout, i )                                                 \
    for ( i = 0; (i < Abc_ObjFanoutNum(pObj)) && (((iFanout) = Abc_ObjFanoutId(pObj, i)), 1); i++ )
// cubes and literals
#define Abc_CubeForEachVar( pCube, Value, i )                                                      \
    for ( i = 0; (pCube[i] != ' ') && (Value = pCube[i]); i++ )           
#define Abc_SopForEachCube( pSop, nFanins, pCube )                                                 \
    for ( pCube = (pSop); *pCube; pCube += (nFanins) + 3 )
#define Abc_SopForEachCubePair( pSop, nFanins, pCube, pCube2 )                                     \
    Abc_SopForEachCube( pSop, nFanins, pCube )                                                     \
    Abc_SopForEachCube( pCube + (nFanins) + 3, nFanins, pCube2 )

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== abcAig.c ==========================================================*/
extern ABC_DLL Abc_Aig_t *        Abc_AigAlloc( Abc_Ntk_t * pNtk );
extern ABC_DLL void               Abc_AigFree( Abc_Aig_t * pMan );
extern ABC_DLL int                Abc_AigCleanup( Abc_Aig_t * pMan );
extern ABC_DLL int                Abc_AigCheck( Abc_Aig_t * pMan );
extern ABC_DLL int                Abc_AigLevel( Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Obj_t *        Abc_AigConst1( Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Obj_t *        Abc_AigAnd( Abc_Aig_t * pMan, Abc_Obj_t * p0, Abc_Obj_t * p1 );
extern ABC_DLL Abc_Obj_t *        Abc_AigAndLookup( Abc_Aig_t * pMan, Abc_Obj_t * p0, Abc_Obj_t * p1 );
extern ABC_DLL Abc_Obj_t *        Abc_AigXorLookup( Abc_Aig_t * pMan, Abc_Obj_t * p0, Abc_Obj_t * p1, int * pType );
extern ABC_DLL Abc_Obj_t *        Abc_AigMuxLookup( Abc_Aig_t * pMan, Abc_Obj_t * pC, Abc_Obj_t * pT, Abc_Obj_t * pE, int * pType );
extern ABC_DLL Abc_Obj_t *        Abc_AigOr( Abc_Aig_t * pMan, Abc_Obj_t * p0, Abc_Obj_t * p1 );
extern ABC_DLL Abc_Obj_t *        Abc_AigXor( Abc_Aig_t * pMan, Abc_Obj_t * p0, Abc_Obj_t * p1 );
extern ABC_DLL Abc_Obj_t *        Abc_AigMux( Abc_Aig_t * pMan, Abc_Obj_t * pC, Abc_Obj_t * p1, Abc_Obj_t * p0 );
extern ABC_DLL Abc_Obj_t *        Abc_AigMiter( Abc_Aig_t * pMan, Vec_Ptr_t * vPairs, int fImplic );
extern ABC_DLL void               Abc_AigReplace( Abc_Aig_t * pMan, Abc_Obj_t * pOld, Abc_Obj_t * pNew, int  fUpdateLevel );
extern ABC_DLL void               Abc_AigDeleteNode( Abc_Aig_t * pMan, Abc_Obj_t * pOld );
extern ABC_DLL void               Abc_AigRehash( Abc_Aig_t * pMan );
extern ABC_DLL int                Abc_AigNodeHasComplFanoutEdge( Abc_Obj_t * pNode );
extern ABC_DLL int                Abc_AigNodeHasComplFanoutEdgeTrav( Abc_Obj_t * pNode );
extern ABC_DLL void               Abc_AigPrintNode( Abc_Obj_t * pNode );
extern ABC_DLL int                Abc_AigNodeIsAcyclic( Abc_Obj_t * pNode, Abc_Obj_t * pRoot );
extern ABC_DLL void               Abc_AigCheckFaninOrder( Abc_Aig_t * pMan );
extern ABC_DLL void               Abc_AigSetNodePhases( Abc_Ntk_t * pNtk );
extern ABC_DLL Vec_Ptr_t *        Abc_AigUpdateStart( Abc_Aig_t * pMan, Vec_Ptr_t ** pvUpdatedNets );
extern ABC_DLL void               Abc_AigUpdateStop( Abc_Aig_t * pMan );
extern ABC_DLL void               Abc_AigUpdateReset( Abc_Aig_t * pMan );
/*=== abcAttach.c ==========================================================*/
extern ABC_DLL int                Abc_NtkAttach( Abc_Ntk_t * pNtk );
/*=== abcBarBuf.c ==========================================================*/
extern ABC_DLL Abc_Ntk_t *        Abc_NtkToBarBufs( Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkFromBarBufs( Abc_Ntk_t * pNtkBase, Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkBarBufsToBuffers( Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkBarBufsFromBuffers( Abc_Ntk_t * pNtkBase, Abc_Ntk_t * pNtk );
/*=== abcBlifMv.c ==========================================================*/
extern ABC_DLL void               Abc_NtkStartMvVars( Abc_Ntk_t * pNtk );
extern ABC_DLL void               Abc_NtkFreeMvVars( Abc_Ntk_t * pNtk );
extern ABC_DLL void               Abc_NtkSetMvVarValues( Abc_Obj_t * pObj, int nValues );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkStrashBlifMv( Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkInsertBlifMv( Abc_Ntk_t * pNtkBase, Abc_Ntk_t * pNtkLogic );
extern ABC_DLL int                Abc_NtkConvertToBlifMv( Abc_Ntk_t * pNtk );
extern ABC_DLL char *             Abc_NodeConvertSopToMvSop( int nVars, Vec_Int_t * vSop0, Vec_Int_t * vSop1 );
extern ABC_DLL int                Abc_NodeEvalMvCost( int nVars, Vec_Int_t * vSop0, Vec_Int_t * vSop1 );
/*=== abcBalance.c ==========================================================*/
extern ABC_DLL Abc_Ntk_t *        Abc_NtkBalance( Abc_Ntk_t * pNtk, int  fDuplicate, int  fSelective, int  fUpdateLevel );
/*=== abcCheck.c ==========================================================*/
extern ABC_DLL int                Abc_NtkCheck( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkCheckRead( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkDoCheck( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkCheckObj( Abc_Ntk_t * pNtk, Abc_Obj_t * pObj );
extern ABC_DLL int                Abc_NtkCompareSignals( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int fOnlyPis, int fComb );
extern ABC_DLL int                Abc_NtkIsAcyclicHierarchy( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkCheckUniqueCiNames( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkCheckUniqueCoNames( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkCheckUniqueCioNames( Abc_Ntk_t * pNtk );
/*=== abcCollapse.c ==========================================================*/
extern ABC_DLL Abc_Ntk_t *        Abc_NtkCollapse( Abc_Ntk_t * pNtk, int fBddSizeMax, int fDualRail, int fReorder, int fVerbose );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkCollapseSat( Abc_Ntk_t * pNtk, int nCubeLim, int nBTLimit, int nCostMax, int fCanon, int fReverse, int fCnfShared, int fVerbose );
extern ABC_DLL Gia_Man_t *        Abc_NtkClpGia( Abc_Ntk_t * pNtk );
/*=== abcCut.c ==========================================================*/
extern ABC_DLL void *             Abc_NodeGetCutsRecursive( void * p, Abc_Obj_t * pObj, int fDag, int fTree );
extern ABC_DLL void *             Abc_NodeGetCuts( void * p, Abc_Obj_t * pObj, int fDag, int fTree );
extern ABC_DLL void               Abc_NodeGetCutsSeq( void * p, Abc_Obj_t * pObj, int fFirst );
extern ABC_DLL void *             Abc_NodeReadCuts( void * p, Abc_Obj_t * pObj );
extern ABC_DLL void               Abc_NodeFreeCuts( void * p, Abc_Obj_t * pObj );
/*=== abcDar.c ============================================================*/
extern ABC_DLL int                Abc_NtkPhaseFrameNum( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkDarPrintCone( Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkBalanceExor( Abc_Ntk_t * pNtk, int fUpdateLevel, int fVerbose );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkDarLatchSweep( Abc_Ntk_t * pNtk, int fLatchConst, int fLatchEqual, int fSaveNames, int fUseMvSweep, int nFramesSymb, int nFramesSatur, int fVerbose, int fVeryVerbose );
/*=== abcDelay.c ==========================================================*/
extern ABC_DLL float              Abc_NtkDelayTraceLut( Abc_Ntk_t * pNtk, int fUseLutLib );
/*=== abcDfs.c ==========================================================*/
extern ABC_DLL Vec_Ptr_t *        Abc_NtkDfs( Abc_Ntk_t * pNtk, int fCollectAll );
extern ABC_DLL Vec_Ptr_t *        Abc_NtkDfs2( Abc_Ntk_t * pNtk );
extern ABC_DLL Vec_Ptr_t *        Abc_NtkDfsNodes( Abc_Ntk_t * pNtk, Abc_Obj_t ** ppNodes, int nNodes );
extern ABC_DLL Vec_Ptr_t *        Abc_NtkDfsReverse( Abc_Ntk_t * pNtk );
extern ABC_DLL Vec_Ptr_t *        Abc_NtkDfsReverseNodes( Abc_Ntk_t * pNtk, Abc_Obj_t ** ppNodes, int nNodes );
extern ABC_DLL Vec_Ptr_t *        Abc_NtkDfsReverseNodesContained( Abc_Ntk_t * pNtk, Abc_Obj_t ** ppNodes, int nNodes );
extern ABC_DLL Vec_Ptr_t *        Abc_NtkDfsSeq( Abc_Ntk_t * pNtk );
extern ABC_DLL Vec_Ptr_t *        Abc_NtkDfsSeqReverse( Abc_Ntk_t * pNtk );
extern ABC_DLL Vec_Ptr_t *        Abc_NtkDfsIter( Abc_Ntk_t * pNtk, int fCollectAll );
extern ABC_DLL Vec_Ptr_t *        Abc_NtkDfsIterNodes( Abc_Ntk_t * pNtk, Vec_Ptr_t * vRoots );
extern ABC_DLL Vec_Ptr_t *        Abc_NtkDfsHie( Abc_Ntk_t * pNtk, int fCollectAll );
extern ABC_DLL int                Abc_NtkIsDfsOrdered( Abc_Ntk_t * pNtk );
extern ABC_DLL Vec_Ptr_t *        Abc_NtkDfsWithBoxes( Abc_Ntk_t * pNtk );
extern ABC_DLL Vec_Ptr_t *        Abc_NtkSupport( Abc_Ntk_t * pNtk );
extern ABC_DLL Vec_Ptr_t *        Abc_NtkNodeSupport( Abc_Ntk_t * pNtk, Abc_Obj_t ** ppNodes, int nNodes );
extern ABC_DLL Vec_Int_t *        Abc_NtkNodeSupportInt( Abc_Ntk_t * pNtk, int iCo );
extern ABC_DLL int                Abc_NtkFunctionalIso( Abc_Ntk_t * pNtk, int iCo1, int iCo2, int fCommon );
extern ABC_DLL Vec_Ptr_t *        Abc_AigDfs( Abc_Ntk_t * pNtk, int fCollectAll, int fCollectCos );
extern ABC_DLL Vec_Ptr_t *        Abc_AigDfsMap( Abc_Ntk_t * pNtk );
extern ABC_DLL Vec_Vec_t *        Abc_DfsLevelized( Abc_Obj_t * pNode, int  fTfi );
extern ABC_DLL Vec_Vec_t *        Abc_NtkLevelize( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkLevel( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkLevelReverse( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkIsAcyclic( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkIsAcyclicWithBoxes( Abc_Ntk_t * pNtk );
extern ABC_DLL Vec_Ptr_t *        Abc_AigGetLevelizedOrder( Abc_Ntk_t * pNtk, int fCollectCis );
/*=== abcExact.c ==========================================================*/
extern ABC_DLL int                Abc_ExactInputNum();
extern ABC_DLL int                Abc_ExactIsRunning();
extern ABC_DLL Abc_Obj_t *        Abc_ExactBuildNode( word * pTruth, int nVars, int * pArrTimeProfile, Abc_Obj_t ** pFanins, Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkFindExact( word * pTruth, int nVars, int nFunc, int nMaxDepth, int * pArrivalTimes, int nBTLimit, int nStartGates, int fVerbose );
/*=== abcFanio.c ==========================================================*/
extern ABC_DLL void               Abc_ObjAddFanin( Abc_Obj_t * pObj, Abc_Obj_t * pFanin );
extern ABC_DLL void               Abc_ObjDeleteFanin( Abc_Obj_t * pObj, Abc_Obj_t * pFanin );
extern ABC_DLL void               Abc_ObjRemoveFanins( Abc_Obj_t * pObj );
extern ABC_DLL void               Abc_ObjPatchFanin( Abc_Obj_t * pObj, Abc_Obj_t * pFaninOld, Abc_Obj_t * pFaninNew );
extern ABC_DLL void               Abc_ObjPatchFanoutFanin( Abc_Obj_t * pObj, int iObjNew );
extern ABC_DLL Abc_Obj_t *        Abc_ObjInsertBetween( Abc_Obj_t * pNodeIn, Abc_Obj_t * pNodeOut, Abc_ObjType_t Type );
extern ABC_DLL void               Abc_ObjTransferFanout( Abc_Obj_t * pObjOld, Abc_Obj_t * pObjNew );
extern ABC_DLL void               Abc_ObjReplace( Abc_Obj_t * pObjOld, Abc_Obj_t * pObjNew );
extern ABC_DLL int                Abc_ObjFanoutFaninNum( Abc_Obj_t * pFanout, Abc_Obj_t * pFanin );
/*=== abcFanOrder.c ==========================================================*/
extern ABC_DLL int                Abc_NtkMakeLegit( Abc_Ntk_t * pNtk );
extern ABC_DLL void               Abc_NtkSortSops( Abc_Ntk_t * pNtk );
/*=== abcFraig.c ==========================================================*/
extern ABC_DLL Abc_Ntk_t *        Abc_NtkFraig( Abc_Ntk_t * pNtk, void * pParams, int fAllNodes, int fExdc );
extern ABC_DLL void *             Abc_NtkToFraig( Abc_Ntk_t * pNtk, void * pParams, int fAllNodes, int fExdc );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkFraigTrust( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkFraigStore( Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkFraigRestore( int nPatsRand, int nPatsDyna, int nBTLimit );
extern ABC_DLL void               Abc_NtkFraigStoreClean();
/*=== abcFunc.c ==========================================================*/
extern ABC_DLL int                Abc_NtkSopToBdd( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkBddToSop( Abc_Ntk_t * pNtk, int fMode, int nCubeLimit );
extern ABC_DLL void               Abc_NodeBddToCnf( Abc_Obj_t * pNode, Mem_Flex_t * pMmMan, Vec_Str_t * vCube, int fAllPrimes, char ** ppSop0, char ** ppSop1 );
extern ABC_DLL void               Abc_NtkLogicMakeDirectSops( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkSopToAig( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkAigToBdd( Abc_Ntk_t * pNtk );
extern ABC_DLL Gia_Man_t *        Abc_NtkAigToGia( Abc_Ntk_t * p, int fGiaSimple );
extern ABC_DLL int                Abc_NtkMapToSop( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkToSop( Abc_Ntk_t * pNtk, int fMode, int nCubeLimit );
extern ABC_DLL int                Abc_NtkToBdd( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkToAig( Abc_Ntk_t * pNtk );
/*=== abcHaig.c ==========================================================*/
extern ABC_DLL int                Abc_NtkHaigStart( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkHaigStop( Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkHaigUse( Abc_Ntk_t * pNtk );
/*=== abcHie.c ==========================================================*/
extern ABC_DLL Abc_Ntk_t *        Abc_NtkFlattenLogicHierarchy( Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkConvertBlackboxes( Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkInsertNewLogic( Abc_Ntk_t * pNtkH, Abc_Ntk_t * pNtkL );
extern ABC_DLL void               Abc_NtkPrintBoxInfo( Abc_Ntk_t * pNtk );
/*=== abcHieGia.c ==========================================================*/
extern ABC_DLL Gia_Man_t *        Abc_NtkFlattenHierarchyGia( Abc_Ntk_t * pNtk, Vec_Ptr_t ** pvBuffers, int fVerbose );
extern ABC_DLL void               Abc_NtkInsertHierarchyGia( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNew, int fVerbose );
/*=== abcLatch.c ==========================================================*/
extern ABC_DLL int                Abc_NtkLatchIsSelfFeed( Abc_Obj_t * pLatch );
extern ABC_DLL int                Abc_NtkCountSelfFeedLatches( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkRemoveSelfFeedLatches( Abc_Ntk_t * pNtk );
extern ABC_DLL Vec_Int_t *        Abc_NtkCollectLatchValues( Abc_Ntk_t * pNtk );
extern ABC_DLL char *             Abc_NtkCollectLatchValuesStr( Abc_Ntk_t * pNtk );
extern ABC_DLL void               Abc_NtkInsertLatchValues( Abc_Ntk_t * pNtk, Vec_Int_t * vValues );
extern ABC_DLL Abc_Obj_t *        Abc_NtkAddLatch( Abc_Ntk_t * pNtk, Abc_Obj_t * pDriver, Abc_InitType_t Init );
extern ABC_DLL void               Abc_NtkConvertDcLatches( Abc_Ntk_t * pNtk );
extern ABC_DLL Vec_Ptr_t *        Abc_NtkConverLatchNamesIntoNumbers( Abc_Ntk_t * pNtk );
 /*=== abcLib.c ==========================================================*/
extern ABC_DLL Abc_Des_t *        Abc_DesCreate( char * pName );
extern ABC_DLL void               Abc_DesCleanManPointer( Abc_Des_t * p, void * pMan );
extern ABC_DLL void               Abc_DesFree( Abc_Des_t * p, Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Des_t *        Abc_DesDup( Abc_Des_t * p );
extern ABC_DLL void               Abc_DesPrint( Abc_Des_t * p );
extern ABC_DLL int                Abc_DesAddModel( Abc_Des_t * p, Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Ntk_t *        Abc_DesFindModelByName( Abc_Des_t * p, char * pName );
extern ABC_DLL int                Abc_DesFindTopLevelModels( Abc_Des_t * p );
extern ABC_DLL Abc_Ntk_t *        Abc_DesDeriveRoot( Abc_Des_t * p );
/*=== abcLog.c ==========================================================*/
extern ABC_DLL void               Abc_NtkWriteLogFile( char * pFileName, Abc_Cex_t * pSeqCex, int Status, int nFrames, char * pCommand );
/*=== abcMap.c ==========================================================*/
extern ABC_DLL Abc_Obj_t *        Abc_NtkFetchTwinNode( Abc_Obj_t * pNode );
/*=== abcMiter.c ==========================================================*/
extern ABC_DLL int                Abc_NtkMinimumBase( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NodeMinimumBase( Abc_Obj_t * pNode );
extern ABC_DLL int                Abc_NtkRemoveDupFanins( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NodeRemoveDupFanins( Abc_Obj_t * pNode );
/*=== abcMiter.c ==========================================================*/
extern ABC_DLL Abc_Ntk_t *        Abc_NtkMiter( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int fComb, int nPartSize, int fImplic, int fMulti );
extern ABC_DLL void               Abc_NtkMiterAddCone( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkMiter, Abc_Obj_t * pNode );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkMiterAnd( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int fOr, int fCompl2 );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkMiterCofactor( Abc_Ntk_t * pNtk, Vec_Int_t * vPiValues );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkMiterForCofactors( Abc_Ntk_t * pNtk, int Out, int In1, int In2 );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkMiterQuantify( Abc_Ntk_t * pNtk, int In, int fExist );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkMiterQuantifyPis( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkMiterIsConstant( Abc_Ntk_t * pMiter );
extern ABC_DLL void               Abc_NtkMiterReport( Abc_Ntk_t * pMiter );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkFrames( Abc_Ntk_t * pNtk, int nFrames, int fInitial, int fVerbose );
extern ABC_DLL int                Abc_NtkCombinePos( Abc_Ntk_t * pNtk, int fAnd, int fXor );
/*=== abcNames.c ====================================================*/
extern ABC_DLL char *             Abc_ObjName( Abc_Obj_t * pNode );
extern ABC_DLL char *             Abc_ObjAssignName( Abc_Obj_t * pObj, char * pName, char * pSuffix );
extern ABC_DLL char *             Abc_ObjNamePrefix( Abc_Obj_t * pObj, char * pPrefix );
extern ABC_DLL char *             Abc_ObjNameSuffix( Abc_Obj_t * pObj, char * pSuffix );
extern ABC_DLL char *             Abc_ObjNameDummy( char * pPrefix, int Num, int nDigits );
extern ABC_DLL void               Abc_NtkTrasferNames( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkNew );
extern ABC_DLL void               Abc_NtkTrasferNamesNoLatches( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkNew );
extern ABC_DLL Vec_Ptr_t *        Abc_NodeGetFaninNames( Abc_Obj_t * pNode );
extern ABC_DLL Vec_Ptr_t *        Abc_NodeGetFakeNames( int nNames );
extern ABC_DLL void               Abc_NodeFreeNames( Vec_Ptr_t * vNames );
extern ABC_DLL char **            Abc_NtkCollectCioNames( Abc_Ntk_t * pNtk, int fCollectCos );
extern ABC_DLL int                Abc_NodeCompareNames( Abc_Obj_t ** pp1, Abc_Obj_t ** pp2 );
extern ABC_DLL void               Abc_NtkOrderObjsByName( Abc_Ntk_t * pNtk, int fComb );
extern ABC_DLL void               Abc_NtkAddDummyPiNames( Abc_Ntk_t * pNtk );
extern ABC_DLL void               Abc_NtkAddDummyPoNames( Abc_Ntk_t * pNtk );
extern ABC_DLL void               Abc_NtkAddDummyBoxNames( Abc_Ntk_t * pNtk );
extern ABC_DLL void               Abc_NtkShortNames( Abc_Ntk_t * pNtk );
extern ABC_DLL void               Abc_NtkStartNameIds( Abc_Ntk_t * p );
extern ABC_DLL void               Abc_NtkTransferNameIds( Abc_Ntk_t * p, Abc_Ntk_t * pNew );
extern ABC_DLL void               Abc_NtkUpdateNameIds( Abc_Ntk_t * p );
/*=== abcNetlist.c ==========================================================*/
extern ABC_DLL Abc_Ntk_t *        Abc_NtkToLogic( Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkToNetlist( Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkToNetlistBench( Abc_Ntk_t * pNtk );
/*=== abcNtbdd.c ==========================================================*/
extern ABC_DLL Abc_Ntk_t *        Abc_NtkDeriveFromBdd( void * dd, void * bFunc, char * pNamePo, Vec_Ptr_t * vNamesPi );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkBddToMuxes( Abc_Ntk_t * pNtk );
extern ABC_DLL void *             Abc_NtkBuildGlobalBdds( Abc_Ntk_t * pNtk, int fBddSizeMax, int fDropInternal, int fReorder, int fVerbose );
extern ABC_DLL void *             Abc_NtkFreeGlobalBdds( Abc_Ntk_t * pNtk, int fFreeMan );
extern ABC_DLL int                Abc_NtkSizeOfGlobalBdds( Abc_Ntk_t * pNtk );
/*=== abcNtk.c ==========================================================*/
extern ABC_DLL Abc_Ntk_t *        Abc_NtkAlloc( Abc_NtkType_t Type, Abc_NtkFunc_t Func, int fUseMemMan );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkStartFrom( Abc_Ntk_t * pNtk, Abc_NtkType_t Type, Abc_NtkFunc_t Func );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkStartFromNoLatches( Abc_Ntk_t * pNtk, Abc_NtkType_t Type, Abc_NtkFunc_t Func );
extern ABC_DLL void               Abc_NtkFinalize( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkNew );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkStartRead( char * pName );
extern ABC_DLL void               Abc_NtkFinalizeRead( Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkDup( Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkDupDfs( Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkDupDfsNoBarBufs( Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkDupTransformMiter( Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkCreateCone( Abc_Ntk_t * pNtk, Abc_Obj_t * pNode, char * pNodeName, int fUseAllCis );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkCreateConeArray( Abc_Ntk_t * pNtk, Vec_Ptr_t * vRoots, int fUseAllCis );
extern ABC_DLL void               Abc_NtkAppendToCone( Abc_Ntk_t * pNtkNew, Abc_Ntk_t * pNtk, Vec_Ptr_t * vRoots );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkCreateMffc( Abc_Ntk_t * pNtk, Abc_Obj_t * pNode, char * pNodeName );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkCreateTarget( Abc_Ntk_t * pNtk, Vec_Ptr_t * vRoots, Vec_Int_t * vValues );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkCreateFromNode( Abc_Ntk_t * pNtk, Abc_Obj_t * pNode );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkCreateWithNode( char * pSop );
extern ABC_DLL void               Abc_NtkDelete( Abc_Ntk_t * pNtk );
extern ABC_DLL void               Abc_NtkFixNonDrivenNets( Abc_Ntk_t * pNtk );
extern ABC_DLL void               Abc_NtkMakeComb( Abc_Ntk_t * pNtk, int fRemoveLatches );
extern ABC_DLL void               Abc_NtkPermute( Abc_Ntk_t * pNtk, int fInputs, int fOutputs, int fFlops, char * pFlopPermFile );
extern ABC_DLL void               Abc_NtkUnpermute( Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkCreateFromSops( char * pName, Vec_Ptr_t * vSops );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkCreateFromGias( char * pName, Vec_Ptr_t * vGias );
/*=== abcObj.c ==========================================================*/
extern ABC_DLL Abc_Obj_t *        Abc_ObjAlloc( Abc_Ntk_t * pNtk, Abc_ObjType_t Type );
extern ABC_DLL void               Abc_ObjRecycle( Abc_Obj_t * pObj );
extern ABC_DLL Abc_Obj_t *        Abc_NtkCreateObj( Abc_Ntk_t * pNtk, Abc_ObjType_t Type );
extern ABC_DLL void               Abc_NtkDeleteObj( Abc_Obj_t * pObj );
extern ABC_DLL void               Abc_NtkDeleteObjPo( Abc_Obj_t * pObj );
extern ABC_DLL void               Abc_NtkDeleteObj_rec( Abc_Obj_t * pObj, int fOnlyNodes );
extern ABC_DLL void               Abc_NtkDeleteAll_rec( Abc_Obj_t * pObj );
extern ABC_DLL Abc_Obj_t *        Abc_NtkDupObj( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pObj, int fCopyName );
extern ABC_DLL Abc_Obj_t *        Abc_NtkDupBox( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pBox, int fCopyName );
extern ABC_DLL Abc_Obj_t *        Abc_NtkCloneObj( Abc_Obj_t * pNode );
extern ABC_DLL Abc_Obj_t *        Abc_NtkFindNode( Abc_Ntk_t * pNtk, char * pName );
extern ABC_DLL Abc_Obj_t *        Abc_NtkFindNet( Abc_Ntk_t * pNtk, char * pName );
extern ABC_DLL Abc_Obj_t *        Abc_NtkFindCi( Abc_Ntk_t * pNtk, char * pName );
extern ABC_DLL Abc_Obj_t *        Abc_NtkFindCo( Abc_Ntk_t * pNtk, char * pName );
extern ABC_DLL Abc_Obj_t *        Abc_NtkFindOrCreateNet( Abc_Ntk_t * pNtk, char * pName );
extern ABC_DLL Abc_Obj_t *        Abc_NtkCreateNodeConst0( Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Obj_t *        Abc_NtkCreateNodeConst1( Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Obj_t *        Abc_NtkCreateNodeInv( Abc_Ntk_t * pNtk, Abc_Obj_t * pFanin );
extern ABC_DLL Abc_Obj_t *        Abc_NtkCreateNodeBuf( Abc_Ntk_t * pNtk, Abc_Obj_t * pFanin );
extern ABC_DLL Abc_Obj_t *        Abc_NtkCreateNodeAnd( Abc_Ntk_t * pNtk, Vec_Ptr_t * vFanins );
extern ABC_DLL Abc_Obj_t *        Abc_NtkCreateNodeOr( Abc_Ntk_t * pNtk, Vec_Ptr_t * vFanins );
extern ABC_DLL Abc_Obj_t *        Abc_NtkCreateNodeExor( Abc_Ntk_t * pNtk, Vec_Ptr_t * vFanins );
extern ABC_DLL Abc_Obj_t *        Abc_NtkCreateNodeMux( Abc_Ntk_t * pNtk, Abc_Obj_t * pNodeC, Abc_Obj_t * pNode1, Abc_Obj_t * pNode0 );
extern ABC_DLL int                Abc_NodeIsConst( Abc_Obj_t * pNode );
extern ABC_DLL int                Abc_NodeIsConst0( Abc_Obj_t * pNode );    
extern ABC_DLL int                Abc_NodeIsConst1( Abc_Obj_t * pNode );    
extern ABC_DLL int                Abc_NodeIsBuf( Abc_Obj_t * pNode );    
extern ABC_DLL int                Abc_NodeIsInv( Abc_Obj_t * pNode );    
extern ABC_DLL void               Abc_NodeComplement( Abc_Obj_t * pNode );
extern ABC_DLL void               Abc_NodeComplementInput( Abc_Obj_t * pNode, Abc_Obj_t * pFanin );
/*=== abcOdc.c ==========================================================*/
typedef struct Odc_Man_t_         Odc_Man_t;
extern ABC_DLL Odc_Man_t *        Abc_NtkDontCareAlloc( int nVarsMax, int nLevels, int fVerbose, int fVeryVerbose );
extern ABC_DLL void               Abc_NtkDontCareClear( Odc_Man_t * p );
extern ABC_DLL void               Abc_NtkDontCareFree( Odc_Man_t * p );
extern ABC_DLL int                Abc_NtkDontCareCompute( Odc_Man_t * p, Abc_Obj_t * pNode, Vec_Ptr_t * vLeaves, unsigned * puTruth );
/*=== abcPrint.c ==========================================================*/
extern ABC_DLL float              Abc_NtkMfsTotalSwitching( Abc_Ntk_t * pNtk );
extern ABC_DLL float              Abc_NtkMfsTotalGlitching( Abc_Ntk_t * pNtk, int nPats, int Prob, int fVerbose );
extern ABC_DLL void               Abc_NtkPrintStats( Abc_Ntk_t * pNtk, int fFactored, int fSaveBest, int fDumpResult, int fUseLutLib, int fPrintMuxes, int fPower, int fGlitch, int fSkipBuf, int fSkipSmall, int fPrintMem );
extern ABC_DLL void               Abc_NtkPrintIo( FILE * pFile, Abc_Ntk_t * pNtk, int fPrintFlops );
extern ABC_DLL void               Abc_NtkPrintLatch( FILE * pFile, Abc_Ntk_t * pNtk );
extern ABC_DLL void               Abc_NtkPrintFanio( FILE * pFile, Abc_Ntk_t * pNtk, int fUseFanio, int fUsePio, int fUseSupp, int fUseCone );
extern ABC_DLL void               Abc_NtkPrintFanioNew( FILE * pFile, Abc_Ntk_t * pNtk, int fMffc );
extern ABC_DLL void               Abc_NodePrintFanio( FILE * pFile, Abc_Obj_t * pNode );
extern ABC_DLL void               Abc_NtkPrintFactor( FILE * pFile, Abc_Ntk_t * pNtk, int fUseRealNames );
extern ABC_DLL void               Abc_NodePrintFactor( FILE * pFile, Abc_Obj_t * pNode, int fUseRealNames );
extern ABC_DLL void               Abc_NtkPrintLevel( FILE * pFile, Abc_Ntk_t * pNtk, int fProfile, int fListNodes, int fVerbose );
extern ABC_DLL void               Abc_NodePrintLevel( FILE * pFile, Abc_Obj_t * pNode );
extern ABC_DLL void               Abc_NtkPrintSkews( FILE * pFile, Abc_Ntk_t * pNtk, int fPrintAll );
extern ABC_DLL void               Abc_ObjPrint( FILE * pFile, Abc_Obj_t * pObj );
extern ABC_DLL void               Abc_NtkShow6VarFunc( char * pF0, char * pF1 );
/*=== abcProve.c ==========================================================*/
extern ABC_DLL int                Abc_NtkMiterProve( Abc_Ntk_t ** ppNtk, void * pParams );
extern ABC_DLL int                Abc_NtkIvyProve( Abc_Ntk_t ** ppNtk, void * pPars );
/*=== abcRec3.c ==========================================================*/
extern ABC_DLL void               Abc_NtkRecStart3( Gia_Man_t * p, int nVars, int nCuts, int fFuncOnly, int fVerbose );
extern ABC_DLL void               Abc_NtkRecStop3();
extern ABC_DLL void               Abc_NtkRecAdd3( Abc_Ntk_t * pNtk, int fUseSOPB );
extern ABC_DLL void               Abc_NtkRecPs3(int fPrintLib);
extern ABC_DLL Gia_Man_t *        Abc_NtkRecGetGia3();
extern ABC_DLL int                Abc_NtkRecIsRunning3();
extern ABC_DLL void               Abc_NtkRecLibMerge3(Gia_Man_t * pGia);
extern ABC_DLL int                Abc_NtkRecInputNum3();
//extern ABC_DLL void               Abc_NtkRecFilter3(int nLimit);
/*=== abcReconv.c ==========================================================*/
extern ABC_DLL Abc_ManCut_t *     Abc_NtkManCutStart( int nNodeSizeMax, int nConeSizeMax, int nNodeFanStop, int nConeFanStop );
extern ABC_DLL void               Abc_NtkManCutStop( Abc_ManCut_t * p );
extern ABC_DLL Vec_Ptr_t *        Abc_NtkManCutReadCutLarge( Abc_ManCut_t * p );
extern ABC_DLL Vec_Ptr_t *        Abc_NtkManCutReadCutSmall( Abc_ManCut_t * p );
extern ABC_DLL Vec_Ptr_t *        Abc_NtkManCutReadVisited( Abc_ManCut_t * p );
extern ABC_DLL Vec_Ptr_t *        Abc_NodeFindCut( Abc_ManCut_t * p, Abc_Obj_t * pRoot, int  fContain );
extern ABC_DLL void               Abc_NodeConeCollect( Abc_Obj_t ** ppRoots, int nRoots, Vec_Ptr_t * vFanins, Vec_Ptr_t * vVisited, int fIncludeFanins );
extern ABC_DLL Vec_Ptr_t *        Abc_NodeCollectTfoCands( Abc_ManCut_t * p, Abc_Obj_t * pRoot, Vec_Ptr_t * vFanins, int LevelMax );
/*=== abcRefs.c ==========================================================*/
extern ABC_DLL int                Abc_NodeMffcSize( Abc_Obj_t * pNode );
extern ABC_DLL int                Abc_NodeMffcSizeSupp( Abc_Obj_t * pNode );
extern ABC_DLL int                Abc_NodeMffcSizeStop( Abc_Obj_t * pNode );
extern ABC_DLL int                Abc_NodeMffcLabelAig( Abc_Obj_t * pNode );
extern ABC_DLL int                Abc_NodeMffcLabel( Abc_Obj_t * pNode );
extern ABC_DLL void               Abc_NodeMffcConeSupp( Abc_Obj_t * pNode, Vec_Ptr_t * vCone, Vec_Ptr_t * vSupp );
extern ABC_DLL int                Abc_NodeDeref_rec( Abc_Obj_t * pNode );
extern ABC_DLL int                Abc_NodeRef_rec( Abc_Obj_t * pNode );
/*=== abcRefactor.c ==========================================================*/
extern ABC_DLL int                Abc_NtkRefactor( Abc_Ntk_t * pNtk, int nNodeSizeMax, int nConeSizeMax, int  fUpdateLevel, int  fUseZeros, int  fUseDcs, int  fVerbose );
/*=== abcRewrite.c ==========================================================*/
extern ABC_DLL int                Abc_NtkRewrite( Abc_Ntk_t * pNtk, int fUpdateLevel, int fUseZeros, int fVerbose, int fVeryVerbose, int fPlaceEnable );
/*=== abcSat.c ==========================================================*/
extern ABC_DLL int                Abc_NtkMiterSat( Abc_Ntk_t * pNtk, ABC_INT64_T nConfLimit, ABC_INT64_T nInsLimit, int fVerbose, ABC_INT64_T * pNumConfs, ABC_INT64_T * pNumInspects );
extern ABC_DLL void *             Abc_NtkMiterSatCreate( Abc_Ntk_t * pNtk, int fAllPrimes );
/*=== abcSop.c ==========================================================*/
extern ABC_DLL char *             Abc_SopRegister( Mem_Flex_t * pMan, const char * pName );
extern ABC_DLL char *             Abc_SopStart( Mem_Flex_t * pMan, int nCubes, int nVars );
extern ABC_DLL char *             Abc_SopCreateConst0( Mem_Flex_t * pMan );
extern ABC_DLL char *             Abc_SopCreateConst1( Mem_Flex_t * pMan );
extern ABC_DLL char *             Abc_SopCreateAnd2( Mem_Flex_t * pMan, int fCompl0, int fCompl1 );
extern ABC_DLL char *             Abc_SopCreateAnd( Mem_Flex_t * pMan, int nVars, int * pfCompl );
extern ABC_DLL char *             Abc_SopCreateNand( Mem_Flex_t * pMan, int nVars );
extern ABC_DLL char *             Abc_SopCreateOr( Mem_Flex_t * pMan, int nVars, int * pfCompl );
extern ABC_DLL char *             Abc_SopCreateOrMultiCube( Mem_Flex_t * pMan, int nVars, int * pfCompl );
extern ABC_DLL char *             Abc_SopCreateNor( Mem_Flex_t * pMan, int nVars );
extern ABC_DLL char *             Abc_SopCreateXor( Mem_Flex_t * pMan, int nVars );
extern ABC_DLL char *             Abc_SopCreateXorSpecial( Mem_Flex_t * pMan, int nVars );
extern ABC_DLL char *             Abc_SopCreateNxor( Mem_Flex_t * pMan, int nVars );
extern ABC_DLL char *             Abc_SopCreateMux( Mem_Flex_t * pMan );
extern ABC_DLL char *             Abc_SopCreateInv( Mem_Flex_t * pMan );
extern ABC_DLL char *             Abc_SopCreateBuf( Mem_Flex_t * pMan );
extern ABC_DLL char *             Abc_SopCreateFromTruth( Mem_Flex_t * pMan, int nVars, unsigned * pTruth );
extern ABC_DLL char *             Abc_SopCreateFromIsop( Mem_Flex_t * pMan, int nVars, Vec_Int_t * vCover );
extern ABC_DLL char *             Abc_SopCreateFromTruthIsop( Mem_Flex_t * pMan, int nVars, word * pTruth, Vec_Int_t * vCover );
extern ABC_DLL int                Abc_SopGetCubeNum( char * pSop );
extern ABC_DLL int                Abc_SopGetLitNum( char * pSop );
extern ABC_DLL int                Abc_SopGetVarNum( char * pSop );
extern ABC_DLL int                Abc_SopGetPhase( char * pSop );
extern ABC_DLL int                Abc_SopGetIthCareLit( char * pSop, int i );
extern ABC_DLL void               Abc_SopComplement( char * pSop );
extern ABC_DLL void               Abc_SopComplementVar( char * pSop, int iVar );
extern ABC_DLL int                Abc_SopIsComplement( char * pSop );
extern ABC_DLL int                Abc_SopIsConst0( char * pSop );
extern ABC_DLL int                Abc_SopIsConst1( char * pSop );
extern ABC_DLL int                Abc_SopIsBuf( char * pSop );
extern ABC_DLL int                Abc_SopIsInv( char * pSop );
extern ABC_DLL int                Abc_SopIsAndType( char * pSop );
extern ABC_DLL int                Abc_SopIsOrType( char * pSop );
extern ABC_DLL int                Abc_SopIsExorType( char * pSop );
extern ABC_DLL int                Abc_SopCheck( char * pSop, int nFanins );
extern ABC_DLL char *             Abc_SopFromTruthBin( char * pTruth );
extern ABC_DLL char *             Abc_SopFromTruthHex( char * pTruth );
extern ABC_DLL char *             Abc_SopEncoderPos( Mem_Flex_t * pMan, int iValue, int nValues );
extern ABC_DLL char *             Abc_SopEncoderLog( Mem_Flex_t * pMan, int iBit, int nValues );
extern ABC_DLL char *             Abc_SopDecoderPos( Mem_Flex_t * pMan, int nValues );
extern ABC_DLL char *             Abc_SopDecoderLog( Mem_Flex_t * pMan, int nValues );
extern ABC_DLL word               Abc_SopToTruth( char * pSop, int nInputs );
extern ABC_DLL void               Abc_SopToTruth7( char * pSop, int nInputs, word r[2] );
extern ABC_DLL void               Abc_SopToTruthBig( char * pSop, int nInputs, word ** pVars, word * pCube, word * pRes );
/*=== abcStrash.c ==========================================================*/
extern ABC_DLL Abc_Ntk_t *        Abc_NtkRestrash( Abc_Ntk_t * pNtk, int fCleanup );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkRestrashZero( Abc_Ntk_t * pNtk, int fCleanup );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkStrash( Abc_Ntk_t * pNtk, int fAllNodes, int fCleanup, int fRecord );
extern ABC_DLL Abc_Obj_t *        Abc_NodeStrash( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pNode, int fRecord );
extern ABC_DLL int                Abc_NtkAppend( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int fAddPos );
extern ABC_DLL Abc_Ntk_t *        Abc_NtkTopmost( Abc_Ntk_t * pNtk, int nLevels );
/*=== abcSweep.c ==========================================================*/
extern ABC_DLL int                Abc_NtkSweep( Abc_Ntk_t * pNtk, int fVerbose );
extern ABC_DLL int                Abc_NtkCleanup( Abc_Ntk_t * pNtk, int fVerbose );
extern ABC_DLL int                Abc_NtkCleanupNodes( Abc_Ntk_t * pNtk, Vec_Ptr_t * vNodes, int fVerbose );
extern ABC_DLL int                Abc_NtkCleanupSeq( Abc_Ntk_t * pNtk, int fLatchSweep, int fAutoSweep, int fVerbose );
extern ABC_DLL int                Abc_NtkSweepBufsInvs( Abc_Ntk_t * pNtk, int fVerbose );
/*=== abcTiming.c ==========================================================*/
extern ABC_DLL Abc_Time_t *       Abc_NtkReadDefaultArrival( Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Time_t *       Abc_NtkReadDefaultRequired( Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Time_t *       Abc_NodeReadArrival( Abc_Obj_t * pNode );
extern ABC_DLL Abc_Time_t *       Abc_NodeReadRequired( Abc_Obj_t * pNode );
extern ABC_DLL float              Abc_NtkReadDefaultArrivalWorst( Abc_Ntk_t * pNtk );
extern ABC_DLL float              Abc_NtkReadDefaultRequiredWorst( Abc_Ntk_t * pNtk );
extern ABC_DLL float              Abc_NodeReadArrivalAve( Abc_Obj_t * pNode );
extern ABC_DLL float              Abc_NodeReadRequiredAve( Abc_Obj_t * pNode );
extern ABC_DLL float              Abc_NodeReadArrivalWorst( Abc_Obj_t * pNode );
extern ABC_DLL float              Abc_NodeReadRequiredWorst( Abc_Obj_t * pNode );
extern ABC_DLL Abc_Time_t *       Abc_NtkReadDefaultInputDrive( Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Time_t *       Abc_NtkReadDefaultOutputLoad( Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Time_t *       Abc_NodeReadInputDrive( Abc_Ntk_t * pNtk, int iPi );
extern ABC_DLL Abc_Time_t *       Abc_NodeReadOutputLoad( Abc_Ntk_t * pNtk, int iPo );
extern ABC_DLL float              Abc_NodeReadInputDriveWorst( Abc_Ntk_t * pNtk, int iPi );
extern ABC_DLL float              Abc_NodeReadOutputLoadWorst( Abc_Ntk_t * pNtk, int iPo );
extern ABC_DLL void               Abc_NtkTimeSetDefaultArrival( Abc_Ntk_t * pNtk, float Rise, float Fall );
extern ABC_DLL void               Abc_NtkTimeSetDefaultRequired( Abc_Ntk_t * pNtk, float Rise, float Fall );
extern ABC_DLL void               Abc_NtkTimeSetArrival( Abc_Ntk_t * pNtk, int ObjId, float Rise, float Fall );
extern ABC_DLL void               Abc_NtkTimeSetRequired( Abc_Ntk_t * pNtk, int ObjId, float Rise, float Fall );
extern ABC_DLL void               Abc_NtkTimeSetDefaultInputDrive( Abc_Ntk_t * pNtk, float Rise, float Fall );
extern ABC_DLL void               Abc_NtkTimeSetDefaultOutputLoad( Abc_Ntk_t * pNtk, float Rise, float Fall );
extern ABC_DLL void               Abc_NtkTimeSetInputDrive( Abc_Ntk_t * pNtk, int PiNum, float Rise, float Fall );
extern ABC_DLL void               Abc_NtkTimeSetOutputLoad( Abc_Ntk_t * pNtk, int PoNum, float Rise, float Fall );
extern ABC_DLL void               Abc_NtkTimeInitialize( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkOld );
extern ABC_DLL void               Abc_ManTimeStop( Abc_ManTime_t * p );
extern ABC_DLL void               Abc_ManTimeDup( Abc_Ntk_t * pNtkOld, Abc_Ntk_t * pNtkNew );
extern ABC_DLL void               Abc_NtkSetNodeLevelsArrival( Abc_Ntk_t * pNtk );
extern ABC_DLL float *            Abc_NtkGetCiArrivalFloats( Abc_Ntk_t * pNtk );
extern ABC_DLL float *            Abc_NtkGetCoRequiredFloats( Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Time_t *       Abc_NtkGetCiArrivalTimes( Abc_Ntk_t * pNtk );
extern ABC_DLL Abc_Time_t *       Abc_NtkGetCoRequiredTimes( Abc_Ntk_t * pNtk );
extern ABC_DLL float              Abc_NtkDelayTrace( Abc_Ntk_t * pNtk, Abc_Obj_t * pOut, Abc_Obj_t * pIn, int fPrint );
extern ABC_DLL int                Abc_ObjLevelNew( Abc_Obj_t * pObj );
extern ABC_DLL int                Abc_ObjReverseLevelNew( Abc_Obj_t * pObj );
extern ABC_DLL int                Abc_ObjRequiredLevel( Abc_Obj_t * pObj );
extern ABC_DLL int                Abc_ObjReverseLevel( Abc_Obj_t * pObj );
extern ABC_DLL void               Abc_ObjSetReverseLevel( Abc_Obj_t * pObj, int LevelR );
extern ABC_DLL void               Abc_NtkStartReverseLevels( Abc_Ntk_t * pNtk, int nMaxLevelIncrease );
extern ABC_DLL void               Abc_NtkStopReverseLevels( Abc_Ntk_t * pNtk );
extern ABC_DLL void               Abc_NtkUpdateLevel( Abc_Obj_t * pObjNew, Vec_Vec_t * vLevels );
extern ABC_DLL void               Abc_NtkUpdateReverseLevel( Abc_Obj_t * pObjNew, Vec_Vec_t * vLevels );
extern ABC_DLL void               Abc_NtkUpdate( Abc_Obj_t * pObj, Abc_Obj_t * pObjNew, Vec_Vec_t * vLevels );
/*=== abcUtil.c ==========================================================*/
extern ABC_DLL void *             Abc_NtkAttrFree( Abc_Ntk_t * pNtk, int Attr, int fFreeMan );
extern ABC_DLL void               Abc_NtkOrderCisCos( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkGetCubeNum( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkGetCubePairNum( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkGetLitNum( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkGetLitFactNum( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkGetBddNodeNum( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkGetAigNodeNum( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkGetClauseNum( Abc_Ntk_t * pNtk );
extern ABC_DLL double             Abc_NtkGetMappedArea( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkGetExorNum( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkGetMuxNum( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkGetBufNum( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkGetLargeNodeNum( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkGetChoiceNum( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkGetFaninMax( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkGetFanoutMax( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkGetTotalFanins( Abc_Ntk_t * pNtk );
extern ABC_DLL void               Abc_NtkCleanCopy( Abc_Ntk_t * pNtk );
extern ABC_DLL void               Abc_NtkCleanCopy_rec( Abc_Ntk_t * pNtk );
extern ABC_DLL void               Abc_NtkCleanData( Abc_Ntk_t * pNtk );
extern ABC_DLL void               Abc_NtkFillTemp( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkCountCopy( Abc_Ntk_t * pNtk );
extern ABC_DLL Vec_Ptr_t *        Abc_NtkSaveCopy( Abc_Ntk_t * pNtk );
extern ABC_DLL void               Abc_NtkLoadCopy( Abc_Ntk_t * pNtk, Vec_Ptr_t * vCopies );
extern ABC_DLL void               Abc_NtkCleanNext( Abc_Ntk_t * pNtk );
extern ABC_DLL void               Abc_NtkCleanNext_rec( Abc_Ntk_t * pNtk );
extern ABC_DLL void               Abc_NtkCleanMarkA( Abc_Ntk_t * pNtk );
extern ABC_DLL void               Abc_NtkCleanMarkB( Abc_Ntk_t * pNtk );
extern ABC_DLL void               Abc_NtkCleanMarkC( Abc_Ntk_t * pNtk );
extern ABC_DLL void               Abc_NtkCleanMarkAB( Abc_Ntk_t * pNtk );
extern ABC_DLL void               Abc_NtkCleanMarkABC( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NodeFindFanin( Abc_Obj_t * pNode, Abc_Obj_t * pFanin );
extern ABC_DLL Abc_Obj_t *        Abc_NodeFindCoFanout( Abc_Obj_t * pNode );
extern ABC_DLL Abc_Obj_t *        Abc_NodeFindNonCoFanout( Abc_Obj_t * pNode );
extern ABC_DLL Abc_Obj_t *        Abc_NodeHasUniqueCoFanout( Abc_Obj_t * pNode );
extern ABC_DLL int                Abc_NtkLogicHasSimpleCos( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkLogicMakeSimpleCos( Abc_Ntk_t * pNtk, int  fDuplicate );
extern ABC_DLL void               Abc_VecObjPushUniqueOrderByLevel( Vec_Ptr_t * p, Abc_Obj_t * pNode );
extern ABC_DLL int                Abc_NodeIsExorType( Abc_Obj_t * pNode );
extern ABC_DLL int                Abc_NodeIsMuxType( Abc_Obj_t * pNode );
extern ABC_DLL int                Abc_NodeIsMuxControlType( Abc_Obj_t * pNode );
extern ABC_DLL Abc_Obj_t *        Abc_NodeRecognizeMux( Abc_Obj_t * pNode, Abc_Obj_t ** ppNodeT, Abc_Obj_t ** ppNodeE );
extern ABC_DLL int                Abc_NtkPrepareTwoNtks( FILE * pErr, Abc_Ntk_t * pNtk, char ** argv, int argc, Abc_Ntk_t ** ppNtk1, Abc_Ntk_t ** ppNtk2, int * pfDelete1, int * pfDelete2, int fCheck );
extern ABC_DLL void               Abc_NodeCollectFanins( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes );
extern ABC_DLL void               Abc_NodeCollectFanouts( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes );
extern ABC_DLL Vec_Ptr_t *        Abc_NtkCollectLatches( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NodeCompareLevelsIncrease( Abc_Obj_t ** pp1, Abc_Obj_t ** pp2 );
extern ABC_DLL int                Abc_NodeCompareLevelsDecrease( Abc_Obj_t ** pp1, Abc_Obj_t ** pp2 );
extern ABC_DLL Vec_Int_t *        Abc_NtkFanoutCounts( Abc_Ntk_t * pNtk );
extern ABC_DLL Vec_Ptr_t *        Abc_NtkCollectObjects( Abc_Ntk_t * pNtk );
extern ABC_DLL Vec_Int_t *        Abc_NtkGetCiIds( Abc_Ntk_t * pNtk );
extern ABC_DLL void               Abc_NtkReassignIds( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_ObjPointerCompare( void ** pp1, void ** pp2 );
extern ABC_DLL void               Abc_NtkTransferCopy( Abc_Ntk_t * pNtk );
extern ABC_DLL void               Abc_NtkInvertConstraints( Abc_Ntk_t * pNtk );
extern ABC_DLL void               Abc_NtkPrintCiLevels( Abc_Ntk_t * pNtk );
extern ABC_DLL void               Abc_NtkReverseTopoOrder( Abc_Ntk_t * pNtk );
extern ABC_DLL int                Abc_NtkIsTopo( Abc_Ntk_t * pNtk );
extern ABC_DLL void               Abc_NtkTransferPhases( Abc_Ntk_t * pNtkNew, Abc_Ntk_t * pNtk );
extern ABC_DLL Gia_Man_t *        Abc_SopSynthesizeOne( char * pSop, int fClp );



/*=== abcVerify.c ==========================================================*/
extern ABC_DLL int *              Abc_NtkVerifyGetCleanModel( Abc_Ntk_t * pNtk, int nFrames );
extern ABC_DLL int *              Abc_NtkVerifySimulatePattern( Abc_Ntk_t * pNtk, int * pModel );
extern ABC_DLL int                Abc_NtkIsTrueCex( Abc_Ntk_t * pNtk, Abc_Cex_t * pCex );
extern ABC_DLL int                Abc_NtkIsValidCex( Abc_Ntk_t * pNtk, Abc_Cex_t * pCex );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
