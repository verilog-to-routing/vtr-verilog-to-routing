/**CFile****************************************************************

  FileName    [wlcAbs.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Verilog parser.]

  Synopsis    [Abstraction for word-level networks.]

  Author      [Yen-Sheng Ho, Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 22, 2014.]

  Revision    [$Id: wlcAbs.c,v 1.00 2014/09/12 00:00:00 alanmi Exp $]

***********************************************************************/

#include "wlc.h"
#include "proof/pdr/pdr.h"
#include "proof/pdr/pdrInt.h"
#include "proof/ssw/ssw.h"
#include "aig/gia/giaAig.h"
#include "sat/bmc/bmc.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern Vec_Vec_t *   IPdr_ManSaveClauses( Pdr_Man_t * p, int fDropLast );
extern int           IPdr_ManRestoreClauses( Pdr_Man_t * p, Vec_Vec_t * vClauses, Vec_Int_t * vMap );
extern int           IPdr_ManRebuildClauses( Pdr_Man_t * p, Vec_Vec_t * vClauses );
extern int           IPdr_ManSolveInt( Pdr_Man_t * p, int fCheckClauses, int fPushClauses );
extern int           IPdr_ManCheckCombUnsat( Pdr_Man_t * p );
extern int           IPdr_ManReduceClauses( Pdr_Man_t * p, Vec_Vec_t * vClauses );
extern void          IPdr_ManPrintClauses( Vec_Vec_t * vClauses, int kStart, int nRegs );
extern void          Wla_ManJoinThread( Wla_Man_t * pWla, int RunId );
extern void          Wla_ManConcurrentBmc3( Wla_Man_t * pWla, Aig_Man_t * pAig, Abc_Cex_t ** ppCex );
extern int           Wla_CallBackToStop( int RunId );
extern int           Wla_GetGlobalRunId();

typedef struct Int_Pair_t_       Int_Pair_t;
struct Int_Pair_t_
{
    int first;
    int second;
};

int IntPairPtrCompare ( Int_Pair_t ** a, Int_Pair_t ** b )
{
    return (*a)->second < (*b)->second; 
}

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

void Wlc_NtkPrintNtk( Wlc_Ntk_t * p )
{
    int i;
    Wlc_Obj_t * pObj;

    Abc_Print( 1, "PIs:");
    Wlc_NtkForEachPi( p, pObj, i )
        Abc_Print( 1, " %s", Wlc_ObjName(p, Wlc_ObjId(p, pObj)) );
    Abc_Print( 1, "\n\n");

    Abc_Print( 1, "POs:");
    Wlc_NtkForEachPo( p, pObj, i )
        Abc_Print( 1, " %s", Wlc_ObjName(p, Wlc_ObjId(p, pObj)) );
    Abc_Print( 1, "\n\n");

    Abc_Print( 1, "FO(Fi)s:");
    Wlc_NtkForEachCi( p, pObj, i )
        if ( !Wlc_ObjIsPi( pObj ) )
            Abc_Print( 1, " %s(%s)", Wlc_ObjName(p, Wlc_ObjId(p, pObj)), Wlc_ObjName(p, Wlc_ObjId(p, Wlc_ObjFo2Fi(p, pObj))) );
    Abc_Print( 1, "\n\n");

    Abc_Print( 1, "Objs:\n");
    Wlc_NtkForEachObj( p, pObj, i )
    {
        if ( !Wlc_ObjIsCi(pObj) )
           Wlc_NtkPrintNode( p, pObj ) ;
    }
}

void Wlc_NtkAbsGetSupp_rec( Wlc_Ntk_t * p, Wlc_Obj_t * pObj, Vec_Bit_t * vCiMarks, Vec_Int_t * vSuppRefs, Vec_Int_t * vSuppList )
{
    int i, iFanin, iObj;
    if ( pObj->Mark ) // visited
        return;
    pObj->Mark = 1;
    iObj = Wlc_ObjId( p, pObj );
    if ( Vec_BitEntry( vCiMarks, iObj ) )
    {
        if ( vSuppRefs )
            Vec_IntAddToEntry( vSuppRefs, iObj, 1 );
        if ( vSuppList )
            Vec_IntPush( vSuppList, iObj );
        return;
    }
    Wlc_ObjForEachFanin( pObj, iFanin, i )
        Wlc_NtkAbsGetSupp_rec( p, Wlc_NtkObj(p, iFanin), vCiMarks, vSuppRefs, vSuppList );
}

void Wlc_NtkAbsGetSupp( Wlc_Ntk_t * p, Wlc_Obj_t * pObj, Vec_Bit_t * vCiMarks, Vec_Int_t * vSuppRefs, Vec_Int_t * vSuppList)
{
    assert( vSuppRefs || vSuppList );
    Wlc_NtkCleanMarks( p );

    Wlc_NtkAbsGetSupp_rec( p, pObj, vCiMarks, vSuppRefs, vSuppList );
}

int Wlc_NtkNumPiBits( Wlc_Ntk_t * pNtk ) 
{
    int num = 0;
    int i;
    Wlc_Obj_t * pObj;
    Wlc_NtkForEachPi(pNtk, pObj, i) {
        num += Wlc_ObjRange(pObj);
    }
    return num;
}

void Wlc_NtkAbsAnalyzeRefine( Wlc_Ntk_t * p, Vec_Int_t * vBlacks, Vec_Bit_t * vUnmark, int * nDisj, int * nNDisj )
{
    Vec_Bit_t * vCurCis, * vCandCis;
    Vec_Int_t * vSuppList;
    Vec_Int_t * vDeltaB;
    Vec_Int_t * vSuppRefs;
    int i, Entry;
    Wlc_Obj_t * pObj;
    
    vCurCis    = Vec_BitStart( Wlc_NtkObjNumMax( p ) );
    vCandCis   = Vec_BitStart( Wlc_NtkObjNumMax( p ) );
    vDeltaB    = Vec_IntAlloc( Vec_IntSize(vBlacks) );
    vSuppList  = Vec_IntAlloc( Wlc_NtkCiNum(p) + Vec_IntSize(vBlacks) );
    vSuppRefs  = Vec_IntAlloc( Wlc_NtkObjNumMax( p ) );


    Vec_IntFill( vSuppRefs, Wlc_NtkObjNumMax( p ), 0 );

    Wlc_NtkForEachCi( p, pObj, i )
    {
        Vec_BitWriteEntry( vCurCis, Wlc_ObjId(p, pObj), 1 ) ;
        Vec_BitWriteEntry( vCandCis, Wlc_ObjId(p, pObj), 1 ) ;
    }

    Vec_IntForEachEntry( vBlacks, Entry, i )
    {
        Vec_BitWriteEntry( vCurCis, Entry, 1 );
        if ( !Vec_BitEntry( vUnmark, Entry ) )
            Vec_BitWriteEntry( vCandCis, Entry, 1 );
        else
            Vec_IntPush( vDeltaB, Entry );
    }
    assert( Vec_IntSize( vDeltaB ) );
    
    Wlc_NtkForEachCo( p, pObj, i )
        Wlc_NtkAbsGetSupp( p, pObj, vCurCis, vSuppRefs, NULL );
    
    /*
    Abc_Print( 1, "SuppCurrentAbs =" );
    Wlc_NtkForEachObj( p, pObj, i )
        if ( Vec_IntEntry(vSuppRefs, i) > 0 )
            Abc_Print( 1, " %d", i );
    Abc_Print( 1, "\n" );
    */

    Vec_IntForEachEntry( vDeltaB, Entry, i )
        Wlc_NtkAbsGetSupp( p, Wlc_NtkObj(p, Entry), vCandCis, vSuppRefs, NULL );

    Vec_IntForEachEntry( vDeltaB, Entry, i )
    {
        int iSupp, ii;
        int fDisjoint = 1;

        Vec_IntClear( vSuppList );
        Wlc_NtkAbsGetSupp( p, Wlc_NtkObj(p, Entry), vCandCis, NULL, vSuppList );

        //Abc_Print( 1, "SuppCandAbs =" );
        Vec_IntForEachEntry( vSuppList, iSupp, ii )
        {
            //Abc_Print( 1, " %d(%d)", iSupp, Vec_IntEntry( vSuppRefs, iSupp ) );
            if ( Vec_IntEntry( vSuppRefs, iSupp ) >= 2 )
            {
                fDisjoint = 0;
                break;
            }
        }
        //Abc_Print( 1, "\n" );
        
        if ( fDisjoint )
        {
            //Abc_Print( 1, "PPI[%d] is disjoint.\n", Entry );
            ++(*nDisj);
        }
        else 
        {
            //Abc_Print( 1, "PPI[%d] is non-disjoint.\n", Entry );
            ++(*nNDisj);
        }
    }

    Vec_BitFree( vCurCis );
    Vec_BitFree( vCandCis );
    Vec_IntFree( vDeltaB );
    Vec_IntFree( vSuppList );
    Vec_IntFree( vSuppRefs );
}

