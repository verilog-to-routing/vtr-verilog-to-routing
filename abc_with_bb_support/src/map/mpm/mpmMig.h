/**CFile****************************************************************

  FileName    [mpmMig.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Configurable technology mapper.]

  Synopsis    [Internal declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 1, 2013.]

  Revision    [$Id: mpmMig.h,v 1.00 2013/06/01 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__map__mpm__mig__h
#define ABC__map__mpm__mig__h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "misc/vec/vec.h"

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

#define MIG_NONE 0x7FFFFFFF
//#define MIG_MASK 0x0000FFFF
//#define MIG_BASE 16
#define MIG_MASK 0x0000FFF
#define MIG_BASE 12

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Mig_Fan_t_ Mig_Fan_t;
struct Mig_Fan_t_
{
    unsigned       fCompl :  1;  // the complemented attribute
    unsigned       Id     : 31;  // fanin ID
};

typedef struct Mig_Obj_t_ Mig_Obj_t;
struct Mig_Obj_t_
{
    Mig_Fan_t      pFans[4];     // fanins
};

typedef struct Mig_Man_t_ Mig_Man_t;
struct Mig_Man_t_
{
    char *         pName;        // name
    int            nObjs;        // number of objects
    int            nRegs;        // number of flops
    int            nChoices;     // number of choices
    Vec_Ptr_t      vPages;       // memory pages
    Vec_Int_t      vCis;         // CI IDs
    Vec_Int_t      vCos;         // CO IDs
    // object iterator
    Mig_Obj_t *    pPage;        // current page
    int            iPage;        // current page index
    // attributes
    int            nTravIds;     // traversal ID counter
    Vec_Int_t      vTravIds;     // traversal IDs
    Vec_Int_t      vLevels;      // levels
    Vec_Int_t      vSibls;       // choice nodes
    Vec_Int_t      vRefs;        // ref counters
    Vec_Int_t      vCopies;      // copies
    void *         pMan;         // mapping manager
};

/*
    Usage of fanin atrributes
    --------------------------------------------------------------------------------------------------------------
       Const0  Terminal    CI      CO     Buf     Node    Node2   Node3   And2    XOR2    MUX     MAJ    Sentinel
    --------------------------------------------------------------------------------------------------------------
    0    -     -/fanin0    -     fanin0  fanin0  fanin0  fanin0  fanin0  fanin0  fanin1  fanin0  fanin1     -
    1    -        -        -       -       -     fanin1  fanin1  fanin1  fanin1  fanin0  fanin1  fanin0     -
    2    -      CIO ID   CIO ID  CIO ID    -    -/fanin2    -    fanin2    -       -     fanin2  fanin2     -
    3    0        ID       ID      ID      ID      ID      ID      ID      ID      ID      ID      ID       -
    --------------------------------------------------------------------------------------------------------------

    One memory page contain 2^MIG_BASE+2 16-byte objects.
    - the first object contains the pointer to the manager (8 bytes)
    - the next 2^MIG_BASE are potentially used as objects
    - the last object is a sentinel to signal the end of the page
*/

static inline int          Mig_IdPage( int v )                 { return v >> MIG_BASE;                                                      }
static inline int          Mig_IdCell( int v )                 { return v & MIG_MASK;                                                       }

static inline char *       Mig_ManName( Mig_Man_t * p )        { return p->pName;                                                           }
static inline int          Mig_ManCiNum( Mig_Man_t * p )       { return Vec_IntSize(&p->vCis);                                              }
static inline int          Mig_ManCoNum( Mig_Man_t * p )       { return Vec_IntSize(&p->vCos);                                              }
static inline int          Mig_ManPiNum( Mig_Man_t * p )       { return Vec_IntSize(&p->vCis) - p->nRegs;                                   }
static inline int          Mig_ManPoNum( Mig_Man_t * p )       { return Vec_IntSize(&p->vCos) - p->nRegs;                                   }
static inline int          Mig_ManRegNum( Mig_Man_t * p )      { return p->nRegs;                                                           }
static inline int          Mig_ManObjNum( Mig_Man_t * p )      { return p->nObjs;                                                           }
static inline int          Mig_ManNodeNum( Mig_Man_t * p )     { return p->nObjs - Vec_IntSize(&p->vCis) - Vec_IntSize(&p->vCos) - 1;       }
static inline int          Mig_ManCandNum( Mig_Man_t * p )     { return Mig_ManCiNum(p) + Mig_ManNodeNum(p);                                }
static inline int          Mig_ManChoiceNum( Mig_Man_t * p )   { return p->nChoices;                                                        }
static inline void         Mig_ManSetRegNum( Mig_Man_t * p, int v )   { p->nRegs = v;                                                       }

