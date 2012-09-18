//===========================================================================//
// Purpose : Definition for the 'TCBP_CircuitBlifHandler_c' class.
//
//===========================================================================//

#ifndef TCBP_CIRCUIT_BLIF_HANDLER_H
#define TCBP_CIRCUIT_BLIF_HANDLER_H

#include "TCD_CircuitDesign.h"

#include "TCBP_CircuitBlifInterface.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TCBP_CircuitBlifHandler_c : public TCBP_CircuitBlifInterface_c
{
public:

   TCBP_CircuitBlifHandler_c( TCD_CircuitDesign_c* pcircuitDesign );
   ~TCBP_CircuitBlifHandler_c( void );

   bool Init( TCD_CircuitDesign_c* pcircuitDesign );

   bool IsValid( void ) const;

private:

   TCD_CircuitDesign_c* pcircuitDesign_;
};

#endif
