/**CFile****************************************************************

  FileName    [tim.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchy/timing manager.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: tim.h,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__aig__tim__tim_h
#define ABC__aig__tim__tim_h

/*
    The data-structure Tim_Man_t implemented in this package stores two types 
    of information:
    (1) hierarchical information about the connectivity of a combinational 
    logic network with combinational logic node and combinational white boxes
    (2) timing information about input-to-output delays of each combinational 
    white box.

    This data-structure is closely coupled with the AIG manager extracted from 
    the same combinational logic network. The AIG manager represents combinational
    logic surrounding white boxes, and contains additional PIs/POs corresponding
    to the outputs/inputs of the white boxes.

    The manager Tim_Man_t is created by a call to Tim_ManStart(). The arguments 
    of this call are the total number of all combinational inputs/output in 
    the extracted AIG. (Note that this number is different from the number of 
    inputs/outputs of the combinational logic network, because the extracted AIG 
    will have additional inputs/output due to white boxes.)

    The extracted AIG and the corresponding Tim_Man_t may be created at the same 
    time or at separate times. The following guideline assumes concurrent creation.

    First, PIs of the AIG are created in 1-to-1 correspondence with the PIs 
    of the original network.
    Next, all nodes (logic nodes and white boxes) of the network are traversed 
    in a topologic order.
    When a white box is encountered, the TFI cone of box inputs are tranversed 
    and all new logic nodes encoutered added to the AIG.
    Then, the white box is created by the call to Tim_ManCreateBox().
    Then, new POs of the AIG are created in 1-to-1 correspondence with box inputs. 
    Then, new PIs of the AIG are created in 1-to-1 correspondence with box outputs.
    Finally, the TFO cone of the POs is traversed and all new logic nodes 
    encountered added to the AIG.
    In the end, the POs of the AIG is constructed in 1-to-1 correspondence with 
    the PIs of the original combinational logic network.

    Delay tables representing input-to-output delays of each type of white
    box should be computed in advance and given to the timing manager in one array
    through the API Tim_ManSetDelayTables().  When each box is constructed, the delay
    table ID of this box (which is the index of the table in the above array) is given 
    as the last argument 'iDelayTable' in Tim_ManCreateBox().

    A delay table is a one-dimensional array of floats whose size is: 3 + nInputs * nOutputs.
    The first entry is the delay table ID used by the boxes to refer to the table.
    The second and third entries are nInputs and nOutputs.
    The following 'nInputs * nOutputs' entries are delay numbers for each output, 
    that is, the first set of nInputs entries give delay of the first output.
    the second set of nInputs entries give delay of the second output, etc.

    The Tim_Man_t is typically associated with the AIG manager (pGia) using 
    pointer (pGia->pManTime). It is automatically deallocated when the host 
    AIG manager is deleted.
*/

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

typedef struct Tim_Man_t_  Tim_Man_t;

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

#define TIM_ETERNITY 1000000000

