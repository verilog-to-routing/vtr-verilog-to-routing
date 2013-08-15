//===========================================================================//
// Purpose : Method definitions for the TCH_RelativeBlock class.
//
//           Public methods include:
//           - TCH_RelativeBlock_c, ~TCH_RelativeBlock_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
//           - Reset
//
//===========================================================================//

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

#include "TCH_RelativeBlock.h"

//===========================================================================//
// Method         : TCH_RelativeBlock_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TCH_RelativeBlock_c::TCH_RelativeBlock_c( 
      void )
      :
      vpr_index_( -1 ),
      vpr_type_( 0 )
{
   this->Reset( );
} 

//===========================================================================//
TCH_RelativeBlock_c::TCH_RelativeBlock_c( 
      const string&            srName,
            int                vpr_index,
      const t_type_descriptor* vpr_type,
            size_t             relativeMacroIndex,
            size_t             relativeNodeIndex )
      :
      srName_( srName ),
      vpr_index_( vpr_index ),
      vpr_type_( const_cast< t_type_descriptor* >( vpr_type )),
      relativeMacroIndex_( relativeMacroIndex ),
      relativeNodeIndex_( relativeNodeIndex )
{
} 

//===========================================================================//
TCH_RelativeBlock_c::TCH_RelativeBlock_c( 
      const char*              pszName,
            int                vpr_index,
      const t_type_descriptor* vpr_type,
            size_t             relativeMacroIndex,
            size_t             relativeNodeIndex )
      :
      srName_( TIO_PSZ_STR( pszName )),
      vpr_index_( vpr_index ),
      vpr_type_( const_cast< t_type_descriptor* >( vpr_type )),
      relativeMacroIndex_( relativeMacroIndex ),
      relativeNodeIndex_( relativeNodeIndex )
{
} 

//===========================================================================//
TCH_RelativeBlock_c::TCH_RelativeBlock_c( 
      const TCH_RelativeBlock_c& relativeBlock )
      :
      srName_( relativeBlock.srName_ ),
      vpr_index_( relativeBlock.vpr_index_ ),
      vpr_type_( relativeBlock.vpr_type_ ),
      relativeMacroIndex_( relativeBlock.relativeMacroIndex_ ),
      relativeNodeIndex_( relativeBlock.relativeNodeIndex_ )
{
} 

//===========================================================================//
// Method         : ~TCH_RelativeBlock_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TCH_RelativeBlock_c::~TCH_RelativeBlock_c( 
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
TCH_RelativeBlock_c& TCH_RelativeBlock_c::operator=( 
      const TCH_RelativeBlock_c& relativeBlock )
{
   if( &relativeBlock != this )
   {
      this->srName_ = relativeBlock.srName_;
      this->vpr_index_ = relativeBlock.vpr_index_;
      this->vpr_type_ = relativeBlock.vpr_type_;
      this->relativeMacroIndex_ = relativeBlock.relativeMacroIndex_;
      this->relativeNodeIndex_ = relativeBlock.relativeNodeIndex_;
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
bool TCH_RelativeBlock_c::operator<( 
      const TCH_RelativeBlock_c& relativeBlock ) const
{
   return(( TC_CompareStrings( this->srName_, relativeBlock.srName_ ) < 0 ) ? 
          true : false );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TCH_RelativeBlock_c::operator==( 
      const TCH_RelativeBlock_c& relativeBlock ) const
{
   return(( this->srName_ == relativeBlock.srName_ ) &&
          ( this->vpr_index_ == relativeBlock.vpr_index_ ) &&
          ( this->vpr_type_ == relativeBlock.vpr_type_ ) &&
          ( this->relativeMacroIndex_ == relativeBlock.relativeMacroIndex_ ) &&
          ( this->relativeNodeIndex_ == relativeBlock.relativeNodeIndex_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TCH_RelativeBlock_c::operator!=( 
      const TCH_RelativeBlock_c& relativeBlock ) const
{
   return( !this->operator==( relativeBlock ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TCH_RelativeBlock_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<block" );
   printHandler.Write( pfile, 0, " name=\"%s\"", TIO_SR_STR( this->srName_ ));
   if( this->vpr_index_ != -1 )
   {
      printHandler.Write( pfile, 0, " vpr_index=%d", this->vpr_index_ );
   }
   if( this->vpr_type_ )
   {
      printHandler.Write( pfile, 0, " vpr_type=[0x%08x]", this->vpr_type_ );
   }
   if( this->relativeMacroIndex_ != TCH_RELATIVE_MACRO_UNDEFINED )
   {
      printHandler.Write( pfile, 0, " macro_index=%u", this->relativeMacroIndex_ );
   }
   if( this->relativeNodeIndex_ != TCH_RELATIVE_NODE_UNDEFINED )
   {
      printHandler.Write( pfile, 0, " node_index=%u", this->relativeNodeIndex_ );
   }
   printHandler.Write( pfile, 0, ">\n" );
} 

//===========================================================================//
// Method         : Reset
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TCH_RelativeBlock_c::Reset( 
      void )
{
   this->relativeMacroIndex_ = TCH_RELATIVE_MACRO_UNDEFINED;
   this->relativeNodeIndex_ = TCH_RELATIVE_NODE_UNDEFINED;
} 
