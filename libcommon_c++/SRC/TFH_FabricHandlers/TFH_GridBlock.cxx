//===========================================================================//
// Purpose : Method definitions for the TFH_GridBlock class.
//
//           Public methods include:
//           - TFH_GridBlock_c, ~TFH_GridBlock_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
//
//===========================================================================//

#include "TC_StringUtils.h"

//---------------------------------------------------------------------------//
// Copyright (C) 2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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

#include "TC_StringUtils.h"

#include "TIO_PrintHandler.h"

#include "TFH_StringUtils.h"
#include "TFH_GridBlock.h"

//===========================================================================//
// Method         : TFH_GridBlock_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TFH_GridBlock_c::TFH_GridBlock_c( 
      void )
      :
      vpr_typeIndex_( -1 ),
      blockType_( TFH_BLOCK_UNDEFINED )
{
} 

//===========================================================================//
TFH_GridBlock_c::TFH_GridBlock_c( 
      const TGO_Point_c& vpr_gridPoint )
      :
      vpr_gridPoint_( vpr_gridPoint ),
      vpr_typeIndex_( -1 ),
      blockType_( TFH_BLOCK_UNDEFINED )
{
} 

//===========================================================================//
TFH_GridBlock_c::TFH_GridBlock_c( 
      const TGO_Point_c&    vpr_gridPoint,
            int             vpr_typeIndex,
            TFH_BlockType_t blockType,
      const string&         srBlockName,
      const string&         srMasterName )
      :
      vpr_gridPoint_( vpr_gridPoint ),
      vpr_typeIndex_( vpr_typeIndex ),
      blockType_( blockType ),
      srBlockName_( srBlockName ),
      srMasterName_( srMasterName )
{
} 

//===========================================================================//
TFH_GridBlock_c::TFH_GridBlock_c( 
      const TGO_Point_c&    vpr_gridPoint,
            int             vpr_typeIndex,
            TFH_BlockType_t blockType,
      const char*           pszBlockName,
      const char*           pszMasterName )
      :
      vpr_gridPoint_( vpr_gridPoint ),
      vpr_typeIndex_( vpr_typeIndex ),
      blockType_( blockType ),
      srBlockName_( TIO_PSZ_STR( pszBlockName )),
      srMasterName_( TIO_PSZ_STR( pszMasterName ))
{
} 

//===========================================================================//
TFH_GridBlock_c::TFH_GridBlock_c( 
      int x,
      int y )
      :
      vpr_gridPoint_( x, y ),
      vpr_typeIndex_( -1 ),
      blockType_( TFH_BLOCK_UNDEFINED )
{
} 

//===========================================================================//
TFH_GridBlock_c::TFH_GridBlock_c( 
      const TFH_GridBlock_c& gridBlock )
      :
      vpr_gridPoint_( gridBlock.vpr_gridPoint_ ),
      vpr_typeIndex_( gridBlock.vpr_typeIndex_ ),
      blockType_( gridBlock.blockType_ ),
      srBlockName_( gridBlock.srBlockName_ ),
      srMasterName_( gridBlock.srMasterName_ )
{
} 

//===========================================================================//
// Method         : ~TFH_GridBlock_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TFH_GridBlock_c::~TFH_GridBlock_c( 
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
TFH_GridBlock_c& TFH_GridBlock_c::operator=( 
      const TFH_GridBlock_c& gridBlock )
{
   if( &gridBlock != this )
   {
      this->vpr_gridPoint_ = gridBlock.vpr_gridPoint_;
      this->vpr_typeIndex_ = gridBlock.vpr_typeIndex_;
      this->blockType_ = gridBlock.blockType_;
      this->srBlockName_ = gridBlock.srBlockName_;
      this->srMasterName_ = gridBlock.srMasterName_;
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
bool TFH_GridBlock_c::operator<( 
      const TFH_GridBlock_c& gridBlock ) const
{
   bool isLessThan = false;

   if( this->vpr_gridPoint_.x < gridBlock.vpr_gridPoint_.x )
   {
      isLessThan = true;
   }
   else if( this->vpr_gridPoint_.x == gridBlock.vpr_gridPoint_.x )
   {
      if( this->vpr_gridPoint_.y < gridBlock.vpr_gridPoint_.y )
      {
         isLessThan = true;
      }
      else if( this->vpr_gridPoint_.y == gridBlock.vpr_gridPoint_.y )
      {
         if( this->vpr_gridPoint_.z < gridBlock.vpr_gridPoint_.z )
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
bool TFH_GridBlock_c::operator==( 
      const TFH_GridBlock_c& gridBlock ) const
{
   return(( this->vpr_gridPoint_ == gridBlock.vpr_gridPoint_ ) &&
          ( this->vpr_typeIndex_ == gridBlock.vpr_typeIndex_ ) &&
          ( this->blockType_ == gridBlock.blockType_ ) &&
          ( this->srBlockName_ == gridBlock.srBlockName_ ) &&
          ( this->srMasterName_ == gridBlock.srMasterName_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TFH_GridBlock_c::operator!=( 
      const TFH_GridBlock_c& gridBlock ) const
{
   return( !this->operator==( gridBlock ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TFH_GridBlock_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<grid" );

   if( this->vpr_gridPoint_.IsValid( ))
   {
      string srGridPoint;
      this->vpr_gridPoint_.ExtractString( &srGridPoint );
      printHandler.Write( pfile, 0, " point=(%s)", TIO_SR_STR( srGridPoint ));
   }
   if( this->vpr_typeIndex_ != -1 )
   {
      printHandler.Write( pfile, 0, " type_index=(%d)", this->vpr_typeIndex_ );
   }
   if( this->blockType_ != TFH_BLOCK_UNDEFINED )
   {
      string srBlockType;
      TFH_ExtractStringBlockType( this->blockType_, &srBlockType );
      printHandler.Write( pfile, 0, " block_type=%s", TIO_SR_STR( srBlockType ));
   }
   if( this->srBlockName_.length( ))
   {
      printHandler.Write( pfile, 0, " block_name=\"%s\"", TIO_SR_STR( this->srBlockName_ ));
   }
   if( this->srMasterName_.length( ))
   {
      printHandler.Write( pfile, 0, " master_name=\"%s\"", TIO_SR_STR( this->srMasterName_ ));
   }
   printHandler.Write( pfile, 0, ">\n" );
} 
