/**CFile****************************************************************

  FileName    [saigConstr2.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Sequential AIG package.]

  Synopsis    [Functional constraint detection.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: saigConstr2.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "saig.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satSolver.h"
#include "bool/kit/kit.h"
#include "misc/bar/bar.h"

ABC_NAMESPACE_IMPL_START



////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline Aig_Obj_t * Aig_ObjFrames( Aig_Obj_t ** pObjMap, int nFs, Aig_Obj_t * pObj, int i )                       { return pObjMap[nFs*pObj->Id + i];  }
static inline void        Aig_ObjSetFrames( Aig_Obj_t ** pObjMap, int nFs, Aig_Obj_t * pObj, int i, Aig_Obj_t * pNode ) { pObjMap[nFs*pObj->Id + i] = pNode; }

static inline Aig_Obj_t * Aig_ObjChild0Frames( Aig_Obj_t ** pObjMap, int nFs, Aig_Obj_t * pObj, int i ) { return Aig_ObjFanin0(pObj)? Aig_NotCond(Aig_ObjFrames(pObjMap,nFs,Aig_ObjFanin0(pObj),i), Aig_ObjFaninC0(pObj)) : NULL;  }
static inline Aig_Obj_t * Aig_ObjChild1Frames( Aig_Obj_t ** pObjMap, int nFs, Aig_Obj_t * pObj, int i ) { return Aig_ObjFanin1(pObj)? Aig_NotCond(Aig_ObjFrames(pObjMap,nFs,Aig_ObjFanin1(pObj),i), Aig_ObjFaninC1(pObj)) : NULL;  }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
 
/**Function*************************************************************

  Synopsis    [Returns the probability of POs being 1 under rand seq sim.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_ManProfileConstraints( Aig_Man_t * p, int nWords, int nFrames, int fVerbose )
{
    Vec_Ptr_t * vInfo;
    Vec_Int_t * vProbs, * vProbs2;
    Aig_Obj_t * pObj, * pObjLi;
    unsigned * pInfo, * pInfo0, * pInfo1, * pInfoMask, * pInfoMask2;
    int i, w, f, RetValue = 1;
    abctime clk = Abc_Clock();
    if ( fVerbose )
        printf( "Simulating %d nodes and %d flops for %d frames with %d words... ", 
            Aig_ManNodeNum(p), Aig_ManRegNum(p), nFrames, nWords );
    Aig_ManRandom( 1 );
    vInfo = Vec_PtrAllocSimInfo( Aig_ManObjNumMax(p)+2, nWords );
    Vec_PtrCleanSimInfo( vInfo, 0, nWords );
    vProbs  = Vec_IntStart( Saig_ManPoNum(p) );
    vProbs2 = Vec_IntStart( Saig_ManPoNum(p) );
    // start the constant
    pInfo = (unsigned *)Vec_PtrEntry( vInfo, Aig_ObjId(Aig_ManConst1(p)) );
    for ( w = 0; w < nWords; w++ )
        pInfo[w] = ~0;
    // start the flop inputs
    Saig_ManForEachLi( p, pObj, i )
    {
        pInfo = (unsigned *)Vec_PtrEntry( vInfo, Aig_ObjId(pObj) );
        for ( w = 0; w < nWords; w++ )
            pInfo[w] = 0;
    }
    // get the info mask
    pInfoMask  = (unsigned *)Vec_PtrEntry( vInfo, Aig_ManObjNumMax(p) );    // PO failed
    pInfoMask2 = (unsigned *)Vec_PtrEntry( vInfo, Aig_ManObjNumMax(p)+1 );  // constr failed
    for ( f = 0; f < nFrames; f++ )
    {
        // assign primary inputs
        Saig_ManForEachPi( p, pObj, i )
        {
            pInfo = (unsigned *)Vec_PtrEntry( vInfo, Aig_ObjId(pObj) );
            for ( w = 0; w < nWords; w++ )
                pInfo[w] = Aig_ManRandom( 0 );
        }
        // move the flop values
        Saig_ManForEachLiLo( p, pObjLi, pObj, i )
        {
            pInfo  = (unsigned *)Vec_PtrEntry( vInfo, Aig_ObjId(pObj) );
            pInfo0 = (unsigned *)Vec_PtrEntry( vInfo, Aig_ObjId(pObjLi) );
            for ( w = 0; w < nWords; w++ )
                pInfo[w] = pInfo0[w];
        }
        // simulate the nodes
        Aig_ManForEachNode( p, pObj, i )
        {
            pInfo  = (unsigned *)Vec_PtrEntry( vInfo, Aig_ObjId(pObj) );
            pInfo0 = (unsigned *)Vec_PtrEntry( vInfo, Aig_ObjFaninId0(pObj) );
            pInfo1 = (unsigned *)Vec_PtrEntry( vInfo, Aig_ObjFaninId1(pObj) );
            if ( Aig_ObjFaninC0(pObj) )
            {
                if (  Aig_ObjFaninC1(pObj) )
                    for ( w = 0; w < nWords; w++ )
                        pInfo[w] = ~(pInfo0[w] | pInfo1[w]);
                else 
                    for ( w = 0; w < nWords; w++ )
                        pInfo[w] = ~pInfo0[w] & pInfo1[w];
            }
            else 
            {
                if (  Aig_ObjFaninC1(pObj) )
                    for ( w = 0; w < nWords; w++ )
                        pInfo[w] = pInfo0[w] & ~pInfo1[w];
                else 
                    for ( w = 0; w < nWords; w++ )
                        pInfo[w] = pInfo0[w] & pInfo1[w];
            }
        }
        // clean the mask
        for ( w = 0; w < nWords; w++ )
            pInfoMask[w] = pInfoMask2[w] = 0;
        // simulate the primary outputs
        Aig_ManForEachCo( p, pObj, i )
        {
            pInfo  = (unsigned *)Vec_PtrEntry( vInfo, Aig_ObjId(pObj) );
            pInfo0 = (unsigned *)Vec_PtrEntry( vInfo, Aig_ObjFaninId0(pObj) );
            if ( i < Saig_ManPoNum(p)-Saig_ManConstrNum(p) || i >= Saig_ManPoNum(p) )
            {
                if ( Aig_ObjFaninC0(pObj) )
                {
                    for ( w = 0; w < nWords; w++ )
                        pInfo[w] = ~pInfo0[w];
                }
                else 
                {
                    for ( w = 0; w < nWords; w++ )
                        pInfo[w] = pInfo0[w];
                }
            }
            else
            {
                if ( Aig_ObjFaninC0(pObj) )
                {
                    for ( w = 0; w < nWords; w++ )
                        pInfo[w] |= ~pInfo0[w];
                }
                else 
                {
                    for ( w = 0; w < nWords; w++ )
                        pInfo[w] |= pInfo0[w];
                }
            }
            // collect patterns when one of the outputs fails
            if ( i < Saig_ManPoNum(p)-Saig_ManConstrNum(p) )
            {
                for ( w = 0; w < nWords; w++ )
                    pInfoMask[w] |= pInfo[w];
            }
            else if ( i < Saig_ManPoNum(p) )
            {
                for ( w = 0; w < nWords; w++ )
                    pInfoMask2[w] |= pInfo[w];
            }
        }
        // compare the PO values (mask=1 => out=0) or UNSAT(mask=1 & out=1)
        Saig_ManForEachPo( p, pObj, i )
        {
            pInfo  = (unsigned *)Vec_PtrEntry( vInfo, Aig_ObjId(pObj) );
            for ( w = 0; w < nWords; w++ )
                Vec_IntAddToEntry( vProbs, i, Aig_WordCountOnes(pInfo[w]) );
            if ( i < Saig_ManPoNum(p)-Saig_ManConstrNum(p) )
            {
                // chek the output
                for ( w = 0; w < nWords; w++ )
                    if ( pInfo[w] & ~pInfoMask2[w] ) 
                        break;
                if ( w == nWords )
                    continue;
                printf( "Primary output %d fails on some input patterns.\n", i );
            }
            else
            {
                // collect patterns that block the POs
                for ( w = 0; w < nWords; w++ )
                    Vec_IntAddToEntry( vProbs2, i, Aig_WordCountOnes(pInfo[w] & pInfoMask[w]) );
            }
        }
    }
    if ( fVerbose )
        Abc_PrintTime( 1, "T", Abc_Clock() - clk );
    // print the state
    if ( fVerbose )
    {
        Saig_ManForEachPo( p, pObj, i )
        {
            if ( i < Saig_ManPoNum(p) - Saig_ManConstrNum(p) )
                printf( "Primary output :  " );
            else
                printf( "Constraint %3d :  ", i-(Saig_ManPoNum(p) - Saig_ManConstrNum(p))  );
            printf( "ProbOne = %f  ",  (float)Vec_IntEntry(vProbs,  i)/(32*nWords*nFrames) );
            printf( "ProbOneC = %f  ", (float)Vec_IntEntry(vProbs2, i)/(32*nWords*nFrames) );
            printf( "AllZeroValue = %d ", Aig_ObjPhase(pObj) );
            printf( "\n" );
        }
    }

    // print the states
    Vec_PtrFree( vInfo );
    Vec_IntFree( vProbs );
    Vec_IntFree( vProbs2 );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Creates COI of the property output.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManCreateIndMiter( Aig_Man_t * pAig, Vec_Vec_t * vCands )
{
    int nFrames = 2;
    Vec_Ptr_t * vNodes;
    Aig_Man_t * pFrames;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo, * pObjNew;
    Aig_Obj_t ** pObjMap;
    int i, f, k;

    // create mapping for the frames nodes
    pObjMap  = ABC_CALLOC( Aig_Obj_t *, nFrames * Aig_ManObjNumMax(pAig) );

    // start the fraig package
    pFrames = Aig_ManStart( Aig_ManObjNumMax(pAig) * nFrames );
    pFrames->pName = Abc_UtilStrsav( pAig->pName );
    pFrames->pSpec = Abc_UtilStrsav( pAig->pSpec );
    // map constant nodes
    for ( f = 0; f < nFrames; f++ )
        Aig_ObjSetFrames( pObjMap, nFrames, Aig_ManConst1(pAig), f, Aig_ManConst1(pFrames) );
    // create PI nodes for the frames
    for ( f = 0; f < nFrames; f++ )
        Aig_ManForEachPiSeq( pAig, pObj, i )
            Aig_ObjSetFrames( pObjMap, nFrames, pObj, f, Aig_ObjCreateCi(pFrames) );
    // set initial state for the latches
    Aig_ManForEachLoSeq( pAig, pObj, i )
        Aig_ObjSetFrames( pObjMap, nFrames, pObj, 0, Aig_ObjCreateCi(pFrames) );

    // add timeframes
    for ( f = 0; f < nFrames; f++ )
    {
        // add internal nodes of this frame
        Aig_ManForEachNode( pAig, pObj, i )
        {
            pObjNew = Aig_And( pFrames, Aig_ObjChild0Frames(pObjMap,nFrames,pObj,f), Aig_ObjChild1Frames(pObjMap,nFrames,pObj,f) );
            Aig_ObjSetFrames( pObjMap, nFrames, pObj, f, pObjNew );
        }
        // set the latch inputs and copy them into the latch outputs of the next frame
        Aig_ManForEachLiLoSeq( pAig, pObjLi, pObjLo, i )
        {
            pObjNew = Aig_ObjChild0Frames(pObjMap,nFrames,pObjLi,f);
            if ( f < nFrames - 1 )
                Aig_ObjSetFrames( pObjMap, nFrames, pObjLo, f+1, pObjNew );
        }
    }

    // go through the candidates
    Vec_VecForEachLevel( vCands, vNodes, i )
    {
        Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, k )
        {
            Aig_Obj_t * pObjR  = Aig_Regular(pObj);
            Aig_Obj_t * pNode0 = pObjMap[nFrames*Aig_ObjId(pObjR)+0];
            Aig_Obj_t * pNode1 = pObjMap[nFrames*Aig_ObjId(pObjR)+1];
            Aig_Obj_t * pFan0  = Aig_NotCond( pNode0,  Aig_IsComplement(pObj) );
            Aig_Obj_t * pFan1  = Aig_NotCond( pNode1, !Aig_IsComplement(pObj) );
            Aig_Obj_t * pMiter = Aig_And( pFrames, pFan0, pFan1 );
            Aig_ObjCreateCo( pFrames, pMiter );
        }
    }
    Aig_ManCleanup( pFrames );
    ABC_FREE( pObjMap );

//Aig_ManShow( pAig, 0, NULL );
//Aig_ManShow( pFrames, 0, NULL );
    return pFrames;
}

/**Function*************************************************************

  Synopsis    [Performs inductive check for one of the constraints.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManFilterUsingIndOne_new( Aig_Man_t * p, Aig_Man_t * pFrame, sat_solver * pSat, Cnf_Dat_t * pCnf, int nConfs, int nProps, int Counter )
{
    Aig_Obj_t * pObj;
    int Lit, status;
    pObj = Aig_ManCo( pFrame, Counter );
    Lit  = toLitCond( pCnf->pVarNums[Aig_ObjId(pObj)], 0 );
    status = sat_solver_solve( pSat, &Lit, &Lit + 1, (ABC_INT64_T)nConfs, 0, 0, 0 );
    if ( status == l_False )
        return 1;
    if ( status == l_Undef )
    {
//        printf( "Solver returned undecided.\n" );
        return 0;
    }
    assert( status == l_True );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Detects constraints functionally.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManFilterUsingInd( Aig_Man_t * p, Vec_Vec_t * vCands, int nConfs, int nProps, int fVerbose )
{
    Vec_Ptr_t * vNodes;
    Aig_Man_t * pFrames;
    sat_solver * pSat;
    Cnf_Dat_t * pCnf;
    Aig_Obj_t * pObj;
    int i, k, k2, Counter;
/*
    Vec_VecForEachLevel( vCands, vNodes, i )
        Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, k )
            printf( "%d ", Aig_ObjId(Aig_Regular(pObj)) );
    printf( "\n" );
*/
    // create timeframes
