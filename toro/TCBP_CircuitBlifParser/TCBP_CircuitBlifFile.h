//===========================================================================//
// Purpose : Declaration and inline definitions for a TCBP_CircuitBlifFile 
//           class.
//
//           Inline methods include:
//           - IsValid
//
//===========================================================================//

#ifndef TCBP_CIRCUIT_BLIF_FILE_H
#define TCBP_CIRCUIT_BLIF_FILE_H

#include <stdio.h>

#include "TCD_CircuitDesign.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TCBP_CircuitBlifFile_c
{
public:

   TCBP_CircuitBlifFile_c( FILE* pfile, 
                           const char* pszFileParserName,
                           TCD_CircuitDesign_c* pcircuitDesign );
   ~TCBP_CircuitBlifFile_c( void );

   bool SyntaxError( unsigned int lineNum, 
                     const string& srFileName,
                     const char* pszMessageText );

   bool IsValid( void ) const;

private:

   bool ok_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline bool TCBP_CircuitBlifFile_c::IsValid( void ) const
{
   return( this->ok_ );
}

#endif
