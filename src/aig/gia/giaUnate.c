/**CFile****************************************************************

  FileName    [giaUnate.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Computation of structural unateness.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaUnate.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Flips the first bit in all entries of the vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Int_t * Vec_IntFlopBit( Vec_Int_t * p )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
    {
        if ( i+1 < p->nSize && Abc_Lit2Var(p->pArray[i]) == Abc_Lit2Var(p->pArray[i+1]) ) 
            i++; // skip variable appearing as both pos and neg literal
        else
            p->pArray[i] ^= 1;
    }
    return p;
}

/**Function*************************************************************

  Synopsis    [Compute unateness for all outputs in terms of inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Wec_t * Gia_ManCheckUnateVec( Gia_Man_t * p, Vec_Int_t * vCiIds, Vec_Int_t * vCoIds )
{
    Vec_Int_t * vCiVec = vCiIds ? Vec_IntDup(vCiIds) : Vec_IntStartNatural(Gia_ManCiNum(p));
    Vec_Int_t * vCoVec = vCoIds ? Vec_IntDup(vCoIds) : Vec_IntStartNatural(Gia_ManCoNum(p));
    Vec_Wec_t * vUnatesCo = Vec_WecStart( Vec_IntSize(vCoVec) );
    Vec_Wec_t * vUnates   = Vec_WecStart( Gia_ManObjNum(p) );
    Vec_Int_t * vUnate0, * vUnate1;
    Gia_Obj_t * pObj; int i, CioId;
    Vec_IntForEachEntry( vCiVec, CioId, i )
    {
        pObj = Gia_ManCi( p, CioId );
        Vec_IntPush( Vec_WecEntry(vUnates, Gia_ObjId(p, pObj)), Abc_Var2Lit(CioId, 0) );
    }
    Gia_ManForEachAnd( p, pObj, i )
    {
        vUnate0 = Vec_WecEntry(vUnates, Gia_ObjFaninId0(pObj, i));
        vUnate1 = Vec_WecEntry(vUnates, Gia_ObjFaninId1(pObj, i));
        vUnate0 = Gia_ObjFaninC0(pObj) ? Vec_IntFlopBit(vUnate0) : vUnate0;
        vUnate1 = Gia_ObjFaninC1(pObj) ? Vec_IntFlopBit(vUnate1) : vUnate1;
        Vec_IntTwoMerge2( vUnate0, vUnate1, Vec_WecEntry(vUnates, i) ); 
        vUnate0 = Gia_ObjFaninC0(pObj) ? Vec_IntFlopBit(vUnate0) : vUnate0;
        vUnate1 = Gia_ObjFaninC1(pObj) ? Vec_IntFlopBit(vUnate1) : vUnate1;
    }
    Vec_IntForEachEntry( vCoVec, CioId, i )
    {
        pObj    = Gia_ManCo( p, CioId );
        vUnate0 = Vec_WecEntry(vUnates, Gia_ObjFaninId0p(p, pObj));
        vUnate0 = Gia_ObjFaninC0(pObj) ? Vec_IntFlopBit(vUnate0) : vUnate0;
        Vec_IntAppend( Vec_WecEntry(vUnatesCo, i), vUnate0 );
        vUnate0 = Gia_ObjFaninC0(pObj) ? Vec_IntFlopBit(vUnate0) : vUnate0;
    }
    Vec_WecFree( vUnates );
    Vec_IntFree( vCiVec );
    Vec_IntFree( vCoVec );
    return vUnatesCo;
}

/**Function*************************************************************

  Synopsis    [Checks unateness one function in one variable.]

  Description [Returns 0 if Co is not unate in Ci.
               Returns 1 if Co is neg-unate in Ci.
               Returns 2 if Co is pos-unate in Ci.
               Returns 3 if Co does not depend on Ci.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManCheckUnate_rec( Gia_Man_t * p, int iObj )
{
    Gia_Obj_t * pObj;
    int Res0, Res1;
    if ( p->nTravIds - p->pTravIds[iObj] <= 3 )
        return p->nTravIds - p->pTravIds[iObj];
    pObj = Gia_ManObj( p, iObj );
    p->pTravIds[iObj] = p->nTravIds - 3;
    if ( Gia_ObjIsCi(pObj) )
        return 3;
    Res0 = Gia_ManCheckUnate_rec( p, Gia_ObjFaninId0(pObj, iObj) );
    Res1 = Gia_ManCheckUnate_rec( p, Gia_ObjFaninId1(pObj, iObj) );
    Res0 = ((Res0 == 1 || Res0 == 2) && Gia_ObjFaninC0(pObj)) ? Res0 ^ 3 : Res0;
    Res1 = ((Res1 == 1 || Res1 == 2) && Gia_ObjFaninC1(pObj)) ? Res1 ^ 3 : Res1;
    p->pTravIds[iObj] = p->nTravIds - (Res0 & Res1);
    assert( (Res0 & Res1) <= 3 );
    return p->nTravIds - p->pTravIds[iObj];
}
int Gia_ManCheckUnate( Gia_Man_t * p, int iCiId, int iCoId )
{
    int Res;
    int CiObjId = Gia_ObjId(p, Gia_ManCi(p, iCiId));
    int CoObjId = Gia_ObjId(p, Gia_ManCo(p, iCoId));
    Gia_Obj_t * pCoObj = Gia_ManCo(p, iCoId);
    Gia_ManIncrementTravId( p ); // Co does not depend on Ci.
    Gia_ManIncrementTravId( p ); // Co is pos-unate in Ci
    Gia_ObjSetTravIdCurrentId( p, CiObjId );
    Gia_ManIncrementTravId( p ); // Co is neg-unate in Ci
    Gia_ManIncrementTravId( p ); // Co is not unate in Ci
    Res = Gia_ManCheckUnate_rec( p, Gia_ObjFaninId0(pCoObj, CoObjId) );
    return ((Res == 1 || Res == 2) && Gia_ObjFaninC0(pCoObj)) ? Res ^ 3 : Res;
}

/**Function*************************************************************

  Synopsis    [Testing procedure for all pairs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCheckUnateVecTest( Gia_Man_t * p, int fVerbose )
{
    abctime clk = Abc_Clock();
    Vec_Wec_t * vUnates = Gia_ManCheckUnateVec( p, NULL, NULL );
    int i, o, Var, nVars = Gia_ManCiNum(p);
    int nUnate = 0, nNonUnate = 0;
    char * pBuffer = ABC_CALLOC( char, nVars+1 );
    if ( fVerbose )
    {
        printf( "Inputs  : " );
        for ( i = 0; i < nVars; i++ )
            printf( "%d", i % 10 );
        printf( "\n" );
    }
    for ( o = 0; o < Gia_ManCoNum(p); o++ )
    {
        Vec_Int_t * vUnate = Vec_WecEntry( vUnates, o );
        memset( pBuffer, ' ', nVars );
        Vec_IntForEachEntry( vUnate, Var, i )
            if ( i+1 < Vec_IntSize(vUnate) && Abc_Lit2Var(Var) == Abc_Lit2Var(Vec_IntEntry(vUnate, i+1)) ) // both lits are present
                pBuffer[Abc_Lit2Var(Var)] = '.', i++, nNonUnate++; // does not depend on this var
            else
                pBuffer[Abc_Lit2Var(Var)] = Abc_LitIsCompl(Var) ? 'n' : 'p', nUnate++;
        if ( fVerbose )
            printf( "Out%4d : %s\n", o, pBuffer );
    }
    ABC_FREE( pBuffer );
    // print stats
    printf( "Ins/Outs = %4d/%4d.  Total supp = %5d.  Total unate = %5d.\n",
        Gia_ManCiNum(p), Gia_ManCoNum(p), nUnate+nNonUnate, nUnate );
    ABC_PRT( "Total time", Abc_Clock() - clk );
    //Vec_WecPrint( vUnates, 0 );
    Vec_WecFree( vUnates );
}

/**Function*************************************************************

  Synopsis    [Testing procedure for one pair.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCheckUnateTest( Gia_Man_t * p, int fComputeAll, int fVerbose )
{
    if ( fComputeAll )
        Gia_ManCheckUnateVecTest( p, fVerbose );
    else
    {
        abctime clk = Abc_Clock();
        int i, o, nVars = Gia_ManCiNum(p);
        int nUnate = 0, nNonUnate = 0;
        char * pBuffer = ABC_CALLOC( char, nVars+1 );
        if ( fVerbose )
        {
            printf( "Inputs  : " );
            for ( i = 0; i < nVars; i++ )
                printf( "%d", i % 10 );
            printf( "\n" );
        }
        for ( o = 0; o < Gia_ManCoNum(p); o++ )
        {
            for ( i = 0; i < nVars; i++ )
            {
                int Res = Gia_ManCheckUnate( p, i, o );
                if ( Res == 3 )       pBuffer[i] = ' ';
                else if ( Res == 2 )  pBuffer[i] = 'p', nUnate++;
                else if ( Res == 1 )  pBuffer[i] = 'n', nUnate++;
                else if ( Res == 0 )  pBuffer[i] = '.', nNonUnate++;
                else assert( 0 );
            }
            if ( fVerbose )
                printf( "Out%4d : %s\n", o, pBuffer );
        }
        ABC_FREE( pBuffer );
        // print stats
        printf( "Ins/Outs = %4d/%4d.  Total supp = %5d.  Total unate = %5d.\n",
            Gia_ManCiNum(p), Gia_ManCoNum(p), nUnate+nNonUnate, nUnate );
        ABC_PRT( "Total time", Abc_Clock() - clk );
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

