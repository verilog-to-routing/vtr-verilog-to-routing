/**CFile****************************************************************

  FileName    [bac.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchical word-level netlist.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 29, 2014.]

  Revision    [$Id: bac.h,v 1.00 2014/11/29 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__base__bac__bac_h
#define ABC__base__bac__bac_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "aig/gia/gia.h"
#include "misc/extra/extra.h"
#include "misc/util/utilNam.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START 

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

// network objects
typedef enum { 
    BAC_OBJ_NONE = 0,  // 0:  unused
    BAC_OBJ_PI,        // 1:  input
    BAC_OBJ_PO,        // 2:  output
    BAC_OBJ_BI,        // 3:  box input
    BAC_OBJ_BO,        // 4:  box output
    BAC_OBJ_BOX,       // 5:  box

    BAC_BOX_CF,   
    BAC_BOX_CT,   
    BAC_BOX_CX,   
    BAC_BOX_CZ,   
    BAC_BOX_BUF,  
    BAC_BOX_INV,  
    BAC_BOX_AND,  
    BAC_BOX_NAND, 
    BAC_BOX_OR,   
    BAC_BOX_NOR,  
    BAC_BOX_XOR,  
    BAC_BOX_XNOR, 
    BAC_BOX_SHARP,
    BAC_BOX_SHARPL,
    BAC_BOX_MUX,  
    BAC_BOX_MAJ,  

    BAC_BOX_RAND,
    BAC_BOX_RNAND,
    BAC_BOX_ROR,
    BAC_BOX_RNOR,
    BAC_BOX_RXOR,
    BAC_BOX_RXNOR,

    BAC_BOX_LAND,
    BAC_BOX_LNAND,
    BAC_BOX_LOR,
    BAC_BOX_LNOR,
    BAC_BOX_LXOR,
    BAC_BOX_LXNOR,

    BAC_BOX_NMUX,  
    BAC_BOX_SEL,
    BAC_BOX_PSEL,
    BAC_BOX_ENC,
    BAC_BOX_PENC,
    BAC_BOX_DEC,
    BAC_BOX_EDEC,

    BAC_BOX_ADD,
    BAC_BOX_SUB,
    BAC_BOX_MUL,
    BAC_BOX_DIV,
    BAC_BOX_MOD,
    BAC_BOX_REM,
    BAC_BOX_POW,
    BAC_BOX_MIN,
    BAC_BOX_ABS,

    BAC_BOX_LTHAN,
    BAC_BOX_LETHAN,
    BAC_BOX_METHAN,
    BAC_BOX_MTHAN,
    BAC_BOX_EQU,
    BAC_BOX_NEQU,

    BAC_BOX_SHIL,
    BAC_BOX_SHIR,
    BAC_BOX_ROTL,
    BAC_BOX_ROTR,

    BAC_BOX_GATE,  
    BAC_BOX_LUT,  
    BAC_BOX_ASSIGN,  

    BAC_BOX_TRI,
    BAC_BOX_RAM,
    BAC_BOX_RAMR,
    BAC_BOX_RAMW,
    BAC_BOX_RAMWC,
    BAC_BOX_RAMBOX,

    BAC_BOX_LATCH,
    BAC_BOX_LATCHRS,
    BAC_BOX_DFF,
    BAC_BOX_DFFRS,

    BAC_BOX_UNKNOWN   // 67
} Bac_ObjType_t; 


// name types
typedef enum { 
    BAC_NAME_BIN = 0,        // 0:  binary variable
    BAC_NAME_WORD,           // 1:  first bit of word-level variable
    BAC_NAME_INFO,           // 2:  first bit of special variable
    BAC_NAME_INDEX,          // 3:  index of word-level variable
} Bac_NameType_t; 


typedef struct Bac_Ntk_t_ Bac_Ntk_t;
typedef struct Bac_Man_t_ Bac_Man_t;

// network
struct Bac_Ntk_t_
{
    Bac_Man_t *  pDesign;  // design
    int          NameId;   // name ID
    int          iCopy;    // copy module
    int          iBoxNtk;  // instance network ID
    int          iBoxObj;  // instance object ID
    int          Count;    // object counter
    int          Mark;     // visit mark 
    // interface
    Vec_Int_t    vInputs;  // inputs 
    Vec_Int_t    vOutputs; // outputs
    Vec_Int_t    vInfo;    // input/output/wire info
    // object attributes
    Vec_Str_t    vType;    // types     
    Vec_Int_t    vFanin;   // fanin
    Vec_Int_t    vIndex;   // index
    Vec_Int_t    vName;    // original NameId or InstId
    Vec_Int_t    vFanout;  // fanout
    Vec_Int_t    vCopy;    // copy
    // other
    Vec_Int_t    vArray;
    Vec_Int_t    vArray2;
};

// design
struct Bac_Man_t_
{
    // design names
    char *       pName;    // design name
    char *       pSpec;    // spec file name
    Abc_Nam_t *  pStrs;    // string manager
    Abc_Nam_t *  pMods;    // module name manager
    // internal data
    int          iRoot;    // root network
    int          nNtks;    // number of current networks
    Bac_Ntk_t *  pNtks;    // networks
    // user data
    Vec_Str_t *  vOut;     
    Vec_Str_t *  vOut2;     
    Vec_Int_t    vBuf2RootNtk;
    Vec_Int_t    vBuf2RootObj;
    Vec_Int_t    vBuf2LeafNtk;
    Vec_Int_t    vBuf2LeafObj;
    void *       pMioLib;
    void **      ppGraphs;
    int          ElemGates[4];
    char *       pPrimNames[BAC_BOX_UNKNOWN];
    char *       pPrimSymbs[BAC_BOX_UNKNOWN];
};

static inline char *         Bac_ManName( Bac_Man_t * p )                    { return p->pName;                                                                            }
static inline char *         Bac_ManSpec( Bac_Man_t * p )                    { return p->pSpec;                                                                            }
static inline int            Bac_ManNtkNum( Bac_Man_t * p )                  { return p->nNtks;                                                                            }
static inline int            Bac_ManPrimNum( Bac_Man_t * p )                 { return Abc_NamObjNumMax(p->pMods) - Bac_ManNtkNum(p);                                       }
static inline int            Bac_ManNtkIsOk( Bac_Man_t * p, int i )          { return i > 0 && i <= Bac_ManNtkNum(p);                                                      }
static inline Bac_Ntk_t *    Bac_ManNtk( Bac_Man_t * p, int i )              { return Bac_ManNtkIsOk(p, i) ? p->pNtks + i : NULL;                                          }
static inline int            Bac_ManNtkFindId( Bac_Man_t * p, char * pName ) { return Abc_NamStrFind(p->pMods, pName);                                                     }
static inline Bac_Ntk_t *    Bac_ManNtkFind( Bac_Man_t * p, char * pName )   { return Bac_ManNtk( p, Bac_ManNtkFindId(p, pName) );                                         }
static inline Bac_Ntk_t *    Bac_ManRoot( Bac_Man_t * p )                    { return Bac_ManNtk(p, p->iRoot);                                                             }
static inline char *         Bac_ManStr( Bac_Man_t * p, int i )              { return Abc_NamStr(p->pStrs, i);                                                             }
static inline int            Bac_ManStrId( Bac_Man_t * p, char * pStr )      { return Abc_NamStrFind(p->pStrs, pStr);                                                      }
static inline char *         Bac_ManPrimName( Bac_Man_t * p, Bac_ObjType_t Type ) { return p->pPrimNames[Type];                                                            }
static inline char *         Bac_ManPrimSymb( Bac_Man_t * p, Bac_ObjType_t Type ) { return p->pPrimSymbs[Type];                                                            }

static inline int            Bac_NtkId( Bac_Ntk_t * p )                      { int i = p - p->pDesign->pNtks; assert(Bac_ManNtkIsOk(p->pDesign, i)); return i;             }
static inline Bac_Man_t *    Bac_NtkMan( Bac_Ntk_t * p )                     { return p->pDesign;                                                                          }
static inline int            Bac_NtkNameId( Bac_Ntk_t * p )                  { return p->NameId;                                                                           }
static inline char *         Bac_NtkName( Bac_Ntk_t * p )                    { return Bac_ManStr(p->pDesign, Bac_NtkNameId(p));                                            }
static inline int            Bac_NtkCopy( Bac_Ntk_t * p )                    { return p->iCopy;                                                                            }
static inline Bac_Ntk_t *    Bac_NtkCopyNtk(Bac_Man_t * pNew, Bac_Ntk_t * p) { return Bac_ManNtk(pNew, Bac_NtkCopy(p));                                                    }
static inline void           Bac_NtkSetCopy( Bac_Ntk_t * p, int i )          { assert(p->iCopy == -1); p->iCopy = i;                                                       }

static inline int            Bac_NtkObjNum( Bac_Ntk_t * p )                  { return Vec_StrSize(&p->vType);                                                              }
static inline int            Bac_NtkObjNumAlloc( Bac_Ntk_t * p )             { return Vec_StrCap(&p->vType);                                                               }
static inline int            Bac_NtkPiNum( Bac_Ntk_t * p )                   { return Vec_IntSize(&p->vInputs);                                                            }
static inline int            Bac_NtkPoNum( Bac_Ntk_t * p )                   { return Vec_IntSize(&p->vOutputs);                                                           }
static inline int            Bac_NtkPioNum( Bac_Ntk_t * p )                  { return Bac_NtkPiNum(p) + Bac_NtkPoNum(p);                                                   }
static inline int            Bac_NtkPiNumAlloc( Bac_Ntk_t * p )              { return Vec_IntCap(&p->vInputs);                                                             }
static inline int            Bac_NtkPoNumAlloc( Bac_Ntk_t * p )              { return Vec_IntCap(&p->vOutputs);                                                            }
static inline int            Bac_NtkBiNum( Bac_Ntk_t * p )                   { return Vec_StrCountEntryLit(&p->vType, (char)BAC_OBJ_BI);                                   }
static inline int            Bac_NtkBoNum( Bac_Ntk_t * p )                   { return Vec_StrCountEntryLit(&p->vType, (char)BAC_OBJ_BO);                                   }
static inline int            Bac_NtkCiNum( Bac_Ntk_t * p )                   { return Bac_NtkPiNum(p) + Bac_NtkBoNum(p);                                                   }
static inline int            Bac_NtkCoNum( Bac_Ntk_t * p )                   { return Bac_NtkPoNum(p) + Bac_NtkBiNum(p);                                                   }
static inline int            Bac_NtkBoxNum( Bac_Ntk_t * p )                  { return Bac_NtkObjNum(p) - Vec_StrCountSmallerLit(&p->vType, (char)BAC_OBJ_BOX);             }
static inline int            Bac_NtkPrimNum( Bac_Ntk_t * p )                 { return Vec_StrCountLargerLit(&p->vType, (char)BAC_OBJ_BOX);                                 }
static inline int            Bac_NtkUserNum( Bac_Ntk_t * p )                 { return Vec_StrCountEntryLit(&p->vType, (char)BAC_OBJ_BOX);                                  }
 
static inline int            Bac_NtkPi( Bac_Ntk_t * p, int i )               { return Vec_IntEntry(&p->vInputs, i);                                                        }
static inline int            Bac_NtkPo( Bac_Ntk_t * p, int i )               { return Vec_IntEntry(&p->vOutputs, i);                                                       }
static inline char *         Bac_NtkStr( Bac_Ntk_t * p, int i )              { return Bac_ManStr(p->pDesign, i);                                                           }
static inline Bac_Ntk_t *    Bac_NtkHostNtk( Bac_Ntk_t * p )                 { return p->iBoxNtk > 0 ? Bac_ManNtk(p->pDesign, p->iBoxNtk) : NULL;                          }
static inline int            Bac_NtkHostObj( Bac_Ntk_t * p )                 { return p->iBoxObj;                                                                          }
static inline void           Bac_NtkSetHost( Bac_Ntk_t * p, int n, int i )   { assert(p->iBoxNtk == -1); p->iBoxNtk = n; p->iBoxObj = i;                                   }

static inline int            Bac_InfoRange( int Beg, int End )               { return End > Beg ? End - Beg + 1 : Beg - End + 1;                                           }
static inline int            Bac_NtkInfoNum( Bac_Ntk_t * p )                 { return Vec_IntSize(&p->vInfo)/3;                                                            }
static inline int            Bac_NtkInfoNumAlloc( Bac_Ntk_t * p )            { return Vec_IntCap(&p->vInfo)/3;                                                             }
static inline int            Bac_NtkInfoType( Bac_Ntk_t * p, int i )         { return Abc_Lit2Att2(Vec_IntEntry(&p->vInfo, 3*i));                                          }
static inline int            Bac_NtkInfoName( Bac_Ntk_t * p, int i )         { return Abc_Lit2Var2(Vec_IntEntry(&p->vInfo, 3*i));                                          }
static inline int            Bac_NtkInfoBeg( Bac_Ntk_t * p, int i )          { return Vec_IntEntry(&p->vInfo, 3*i+1);                                                      }
static inline int            Bac_NtkInfoEnd( Bac_Ntk_t * p, int i )          { return Vec_IntEntry(&p->vInfo, 3*i+2);                                                      }
static inline int            Bac_NtkInfoRange( Bac_Ntk_t * p, int i )        { int* a = Vec_IntEntryP(&p->vInfo, 3*i); return a[1]>=0 ? Bac_InfoRange( a[1], a[2] ) : 1;   }
static inline int            Bac_NtkInfoIndex( Bac_Ntk_t * p, int i, int j ) { int* a = Vec_IntEntryP(&p->vInfo, 3*i); assert(a[1]>=0); return a[1]<a[2] ? a[1]+j : a[1]-j;}
static inline void           Bac_NtkAddInfo( Bac_Ntk_t * p,int i,int b,int e){ Vec_IntPush(&p->vInfo, i); Vec_IntPushTwo(&p->vInfo, b, e);                                 }
static inline void           Bac_NtkSetInfoName( Bac_Ntk_t * p, int i, int n){ Vec_IntWriteEntry( &p->vInfo, 3*i, n );                                                     }

static inline void           Bac_NtkStartNames( Bac_Ntk_t * p )              { assert(Bac_NtkObjNumAlloc(p)); Vec_IntFill(&p->vName,   Bac_NtkObjNumAlloc(p),  0);         }
static inline void           Bac_NtkStartFanouts( Bac_Ntk_t * p )            { assert(Bac_NtkObjNumAlloc(p)); Vec_IntFill(&p->vFanout, Bac_NtkObjNumAlloc(p),  0);         }
static inline void           Bac_NtkStartCopies( Bac_Ntk_t * p )             { assert(Bac_NtkObjNumAlloc(p)); Vec_IntFill(&p->vCopy,   Bac_NtkObjNumAlloc(p), -1);         }
static inline void           Bac_NtkFreeNames( Bac_Ntk_t * p )               { Vec_IntErase(&p->vName);                                                                    }
static inline void           Bac_NtkFreeFanouts( Bac_Ntk_t * p )             { Vec_IntErase(&p->vFanout);                                                                  }
static inline void           Bac_NtkFreeCopies( Bac_Ntk_t * p )              { Vec_IntErase(&p->vCopy);                                                                    }
static inline int            Bac_NtkHasNames( Bac_Ntk_t * p )                { return p->vName.pArray != NULL;                                                             }
static inline int            Bac_NtkHasFanouts( Bac_Ntk_t * p )              { return p->vFanout.pArray != NULL;                                                           }
static inline int            Bac_NtkHasCopies( Bac_Ntk_t * p )               { return p->vCopy.pArray != NULL;                                                             }

static inline int            Bac_TypeIsBox( Bac_ObjType_t Type )             { return Type >= BAC_OBJ_BOX && Type < BAC_BOX_UNKNOWN;                                       }
static inline Bac_NameType_t Bac_NameType( int n )                           { assert( n ); return (Bac_NameType_t)Abc_Lit2Att2( n );                                      }
static inline int            Bac_CharIsDigit( char c )                       { return c >= '0' && c <= '9'; }

static inline Bac_ObjType_t  Bac_ObjType( Bac_Ntk_t * p, int i )             { return (Bac_ObjType_t)Abc_Lit2Var((int)(unsigned char)Vec_StrEntry(&p->vType, i));          }
static inline int            Bac_ObjIsPi( Bac_Ntk_t * p, int i )             { return Bac_ObjType(p, i) == BAC_OBJ_PI;                                                     }
static inline int            Bac_ObjIsPo( Bac_Ntk_t * p, int i )             { return Bac_ObjType(p, i) == BAC_OBJ_PO;                                                     }
static inline int            Bac_ObjIsPio( Bac_Ntk_t * p, int i )            { return Bac_ObjIsPi(p, i) || Bac_ObjIsPo(p, i);                                              }
static inline int            Bac_ObjIsBi( Bac_Ntk_t * p, int i )             { return Bac_ObjType(p, i) == BAC_OBJ_BI;                                                     }
static inline int            Bac_ObjIsBo( Bac_Ntk_t * p, int i )             { return Bac_ObjType(p, i) == BAC_OBJ_BO;                                                     }
static inline int            Bac_ObjIsBio( Bac_Ntk_t * p, int i )            { return Bac_ObjIsBi(p, i) || Bac_ObjIsBo(p, i);                                              }
static inline int            Bac_ObjIsBox( Bac_Ntk_t * p, int i )            { return Bac_TypeIsBox(Bac_ObjType(p, i));                                                    }
static inline int            Bac_ObjIsBoxUser( Bac_Ntk_t * p, int i )        { return Bac_ObjType(p, i) == BAC_OBJ_BOX;                                                    }
static inline int            Bac_ObjIsBoxPrim( Bac_Ntk_t * p, int i )        { return Bac_ObjIsBox(p, i) && !Bac_ObjIsBoxUser(p, i);                                       }
static inline int            Bac_ObjIsGate( Bac_Ntk_t * p, int i )           { return Bac_ObjType(p, i) == BAC_BOX_GATE;                                                   }
static inline int            Bac_ObjIsCi( Bac_Ntk_t * p, int i )             { return Bac_ObjIsPi(p, i) || Bac_ObjIsBo(p, i);                                              }
static inline int            Bac_ObjIsCo( Bac_Ntk_t * p, int i )             { return Bac_ObjIsPo(p, i) || Bac_ObjIsBi(p, i);                                              }
static inline int            Bac_ObjIsCio( Bac_Ntk_t * p, int i )            { return Bac_ObjType(p, i) < BAC_OBJ_BOX;                                                     }
static inline int            Bac_ObjIsConst( Bac_Ntk_t * p, int i )          { return Bac_ObjType(p, i) >= BAC_BOX_CF && Bac_ObjType(p, i) <= BAC_BOX_CZ;                  }
static inline int            Bac_ObjIsConstBin( Bac_Ntk_t * p, int i )       { return Bac_ObjType(p, i) == BAC_BOX_CF || Bac_ObjType(p, i) == BAC_BOX_CT;                  }

static inline int            Bac_ObjBit( Bac_Ntk_t * p, int i )              { assert(!Bac_ObjIsBox(p, i)); return Abc_LitIsCompl((int)Vec_StrEntry(&p->vType, i));        }
static inline void           Bac_ObjSetBit( Bac_Ntk_t * p, int i )           { char *q = Vec_StrArray(&p->vType); assert(!Bac_ObjIsBox(p, i)); q[i] = (char)Abc_LitNot((int)q[i]); }
static inline int            Bac_ObjFanin( Bac_Ntk_t * p, int i )            { assert(Bac_ObjIsCo(p, i)); return Vec_IntEntry(&p->vFanin, i);                              }
static inline int            Bac_ObjIndex( Bac_Ntk_t * p, int i )            { assert(Bac_ObjIsCio(p, i)); return Vec_IntEntry(&p->vIndex, i);                             }
static inline int            Bac_ObjNameInt( Bac_Ntk_t * p, int i )          { assert(!Bac_ObjIsCo(p, i)); return Vec_IntEntry(&p->vName, i);                              }
static inline int            Bac_ObjName( Bac_Ntk_t * p, int i )             { return Bac_ObjIsCo(p, i) ? Bac_ObjNameInt(p, Bac_ObjFanin(p,i)) : Bac_ObjNameInt(p, i);     }
static inline Bac_NameType_t Bac_ObjNameType( Bac_Ntk_t * p, int i )         { return Bac_NameType( Bac_ObjName(p, i) );                                                   }
static inline int            Bac_ObjNameId( Bac_Ntk_t * p, int i )           { return Abc_Lit2Var2( Bac_ObjName(p, i) );                                                   }
static inline char *         Bac_ObjNameStr( Bac_Ntk_t * p, int i )          { assert(Bac_ObjNameType(p, i) <= BAC_NAME_WORD); return Bac_NtkStr(p, Bac_ObjNameId(p, i));  }
static inline int            Bac_ObjCopy( Bac_Ntk_t * p, int i )             { return Vec_IntEntry(&p->vCopy, i);                                                          }
static inline int            Bac_ObjFanout( Bac_Ntk_t * p, int i )           { assert(Bac_ObjIsCi(p, i)); return Vec_IntEntry(&p->vFanout, i);                             }
static inline int            Bac_ObjNextFanout( Bac_Ntk_t * p, int i )       { assert(Bac_ObjIsCo(p, i)); return Vec_IntEntry(&p->vFanout, i);                             }
static inline void           Bac_ObjSetFanout( Bac_Ntk_t * p, int i, int x ) { assert(Bac_ObjIsCi(p, i)); Vec_IntSetEntry(&p->vFanout, i, x);                              }
static inline void           Bac_ObjSetNextFanout( Bac_Ntk_t * p,int i,int x){ assert(Bac_ObjIsCo(p, i)); Vec_IntSetEntry(&p->vFanout, i, x);                              }
static inline void           Bac_ObjCleanFanin( Bac_Ntk_t * p, int i )       { assert(Bac_ObjFanin(p, i) >= 0 && Bac_ObjIsCo(p, i)); Vec_IntSetEntry( &p->vFanin, i, -1);  }
static inline void           Bac_ObjSetFanin( Bac_Ntk_t * p, int i, int x )  { assert(Bac_ObjFanin(p, i) == -1 && Bac_ObjIsCo(p, i)); Vec_IntSetEntry( &p->vFanin, i, x);  }
static inline void           Bac_ObjSetIndex( Bac_Ntk_t * p, int i, int x )  { assert(Bac_ObjIndex(p, i) == -1); Vec_IntSetEntry( &p->vIndex, i, x );                      }
static inline void           Bac_ObjSetName( Bac_Ntk_t * p, int i, int x )   { assert(Bac_ObjName(p, i) == 0 && !Bac_ObjIsCo(p, i)); Vec_IntSetEntry( &p->vName, i, x );   }
static inline void           Bac_ObjSetCopy( Bac_Ntk_t * p, int i, int x )   { assert(Bac_ObjCopy(p, i) == -1);  Vec_IntSetEntry( &p->vCopy,  i, x );                      }
static inline int            Bac_ObjGetConst( Bac_Ntk_t * p, int i )         { assert(Bac_ObjIsCi(p, i)); return Bac_ObjIsBo(p, i) && Bac_ObjIsConst(p, i-1) ? Bac_ObjType(p, i-1) : 0;              }

static inline int            Bac_BoxBiNum( Bac_Ntk_t * p, int i )            { int s = i-1; assert(Bac_ObjIsBox(p, i)); while (--i >= 0               && Bac_ObjIsBi(p, i)) {} return s - i;  }
static inline int            Bac_BoxBoNum( Bac_Ntk_t * p, int i )            { int s = i+1; assert(Bac_ObjIsBox(p, i)); while (++i < Bac_NtkObjNum(p) && Bac_ObjIsBo(p, i)) {} return i - s;  }
static inline int            Bac_BoxSize( Bac_Ntk_t * p, int i )             { return 1 + Bac_BoxBiNum(p, i) + Bac_BoxBoNum(p, i);                                         }
static inline int            Bac_BoxBi( Bac_Ntk_t * p, int b, int i )        { assert(Bac_ObjIsBox(p, b)); return b - 1 - i;                                               }
static inline int            Bac_BoxBo( Bac_Ntk_t * p, int b, int i )        { assert(Bac_ObjIsBox(p, b)); return b + 1 + i;                                               }
//static inline int            Bac_BoxBiBox( Bac_Ntk_t * p, int i )            { assert(Bac_ObjIsBi(p, i)); return i + 1 + Bac_ObjIndex(p, i);                               }
static inline int            Bac_BoxBoBox( Bac_Ntk_t * p, int i )            { assert(Bac_ObjIsBo(p, i)); return i - 1 - Bac_ObjIndex(p, i);                               }
static inline int            Bac_BoxFanin( Bac_Ntk_t * p, int b, int i )     { return Bac_ObjFanin(p, Bac_BoxBi(p, b, i));                                                 }
static inline int            Bac_BoxFaninBox( Bac_Ntk_t * p, int b, int i )  { return Bac_BoxBoBox(p, Bac_BoxFanin(p, b, i));                                              }
static inline int            Bac_BoxBiRange( Bac_Ntk_t * p, int i )          { int s = i; assert(Bac_ObjIsBi(p, i) && !Bac_ObjBit(p, i)); while (--i >= 0               && Bac_ObjIsBi(p, i) && Bac_ObjBit(p, i)) {} return s - i;  }
static inline int            Bac_BoxBoRange( Bac_Ntk_t * p, int i )          { int s = i; assert(Bac_ObjIsBo(p, i) && !Bac_ObjBit(p, i)); while (++i < Bac_NtkObjNum(p) && Bac_ObjIsBo(p, i) && Bac_ObjBit(p, i)) {} return i - s;  }
static inline int            Bac_ObjPiRange( Bac_Ntk_t * p, int i )          { int s = i; assert(Bac_ObjIsPi(p, i) && !Bac_ObjBit(p, i)); while (++i < Bac_NtkObjNum(p) && Bac_ObjIsPi(p, i) && Bac_ObjBit(p, i)) {} return i - s;  }

static inline int            Bac_BoxNtkId( Bac_Ntk_t * p, int i )            { assert(Bac_ObjIsBox(p, i)); return Vec_IntEntry(&p->vFanin, i);                             }
static inline void           Bac_BoxSetNtkId( Bac_Ntk_t * p, int i, int x )  { assert(Bac_ObjIsBox(p, i)&&Bac_ManNtkIsOk(p->pDesign, x));Vec_IntSetEntry(&p->vFanin, i, x);}
//static inline int            Bac_BoxBiNtkId( Bac_Ntk_t * p, int i )          { assert(Bac_ObjIsBi(p, i)); return Bac_BoxNtkId(p, Bac_BoxBiBox(p, i));                      }
static inline int            Bac_BoxBoNtkId( Bac_Ntk_t * p, int i )          { assert(Bac_ObjIsBo(p, i)); return Bac_BoxNtkId(p, Bac_BoxBoBox(p, i));                      }
static inline Bac_Ntk_t *    Bac_BoxNtk( Bac_Ntk_t * p, int i )              { return Bac_ManNtk( p->pDesign, Bac_BoxNtkId(p, i) );                                        }
//static inline Bac_Ntk_t *    Bac_BoxBiNtk( Bac_Ntk_t * p, int i )            { return Bac_ManNtk( p->pDesign, Bac_BoxBiNtkId(p, i) );                                      }
static inline Bac_Ntk_t *    Bac_BoxBoNtk( Bac_Ntk_t * p, int i )            { return Bac_ManNtk( p->pDesign, Bac_BoxBoNtkId(p, i) );                                      }
static inline char *         Bac_BoxNtkName( Bac_Ntk_t * p, int i )          { return Abc_NamStr( p->pDesign->pMods, Bac_BoxNtkId(p, i) );                                 }

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                             ITERATORS                            ///
////////////////////////////////////////////////////////////////////////

#define Bac_ManForEachNtk( p, pNtk, i )                                   \
    for ( i = 1; (i <= Bac_ManNtkNum(p)) && (((pNtk) = Bac_ManNtk(p, i)), 1); i++ ) 

#define Bac_NtkForEachPi( p, iObj, i )                                    \
    for ( i = 0; (i < Bac_NtkPiNum(p))  && (((iObj) = Bac_NtkPi(p, i)), 1); i++ ) 
#define Bac_NtkForEachPo( p, iObj, i )                                    \
    for ( i = 0; (i < Bac_NtkPoNum(p))  && (((iObj) = Bac_NtkPo(p, i)), 1); i++ ) 
#define Bac_NtkForEachPoDriver( p, iObj, i )                              \
    for ( i = 0; (i < Bac_NtkPoNum(p))  && (((iObj) = Bac_ObjFanin(p, Bac_NtkPo(p, i))), 1); i++ ) 

#define Bac_NtkForEachPiMain( p, iObj, i )                                \
    for ( i = 0; (i < Bac_NtkPiNum(p))  && (((iObj) = Bac_NtkPi(p, i)), 1); i++ )  if ( Bac_ObjBit(p, iObj) ) {} else
#define Bac_NtkForEachPoMain( p, iObj, i )                                \
    for ( i = 0; (i < Bac_NtkPoNum(p))  && (((iObj) = Bac_NtkPo(p, i)), 1); i++ )  if ( Bac_ObjBit(p, iObj) ) {} else

#define Bac_NtkForEachObj( p, i )  if ( !Bac_ObjType(p, i) ) {} else      \
    for ( i = 0; (i < Bac_NtkObjNum(p)); i++ ) 
#define Bac_NtkForEachObjType( p, Type, i )                               \
    for ( i = 0; (i < Bac_NtkObjNum(p))  && (((Type) = Bac_ObjType(p, i)), 1); i++ ) if ( !Type ) {} else

#define Bac_NtkForEachBox( p, i )                                         \
    for ( i = 0; (i < Bac_NtkObjNum(p)); i++ ) if ( !Bac_ObjIsBox(p, i) ) {} else
#define Bac_NtkForEachBoxUser( p, i )                                     \
    for ( i = 0; (i < Bac_NtkObjNum(p)); i++ ) if ( !Bac_ObjIsBoxUser(p, i) ) {} else
#define Bac_NtkForEachBoxPrim( p, i )                                     \
    for ( i = 0; (i < Bac_NtkObjNum(p)); i++ ) if ( !Bac_ObjIsBoxPrim(p, i) ) {} else

#define Bac_NtkForEachCi( p, i )                                          \
    for ( i = 0; (i < Bac_NtkObjNum(p)); i++ ) if ( !Bac_ObjIsCi(p, i) ) {} else
#define Bac_NtkForEachCo( p, i )                                          \
    for ( i = 0; (i < Bac_NtkObjNum(p)); i++ ) if ( !Bac_ObjIsCo(p, i) ) {} else
#define Bac_NtkForEachCio( p, i )                                         \
    for ( i = 0; (i < Bac_NtkObjNum(p)); i++ ) if ( !Bac_ObjIsCio(p, i) ){} else

#define Bac_NtkForEachBi( p, i )                                          \
    for ( i = 0; (i < Bac_NtkObjNum(p)); i++ ) if ( !Bac_ObjIsBi(p, i) ){} else
#define Bac_NtkForEachBo( p, i )                                          \
    for ( i = 0; (i < Bac_NtkObjNum(p)); i++ ) if ( !Bac_ObjIsBo(p, i) ){} else
#define Bac_NtkForEachBio( p, i )                                         \
    for ( i = 0; (i < Bac_NtkObjNum(p)); i++ ) if ( !Bac_ObjIsBio(p, i) ){} else

#define Bac_BoxForEachBi( p, iBox, iTerm, i )                             \
    for ( iTerm = iBox - 1, i = 0; iTerm >= 0 && Bac_ObjIsBi(p, iTerm); iTerm--, i++ )
#define Bac_BoxForEachBo( p, iBox, iTerm, i )                             \
    for ( iTerm = iBox + 1, i = 0; iTerm < Bac_NtkObjNum(p) && Bac_ObjIsBo(p, iTerm); iTerm++, i++ )
#define Bac_BoxForEachBiReverse( p, iBox, iTerm, i )                      \
    for ( i = Bac_BoxBiNum(p, iBox), iTerm = iBox - i--; Bac_ObjIsBi(p, iTerm); iTerm++, i-- )

#define Bac_BoxForEachBiMain( p, iBox, iTerm, i )                         \
    for ( iTerm = iBox - 1, i = 0; iTerm >= 0 && Bac_ObjIsBi(p, iTerm); iTerm--, i++ )                if ( Bac_ObjBit(p, iTerm) ) {} else
#define Bac_BoxForEachBoMain( p, iBox, iTerm, i )                         \
    for ( iTerm = iBox + 1, i = 0; iTerm < Bac_NtkObjNum(p) && Bac_ObjIsBo(p, iTerm); iTerm++, i++ )  if ( Bac_ObjBit(p, iTerm) ) {} else

#define Bac_BoxForEachFanin( p, iBox, iFanin, i )                         \
    for ( i = 0; iBox - 1 - i >= 0 && Bac_ObjIsBi(p, iBox - 1 - i) && (((iFanin) = Bac_BoxFanin(p, iBox, i)), 1); i++ )
#define Bac_BoxForEachFaninBox( p, iBox, iFanin, i )                      \
    for ( i = 0; iBox - 1 - i >= 0 && Bac_ObjIsBi(p, iBox - 1 - i) && (((iFanin) = Bac_BoxFaninBox(p, iBox, i)), 1); i++ )

#define Bac_ObjForEachFanout( p, iCi, iCo )                               \
    for ( iCo = Bac_ObjFanout(p, iCi); iCo; iCo = Bac_ObjNextFanout(p, iCo) )
#define Bac_BoxForEachFanoutBox( p, iBox, iCo, iFanBox )                  \
    for ( assert(Bac_BoxBoNum(p, iBox) == 1), iCo = Bac_ObjFanout(p, Bac_BoxBo(p, iBox, 0)); iCo && ((iFanBox = Bac_BoxBiBox(p, iCo)), 1); iCo = Bac_ObjNextFanout(p, iCo) )

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Object APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Bac_ObjAlloc( Bac_Ntk_t * p, Bac_ObjType_t Type, int Fanin )
{
    int iObj = Bac_NtkObjNum(p);
    assert( iObj == Vec_IntSize(&p->vFanin) );
    if ( Type == BAC_OBJ_PI )
        Vec_IntPush( &p->vInputs, iObj );
    else if ( Type == BAC_OBJ_PO )
        Vec_IntPush( &p->vOutputs, iObj );
    Vec_StrPush( &p->vType, (char)Abc_Var2Lit(Type, 0) );
    Vec_IntPush( &p->vFanin, Fanin );
    return iObj;
}
static inline int Bac_ObjDup( Bac_Ntk_t * pNew, Bac_Ntk_t * p, int i )
{
    int iObj = Bac_ObjAlloc( pNew, Bac_ObjType(p, i), Bac_ObjIsBox(p, i) ? Bac_BoxNtkId(p, i) : -1 );
    if ( Bac_NtkHasNames(p) && Bac_NtkHasNames(pNew) && !Bac_ObjIsCo(p, i) ) 
        Bac_ObjSetName( pNew, iObj, Bac_ObjName(p, i) );
    Bac_ObjSetCopy( p, i, iObj );
    return iObj;
}
static inline int Bac_BoxAlloc( Bac_Ntk_t * p, Bac_ObjType_t Type, int nIns, int nOuts, int iNtk )
{
    int i, iObj;
    for ( i = nIns - 1; i >= 0; i-- )
        Bac_ObjAlloc( p, BAC_OBJ_BI, -1 );
    iObj = Bac_ObjAlloc( p, Type, iNtk );
    for ( i = 0; i < nOuts; i++ )
        Bac_ObjAlloc( p, BAC_OBJ_BO, -1 );
    return iObj;
}
static inline int Bac_BoxDup( Bac_Ntk_t * pNew, Bac_Ntk_t * p, int iBox )
{
    int i, iTerm, iBoxNew;
    Bac_BoxForEachBiReverse( p, iBox, iTerm, i )
        Bac_ObjDup( pNew, p, iTerm );
    iBoxNew = Bac_ObjDup( pNew, p, iBox );
    if ( Bac_NtkHasNames(p) && Bac_NtkHasNames(pNew) && Bac_ObjName(p, iBox) ) 
        Bac_ObjSetName( pNew, iBoxNew, Bac_ObjName(p, iBox) );
    if ( Bac_BoxNtk(p, iBox) )
        Bac_BoxSetNtkId( pNew, iBoxNew, Bac_NtkCopy(Bac_BoxNtk(p, iBox)) );
    Bac_BoxForEachBo( p, iBox, iTerm, i )
        Bac_ObjDup( pNew, p, iTerm );
    return iBoxNew;
}
static inline void Bac_BoxDelete( Bac_Ntk_t * p, int iBox )
{
    int iStart = iBox - Bac_BoxBiNum(p, iBox);
    int i, iStop = iBox + Bac_BoxBoNum(p, iBox);
    for ( i = iStart; i <= iStop; i++ )
    {
        Vec_StrWriteEntry( &p->vType,  i, (char)0 );
        Vec_IntWriteEntry( &p->vFanin, i, -1 );
        if ( Bac_NtkHasNames(p) )
            Vec_IntWriteEntry( &p->vName, i, 0 );
        if ( Bac_NtkHasFanouts(p) )
            Vec_IntWriteEntry( &p->vFanout, i, 0 );
    }
}
static inline void Bac_BoxReplace( Bac_Ntk_t * p, int iBox, int * pArray, int nSize )
{
    extern void Bac_NtkUpdateFanout( Bac_Ntk_t * p, int iOld, int iNew );
    int i, Limit = Bac_BoxBoNum(p, iBox);
    assert( Limit == nSize );
    for ( i = 0; i < Limit; i++ )
        Bac_NtkUpdateFanout( p, Bac_BoxBo(p, iBox, i), pArray[i] );
}


static inline Vec_Int_t * Bac_BoxCollectRanges( Bac_Ntk_t * p, int iBox )
{
    static Vec_Int_t Bits, * vBits = &Bits;
    static int pArray[10]; int i, iTerm;
    assert( !Bac_ObjIsBoxUser(p, iBox) );
    // initialize array
    vBits->pArray = pArray;
    vBits->nSize = 0;
    vBits->nCap = 10;
    // iterate through inputs
    Bac_BoxForEachBiMain( p, iBox, iTerm, i )
        Vec_IntPush( vBits, Bac_BoxBiRange(p, iTerm) );
    // iterate through outputs
    Bac_BoxForEachBoMain( p, iBox, iTerm, i )
        Vec_IntPush( vBits, Bac_BoxBoRange(p, iTerm) );
    assert( Vec_IntSize(vBits) < 10 );
    //Vec_IntPrint( vBits );
    return vBits;
}

/**Function*************************************************************

  Synopsis    [Prints vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_StrPrint( Vec_Str_t * p, int fInt )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( fInt )
            printf( "%d ", (int)p->pArray[i] );
        else
            printf( "%c ", p->pArray[i] );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Network APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Bac_NtkAlloc( Bac_Ntk_t * pNew, int NameId, int nIns, int nOuts, int nObjs )
{
    int NtkId, fFound;
    assert( pNew->pDesign != NULL );
    assert( Bac_NtkPiNum(pNew) == 0 );
    assert( Bac_NtkPoNum(pNew) == 0 );
    pNew->NameId  = NameId;
    pNew->iCopy   = -1;
    pNew->iBoxNtk = -1;
    pNew->iBoxObj = -1;
    Vec_IntGrow( &pNew->vInputs,  nIns );
    Vec_IntGrow( &pNew->vOutputs, nOuts );
    Vec_StrGrow( &pNew->vType,    nObjs );
    Vec_IntGrow( &pNew->vFanin,   nObjs );
    // check if the network is unique
    NtkId = Abc_NamStrFindOrAdd( pNew->pDesign->pMods, Bac_NtkStr(pNew, NameId), &fFound );
    if ( fFound )
        printf( "Network with name %s already exists.\n", Bac_NtkStr(pNew, NameId) );
    else
        assert( NtkId == Bac_NtkId(pNew) );
}
static inline void Bac_NtkDup( Bac_Ntk_t * pNew, Bac_Ntk_t * p )
{
    int i, iObj;
    assert( pNew != p );
    Bac_NtkAlloc( pNew, Bac_NtkNameId(p), Bac_NtkPiNum(p), Bac_NtkPoNum(p), Bac_NtkObjNum(p) );
    if ( Vec_IntSize(&p->vInfo) )
        Vec_IntAppend( &pNew->vInfo, &p->vInfo );
    Bac_NtkStartCopies( p );
    if ( Bac_NtkHasNames(p) )
        Bac_NtkStartNames( pNew );
    Bac_NtkForEachPi( p, iObj, i )
        Bac_ObjDup( pNew, p, iObj );
    Bac_NtkForEachBox( p, iObj )
        Bac_BoxDup( pNew, p, iObj );
    Bac_NtkForEachPo( p, iObj, i )
        Bac_ObjDup( pNew, p, iObj );
    Bac_NtkForEachCo( p, iObj )
        Bac_ObjSetFanin( pNew, Bac_ObjCopy(p, iObj), Bac_ObjCopy(p, Bac_ObjFanin(p, iObj)) );
    //Bac_NtkFreeCopies( p ); // needed for name transfer and host ntk
    assert( Bac_NtkObjNum(pNew) == Bac_NtkObjNumAlloc(pNew) );
}
static inline void Bac_NtkDupUserBoxes( Bac_Ntk_t * pNew, Bac_Ntk_t * p )
{
    int i, iObj;
    assert( pNew != p );
    Bac_NtkAlloc( pNew, Bac_NtkNameId(p), Bac_NtkPiNum(p), Bac_NtkPoNum(p), Bac_NtkObjNum(p) + 3*Bac_NtkCoNum(p) );
    if ( Vec_IntSize(&p->vInfo) )
        Vec_IntAppend( &pNew->vInfo, &p->vInfo );
    Bac_NtkStartCopies( p );
    Bac_NtkForEachPi( p, iObj, i )
        Bac_ObjDup( pNew, p, iObj );
    Bac_NtkForEachPo( p, iObj, i )
        Bac_ObjDup( pNew, p, iObj );
    Bac_NtkForEachBoxUser( p, iObj )
        Bac_BoxDup( pNew, p, iObj );
    // connect feed-throughs
    Bac_NtkForEachCo( p, iObj )
        if ( Bac_ObjCopy(p, iObj) >= 0 && Bac_ObjCopy(p, Bac_ObjFanin(p, iObj)) >= 0 )
            Bac_ObjSetFanin( pNew, Bac_ObjCopy(p, iObj), Bac_ObjCopy(p, Bac_ObjFanin(p, iObj)) );
}
static inline void Bac_NtkMoveNames( Bac_Ntk_t * pNew, Bac_Ntk_t * p )
{
    int i, iBox, iObj;
    assert( Bac_NtkHasNames(p) );
    assert( !Bac_NtkHasNames(pNew) );
    Bac_NtkStartNames( pNew );
    Bac_NtkForEachPi( p, iObj, i )
        Bac_ObjSetName( pNew, Bac_ObjCopy(p, iObj), Bac_ObjName(p, iObj) );
    Bac_NtkForEachBoxUser( p, iBox )
    {
        Bac_ObjSetName( pNew, Bac_ObjCopy(p, iBox), Bac_ObjName(p, iBox) );
        Bac_BoxForEachBo( p, iBox, iObj, i )
            Bac_ObjSetName( pNew, Bac_ObjCopy(p, iObj), Bac_ObjName(p, iObj) );
    }
    Bac_NtkForEachBoxUser( p, iBox )
        Bac_BoxForEachBi( p, iBox, iObj, i )
            if ( !Bac_ObjName(pNew, Bac_ObjFanin(pNew, Bac_ObjCopy(p, iObj))) )
                Bac_ObjSetName( pNew, Bac_ObjFanin(pNew, Bac_ObjCopy(p, iObj)), Bac_ObjName(p, iObj) );
    Bac_NtkForEachPo( p, iObj, i )
        if ( !Bac_ObjName(pNew, Bac_ObjFanin(pNew, Bac_ObjCopy(p, iObj))) )
            Bac_ObjSetName( pNew, Bac_ObjFanin(pNew, Bac_ObjCopy(p, iObj)), Bac_ObjName(p, iObj) );
}

static inline void Bac_NtkFree( Bac_Ntk_t * p )
{
    Vec_IntErase( &p->vInputs );
    Vec_IntErase( &p->vOutputs );
    Vec_IntErase( &p->vInfo );
    Vec_StrErase( &p->vType );
    Vec_IntErase( &p->vFanin );    
    Vec_IntErase( &p->vIndex );
    Vec_IntErase( &p->vName );    
    Vec_IntErase( &p->vFanout );    
    Vec_IntErase( &p->vCopy );    
    Vec_IntErase( &p->vArray );    
    Vec_IntErase( &p->vArray2 );    
}
static inline int Bac_NtkMemory( Bac_Ntk_t * p )
{
    int nMem = sizeof(Bac_Ntk_t);
    nMem += (int)Vec_IntMemory(&p->vInputs);
    nMem += (int)Vec_IntMemory(&p->vOutputs);
    nMem += (int)Vec_IntMemory(&p->vInfo);
    nMem += (int)Vec_StrMemory(&p->vType);
    nMem += (int)Vec_IntMemory(&p->vFanin);
    nMem += (int)Vec_IntMemory(&p->vIndex);
    nMem += (int)Vec_IntMemory(&p->vName);
    nMem += (int)Vec_IntMemory(&p->vFanout);
    nMem += (int)Vec_IntMemory(&p->vCopy);
    return nMem;
}
static inline void Bac_NtkPrintStats( Bac_Ntk_t * p )
{
    printf( "pi =%5d  ",   Bac_NtkPiNum(p) );
    printf( "pi =%5d  ",   Bac_NtkPoNum(p) );
    printf( "box =%6d  ",  Bac_NtkBoxNum(p) );
    printf( "clp =%7d  ",  p->Count );
    printf( "obj =%7d  ",  Bac_NtkObjNum(p) );
    printf( "%s ",         Bac_NtkName(p) );
    if ( Bac_NtkHostNtk(p) )
        printf( "-> %s",   Bac_NtkName(Bac_NtkHostNtk(p)) );
    printf( "\n" );
}
static inline void Bac_NtkDeriveIndex( Bac_Ntk_t * p )
{
    int i, iObj, iTerm;
    Vec_IntFill( &p->vIndex, Bac_NtkObjNum(p), -1 );
    Bac_NtkForEachPi( p, iObj, i )
        Bac_ObjSetIndex( p, iObj, i );
    Bac_NtkForEachPo( p, iObj, i )
        Bac_ObjSetIndex( p, iObj, i );
    Bac_NtkForEachBox( p, iObj )
    {
        Bac_BoxForEachBi( p, iObj, iTerm, i )
            Bac_ObjSetIndex( p, iTerm, i );
        Bac_BoxForEachBo( p, iObj, iTerm, i )
            Bac_ObjSetIndex( p, iTerm, i );
    }
}
static inline void Bac_NtkPrint( Bac_Ntk_t * p )
{
    int i, Type, Value, Beg, End;
    printf( "Interface (%d):\n", Bac_NtkInfoNum(p) );
    Vec_IntForEachEntryTriple( &p->vInfo, Value, Beg, End, i )
    {
        printf( "%6d : ", i );
        printf( "Type =%3d  ", Bac_NtkInfoType(p, i/3) );
        if ( Beg >= 0 )
            printf( "[%d:%d]   ", End, Beg );
        else
            printf( "        " );
        printf( "Name =%3d   ", Bac_NtkInfoName(p, i/3) );
        if ( Bac_NtkInfoName(p, i/3) )
            printf( "%s", Bac_NtkStr( p, Bac_NtkInfoName(p, i/3) ) );
        printf( "\n" );
    }
    printf( "Objects (%d):\n", Bac_NtkObjNum(p) );
    Bac_NtkForEachObjType( p, Type, i )
    {
        printf( "%6d : ", i );
        printf( "Type =%3d  ", Type );
        if ( Bac_ObjIsCo(p, i) )
            printf( "Fanin =%6d  ", Bac_ObjFanin(p, i) );
        else if ( Bac_NtkHasNames(p) && Bac_ObjName(p, i) )
        {
            printf( "Name  =%6d(%d)  ", Bac_ObjNameId(p, i), Bac_ObjNameType(p, i) );
            if ( Bac_ObjNameType(p, i) <= BAC_NAME_WORD )
                printf( "%s", Bac_ObjNameStr(p, i) );
        }
        printf( "\n" );
    }
}


/**Function*************************************************************

  Synopsis    [Manager APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Bac_Man_t * Bac_ManAlloc( char * pFileName, int nNtks )
{
    extern void Bac_ManSetupTypes( char ** pNames, char ** pSymbs );
    Bac_Ntk_t * pNtk; int i;
    Bac_Man_t * pNew = ABC_CALLOC( Bac_Man_t, 1 );
    pNew->pName = Extra_FileDesignName( pFileName );
    pNew->pSpec = Abc_UtilStrsav( pFileName );
    pNew->pStrs = Abc_NamStart( 1000, 24 );
    pNew->pMods = Abc_NamStart( 1000, 24 );
    pNew->iRoot = 1;
    pNew->nNtks = nNtks;
    pNew->pNtks = ABC_CALLOC( Bac_Ntk_t, pNew->nNtks + 1 );
    Bac_ManForEachNtk( pNew, pNtk, i )
        pNtk->pDesign = pNew;
    Bac_ManSetupTypes( pNew->pPrimNames, pNew->pPrimSymbs );
    return pNew;
}
static inline Bac_Man_t * Bac_ManStart( Bac_Man_t * p, int nNtks )
{
    Bac_Ntk_t * pNtk; int i;
    Bac_Man_t * pNew = ABC_CALLOC( Bac_Man_t, 1 );
    pNew->pName = Abc_UtilStrsav( Bac_ManName(p) );
    pNew->pSpec = Abc_UtilStrsav( Bac_ManSpec(p) );
    pNew->pStrs = Abc_NamRef( p->pStrs );  
    pNew->pMods = Abc_NamStart( 1000, 24 );
    pNew->iRoot = 1;
    pNew->nNtks = nNtks;
    pNew->pNtks = ABC_CALLOC( Bac_Ntk_t, nNtks + 1 );
    Bac_ManForEachNtk( pNew, pNtk, i )
        pNtk->pDesign = pNew;
    return pNew;
}
static inline Bac_Man_t * Bac_ManDup( Bac_Man_t * p )
{
    Bac_Ntk_t * pNtk, * pHost; int i;
    Bac_Man_t * pNew = Bac_ManStart( p, Bac_ManNtkNum(p) );
    Bac_ManForEachNtk( p, pNtk, i )
        Bac_NtkSetCopy( pNtk, i );
    Bac_ManForEachNtk( p, pNtk, i )
        Bac_NtkDup( Bac_NtkCopyNtk(pNew, pNtk), pNtk );
    Bac_ManForEachNtk( p, pNtk, i )
        if ( (pHost = Bac_NtkHostNtk(pNtk)) )
            Bac_NtkSetHost( Bac_NtkCopyNtk(pNew, pNtk), Bac_NtkCopy(pHost), Bac_ObjCopy(pHost, Bac_NtkHostObj(pNtk)) );
    return pNew;
}
static inline Bac_Man_t * Bac_ManDupUserBoxes( Bac_Man_t * p )
{
    Bac_Ntk_t * pNtk, * pHost; int i;
    Bac_Man_t * pNew = Bac_ManStart( p, Bac_ManNtkNum(p) );
    Bac_ManForEachNtk( p, pNtk, i )
        Bac_NtkSetCopy( pNtk, i );
    Bac_ManForEachNtk( p, pNtk, i )
        Bac_NtkDupUserBoxes( Bac_NtkCopyNtk(pNew, pNtk), pNtk );
    Bac_ManForEachNtk( p, pNtk, i )
        if ( (pHost = Bac_NtkHostNtk(pNtk)) )
            Bac_NtkSetHost( Bac_NtkCopyNtk(pNew, pNtk), Bac_NtkCopy(pHost), Bac_ObjCopy(pHost, Bac_NtkHostObj(pNtk)) );
    return pNew;
}
static inline void Bac_ManMoveNames( Bac_Man_t * pNew, Bac_Man_t * p )
{
    Bac_Ntk_t * pNtk; int i;
    Bac_ManForEachNtk( p, pNtk, i )
        Bac_NtkMoveNames( Bac_NtkCopyNtk(pNew, pNtk), pNtk );
}


static inline void Bac_ManFree( Bac_Man_t * p )
{
    Bac_Ntk_t * pNtk; int i;
    Bac_ManForEachNtk( p, pNtk, i )
        Bac_NtkFree( pNtk );
    Vec_IntErase( &p->vBuf2LeafNtk );
    Vec_IntErase( &p->vBuf2LeafObj );
    Vec_IntErase( &p->vBuf2RootNtk );
    Vec_IntErase( &p->vBuf2RootObj );
    Abc_NamDeref( p->pStrs );
    Abc_NamDeref( p->pMods );
    ABC_FREE( p->pName );
    ABC_FREE( p->pSpec );
    ABC_FREE( p->pNtks );
    ABC_FREE( p );
}
static inline int Bac_ManMemory( Bac_Man_t * p )
{
    Bac_Ntk_t * pNtk; int i;
    int nMem = sizeof(Bac_Man_t);
    if ( p->pName )
    nMem += (int)strlen(p->pName);
    if ( p->pSpec )
    nMem += (int)strlen(p->pSpec);
    nMem += Abc_NamMemUsed(p->pStrs);
    nMem += Abc_NamMemUsed(p->pMods);
    Bac_ManForEachNtk( p, pNtk, i )
        nMem += Bac_NtkMemory( pNtk );
    return nMem;
}
static inline int Bac_ManObjNum( Bac_Man_t * p )
{
    Bac_Ntk_t * pNtk; int i, Count = 0;
    Bac_ManForEachNtk( p, pNtk, i )
        Count += Bac_NtkObjNum(pNtk);
    return Count;
}
static inline int Bac_ManNodeNum( Bac_Man_t * p )
{
    Bac_Ntk_t * pNtk; int i, Count = 0;
    Bac_ManForEachNtk( p, pNtk, i )
        Count += Bac_NtkBoxNum( pNtk );
    return Count;
}
static inline int Bac_ManBoxNum_rec( Bac_Ntk_t * p )
{
    int iObj, Counter = 0;
    if ( p->Count >= 0 )
        return p->Count;
    Bac_NtkForEachBox( p, iObj )
        Counter += Bac_ObjIsBoxUser(p, iObj) ? Bac_ManBoxNum_rec( Bac_BoxNtk(p, iObj) ) : 1;
    return (p->Count = Counter);
}
static inline int Bac_ManBoxNum( Bac_Man_t * p )
{
    Bac_Ntk_t * pNtk; int i;
    Bac_ManForEachNtk( p, pNtk, i )
        pNtk->Count = -1;
    return Bac_ManBoxNum_rec( Bac_ManRoot(p) );
}
static inline void Bac_ManPrintStats( Bac_Man_t * p, int nModules, int fVerbose )
{
    Bac_Ntk_t * pNtk; int i;
    Bac_Ntk_t * pRoot = Bac_ManRoot( p );
    printf( "%-12s : ",   Bac_ManName(p) );
    printf( "pi =%5d  ",  Bac_NtkPiNum(pRoot) );
    printf( "po =%5d  ",  Bac_NtkPoNum(pRoot) );
    printf( "pri =%4d  ", Bac_ManPrimNum(p) );
    printf( "mod =%6d  ", Bac_ManNtkNum(p) );
    printf( "box =%7d  ", Bac_ManNodeNum(p) );
    printf( "obj =%7d  ", Bac_ManObjNum(p) );
    printf( "mem =%6.3f MB", 1.0*Bac_ManMemory(p)/(1<<20) );
    printf( "\n" );
    Bac_ManBoxNum( p );
    Bac_ManForEachNtk( p, pNtk, i )
    {
        if ( i == nModules+1 )
            break;
        printf( "Module %5d : ", i );
        Bac_NtkPrintStats( pNtk );
    }
}



/**Function*************************************************************

  Synopsis    [Other APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Bac_ObjType_t Ptr_SopToType( char * pSop )
{
    if ( !strcmp(pSop, " 0\n") )         return BAC_BOX_CF;
    if ( !strcmp(pSop, " 1\n") )         return BAC_BOX_CT;
    if ( !strcmp(pSop, "1 1\n") )        return BAC_BOX_BUF;
    if ( !strcmp(pSop, "0 1\n") )        return BAC_BOX_INV;
    if ( !strcmp(pSop, "11 1\n") )       return BAC_BOX_AND;
    if ( !strcmp(pSop, "00 1\n") )       return BAC_BOX_NOR;
    if ( !strcmp(pSop, "00 0\n") )       return BAC_BOX_OR;
    if ( !strcmp(pSop, "-1 1\n1- 1\n") ) return BAC_BOX_OR;
    if ( !strcmp(pSop, "1- 1\n-1 1\n") ) return BAC_BOX_OR;
    if ( !strcmp(pSop, "01 1\n10 1\n") ) return BAC_BOX_XOR;
    if ( !strcmp(pSop, "10 1\n01 1\n") ) return BAC_BOX_XOR;
    if ( !strcmp(pSop, "11 1\n00 1\n") ) return BAC_BOX_XNOR;
    if ( !strcmp(pSop, "00 1\n11 1\n") ) return BAC_BOX_XNOR;
    if ( !strcmp(pSop, "10 1\n") )       return BAC_BOX_SHARP;
    if ( !strcmp(pSop, "01 1\n") )       return BAC_BOX_SHARPL;
    assert( 0 );
    return BAC_OBJ_NONE;
}
static inline char * Ptr_SopToTypeName( char * pSop )
{
    if ( !strcmp(pSop, " 0\n") )         return "BAC_BOX_C0";
    if ( !strcmp(pSop, " 1\n") )         return "BAC_BOX_C1";
    if ( !strcmp(pSop, "1 1\n") )        return "BAC_BOX_BUF";
    if ( !strcmp(pSop, "0 1\n") )        return "BAC_BOX_INV";
    if ( !strcmp(pSop, "11 1\n") )       return "BAC_BOX_AND";
    if ( !strcmp(pSop, "00 1\n") )       return "BAC_BOX_NOR";
    if ( !strcmp(pSop, "00 0\n") )       return "BAC_BOX_OR";
    if ( !strcmp(pSop, "-1 1\n1- 1\n") ) return "BAC_BOX_OR";
    if ( !strcmp(pSop, "1- 1\n-1 1\n") ) return "BAC_BOX_OR";
    if ( !strcmp(pSop, "01 1\n10 1\n") ) return "BAC_BOX_XOR";
    if ( !strcmp(pSop, "10 1\n01 1\n") ) return "BAC_BOX_XOR";
    if ( !strcmp(pSop, "11 1\n00 1\n") ) return "BAC_BOX_XNOR";
    if ( !strcmp(pSop, "00 1\n11 1\n") ) return "BAC_BOX_XNOR";
    if ( !strcmp(pSop, "10 1\n") )       return "BAC_BOX_SHARP";
    if ( !strcmp(pSop, "01 1\n") )       return "BAC_BOX_SHARPL";
    assert( 0 );
    return NULL;
}
static inline char * Ptr_TypeToName( Bac_ObjType_t Type )
{
    if ( Type == BAC_BOX_CF )    return "const0";
    if ( Type == BAC_BOX_CT )    return "const1";
    if ( Type == BAC_BOX_CX )    return "constX";
    if ( Type == BAC_BOX_CZ )    return "constZ";
    if ( Type == BAC_BOX_BUF )   return "buf";
    if ( Type == BAC_BOX_INV )   return "not";
    if ( Type == BAC_BOX_AND )   return "and";
    if ( Type == BAC_BOX_NAND )  return "nand";
    if ( Type == BAC_BOX_OR )    return "or";
    if ( Type == BAC_BOX_NOR )   return "nor";
    if ( Type == BAC_BOX_XOR )   return "xor";
    if ( Type == BAC_BOX_XNOR )  return "xnor";
    if ( Type == BAC_BOX_MUX )   return "mux";
    if ( Type == BAC_BOX_MAJ )   return "maj";
    if ( Type == BAC_BOX_SHARP ) return "sharp";
    if ( Type == BAC_BOX_SHARPL) return "sharpl";
    assert( 0 );
    return "???";
}
static inline char * Ptr_TypeToSop( Bac_ObjType_t Type )
{
    if ( Type == BAC_BOX_CF )    return " 0\n";
    if ( Type == BAC_BOX_CT )    return " 1\n";
    if ( Type == BAC_BOX_CX )    return " 0\n";
    if ( Type == BAC_BOX_CZ )    return " 0\n";
    if ( Type == BAC_BOX_BUF )   return "1 1\n";
    if ( Type == BAC_BOX_INV )   return "0 1\n";
    if ( Type == BAC_BOX_AND )   return "11 1\n";
    if ( Type == BAC_BOX_NAND )  return "11 0\n";
    if ( Type == BAC_BOX_OR )    return "00 0\n";
    if ( Type == BAC_BOX_NOR )   return "00 1\n";
    if ( Type == BAC_BOX_XOR )   return "01 1\n10 1\n";
    if ( Type == BAC_BOX_XNOR )  return "00 1\n11 1\n";
    if ( Type == BAC_BOX_SHARP ) return "10 1\n";
    if ( Type == BAC_BOX_SHARPL) return "01 1\n";
    if ( Type == BAC_BOX_MUX )   return "11- 1\n0-1 1\n";
    if ( Type == BAC_BOX_MAJ )   return "11- 1\n1-1 1\n-11 1\n";
    assert( 0 );
    return "???";
}

/*=== bacCom.c ===============================================================*/
extern void          Abc_FrameImportPtr( Vec_Ptr_t * vPtr );
extern Vec_Ptr_t *   Abc_FrameExportPtr();