static Vec_Int_t * Wlc_NtkGetCoreSels( Gia_Man_t * pFrames, int nFrames, int first_sel_pi, int num_sel_pis, Vec_Bit_t * vMark, int nConfLimit, Wlc_Par_t * pPars, int fSetPO, int RunId ) 
{
    Vec_Int_t * vCores = NULL;
    Aig_Man_t * pAigFrames = Gia_ManToAigSimple( pFrames );
    Cnf_Dat_t * pCnf = Cnf_Derive(pAigFrames, Aig_ManCoNum(pAigFrames));
    sat_solver * pSat = sat_solver_new();
    int i;

    sat_solver_setnvars(pSat, pCnf->nVars);
    if ( RunId >= 0 )
    {
        sat_solver_set_runid( pSat, RunId );
        sat_solver_set_stop_func( pSat, Wla_CallBackToStop );
    }

    for (i = 0; i < pCnf->nClauses; i++) 
    {
        if (!sat_solver_addclause(pSat, pCnf->pClauses[i], pCnf->pClauses[i + 1]))
            assert(false);
    }
    // add PO clauses
    {
        Vec_Int_t* vLits = Vec_IntAlloc(100);
        Aig_Obj_t* pObj;
        int i, ret;
        Aig_ManForEachCo( pAigFrames, pObj, i )
        {
            assert(pCnf->pVarNums[pObj->Id] >= 0);
            Vec_IntPush(vLits, toLitCond(pCnf->pVarNums[pObj->Id], 0));
        }
        if ( !fSetPO )
        {
            ret = sat_solver_addclause(pSat, Vec_IntArray(vLits), Vec_IntArray(vLits) + Vec_IntSize(vLits));
            if (!ret) 
                Abc_Print( 1, "UNSAT after adding PO clauses.\n" );
        }
        else
        {
            int Lit;
            for ( i = 0; i < Vec_IntSize(vLits); ++i )
            {
                if ( i == Vec_IntSize(vLits) - 1 )
                    Lit = Vec_IntEntry( vLits, i );
                else
                    Lit = lit_neg(Vec_IntEntry( vLits, i ));
                ret = sat_solver_addclause(pSat, &Lit, &Lit + 1);
                if (!ret) 
                    Abc_Print( 1, "UNSAT after adding PO clauses.\n" );
            }
        }

        Vec_IntFree(vLits);
    }
    
    // main procedure
    {
        int status;
        Vec_Int_t* vLits = Vec_IntAlloc(100);
        Vec_Int_t* vMapVar2Sel = Vec_IntStart( pCnf->nVars );
        for ( i = 0; i < num_sel_pis; ++i ) 
        {
            int cur_pi = first_sel_pi + i;
            int var = pCnf->pVarNums[Aig_ManCi(pAigFrames, cur_pi)->Id];
            int Lit;
            assert(var >= 0);
            Vec_IntWriteEntry( vMapVar2Sel, var, i );
            Lit = toLitCond( var, 0 );
            if ( Vec_BitEntry( vMark, i ) )
                Vec_IntPush(vLits, Lit);
            else
                sat_solver_addclause( pSat, &Lit, &Lit+1 );
        }
        /*
            int i, Entry;
            Abc_Print( 1, "#vLits = %d; vLits = ", Vec_IntSize(vLits) );
            Vec_IntForEachEntry(vLits, Entry, i)
                Abc_Print( 1, "%d ", Entry);
            Abc_Print( 1, "\n");
        */
        status = sat_solver_solve(pSat, Vec_IntArray(vLits), Vec_IntArray(vLits) + Vec_IntSize(vLits), (ABC_INT64_T)(nConfLimit), (ABC_INT64_T)(0), (ABC_INT64_T)(0), (ABC_INT64_T)(0));
        if (status == l_False) {
            int nCoreLits, *pCoreLits;
            Abc_Print( 1, "UNSAT.\n" );
            nCoreLits = sat_solver_final(pSat, &pCoreLits);
            vCores = Vec_IntAlloc( nCoreLits );
            for (i = 0; i < nCoreLits; i++) 
            {
                Vec_IntPush( vCores, Vec_IntEntry( vMapVar2Sel, lit_var( pCoreLits[i] ) ) );
            }
        } else if (status == l_True) {
            Abc_Print( 1, "SAT.\n" );
        } else {
            Abc_Print( 1, "UNKNOWN.\n" );
        }

        Vec_IntFree(vLits);
        Vec_IntFree(vMapVar2Sel);
    }
    Cnf_ManFree();
    sat_solver_delete(pSat);
    Aig_ManStop(pAigFrames);

    return vCores;
}

static Gia_Man_t * Wlc_NtkUnrollWoCex(Wlc_Ntk_t * pChoice, int nFrames, int first_sel_pi, int num_sel_pis)
{
    Gia_Man_t * pGiaChoice = Wlc_NtkBitBlast( pChoice, NULL );
    Gia_Man_t * pFrames = NULL, * pGia;
    Gia_Obj_t * pObj, * pObjRi;
    int f, i;

    pFrames = Gia_ManStart( 10000 );
    pFrames->pName = Abc_UtilStrsav( pGiaChoice->pName );
    Gia_ManHashAlloc( pFrames );
    Gia_ManConst0(pGiaChoice)->Value = 0;
    Gia_ManForEachRi( pGiaChoice, pObj, i )
        pObj->Value = 0;

    for ( f = 0; f < nFrames; f++ ) 
    {
        for( i = 0; i < Gia_ManPiNum(pGiaChoice); i++ ) 
        {
            if ( i >= first_sel_pi && i < first_sel_pi + num_sel_pis ) 
            {
                if( f == 0 )
                    Gia_ManPi(pGiaChoice, i)->Value = Gia_ManAppendCi(pFrames);
            } 
            else
            {
                Gia_ManPi(pGiaChoice, i)->Value = Gia_ManAppendCi(pFrames);
            } 
        }
        Gia_ManForEachRiRo( pGiaChoice, pObjRi, pObj, i )
            pObj->Value = pObjRi->Value;
        Gia_ManForEachAnd( pGiaChoice, pObj, i )
            pObj->Value = Gia_ManHashAnd(pFrames, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj));
        Gia_ManForEachCo( pGiaChoice, pObj, i )
            pObj->Value = Gia_ObjFanin0Copy(pObj);
        Gia_ManForEachPo( pGiaChoice, pObj, i )
            Gia_ManAppendCo(pFrames, pObj->Value);
    }
    Gia_ManHashStop (pFrames);
    Gia_ManSetRegNum(pFrames, 0);
    pFrames = Gia_ManCleanup(pGia = pFrames);
    Gia_ManStop(pGia);
    Gia_ManStop(pGiaChoice);

    return pFrames;
}

static Gia_Man_t * Wlc_NtkUnrollWithCex(Wlc_Ntk_t * pChoice, Abc_Cex_t * pCex, int nbits_old_pis, int num_sel_pis, int * p_num_ppis, int sel_pi_first, int fUsePPI) 
{
    Gia_Man_t * pGiaChoice = Wlc_NtkBitBlast( pChoice, NULL );
    int nbits_new_pis = Wlc_NtkNumPiBits( pChoice );
    int num_ppis = nbits_new_pis - nbits_old_pis - num_sel_pis;
    int num_undc_pis = Gia_ManPiNum(pGiaChoice) - nbits_new_pis;
    Gia_Man_t * pFrames = NULL;
    Gia_Obj_t * pObj, * pObjRi;
    int f, i;
    int is_sel_pi;
    Gia_Man_t * pGia;
    *p_num_ppis = num_ppis;

    Abc_Print( 1, "#orig_pis = %d, #ppis = %d, #sel_pis = %d, #undc_pis = %d\n", nbits_old_pis, num_ppis, num_sel_pis, num_undc_pis );
    assert(Gia_ManPiNum(pGiaChoice)==nbits_old_pis+num_ppis+num_sel_pis+num_undc_pis);
    assert(Gia_ManPiNum(pGiaChoice)==pCex->nPis+num_sel_pis);

    pFrames = Gia_ManStart( 10000 );
    pFrames->pName = Abc_UtilStrsav( pGiaChoice->pName );
    Gia_ManHashAlloc( pFrames );
    Gia_ManConst0(pGiaChoice)->Value = 0;
    Gia_ManForEachRi( pGiaChoice, pObj, i )
        pObj->Value = 0;

    for ( f = 0; f <= pCex->iFrame; f++ ) 
    {
        for( i = 0; i < Gia_ManPiNum(pGiaChoice); i++ ) 
        {
            if ( i >= nbits_old_pis && i < nbits_old_pis + num_ppis + num_sel_pis ) 
            {
                is_sel_pi = sel_pi_first ? (i < nbits_old_pis + num_sel_pis) : (i >= nbits_old_pis + num_ppis);
                if( f == 0 && is_sel_pi )
                    Gia_ManPi(pGiaChoice, i)->Value = Gia_ManAppendCi(pFrames);
                if( !is_sel_pi )
                {
                    if ( !fUsePPI )
                        Gia_ManPi(pGiaChoice, i)->Value = Gia_ManAppendCi(pFrames);
                    else
                        Gia_ManPi(pGiaChoice, i)->Value = Abc_InfoHasBit(pCex->pData, pCex->nRegs+pCex->nPis*f + i + num_undc_pis);
                }
            } 
            else if (i < nbits_old_pis) 
            {
                Gia_ManPi(pGiaChoice, i)->Value = Abc_InfoHasBit(pCex->pData, pCex->nRegs+pCex->nPis*f + i);
            } 
            else if (i >= nbits_old_pis + num_ppis + num_sel_pis) 
            {
                Gia_ManPi(pGiaChoice, i)->Value = Abc_InfoHasBit(pCex->pData, pCex->nRegs+pCex->nPis*f + i - num_sel_pis - num_ppis);
            }
        }
        Gia_ManForEachRiRo( pGiaChoice, pObjRi, pObj, i )
            pObj->Value = pObjRi->Value;
        Gia_ManForEachAnd( pGiaChoice, pObj, i )
            pObj->Value = Gia_ManHashAnd(pFrames, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj));
        Gia_ManForEachCo( pGiaChoice, pObj, i )
            pObj->Value = Gia_ObjFanin0Copy(pObj);
        Gia_ManForEachPo( pGiaChoice, pObj, i )
            Gia_ManAppendCo(pFrames, pObj->Value);
    }
    Gia_ManHashStop (pFrames);
    Gia_ManSetRegNum(pFrames, 0);
    pFrames = Gia_ManCleanup(pGia = pFrames);
    Gia_ManStop(pGia);
    Gia_ManStop(pGiaChoice);

    return pFrames;
}

