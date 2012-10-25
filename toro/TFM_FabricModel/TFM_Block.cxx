//===========================================================================//
// Purpose : Method definitions for the TFM_Block class.
//
//           Public methods include:
//           - TFM_Block_c, ~TFM_Block_c
//           - operator=
//           - operator==, operator!=
//           - Print
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

#if defined( SUN8 ) || defined( SUN10 )
   #include <math.h>
#endif

#include "TC_MinGrid.h"

#include "TIO_PrintHandler.h"

#include "TFM_Block.h"
#include "TFM_StringUtils.h"

//===========================================================================//
// Method         : TFM_Block_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TFM_Block_c::TFM_Block_c( 
      void )
      :
      blockType( TFM_BLOCK_UNDEFINED )
{
   this->slice.count = 0;
   this->slice.capacity = 0;

   this->timing.res = 0.0;
   this->timing.capInput = 0.0;
   this->timing.capOutput = 0.0;
   this->timing.delay = 0.0;
}

//===========================================================================//
TFM_Block_c::TFM_Block_c( 
      TFM_BlockType_t blockType_ )
      :
      blockType( blockType_ )
{
   this->slice.count = 0;
   this->slice.capacity = 0;

   this->timing.res = 0.0;
   this->timing.capInput = 0.0;
   this->timing.capOutput = 0.0;
   this->timing.delay = 0.0;
}

//===========================================================================//
TFM_Block_c::TFM_Block_c( 
            TFM_BlockType_t blockType_,
      const string&         srName_,
      const string&         srMasterName_,
      const TGS_Region_c&   region_ )
      :
      blockType( blockType_ ),
      srName( srName_ ),
      srMasterName( srMasterName_ ),
      region( region_ )
{
   this->slice.count = 0;
   this->slice.capacity = 0;

   this->timing.res = 0.0;
   this->timing.capInput = 0.0;
   this->timing.capOutput = 0.0;
   this->timing.delay = 0.0;
}

//===========================================================================//
TFM_Block_c::TFM_Block_c( 
            TFM_BlockType_t blockType_,
      const char*           pszName_,
      const char*           pszMasterName_,
      const TGS_Region_c&   region_ )
      :
      blockType( blockType_ ),
      srName( TIO_PSZ_STR( pszName_ )),
      srMasterName( TIO_PSZ_STR( pszMasterName_ )),
      region( region_ )
{
   this->slice.count = 0;
   this->slice.capacity = 0;

   this->timing.res = 0.0;
   this->timing.capInput = 0.0;
   this->timing.capOutput = 0.0;
   this->timing.delay = 0.0;
}

//===========================================================================//
TFM_Block_c::TFM_Block_c( 
            TFM_BlockType_t blockType_,
      const string&         srName_,
      const TGS_Region_c&   region_ )
      :
      blockType( blockType_ ),
      srName( srName_ ),
      region( region_ )
{
   this->slice.count = 0;
   this->slice.capacity = 0;

   this->timing.res = 0.0;
   this->timing.capInput = 0.0;
   this->timing.capOutput = 0.0;
   this->timing.delay = 0.0;
}

//===========================================================================//
TFM_Block_c::TFM_Block_c( 
            TFM_BlockType_t blockType_,
      const char*           pszName_,
      const TGS_Region_c&   region_ )
      :
      blockType( blockType_ ),
      srName( TIO_PSZ_STR( pszName_ )),
      region( region_ )
{
   this->slice.count = 0;
   this->slice.capacity = 0;

   this->timing.res = 0.0;
   this->timing.capInput = 0.0;
   this->timing.capOutput = 0.0;
   this->timing.delay = 0.0;
}

//===========================================================================//
TFM_Block_c::TFM_Block_c( 
      const TFM_Block_c& block )
      :
      blockType( block.blockType ),
      srName( block.srName ),
      srMasterName( block.srMasterName ),
      region( block.region ),
      pinList( block.pinList ),
      mapTable( block.mapTable )
{
   this->slice.count = block.slice.count;
   this->slice.capacity = block.slice.capacity;

   this->timing.res = block.timing.res;
   this->timing.capInput = block.timing.capInput;
   this->timing.capOutput = block.timing.capOutput;
   this->timing.delay = block.timing.delay;
}