static inline Mig_Obj_t *  Mig_ManPage( Mig_Man_t * p, int v ) { return (Mig_Obj_t *)Vec_PtrEntry(&p->vPages, Mig_IdPage(v));               }
static inline Mig_Obj_t *  Mig_ManObj( Mig_Man_t * p, int v )  { assert(v >= 0 && v < p->nObjs);  return Mig_ManPage(p, v) + Mig_IdCell(v); }
static inline Mig_Obj_t *  Mig_ManCi( Mig_Man_t * p, int v )   { return Mig_ManObj( p, Vec_IntEntry(&p->vCis,v) );                          }
static inline Mig_Obj_t *  Mig_ManCo( Mig_Man_t * p, int v )   { return Mig_ManObj( p, Vec_IntEntry(&p->vCos,v) );                          }
static inline Mig_Obj_t *  Mig_ManPi( Mig_Man_t * p, int v )   { assert( v < Mig_ManPiNum(p) );  return Mig_ManCi( p, v );                  }
static inline Mig_Obj_t *  Mig_ManPo( Mig_Man_t * p, int v )   { assert( v < Mig_ManPoNum(p) );  return Mig_ManCo( p, v );                  }
static inline Mig_Obj_t *  Mig_ManRo( Mig_Man_t * p, int v )   { assert( v < Mig_ManRegNum(p) ); return Mig_ManCi( p, Mig_ManPiNum(p)+v );  }
static inline Mig_Obj_t *  Mig_ManRi( Mig_Man_t * p, int v )   { assert( v < Mig_ManRegNum(p) ); return Mig_ManCo( p, Mig_ManPoNum(p)+v );  }
static inline Mig_Obj_t *  Mig_ManConst0( Mig_Man_t * p )      { return Mig_ManObj(p, 0);                                                   }

static inline int          Mig_FanCompl( Mig_Obj_t * p, int i )                { return p->pFans[i].fCompl;                                 }
static inline int          Mig_FanId( Mig_Obj_t * p, int i )                   { return p->pFans[i].Id;                                     }
static inline int          Mig_FanIsNone( Mig_Obj_t * p, int i )               { return p->pFans[i].Id == MIG_NONE;                         }
static inline int          Mig_FanSetCompl( Mig_Obj_t * p, int i, int v )      { assert( !(v >> 1) ); return p->pFans[i].fCompl = v;        }
static inline int          Mig_FanSetId( Mig_Obj_t * p, int i, int v )         { assert(v >= 0 && v < MIG_NONE); return p->pFans[i].Id = v; }