Wlc_Ntk_t * Wlc_NtkIntroduceChoices( Wlc_Ntk_t * pNtk, Vec_Int_t * vBlacks, Vec_Int_t * vPPIs )
{
    //if ( vBlacks== NULL ) return NULL;
    Vec_Int_t * vNodes = Vec_IntDup( vBlacks );
    Wlc_Ntk_t * pNew;
    Wlc_Obj_t * pObj;
    int i, k, iObj, iFanin;
    Vec_Int_t * vFanins = Vec_IntAlloc( 3 );
    Vec_Int_t * vMapNode2Pi = Vec_IntStart( Wlc_NtkObjNumMax(pNtk) );
    Vec_Int_t * vMapNode2Sel = Vec_IntStart( Wlc_NtkObjNumMax(pNtk) );
    int nOrigObjNum = Wlc_NtkObjNumMax( pNtk );
    Wlc_Ntk_t * p = Wlc_NtkDupDfsSimple( pNtk );
    Vec_Bit_t * vPPIMark = NULL;

    Wlc_NtkForEachObjVec( vNodes, pNtk, pObj, i ) 
    {
        // TODO : fix FOs here
        Vec_IntWriteEntry(vNodes, i, Wlc_ObjCopy(pNtk, Wlc_ObjId(pNtk, pObj)));
    }

    if ( vPPIs )
    {
        vPPIMark = Vec_BitStart( Wlc_NtkObjNumMax(pNtk) );
        Wlc_NtkForEachObjVec( vPPIs, pNtk, pObj, i ) 
        {
            Vec_IntWriteEntry(vPPIs, i, Wlc_ObjCopy(pNtk, Wlc_ObjId(pNtk, pObj)));
            Vec_BitWriteEntry( vPPIMark, Vec_IntEntry(vPPIs, i), 1 );
        }
    }

    // Vec_IntPrint(vNodes);
    Wlc_NtkCleanCopy( p );

    // mark nodes
    Wlc_NtkForEachObjVec( vNodes, p, pObj, i ) 
    {
        iObj = Wlc_ObjId(p, pObj);
        pObj->Mark = 1;
        // add fresh PI with the same number of bits
        Vec_IntWriteEntry( vMapNode2Pi, iObj, Wlc_ObjAlloc( p, WLC_OBJ_PI, Wlc_ObjIsSigned(pObj), Wlc_ObjRange(pObj) - 1, 0 ) );
    }

    if ( vPPIs )
    {
        Wlc_NtkForEachObjVec( vPPIs, p, pObj, i ) 
        {
            iObj = Wlc_ObjId(p, pObj);
            pObj->Mark = 1;
            // add fresh PI with the same number of bits
            Vec_IntWriteEntry( vMapNode2Pi, iObj, Wlc_ObjAlloc( p, WLC_OBJ_PI, Wlc_ObjIsSigned(pObj), Wlc_ObjRange(pObj) - 1, 0 ) );
        }
    }

    // add sel PI
    Wlc_NtkForEachObjVec( vNodes, p, pObj, i ) 
    {
        iObj = Wlc_ObjId( p, pObj );
        Vec_IntWriteEntry( vMapNode2Sel, iObj, Wlc_ObjAlloc( p, WLC_OBJ_PI, 0, 0, 0 ) );
    }

    // iterate through the nodes in the DFS order
    Wlc_NtkForEachObj( p, pObj, i )
    {
        int isSigned, range;
        if ( i == nOrigObjNum ) 
        {
            // cout << "break at " << i << endl;
            break;
        }

        // update fanins
        Wlc_ObjForEachFanin( pObj, iFanin, k )
            Wlc_ObjFanins(pObj)[k] = Wlc_ObjCopy(p, iFanin);
        // node to remain
        iObj = i;

        if ( pObj->Mark ) 
        {
            // clean
            pObj->Mark = 0;

            if ( vPPIMark && Vec_BitEntry(vPPIMark, i) )
                iObj = Vec_IntEntry( vMapNode2Pi, i );
            else
            {
                isSigned = Wlc_ObjIsSigned(pObj);
                range = Wlc_ObjRange(pObj);
                Vec_IntClear(vFanins);
                Vec_IntPush(vFanins, Vec_IntEntry( vMapNode2Sel, i) );
                Vec_IntPush(vFanins, Vec_IntEntry( vMapNode2Pi, i ) );
                Vec_IntPush(vFanins, i);
                iObj = Wlc_ObjCreate(p, WLC_OBJ_MUX, isSigned, range - 1, 0, vFanins);
            }
        }
        
        Wlc_ObjSetCopy( p, i, iObj );
    }

    Wlc_NtkForEachCo( p, pObj, i )
    {
        iObj = Wlc_ObjId(p, pObj);
        if (iObj != Wlc_ObjCopy(p, iObj)) 
        {
            if (pObj->fIsFi)
                Wlc_NtkObj(p, Wlc_ObjCopy(p, iObj))->fIsFi = 1;
            else
                Wlc_NtkObj(p, Wlc_ObjCopy(p, iObj))->fIsPo = 1;

            Vec_IntWriteEntry(&p->vCos, i, Wlc_ObjCopy(p, iObj));
        }
    }

    // DumpWlcNtk(p);
    pNew = Wlc_NtkDupDfsSimple( p );

    if ( vPPIMark )
        Vec_BitFree( vPPIMark );
    Vec_IntFree( vFanins );
    Vec_IntFree( vMapNode2Pi );
    Vec_IntFree( vMapNode2Sel );
    Vec_IntFree( vNodes );
    Wlc_NtkFree( p );

    return pNew;
}

static Abc_Cex_t * Wlc_NtkCexIsReal( Wlc_Ntk_t * pOrig, Abc_Cex_t * pCex ) 
{
    Gia_Man_t * pGiaOrig = Wlc_NtkBitBlast( pOrig, NULL );
    int f, i;
    Gia_Obj_t * pObj, * pObjRi;
    Abc_Cex_t * pCexReal = Abc_CexAlloc( Gia_ManRegNum(pGiaOrig), Gia_ManPiNum(pGiaOrig), pCex->iFrame + 1 );

    Gia_ManConst0(pGiaOrig)->Value = 0;
    Gia_ManForEachRi( pGiaOrig, pObj, i )
        pObj->Value = 0;
    for ( f = 0; f <= pCex->iFrame; f++ ) 
    {
        for( i = 0; i < Gia_ManPiNum( pGiaOrig ); i++ )
        { 
            Gia_ManPi(pGiaOrig, i)->Value = Abc_InfoHasBit(pCex->pData, pCex->nRegs+pCex->nPis*f + i);
            if ( Gia_ManPi(pGiaOrig, i)->Value )
                Abc_InfoSetBit(pCexReal->pData, pCexReal->nRegs + pCexReal->nPis*f + i);
        }
        Gia_ManForEachRiRo( pGiaOrig, pObjRi, pObj, i )
            pObj->Value = pObjRi->Value;
        Gia_ManForEachAnd( pGiaOrig, pObj, i )
            pObj->Value = Gia_ObjFanin0Copy(pObj) & Gia_ObjFanin1Copy(pObj);
        Gia_ManForEachCo( pGiaOrig, pObj, i )
            pObj->Value = Gia_ObjFanin0Copy(pObj);
        Gia_ManForEachPo( pGiaOrig, pObj, i ) 
        {
            if (pObj->Value==1) {
                Abc_Print( 1, "CEX is real on the original model.\n" );
                Gia_ManStop(pGiaOrig);
                pCexReal->iFrame = f;
                pCexReal->iPo = i;
                return pCexReal;
            }
        }
    }

    // Abc_Print( 1, "CEX is spurious.\n" );
    Gia_ManStop(pGiaOrig);
    Abc_CexFree(pCexReal);
    return NULL;
}

static Wlc_Ntk_t * Wlc_NtkAbs2( Wlc_Ntk_t * pNtk, Vec_Int_t * vBlacks, Vec_Int_t ** pvFlops )
{
    Vec_Int_t * vFlops  = Vec_IntAlloc( 100 );
    Vec_Int_t * vNodes  = Vec_IntDup( vBlacks );
    Wlc_Ntk_t * pNew;
    Wlc_Obj_t * pObj;
    int i, k, iObj, iFanin;
    Vec_Int_t * vMapNode2Pi = Vec_IntStart( Wlc_NtkObjNumMax(pNtk) );
    int nOrigObjNum = Wlc_NtkObjNumMax( pNtk );
    Wlc_Ntk_t * p = Wlc_NtkDupDfsSimple( pNtk );

    Wlc_NtkForEachCi( pNtk, pObj, i )
    {
        if ( !Wlc_ObjIsPi( pObj ) )
            Vec_IntPush( vFlops, Wlc_ObjId( pNtk, pObj ) ); 
    }

    Wlc_NtkForEachObjVec( vNodes, pNtk, pObj, i ) 
        Vec_IntWriteEntry(vNodes, i, Wlc_ObjCopy(pNtk, Wlc_ObjId(pNtk, pObj)));

    // mark nodes
    Wlc_NtkForEachObjVec( vNodes, p, pObj, i ) 
    {
        iObj = Wlc_ObjId(p, pObj);
        pObj->Mark = 1;
        // add fresh PI with the same number of bits
        Vec_IntWriteEntry( vMapNode2Pi, iObj, Wlc_ObjAlloc( p, WLC_OBJ_PI, Wlc_ObjIsSigned(pObj), Wlc_ObjRange(pObj) - 1, 0 ) );
    }

    Wlc_NtkCleanCopy( p );

    Wlc_NtkForEachObj( p, pObj, i )
    {
        if ( i == nOrigObjNum ) 
            break;

        if ( pObj->Mark ) {
            // clean
            pObj->Mark = 0;
            iObj = Vec_IntEntry( vMapNode2Pi, i );
        }
        else {
            // update fanins
            Wlc_ObjForEachFanin( pObj, iFanin, k )
                Wlc_ObjFanins(pObj)[k] = Wlc_ObjCopy(p, iFanin);
            // node to remain
            iObj = i;
        }
        Wlc_ObjSetCopy( p, i, iObj );
    }

    Wlc_NtkForEachCo( p, pObj, i )
    {
        iObj = Wlc_ObjId(p, pObj);
        if (iObj != Wlc_ObjCopy(p, iObj)) 
        {
            if (pObj->fIsFi)
                Wlc_NtkObj(p, Wlc_ObjCopy(p, iObj))->fIsFi = 1;
            else
                Wlc_NtkObj(p, Wlc_ObjCopy(p, iObj))->fIsPo = 1;


            Vec_IntWriteEntry(&p->vCos, i, Wlc_ObjCopy(p, iObj));
        }
    }

    pNew = Wlc_NtkDupDfsSimple( p );
    Vec_IntFree( vMapNode2Pi );
    Vec_IntFree( vNodes );
    Wlc_NtkFree( p );

    if ( pvFlops )
        *pvFlops = vFlops;
    else
        Vec_IntFree( vFlops );

    return pNew;
}

static Vec_Bit_t * Wlc_NtkProofReduce( Wlc_Ntk_t * p, Wlc_Par_t * pPars, int nFrames, Vec_Int_t * vWhites, Vec_Int_t * vBlacks, int RunId )
{
    Gia_Man_t * pGiaFrames;
    Wlc_Ntk_t * pNtkWithChoices = NULL; 
    Vec_Int_t * vCoreSels;
    Vec_Bit_t * vChoiceMark; 
    int first_sel_pi;
    int i, Entry;
    abctime clk = Abc_Clock();

    assert( vWhites && Vec_IntSize(vWhites) );
    pNtkWithChoices = Wlc_NtkIntroduceChoices( p, vWhites, vBlacks );

    first_sel_pi = Wlc_NtkNumPiBits( pNtkWithChoices ) - Vec_IntSize( vWhites );
    pGiaFrames = Wlc_NtkUnrollWoCex( pNtkWithChoices, nFrames, first_sel_pi, Vec_IntSize( vWhites ) );

    vChoiceMark = Vec_BitStartFull( Vec_IntSize( vWhites ) );      
    vCoreSels = Wlc_NtkGetCoreSels( pGiaFrames, nFrames, first_sel_pi, Vec_IntSize( vWhites ), vChoiceMark, 0, pPars, 0, RunId );

    Wlc_NtkFree( pNtkWithChoices );
    Gia_ManStop( pGiaFrames );

    if ( vCoreSels == NULL )
        return NULL;
 
    Vec_BitReset( vChoiceMark );
    Vec_IntForEachEntry( vCoreSels, Entry, i )
        Vec_BitWriteEntry( vChoiceMark, Entry, 1 );

    Abc_Print( 1, "ProofReduce: remove %d out of %d white boxes.", Vec_IntSize(vWhites) - Vec_BitCount(vChoiceMark), Vec_IntSize(vWhites) );
    Abc_PrintTime( 1, " Time", Abc_Clock() - clk );

    Vec_IntFree( vCoreSels );
    return vChoiceMark;
}