//===========================================================================//
// Method         : ~TFM_Block_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TFM_Block_c::~TFM_Block_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TFM_Block_c& TFM_Block_c::operator=( 
      const TFM_Block_c& block )
{
   if( &block != this )
   {
      this->blockType = block.blockType;
      this->srName = block.srName;
      this->srMasterName = block.srMasterName;
      this->region = block.region;
      this->pinList = block.pinList;
      this->mapTable = block.mapTable;
      this->slice.count = block.slice.count;
      this->slice.capacity = block.slice.capacity;
      this->timing.res = block.timing.res;
      this->timing.capInput = block.timing.capInput;
      this->timing.capOutput = block.timing.capOutput;
      this->timing.delay = block.timing.delay;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TFM_Block_c::operator==( 
      const TFM_Block_c& block ) const
{
   return(( this->blockType == block.blockType ) &&
          ( this->srName == block.srName ) &&
          ( this->srMasterName == block.srMasterName ) &&
          ( this->region == block.region ) &&
          ( this->pinList == block.pinList ) &&
          ( this->mapTable == block.mapTable ) &&
          ( this->slice.count == block.slice.count ) &&
          ( this->slice.capacity == block.slice.capacity ) &&
          ( TCTF_IsEQ( this->timing.res, block.timing.res )) &&
          ( TCTF_IsEQ( this->timing.capInput, block.timing.capInput )) &&
          ( TCTF_IsEQ( this->timing.capOutput, block.timing.capOutput )) &&
          ( TCTF_IsEQ( this->timing.delay, block.timing.delay )) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TFM_Block_c::operator!=( 
      const TFM_Block_c& block ) const
{
   return( !this->operator==( block ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TFM_Block_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   const char* pszUsage = "?";

   switch( this->blockType )
   {
   case TFM_BLOCK_PHYSICAL_BLOCK: pszUsage = "pb"; break;
   case TFM_BLOCK_INPUT_OUTPUT:   pszUsage = "io"; break;
   case TFM_BLOCK_SWITCH_BOX:     pszUsage = "sb"; break;
   case TFM_BLOCK_UNDEFINED:                       break;
   }
   printHandler.Write( pfile, spaceLen, "<%s ", pszUsage );

   printHandler.Write( pfile, 0, "\"%s\" ", TIO_SR_STR( this->srName ));
   if( this->srMasterName.length( ))
   {
      printHandler.Write( pfile, 0, "master = \"%s\" ", TIO_SR_STR( this->srMasterName ));
   }
   printHandler.Write( pfile, 0, "> " );
   if( this->region.IsValid( ))
   {
      string srRegion;
      this->region.ExtractString( &srRegion );
      printHandler.Write( pfile, 0, "<region %s >", TIO_SR_STR( srRegion ));
   }
   printHandler.Write( pfile, 0, "\n" );
   spaceLen += 3;

   if( this->pinList.IsValid( ))
   {
      this->pinList.Print( pfile, spaceLen );
   }

   if( this->mapTable.IsValid( ))
   {
      printHandler.Write( pfile, spaceLen, "<mapping\n" );
      this->mapTable.Print( pfile, spaceLen + 3 );
      printHandler.Write( pfile, spaceLen, ">\n" );
   }

   if( this->slice.count ||
       this->slice.capacity )
   {
      printHandler.Write( pfile, spaceLen, "<slice count = %u capacity = %u >\n",
                                           this->slice.count,
                                           this->slice.capacity );
   }

   if( TCTF_IsGT( this->timing.res, 0.0 ) ||
       TCTF_IsGT( this->timing.capInput, 0.0 ) ||
       TCTF_IsGT( this->timing.capOutput, 0.0 ) ||
       TCTF_IsGT( this->timing.delay, 0.0 ))
   {
      printHandler.Write( pfile, spaceLen, "<timing res = %0.*f cap_in = %0.*f cap_out = %0.*f delay = %0.*e >\n",
                                           precision, this->timing.res,
                                           precision, this->timing.capInput,
                                           precision, this->timing.capOutput,
                                           precision + 1, this->timing.delay );
   }
   spaceLen -= 3;
   printHandler.Write( pfile, spaceLen, "</%s>\n", pszUsage );
}
