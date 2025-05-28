/**CFile****************************************************************

  FileName    [amap.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Technology mapper for standard cells.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: amap.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__map__amap__amap_h
#define ABC__map__amap__amap_h


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

typedef struct Amap_Lib_t_ Amap_Lib_t;

typedef struct Amap_Par_t_ Amap_Par_t;
struct Amap_Par_t_
{
    int    nIterFlow;   // iterations of area flow
    int    nIterArea;   // iteratoins of exact area
    int    nCutsMax;    // the maximum number of cuts at a node
    int    fUseMuxes;   // enables the use of MUXes
    int    fUseXors;    // enables the use of XORs
    int    fFreeInvs;   // assume inverters are free (area = 0)
    float  fEpsilon;    // used to compare floating point numbers
    float  fADratio;    // ratio of area/delay improvement
    int    fVerbose;    // verbosity flag
};

typedef struct Amap_Out_t_ Amap_Out_t;
struct Amap_Out_t_
{
    char * pName;     // gate name
    short  Type;      // node type   (-1=input; 0=internal; 1=output)
    short  nFans;     // number of fanins
    int    pFans[0];  // fanin
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== amapCore.c ==========================================================*/
extern void          Amap_ManSetDefaultParams( Amap_Par_t * pPars );
//extern Vec_Ptr_t * Amap_ManTest( Aig_Man_t * pAig, Amap_Par_t * pPars );
/*=== amapLib.c ==========================================================*/
extern void          Amap_LibFree( Amap_Lib_t * p );
extern void          Amap_LibPrintSelectedGates( Amap_Lib_t * p, int fAllGates );
extern Amap_Lib_t *  Amap_LibReadAndPrepare( char * pFileName, char * pBuffer, int fVerbose, int fVeryVerbose );
/*=== amapLiberty.c ==========================================================*/
extern int           Amap_LibertyParse( char * pFileName, int fVerbose );
extern Vec_Str_t *   Amap_LibertyParseStr( char * pFileName, int fVerbose );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

