//===========================================================================//
// Purpose : Method definitions for the TOS_PlaceOptions class.
//
//           Public methods include:
//           - TOS_PlaceOptions_c, ~TOS_PlaceOptions_c
//           - Print
//
//===========================================================================//

#include "TC_MinGrid.h"

#include "TIO_PrintHandler.h"

#include "TOS_StringUtils.h"
#include "TOS_PlaceOptions.h"

//===========================================================================//
// Method         : TOS_PlaceOptions_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TOS_PlaceOptions_c::TOS_PlaceOptions_c( 
      void )
      :
      algorithmMode( TOS_PLACE_ALGORITHM_UNDEFINED ),
      randomSeed( 0 ),
      initTemp( 0.0 ),
      initTempFactor( 0.0 ),
      initTempEpsilon( 0.0 ),
      exitTemp( 0.0 ),
      exitTempFactor( 0.0 ),
      exitTempEpsilon( 0.0 ),
      reduceTemp( 0.0 ),
      reduceTempFactor( 0.0 ),
      innerNum( 0.0 ),
      searchLimit( 0.0 ),
      costMode( TOS_PLACE_COST_UNDEFINED ),
      fixedDelay( 0.0 ),
      timingCostFactor( 0.0 ),
      timingUpdateInt( 0 ),
      timingUpdateCount( 0 ),
      slackInitWeight( 0.0 ),
      slackFinalWeight( 0.0 )
{
}

//===========================================================================//
TOS_PlaceOptions_c::TOS_PlaceOptions_c( 
      TOS_PlaceAlgorithmMode_t algorithmMode_,
      unsigned int             randomSeed_,
      double                   initTemp_,
      double                   initTempFactor_,
      double                   initTempEpsilon_,
      double                   exitTemp_,
      double                   exitTempFactor_,
      double                   exitTempEpsilon_,
      double                   reduceTemp_,
      double                   reduceTempFactor_,
      double                   innerNum_,
      double                   searchLimit_,
      TOS_PlaceCostMode_t      costMode_,
      double                   timingCostFactor_,
      unsigned int             timingUpdateInt_,
      unsigned int             timingUpdateCount_,
      double                   slackInitWeight_,
      double                   slackFinalWeight_ )
      :
      algorithmMode( algorithmMode_ ),
      randomSeed( randomSeed_ ),
      initTemp( initTemp_ ),
      initTempFactor( initTempFactor_ ),
      initTempEpsilon( initTempEpsilon_ ),
      exitTemp( exitTemp_ ),
      exitTempFactor( exitTempFactor_ ),
      exitTempEpsilon( exitTempEpsilon_ ),
      reduceTemp( reduceTemp_ ),
      reduceTempFactor( reduceTempFactor_ ),
      innerNum( innerNum_ ),
      searchLimit( searchLimit_ ),
      costMode( costMode_ ),
      fixedDelay( 0.0 ),
      timingCostFactor( timingCostFactor_ ),
      timingUpdateInt( timingUpdateInt_ ),
      timingUpdateCount( timingUpdateCount_ ),
      slackInitWeight( slackInitWeight_ ),
      slackFinalWeight( slackFinalWeight_ )
{
}

//===========================================================================//
// Method         : ~TOS_PlaceOptions_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TOS_PlaceOptions_c::~TOS_PlaceOptions_c( 
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
void TOS_PlaceOptions_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   string srAlgorithmMode, srCostMode;
   TOS_ExtractStringPlaceAlgorithmMode( this->algorithmMode, &srAlgorithmMode );
   TOS_ExtractStringPlaceCostMode( this->costMode, &srCostMode );

   printHandler.Write( pfile, spaceLen, "PLACE_ALGORITHM            = %s\n", TIO_SR_STR( srAlgorithmMode ));

   printHandler.Write( pfile, spaceLen, "PLACE_RANDOM_SEED          = %u\n", this->randomSeed );
   printHandler.Write( pfile, spaceLen, "PLACE_TEMP_INIT            = %0.*f\n", precision, this->initTemp );
   printHandler.Write( pfile, spaceLen, "PLACE_TEMP_INIT_FACTOR     = %0.*f\n", precision, this->initTempFactor );
   printHandler.Write( pfile, spaceLen, "PLACE_TEMP_INIT_EPSILON    = %0.*f\n", precision, this->initTempEpsilon );
   printHandler.Write( pfile, spaceLen, "PLACE_TEMP_EXIT            = %0.*f\n", precision, this->exitTemp );
   printHandler.Write( pfile, spaceLen, "PLACE_TEMP_EXIT_FACTOR     = %0.*f\n", precision, this->exitTempFactor );
   printHandler.Write( pfile, spaceLen, "PLACE_TEMP_EXIT_EPSILON    = %0.*f\n", precision, this->exitTempEpsilon );
   printHandler.Write( pfile, spaceLen, "PLACE_TEMP_REDUCE          = %0.*f\n", precision, this->reduceTemp );
   printHandler.Write( pfile, spaceLen, "PLACE_TEMP_REDUCE_FACTOR   = %0.*f\n", precision, this->reduceTempFactor );
   printHandler.Write( pfile, spaceLen, "PLACE_TEMP_INNER_NUM       = %0.*f\n", precision, this->innerNum );
   printHandler.Write( pfile, spaceLen, "PLACE_SEARCH_LIMIT         = %0.*f\n", precision, this->searchLimit );

   printHandler.Write( pfile, spaceLen, "PLACE_COST_MODE            = %s\n", TIO_SR_STR( srCostMode ));

   printHandler.Write( pfile, spaceLen, "PLACE_TIMING_COST_FACTOR   = %0.*f\n", precision, this->timingCostFactor );
   printHandler.Write( pfile, spaceLen, "PLACE_TIMING_UPDATE_INT    = %u\n", this->timingUpdateInt );
   printHandler.Write( pfile, spaceLen, "PLACE_TIMING_UPDATE_COUNT  = %u\n", this->timingUpdateCount );
   printHandler.Write( pfile, spaceLen, "PLACE_SLACK_INIT_WEIGHT    = %0.*f\n", precision, this->slackInitWeight );
   printHandler.Write( pfile, spaceLen, "PLACE_SLACK_FINAL_WEIGHT   = %0.*f\n", precision, this->slackFinalWeight );
}
