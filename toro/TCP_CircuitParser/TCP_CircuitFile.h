//===========================================================================//
// Purpose : Declaration and inline definitions for a TCP_CircuitFile 
//           class.
//
//           Inline methods include:
//           - IsValid
//
//===========================================================================//

#ifndef TCP_CIRCUIT_FILE_H
#define TCP_CIRCUIT_FILE_H

#include <cstdio>
using namespace std;

#include "TCD_CircuitDesign.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
class TCP_CircuitFile_c
{
public:

   TCP_CircuitFile_c( FILE* pfile, 
                      const char* pszFileParserName,
                      TCD_CircuitDesign_c* pcircuitDesign );
   ~TCP_CircuitFile_c( void );

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
// 06/15/12 jeffr : Original
//===========================================================================//
inline bool TCP_CircuitFile_c::IsValid( void ) const
{
   return( this->ok_ );
}

#endif
