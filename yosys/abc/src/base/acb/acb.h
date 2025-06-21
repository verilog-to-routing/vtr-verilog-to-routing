/**CFile****************************************************************

  FileName    [acb.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchical word-level netlist.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 21, 2015.]

  Revision    [$Id: acb.h,v 1.00 2014/11/29 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__base__acb__acb_h
#define ABC__base__acb__acb_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "aig/gia/gia.h"
#include "misc/extra/extra.h"
#include "misc/util/utilNam.h"
#include "misc/vec/vecHash.h"
#include "aig/miniaig/abcOper.h"
#include "misc/vec/vecQue.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START 

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////


typedef struct Acb_Aig_t_ Acb_Aig_t;
typedef struct Acb_Ntk_t_ Acb_Ntk_t;
typedef struct Acb_Man_t_ Acb_Man_t;

// network
struct Acb_Ntk_t_
{
    Acb_Man_t *     pDesign;    // design
    int             Id;         // network ID
    int             NameId;     // name ID 
    int             iCopy;      // copy module
    unsigned        Mark : 1;   // visit mark 
    unsigned        fComb: 1;   // the network is combinational
    unsigned        fSeq : 1;   // the network is sequential
    // interface
    Vec_Int_t       vCis;       // inputs 
    Vec_Int_t       vCos;       // outputs
    Vec_Int_t       vOrder;     // order
    Vec_Int_t       vSeq;       // sequential objects
    int             nRegs;      // flop count
    int             nFaninMax;  // default fanin count
    int             nObjTravs;  // trav ID
    int             LevelMax;   // max level
    int             nPaths;     // the number of paths
    // stucture
    Vec_Str_t       vObjType;   // type     
    Vec_Int_t       vObjFans;   // fanin offsets
    Vec_Int_t       vFanSto;    // fanin storage
    // optional
    Vec_Int_t       vObjCopy;   // copy
    Vec_Int_t       vObjFunc;   // function
    Vec_Int_t       vObjWeight; // weight
    Vec_Wrd_t       vObjTruth;  // function
    Vec_Int_t       vObjName;   // name
    Vec_Int_t       vObjRange;  // range
    Vec_Int_t       vObjTrav;   // trav ID
    Vec_Int_t       vObjBits;   // obj mapping into AIG nodes
    Vec_Int_t       vObjAttr;   // attribute offset
    Vec_Int_t       vAttrSto;   // attribute storage
    Vec_Int_t       vNtkObjs;   // instances
    Vec_Int_t       vTargets;   // targets
    Vec_Int_t       vLevelD;    // level
    Vec_Int_t       vLevelR;    // level
    Vec_Int_t       vPathD;     // path
    Vec_Int_t       vPathR;     // path
    Vec_Flt_t       vCounts;    // priority counts
    Vec_Wec_t       vFanouts;   // fanouts
    Vec_Wec_t       vCnfs;      // CNF
    Vec_Str_t       vCnf;       // CNF
    Vec_Int_t       vSuppOld;   // previous support
    // other
    Vec_Que_t *     vQue;       // temporary
    Vec_Int_t       vCover;     // temporary
    Vec_Int_t       vArray0;    // temporary
    Vec_Int_t       vArray1;    // temporary
    Vec_Int_t       vArray2;    // temporary
};

// design
struct Acb_Man_t_
{
    char *          pName;      // design name
    char *          pSpec;      // spec file name
    Abc_Nam_t *     pStrs;      // string manager
    Abc_Nam_t *     pFuns;      // constant manager
    Abc_Nam_t *     pMods;      // module name manager
    Hash_IntMan_t * vHash;      // variable ranges
    Vec_Int_t       vNameMap;   // mapping names
    Vec_Int_t       vNameMap2;  // mapping names
    Vec_Int_t       vUsed;      // used map entries
    Vec_Int_t       vUsed2;     // used map entries
    char *          pTypeNames[ABC_OPER_LAST];
    int             nObjs[ABC_OPER_LAST]; // counter of objects of each type
    int             nAnds[ABC_OPER_LAST]; // counter of AND gates after blasting
    // internal data
    int             iRoot;      // root network
    Vec_Ptr_t       vNtks;      // networks
    // user data
    int             nOpens;
    Vec_Str_t       vOut;     
    Vec_Str_t       vOut2;     
    void *          pMioLib;
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

static inline char *         Acb_ManName( Acb_Man_t * p )                    { return p->pName;                                                                            }
static inline char *         Acb_ManSpec( Acb_Man_t * p )                    { return p->pSpec;                                                                            }
static inline int            Acb_ManNtkNum( Acb_Man_t * p )                  { return Vec_PtrSize(&p->vNtks)-1;                                                            }
static inline int            Acb_ManNtkIsOk( Acb_Man_t * p, int i )          { return i > 0 && i <= Acb_ManNtkNum(p);                                                      }
static inline Acb_Ntk_t *    Acb_ManNtk( Acb_Man_t * p, int i )              { return Acb_ManNtkIsOk(p, i) ? (Acb_Ntk_t *)Vec_PtrEntry(&p->vNtks, i) : NULL;               }
static inline int            Acb_ManNtkFindId( Acb_Man_t * p, char * pName ) { return Abc_NamStrFind(p->pMods, pName);                                                     }
static inline Acb_Ntk_t *    Acb_ManNtkFind( Acb_Man_t * p, char * pName )   { return Acb_ManNtk( p, Acb_ManNtkFindId(p, pName) );                                         }
static inline Acb_Ntk_t *    Acb_ManRoot( Acb_Man_t * p )                    { return Acb_ManNtk(p, p->iRoot);                                                             }
static inline char *         Acb_ManStr( Acb_Man_t * p, int i )              { return Abc_NamStr(p->pStrs, i);                                                             }
static inline int            Acb_ManStrId( Acb_Man_t * p, char * pStr )      { return Abc_NamStrFind(p->pStrs, pStr);                                                      }
static inline int            Acb_ManNameIdMax( Acb_Man_t * p )               { return Abc_NamObjNumMax(p->pStrs) + 1;                                                      }
static inline char *         Acb_ManConst( Acb_Man_t * p, int i )            { return Abc_NamStr(p->pFuns, i);                                                             }
static inline int            Acb_ManConstId( Acb_Man_t * p, char * pStr )    { return Abc_NamStrFind(p->pFuns, pStr);                                                      }
static inline int            Acb_ManConstIdMax( Acb_Man_t * p )              { return Abc_NamObjNumMax(p->pFuns) + 1;                                                      }

static inline Acb_Man_t *    Acb_NtkMan( Acb_Ntk_t * p )                     { return p->pDesign;                                                                          }
static inline Acb_Ntk_t *    Acb_NtkNtk( Acb_Ntk_t * p, int i )              { return Acb_ManNtk(p->pDesign, i);                                                           }
static inline int            Acb_NtkId( Acb_Ntk_t * p )                      { return p->Id;                                                                               }
static inline int            Acb_NtkCi( Acb_Ntk_t * p, int i )               { return Vec_IntEntry(&p->vCis, i);                                                           }
static inline int            Acb_NtkCo( Acb_Ntk_t * p, int i )               { return Vec_IntEntry(&p->vCos, i);                                                           }
static inline int            Acb_NtkCioOrder( Acb_Ntk_t * p, int i )         { return Vec_IntEntry(&p->vOrder, i);                                                         }
static inline int            Acb_NtkBoxSeq( Acb_Ntk_t * p, int i )           { return Vec_IntEntry(&p->vSeq, i);                                                           }
static inline Abc_Nam_t *    Acb_NtkNam( Acb_Ntk_t * p )                     { return p->pDesign->pStrs;                                                                   }
static inline char *         Acb_NtkStr( Acb_Ntk_t * p, int i )              { return Acb_ManStr(p->pDesign, i);                                                           }
static inline int            Acb_NtkStrId( Acb_Ntk_t * p, char * pName )     { return Acb_ManStrId(p->pDesign, pName);                                                     }
static inline char *         Acb_NtkConst( Acb_Ntk_t * p, int i )            { return Acb_ManConst(p->pDesign, i);                                                         }
static inline char *         Acb_NtkSop( Acb_Ntk_t * p, int i )              { return Acb_ManConst(p->pDesign, i);                                                         }
static inline int            Acb_NtkNameId( Acb_Ntk_t * p )                  { return p->NameId;                                                                           }
static inline char *         Acb_NtkName( Acb_Ntk_t * p )                    { return Acb_NtkStr(p, p->NameId);                                                            }
static inline char *         Acb_NtkTypeName( Acb_Ntk_t * p, int Type )      { return p->pDesign->pTypeNames[Type];                                                        }
static inline int            Acb_NtkCopy( Acb_Ntk_t * p )                    { return p->iCopy;                                                                            }
static inline Acb_Ntk_t *    Acb_NtkCopyNtk(Acb_Man_t * pNew, Acb_Ntk_t * p) { return Acb_ManNtk(pNew, Acb_NtkCopy(p));                                                    }
static inline void           Acb_NtkSetCopy( Acb_Ntk_t * p, int i )          { assert(p->iCopy == 0); p->iCopy = i;                                                        }
static inline int            Acb_NtkHashRange( Acb_Ntk_t * p, int l, int r ) { return Hash_Int2ManInsert( p->pDesign->vHash, l, r, 0 );                                    }
static inline int            Acb_NtkRangeLeft( Acb_Ntk_t * p, int h )        { return h ? Hash_IntObjData0( p->pDesign->vHash, h ) : 0;                                    }
static inline int            Acb_NtkRangeRight( Acb_Ntk_t * p, int h )       { return h ? Hash_IntObjData1( p->pDesign->vHash, h ) : 0;                                    }
static inline int            Acb_NtkRangeSize( Acb_Ntk_t * p, int h )        { int l = Acb_NtkRangeLeft(p, h), r = Acb_NtkRangeRight(p, h); return 1 + (l > r ? l-r : r-l);}

static inline int            Acb_NtkCiNum( Acb_Ntk_t * p )                   { return Vec_IntSize(&p->vCis);                                                               }
static inline int            Acb_NtkCoNum( Acb_Ntk_t * p )                   { return Vec_IntSize(&p->vCos);                                                               }
static inline int            Acb_NtkCioNum( Acb_Ntk_t * p )                  { return Acb_NtkCiNum(p) + Acb_NtkCoNum(p);                                                   }
static inline int            Acb_NtkCiNumAlloc( Acb_Ntk_t * p )              { return Vec_IntCap(&p->vCis);                                                                }
static inline int            Acb_NtkCoNumAlloc( Acb_Ntk_t * p )              { return Vec_IntCap(&p->vCos);                                                                }
static inline int            Acb_NtkRegNum( Acb_Ntk_t * p )                  { return p->nRegs;                                                                            }
static inline void           Acb_NtkSetRegNum( Acb_Ntk_t * p, int nRegs )    { p->nRegs = nRegs;                                                                           }
static inline int            Acb_NtkPiNum( Acb_Ntk_t * p )                   { return Acb_NtkCiNum(p) - Acb_NtkRegNum(p);                                                  }
static inline int            Acb_NtkPoNum( Acb_Ntk_t * p )                   { return Acb_NtkCoNum(p) - Acb_NtkRegNum(p);                                                  }
static inline int            Acb_NtkCioOrderNum( Acb_Ntk_t * p )             { return Vec_IntSize(&p->vOrder);                                                             }
static inline int            Acb_NtkObjNumAlloc( Acb_Ntk_t * p )             { return Vec_StrCap(&p->vObjType)-1;                                                          }
static inline int            Acb_NtkObjNum( Acb_Ntk_t * p )                  { return Vec_StrSize(&p->vObjType)-1;                                                         }
static inline int            Acb_NtkObjNumMax( Acb_Ntk_t * p )               { return Vec_StrSize(&p->vObjType);                                                           }
static inline int            Acb_NtkTypeNum( Acb_Ntk_t * p, int Type )       { return Vec_StrCountEntry(&p->vObjType, (char)Type);                                         }
static inline int            Acb_NtkBoxNum( Acb_Ntk_t * p )                  { return Acb_NtkTypeNum(p, ABC_OPER_BOX);                                                     }
static inline int            Acb_NtkNodeNum( Acb_Ntk_t * p )                 { return Vec_StrCountLarger(&p->vObjType, (char)ABC_OPER_BOX);                                }
static inline int            Acb_NtkSeqNum( Acb_Ntk_t * p )                  { return Vec_IntSize(&p->vSeq);                                                               }

static inline void           Acb_NtkCleanObjCopies( Acb_Ntk_t * p )          { Vec_IntFill(&p->vObjCopy,  Vec_StrCap(&p->vObjType), -1);                                   }
static inline void           Acb_NtkCleanObjFuncs( Acb_Ntk_t * p )           { Vec_IntFill(&p->vObjFunc,  Vec_StrCap(&p->vObjType), -1);                                   }
static inline void           Acb_NtkCleanObjWeights( Acb_Ntk_t * p )         { Vec_IntFill(&p->vObjWeight,Vec_StrCap(&p->vObjType),  0);                                   }
static inline void           Acb_NtkCleanObjTruths( Acb_Ntk_t * p )          { Vec_WrdFill(&p->vObjTruth, Vec_StrCap(&p->vObjType),  0);                                   }
static inline void           Acb_NtkCleanObjNames( Acb_Ntk_t * p )           { Vec_IntFill(&p->vObjName,  Vec_StrCap(&p->vObjType),  0);                                   }
static inline void           Acb_NtkCleanObjRanges( Acb_Ntk_t * p )          { Vec_IntFill(&p->vObjRange, Vec_StrCap(&p->vObjType),  0);                                   }
static inline void           Acb_NtkCleanObjTravs( Acb_Ntk_t * p )           { Vec_IntFill(&p->vObjTrav,  Vec_StrCap(&p->vObjType),  0);                                   }
static inline void           Acb_NtkCleanObjAttrs( Acb_Ntk_t * p )           { Vec_IntFill(&p->vObjAttr,  Vec_StrCap(&p->vObjType),  0); Vec_IntFill(&p->vAttrSto, 1, -1); }
static inline void           Acb_NtkCleanObjLevelD( Acb_Ntk_t * p )          { Vec_IntFill(&p->vLevelD,   Vec_StrCap(&p->vObjType),  0);                                   }
static inline void           Acb_NtkCleanObjLevelR( Acb_Ntk_t * p )          { Vec_IntFill(&p->vLevelR,   Vec_StrCap(&p->vObjType),  0);                                   }
static inline void           Acb_NtkCleanObjPathD( Acb_Ntk_t * p )           { Vec_IntFill(&p->vPathD,    Vec_StrCap(&p->vObjType),  0);                                   }
static inline void           Acb_NtkCleanObjPathR( Acb_Ntk_t * p )           { Vec_IntFill(&p->vPathR,    Vec_StrCap(&p->vObjType),  0);                                   }
static inline void           Acb_NtkCleanObjCounts( Acb_Ntk_t * p )          { Vec_FltFill(&p->vCounts,   Vec_StrCap(&p->vObjType),  0);                                   }
static inline void           Acb_NtkCleanObjFanout( Acb_Ntk_t * p )          { Vec_WecInit(&p->vFanouts,  Vec_StrCap(&p->vObjType)    );                                   }
static inline void           Acb_NtkCleanObjCnfs( Acb_Ntk_t * p )            { Vec_WecInit(&p->vCnfs,     Vec_StrCap(&p->vObjType)    );                                   }

static inline int            Acb_NtkHasObjCopies( Acb_Ntk_t * p )            { return Vec_IntSize(&p->vObjCopy)  > 0;                                                      }
static inline int            Acb_NtkHasObjFuncs( Acb_Ntk_t * p )             { return Vec_IntSize(&p->vObjFunc)  > 0;                                                      }
static inline int            Acb_NtkHasObjWeights( Acb_Ntk_t * p )           { return Vec_IntSize(&p->vObjWeight)> 0;                                                      }
static inline int            Acb_NtkHasObjTruths( Acb_Ntk_t * p )            { return Vec_WrdSize(&p->vObjTruth) > 0;                                                      }
static inline int            Acb_NtkHasObjNames( Acb_Ntk_t * p )             { return Vec_IntSize(&p->vObjName)  > 0;                                                      }
static inline int            Acb_NtkHasObjRanges( Acb_Ntk_t * p )            { return Vec_IntSize(&p->vObjRange) > 0;                                                      }
static inline int            Acb_NtkHasObjTravs( Acb_Ntk_t * p )             { return Vec_IntSize(&p->vObjTrav)  > 0;                                                      }
static inline int            Acb_NtkHasObjAttrs( Acb_Ntk_t * p )             { return Vec_IntSize(&p->vObjAttr)  > 0;                                                      }
static inline int            Acb_NtkHasObjLevelD( Acb_Ntk_t * p )            { return Vec_IntSize(&p->vLevelD)   > 0;                                                      }
static inline int            Acb_NtkHasObjLevelR( Acb_Ntk_t * p )            { return Vec_IntSize(&p->vLevelR)   > 0;                                                      }
static inline int            Acb_NtkHasObjPathD( Acb_Ntk_t * p )             { return Vec_IntSize(&p->vPathD)    > 0;                                                      }
static inline int            Acb_NtkHasObjPathR( Acb_Ntk_t * p )             { return Vec_IntSize(&p->vPathR)    > 0;                                                      }
static inline int            Acb_NtkHasObjCounts( Acb_Ntk_t * p )            { return Vec_FltSize(&p->vCounts)   > 0;                                                      }
static inline int            Acb_NtkHasObjFanout( Acb_Ntk_t * p )            { return Vec_WecSize(&p->vFanouts)  > 0;                                                      }
static inline int            Acb_NtkHasObjCnfs( Acb_Ntk_t * p )              { return Vec_WecSize(&p->vCnfs)     > 0;                                                      }

static inline void           Acb_NtkFreeObjCopies( Acb_Ntk_t * p )           { Vec_IntErase(&p->vObjCopy);                                                                 }
static inline void           Acb_NtkFreeObjFuncs( Acb_Ntk_t * p )            { Vec_IntErase(&p->vObjFunc);                                                                 }
static inline void           Acb_NtkFreeObjWeights( Acb_Ntk_t * p )          { Vec_IntErase(&p->vObjWeight);                                                               }
static inline void           Acb_NtkFreeObjTruths( Acb_Ntk_t * p )           { Vec_WrdErase(&p->vObjTruth);                                                                }
static inline void           Acb_NtkFreeObjNames( Acb_Ntk_t * p )            { Vec_IntErase(&p->vObjName);                                                                 }
static inline void           Acb_NtkFreeObjRanges( Acb_Ntk_t * p )           { Vec_IntErase(&p->vObjRange);                                                                }
static inline void           Acb_NtkFreeObjTravs( Acb_Ntk_t * p )            { Vec_IntErase(&p->vObjTrav);                                                                 }
static inline void           Acb_NtkFreeObjAttrs( Acb_Ntk_t * p )            { Vec_IntErase(&p->vObjAttr);                                                                 }
static inline void           Acb_NtkFreeObjLevelD( Acb_Ntk_t * p )           { Vec_IntErase(&p->vLevelD);                                                                  }
static inline void           Acb_NtkFreeObjLevelR( Acb_Ntk_t * p )           { Vec_IntErase(&p->vLevelR);                                                                  }
static inline void           Acb_NtkFreeObjPathD( Acb_Ntk_t * p )            { Vec_IntErase(&p->vPathD);                                                                   }
static inline void           Acb_NtkFreeObjPathR( Acb_Ntk_t * p )            { Vec_IntErase(&p->vPathR);                                                                   }
static inline void           Acb_NtkFreeObjCounts( Acb_Ntk_t * p )           { Vec_FltErase(&p->vCounts);                                                                  }
static inline void           Acb_NtkFreeObjFanout( Acb_Ntk_t * p )           { Vec_WecErase(&p->vFanouts);                                                                 }
static inline void           Acb_NtkFreeObjCnfs( Acb_Ntk_t * p )             { Vec_WecErase(&p->vCnfs);                                                                    }

static inline Acb_ObjType_t  Acb_ObjType( Acb_Ntk_t * p, int i )                     { assert(i>0); return (Acb_ObjType_t)(int)(unsigned char)Vec_StrEntry(&p->vObjType, i);                }
static inline void           Acb_ObjCleanType( Acb_Ntk_t * p, int i )                { assert(i>0); Vec_StrWriteEntry( &p->vObjType, i, (char)ABC_OPER_NONE );                              }
static inline int            Acb_TypeIsSeq( Acb_ObjType_t Type )                     { return Type >= ABC_OPER_RAM && Type <= ABC_OPER_DFFLAST;                                             }
static inline int            Acb_TypeIsUnary( Acb_ObjType_t Type )                   { return Type == ABC_OPER_BIT_BUF || Type == ABC_OPER_BIT_INV || Type == ABC_OPER_LOGIC_NOT || Type == ABC_OPER_ARI_MIN || Type == ABC_OPER_ARI_SQRT || Type == ABC_OPER_ARI_ABS || (Type >= ABC_OPER_RED_AND && Type <= ABC_OPER_RED_NXOR);  }
static inline int            Acb_TypeIsMux( Acb_ObjType_t Type )                     { return Type == ABC_OPER_BIT_MUX || Type == ABC_OPER_SEL_NMUX || Type == ABC_OPER_SEL_SEL || Type == ABC_OPER_SEL_PSEL;  }

static inline int            Acb_ObjIsCi( Acb_Ntk_t * p, int i )                     { return Acb_ObjType(p, i) == ABC_OPER_CI;                                                             }
static inline int            Acb_ObjIsCo( Acb_Ntk_t * p, int i )                     { return Acb_ObjType(p, i) == ABC_OPER_CO;                                                             }
static inline int            Acb_ObjIsCio( Acb_Ntk_t * p, int i )                    { return Acb_ObjIsCi(p, i) || Acb_ObjIsCo(p, i);                                                       }
static inline int            Acb_ObjIsFon( Acb_Ntk_t * p, int i )                    { return Acb_ObjType(p, i) == ABC_OPER_FON;                                                            }
static inline int            Acb_ObjIsBox( Acb_Ntk_t * p, int i )                    { return Acb_ObjType(p, i) == ABC_OPER_BOX;                                                            }
static inline int            Acb_ObjIsGate( Acb_Ntk_t * p, int i )                   { return Acb_ObjType(p, i) == ABC_OPER_GATE;                                                           }
static inline int            Acb_ObjIsSlice( Acb_Ntk_t * p, int i )                  { return Acb_ObjType(p, i) == ABC_OPER_SLICE;                                                          }
static inline int            Acb_ObjIsConcat( Acb_Ntk_t * p, int i )                 { return Acb_ObjType(p, i) == ABC_OPER_CONCAT;                                                         }
static inline int            Acb_ObjIsUnary( Acb_Ntk_t * p, int i )                  { return Acb_TypeIsUnary(Acb_ObjType(p, i));                                                           }
 
static inline int            Acb_ObjFanOffset( Acb_Ntk_t * p, int i )                { assert(i>0); return Vec_IntEntry(&p->vObjFans, i);                                                   }
static inline int *          Acb_ObjFanins( Acb_Ntk_t * p, int i )                   { return Vec_IntEntryP(&p->vFanSto, Acb_ObjFanOffset(p, i));                                           }
static inline int            Acb_ObjFanin( Acb_Ntk_t * p, int i, int k )             { return Acb_ObjFanins(p, i)[k+1];                                                                     }
static inline int            Acb_ObjFaninNum( Acb_Ntk_t * p, int i )                 { return Acb_ObjFanins(p, i)[0];                                                                       }
static inline int            Acb_ObjFanoutNum( Acb_Ntk_t * p, int i )                { return Vec_IntSize( Vec_WecEntry(&p->vFanouts, i) );                                                 }
static inline Vec_Int_t *    Acb_ObjFanoutVec( Acb_Ntk_t * p, int i )                { assert(i>0); return Vec_WecEntry( &p->vFanouts, i );                                                 }
static inline int            Acb_ObjFanout( Acb_Ntk_t * p, int i, int k )            { return Vec_IntEntry( Acb_ObjFanoutVec(p, i), k );                                                    }
static inline int            Acb_ObjFanin0( Acb_Ntk_t * p, int i )                   { return Acb_ObjFanins(p, i)[1];                                                                       }
static inline int            Acb_ObjCioId( Acb_Ntk_t * p, int i )                    { return Acb_ObjFanins(p, i)[2];                                                                       }
 
static inline int            Acb_ObjCopy( Acb_Ntk_t * p, int i )                     { assert(i>0); assert( Acb_NtkHasObjCopies(p) ); return Vec_IntEntry(&p->vObjCopy, i);                 }
static inline int            Acb_ObjFunc( Acb_Ntk_t * p, int i )                     { assert(i>0); assert( Acb_NtkHasObjFuncs(p) );  return Vec_IntEntry(&p->vObjFunc, i);                 }
static inline int            Acb_ObjWeight( Acb_Ntk_t * p, int i )                   { assert(i>0); assert( Acb_NtkHasObjWeights(p) );return Vec_IntEntry(&p->vObjWeight, i);               }
static inline word           Acb_ObjTruth( Acb_Ntk_t * p, int i )                    { assert(i>0); assert( Acb_NtkHasObjTruths(p) ); return Vec_WrdEntry(&p->vObjTruth, i);                }
static inline word *         Acb_ObjTruthP( Acb_Ntk_t * p, int i )                   { assert(i>0); assert( Acb_NtkHasObjTruths(p) ); return Vec_WrdEntryP(&p->vObjTruth, i);               }
static inline int            Acb_ObjName( Acb_Ntk_t * p, int i )                     { assert(i>0); assert( Acb_NtkHasObjNames(p) );  return Vec_IntEntry(&p->vObjName, i);                 }
static inline char *         Acb_ObjNameStr( Acb_Ntk_t * p, int i )                  { assert(i>0); return Acb_NtkStr(p, Acb_ObjName(p, i));                                                }
static inline int            Acb_ObjAttr( Acb_Ntk_t * p, int i )                     { assert(i>=0); return Acb_NtkHasObjAttrs(p) ? Vec_IntEntry(&p->vObjAttr, i) : 0;                      }
static inline int            Acb_ObjAttrSize( Acb_Ntk_t * p, int i )                 { assert(i>=0); return Acb_ObjAttr(p, i) ? Vec_IntEntry(&p->vAttrSto, Acb_ObjAttr(p, i)) : 0;          }
static inline int *          Acb_ObjAttrArray( Acb_Ntk_t * p, int i )                { assert(i>=0); return Acb_ObjAttr(p, i) ? Vec_IntEntryP(&p->vAttrSto, Acb_ObjAttr(p, i)+1) : NULL;    }
static inline int            Acb_ObjAttrValue( Acb_Ntk_t * p, int i, int x )         { int k, s = Acb_ObjAttrSize(p, i), * a = Acb_ObjAttrArray(p, i); for ( k = 0; k < s; k += 2)  if (a[k] == x) return a[k+1]; return 0; }
static inline int            Acb_ObjLevelD( Acb_Ntk_t * p, int i )                   { assert(i>0); return Vec_IntEntry(&p->vLevelD, i);                                                    }
static inline int            Acb_ObjLevelR( Acb_Ntk_t * p, int i )                   { assert(i>0); return Vec_IntEntry(&p->vLevelR, i);                                                    }
static inline int            Acb_ObjPathD( Acb_Ntk_t * p, int i )                    { assert(i>0); return Vec_IntEntry(&p->vPathD, i);                                                     }
static inline int            Acb_ObjPathR( Acb_Ntk_t * p, int i )                    { assert(i>0); return Vec_IntEntry(&p->vPathR, i);                                                     }
static inline float          Acb_ObjCounts( Acb_Ntk_t * p, int i )                   { assert(i>0); return Vec_FltEntry(&p->vCounts, i);                                                    }
static inline Vec_Str_t *    Acb_ObjCnfs( Acb_Ntk_t * p, int i )                     { assert(i>0); return (Vec_Str_t *)Vec_WecEntry(&p->vCnfs, i);                                         }

static inline void           Acb_ObjSetCopy( Acb_Ntk_t * p, int i, int x )           { assert(Acb_ObjCopy(p, i) == -1); Vec_IntWriteEntry( &p->vObjCopy, i, x );                            }
static inline void           Acb_ObjSetFunc( Acb_Ntk_t * p, int i, int x )           { assert(Acb_ObjFunc(p, i) == -1); Vec_IntWriteEntry( &p->vObjFunc, i, x );                            }
static inline void           Acb_ObjSetWeight( Acb_Ntk_t * p, int i, int x )         { assert(Acb_ObjWeight(p, i)== 0); Vec_IntWriteEntry( &p->vObjWeight, i, x );                          }
static inline void           Acb_ObjSetTruth( Acb_Ntk_t * p, int i, word x )         { assert(Acb_ObjTruth(p, i) == 0); Vec_WrdWriteEntry( &p->vObjTruth, i, x );                           }
static inline void           Acb_ObjSetName( Acb_Ntk_t * p, int i, int x )           { assert(Acb_ObjName(p, i) ==  0); Vec_IntWriteEntry( &p->vObjName, i, x );                            }
static inline void           Acb_ObjSetAttrs( Acb_Ntk_t * p, int i, int * a, int s ) { assert(Acb_ObjAttr(p, i) == 0); if ( !a ) return; Vec_IntWriteEntry(&p->vObjAttr, i, Vec_IntSize(&p->vAttrSto)); Vec_IntPush(&p->vAttrSto, s); Vec_IntPushArray(&p->vAttrSto, a, s);  }
static inline int            Acb_ObjSetLevelD( Acb_Ntk_t * p, int i, int x )         { Vec_IntWriteEntry( &p->vLevelD, i, x );  return x;                                                   }
static inline int            Acb_ObjSetLevelR( Acb_Ntk_t * p, int i, int x )         { Vec_IntWriteEntry( &p->vLevelR, i, x );  return x;                                                   }
static inline int            Acb_ObjSetPathD( Acb_Ntk_t * p, int i, int x )          { Vec_IntWriteEntry( &p->vPathD, i, x );   return x;                                                   }
static inline int            Acb_ObjSetPathR( Acb_Ntk_t * p, int i, int x )          { Vec_IntWriteEntry( &p->vPathR, i, x );   return x;                                                   }
static inline float          Acb_ObjSetCounts( Acb_Ntk_t * p, int i, float x )       { Vec_FltWriteEntry( &p->vCounts, i, x );   return x;                                                  }
static inline int            Acb_ObjUpdateLevelD( Acb_Ntk_t * p, int i, int x )      { Vec_IntUpdateEntry( &p->vLevelD, i, x );  return x;                                                  }
static inline int            Acb_ObjUpdateLevelR( Acb_Ntk_t * p, int i, int x )      { Vec_IntUpdateEntry( &p->vLevelR, i, x );  return x;                                                  }
static inline int            Acb_ObjAddToPathD( Acb_Ntk_t * p, int i, int x )        { Vec_IntAddToEntry( &p->vPathD, i, x );   return x;                                                   }
static inline int            Acb_ObjAddToPathR( Acb_Ntk_t * p, int i, int x )        { Vec_IntAddToEntry( &p->vPathR, i, x );   return x;                                                   }

static inline int            Acb_ObjNtkId( Acb_Ntk_t * p, int i )                    { assert(i>0); return Acb_ObjIsBox(p, i) ? Acb_ObjFanin(p, i, Acb_ObjFaninNum(p, i)) : 0;              }
static inline Acb_Ntk_t *    Acb_ObjNtk( Acb_Ntk_t * p, int i )                      { assert(i>0); return Acb_NtkNtk(p, Acb_ObjNtkId(p, i));                                               }
static inline int            Acb_ObjIsSeq( Acb_Ntk_t * p, int i )                    { assert(i>0); return Acb_ObjIsBox(p, i) ? Acb_ObjNtk(p, i)->fSeq : Acb_TypeIsSeq(Acb_ObjType(p, i));  }

static inline int            Acb_ObjRangeId( Acb_Ntk_t * p, int i )                  { return Acb_NtkHasObjRanges(p) ? Vec_IntEntry(&p->vObjRange, i) : 0;                                  }
static inline int            Acb_ObjRange( Acb_Ntk_t * p, int i )                    { return Abc_Lit2Var( Acb_ObjRangeId(p, i) );                                                          }
static inline int            Acb_ObjLeft( Acb_Ntk_t * p, int i )                     { return Acb_NtkRangeLeft(p, Acb_ObjRange(p, i));                                                      }
static inline int            Acb_ObjRight( Acb_Ntk_t * p, int i )                    { return Acb_NtkRangeRight(p, Acb_ObjRange(p, i));                                                     }
static inline int            Acb_ObjSigned( Acb_Ntk_t * p, int i )                   { return Abc_LitIsCompl(Acb_ObjRangeId(p, i));                                                         }
static inline int            Acb_ObjRangeSize( Acb_Ntk_t * p, int i )                { return Acb_NtkRangeSize(p, Acb_ObjRange(p, i));                                                      }
static inline void           Acb_ObjSetRangeSign( Acb_Ntk_t * p, int i, int x )      { assert(Acb_NtkHasObjRanges(p));  Vec_IntWriteEntry(&p->vObjRange, i, x);                             }
static inline void           Acb_ObjSetRange( Acb_Ntk_t * p, int i, int x )          { assert(Acb_NtkHasObjRanges(p));  Vec_IntWriteEntry(&p->vObjRange, i, Abc_Var2Lit(x,0));              }
static inline void           Acb_ObjHashRange( Acb_Ntk_t * p, int i, int l, int r )  { Acb_ObjSetRange( p, i, Acb_NtkHashRange(p, l, r) );                                                  }
static inline int            Acb_ObjRangeSign( Acb_Ntk_t * p, int i )                { return Abc_Var2Lit( Acb_ObjRangeSize(p, i), Acb_ObjSigned(p, i) );                                   }

static inline int            Acb_ObjTravId( Acb_Ntk_t * p, int i )                   { return Vec_IntEntry(&p->vObjTrav, i);                                                                }
static inline int            Acb_ObjTravIdDiff( Acb_Ntk_t * p, int i )               { return p->nObjTravs - Vec_IntEntry(&p->vObjTrav, i);                                                 }
static inline int            Acb_ObjIsTravIdCur( Acb_Ntk_t * p, int i )              { return Acb_ObjTravId(p, i) == p->nObjTravs;                                                          }
static inline int            Acb_ObjIsTravIdPrev( Acb_Ntk_t * p, int i )             { return Acb_ObjTravId(p, i) == p->nObjTravs-1;                                                        }
static inline int            Acb_ObjIsTravIdDiff( Acb_Ntk_t * p, int i, int d )      { return Acb_ObjTravId(p, i) == p->nObjTravs-d;                                                        }
static inline int            Acb_ObjSetTravIdCur( Acb_Ntk_t * p, int i )             { int r = Acb_ObjIsTravIdCur(p, i);  Vec_IntWriteEntry(&p->vObjTrav, i, p->nObjTravs);   return r;     }
static inline int            Acb_ObjSetTravIdPrev( Acb_Ntk_t * p, int i )            { int r = Acb_ObjIsTravIdPrev(p, i); Vec_IntWriteEntry(&p->vObjTrav, i, p->nObjTravs-1); return r;     }
static inline int            Acb_ObjSetTravIdDiff( Acb_Ntk_t * p, int i, int d )     { int r = Acb_ObjTravIdDiff(p, i);   Vec_IntWriteEntry(&p->vObjTrav, i, p->nObjTravs-d); return r;     }
static inline int            Acb_NtkTravId( Acb_Ntk_t * p )                          { return p->nObjTravs;                                                                                 }
static inline void           Acb_NtkIncTravId( Acb_Ntk_t * p )                       { if ( !Acb_NtkHasObjTravs(p) ) Acb_NtkCleanObjTravs(p); p->nObjTravs++;                               }

////////////////////////////////////////////////////////////////////////
///                          ITERATORS                               ///
////////////////////////////////////////////////////////////////////////

#define Acb_ManForEachNtk( p, pNtk, i )                                   \
    for ( i = 1; (i <= Acb_ManNtkNum(p)) && (((pNtk) = Acb_ManNtk(p, i)), 1); i++ ) 

#define Acb_NtkForEachPi( p, iObj, i )                                    \
    for ( i = 0; (i < Acb_NtkPiNum(p))  && (((iObj) = Acb_NtkCi(p, i)), 1); i++ ) 
#define Acb_NtkForEachPo( p, iObj, i )                                    \
    for ( i = 0; (i < Acb_NtkPoNum(p))  && (((iObj) = Acb_NtkCo(p, i)), 1); i++ ) 

#define Acb_NtkForEachCi( p, iObj, i )                                    \
    for ( i = 0; (i < Acb_NtkCiNum(p))  && (((iObj) = Acb_NtkCi(p, i)), 1); i++ ) 
#define Acb_NtkForEachCo( p, iObj, i )                                    \
    for ( i = 0; (i < Acb_NtkCoNum(p))  && (((iObj) = Acb_NtkCo(p, i)), 1); i++ ) 
#define Acb_NtkForEachCoDriver( p, iObj, i )                              \
    for ( i = 0; (i < Acb_NtkCoNum(p))  && (((iObj) = Acb_ObjFanin(p, Acb_NtkCo(p, i), 0)), 1); i++ ) 
#define Acb_NtkForEachCoAndDriver( p, iObj, iDriver, i )                  \
    for ( i = 0; (i < Acb_NtkCoNum(p))  && (((iObj) = Acb_NtkCo(p, i)), 1)  && (((iDriver) = Acb_ObjFanin(p, iObj, 0)), 1); i++ ) 

#define Acb_NtkForEachCiVec( vVec, p, iObj, i )                           \
    for ( i = 0; (i < Vec_IntSize(vVec))  && (((iObj) = Acb_NtkCi(p, Vec_IntEntry(vVec,i))), 1); i++ ) 
#define Acb_NtkForEachCoVec( vVec, p, iObj, i )                           \
    for ( i = 0; (i < Vec_IntSize(vVec))  && (((iObj) = Acb_NtkCo(p, Vec_IntEntry(vVec,i))), 1); i++ ) 
#define Acb_NtkForEachCoDriverVec( vVec, p, iObj, i )                     \
    for ( i = 0; (i < Vec_IntSize(vVec))  && (((iObj) = Acb_ObjFanin(p, Acb_NtkCo(p, Vec_IntEntry(vVec,i)), 0)), 1); i++ ) 

#define Acb_NtkForEachBoxSeq( p, iObj, i )                                \
    for ( i = 0; (i < Acb_NtkSeqNum(p))  && (((iObj) = Acb_NtkBoxSeq(p, i)), 1); i++ ) 
#define Acb_NtkForEachCioOrder( p, iObj, i )                              \
    for ( i = 0; (i < Acb_NtkCioOrderNum(p))  && (((iObj) = Acb_NtkCioOrder(p, i)), 1); i++ ) 


#define Acb_NtkForEachObj( p, i )                                         \
    for ( i = 1; i < Vec_StrSize(&p->vObjType); i++ ) if ( !Acb_ObjType(p, i) ) {} else   
#define Acb_NtkForEachObjReverse( p, i )                                  \
    for ( i = Vec_StrSize(&p->vObjType)-1; i > 0; i-- ) if ( !Acb_ObjType(p, i) ) {} else   
#define Acb_NtkForEachNode( p, i )                                        \
    for ( i = 1; i < Vec_StrSize(&p->vObjType); i++ ) if ( !Acb_ObjType(p, i) || Acb_ObjIsCio(p, i) ) {} else   
#define Acb_NtkForEachNodeSupp( p, i, nSuppSize )                         \
    for ( i = 1; i < Vec_StrSize(&p->vObjType); i++ ) if ( !Acb_ObjType(p, i) || Acb_ObjIsCio(p, i) || Acb_ObjFaninNum(p, i) != nSuppSize ) {} else   
#define Acb_NtkForEachNodeReverse( p, i )                                 \
    for ( i = Vec_StrSize(&p->vObjType)-1; i > 0; i-- ) if ( !Acb_ObjType(p, i) || Acb_ObjIsCio(p, i) ) {} else   
#define Acb_NtkForEachObjType( p, Type, i )                               \
    for ( i = 1; i < Vec_StrSize(&p->vObjType) && (((Type) = Acb_ObjType(p, i)), 1); i++ ) if ( !Type ) {} else
#define Acb_NtkForEachBox( p, i )                                         \
    for ( i = 1; i < Vec_StrSize(&p->vObjType); i++ ) if ( !Acb_ObjIsBox(p, i) ) {} else

#define Acb_ObjForEachFanin( p, iObj, iFanin, k )                         \
    for ( k = 0; k < Acb_ObjFaninNum(p, iObj) && ((iFanin = Acb_ObjFanin(p, iObj, k)), 1); k++ )
#define Acb_ObjForEachFaninFast( p, iObj, pFanins, iFanin, k )            \
    for ( k = 0, pFanins = Acb_ObjFanins(p, iObj); k < pFanins[0] && ((iFanin = pFanins[k+1]), 1); k++ )

#define Acb_ObjForEachFanout( p, iObj, iFanout, k )                       \
    Vec_IntForEachEntry( Vec_WecEntry(&p->vFanouts, iObj), iFanout, k ) if ( !Acb_ObjType(p, iFanout) ) {} else 

#define Acb_ObjForEachFon( p, iObj, iFon )                                \
    for ( assert(Acb_ObjIsBox(p, iObj)), iFon = iObj + 1; iFon < Acb_NtkObjNum(p) && Acb_ObjIsFon(p, iFon); iFon++ ) 

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Object APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Acb_ObjFonNum( Acb_Ntk_t * p, int iObj )
{
    int i, Count = 0;
    if ( !Acb_ObjIsBox(p, iObj) ) 
        return 0;
    Acb_ObjForEachFon( p, iObj, i )
        Count++;
    return Count;
}
static inline int Acb_ObjWhatFanin( Acb_Ntk_t * p, int iObj, int iFaninGiven )
{
    int k, iFanin, * pFanins;
    Acb_ObjForEachFaninFast( p, iObj, pFanins, iFanin, k )
        if ( iFanin == iFaninGiven )
            return k;
    return -1;
}
static inline void Acb_ObjAddFanin( Acb_Ntk_t * p, int iObj, int iFanin )
{
    int * pFanins = Acb_ObjFanins( p, iObj );
    assert( iFanin > 0 );
    assert( pFanins[ 1 + pFanins[0] ] == -1 );
    pFanins[ 1 + pFanins[0]++ ] = iFanin;
}
static inline void Acb_ObjDeleteFaninIndex( Acb_Ntk_t * p, int iObj, int iFaninIndex )
{
    int i, * pFanins = Acb_ObjFanins( p, iObj );
    pFanins[0]--;
    for ( i = iFaninIndex; i < pFanins[0]; i++ )
        pFanins[ 1 + i ] = pFanins[ 2 + i ];
    pFanins[ 1 + pFanins[0] ] = -1;
}
static inline void Acb_ObjDeleteFanin( Acb_Ntk_t * p, int iObj, int iFanin )
{
    int * pFanins = Acb_ObjFanins( p, iObj );
    int iFaninIndex = Acb_ObjWhatFanin( p, iObj, iFanin );
    assert( pFanins[ 1 + iFaninIndex ] == iFanin );
    Acb_ObjDeleteFaninIndex( p, iObj, iFaninIndex );
}
static inline void Acb_ObjAddFanins( Acb_Ntk_t * p, int iObj, Vec_Int_t * vFanins )
{
    int i, iFanin;
    if ( vFanins )
        Vec_IntForEachEntry( vFanins, iFanin, i )
            Acb_ObjAddFanin( p, iObj, iFanin );
}
static inline void Acb_ObjSetNtkId( Acb_Ntk_t * p, int iObj, int iNtk ) // should be called after fanins are added
{
    int * pFanins = Acb_ObjFanins( p, iObj );
    assert( pFanins[ 1 + pFanins[0] ] == -1 );
    pFanins[ 1 + pFanins[0] ] = iNtk;
}
static inline void Acb_ObjRemoveFanins( Acb_Ntk_t * p, int iObj )
{
    int i, * pFanins = Acb_ObjFanins( p, iObj );
    for ( i = 1; i <= pFanins[0]; i++ )
    {
        assert( pFanins[i] > 0 );
        pFanins[i] = -1;
    }
    pFanins[0] = 0;
}
static inline int Acb_ObjAlloc( Acb_Ntk_t * p, Acb_ObjType_t Type, int nFans, int nFons )
{
    int i, nFansReal, CioId = -1, iObj = Vec_StrSize(&p->vObjType);
    Vec_StrPush( &p->vObjType, (char)Type );
    if ( Type == ABC_OPER_CI )
    {
        assert( nFans == 0 );
        CioId = Vec_IntSize( &p->vCis );
        Vec_IntPush( &p->vCis, iObj );
        nFansReal = 2;
    }
    else if ( Type == ABC_OPER_CO )
    {
        assert( nFans == 1 );
        CioId = Vec_IntSize( &p->vCos );
        Vec_IntPush( &p->vCos, iObj );
        nFansReal = 2;
    }
    else 
        nFansReal = Abc_MaxInt( nFans + Acb_ObjIsBox(p, iObj), p->nFaninMax );
    // add fins
    Vec_IntPush( &p->vObjFans, Vec_IntSize(&p->vFanSto) );
    Vec_IntPush( &p->vFanSto, 0 );
    for ( i = 0; i < nFansReal; i++ )
        Vec_IntPush( &p->vFanSto, (CioId >= 0 && i == 1) ? CioId : -1 );
    // add fons
    assert( !Acb_ObjIsBox(p, iObj) || nFons > 0 );
    for ( i = 0; i < nFons; i++ )
        Acb_ObjAddFanin( p, Acb_ObjAlloc(p, ABC_OPER_FON, 1, 0), iObj );
    // extend storage
    if ( Acb_NtkHasObjCopies(p) ) Vec_IntPush(&p->vObjCopy  , -1);
    if ( Acb_NtkHasObjFuncs(p)  ) Vec_IntPush(&p->vObjFunc  , -1);
    if ( Acb_NtkHasObjWeights(p)) Vec_IntPush(&p->vObjWeight,  0);
    if ( Acb_NtkHasObjTruths(p) ) Vec_WrdPush(&p->vObjTruth ,  0);
    if ( Acb_NtkHasObjNames(p)  ) Vec_IntPush(&p->vObjName  ,  0);
    if ( Acb_NtkHasObjRanges(p) ) Vec_IntPush(&p->vObjRange ,  0);
    if ( Acb_NtkHasObjTravs(p)  ) Vec_IntPush(&p->vObjTrav  ,  0);
    if ( Acb_NtkHasObjAttrs(p)  ) Vec_IntPush(&p->vObjAttr  ,  0);
    if ( Acb_NtkHasObjLevelD(p) ) Vec_IntPush(&p->vLevelD   ,  0);
    if ( Acb_NtkHasObjLevelR(p) ) Vec_IntPush(&p->vLevelR   ,  0);
    if ( Acb_NtkHasObjPathD(p)  ) Vec_IntPush(&p->vPathD    ,  0);
    if ( Acb_NtkHasObjPathR(p)  ) Vec_IntPush(&p->vPathR    ,  0);
    if ( Acb_NtkHasObjCounts(p) ) Vec_FltPush(&p->vCounts   ,  0);
    if ( Acb_NtkHasObjFanout(p) ) Vec_WecPushLevel(&p->vFanouts);
    if ( Acb_NtkHasObjCnfs(p)   ) Vec_WecPushLevel(&p->vCnfs);
    if ( p->vQue ) Vec_QueSetPriority( p->vQue, Vec_FltArrayP(&p->vCounts) );
    return iObj;
}
static inline int Acb_ObjDup( Acb_Ntk_t * pNew, Acb_Ntk_t * p, int i )
{
    int iObj = Acb_ObjAlloc( pNew, Acb_ObjType(p, i), Acb_ObjFaninNum(p, i), Acb_ObjFonNum(p, i) );
    Acb_ObjSetCopy( p, i, iObj );
    return iObj;
}
static inline void Acb_ObjDelete( Acb_Ntk_t * p, int iObj )
{
    int i;
    Acb_ObjCleanType( p, iObj );
    if ( !Acb_ObjIsBox(p, iObj) ) 
        return;
    Acb_ObjForEachFon( p, iObj, i )
        Acb_ObjCleanType( p, i );
}
static inline void Acb_ObjAddFaninFanoutOne( Acb_Ntk_t * p, int iObj, int iFanin )
{
    Vec_IntPush( Vec_WecEntry(&p->vFanouts, iFanin), iObj );
    Acb_ObjAddFanin( p, iObj, iFanin );
}
static inline void Acb_ObjAddFaninFanout( Acb_Ntk_t * p, int iObj )
{
    int k, iFanin, * pFanins; 
    Acb_ObjForEachFaninFast( p, iObj, pFanins, iFanin, k )
        Vec_IntPush( Vec_WecEntry(&p->vFanouts, iFanin), iObj );
}
static inline void Acb_ObjRemoveFaninFanoutOne( Acb_Ntk_t * p, int iObj, int iFanin )
{
    int RetValue = Vec_IntRemove( Vec_WecEntry(&p->vFanouts, iFanin), iObj );
    assert( RetValue );
    Acb_ObjDeleteFanin( p, iObj, iFanin );
}
static inline void Acb_ObjRemoveFaninFanout( Acb_Ntk_t * p, int iObj )
{
    int k, iFanin, * pFanins; 
    Acb_ObjForEachFaninFast( p, iObj, pFanins, iFanin, k )
    {
        int RetValue = Vec_IntRemove( Vec_WecEntry(&p->vFanouts, iFanin), iObj );
        assert( RetValue );
    }
}
static inline void Acb_ObjPatchFanin( Acb_Ntk_t * p, int iObj, int iFanin, int iFaninNew )
{
    int i, RetValue, * pFanins = Acb_ObjFanins( p, iObj );
    assert( iFanin != iFaninNew );
    for ( i = 0; i < pFanins[0]; i++ )
        if ( pFanins[ 1 + i ] == iFanin )
            pFanins[ 1 + i ] = iFaninNew;
    if ( !Acb_NtkHasObjFanout(p) )
        return;
    RetValue = Vec_IntRemove( Vec_WecEntry(&p->vFanouts, iFanin), iObj );
    assert( RetValue );
    Vec_IntPush( Vec_WecEntry(&p->vFanouts, iFaninNew), iObj );
}
static inline void Acb_NtkCreateFanout( Acb_Ntk_t * p )
{
    int iObj; 
    Acb_NtkCleanObjFanout( p );
    Acb_NtkForEachObj( p, iObj )
        Acb_ObjAddFaninFanout( p, iObj );
}

/**Function*************************************************************

  Synopsis    [Network APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Acb_Ntk_t *  Acb_NtkAlloc( Acb_Man_t * p, int NameId, int nCis, int nCos, int nObjs )
{    
    Acb_Ntk_t * pNew = ABC_CALLOC( Acb_Ntk_t, 1 );
    assert( nCis >= 0 && nCos >= 0 && nObjs >= 0 );
    pNew->Id      = Vec_PtrSize(&p->vNtks);   Vec_PtrPush( &p->vNtks, pNew );
    pNew->NameId  = NameId;
    pNew->pDesign = p;
    Vec_IntGrow( &pNew->vCis, nCis );
    Vec_IntGrow( &pNew->vCos, nCos );
    Vec_StrGrow( &pNew->vObjType, nObjs+1 );  Vec_StrPush( &pNew->vObjType,  (char)ABC_OPER_NONE );
    Vec_IntGrow( &pNew->vObjFans, nObjs+1 );  Vec_IntPush( &pNew->vObjFans, -1 );
    Vec_IntGrow( &pNew->vFanSto,  3*nObjs );  
    return pNew;
}
static inline void Acb_NtkFree( Acb_Ntk_t * p )
{
    // interface
    Vec_IntErase( &p->vCis );
    Vec_IntErase( &p->vCos );
    Vec_IntErase( &p->vOrder );
    Vec_IntErase( &p->vSeq );
    // stucture
    Vec_StrErase( &p->vObjType );
    Vec_IntErase( &p->vObjFans );    
    Vec_IntErase( &p->vFanSto );
    // optional
    Vec_IntErase( &p->vObjCopy );    
    Vec_IntErase( &p->vObjFunc );    
    Vec_IntErase( &p->vObjWeight );    
    Vec_WrdErase( &p->vObjTruth );    
    Vec_IntErase( &p->vObjName );    
    Vec_IntErase( &p->vObjRange );    
    Vec_IntErase( &p->vObjTrav );    
    Vec_IntErase( &p->vObjBits );    
    Vec_IntErase( &p->vObjAttr );    
    Vec_IntErase( &p->vAttrSto );    
    Vec_IntErase( &p->vNtkObjs );    
    Vec_IntErase( &p->vTargets );    
    Vec_IntErase( &p->vLevelD );    
    Vec_IntErase( &p->vLevelR );    
    Vec_IntErase( &p->vPathD );    
    Vec_IntErase( &p->vPathR );    
    Vec_FltErase( &p->vCounts );    
    Vec_WecErase( &p->vFanouts );
    Vec_WecErase( &p->vCnfs );    
    Vec_StrErase( &p->vCnf );    
    Vec_IntErase( &p->vSuppOld );    
    // other
    Vec_QueFreeP( &p->vQue );
    Vec_IntErase( &p->vCover );    
    Vec_IntErase( &p->vArray0 );    
    Vec_IntErase( &p->vArray1 );    
    Vec_IntErase( &p->vArray2 );    
    ABC_FREE( p );
}
static inline int Acb_NtkNewStrId( Acb_Ntk_t * pNtk, const char * format, ...  )
{
    Abc_Nam_t * p = Acb_NtkNam( pNtk );
    Vec_Str_t * vBuf = Abc_NamBuffer( p );
    int nAdded, nSize = 1000; 
    va_list args;   va_start( args, format );
    Vec_StrGrow( vBuf, Vec_StrSize(vBuf) + nSize );
    nAdded = vsnprintf( Vec_StrLimit(vBuf), nSize, format, args );
    if ( nAdded > nSize )
    {
        Vec_StrGrow( vBuf, Vec_StrSize(vBuf) + nAdded + nSize );
        nSize = vsnprintf( Vec_StrLimit(vBuf), nAdded, format, args );
        assert( nSize == nAdded );
    }
    va_end( args );
    return Abc_NamStrFindOrAddLim( p, Vec_StrLimit(vBuf), Vec_StrLimit(vBuf) + nAdded, NULL );
}
static inline int Acb_ManNewConstId( Acb_Ntk_t * p, Vec_Str_t * vBits ) 
{ 
    Vec_Str_t * vOut = Abc_NamBuffer(Acb_NtkNam(p)); 
    Vec_StrPrintF( vOut, "%d\'b%s", Vec_StrSize(vBits)-1, Vec_StrArray(vBits) );
    return Abc_NamStrFindOrAdd(p->pDesign->pFuns, Vec_StrArray(vOut), NULL);
}
static inline int Acb_ManNewConstZero( Acb_Ntk_t * p, int nBits ) 
{ 
    Vec_Str_t * vOut = Abc_NamBuffer(Acb_NtkNam(p)); 
    Vec_StrPrintF( vOut, "%d\'b%0s", nBits, "" );
    return Abc_NamStrFindOrAdd(p->pDesign->pFuns, Vec_StrArray(vOut), NULL);
}
static inline void Acb_NtkAdd( Acb_Man_t * p, Acb_Ntk_t * pNtk )
{    
    int fFound, NtkId = Abc_NamStrFindOrAdd( p->pMods, Acb_NtkName(pNtk), &fFound );
    if ( fFound )
        printf( "Network with name \"%s\" already exists.\n", Acb_NtkName(pNtk) );
    else
        assert( NtkId == pNtk->Id );
}
static inline void Acb_NtkUpdate( Acb_Man_t * p, Acb_Ntk_t * pNtk )
{
    int fFound, NtkId = Abc_NamStrFindOrAdd( p->pMods, Acb_NtkName(pNtk), &fFound );
    if ( !fFound )
        printf( "Network with name \"%s\" does not exist.\n", Acb_NtkName(pNtk) );
    else
    {
        Acb_NtkFree( Acb_ManNtk(p, NtkId) );
        Vec_PtrWriteEntry( &p->vNtks, NtkId, pNtk );
    }
}
static inline Vec_Int_t * Acb_NtkCollect( Acb_Ntk_t * p )
{
    int iObj;
    Vec_Int_t * vObjs = Vec_IntAlloc( Acb_NtkObjNum(p) );
    Acb_NtkForEachObj( p, iObj )
        Vec_IntPush( vObjs, iObj );
    return vObjs;
}
static inline Acb_Ntk_t * Acb_NtkDup( Acb_Man_t * pMan, Acb_Ntk_t * p, Vec_Int_t * vObjs )
{
    Acb_Ntk_t * pNew;
    int i, k, iObj, iObjNew, iFanin;
    pNew = Acb_NtkAlloc( pMan, Acb_NtkNameId(p), Acb_NtkCiNum(p), Acb_NtkCoNum(p), Vec_IntSize(vObjs) );
    Acb_NtkCleanObjCopies( p );
    Vec_IntForEachEntry( vObjs, iObj, i )
        iObjNew = Acb_ObjDup( pNew, p, iObj );
    Vec_IntForEachEntry( vObjs, iObj, i )
    {
        iObjNew = Acb_ObjCopy( p, iObj );
        Acb_ObjForEachFanin( p, iObj, iFanin, k )
            Acb_ObjAddFanin( pNew, iObjNew, Acb_ObjCopy(p, iFanin) );
    }
    //Acb_NtkFreeObjCopies( p );
    assert( Acb_NtkObjNum(pNew) == Acb_NtkObjNumAlloc(pNew) );
    Acb_NtkSetCopy( p, Acb_NtkId(pNew) );
    Acb_NtkSetRegNum( pNew, Acb_NtkRegNum(p) );
    return pNew;
}
static inline Acb_Ntk_t * Acb_NtkDupOrder( Acb_Man_t * pMan, Acb_Ntk_t * p, Vec_Int_t*(* pFuncOrder)(Acb_Ntk_t*) )
{
    Acb_Ntk_t * pNew;
    Vec_Int_t * vObjs = pFuncOrder ? pFuncOrder(p) : Acb_NtkCollect(p);
    if ( vObjs == NULL )
        return NULL;
    pNew = Acb_NtkDup( pMan, p, vObjs );
    Vec_IntFree( vObjs );
    //Acb_NtkPrepareSeq( pNew );
    return pNew;
}
static inline void Acb_NtkDupAttrs( Acb_Ntk_t * pNew, Acb_Ntk_t * p )
{
    int i, iObj;
    assert( Vec_IntSize(&pNew->vOrder) == 0 );
    Acb_NtkForEachCioOrder( p, iObj, i )
        Vec_IntPush( &pNew->vOrder, Acb_ObjCopy(p, iObj) );
//    Vec_IntRemapArray( &p->vObjCopy, &p->vOrder,    &pNew->vOrder,    Acb_NtkCioOrderNum(p) );
//    Vec_IntRemapArray( &p->vObjCopy, &p->vSeq,      &pNew->vSeq,      Acb_NtkSeqNum(p) );
    // transfer object attributes
    Vec_IntRemapArray( &p->vObjCopy, &p->vObjFunc,   &pNew->vObjFunc,   Acb_NtkObjNum(pNew) + 1 );
    Vec_IntRemapArray( &p->vObjCopy, &p->vObjWeight, &pNew->vObjWeight, Acb_NtkObjNum(pNew) + 1 );
//    Vec_WrdRemapArray( &p->vObjCopy, &p->vObjTruth,  &pNew->vObjTruth,  Acb_NtkObjNum(pNew) + 1 );
    Vec_IntRemapArray( &p->vObjCopy, &p->vObjName,   &pNew->vObjName,   Acb_NtkObjNum(pNew) + 1 );
    Vec_IntRemapArray( &p->vObjCopy, &p->vObjRange,  &pNew->vObjRange,  Acb_NtkObjNum(pNew) + 1 );
    Vec_IntRemapArray( &p->vObjCopy, &p->vObjAttr,   &pNew->vObjAttr,   Acb_NtkObjNum(pNew) + 1 );
    // duplicate attributes
    Vec_IntAppend( &pNew->vAttrSto, &p->vAttrSto );
}
static inline int Acb_NtkMemory( Acb_Ntk_t * p )
{
    int nMem = sizeof(Acb_Ntk_t);
    // interface
    nMem += (int)Vec_IntMemory(&p->vCis);
    nMem += (int)Vec_IntMemory(&p->vCos);
    nMem += (int)Vec_IntMemory(&p->vOrder);
    nMem += (int)Vec_IntMemory(&p->vSeq);
    // stucture
    nMem += (int)Vec_StrMemory(&p->vObjType);
    nMem += (int)Vec_IntMemory(&p->vObjFans);
    nMem += (int)Vec_IntMemory(&p->vFanSto);
    nMem += (int)Vec_WecMemory(&p->vFanouts);
    // optional
    nMem += (int)Vec_IntMemory(&p->vObjCopy );  
    nMem += (int)Vec_IntMemory(&p->vObjFunc );  
    nMem += (int)Vec_IntMemory(&p->vObjWeight );  
    nMem += (int)Vec_WrdMemory(&p->vObjTruth );  
    nMem += (int)Vec_IntMemory(&p->vObjName );  
    nMem += (int)Vec_IntMemory(&p->vObjRange );  
    nMem += (int)Vec_IntMemory(&p->vObjTrav );  
    nMem += (int)Vec_IntMemory(&p->vObjBits );  
    nMem += (int)Vec_IntMemory(&p->vObjAttr );  
    nMem += (int)Vec_IntMemory(&p->vAttrSto );  
    nMem += (int)Vec_IntMemory(&p->vNtkObjs );  
    nMem += (int)Vec_IntMemory(&p->vTargets );  
    // other
    nMem += (int)Vec_IntMemory(&p->vCover );
    nMem += (int)Vec_IntMemory(&p->vArray0 );
    nMem += (int)Vec_IntMemory(&p->vArray1 );
    nMem += (int)Vec_IntMemory(&p->vArray2 );
    return nMem;
}
static inline void Acb_NtkPrintStats( Acb_Ntk_t * p )
{
    printf( "pi =%5d  ",   Acb_NtkPiNum(p) );
    printf( "po =%5d  ",   Acb_NtkPoNum(p) );
    printf( "ff =%5d  ",   Acb_NtkRegNum(p) );
    printf( "node =%5d  ", Acb_NtkNodeNum(p) );
    printf( "box =%5d  ",  Acb_NtkBoxNum(p) );
    //printf( "topo =%4s  ", Acb_NtkIsTopoOrder(p) ? "yes" : "no" );
    printf( "  %s ",       Acb_NtkName(p) );
    if ( Vec_IntSize(&p->vNtkObjs) )
        printf( "-> %s",   Acb_NtkName(Acb_NtkNtk(p, Vec_IntEntry(&p->vNtkObjs, 0))) );
    printf( "\n" );
//    Vec_StrIntPrint( &p->vObjType );
}
static inline void Acb_NtkPrint( Acb_Ntk_t * p )
{
    int i, Type;
    printf( "Interface (%d):\n", Acb_NtkCioNum(p) );
    printf( "Objects (%d):\n", Acb_NtkObjNum(p) );
    Acb_NtkForEachObjType( p, Type, i )
    {
        printf( "%6d : ", i );
        printf( "Type =%3d  ", Type );
        printf( "Fanins = %d  ", Acb_ObjFaninNum(p, i) );
        if ( Acb_NtkHasObjNames(p) && Acb_ObjName(p, i) )
            printf( "%s", Acb_ObjNameStr(p, i) );
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Manager APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Acb_Man_t * Acb_ManAlloc( char * pFileName, int nNtks, Abc_Nam_t * pStrs, Abc_Nam_t * pFuns, Abc_Nam_t * pMods, Hash_IntMan_t * vHash )
{
    Acb_Man_t * pNew = ABC_CALLOC( Acb_Man_t, 1 );
    pNew->pName = Extra_FileDesignName( pFileName );
    pNew->pSpec = Abc_UtilStrsav( pFileName );
    pNew->pStrs = pStrs ? pStrs : Abc_NamStart( 1000, 24 );
    pNew->pFuns = pFuns ? pFuns : Abc_NamStart( 100, 24 );
    pNew->pMods = pMods ? pMods : Abc_NamStart( 100, 24 );
    pNew->vHash = vHash ? vHash : Hash_IntManStart( 1000 );
    if ( pFuns == NULL )
    {
        Abc_NamStrFindOrAdd(pNew->pFuns, "1\'b0", NULL);
        Abc_NamStrFindOrAdd(pNew->pFuns, "1\'b1", NULL);
        Abc_NamStrFindOrAdd(pNew->pFuns, "1\'bx", NULL);
        Abc_NamStrFindOrAdd(pNew->pFuns, "1\'bz", NULL);
    }
//    if ( vHash == NULL )
//        Hash_Int2ManInsert( pNew->vHash, 0, 0, 0 );
    Vec_PtrGrow( &pNew->vNtks,  nNtks+1 ); Vec_PtrPush( &pNew->vNtks, NULL );
    // set default root module
    pNew->iRoot = 1;
    return pNew;
}
static inline void Acb_ManDupTypeNames( Acb_Man_t * pNew, Acb_Man_t * p )
{
   memcpy( pNew->pTypeNames, p->pTypeNames, sizeof(void *) * ABC_OPER_LAST );
}
static inline Acb_Man_t * Acb_ManDup( Acb_Man_t * p, Vec_Int_t*(* pFuncOrder)(Acb_Ntk_t*) )
{
    Acb_Ntk_t * pNtk, * pNtkNew; int i;
    Acb_Man_t * pNew = Acb_ManAlloc( p->pSpec, Acb_ManNtkNum(p), Abc_NamRef(p->pStrs), Abc_NamRef(p->pFuns), Abc_NamStart(100, 24), Hash_IntManRef(p->vHash) );
    Acb_ManDupTypeNames( pNew, p );
    Acb_ManForEachNtk( p, pNtk, i )
    {
        pNtkNew = Acb_NtkDupOrder( pNew, pNtk, pFuncOrder );
        Acb_NtkAdd( pNew, pNtkNew );
        Acb_NtkDupAttrs( pNtkNew, pNtk );
    }
//    Acb_ManForEachNtk( p, pNtk, i )
//        if ( (pHost = Acb_NtkHostNtk(pNtk)) )
//            Acb_NtkSetHost( Acb_NtkCopyNtk(pNew, pNtk), Acb_NtkCopy(pHost), Acb_ObjCopy(pHost, Acb_NtkHostObj(pNtk)) );
    pNew->iRoot = Acb_ManNtkNum(pNew);
    return pNew;
}
static inline void Acb_ManFree( Acb_Man_t * p )
{
    Acb_Ntk_t * pNtk; int i;
    Acb_ManForEachNtk( p, pNtk, i )
        Acb_NtkFree( pNtk );
    ABC_FREE( p->vNtks.pArray );
    Abc_NamDeref( p->pStrs );
    Abc_NamDeref( p->pFuns );
    Abc_NamDeref( p->pMods );
    Hash_IntManDeref( p->vHash );
    Vec_IntErase( &p->vNameMap );
    Vec_IntErase( &p->vUsed );
    Vec_IntErase( &p->vNameMap2 );
    Vec_IntErase( &p->vUsed2 );
    Vec_StrErase( &p->vOut );
    Vec_StrErase( &p->vOut2 );
    ABC_FREE( p->pName );
    ABC_FREE( p->pSpec );
    ABC_FREE( p );
}
static inline int Acb_ManGetMap( Acb_Man_t * p, int i )
{
    return Vec_IntGetEntry(&p->vNameMap, i);
}
static inline void Acb_ManSetMap( Acb_Man_t * p, int i, int x )
{
    assert( Vec_IntGetEntry(&p->vNameMap, i) == 0 );
    Vec_IntSetEntry( &p->vNameMap, i, x );
    Vec_IntPush( &p->vUsed, i );
}
static inline void Acb_ManUnsetMap( Acb_Man_t * p, int i )
{
    Vec_IntSetEntry( &p->vNameMap, i, 0 );
}
static inline void Acb_ManCleanMap( Acb_Man_t * p )
{
    int i, Entry;
    Vec_IntForEachEntry( &p->vUsed, Entry, i )
        Vec_IntWriteEntry( &p->vNameMap, Entry, 0 );
    Vec_IntClear( &p->vUsed );
}
static inline int  Acb_NtkGetMap( Acb_Ntk_t * p, int i )          { return Acb_ManGetMap(p->pDesign, i); }
static inline void Acb_NtkSetMap( Acb_Ntk_t * p, int i, int x )   { Acb_ManSetMap(p->pDesign, i, x);     }
static inline void Acb_NtkUnsetMap( Acb_Ntk_t * p, int i )        { Acb_ManUnsetMap(p->pDesign, i);      }
static inline void Acb_NtkCleanMap( Acb_Ntk_t * p )               { Acb_ManCleanMap(p->pDesign);         }

static inline int Acb_ManGetMap2( Acb_Man_t * p, int i )
{
    return Vec_IntGetEntry(&p->vNameMap2, i);
}
static inline void Acb_ManSetMap2( Acb_Man_t * p, int i, int x )
{
    assert( Vec_IntGetEntry(&p->vNameMap2, i) == 0 );
    Vec_IntSetEntry( &p->vNameMap2, i, x );
    Vec_IntPush( &p->vUsed2, i );
}
static inline void Acb_ManUnsetMap2( Acb_Man_t * p, int i )
{
    Vec_IntSetEntry( &p->vNameMap2, i, 0 );
}
static inline void Acb_ManCleanMap2( Acb_Man_t * p )
{
    int i, Entry;
    Vec_IntForEachEntry( &p->vUsed2, Entry, i )
        Vec_IntWriteEntry( &p->vNameMap2, Entry, 0 );
    Vec_IntClear( &p->vUsed2 );
}
static inline int  Acb_NtkGetMap2( Acb_Ntk_t * p, int i )          { return Acb_ManGetMap2(p->pDesign, i); }
static inline void Acb_NtkSetMap2( Acb_Ntk_t * p, int i, int x )   { Acb_ManSetMap2(p->pDesign, i, x);     }
static inline void Acb_NtkUnsetMap2( Acb_Ntk_t * p, int i )        { Acb_ManUnsetMap2(p->pDesign, i);      }
static inline void Acb_NtkCleanMap2( Acb_Ntk_t * p )               { Acb_ManCleanMap2(p->pDesign);         }

static inline int Acb_ManMemory( Acb_Man_t * p )
{
    Acb_Ntk_t * pNtk; int i;
    int nMem = sizeof(Acb_Man_t);
    nMem += p->pName ? (int)strlen(p->pName) : 0;
    nMem += p->pSpec ? (int)strlen(p->pSpec) : 0;
    nMem += Abc_NamMemUsed(p->pStrs);
    nMem += Abc_NamMemUsed(p->pFuns);
    nMem += Abc_NamMemUsed(p->pMods);
    nMem += (int)Vec_IntMemory(&p->vNameMap );   
    nMem += (int)Vec_IntMemory(&p->vUsed );  
    nMem += (int)Vec_StrMemory(&p->vOut );   
    nMem += (int)Vec_StrMemory(&p->vOut2 );  
    nMem += (int)Vec_PtrMemory(&p->vNtks);
    Acb_ManForEachNtk( p, pNtk, i )
        nMem += Acb_NtkMemory( pNtk );
    return nMem;
}
static inline int Acb_ManObjNum( Acb_Man_t * p )
{
    Acb_Ntk_t * pNtk; int i, Count = 0;
    Acb_ManForEachNtk( p, pNtk, i )
        Count += Acb_NtkObjNum(pNtk);
    return Count;
}
static inline int Acb_ManBoxNum( Acb_Man_t * p )
{
    Acb_Ntk_t * pNtk; int i, Count = 0;
    Acb_ManForEachNtk( p, pNtk, i )
        Count += Acb_NtkBoxNum( pNtk );
    return Count;
}
static inline void Acb_ManBoxNumRec_rec( Acb_Ntk_t * p, int * pCountP, int * pCountU )
{
    int iObj, Id = Acb_NtkId(p);
    if ( pCountP[Id] >= 0 )
        return;
    pCountP[Id] = pCountU[Id] = 0;
    Acb_NtkForEachObj( p, iObj )
    {
        if ( Acb_ObjIsBox(p, iObj) )
        {
            Acb_ManBoxNumRec_rec( Acb_ObjNtk(p, iObj), pCountP, pCountU );
            pCountP[Id] += pCountP[Acb_ObjNtkId(p, iObj)];
            pCountU[Id] += pCountU[Acb_ObjNtkId(p, iObj)] + 1;
        }
        else
            pCountP[Id] += 1;
    }
}
static inline void Acb_ManBoxNumRec( Acb_Man_t * p, int * pnPrims, int * pnUsers )
{
    Acb_Ntk_t * pNtk = Acb_ManRoot(p);
    int * pCountP = ABC_FALLOC( int, Acb_ManNtkNum(p) + 1 );
    int * pCountU = ABC_FALLOC( int, Acb_ManNtkNum(p) + 1 );
    Acb_ManBoxNumRec_rec( pNtk, pCountP, pCountU );
    *pnPrims = pCountP[Acb_NtkId(pNtk)];
    *pnUsers = pCountU[Acb_NtkId(pNtk)];
    ABC_FREE( pCountP );
    ABC_FREE( pCountU );
}
static inline void Acb_ManPrintStats( Acb_Man_t * p, int nModules, int fVerbose )
{
    Acb_Ntk_t * pNtk; int i, nPrims, nUsers;
    Acb_Ntk_t * pRoot = Acb_ManRoot( p );
    Acb_ManBoxNumRec( p, &nPrims, &nUsers );
    printf( "%-12s : ",     Acb_ManName(p) );
    printf( "pi =%5d  ",    Acb_NtkCiNum(pRoot) );
    printf( "po =%5d  ",    Acb_NtkCoNum(pRoot) );
    printf( "mod =%5d  ",   Acb_ManNtkNum(p) );
    printf( "box =%5d  ",   nPrims + nUsers );
    printf( "prim =%5d  ",  nPrims );
    printf( "user =%5d  ",  nUsers );
    printf( "mem =%6.3f MB", 1.0*Acb_ManMemory(p)/(1<<20) );
    printf( "\n" );
    Acb_ManForEachNtk( p, pNtk, i )
    {
        if ( i == nModules+1 )
            break;
        printf( "Module %5d : ", i );
        Acb_NtkPrintStats( pNtk );
    }
}



/*=== acbUtil.c =============================================================*/
extern Vec_Int_t * Acb_ObjCollectTfi( Acb_Ntk_t * p, int iObj, int fTerm );
extern Vec_Int_t * Acb_ObjCollectTfo( Acb_Ntk_t * p, int iObj, int fTerm );