static int Wlc_NtkProofRefine( Wlc_Ntk_t * p, Wlc_Par_t * pPars, Abc_Cex_t * pCex, Vec_Int_t * vBlacks, Vec_Int_t ** pvRefine )
{
    Gia_Man_t * pGiaFrames;
    Vec_Int_t * vRefine = NULL;
    Vec_Bit_t * vUnmark;
    Vec_Bit_t * vChoiceMark; 
    Wlc_Ntk_t * pNtkWithChoices = NULL; 
    Vec_Int_t * vCoreSels;
    int num_ppis = -1;
    int Entry, i;

    if ( *pvRefine == NULL )
        return 0;

    vUnmark = Vec_BitStart( Wlc_NtkObjNumMax( p ) );
    vChoiceMark = Vec_BitStart( Vec_IntSize( vBlacks ) );      

    Vec_IntForEachEntry( *pvRefine, Entry, i )
        Vec_BitWriteEntry( vUnmark, Entry, 1 );

    Vec_IntForEachEntry( vBlacks, Entry, i )
    {
        if ( Vec_BitEntry( vUnmark, Entry ) )
            Vec_BitWriteEntry( vChoiceMark, i, 1 );
    }

    pNtkWithChoices = vBlacks ? Wlc_NtkIntroduceChoices( p, vBlacks, NULL ) : NULL;
    pGiaFrames = Wlc_NtkUnrollWithCex( pNtkWithChoices, pCex, Wlc_NtkNumPiBits( p ), Vec_IntSize( vBlacks ), &num_ppis, 0, pPars->fProofUsePPI );

    if ( !pPars->fProofUsePPI )
        vCoreSels = Wlc_NtkGetCoreSels( pGiaFrames, pCex->iFrame+1, num_ppis, Vec_IntSize( vBlacks ), vChoiceMark, 0, pPars, 0, -1 );
    else
        vCoreSels = Wlc_NtkGetCoreSels( pGiaFrames, pCex->iFrame+1, 0, Vec_IntSize( vBlacks ), vChoiceMark, 0, pPars, 0, -1 );

    Wlc_NtkFree( pNtkWithChoices );
    Gia_ManStop( pGiaFrames );
    Vec_BitFree( vUnmark );
    Vec_BitFree( vChoiceMark );
 
    assert( Vec_IntSize( vCoreSels ) );

    vRefine = Vec_IntAlloc( 100 );
    Vec_IntForEachEntry( vCoreSels, Entry, i )
        Vec_IntPush( vRefine, Vec_IntEntry( vBlacks, Entry ) );

    Vec_IntFree( vCoreSels );

    if ( pPars->fVerbose )
        Abc_Print( 1, "Proof-based refinement reduces %d (out of %d) white boxes\n", Vec_IntSize( *pvRefine ) - Vec_IntSize( vRefine ), Vec_IntSize( *pvRefine ) );

    Vec_IntFree( *pvRefine );
    *pvRefine = vRefine;

    return 0;
}

static int Wlc_NtkUpdateBlacks( Wlc_Ntk_t * p, Wlc_Par_t * pPars, Vec_Int_t ** pvBlacks, Vec_Bit_t * vUnmark, Vec_Int_t * vSignals )
{
    int Entry, i;
    Wlc_Obj_t * pObj; int Count[4] = {0};
    Vec_Int_t * vBlacks = Vec_IntAlloc( 100 );
    Vec_Int_t * vVec;

    assert( *pvBlacks );

    vVec = vSignals ? vSignals : *pvBlacks;

    Vec_IntForEachEntry( vVec, Entry, i )
    {
        if ( Vec_BitEntry( vUnmark, Entry) )
            continue;
        Vec_IntPush( vBlacks, Entry );

        pObj = Wlc_NtkObj( p, Entry );
        if ( pObj->Type == WLC_OBJ_ARI_ADD || pObj->Type == WLC_OBJ_ARI_SUB || pObj->Type == WLC_OBJ_ARI_MINUS )
            Count[0]++;
        else if ( pObj->Type == WLC_OBJ_ARI_MULTI || pObj->Type == WLC_OBJ_ARI_DIVIDE || pObj->Type == WLC_OBJ_ARI_REM || pObj->Type == WLC_OBJ_ARI_MODULUS )
            Count[1]++;
        else if ( pObj->Type == WLC_OBJ_MUX )
            Count[2]++;
        else if ( Wlc_ObjIsCi(pObj) && !Wlc_ObjIsPi(pObj) )
            Count[3]++;
    }
    
    Vec_IntFree( *pvBlacks );
    *pvBlacks = vBlacks;

    if ( pPars->fVerbose )
        printf( "Abstraction engine marked %d adds/subs, %d muls/divs, %d muxes, and %d flops to be abstracted away.\n", Count[0], Count[1], Count[2], Vec_IntSize( vBlacks ) - Count[0] - Count[1] - Count[2] );

    return 0;
}

static Vec_Bit_t * Wlc_NtkMarkLimit( Wlc_Ntk_t * p, Wlc_Par_t * pPars )
{
    Vec_Bit_t * vMarks = NULL;
    Vec_Ptr_t * vAdds  = Vec_PtrAlloc( 1000 );
    Vec_Ptr_t * vMuxes = Vec_PtrAlloc( 1000 );
    Vec_Ptr_t * vMults = Vec_PtrAlloc( 1000 );
    Vec_Ptr_t * vFlops = Vec_PtrAlloc( 1000 );
    Wlc_Obj_t * pObj; int i;
    Int_Pair_t * pPair = NULL;

    if ( pPars->nLimit == ABC_INFINITY )
        return NULL;

    vMarks = Vec_BitStart( Wlc_NtkObjNumMax( p ) );

    Wlc_NtkForEachObj( p, pObj, i )
    {
        if ( pObj->Type == WLC_OBJ_ARI_ADD || pObj->Type == WLC_OBJ_ARI_SUB || pObj->Type == WLC_OBJ_ARI_MINUS )
        {
            if ( Wlc_ObjRange(pObj) >= pPars->nBitsAdd )
            {
                pPair = ABC_ALLOC( Int_Pair_t, 1 );
                pPair->first = i;
                pPair->second = Wlc_ObjRange( pObj );
                Vec_PtrPush( vAdds, pPair );
            }
        }
        else if ( pObj->Type == WLC_OBJ_ARI_MULTI || pObj->Type == WLC_OBJ_ARI_DIVIDE || pObj->Type == WLC_OBJ_ARI_REM || pObj->Type == WLC_OBJ_ARI_MODULUS )
        {
            if ( Wlc_ObjRange(pObj) >= pPars->nBitsMul )
            {
                pPair = ABC_ALLOC( Int_Pair_t, 1 );
                pPair->first = i;
                pPair->second = Wlc_ObjRange( pObj );
                Vec_PtrPush( vMults, pPair );
            }
        }
        else if ( pObj->Type == WLC_OBJ_MUX ) 
        {
            if ( Wlc_ObjRange(pObj) >= pPars->nBitsMux )
            {
                pPair = ABC_ALLOC( Int_Pair_t, 1 );
                pPair->first = i;
                pPair->second = Wlc_ObjRange( pObj );
                Vec_PtrPush( vMuxes, pPair );
            }
        }
        else if ( Wlc_ObjIsCi(pObj) && !Wlc_ObjIsPi(pObj) )
        {
            if ( Wlc_ObjRange(pObj) >= pPars->nBitsFlop )
            {
                pPair = ABC_ALLOC( Int_Pair_t, 1 );
                pPair->first = i;
                pPair->second = Wlc_ObjRange( pObj );
                Vec_PtrPush( vFlops, pPair );
            }
        }
    }

    Vec_PtrSort( vAdds,  (int (*)(const void *, const void *))IntPairPtrCompare ) ;
    Vec_PtrSort( vMults, (int (*)(const void *, const void *))IntPairPtrCompare ) ;
    Vec_PtrSort( vMuxes, (int (*)(const void *, const void *))IntPairPtrCompare ) ;
    Vec_PtrSort( vFlops, (int (*)(const void *, const void *))IntPairPtrCompare ) ;

    Vec_PtrForEachEntry( Int_Pair_t *, vAdds, pPair, i )
    {
        if ( i >= pPars->nLimit ) break;
        Vec_BitWriteEntry( vMarks, pPair->first, 1 ); 
    }
    if ( i && pPars->fVerbose ) Abc_Print( 1, "%%PDRA: %d-th ADD has width = %d\n", i, pPair->second );

    Vec_PtrForEachEntry( Int_Pair_t *, vMults, pPair, i )
    {
        if ( i >= pPars->nLimit ) break;
        Vec_BitWriteEntry( vMarks, pPair->first, 1 ); 
    }
    if ( i && pPars->fVerbose ) Abc_Print( 1, "%%PDRA: %d-th MUL has width = %d\n", i, pPair->second );

    Vec_PtrForEachEntry( Int_Pair_t *, vMuxes, pPair, i )
    {
        if ( i >= pPars->nLimit ) break; 
        Vec_BitWriteEntry( vMarks, pPair->first, 1 ); 
    }
    if ( i && pPars->fVerbose ) Abc_Print( 1, "%%PDRA: %d-th MUX has width = %d\n", i, pPair->second );

    Vec_PtrForEachEntry( Int_Pair_t *, vFlops, pPair, i )
    {
        if ( i >= pPars->nLimit ) break;
        Vec_BitWriteEntry( vMarks, pPair->first, 1 ); 
    }
    if ( i && pPars->fVerbose ) Abc_Print( 1, "%%PDRA: %d-th FF has width = %d\n", i, pPair->second );


    Vec_PtrForEachEntry( Int_Pair_t *, vAdds,  pPair, i ) ABC_FREE( pPair );
    Vec_PtrForEachEntry( Int_Pair_t *, vMults, pPair, i ) ABC_FREE( pPair );
    Vec_PtrForEachEntry( Int_Pair_t *, vMuxes, pPair, i ) ABC_FREE( pPair );
    Vec_PtrForEachEntry( Int_Pair_t *, vFlops, pPair, i ) ABC_FREE( pPair );
    Vec_PtrFree( vAdds );
    Vec_PtrFree( vMults );
    Vec_PtrFree( vMuxes );
    Vec_PtrFree( vFlops );

    return vMarks;
}

