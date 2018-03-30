/**CFile****************************************************************

  FileName    [mainInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [The main package.]

  Synopsis    [Internal declarations of the main package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: mainInt.h,v 1.1 2008/05/14 22:13:13 wudenni Exp $]

***********************************************************************/

#ifndef ABC__base__main__mainInt_h
#define ABC__base__main__mainInt_h

 
////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "main.h"
#include "misc/tim/tim.h"
#include "map/if/if.h"
#include "aig/aig/aig.h"
#include "aig/gia/gia.h"
#include "proof/ssw/ssw.h"
#include "proof/fra/fra.h"

#ifdef ABC_USE_CUDD
#include "bdd/extrab/extraBdd.h"
#endif

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

// the current version
#define ABC_VERSION "UC Berkeley, ABC 1.01"

// the maximum length of an input line 
#define ABC_MAX_STR     (1<<15)

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef void (*Abc_Frame_Callback_BmcFrameDone_Func)(int frame, int po, int status);

struct Abc_Frame_t_
{
    // general info
    char *          sVersion;      // the name of the current version
    char *          sBinary;       // the name of the binary running
    // commands, aliases, etc
    st__table *      tCommands;     // the command table
    st__table *      tAliases;      // the alias table
    st__table *      tFlags;        // the flag table
    Vec_Ptr_t *     aHistory;      // the command history
    // the functionality
    Abc_Ntk_t *     pNtkCur;       // the current network
    Abc_Ntk_t *     pNtkBestDelay; // the current network
    Abc_Ntk_t *     pNtkBestArea;  // the current network
    Abc_Ntk_t *     pNtkBackup;    // the current network
    int             nSteps;        // the counter of different network processed
    int             fSource;       // marks the source mode
    int             fAutoexac;     // marks the autoexec mode
    int             fBatchMode;    // batch mode flag
    int             fBridgeMode;   // bridge mode flag
    // output streams
    FILE *          Out;
    FILE *          Err;
    FILE *          Hst;
    // used for runtime measurement
    double          TimeCommand;   // the runtime of the last command
    double          TimeTotal;     // the total runtime of all commands
    // temporary storage for structural choices
    Vec_Ptr_t *     vStore;        // networks to be used by choice
    // decomposition package    
    void *          pManDec;       // decomposition manager
    void *          pManDsd;       // decomposition manager
    void *          pManDsd2;      // decomposition manager
    // libraries for mapping
    void *          pLibLut;       // the current LUT library
    void *          pLibBox;       // the current box library
    void *          pLibGen;       // the current genlib
    void *          pLibGen2;      // the current genlib
    void *          pLibSuper;     // the current supergate library
    void *          pLibScl;       // the current Liberty library
    void *          pAbcCon;       // constraint manager
    // timing constraints
    char *          pDrivingCell;  // name of the driving cell
    float           MaxLoad;       // maximum output load
    // inductive don't-cares
    Vec_Int_t *     vIndFlops;
    int             nIndFrames;

    // new code
    Gia_Man_t *     pGia;          // alternative current network as a light-weight AIG
    Gia_Man_t *     pGia2;         // copy of the above
    Gia_Man_t *     pGiaBest;      // copy of the above
    Gia_Man_t *     pGiaBest2;     // copy of the above
    Gia_Man_t *     pGiaSaved;     // copy of the above
    int             nBestLuts;     // best LUT count
    int             nBestEdges;    // best edge count
    int             nBestLevels;   // best level count
    int             nBestLuts2;     // best LUT count
    int             nBestEdges2;    // best edge count
    int             nBestLevels2;   // best level count
    Abc_Cex_t *     pCex;          // a counter-example to fail the current network
    Abc_Cex_t *     pCex2;         // copy of the above
    Vec_Ptr_t *     vCexVec;       // a vector of counter-examples if more than one PO fails
    Vec_Ptr_t *     vPoEquivs;     // equivalence classes of isomorphic primary outputs
    Vec_Int_t *     vStatuses;     // problem status for each output
    Vec_Int_t *     vAbcObjIds;    // object IDs
    int             Status;                // the status of verification problem (proved=1, disproved=0, undecided=-1)
    int             nFrames;               // the number of time frames completed by BMC
    Vec_Ptr_t *     vPlugInComBinPairs;    // pairs of command and its binary name
    Vec_Ptr_t *     vLTLProperties_global; // related to LTL
    void *          pSave1; 
    void *          pSave2; 
    void *          pSave3; 
    void *          pSave4; 
    void *          pAbc85Ntl;
    void *          pAbc85Ntl2;
    void *          pAbc85Best;
    void *          pAbc85Delay;
    void *          pAbcWlc;
    Vec_Int_t *     pAbcWlcInv;
    void *          pAbcBac;
    void *          pAbcCba;
    void *          pAbcPla;
    Abc_Nam_t *     pJsonStrs;
    Vec_Wec_t *     vJsonObjs;
#ifdef ABC_USE_CUDD
    DdManager *     dd;            // temporary BDD package
#endif
    Gia_Man_t *     pGiaMiniAig; 
    Gia_Man_t *     pGiaMiniLut; 
    Vec_Int_t *     vCopyMiniAig;
    Vec_Int_t *     vCopyMiniLut;
    int *           pArray;
    int *           pBoxes;

    Abc_Frame_Callback_BmcFrameDone_Func pFuncOnFrameDone;
};

typedef void (*Abc_Frame_Initialization_Func)( Abc_Frame_t * pAbc );

struct Abc_FrameInitializer_t_;
typedef struct Abc_FrameInitializer_t_ Abc_FrameInitializer_t;

struct Abc_FrameInitializer_t_
{
    Abc_Frame_Initialization_Func init;
    Abc_Frame_Initialization_Func destroy;

    Abc_FrameInitializer_t* next;
    Abc_FrameInitializer_t* prev;
};

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== mvMain.c ===========================================================*/
extern ABC_DLL int             main( int argc, char * argv[] );
/*=== mvInit.c ===================================================*/
extern ABC_DLL void            Abc_FrameInit( Abc_Frame_t * pAbc );
extern ABC_DLL void            Abc_FrameEnd( Abc_Frame_t * pAbc );
extern ABC_DLL void            Abc_FrameAddInitializer( Abc_FrameInitializer_t* p );
/*=== mvFrame.c =====================================================*/
extern ABC_DLL Abc_Frame_t *   Abc_FrameAllocate();
extern ABC_DLL void            Abc_FrameDeallocate( Abc_Frame_t * p );
/*=== mvUtils.c =====================================================*/
extern ABC_DLL char *          Abc_UtilsGetVersion( Abc_Frame_t * pAbc );
extern ABC_DLL char *          Abc_UtilsGetUsersInput( Abc_Frame_t * pAbc );
extern ABC_DLL void            Abc_UtilsPrintHello( Abc_Frame_t * pAbc );
extern ABC_DLL void            Abc_UtilsPrintUsage( Abc_Frame_t * pAbc, char * ProgName );
extern ABC_DLL void            Abc_UtilsSource( Abc_Frame_t * pAbc );



ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
