//===========================================================================//
// Purpose : Method definitions for the TC_MapTable class.
//
//           Public methods include:
//           - TC_MapTable_c, ~TC_MapTable_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - Init
//           - Add
//           - FindMapSideList
//           - FindSideList
//
//           Public methods include:
//           - FindMapSideList_
//           - FindSideList_
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

#include "TC_MapTable.h"

//===========================================================================//
// Method         : TC_MapTable_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TC_MapTable_c::TC_MapTable_c( 
      void )
{
}

//===========================================================================//
TC_MapTable_c::TC_MapTable_c( 
      const TC_MapTable_c& mapTable )
      :
      leftSideList_( mapTable.leftSideList_ ),
      rightSideList_( mapTable.rightSideList_ ),
      lowerSideList_ ( mapTable.lowerSideList_ ),
      upperSideList_( mapTable.upperSideList_ )
{
}

//===========================================================================//
// Method         : ~TC_MapTable_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TC_MapTable_c::~TC_MapTable_c( 
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
TC_MapTable_c& TC_MapTable_c::operator=( 
      const TC_MapTable_c& mapTable )
{
   if( &mapTable != this )
   {
      this->leftSideList_ = mapTable.leftSideList_;
      this->rightSideList_ = mapTable.rightSideList_;
      this->lowerSideList_ = mapTable.lowerSideList_;
      this->upperSideList_ = mapTable.upperSideList_;
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
bool TC_MapTable_c::operator==( 
      const TC_MapTable_c& mapTable ) const
{
   return(( this->leftSideList_ == mapTable.leftSideList_ ) &&
          ( this->rightSideList_ == mapTable.rightSideList_ ) &&
          ( this->lowerSideList_ == mapTable.lowerSideList_ ) &&
          ( this->upperSideList_ == mapTable.upperSideList_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TC_MapTable_c::operator!=( 
      const TC_MapTable_c& mapTable ) const
{
   return( !this->operator==( mapTable ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TC_MapTable_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   this->Print( pfile, spaceLen, TC_SIDE_LEFT, this->leftSideList_ );
   this->Print( pfile, spaceLen, TC_SIDE_RIGHT, this->rightSideList_ );
   this->Print( pfile, spaceLen, TC_SIDE_LOWER, this->lowerSideList_ );
   this->Print( pfile, spaceLen, TC_SIDE_UPPER, this->upperSideList_ );
}

//===========================================================================//
void TC_MapTable_c::Print( 
            FILE*             pfile,
            size_t            spaceLen,
            TC_SideMode_t     side,
      const TC_MapSideList_t& mapSideList ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   for( size_t i = 0; i < mapSideList.GetLength( ); ++i )
   {
      const TC_SideList_t& sideList = *mapSideList[i];
      if( !sideList.IsValid( ))
	 continue;

      TC_SideIndex_c sideIndex( side, i );
      string srSideIndex;
      sideIndex.ExtractString( &srSideIndex );

      string srSideList;
      for( size_t j = 0; j < sideList.GetLength( ); ++j )
      {
         string srSideIndex_;
         sideList[j]->ExtractString( &srSideIndex_ );

         srSideList += ( j == 0 ? "" : " " );
         srSideList += srSideIndex_;
      }
      printHandler.Write( pfile, spaceLen, "%s %s\n",
                                           TIO_SR_STR( srSideIndex ),
                                           TIO_SR_STR( srSideList ));
   }
}

//===========================================================================//
// Method         : Init
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TC_MapTable_c::Init(
      size_t widthHorz,
      size_t widthVert )
{
   this->Init( widthHorz, widthHorz, widthVert, widthVert );
}

//===========================================================================//
void TC_MapTable_c::Init(
      size_t widthLeft,
      size_t widthRight,
      size_t widthLower,
      size_t widthUpper )
{
   this->Init( widthLeft, &this->leftSideList_ );
   this->Init( widthRight, &this->rightSideList_ );
   this->Init( widthLower, &this->lowerSideList_ );
   this->Init( widthUpper, &this->upperSideList_ );
}

//===========================================================================//
void TC_MapTable_c::Init(
      size_t            width,
      TC_MapSideList_t* pmapSideList )
{
   pmapSideList->Clear( );

   pmapSideList->SetCapacity( width );

   for( size_t i = 0; i < width; ++i )
   {
      TC_SideList_t sideList;
      pmapSideList->Insert( i, sideList );
   }
}

//===========================================================================//
// Method         : Add
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TC_MapTable_c::Add(
      TC_SideMode_t side,
      size_t        index,
      TC_SideMode_t side_,
      size_t        index_ )
{
   TC_SideIndex_c sideIndex( side_, index_ );
   return( this->Add( side, index, sideIndex ));
}

//===========================================================================//
bool TC_MapTable_c::Add(
            TC_SideMode_t   side,
            size_t          index,
      const TC_SideIndex_c& sideIndex )
{
   TC_SideList_t* psideList = this->FindSideList_( side, index );
   if( psideList )
   {
      psideList->Add( sideIndex );
   }
   return( psideList ? true : false );
}

//===========================================================================//
bool TC_MapTable_c::Add(
            TC_SideMode_t  side,
            size_t         index,
      const TC_SideList_t& sideList )
{
   bool ok = true;

   for( size_t i = 0; i < sideList.GetLength( ); ++i )
   {
      const TC_SideIndex_c& sideIndex = *sideList[i];
      ok = this->Add( side, index, sideIndex );
      if( !ok )
	 break;
   }
   return( ok );
}

//===========================================================================//
bool TC_MapTable_c::Add( 
            TC_SideMode_t     side,
      const TC_MapSideList_t& mapSideList )
{
   bool ok = true;

   for( size_t i = 0; i < mapSideList.GetLength( ); ++i )
   {
      const TC_SideList_t& sideList = *mapSideList[i];
      if( !sideList.IsValid( ))
	 continue;

      TC_SideList_t* psideList = this->FindSideList_( side, i );
      if( psideList )
      {
         psideList->Add( sideList );
      }
   }
   return( ok );
}

//===========================================================================//
bool TC_MapTable_c::Add(
      const TC_MapTable_c& mapTable )
{
   bool ok = true;

   if( ok )
   {
      ok = this->Add( TC_SIDE_LEFT, mapTable.leftSideList_ );
   }
   if( ok )
   {
      ok = this->Add( TC_SIDE_RIGHT, mapTable.rightSideList_ );
   }
   if( ok )
   {
      ok = this->Add( TC_SIDE_LOWER, mapTable.lowerSideList_ );
   }
   if( ok )
   {
      ok = this->Add( TC_SIDE_UPPER, mapTable.upperSideList_ );
   }
   return( ok );
}

//===========================================================================//
// Method         : FindMapSideList
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
const TC_MapSideList_t* TC_MapTable_c::FindMapSideList(
      TC_SideMode_t side ) const
{
   TC_MapTable_c* pmapTable = const_cast< TC_MapTable_c* >( this );
   return( pmapTable->FindMapSideList_( side ));
}

//===========================================================================//
// Method         : FindSideList
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
const TC_SideList_t* TC_MapTable_c::FindSideList(
      TC_SideMode_t side,
      size_t        index ) const
{
   TC_MapTable_c* pmapTable = const_cast< TC_MapTable_c* >( this );
   return( pmapTable->FindSideList_( side, index ));
}

//===========================================================================//
// Method         : FindMapSideList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TC_MapSideList_t* TC_MapTable_c::FindMapSideList_(
      TC_SideMode_t side )
{
   TC_MapSideList_t* pmapSideList = 0;

   switch( side )
   {
   case TC_SIDE_LEFT:  pmapSideList = &this->leftSideList_;  break;
   case TC_SIDE_RIGHT: pmapSideList = &this->rightSideList_; break;
   case TC_SIDE_LOWER: pmapSideList = &this->lowerSideList_; break;
   case TC_SIDE_UPPER: pmapSideList = &this->upperSideList_; break;
   default:                                                  break;
   }
   return( pmapSideList );
}

//===========================================================================//
// Method         : FindSideList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TC_SideList_t* TC_MapTable_c::FindSideList_(
      TC_SideMode_t side,
      size_t        index )
{
   TC_MapSideList_t* pmapSideList = this->FindMapSideList_( side );
   return( pmapSideList ? ( *pmapSideList )[index] : 0 );
}
