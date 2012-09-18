//===========================================================================//
// Purpose : Method definitions for the TNO_Route class.
//
//           Public methods include:
//           - TNO_Route_c, ~TNO_Route_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - Set
//           - Clear
//
//===========================================================================//

#include "TIO_PrintHandler.h"

#include "TNO_StringUtils.h"
#include "TNO_Route.h"

//===========================================================================//
// Method         : TNO_Route_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
TNO_Route_c::TNO_Route_c( 
      void )
      :
      type_( TNO_ROUTE_UNDEFINED )
{
} 

//===========================================================================//
TNO_Route_c::TNO_Route_c( 
      const TNO_InstPin_c& instPin )
      :
      type_( TNO_ROUTE_INST_PIN ),
      instPin_( instPin )
{
} 

//===========================================================================//
TNO_Route_c::TNO_Route_c( 
      const TNO_SwitchBox_c& switchBox )
      :
      type_( TNO_ROUTE_SWITCH_BOX ),
      switchBox_( switchBox )
{
} 

//===========================================================================//
TNO_Route_c::TNO_Route_c( 
      const TNO_Channel_t& channel )
      :
      type_( TNO_ROUTE_CHANNEL ),
      channel_( channel )
{
} 

//===========================================================================//
TNO_Route_c::TNO_Route_c( 
      const TNO_Route_c& route )
      :
      type_( route.type_ ),
      instPin_( route.instPin_ ),
      switchBox_( route.switchBox_ ),
      channel_( route.channel_ )
{
} 

//===========================================================================//
// Method         : ~TNO_Route_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
TNO_Route_c::~TNO_Route_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
TNO_Route_c& TNO_Route_c::operator=( 
      const TNO_Route_c& route )
{
   if( &route != this )
   {
      this->type_ = route.type_;
      this->instPin_ = route.instPin_;
      this->switchBox_ = route.switchBox_;
      this->channel_ = route.channel_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
bool TNO_Route_c::operator==( 
      const TNO_Route_c& route ) const
{
   return(( this->type_ == route.type_ ) &&
          ( this->instPin_ == route.instPin_ ) &&
          ( this->switchBox_ == route.switchBox_ ) &&
          ( this->channel_ == route.channel_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
bool TNO_Route_c::operator!=( 
      const TNO_Route_c& route ) const
{
   return( !this->operator==( route ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
void TNO_Route_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   string srType;
   TNO_ExtractStringRouteType( this->type_, &srType );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Write( pfile, spaceLen, "type = %s\n", TIO_SR_STR( srType ));

   switch( this->type_ )
   {
   case TNO_ROUTE_INST_PIN: 
      this->instPin_.Print( pfile, spaceLen );
      break;
   case TNO_ROUTE_SWITCH_BOX:
      this->switchBox_.Print( pfile, spaceLen );
      break;
   case TNO_ROUTE_CHANNEL: 
      this->channel_.Print( pfile, spaceLen );
      break;
   default:
      break;
   }
}

//===========================================================================//
// Method         : Set
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
void TNO_Route_c::Set( 
      const TNO_InstPin_c& instPin )
{
   this->Clear( );

   this->type_ = TNO_ROUTE_INST_PIN;
   this->instPin_ = instPin;
} 

//===========================================================================//
void TNO_Route_c::Set( 
      const TNO_SwitchBox_c& switchBox )
{
   this->Clear( );

   this->type_ = TNO_ROUTE_SWITCH_BOX;
   this->switchBox_ = switchBox;
} 

//===========================================================================//
void TNO_Route_c::Set( 
      const TNO_Channel_t& channel )
{
   this->Clear( );

   this->type_ = TNO_ROUTE_CHANNEL;
   this->channel_ = channel;
} 

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
void TNO_Route_c::Clear( 
      void )
{
   this->type_ = TNO_ROUTE_UNDEFINED;

   this->instPin_.Clear( );
   this->switchBox_.Clear( );
   this->channel_.Clear( );
} 
