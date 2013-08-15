//===========================================================================//
// Purpose : Method definitions for the TPO_InstHierMap class.
//
//           Public methods include:
//           - TPO_InstHierMap_c, ~TPO_InstHierMap_c
//           - operator=
//           - operator==, operator!=
//           - Print
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012-2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com) //
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

#include "TPO_InstHierMap.h"

//===========================================================================//
// Method         : TPO_InstHierMap_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TPO_InstHierMap_c::TPO_InstHierMap_c( 
      void )
      :
      hierNameList_( TPO_INST_HIER_NAME_LIST_DEF_CAPACITY )
{
}

//===========================================================================//
TPO_InstHierMap_c::TPO_InstHierMap_c( 
      const string&         srInstName,
      const TPO_NameList_t& hierNameList )
      :
      srInstName_( srInstName ),
      hierNameList_( hierNameList )
{
}

//===========================================================================//
TPO_InstHierMap_c::TPO_InstHierMap_c( 
      const char*           pszInstName,
      const TPO_NameList_t& hierNameList )
      :
      srInstName_( TIO_PSZ_STR( pszInstName )),
      hierNameList_( hierNameList )
{
}

//===========================================================================//
TPO_InstHierMap_c::TPO_InstHierMap_c( 
      const TPO_InstHierMap_c& instHierMap )
      :
      srInstName_( instHierMap.srInstName_ ),
      hierNameList_( instHierMap.hierNameList_ )
{
}

//===========================================================================//
// Method         : ~TPO_InstHierMap_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TPO_InstHierMap_c::~TPO_InstHierMap_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TPO_InstHierMap_c& TPO_InstHierMap_c::operator=( 
      const TPO_InstHierMap_c& instHierMap )
{
   if( &instHierMap != this )
   {
      this->srInstName_ = instHierMap.srInstName_;
      this->hierNameList_ = instHierMap.hierNameList_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TPO_InstHierMap_c::operator==( 
      const TPO_InstHierMap_c& instHierMap ) const
{
   return(( this->srInstName_ == instHierMap.srInstName_ ) &&
          ( this->hierNameList_ == instHierMap.hierNameList_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TPO_InstHierMap_c::operator!=( 
      const TPO_InstHierMap_c& instHierMap ) const
{
   return( !this->operator==( instHierMap ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TPO_InstHierMap_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<pack name=\"%s\">\n", 
                                        TIO_SR_STR( this->srInstName_ ));
   spaceLen += 3;

   printHandler.Write( pfile, spaceLen, "<hier> " );
   for( size_t i = 0; i < this->hierNameList_.GetLength( ); ++i )
   {
      const TC_Name_c& hierName = *this->hierNameList_[i];
      printHandler.Write( pfile, 0, "\"%s\" ", 
                                    TIO_PSZ_STR( hierName.GetName( )));
   }
   printHandler.Write( pfile, 0, "</hier>\n" );

   spaceLen -= 3;
   printHandler.Write( pfile, spaceLen, "</pack>\n" );
}