//    pFrames = Saig_ManUnrollInd( p );
    pFrames = Saig_ManCreateIndMiter( p, vCands );
    assert( Aig_ManCoNum(pFrames) == Vec_VecSizeSize(vCands) );
    // start the SAT solver
    pCnf = Cnf_DeriveSimple( pFrames, Aig_ManCoNum(pFrames) );
    pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
    // check candidates
    if ( fVerbose )
        printf( "Filtered cands:  " );
    Counter = 0;
    Vec_VecForEachLevel( vCands, vNodes, i )
    {
        k2 = 0;
        Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, k )
        {
            if ( Saig_ManFilterUsingIndOne_new( p, pFrames, pSat, pCnf, nConfs, nProps, Counter++ ) )
//            if ( Saig_ManFilterUsingIndOne_old( p, pSat, pCnf, nConfs, pObj ) )
            {
                Vec_PtrWriteEntry( vNodes, k2++, pObj );
                if ( fVerbose )
                    printf( "%d:%s%d  ", i, Aig_IsComplement(pObj)? "!":"", Aig_ObjId(Aig_Regular(pObj)) );
            }
        }
        Vec_PtrShrink( vNodes, k2 );
    }
    if ( fVerbose )
        printf( "\n" );
    // clean up
    Cnf_DataFree( pCnf );
    sat_solver_delete( pSat );
    if ( fVerbose )
        Aig_ManPrintStats( pFrames );
    Aig_ManStop( pFrames );
}




