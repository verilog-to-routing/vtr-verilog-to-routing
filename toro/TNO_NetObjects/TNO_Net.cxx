//===========================================================================//
// Purpose : Method definitions for the TNO_Net class.
//
//           Public methods include:
//           - TNO_Net_c, ~TNO_Net_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
//           - AddInstPin, AddGlobalRoute, AddRoute
//           - FindInstPinCount
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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

#include "TIO_PrintHandler.h"

#include "TC_StringUtils.h"

#include "TCT_Generic.h"

#include "TNO_StringUtils.h"
#include "TNO_Net.h"

//===========================================================================//
// Method         : TNO_Net_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
TNO_Net_c::TNO_Net_c( 
      void )
      :
      type_( TC_TYPE_UNDEFINED ),
      routable_( true ),
      status_( TNO_STATUS_UNDEFINED ),
      index_( 0 ),
      instPinList_( TNO_INST_PIN_LIST_DEF_CAPACITY ),
      globalRouteList_( TNO_GLOBAL_ROUTE_LIST_DEF_CAPACITY ),
      routeList_( TNO_ROUTE_LIST_DEF_CAPACITY )
{
}

//===========================================================================//
TNO_Net_c::TNO_Net_c( 
      const string&          srName, 
            TC_TypeMode_t    type,
            bool             routable,
            TNO_StatusMode_t status )
      :
      srName_( srName ),
      type_( type ),
      routable_( routable ),
      status_( status ),
      index_( 0 ),
      instPinList_( TNO_INST_PIN_LIST_DEF_CAPACITY ),
      globalRouteList_( TNO_GLOBAL_ROUTE_LIST_DEF_CAPACITY ),
      routeList_( TNO_ROUTE_LIST_DEF_CAPACITY )
{
}

//===========================================================================//
TNO_Net_c::TNO_Net_c( 
      const char*            pszName, 
            TC_TypeMode_t    type,
            bool             routable,
            TNO_StatusMode_t status )
      :
      srName_( TIO_PSZ_STR( pszName )),
      type_( type ),
      routable_( routable ),
      status_( status ),
      index_( 0 ),
      instPinList_( TNO_INST_PIN_LIST_DEF_CAPACITY ),
      globalRouteList_( TNO_GLOBAL_ROUTE_LIST_DEF_CAPACITY ),
      routeList_( TNO_ROUTE_LIST_DEF_CAPACITY )
{
}

//===========================================================================//
TNO_Net_c::TNO_Net_c( 
      const TNO_Net_c& net )
{
   this->srName_ = net.srName_;
   this->type_ = net.type_;
   this->routable_ = net.routable_;
   this->status_ = net.status_;
   this->index_ = net.index_;
   this->instPinList_ = net.instPinList_;
   this->globalRouteList_ = net.globalRouteList_;
   this->routeList_ = net.routeList_;
}

