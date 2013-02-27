//===========================================================================//
// Purpose : Method definitions for the TFH_BlockGrid class.
//
//           Public methods include:
//           - TFH_BlockGrid_c, ~TFH_BlockGrid_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
//
//===========================================================================//

#include "TC_StringUtils.h"

//---------------------------------------------------------------------------//
// Copyright (C) 2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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

#include "TC_StringUtils.h"

#include "TIO_PrintHandler.h"

#include "TFH_StringUtils.h"
#include "TFH_BlockGrid.h"

//===========================================================================//
// Method         : TFH_BlockGrid_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TFH_BlockGrid_c::TFH_BlockGrid_c( 
      void )
      :
      blockType_( TFH_BLOCK_UNDEFINED )
{
} 

//===========================================================================//
TFH_BlockGrid_c::TFH_BlockGrid_c( 
      const string&         srBlockName,
            TFH_BlockType_t blockType,
      const TGO_Point_c&    gridPoint )
      :
      srBlockName_( srBlockName ),
      blockType_( blockType ),
      gridPoint_( gridPoint )

{
} 

//===========================================================================//
TFH_BlockGrid_c::TFH_BlockGrid_c( 
      const char*           pszBlockName,
            TFH_BlockType_t blockType,
      const TGO_Point_c&    gridPoint )
      :
      srBlockName_( TIO_PSZ_STR( pszBlockName )),
      blockType_( blockType ),
      gridPoint_( gridPoint )
{
} 

//===========================================================================//
TFH_BlockGrid_c::TFH_BlockGrid_c( 
      const TFH_BlockGrid_c& blockGrid )
      :
      srBlockName_( blockGrid.srBlockName_ ),
      blockType_( blockGrid.blockType_ ),
      gridPoint_( blockGrid.gridPoint_ )
{
} 

//===========================================================================//
// Method         : ~TFH_BlockGrid_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TFH_BlockGrid_c::~TFH_BlockGrid_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TFH_BlockGrid_c& TFH_BlockGrid_c::operator=( 
      const TFH_BlockGrid_c& blockGrid )
{
   if( &blockGrid != this )
   {
      this->srBlockName_ = blockGrid.srBlockName_;
      this->blockType_ = blockGrid.blockType_;
      this->gridPoint_ = blockGrid.gridPoint_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator<
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TFH_BlockGrid_c::operator<( 
      const TFH_BlockGrid_c& blockGrid ) const
{
   bool isLessThan = false;

   if( TC_CompareStrings( this->srBlockName_, blockGrid.srBlockName_ ) < 0 )
   {
      isLessThan = true;
   }
   else if( TC_CompareStrings( this->srBlockName_, blockGrid.srBlockName_ ) == 0 )
   {
      if( this->blockType_ < blockGrid.blockType_ )
      {
         isLessThan = true;
      }
      else if( this->blockType_ == blockGrid.blockType_ )
      {
         if( this->gridPoint_ < blockGrid.gridPoint_ )
         {
            isLessThan = true;
         }
      }
   }
   return( isLessThan );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TFH_BlockGrid_c::operator==( 
      const TFH_BlockGrid_c& blockGrid ) const
{
   return(( this->srBlockName_ == blockGrid.srBlockName_ ) &&
          ( this->blockType_ == blockGrid.blockType_ ) &&
          ( this->gridPoint_ == blockGrid.gridPoint_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TFH_BlockGrid_c::operator!=( 
      const TFH_BlockGrid_c& blockGrid ) const
{
   return( !this->operator==( blockGrid ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TFH_BlockGrid_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<grid" );
   if( this->srBlockName_.length( ))
   {
      printHandler.Write( pfile, 0, " block=\"%s\"", TIO_SR_STR( this->srBlockName_ ));
   }
   if( this->blockType_ != TFH_BLOCK_UNDEFINED )
   {
      string srBlockType;
      TFH_ExtractStringBlockType( this->blockType_, &srBlockType );
      printHandler.Write( pfile, 0, " type=%s", TIO_SR_STR( srBlockType ));
   }
   if( this->gridPoint_.IsValid( ))
   {
      string srGridPoint;
      this->gridPoint_.ExtractString( &srGridPoint );
      printHandler.Write( pfile, 0, " point=(%s)", TIO_SR_STR( srGridPoint ));
   }
   printHandler.Write( pfile, 0, ">\n" );
} 