static void Wlc_NtkSetUnmark( Wlc_Ntk_t * p, Wlc_Par_t * pPars, Vec_Bit_t * vUnmark )
{
    int Entry, i;
    Vec_Bit_t * vMarks = Wlc_NtkMarkLimit( p, pPars );

    Vec_BitForEachEntry( vMarks, Entry, i )
        Vec_BitWriteEntry( vUnmark, i, Entry^1 );

    Vec_BitFree( vMarks );
}

static Vec_Int_t * Wlc_NtkGetBlacks( Wlc_Ntk_t * p, Wlc_Par_t * pPars )
{
    Vec_Int_t * vBlacks = Vec_IntAlloc( 100 ) ;
    Wlc_Obj_t * pObj; int i, Count[4] = {0};
    Vec_Bit_t * vMarks = NULL;
    int nTotal = 0;

    vMarks = Wlc_NtkMarkLimit( p, pPars ) ; 

    Wlc_NtkForEachObj( p, pObj, i )
    {
        if ( pObj->Type == WLC_OBJ_ARI_ADD || pObj->Type == WLC_OBJ_ARI_SUB || pObj->Type == WLC_OBJ_ARI_MINUS )
        {
            if ( Wlc_ObjRange(pObj) >= pPars->nBitsAdd )
            {
                ++nTotal;
                if ( vMarks == NULL )
                    Vec_IntPushUniqueOrder( vBlacks, Wlc_ObjId(p, pObj) ), Count[0]++;
                else if ( Vec_BitEntry( vMarks, i ) )
                    Vec_IntPushUniqueOrder( vBlacks, Wlc_ObjId(p, pObj) ), Count[0]++;
            }
            continue;
        }
        if ( pObj->Type == WLC_OBJ_ARI_MULTI || pObj->Type == WLC_OBJ_ARI_DIVIDE || pObj->Type == WLC_OBJ_ARI_REM || pObj->Type == WLC_OBJ_ARI_MODULUS )
        {
            if ( Wlc_ObjRange(pObj) >= pPars->nBitsMul )
            {
                ++nTotal;
                if ( vMarks == NULL )
                    Vec_IntPushUniqueOrder( vBlacks, Wlc_ObjId(p, pObj) ), Count[1]++;
                else if ( Vec_BitEntry( vMarks, i ) )
                    Vec_IntPushUniqueOrder( vBlacks, Wlc_ObjId(p, pObj) ), Count[1]++;
            }
            continue;
        }
        if ( pObj->Type == WLC_OBJ_MUX )
        {
            if ( Wlc_ObjRange(pObj) >= pPars->nBitsMux )
            {
                ++nTotal;
                if ( vMarks == NULL )
                    Vec_IntPushUniqueOrder( vBlacks, Wlc_ObjId(p, pObj) ), Count[2]++;
                else if ( Vec_BitEntry( vMarks, i ) )
                    Vec_IntPushUniqueOrder( vBlacks, Wlc_ObjId(p, pObj) ), Count[2]++;
            }
            continue;
        }
        if ( Wlc_ObjIsCi(pObj) && !Wlc_ObjIsPi(pObj) )
        {
            if ( Wlc_ObjRange(pObj) >= pPars->nBitsFlop )
            {
                ++nTotal;
                if ( vMarks == NULL )
                    Vec_IntPushUniqueOrder( vBlacks, Wlc_ObjId( p, Wlc_ObjFo2Fi( p, pObj ) ) ), Count[3]++;
                else if ( Vec_BitEntry( vMarks, i ) )
                    Vec_IntPushUniqueOrder( vBlacks, Wlc_ObjId( p, Wlc_ObjFo2Fi( p, pObj ) ) ), Count[3]++;
            }
            continue;
        }
    }
    if ( vMarks )
        Vec_BitFree( vMarks );
    if ( pPars->fVerbose )
        printf( "Abstraction engine marked %d adds/subs, %d muls/divs, %d muxes, and %d flops to be abstracted away (out of %d signals).\n", Count[0], Count[1], Count[2], Count[3], nTotal );
    return vBlacks;
}

/**Function*************************************************************

  Synopsis    [Mark operators that meet the abstraction criteria.]

  Description [This procedure returns the array of objects (vLeaves) that 
  should be abstracted because of their high bit-width. It uses input array (vUnmark)
  to not abstract those objects that have been refined in the previous rounds.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

static Vec_Bit_t * Wlc_NtkAbsMarkOpers( Wlc_Ntk_t * p, Wlc_Par_t * pPars, Vec_Bit_t * vUnmark, int fVerbose )
{
    Vec_Bit_t * vLeaves = Vec_BitStart( Wlc_NtkObjNumMax(p) );
    Wlc_Obj_t * pObj; int i, Count[4] = {0};
    Wlc_NtkForEachObj( p, pObj, i )
    {
        if ( vUnmark && Vec_BitEntry(vUnmark, i) ) // not allow this object to be abstracted away
            continue;
        if ( pObj->Type == WLC_OBJ_ARI_ADD || pObj->Type == WLC_OBJ_ARI_SUB || pObj->Type == WLC_OBJ_ARI_MINUS )
        {
            if ( Wlc_ObjRange(pObj) >= pPars->nBitsAdd )
                Vec_BitWriteEntry( vLeaves, Wlc_ObjId(p, pObj), 1 ), Count[0]++;
            continue;
        }
        if ( pObj->Type == WLC_OBJ_ARI_MULTI || pObj->Type == WLC_OBJ_ARI_DIVIDE || pObj->Type == WLC_OBJ_ARI_REM || pObj->Type == WLC_OBJ_ARI_MODULUS )
        {
            if ( Wlc_ObjRange(pObj) >= pPars->nBitsMul )
                Vec_BitWriteEntry( vLeaves, Wlc_ObjId(p, pObj), 1 ), Count[1]++;
            continue;
        }
        if ( pObj->Type == WLC_OBJ_MUX )
        {
            if ( Wlc_ObjRange(pObj) >= pPars->nBitsMux )
                Vec_BitWriteEntry( vLeaves, Wlc_ObjId(p, pObj), 1 ), Count[2]++;
            continue;
        }
        if ( Wlc_ObjIsCi(pObj) && !Wlc_ObjIsPi(pObj) )
        {
            if ( Wlc_ObjRange(pObj) >= pPars->nBitsFlop )
                Vec_BitWriteEntry( vLeaves, Wlc_ObjId(p, pObj), 1 ), Count[3]++;
            continue;
        }
    }
    if ( fVerbose )
        printf( "Abstraction engine marked %d adds/subs, %d muls/divs, %d muxes, and %d flops to be abstracted away.\n", Count[0], Count[1], Count[2], Count[3] );
    return vLeaves;
}

/**Function*************************************************************

  Synopsis    [Marks nodes to be included in the abstracted network.]

  Description [Marks all objects that will be included in the abstracted model.  
  Stops at the objects (vLeaves) that are abstracted away. Returns three arrays:
  a subset of original PIs (vPisOld), a subset of pseudo-PIs (vPisNew) and the
  set of flops present as flops in the abstracted network.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Wlc_NtkAbsMarkNodes_rec( Wlc_Ntk_t * p, Wlc_Obj_t * pObj, Vec_Bit_t * vLeaves, Vec_Int_t * vPisOld, Vec_Int_t * vPisNew, Vec_Int_t * vFlops )
{
    int i, iFanin;
    if ( pObj->Mark )
        return;
    pObj->Mark = 1;
    if ( Vec_BitEntry(vLeaves, Wlc_ObjId(p, pObj)) )
    {
        assert( !Wlc_ObjIsPi(pObj) );
        Vec_IntPush( vPisNew, Wlc_ObjId(p, pObj) );
        return;
    }
    if ( Wlc_ObjIsCi(pObj) )
    {
        if ( Wlc_ObjIsPi(pObj) )
            Vec_IntPush( vPisOld, Wlc_ObjId(p, pObj) );
        else
            Vec_IntPush( vFlops, Wlc_ObjId(p, pObj) );
        return;
    }
    Wlc_ObjForEachFanin( pObj, iFanin, i )
        Wlc_NtkAbsMarkNodes_rec( p, Wlc_NtkObj(p, iFanin), vLeaves, vPisOld, vPisNew, vFlops );
}
static void Wlc_NtkAbsMarkNodes( Wlc_Ntk_t * p, Vec_Bit_t * vLeaves, Vec_Int_t * vPisOld, Vec_Int_t * vPisNew, Vec_Int_t * vFlops )
{
    Wlc_Obj_t * pObj;
    int i, Count = 0;
    Wlc_NtkCleanMarks( p );
    Wlc_NtkForEachCo( p, pObj, i )
        Wlc_NtkAbsMarkNodes_rec( p, pObj, vLeaves, vPisOld, vPisNew, vFlops );
    
/*    
    Vec_IntClear(vFlops);
    Wlc_NtkForEachCi( p, pObj, i ) {
        if ( !Wlc_ObjIsPi(pObj) ) {
            Vec_IntPush( vFlops, Wlc_ObjId(p, pObj) );
            pObj->Mark = 1;
        }
    }
*/    

    Wlc_NtkForEachObjVec( vFlops, p, pObj, i )
        Wlc_NtkAbsMarkNodes_rec( p, Wlc_ObjFo2Fi(p, pObj), vLeaves, vPisOld, vPisNew, vFlops );
    Wlc_NtkForEachObj( p, pObj, i )
        Count += pObj->Mark;
//    printf( "Collected %d old PIs, %d new PIs, %d flops, and %d other objects.\n", 
//        Vec_IntSize(vPisOld), Vec_IntSize(vPisNew), Vec_IntSize(vFlops), 
//        Count - Vec_IntSize(vPisOld) - Vec_IntSize(vPisNew) - Vec_IntSize(vFlops) );
    Vec_IntSort( vPisOld, 0 );
    Vec_IntSort( vPisNew, 0 );
    Vec_IntSort( vFlops, 0 );
    Wlc_NtkCleanMarks( p );
}

