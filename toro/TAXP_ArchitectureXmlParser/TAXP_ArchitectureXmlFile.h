//===========================================================================//
// Purpose : Declaration and inline definitions for TAXP_ArchitectureXmlFile 
//           class.
//
//           Inline methods include:
//           - IsValid
//
//===========================================================================//

#ifndef TAXP_ARCHITECTURE_XML_FILE_H
#define TAXP_ARCHITECTURE_XML_FILE_H

#include <stdio.h>

#include "TAS_ArchitectureSpec.h"

#include "TAXP_ArchitectureXmlInterface.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TAXP_ArchitectureXmlFile_c
{
public:

   TAXP_ArchitectureXmlFile_c( FILE* pfile, 
                               const char* pszFileParserName,
                               TAXP_ArchitectureXmlInterface_c* parchitectureXmlInterface,
                               TAS_ArchitectureSpec_c* parchitectureSpec );
   ~TAXP_ArchitectureXmlFile_c( void );

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
inline bool TAXP_ArchitectureXmlFile_c::IsValid( void ) const
{
   return( this->ok_ );
}

#endif
