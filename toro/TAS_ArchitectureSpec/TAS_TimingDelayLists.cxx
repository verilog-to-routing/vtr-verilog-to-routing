//===========================================================================//
// Purpose : Method definitions for the TAS_TimingDelayLists class.
//
//           Public methods include:
//           - TAS_TimingDelayLists_c, ~TAS_TimingDelayLists_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - PrintXML
//
//===========================================================================//

#include "TAS_TimingDelayLists.h"

//===========================================================================//
// Method         : TAS_TimingDelayLists_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TAS_TimingDelayLists_c::TAS_TimingDelayLists_c( 
      void )
      :
      delayList( TAS_DELAY_LIST_DEF_CAPACITY ),
      delayMatrixList( TAS_DELAY_MATRIX_LIST_DEF_CAPACITY ),
      delayClockSetupList ( TAS_DELAY_CLOCK_SETUP_LIST_DEF_CAPACITY ),
      delayClockToQList( TAS_DELAY_CLOCK_TO_Q_LIST_DEF_CAPACITY )
{
}

//===========================================================================//
TAS_TimingDelayLists_c::TAS_TimingDelayLists_c( 
      const TAS_TimingDelayLists_c& timingDelayLists )
      :
      delayList( timingDelayLists.delayList ),
      delayMatrixList( timingDelayLists.delayMatrixList ),
      delayClockSetupList ( timingDelayLists.delayClockSetupList  ),
      delayClockToQList( timingDelayLists.delayClockToQList )
{
}

//===========================================================================//
// Method         : ~TAS_TimingDelayLists_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TAS_TimingDelayLists_c::~TAS_TimingDelayLists_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TAS_TimingDelayLists_c& TAS_TimingDelayLists_c::operator=( 
      const TAS_TimingDelayLists_c& timingDelayLists )
{
   if( &timingDelayLists != this )
   {
      this->delayList = timingDelayLists.delayList;
      this->delayMatrixList = timingDelayLists.delayMatrixList;
      this->delayClockSetupList  = timingDelayLists.delayClockSetupList;
      this->delayClockToQList = timingDelayLists.delayClockToQList;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TAS_TimingDelayLists_c::operator==( 
      const TAS_TimingDelayLists_c& timingDelayLists ) const
{
   return(( this->delayList == timingDelayLists.delayList ) &&
          ( this->delayMatrixList == timingDelayLists.delayMatrixList ) &&
          ( this->delayClockSetupList == timingDelayLists.delayClockSetupList ) &&
          ( this->delayClockToQList == timingDelayLists.delayClockToQList ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TAS_TimingDelayLists_c::operator!=( 
      const TAS_TimingDelayLists_c& timingDelayLists ) const
{
   return( !this->operator==( timingDelayLists ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TAS_TimingDelayLists_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   this->delayList.Print( pfile, spaceLen );
   this->delayMatrixList.Print( pfile, spaceLen );
   this->delayClockSetupList.Print( pfile, spaceLen );
   this->delayClockToQList.Print( pfile, spaceLen );
}

//===========================================================================//
// Method         : PrintXML
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TAS_TimingDelayLists_c::PrintXML( 
      void ) const
{
   FILE* pfile = 0;
   size_t spaceLen = 0;

   this->PrintXML( pfile, spaceLen );
}

//===========================================================================//
void TAS_TimingDelayLists_c::PrintXML( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   for( size_t i = 0; i < this->delayList.GetLength( ); ++i )
   {
      this->delayList[i]->PrintXML( pfile, spaceLen );
   }
   for( size_t i = 0; i < this->delayMatrixList.GetLength( ); ++i )
   {
      this->delayMatrixList[i]->PrintXML( pfile, spaceLen );
   }
   for( size_t i = 0; i < this->delayClockSetupList.GetLength( ); ++i )
   {
      this->delayClockSetupList[i]->PrintXML( pfile, spaceLen );
   }
   for( size_t i = 0; i < this->delayClockToQList.GetLength( ); ++i )
   {
      this->delayClockToQList[i]->PrintXML( pfile, spaceLen );
   }
}
