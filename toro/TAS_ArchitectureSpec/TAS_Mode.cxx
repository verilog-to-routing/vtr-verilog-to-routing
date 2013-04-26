//===========================================================================//
// Purpose : Method definitions for the TAS_Mode class.
//
//           Public methods include:
//           - TAS_Mode_c, ~TAS_Mode_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - PrintXML
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012-2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com) //
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

#include "TAS_Mode.h"

//===========================================================================//
// Method         : TAS_Mode_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAS_Mode_c::TAS_Mode_c( 
      void )
      :
      physicalBlockList( TAS_PHYSICAL_BLOCK_LIST_DEF_CAPACITY ),
      interconnectList( TAS_INTERCONNECT_LIST_DEF_CAPACITY )
{
   this->sorted.physicalBlockList.SetCapacity( TAS_PHYSICAL_BLOCK_LIST_DEF_CAPACITY );

   this->pphysicalBlockParent_ = 0;
}

//===========================================================================//
TAS_Mode_c::TAS_Mode_c( 
      const string& srName_ )
      :
      srName( srName_ ),
      physicalBlockList( TAS_PHYSICAL_BLOCK_LIST_DEF_CAPACITY ),
      interconnectList( TAS_INTERCONNECT_LIST_DEF_CAPACITY )
{
   this->sorted.physicalBlockList.SetCapacity( TAS_PHYSICAL_BLOCK_LIST_DEF_CAPACITY );

   this->pphysicalBlockParent_ = 0;
}

//===========================================================================//
TAS_Mode_c::TAS_Mode_c( 
      const char* pszName )
      :
      srName( TIO_PSZ_STR( pszName )),
      physicalBlockList( TAS_PHYSICAL_BLOCK_LIST_DEF_CAPACITY ),
      interconnectList( TAS_INTERCONNECT_LIST_DEF_CAPACITY )
{
   this->sorted.physicalBlockList.SetCapacity( TAS_PHYSICAL_BLOCK_LIST_DEF_CAPACITY );

   this->pphysicalBlockParent_ = 0;
}

//===========================================================================//
TAS_Mode_c::TAS_Mode_c( 
      const TAS_Mode_c& mode )
      :
      srName( mode.srName ),
      physicalBlockList( mode.physicalBlockList ),
      interconnectList( mode.interconnectList )
{
   this->sorted.physicalBlockList = mode.sorted.physicalBlockList;

   this->pphysicalBlockParent_ = 0;
}

//===========================================================================//
// Method         : ~TAS_Mode_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAS_Mode_c::~TAS_Mode_c( 
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
TAS_Mode_c& TAS_Mode_c::operator=( 
      const TAS_Mode_c& mode )
{
   if( &mode != this )
   {
      this->srName = mode.srName;
      this->physicalBlockList = mode.physicalBlockList;
      this->interconnectList = mode.interconnectList;
      this->sorted.physicalBlockList = mode.sorted.physicalBlockList;
      this->pphysicalBlockParent_ = 0;
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
bool TAS_Mode_c::operator==( 
      const TAS_Mode_c& mode ) const
{
   return(( this->srName == mode.srName ) ? true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAS_Mode_c::operator!=( 
      const TAS_Mode_c& mode ) const
{
   return( !this->operator==( mode ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAS_Mode_c::Print( 
      FILE*           pfile,
      size_t          spaceLen,
      size_t          hierLevel,
      TAS_ModeList_t* pmodeList ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<model name=\"%s\">\n", TIO_SR_STR( this->srName ));
   spaceLen += 3;

   for( size_t i = 0; i < this->physicalBlockList.GetLength( ); ++i )
   {
      const TAS_PhysicalBlock_c& physicalBlock = *this->physicalBlockList[i];
      printHandler.Write( pfile, spaceLen, "<pb name=\"%s\"/>\n", TIO_PSZ_STR( physicalBlock.GetName( )));
   }

   if( this->interconnectList.IsValid( ))
   {
      this->interconnectList.Print( pfile, spaceLen );
   }

   spaceLen -= 3;
   printHandler.Write( pfile, spaceLen, "</model>\n" );

   for( size_t i = 0; i < this->physicalBlockList.GetLength( ); ++i )
   {
      const TAS_PhysicalBlock_c& physicalBlock = *this->physicalBlockList[i];
      for( size_t j = 0; j < physicalBlock.modeList.GetLength( ); ++j )
      {
         const TAS_Mode_c& mode = *physicalBlock.modeList[j];
         if( pmodeList )
         { 
            if( pmodeList->IsMember( mode ))
               continue;

            pmodeList->Add( mode );
         }
         mode.Print( pfile, spaceLen, hierLevel+1 );
      }
   }
}

//===========================================================================//
// Method         : PrintXML
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAS_Mode_c::PrintXML( 
      void ) const
{
   FILE* pfile = 0;
   size_t spaceLen = 0;

   this->PrintXML( pfile, spaceLen );
}

//===========================================================================//
void TAS_Mode_c::PrintXML( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<mode name=\"%s\">\n",
                                        TIO_SR_STR( this->srName ));
   spaceLen += 3;

   if( this->physicalBlockList.IsValid( ))
   {
      for( size_t i = 0; i < this->physicalBlockList.GetLength( ); ++i )
      {
         this->physicalBlockList[i]->PrintXML( pfile, spaceLen );
      }
   }
   if( this->interconnectList.IsValid( ))
   {
      printHandler.Write( pfile, spaceLen, "<interconnect>\n" );
      for( size_t i = 0; i < this->interconnectList.GetLength( ); ++i )
      {
         this->interconnectList[i]->PrintXML( pfile, spaceLen + 3 );
      }
      printHandler.Write( pfile, spaceLen, "</interconnect>\n" );
   }
   spaceLen -= 3;
   printHandler.Write( pfile, spaceLen, "</mode>\n" );
}
