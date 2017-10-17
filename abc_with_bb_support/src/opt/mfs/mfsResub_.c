/**CFile****************************************************************

  FileName    [mfsResub.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [The good old minimization with complete don't-cares.]

  Synopsis    [Procedures to perform resubstitution.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: mfsResub.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "mfsInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Updates the network after resubstitution.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkMfsUpdateNetwork( Mfs_Man_t * p, Abc_Obj_t * pObj, Vec_Ptr_t * vFanins, Hop_Obj_t * pFunc )
{
    Abc_Obj_t * pObjNew, * pFanin;
    int k;
    // create the new node
    pObjNew = Abc_NtkCreateNode( pObj->pNtk );
    pObjNew->pData = pFunc;
    Vec_PtrForEachEntry( vFanins, pFanin, k )
        Abc_ObjAddFanin( pObjNew, pFanin );
    // replace the old node by the new node
//printf( "Replacing node " ); Abc_ObjPrint( stdout, pObj );
//printf( "Inserting node " ); Abc_ObjPrint( stdout, pObjNew );
    // update the level of the node
    Abc_NtkUpdate( pObj, pObjNew, p->vLevels );
}

/**Function*************************************************************

  Synopsis    [Prints resub candidate stats.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkMfsPrintResubStats( Mfs_Man_t * p )
{
    Abc_Obj_t * pFanin, * pNode;
    int i, k, nAreaCrits = 0, nAreaExpanse = 0;
    int nFaninMax = Abc_NtkGetFaninMax(p->pNtk);
    Abc_NtkForEachNode( p->pNtk, pNode, i )
        Abc_ObjForEachFanin( pNode, pFanin, k )
        {
            if ( !Abc_ObjIsCi(pFanin) && Abc_ObjFanoutNum(pFanin) == 1 )
            {
                nAreaCrits++;
                nAreaExpanse += (int)(Abc_ObjFaninNum(pNode) < nFaninMax);
            }
        }
    printf( "Total area-critical fanins = %d. Belonging to expandable nodes = %d.\n", 
        nAreaCrits, nAreaExpanse );
}

/**Function*************************************************************

  Synopsis    [Tries resubstitution.]

  Description [Returns 1 if it is feasible, or 0 if c-ex is found.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkMfsTryResubOnce( Mfs_Man_t * p, int * pCands, int nCands )
{
    unsigned * pData;
    int RetValue, iVar, i;
    p->nSatCalls++;
    RetValue = sat_solver_solve( p->pSat, pCands, pCands + nCands, (sint64)p->pPars->nBTLimit, (sint64)0, (sint64)0, (sint64)0 );
//    assert( RetValue == l_False || RetValue == l_True );
    if ( RetValue == l_False )
        return 1;
    if ( RetValue != l_True )
    {
        p->nTimeOuts++;
        return -1;
    }
    p->nSatCexes++;
    // store the counter-example
    Vec_IntForEachEntry( p->vProjVars, iVar, i )
    {
        pData = Vec_PtrEntry( p->vDivCexes, i );
        if ( !sat_solver_var_value( p->pSat, iVar ) ) // remove 0s!!!
        {
            assert( Aig_InfoHasBit(pData, p->nCexes) );
            Aig_InfoXorBit( pData, p->nCexes );
        }
    }
    p->nCexes++;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Performs resubstitution for the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkMfsSolveSatResub( Mfs_Man_t * p, Abc_Obj_t * pNode, int iFanin, int fOnlyRemove, int fSkipUpdate )
{
    int fVeryVerbose = p->pPars->fVeryVerbose && Vec_PtrSize(p->vDivs) < 80;
    unsigned * pData;
    int pCands[MFS_FANIN_MAX];
    int RetValue, iVar, i, nCands, nWords, w;
    clock_t clk;
    Abc_Obj_t * pFanin;
    Hop_Obj_t * pFunc;
    assert( iFanin >= 0 );

    // clean simulation info
    Vec_PtrFillSimInfo( p->vDivCexes, 0, p->nDivWords ); 
    p->nCexes = 0;
    if ( fVeryVerbose )
    {
        printf( "\n" );
        printf( "Node %5d : Level = %2d. Divs = %3d.  Fanin = %d (out of %d). MFFC = %d\n", 
            pNode->Id, pNode->Level, Vec_PtrSize(p->vDivs)-Abc_ObjFaninNum(pNode), 
            iFanin, Abc_ObjFaninNum(pNode), 
            Abc_ObjFanoutNum(Abc_ObjFanin(pNode, iFanin)) == 1 ? Abc_NodeMffcLabel(Abc_ObjFanin(pNode, iFanin)) : 0 );
    }

    // try fanins without the critical fanin
    nCands = 0;
    Vec_PtrClear( p->vFanins );
    Abc_ObjForEachFanin( pNode, pFanin, i )
    {
        if ( i == iFanin )
            continue;
        Vec_PtrPush( p->vFanins, pFanin );
        iVar = Vec_PtrSize(p->vDivs) - Abc_ObjFaninNum(pNode) + i;
        pCands[nCands++] = toLitCond( Vec_IntEntry( p->vProjVars, iVar ), 1 );
    }
    RetValue = Abc_NtkMfsTryResubOnce( p, pCands, nCands );
    if ( RetValue == -1 )
        return 0;
    if ( RetValue == 1 )
    {
        if ( fVeryVerbose )
        printf( "Node %d: Fanin %d can be removed.\n", pNode->Id, iFanin );
        p->nNodesResub++;
        p->nNodesGainedLevel++;
        if ( fSkipUpdate )
            return 1;
clk = clock();
        // derive the function
        pFunc = Abc_NtkMfsInterplate( p, pCands, nCands );
        if ( pFunc == NULL )
            return 0;
        // update the network
        Abc_NtkMfsUpdateNetwork( p, pNode, p->vFanins, pFunc );
p->timeInt += clock() - clk;
        return 1;
    }

    if ( fOnlyRemove )
        return 0;

    if ( fVeryVerbose )
    {
        for ( i = 0; i < 8; i++ )
            printf( " " );
        for ( i = 0; i < Vec_PtrSize(p->vDivs)-Abc_ObjFaninNum(pNode); i++ )
            printf( "%d", i % 10 );
        for ( i = 0; i < Abc_ObjFaninNum(pNode); i++ )
            if ( i == iFanin )
                printf( "*" );
            else
                printf( "%c", 'a' + i );
        printf( "\n" );
    }
    iVar = -1;
    while ( 1 )
    {
        float * pProbab = (float *)(p->vProbs? p->vProbs->pArray : NULL);
        assert( (pProbab != NULL) == p->pPars->fPower );
        if ( fVeryVerbose )
        {
            printf( "%3d: %2d ", p->nCexes, iVar );
            for ( i = 0; i < Vec_PtrSize(p->vDivs); i++ )
            {
                pData = Vec_PtrEntry( p->vDivCexes, i );
                printf( "%d", Aig_InfoHasBit(pData, p->nCexes-1) );
            }
            printf( "\n" );
        }

        // find the next divisor to try
        nWords = Aig_BitWordNum(p->nCexes);
        assert( nWords <= p->nDivWords );
        for ( iVar = 0; iVar < Vec_PtrSize(p->vDivs)-Abc_ObjFaninNum(pNode); iVar++ )
        {
            if ( p->pPars->fPower )
            {
                Abc_Obj_t * pDiv = Vec_PtrEntry(p->vDivs, iVar);
                // only accept the divisor if it is "cool"
                if ( pProbab[Abc_ObjId(pDiv)] >= 0.2 )
                    continue;
            }
            pData  = Vec_PtrEntry( p->vDivCexes, iVar );
            for ( w = 0; w < nWords; w++ )
                if ( pData[w] != ~0 )
                    break;
            if ( w == nWords )
                break;
        }
        if ( iVar == Vec_PtrSize(p->vDivs)-Abc_ObjFaninNum(pNode) )
            return 0;

        pCands[nCands] = toLitCond( Vec_IntEntry(p->vProjVars, iVar), 1 );
        RetValue = Abc_NtkMfsTryResubOnce( p, pCands, nCands+1 );
        if ( RetValue == -1 )
            return 0;
        if ( RetValue == 1 )
        {
            if ( fVeryVerbose )
            printf( "Node %d: Fanin %d can be replaced by divisor %d.\n", pNode->Id, iFanin, iVar );
            p->nNodesResub++;
            p->nNodesGainedLevel++;
            if ( fSkipUpdate )
                return 1;
clk = clock();
            // derive the function
            pFunc = Abc_NtkMfsInterplate( p, pCands, nCands+1 );
            if ( pFunc == NULL )
                return 0;
            // update the network
            Vec_PtrPush( p->vFanins, Vec_PtrEntry(p->vDivs, iVar) );
            Abc_NtkMfsUpdateNetwork( p, pNode, p->vFanins, pFunc );
p->timeInt += clock() - clk;
            return 1;
        }
        if ( p->nCexes >= p->pPars->nDivMax )
            break;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Performs resubstitution for the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkMfsSolveSatResub2( Mfs_Man_t * p, Abc_Obj_t * pNode, int iFanin, int iFanin2 )
{
    int fVeryVerbose = p->pPars->fVeryVerbose && Vec_PtrSize(p->vDivs) < 80;
    unsigned * pData, * pData2;
    int pCands[MFS_FANIN_MAX];
    int RetValue, iVar, iVar2, i, w, nCands, nWords, fBreak;
    clock_t clk;
    Abc_Obj_t * pFanin;
    Hop_Obj_t * pFunc;
    assert( iFanin >= 0 );
    assert( iFanin2 >= 0 || iFanin2 == -1 );

    // clean simulation info
    Vec_PtrFillSimInfo( p->vDivCexes, 0, p->nDivWords ); 
    p->nCexes = 0;
    if ( fVeryVerbose )
    {
        printf( "\n" );
        printf( "Node %5d : Level = %2d. Divs = %3d.  Fanins = %d/%d (out of %d). MFFC = %d\n", 
            pNode->Id, pNode->Level, Vec_PtrSize(p->vDivs)-Abc_ObjFaninNum(pNode), 
            iFanin, iFanin2, Abc_ObjFaninNum(pNode), 
            Abc_ObjFanoutNum(Abc_ObjFanin(pNode, iFanin)) == 1 ? Abc_NodeMffcLabel(Abc_ObjFanin(pNode, iFanin)) : 0 );
    }

    // try fanins without the critical fanin
    nCands = 0;
    Vec_PtrClear( p->vFanins );
    Abc_ObjForEachFanin( pNode, pFanin, i )
    {
        if ( i == iFanin || i == iFanin2 )
            continue;
        Vec_PtrPush( p->vFanins, pFanin );
        iVar = Vec_PtrSize(p->vDivs) - Abc_ObjFaninNum(pNode) + i;
        pCands[nCands++] = toLitCond( Vec_IntEntry( p->vProjVars, iVar ), 1 );
    }
    RetValue = Abc_NtkMfsTryResubOnce( p, pCands, nCands );
    if ( RetValue == -1 )
        return 0;
    if ( RetValue == 1 )
    {
        if ( fVeryVerbose )
        printf( "Node %d: Fanins %d/%d can be removed.\n", pNode->Id, iFanin, iFanin2 );
        p->nNodesResub++;
        p->nNodesGainedLevel++;
clk = clock();
        // derive the function
        pFunc = Abc_NtkMfsInterplate( p, pCands, nCands );
        if ( pFunc == NULL )
            return 0;
        // update the network
        Abc_NtkMfsUpdateNetwork( p, pNode, p->vFanins, pFunc );
p->timeInt += clock() - clk;
        return 1;
    }

    if ( fVeryVerbose )
    {
        for ( i = 0; i < 11; i++ )
            printf( " " );
        for ( i = 0; i < Vec_PtrSize(p->vDivs)-Abc_ObjFaninNum(pNode); i++ )
            printf( "%d", i % 10 );
        for ( i = 0; i < Abc_ObjFaninNum(pNode); i++ )
            if ( i == iFanin || i == iFanin2 )
                printf( "*" );
            else
                printf( "%c", 'a' + i );
        printf( "\n" );
    }
    iVar = iVar2 = -1;
    while ( 1 )
    {
        if ( fVeryVerbose )
        {
            printf( "%3d: %2d %2d ", p->nCexes, iVar, iVar2 );
            for ( i = 0; i < Vec_PtrSize(p->vDivs); i++ )
            {
                pData = Vec_PtrEntry( p->vDivCexes, i );
                printf( "%d", Aig_InfoHasBit(pData, p->nCexes-1) );
            }
            printf( "\n" );
        }

        // find the next divisor to try
        nWords = Aig_BitWordNum(p->nCexes);
        assert( nWords <= p->nDivWords );
        fBreak = 0;
        for ( iVar = 1; iVar < Vec_PtrSize(p->vDivs)-Abc_ObjFaninNum(pNode); iVar++ )
        {
            pData  = Vec_PtrEntry( p->vDivCexes, iVar );
            for ( iVar2 = 0; iVar2 < iVar; iVar2++ )
            {
                pData2 = Vec_PtrEntry( p->vDivCexes, iVar2 );
                for ( w = 0; w < nWords; w++ )
                    if ( (pData[w] | pData2[w]) != ~0 )
                        break;
                if ( w == nWords )
                {
                    fBreak = 1;
                    break;
                }
            }
            if ( fBreak )
                break;
        }
        if ( iVar == Vec_PtrSize(p->vDivs)-Abc_ObjFaninNum(pNode) )
            return 0;

        pCands[nCands]   = toLitCond( Vec_IntEntry(p->vProjVars, iVar2), 1 );
        pCands[nCands+1] = toLitCond( Vec_IntEntry(p->vProjVars, iVar), 1 );
        RetValue = Abc_NtkMfsTryResubOnce( p, pCands, nCands+2 );
        if ( RetValue == -1 )
            return 0;
        if ( RetValue == 1 )
        {
            if ( fVeryVerbose )
            printf( "Node %d: Fanins %d/%d can be replaced by divisors %d/%d.\n", pNode->Id, iFanin, iFanin2, iVar, iVar2 );
            p->nNodesResub++;
            p->nNodesGainedLevel++;
clk = clock();
            // derive the function
            pFunc = Abc_NtkMfsInterplate( p, pCands, nCands+2 );
            if ( pFunc == NULL )
                return 0;
            // update the network
            Vec_PtrPush( p->vFanins, Vec_PtrEntry(p->vDivs, iVar2) );
            Vec_PtrPush( p->vFanins, Vec_PtrEntry(p->vDivs, iVar) );
            assert( Vec_PtrSize(p->vFanins) == nCands + 2 );
            Abc_NtkMfsUpdateNetwork( p, pNode, p->vFanins, pFunc );
p->timeInt += clock() - clk;
            return 1;
        }
        if ( p->nCexes >= p->pPars->nDivMax )
            break;
    }
    return 0;
}


/**Function*************************************************************

  Synopsis    [Evaluates the possibility of replacing given edge by another edge.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkMfsEdgeSwapEval( Mfs_Man_t * p, Abc_Obj_t * pNode )
{
    Abc_Obj_t * pFanin;
    int i;
    Abc_ObjForEachFanin( pNode, pFanin, i )
        Abc_NtkMfsSolveSatResub( p, pNode, i, 0, 1 );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Evaluates the possibility of replacing given edge by another edge.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkMfsEdgePower( Mfs_Man_t * p, Abc_Obj_t * pNode )
{
    Abc_Obj_t * pFanin;
    float * pProbab = (float *)p->vProbs->pArray;
    int i;
    // try replacing area critical fanins
    Abc_ObjForEachFanin( pNode, pFanin, i )
        if ( pProbab[pFanin->Id] >= 0.4 )
        {
            if ( Abc_NtkMfsSolveSatResub( p, pNode, i, 0, 0 ) )
                return 1;
        }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Performs resubstitution for the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkMfsResubNode( Mfs_Man_t * p, Abc_Obj_t * pNode )
{
    Abc_Obj_t * pFanin;
    int i;
    // try replacing area critical fanins
    Abc_ObjForEachFanin( pNode, pFanin, i )
        if ( !Abc_ObjIsCi(pFanin) && Abc_ObjFanoutNum(pFanin) == 1 )
        {
            if ( Abc_NtkMfsSolveSatResub( p, pNode, i, 0, 0 ) )
                return 1;
        }
    // try removing redundant edges
    if ( !p->pPars->fArea )
    {
        Abc_ObjForEachFanin( pNode, pFanin, i )
            if ( Abc_ObjIsCi(pFanin) || Abc_ObjFanoutNum(pFanin) != 1 )
            {
                if ( Abc_NtkMfsSolveSatResub( p, pNode, i, 1, 0 ) )
                    return 1;
            }
    }
    if ( Abc_ObjFaninNum(pNode) == p->nFaninMax )
        return 0;
    // try replacing area critical fanins while adding two new fanins
    Abc_ObjForEachFanin( pNode, pFanin, i )
        if ( !Abc_ObjIsCi(pFanin) && Abc_ObjFanoutNum(pFanin) == 1 )
        {
            if ( Abc_NtkMfsSolveSatResub2( p, pNode, i, -1 ) )
                return 1;
        }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Performs resubstitution for the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkMfsResubNode2( Mfs_Man_t * p, Abc_Obj_t * pNode )
{
    Abc_Obj_t * pFanin, * pFanin2;
    int i, k;
/*
    Abc_ObjForEachFanin( pNode, pFanin, i )
        if ( !Abc_ObjIsCi(pFanin) && Abc_ObjFanoutNum(pFanin) == 1 )
        {
            if ( Abc_NtkMfsSolveSatResub( p, pNode, i, 0, 0 ) )
                return 1;
        }
*/
    if ( Abc_ObjFaninNum(pNode) < 2 )
        return 0;
    // try replacing one area critical fanin and one other fanin while adding two new fanins
    Abc_ObjForEachFanin( pNode, pFanin, i )
    {
        if ( !Abc_ObjIsCi(pFanin) && Abc_ObjFanoutNum(pFanin) == 1 )
        {
            // consider second fanin to remove at the same time
            Abc_ObjForEachFanin( pNode, pFanin2, k )
            {
                if ( i != k && Abc_NtkMfsSolveSatResub2( p, pNode, i, k ) )
                    return 1;
            }
        }
    }
    return 0;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

