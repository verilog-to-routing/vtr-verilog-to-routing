/**CFile****************************************************************

  FileName    [giaAig.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaAig.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define AGI_PI ABC_CONST(0xFFFFFFFF00000000) 
#define AGI_RO ABC_CONST(0xFFFFFFFE00000000) 
#define AGI_PO ABC_CONST(0xFFFFFFFD00000000) 
#define AGI_RI ABC_CONST(0xFFFFFFFC00000000) 
#define AGI_C0 ABC_CONST(0xFFFFFFFBFFFFFFFA) 
#define AGI_M0 ABC_CONST(0x00000000FFFFFFFF) 
#define AGI_M1 ABC_CONST(0xFFFFFFFF00000000) 

typedef struct Agi_Man_t_ Agi_Man_t;
struct Agi_Man_t_
{
    char *             pName;       // name of the AIG
    char *             pSpec;       // name of the input file
    int                nCap;        // number of objects
    int                nObjs;       // number of objects
    int                nNodes;      // number of objects
    int                nRegs;       // number of registers
    unsigned           nTravIds;    // number of objects
    Vec_Int_t          vCis;        // comb inputs
    Vec_Int_t          vCos;        // comb outputs
    word *             pObjs;       // objects
    unsigned *         pThird;      // third input
    unsigned *         pTravIds;    // traversal IDs
    unsigned *         pNext;       // next values
    unsigned *         pTable;      // hash table
    unsigned *         pCopy;       // hash table
};

static inline int      Agi_ManObjNum( Agi_Man_t * p )                      { return p->nObjs;                                        } 
static inline int      Agi_ManCiNum( Agi_Man_t * p )                       { return Vec_IntSize( &p->vCis );                         } 
static inline int      Agi_ManCoNum( Agi_Man_t * p )                       { return Vec_IntSize( &p->vCos );                         } 
static inline int      Agi_ManNodeNum( Agi_Man_t * p )                     { return p->nNodes;                                       } 

static inline unsigned Agi_ObjLit0( Agi_Man_t * p, int i )                 { return (unsigned)(p->pObjs[i]);                         } 
static inline unsigned Agi_ObjLit1( Agi_Man_t * p, int i )                 { return (unsigned)(p->pObjs[i] >> 32);                   } 
static inline unsigned Agi_ObjLit2( Agi_Man_t * p, int i )                 { return p->pThird[i];                                    } 
static inline int      Agi_ObjVar0( Agi_Man_t * p, int i )                 { return Agi_ObjLit0(p, i) >> 1;                          } 
static inline int      Agi_ObjVar1( Agi_Man_t * p, int i )                 { return Agi_ObjLit1(p, i) >> 1;                          } 
static inline int      Agi_ObjVar2( Agi_Man_t * p, int i )                 { return Agi_ObjLit2(p, i) >> 1;                          } 
static inline void     Agi_ObjSetLit0( Agi_Man_t * p, int i, unsigned l )  { p->pObjs[i] = (p->pObjs[i] & AGI_M1) | (word)l;         } 
static inline void     Agi_ObjSetLit1( Agi_Man_t * p, int i, unsigned l )  { p->pObjs[i] = (p->pObjs[i] & AGI_M0) | ((word)l << 32); } 
static inline void     Agi_ObjSetLit2( Agi_Man_t * p, int i, unsigned l )  { p->pThird[i] = l;                                       }  

static inline int      Agi_ObjIsC0( Agi_Man_t * p, int i )                 { return (i == 0);                                        } 
static inline int      Agi_ObjIsPi( Agi_Man_t * p, int i )                 { return (p->pObjs[i] & AGI_PI) == AGI_PI;                } 
static inline int      Agi_ObjIsRo( Agi_Man_t * p, int i )                 { return (p->pObjs[i] & AGI_PI) == AGI_RO;                } 
static inline int      Agi_ObjIsPo( Agi_Man_t * p, int i )                 { return (p->pObjs[i] & AGI_PI) == AGI_PO;                } 
static inline int      Agi_ObjIsRi( Agi_Man_t * p, int i )                 { return (p->pObjs[i] & AGI_PI) == AGI_RI;                } 
static inline int      Agi_ObjIsCi( Agi_Man_t * p, int i )                 { return (p->pObjs[i] & AGI_RO) == AGI_RO;                } 
static inline int      Agi_ObjIsCo( Agi_Man_t * p, int i )                 { return (p->pObjs[i] & AGI_RO) == AGI_PO;                } 
static inline int      Agi_ObjIsNode( Agi_Man_t * p, int i )               { return p->pObjs[i] < AGI_C0;                            } 
static inline int      Agi_ObjIsBuf( Agi_Man_t * p, int i )                { return Agi_ObjLit0(p, i) == Agi_ObjLit1(p, i);          } 
static inline int      Agi_ObjIsAnd( Agi_Man_t * p, int i )                { return Agi_ObjIsNode(p, i) && Agi_ObjLit0(p, i) < Agi_ObjLit1(p, i);  } 
static inline int      Agi_ObjIsXor( Agi_Man_t * p, int i )                { return Agi_ObjIsNode(p, i) && Agi_ObjLit0(p, i) > Agi_ObjLit1(p, i);  } 
static inline int      Agi_ObjIsMux( Agi_Man_t * p, int i )                { return Agi_ObjIsAnd(p, i) && ~Agi_ObjLit2(p, i);        }
static inline int      Agi_ObjIsMaj( Agi_Man_t * p, int i )                { return Agi_ObjIsXor(p, i) && ~Agi_ObjLit2(p, i);        } 

static inline int Agi_ManAppendObj( Agi_Man_t * p )
{
    assert( p->nObjs < p->nCap );
    return p->nObjs++; // return var
}
static inline int Agi_ManAppendCi( Agi_Man_t * p )
{
    int iObj = Agi_ManAppendObj( p );
    p->pObjs[iObj] = AGI_PI | (word)Vec_IntSize(&p->vCis);
    Vec_IntPush( &p->vCis, iObj );
    return Abc_Var2Lit( iObj, 0 ); // return lit
}
static inline int Agi_ManAppendCo( Agi_Man_t * p, int iLit0 )
{
    int iObj = Agi_ManAppendObj( p );
    p->pObjs[iObj] = AGI_PO | (word)iLit0;
    Vec_IntPush( &p->vCos, iObj );
    return Abc_Var2Lit( iObj, 0 ); // return lit
}
static inline int Agi_ManAppendAnd( Agi_Man_t * p, int iLit0, int iLit1 )
{
    int iObj = Agi_ManAppendObj( p );
    assert( iLit0 < iLit1 );
    p->pObjs[iObj] = ((word)iLit1 << 32) | (word)iLit0;
    p->nNodes++;
    return Abc_Var2Lit( iObj, 0 ); // return lit
}

#define Agi_ManForEachCi( p, iCi, i )   Vec_IntForEachEntry( &p->vCis, iCi, i )
#define Agi_ManForEachCo( p, iCo, i )   Vec_IntForEachEntry( &p->vCos, iCo, i )
#define Agi_ManForEachObj( p, i )       for ( i = 0; i < Agi_ManObjNum(p); i++ )
#define Agi_ManForEachObj1( p, i )      for ( i = 1; i < Agi_ManObjNum(p); i++ )
#define Agi_ManForEachNode( p, i )      for ( i = 1; i < Agi_ManObjNum(p); i++ ) if ( !Agi_ObjIsNode(p, i) ) {} else

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Agi_Man_t * Agi_ManAlloc( int nCap )
{
    Agi_Man_t * p;
    nCap = Abc_MaxInt( nCap, 16 );
    p = ABC_CALLOC( Agi_Man_t, 1 );
    p->nCap     = nCap;
    p->pObjs    = ABC_CALLOC( word, nCap );
    p->pTravIds = ABC_CALLOC( unsigned, nCap );
    p->pObjs[0] = AGI_C0;
    p->nObjs    = 1;
    return p;
}
void Agi_ManFree( Agi_Man_t * p )
{
    ABC_FREE( p->pObjs );
    ABC_FREE( p->pTravIds );
    ABC_FREE( p->vCis.pArray );
    ABC_FREE( p->vCos.pArray );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Agi_Man_t * Agi_ManFromGia( Gia_Man_t * p )
{
    Agi_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    pNew = Agi_ManAlloc( Gia_ManObjNum(p) );
    Gia_ManForEachObj1( p, pObj, i )
        if ( Gia_ObjIsAnd(pObj) )
            pObj->Value = Agi_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else if ( Gia_ObjIsCo(pObj) )
            pObj->Value = Agi_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
        else if ( Gia_ObjIsCi(pObj) )
            pObj->Value = Agi_ManAppendCi( pNew );
        else assert( 0 );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Agi_ManSuppSize_rec( Agi_Man_t * p, int i )
{
    if ( p->pTravIds[i] == p->nTravIds )
        return 0;
    p->pTravIds[i] = p->nTravIds;
    if ( Agi_ObjIsCi(p, i) )
        return 1;
    assert( Agi_ObjIsAnd(p, i) );
    return Agi_ManSuppSize_rec( p, Agi_ObjVar0(p, i) ) +  Agi_ManSuppSize_rec( p, Agi_ObjVar1(p, i) );
}
int Agi_ManSuppSizeOne( Agi_Man_t * p, int i )
{
    p->nTravIds++;
    return Agi_ManSuppSize_rec( p, i );
}
int Agi_ManSuppSizeTest( Agi_Man_t * p )
{
    abctime clk = Abc_Clock();
    int i, Counter = 0;
    Agi_ManForEachNode( p, i )
        Counter += (Agi_ManSuppSizeOne(p, i) <= 16);
    printf( "Nodes with small support %d (out of %d)\n", Counter, Agi_ManNodeNum(p) );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    return Counter;

}
void Agi_ManTest( Gia_Man_t * pGia )
{
    extern int Gia_ManSuppSizeTest( Gia_Man_t * p );
    Agi_Man_t * p;
    Gia_ManSuppSizeTest( pGia );
    p = Agi_ManFromGia( pGia );
    Agi_ManSuppSizeTest( p );
    Agi_ManFree( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