/*=== bacBlast.c =============================================================*/
extern int           Bac_NtkBuildLibrary( Bac_Man_t * p );
extern Gia_Man_t *   Bac_ManExtract( Bac_Man_t * p, int fBuffers, int fVerbose );
extern Bac_Man_t *   Bac_ManInsertGia( Bac_Man_t * p, Gia_Man_t * pGia );
extern void *        Bac_ManInsertAbc( Bac_Man_t * p, void * pAbc );
/*=== bacCba.c ===============================================================*/
extern Bac_Man_t *   Bac_ManReadBac( char * pFileName );
extern void          Bac_ManWriteBac( char * pFileName, Bac_Man_t * p );
/*=== bacNtk.c ===============================================================*/
extern char *        Bac_NtkGenerateName( Bac_Ntk_t * p, Bac_ObjType_t Type, Vec_Int_t * vBits );
extern Bac_ObjType_t Bac_NameToType( char * pName );
extern Vec_Int_t *   Bac_NameToRanges( char * pName );
extern void          Bac_NtkUpdateFanout( Bac_Ntk_t * p, int iOld, int iNew );
extern void          Bac_ManDeriveFanout( Bac_Man_t * p );
//extern void          Bac_ManAssignInternNames( Bac_Man_t * p );
extern void          Bac_ManAssignInternWordNames( Bac_Man_t * p );
extern Bac_Man_t *   Bac_ManCollapse( Bac_Man_t * p );
extern void          Bac_ManSetupTypes( char ** pNames, char ** pSymbs );
/*=== bacPtr.c ===============================================================*/
extern void          Bac_PtrFree( Vec_Ptr_t * vDes );
extern int           Bac_PtrMemory( Vec_Ptr_t * vDes );
extern void          Bac_PtrDumpBlif( char * pFileName, Vec_Ptr_t * vDes );
extern void          Bac_PtrDumpVerilog( char * pFileName, Vec_Ptr_t * vDes );
extern Vec_Ptr_t *   Bac_PtrTransformTest( Vec_Ptr_t * vDes );
/*=== bacPtrAbc.c ============================================================*/
extern Bac_Man_t *   Bac_PtrTransformToCba( Vec_Ptr_t * vDes );
extern Vec_Ptr_t *   Bac_PtrDeriveFromCba( Bac_Man_t * p );
/*=== bacPrsBuild.c ==========================================================*/
extern Bac_Man_t *   Psr_ManBuildCba( char * pFileName, Vec_Ptr_t * vDes );
/*=== bacReadBlif.c ==========================================================*/
extern Vec_Ptr_t *   Psr_ManReadBlif( char * pFileName );
/*=== bacReadSmt.c ===========================================================*/
extern Vec_Ptr_t *   Psr_ManReadSmt( char * pFileName );
/*=== bacReadVer.c ===========================================================*/
extern Vec_Ptr_t *   Psr_ManReadVerilog( char * pFileName );
/*=== bacWriteBlif.c =========================================================*/
extern void          Psr_ManWriteBlif( char * pFileName, Vec_Ptr_t * p );
extern void          Bac_ManWriteBlif( char * pFileName, Bac_Man_t * p );
/*=== bacWriteVer.c ==========================================================*/
extern void          Psr_ManWriteVerilog( char * pFileName, Vec_Ptr_t * p );
extern void          Bac_ManWriteVerilog( char * pFileName, Bac_Man_t * p, int fUseAssign );

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

