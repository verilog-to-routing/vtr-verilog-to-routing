//===========================================================================//
// Purpose : Declaration and inline definitions for the 
//           TAXP_ArchitectureXmlInterface abstract base class.
//
//           Inline methods include:
//           - TAXP_ArchitectureXmlInterface, ~TAXP_ArchitectureXmlInterface
//
//===========================================================================//

#ifndef TAXP_ARCHITECTURE_XML_INTERFACE_H
#define TAXP_ARCHITECTURE_XML_INTERFACE_H

#include <string>
using namespace std;

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TAXP_ArchitectureXmlInterface_c
{
public:

   TAXP_ArchitectureXmlInterface_c( void );
   virtual ~TAXP_ArchitectureXmlInterface_c( void );

   virtual bool SyntaxError( unsigned int lineNum, 
                             const string& srFileName,
			     const char* pszMessageText ) = 0;

   virtual bool IsValid( void ) const = 0;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline TAXP_ArchitectureXmlInterface_c::TAXP_ArchitectureXmlInterface_c( 
      void )
{
}

//===========================================================================//
inline TAXP_ArchitectureXmlInterface_c::~TAXP_ArchitectureXmlInterface_c( 
      void )
{
}

#endif
