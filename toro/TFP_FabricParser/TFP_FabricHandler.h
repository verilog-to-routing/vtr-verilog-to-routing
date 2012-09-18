//===========================================================================//
// Purpose : Definition for the 'TFP_FabricHandler_c' class.
//
//===========================================================================//

#ifndef TFP_FABRIC_HANDLER_H
#define TFP_FABRIC_HANDLER_H

#include "TFM_FabricModel.h"

#include "TFP_FabricInterface.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
class TFP_FabricHandler_c : public TFP_FabricInterface_c
{
public:

   TFP_FabricHandler_c( TFM_FabricModel_c* pfabricModel );
   ~TFP_FabricHandler_c( void );

   bool Init( TFM_FabricModel_c* pfabricModel );

   bool SyntaxError( unsigned int lineNum, 
                     const string& srFileName,
                     const char* pszMessageText );

   bool IsValid( void ) const;

private:

   TFM_FabricModel_c* pfabricModel_;
};

#endif
