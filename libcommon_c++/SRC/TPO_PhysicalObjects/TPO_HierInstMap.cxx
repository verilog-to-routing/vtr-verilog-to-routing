//===========================================================================//
// Purpose : Method definitions for the TPO_HierInstMap class.
//
//           Public methods include:
//           - TPO_HierInstMap_c, ~TPO_HierInstMap_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
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

#include "TIO_PrintHandler.h"

#include "TC_StringUtils.h"

#include "TPO_HierInstMap.h"

//===========================================================================//
// Method         : TPO_HierInstMap_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/26/13 jeffr : Original
//===========================================================================//
TPO_HierInstMap_c::TPO_HierInstMap_c( 
      void )
      :
      instIndex_( TPO_INST_INDEX_INVALID )
{
}

//===========================================================================//
TPO_HierInstMap_c::TPO_HierInstMap_c( 
      const string& srHierName,
      const string& srInstName,
            size_t  instIndex )
      :
      srHierName_( srHierName ),
      srInstName_( srInstName ),
      instIndex_( instIndex )
{
}

//===========================================================================//
TPO_HierInstMap_c::TPO_HierInstMap_c( 
      const char*  pszHierName,
      const char*  pszInstName,
            size_t instIndex )
      :
      srHierName_( TIO_PSZ_STR( pszHierName )),
      srInstName_( TIO_PSZ_STR( pszInstName )),
      instIndex_( instIndex )
{
}

//===========================================================================//
TPO_HierInstMap_c::TPO_HierInstMap_c( 
      const string& srHierName )
      :
      srHierName_( srHierName ),
      instIndex_( TPO_INST_INDEX_INVALID )
{
}

//===========================================================================//
TPO_HierInstMap_c::TPO_HierInstMap_c( 
      const char* pszHierName )
      :
      srHierName_( TIO_PSZ_STR( pszHierName )),
      instIndex_( TPO_INST_INDEX_INVALID )
{
}

//===========================================================================//
TPO_HierInstMap_c::TPO_HierInstMap_c( 
      const TPO_HierInstMap_c& hierInstMap )
      :
      srHierName_( hierInstMap.srHierName_ ),
      srInstName_( hierInstMap.srInstName_ ),
      instIndex_( hierInstMap.instIndex_ )
{
}

//===========================================================================//
// Method         : ~TPO_HierInstMap_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/26/13 jeffr : Original
//===========================================================================//
TPO_HierInstMap_c::~TPO_HierInstMap_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/26/13 jeffr : Original
//===========================================================================//
TPO_HierInstMap_c& TPO_HierInstMap_c::operator=( 
      const TPO_HierInstMap_c& hierInstMap )
{
   if( &hierInstMap != this )
   {
      this->srHierName_ = hierInstMap.srHierName_;
      this->srInstName_ = hierInstMap.srInstName_;
      this->instIndex_ = hierInstMap.instIndex_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator<
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/26/13 jeffr : Original
//===========================================================================//
bool TPO_HierInstMap_c::operator<( 
      const TPO_HierInstMap_c& hierInstMap ) const
{
   return(( TC_CompareStrings( this->srHierName_, hierInstMap.srHierName_ ) < 0 ) ? 
          true : false );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/26/13 jeffr : Original
//===========================================================================//
bool TPO_HierInstMap_c::operator==( 
      const TPO_HierInstMap_c& hierInstMap ) const
{
   return(( this->srHierName_ == hierInstMap.srHierName_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/26/13 jeffr : Original
//===========================================================================//
bool TPO_HierInstMap_c::operator!=( 
      const TPO_HierInstMap_c& hierInstMap ) const
{
   return( !this->operator==( hierInstMap ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/26/13 jeffr : Original
//===========================================================================//
void TPO_HierInstMap_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "hier_name=\"%s\"", 
                                        TIO_SR_STR( this->srHierName_ ));
   if( this->srInstName_.length( ))
   {
      printHandler.Write( pfile, 0, " inst_name=\"%s\"", 
                                    TIO_SR_STR( this->srInstName_ ));
   }
   if( this->instIndex_ != TPO_INST_INDEX_INVALID )
   {
      printHandler.Write( pfile, 0, " inst_index=%d", 
                                    this->instIndex_ );
   }
   printHandler.Write( pfile, 0, "\n" );
}