static inline int          Mig_ObjIsNone( Mig_Obj_t * p )                      { return Mig_FanIsNone( p, 3 );                              }
static inline int          Mig_ObjIsConst0( Mig_Obj_t * p )                    { return Mig_FanId( p, 3 ) == 0;                             } 
static inline int          Mig_ObjIsTerm( Mig_Obj_t * p )                      { return Mig_FanIsNone( p, 1 ) && !Mig_FanIsNone( p, 2 );    }
static inline int          Mig_ObjIsCi( Mig_Obj_t * p )                        { return Mig_ObjIsTerm(p) &&  Mig_FanIsNone( p, 0 );         } 
static inline int          Mig_ObjIsCo( Mig_Obj_t * p )                        { return Mig_ObjIsTerm(p) && !Mig_FanIsNone( p, 0 );         } 
static inline int          Mig_ObjIsBuf( Mig_Obj_t * p )                       { return Mig_FanIsNone( p, 1 ) && Mig_FanIsNone( p, 2 ) && !Mig_FanIsNone( p, 0 );     } 
static inline int          Mig_ObjIsNode( Mig_Obj_t * p )                      { return!Mig_FanIsNone( p, 1 );                              } 
static inline int          Mig_ObjIsNode2( Mig_Obj_t * p )                     { return Mig_ObjIsNode( p ) &&  Mig_FanIsNone( p, 2 );       } 
static inline int          Mig_ObjIsNode3( Mig_Obj_t * p )                     { return Mig_ObjIsNode( p ) && !Mig_FanIsNone( p, 2 );       } 
static inline int          Mig_ObjIsAnd( Mig_Obj_t * p )                       { return Mig_ObjIsNode2( p ) && Mig_FanId(p, 0) < Mig_FanId(p, 1); } 
static inline int          Mig_ObjIsXor( Mig_Obj_t * p )                       { return Mig_ObjIsNode2( p ) && Mig_FanId(p, 0) > Mig_FanId(p, 1); } 
static inline int          Mig_ObjIsMux( Mig_Obj_t * p )                       { return Mig_ObjIsNode3( p );                                } 
static inline int          Mig_ObjIsCand( Mig_Obj_t * p )                      { return Mig_ObjIsNode(p) || Mig_ObjIsCi(p);                 } 
static inline int          Mig_ObjNodeType( Mig_Obj_t * p )                    { return Mig_ObjIsAnd(p) ? 1 : (Mig_ObjIsXor(p) ? 2 : 3);    } 

static inline int          Mig_ObjId( Mig_Obj_t * p )                          { return Mig_FanId( p, 3 );                                  }
static inline void         Mig_ObjSetId( Mig_Obj_t * p, int v )                { Mig_FanSetId( p, 3, v );                                   }
static inline int          Mig_ObjCioId( Mig_Obj_t * p )                       { assert( Mig_ObjIsTerm(p) ); return Mig_FanId( p, 2 );      }
static inline void         Mig_ObjSetCioId( Mig_Obj_t * p, int v )             { assert( Mig_FanIsNone(p, 1) ); Mig_FanSetId( p, 2, v );    }
static inline int          Mig_ObjPhase( Mig_Obj_t * p )                       { return Mig_FanCompl( p, 3 );                               }
static inline void         Mig_ObjSetPhase( Mig_Obj_t * p, int v )             { Mig_FanSetCompl( p, 3, v );                                }

static inline Mig_Man_t *  Mig_ObjMan( Mig_Obj_t * p )                         { return *((Mig_Man_t**)(p - Mig_IdCell(Mig_ObjId(p)) - 1)); }
//static inline Mig_Obj_t ** Mig_ObjPageP( Mig_Obj_t * p )                       { return *((Mig_Obj_t***)(p - Mig_IdCell(Mig_ObjId(p))) - 1);} 
static inline Mig_Obj_t *  Mig_ObjObj( Mig_Obj_t * p, int i )                  { return Mig_ManObj( Mig_ObjMan(p), i );                     } 

static inline int          Mig_ManIdToCioId( Mig_Man_t * p, int Id )           { return Mig_ObjCioId( Mig_ManObj(p, Id) );                  }
static inline int          Mig_ManCiIdToId( Mig_Man_t * p, int CiId )          { return Mig_ObjId( Mig_ManCi(p, CiId) );                    }
static inline int          Mig_ManCoIdToId( Mig_Man_t * p, int CoId )          { return Mig_ObjId( Mig_ManCo(p, CoId) );                    }

static inline int          Mig_ObjIsPi( Mig_Obj_t * p )                        { return Mig_ObjIsCi(p) && Mig_ObjCioId(p) < Mig_ManPiNum(Mig_ObjMan(p));   } 
static inline int          Mig_ObjIsPo( Mig_Obj_t * p )                        { return Mig_ObjIsCo(p) && Mig_ObjCioId(p) < Mig_ManPoNum(Mig_ObjMan(p));   } 
static inline int          Mig_ObjIsRo( Mig_Obj_t * p )                        { return Mig_ObjIsCi(p) && Mig_ObjCioId(p) >= Mig_ManPiNum(Mig_ObjMan(p));  } 
static inline int          Mig_ObjIsRi( Mig_Obj_t * p )                        { return Mig_ObjIsCo(p) && Mig_ObjCioId(p) >= Mig_ManPoNum(Mig_ObjMan(p));  } 

