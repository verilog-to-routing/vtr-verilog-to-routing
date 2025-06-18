/**CFile****************************************************************

  FileName    [gia.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: gia.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/util/utilTruth.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
 
typedef struct Slv_Man_t_ Slv_Man_t;
struct Slv_Man_t_
{
    Vec_Int_t      vCis;
    Vec_Int_t      vCos;
    Vec_Int_t      vFanins;
    Vec_Int_t      vFanoutN;
    Vec_Int_t      vFanout0;
    Vec_Str_t      vValues;
    Vec_Int_t      vCopies;
};

static inline int  Slv_ManObjNum       ( Slv_Man_t * p )                      { return Vec_IntSize(&p->vFanins)/2;                       }

static inline int  Slv_ObjFaninLit     ( Slv_Man_t * p, int iObj, int f )     { return Vec_IntEntry(&p->vFanins, Abc_Var2Lit(iObj, f));  }
static inline int  Slv_ObjFanin        ( Slv_Man_t * p, int iObj, int f )     { return Abc_Lit2Var(Slv_ObjFaninLit(p, iObj, f));         }
static inline int  Slv_ObjFaninC       ( Slv_Man_t * p, int iObj, int f )     { return Abc_LitIsCompl(Slv_ObjFaninLit(p, iObj, f));      }

static inline int  Slv_ObjIsCi         ( Slv_Man_t * p, int iObj )            { return !Slv_ObjFaninLit(p, iObj, 0) &&  Slv_ObjFaninLit(p, iObj, 1);  }
static inline int  Slv_ObjIsCo         ( Slv_Man_t * p, int iObj )            { return  Slv_ObjFaninLit(p, iObj, 0) && !Slv_ObjFaninLit(p, iObj, 1);  }
static inline int  Slv_ObjIsAnd        ( Slv_Man_t * p, int iObj )            { return  Slv_ObjFaninLit(p, iObj, 0) &&  Slv_ObjFaninLit(p, iObj, 1);  }

static inline int  Slv_ObjFanout0      ( Slv_Man_t * p, int iObj )            { return Vec_IntEntry(&p->vFanout0, iObj);      }
static inline void Slv_ObjSetFanout0   ( Slv_Man_t * p, int iObj, int iLit )  { Vec_IntWriteEntry(&p->vFanout0, iObj, iLit);  }

static inline int  Slv_ObjNextFanout   ( Slv_Man_t * p, int iLit )            { return Vec_IntEntry(&p->vFanoutN, iLit);      }
static inline void Slv_ObjSetNextFanout( Slv_Man_t * p, int iLit, int iLitF ) { Vec_IntWriteEntry(&p->vFanoutN, iLit, iLitF); }

static inline int  Slv_ObjCopyLit      ( Slv_Man_t * p, int iObj )            { return Vec_IntEntry(&p->vCopies, iObj);       }
static inline void Slv_ObjSetCopyLit   ( Slv_Man_t * p, int iObj, int iLit )  { Vec_IntWriteEntry(&p->vCopies, iObj, iLit);   }

#define Slv_ManForEachObj( p, iObj )                                  \
    for ( iObj = 1; iObj < Slv_ManObjNum(p); iObj++ )

#define Slv_ObjForEachFanout( p, iObj, iFanLit )                      \
    for ( iFanLit = Slv_ObjFanout0(p, iObj); iFanLit; iFanLit = Slv_ObjNextFanout(p, iFanLit) )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Slv_Man_t * Slv_ManAlloc( int nObjs )
{
    Slv_Man_t * p = ABC_CALLOC( Slv_Man_t, 1 );
    Vec_IntGrow( &p->vCis,     100 );
    Vec_IntGrow( &p->vCos,     100 );
    Vec_IntGrow( &p->vFanins,  2*nObjs );
    Vec_IntGrow( &p->vFanoutN, 2*nObjs );
    Vec_IntGrow( &p->vFanout0, nObjs );
    Vec_StrGrow( &p->vValues,  nObjs );
    // constant node
    Vec_IntFill( &p->vFanins,  2, 0 );
    Vec_IntFill( &p->vFanoutN, 2, 0 );
    Vec_IntFill( &p->vFanout0, 1, 0 );
    return p;
}
void Slv_ManFree( Slv_Man_t * p )
{
    Vec_IntErase( &p->vCis     );
    Vec_IntErase( &p->vCos     );
    Vec_IntErase( &p->vFanins  );
    Vec_IntErase( &p->vFanoutN );
    Vec_IntErase( &p->vFanout0 );
    Vec_StrErase( &p->vValues  );
    Vec_IntErase( &p->vCopies  );
    ABC_FREE( p );
}
void Slv_ManPrintFanouts( Slv_Man_t * p )
{
    int iObj, iFanLit;
    Slv_ManForEachObj( p, iObj )
    {
        printf( "Fanouts of node %d: ", iObj );
        Slv_ObjForEachFanout( p, iObj, iFanLit )
            printf( "%d ", Abc_Lit2Var(iFanLit) );
        printf( "\n" );
    }
}
static inline int Slv_ManAppendObj( Slv_Man_t * p, int iLit0, int iLit1 )
{
    int iObj = Slv_ManObjNum(p);
    Vec_StrPush( &p->vValues,  0 );
    Vec_IntPush( &p->vFanout0, 0 );
    Vec_IntPushTwo( &p->vFanoutN, 0, 0 );
    if ( !iLit0 )  // primary input
        assert(!iLit1), iLit1 = Vec_IntSize(&p->vCis)+1, Vec_IntPush(&p->vCis, iObj);
    else if ( !iLit1 )  // primary output
        assert(iLit0), Vec_IntPush(&p->vCos, iObj);
    else
    {
        Slv_ObjSetNextFanout( p, Abc_Var2Lit(iObj, 0), Slv_ObjFanout0(p, Abc_Lit2Var(iLit0)) );
        Slv_ObjSetNextFanout( p, Abc_Var2Lit(iObj, 1), Slv_ObjFanout0(p, Abc_Lit2Var(iLit1)) );
        Slv_ObjSetFanout0( p, Abc_Lit2Var(iLit0), Abc_Var2Lit(iObj, 0) );
        Slv_ObjSetFanout0( p, Abc_Lit2Var(iLit1), Abc_Var2Lit(iObj, 1) );
    }
    Vec_IntPushTwo( &p->vFanins, iLit0, iLit1 );
    return Abc_Var2Lit( iObj, 0 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Slv_Man_t * Slv_ManFromGia( Gia_Man_t * p )
{
    Gia_Obj_t * pObj; int i;
    Slv_Man_t * pNew = Slv_ManAlloc( Gia_ManObjNum(p) );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
            pObj->Value = Slv_ManAppendObj( pNew, Abc_LitNotCond(Gia_ObjFanin0(pObj)->Value, Gia_ObjFaninC0(pObj)), Abc_LitNotCond(Gia_ObjFanin1(pObj)->Value, Gia_ObjFaninC1(pObj)) );
        else if ( Gia_ObjIsCo(pObj) )
            pObj->Value = Slv_ManAppendObj( pNew, Abc_LitNotCond(Gia_ObjFanin0(pObj)->Value, Gia_ObjFaninC0(pObj)), 0 );
        else if ( Gia_ObjIsCi(pObj) )
            pObj->Value = Slv_ManAppendObj( pNew, 0, 0 );
        else
            assert( 0 );
    }
    assert( Gia_ManObjNum(p) == Slv_ManObjNum(pNew) );
    return pNew;
}
Gia_Man_t * Slv_ManToGia( Slv_Man_t * p )
{
    int iObj;
    Gia_Man_t * pNew = Gia_ManStart( Slv_ManObjNum(p) ); 
    Vec_IntFill( &p->vCopies,  Slv_ManObjNum(p), 0 );
    Slv_ManForEachObj( p, iObj )
        if ( Slv_ObjIsCi(p, iObj) )
            Slv_ObjSetCopyLit( p, iObj, Gia_ManAppendCi(pNew) );
        else if ( Slv_ObjIsCo(p, iObj) )
        {
            int iLit0 = Abc_LitNotCond( Slv_ObjCopyLit(p, Slv_ObjFanin(p, iObj, 0)), Slv_ObjFaninC(p, iObj, 0) );
            Slv_ObjSetCopyLit( p, iObj, Gia_ManAppendCo(pNew, iLit0) );
        }
        else if ( Slv_ObjIsAnd(p, iObj) )
        {
            int iLit0 = Abc_LitNotCond( Slv_ObjCopyLit(p, Slv_ObjFanin(p, iObj, 0)), Slv_ObjFaninC(p, iObj, 0) );
            int iLit1 = Abc_LitNotCond( Slv_ObjCopyLit(p, Slv_ObjFanin(p, iObj, 1)), Slv_ObjFaninC(p, iObj, 1) );
            Slv_ObjSetCopyLit( p, iObj, Gia_ManAppendAnd(pNew, iLit0, iLit1) );
        }
        else assert(0);
    assert( Gia_ManObjNum(pNew) == Slv_ManObjNum(p) );
    return pNew;
}

Gia_Man_t * Slv_ManToAig( Gia_Man_t * pGia )
{
    Slv_Man_t * p = Slv_ManFromGia( pGia );
    Gia_Man_t * pNew = Slv_ManToGia( p );
    pNew->pName = Abc_UtilStrsav( pGia->pName );
    pNew->pSpec = Abc_UtilStrsav( pGia->pSpec );
    Slv_ManPrintFanouts( p );
    Slv_ManFree( p );
    return pNew;
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManCofPisVars( Gia_Man_t * p, int nVars )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj; int i, m;
    pNew = Gia_ManStart( 1000 );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManForEachPi( p, pObj, i )
        Gia_ManAppendCi( pNew );
    Gia_ManHashStart( pNew );
    for ( m = 0; m < (1 << nVars); m++ )
    {
        Gia_ManFillValue( p );
        Gia_ManConst0(p)->Value = 0;
        Gia_ManForEachPi( p, pObj, i )
        {
            if ( i < nVars )
                pObj->Value = (m >> i) & 1;
            else
                pObj->Value = Gia_ObjToLit(pNew, Gia_ManCi(pNew, i));
        }
        Gia_ManForEachAnd( p, pObj, i )
            pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        Gia_ManForEachPo( p, pObj, i )
            Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    }
    Gia_ManHashStop( pNew );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManStructExperiment( Gia_Man_t * p )
{
    extern int Cec_ManVerifyTwo( Gia_Man_t * p0, Gia_Man_t * p1, int fVerbose );
    Gia_Man_t * pTemp, * pUsed;
    Vec_Ptr_t * vGias = Vec_PtrAlloc( 100 );
    Gia_Obj_t * pObj; int i, k;
    Gia_ManForEachCo( p, pObj, i )
    {
        int iFan0 = Gia_ObjFaninId0p(p, pObj);
        pTemp = Gia_ManDupAndCones( p, &iFan0, 1, 1 );
        Vec_PtrForEachEntry( Gia_Man_t *, vGias, pUsed, k )
            if ( Gia_ManCiNum(pTemp) == Gia_ManCiNum(pUsed) && Cec_ManVerifyTwo(pTemp, pUsed, 0) == 1 )
            {
                ABC_SWAP( void *, Vec_PtrArray(vGias)[0], Vec_PtrArray(vGias)[k] );
                break;
            }
            else
                ABC_FREE( pTemp->pCexComb );

        printf( "\nOut %6d : ", i );
        if ( k == Vec_PtrSize(vGias) )
            printf( "Equiv to none    " );
        else
            printf( "Equiv to %6d  ", k );
        Gia_ManPrintStats( pTemp, NULL );

        if ( k == Vec_PtrSize(vGias) )
            Vec_PtrPush( vGias, pTemp );
        else
            Gia_ManStop( pTemp );
    }
    printf( "\nComputed %d classes.\n\n", Vec_PtrSize(vGias) );
    Vec_PtrForEachEntry( Gia_Man_t *, vGias, pTemp, i )
        Gia_ManStop( pTemp );
    Vec_PtrFree( vGias );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_EnumFirstUnused( int * pUsed, int nVars )
{
    int i;
    for ( i = 0; i < nVars; i++ )
        if ( pUsed[i] == 0 )
            return i;
    return -1;
}
void Gia_EnumPerms_rec( int * pUsed, int nVars, int * pPerm, int nPerm, int * pCount, FILE * pFile, int nLogVars )
{
    int i, k, New;
    if ( nPerm == nVars )
    {
        if ( pFile )
        {
            for ( i = 0; i < nLogVars; i++ )
                fprintf( pFile, "%c", '0' + ((*pCount) >> (nLogVars-1-i) & 1) );
            fprintf( pFile, " " );
            for ( i = 0; i < nVars; i++ )
            for ( k = 0; k < nVars; k++ )
                fprintf( pFile, "%c", '0' + (pPerm[i] == k) );
            fprintf( pFile, "\n" );
        }
        else
        {
            if ( *pCount < 20 )
            {
                printf( "%5d : ", (*pCount) );
                for ( i = 0; i < nVars; i += 2 )
                    printf( "%d %d  ", pPerm[i], pPerm[i+1] );
                printf( "\n" );
            }
        }
        (*pCount)++;
        return;
    }
    New = Gia_EnumFirstUnused( pUsed, nVars );
    assert( New >= 0 );
    pPerm[nPerm] = New;
    assert( pUsed[New] == 0 );
    pUsed[New] = 1;
    // try remaining ones
    for ( i = 0; i < nVars; i++ )
    {
        if ( pUsed[i] == 1 )
            continue;
        pPerm[nPerm+1] = i;
        assert( pUsed[i] == 0 );
        pUsed[i] = 1;
        Gia_EnumPerms_rec( pUsed, nVars, pPerm, nPerm+2, pCount, pFile, nLogVars );
        assert( pUsed[i] == 1 );
        pUsed[i] = 0;
    }
    assert( pUsed[New] == 1 );
    pUsed[New] = 0;
}
void Gia_EnumPerms( int nVars )
{
    int nLogVars = 0, Count = 0;
    int * pUsed = ABC_CALLOC( int, nVars );
    int * pPerm = ABC_CALLOC( int, nVars );
    FILE * pFile = fopen( "pairset.pla", "wb" );
    assert( nVars % 2 == 0 );

    printf( "Printing sets of pairs for %d objects:\n", nVars );
    Gia_EnumPerms_rec( pUsed, nVars, pPerm, 0, &Count, NULL, -1 ); 
    if ( Count > 20 )
        printf( "...\n" );
    printf( "Finished enumerating %d sets of pairs.\n", Count );

    nLogVars = Abc_Base2Log( Count );
    printf( "Need %d variables to encode %d sets.\n", nLogVars, Count );
    Count = 0;
    fprintf( pFile, ".i %d\n", nLogVars );
    fprintf( pFile, ".o %d\n", nVars*nVars );
    Gia_EnumPerms_rec( pUsed, nVars, pPerm, 0, &Count, pFile, nLogVars );   
    fprintf( pFile, ".e\n" );
    fclose( pFile );
    printf( "Finished dumping file \"%s\".\n", "pairset.pla" );

    ABC_FREE( pUsed );
    ABC_FREE( pPerm );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