extern Vec_Int_t * Acb_ObjCollectTfiVec( Acb_Ntk_t * p, Vec_Int_t * vObjs );
extern Vec_Int_t * Acb_ObjCollectTfoVec( Acb_Ntk_t * p, Vec_Int_t * vObjs );
extern int         Acb_NtkCountPiBuffers( Acb_Ntk_t * p, Vec_Int_t * vObjs );
extern int         Acb_NtkCountPoDrivers( Acb_Ntk_t * p, Vec_Int_t * vObjs );
extern int         Acb_NtkFindMffcSize( Acb_Ntk_t * p, Vec_Int_t * vObjsRefed, Vec_Int_t * vObjsDerefed, int nGates[5] );

extern int         Acb_ObjComputeLevelD( Acb_Ntk_t * p, int iObj );
extern int         Acb_NtkComputeLevelD( Acb_Ntk_t * p, Vec_Int_t * vTfo );
extern void        Acb_NtkUpdateLevelD( Acb_Ntk_t * p, int iObj );
extern void        Acb_NtkUpdateTiming( Acb_Ntk_t * p, int iObj );

extern void        Acb_NtkPrintNode( Acb_Ntk_t * p, int iObj );
extern int         Acb_NtkCreateNode( Acb_Ntk_t * p, word uTruth, Vec_Int_t * vSupp );
extern void        Acb_NtkUpdateNode( Acb_Ntk_t * p, int Pivot, word uTruth, Vec_Int_t * vSupp );

extern Acb_Ntk_t * Acb_VerilogSimpleRead( char * pFileName, char * pFileNameW );

ABC_NAMESPACE_HEADER_END


#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

