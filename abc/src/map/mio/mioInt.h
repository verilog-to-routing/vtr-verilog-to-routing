/**CFile****************************************************************

  FileName    [mioInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [File reading/writing for technology mapping.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mioInt.h,v 1.4 2004/06/28 14:20:25 alanmi Exp $]

***********************************************************************/

#ifndef ABC__map__mio__mioInt_h
#define ABC__map__mio__mioInt_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "misc/vec/vec.h"
#include "misc/mem/mem.h"
#include "misc/st/st.h"
#include "mio.h"
 
ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

#define    MIO_STRING_GATE       "GATE"
#define    MIO_STRING_LATCH      "LATCH"
#define    MIO_STRING_PIN        "PIN"
#define    MIO_STRING_NONINV     "NONINV"
#define    MIO_STRING_INV        "INV"
#define    MIO_STRING_UNKNOWN    "UNKNOWN"

#define    MIO_STRING_CONST0     "CONST0"
#define    MIO_STRING_CONST1     "CONST1"
 
// the bit masks
#define    MIO_MASK(n)         ((~((unsigned)0)) >> (32-(n)))
#define    MIO_FULL             (~((unsigned)0))

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

struct  Mio_LibraryStruct_t_
{
    char *             pName;       // the name of the library
    int                nGates;      // the number of the gates
    Mio_Gate_t **      ppGates0;    // the array of gates in the original order
    Mio_Gate_t **      ppGatesName; // the array of gates sorted by name
    Mio_Gate_t *       pGates;      // the linked list of all gates in no particular order
    Mio_Gate_t *       pGate0;      // the constant zero gate
    Mio_Gate_t *       pGate1;      // the constant one gate
    Mio_Gate_t *       pGateBuf;    // the buffer
    Mio_Gate_t *       pGateInv;    // the inverter
    Mio_Gate_t *       pGateNand2;  // the NAND2 gate
    Mio_Gate_t *       pGateAnd2;   // the AND2 gate
    Mio_Gate_t *       pGateNor2;   // the NOR2 gate
    Mio_Gate_t *       pGateOr2;    // the OR2 gate
    st__table *         tName2Gate;  // the mapping of gate names into their pointer
    Mem_Flex_t *       pMmFlex;     // the memory manaqer for SOPs
    Vec_Str_t *        vCube;       // temporary cube
    // matching
    int                fPinFilter;  // pin filtering
    int                fPinPerm;    // pin permutation
    int                fPinQuick;   // pin permutation
    Vec_Mem_t *        vTtMem;      // truth tables
    Vec_Wec_t *        vTt2Match;   // matches for truth tables
    Mio_Cell2_t *      pCells;      // library gates
    int                nCells;      // library gate count
    Vec_Ptr_t *        vNames;
    Vec_Wrd_t *        vTruths; 
    Vec_Int_t *        vTt2Match4;
    Vec_Int_t *        vConfigs;
    Vec_Mem_t *        vTtMem2[3]; 
    Vec_Int_t *        vTt2Match2[3];
}; 

struct  Mio_GateStruct_t_
{
    // information derived from the genlib file
    char *             pName;       // the name of the gate
    double             dArea;       // the area of the gate
    char *             pForm;       // the formula describing functionality of the gate
    Mio_Pin_t *        pPins;       // the linked list of all pins (one pin if info is the same)
    char *             pOutName;    // the name of the output pin 
    // the library to which this gate belongs
    Mio_Library_t *    pLib; 
    // the next gate in the list
    Mio_Gate_t *       pNext;    
    Mio_Gate_t *       pTwin;    

    // the derived information
    int                Cell;        // cell id
    int                nInputs;     // the number of inputs
    int                Profile;     // the number of occurrences
    int                Profile2;    // the number of occurrences
    double             dDelayMax;   // the maximum delay
    char *             pSop;        // sum-of-products
    Vec_Int_t *        vExpr;       // boolean expression
    union { word       uTruth;      // truth table
    word *             pTruth; };   // pointer to the truth table
    int                Value;       // user's information
};

struct  Mio_PinStruct_t_
{
    char *             pName;
    Mio_PinPhase_t     Phase;
    double             dLoadInput;
    double             dLoadMax;
    double             dDelayBlockRise;
    double             dDelayFanoutRise;
    double             dDelayBlockFall;
    double             dDelayFanoutFall;
    double             dDelayBlockMax;
    Mio_Pin_t *        pNext;     
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

/*=== mio.c =============================================================*/
/*=== mioRead.c =============================================================*/
/*=== mioUtils.c =============================================================*/


ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
