//===========================================================================//
// Purpose : Declaration and inline definitions for a TFM_Config class.
//
//           Inline methods include:
//           - IsValid
//
//===========================================================================//

#ifndef TFM_CONFIG_H
#define TFM_CONFIG_H

#include <stdio.h>

#include "TGS_Region.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
class TFM_Config_c
{
public:

   TFM_Config_c( void );
   TFM_Config_c( const TFM_Config_c& config );
   ~TFM_Config_c( void );

   TFM_Config_c& operator=( const TFM_Config_c& config );
   bool operator==( const TFM_Config_c& config ) const;
   bool operator!=( const TFM_Config_c& config ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   bool IsValid( void ) const;

public:

   TGS_Region_c fabricRegion;   // Fabric floorplan region boundary
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
inline bool TFM_Config_c::IsValid( 
      void ) const
{
   return( true );
}

#endif