//===========================================================================//
// Method         : ~TNO_Net_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
TNO_Net_c::~TNO_Net_c( 
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
TNO_Net_c& TNO_Net_c::operator=( 
      const TNO_Net_c& net )
{
   if( &net != this )
   {
      this->srName_ = net.srName_;
      this->type_ = net.type_;
      this->routable_ = net.routable_;
      this->status_ = net.status_;
      this->index_ = net.index_;
      this->instPinList_ = net.instPinList_;
      this->globalRouteList_ = net.globalRouteList_;
      this->routeList_ = net.routeList_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator<
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
bool TNO_Net_c::operator<( 
      const TNO_Net_c& net ) const
{
   return(( TC_CompareStrings( this->srName_, net.srName_ ) < 0 ) ? 
          true : false );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
bool TNO_Net_c::operator==( 
      const TNO_Net_c& net ) const
{
  return(( this->srName_ == net.srName_ ) &&
          ( this->type_ == net.type_ ) &&
          ( this->routable_ == net.routable_ ) &&
          ( this->status_ == net.status_ ) &&
          ( this->index_ == net.index_ ) &&
          ( this->instPinList_ == net.instPinList_ ) &&
          ( this->globalRouteList_ == net.globalRouteList_ ) &&
          ( this->routeList_ == net.routeList_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
bool TNO_Net_c::operator!=( 
      const TNO_Net_c& net ) const
{
   return( !this->operator==( net ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
void TNO_Net_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<net name=\"%s\"",
                                        TIO_PSZ_STR( this->GetName( )));

   if( this->GetType( ) != TC_TYPE_UNDEFINED )
   {
      string srType;
      TC_ExtractStringTypeMode( this->GetType( ), &srType );
      printHandler.Write( pfile, 0, " type=\"%s\"",  
                                    TIO_SR_STR( srType ));
   }
   if( !this->GetRoutable( ))
   {
      printHandler.Write( pfile, 0, " routable=\"%s\"",  
                                    TIO_BOOL_VAL( this->GetRoutable( )));
   }
   if( this->GetStatus( ) != TNO_STATUS_UNDEFINED )
   {
      string srStatus;
      TNO_ExtractStringStatusMode( this->GetStatus( ), &srStatus );
      printHandler.Write( pfile, 0, " status=\"%s\"",  
                                    TIO_SR_STR( srStatus ));
   }
   printHandler.Write( pfile, 0, ">\n" );
   spaceLen += 3;

   if( this->instPinList_.IsValid( ))
   {
      string srInstPin;
      for( size_t i = 0; i < this->instPinList_.GetLength( ); ++i )
      {
         const TNO_InstPin_c& instPin = *this->instPinList_[i];
         if( instPin.GetType( ) != TC_TYPE_OUTPUT )
            continue;

         instPin.ExtractString( &srInstPin );
         printHandler.Write( pfile, spaceLen, "%s\n",
                                              TIO_SR_STR( srInstPin ));
      }
      for( size_t i = 0; i < this->instPinList_.GetLength( ); ++i )
      {
         const TNO_InstPin_c& instPin = *this->instPinList_[i];
         if( instPin.GetType( ) != TC_TYPE_INPUT )
            continue;

         instPin.ExtractString( &srInstPin );
         printHandler.Write( pfile, spaceLen, "%s\n",
                                              TIO_SR_STR( srInstPin ));
      }
      for( size_t i = 0; i < this->instPinList_.GetLength( ); ++i )
      {
         const TNO_InstPin_c& instPin = *this->instPinList_[i];
         if( instPin.GetType( ) != TC_TYPE_CLOCK )
            continue;

         instPin.ExtractString( &srInstPin );
         printHandler.Write( pfile, spaceLen, "%s\n",
                                              TIO_SR_STR( srInstPin ));
      }
   }

   if( this->globalRouteList_.IsValid( ))
   {
      for( size_t i = 0; i < this->globalRouteList_.GetLength( ); ++i )
      {
         const TNO_GlobalRoute_t& globalRoute = *this->globalRouteList_[i];

         printHandler.Write( pfile, spaceLen, "<groute name=\"%s\" length=\"%u\"/>\n",  
                                              TIO_PSZ_STR( globalRoute.GetName( )),
                                              globalRoute.GetLength( ));
      }
   }

   if( this->routeList_.IsValid( ))
   {
      for( size_t i = 0; i < this->routeList_.GetLength( ); ++i )
      {
         const TNO_Route_t& route = *this->routeList_[i];

         printHandler.Write( pfile, spaceLen, "<route>\n" ); 
         spaceLen += 3;

         for( size_t j = 0; j < route.GetLength( ); ++j )
         {
            const TNO_Node_c& node = *route[j];

            if( node.IsInstPin( ))
            {
               string srInstPin;
               node.GetInstPin( ).ExtractString( &srInstPin );
               printHandler.Write( pfile, spaceLen, "%s\n",
                                                    TIO_SR_STR( srInstPin ));
            }
            if( node.IsSegment( ))
            {
               string srSegment;
               node.GetSegment( ).ExtractString( &srSegment );
               printHandler.Write( pfile, spaceLen, "%s\n",
                                                    TIO_SR_STR( srSegment ));
            }
            if( node.IsSwitchBox( ))
            {
               string srSwitchBox;
               node.GetSwitchBox( ).ExtractString( &srSwitchBox );
               printHandler.Write( pfile, spaceLen, "%s\n",
                                                    TIO_SR_STR( srSwitchBox ));
            }
         }
         spaceLen -= 3;
         printHandler.Write( pfile, spaceLen, "</route>\n" ); 
      }
   }

   spaceLen -= 3;
   printHandler.Write( pfile, spaceLen, "</net>\n" );
}

//===========================================================================//
// Method         : AddInstPin
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
void TNO_Net_c::AddInstPin( 
      const TNO_InstPin_c& instPin )
{
   this->instPinList_.Add( instPin );
}

//===========================================================================//
void TNO_Net_c::AddInstPin( 
      const string&       srInstName, 
      const string&       srPinName,
            TC_TypeMode_t type )
{
   TNO_InstPin_c instPin( srInstName, srPinName, type );
   this->instPinList_.Add( instPin );
}

//===========================================================================//
void TNO_Net_c::AddInstPin( 
      const char*         pszInstName, 
      const char*         pszPinName,
            TC_TypeMode_t type )
{
   TNO_InstPin_c instPin( pszInstName, pszPinName, type );
   this->instPinList_.Add( instPin );
}

//===========================================================================//
// Method         : AddGlobalRoute
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
void TNO_Net_c::AddGlobalRoute( 
      const TNO_GlobalRoute_t& globalRoute )
{
   this->globalRouteList_.Add( globalRoute );
}

//===========================================================================//
// Method         : AddRoute
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
void TNO_Net_c::AddRoute( 
      const TNO_Route_t& route )
{
   this->routeList_.Add( route );
}

//===========================================================================//
// Method         : FindInstPinCount
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
size_t TNO_Net_c::FindInstPinCount(
      TC_TypeMode_t type ) const
{
   size_t instPinCount = 0;

   if( type != TC_TYPE_UNDEFINED )
   {  
      for( size_t i = 0; i < this->instPinList_.GetLength( ); ++i )
      {
         const TNO_InstPin_c& instPin = *this->instPinList_[i];
         if( instPin.GetType( ) == type )
         {
            ++instPinCount;
         }
      }
   }
   else
   {  
      instPinCount = this->instPinList_.GetLength( );
   }
   return( instPinCount );
}
