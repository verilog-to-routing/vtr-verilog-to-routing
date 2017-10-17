/**CFile****************************************************************

  FileName    [csat_apis.h]

  PackageName [Interface to CSAT.]

  Synopsis    [APIs, enums, and data structures expected from the use of CSAT.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 28, 2005]

  Revision    [$Id: csat_apis.h,v 1.5 2005/12/30 10:54:40 rmukherj Exp $]

***********************************************************************/

#ifndef ABC__sat__csat__csat_apis_h
#define ABC__sat__csat__csat_apis_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////


typedef struct ABC_ManagerStruct_t     ABC_Manager_t;
typedef struct ABC_ManagerStruct_t *   ABC_Manager;


// GateType defines the gate type that can be added to circuit by
// ABC_AddGate();
#ifndef _ABC_GATE_TYPE_
#define _ABC_GATE_TYPE_
enum GateType
{
    CSAT_CONST = 0,  // constant gate
    CSAT_BPI,    // boolean PI
    CSAT_BPPI,   // bit level PSEUDO PRIMARY INPUT
    CSAT_BAND,   // bit level AND
    CSAT_BNAND,  // bit level NAND
    CSAT_BOR,    // bit level OR
    CSAT_BNOR,   // bit level NOR
    CSAT_BXOR,   // bit level XOR
    CSAT_BXNOR,  // bit level XNOR
    CSAT_BINV,   // bit level INVERTER
    CSAT_BBUF,   // bit level BUFFER
    CSAT_BMUX,   // bit level MUX --not supported 
    CSAT_BDFF,   // bit level D-type FF
    CSAT_BSDFF,  // bit level scan FF --not supported 
    CSAT_BTRIH,  // bit level TRISTATE gate with active high control --not supported 
    CSAT_BTRIL,  // bit level TRISTATE gate with active low control --not supported 
    CSAT_BBUS,   // bit level BUS --not supported 
    CSAT_BPPO,   // bit level PSEUDO PRIMARY OUTPUT
    CSAT_BPO,    // boolean PO
    CSAT_BCNF,   // boolean constraint
    CSAT_BDC,    // boolean don't care gate (2 input)
};
#endif


//CSAT_StatusT defines the return value by ABC_Solve();
#ifndef _ABC_STATUS_
#define _ABC_STATUS_
enum CSAT_StatusT 
{
    UNDETERMINED = 0,
    UNSATISFIABLE,
    SATISFIABLE,
    TIME_OUT,
    FRAME_OUT,
    NO_TARGET,
    ABORTED,
    SEQ_SATISFIABLE     
};
#endif


// to identify who called the CSAT solver
#ifndef _ABC_CALLER_
#define _ABC_CALLER_
enum CSAT_CallerT
{
    BLS = 0,
    SATORI,
    NONE
};
#endif


// CSAT_OptionT defines the solver option about learning
// which is used by ABC_SetSolveOption();
#ifndef _ABC_OPTION_
#define _ABC_OPTION_
enum CSAT_OptionT
{
    BASE_LINE = 0,
    IMPLICT_LEARNING, //default
    EXPLICT_LEARNING
};
#endif


#ifndef _ABC_Target_Result
#define _ABC_Target_Result
typedef struct _CSAT_Target_ResultT CSAT_Target_ResultT;
struct _CSAT_Target_ResultT
{
    enum CSAT_StatusT status; // solve status of the target
    int num_dec;              // num of decisions to solve the target
    int num_imp;              // num of implications to solve the target
    int num_cftg;             // num of conflict gates learned 
    int num_cfts;             // num of conflict signals in conflict gates
    double time;              // time(in second) used to solve the target
    int no_sig;               // if "status" is SATISFIABLE, "no_sig" is the number of 
                              // primary inputs, if the "status" is TIME_OUT, "no_sig" is the
                              // number of constant signals found.
    char** names;             // if the "status" is SATISFIABLE, "names" is the name array of
                              // primary inputs, "values" is the value array of primary 
                              // inputs that satisfy the target. 
                              // if the "status" is TIME_OUT, "names" is the name array of
                              // constant signals found (signals at the root of decision 
                              // tree), "values" is the value array of constant signals found.
    int* values;
};
#endif

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

// create a new manager
extern ABC_Manager          ABC_InitManager(void);

// release a manager
extern void ABC_ReleaseManager(ABC_Manager mng);

// set solver options for learning
extern void                  ABC_SetSolveOption(ABC_Manager mng, enum CSAT_OptionT option);

// enable checking by brute-force SAT solver (MiniSat-1.14)
extern void                  ABC_UseOnlyCoreSatSolver(ABC_Manager mng);


// add a gate to the circuit
// the meaning of the parameters are:
// type: the type of the gate to be added
// name: the name of the gate to be added, name should be unique in a circuit.
// nofi: number of fanins of the gate to be added;
// fanins: the name array of fanins of the gate to be added
extern int                   ABC_AddGate(ABC_Manager mng,
					 enum GateType type,
					 char* name,
					 int nofi,
					 char** fanins,
					 int dc_attr);

// check if there are gates that are not used by any primary ouput.
// if no such gates exist, return 1 else return 0;
extern int                   ABC_Check_Integrity(ABC_Manager mng);

// THIS PROCEDURE SHOULD BE CALLED AFTER THE NETWORK IS CONSTRUCTED!!!
extern void                  ABC_Network_Finalize( ABC_Manager mng );

// set time limit for solving a target.
// runtime: time limit (in second).
extern void                  ABC_SetTimeLimit(ABC_Manager mng, int runtime);
extern void                  ABC_SetLearnLimit(ABC_Manager mng, int num);
extern void                  ABC_SetSolveBacktrackLimit(ABC_Manager mng, int num);
extern void                  ABC_SetLearnBacktrackLimit(ABC_Manager mng, int num);
extern void                  ABC_EnableDump(ABC_Manager mng, char* dump_file);

extern void                  ABC_SetTotalBacktrackLimit( ABC_Manager mng, ABC_UINT64_T num );
extern void                  ABC_SetTotalInspectLimit( ABC_Manager mng, ABC_UINT64_T num );
extern ABC_UINT64_T         ABC_GetTotalBacktracksMade( ABC_Manager mng );
extern ABC_UINT64_T         ABC_GetTotalInspectsMade( ABC_Manager mng );

// the meaning of the parameters are:
// nog: number of gates that are in the targets
// names: name array of gates
// values: value array of the corresponding gates given in "names" to be
// solved. the relation of them is AND.
extern int                   ABC_AddTarget(ABC_Manager mng, int nog, char**names, int* values);

// initialize the solver internal data structure.
extern void                  ABC_SolveInit(ABC_Manager mng);
extern void                  ABC_AnalyzeTargets(ABC_Manager mng);

// solve the targets added by ABC_AddTarget()
extern enum CSAT_StatusT     ABC_Solve(ABC_Manager mng);

// get the solve status of a target
// TargetID: the target id returned by  ABC_AddTarget().
extern CSAT_Target_ResultT * ABC_Get_Target_Result(ABC_Manager mng, int TargetID);
extern void                  ABC_Dump_Bench_File(ABC_Manager mng);

// ADDED PROCEDURES:
extern void                  ABC_TargetResFree( CSAT_Target_ResultT * p );

extern void CSAT_SetCaller(ABC_Manager mng, enum CSAT_CallerT caller);



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
