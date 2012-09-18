//===========================================================================//
// Purpose : Definition for the 'TCP_CircuitHandler_c' class.
//
//===========================================================================//

#ifndef TCP_CIRCUIT_HANDLER_H
#define TCP_CIRCUIT_HANDLER_H

#include "TCD_CircuitDesign.h"

#include "TCP_CircuitInterface.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
class TCP_CircuitHandler_c : public TCP_CircuitInterface_c
{
public:

   TCP_CircuitHandler_c( TCD_CircuitDesign_c* pcircuitDesign );
   ~TCP_CircuitHandler_c( void );

   bool Init( TCD_CircuitDesign_c* pcircuitDesign );

   bool SyntaxError( unsigned int lineNum, 
                     const string& srFileName,
                     const char* pszMessageText );

   bool IsValid( void ) const;

private:

   TCD_CircuitDesign_c* pcircuitDesign_;
};

#endif
