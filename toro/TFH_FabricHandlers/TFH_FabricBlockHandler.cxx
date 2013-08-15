//===========================================================================//
// Purpose : Method definitions for the TFH_FabricBlockHandler class.
//
//           Public methods include:
//           - NewInstance, DeleteInstance, GetInstance, HasInstance
//           - GetLength
//           - GetGridBlockList
//           - Clear
//           - Add
//           - IsMember
//           - IsValid
//
//           Protected methods include:
//           - TFH_FabricBlockHandler_c, ~TFH_FabricBlockHandler_c
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

#include "TFH_Typedefs.h"
#include "TFH_FabricBlockHandler.h"

// Initialize the grid block handler "singleton" class, as needed
TFH_FabricBlockHandler_c* TFH_FabricBlockHandler_c::pinstance_ = 0;

//===========================================================================//
// Method         : TFH_FabricBlockHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TFH_FabricBlockHandler_c::TFH_FabricBlockHandler_c( 
      void )
      :
      gridBlockList_( TFH_GRID_BLOCK_LIST_DEF_CAPACITY )
{
   pinstance_ = 0;
}

//===========================================================================//
// Method         : ~TFH_FabricBlockHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TFH_FabricBlockHandler_c::~TFH_FabricBlockHandler_c( 
      void )
{
}

//===========================================================================//
// Method         : NewInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TFH_FabricBlockHandler_c::NewInstance( 
      void )
{
   pinstance_ = new TC_NOTHROW TFH_FabricBlockHandler_c;
}

//===========================================================================//
// Method         : DeleteInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TFH_FabricBlockHandler_c::DeleteInstance( 
      void )
{
   if( pinstance_ )
   {
      delete pinstance_;
      pinstance_ = 0;
   }
}

//===========================================================================//
// Method         : GetInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TFH_FabricBlockHandler_c& TFH_FabricBlockHandler_c::GetInstance(
      bool newInstance )
{
   if( !pinstance_ )
   {
      if( newInstance )
      {
         NewInstance( );
      }
   }
   return( *pinstance_ );
}

//===========================================================================//
// Method         : HasInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TFH_FabricBlockHandler_c::HasInstance(
      void )
{
   return( pinstance_ ? true : false );
}

//===========================================================================//
// Method         : GetLength
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
size_t TFH_FabricBlockHandler_c::GetLength(
      void ) const
{
   return( this->gridBlockList_.GetLength( )); 
}

//===========================================================================//
// Method         : GetGridBlockList
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
const TFH_GridBlockList_t& TFH_FabricBlockHandler_c::GetGridBlockList(
      void ) const
{
   return( this->gridBlockList_ );
}

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TFH_FabricBlockHandler_c::Clear( 
      void )
{
   this->gridBlockList_.Clear( );
}

//===========================================================================//
// Method         : Add
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TFH_FabricBlockHandler_c::Add( 
      const TGO_Point_c&    vpr_gridPoint,
            int             vpr_typeIndex,
            TFH_BlockType_t blockType,
      const string&         srBlockName,
      const string&         srMasterName )
{
   TFH_GridBlock_c gridBlock( vpr_gridPoint, vpr_typeIndex,
                              blockType, srBlockName, srMasterName );
   this->Add( gridBlock );
}

//===========================================================================//
void TFH_FabricBlockHandler_c::Add( 
      const TGO_Point_c&    vpr_gridPoint,
            int             vpr_typeIndex,
            TFH_BlockType_t blockType,
      const char*           pszBlockName,
      const char*           pszMasterName )
{
   TFH_GridBlock_c gridBlock( vpr_gridPoint, vpr_typeIndex,
                              blockType, pszBlockName, pszMasterName );
   this->Add( gridBlock );
}

//===========================================================================//
void TFH_FabricBlockHandler_c::Add( 
      const TFH_GridBlock_c& gridBlock )
{
   this->gridBlockList_.Add( gridBlock );
}

//===========================================================================//
// Method         : IsMember
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TFH_FabricBlockHandler_c::IsMember( 
      const TGO_Point_c& vpr_gridPoint ) const
{
   TFH_GridBlock_c gridBlock( vpr_gridPoint );
   return( this->IsMember( gridBlock ));
}

//===========================================================================//
bool TFH_FabricBlockHandler_c::IsMember( 
      const TFH_GridBlock_c& gridBlock ) const
{
   return( this->gridBlockList_.IsMember( gridBlock ));
}

//===========================================================================//
// Method         : IsValid
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TFH_FabricBlockHandler_c::IsValid(
      void ) const
{
   return( this->gridBlockList_.IsValid( ) ? true : false );
}