static inline Mig_Obj_t *  Mig_ObjRoToRi( Mig_Obj_t * p )                      { Mig_Man_t * pMan = Mig_ObjMan(p); assert( Mig_ObjIsRo(p) ); return Mig_ManCo(pMan, Mig_ManCoNum(pMan) - Mig_ManCiNum(pMan) + Mig_ObjCioId(p)); } 
static inline Mig_Obj_t *  Mig_ObjRiToRo( Mig_Obj_t * p )                      { Mig_Man_t * pMan = Mig_ObjMan(p); assert( Mig_ObjIsRi(p) ); return Mig_ManCi(pMan, Mig_ManCiNum(pMan) - Mig_ManCoNum(pMan) + Mig_ObjCioId(p)); } 

static inline int          Mig_ObjHasFanin( Mig_Obj_t * p, int i )             { return i < 3 && Mig_FanId(p, i) != MIG_NONE;               }
static inline int          Mig_ObjFaninId( Mig_Obj_t * p, int i )              { assert( i < 3 && Mig_FanId(p, i) < Mig_ObjId(p) ); return Mig_FanId( p, i );  }
static inline int          Mig_ObjFaninId0( Mig_Obj_t * p )                    { return Mig_FanId( p, 0 );                                  }
static inline int          Mig_ObjFaninId1( Mig_Obj_t * p )                    { return Mig_FanId( p, 1 );                                  }
static inline int          Mig_ObjFaninId2( Mig_Obj_t * p )                    { return Mig_FanId( p, 2 );                                  }
static inline Mig_Obj_t *  Mig_ObjFanin( Mig_Obj_t * p, int i )                { return Mig_ManObj( Mig_ObjMan(p), Mig_ObjFaninId(p, i) );    }
//static inline Mig_Obj_t *  Mig_ObjFanin( Mig_Obj_t * p, int i )                { return Mig_ObjPageP(p)[Mig_IdPage(Mig_ObjFaninId(p, i))] + Mig_IdCell(Mig_ObjFaninId(p, i));    }
static inline Mig_Obj_t *  Mig_ObjFanin0( Mig_Obj_t * p )                      { return Mig_FanIsNone(p, 0) ? NULL: Mig_ObjFanin(p, 0);     }
static inline Mig_Obj_t *  Mig_ObjFanin1( Mig_Obj_t * p )                      { return Mig_FanIsNone(p, 1) ? NULL: Mig_ObjFanin(p, 1);     }
static inline Mig_Obj_t *  Mig_ObjFanin2( Mig_Obj_t * p )                      { return Mig_FanIsNone(p, 2) ? NULL: Mig_ObjFanin(p, 2);     }
static inline int          Mig_ObjFaninC( Mig_Obj_t * p, int i )               { assert( i < 3 ); return Mig_FanCompl(p, i);                }
static inline int          Mig_ObjFaninC0( Mig_Obj_t * p )                     { return Mig_FanCompl(p, 0);                                 }
static inline int          Mig_ObjFaninC1( Mig_Obj_t * p )                     { return Mig_FanCompl(p, 1);                                 }
static inline int          Mig_ObjFaninC2( Mig_Obj_t * p )                     { return Mig_FanCompl(p, 2);                                 }
static inline int          Mig_ObjFaninLit( Mig_Obj_t * p, int i )             { return Abc_Var2Lit( Mig_FanId(p, i), Mig_FanCompl(p, i) ); }
static inline void         Mig_ObjFlipFaninC( Mig_Obj_t * p, int i )           { Mig_FanSetCompl( p, i, !Mig_FanCompl(p, i) );              }
static inline int          Mig_ObjWhatFanin( Mig_Obj_t * p, int i )            { if (Mig_FanId(p, 0) == i) return 0; if (Mig_FanId(p, 1) == i) return 1; if (Mig_FanId(p, 2) == i) return 2; return -1;           }
static inline void         Mig_ObjSetFaninLit( Mig_Obj_t * p, int i, int l )   { assert( l >= 0 && (l >> 1) < Mig_ObjId(p) ); Mig_FanSetId(p, i, Abc_Lit2Var(l)); Mig_FanSetCompl(p, i, Abc_LitIsCompl(l));       }