/**Function*************************************************************

  Synopsis    [Derive word-level abstracted model based on the parameter values.]

  Description [Retuns the word-level abstracted network and the set of pseudo-PIs 
  (vPisNew), which were created during abstraction. If the abstraction is
  satisfiable, some of the pseudo-PIs will be un-abstracted. These pseudo-PIs
  and their MFFC cones will be listed in the array (vUnmark), which will
  force the abstraction to not stop at these pseudo-PIs in the future.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static Wlc_Ntk_t * Wlc_NtkAbs( Wlc_Ntk_t * p, Wlc_Par_t * pPars, Vec_Bit_t * vUnmark, Vec_Int_t ** pvPisNew, Vec_Int_t ** pvFlops, int fVerbose )
{
    Wlc_Ntk_t * pNtkNew = NULL;
    Vec_Int_t * vPisOld = Vec_IntAlloc( 100 );
    Vec_Int_t * vPisNew = Vec_IntAlloc( 100 );
    Vec_Int_t * vFlops  = Vec_IntAlloc( 100 );
    Vec_Bit_t * vLeaves = Wlc_NtkAbsMarkOpers( p, pPars, vUnmark, fVerbose );
    Wlc_NtkAbsMarkNodes( p, vLeaves, vPisOld, vPisNew, vFlops );
    Vec_BitFree( vLeaves );
    pNtkNew = Wlc_NtkDupDfsAbs( p, vPisOld, vPisNew, vFlops );
    Vec_IntFree( vPisOld );
    if ( pvFlops )
        *pvFlops = vFlops;
    else
        Vec_IntFree( vFlops );
    if ( pvPisNew )
        *pvPisNew = vPisNew;
    else
        Vec_IntFree( vPisNew );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Find what objects need to be un-abstracted.]

  Description [Returns a subset of pseudo-PIs (vPisNew), which will be 
  prevented from being abstracted in the future rounds of abstraction.
  The AIG manager (pGia) is a bit-level view of the abstracted model.
  The counter-example (pCex) is used to find waht PPIs to refine.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static Vec_Int_t * Wlc_NtkAbsRefinement( Wlc_Ntk_t * p, Gia_Man_t * pGia, Abc_Cex_t * pCex, Vec_Int_t * vPisNew )
{
    Vec_Int_t * vRefine = Vec_IntAlloc( 100 );
    Abc_Cex_t * pCexCare;
    Wlc_Obj_t * pObj; 
    // count the number of bit-level PPIs and map them into word-level objects they were derived from
    int f, i, b, nRealPis, nPpiBits = 0;
    Vec_Int_t * vMap = Vec_IntStartFull( pCex->nPis );
    Wlc_NtkForEachObjVec( vPisNew, p, pObj, i )
        for ( b = 0; b < Wlc_ObjRange(pObj); b++ )
            Vec_IntWriteEntry( vMap, nPpiBits++, Wlc_ObjId(p, pObj) );
    // since PPIs are ordered last, the previous bits are real PIs
    nRealPis = pCex->nPis - nPpiBits;
    // find the care-set
    pCexCare = Bmc_CexCareMinimizeAig( pGia, nRealPis, pCex, 1, 0, 0 );
    assert( pCexCare->nPis == pCex->nPis );
    // detect care PPIs
    for ( f = 0; f <= pCexCare->iFrame; f++ )
        for ( i = nRealPis; i < pCexCare->nPis; i++ )
            if ( Abc_InfoHasBit(pCexCare->pData, pCexCare->nRegs + pCexCare->nPis * f + i) )
                Vec_IntPushUniqueOrder( vRefine, Vec_IntEntry(vMap, i-nRealPis) );
    Abc_CexFree( pCexCare );
    Vec_IntFree( vMap );
    if ( Vec_IntSize(vRefine) == 0 )// real CEX
        Vec_IntFreeP( &vRefine );
    return vRefine;
}

/**Function*************************************************************

  Synopsis    [Mark MFFC cones of the un-abstracted objects.]

  Description [The MFFC cones of the objects in vRefine are traversed
  and all their nodes are marked in vUnmark.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Wlc_NtkNodeDeref_rec( Wlc_Ntk_t * p, Wlc_Obj_t * pNode, Vec_Bit_t * vUnmark )
{
    int i, Fanin, Counter = 1;
    if ( Wlc_ObjIsCi(pNode) )
        return 0;
    Vec_BitWriteEntry( vUnmark, Wlc_ObjId(p, pNode), 1 );
    Wlc_ObjForEachFanin( pNode, Fanin, i )
    {
        Vec_IntAddToEntry( &p->vRefs, Fanin, -1 );
        if ( Vec_IntEntry(&p->vRefs, Fanin) == 0 )
            Counter += Wlc_NtkNodeDeref_rec( p, Wlc_NtkObj(p, Fanin), vUnmark );
    }
    return Counter;
}
static int Wlc_NtkNodeRef_rec( Wlc_Ntk_t * p, Wlc_Obj_t * pNode )
{
    int i, Fanin, Counter = 1;
    if ( Wlc_ObjIsCi(pNode) )
        return 0;
    Wlc_ObjForEachFanin( pNode, Fanin, i )
    {
        if ( Vec_IntEntry(&p->vRefs, Fanin) == 0 )
            Counter += Wlc_NtkNodeRef_rec( p, Wlc_NtkObj(p, Fanin) );
        Vec_IntAddToEntry( &p->vRefs, Fanin, 1 );
    }
    return Counter;
}
static int Wlc_NtkMarkMffc( Wlc_Ntk_t * p, Wlc_Obj_t * pNode, Vec_Bit_t * vUnmark )
{
    int Count1, Count2;
    // if this is a flop output, compute MFFC of the corresponding flop input
    while ( Wlc_ObjIsCi(pNode) )
    {
        Vec_BitWriteEntry( vUnmark, Wlc_ObjId(p, pNode), 1 );
        pNode = Wlc_ObjFo2Fi(p, pNode);
    }
    assert( !Wlc_ObjIsCi(pNode) );
    // dereference the node (and set the bits in vUnmark)
    Count1 = Wlc_NtkNodeDeref_rec( p, pNode, vUnmark );
    // reference it back
    Count2 = Wlc_NtkNodeRef_rec( p, pNode );
    assert( Count1 == Count2 );
    return Count1;
}
static int Wlc_NtkRemoveFromAbstraction( Wlc_Ntk_t * p, Vec_Int_t * vRefine, Vec_Bit_t * vUnmark )
{
    Wlc_Obj_t * pObj; int i, nNodes = 0;
    if ( Vec_IntSize(&p->vRefs) == 0 )
        Wlc_NtkSetRefs( p );
    Wlc_NtkForEachObjVec( vRefine, p, pObj, i )
        nNodes += Wlc_NtkMarkMffc( p, pObj, vUnmark );
    return nNodes;
}

static int Wlc_NtkUnmarkRefinement( Wlc_Ntk_t * p, Vec_Int_t * vRefine, Vec_Bit_t * vUnmark )
{
    Wlc_Obj_t * pObj; int i, nNodes = 0;
    Wlc_NtkForEachObjVec( vRefine, p, pObj, i )
    {
        Vec_BitWriteEntry( vUnmark, Wlc_ObjId(p, pObj), 1 );
        ++nNodes;
    }
    return nNodes;
}

/**Function*************************************************************

  Synopsis    [Computes the map for remapping flop IDs used in the clauses.]

  Description [Takes the original network (Wlc_Ntk_t) and the array of word-level 
  flops used in the old abstraction (vFfOld) and those used in the new abstraction
  (vFfNew). Returns the integer map, which remaps every binary flop found
  in the old abstraction into a binary flop found in the new abstraction.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Wlc_NtkFlopsRemap( Wlc_Ntk_t * p, Vec_Int_t * vFfOld, Vec_Int_t * vFfNew )
{
    Vec_Int_t * vMap = Vec_IntAlloc( 1000 );             // the resulting map
    Vec_Int_t * vMapFfNew2Bit1 = Vec_IntAlloc( 1000 );   // first binary bit of each new word-level flop
    int i, b, iFfOld, iFfNew, iBit1New, nBits = 0;
    // map object IDs of old flops into their flop indexes
    Vec_Int_t * vMapFfObj2FfId = Vec_IntStartFull( Wlc_NtkObjNumMax(p) );
    Vec_IntForEachEntry( vFfNew, iFfNew, i )
        Vec_IntWriteEntry( vMapFfObj2FfId, iFfNew, i );
    // map each new flop index into its first bit
    Vec_IntForEachEntry( vFfNew, iFfNew, i )
    {
        Wlc_Obj_t * pObj = Wlc_NtkObj( p, iFfNew );
        int nRange = Wlc_ObjRange( pObj );
        Vec_IntPush( vMapFfNew2Bit1, nBits );
        nBits += nRange;
    }
    assert( Vec_IntSize(vMapFfNew2Bit1) == Vec_IntSize(vFfNew) );
    // remap old binary flops into new binary flops
    Vec_IntForEachEntry( vFfOld, iFfOld, i )
    {
        Wlc_Obj_t * pObj = Wlc_NtkObj( p, iFfOld );
        int nRange = Wlc_ObjRange( pObj );
        iFfNew = Vec_IntEntry( vMapFfObj2FfId, iFfOld );
        assert( iFfNew >= 0 ); // every old flop should be present in the new abstraction
        // find the first bit of this new flop
        iBit1New = Vec_IntEntry( vMapFfNew2Bit1, iFfNew );
        for ( b = 0; b < nRange; b++ )
            Vec_IntPush( vMap, iBit1New + b );
    }
    Vec_IntFree( vMapFfNew2Bit1 );
    Vec_IntFree( vMapFfObj2FfId );
    return vMap;
}

/**Function*************************************************************

  Synopsis    [Performs PDR with word-level abstraction.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Wla_ManCollectNodes( Wla_Man_t * pWla, int fBlack )
{
    Vec_Int_t * vNodes = NULL;
    int i, Entry;

    assert( pWla->vSignals );

    vNodes = Vec_IntAlloc( 100 );
    Vec_IntForEachEntry( pWla->vSignals, Entry, i )
    {
        if ( !fBlack && Vec_BitEntry(pWla->vUnmark, Entry) )
            Vec_IntPush( vNodes, Entry );
        
        if ( fBlack && !Vec_BitEntry(pWla->vUnmark, Entry) )
            Vec_IntPush( vNodes, Entry );
    }

    return vNodes;
}

int Wla_ManShrinkAbs( Wla_Man_t * pWla, int nFrames, int RunId )
{
    int i, Entry;
    int RetValue = 0;

    Vec_Int_t * vWhites = Wla_ManCollectNodes( pWla, 0 );
    Vec_Int_t * vBlacks = Wla_ManCollectNodes( pWla, 1 );
    Vec_Bit_t * vCoreMarks = Wlc_NtkProofReduce( pWla->p, pWla->pPars, nFrames, vWhites, vBlacks, RunId );

    if ( vCoreMarks == NULL )
    {
        Vec_IntFree( vWhites );
        Vec_IntFree( vBlacks );
        return -1;
    }

    RetValue = Vec_IntSize( vWhites ) != Vec_BitCount( vCoreMarks );

    Vec_IntForEachEntry( vWhites, Entry, i )
        if ( !Vec_BitEntry( vCoreMarks, i ) )
            Vec_BitWriteEntry( pWla->vUnmark, Entry, 0 );

    Vec_IntFree( vWhites );
    Vec_IntFree( vBlacks );
    Vec_BitFree( vCoreMarks );

    return RetValue;
}

Wlc_Ntk_t * Wla_ManCreateAbs( Wla_Man_t * pWla )
{
    Wlc_Ntk_t * pAbs;

    // get abstracted GIA and the set of pseudo-PIs (vBlacks)
    if ( pWla->vBlacks == NULL )
    {
        pWla->vBlacks = Wlc_NtkGetBlacks( pWla->p, pWla->pPars );
        pWla->vSignals = Vec_IntDup( pWla->vBlacks );
    }
    else
    {
        Wlc_NtkUpdateBlacks( pWla->p, pWla->pPars, &pWla->vBlacks, pWla->vUnmark, pWla->vSignals );
    }
    pAbs = Wlc_NtkAbs2( pWla->p, pWla->vBlacks, NULL );

    return pAbs;
}

Aig_Man_t * Wla_ManBitBlast( Wla_Man_t * pWla, Wlc_Ntk_t * pAbs )
{
    int nDcFlops;
    Gia_Man_t * pTemp;
    Aig_Man_t * pAig;

    pWla->pGia = Wlc_NtkBitBlast( pAbs, NULL );

    // if the abstraction has flops with DC-init state,
    // new PIs were introduced by bit-blasting at the end of the PI list
    // here we move these variables to be *before* PPIs, because
    // PPIs are supposed to be at the end of the PI list for refinement
    nDcFlops = Wlc_NtkDcFlopNum(pAbs);
    if ( nDcFlops > 0 ) // DC-init flops are present
    {
        pWla->pGia = Gia_ManPermuteInputs( pTemp = pWla->pGia, Wlc_NtkCountObjBits(pWla->p, pWla->vBlacks), nDcFlops );
        Gia_ManStop( pTemp );
    }
    // if the word-level outputs have to be XORs, this is a place to do it
    if ( pWla->pPars->fXorOutput )
    {
        pWla->pGia = Gia_ManTransformMiter2( pTemp = pWla->pGia );
        Gia_ManStop( pTemp );
    }
    if ( pWla->pPars->fVerbose )
    {
        printf( "Derived abstraction with %d objects and %d PPIs. Bit-blasted AIG stats are:\n", Wlc_NtkObjNum(pAbs), Vec_IntSize(pWla->vBlacks) ); 
        Gia_ManPrintStats( pWla->pGia, NULL );
    }

    // try to prove abstracted GIA by converting it to AIG and calling PDR
    pAig = Gia_ManToAigSimple( pWla->pGia );

    return pAig;
}

int Wla_ManCheckCombUnsat( Wla_Man_t * pWla, Aig_Man_t * pAig )
{
    Pdr_Man_t * pPdr;
    Pdr_Par_t * pPdrPars = (Pdr_Par_t *)pWla->pPdrPars;
    abctime clk;
    int RetValue = -1;

    if ( Aig_ManAndNum( pAig ) <= 20000 )
    { 
        Aig_Man_t * pAigScorr;
        Ssw_Pars_t ScorrPars, * pScorrPars = &ScorrPars;
        int nAnds;

        clk = Abc_Clock();

        Ssw_ManSetDefaultParams( pScorrPars );
        pScorrPars->fStopWhenGone = 1;
        pScorrPars->nFramesK = 1;
        pAigScorr = Ssw_SignalCorrespondence( pAig, pScorrPars );
        assert ( pAigScorr );
        nAnds = Aig_ManAndNum( pAigScorr); 
        Aig_ManStop( pAigScorr );

        if ( nAnds == 0 )
        {
            if ( pWla->pPars->fVerbose )
                Abc_PrintTime( 1, "SCORR proved UNSAT. Time", Abc_Clock() - clk );
            return 1;
        }
        else if ( pWla->pPars->fVerbose )
        {
            Abc_Print( 1, "SCORR failed with %d ANDs. ", nAnds);
            Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
        }
    }

    clk = Abc_Clock();

    pPdrPars->fVerbose = 0;
    pPdr = Pdr_ManStart( pAig, pPdrPars, NULL );
    RetValue = IPdr_ManCheckCombUnsat( pPdr );
    Pdr_ManStop( pPdr );
    pPdrPars->fVerbose = pWla->pPars->fPdrVerbose;

    pWla->tPdr += Abc_Clock() - clk;

    return RetValue;
}

int Wla_ManSolveInt( Wla_Man_t * pWla, Aig_Man_t * pAig )
{
    abctime clk;
    Pdr_Man_t * pPdr;
    Pdr_Par_t * pPdrPars = (Pdr_Par_t *)pWla->pPdrPars;
    Abc_Cex_t * pBmcCex = NULL;
    Abc_Cex_t * pCexReal = NULL;
    int RetValue = -1;
    int RunId = Wla_GetGlobalRunId();

    if ( pWla->vClauses && pWla->pPars->fCheckCombUnsat )
    {
        clk = Abc_Clock();

        RetValue = Wla_ManCheckCombUnsat( pWla, pAig );
        if ( RetValue == 1 )
        {
            if ( pWla->pPars->fVerbose )
                Abc_PrintTime( 1,  "ABS becomes combinationally UNSAT. Time", Abc_Clock() - clk );
            return 1;
        }

        if ( pWla->pPars->fVerbose )
            Abc_PrintTime( 1, "Check comb. unsat failed. Time", Abc_Clock() - clk );
    }

    if ( pWla->pPars->fUseBmc3 )
    {
        pPdrPars->RunId = RunId;
        pPdrPars->pFuncStop = Wla_CallBackToStop;

        Wla_ManConcurrentBmc3( pWla, Aig_ManDupSimple(pAig), &pBmcCex );
    }

    clk = Abc_Clock();
    pPdr = Pdr_ManStart( pAig, pPdrPars, NULL );
    if ( pWla->vClauses ) {
        assert( Vec_VecSize( pWla->vClauses) >= 2 ); 
        
        if ( pWla->fNewAbs )
            IPdr_ManRebuildClauses( pPdr, pWla->pPars->fShrinkScratch ? NULL : pWla->vClauses );
        else
            IPdr_ManRestoreClauses( pPdr, pWla->vClauses, NULL );

        pWla->fNewAbs = 0;
    }

    RetValue = IPdr_ManSolveInt( pPdr, pWla->pPars->fCheckClauses, pWla->pPars->fPushClauses );
    pPdr->tTotal += Abc_Clock() - clk;
    pWla->tPdr += pPdr->tTotal;
    if ( pWla->pPars->fLoadTrace)
        pWla->vClauses = IPdr_ManSaveClauses( pPdr, 0 );
    Pdr_ManStop( pPdr );


    if ( pWla->pPars->fUseBmc3 )
        Wla_ManJoinThread( pWla, RunId );

    if ( pBmcCex )
    {
        pWla->pCex = pBmcCex ;
    }
    else
    {
        pWla->pCex = pAig->pSeqModel;
        pAig->pSeqModel = NULL;
    }

    // consider outcomes
    if ( pWla->pCex == NULL ) 
    {
        assert( RetValue ); // proved or undecided
        return RetValue;
    }

    // verify CEX
    pCexReal = Wlc_NtkCexIsReal( pWla->p, pWla->pCex );
    if ( pCexReal )
    {
        Abc_CexFree( pWla->pCex );
        pWla->pCex = pCexReal; 
        return 0;
    }

    return -1;
}

void Wla_ManRefine( Wla_Man_t * pWla )
{
    abctime clk;
    int nNodes;
    Vec_Int_t * vRefine = NULL;

    if ( pWla->fNewAbs )
    {
        if ( pWla->pCex )
            Abc_CexFree( pWla->pCex );
        pWla->pCex = NULL;
        Gia_ManStop( pWla->pGia ); pWla->pGia = NULL;
        return;
    }

    // perform refinement
    if ( pWla->pPars->fHybrid || !pWla->pPars->fProofRefine )
    {
        clk = Abc_Clock();
        vRefine = Wlc_NtkAbsRefinement( pWla->p, pWla->pGia, pWla->pCex, pWla->vBlacks );
        pWla->tCbr += Abc_Clock() - clk;
    }
    else // proof-based only
    {
        vRefine = Vec_IntDup( pWla->vBlacks );
    }
    if ( pWla->pPars->fProofRefine ) 
    {
        clk = Abc_Clock();
        Wlc_NtkProofRefine( pWla->p, pWla->pPars, pWla->pCex, pWla->vBlacks, &vRefine );
        pWla->tPbr += Abc_Clock() - clk;
    }

    if ( pWla->vClauses && pWla->pPars->fVerbose )
    {
        int i;
        Vec_Ptr_t * vVec; 
        Vec_VecForEachLevel( pWla->vClauses, vVec, i )
            pWla->nTotalCla += Vec_PtrSize( vVec ); 
    }

    // update the set of objects to be un-abstracted
    clk = Abc_Clock();
    if ( pWla->pPars->fMFFC )
    {
        nNodes = Wlc_NtkRemoveFromAbstraction( pWla->p, vRefine, pWla->vUnmark );
        if ( pWla->pPars->fVerbose )
            printf( "Refinement of CEX in frame %d came up with %d un-abstacted PPIs, whose MFFCs include %d objects.\n", pWla->pCex->iFrame, Vec_IntSize(vRefine), nNodes );
    }
    else
    {
        nNodes = Wlc_NtkUnmarkRefinement( pWla->p, vRefine, pWla->vUnmark );
        if ( pWla->pPars->fVerbose )
            printf( "Refinement of CEX in frame %d came up with %d un-abstacted PPIs.\n", pWla->pCex->iFrame, Vec_IntSize(vRefine) );

    }
    /*
    if ( pWla->pPars->fVerbose ) 
    {
        Wlc_NtkAbsAnalyzeRefine( pWla->p, pWla->vBlacks, pWla->vUnmark, &pWla->nDisj, &pWla->nNDisj );
        Abc_Print( 1, "Refine analysis (total): %d disjoint PPIs and %d non-disjoint PPIs\n", pWla->nDisj, pWla->nNDisj );
    }
    */
    pWla->tCbr += Abc_Clock() - clk;
    
    // Experimental
    /*
    if ( pWla->pCex->iFrame > 0 )
    {
        Vec_Int_t * vWhites = Wla_ManCollectWhites( pWla );
        Vec_Bit_t * vCore = Wlc_NtkProofReduce( pWla->p, pWla->pPars, pWla->pCex->iFrame, vWhites );
        Vec_IntFree( vWhites );
        Vec_BitFree( vCore );
    }
    */

    pWla->iCexFrame = pWla->pCex->iFrame;

    Vec_IntFree( vRefine );
    Gia_ManStop( pWla->pGia ); pWla->pGia = NULL;
    Abc_CexFree( pWla->pCex ); pWla->pCex = NULL;
}

