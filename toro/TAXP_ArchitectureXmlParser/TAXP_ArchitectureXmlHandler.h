//===========================================================================//
// Purpose : Definition for the 'TAXP_ArchitectureXmlHandler_c' class.
//
//===========================================================================//

#ifndef TAXP_ARCHITECTURE_XML_HANDLER_H
#define TAXP_ARCHITECTURE_XML_HANDLER_H

#include "TAS_ArchitectureSpec.h"

#include "TAXP_ArchitectureXmlInterface.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TAXP_ArchitectureXmlHandler_c : public TAXP_ArchitectureXmlInterface_c
{
public:

   TAXP_ArchitectureXmlHandler_c( TAS_ArchitectureSpec_c* parchitectureSpec );
   ~TAXP_ArchitectureXmlHandler_c( void );

   bool Init( TAS_ArchitectureSpec_c* parchitectureSpec );

   bool SyntaxError( unsigned int lineNum, 
                     const string& srFileName,
                     const char* pszMessageText );

   bool IsValid( void ) const;

private:

   TAS_ArchitectureSpec_c* parchitectureSpec_;
};

#endif