static inline int          Mig_ObjSiblId( Mig_Obj_t * p )                      { return Vec_IntSize(&Mig_ObjMan(p)->vSibls) == 0 ? 0: Vec_IntEntry(&Mig_ObjMan(p)->vSibls, Mig_ObjId(p));    }
static inline void         Mig_ObjSetSiblId( Mig_Obj_t * p, int s )            { assert( s > 0 && Mig_ObjId(p) > s ); Vec_IntWriteEntry( &Mig_ObjMan(p)->vSibls, Mig_ObjId(p), s );          }
static inline Mig_Obj_t *  Mig_ObjSibl( Mig_Obj_t * p )                        { return Mig_ObjSiblId(p) == 0 ? NULL: Mig_ObjObj(p, Mig_ObjSiblId(p));  }
static inline int          Mig_ObjRefNum( Mig_Obj_t * p )                      { return Vec_IntEntry(&Mig_ObjMan(p)->vRefs, Mig_ObjId(p));              }

static inline void         Mig_ManCleanCopy( Mig_Man_t * p )                   { if ( p->vCopies.pArray == NULL ) Vec_IntFill( &p->vCopies, Mig_ManObjNum(p), -1 );              }
static inline int          Mig_ObjCopy( Mig_Obj_t * p )                        { return Vec_IntSize(&Mig_ObjMan(p)->vCopies) == 0 ? -1: Vec_IntEntry(&Mig_ObjMan(p)->vCopies, Mig_ObjId(p));      }
static inline void         Mig_ObjSetCopy( Mig_Obj_t * p, int i )              { assert( Vec_IntSize(&Mig_ObjMan(p)->vCopies) != 0 ); Vec_IntWriteEntry(&Mig_ObjMan(p)->vCopies, Mig_ObjId(p), i); }

