/**CFile****************************************************************

  FileName    [cba.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchical word-level netlist.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 21, 2015.]

  Revision    [$Id: cba.h,v 1.00 2014/11/29 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__base__cba__cba_h
#define ABC__base__cba__cba_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "aig/gia/gia.h"
#include "misc/extra/extra.h"
#include "misc/util/utilNam.h"
#include "misc/vec/vecHash.h"
#include "cbaTypes.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START 

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////


typedef struct Cba_Ntk_t_ Cba_Ntk_t;
typedef struct Cba_Man_t_ Cba_Man_t;

// network
struct Cba_Ntk_t_
{
    Cba_Man_t *     pDesign;  // design
    int             Id;       // network ID
    int             NameId;   // name ID 
    int             iCopy;    // copy module
    unsigned        Mark : 1; // visit mark 
    unsigned        fComb: 1; // the network is combinational
    unsigned        fSeq : 1; // the network is sequential
    // interface
    Vec_Int_t       vInputs;  // inputs 
    Vec_Int_t       vOutputs; // outputs
    Vec_Int_t       vOrder;   // order
    Vec_Int_t       vSeq;     // sequential objects
    // stucture
    Vec_Str_t       vObjType; // type     
    Vec_Int_t       vObjFin0; // fanins
    Vec_Int_t       vObjFon0; // outputs
    Vec_Int_t       vFinFon;  // fons
    Vec_Int_t       vFonObj;  // object
    // optional
    Vec_Int_t       vObjCopy; // copy
    Vec_Int_t       vObjFunc; // function
    Vec_Int_t       vObjName; // name
    Vec_Int_t       vObjAttr; // attribute offset
    Vec_Int_t       vAttrSto; // attribute storage
    Vec_Int_t       vFonCopy; // copy
    Vec_Int_t       vFonName; // name
    Vec_Int_t       vFonRange;// range
    Vec_Int_t       vFonPrev; // fanout: prev fon
    Vec_Int_t       vFonNext; // fanout: next fon
    Vec_Int_t       vFinFon0; // fanout: first fon
    Vec_Int_t       vFinObj;  // object
    Vec_Int_t       vNtkObjs; // instances
    Vec_Int_t       vFonBits; // fon mapping into AIG nodes
    // other
    Vec_Ptr_t *     vOther;   // various data
    Vec_Int_t       vArray0;
    Vec_Int_t       vArray1;
};

// design
struct Cba_Man_t_
{
    // design names
    char *          pName;    // design name
    char *          pSpec;    // spec file name
    Abc_Nam_t *     pStrs;    // string manager
    Abc_Nam_t *     pFuns;    // constant manager
    Abc_Nam_t *     pMods;    // module name manager
    Hash_IntMan_t * vHash; // variable ranges
    Vec_Int_t       vNameMap; // mapping names
    Vec_Int_t       vNameMap2;// mapping names
    Vec_Int_t       vUsed;    // used map entries
    Vec_Int_t       vUsed2;   // used map entries
    char *          pTypeNames[CBA_BOX_LAST];
    int             nObjs[CBA_BOX_LAST]; // counter of objects of each type
    int             nAnds[CBA_BOX_LAST]; // counter of AND gates after blasting
    // internal data
    int             iRoot;    // root network
    Vec_Ptr_t       vNtks;    // networks
    // user data
    int             nOpens;
    Vec_Str_t       vOut;     
    Vec_Str_t       vOut2;     
    void *          pMioLib;
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

static inline char *         Cba_ManName( Cba_Man_t * p )                    { return p->pName;                                                                            }
static inline char *         Cba_ManSpec( Cba_Man_t * p )                    { return p->pSpec;                                                                            }
static inline int            Cba_ManNtkNum( Cba_Man_t * p )                  { return Vec_PtrSize(&p->vNtks)-1;                                                            }
static inline int            Cba_ManNtkIsOk( Cba_Man_t * p, int i )          { return i > 0 && i <= Cba_ManNtkNum(p);                                                      }
static inline Cba_Ntk_t *    Cba_ManNtk( Cba_Man_t * p, int i )              { return Cba_ManNtkIsOk(p, i) ? (Cba_Ntk_t *)Vec_PtrEntry(&p->vNtks, i) : NULL;               }
static inline int            Cba_ManNtkFindId( Cba_Man_t * p, char * pName ) { return Abc_NamStrFind(p->pMods, pName);                                                     }
static inline Cba_Ntk_t *    Cba_ManNtkFind( Cba_Man_t * p, char * pName )   { return Cba_ManNtk( p, Cba_ManNtkFindId(p, pName) );                                         }
static inline Cba_Ntk_t *    Cba_ManRoot( Cba_Man_t * p )                    { return Cba_ManNtk(p, p->iRoot);                                                             }
static inline char *         Cba_ManStr( Cba_Man_t * p, int i )              { return Abc_NamStr(p->pStrs, i);                                                             }
static inline int            Cba_ManStrId( Cba_Man_t * p, char * pStr )      { return Abc_NamStrFind(p->pStrs, pStr);                                                      }
static inline int            Cba_ManNameIdMax( Cba_Man_t * p )               { return Abc_NamObjNumMax(p->pStrs) + 1;                                                      }
static inline char *         Cba_ManConst( Cba_Man_t * p, int i )            { return Abc_NamStr(p->pFuns, i);                                                             }
static inline int            Cba_ManConstId( Cba_Man_t * p, char * pStr )    { return Abc_NamStrFind(p->pFuns, pStr);                                                      }
static inline int            Cba_ManConstIdMax( Cba_Man_t * p )              { return Abc_NamObjNumMax(p->pFuns) + 1;                                                      }

static inline Cba_Man_t *    Cba_NtkMan( Cba_Ntk_t * p )                     { return p->pDesign;                                                                          }
static inline Cba_Ntk_t *    Cba_NtkNtk( Cba_Ntk_t * p, int i )              { return Cba_ManNtk(p->pDesign, i);                                                           }
static inline int            Cba_NtkId( Cba_Ntk_t * p )                      { return p->Id;                                                                               }
static inline int            Cba_NtkPi( Cba_Ntk_t * p, int i )               { return Vec_IntEntry(&p->vInputs, i);                                                        }
static inline int            Cba_NtkPo( Cba_Ntk_t * p, int i )               { return Vec_IntEntry(&p->vOutputs, i);                                                       }
static inline int            Cba_NtkPioOrder( Cba_Ntk_t * p, int i )         { return Vec_IntEntry(&p->vOrder, i);                                                         }
static inline int            Cba_NtkBoxSeq( Cba_Ntk_t * p, int i )           { return Vec_IntEntry(&p->vSeq, i);                                                           }
static inline Abc_Nam_t *    Cba_NtkNam( Cba_Ntk_t * p )                     { return p->pDesign->pStrs;                                                                   }
static inline char *         Cba_NtkStr( Cba_Ntk_t * p, int i )              { return Cba_ManStr(p->pDesign, i);                                                           }
static inline int            Cba_NtkStrId( Cba_Ntk_t * p, char * pName )     { return Cba_ManStrId(p->pDesign, pName);                                                     }
static inline char *         Cba_NtkConst( Cba_Ntk_t * p, int i )            { return Cba_ManConst(p->pDesign, i);                                                         }
static inline char *         Cba_NtkSop( Cba_Ntk_t * p, int i )              { return Cba_ManConst(p->pDesign, i);                                                         }
static inline int            Cba_NtkNameId( Cba_Ntk_t * p )                  { return p->NameId;                                                                           }
static inline char *         Cba_NtkName( Cba_Ntk_t * p )                    { return Cba_NtkStr(p, p->NameId);                                                            }
static inline char *         Cba_NtkTypeName( Cba_Ntk_t * p, int Type )      { return p->pDesign->pTypeNames[Type];                                                        }
static inline int            Cba_NtkCopy( Cba_Ntk_t * p )                    { return p->iCopy;                                                                            }
static inline Cba_Ntk_t *    Cba_NtkCopyNtk(Cba_Man_t * pNew, Cba_Ntk_t * p) { return Cba_ManNtk(pNew, Cba_NtkCopy(p));                                                    }
static inline void           Cba_NtkSetCopy( Cba_Ntk_t * p, int i )          { assert(p->iCopy == 0); p->iCopy = i;                                                        }
static inline int            Cba_NtkHashRange( Cba_Ntk_t * p, int l, int r ) { return Hash_Int2ManInsert( p->pDesign->vHash, l, r, 0 );                                    }
static inline int            Cba_NtkRangeLeft( Cba_Ntk_t * p, int h )        { return h ? Hash_IntObjData0( p->pDesign->vHash, h ) : 0;                                    }
static inline int            Cba_NtkRangeRight( Cba_Ntk_t * p, int h )       { return h ? Hash_IntObjData1( p->pDesign->vHash, h ) : 0;                                    }
static inline int            Cba_NtkRangeSize( Cba_Ntk_t * p, int h )        { int l = Cba_NtkRangeLeft(p, h), r = Cba_NtkRangeRight(p, h); return 1 + (l > r ? l-r : r-l);}

static inline int            Cba_NtkPiNum( Cba_Ntk_t * p )                   { return Vec_IntSize(&p->vInputs);                                                            }
static inline int            Cba_NtkPoNum( Cba_Ntk_t * p )                   { return Vec_IntSize(&p->vOutputs);                                                           }
static inline int            Cba_NtkPioNum( Cba_Ntk_t * p )                  { return Cba_NtkPiNum(p) + Cba_NtkPoNum(p);                                                   }
static inline int            Cba_NtkPiNumAlloc( Cba_Ntk_t * p )              { return Vec_IntCap(&p->vInputs);                                                             }
static inline int            Cba_NtkPoNumAlloc( Cba_Ntk_t * p )              { return Vec_IntCap(&p->vOutputs);                                                            }
static inline int            Cba_NtkPioOrderNum( Cba_Ntk_t * p )             { return Vec_IntSize(&p->vOrder);                                                             }
static inline int            Cba_NtkObjNum( Cba_Ntk_t * p )                  { return Vec_StrSize(&p->vObjType)-1;                                                         }
static inline int            Cba_NtkObjNumAlloc( Cba_Ntk_t * p )             { return Vec_StrCap(&p->vObjType)-1;                                                          }
static inline int            Cba_NtkFinNum( Cba_Ntk_t * p )                  { return Vec_IntSize(&p->vFinFon)-1;                                                          }
static inline int            Cba_NtkFinNumAlloc( Cba_Ntk_t * p )             { return Vec_IntCap(&p->vFinFon)-1;                                                           }
static inline int            Cba_NtkFonNum( Cba_Ntk_t * p )                  { return Vec_IntSize(&p->vFonObj)-1;                                                          }
static inline int            Cba_NtkFonNumAlloc( Cba_Ntk_t * p )             { return Vec_IntCap(&p->vFonObj)-1;                                                           }
static inline int            Cba_NtkTypeNum( Cba_Ntk_t * p, int Type )       { return Vec_StrCountEntry(&p->vObjType, (char)Type);                                         }
static inline int            Cba_NtkBoxNum( Cba_Ntk_t * p )                  { return Cba_NtkObjNum(p) - Cba_NtkPioNum(p);                                                 }
static inline int            Cba_NtkBoxUserNum( Cba_Ntk_t * p )              { return Cba_NtkTypeNum(p, CBA_OBJ_BOX);                                                      }
static inline int            Cba_NtkBoxPrimNum( Cba_Ntk_t * p )              { return Vec_StrCountLarger(&p->vObjType, (char)CBA_OBJ_BOX);                                 }
static inline int            Cba_NtkBoxSeqNum( Cba_Ntk_t * p )               { return Vec_IntSize(&p->vSeq);                                                               }

static inline void           Cba_NtkCleanObjCopies( Cba_Ntk_t * p )          { Vec_IntFill(&p->vObjCopy,  Vec_StrCap(&p->vObjType), -1);        }
static inline void           Cba_NtkCleanObjFuncs( Cba_Ntk_t * p )           { Vec_IntFill(&p->vObjFunc,  Vec_StrCap(&p->vObjType),  0);        }
static inline void           Cba_NtkCleanObjNames( Cba_Ntk_t * p )           { Vec_IntFill(&p->vObjName,  Vec_StrCap(&p->vObjType),  0);        }
static inline void           Cba_NtkCleanObjAttrs( Cba_Ntk_t * p )           { Vec_IntFill(&p->vObjAttr,  Vec_StrCap(&p->vObjType),  0); Vec_IntFill(&p->vAttrSto, 1, -1); }
static inline void           Cba_NtkCleanFonCopies( Cba_Ntk_t * p )          { Vec_IntFill(&p->vFonCopy,  Vec_IntCap(&p->vFonObj),   0);        }
static inline void           Cba_NtkCleanFonNames( Cba_Ntk_t * p )           { Vec_IntFill(&p->vFonName,  Vec_IntCap(&p->vFonObj),   0);        }
static inline void           Cba_NtkCleanFonRanges( Cba_Ntk_t * p )          { Vec_IntFill(&p->vFonRange, Vec_IntCap(&p->vFonObj),   0);        }
static inline void           Cba_NtkCleanFonPrevs( Cba_Ntk_t * p )           { Vec_IntFill(&p->vFonPrev,  Vec_IntCap(&p->vFonObj),   0);        }
static inline void           Cba_NtkCleanFonNexts( Cba_Ntk_t * p )           { Vec_IntFill(&p->vFonNext,  Vec_IntCap(&p->vFonObj),   0);        }
static inline void           Cba_NtkCleanFinFon0( Cba_Ntk_t * p )            { Vec_IntFill(&p->vFinFon0,  Vec_IntCap(&p->vFinFon),   0);        }
static inline void           Cba_NtkCleanFinObjs( Cba_Ntk_t * p )            { Vec_IntFill(&p->vFinObj,   Vec_IntCap(&p->vFinFon),   0);        }

static inline int            Cba_NtkHasObjCopies( Cba_Ntk_t * p )            { return Vec_IntSize(&p->vObjCopy)  > 0; }
static inline int            Cba_NtkHasObjFuncs( Cba_Ntk_t * p )             { return Vec_IntSize(&p->vObjFunc)  > 0; }
static inline int            Cba_NtkHasObjNames( Cba_Ntk_t * p )             { return Vec_IntSize(&p->vObjName)  > 0; }
static inline int            Cba_NtkHasObjAttrs( Cba_Ntk_t * p )             { return Vec_IntSize(&p->vObjAttr)  > 0; }
static inline int            Cba_NtkHasFonCopies( Cba_Ntk_t * p )            { return Vec_IntSize(&p->vFonCopy)  > 0; }
static inline int            Cba_NtkHasFonNames( Cba_Ntk_t * p )             { return Vec_IntSize(&p->vFonName)  > 0; }
static inline int            Cba_NtkHasFonRanges( Cba_Ntk_t * p )            { return Vec_IntSize(&p->vFonRange) > 0; }
static inline int            Cba_NtkHasFonPrevs( Cba_Ntk_t * p )             { return Vec_IntSize(&p->vFonPrev)  > 0; }
static inline int            Cba_NtkHasFonNexts( Cba_Ntk_t * p )             { return Vec_IntSize(&p->vFonNext)  > 0; }
static inline int            Cba_NtkHasFinFon0( Cba_Ntk_t * p )              { return Vec_IntSize(&p->vFinFon0)  > 0; }
static inline int            Cba_NtkHasFinObjs( Cba_Ntk_t * p )              { return Vec_IntSize(&p->vFinObj)   > 0; }

static inline void           Cba_NtkFreeObjCopies( Cba_Ntk_t * p )           { Vec_IntErase(&p->vObjCopy);   }
static inline void           Cba_NtkFreeObjFuncs( Cba_Ntk_t * p )            { Vec_IntErase(&p->vObjFunc);   }
static inline void           Cba_NtkFreeObjNames( Cba_Ntk_t * p )            { Vec_IntErase(&p->vObjName);   }
static inline void           Cba_NtkFreeObjAttrs( Cba_Ntk_t * p )            { Vec_IntErase(&p->vObjAttr);   }
static inline void           Cba_NtkFreeFonCopies( Cba_Ntk_t * p )           { Vec_IntErase(&p->vFonCopy);   }
static inline void           Cba_NtkFreeFonNames( Cba_Ntk_t * p )            { Vec_IntErase(&p->vFonName);   }
static inline void           Cba_NtkFreeFonRanges( Cba_Ntk_t * p )           { Vec_IntErase(&p->vFonRange);  }
static inline void           Cba_NtkFreeFonPrevs( Cba_Ntk_t * p )            { Vec_IntErase(&p->vFonPrev);   }
static inline void           Cba_NtkFreeFonNexts( Cba_Ntk_t * p )            { Vec_IntErase(&p->vFonNext);   }
static inline void           Cba_NtkFreeFinFon0( Cba_Ntk_t * p )             { Vec_IntErase(&p->vFinFon0);   }
static inline void           Cba_NtkFreeFinObjs( Cba_Ntk_t * p )             { Vec_IntErase(&p->vFinObj);    }

static inline Cba_ObjType_t  Cba_ObjType( Cba_Ntk_t * p, int i )             { assert(i>0); return (Cba_ObjType_t)(int)(unsigned char)Vec_StrEntry(&p->vObjType, i);       }
static inline void           Cba_ObjCleanType( Cba_Ntk_t * p, int i )        { assert(i>0); Vec_StrWriteEntry( &p->vObjType, i, (char)CBA_OBJ_NONE );                      }
static inline int            Cba_TypeIsBox( Cba_ObjType_t Type )             { return Type >= CBA_OBJ_BOX && Type < CBA_BOX_LAST;                                          }
static inline int            Cba_TypeIsSeq( Cba_ObjType_t Type )             { return Type >= CBA_BOX_RAM && Type <= CBA_BOX_DFFRS;                                        }
static inline int            Cba_TypeIsUnary( Cba_ObjType_t Type )           { return Type == CBA_BOX_BUF || Type == CBA_BOX_INV || Type == CBA_BOX_LNOT || Type == CBA_BOX_MIN || Type == CBA_BOX_SQRT || Type == CBA_BOX_ABS || (Type >= CBA_BOX_RAND && Type <= CBA_BOX_RXNOR);  }
static inline int            Cba_TypeIsMux( Cba_ObjType_t Type )             { return Type == CBA_BOX_MUX || Type == CBA_BOX_NMUX || Type == CBA_BOX_SEL || Type == CBA_BOX_PSEL;  }

static inline int            Cba_ObjIsPi( Cba_Ntk_t * p, int i )             { return Cba_ObjType(p, i) == CBA_OBJ_PI;                                                     }
static inline int            Cba_ObjIsPo( Cba_Ntk_t * p, int i )             { return Cba_ObjType(p, i) == CBA_OBJ_PO;                                                     }
static inline int            Cba_ObjIsPio( Cba_Ntk_t * p, int i )            { return Cba_ObjIsPi(p, i) || Cba_ObjIsPo(p, i);                                              }
static inline int            Cba_ObjIsBox( Cba_Ntk_t * p, int i )            { return Cba_TypeIsBox(Cba_ObjType(p, i));                                                    }
static inline int            Cba_ObjIsBoxUser( Cba_Ntk_t * p, int i )        { return Cba_ObjType(p, i) == CBA_OBJ_BOX;                                                    }
static inline int            Cba_ObjIsBoxPrim( Cba_Ntk_t * p, int i )        { return Cba_ObjType(p, i) >  CBA_OBJ_BOX && Cba_ObjType(p, i) < CBA_BOX_LAST;                }
static inline int            Cba_ObjIsGate( Cba_Ntk_t * p, int i )           { return Cba_ObjType(p, i) == CBA_BOX_GATE;                                                   }
static inline int            Cba_ObjIsSlice( Cba_Ntk_t * p, int i )          { return Cba_ObjType(p, i) == CBA_BOX_SLICE;                                                  }
static inline int            Cba_ObjIsConcat( Cba_Ntk_t * p, int i )         { return Cba_ObjType(p, i) == CBA_BOX_CONCAT;                                                 }
static inline int            Cba_ObjIsUnary( Cba_Ntk_t * p, int i )          { return Cba_TypeIsUnary(Cba_ObjType(p, i));                                                  }

static inline int            Cba_ObjFin0( Cba_Ntk_t * p, int i )             { assert(i>0); return Vec_IntEntry(&p->vObjFin0, i);                                          }
static inline int            Cba_ObjFon0( Cba_Ntk_t * p, int i )             { assert(i>0); return Vec_IntEntry(&p->vObjFon0, i);                                          }
static inline int            Cba_ObjFin( Cba_Ntk_t * p, int i, int k )       { assert(i>0); return Cba_ObjFin0(p, i) + k;                                                  }
static inline int            Cba_ObjFon( Cba_Ntk_t * p, int i, int k )       { assert(i>0); return Cba_ObjFon0(p, i) + k;                                                  }
static inline int            Cba_ObjFinNum( Cba_Ntk_t * p, int i )           { assert(i>0); return Cba_ObjFin0(p, i+1) - Cba_ObjFin0(p, i);                                }
static inline int            Cba_ObjFonNum( Cba_Ntk_t * p, int i )           { assert(i>0); return Cba_ObjFon0(p, i+1) - Cba_ObjFon0(p, i);                                }

static inline int            Cba_ObjCopy( Cba_Ntk_t * p, int i )             { assert(i>0); assert( Cba_NtkHasObjCopies(p) ); return Vec_IntGetEntryFull(&p->vObjCopy, i); }
static inline int            Cba_ObjFunc( Cba_Ntk_t * p, int i )             { assert(i>0); assert( Cba_NtkHasObjFuncs(p) );  return Vec_IntGetEntry(&p->vObjFunc, i);     }
static inline int            Cba_ObjName( Cba_Ntk_t * p, int i )             { assert(i>0); assert( Cba_NtkHasObjNames(p) );  return Vec_IntGetEntry(&p->vObjName, i);     }
static inline char *         Cba_ObjNameStr( Cba_Ntk_t * p, int i )          { assert(i>0); return Cba_NtkStr(p, Cba_ObjName(p, i));                                       }
static inline int            Cba_ObjAttr( Cba_Ntk_t * p, int i )             { assert(i>=0); return Cba_NtkHasObjAttrs(p) ? Vec_IntGetEntry(&p->vObjAttr, i) : 0;          }
static inline int            Cba_ObjAttrSize( Cba_Ntk_t * p, int i )         { assert(i>=0); return Cba_ObjAttr(p, i) ? Vec_IntEntry(&p->vAttrSto, Cba_ObjAttr(p, i)) : 0; }
static inline int *          Cba_ObjAttrArray( Cba_Ntk_t * p, int i )        { assert(i>=0); return Cba_ObjAttr(p, i) ? Vec_IntEntryP(&p->vAttrSto, Cba_ObjAttr(p, i)+1) : NULL; }
static inline int            Cba_ObjAttrValue( Cba_Ntk_t * p, int i, int x ) { int k, s = Cba_ObjAttrSize(p, i), * a = Cba_ObjAttrArray(p, i); for ( k = 0; k < s; k += 2)  if (a[k] == x) return a[k+1]; return 0; }

static inline void           Cba_ObjSetCopy( Cba_Ntk_t * p, int i, int x )   { assert(Cba_ObjCopy(p, i) == -1); Vec_IntSetEntry( &p->vObjCopy, i, x );                     }
static inline void           Cba_ObjSetFunc( Cba_Ntk_t * p, int i, int x )   { assert(Cba_ObjFunc(p, i) ==  0); Vec_IntSetEntry( &p->vObjFunc, i, x );                     }
static inline void           Cba_ObjSetName( Cba_Ntk_t * p, int i, int x )   { assert(Cba_ObjName(p, i) ==  0); Vec_IntSetEntry( &p->vObjName, i, x );                     }
static inline void           Cba_ObjSetAttrs( Cba_Ntk_t * p, int i, int * a, int s )  { assert(Cba_ObjAttr(p, i) == 0); if ( !a ) return; Vec_IntSetEntry(&p->vObjAttr, i, Vec_IntSize(&p->vAttrSto)); Vec_IntPush(&p->vAttrSto, s); Vec_IntPushArray(&p->vAttrSto, a, s);  }

static inline int            Cba_FinFon( Cba_Ntk_t * p, int f )              { assert(f>0); return Vec_IntEntry(&p->vFinFon, f);                                           }
static inline void           Cba_PatchFinFon( Cba_Ntk_t * p, int f, int x )  { assert(f>0); Vec_IntWriteEntry(&p->vFinFon, f, x);                                          }
static inline int            Cba_ObjFinFon( Cba_Ntk_t * p, int i, int k )    { assert(i>0); return Cba_FinFon(p, Cba_ObjFin(p, i, k));                                     }
static inline int *          Cba_ObjFinFons( Cba_Ntk_t * p, int i )          { assert(i>0); return Vec_IntEntryP(&p->vFinFon, Cba_ObjFin0(p, i));                          }

static inline void           Cba_ObjSetFinFon( Cba_Ntk_t * p, int i, int k, int x ) { assert(i>0); assert(Cba_ObjFinFon(p, i, k)== 0); Vec_IntWriteEntry(&p->vFinFon, Cba_ObjFin(p, i, k), x); }
static inline void           Cba_ObjCleanFinFon( Cba_Ntk_t * p, int i, int k)       { assert(i>0); assert(Cba_ObjFinFon(p, i, k)!= 0); Vec_IntWriteEntry(&p->vFinFon, Cba_ObjFin(p, i, k), 0); }
static inline void           Cba_ObjPatchFinFon( Cba_Ntk_t * p, int i, int k, int x){ assert(i>0); Cba_ObjCleanFinFon(p, i, k); Cba_ObjSetFinFon(p, i, k, x);                                  }

static inline int            Cba_ObjNtkId( Cba_Ntk_t * p, int i )            { assert(i>0); return (Cba_ObjIsBoxUser(p, i) && Cba_NtkHasObjFuncs(p)) ? Cba_ObjFunc(p, i) : 0;}
static inline Cba_Ntk_t *    Cba_ObjNtk( Cba_Ntk_t * p, int i )              { assert(i>0); return Cba_NtkNtk(p, Cba_ObjNtkId(p, i));                                      }
static inline void           Cba_ObjSetNtkId( Cba_Ntk_t * p, int i, int x )  { assert(i>0); assert(Cba_ObjIsBoxUser(p, i)); Cba_ObjSetFunc( p, i, x );                     }
static inline int            Cba_ObjIsSeq( Cba_Ntk_t * p, int i )            { assert(i>0); return Cba_ObjIsBoxUser(p, i) ? Cba_ObjNtk(p, i)->fSeq : Cba_TypeIsSeq(Cba_ObjType(p, i));  }

static inline int            Cba_FonIsReal( int f )                          { return f > 0;                             }
static inline int            Cba_FonIsConst( int f )                         { return f < 0;                             }
static inline int            Cba_FonConst( int f )                           { assert(Cba_FonIsConst(f)); return -f-1;   }
static inline int            Cba_FonFromConst( int c )                       { assert(c >= 0); return -c-1;              }
static inline int            Cba_FonConstRangeSize( Cba_Ntk_t * p, int f )   { assert(Cba_FonIsConst(f)); return atoi(Cba_NtkConst(p, Cba_FonConst(f)));                   }
static inline int            Cba_FonConstSigned( Cba_Ntk_t * p, int f )      { assert(Cba_FonIsConst(f)); return strchr(Cba_NtkConst(p, Cba_FonConst(f)), 's') != NULL;    }

static inline int            Cba_FonObj( Cba_Ntk_t * p, int f )              { return Cba_FonIsReal(f) ? Vec_IntEntry(&p->vFonObj, f) : 0;                                 }
static inline int            Cba_FonRangeId( Cba_Ntk_t * p, int f )          { assert(Cba_FonIsReal(f)); return Cba_NtkHasFonRanges(p)?Vec_IntGetEntry(&p->vFonRange, f):0;}
static inline int            Cba_FonRange( Cba_Ntk_t * p, int f )            { assert(Cba_FonIsReal(f)); return Abc_Lit2Var( Cba_FonRangeId(p, f) );                       }
static inline int            Cba_FonLeft( Cba_Ntk_t * p, int f )             { return Cba_NtkRangeLeft(p, Cba_FonRange(p, f));                                             }
static inline int            Cba_FonRight( Cba_Ntk_t * p, int f )            { return Cba_NtkRangeRight(p, Cba_FonRange(p, f));                                            }
static inline int            Cba_FonSigned( Cba_Ntk_t * p, int f )           { return Cba_FonIsConst(f) ? Cba_FonConstSigned(p, f) : Abc_LitIsCompl(Cba_FonRangeId(p, f)); }
static inline int            Cba_FonRangeSize( Cba_Ntk_t * p, int f )        { return Cba_FonIsConst(f) ? Cba_FonConstRangeSize(p, f):Cba_NtkRangeSize(p, Cba_FonRange(p, f)); }
static inline void           Cba_FonSetRangeSign( Cba_Ntk_t * p, int f, int x )  { assert(Cba_NtkHasFonRanges(p));  Vec_IntSetEntry(&p->vFonRange, f, x);                  }
static inline void           Cba_FonSetRange( Cba_Ntk_t * p, int f, int x )  { assert(Cba_NtkHasFonRanges(p));  Vec_IntSetEntry(&p->vFonRange, f, Abc_Var2Lit(x,0));       }
static inline void           Cba_FonHashRange( Cba_Ntk_t * p, int f, int l, int r ) { Cba_FonSetRange( p, f, Cba_NtkHashRange(p, l, r) );                                  }
static inline int            Cba_FonCopy( Cba_Ntk_t * p, int f )             { return Cba_FonIsReal(f) ? Vec_IntEntry(&p->vFonCopy, f) : f;                                }
static inline void           Cba_FonSetCopy( Cba_Ntk_t * p, int f, int x )   { assert(Cba_FonIsReal(f)); assert(Cba_FonCopy(p, f) == 0); Vec_IntWriteEntry(&p->vFonCopy, f, x); }
static inline int            Cba_FonName( Cba_Ntk_t * p, int f )             { assert(Cba_NtkHasFonNames(p)); assert(Cba_FonIsReal(f)); return Vec_IntGetEntry( &p->vFonName, f );                        }
static inline char *         Cba_FonNameStr( Cba_Ntk_t * p, int f )          { assert(Cba_NtkHasFonNames(p)); assert(Cba_FonIsReal(f)); return Cba_NtkStr(p, Cba_FonName(p, f));                          }
static inline void           Cba_FonSetName( Cba_Ntk_t * p, int f, int x )   { assert(Cba_NtkHasFonNames(p)); assert(Cba_FonIsReal(f)); assert(Cba_FonName(p, f) == 0); Vec_IntSetEntry(&p->vFonName, f, x); }
static inline void           Cba_FonCleanName( Cba_Ntk_t * p, int f )        { assert(Cba_NtkHasFonNames(p)); assert(Cba_FonIsReal(f)); assert(Cba_FonName(p, f) != 0); Vec_IntSetEntry(&p->vFonName, f, 0); }
static inline void           Cba_FonPatchName( Cba_Ntk_t * p, int f, int x)  { assert(Cba_NtkHasFonNames(p)); assert(Cba_FonIsReal(f)); Cba_FonCleanName(p, f); Cba_FonSetName(p, f, x);                  }
static inline int            Cba_FonIndex( Cba_Ntk_t * p, int f )            { assert(Cba_FonIsReal(f)); return f - Cba_ObjFon0( p, Cba_FonObj(p, f) );                    }
static inline int            Cba_FonNtkId( Cba_Ntk_t * p, int f )            { assert(Cba_FonIsReal(f)); return Cba_ObjNtkId( p, Cba_FonObj(p, f) );                       }
static inline Cba_Ntk_t *    Cba_FonNtk( Cba_Ntk_t * p, int f )              { assert(Cba_FonIsReal(f)); return Cba_ObjNtk( p, Cba_FonObj(p, f) );                         }

static inline int            Cba_ObjFanin( Cba_Ntk_t * p, int i, int k )     { assert(i>0); return Cba_FonObj( p, Cba_ObjFinFon(p, i, k) );                                }
static inline int            Cba_ObjLeft( Cba_Ntk_t * p, int i )             { return Cba_FonLeft( p, Cba_ObjFon0(p, i) );                                                 }
static inline int            Cba_ObjRight( Cba_Ntk_t * p, int i )            { return Cba_FonRight( p, Cba_ObjFon0(p, i) );                                                }
static inline int            Cba_ObjRangeSize( Cba_Ntk_t * p, int i )        { return Cba_FonRangeSize( p, Cba_ObjFon0(p, i) );                                            }
static inline int            Cba_ObjSigned( Cba_Ntk_t * p, int i )           { return Cba_FonSigned( p, Cba_ObjFon0(p, i) );                                               }
static inline int            Cba_ObjSign( Cba_Ntk_t * p, int i )             { return Abc_Var2Lit( Cba_FonRangeSize(p, i), Cba_ObjSigned(p, i) );                          }

////////////////////////////////////////////////////////////////////////
///                          ITERATORS                               ///
////////////////////////////////////////////////////////////////////////

#define Cba_ManForEachNtk( p, pNtk, i )                                   \
    for ( i = 1; (i <= Cba_ManNtkNum(p)) && (((pNtk) = Cba_ManNtk(p, i)), 1); i++ ) 

#define Cba_NtkForEachPi( p, iObj, i )                                    \
    for ( i = 0; (i < Cba_NtkPiNum(p))  && (((iObj) = Cba_NtkPi(p, i)), 1); i++ ) 
#define Cba_NtkForEachPo( p, iObj, i )                                    \
    for ( i = 0; (i < Cba_NtkPoNum(p))  && (((iObj) = Cba_NtkPo(p, i)), 1); i++ ) 
#define Cba_NtkForEachBoxSeq( p, iObj, i )                                \
    for ( i = 0; (i < Cba_NtkBoxSeqNum(p))  && (((iObj) = Cba_NtkBoxSeq(p, i)), 1); i++ ) 
#define Cba_NtkForEachPioOrder( p, iObj, i )                              \
    for ( i = 0; (i < Cba_NtkPioOrderNum(p))  && (((iObj) = Cba_NtkPioOrder(p, i)), 1); i++ ) 

#define Cba_NtkForEachPiFon( p, iObj, iFon, i )                           \
    for ( i = 0; (i < Cba_NtkPiNum(p))  && (((iObj) = Cba_NtkPi(p, i)), 1)  && (((iFon) = Cba_ObjFon0(p, iObj)), 1); i++ ) 
#define Cba_NtkForEachPoDriverFon( p, iObj, iFon, i )                     \
    for ( i = 0; (i < Cba_NtkPoNum(p))  && (((iObj) = Cba_NtkPo(p, i)), 1)  && (((iFon) = Cba_ObjFinFon(p, iObj, 0)), 1); i++ ) 
#define Cba_NtkForEachPoDriver( p, iObj, i )                              \
    for ( i = 0; (i < Cba_NtkPoNum(p))  && (((iObj) = Cba_ObjFanin(p, Cba_NtkPo(p, i), 0)), 1); i++ ) 

#define Cba_NtkForEachObj( p, i )                                         \
    for ( i = 1; i < Vec_StrSize(&p->vObjType); i++ ) if ( !Cba_ObjType(p, i) ) {} else   
#define Cba_NtkForEachObjType( p, Type, i )                               \
    for ( i = 1; i < Vec_StrSize(&p->vObjType)  && (((Type) = Cba_ObjType(p, i)), 1); i++ ) if ( !Type ) {} else
#define Cba_NtkForEachBox( p, i )                                         \
    for ( i = 1; i < Vec_StrSize(&p->vObjType); i++ ) if ( !Cba_ObjIsBox(p, i) ) {} else
#define Cba_NtkForEachBoxComb( p, i )                                     \
    for ( i = 1; i < Vec_StrSize(&p->vObjType); i++ ) if ( !Cba_ObjIsBox(p, i) || Cba_ObjIsSeq(p, i) ) {} else
#define Cba_NtkForEachBoxUser( p, i )                                     \
    for ( i = 1; i < Vec_StrSize(&p->vObjType); i++ ) if ( !Cba_ObjIsBoxUser(p, i) ) {} else
#define Cba_NtkForEachBoxPrim( p, i )                                     \
    for ( i = 1; i < Vec_StrSize(&p->vObjType); i++ ) if ( !Cba_ObjIsBoxPrim(p, i) ) {} else

#define Cba_NtkForEachFon( p, i )                                         \
    for ( i = 1; i < Vec_IntSize(&p->vFonObj); i++ )
#define Cba_NtkForEachFinFon( p, iFon, iFin )                             \
    for ( iFin = 1; iFin < Vec_IntSize(&p->vFinFon) && (((iFon) = Vec_IntEntry(&p->vFinFon, iFin)), 1); iFin++ ) if ( !iFon ) {} else   
#define Cba_NtkForEachFonName( p, NameId, iFon )                          \
    for ( iFon = 1; iFon < Vec_IntSize(&p->vFonName) && (((NameId) = Vec_IntEntry(&p->vFonName, iFon)), 1); iFon++ ) if ( !NameId ) {} else   

#define Cba_ObjForEachFin( p, iObj, iFin, k )                             \
    for ( k = 0, iFin = Cba_ObjFin0(p, iObj); iFin < Cba_ObjFin0(p, iObj+1); iFin++, k++ )
#define Cba_ObjForEachFon( p, iObj, iFon, k )                             \
    for ( k = 0, iFon = Cba_ObjFon0(p, iObj); iFon < Cba_ObjFon0(p, iObj+1); iFon++, k++ )
#define Cba_ObjForEachFinFon( p, iObj, iFin, iFon, k )                    \
    for ( k = 0, iFin = Cba_ObjFin0(p, iObj); iFin < Cba_ObjFin0(p, iObj+1) && ((iFon = Cba_FinFon(p, iFin)), 1); iFin++, k++ )
#define Cba_ObjForEachFinFanin( p, iObj, iFin, iFanin, k )                \
    for ( k = 0, iFin = Cba_ObjFin0(p, iObj); iFin < Cba_ObjFin0(p, iObj+1) && ((iFanin = Cba_FonObj(p, Cba_FinFon(p, iFin))), 1); iFin++, k++ )
#define Cba_ObjForEachFinFaninReal( p, iObj, iFin, iFanin, k )            \
    for ( k = 0, iFin = Cba_ObjFin0(p, iObj); iFin < Cba_ObjFin0(p, iObj+1) && ((iFanin = Cba_FonObj(p, Cba_FinFon(p, iFin))), 1); iFin++, k++ ) if ( !iFanin ) {} else


////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Object APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cba_ObjAlloc( Cba_Ntk_t * p, Cba_ObjType_t Type, int nFins, int nFons )
{
    int i, iObj = Vec_StrSize(&p->vObjType);
    if ( Type == CBA_OBJ_PI )
        Vec_IntPush( &p->vInputs, iObj );
    else if ( Type == CBA_OBJ_PO )
        Vec_IntPush( &p->vOutputs, iObj );
    Vec_StrPush( &p->vObjType, (char)Type );
    // add fins
    for ( i = 0; i < nFins; i++ )
        Vec_IntPush( &p->vFinFon, 0 );
    if ( Vec_IntSize(&p->vObjFin0) )
        Vec_IntPush( &p->vObjFin0, Vec_IntSize(&p->vFinFon) );
    // add fons
    for ( i = 0; i < nFons; i++ )
        Vec_IntPush( &p->vFonObj, iObj );
    if ( Vec_IntSize(&p->vObjFon0) )
        Vec_IntPush( &p->vObjFon0, Vec_IntSize(&p->vFonObj) );
    return iObj;
}
static inline int Cba_ObjDup( Cba_Ntk_t * pNew, Cba_Ntk_t * p, int i )
{
    int iObj = Cba_ObjAlloc( pNew, Cba_ObjType(p, i), Cba_ObjFinNum(p, i), Cba_ObjFonNum(p, i) );
    Cba_ObjSetCopy( p, i, iObj );
    return iObj;
}
static inline void Cba_ObjDelete( Cba_Ntk_t * p, int i )
{
    int k, iFin, iFon;
    Cba_ObjCleanType( p, i );
    Cba_ObjForEachFin( p, i, iFin, k )
        Vec_IntWriteEntry( &p->vFinFon, iFin, 0 );
    Cba_ObjForEachFon( p, i, iFon, k )
        Vec_IntWriteEntry( &p->vFonObj, iFon, 0 );
}

/**Function*************************************************************

  Synopsis    [Network APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Cba_Ntk_t *  Cba_NtkAlloc( Cba_Man_t * p, int NameId, int nIns, int nOuts, int nObjs, int nFins, int nFons )
{    
    Cba_Ntk_t * pNew = ABC_CALLOC( Cba_Ntk_t, 1 );
    assert( nIns >= 0 && nOuts >= 0 && nObjs >= 0 && nFins >= 0 && nFons >= 0 );
    pNew->Id      = Vec_PtrSize(&p->vNtks);   Vec_PtrPush( &p->vNtks, pNew );
    pNew->NameId  = NameId;
    pNew->pDesign = p;
    Vec_IntGrow( &pNew->vInputs,  nIns );
    Vec_IntGrow( &pNew->vOutputs, nOuts );
    Vec_StrGrow( &pNew->vObjType, nObjs+1 );  Vec_StrPush( &pNew->vObjType,  (char)CBA_OBJ_NONE );
    Vec_IntGrow( &pNew->vObjFin0, nObjs+2 );  Vec_IntPush( &pNew->vObjFin0,  0 ); Vec_IntPush( &pNew->vObjFin0, 1 );
    Vec_IntGrow( &pNew->vObjFon0, nObjs+2 );  Vec_IntPush( &pNew->vObjFon0,  0 ); Vec_IntPush( &pNew->vObjFon0, 1 );
    Vec_IntGrow( &pNew->vFinFon,  nFins+1 );  Vec_IntPush( &pNew->vFinFon,   0 );
    Vec_IntGrow( &pNew->vFonObj,  nFons+1 );  Vec_IntPush( &pNew->vFonObj,   0 );
    return pNew;
}
static inline void Cba_NtkFree( Cba_Ntk_t * p )
{
    // interface
    Vec_IntErase( &p->vInputs );
    Vec_IntErase( &p->vOutputs );
    Vec_IntErase( &p->vOrder );
    Vec_IntErase( &p->vSeq );
    // stucture
    Vec_StrErase( &p->vObjType );
    Vec_IntErase( &p->vObjFin0 );    
    Vec_IntErase( &p->vObjFon0 );
    Vec_IntErase( &p->vFinFon );    
    Vec_IntErase( &p->vFonObj );    
    // optional
    Vec_IntErase( &p->vObjCopy );    
    Vec_IntErase( &p->vObjFunc );    
    Vec_IntErase( &p->vObjName );    
    Vec_IntErase( &p->vObjAttr );    
    Vec_IntErase( &p->vAttrSto );    
    Vec_IntErase( &p->vFonCopy );    
    Vec_IntErase( &p->vFonName );    
    Vec_IntErase( &p->vFonRange );    
    Vec_IntErase( &p->vFonPrev );    
    Vec_IntErase( &p->vFonNext );    
    Vec_IntErase( &p->vFinFon0 );    
    Vec_IntErase( &p->vFinObj );    
    Vec_IntErase( &p->vNtkObjs );    
    Vec_IntErase( &p->vFonBits );    
    // other
    Vec_IntErase( &p->vArray0 );    
    Vec_IntErase( &p->vArray1 );    
    if ( p->vOther ) Vec_VecFree( (Vec_Vec_t *)p->vOther );
    ABC_FREE( p );
}
static inline int Cba_NtkNewStrId( Cba_Ntk_t * pNtk, const char * format, ...  )
{
    Abc_Nam_t * p = Cba_NtkNam( pNtk );
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
static inline int Cba_ManNewConstId( Cba_Ntk_t * p, Vec_Str_t * vBits ) 
{ 
    Vec_Str_t * vOut = Abc_NamBuffer(Cba_NtkNam(p)); 
    Vec_StrPrintF( vOut, "%d\'b%s", Vec_StrSize(vBits)-1, Vec_StrArray(vBits) );
    return Abc_NamStrFindOrAdd(p->pDesign->pFuns, Vec_StrArray(vOut), NULL);
}
static inline int Cba_ManNewConstZero( Cba_Ntk_t * p, int nBits ) 
{ 
    Vec_Str_t * vOut = Abc_NamBuffer(Cba_NtkNam(p)); 
    Vec_StrPrintF( vOut, "%d\'b%0s", nBits, "" );
    return Abc_NamStrFindOrAdd(p->pDesign->pFuns, Vec_StrArray(vOut), NULL);
}
static inline void Cba_NtkAdd( Cba_Man_t * p, Cba_Ntk_t * pNtk )
{    
    int fFound, NtkId = Abc_NamStrFindOrAdd( p->pMods, Cba_NtkName(pNtk), &fFound );
    if ( fFound )
        printf( "Network with name \"%s\" already exists.\n", Cba_NtkName(pNtk) );
    else
        assert( NtkId == pNtk->Id );
}
static inline void Cba_NtkUpdate( Cba_Man_t * p, Cba_Ntk_t * pNtk )
{
    int fFound, NtkId = Abc_NamStrFindOrAdd( p->pMods, Cba_NtkName(pNtk), &fFound );
    if ( !fFound )
        printf( "Network with name \"%s\" does not exist.\n", Cba_NtkName(pNtk) );
    else
    {
        Cba_NtkFree( Cba_ManNtk(p, NtkId) );
        Vec_PtrWriteEntry( &p->vNtks, NtkId, pNtk );
    }
}
static inline Vec_Int_t * Cba_NtkCollect( Cba_Ntk_t * p )
{
    int iObj;
    Vec_Int_t * vObjs = Vec_IntAlloc( Cba_NtkObjNum(p) );
    Cba_NtkForEachObj( p, iObj )
        Vec_IntPush( vObjs, iObj );
    return vObjs;
}
static inline int Cba_NtkIsSeq( Cba_Ntk_t * p )
{
    int iObj;
    if ( p->fSeq )
        return 1;
    if ( p->fComb )
        return 0;
    assert( !p->fSeq && !p->fComb );
    Cba_NtkForEachBox( p, iObj )
        if ( Cba_ObjIsBoxUser(p, iObj) )
        {
            if ( Cba_NtkIsSeq( Cba_ObjNtk(p, iObj) ) )
            {
                p->fSeq = 1;
                return 1;
            }
        }
        else if ( Cba_ObjIsSeq(p, iObj) )
        {
            p->fSeq = 1;
            return 1;
        }
    p->fComb = 1;
    return 0;
}
static inline void Cba_NtkPrepareSeq( Cba_Ntk_t * p )
{
    int iObj;
    assert( Cba_NtkBoxSeqNum(p) == 0 );
    if ( !Cba_NtkIsSeq(p) )
        return;
    Cba_NtkForEachBox( p, iObj )
        if ( Cba_ObjIsSeq(p, iObj) )
            Vec_IntPush( &p->vSeq, iObj );
    // Cba_NtkObjOrder( p, &p->vSeq, NULL );
}
static inline void Cba_NtkCountParams( Cba_Ntk_t * p, Vec_Int_t * vObjs, int * nPis, int * nPos, int * nFins, int * nFons )
{
    int i, iObj;
    *nPis = *nPos = *nFins = *nFons = 0;
    Vec_IntForEachEntry( vObjs, iObj, i )
    {
        *nPis += Cba_ObjIsPi(p, iObj);
        *nPos += Cba_ObjIsPo(p, iObj);
        *nFins += Cba_ObjFinNum(p, iObj);
        *nFons += Cba_ObjFonNum(p, iObj);
    }
}
static inline Cba_Ntk_t * Cba_NtkDup( Cba_Man_t * pMan, Cba_Ntk_t * p, Vec_Int_t * vObjs )
{
    Cba_Ntk_t * pNew;
    int i, k, iObj, iObjNew, iFin, iFon;
    int nPis, nPos, nFins, nFons;
    Cba_NtkCountParams( p, vObjs, &nPis, &nPos, &nFins, &nFons );
    pNew = Cba_NtkAlloc( pMan, Cba_NtkNameId(p), nPis, nPos, Vec_IntSize(vObjs), nFins, nFons );
    Cba_NtkCleanObjCopies( p );
    Cba_NtkCleanFonCopies( p );
    Vec_IntForEachEntry( vObjs, iObj, i )
    {
        iObjNew = Cba_ObjDup( pNew, p, iObj );
        Cba_ObjForEachFon( p, iObj, iFon, k )
            Cba_FonSetCopy( p, iFon, Cba_ObjFon(pNew, iObjNew, k) );
    }
    Vec_IntForEachEntry( vObjs, iObj, i )
    {
        iObjNew = Cba_ObjCopy( p, iObj );
        Cba_ObjForEachFinFon( p, iObj, iFin, iFon, k )
            Cba_ObjSetFinFon( pNew, iObjNew, k, Cba_FonCopy(p, iFon) );
    }
    //Cba_NtkFreeObjCopies( p );
    //Cba_NtkFreeFonCopies( p );
    assert( Cba_NtkObjNum(pNew) == Cba_NtkObjNumAlloc(pNew) );
    Cba_NtkSetCopy( p, Cba_NtkId(pNew) );
    return pNew;
}
static inline Cba_Ntk_t * Cba_NtkDupOrder( Cba_Man_t * pMan, Cba_Ntk_t * p, Vec_Int_t*(* pFuncOrder)(Cba_Ntk_t*) )
{
    Cba_Ntk_t * pNew;
    Vec_Int_t * vObjs = pFuncOrder ? pFuncOrder(p) : Cba_NtkCollect(p);
    if ( vObjs == NULL )
        return NULL;
    pNew = Cba_NtkDup( pMan, p, vObjs );
    Vec_IntFree( vObjs );
    //Cba_NtkPrepareSeq( pNew );
    return pNew;
}
static inline void Cba_NtkDupAttrs( Cba_Ntk_t * pNew, Cba_Ntk_t * p )
{
    int i, iObj;
    assert( Vec_IntSize(&pNew->vOrder) == 0 );
    Cba_NtkForEachPioOrder( p, iObj, i )
        Vec_IntPush( &pNew->vOrder, Cba_ObjCopy(p, iObj) );
//    Vec_IntRemapArray( &p->vObjCopy, &p->vOrder,    &pNew->vOrder,    Cba_NtkPioOrderNum(p) );
//    Vec_IntRemapArray( &p->vObjCopy, &p->vSeq,      &pNew->vSeq,      Cba_NtkBoxSeqNum(p) );
    // transfer object attributes
    Vec_IntRemapArray( &p->vObjCopy, &p->vObjFunc,  &pNew->vObjFunc,  Cba_NtkObjNum(pNew) + 1 );
    Vec_IntRemapArray( &p->vObjCopy, &p->vObjName,  &pNew->vObjName,  Cba_NtkObjNum(pNew) + 1 );
    Vec_IntRemapArray( &p->vObjCopy, &p->vObjAttr,  &pNew->vObjAttr,  Cba_NtkObjNum(pNew) + 1 );
    // transfer fon attributes
    Vec_IntRemapArray( &p->vFonCopy, &p->vFonName,  &pNew->vFonName,  Cba_NtkFonNum(pNew) + 1 );
    Vec_IntRemapArray( &p->vFonCopy, &p->vFonRange, &pNew->vFonRange, Cba_NtkFonNum(pNew) + 1 );
    // duplicate attributes
    Vec_IntAppend( &pNew->vAttrSto, &p->vAttrSto );
    pNew->vOther = p->vOther ? (Vec_Ptr_t *)Vec_VecDup( (Vec_Vec_t *)p->vOther ) : NULL;
}
static inline int Cba_NtkMemory( Cba_Ntk_t * p )
{
    int nMem = sizeof(Cba_Ntk_t);
    // interface
    nMem += (int)Vec_IntMemory(&p->vInputs);
    nMem += (int)Vec_IntMemory(&p->vOutputs);
    nMem += (int)Vec_IntMemory(&p->vOrder);
    nMem += (int)Vec_IntMemory(&p->vSeq);
    // stucture
    nMem += (int)Vec_StrMemory(&p->vObjType);
    nMem += (int)Vec_IntMemory(&p->vObjFin0);
    nMem += (int)Vec_IntMemory(&p->vObjFon0);
    nMem += (int)Vec_IntMemory(&p->vFinFon);
    nMem += (int)Vec_IntMemory(&p->vFonObj);
    // optional
    nMem += (int)Vec_IntMemory(&p->vObjCopy );  
    nMem += (int)Vec_IntMemory(&p->vObjFunc );  
    nMem += (int)Vec_IntMemory(&p->vObjName );  
    nMem += (int)Vec_IntMemory(&p->vObjAttr );  
    nMem += (int)Vec_IntMemory(&p->vAttrSto );  
    nMem += (int)Vec_IntMemory(&p->vFonCopy );  
    nMem += (int)Vec_IntMemory(&p->vFonName );  
    nMem += (int)Vec_IntMemory(&p->vFonRange ); 
    nMem += (int)Vec_IntMemory(&p->vFonPrev );  
    nMem += (int)Vec_IntMemory(&p->vFonNext );  
    nMem += (int)Vec_IntMemory(&p->vFinFon0 );  
    nMem += (int)Vec_IntMemory(&p->vFinObj );   
    nMem += (int)Vec_IntMemory(&p->vNtkObjs );  
    nMem += (int)Vec_IntMemory(&p->vFonBits );  
    // other
    nMem += (int)Vec_IntMemory(&p->vArray1 );
    nMem += (int)Vec_IntMemory(&p->vArray1 );
    return nMem;
}
static inline void Cba_NtkPrintStats( Cba_Ntk_t * p )
{
    printf( "pi =%5d  ",   Cba_NtkPiNum(p) );
    printf( "po =%5d  ",   Cba_NtkPoNum(p) );
    printf( "seq =%5d  ",  Cba_NtkBoxSeqNum(p) );
    printf( "box =%5d  ",  Cba_NtkBoxNum(p) );
    printf( "prim =%5d  ", Cba_NtkBoxPrimNum(p) );
    printf( "user =%5d  ", Cba_NtkBoxUserNum(p) );
    //printf( "topo =%4s  ", Cba_NtkIsTopoOrder(p) ? "yes" : "no" );
    printf( "  %s ",       Cba_NtkName(p) );
    if ( Vec_IntSize(&p->vNtkObjs) )
        printf( "-> %s",   Cba_NtkName(Cba_NtkNtk(p, Vec_IntEntry(&p->vNtkObjs, 0))) );
    printf( "\n" );
//    Vec_StrIntPrint( &p->vObjType );
}
static inline void Cba_NtkPrint( Cba_Ntk_t * p )
{
    int i, Type;
    printf( "Interface (%d):\n", Cba_NtkPioNum(p) );
    printf( "Objects (%d):\n", Cba_NtkObjNum(p) );
    Cba_NtkForEachObjType( p, Type, i )
    {
        printf( "%6d : ", i );
        printf( "Type =%3d  ", Type );
        printf( "Fins = %d  ", Cba_ObjFinNum(p, i) );
        printf( "Fons = %d  ", Cba_ObjFonNum(p, i) );
        if ( Cba_NtkHasObjNames(p) && Cba_ObjName(p, i) )
            printf( "%s", Cba_ObjNameStr(p, i) );
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Manager APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Cba_Man_t * Cba_ManAlloc( char * pFileName, int nNtks, Abc_Nam_t * pStrs, Abc_Nam_t * pFuns, Abc_Nam_t * pMods, Hash_IntMan_t * vHash )
{
    Cba_Man_t * pNew = ABC_CALLOC( Cba_Man_t, 1 );
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
static inline void Cba_ManDupTypeNames( Cba_Man_t * pNew, Cba_Man_t * p )
{
   memcpy( pNew->pTypeNames, p->pTypeNames, sizeof(void *) * CBA_BOX_LAST );
}
static inline Cba_Man_t * Cba_ManDup( Cba_Man_t * p, Vec_Int_t*(* pFuncOrder)(Cba_Ntk_t*) )
{
    Cba_Ntk_t * pNtk, * pNtkNew; int i;
    Cba_Man_t * pNew = Cba_ManAlloc( p->pSpec, Cba_ManNtkNum(p), Abc_NamRef(p->pStrs), Abc_NamRef(p->pFuns), Abc_NamStart(100, 24), Hash_IntManRef(p->vHash) );
    Cba_ManDupTypeNames( pNew, p );
    Cba_ManForEachNtk( p, pNtk, i )
    {
        pNtkNew = Cba_NtkDupOrder( pNew, pNtk, pFuncOrder );
        Cba_NtkAdd( pNew, pNtkNew );
        Cba_NtkDupAttrs( pNtkNew, pNtk );
    }
//    Cba_ManForEachNtk( p, pNtk, i )
//        if ( (pHost = Cba_NtkHostNtk(pNtk)) )
//            Cba_NtkSetHost( Cba_NtkCopyNtk(pNew, pNtk), Cba_NtkCopy(pHost), Cba_ObjCopy(pHost, Cba_NtkHostObj(pNtk)) );
    pNew->iRoot = Cba_ManNtkNum(pNew);
    return pNew;
}
static inline void Cba_ManPrepareSeq( Cba_Man_t * p )
{
    Cba_Ntk_t * pNtk; int i;
    Cba_ManForEachNtk( p, pNtk, i )
        Cba_NtkPrepareSeq( pNtk );
}
static inline void Cba_ManFree( Cba_Man_t * p )
{
    Cba_Ntk_t * pNtk; int i;
    Cba_ManForEachNtk( p, pNtk, i )
        Cba_NtkFree( pNtk );
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
static inline int Cba_ManGetMap( Cba_Man_t * p, int i )
{
    return Vec_IntGetEntry(&p->vNameMap, i);
}
static inline void Cba_ManSetMap( Cba_Man_t * p, int i, int x )
{
    assert( Vec_IntGetEntry(&p->vNameMap, i) == 0 );
    Vec_IntSetEntry( &p->vNameMap, i, x );
    Vec_IntPush( &p->vUsed, i );
}
static inline void Cba_ManUnsetMap( Cba_Man_t * p, int i )
{
    Vec_IntSetEntry( &p->vNameMap, i, 0 );
}
static inline void Cba_ManCleanMap( Cba_Man_t * p )
{
    int i, Entry;
    Vec_IntForEachEntry( &p->vUsed, Entry, i )
        Vec_IntWriteEntry( &p->vNameMap, Entry, 0 );
    Vec_IntClear( &p->vUsed );
}
static inline int  Cba_NtkGetMap( Cba_Ntk_t * p, int i )          { return Cba_ManGetMap(p->pDesign, i); }
static inline void Cba_NtkSetMap( Cba_Ntk_t * p, int i, int x )   { Cba_ManSetMap(p->pDesign, i, x);     }
static inline void Cba_NtkUnsetMap( Cba_Ntk_t * p, int i )        { Cba_ManUnsetMap(p->pDesign, i);      }
static inline void Cba_NtkCleanMap( Cba_Ntk_t * p )               { Cba_ManCleanMap(p->pDesign);         }

static inline int Cba_ManGetMap2( Cba_Man_t * p, int i )
{
    return Vec_IntGetEntry(&p->vNameMap2, i);
}
static inline void Cba_ManSetMap2( Cba_Man_t * p, int i, int x )
{
    assert( Vec_IntGetEntry(&p->vNameMap2, i) == 0 );
    Vec_IntSetEntry( &p->vNameMap2, i, x );
    Vec_IntPush( &p->vUsed2, i );
}
static inline void Cba_ManUnsetMap2( Cba_Man_t * p, int i )
{
    Vec_IntSetEntry( &p->vNameMap2, i, 0 );
}
static inline void Cba_ManCleanMap2( Cba_Man_t * p )
{
    int i, Entry;
    Vec_IntForEachEntry( &p->vUsed2, Entry, i )
        Vec_IntWriteEntry( &p->vNameMap2, Entry, 0 );
    Vec_IntClear( &p->vUsed2 );
}
static inline int  Cba_NtkGetMap2( Cba_Ntk_t * p, int i )          { return Cba_ManGetMap2(p->pDesign, i); }
static inline void Cba_NtkSetMap2( Cba_Ntk_t * p, int i, int x )   { Cba_ManSetMap2(p->pDesign, i, x);     }
static inline void Cba_NtkUnsetMap2( Cba_Ntk_t * p, int i )        { Cba_ManUnsetMap2(p->pDesign, i);      }
static inline void Cba_NtkCleanMap2( Cba_Ntk_t * p )               { Cba_ManCleanMap2(p->pDesign);         }

static inline int Cba_ManMemory( Cba_Man_t * p )
{
    Cba_Ntk_t * pNtk; int i;
    int nMem = sizeof(Cba_Man_t);
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
    Cba_ManForEachNtk( p, pNtk, i )
        nMem += Cba_NtkMemory( pNtk );
    return nMem;
}
static inline int Cba_ManObjNum( Cba_Man_t * p )
{
    Cba_Ntk_t * pNtk; int i, Count = 0;
    Cba_ManForEachNtk( p, pNtk, i )
        Count += Cba_NtkObjNum(pNtk);
    return Count;
}
static inline int Cba_ManBoxNum( Cba_Man_t * p )
{
    Cba_Ntk_t * pNtk; int i, Count = 0;
    Cba_ManForEachNtk( p, pNtk, i )
        Count += Cba_NtkBoxNum( pNtk );
    return Count;
}
static inline void Cba_ManBoxNumRec_rec( Cba_Ntk_t * p, int * pCountP, int * pCountU )
{
    int iObj, Id = Cba_NtkId(p);
    if ( pCountP[Id] >= 0 )
        return;
    pCountP[Id] = pCountU[Id] = 0;
    Cba_NtkForEachObj( p, iObj )
    {
        if ( Cba_ObjIsBoxUser(p, iObj) )
        {
            Cba_ManBoxNumRec_rec( Cba_ObjNtk(p, iObj), pCountP, pCountU );
            pCountP[Id] += pCountP[Cba_ObjNtkId(p, iObj)];
            pCountU[Id] += pCountU[Cba_ObjNtkId(p, iObj)] + 1;
        }
        else
            pCountP[Id] += 1;
    }
}
static inline void Cba_ManBoxNumRec( Cba_Man_t * p, int * pnPrims, int * pnUsers )
{
    Cba_Ntk_t * pNtk = Cba_ManRoot(p);
    int * pCountP = ABC_FALLOC( int, Cba_ManNtkNum(p) + 1 );
    int * pCountU = ABC_FALLOC( int, Cba_ManNtkNum(p) + 1 );
    Cba_ManBoxNumRec_rec( pNtk, pCountP, pCountU );
    *pnPrims = pCountP[Cba_NtkId(pNtk)];
    *pnUsers = pCountU[Cba_NtkId(pNtk)];
    ABC_FREE( pCountP );
    ABC_FREE( pCountU );
}
static inline void Cba_ManPrintStats( Cba_Man_t * p, int nModules, int fVerbose )
{
    Cba_Ntk_t * pNtk; int i, nPrims, nUsers;
    Cba_Ntk_t * pRoot = Cba_ManRoot( p );
    Cba_ManBoxNumRec( p, &nPrims, &nUsers );
    printf( "%-12s : ",     Cba_ManName(p) );
    printf( "pi =%5d  ",    Cba_NtkPiNum(pRoot) );
    printf( "po =%5d  ",    Cba_NtkPoNum(pRoot) );
    printf( "mod =%5d  ",   Cba_ManNtkNum(p) );
    printf( "box =%5d  ",   nPrims + nUsers );
    printf( "prim =%5d  ",  nPrims );
    printf( "user =%5d  ",  nUsers );
    printf( "mem =%6.3f MB", 1.0*Cba_ManMemory(p)/(1<<20) );
    printf( "\n" );
    Cba_ManForEachNtk( p, pNtk, i )
    {
        if ( i == nModules+1 )
            break;
        printf( "Module %5d : ", i );
        Cba_NtkPrintStats( pNtk );
    }
}


/**Function*************************************************************

  Synopsis    [Name handling.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Cba_NtkAddMissingFonNames( Cba_Ntk_t * p, char * pPref )
{
    int iFon, NameId, Index;
    // populate map
    Cba_ManCleanMap( p->pDesign );
    Vec_IntForEachEntryStart( &p->vFonName, NameId, iFon, 1 )
        if ( NameId )
            Cba_ManSetMap( p->pDesign, NameId, iFon );
    // check remaining ones
    Vec_IntForEachEntryStart( &p->vFonName, NameId, iFon, 1 )
    {
        if ( NameId )
            continue;
        NameId = Cba_NtkNewStrId(p, "%s%d", pPref, iFon);
        for ( Index = 1; Cba_ManGetMap(p->pDesign, NameId); Index++ )
            NameId = Cba_NtkNewStrId(p, "%s%d_%d", pPref, iFon, Index);
        Cba_FonSetName( p, iFon, NameId );
        Cba_ManSetMap( p->pDesign, NameId, iFon );
    }
}
static inline void Cba_NtkCreateFonNames( Cba_Ntk_t * p, char * pPref )
{
    int i, iObj, iFon;//, NameId;
    Cba_NtkCleanFonNames( p );
    Cba_NtkForEachPiFon( p, iObj, iFon, i )
        if ( !Cba_FonName(p, iFon) )
            Cba_FonSetName( p, iFon, Cba_ObjName(p, iObj) );
    Cba_NtkForEachPoDriverFon( p, iObj, iFon, i )
        if ( Cba_FonIsReal(iFon) && !Cba_FonName(p, iFon) )
            Cba_FonSetName( p, iFon, Cba_ObjName(p, iObj) );
//    Vec_IntForEachEntryStart( &p->vFonName, NameId, iFon, 1 )
//        if ( NameId == 0 )
//            Vec_IntWriteEntry( &p->vFonName, iFon, Cba_NtkNewStrId(p, "%s%d", pPref, iFon) );
    Cba_NtkAddMissingFonNames( p, pPref );
}
static inline void Cba_NtkMissingFonNames( Cba_Ntk_t * p, char * pPref )
{
    int i, iObj, iFon;//, NameId;
    Cba_NtkForEachPiFon( p, iObj, iFon, i )
        if ( !Cba_FonName(p, iFon) )
            Cba_FonSetName( p, iFon, Cba_ObjName(p, iObj) );
    Cba_NtkForEachPoDriverFon( p, iObj, iFon, i )
        if ( Cba_FonIsReal(iFon) && !Cba_FonName(p, iFon) )
            Cba_FonSetName( p, iFon, Cba_ObjName(p, iObj) );
//    Vec_IntForEachEntryStart( &p->vFonName, NameId, iFon, 1 )
//        if ( NameId == 0 )
//            Cba_FonSetName( p, iFon, Cba_NtkNewStrId(p, "%s%d", pPref, iFon) );
    Cba_NtkAddMissingFonNames( p, pPref );
}


/*=== cbaBlast.c =============================================================*/
extern Gia_Man_t *   Cba_ManBlast( Cba_Man_t * p, int fBarBufs, int fSeq, int fVerbose );
extern Cba_Man_t *   Cba_ManInsertGia( Cba_Man_t * p, Gia_Man_t * pGia );
extern Cba_Man_t *   Cba_ManInsertAbc( Cba_Man_t * p, void * pAbc );
/*=== cbaCba.c ===============================================================*/
extern Cba_Man_t *   Cba_ManReadCba( char * pFileName );
extern void          Cba_ManWriteCba( char * pFileName, Cba_Man_t * p );
/*=== cbaCom.c ===============================================================*/
/*=== cbaNtk.c ===============================================================*/
extern void          Cba_NtkPrintStatsFull( Cba_Ntk_t * p, int fDistrib, int fVerbose );
extern void          Cba_NtkPrintNodes( Cba_Ntk_t * p, int Type );
extern void          Cba_NtkPrintDistribOld( Cba_Ntk_t * p );
extern void          Cba_ManPrintDistrib( Cba_Man_t * p );
//extern void           Cba_ManPrepareTypeNames( Cba_Man_t * p );
extern void          Cba_NtkObjOrder( Cba_Ntk_t * p, Vec_Int_t * vObjs, Vec_Int_t * vNameIds );
extern int           Cba_NtkCiFonNum( Cba_Ntk_t * p );
extern int           Cba_NtkCoFinNum( Cba_Ntk_t * p );
extern int           Cba_NtkCheckComboLoop( Cba_Ntk_t * p );
extern int           Cba_ManIsTopoOrder( Cba_Man_t * p );
extern Vec_Int_t *   Cba_NtkCollectDfs( Cba_Ntk_t * p );
extern Cba_Man_t *   Cba_ManCollapse( Cba_Man_t * p );
extern Cba_Man_t *   Cba_ManExtractGroup( Cba_Man_t * p, Vec_Int_t * vObjs );
extern Cba_Man_t *   Cba_ManDeriveFromGia( Cba_Man_t * p, Gia_Man_t * pGia, int fUseXor );
extern Cba_Man_t *   Cba_ManInsertGroup( Cba_Man_t * p, Vec_Int_t * vObjs, Cba_Ntk_t * pSyn );
/*=== cbaReadBlif.c ==========================================================*/
extern Cba_Man_t *   Prs_ManBuildCbaBlif( char * pFileName, Vec_Ptr_t * vDes );
extern void          Prs_ManReadBlifTest( char * pFileName );
extern Cba_Man_t *   Cba_ManReadBlif( char * pFileName );
/*=== cbaReadVer.c ===========================================================*/
extern Cba_Man_t *   Prs_ManBuildCbaVerilog( char * pFileName, Vec_Ptr_t * vDes );
extern void          Prs_ManReadVerilogTest( char * pFileName );
extern Cba_Man_t *   Cba_ManReadVerilog( char * pFileName );
/*=== cbaWriteBlif.c =========================================================*/
extern void          Prs_ManWriteBlif( char * pFileName, Vec_Ptr_t * p );
extern void          Cba_ManWriteBlif( char * pFileName, Cba_Man_t * p );
/*=== cbaWriteVer.c ==========================================================*/
extern void          Cba_ManCreatePrimMap( char ** pMap );
extern char *        Cba_ManGetSliceName( Cba_Ntk_t * p, int iFon, int RangeId );
extern void          Prs_ManWriteVerilog( char * pFileName, Vec_Ptr_t * p );
extern void          Cba_ManWriteVerilog( char * pFileName, Cba_Man_t * p, int fInlineConcat );

ABC_NAMESPACE_HEADER_END


#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

