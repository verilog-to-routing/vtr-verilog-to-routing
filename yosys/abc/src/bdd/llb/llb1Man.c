/**CFile****************************************************************

  FileName    [llb1Man.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [BDD based reachability.]

  Synopsis    [Reachability manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: llb1Man.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "llbInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManPrepareVarMap( Llb_Man_t * p )
{
    Aig_Obj_t * pObjLi, * pObjLo;
    int i, iVarLi, iVarLo;
    assert( p->vNs2Glo == NULL );
    assert( p->vCs2Glo == NULL );
    assert( p->vGlo2Cs == NULL );
    assert( p->vGlo2Ns == NULL );
    p->vNs2Glo = Vec_IntStartFull( Vec_IntSize(p->vVar2Obj) );
    p->vCs2Glo = Vec_IntStartFull( Vec_IntSize(p->vVar2Obj) );
    p->vGlo2Cs = Vec_IntStartFull( Aig_ManRegNum(p->pAig) );
    p->vGlo2Ns = Vec_IntStartFull( Aig_ManRegNum(p->pAig) );
    Saig_ManForEachLiLo( p->pAig, pObjLi, pObjLo, i )
    {
        iVarLi = Vec_IntEntry(p->vObj2Var, Aig_ObjId(pObjLi));
        iVarLo = Vec_IntEntry(p->vObj2Var, Aig_ObjId(pObjLo));
        assert( iVarLi >= 0 && iVarLi < Vec_IntSize(p->vVar2Obj) );
        assert( iVarLo >= 0 && iVarLo < Vec_IntSize(p->vVar2Obj) );
        Vec_IntWriteEntry( p->vNs2Glo, iVarLi, i );
        Vec_IntWriteEntry( p->vCs2Glo, iVarLo, i );
        Vec_IntWriteEntry( p->vGlo2Cs, i, iVarLo );
        Vec_IntWriteEntry( p->vGlo2Ns, i, iVarLi );
    }
    // add mapping of the PIs
    Saig_ManForEachPi( p->pAig, pObjLo, i )
    {
        iVarLo = Vec_IntEntry(p->vObj2Var, Aig_ObjId(pObjLo));
        Vec_IntWriteEntry( p->vCs2Glo, iVarLo, Aig_ManRegNum(p->pAig)+i );
        Vec_IntWriteEntry( p->vNs2Glo, iVarLo, Aig_ManRegNum(p->pAig)+i );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManPrepareVarLimits( Llb_Man_t * p )
{
    Llb_Grp_t * pGroup;
    Aig_Obj_t * pVar;
    int i, k;
    assert( p->vVarBegs == NULL );
    assert( p->vVarEnds == NULL );
    p->vVarEnds = Vec_IntStart( Aig_ManObjNumMax(p->pAig) ); 
    p->vVarBegs = Vec_IntStart( Aig_ManObjNumMax(p->pAig) ); 
    Vec_IntFill( p->vVarBegs, Aig_ManObjNumMax(p->pAig), p->pMatrix->nCols );

    for ( i = 0; i < p->pMatrix->nCols; i++ )
    {
        pGroup = p->pMatrix->pColGrps[i];

        Vec_PtrForEachEntry( Aig_Obj_t *, pGroup->vIns, pVar, k )
            if ( Vec_IntEntry(p->vVarBegs, pVar->Id) > i )
                Vec_IntWriteEntry( p->vVarBegs, pVar->Id, i );
        Vec_PtrForEachEntry( Aig_Obj_t *, pGroup->vOuts, pVar, k )
            if ( Vec_IntEntry(p->vVarBegs, pVar->Id) > i )
                Vec_IntWriteEntry( p->vVarBegs, pVar->Id, i );

        Vec_PtrForEachEntry( Aig_Obj_t *, pGroup->vIns, pVar, k )
            if ( Vec_IntEntry(p->vVarEnds, pVar->Id) < i )
                Vec_IntWriteEntry( p->vVarEnds, pVar->Id, i );
        Vec_PtrForEachEntry( Aig_Obj_t *, pGroup->vOuts, pVar, k )
            if ( Vec_IntEntry(p->vVarEnds, pVar->Id) < i )
                Vec_IntWriteEntry( p->vVarEnds, pVar->Id, i );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManStop( Llb_Man_t * p )
{
    Llb_Grp_t * pGrp;
    DdNode * bTemp;
    int i;

//    Vec_IntFreeP( &p->vMem );
//    Vec_PtrFreeP( &p->vTops );
//    Vec_PtrFreeP( &p->vBots );
//    Vec_VecFreeP( (Vec_Vec_t **)&p->vCuts );

    if ( p->pMatrix )
        Llb_MtrFree( p->pMatrix );
    Vec_PtrForEachEntry( Llb_Grp_t *, p->vGroups, pGrp, i )
        Llb_ManGroupStop( pGrp );
    if ( p->dd )
    {
//        printf( "Manager dd\n" );
        Extra_StopManager( p->dd );
    }
    if ( p->ddG )
    {
//        printf( "Manager ddG\n" );
        if ( p->ddG->bFunc )
            Cudd_RecursiveDeref( p->ddG, p->ddG->bFunc ); 
        Extra_StopManager( p->ddG );
    }
    if ( p->ddR )
    {
//        printf( "Manager ddR\n" );
        if ( p->ddR->bFunc )
            Cudd_RecursiveDeref( p->ddR, p->ddR->bFunc ); 
        Vec_PtrForEachEntry( DdNode *, p->vRings, bTemp, i )
            Cudd_RecursiveDeref( p->ddR, bTemp );
        Extra_StopManager( p->ddR );
    }
    Aig_ManStop( p->pAig );
    Vec_PtrFreeP( &p->vGroups );
    Vec_IntFreeP( &p->vVar2Obj );
    Vec_IntFreeP( &p->vObj2Var );
    Vec_IntFreeP( &p->vVarBegs );
    Vec_IntFreeP( &p->vVarEnds );
    Vec_PtrFreeP( &p->vRings  );
    Vec_IntFreeP( &p->vNs2Glo );
    Vec_IntFreeP( &p->vCs2Glo );
    Vec_IntFreeP( &p->vGlo2Cs );
    Vec_IntFreeP( &p->vGlo2Ns );
//    Vec_IntFreeP( &p->vHints );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Llb_Man_t * Llb_ManStart( Aig_Man_t * pAigGlo, Aig_Man_t * pAig, Gia_ParLlb_t * pPars )
{
    Llb_Man_t * p;
    Aig_ManCleanMarkA( pAig );
    p = ABC_CALLOC( Llb_Man_t, 1 );
    p->pAigGlo  = pAigGlo;
    p->pPars    = pPars;
    p->pAig     = pAig;
    p->vVar2Obj = Llb_ManMarkPivotNodes( p->pAig, pPars->fUsePivots );
    p->vObj2Var = Vec_IntInvert( p->vVar2Obj, -1 );
    p->vRings   = Vec_PtrAlloc( 100 );
    Llb_ManPrepareVarMap( p );
    Llb_ManPrepareGroups( p );
    Aig_ManCleanMarkA( pAig );
    p->pMatrix = Llb_MtrCreate( p );
    p->pMatrix->pMan = p;
    return p;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