static inline void         Mig_ManIncrementTravId( Mig_Man_t * p )             { if ( p->vTravIds.pArray == NULL ) Vec_IntFill( &p->vTravIds, Mig_ManObjNum(p)+500, 0 ); p->nTravIds++;      }
static inline void         Mig_ObjIncrementTravId( Mig_Obj_t * p )             { if ( Mig_ObjMan(p)->vTravIds.pArray == NULL ) Vec_IntFill( &Mig_ObjMan(p)->vTravIds, Mig_ManObjNum(Mig_ObjMan(p))+500, 0 ); Mig_ObjMan(p)->nTravIds++;           }
static inline void         Mig_ObjSetTravIdCurrent( Mig_Obj_t * p )            { Vec_IntSetEntry(&Mig_ObjMan(p)->vTravIds, Mig_ObjId(p), Mig_ObjMan(p)->nTravIds );              }
static inline void         Mig_ObjSetTravIdPrevious( Mig_Obj_t * p )           { Vec_IntSetEntry(&Mig_ObjMan(p)->vTravIds, Mig_ObjId(p), Mig_ObjMan(p)->nTravIds-1 );            }
static inline int          Mig_ObjIsTravIdCurrent( Mig_Obj_t * p )             { return (Vec_IntGetEntry(&Mig_ObjMan(p)->vTravIds, Mig_ObjId(p)) == Mig_ObjMan(p)->nTravIds);    }
static inline int          Mig_ObjIsTravIdPrevious( Mig_Obj_t * p )            { return (Vec_IntGetEntry(&Mig_ObjMan(p)->vTravIds, Mig_ObjId(p)) == Mig_ObjMan(p)->nTravIds-1);  }
static inline void         Mig_ObjSetTravIdCurrentId( Mig_Man_t * p, int Id )  { Vec_IntSetEntry(&p->vTravIds, Id, p->nTravIds );                                                }
static inline int          Mig_ObjIsTravIdCurrentId( Mig_Man_t * p, int Id )   { return (Vec_IntGetEntry(&p->vTravIds, Id) == p->nTravIds);                                      }

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Mig_Obj_t * Mig_ManAppendObj( Mig_Man_t * p )
{
    Mig_Obj_t * pObj;
    assert( p->nObjs < MIG_NONE );
    if ( p->nObjs >= (Vec_PtrSize(&p->vPages) << MIG_BASE) )
    {
        Mig_Obj_t * pPage;// int i;
        assert( p->nObjs == (Vec_PtrSize(&p->vPages) << MIG_BASE) );
        pPage = ABC_FALLOC( Mig_Obj_t, MIG_MASK + 3 ); // 1 for mask, 1 for prefix, 1 for sentinel
        *((void **)pPage) = p;
//        *((void ***)(pPage + 1) - 1) = Vec_PtrArray(&p->vPages);
        Vec_PtrPush( &p->vPages, pPage + 1 );
//        if ( *((void ***)(pPage + 1) - 1) != Vec_PtrArray(&p->vPages) )
//            Vec_PtrForEachEntry( Mig_Obj_t *, &p->vPages, pPage, i )
//                *((void ***)pPage - 1) = Vec_PtrArray(&p->vPages);
    }
    pObj = Mig_ManObj( p, p->nObjs++ );
    assert( Mig_ObjIsNone(pObj) );
    Mig_ObjSetId( pObj, p->nObjs-1 );
    return pObj;
}
static inline int Mig_ManAppendCi( Mig_Man_t * p )  
{ 
    Mig_Obj_t * pObj = Mig_ManAppendObj( p );
    Mig_ObjSetCioId( pObj, Vec_IntSize(&p->vCis) );
    Vec_IntPush( &p->vCis, Mig_ObjId(pObj) );
    return Mig_ObjId(pObj) << 1;
}
static inline int Mig_ManAppendCo( Mig_Man_t * p, int iLit0 )  
{ 
    Mig_Obj_t * pObj;
    assert( !Mig_ObjIsCo(Mig_ManObj(p, Abc_Lit2Var(iLit0))) );
    pObj = Mig_ManAppendObj( p );    
    Mig_ObjSetFaninLit( pObj, 0, iLit0 );
    Mig_ObjSetCioId( pObj, Vec_IntSize(&p->vCos) );
    Vec_IntPush( &p->vCos, Mig_ObjId(pObj) );
    return Mig_ObjId( pObj ) << 1;
}
static inline int Mig_ManAppendBuf( Mig_Man_t * p, int iLit0 )  
{ 
    Mig_Obj_t * pObj;
    pObj = Mig_ManAppendObj( p );    
    Mig_ObjSetFaninLit( pObj, 0, iLit0 );
    return Mig_ObjId( pObj ) << 1;
}
static inline int Mig_ManAppendAnd( Mig_Man_t * p, int iLit0, int iLit1 )  
{ 
    Mig_Obj_t * pObj = Mig_ManAppendObj( p );
    assert( iLit0 != iLit1 );
    Mig_ObjSetFaninLit( pObj, 0, iLit0 < iLit1 ? iLit0 : iLit1 );
    Mig_ObjSetFaninLit( pObj, 1, iLit0 < iLit1 ? iLit1 : iLit0 );
    return Mig_ObjId( pObj ) << 1;
}
static inline int Mig_ManAppendXor( Mig_Man_t * p, int iLit0, int iLit1 )  
{ 
    Mig_Obj_t * pObj = Mig_ManAppendObj( p );
    assert( iLit0 != iLit1 );
    assert( !Abc_LitIsCompl(iLit0) && !Abc_LitIsCompl(iLit1) );
    Mig_ObjSetFaninLit( pObj, 0, iLit0 < iLit1 ? iLit1 : iLit0 );
    Mig_ObjSetFaninLit( pObj, 1, iLit0 < iLit1 ? iLit0 : iLit1 );
    return Mig_ObjId( pObj ) << 1;
}
static inline int Mig_ManAppendMux( Mig_Man_t * p, int iLit0, int iLit1, int iCtrl )  
{ 
    Mig_Obj_t * pObj = Mig_ManAppendObj( p );
    assert( iLit0 != iLit1 && iLit0 != iCtrl && iLit1 != iCtrl );
    assert( !Abc_LitIsCompl(iLit0) || !Abc_LitIsCompl(iLit1) );
    Mig_ObjSetFaninLit( pObj, 0, iLit0 < iLit1 ? iLit0 : iLit1 );
    Mig_ObjSetFaninLit( pObj, 1, iLit0 < iLit1 ? iLit1 : iLit0 );
    Mig_ObjSetFaninLit( pObj, 2, iLit0 < iLit1 ? iCtrl : Abc_LitNot(iCtrl) );
    return Mig_ObjId( pObj ) << 1;
}
static inline int Mig_ManAppendMaj( Mig_Man_t * p, int iLit0, int iLit1, int iLit2 )  
{ 
    Mig_Obj_t * pObj = Mig_ManAppendObj( p );
    assert( iLit0 != iLit1 && iLit0 != iLit2 && iLit1 != iLit2 );
    Mig_ObjSetFaninLit( pObj, 0, iLit0 < iLit1 ? iLit1 : iLit0 );
    Mig_ObjSetFaninLit( pObj, 1, iLit0 < iLit1 ? iLit0 : iLit1 );
    Mig_ObjSetFaninLit( pObj, 2, iLit2 );
    return Mig_ObjId( pObj ) << 1;
}

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

