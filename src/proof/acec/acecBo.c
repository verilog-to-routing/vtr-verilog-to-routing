/**CFile****************************************************************

  FileName    [acecBo.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [CEC for arithmetic circuits.]

  Synopsis    [Core procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: acecBo.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "acecInt.h"
#include "misc/vec/vecWec.h"
#include "misc/extra/extra.h"

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
int Acec_DetectBoothXorMux( Gia_Man_t * p, Gia_Obj_t * pMux, Gia_Obj_t * pXor, int pIns[3] )
{
    Gia_Obj_t * pFan0, * pFan1;
    Gia_Obj_t * pDat0, * pDat1, * pCtrl;
    if ( !Gia_ObjIsMuxType(pMux) || !Gia_ObjIsMuxType(pXor) )
        return 0;
    if ( !Gia_ObjRecognizeExor( pXor, &pFan0, &pFan1 ) )
        return 0;
    pFan0 = Gia_Regular(pFan0);
    pFan1 = Gia_Regular(pFan1);
    if ( Gia_ObjId(p, pFan0) > Gia_ObjId(p, pFan1) )
        ABC_SWAP( Gia_Obj_t *, pFan0, pFan1 );
    if ( !(pCtrl = Gia_ObjRecognizeMux( pMux, &pDat0, &pDat1 )) )
        return 0;
    pDat0 = Gia_Regular(pDat0);
    pDat1 = Gia_Regular(pDat1);
    pCtrl = Gia_Regular(pCtrl);
    if ( !Gia_ObjIsAnd(pDat0) || !Gia_ObjIsAnd(pDat1) )
        return 0;
    if ( Gia_ObjFaninId0p(p, pDat0) != Gia_ObjFaninId0p(p, pDat1) ||
         Gia_ObjFaninId1p(p, pDat0) != Gia_ObjFaninId1p(p, pDat1) )
         return 0;
    if ( Gia_ObjFaninId0p(p, pDat0) != Gia_ObjId(p, pFan0) ||
         Gia_ObjFaninId1p(p, pDat0) != Gia_ObjId(p, pFan1) )
         return 0;
    pIns[0] = Gia_ObjId(p, pFan0);
    pIns[1] = Gia_ObjId(p, pFan1);
    pIns[2] = Gia_ObjId(p, pCtrl);
    return 1;
}
int Acec_DetectBoothXorFanin( Gia_Man_t * p, Gia_Obj_t * pObj, int pIns[5] )
{
    Gia_Obj_t * pFan0, * pFan1;
    //int Id = Gia_ObjId(p, pObj);
    if ( !Gia_ObjIsAnd(pObj) )
        return 0;
    if ( !Gia_ObjFaninC0(pObj) || !Gia_ObjFaninC1(pObj) )
        return 0;
    pFan0 = Gia_ObjFanin0(pObj);
    pFan1 = Gia_ObjFanin1(pObj);
    if ( !Gia_ObjIsAnd(pFan0) || !Gia_ObjIsAnd(pFan1) )
        return 0;
    if ( Acec_DetectBoothXorMux(p, Gia_ObjFanin0(pFan0), Gia_ObjFanin0(pFan1), pIns) )
    {
        pIns[3] = Gia_ObjId(p, Gia_ObjFanin1(pFan0));
        pIns[4] = Gia_ObjId(p, Gia_ObjFanin1(pFan1));
        return 1;
    }
    if ( Acec_DetectBoothXorMux(p, Gia_ObjFanin0(pFan0), Gia_ObjFanin1(pFan1), pIns) )
    {
        pIns[3] = Gia_ObjId(p, Gia_ObjFanin1(pFan0));
        pIns[4] = Gia_ObjId(p, Gia_ObjFanin0(pFan1));
        return 1;
    }
    if ( Acec_DetectBoothXorMux(p, Gia_ObjFanin1(pFan0), Gia_ObjFanin0(pFan1), pIns) )
    {
        pIns[3] = Gia_ObjId(p, Gia_ObjFanin0(pFan0));
        pIns[4] = Gia_ObjId(p, Gia_ObjFanin1(pFan1));
        return 1;
    }
    if ( Acec_DetectBoothXorMux(p, Gia_ObjFanin1(pFan0), Gia_ObjFanin1(pFan1), pIns) )
    {
        pIns[3] = Gia_ObjId(p, Gia_ObjFanin0(pFan0));
        pIns[4] = Gia_ObjId(p, Gia_ObjFanin0(pFan1));
        return 1;
    }
    return 0;
}
int Acec_DetectBoothOne( Gia_Man_t * p, Gia_Obj_t * pObj, int pIns[5] )
{
    Gia_Obj_t * pFan0, * pFan1;
    if ( !Gia_ObjRecognizeExor( pObj, &pFan0, &pFan1 ) )
        return 0;
    pFan0 = Gia_Regular(pFan0);
    pFan1 = Gia_Regular(pFan1);
    if ( Acec_DetectBoothXorFanin( p, pFan0, pIns ) && pIns[2] == Gia_ObjId(p, pFan1) )
        return 1;
    if ( Acec_DetectBoothXorFanin( p, pFan1, pIns ) && pIns[2] == Gia_ObjId(p, pFan0) )
        return 1;
    return 0;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Acec_DetectBoothTwoXor( Gia_Man_t * p, Gia_Obj_t * pObj, int pIns[5] )
{
    Gia_Obj_t * pFan0, * pFan1;
    if ( !Gia_ObjIsAnd(pObj) )
        return 0;
    if ( Gia_ObjRecognizeExor( Gia_ObjFanin0(pObj), &pFan0, &pFan1 ) )
    {
        pIns[0] = Gia_ObjId(p, Gia_Regular(pFan0));
        pIns[1] = Gia_ObjId(p, Gia_Regular(pFan1));
        pIns[2] = -1;
        pIns[3] = 0;
        pIns[4] = Gia_ObjId(p, Gia_ObjFanin1(pObj));
        return 1;
    }
    if ( Gia_ObjRecognizeExor( Gia_ObjFanin1(pObj), &pFan0, &pFan1 ) )
    {
        pIns[0] = Gia_ObjId(p, Gia_Regular(pFan0));
        pIns[1] = Gia_ObjId(p, Gia_Regular(pFan1));
        pIns[2] = -1;
        pIns[3] = 0;
        pIns[4] = Gia_ObjId(p, Gia_ObjFanin0(pObj));
        return 1;
    }
    return 0;
}
int Acec_DetectBoothTwo( Gia_Man_t * p, Gia_Obj_t * pObj, int pIns[5] )
{
    Gia_Obj_t * pFan0, * pFan1;
    if ( !Gia_ObjRecognizeExor( pObj, &pFan0, &pFan1 ) )
        return 0;
    pFan0 = Gia_Regular(pFan0);
    pFan1 = Gia_Regular(pFan1);
    if ( Acec_DetectBoothTwoXor( p, pFan0, pIns ) )
    {
        pIns[2] = Gia_ObjId(p, pFan1);
        return 1;
    }
    if ( Acec_DetectBoothTwoXor( p, pFan1, pIns ) )
    {
        pIns[2] = Gia_ObjId(p, pFan0);
        return 1;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Acec_DetectBoothTest( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i, pIns[5];
    Gia_ManForEachAnd( p, pObj, i )
    {        
        if ( !Acec_DetectBoothOne(p, pObj, pIns) && !Acec_DetectBoothTwo(p, pObj, pIns) )
                continue;
        printf( "obj = %4d  :  b0 = %4d  b1 = %4d  b2 = %4d    a0 = %4d  a1 = %4d\n", 
            i, pIns[0], pIns[1], pIns[2], pIns[3], pIns[4] );
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

