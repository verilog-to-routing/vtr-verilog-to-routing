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
// Permission is hereby granted, free of charge, to any person obtaining a   //
// copy of this software and associated documentation files (the "Software"),//
// to deal in the Software without restriction, including without limitation //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,  //
// and/or sell copies of the Software, and to permit persons to whom the     //
// Software is furnished to do so, subject to the following conditions:      //
//                                                                           //
// The above copyright notice and this permission notice shall be included   //
// in all copies or substantial portions of the Software.                    //
//                                                                           //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS   //
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF                //
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN // 
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,  //
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR     //
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE //
// USE OR OTHER DEALINGS IN THE SOFTWARE.                                    //
//---------------------------------------------------------------------------//

#include "TC_MinGrid.h"
#include "TCT_Generic.h"

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
   this->power.enable = false;
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
      TOS_PackCostMode_t        costMode_,
      bool                      power_enable_ )
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
   this->power.enable = power_enable_;
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
   if( this->clusterNetsMode != TOS_PACK_CLUSTER_NETS_UNDEFINED )
   {
      printHandler.Write( pfile, spaceLen, "PACK_AAPACK_CLUSTER_NETS   = %s\n", TIO_SR_STR( srClusterNetsMode ));
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// PACK_AAPACK_CLUSTER_NETS\n" );
   }
   if( TCTF_IsGT( this->areaWeight, 0.0 ))
   {
      printHandler.Write( pfile, spaceLen, "PACK_AAPACK_AREA_WEIGHT    = %0.*f\n", precision, this->areaWeight );
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// PACK_AAPACK_AREA_WEIGHT\n" );
   }
   if( TCTF_IsGT( this->netsWeight, 0.0 ))
   {
      printHandler.Write( pfile, spaceLen, "PACK_AAPACK_NETS_WEIGHT    = %0.*f\n", precision, this->netsWeight );
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// PACK_AAPACK_NETS_WEIGHT\n" );
   }
   if( this->affinityMode != TOS_PACK_AFFINITY_UNDEFINED )
   {
      printHandler.Write( pfile, spaceLen, "PACK_AAPACK_AFFINITY       = %s\n", TIO_SR_STR( srAffinityMode ));
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// PACK_AAPACK_AFFINITY\n" );
   }
   // printHandler.Write( pfile, spaceLen, "PACK_BLOCK_SIZE            = %lu\n", this->blockSize );
   // printHandler.Write( pfile, spaceLen, "PACK_LUT_SIZE              = %lu\n", this->lutSize );
   printHandler.Write( pfile, spaceLen, "PACK_COST_MODE             = %s\n", TIO_SR_STR( srCostMode ));

   printHandler.Write( pfile, spaceLen, "\n" );
   printHandler.Write( pfile, spaceLen, "PACK_POWER_ENABLE          = %s\n", TIO_BOOL_STR( this->power.enable ));
}
