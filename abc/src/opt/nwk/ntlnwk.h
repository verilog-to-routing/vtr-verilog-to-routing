/**CFile****************************************************************

  FileName    [ntlnwk.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Netlist and network representation.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: ntlnwk.h,v 1.3 2008/10/24 14:18:44 mjarvin Exp $]

***********************************************************************/
 
#ifndef __NTLNWK_abc_opt_nwk_h
#define __NTLNWK_abc_opt_nwk_h


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

typedef struct Ntl_Man_t_    Ntl_Man_t;
typedef struct Nwk_Man_t_    Nwk_Man_t;

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                      INLINED FUNCTIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         ITERATORS                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

extern ABC_DLL Ntl_Man_t * Ntl_ManReadBlif( char * pFileName, int fCheck );
extern ABC_DLL void        Ntl_ManWriteBlif( Ntl_Man_t * p, char * pFileName );

extern ABC_DLL Tim_Man_t * Ntl_ManReadTimeMan( Ntl_Man_t * p );
extern ABC_DLL Ntl_Man_t * Ntl_ManDup( Ntl_Man_t * p );
extern ABC_DLL void        Ntl_ManFree( Ntl_Man_t * p );
extern ABC_DLL int         Ntl_ManIsComb( Ntl_Man_t * p );
extern ABC_DLL void        Ntl_ManPrintStats( Ntl_Man_t * p );
extern ABC_DLL int         Ntl_ManSweep( Ntl_Man_t * p, int fVerbose );
extern ABC_DLL Ntl_Man_t * Ntl_ManInsertNtk( Ntl_Man_t * p, Nwk_Man_t * pNtk );
extern ABC_DLL Ntl_Man_t * Ntl_ManInsertAig( Ntl_Man_t * p, Aig_Man_t * pAig );
extern ABC_DLL Aig_Man_t * Ntl_ManExtract( Ntl_Man_t * p );
extern ABC_DLL Aig_Man_t * Ntl_ManCollapse( Ntl_Man_t * p, int fSeq );
extern ABC_DLL Aig_Man_t * Ntl_ManCollapseSeq( Ntl_Man_t * p, int nMinDomSize, int fVerbose );
extern ABC_DLL Ntl_Man_t * Ntl_ManDupCollapseLuts( Ntl_Man_t * p );
extern ABC_DLL Ntl_Man_t * Ntl_ManFraig( Ntl_Man_t * p, int nPartSize, int nConfLimit, int nLevelMax, int fUseCSat, int fVerbose );
extern ABC_DLL void        Ntl_ManPrepareCecMans( Ntl_Man_t * pMan1, Ntl_Man_t * pMan2, Aig_Man_t ** ppAig1, Aig_Man_t ** ppAig2 );
extern ABC_DLL Vec_Ptr_t * Ntl_ManCollectCiNames( Ntl_Man_t * p );
extern ABC_DLL Vec_Ptr_t * Ntl_ManCollectCoNames( Ntl_Man_t * p );
extern ABC_DLL Ntl_Man_t * Ntl_ManScl( Ntl_Man_t * p, int fLatchConst, int fLatchEqual, int fVerbose );
extern ABC_DLL Ntl_Man_t * Ntl_ManLcorr( Ntl_Man_t * p, int nConfMax, int fScorrGia, int fUseCSat, int fVerbose );
extern ABC_DLL Ntl_Man_t * Ntl_ManSsw( Ntl_Man_t * p, Fra_Ssw_t * pPars );
extern ABC_DLL Ntl_Man_t * Ntl_ManScorr( Ntl_Man_t * p, Ssw_Pars_t * pPars );
extern ABC_DLL void        Ntl_ManTransformInitValues( Ntl_Man_t * p );

extern ABC_DLL void        Ntl_ManPrepareCec( char * pFileName1, char * pFileName2, Aig_Man_t ** ppMan1, Aig_Man_t ** ppMan2 );
extern ABC_DLL Aig_Man_t * Ntl_ManPrepareSec( char * pFileName1, char * pFileName2 );

extern ABC_DLL Nwk_Man_t * Ntl_ManExtractNwk( Ntl_Man_t * p, Aig_Man_t * pAig, Tim_Man_t * pManTime );
extern ABC_DLL Nwk_Man_t * Ntl_ManReadNwk( char * pFileName, Aig_Man_t * pAig, Tim_Man_t * pManTime );
extern ABC_DLL void        Nwk_ManPrintStats( Nwk_Man_t * p, If_LibLut_t * pLutLib, int fSaveBest, int fDumpResult, int fPower, Ntl_Man_t * pNtl );
extern ABC_DLL void        Nwk_ManPrintStatsShort( Ntl_Man_t * p, Aig_Man_t * pAig, Nwk_Man_t * pNtk );
extern ABC_DLL void        Nwk_ManPrintFanioNew( Nwk_Man_t * p );
extern ABC_DLL Nwk_Man_t * Nwk_MappingIf( Aig_Man_t * p, Tim_Man_t * pManTime, If_Par_t * pPars );
extern ABC_DLL void        Nwk_ManSetIfParsDefault( If_Par_t * pPars );
extern ABC_DLL void        Nwk_ManBidecResyn( Nwk_Man_t * p, int fVerbose );
extern ABC_DLL Aig_Man_t * Nwk_ManSpeedup( Nwk_Man_t * p, int fUseLutLib, int Percentage, int Degree, int fVerbose, int fVeryVerbose );
extern ABC_DLL Aig_Man_t * Nwk_ManStrash( Nwk_Man_t * p );
extern ABC_DLL Vec_Int_t * Nwk_ManLutMerge( Nwk_Man_t * p, void * pPars );
extern ABC_DLL int         Nwk_ManCheck( Nwk_Man_t * p );
extern ABC_DLL void        Nwk_ManDumpBlif( Nwk_Man_t * p, char * pFileName, Vec_Ptr_t * vCiNames, Vec_Ptr_t * vCoNames );
extern ABC_DLL void        Nwk_ManFree( Nwk_Man_t * p );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

