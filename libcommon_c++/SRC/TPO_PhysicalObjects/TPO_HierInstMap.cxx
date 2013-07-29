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
   return(( this->srHierName_ == hierInstMap.srHierName_ ) &&
          ( this->srInstName_ == hierInstMap.srInstName_ ) &&
          ( this->instIndex_ == hierInstMap.instIndex_ ) ?
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