////////////////////////////////////////////////////////////////////////
///                             ITERATORS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     SEQUENTIAL ITERATORS                         ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== timBox.c ===========================================================*/
extern void            Tim_ManCreateBox( Tim_Man_t * p, int firstIn, int nIns, int firstOut, int nOuts, int iDelayTable, int fBlack );
extern int             Tim_ManBoxForCi( Tim_Man_t * p, int iCo );
extern int             Tim_ManBoxForCo( Tim_Man_t * p, int iCi );
extern int             Tim_ManBoxInputFirst( Tim_Man_t * p, int iBox );
extern int             Tim_ManBoxInputLast( Tim_Man_t * p, int iBox );
extern int             Tim_ManBoxOutputFirst( Tim_Man_t * p, int iBox );
extern int             Tim_ManBoxOutputLast( Tim_Man_t * p, int iBox );
extern int             Tim_ManBoxInputNum( Tim_Man_t * p, int iBox );
extern int             Tim_ManBoxOutputNum( Tim_Man_t * p, int iBox );
extern int             Tim_ManBoxDelayTableId( Tim_Man_t * p, int iBox );
extern float *         Tim_ManBoxDelayTable( Tim_Man_t * p, int iBox );
extern int             Tim_ManBoxIsBlack( Tim_Man_t * p, int iBox );
extern int             Tim_ManBoxCopy( Tim_Man_t * p, int iBox );
extern void            Tim_ManBoxSetCopy( Tim_Man_t * p, int iBox, int iCopy );
extern int             Tim_ManBoxFindFromCiNum( Tim_Man_t * p, int iCiNum );
/*=== timDump.c ===========================================================*/
extern Vec_Str_t *     Tim_ManSave( Tim_Man_t * p, int fHieOnly );
extern Tim_Man_t *     Tim_ManLoad( Vec_Str_t * p, int fHieOnly );
/*=== timMan.c ===========================================================*/
extern Tim_Man_t *     Tim_ManStart( int nCis, int nCos );
extern Tim_Man_t *     Tim_ManDup( Tim_Man_t * p, int fUnitDelay );
extern Tim_Man_t *     Tim_ManTrim( Tim_Man_t * p, Vec_Int_t * vBoxPres );
extern Tim_Man_t *     Tim_ManReduce( Tim_Man_t * p, Vec_Int_t * vBoxesLeft, int nTermsDiff );
extern Vec_Int_t *     Tim_ManAlignTwo( Tim_Man_t * pSpec, Tim_Man_t * pImpl );
extern void            Tim_ManCreate( Tim_Man_t * p, void * pLib, Vec_Flt_t * vInArrs, Vec_Flt_t * vOutReqs );
extern float *         Tim_ManGetArrTimes( Tim_Man_t * p );
extern float *         Tim_ManGetReqTimes( Tim_Man_t * p );
extern void            Tim_ManStop( Tim_Man_t * p );
extern void            Tim_ManStopP( Tim_Man_t ** p );
extern void            Tim_ManPrint( Tim_Man_t * p );
extern void            Tim_ManPrintStats( Tim_Man_t * p, int nAnd2Delay );
extern int             Tim_ManCiNum( Tim_Man_t * p );
extern int             Tim_ManCoNum( Tim_Man_t * p );
extern int             Tim_ManPiNum( Tim_Man_t * p );
extern int             Tim_ManPoNum( Tim_Man_t * p );
extern int             Tim_ManBoxNum( Tim_Man_t * p );
extern int             Tim_ManBlackBoxNum( Tim_Man_t * p );
extern void            Tim_ManBlackBoxIoNum( Tim_Man_t * p, int * pnBbIns, int * pnBbOuts );
extern int             Tim_ManDelayTableNum( Tim_Man_t * p );
extern void            Tim_ManSetDelayTables( Tim_Man_t * p, Vec_Ptr_t * vDelayTables );
extern void            Tim_ManTravIdDisable( Tim_Man_t * p );
extern void            Tim_ManTravIdEnable( Tim_Man_t * p );
/*=== timTime.c ===========================================================*/
extern void            Tim_ManInitPiArrival( Tim_Man_t * p, int iPi, float Delay );
extern void            Tim_ManInitPoRequired( Tim_Man_t * p, int iPo, float Delay );
extern void            Tim_ManInitPiArrivalAll( Tim_Man_t * p, float Delay );
extern void            Tim_ManInitPoRequiredAll( Tim_Man_t * p, float Delay );
extern void            Tim_ManSetCoArrival( Tim_Man_t * p, int iCo, float Delay );
extern void            Tim_ManSetCiRequired( Tim_Man_t * p, int iCi, float Delay );
extern void            Tim_ManSetCoRequired( Tim_Man_t * p, int iCo, float Delay );
extern float           Tim_ManGetCiArrival( Tim_Man_t * p, int iCi );
extern float           Tim_ManGetCoRequired( Tim_Man_t * p, int iCo );
/*=== timTrav.c ===========================================================*/
extern void            Tim_ManIncrementTravId( Tim_Man_t * p );
extern void            Tim_ManSetCurrentTravIdBoxInputs( Tim_Man_t * p, int iBox );
extern void            Tim_ManSetCurrentTravIdBoxOutputs( Tim_Man_t * p, int iBox );
extern void            Tim_ManSetPreviousTravIdBoxInputs( Tim_Man_t * p, int iBox );
extern void            Tim_ManSetPreviousTravIdBoxOutputs( Tim_Man_t * p, int iBox );
extern int             Tim_ManIsCiTravIdCurrent( Tim_Man_t * p, int iCi );
extern int             Tim_ManIsCoTravIdCurrent( Tim_Man_t * p, int iCo );


ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

