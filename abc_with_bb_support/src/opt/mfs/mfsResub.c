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
void Abc_NtkMfsUpdateNetwork( Mfs_Man_t * p, Abc_Obj_t * pObj, Vec_Ptr_t * vMfsFanins, Hop_Obj_t * pFunc )
{
    Abc_Obj_t * pObjNew, * pFanin;
    int k;
    // create the new node
    pObjNew = Abc_NtkCreateNode( pObj->pNtk );
    pObjNew->pData = pFunc;
    Vec_PtrForEachEntry( Abc_Obj_t *, vMfsFanins, pFanin, k )
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
//    printf( "Total area-critical fanins = %d. Belonging to expandable nodes = %d.\n", 
//        nAreaCrits, nAreaExpanse );
}

/**Function*************************************************************

  Synopsis    [Tries resubstitution.]

  Description [Returns 1 if it is feasible, or 0 if c-ex is found.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkMfsTryResubOnce( Mfs_Man_t * p, int * pCands, int nCands )
{
    int fVeryVerbose = 0;
    unsigned * pData;
    int RetValue, RetValue2 = -1, iVar, i;//, clk = Abc_Clock();
/*
    if ( p->pPars->fGiaSat )
    {
        RetValue2 = Abc_NtkMfsTryResubOnceGia( p, pCands, nCands );
p->timeGia += Abc_Clock() - clk;
        return RetValue2;
    }
*/ 
    p->nSatCalls++;
    RetValue = sat_solver_solve( p->pSat, pCands, pCands + nCands, (ABC_INT64_T)p->pPars->nBTLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
//    assert( RetValue == l_False || RetValue == l_True );

    if ( RetValue != l_Undef && RetValue2 != -1 )
    {
        assert( (RetValue == l_False) == (RetValue2 == 1) );
    }

    if ( RetValue == l_False )
    {
        if ( fVeryVerbose )
        printf( "U " );
        return 1;
    }
    if ( RetValue != l_True )
    {
        if ( fVeryVerbose )
        printf( "T " );
        p->nTimeOuts++;
        return -1;
    }
    if ( fVeryVerbose )
    printf( "S " );
    p->nSatCexes++;
    // store the counter-example
    Vec_IntForEachEntry( p->vProjVarsSat, iVar, i )
    {
        pData = (unsigned *)Vec_PtrEntry( p->vDivCexes, i );
        if ( !sat_solver_var_value( p->pSat, iVar ) ) // remove 0s!!!
        {
            assert( Abc_InfoHasBit(pData, p->nCexes) );
            Abc_InfoXorBit( pData, p->nCexes );
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
    int fVeryVerbose = 0;//p->pPars->fVeryVerbose && Vec_PtrSize(p->vDivs) < 200;// || pNode->Id == 556;
    unsigned * pData;
    int pCands[MFS_FANIN_MAX];
    int RetValue, iVar, i, nCands, nWords, w;
    abctime clk;
    Abc_Obj_t * pFanin;
    Hop_Obj_t * pFunc;
    assert( iFanin >= 0 );
    p->nTryRemoves++;

    // clean simulation info
    Vec_PtrFillSimInfo( p->vDivCexes, 0, p->nDivWords ); 
    p->nCexes = 0;
    if ( p->pPars->fVeryVerbose )
    {
//        printf( "\n" );
        printf( "%5d : Lev =%3d. Leaf =%3d. Node =%3d. Divs =%3d.  Fanin = %4d (%d/%d), MFFC = %d\n", 
            pNode->Id, pNode->Level, Vec_PtrSize(p->vSupp), Vec_PtrSize(p->vNodes), Vec_PtrSize(p->vDivs)-Abc_ObjFaninNum(pNode), 
            Abc_ObjFaninId(pNode, iFanin), iFanin, Abc_ObjFaninNum(pNode), 
            Abc_ObjFanoutNum(Abc_ObjFanin(pNode, iFanin)) == 1 ? Abc_NodeMffcLabel(Abc_ObjFanin(pNode, iFanin)) : 0 );
    }

    // try fanins without the critical fanin
    nCands = 0;
    Vec_PtrClear( p->vMfsFanins );
    Abc_ObjForEachFanin( pNode, pFanin, i )
    {
        if ( i == iFanin )
            continue;
        Vec_PtrPush( p->vMfsFanins, pFanin );
        iVar = Vec_PtrSize(p->vDivs) - Abc_ObjFaninNum(pNode) + i;
        pCands[nCands++] = toLitCond( Vec_IntEntry( p->vProjVarsSat, iVar ), 1 );
    }
    RetValue = Abc_NtkMfsTryResubOnce( p, pCands, nCands );
    if ( RetValue == -1 )
        return 0;
    if ( RetValue == 1 )
    {
        if ( p->pPars->fVeryVerbose )
            printf( "Node %d: Fanin %d can be removed.\n", pNode->Id, iFanin );
        p->nNodesResub++;
        p->nNodesGainedLevel++;
        if ( fSkipUpdate )
            return 1;
clk = Abc_Clock();
        // derive the function
        pFunc = Abc_NtkMfsInterplate( p, pCands, nCands );
        if ( pFunc == NULL )
            return 0;
        // update the network
        Abc_NtkMfsUpdateNetwork( p, pNode, p->vMfsFanins, pFunc );
p->timeInt += Abc_Clock() - clk;
        p->nRemoves++;
        return 1;
    }

    if ( fOnlyRemove || p->pPars->fRrOnly )
        return 0;

    p->nTryResubs++;
    if ( fVeryVerbose )
    {
        for ( i = 0; i < 9; i++ )
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
        if ( fVeryVerbose )
        {
            printf( "%3d: %3d ", p->nCexes, iVar );
            for ( i = 0; i < Vec_PtrSize(p->vDivs); i++ )
            {
                pData = (unsigned *)Vec_PtrEntry( p->vDivCexes, i );
                printf( "%d", Abc_InfoHasBit(pData, p->nCexes-1) );
            }
            printf( "\n" );
        }

        // find the next divisor to try
        nWords = Abc_BitWordNum(p->nCexes);
        assert( nWords <= p->nDivWords );
        for ( iVar = 0; iVar < Vec_PtrSize(p->vDivs)-Abc_ObjFaninNum(pNode); iVar++ )
        {
            if ( p->pPars->fPower )
            {
                Abc_Obj_t * pDiv = (Abc_Obj_t *)Vec_PtrEntry(p->vDivs, iVar);
                // only accept the divisor if it is "cool"
                if ( Abc_MfsObjProb(p, pDiv) >= 0.15 )
                    continue;
            }
            pData  = (unsigned *)Vec_PtrEntry( p->vDivCexes, iVar );
            for ( w = 0; w < nWords; w++ )
                if ( pData[w] != ~0 )
                    break;
            if ( w == nWords )
                break;
        }
        if ( iVar == Vec_PtrSize(p->vDivs)-Abc_ObjFaninNum(pNode) )
            return 0;

        pCands[nCands] = toLitCond( Vec_IntEntry(p->vProjVarsSat, iVar), 1 );
        RetValue = Abc_NtkMfsTryResubOnce( p, pCands, nCands+1 );
        if ( RetValue == -1 )
            return 0;
        if ( RetValue == 1 )
        {
            if ( p->pPars->fVeryVerbose )
                printf( "Node %d: Fanin %d can be replaced by divisor %d.\n", pNode->Id, iFanin, iVar );
            p->nNodesResub++;
            p->nNodesGainedLevel++;
            if ( fSkipUpdate )
                return 1;
clk = Abc_Clock();
            // derive the function
            pFunc = Abc_NtkMfsInterplate( p, pCands, nCands+1 );
            if ( pFunc == NULL )
                return 0;
            // update the network
            Vec_PtrPush( p->vMfsFanins, Vec_PtrEntry(p->vDivs, iVar) );
            Abc_NtkMfsUpdateNetwork( p, pNode, p->vMfsFanins, pFunc );
p->timeInt += Abc_Clock() - clk;
            p->nResubs++;
            return 1;
        }
        if ( p->nCexes >= p->pPars->nWinMax )
            break;
    }
    if ( p->pPars->fVeryVerbose )
        printf( "Node %d: Cannot find replacement for fanin %d.\n", pNode->Id, iFanin );
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
    abctime clk;
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
    Vec_PtrClear( p->vMfsFanins );
    Abc_ObjForEachFanin( pNode, pFanin, i )
    {
        if ( i == iFanin || i == iFanin2 )
            continue;
        Vec_PtrPush( p->vMfsFanins, pFanin );
        iVar = Vec_PtrSize(p->vDivs) - Abc_ObjFaninNum(pNode) + i;
        pCands[nCands++] = toLitCond( Vec_IntEntry( p->vProjVarsSat, iVar ), 1 );
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
clk = Abc_Clock();
        // derive the function
        pFunc = Abc_NtkMfsInterplate( p, pCands, nCands );
        if ( pFunc == NULL )
            return 0;
        // update the network
        Abc_NtkMfsUpdateNetwork( p, pNode, p->vMfsFanins, pFunc );
p->timeInt += Abc_Clock() - clk;
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
#if 1 // sjang
#endif
        if ( fVeryVerbose )
        {
            printf( "%3d: %2d %2d ", p->nCexes, iVar, iVar2 );
            for ( i = 0; i < Vec_PtrSize(p->vDivs); i++ )
            {
                pData = (unsigned *)Vec_PtrEntry( p->vDivCexes, i );
                printf( "%d", Abc_InfoHasBit(pData, p->nCexes-1) );
            }
            printf( "\n" );
        }

        // find the next divisor to try
        nWords = Abc_BitWordNum(p->nCexes);
        assert( nWords <= p->nDivWords );
        fBreak = 0;
        for ( iVar = 1; iVar < Vec_PtrSize(p->vDivs)-Abc_ObjFaninNum(pNode); iVar++ )
        {
            pData  = (unsigned *)Vec_PtrEntry( p->vDivCexes, iVar );
#if 1  // sjang
            if ( p->pPars->fPower )
            {
                Abc_Obj_t * pDiv = (Abc_Obj_t *)Vec_PtrEntry(p->vDivs, iVar);
                // only accept the divisor if it is "cool"
                if ( Abc_MfsObjProb(p, pDiv) >= 0.12 )
                    continue;
            }
#endif
            for ( iVar2 = 0; iVar2 < iVar; iVar2++ )
            {
                pData2 = (unsigned *)Vec_PtrEntry( p->vDivCexes, iVar2 );
#if 1 // sjang
	            if ( p->pPars->fPower )
		        {
			        Abc_Obj_t * pDiv = (Abc_Obj_t *)Vec_PtrEntry(p->vDivs, iVar2);
				    // only accept the divisor if it is "cool"
					if ( Abc_MfsObjProb(p, pDiv) >= 0.12 )
						continue;
	            }
#endif
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

        pCands[nCands]   = toLitCond( Vec_IntEntry(p->vProjVarsSat, iVar2), 1 );
        pCands[nCands+1] = toLitCond( Vec_IntEntry(p->vProjVarsSat, iVar), 1 );
        RetValue = Abc_NtkMfsTryResubOnce( p, pCands, nCands+2 );
        if ( RetValue == -1 )
            return 0;
        if ( RetValue == 1 )
        {
            if ( fVeryVerbose )
            printf( "Node %d: Fanins %d/%d can be replaced by divisors %d/%d.\n", pNode->Id, iFanin, iFanin2, iVar, iVar2 );
            p->nNodesResub++;
            p->nNodesGainedLevel++;
clk = Abc_Clock();
            // derive the function
            pFunc = Abc_NtkMfsInterplate( p, pCands, nCands+2 );
            if ( pFunc == NULL )
                return 0;
            // update the network
            Vec_PtrPush( p->vMfsFanins, Vec_PtrEntry(p->vDivs, iVar2) );
            Vec_PtrPush( p->vMfsFanins, Vec_PtrEntry(p->vDivs, iVar) );
            assert( Vec_PtrSize(p->vMfsFanins) == nCands + 2 );
            Abc_NtkMfsUpdateNetwork( p, pNode, p->vMfsFanins, pFunc );
p->timeInt += Abc_Clock() - clk;
            return 1;
        }
        if ( p->nCexes >= p->pPars->nWinMax )
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
    int i;
    // try replacing area critical fanins
    Abc_ObjForEachFanin( pNode, pFanin, i )
	{
        if ( Abc_MfsObjProb(p, pFanin) >= 0.35 )
        {
            if ( Abc_NtkMfsSolveSatResub( p, pNode, i, 0, 0 ) )
                return 1;
        } else if ( Abc_MfsObjProb(p, pFanin) >= 0.25 ) // sjang
        {
            if ( Abc_NtkMfsSolveSatResub( p, pNode, i, 1, 0 ) )
                return 1;
        }
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
/*
    // try replacing area critical fanins while adding two new fanins
    Abc_ObjForEachFanin( pNode, pFanin, i )
        if ( !Abc_ObjIsCi(pFanin) && Abc_ObjFanoutNum(pFanin) == 1 )
        {
            if ( Abc_NtkMfsSolveSatResub2( p, pNode, i, -1 ) )
                return 1;
        }
*/
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