/**Function*************************************************************

  Synopsis    [Creates COI of the property output.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManUnrollCOI_( Aig_Man_t * p, int nFrames )
{
    Aig_Man_t * pFrames;
    Aig_Obj_t ** pObjMap;
    int i;
//Aig_Man_t * Aig_ManFrames( Aig_Man_t * pAig, int nFrames, int fInit, int fOuts, int fRegs, int fEnlarge, Aig_Obj_t *** ppObjMap )
    pFrames = Aig_ManFrames( p, nFrames, 0, 1, 1, 0, &pObjMap );
    for ( i = 0; i < nFrames * Aig_ManObjNumMax(p); i++ )
        if ( pObjMap[i] && Aig_ObjIsNone( Aig_Regular(pObjMap[i]) ) )
            pObjMap[i] = NULL;
    assert( p->pObjCopies == NULL );
    p->pObjCopies = pObjMap;
    return pFrames;
}

/**Function*************************************************************

  Synopsis    [Creates COI of the property output.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManUnrollCOI( Aig_Man_t * pAig, int nFrames )
{
    Aig_Man_t * pFrames;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo, * pObjNew;
    Aig_Obj_t ** pObjMap;
    int i, f;
    // create mapping for the frames nodes
    pObjMap = ABC_CALLOC( Aig_Obj_t *, nFrames * Aig_ManObjNumMax(pAig) );
    // start the fraig package
    pFrames = Aig_ManStart( Aig_ManObjNumMax(pAig) * nFrames );
    pFrames->pName = Abc_UtilStrsav( pAig->pName );
    pFrames->pSpec = Abc_UtilStrsav( pAig->pSpec );
    // map constant nodes
    for ( f = 0; f < nFrames; f++ )
        Aig_ObjSetFrames( pObjMap, nFrames, Aig_ManConst1(pAig), f, Aig_ManConst1(pFrames) );
    // create PI nodes for the frames
    for ( f = 0; f < nFrames; f++ )
        Aig_ManForEachPiSeq( pAig, pObj, i )
            Aig_ObjSetFrames( pObjMap, nFrames, pObj, f, Aig_ObjCreateCi(pFrames) );
    // set initial state for the latches
    Aig_ManForEachLoSeq( pAig, pObj, i )
        Aig_ObjSetFrames( pObjMap, nFrames, pObj, 0, Aig_ObjCreateCi(pFrames) );
    // add timeframes
    for ( f = 0; f < nFrames; f++ )
    {
        Aig_ManForEachNode( pAig, pObj, i )
        {
            pObjNew = Aig_And( pFrames, Aig_ObjChild0Frames(pObjMap,nFrames,pObj,f), Aig_ObjChild1Frames(pObjMap,nFrames,pObj,f) );
            Aig_ObjSetFrames( pObjMap, nFrames, pObj, f, pObjNew );
        }
        // set the latch inputs and copy them into the latch outputs of the next frame
        Aig_ManForEachLiLoSeq( pAig, pObjLi, pObjLo, i )
        {
            pObjNew = Aig_ObjChild0Frames(pObjMap,nFrames,pObjLi,f);
            if ( f < nFrames - 1 )
                Aig_ObjSetFrames( pObjMap, nFrames, pObjLo, f+1, pObjNew );
        }
    }
    // create the only output
    for ( f = nFrames-1; f < nFrames; f++ )
    {
        Aig_ManForEachPoSeq( pAig, pObj, i )
        {
            pObjNew = Aig_ObjCreateCo( pFrames, Aig_ObjChild0Frames(pObjMap,nFrames,pObj,f) );
            Aig_ObjSetFrames( pObjMap, nFrames, pObj, f, pObjNew );
        }
    }
    // created lots of dangling nodes - no sweeping!
    //Aig_ManCleanup( pFrames );
    assert( pAig->pObjCopies == NULL );
    pAig->pObjCopies = pObjMap;
    return pFrames;
}


/**Function*************************************************************

  Synopsis    [Collects and saves values of the SAT variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_CollectSatValues( sat_solver * pSat, Cnf_Dat_t * pCnf, Vec_Ptr_t * vInfo, int * piPat )
{
    Aig_Obj_t * pObj;
    unsigned * pInfo;
    int i;
    Aig_ManForEachObj( pCnf->pMan, pObj, i )
    {
        if ( !Aig_ObjIsNode(pObj) && !Aig_ObjIsCi(pObj) )
            continue;
        assert( pCnf->pVarNums[i] > 0 );
        pInfo = (unsigned *)Vec_PtrEntry( vInfo, i );
        if ( Abc_InfoHasBit(pInfo, *piPat) != sat_solver_var_value(pSat, pCnf->pVarNums[i]) )
            Abc_InfoXorBit(pInfo, *piPat);
    }
}

/**Function*************************************************************

  Synopsis    [Runs the SAT test for the node in one polarity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_DetectTryPolarity( sat_solver * pSat, int nConfs, int nProps, Cnf_Dat_t * pCnf, Aig_Obj_t * pObj, int iPol, Vec_Ptr_t * vInfo, int * piPat, int fVerbose )
{
    Aig_Obj_t * pOut = Aig_ManCo( pCnf->pMan, 0 );
    int status, Lits[2];
//    ABC_INT64_T nOldConfs = pSat->stats.conflicts;
//    ABC_INT64_T nOldImps = pSat->stats.propagations;
    Lits[0] = toLitCond( pCnf->pVarNums[Aig_ObjId(pOut)], 0 );
    Lits[1] = toLitCond( pCnf->pVarNums[Aig_ObjId(pObj)], !iPol );
    status = sat_solver_solve( pSat, Lits, Lits + 2, (ABC_INT64_T)nConfs, (ABC_INT64_T)nProps, 0, 0 );
    if ( status == l_False )
    {
//        printf( "u%d(%d) ", (int)(pSat->stats.conflicts-nOldConfs), (int)(pSat->stats.propagations-nOldImps) );
        return 1;
    }
    if ( status == l_Undef )
    {
//        printf( "Solver returned undecided.\n" );
        return 0;
    }
//    printf( "s%d(%d) ", (int)(pSat->stats.conflicts-nOldConfs), (int)(pSat->stats.propagations-nOldImps) );
    assert( status == l_True );
    Saig_CollectSatValues( pSat, pCnf, vInfo, piPat );
    (*piPat)++;
    if ( *piPat == Vec_PtrReadWordsSimInfo(vInfo) * 32 )
    {
        if ( fVerbose )
            printf( "Warning: Reached the limit on the number of patterns.\n" );
        *piPat = 0;
    }
    return 0;
}
 
/**Function*************************************************************

  Synopsis    [Returns the number of variables implied by the output.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Vec_t * Ssw_ManFindDirectImplications( Aig_Man_t * p, int nFrames, int nConfs, int nProps, int fVerbose )
{
    Vec_Vec_t * vCands = NULL;
    Vec_Ptr_t * vNodes;
    Cnf_Dat_t * pCnf;
    sat_solver * pSat;
    Aig_Man_t * pFrames;
    Aig_Obj_t * pObj, * pRepr, * pReprR;
    int i, f, k, value;
    vCands = Vec_VecAlloc( nFrames );

    // perform unrolling
    pFrames = Saig_ManUnrollCOI( p, nFrames );
    assert( Aig_ManCoNum(pFrames) == 1 );
    // start the SAT solver
    pCnf = Cnf_DeriveSimple( pFrames, 0 );
    pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
    if ( pSat != NULL )
    {
        Aig_ManIncrementTravId( p );
        for ( f = 0; f < nFrames; f++ )
        {
            Aig_ManForEachObj( p, pObj, i )
            {
                if ( !Aig_ObjIsCand(pObj) )
                    continue;
                if ( Aig_ObjIsTravIdCurrent(p, pObj) )
                    continue;
                // get the node from timeframes
                pRepr  = p->pObjCopies[nFrames*i + nFrames-1-f];
                pReprR = Aig_Regular(pRepr);
                if ( pCnf->pVarNums[Aig_ObjId(pReprR)] < 0 )
                    continue;
//                value = pSat->assigns[ pCnf->pVarNums[Aig_ObjId(pReprR)] ];
                value = sat_solver_get_var_value( pSat, pCnf->pVarNums[Aig_ObjId(pReprR)] );
                if ( value == l_Undef )
                    continue;
                // label this node as taken
                Aig_ObjSetTravIdCurrent(p, pObj);
                if ( Saig_ObjIsLo(p, pObj) )
                    Aig_ObjSetTravIdCurrent( p, Aig_ObjFanin0(Saig_ObjLoToLi(p, pObj)) );
                // remember the node
                Vec_VecPush( vCands, f, Aig_NotCond( pObj, (value == l_True) ^ Aig_IsComplement(pRepr) ) );
        //        printf( "%s%d ", (value == l_False)? "":"!", i );
            }
        }
    //    printf( "\n" );
        sat_solver_delete( pSat );
    }
    Aig_ManStop( pFrames );
    Cnf_DataFree( pCnf );

    if ( fVerbose )
    {
        printf( "Found %3d candidates.\n", Vec_VecSizeSize(vCands) );
        Vec_VecForEachLevel( vCands, vNodes, k )
        {
            printf( "Level %d. Cands  =%d    ", k, Vec_PtrSize(vNodes) );
//            Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
//                printf( "%d:%s%d ", k, Aig_IsComplement(pObj)? "!":"", Aig_ObjId(Aig_Regular(pObj)) );
            printf( "\n" );
        }
    }

    ABC_FREE( p->pObjCopies );
    Saig_ManFilterUsingInd( p, vCands, nConfs, nProps, fVerbose );
    if ( Vec_VecSizeSize(vCands) )
        printf( "Found %3d constraints after filtering.\n", Vec_VecSizeSize(vCands) );
    if ( fVerbose )
    {
        Vec_VecForEachLevel( vCands, vNodes, k )
        {
            printf( "Level %d. Constr =%d    ", k, Vec_PtrSize(vNodes) );
//            Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
//                printf( "%d:%s%d ", k, Aig_IsComplement(pObj)? "!":"", Aig_ObjId(Aig_Regular(pObj)) );
            printf( "\n" );
        }
    }

    return vCands;
}


/**Function*************************************************************

  Synopsis    [Detects constraints functionally.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Vec_t * Saig_ManDetectConstrFunc( Aig_Man_t * p, int nFrames, int nConfs, int nProps, int fVerbose )
{
    int iPat = 0, nWordsAlloc = 16;
    Bar_Progress_t * pProgress = NULL;
    Vec_Vec_t * vCands = NULL;
    Vec_Ptr_t * vInfo, * vNodes;
    Aig_Obj_t * pObj, * pRepr, * pObjNew;
    Aig_Man_t * pFrames;
    sat_solver * pSat;
    Cnf_Dat_t * pCnf;
    unsigned * pInfo;
    int i, j, k, Lit, status, nCands = 0;
    assert( Saig_ManPoNum(p) == 1 );
    if ( Saig_ManPoNum(p) != 1 )
    {
        printf( "The number of outputs is different from 1.\n" );
        return NULL;
    }
//printf( "Implications = %d.\n", Ssw_ManCountImplications(p, nFrames) );

    // perform unrolling
    pFrames = Saig_ManUnrollCOI( p, nFrames );
    assert( Aig_ManCoNum(pFrames) == 1 );
    if ( fVerbose )
    {
        printf( "Detecting constraints with %d frames, %d conflicts, and %d propagations.\n", nFrames, nConfs, nProps );
        printf( "Frames: " );
        Aig_ManPrintStats( pFrames );
    }
//    Aig_ManShow( pFrames, 0, NULL );

    // start the SAT solver
    pCnf = Cnf_DeriveSimple( pFrames, Aig_ManCoNum(pFrames) );
    pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
//printf( "Implications = %d.\n", pSat->qhead );

    // solve the original problem
    Lit = toLitCond( pCnf->pVarNums[Aig_ObjId(Aig_ManCo(pFrames,0))], 0 );
    status = sat_solver_solve( pSat, &Lit, &Lit + 1, (ABC_INT64_T)nConfs, 0, 0, 0 );
    if ( status == l_False )
    {
        printf( "The problem is trivially UNSAT (inductive with k=%d).\n", nFrames-1 );
        Cnf_DataFree( pCnf );
        sat_solver_delete( pSat );
        Aig_ManStop( pFrames );
        return NULL;
    }
    if ( status == l_Undef )
    {
        printf( "Solver could not solve the original problem.\n" );
        Cnf_DataFree( pCnf );
        sat_solver_delete( pSat );
        Aig_ManStop( pFrames );
        return NULL;
    }
    assert( status == l_True );

    // create simulation info
    vInfo = Vec_PtrAllocSimInfo( Aig_ManObjNumMax(pFrames), nWordsAlloc );
    Vec_PtrCleanSimInfo( vInfo, 0, nWordsAlloc );
    Saig_CollectSatValues( pSat, pCnf, vInfo, &iPat );
    Aig_ManForEachObj( pFrames, pObj, i )
    {
        pInfo = (unsigned *)Vec_PtrEntry( vInfo, i );
        if ( pInfo[0] & 1 )
            memset( (char*)pInfo, 0xff, 4*nWordsAlloc );
    }
//    Aig_ManShow( pFrames, 0, NULL );
//    Aig_ManShow( p, 0, NULL );

    // consider the nodes for ci=>!Out and label when it holds
    pProgress = Bar_ProgressStart( stdout, Aig_ManObjNumMax(pFrames) );
    Aig_ManCleanMarkAB( pFrames );
    Aig_ManForEachObj( pFrames, pObj, i )
    {
        if ( !Aig_ObjIsNode(pObj) && !Aig_ObjIsCi(pObj) )
            continue;
        Bar_ProgressUpdate( pProgress, i, NULL );
        // check if the node is available in both polarities
        pInfo = (unsigned *)Vec_PtrEntry( vInfo, i );
        for ( k = 0; k < nWordsAlloc; k++ )
            if ( pInfo[k] != ~0 )
                break;
        if ( k == nWordsAlloc )
        {
            if ( Saig_DetectTryPolarity(pSat, nConfs, nProps, pCnf, pObj, 0, vInfo, &iPat, fVerbose) ) // !pObj is a constr
            {
                pObj->fMarkA = 1, nCands++;
//                printf( "!%d  ", Aig_ObjId(pObj) );
            }
            continue;
        }
        for ( k = 0; k < nWordsAlloc; k++ )
            if ( pInfo[k] != 0 )
                break;
        if ( k == nWordsAlloc )
        {
            if ( Saig_DetectTryPolarity(pSat, nConfs, nProps, pCnf, pObj, 1, vInfo, &iPat, fVerbose) ) //  pObj is a constr
            {
                pObj->fMarkB = 1, nCands++;
//                printf( "%d  ", Aig_ObjId(pObj) );
            }
            continue;
        }
    }
    Bar_ProgressStop( pProgress );
    if ( nCands )
    {

//        printf( "\n" );
        if ( fVerbose )
            printf( "Found %3d classes of candidates.\n", nCands );
        vCands = Vec_VecAlloc( nFrames );
        for ( k = 0; k < nFrames; k++ )
        {
            Aig_ManForEachObj( p, pObj, i )
            {
                if ( !Aig_ObjIsNode(pObj) && !Aig_ObjIsCi(pObj) )
                    continue;
                pRepr = p->pObjCopies[nFrames*i + nFrames-1-k];
//                pRepr = p->pObjCopies[nFrames*i + k];
                if ( pRepr == NULL )
                    continue;
                if ( Aig_Regular(pRepr)->fMarkA ) // !pObj is a constr
                {
                    pObjNew = Aig_NotCond(pObj, !Aig_IsComplement(pRepr));

                    for ( j = 0; j < k; j++ )
                        if ( Vec_PtrFind( Vec_VecEntry(vCands, j), pObjNew ) >= 0 )
                            break;
                    if ( j == k )
                        Vec_VecPush( vCands, k, pObjNew );
//                    printf( "%d->!%d ", Aig_ObjId(Aig_Regular(pRepr)), Aig_ObjId(pObj) );
                }
                else if ( Aig_Regular(pRepr)->fMarkB ) //  pObj is a constr
                {
                    pObjNew = Aig_NotCond(pObj,  Aig_IsComplement(pRepr));

                    for ( j = 0; j < k; j++ )
                        if ( Vec_PtrFind( Vec_VecEntry(vCands, j), pObjNew ) >= 0  )
                            break;
                    if ( j == k )
                        Vec_VecPush( vCands, k, pObjNew );
//                    printf( "%d->%d ", Aig_ObjId(Aig_Regular(pRepr)), Aig_ObjId(pObj) );
                }
            }
        }

//        printf( "\n" );
        if ( fVerbose )
        {
            printf( "Found %3d candidates.\n", Vec_VecSizeSize(vCands) );
            Vec_VecForEachLevel( vCands, vNodes, k )
            {
                printf( "Level %d. Cands  =%d    ", k, Vec_PtrSize(vNodes) );
//                Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
//                    printf( "%d:%s%d ", k, Aig_IsComplement(pObj)? "!":"", Aig_ObjId(Aig_Regular(pObj)) );
                printf( "\n" );
            }
        }

        ABC_FREE( p->pObjCopies );
        Saig_ManFilterUsingInd( p, vCands, nConfs, nProps, fVerbose );
        if ( Vec_VecSizeSize(vCands) )
            printf( "Found %3d constraints after filtering.\n", Vec_VecSizeSize(vCands) );
        if ( fVerbose )
        {
            Vec_VecForEachLevel( vCands, vNodes, k )
            {
                printf( "Level %d. Constr =%d    ", k, Vec_PtrSize(vNodes) );
//                Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
//                    printf( "%d:%s%d ", k, Aig_IsComplement(pObj)? "!":"", Aig_ObjId(Aig_Regular(pObj)) );
                printf( "\n" );
            }
        }
    }
    Vec_PtrFree( vInfo );
    Cnf_DataFree( pCnf );
    sat_solver_delete( pSat );
    Aig_ManCleanMarkAB( pFrames );
    Aig_ManStop( pFrames );
    return vCands;
}

/**Function*************************************************************

  Synopsis    [Experimental procedure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManDetectConstrFuncTest( Aig_Man_t * p, int nFrames, int nConfs, int nProps, int fOldAlgo, int fVerbose )
{
    Vec_Vec_t * vCands;
    if ( fOldAlgo )
        vCands = Saig_ManDetectConstrFunc( p, nFrames, nConfs, nProps, fVerbose );
    else
        vCands = Ssw_ManFindDirectImplications( p, nFrames, nConfs, nProps, fVerbose );
    Vec_VecFreeP( &vCands );
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG while unfolding constraints.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManDupUnfoldConstrsFunc( Aig_Man_t * pAig, int nFrames, int nConfs, int nProps, int fOldAlgo, int fVerbose )
{
    Aig_Man_t * pNew;
    Vec_Vec_t * vCands;
    Vec_Ptr_t * vNodes, * vNewFlops;
    Aig_Obj_t * pObj;
    int i, j, k, nNewFlops;
    if ( fOldAlgo )
        vCands = Saig_ManDetectConstrFunc( pAig, nFrames, nConfs, nProps, fVerbose );
    else
        vCands = Ssw_ManFindDirectImplications( pAig, nFrames, nConfs, nProps, fVerbose );
    if ( vCands == NULL || Vec_VecSizeSize(vCands) == 0 )
    {
        Vec_VecFreeP( &vCands );
        return Aig_ManDupDfs( pAig );
    }
    // create new manager
    pNew = Aig_ManDupWithoutPos( pAig );
    pNew->nConstrs = pAig->nConstrs + Vec_VecSizeSize(vCands);
    // add normal POs
    Saig_ManForEachPo( pAig, pObj, i )
        Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
    // create constraint outputs
    vNewFlops = Vec_PtrAlloc( 100 );
    Vec_VecForEachLevel( vCands, vNodes, i )
    {
        Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, k )
        {
            Vec_PtrPush( vNewFlops, Aig_ObjRealCopy(pObj) );
            for ( j = 0; j < i; j++ )
                Vec_PtrPush( vNewFlops, Aig_ObjCreateCi(pNew) );
            Aig_ObjCreateCo( pNew, (Aig_Obj_t *)Vec_PtrPop(vNewFlops) );
        }
    }
    // add latch outputs
    Saig_ManForEachLi( pAig, pObj, i )
        Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
    // add new latch outputs
    nNewFlops = 0;
    Vec_VecForEachLevel( vCands, vNodes, i )
    {
        Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, k )
        {
            for ( j = 0; j < i; j++ )
                Aig_ObjCreateCo( pNew, (Aig_Obj_t *)Vec_PtrEntry(vNewFlops, nNewFlops++) );
        }
    }
    assert( nNewFlops == Vec_PtrSize(vNewFlops) );
    Aig_ManSetRegNum( pNew, Aig_ManRegNum(pAig) + nNewFlops );
    Vec_VecFreeP( &vCands );
    Vec_PtrFree( vNewFlops );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG while unfolding constraints.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManDupFoldConstrsFunc( Aig_Man_t * pAig, int fCompl, int fVerbose, int fSeqCleanup )
{
    Aig_Man_t * pAigNew;
    Aig_Obj_t * pMiter, * pFlopOut, * pFlopIn, * pObj;
    int i;
    if ( Aig_ManConstrNum(pAig) == 0 )
        return Aig_ManDupDfs( pAig );
    assert( Aig_ManConstrNum(pAig) < Saig_ManPoNum(pAig) );
    // start the new manager
    pAigNew = Aig_ManStart( Aig_ManNodeNum(pAig) );
    pAigNew->pName = Abc_UtilStrsav( pAig->pName );
    pAigNew->pSpec = Abc_UtilStrsav( pAig->pSpec );
    // map the constant node
    Aig_ManConst1(pAig)->pData = Aig_ManConst1( pAigNew );
    // create variables for PIs
    Aig_ManForEachCi( pAig, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pAigNew );
    // add internal nodes of this frame
    Aig_ManForEachNode( pAig, pObj, i )
        pObj->pData = Aig_And( pAigNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );

    // OR the constraint outputs
    pMiter = Aig_ManConst0( pAigNew );
    Saig_ManForEachPo( pAig, pObj, i )
    {
        if ( i < Saig_ManPoNum(pAig)-Aig_ManConstrNum(pAig) )
            continue;
        pMiter = Aig_Or( pAigNew, pMiter, Aig_NotCond( Aig_ObjChild0Copy(pObj), fCompl ) );
    }

    // create additional flop
    if ( Saig_ManRegNum(pAig) > 0 )
    {
        pFlopOut = Aig_ObjCreateCi( pAigNew );
        pFlopIn  = Aig_Or( pAigNew, pMiter, pFlopOut );
    }
    else 
        pFlopIn = pMiter;

    // create primary output
    Saig_ManForEachPo( pAig, pObj, i )
    {
        if ( i >= Saig_ManPoNum(pAig)-Aig_ManConstrNum(pAig) )
            continue;
        pMiter = Aig_And( pAigNew, Aig_ObjChild0Copy(pObj), Aig_Not(pFlopIn) );
        Aig_ObjCreateCo( pAigNew, pMiter );
    }

    // transfer to register outputs
    Saig_ManForEachLi( pAig, pObj, i )
        Aig_ObjCreateCo( pAigNew, Aig_ObjChild0Copy(pObj) );

    // create additional flop 
    if ( Saig_ManRegNum(pAig) > 0 )
    {
        Aig_ObjCreateCo( pAigNew, pFlopIn );
        Aig_ManSetRegNum( pAigNew, Aig_ManRegNum(pAig)+1 );
    }

    // perform cleanup
    Aig_ManCleanup( pAigNew );
    if ( fSeqCleanup )
        Aig_ManSeqCleanup( pAigNew );
    return pAigNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#include "saigUnfold2.c"
ABC_NAMESPACE_IMPL_END

