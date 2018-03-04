/**CFile****************************************************************

  FileName    [abcScorr.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Signal correspondence testing procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcScorr.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#include "base/abc/abc.h"
#include "base/io/ioAbc.h"
#include "aig/saig/saig.h"
#include "proof/ssw/ssw.h"
#include "aig/gia/gia.h"
#include "proof/cec/cec.h"
#include "aig/gia/giaAig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Tst_Dat_t_ Tst_Dat_t;
struct Tst_Dat_t_
{
    Abc_Ntk_t * pNetlist;
    Aig_Man_t * pAig;
    Gia_Man_t * pGia;
    Vec_Int_t * vId2Name;
    char *      pFileNameOut;
    int         fFlopOnly;
    int         fFfNdOnly;
    int         fDumpBmc;      
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Abc_NtkMapGiaIntoNameId( Abc_Ntk_t * pNetlist, Aig_Man_t * pAig, Gia_Man_t * pGia )
{
    Vec_Int_t * vId2Name;
    Abc_Obj_t * pNet, * pNode, * pAnd;
    Aig_Obj_t * pObjAig;
    int i;
    vId2Name = Vec_IntAlloc( 0 );
    Vec_IntFill( vId2Name, pGia ? Gia_ManObjNum(pGia) : Aig_ManObjNumMax(pAig), ~0 );
    // copy all names
    Abc_NtkForEachNet( pNetlist, pNet, i )
    {
        pNode = Abc_ObjFanin0(pNet)->pCopy; 
        if ( pNode && (pAnd = Abc_ObjRegular(pNode->pCopy)) && 
            (pObjAig = (Aig_Obj_t *)Abc_ObjRegular(pAnd->pCopy)) && 
             Aig_ObjType(pObjAig) != AIG_OBJ_NONE )
        {
            if ( pGia == NULL )
                Vec_IntWriteEntry( vId2Name, Aig_ObjId(pObjAig), Abc_ObjId(pNet) );
            else
                Vec_IntWriteEntry( vId2Name, Abc_Lit2Var(pObjAig->iData), Abc_ObjId(pNet) );
        }
    }
    // overwrite CO names
    Abc_NtkForEachCo( pNetlist, pNode, i )
    {
        pNet = Abc_ObjFanin0(pNode);
        pNode = pNode->pCopy;
        if ( pNode && (pAnd = Abc_ObjRegular(pNode->pCopy)) && 
            (pObjAig = (Aig_Obj_t *)Abc_ObjRegular(pAnd->pCopy)) && 
             Aig_ObjType(pObjAig) != AIG_OBJ_NONE )
        {
            if ( pGia == NULL )
                Vec_IntWriteEntry( vId2Name, Aig_ObjId(pObjAig), Abc_ObjId(pNet) );
            else
                Vec_IntWriteEntry( vId2Name, Abc_Lit2Var(pObjAig->iData), Abc_ObjId(pNet) );
        }
    }
    // overwrite CI names
    Abc_NtkForEachCi( pNetlist, pNode, i )
    {
        pNet = Abc_ObjFanout0(pNode);
        pNode = pNode->pCopy;
        if ( pNode && (pAnd = Abc_ObjRegular(pNode->pCopy)) && 
            (pObjAig = (Aig_Obj_t *)Abc_ObjRegular(pAnd->pCopy)) && 
             Aig_ObjType(pObjAig) != AIG_OBJ_NONE )
        {
            if ( pGia == NULL )
                Vec_IntWriteEntry( vId2Name, Aig_ObjId(pObjAig), Abc_ObjId(pNet) );
            else
                Vec_IntWriteEntry( vId2Name, Abc_Lit2Var(pObjAig->iData), Abc_ObjId(pNet) );
        }
    }
    return vId2Name;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_NtkTestScorrGetName( Abc_Ntk_t * pNetlist, Vec_Int_t * vId2Name, int Id )
{
//    Abc_Obj_t * pObj;
//printf( "trying to get name for %d\n", Id );
    if ( Vec_IntEntry(vId2Name, Id) == ~0 )
        return NULL;
//    pObj = Abc_NtkObj( pNetlist, Vec_IntEntry(vId2Name, Id) );
//    pObj = Abc_ObjFanin0(pObj);
//    assert( Abc_ObjIsCi(pObj) );
    return Nm_ManFindNameById( pNetlist->pManName, Vec_IntEntry(vId2Name, Id) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkTestScorrWriteEquivPair( Abc_Ntk_t * pNetlist, Vec_Int_t * vId2Name, int Id1, int Id2, FILE * pFile, int fPol )
{
    char * pName1 = Abc_NtkTestScorrGetName( pNetlist, vId2Name, Id1 );
    char * pName2 = Abc_NtkTestScorrGetName( pNetlist, vId2Name, Id2 );
    if ( pName1 == NULL || pName2 == NULL )
        return 0;
    fprintf( pFile, "%s=%s%s\n", pName1, fPol? "~": "", pName2 );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkTestScorrWriteEquivConst( Abc_Ntk_t * pNetlist, Vec_Int_t * vId2Name, int Id1, FILE * pFile, int fPol )
{
    char * pName1 = Abc_NtkTestScorrGetName( pNetlist, vId2Name, Id1 );
    if ( pName1 == NULL )
        return 0;
    fprintf( pFile, "%s=%s%s\n", pName1, fPol? "~": "", "const0" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_NtkBmcFileName( char * pName )
{
    static char Buffer[1000];
    char * pNameGeneric = Extra_FileNameGeneric( pName );
    sprintf( Buffer, "%s_bmc%s", pNameGeneric, pName + strlen(pNameGeneric) );
    ABC_FREE( pNameGeneric );
    return Buffer;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkTestScorrWriteEquivGia( Tst_Dat_t * pData )
{
    Abc_Ntk_t * pNetlist = pData->pNetlist;
    Vec_Int_t * vId2Name = pData->vId2Name;
    Gia_Man_t * pGia     = pData->pGia;
    char * pFileNameOut  = pData->pFileNameOut;
    FILE * pFile;
    Gia_Obj_t * pObj, * pRepr;
    int i, Counter = 0;
    if ( pData->fDumpBmc )
    {
        pData->fDumpBmc = 0;
        pFileNameOut = Abc_NtkBmcFileName( pFileNameOut );
    }
    pFile = fopen( pFileNameOut, "wb" );
    Gia_ManSetPhase( pGia );
    Gia_ManForEachObj( pGia, pObj, i )
    {
        if ( !Gia_ObjHasRepr(pGia, i) )
            continue;
        pRepr = Gia_ManObj( pGia,Gia_ObjRepr(pGia, i) );
        if ( pData->fFlopOnly )
        {
            if ( !Gia_ObjIsRo(pGia, pObj) || !(Gia_ObjIsRo(pGia, pRepr)||Gia_ObjIsConst0(pRepr)) )
                continue;
        }
        else if ( pData->fFfNdOnly )
        {
            if ( !Gia_ObjIsRo(pGia, pObj) && !(Gia_ObjIsRo(pGia, pRepr)||Gia_ObjIsConst0(pRepr)) )
                continue;
        }
        if ( Gia_ObjRepr(pGia, i) == 0 )
            Counter += Abc_NtkTestScorrWriteEquivConst( pNetlist, vId2Name, i, pFile, Gia_ObjPhase(pObj) );
        else
            Counter += Abc_NtkTestScorrWriteEquivPair( pNetlist, vId2Name, Gia_ObjRepr(pGia, i), i, pFile, 
                Gia_ObjPhase(pRepr) ^ Gia_ObjPhase(pObj) );
    }
    fclose( pFile );
    printf( "%d pairs of sequentially equivalent nodes are written into file \"%s\".\n", Counter, pFileNameOut );
    return Counter;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkTestScorrWriteEquivAig( Tst_Dat_t * pData )
{
    Abc_Ntk_t * pNetlist = pData->pNetlist;
    Vec_Int_t * vId2Name = pData->vId2Name;
    Aig_Man_t * pAig     = pData->pAig;
    char * pFileNameOut  = pData->pFileNameOut;
    FILE * pFile;
    Aig_Obj_t * pObj, * pRepr;
    int i, Counter = 0;
    if ( pData->fDumpBmc )
    {
        pData->fDumpBmc = 0;
        pFileNameOut = Abc_NtkBmcFileName( pFileNameOut );
    }
    pFile = fopen( pFileNameOut, "wb" );
    Aig_ManForEachObj( pAig, pObj, i )
    {
        if ( (pRepr = Aig_ObjRepr(pAig, pObj)) == NULL )
            continue;
        if ( pData->fFlopOnly )
        {
            if ( !Saig_ObjIsLo(pAig, pObj) || !(Saig_ObjIsLo(pAig, pRepr)||pRepr==Aig_ManConst1(pAig)) )
                continue;
        }
        else if ( pData->fFfNdOnly )
        {
            if ( !Saig_ObjIsLo(pAig, pObj) && !(Saig_ObjIsLo(pAig, pRepr)||pRepr==Aig_ManConst1(pAig)) )
                continue;
        }
        if ( pRepr == Aig_ManConst1(pAig) )
            Counter += Abc_NtkTestScorrWriteEquivConst( pNetlist, vId2Name, Aig_ObjId(pObj), pFile, Aig_ObjPhase(pObj) );
        else
            Counter += Abc_NtkTestScorrWriteEquivPair( pNetlist, vId2Name, Aig_ObjId(pRepr), Aig_ObjId(pObj), pFile, 
                Aig_ObjPhase(pRepr) ^ Aig_ObjPhase(pObj) );
    }
    fclose( pFile );
    printf( "%d pairs of sequentially equivalent nodes are written into file \"%s\".\n", Counter, pFileNameOut );
    return Counter;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkTestScorr( char * pFileNameIn, char * pFileNameOut, int nStepsMax, int nBTLimit, int fNewAlgo, int fFlopOnly, int fFfNdOnly, int fVerbose )
{
    extern Aig_Man_t * Abc_NtkToDar( Abc_Ntk_t * pNtk, int fExors, int fRegisters );
    extern Abc_Ntk_t * Abc_NtkFromDarSeqSweep( Abc_Ntk_t * pNtkOld, Aig_Man_t * pMan );

    FILE * pFile;
    Tst_Dat_t Data, * pData = &Data;
    Vec_Int_t * vId2Name;
    Abc_Ntk_t * pNetlist, * pLogic, * pStrash, * pResult;
    Aig_Man_t * pAig, * pTempAig;
    Gia_Man_t * pGia, * pTempGia;
//    int Counter = 0;
    // check the files
    pFile = fopen( pFileNameIn, "rb" );
    if ( pFile == NULL )
    {
        printf( "Input file \"%s\" cannot be opened.\n", pFileNameIn );
        return NULL;
    }
    fclose( pFile );
    // check the files
    pFile = fopen( pFileNameOut, "wb" );
    if ( pFile == NULL )
    {
        printf( "Output file \"%s\" cannot be opened.\n", pFileNameOut );
        return NULL;
    }
    fclose( pFile );
    // derive AIG for signal correspondence
    pNetlist = Io_ReadNetlist( pFileNameIn, Io_ReadFileType(pFileNameIn), 1 );
    if ( pNetlist == NULL )
    {
        printf( "Reading input file \"%s\" has failed.\n", pFileNameIn );
        return NULL;
    }
    pLogic = Abc_NtkToLogic( pNetlist );
    if ( pLogic == NULL )
    {
        Abc_NtkDelete( pNetlist );
        printf( "Deriving logic network from input file %s has failed.\n", pFileNameIn );
        return NULL;
    }
    if ( Extra_FileIsType( pFileNameIn, ".bench", ".BENCH", NULL ) )
    {
        // get the init file name
        char * pFileNameInit = Extra_FileNameGenericAppend( pLogic->pSpec, ".init" );
        pFile = fopen( pFileNameInit, "rb" );
        if ( pFile == NULL )
        {
            printf( "Init file \"%s\" cannot be opened.\n", pFileNameInit );
            return NULL;
        }
        Io_ReadBenchInit( pLogic, pFileNameInit );
        Abc_NtkConvertDcLatches( pLogic );
        if ( fVerbose )
            printf( "Initial state was derived from file \"%s\".\n", pFileNameInit );
    }
    pStrash = Abc_NtkStrash( pLogic, 0, 1, 0 );
    if ( pStrash == NULL )
    {
        Abc_NtkDelete( pLogic );
        Abc_NtkDelete( pNetlist );
        printf( "Deriving strashed network from input file %s has failed.\n", pFileNameIn );
        return NULL;
    }
    pAig = Abc_NtkToDar( pStrash, 0, 1 ); // performs "zero" internally
    // the newer computation (&scorr)
    if ( fNewAlgo )
    {
        Cec_ParCor_t CorPars, * pCorPars = &CorPars;
        Cec_ManCorSetDefaultParams( pCorPars );
        pCorPars->nBTLimit  = nBTLimit;
        pCorPars->nStepsMax = nStepsMax;
        pCorPars->fVerbose  = fVerbose;
        pCorPars->fUseCSat  = 1;
        pGia = Gia_ManFromAig( pAig );
        // prepare the data-structure
        memset( pData, 0, sizeof(Tst_Dat_t) );
        pData->pNetlist     = pNetlist;
        pData->pAig         = NULL;
        pData->pGia         = pGia;
        pData->vId2Name     = vId2Name = Abc_NtkMapGiaIntoNameId( pNetlist, pAig, pGia );
        pData->pFileNameOut = pFileNameOut;
        pData->fFlopOnly    = fFlopOnly;
        pData->fFfNdOnly    = fFfNdOnly;
        pData->fDumpBmc     = 1;
        pCorPars->pData     = pData;
        pCorPars->pFunc     = (void *)Abc_NtkTestScorrWriteEquivGia;
        // call signal correspondence
        pTempGia = Cec_ManLSCorrespondence( pGia, pCorPars );
        pTempAig = Gia_ManToAigSimple( pTempGia );
        Gia_ManStop( pTempGia );
        Gia_ManStop( pGia );
    }
    // the older computation (scorr)
    else
    {
        Ssw_Pars_t SswPars, * pSswPars = &SswPars;
        Ssw_ManSetDefaultParams( pSswPars );
        pSswPars->nBTLimit  = nBTLimit;
        pSswPars->nStepsMax = nStepsMax;
        pSswPars->fVerbose  = fVerbose;
        // preSswPare the data-structure
        memset( pData, 0, sizeof(Tst_Dat_t) );
        pData->pNetlist     = pNetlist;
        pData->pAig         = pAig;
        pData->pGia         = NULL;
        pData->vId2Name     = vId2Name = Abc_NtkMapGiaIntoNameId( pNetlist, pAig, NULL );
        pData->pFileNameOut = pFileNameOut;
        pData->fFlopOnly    = fFlopOnly;
        pData->fFfNdOnly    = fFfNdOnly;
        pData->fDumpBmc     = 1;
        pSswPars->pData     = pData;
        pSswPars->pFunc     = (void *)Abc_NtkTestScorrWriteEquivAig;
        // call signal correspondence
        pTempAig = Ssw_SignalCorrespondence( pAig, pSswPars );
    }
    // create the resulting AIG
    pResult = Abc_NtkFromDarSeqSweep( pStrash, pTempAig );
    // cleanup
    Vec_IntFree( vId2Name );
    Aig_ManStop( pAig );
    Aig_ManStop( pTempAig );
    Abc_NtkDelete( pStrash );
    Abc_NtkDelete( pLogic );
    Abc_NtkDelete( pNetlist );
    return pResult;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