// iterators over objects
#define Mig_ManForEachObj( p, pObj )                                    \
    for ( p->iPage = 0; p->iPage < Vec_PtrSize(&p->vPages) &&           \
        ((p->pPage) = (Mig_Obj_t *)Vec_PtrEntry(&p->vPages, p->iPage)); p->iPage++ ) \
        for ( pObj = p->pPage; !Mig_ObjIsNone(pObj); pObj++ )
#define Mig_ManForEachObj1( p, pObj )                                   \
    for ( p->iPage = 0; p->iPage < Vec_PtrSize(&p->vPages) &&           \
        ((p->pPage) = (Mig_Obj_t *)Vec_PtrEntry(&p->vPages, p->iPage)); p->iPage++ ) \
        for ( pObj = p->pPage + (p->iPage == 0); !Mig_ObjIsNone(pObj); pObj++ )
#define Mig_ManForEachObjReverse( p, pObj )                             \
    for ( p->iPage = Vec_PtrSize(&p->vPages) - 1; p->iPage >= 0 &&      \
        ((p->pPage) = (Mig_Obj_t *)Vec_PtrEntry(&p->vPages, p->iPage)); p->iPage-- ) \
        for ( pObj = (p->iPage == Vec_PtrSize(&p->vPages) - 1) ?        \
            Mig_ManObj(p, Mig_ManObjNum(p)-1) :  p->pPage + MIG_MASK;   \
                pObj - p->pPage >= 0; pObj-- )

#define Mig_ManForEachObjVec( vVec, p, pObj, i )                        \
    for ( i = 0; (i < Vec_IntSize(vVec)) && ((pObj) = Mig_ManObj(p, Vec_IntEntry(vVec,i))); i++ )

#define Mig_ManForEachNode( p, pObj )                                   \
    Mig_ManForEachObj( p, pObj ) if ( !Mig_ObjIsNode(pObj) ) {} else
#define Mig_ManForEachCand( p, pObj )                                   \
    Mig_ManForEachObj( p, pObj ) if ( !Mig_ObjIsCand(pObj) ) {} else

#define Mig_ManForEachCi( p, pObj, i )                                  \
    for ( i = 0; (i < Vec_IntSize(&p->vCis)) && ((pObj) = Mig_ManCi(p, i)); i++ )
#define Mig_ManForEachCo( p, pObj, i )                                  \
    for ( i = 0; (i < Vec_IntSize(&p->vCos)) && ((pObj) = Mig_ManCo(p, i)); i++ )

// iterators over fanins
#define Mig_ObjForEachFaninId( p, iFanin, i )                           \
    for ( i = 0; Mig_ObjHasFanin(p, i) && ((iFanin) = Mig_ObjFaninId(p, i)); i++ )
#define Mig_ObjForEachFanin( p, pFanin, i )                             \
    for ( i = 0; Mig_ObjHasFanin(p, i) && ((pFanin) = Mig_ObjFanin(p, i)); i++ )


////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== mpmMig.c ===========================================================*/
extern Mig_Man_t *           Mig_ManStart();
extern void                  Mig_ManStop( Mig_Man_t * p );
extern void                  Mig_ManSetRefs( Mig_Man_t * p );
extern int                   Mig_ManAndNum( Mig_Man_t * p );
extern int                   Mig_ManXorNum( Mig_Man_t * p );
extern int                   Mig_ManMuxNum( Mig_Man_t * p );

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

