#ifndef ABC__aig__gia__giaCSatP_h
#define ABC__aig__gia__giaCSatP_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "gia.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_HEADER_START


typedef struct CbsP_Par_t_ CbsP_Par_t;
struct CbsP_Par_t_
{
    // conflict limits
    int           nBTLimit;     // limit on the number of conflicts
    int           nJustLimit;   // limit on the size of justification queue
    // current parameters
    int           nBTThis;      // number of conflicts
    int           nBTThisNc;    // number of conflicts
    int           nJustThis;    // max size of the frontier
    int           nBTTotal;     // total number of conflicts
    int           nJustTotal;   // total size of the frontier
    // decision heuristics
    int           fUseHighest;  // use node with the highest ID
    int           fUseLowest;   // use node with the highest ID
    int           fUseMaxFF;    // use node with the largest fanin fanout
    // other
    int           fVerbose;
    int           fUseProved;

    // statistics 
    int           nJscanThis;
    int           nRscanThis;
    int           nPropThis;
    int           maxJscanUndec;
    int           maxRscanUndec;
    int           maxPropUndec;
    int           maxJscanSolved;
    int           maxRscanSolved;
    int           maxPropSolved;
    int nSat, nUnsat, nUndec;
    long          accJscanSat;
    long          accJscanUnsat;
    long          accJscanUndec;
    long          accRscanSat;
    long          accRscanUnsat;
    long          accRscanUndec;
    long          accPropSat;
    long          accPropUnsat;
    long          accPropUndec;

    // other limits 
    int           nJscanLimit;
    int           nRscanLimit;
    int           nPropLimit;
};

typedef struct CbsP_Que_t_ CbsP_Que_t;
struct CbsP_Que_t_
{
    int           iHead;        // beginning of the queue
    int           iTail;        // end of the queue
    int           nSize;        // allocated size
    Gia_Obj_t **  pData;        // nodes stored in the queue
};
 
typedef struct CbsP_Man_t_ CbsP_Man_t;
struct CbsP_Man_t_
{
    CbsP_Par_t     Pars;         // parameters
    Gia_Man_t *   pAig;         // AIG manager
    CbsP_Que_t     pProp;        // propagation queue
    CbsP_Que_t     pJust;        // justification queue
    CbsP_Que_t     pClauses;     // clause queue
    Gia_Obj_t **  pIter;        // iterator through clause vars
    Vec_Int_t *   vLevReas;     // levels and decisions
    Vec_Int_t *   vValue;
    Vec_Int_t *   vModel;       // satisfying assignment
    Vec_Ptr_t *   vTemp;        // temporary storage
    // SAT calls statistics
    int           nSatUnsat;    // the number of proofs
    int           nSatSat;      // the number of failure
    int           nSatUndec;    // the number of timeouts
    int           nSatTotal;    // the number of calls
    // conflicts
    int           nConfUnsat;   // conflicts in unsat problems
    int           nConfSat;     // conflicts in sat problems
    int           nConfUndec;   // conflicts in undec problems
    // runtime stats
    abctime       timeSatUnsat; // unsat
    abctime       timeSatSat;   // sat
    abctime       timeSatUndec; // undecided
    abctime       timeTotal;    // total runtime
};

CbsP_Man_t * CbsP_ManAlloc( Gia_Man_t * pGia );
void CbsP_ManStop( CbsP_Man_t * p );
void CbsP_ManSatPrintStats( CbsP_Man_t * p );
void CbsP_PrintRecord( CbsP_Par_t * pPars );
int CbsP_ManSolve2( CbsP_Man_t * p, Gia_Obj_t * pObj, Gia_Obj_t * pObj2 );

#define CBS_UNSAT  1
#define CBS_SAT    0
#define CBS_UNDEC -1

ABC_NAMESPACE_HEADER_END


#endif
