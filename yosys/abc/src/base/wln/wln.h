/**CFile****************************************************************

  FileName    [wlc.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Verilog parser.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 22, 2014.]

  Revision    [$Id: wlc.h,v 1.00 2014/09/12 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__base__wln__wln_h
#define ABC__base__wln__wln_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "aig/gia/gia.h"
#include "misc/vec/vecHash.h"
#include "misc/extra/extra.h"
#include "misc/util/utilNam.h"
#include "misc/util/utilTruth.h"
#include "aig/miniaig/abcOper.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START 

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Wln_Vec_t_  Wln_Vec_t;
struct Wln_Vec_t_ 
{
    int                    nCap;
    int                    nSize;
    union { int            Array[2];  
            int *          pArray[1]; };
};

typedef struct Wln_Ntk_t_  Wln_Ntk_t;
struct Wln_Ntk_t_ 
{
    char *                 pName;                // model name
    char *                 pSpec;                // input file name
    int                    fSmtLib;              // derived from SMT-LIB
    Vec_Int_t              vCis;                 // combinational inputs
    Vec_Int_t              vCos;                 // combinational outputs
    Vec_Int_t              vFfs;                 // flops
    Vec_Int_t              vTypes;               // object types   
    Wln_Vec_t *            vFanins;              // object fanins (exceptions: const, select)
    Vec_Int_t              vRanges;              // object ranges
    Hash_IntMan_t *        pRanges;              // object ranges
    Vec_Int_t              vNameIds;             // object name IDs
    Vec_Int_t              vInstIds;             // object name IDs
    Abc_Nam_t *            pManName;             // object names
    Vec_Str_t              vSigns;               // object signedness
    int                    nTravIds;             // counter of traversal IDs
    Vec_Int_t              vTravIds;             // trav IDs of the objects
    Vec_Int_t              vCopies;              // object first bits
    Vec_Int_t              vBits;                // object mapping into AIG nodes
    Vec_Int_t              vLevels;              // object levels
    Vec_Int_t              vRefs;                // object reference counters
    Vec_Int_t              vFanout;              // static fanout
    Vec_Int_t              vFaninAttrs;          // static fanin attributes
    Vec_Int_t              vFaninLists;          // static fanin attributes
    Vec_Ptr_t *            vTables;              // tables
    int                    nObjs[ABC_OPER_LAST]; // counter of objects of each type
    int                    nAnds[ABC_OPER_LAST]; // counter of AND gates after blasting
};

static inline int          Wln_NtkObjNum( Wln_Ntk_t * p )                        { return Vec_IntSize(&p->vTypes);                                            }
static inline int          Wln_NtkCiNum( Wln_Ntk_t * p )                         { return Vec_IntSize(&p->vCis);                                              }
static inline int          Wln_NtkCoNum( Wln_Ntk_t * p )                         { return Vec_IntSize(&p->vCos);                                              }
static inline int          Wln_NtkFfNum( Wln_Ntk_t * p )                         { return Vec_IntSize(&p->vFfs);                                              }
static inline int          Wln_NtkPiNum( Wln_Ntk_t * p )                         { return Wln_NtkCiNum(p) - Wln_NtkFfNum(p);                                  }
static inline int          Wln_NtkPoNum( Wln_Ntk_t * p )                         { return Wln_NtkCoNum(p) - Wln_NtkFfNum(p);                                  }

static inline int          Wln_NtkCi( Wln_Ntk_t * p, int i )                     { return Vec_IntEntry(&p->vCis, i);                                          }
static inline int          Wln_NtkCo( Wln_Ntk_t * p, int i )                     { return Vec_IntEntry(&p->vCos, i);                                          }
static inline int          Wln_NtkFf( Wln_Ntk_t * p, int i )                     { return Vec_IntEntry(&p->vFfs, i);                                          }

static inline int          Wln_ObjType( Wln_Ntk_t * p, int i )                   { return Vec_IntEntry(&p->vTypes, i);                                        }
static inline int          Wln_ObjIsNone( Wln_Ntk_t * p, int i )                 { return Wln_ObjType(p, i) == ABC_OPER_NONE;                                 }
static inline int          Wln_ObjIsCi( Wln_Ntk_t * p, int i )                   { return Wln_ObjType(p, i) == ABC_OPER_CI;                                   }
static inline int          Wln_ObjIsCo( Wln_Ntk_t * p, int i )                   { return Wln_ObjType(p, i) == ABC_OPER_CO;                                   }
static inline int          Wln_ObjIsCio( Wln_Ntk_t * p, int i )                  { return Wln_ObjType(p, i) == ABC_OPER_CI || Wln_ObjType(p, i)==ABC_OPER_CO; }
static inline int          Wln_ObjIsFon( Wln_Ntk_t * p, int i )                  { return Wln_ObjType(p, i) == ABC_OPER_FON;                                  }
static inline int          Wln_ObjIsFf( Wln_Ntk_t * p, int i )                   { return Wln_ObjType(p, i) == ABC_OPER_DFFRSE;                               }
static inline int          Wln_ObjIsConst( Wln_Ntk_t * p, int i )                { return Wln_ObjType(p, i) == ABC_OPER_CONST;                                }
static inline int          Wln_ObjIsSlice( Wln_Ntk_t * p, int i )                { return Wln_ObjType(p, i) == ABC_OPER_SLICE;                                }
static inline int          Wln_ObjIsRotate( Wln_Ntk_t * p, int i )               { return Wln_ObjType(p, i) == ABC_OPER_SHIFT_ROTL || Wln_ObjType(p, i) == ABC_OPER_SHIFT_ROTR; }
static inline int          Wln_ObjIsTable( Wln_Ntk_t * p, int i )                { return Wln_ObjType(p, i) == ABC_OPER_TABLE;                                }

static inline int          Wln_ObjFaninNum( Wln_Ntk_t * p, int i )               { return p->vFanins[i].nSize;                                                }
static inline int *        Wln_ObjFanins( Wln_Ntk_t * p, int i )                 { return Wln_ObjFaninNum(p, i) > 2 ? p->vFanins[i].pArray[0]    : p->vFanins[i].Array;    }
static inline int          Wln_ObjFanin( Wln_Ntk_t * p, int i, int f )           { return Wln_ObjFaninNum(p, i) > 2 ? p->vFanins[i].pArray[0][f] : p->vFanins[i].Array[f]; }
static inline void         Wln_ObjSetFanin( Wln_Ntk_t * p, int i, int f, int v ) { Wln_ObjFanins( p, i )[f] = v;                                              }
static inline int          Wln_ObjFanin0( Wln_Ntk_t * p, int i )                 { return Wln_ObjFanin( p, i, 0 );                                            }
static inline int          Wln_ObjFanin1( Wln_Ntk_t * p, int i )                 { return Wln_ObjFanin( p, i, 1 );                                            }
static inline int          Wln_ObjFanin2( Wln_Ntk_t * p, int i )                 { return Wln_ObjFanin( p, i, 2 );                                            }

static inline int          Wln_ObjRangeId( Wln_Ntk_t * p, int i )                { return Vec_IntEntry( &p->vRanges, i );                                     }
static inline int          Wln_ObjRangeEnd( Wln_Ntk_t * p, int i )               { return Hash_IntObjData0( p->pRanges, Wln_ObjRangeId(p, i) );               }
static inline int          Wln_ObjRangeBeg( Wln_Ntk_t * p, int i )               { return Hash_IntObjData1( p->pRanges, Wln_ObjRangeId(p, i) );               }
static inline int          Wln_ObjRangeIsReversed( Wln_Ntk_t * p, int i )        { return Wln_ObjRangeEnd(p, i) < Wln_ObjRangeBeg(p, i);                      }
static inline int          Wln_ObjRange( Wln_Ntk_t * p, int i )                  { return 1 + Abc_AbsInt(Wln_ObjRangeEnd(p, i)-Wln_ObjRangeBeg(p, i));        }
        
static inline int          Wln_ObjIsSigned( Wln_Ntk_t * p, int i )               { return (int)Vec_StrEntry(&p->vSigns, i);                                   }
static inline void         Wln_ObjSetSigned( Wln_Ntk_t * p, int i )              { Vec_StrSetEntry(&p->vSigns, i, (char)1);                                   }
static inline int          Wln_ObjIsSignedFanin0( Wln_Ntk_t * p, int i )         { return Wln_ObjIsSigned( p, p->fSmtLib ? i : Wln_ObjFanin0(p, i) );         }
static inline int          Wln_ObjIsSignedFanin1( Wln_Ntk_t * p, int i )         { return Wln_ObjIsSigned( p, p->fSmtLib ? i : Wln_ObjFanin1(p, i) );         }
static inline int          Wln_ObjIsSignedFanin01( Wln_Ntk_t * p, int i )        { return Wln_ObjIsSignedFanin0( p, i ) && Wln_ObjIsSignedFanin1( p, i );     }
static inline int          Wln_ObjSign( Wln_Ntk_t * p, int i )                   { return Abc_Var2Lit( Wln_ObjRange(p, i), Wln_ObjIsSigned(p, i) );           }

static inline void         Wln_NtkCleanCopy( Wln_Ntk_t * p )                     { Vec_IntFill( &p->vCopies, Vec_IntCap(&p->vTypes), 0 );                     }
static inline int          Wln_NtkHasCopy( Wln_Ntk_t * p )                       { return Vec_IntSize( &p->vCopies ) > 0;                                     }
static inline void         Wln_ObjSetCopy( Wln_Ntk_t * p, int i, int c )         { Vec_IntWriteEntry( &p->vCopies, i, c );                                    }
static inline int          Wln_ObjCopy( Wln_Ntk_t * p, int i )                   { return Vec_IntEntry( &p->vCopies, i );                                     }

static inline void         Wln_NtkCleanLevel( Wln_Ntk_t * p )                    { Vec_IntFill( &p->vLevels, Vec_IntCap(&p->vTypes), 0 );                     }
static inline int          Wln_NtkHasLevel( Wln_Ntk_t * p )                      { return Vec_IntSize( &p->vLevels ) > 0;                                     }
static inline void         Wln_ObjSetLevel( Wln_Ntk_t * p, int i, int l )        { Vec_IntWriteEntry( &p->vLevels, i, l );                                    }
static inline int          Wln_ObjLevel( Wln_Ntk_t * p, int i )                  { return Vec_IntEntry( &p->vLevels, i );                                     }

static inline void         Wln_NtkCleanNameId( Wln_Ntk_t * p )                   { Vec_IntFill( &p->vNameIds, Vec_IntCap(&p->vTypes), 0 );                    }
static inline int          Wln_NtkHasNameId( Wln_Ntk_t * p )                     { return Vec_IntSize( &p->vNameIds ) > 0;                                    }
static inline void         Wln_ObjSetNameId( Wln_Ntk_t * p, int i, int n )       { Vec_IntWriteEntry( &p->vNameIds, i, n );                                   }
static inline int          Wln_ObjNameId( Wln_Ntk_t * p, int i )                 { return Vec_IntEntry( &p->vNameIds, i );                                    }

static inline void         Wln_NtkCleanInstId( Wln_Ntk_t * p )                   { Vec_IntFill( &p->vInstIds, Vec_IntCap(&p->vTypes), 0 );                    }
static inline int          Wln_NtkHasInstId( Wln_Ntk_t * p )                     { return Vec_IntSize( &p->vInstIds ) > 0;                                    }
static inline void         Wln_ObjSetInstId( Wln_Ntk_t * p, int i, int n )       { Vec_IntWriteEntry( &p->vInstIds, i, n );                                   }
static inline int          Wln_ObjInstId( Wln_Ntk_t * p, int i )                 { return Vec_IntEntry( &p->vInstIds, i );                                    }

static inline void         Wln_NtkCleanRefs( Wln_Ntk_t * p )                     { Vec_IntFill( &p->vRefs, Vec_IntCap(&p->vTypes), 0 );                       }
static inline int          Wln_NtkHasRefs( Wln_Ntk_t * p )                       { return Vec_IntSize( &p->vRefs ) > 0;                                       }
static inline void         Wln_ObjSetRefs( Wln_Ntk_t * p, int i, int n )         { Vec_IntWriteEntry( &p->vRefs, i, n );                                      }
static inline int          Wln_ObjRefs( Wln_Ntk_t * p, int i )                   { return Vec_IntEntry( &p->vRefs, i );                                       }
static inline int          Wln_ObjRefsInc( Wln_Ntk_t * p, int i )                { return (*Vec_IntEntryP( &p->vRefs, i ))++;                                 }
static inline int          Wln_ObjRefsDec( Wln_Ntk_t * p, int i )                { return --(*Vec_IntEntryP( &p->vRefs, i ));                                 }
static inline void         Wln_ObjRefsFaninInc( Wln_Ntk_t * p, int i, int k )    { Wln_ObjRefsInc( p, Wln_ObjFanin(p, i, k) );                                }
static inline void         Wln_ObjRefsFaninDec( Wln_Ntk_t * p, int i, int k )    { Wln_ObjRefsDec( p, Wln_ObjFanin(p, i, k) );                                }

static inline int          Wln_ObjFanoutNum( Wln_Ntk_t * p, int i )              { return Vec_IntEntry( &p->vRefs, i );                                       }
static inline int *        Wln_ObjFanouts( Wln_Ntk_t * p, int i )                { return Vec_IntEntryP( &p->vFanout, Vec_IntEntry(&p->vFanout, i) );         }
static inline int          Wln_ObjFanout( Wln_Ntk_t * p, int i, int f )          { return Wln_ObjFanouts( p, i )[f];                                          }
static inline void         Wln_ObjSetFanout( Wln_Ntk_t * p, int i, int f, int v ){ Wln_ObjFanouts( p, i )[f] = v;                                             }

static inline void         Wln_NtkIncrementTravId( Wln_Ntk_t * p )               { if (!p->nTravIds++) Vec_IntFill(&p->vTravIds, Vec_IntCap(&p->vTypes), 0);  }       
static inline void         Wln_ObjSetTravIdCurrent( Wln_Ntk_t * p, int i )       { Vec_IntWriteEntry( &p->vTravIds, i, p->nTravIds );                         }
static inline void         Wln_ObjSetTravIdPrevious( Wln_Ntk_t * p, int i )      { Vec_IntWriteEntry( &p->vTravIds, i, p->nTravIds-1 );                       }
static inline int          Wln_ObjIsTravIdCurrent( Wln_Ntk_t * p, int i )        { return (Vec_IntEntry(&p->vTravIds, i) == p->nTravIds);                     }   
static inline int          Wln_ObjIsTravIdPrevious( Wln_Ntk_t * p, int i )       { return (Vec_IntEntry(&p->vTravIds, i) == p->nTravIds-1);                   }   
static inline int          Wln_ObjCheckTravId( Wln_Ntk_t * p, int i )            { if ( Wln_ObjIsTravIdCurrent(p, i) ) return 1; Wln_ObjSetTravIdCurrent(p, i); return 0; }   

static inline int          Wln_ObjCioId( Wln_Ntk_t * p, int i )                  { assert( Wln_ObjIsCio(p, i) ); return Wln_ObjFanin1(p, i);                  }
static inline int          Wln_ObjIsPi( Wln_Ntk_t * p, int i )                   { return Wln_ObjIsCi(p, i) && Wln_ObjCioId(p, i) <  Wln_NtkPiNum(p);         } 
static inline int          Wln_ObjIsPo( Wln_Ntk_t * p, int i )                   { return Wln_ObjIsCo(p, i) && Wln_ObjCioId(p, i) <  Wln_NtkPoNum(p);         } 
static inline int          Wln_ObjIsRo( Wln_Ntk_t * p, int i )                   { return Wln_ObjIsCi(p, i) && Wln_ObjCioId(p, i) >= Wln_NtkPiNum(p);         } 
static inline int          Wln_ObjIsRi( Wln_Ntk_t * p, int i )                   { return Wln_ObjIsCo(p, i) && Wln_ObjCioId(p, i) >= Wln_NtkPoNum(p);         } 
static inline int          Wln_ObjRoToRi( Wln_Ntk_t * p, int i )                 { assert( Wln_ObjIsRo(p, i) ); return Wln_NtkCo(p, Wln_NtkCoNum(p) - Wln_NtkCiNum(p) + Wln_ObjCioId(p, i)); } 
static inline int          Wln_ObjRiToRo( Wln_Ntk_t * p, int i )                 { assert( Wln_ObjIsRi(p, i) ); return Wln_NtkCi(p, Wln_NtkCiNum(p) - Wln_NtkCoNum(p) + Wln_ObjCioId(p, i)); } 

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                             ITERATORS                            ///
////////////////////////////////////////////////////////////////////////

#define Wln_NtkForEachObj( p, i )                                                   \
    for ( i = 1; i < Wln_NtkObjNum(p); i++ )
#define Wln_NtkForEachObjReverse( p, i )                                            \
    for ( i = Wln_NtkObjNum(p) - 1; i >  0; i-- )
#define Wln_NtkForEachObjInternal( p, i )                                           \
    for ( i = 1; i < Wln_NtkObjNum(p); i++ ) if ( Wln_ObjIsCio(p, i) ) {} else 

#define Wln_NtkForEachPi( p, iPi, i )                                               \
    for ( i = 0; (i < Wln_NtkPiNum(p)) && (((iPi) = Wln_NtkCi(p, i)), 1); i++ )
#define Wln_NtkForEachPo( p, iPo, i )                                               \
    for ( i = 0; (i < Wln_NtkPoNum(p)) && (((iPo) = Wln_NtkCo(p, i)), 1); i++ )
#define Wln_NtkForEachCi( p, iCi, i )                                               \
    for ( i = 0; (i < Wln_NtkCiNum(p)) && (((iCi) = Wln_NtkCi(p, i)), 1); i++ )
#define Wln_NtkForEachCo( p, iCo, i )                                               \
    for ( i = 0; (i < Wln_NtkCoNum(p)) && (((iCo) = Wln_NtkCo(p, i)), 1); i++ )
#define Wln_NtkForEachFf( p, iFf, i )                                               \
    for ( i = 0; (i < Wln_NtkFfNum(p)) && (((iFf) = Wln_NtkFf(p, i)), 1); i++ )

#define Wln_ObjForEachFanin( p, iObj, iFanin, i )                                   \
    for ( i = 0; (i < Wln_ObjFaninNum(p, iObj)) && (((iFanin) = Wln_ObjFanin(p, iObj, i)), 1); i++ ) if ( !iFanin ) {} else
#define Wln_ObjForEachFaninReverse( pObj, iFanin, i )                               \
    for ( i = Wln_ObjFaninNum(p, iObj) - 1; (i >= 0) && (((iFanin) = Wln_ObjFanin(p, iObj, i)), 1); i-- ) if ( !iFanin ) {} else

#define Wln_ObjForEachFanoutStatic( p, iObj, iFanout, i )                           \
    for ( i = 0; (i < Wln_ObjRefs(p, iObj)) && (((iFanout) = Wln_ObjFanout(p, iObj, i)), 1); i++ )


////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== wlcNdr.c ========================================================*/
extern Wln_Ntk_t *    Wln_ReadNdr( char * pFileName );
extern void           Wln_WriteNdr( Wln_Ntk_t * pNtk, char * pFileName );
extern Wln_Ntk_t *    Wln_NtkFromNdr( void * pData, int fDump );
extern void *         Wln_NtkToNdr( Wln_Ntk_t * pNtk );
/*=== wlcNtk.c ========================================================*/
extern Wln_Ntk_t *    Wln_NtkAlloc( char * pName, int nObjsMax );
extern void           Wln_NtkFree( Wln_Ntk_t * p );
extern int            Wln_NtkMemUsage( Wln_Ntk_t * p );
extern void           Wln_NtkPrint( Wln_Ntk_t * p );
extern Wln_Ntk_t *    Wln_NtkDupDfs( Wln_Ntk_t * p );
extern int            Wln_NtkIsAcyclic( Wln_Ntk_t * p );
extern void           Wln_NtkCreateRefs( Wln_Ntk_t * p );
extern void           Wln_NtkStartFaninMap( Wln_Ntk_t * p, Vec_Int_t * vFaninMap, int nMulti );
extern void           Wln_NtkStartFanoutMap( Wln_Ntk_t * p, Vec_Int_t * vFanoutMap, Vec_Int_t * vFanoutNums, int nMulti );
extern void           Wln_NtkStaticFanoutStart( Wln_Ntk_t * p );
extern void           Wln_NtkStaticFanoutStop( Wln_Ntk_t * p );
extern void           Wln_NtkStaticFanoutTest( Wln_Ntk_t * p );
/*=== wlcObj.c ========================================================*/
extern char *         Wln_ObjName( Wln_Ntk_t * p, int iObj );
extern char *         Wln_ObjConstString( Wln_Ntk_t * p, int iObj );
extern void           Wln_ObjUpdateType( Wln_Ntk_t * p, int iObj, int Type );
extern void           Wln_ObjSetConst( Wln_Ntk_t * p, int iObj, int NameId );
extern void           Wln_ObjSetSlice( Wln_Ntk_t * p, int iObj, int SliceId );
extern void           Wln_ObjAddFanin( Wln_Ntk_t * p, int iObj, int i );
extern int            Wln_ObjAddFanins( Wln_Ntk_t * p, int iObj, Vec_Int_t * vFanins );
extern int            Wln_ObjAlloc( Wln_Ntk_t * p, int Type, int Signed, int End, int Beg );
extern int            Wln_ObjClone( Wln_Ntk_t * pNew, Wln_Ntk_t * p, int iObj );
extern int            Wln_ObjCreateCo( Wln_Ntk_t * p, int iFanin );
extern void           Wln_ObjPrint( Wln_Ntk_t * p, int iObj );
/*=== wlcRetime.c ========================================================*/
extern Vec_Int_t *    Wln_NtkRetime( Wln_Ntk_t * p, int fIgnoreIO, int fSkipSimple, int fVerbose );
extern void           Wln_NtkRetimeCreateDelayInfo( Wln_Ntk_t * pNtk );
/*=== wlcWriteVer.c ========================================================*/
extern void           Wln_WriteVer( Wln_Ntk_t * p, char * pFileName );

/*=== wlcRead.c ========================================================*/
typedef struct Rtl_Lib_t_ Rtl_Lib_t;
extern Rtl_Lib_t *    Rtl_LibReadFile( char * pFileName, char * pFileSpec );
extern void           Rtl_LibFree( Rtl_Lib_t * p );

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

