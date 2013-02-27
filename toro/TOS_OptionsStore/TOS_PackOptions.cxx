//===========================================================================//
// Purpose : Method definitions for the TOS_PackOptions class.
//
//           Public methods include:
//           - TOS_PackOptions_c, ~TOS_PackOptions_c
//           - Print
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012-2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com) //
//                                                                           //
// This program is free software; you can redistribute it and/or modify it   //
// under the terms of the GNU General Public License as published by the     //
// Free Software Foundation; version 3 of the License, or any later version. //
//                                                                           //
// This program is distributed in the hope that it will be useful, but       //
// WITHOUT ANY WARRANTY; without even an implied warranty of MERCHANTABILITY //
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License   //
// for more details.                                                         //
//                                                                           //
// You should have received a copy of the GNU General Public License along   //
// with this program; if not, see <http://www.gnu.org/licenses>.             //
//---------------------------------------------------------------------------//

#include "TC_MinGrid.h"

#include "TIO_PrintHandler.h"

#include "TOS_StringUtils.h"
#include "TOS_PackOptions.h"

//===========================================================================//
// Method         : TOS_PackOptions_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TOS_PackOptions_c::TOS_PackOptions_c( 
      void )
      :
      algorithmMode( TOS_PACK_ALGORITHM_UNDEFINED ),
      clusterNetsMode( TOS_PACK_CLUSTER_NETS_UNDEFINED ),
      areaWeight( 0.0 ),
      netsWeight( 0.0 ), 
      affinityMode( TOS_PACK_AFFINITY_UNDEFINED ),
      blockSize( 0 ),
      lutSize( 0 ),
      costMode( TOS_PACK_COST_UNDEFINED ),
      fixedDelay( 0.0 )
{
}

//===========================================================================//
TOS_PackOptions_c::TOS_PackOptions_c( 
      TOS_PackAlgorithmMode_t   algorithmMode_,
      TOS_PackClusterNetsMode_t clusterNetsMode_,
      double                    areaWeight_,
      double                    netsWeight_,
      TOS_PackAffinityMode_t    affinityMode_,
      unsigned int              blockSize_,
      unsigned int              lutSize_,
      TOS_PackCostMode_t        costMode_ )
      :
      algorithmMode( algorithmMode_ ),
      clusterNetsMode( clusterNetsMode_ ),
      areaWeight( areaWeight_ ),
      netsWeight( netsWeight_ ),
      affinityMode( affinityMode_ ),
      blockSize( blockSize_ ),
      lutSize( lutSize_ ),
      costMode( costMode_ ),
      fixedDelay( 0.0 )
{
}

//===========================================================================//
// Method         : ~TOS_PackOptions_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TOS_PackOptions_c::~TOS_PackOptions_c( 
      void )
{
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_PackOptions_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   string srAlgorithmMode, srClusterNetsMode, srAffinityMode, srCostMode;
   TOS_ExtractStringPackAlgorithmMode( this->algorithmMode, &srAlgorithmMode );
   TOS_ExtractStringPackClusterNetsMode( this->clusterNetsMode, &srClusterNetsMode );
   TOS_ExtractStringPackAffinityMode( this->affinityMode, &srAffinityMode );
   TOS_ExtractStringPackCostMode( this->costMode, &srCostMode );

   printHandler.Write( pfile, spaceLen, "PACK_ALGORITHM             = %s\n", TIO_SR_STR( srAlgorithmMode ));
   printHandler.Write( pfile, spaceLen, "PACK_AAPACK_CLUSTER_NETS   = %s\n", TIO_SR_STR( srClusterNetsMode ));
   printHandler.Write( pfile, spaceLen, "PACK_AAPACK_AREA_WEIGHT    = %0.*f\n", precision, this->areaWeight );
   printHandler.Write( pfile, spaceLen, "PACK_AAPACK_NETS_WEIGHT    = %0.*f\n", precision, this->netsWeight );
   printHandler.Write( pfile, spaceLen, "PACK_AAPACK_AFFINITY       = %s\n", TIO_SR_STR( srAffinityMode ));
   printHandler.Write( pfile, spaceLen, "PACK_BLOCK_SIZE            = %lu\n", this->blockSize );
   printHandler.Write( pfile, spaceLen, "PACK_LUT_SIZE              = %lu\n", this->lutSize );
   printHandler.Write( pfile, spaceLen, "PACK_COST_MODE             = %s\n", TIO_SR_STR( srCostMode ));
}
