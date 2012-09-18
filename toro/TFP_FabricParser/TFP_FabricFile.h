//===========================================================================//
// Purpose : Declaration and inline definitions for a TFP_FabricFile class.
//
//           Inline methods include:
//           - IsValid
//
//===========================================================================//

#ifndef TFP_FABRIC_FILE_H
#define TFP_FABRIC_FILE_H

#include <stdio.h>

#include "TFM_FabricModel.h"

#include "TFP_FabricInterface.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
class TFP_FabricFile_c
{
public:

   TFP_FabricFile_c( FILE* pfile, 
                     const char* pszFileParserName,
                     TFP_FabricInterface_c* pfabricInterface,
                     TFM_FabricModel_c* pfabricModel );
   ~TFP_FabricFile_c( void );

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
inline bool TFP_FabricFile_c::IsValid( void ) const
{
   return( this->ok_ );
}

#endif