Wla_Man_t * Wla_ManStart( Wlc_Ntk_t * pNtk, Wlc_Par_t * pPars )
{
    Wla_Man_t * p = ABC_CALLOC( Wla_Man_t, 1 );
    Pdr_Par_t * pPdrPars;
    p->p = pNtk;
    p->pPars = pPars;
    p->vUnmark = Vec_BitStart( Wlc_NtkObjNumMax(pNtk) );

    pPdrPars = ABC_CALLOC( Pdr_Par_t, 1 );
    Pdr_ManSetDefaultParams( pPdrPars );
    pPdrPars->fVerbose   = pPars->fPdrVerbose;
    pPdrPars->fVeryVerbose = 0;
    pPdrPars->pFuncStop  = pPars->pFuncStop;
    pPdrPars->RunId      = pPars->RunId;
    if ( pPars->fPdra )
    {
        pPdrPars->fUseAbs    = 1;   // use 'pdr -t'  (on-the-fly abstraction)
        pPdrPars->fCtgs      = 1;   // use 'pdr -nc' (improved generalization)
        pPdrPars->fSkipDown  = 0;   // use 'pdr -nc' (improved generalization)
        pPdrPars->nRestLimit = 500; // reset queue or proof-obligations when it gets larger than this
    } 
    p->pPdrPars = (void *)pPdrPars;

    p->iCexFrame = 0;
    p->fNewAbs   = 0;

    p->nIters    = 1;
    p->nTotalCla = 0;
    p->nDisj     = 0;
    p->nNDisj    = 0;

    return p;
}

