//===========================================================================//
// Purpose : Method definitions for the TPO_PlaceRegions class.
//
//           Public methods include:
//           - TPO_PlaceRegions_c, ~TPO_PlaceRegions_c
//           - operator=
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

#include "TPO_PlaceRegions.h"

//===========================================================================//
// Method         : TPO_PlaceRegions_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07//26/13 jeffr : Original
//===========================================================================//
TPO_PlaceRegions_c::TPO_PlaceRegions_c( 
      void )
      :
      regionList_( TPO_REGION_LIST_DEF_CAPACITY ),
      nameList_( TPO_NAME_LIST_DEF_CAPACITY )
{
} 

//===========================================================================//
TPO_PlaceRegions_c::TPO_PlaceRegions_c( 
      const TGO_RegionList_t& regionList,
      const TPO_NameList_t&   nameList )
{
   this->regionList_ = regionList;
   this->nameList_ = nameList;
} 

//===========================================================================//
TPO_PlaceRegions_c::TPO_PlaceRegions_c( 
      const TPO_PlaceRegions_c& placeRegions )
      :
      regionList_( placeRegions.regionList_ ),
      nameList_( placeRegions.nameList_ )
{
} 

//===========================================================================//
// Method         : ~TPO_PlaceRegions_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07//26/13 jeffr : Original
//===========================================================================//
TPO_PlaceRegions_c::~TPO_PlaceRegions_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07//26/13 jeffr : Original
//===========================================================================//
TPO_PlaceRegions_c& TPO_PlaceRegions_c::operator=( 
      const TPO_PlaceRegions_c& placeRegions )
{
   if( &placeRegions != this )
   {
      this->regionList_ = placeRegions.regionList_;
      this->nameList_ = placeRegions.nameList_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07//26/13 jeffr : Original
//===========================================================================//
bool TPO_PlaceRegions_c::operator==( 
      const TPO_PlaceRegions_c& placeRegions ) const
{
   return(( this->regionList_ == placeRegions.regionList_ ) &&
          ( this->nameList_ == placeRegions.nameList_ ) ? 
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07//26/13 jeffr : Original
//===========================================================================//
bool TPO_PlaceRegions_c::operator!=( 
      const TPO_PlaceRegions_c& placeRegions ) const
{
   return( !this->operator==( placeRegions ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07//26/13 jeffr : Original
//===========================================================================//
void TPO_PlaceRegions_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<place>\n" );
   spaceLen += 3;

   if( this->regionList_.IsValid( ))
   {
      for( size_t i = 0; i < this->regionList_.GetLength( ); ++i )
      {
         string srRegion;
         this->regionList_[i]->ExtractString( &srRegion );
         printHandler.Write( pfile, spaceLen, "<region> %s </region>\n", 
                                              TIO_SR_STR( srRegion ));
      }
   }

   if( this->nameList_.IsValid( ))
   {
      printHandler.Write( pfile, spaceLen, "<names>" );
      for( size_t i = 0; i < this->nameList_.GetLength( ); ++i )
      {
         const char* pszName = this->nameList_[i]->GetName( );
         printHandler.Write( pfile, 0, " \"%s\"", 
                                       TIO_PSZ_STR( pszName ));
      }
      printHandler.Write( pfile, 0, " </names>\n" );
   }

   spaceLen -= 3;
   printHandler.Write( pfile, spaceLen, "</place>\n" );
}
