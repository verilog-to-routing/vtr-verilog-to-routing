//===========================================================================//
// Purpose : Declaration and inline definitions for a TFS_FloorplanStore 
//           class.
//
//           Inline methods include:
//           - GetArchitectureSpec
//           - GetFabricModel
//           - GetCircuitDesign
//
//===========================================================================//

#ifndef TFS_FLOORPLAN_STORE_H
#define TFS_FLOORPLAN_STORE_H

#include <cstdio>
using namespace std;

#include "TAS_ArchitectureSpec.h"

#include "TFM_FabricModel.h"

#include "TCD_CircuitDesign.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
class TFS_FloorplanStore_c
{
public:

   TFS_FloorplanStore_c( void );
   TFS_FloorplanStore_c( const TFS_FloorplanStore_c& floorplanStore );
   ~TFS_FloorplanStore_c( void );

   TFS_FloorplanStore_c& operator=( const TFS_FloorplanStore_c& floorplanStore );
   bool operator==( const TFS_FloorplanStore_c& floorplanStore ) const;
   bool operator!=( const TFS_FloorplanStore_c& floorplanStore ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   const TAS_ArchitectureSpec_c& GetArchitectureSpec( void ) const;
   const TFM_FabricModel_c& GetFabricModel( void ) const;
   const TCD_CircuitDesign_c& GetCircuitDesign( void ) const;

   bool IsValid( void ) const;

public:

   TAS_ArchitectureSpec_c architectureSpec;
   TFM_FabricModel_c      fabricModel;
   TCD_CircuitDesign_c    circuitDesign;
};


//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
inline const TAS_ArchitectureSpec_c& TFS_FloorplanStore_c::GetArchitectureSpec(
      void ) const
{
   return( this->architectureSpec );
}

//===========================================================================//
inline const TFM_FabricModel_c& TFS_FloorplanStore_c::GetFabricModel(
      void ) const
{
   return( this->fabricModel );
}

//===========================================================================//
inline const TCD_CircuitDesign_c& TFS_FloorplanStore_c::GetCircuitDesign(
      void ) const
{
   return( this->circuitDesign );
}

#endif