void Wla_ManStop( Wla_Man_t * p )
{
    if ( p->vBlacks )   Vec_IntFree( p->vBlacks );
    if ( p->vSignals )  Vec_IntFree( p->vSignals );
    if ( p->pGia )      Gia_ManStop( p->pGia );
    if ( p->pCex )      Abc_CexFree( p->pCex );
    Vec_BitFree( p->vUnmark );
    ABC_FREE( p->pPdrPars );
    ABC_FREE( p );
}

int Wla_ManSolve( Wla_Man_t * pWla, Wlc_Par_t * pPars )
{
    abctime clk = Abc_Clock();
    abctime tTotal;
    Wlc_Ntk_t * pAbs = NULL;
    Aig_Man_t * pAig = NULL;

    int RetValue = -1;

    // perform refinement iterations
    for ( pWla->nIters = 1; pWla->nIters < pPars->nIterMax; ++pWla->nIters )
    {
        if ( pPars->fVerbose )
            printf( "\nIteration %d:\n", pWla->nIters );

        pAbs = Wla_ManCreateAbs( pWla );

        pAig = Wla_ManBitBlast( pWla, pAbs );
        Wlc_NtkFree( pAbs );

        RetValue = Wla_ManSolveInt( pWla, pAig );
        Aig_ManStop( pAig );

        if ( RetValue != -1 || (pPars->pFuncStop && pPars->pFuncStop( pPars->RunId)) )
            break;

        Wla_ManRefine( pWla );
    }

    // report the result
    if ( pPars->fVerbose )
        printf( "\n" );
    printf( "Abstraction " );
    if ( RetValue == 0 )
        printf( "resulted in a real CEX" );
    else if ( RetValue == 1 )
        printf( "is successfully proved" );
    else 
        printf( "timed out" );
    printf( " after %d iterations. ", pWla->nIters );
    tTotal = Abc_Clock() - clk;
    Abc_PrintTime( 1, "Time", tTotal );

    if ( pPars->fVerbose )
        Abc_Print( 1, "PDRA reused %d clauses.\n", pWla->nTotalCla );
    if ( pPars->fVerbose )
    {
        ABC_PRTP( "PDR          ", pWla->tPdr,                          tTotal );
        ABC_PRTP( "CEX Refine   ", pWla->tCbr,                          tTotal );
        ABC_PRTP( "Proof Refine ", pWla->tPbr,                          tTotal );
        ABC_PRTP( "Misc.        ", tTotal - pWla->tPdr - pWla->tCbr - pWla->tPbr,   tTotal );
        ABC_PRTP( "Total        ", tTotal,                              tTotal );
    }

    return RetValue;
} 

int Wlc_NtkPdrAbs( Wlc_Ntk_t * p, Wlc_Par_t * pPars )
{
    Wla_Man_t * pWla = NULL;
    
    int RetValue = -1;

    pWla = Wla_ManStart( p, pPars );

    RetValue = Wla_ManSolve( pWla, pPars );

    Wla_ManStop( pWla );
    
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Performs abstraction.]

  Description [Derives initial abstraction based on user-specified
  parameter values, which tell what is the smallest bit-width of a
  primitive that is being abstracted away.  Currently only add/sub,
  mul/div, mux, and flop are supported with individual parameters.
  The second step is to refine the initial abstraction until the
  point when the property is proved.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Wlc_NtkAbsCore( Wlc_Ntk_t * p, Wlc_Par_t * pPars )
{
    abctime clk = Abc_Clock();
    Vec_Int_t * vBlacks = NULL;
    int nIters, nNodes, nDcFlops, RetValue = -1;
    // start the bitmap to mark objects that cannot be abstracted because of refinement
    // currently, this bitmap is empty because abstraction begins without refinement
    Vec_Bit_t * vUnmark = Vec_BitStart( Wlc_NtkObjNumMax(p) );
    // set up parameters to run PDR
    Pdr_Par_t PdrPars, * pPdrPars = &PdrPars;
    Pdr_ManSetDefaultParams( pPdrPars );
    //pPdrPars->fUseAbs    = 1;   // use 'pdr -t'  (on-the-fly abstraction)
    //pPdrPars->fCtgs      = 1;   // use 'pdr -nc' (improved generalization)
    //pPdrPars->fSkipDown  = 0;   // use 'pdr -nc' (improved generalization)
    //pPdrPars->nRestLimit = 500; // reset queue or proof-obligations when it gets larger than this
    pPdrPars->fVerbose   = pPars->fPdrVerbose;
    pPdrPars->fVeryVerbose = 0;
    // perform refinement iterations
    for ( nIters = 1; nIters < pPars->nIterMax; nIters++ )
    {
        Aig_Man_t * pAig;
        Abc_Cex_t * pCex;
        Vec_Int_t * vPisNew, * vRefine;  
        Gia_Man_t * pGia, * pTemp;
        Wlc_Ntk_t * pAbs;

        if ( pPars->fVerbose )
            printf( "\nIteration %d:\n", nIters );

        // get abstracted GIA and the set of pseudo-PIs (vPisNew)
        if ( pPars->fAbs2 )
        {
            if ( vBlacks == NULL )
                vBlacks = Wlc_NtkGetBlacks( p, pPars );
            else
                Wlc_NtkUpdateBlacks( p, pPars, &vBlacks, vUnmark, NULL );
            pAbs = Wlc_NtkAbs2( p, vBlacks, NULL );
            vPisNew = Vec_IntDup( vBlacks );
        }
        else
        {
            if ( nIters == 1 && pPars->nLimit < ABC_INFINITY )
                Wlc_NtkSetUnmark( p, pPars, vUnmark );

            pAbs = Wlc_NtkAbs( p, pPars, vUnmark, &vPisNew, NULL, pPars->fVerbose );
        }
        pGia = Wlc_NtkBitBlast( pAbs, NULL );

        // if the abstraction has flops with DC-init state,
        // new PIs were introduced by bit-blasting at the end of the PI list
        // here we move these variables to be *before* PPIs, because
        // PPIs are supposed to be at the end of the PI list for refinement
        nDcFlops = Wlc_NtkDcFlopNum(pAbs);
        if ( nDcFlops > 0 ) // DC-init flops are present
        {
            pGia = Gia_ManPermuteInputs( pTemp = pGia, Wlc_NtkCountObjBits(p, vPisNew), nDcFlops );
            Gia_ManStop( pTemp );
        }
        // if the word-level outputs have to be XORs, this is a place to do it
        if ( pPars->fXorOutput )
        {
            pGia = Gia_ManTransformMiter2( pTemp = pGia );
            Gia_ManStop( pTemp );
        }
        if ( pPars->fVerbose )
        {
            printf( "Derived abstraction with %d objects and %d PPIs. Bit-blasted AIG stats are:\n", Wlc_NtkObjNum(pAbs), Vec_IntSize(vPisNew) ); 
            Gia_ManPrintStats( pGia, NULL );
        }
        Wlc_NtkFree( pAbs );

        // try to prove abstracted GIA by converting it to AIG and calling PDR
        pAig = Gia_ManToAigSimple( pGia );
        RetValue = Pdr_ManSolve( pAig, pPdrPars );
        pCex = pAig->pSeqModel; pAig->pSeqModel = NULL;
        Aig_ManStop( pAig );

        // consider outcomes
        if ( pCex == NULL ) 
        {
            assert( RetValue ); // proved or undecided
            Gia_ManStop( pGia );
            Vec_IntFree( vPisNew );
            break;
        }

        // perform refinement
        vRefine = Wlc_NtkAbsRefinement( p, pGia, pCex, vPisNew );
        Gia_ManStop( pGia );
        Vec_IntFree( vPisNew );
        if ( vRefine == NULL ) // real CEX
        {
            Abc_CexFree( pCex ); // return CEX in the future
            break;
        }

        // update the set of objects to be un-abstracted
        nNodes = Wlc_NtkRemoveFromAbstraction( p, vRefine, vUnmark );
        if ( pPars->fVerbose )
            printf( "Refinement of CEX in frame %d came up with %d un-abstacted PPIs, whose MFFCs include %d objects.\n", pCex->iFrame, Vec_IntSize(vRefine), nNodes );
        Vec_IntFree( vRefine );
        Abc_CexFree( pCex );
    }
    Vec_IntFreeP( &vBlacks );
    Vec_BitFreeP( &vUnmark );
    // report the result
    if ( pPars->fVerbose )
        printf( "\n" );
    printf( "Abstraction " );
    if ( RetValue == 0 )
        printf( "resulted in a real CEX" );
    else if ( RetValue == 1 )
        printf( "is successfully proved" );
    else 
        printf( "timed out" );
    printf( " after %d iterations. ", nIters );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    return RetValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

