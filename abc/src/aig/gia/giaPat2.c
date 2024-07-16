/**CFile****************************************************************

  FileName    [giaPat2.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Pattern generation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaPat2.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/vec/vecHsh.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satStore.h"
#include "misc/util/utilTruth.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Min_Man_t_ Min_Man_t;
struct Min_Man_t_
{
    int              nCis;
    int              nCos;
    int              FirstAndLit;
    int              FirstCoLit;
    Vec_Int_t        vFans;
    Vec_Str_t        vValsN;
    Vec_Str_t        vValsL;
    Vec_Int_t        vVis;
    Vec_Int_t        vPat;
};

static inline int    Min_ManCiNum( Min_Man_t * p )                 { return p->nCis;                                       }
static inline int    Min_ManCoNum( Min_Man_t * p )                 { return p->nCos;                                       }
static inline int    Min_ManObjNum( Min_Man_t * p )                { return Vec_IntSize(&p->vFans) >> 1;                   }
static inline int    Min_ManAndNum( Min_Man_t * p )                { return Min_ManObjNum(p) - p->nCis - p->nCos - 1;      }

static inline int    Min_ManCi( Min_Man_t * p, int i )             { return 1 + i;                                         }
static inline int    Min_ManCo( Min_Man_t * p, int i )             { return Min_ManObjNum(p) - Min_ManCoNum(p) + i;        }

static inline int    Min_ObjIsCi( Min_Man_t * p, int i )           { return i > 0 && i <= Min_ManCiNum(p);                 }
static inline int    Min_ObjIsNode( Min_Man_t * p, int i )         { return i > Min_ManCiNum(p) && i < Min_ManObjNum(p) - Min_ManCoNum(p);      }
static inline int    Min_ObjIsAnd( Min_Man_t * p, int i )          { return Min_ObjIsNode(p, i) && Vec_IntEntry(&p->vFans, 2*i) < Vec_IntEntry(&p->vFans, 2*i+1);  }
static inline int    Min_ObjIsXor( Min_Man_t * p, int i )          { return Min_ObjIsNode(p, i) && Vec_IntEntry(&p->vFans, 2*i) > Vec_IntEntry(&p->vFans, 2*i+1);  }
static inline int    Min_ObjIsBuf( Min_Man_t * p, int i )          { return Min_ObjIsNode(p, i) && Vec_IntEntry(&p->vFans, 2*i) ==Vec_IntEntry(&p->vFans, 2*i+1);  }
static inline int    Min_ObjIsCo( Min_Man_t * p, int i )           { return i >= Min_ManObjNum(p) - Min_ManCoNum(p) && i < Min_ManObjNum(p);    }

static inline int    Min_ObjLit( Min_Man_t * p, int i, int n )     { return Vec_IntEntry(&p->vFans, i + i + n);            }
static inline int    Min_ObjLit0( Min_Man_t * p, int i )           { return Vec_IntEntry(&p->vFans, i + i + 0);            }
static inline int    Min_ObjLit1( Min_Man_t * p, int i )           { return Vec_IntEntry(&p->vFans, i + i + 1);            }
static inline int    Min_ObjCioId( Min_Man_t * p, int i )          { assert( i && !Min_ObjIsNode(p, i) ); return Min_ObjLit1(p, i);  }

static inline int    Min_ObjFan0( Min_Man_t * p, int i )           { return Abc_Lit2Var( Min_ObjLit0(p, i) );              }
static inline int    Min_ObjFan1( Min_Man_t * p, int i )           { return Abc_Lit2Var( Min_ObjLit1(p, i) );              }

static inline int    Min_ObjFanC0( Min_Man_t * p, int i )          { return Abc_LitIsCompl( Min_ObjLit0(p, i) );           }
static inline int    Min_ObjFanC1( Min_Man_t * p, int i )          { return Abc_LitIsCompl( Min_ObjLit1(p, i) );           }

static inline char   Min_ObjValN( Min_Man_t * p, int i )           { return Vec_StrEntry(&p->vValsN, i);                   }
static inline void   Min_ObjSetValN( Min_Man_t * p, int i, char v ){ Vec_StrWriteEntry(&p->vValsN, i, v);                  }

static inline char   Min_LitValL( Min_Man_t * p, int i )           { return Vec_StrEntry(&p->vValsL, i);                   }
static inline void   Min_LitSetValL( Min_Man_t * p, int i, char v ){ assert(v==0 || v==1); Vec_StrWriteEntry(&p->vValsL, i, v); Vec_StrWriteEntry(&p->vValsL, i^1, (char)!v); Vec_IntPush(&p->vVis, Abc_Lit2Var(i)); }
static inline void   Min_ObjCleanValL( Min_Man_t * p, int i )      { ((short *)Vec_StrArray(&p->vValsL))[i]  = 0x0202;     }
static inline void   Min_ObjMarkValL( Min_Man_t * p, int i )       { ((short *)Vec_StrArray(&p->vValsL))[i] |= 0x0404;     }
static inline void   Min_ObjMark2ValL( Min_Man_t * p, int i )      { ((short *)Vec_StrArray(&p->vValsL))[i] |= 0x0808;     }
static inline void   Min_ObjUnmark2ValL( Min_Man_t * p, int i )    { ((short *)Vec_StrArray(&p->vValsL))[i] &= 0xF7F7;     }

static inline int    Min_LitIsCi( Min_Man_t * p, int v )           { return v > 1 && v < p->FirstAndLit;                   }
static inline int    Min_LitIsNode( Min_Man_t * p, int v )         { return v >= p->FirstAndLit && v < p->FirstCoLit;      }
static inline int    Min_LitIsCo( Min_Man_t * p, int v )           { return v >= p->FirstCoLit;                            }

static inline int    Min_LitIsAnd( int v, int v0, int v1 )         { return Abc_LitIsCompl(v) ^ (v0 < v1);                 }
static inline int    Min_LitIsXor( int v, int v0, int v1 )         { return Abc_LitIsCompl(v) ^ (v0 > v1);                 }
static inline int    Min_LitIsBuf( int v, int v0, int v1 )         { return v0 == v1;                                      }

static inline int    Min_LitFan( Min_Man_t * p, int v )            { return Vec_IntEntry(&p->vFans, v);                    }
static inline int    Min_LitFanC( Min_Man_t * p, int v )           { return Abc_LitIsCompl( Min_LitFan(p, v) );            }

static inline void   Min_ManStartValsN( Min_Man_t * p )            { Vec_StrGrow(&p->vValsN, Vec_IntCap(&p->vFans)/2); Vec_StrFill(&p->vValsN, Min_ManObjNum(p), 2);       }
static inline void   Min_ManStartValsL( Min_Man_t * p )            { Vec_StrGrow(&p->vValsL, Vec_IntCap(&p->vFans));   Vec_StrFill(&p->vValsL, Vec_IntSize(&p->vFans), 2); }
static inline int    Min_ManCheckCleanValsL( Min_Man_t * p )       { int i; char c; Vec_StrForEachEntry( &p->vValsL, c, i ) if ( c != 2 ) return 0; return 1;  }
static inline void   Min_ManCleanVisitedValL( Min_Man_t * p )      { int i, iObj; Vec_IntForEachEntry(&p->vVis, iObj, i)  Min_ObjCleanValL(p, iObj); Vec_IntClear(&p->vVis); }


#define Min_ManForEachObj( p, i )                                  \
    for ( i = 0; i < Min_ManObjNum(p); i++ )
#define Min_ManForEachCi( p, i )                                   \
    for ( i = 1; i <= Min_ManCiNum(p); i++ )
#define Min_ManForEachCo( p, i )                                   \
    for ( i = Min_ManObjNum(p) - Min_ManCoNum(p); i < Min_ManObjNum(p); i++ )
#define Min_ManForEachAnd( p, i )                                  \
    for ( i = 1 + Min_ManCiNum(p); i < Min_ManObjNum(p) - Min_ManCoNum(p); i++ )


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Min_Man_t * Min_ManStart( int nObjMax )
{
    Min_Man_t * p = ABC_CALLOC( Min_Man_t, 1 );
    Vec_IntGrow( &p->vFans,  nObjMax );
    Vec_IntPushTwo( &p->vFans, -1, -1 );
    return p;
}
static inline void Min_ManStop( Min_Man_t * p )
{
    Vec_IntErase( &p->vFans );
    Vec_StrErase( &p->vValsN );
    Vec_StrErase( &p->vValsL );
    Vec_IntErase( &p->vVis );
    Vec_IntErase( &p->vPat );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Min_ManAppendObj( Min_Man_t * p, int iLit0, int iLit1 )
{
    int iLit = Vec_IntSize(&p->vFans);
    Vec_IntPushTwo( &p->vFans, iLit0, iLit1 );
    return iLit;
}
static inline int Min_ManAppendCi( Min_Man_t * p )
{
    p->nCis++;
    p->FirstAndLit = Vec_IntSize(&p->vFans) + 2;
    return Min_ManAppendObj( p, 0, p->nCis-1 );
}
static inline int Min_ManAppendCo( Min_Man_t * p, int iLit0 )
{
    p->nCos++;
    if ( p->FirstCoLit == 0 )
        p->FirstCoLit = Vec_IntSize(&p->vFans);
    return Min_ManAppendObj( p, iLit0, p->nCos-1 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Min_ManFromGia_rec( Min_Man_t * pNew, Gia_Man_t * p, int iObj )
{
    Gia_Obj_t * pObj = Gia_ManObj(p, iObj);
    if ( ~pObj->Value )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    Min_ManFromGia_rec( pNew, p, Gia_ObjFaninId0(pObj, iObj) );
    Min_ManFromGia_rec( pNew, p, Gia_ObjFaninId1(pObj, iObj) );
    pObj->Value = Min_ManAppendObj( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
}
Min_Man_t * Min_ManFromGia( Gia_Man_t * p, Vec_Int_t * vOuts )
{
    Gia_Obj_t * pObj;  int i;
    Min_Man_t * pNew = Min_ManStart( Gia_ManObjNum(p) );
    Gia_ManFillValue(p);
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Min_ManAppendCi( pNew );
    if ( vOuts == NULL )
    {
        Gia_ManForEachAnd( p, pObj, i )
            pObj->Value = Min_ManAppendObj( pNew, Gia_ObjFaninLit0(pObj, i), Gia_ObjFaninLit1(pObj, i) );
        Gia_ManForEachCo( p, pObj, i )
            pObj->Value = Min_ManAppendCo( pNew, Gia_ObjFaninLit0p(p, pObj) );
    }
    else
    {
        Gia_ManForEachCoVec( vOuts, p, pObj, i )
            Min_ManFromGia_rec( pNew, p, Gia_ObjFaninId0p(p, pObj) );
        Gia_ManForEachCoVec( vOuts, p, pObj, i )
            Min_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    }
    return pNew;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline char Min_XsimNot( char Val )   
{ 
    if ( Val < 2 )
        return Val ^ 1;
    return 2;
}
static inline char Min_XsimXor( char Val0, char Val1 )   
{ 
    if ( Val0 < 2 && Val1 < 2 )
        return Val0 ^ Val1;
    return 2;
}
static inline char Min_XsimAnd( char Val0, char Val1 )   
{ 
    if ( Val0 == 0 || Val1 == 0 )
        return 0;
    if ( Val0 == 1 && Val1 == 1 )
        return 1;
    return 2;
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char Min_LitVerify_rec( Min_Man_t * p, int iLit )
{
    char Val = Min_LitValL(p, iLit);
    if ( Val == 2 && Min_LitIsNode(p, iLit) ) // unassigned
    {
        int iLit0 = Min_LitFan(p, iLit);
        int iLit1 = Min_LitFan(p, iLit^1);
        char Val0 = Min_LitVerify_rec( p, iLit0 );
        char Val1 = Min_LitVerify_rec( p, iLit1 );
        assert( Val0 < 3 && Val1 < 3 );
        if ( Min_LitIsXor(iLit, iLit0, iLit1) )
            Val = Min_XsimXor( Val0, Val1 );
        else
            Val = Min_XsimAnd( Val0, Val1 );
        if ( Val < 2 )
        {
            Val ^= Abc_LitIsCompl(iLit);
            Min_LitSetValL( p, iLit, Val );
        }
        else
            Vec_IntPush( &p->vVis, Abc_Lit2Var(iLit) );
        Min_ObjMark2ValL( p, Abc_Lit2Var(iLit) );
    }
    return Val&3;
}
char Min_LitVerify( Min_Man_t * p, int iLit, Vec_Int_t * vLits )
{
    int i, Entry; char Res;
    if ( iLit < 2 ) return 1;
    assert( !Min_LitIsCo(p, iLit) );
    //assert( Min_ManCheckCleanValsL(p) );
    assert( Vec_IntSize(&p->vVis) == 0 );
    Vec_IntForEachEntry( vLits, Entry, i )
        Min_LitSetValL( p, Entry, 1 ); // ms notation
    Res = Min_LitVerify_rec( p, iLit );
    Min_ManCleanVisitedValL( p );
    return Res;
}

void Min_LitMinimize( Min_Man_t * p, int iLit, Vec_Int_t * vLits )
{
    int i, iObj, iTemp; char Res;
    Vec_IntClear( &p->vPat );
    if ( iLit < 2 ) return;
    assert( !Min_LitIsCo(p, iLit) );
    //assert( Min_ManCheckCleanValsL(p) );
    assert( Vec_IntSize(&p->vVis) == 0 );
    Vec_IntForEachEntry( vLits, iTemp, i )
        Min_LitSetValL( p, iTemp, 1 ); // ms notation
    Res = Min_LitVerify_rec( p, iLit );
    assert( Res == 1 );
    Min_ObjMarkValL( p, Abc_Lit2Var(iLit) );
    Vec_IntForEachEntryReverse( &p->vVis, iObj, i )
    {
        int iLit = Abc_Var2Lit( iObj, 0 );
        int Value = 7 & Min_LitValL(p, iLit);
        if ( Value >= 4 )
        {
            if ( Min_LitIsCi(p, iLit) )
                Vec_IntPush( &p->vPat, Abc_LitNotCond(iLit, !(Value&1)) );
            else
            {
                int iLit0 = Min_LitFan(p, iLit);
                int iLit1 = Min_LitFan(p, iLit^1);
                char Val0 = Min_LitValL( p, iLit0 );
                char Val1 = Min_LitValL( p, iLit1 );
                if ( Value&1 ) // value == 1
                {
                    assert( (Val0&1) && (Val1&1) );
                    Min_ObjMarkValL( p, Abc_Lit2Var(iLit0) );
                    Min_ObjMarkValL( p, Abc_Lit2Var(iLit1) );
                }
                else // value == 0
                {
                    int Zero0 = !(Val0&3);
                    int Zero1 = !(Val1&3);
                    assert( Zero0 || Zero1 );
                    if ( Zero0 && !Zero1 )
                        Min_ObjMarkValL( p, Abc_Lit2Var(iLit0) );
                    else if ( !Zero0 && Zero1 )
                        Min_ObjMarkValL( p, Abc_Lit2Var(iLit1) );
                    else if ( Val0 == 4 && Val1 != 4 )
                        Min_ObjMarkValL( p, Abc_Lit2Var(iLit0) );
                    else if ( Val1 == 4 && Val0 != 4 )
                        Min_ObjMarkValL( p, Abc_Lit2Var(iLit1) );
                    else if ( Abc_Random(0) & 1 )
                        Min_ObjMarkValL( p, Abc_Lit2Var(iLit0) );
                    else
                        Min_ObjMarkValL( p, Abc_Lit2Var(iLit1) );
                }
            }
        }
        Min_ObjCleanValL( p, Abc_Lit2Var(iLit) );
    }
    Vec_IntClear( &p->vVis );
    //Min_ManCleanVisitedValL( p );
    //assert( Min_LitVerify(p, iLit, &p->vPat) == 1 );
    assert( Vec_IntSize(&p->vPat) <= Vec_IntSize(vLits) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline char Min_LitIsImplied1( Min_Man_t * p, int iLit )
{
    char Val = 2;
    int iLit0 = Min_LitFan(p, iLit);
    int iLit1 = Min_LitFan(p, iLit^1);
    char Val0 = Min_LitValL(p, iLit0);
    char Val1 = Min_LitValL(p, iLit1);
    assert( Min_LitIsNode(p, iLit) );    // internal node
    assert( Min_LitValL(p, iLit) == 2 ); // unassigned
    if ( Min_LitIsXor(iLit, iLit0, iLit1) )
        Val = Min_XsimXor( Val0, Val1 );
    else
        Val = Min_XsimAnd( Val0, Val1 );
    if ( Val < 2 )
    {
        Val ^= Abc_LitIsCompl(iLit);
        Min_LitSetValL( p, iLit, Val );
    }
    return Val;
}
static inline char Min_LitIsImplied2( Min_Man_t * p, int iLit )
{
    char Val = 2;
    int iLit0 = Min_LitFan(p, iLit);
    int iLit1 = Min_LitFan(p, iLit^1);
    char Val0 = Min_LitValL(p, iLit0);
    char Val1 = Min_LitValL(p, iLit1);
    assert( Min_LitIsNode(p, iLit) );    // internal node
    assert( Min_LitValL(p, iLit) == 2 ); // unassigned
    if ( Val0 == 2 && Min_LitIsNode(p, iLit0) )
        Val0 = Min_LitIsImplied1(p, iLit0);
    if ( Val1 == 2 && Min_LitIsNode(p, iLit1) )
        Val1 = Min_LitIsImplied1(p, iLit1);
    if ( Min_LitIsXor(iLit, iLit0, iLit1) )
        Val = Min_XsimXor( Val0, Val1 );
    else
        Val = Min_XsimAnd( Val0, Val1 );
    if ( Val < 2 )
    {
        Val ^= Abc_LitIsCompl(iLit);
        Min_LitSetValL( p, iLit, Val );
    }
    return Val;
}
static inline char Min_LitIsImplied3( Min_Man_t * p, int iLit )
{
    char Val = 2;
    int iLit0 = Min_LitFan(p, iLit);
    int iLit1 = Min_LitFan(p, iLit^1);
    char Val0 = Min_LitValL(p, iLit0);
    char Val1 = Min_LitValL(p, iLit1);
    assert( Min_LitIsNode(p, iLit) );    // internal node
    assert( Min_LitValL(p, iLit) == 2 ); // unassigned
    if ( Val0 == 2 && Min_LitIsNode(p, iLit0) )
        Val0 = Min_LitIsImplied2(p, iLit0);
    if ( Val1 == 2 && Min_LitIsNode(p, iLit1) )
        Val1 = Min_LitIsImplied2(p, iLit1);
    if ( Min_LitIsXor(iLit, iLit0, iLit1) )
        Val = Min_XsimXor( Val0, Val1 );
    else
        Val = Min_XsimAnd( Val0, Val1 );
    if ( Val < 2 )
    {
        Val ^= Abc_LitIsCompl(iLit);
        Min_LitSetValL( p, iLit, Val );
    }
    return Val;
}
static inline char Min_LitIsImplied4( Min_Man_t * p, int iLit )
{
    char Val = 2;
    int iLit0 = Min_LitFan(p, iLit);
    int iLit1 = Min_LitFan(p, iLit^1);
    char Val0 = Min_LitValL(p, iLit0);
    char Val1 = Min_LitValL(p, iLit1);
    assert( Min_LitIsNode(p, iLit) );    // internal node
    assert( Min_LitValL(p, iLit) == 2 ); // unassigned
    if ( Val0 == 2 && Min_LitIsNode(p, iLit0) )
        Val0 = Min_LitIsImplied3(p, iLit0);
    if ( Val1 == 2 && Min_LitIsNode(p, iLit1) )
        Val1 = Min_LitIsImplied3(p, iLit1);
    if ( Min_LitIsXor(iLit, iLit0, iLit1) )
        Val = Min_XsimXor( Val0, Val1 );
    else
        Val = Min_XsimAnd( Val0, Val1 );
    if ( Val < 2 )
    {
        Val ^= Abc_LitIsCompl(iLit);
        Min_LitSetValL( p, iLit, Val );
    }
    return Val;
}
static inline char Min_LitIsImplied5( Min_Man_t * p, int iLit )
{
    char Val = 2;
    int iLit0 = Min_LitFan(p, iLit);
    int iLit1 = Min_LitFan(p, iLit^1);
    char Val0 = Min_LitValL(p, iLit0);
    char Val1 = Min_LitValL(p, iLit1);
    assert( Min_LitIsNode(p, iLit) );    // internal node
    assert( Min_LitValL(p, iLit) == 2 ); // unassigned
    if ( Val0 == 2 && Min_LitIsNode(p, iLit0) )
        Val0 = Min_LitIsImplied4(p, iLit0);
    if ( Val1 == 2 && Min_LitIsNode(p, iLit1) )
        Val1 = Min_LitIsImplied4(p, iLit1);
    if ( Min_LitIsXor(iLit, iLit0, iLit1) )
        Val = Min_XsimXor( Val0, Val1 );
    else
        Val = Min_XsimAnd( Val0, Val1 );
    if ( Val < 2 )
    {
        Val ^= Abc_LitIsCompl(iLit);
        Min_LitSetValL( p, iLit, Val );
    }
    return Val;
}

// this recursive procedure is about 10% slower
char Min_LitIsImplied_rec( Min_Man_t * p, int iLit, int Depth )
{
    char Val = 2;
    int iLit0 = Min_LitFan(p, iLit);
    int iLit1 = Min_LitFan(p, iLit^1);
    char Val0 = Min_LitValL(p, iLit0);
    char Val1 = Min_LitValL(p, iLit1);
    assert( Depth > 0 );
    assert( Min_LitIsNode(p, iLit) );    // internal node
    assert( Min_LitValL(p, iLit) == 2 ); // unassigned
    if ( Depth > 1 && Val0 == 2 && Min_LitIsNode(p, iLit0) )
    {
        Val0 = Min_LitIsImplied_rec(p, iLit0, Depth-1);
        Val1 = Min_LitValL(p, iLit1);
    }
    if ( Depth > 1 && Val1 == 2 && Min_LitIsNode(p, iLit1) )
    {
        Val1 = Min_LitIsImplied_rec(p, iLit1, Depth-1);
        Val0 = Min_LitValL(p, iLit0);
    }
    if ( Min_LitIsXor(iLit, iLit0, iLit1) )
        Val = Min_XsimXor( Val0, Val1 );
    else
        Val = Min_XsimAnd( Val0, Val1 );
    if ( Val < 2 )
    {
        Val ^= Abc_LitIsCompl(iLit);
        Min_LitSetValL( p, iLit, Val );
    }
    return Val;
}
int Min_LitJustify_rec( Min_Man_t * p, int iLit )
{
    int Res = 1, LitValue = !Abc_LitIsCompl(iLit);
    int Val = (int)Min_LitValL(p, iLit);
    if ( Val < 2 ) // assigned
        return Val == LitValue;
    // unassigned
    if ( Min_LitIsCi(p, iLit) )
        Vec_IntPush( &p->vPat, iLit ); // ms notation
    else
    {
        int iLit0 = Min_LitFan(p, iLit);
        int iLit1 = Min_LitFan(p, iLit^1);
        char Val0 = Min_LitValL(p, iLit0);
        char Val1 = Min_LitValL(p, iLit1);
        if ( Min_LitIsXor(iLit, iLit0, iLit1) )
        {
            if ( Val0 < 2 && Val1 < 2 )
                Res = LitValue == (Val0 ^ Val1);
            else if ( Val0 < 2 )
                Res = Min_LitJustify_rec(p, iLit1^Val0^!LitValue);
            else if ( Val1 < 2 )
                Res = Min_LitJustify_rec(p, iLit0^Val1^!LitValue);
            else if ( Abc_Random(0) & 1 )
                Res = Min_LitJustify_rec(p, iLit0)   && Min_LitJustify_rec(p, iLit1^ LitValue);
            else
                Res = Min_LitJustify_rec(p, iLit0^1) && Min_LitJustify_rec(p, iLit1^!LitValue);
            assert( !Res || LitValue == Min_XsimXor(Min_LitValL(p, iLit0), Min_LitValL(p, iLit1)) );
        }
        else if ( LitValue ) // value 1
        {
            if ( Val0 == 0 || Val1 == 0 )
                Res = 0;
            else if ( Val0 == 1 && Val1 == 1 )
                Res = 1;
            else if ( Val0 == 1 )
                Res = Min_LitJustify_rec(p, iLit1);
            else if ( Val1 == 1 )
                Res = Min_LitJustify_rec(p, iLit0);
            else 
                Res = Min_LitJustify_rec(p, iLit0) && Min_LitJustify_rec(p, iLit1);
            assert( !Res || 1 == Min_XsimAnd(Min_LitValL(p, iLit0), Min_LitValL(p, iLit1)) );
        }
        else // value 0
        {
/*
            int Depth = 3;
            if ( Val0 == 2 && Min_LitIsNode(p, iLit0) )
            {
                Val0 = Min_LitIsImplied_rec(p, iLit0, Depth);
                Val1 = Min_LitValL(p, iLit1);
            }
            if ( Val1 == 2 && Min_LitIsNode(p, iLit1) )
            {
                Val1 = Min_LitIsImplied_rec(p, iLit1, Depth);
                Val0 = Min_LitValL(p, iLit0);
            }
*/
            if ( Val0 == 2 && Min_LitIsNode(p, iLit0) )
            {
                Val0 = Min_LitIsImplied3(p, iLit0);
                Val1 = Min_LitValL(p, iLit1);
            }
            if ( Val1 == 2 && Min_LitIsNode(p, iLit1) )
            {
                Val1 = Min_LitIsImplied3(p, iLit1);
                Val0 = Min_LitValL(p, iLit0);
            }
            if ( Val0 == 0 || Val1 == 0 )
                Res = 1;
            else if ( Val0 == 1 && Val1 == 1 )
                Res = 0;
            else if ( Val0 == 1 )
                Res = Min_LitJustify_rec(p, iLit1^1);
            else if ( Val1 == 1 )
                Res = Min_LitJustify_rec(p, iLit0^1);
            else if ( Abc_Random(0) & 1 )
            //else if ( (p->Random >> (iLit & 0x1F)) & 1 )
                Res = Min_LitJustify_rec(p, iLit0^1);
            else
                Res = Min_LitJustify_rec(p, iLit1^1);
            //Val0 = Min_LitValL(p, iLit0);
            //Val1 = Min_LitValL(p, iLit1);
            assert( !Res || 0 == Min_XsimAnd(Min_LitValL(p, iLit0), Min_LitValL(p, iLit1)) );
        }
    }
    if ( Res )
        Min_LitSetValL( p, iLit, 1 );
    return Res;
}
int Min_LitJustify( Min_Man_t * p, int iLit )
{ 
    int Res, fCheck = 0;
    Vec_IntClear( &p->vPat );
    if ( iLit < 2 )  return 1;
    assert( !Min_LitIsCo(p, iLit) );
    //assert( Min_ManCheckCleanValsL(p) );
    assert( Vec_IntSize(&p->vVis) == 0 );
    //p->Random = Abc_Random(0);
    Res = Min_LitJustify_rec( p, iLit );
    Min_ManCleanVisitedValL( p );
    if ( Res )
    {
        if ( fCheck && Min_LitVerify(p, iLit, &p->vPat) != 1 )
            printf( "Verification FAILED for literal %d.\n", iLit );
        //else
        //    printf( "Verification succeeded for literal %d.\n", iLit );
    }
    //else
    //    printf( "Could not justify literal %d.\n", iLit );
    return Res;
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Min_TargGenerateCexes( Min_Man_t * p, Vec_Int_t * vCoErrs, int nCexes, int nCexesStop, int * pnComputed, int fVerbose )
{
    abctime clk = Abc_Clock(); 
    int t, iObj, Count = 0, CountPos = 0, CountPosSat = 0, nRuns[2] = {0}, nCountCexes[2] = {0};
    Vec_Int_t * vPats    = Vec_IntAlloc( 1000 );
    Vec_Int_t * vPatBest = Vec_IntAlloc( Min_ManCiNum(p) );
    Hsh_VecMan_t * pHash = Hsh_VecManStart( 10000 );
    Min_ManForEachCo( p, iObj ) if ( Min_ObjLit0(p, iObj) > 1 )
    {
        int nCexesGenSim0 = 0;
        int nCexesGenSim = 0;
        int nCexesGenSat = 0;
        if ( vCoErrs && Vec_IntEntry(vCoErrs, Min_ObjCioId(p, iObj)) >= nCexesStop )
            continue;
        //printf( "%d ", i );
        for ( t = 0; t < nCexes; t++ )
        {
            nRuns[0]++;
            if ( Min_LitJustify( p, Min_ObjLit0(p, iObj) )  )
            {
                int Before, After;
                assert( Vec_IntSize(&p->vPat) > 0 );
                //printf( "%d   ", Vec_IntSize(vPat) );
                Vec_IntClear( vPatBest );
                if ( 1 ) // no minimization 
                    Vec_IntAppend( vPatBest, &p->vPat );
                else
                {
/*
                    for ( k = 0; k < 10; k++ )
                    {
                        Vec_IntClear( vPat2 );
                        Gia_ManIncrementTravId( p );
                        Cexes_MinimizePattern_rec( p, Gia_ObjFanin0(pObj), !Gia_ObjFaninC0(pObj), vPat2 );
                        assert( Vec_IntSize(vPat2) <= Vec_IntSize(vPat) );
                        if ( Vec_IntSize(vPatBest) == 0 || Vec_IntSize(vPatBest) > Vec_IntSize(vPat2) )
                        {
                            Vec_IntClear( vPatBest );
                            Vec_IntAppend( vPatBest, vPat2 );
                        }
                        //printf( "%d ", Vec_IntSize(vPat2) );
                    }
*/
                }

                //Gia_CexVerify( p, Gia_ObjFaninId0p(p, pObj), !Gia_ObjFaninC0(pObj), vPatBest );
                //printf( "\n" );
                Before = Hsh_VecSize( pHash );
                Vec_IntSort( vPatBest, 0 );
                Hsh_VecManAdd( pHash, vPatBest );
                After = Hsh_VecSize( pHash );
                if ( Before != After )
                {
                    Vec_IntPush( vPats, Min_ObjCioId(p, iObj) );
                    Vec_IntPush( vPats, Vec_IntSize(vPatBest) );
                    Vec_IntAppend( vPats, vPatBest );
                    nCexesGenSim++;
                }
                nCexesGenSim0++;
                if ( nCexesGenSim0 > nCexesGenSim*10 )
                {
                    printf( "**** Skipping output %d (out of %d)\n", Min_ObjCioId(p, iObj), Min_ManCoNum(p) );
                    break;
                }
            }
            if ( nCexesGenSim == nCexesStop )
                break;
        }
        //printf( "(%d %d)  ", nCexesGenSim0, nCexesGenSim );
        //printf( "%d ", t/nCexesGenSim );

        //printf( "The number of CEXes = %d\n", nCexesGen );
        //if ( fVerbose )
        //    printf( "%d ", nCexesGen );
        nCountCexes[0] += nCexesGenSim;
        nCountCexes[1] += nCexesGenSat;
        Count += nCexesGenSim + nCexesGenSat;
        CountPos++;

        if ( nCexesGenSim0 == 0 && t == nCexes )
            printf( "#### Output %d (out of %d)\n", Min_ObjCioId(p, iObj), Min_ManCoNum(p) );
    }
    //printf( "\n" );
    if ( fVerbose )
    printf( "\n" );
    if ( fVerbose )
    printf( "Got %d unique CEXes using %d sim (%d) and %d SAT (%d) runs (ave size %.1f). PO = %d  ErrPO = %d  SatPO = %d  ", 
        Count, nRuns[0], nCountCexes[0], nRuns[1], nCountCexes[1], 
        1.0*Vec_IntSize(vPats)/Abc_MaxInt(1, Count)-2, Min_ManCoNum(p), CountPos, CountPosSat );
    if ( fVerbose )
        Abc_PrintTime( 0, "Time", Abc_Clock() - clk );
    Hsh_VecManStop( pHash );
    Vec_IntFree( vPatBest );
    *pnComputed = Count;
    return vPats;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Min_ManTest3( Gia_Man_t * p, Vec_Int_t * vCoErrs )
{
    int fXor = 0;
    int nComputed;
    Vec_Int_t * vPats;
    Gia_Man_t * pXor = fXor ? Gia_ManDupMuxes(p, 1) : NULL;
    Min_Man_t * pNew = Min_ManFromGia( fXor ? pXor : p, NULL );
    Gia_ManStopP( &pXor );
    Min_ManStartValsL( pNew );
    //Vec_IntFill( vCoErrs, Vec_IntSize(vCoErrs), 0 );
    //vPats = Min_TargGenerateCexes( pNew, vCoErrs, 10000, 10, &nComputed, 1 );
    vPats = Min_TargGenerateCexes( pNew, vCoErrs, 10000, 10, &nComputed, 1 );
    Vec_IntFree( vPats );
    Min_ManStop( pNew );
}
void Min_ManTest4( Gia_Man_t * p )
{
    Vec_Int_t * vCoErrs = Vec_IntStartNatural( Gia_ManCoNum(p) );
    Min_ManTest3(p, vCoErrs);
    Vec_IntFree( vCoErrs );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDupCones2CollectPis_rec( Gia_Man_t * p, int iObj, Vec_Int_t * vMap )
{
    Gia_Obj_t * pObj;
    if ( Gia_ObjUpdateTravIdCurrentId(p, iObj) )
        return;
    pObj = Gia_ManObj( p, iObj );
    if ( Gia_ObjIsAnd(pObj) )
    {
        Gia_ManDupCones2CollectPis_rec( p, Gia_ObjFaninId0(pObj, iObj), vMap );
        Gia_ManDupCones2CollectPis_rec( p, Gia_ObjFaninId1(pObj, iObj), vMap );
    }
    else if ( Gia_ObjIsCi(pObj) )
        Vec_IntPush( vMap, iObj );
    else assert( 0 );
}
void Gia_ManDupCones2_rec( Gia_Man_t * pNew, Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( Gia_ObjIsCi(pObj) || Gia_ObjUpdateTravIdCurrent(p, pObj) )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManDupCones2_rec( pNew, p, Gia_ObjFanin0(pObj) );
    Gia_ManDupCones2_rec( pNew, p, Gia_ObjFanin1(pObj) );
    pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
}
Gia_Man_t * Gia_ManDupCones2( Gia_Man_t * p, int * pOuts, int nOuts, Vec_Int_t * vMap )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj; int i;
    Vec_IntClear( vMap );
    Gia_ManIncrementTravId( p );
    for ( i = 0; i < nOuts; i++ )
        Gia_ManDupCones2CollectPis_rec( p, Gia_ManCoDriverId(p, pOuts[i]), vMap );
    pNew = Gia_ManStart( 1000 );
    pNew->pName = Abc_UtilStrsav( p->pName );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachObjVec( vMap, p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManIncrementTravId( p );
    for ( i = 0; i < nOuts; i++ )
        Gia_ManDupCones2_rec( pNew, p, Gia_ObjFanin0(Gia_ManCo(p, pOuts[i])) );
    for ( i = 0; i < nOuts; i++ )
        Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(Gia_ManCo(p, pOuts[i])) );
    return pNew;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Min_ManRemoveItem( Vec_Wec_t * vCexes, int iItem, int iFirst, int iLimit )
{
    Vec_Int_t * vLevel = NULL, * vLevel0 = Vec_WecEntry(vCexes, iItem);  int i;
    assert( iFirst <= iItem && iItem < iLimit );
    Vec_WecForEachLevelReverseStartStop( vCexes, vLevel, i, iLimit, iFirst )
        if ( Vec_IntSize(vLevel) > 0 )
            break;
    assert( iFirst <= i && iItem <= i );
    Vec_IntClear( vLevel0 );
    if ( iItem < i )
        ABC_SWAP( Vec_Int_t, *vLevel0, *vLevel );
    return -1;
}
int Min_ManAccumulate( Vec_Wec_t * vCexes, int iFirst, int iLimit, Vec_Int_t * vCex )
{
    Vec_Int_t * vLevel;  int i, nCommon, nDiff = 0;
    Vec_WecForEachLevelStartStop( vCexes, vLevel, i, iFirst, iLimit )
    {
        if ( Vec_IntSize(vLevel) == 0 )
        {
            Vec_IntAppend(vLevel, vCex);
            return nDiff+1;
        }
        nCommon = Vec_IntTwoCountCommon( vLevel, vCex );
        if ( nCommon == Vec_IntSize(vLevel) ) // ignore vCex
            return nDiff;
        if ( nCommon == Vec_IntSize(vCex) ) // remove vLevel
            nDiff += Min_ManRemoveItem( vCexes, i, iFirst, iLimit );
    }
    assert( 0 );
    return ABC_INFINITY;
}
int Min_ManCountSize( Vec_Wec_t * vCexes, int iFirst, int iLimit )
{
    Vec_Int_t * vLevel;  int i, nTotal = 0;
    Vec_WecForEachLevelStartStop( vCexes, vLevel, i, iFirst, iLimit )
        nTotal += Vec_IntSize(vLevel) > 0;
    return nTotal;
}
Vec_Wec_t * Min_ManComputeCexes( Gia_Man_t * p, Vec_Int_t * vOuts0, int nMaxTries, int nMinCexes, Vec_Int_t * vStats[3], int fUseSim, int fUseSat, int fVerbose )
{
    int fUseSynthesis  = 1;
    abctime clkSim = Abc_Clock(), clkSat = Abc_Clock();
    Vec_Int_t * vOuts  = vOuts0 ? vOuts0 : Vec_IntStartNatural( Gia_ManCoNum(p) );
    Min_Man_t * pNew   = Min_ManFromGia( p, vOuts ); 
    Vec_Wec_t * vCexes = Vec_WecStart( Vec_IntSize(vOuts) * nMinCexes );
    Vec_Int_t * vPatBest = Vec_IntAlloc( 100 );
    Vec_Int_t * vLits = Vec_IntAlloc( 100 );
    Gia_Obj_t * pObj; int i, iObj, nOuts = 0, nSimOuts = 0, nSatOuts = 0;
    vStats[0] = Vec_IntAlloc( Vec_IntSize(vOuts) ); // total calls
    vStats[1] = Vec_IntAlloc( Vec_IntSize(vOuts) ); // successful calls + SAT runs
    vStats[2] = Vec_IntAlloc( Vec_IntSize(vOuts) ); // results
    Min_ManStartValsL( pNew );
    Min_ManForEachCo( pNew, iObj )
    {
        int nAllCalls  = 0;
        int nGoodCalls = 0;
        int nCurrCexes = 0;
        if ( fUseSim && Min_ObjLit0(pNew, iObj) >= 2 )
        {
            while ( nAllCalls++ < nMaxTries )
            {
                if ( Min_LitJustify( pNew, Min_ObjLit0(pNew, iObj) ) )
                {
                    Vec_IntClearAppend( vLits, &pNew->vPat );
                    Vec_IntClearAppend( vPatBest, &pNew->vPat );
                    if ( 1 ) // minimization 
                    {
                        //printf( "%d -> ", Vec_IntSize(vPatBest) );
                        for ( i = 0; i < 20; i++ )
                        {
                            Min_LitMinimize( pNew, Min_ObjLit0(pNew, iObj), vLits );
                            if ( Vec_IntSize(vPatBest) > Vec_IntSize(&pNew->vPat) )
                                Vec_IntClearAppend( vPatBest, &pNew->vPat );
                        }
                        //printf( "%d   ", Vec_IntSize(vPatBest) );
                    }
                    assert( Vec_IntSize(vPatBest) > 0 );
                    Vec_IntSort( vPatBest, 0 );
                    nCurrCexes += Min_ManAccumulate( vCexes, nOuts*nMinCexes, (nOuts+1)*nMinCexes, vPatBest );
                    nGoodCalls++;
                }
                if ( nCurrCexes == nMinCexes || nGoodCalls > 10*nCurrCexes )
                    break;
            }
            nSimOuts++;
        }
        assert( nCurrCexes <= nMinCexes );
        assert( nCurrCexes == Min_ManCountSize(vCexes, nOuts*nMinCexes, (nOuts+1)*nMinCexes) );
        Vec_IntPush( vStats[0], nAllCalls );
        Vec_IntPush( vStats[1], nGoodCalls );
        Vec_IntPush( vStats[2], nCurrCexes );
        nOuts++;
    }
    assert( Vec_IntSize(vOuts) == nOuts );
    assert( Vec_IntSize(vOuts) == Vec_IntSize(vStats[0]) );
    assert( Vec_IntSize(vOuts) == Vec_IntSize(vStats[1]) );
    assert( Vec_IntSize(vOuts) == Vec_IntSize(vStats[2]) );
    clkSim = Abc_Clock() - clkSim;

    if ( fUseSat )
    Gia_ManForEachCoVec( vOuts, p, pObj, i )
    {
        if ( Vec_IntEntry(vStats[2], i) >= nMinCexes || Vec_IntEntry(vStats[1], i) > 10*Vec_IntEntry(vStats[2], i) )
            continue;
        {
            abctime clk = Abc_Clock();
            int iObj  = Min_ManCo(pNew, i);
            int Index = Gia_ObjCioId(pObj);
            Vec_Int_t * vMap = Vec_IntAlloc( 100 );
            Gia_Man_t * pCon = Gia_ManDupCones2( p, &Index, 1, vMap );
            Gia_Man_t * pCon1= fUseSynthesis ? Gia_ManAigSyn2( pCon, 0, 1, 0, 100, 0, 0, 0 ) : NULL;
            Cnf_Dat_t * pCnf = (Cnf_Dat_t *)Mf_ManGenerateCnf( fUseSynthesis ? pCon1 : pCon, 8, 0, 0, 0, 0 );
            sat_solver* pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
            int Lit          = Abc_Var2Lit( 1, 0 );
            int status       = sat_solver_addclause( pSat, &Lit, &Lit+1 );
            int nAllCalls    = 0;
            int nCurrCexes   = Vec_IntEntry(vStats[2], i);
            //Gia_AigerWrite( pCon, "temp_miter.aig", 0, 0, 0 );
            if ( status == l_True )
            {
                nSatOuts++;
                //printf( "Running SAT for output %d\n", i );
                if ( Min_ObjLit0(pNew, iObj) >= 2 )
                {
                    while ( nAllCalls++ < 100 )
                    {
                        int v, iVar = pCnf->nVars - Gia_ManPiNum(pCon), nVars = Gia_ManPiNum(pCon);
                        if ( nAllCalls > 1 )
                            sat_solver_randomize( pSat, iVar, nVars );
                        status = sat_solver_solve( pSat, NULL, NULL, 0, 0, 0, 0 );
                        if ( status != l_True )
                            break;
                        assert( status == l_True );
                        Vec_IntClear( vLits );
                        for ( v = 0; v < nVars; v++ )
                            Vec_IntPush( vLits, Abc_Var2Lit(Vec_IntEntry(vMap, v), !sat_solver_var_value(pSat, iVar + v)) );
                        Min_LitMinimize( pNew, Min_ObjLit0(pNew, iObj), vLits );
                        Vec_IntClearAppend( vPatBest, &pNew->vPat );
                        if ( 1 ) // minimization 
                        {
                            //printf( "%d -> ", Vec_IntSize(vPatBest) );
                            for ( v = 0; v < 20; v++ )
                            {
                                Min_LitMinimize( pNew, Min_ObjLit0(pNew, iObj), vLits );
                                if ( Vec_IntSize(vPatBest) > Vec_IntSize(&pNew->vPat) )
                                    Vec_IntClearAppend( vPatBest, &pNew->vPat );
                            }
                            //printf( "%d   ", Vec_IntSize(vPatBest) );
                        }
                        Vec_IntSort( vPatBest, 0 );
                        nCurrCexes += Min_ManAccumulate( vCexes, i*nMinCexes, (i+1)*nMinCexes, vPatBest );
                        if ( nCurrCexes == nMinCexes || nAllCalls > 10*nCurrCexes )
                            break;
                    }
                }
            }
            Vec_IntWriteEntry( vStats[0], i, nAllCalls*nMaxTries );
            Vec_IntWriteEntry( vStats[1], i, nAllCalls*nMaxTries );
            Vec_IntWriteEntry( vStats[2], i, nCurrCexes );
            sat_solver_delete( pSat );
            Cnf_DataFree( pCnf );
            Gia_ManStop( pCon );
            Gia_ManStopP( &pCon1 );
            Vec_IntFree( vMap );
            if ( fVerbose )
            {
                printf( "SAT solving for output %3d (cexes = %5d) : ", i, nCurrCexes );
                Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
            }
        }
    }
    clkSat = Abc_Clock() - clkSat - clkSim;
    if ( fVerbose )
        printf( "Used simulation for %d and SAT for %d outputs (out of %d).\n", nSimOuts, nSatOuts, nOuts );
    if ( fVerbose )
        Abc_PrintTime( 1, "Simulation time  ", clkSim );
    if ( fVerbose )
        Abc_PrintTime( 1, "SAT solving time ", clkSat );
    //Vec_WecPrint( vCexes, 0 );
    if ( vOuts != vOuts0 )
        Vec_IntFreeP( &vOuts );
    Min_ManStop( pNew );
    Vec_IntFree( vPatBest );
    Vec_IntFree( vLits );
    return vCexes;
}

/**Function*************************************************************

  Synopsis    [Bit-packing for selected patterns.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Min_ManBitPackTry( Vec_Wrd_t * vSimsPi, int nWords, int iPat, Vec_Int_t * vLits )
{
    int i, Lit;
    assert( iPat >= 0 && iPat < 64 * nWords );
    Vec_IntForEachEntry( vLits, Lit, i )
    {
        word * pInfo = Vec_WrdEntryP( vSimsPi, nWords * Abc_Lit2Var(Lit-2) );   // Lit is based on ObjId
        word * pCare = pInfo + Vec_WrdSize(vSimsPi);
        if ( Abc_InfoHasBit( (unsigned *)pCare, iPat ) && 
             Abc_InfoHasBit( (unsigned *)pInfo, iPat ) == Abc_LitIsCompl(Lit) ) // Lit is in ms notation
             return 0;
    }
    Vec_IntForEachEntry( vLits, Lit, i )
    {
        word * pInfo = Vec_WrdEntryP( vSimsPi, nWords * Abc_Lit2Var(Lit-2) );   // Lit is based on ObjId
        word * pCare = pInfo + Vec_WrdSize(vSimsPi);
        Abc_InfoSetBit( (unsigned *)pCare, iPat );
        if ( Abc_InfoHasBit( (unsigned *)pInfo, iPat ) == Abc_LitIsCompl(Lit) ) // Lit is in ms notation
             Abc_InfoXorBit( (unsigned *)pInfo, iPat );
    }
    return 1;
}
int Min_ManBitPackOne( Vec_Wrd_t * vSimsPi, int iPat0, int nWords, Vec_Int_t * vLits )
{
    int iPat, nTotal = 64*nWords;
    for ( iPat = iPat0 + 1; iPat != iPat0; iPat = (iPat + 1) % nTotal )
        if ( Min_ManBitPackTry( vSimsPi, nWords, iPat, vLits ) )
            break;
    return iPat;
}
Vec_Ptr_t * Min_ReloadCexes( Vec_Wec_t * vCexes, int nMinCexes )
{
    Vec_Ptr_t * vRes = Vec_PtrAlloc( Vec_WecSize(vCexes) );
    int i, c, nOuts = Vec_WecSize(vCexes)/nMinCexes;
    for ( i = 0; i < nMinCexes; i++ )
    for ( c = 0; c < nOuts; c++ )
    {
        Vec_Int_t * vLevel = Vec_WecEntry( vCexes, c*nMinCexes+i );
        if ( Vec_IntSize(vLevel) )
            Vec_PtrPush( vRes, vLevel );
    }
    return vRes;
}

Vec_Wrd_t * Min_ManBitPack( Gia_Man_t * p, int nWords0, Vec_Wec_t * vCexes, int fRandom, int nMinCexes, Vec_Int_t * vScores, int fVerbose )
{
    abctime clk = Abc_Clock();
    //int fVeryVerbose = 0;
    Vec_Wrd_t * vSimsPi = NULL;
    Vec_Int_t * vLevel; 
    int w, nBits, nTotal = 0, fFailed = ABC_INFINITY;
    Vec_Int_t * vOrder  = NULL;
    Vec_Ptr_t * vReload = NULL;
    if ( 0 )
    {
        vOrder = Vec_IntStartNatural( Vec_WecSize(vCexes)/nMinCexes );
        assert( Vec_IntSize(vOrder) == Vec_IntSize(vScores) );
        assert( Vec_WecSize(vCexes)%nMinCexes == 0 );
        Abc_MergeSortCost2Reverse( Vec_IntArray(vOrder), Vec_IntSize(vOrder), Vec_IntArray(vScores) );
    }
    else
        vReload = Min_ReloadCexes( vCexes, nMinCexes );
    if ( fVerbose )
        printf( "Packing: " );
    for ( w = nWords0 ? nWords0 : 1; nWords0 ? w <= nWords0 : fFailed > 0; w++ )
    {
        int i, iPatUsed, iPat = 0;
        //int k, iOut;
        Vec_WrdFreeP( &vSimsPi );
        vSimsPi = fRandom ? Vec_WrdStartRandom(2 * Gia_ManCiNum(p) * w) : Vec_WrdStart(2 * Gia_ManCiNum(p) * w);
        Vec_WrdShrink( vSimsPi, Vec_WrdSize(vSimsPi)/2 );
        Abc_TtClear( Vec_WrdLimit(vSimsPi), Vec_WrdSize(vSimsPi) );
        fFailed = nTotal = 0;
        //Vec_IntForEachEntry( vOrder, iOut, k )
        //Vec_WecForEachLevelStartStop( vCexes, vLevel, i, iOut*nMinCexes, (iOut+1)*nMinCexes )
        Vec_PtrForEachEntry( Vec_Int_t *, vReload, vLevel, i )
        {
            //if ( fVeryVerbose && i%nMinCexes == 0 )
            //    printf( "\n" );
            if ( Vec_IntSize(vLevel) == 0 )
                continue;
            iPatUsed = Min_ManBitPackOne( vSimsPi, iPat, w, vLevel );
            fFailed += iPatUsed == iPat;
            iPat = (iPatUsed + 1) % (64*(w-1) - 1);
            //if ( fVeryVerbose )
            //printf( "Adding output %3d cex %3d to pattern %3d   ", i/nMinCexes, i%nMinCexes, iPatUsed );
            //if ( fVeryVerbose )
            //Vec_IntPrint( vLevel );
            nTotal++;
        }
        if ( fVerbose )
            printf( "W = %d (F = %d)  ", w, fFailed );
//        printf( "Failed patterns = %d\n", fFailed );
    }
    if ( fVerbose )
        printf( "Total = %d\n", nTotal );
    if ( fVerbose )
    {
        nBits = Abc_TtCountOnesVec( Vec_WrdLimit(vSimsPi), Vec_WrdSize(vSimsPi) );
        printf( "Bit-packing is using %d words and %d bits.  Density =%8.4f %%.  ", 
            Vec_WrdSize(vSimsPi)/Gia_ManCiNum(p), nBits, 100.0*nBits/64/Vec_WrdSize(vSimsPi) );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    }
    Vec_IntFreeP( &vOrder );
    Vec_PtrFreeP( &vReload );
    return vSimsPi;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Patt_ManOutputErrorCoverage( Vec_Wrd_t * vErrors, int nOuts )
{
    Vec_Int_t * vCounts = Vec_IntAlloc( nOuts );
    int i, nWords = Vec_WrdSize(vErrors)/nOuts;
    assert( Vec_WrdSize(vErrors) == nOuts * nWords );
    for ( i = 0; i < nOuts; i++ )
        Vec_IntPush( vCounts, Abc_TtCountOnesVec(Vec_WrdEntryP(vErrors, nWords * i), nWords) );
    return vCounts;
}
Vec_Wrd_t * Patt_ManTransposeErrors( Vec_Wrd_t * vErrors, int nOuts )
{
    extern void Extra_BitMatrixTransposeP( Vec_Wrd_t * vSimsIn, int nWordsIn, Vec_Wrd_t * vSimsOut, int nWordsOut );
    int nWordsIn  = Vec_WrdSize(vErrors) / nOuts;
    int nWordsOut = Abc_Bit6WordNum(nOuts);
    Vec_Wrd_t * vSims1 = Vec_WrdStart( 64*nWordsIn*nWordsOut );
    Vec_Wrd_t * vSims2 = Vec_WrdStart( 64*nWordsIn*nWordsOut );
    assert( Vec_WrdSize(vErrors) == nWordsIn * nOuts );
    Abc_TtCopy( Vec_WrdArray(vSims1), Vec_WrdArray(vErrors), Vec_WrdSize(vErrors), 0 );
    Extra_BitMatrixTransposeP( vSims1, nWordsIn, vSims2, nWordsOut );
    Vec_WrdFree( vSims1 );
    return vSims2;
}
Vec_Int_t * Patt_ManPatternErrorCoverage( Vec_Wrd_t * vErrors, int nOuts )
{
    int nWords = Vec_WrdSize(vErrors)/nOuts;
    Vec_Wrd_t * vErrors2 = Patt_ManTransposeErrors( vErrors, nOuts );
    Vec_Int_t * vPatErrs = Patt_ManOutputErrorCoverage( vErrors2, 64*nWords );
    Vec_WrdFree( vErrors2 );
    return vPatErrs;
}

#define ERR_REPT_SIZE 32
void Patt_ManProfileErrors( Vec_Int_t * vOutErrs, Vec_Int_t * vPatErrs )
{
    int nOuts = Vec_IntSize(vOutErrs);
    int nPats = Vec_IntSize(vPatErrs);
    int ErrOuts[ERR_REPT_SIZE+1] = {0};
    int ErrPats[ERR_REPT_SIZE+1] = {0};
    int i, Errs, nErrors1 = 0, nErrors2 = 0;
    Vec_IntForEachEntry( vOutErrs, Errs, i )
    {
        nErrors1 += Errs;
        ErrOuts[Errs < ERR_REPT_SIZE ? Errs : ERR_REPT_SIZE]++;
    }
    Vec_IntForEachEntry( vPatErrs, Errs, i )
    {
        nErrors2 += Errs;
        ErrPats[Errs < ERR_REPT_SIZE ? Errs : ERR_REPT_SIZE]++;
    }
    assert( nErrors1 == nErrors2 );
    // errors/error_outputs/error_patterns
    //printf( "\nError statistics:\n" );
    printf( "Errors =%6d  ", nErrors1 );
    printf( "ErrPOs =%5d  (Ave = %5.2f)    ",  nOuts-ErrOuts[0], 1.0*nErrors1/Abc_MaxInt(1, nOuts-ErrOuts[0]) );
    printf( "Patterns =%5d  (Ave = %5.2f)   ", nPats, 1.0*nErrors1/nPats );
    printf( "Density =%8.4f %%\n",             100.0*nErrors1/nPats/Abc_MaxInt(1, nOuts-ErrOuts[0]) );
    // how many times each output fails
    printf( "Outputs: " );
    for ( i = 0; i <= ERR_REPT_SIZE; i++ )
        if ( ErrOuts[i] )
            printf( "%s%d=%d ", i == ERR_REPT_SIZE? ">" : "", i, ErrOuts[i] );
    printf( "\n" );
    // how many times each patterns fails an output
    printf( "Patterns: " );
    for ( i = 0; i <= ERR_REPT_SIZE; i++ )
        if ( ErrPats[i] )
            printf( "%s%d=%d ", i == ERR_REPT_SIZE? ">" : "", i, ErrPats[i] );
    printf( "\n" );
}
int Patt_ManProfileErrorsOne( Vec_Wrd_t * vErrors, int nOuts )
{
    Vec_Int_t * vCoErrs  = Patt_ManOutputErrorCoverage( vErrors, nOuts );
    Vec_Int_t * vPatErrs = Patt_ManPatternErrorCoverage( vErrors, nOuts );
    Patt_ManProfileErrors( vCoErrs, vPatErrs );
    Vec_IntFree( vPatErrs );
    Vec_IntFree( vCoErrs );
    return 1;
}

Vec_Int_t * Min_ManGetUnsolved( Gia_Man_t * p )
{
    Vec_Int_t * vRes = Vec_IntAlloc( 100 );
    int i, Driver;
    Gia_ManForEachCoDriverId( p, Driver, i )
        if ( Driver > 0 )
            Vec_IntPush( vRes, i );
    if ( Vec_IntSize(vRes) == 0 )
        Vec_IntFreeP( &vRes );
    return vRes;
}
Vec_Wrd_t * Min_ManRemapSims( int nInputs, Vec_Int_t * vMap, Vec_Wrd_t * vSimsPi )
{
    int i, iObj, nWords = Vec_WrdSize(vSimsPi)/Vec_IntSize(vMap);
    Vec_Wrd_t * vSimsNew = Vec_WrdStart( 2 * nInputs * nWords );
    //Vec_Wrd_t * vSimsNew = Vec_WrdStartRandom( nInputs * nWords );
    //Vec_WrdFillExtra( vSimsNew, 2 * nInputs * nWords, 0 );
    assert( Vec_WrdSize(vSimsPi)%Vec_IntSize(vMap) == 0 );
    Vec_WrdShrink( vSimsNew, Vec_WrdSize(vSimsNew)/2 );
    Vec_IntForEachEntry( vMap, iObj, i )
    {
        Abc_TtCopy( Vec_WrdArray(vSimsNew) + (iObj-1)*nWords, Vec_WrdArray(vSimsPi) + i*nWords, nWords, 0 );
        Abc_TtCopy( Vec_WrdLimit(vSimsNew) + (iObj-1)*nWords, Vec_WrdLimit(vSimsPi) + i*nWords, nWords, 0 );
    }
    return vSimsNew;
}
Vec_Wrd_t * Gia_ManCollectSims( Gia_Man_t * pSwp, int nWords, Vec_Int_t * vOuts, int nMaxTries, int nMinCexes, int fUseSim, int fUseSat, int fVerbose, int fVeryVerbose )
{
    Vec_Int_t * vStats[3] = {0};  int i, iObj;
    Vec_Int_t * vMap    = Vec_IntAlloc( 100 );
    Gia_Man_t * pSwp2   = Gia_ManDupCones2( pSwp, Vec_IntArray(vOuts), Vec_IntSize(vOuts), vMap );
    Vec_Wec_t * vCexes  = Min_ManComputeCexes( pSwp2, NULL, nMaxTries, nMinCexes, vStats, fUseSim, fUseSat, fVerbose );
    if ( Vec_IntSum(vStats[2]) == 0 )
    {
        for ( i = 0; i < 3; i++ )
            Vec_IntFree( vStats[i] );
        Vec_IntFree( vMap );
        Gia_ManStop( pSwp2 );
        Vec_WecFree( vCexes );
        return NULL;
    }
    else
    {
        Vec_Wrd_t * vSimsPi = Min_ManBitPack( pSwp2, nWords, vCexes, 1, nMinCexes, vStats[0], fVerbose );
        Vec_Wrd_t * vSimsPo = Gia_ManSimPatSimOut( pSwp2, vSimsPi, 1 );
        Vec_Int_t * vCounts = Patt_ManOutputErrorCoverage( vSimsPo, Vec_IntSize(vOuts) );
        if ( fVerbose )
            Patt_ManProfileErrorsOne( vSimsPo, Vec_IntSize(vOuts) );
        if ( fVeryVerbose )
        {
            printf( "Unsolved = %4d  ", Vec_IntSize(vOuts) );
            Gia_ManPrintStats( pSwp2, NULL );
            Vec_IntForEachEntry( vOuts, iObj, i )
            {
                printf( "%4d : ", i );
                printf( "Out = %5d  ",    Vec_IntEntry(vMap, i) );
                printf( "SimAll =%8d  ",  Vec_IntEntry(vStats[0], i) );
                printf( "SimGood =%8d  ", Vec_IntEntry(vStats[1], i) );
                printf( "PatsAll =%8d  ", Vec_IntEntry(vStats[2], i) );
                printf( "Count = %5d  ",  Vec_IntEntry(vCounts, i) );
                printf( "\n" );
                if ( i == 20 )
                    break;
            }
        }
        for ( i = 0; i < 3; i++ )
            Vec_IntFree( vStats[i] );
        Vec_IntFree( vCounts );
        Vec_WrdFree( vSimsPo );
        Vec_WecFree( vCexes );
        Gia_ManStop( pSwp2 );
        //printf( "Compressing inputs: %5d -> %5d\n", Gia_ManCiNum(pSwp), Vec_IntSize(vMap) );
        vSimsPi = Min_ManRemapSims( Gia_ManCiNum(pSwp), vMap, vSimsPo = vSimsPi );
        Vec_WrdFree( vSimsPo );
        Vec_IntFree( vMap );
        return vSimsPi;
    }
}
Vec_Wrd_t * Min_ManCollect( Gia_Man_t * p, int nConf, int nConf2, int nMaxTries, int nMinCexes, int fUseSim, int fUseSat, int fVerbose, int fVeryVerbose )
{
    abctime clk = Abc_Clock();
    extern Gia_Man_t *    Cec4_ManSimulateTest4( Gia_Man_t * p, int nBTLimit, int nBTLimitPo, int fVerbose );
    Gia_Man_t * pSwp    = Cec4_ManSimulateTest4( p, nConf, nConf2, 0 );
    abctime clkSweep    = Abc_Clock() - clk;
    int nArgs           = fVerbose ? printf( "Generating patterns: Conf = %d (%d). Tries = %d. Pats = %d. Sim = %d. SAT = %d.\n", 
                          nConf, nConf2, nMaxTries, nMinCexes, fUseSim, fUseSat ) : 0;
    Vec_Int_t * vOuts   = Min_ManGetUnsolved( pSwp );
    Vec_Wrd_t * vSimsPi = vOuts ? Gia_ManCollectSims( pSwp, 0, vOuts, nMaxTries, nMinCexes, fUseSim, fUseSat, fVerbose, fVeryVerbose ) : NULL;
    if ( vOuts == NULL )
        printf( "There is no satisfiable outputs.\n" );
    if ( fVerbose )
        Abc_PrintTime( 1, "Sweep time", clkSweep );
    if ( fVerbose )
        Abc_PrintTime( 1, "Total time", Abc_Clock() - clk );
    Vec_IntFreeP( &vOuts );
    Gia_ManStop( pSwp ); 
    nArgs = 0;
    return vSimsPi;
}
void Min_ManTest2( Gia_Man_t * p )
{
    Vec_Wrd_t * vSimsPi = Min_ManCollect( p, 100000, 100000, 10000, 20, 1, 0, 1, 1 );
    Vec_WrdFreeP( &vSimsPi );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

