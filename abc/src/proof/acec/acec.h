/**CFile****************************************************************

  FileName    [acec.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [CEC for arithmetic circuits.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: acec.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__proof__acec__acec_h
#define ABC__proof__acec__acec_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_HEADER_START
 

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

// combinational equivalence checking parameters
typedef struct Acec_ParCec_t_ Acec_ParCec_t;
struct Acec_ParCec_t_
{
    int              nBTLimit;      // conflict limit at a node
    int              TimeLimit;     // the runtime limit in seconds
    int              fMiter;        // input circuit is a miter
    int              fDualOutput;   // dual-output miter
    int              fTwoOutput;    // two-output miter
    int              fBooth;        // expecting Booth multiplier
    int              fSilent;       // print no messages
    int              fVeryVerbose;  // verbose stats
    int              fVerbose;      // verbose stats
    int              iOutFail;      // the number of failed output
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                             ITERATORS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== acecCl.c ========================================================*/
extern Gia_Man_t *   Acec_ManDecla( Gia_Man_t * pGia, int fBooth, int fVerbose );
/*=== acecCore.c ========================================================*/
extern void          Acec_ManCecSetDefaultParams( Acec_ParCec_t * p );
extern int           Acec_Solve( Gia_Man_t * pGia0, Gia_Man_t * pGia1, Acec_ParCec_t * pPars );
/*=== acecFadds.c ========================================================*/
extern Vec_Int_t *   Gia_ManDetectFullAdders( Gia_Man_t * p, int fVerbose, Vec_Int_t ** vCutsXor2 );
extern Vec_Int_t *   Gia_ManDetectHalfAdders( Gia_Man_t * p, int fVerbose );
/*=== acecOrder.c ========================================================*/
extern Vec_Int_t *   Gia_PolynReorder( Gia_Man_t * pGia, int fVerbose, int fVeryVerbose );
extern Vec_Int_t *   Gia_PolynFindOrder( Gia_Man_t * pGia, Vec_Int_t * vFadds, Vec_Int_t * vHadds, int fVerbose, int fVeryVerbose );
/*=== acecPolyn.c ========================================================*/
extern void          Gia_PolynBuild( Gia_Man_t * pGia, Vec_Int_t * vOrder, int fSigned, int fVerbose, int fVeryVerbose );
/*=== acecRe.c ========================================================*/
extern Vec_Int_t *   Ree_ManComputeCuts( Gia_Man_t * p, Vec_Int_t ** pvXors, int fVerbose );
extern int           Ree_ManCountFadds( Vec_Int_t * vAdds );
extern void          Ree_ManPrintAdders( Vec_Int_t * vAdds, int fVerbose );
/*=== acecTree.c ========================================================*/
extern Gia_Man_t *   Acec_Normalize( Gia_Man_t * pGia, int fBooth, int fVerbose );


ABC_NAMESPACE_HEADER_END


#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

