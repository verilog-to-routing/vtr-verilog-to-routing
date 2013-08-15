//===========================================================================//
// Purpose : Declaration and inline definitions for the universal 
//           TFH_FabricSwitchBoxHandler singleton class. This class supports
//           switch box overrides for the VPR place and route tool.
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

#ifndef TFH_FABRIC_SWITCH_BOX_HANDLER_H
#define TFH_FABRIC_SWITCH_BOX_HANDLER_H

#include <cstdio>
using namespace std;

#include "TFH_Typedefs.h"
#include "TFH_SwitchBox.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
class TFH_FabricSwitchBoxHandler_c
{
public:

   static void NewInstance( void );
   static void DeleteInstance( void );
   static TFH_FabricSwitchBoxHandler_c& GetInstance( bool newInstance = true );
   static bool HasInstance( void );

   size_t GetLength( void ) const;
   const TFH_SwitchBoxList_t& GetSwitchBoxList( void ) const;

   void Clear( void );

   void Add( const TGO_Point_c& vpr_gridPoint,
             const string& srSwitchBoxName,
             const TC_MapTable_c& mapTable );
   void Add( const TGO_Point_c& vpr_gridPoint,
             const char* pszSwitchBoxName,
             const TC_MapTable_c& mapTable );
   void Add( const TFH_SwitchBox_c& switchBox );

   bool IsMember( const TGO_Point_c& vpr_gridPoint ) const;
   bool IsMember( const TFH_SwitchBox_c& switchBox ) const;

   bool IsValid( void ) const;

protected:

   TFH_FabricSwitchBoxHandler_c( void );
   ~TFH_FabricSwitchBoxHandler_c( void );

private:

   TFH_SwitchBoxList_t switchBoxList_;

   static TFH_FabricSwitchBoxHandler_c* pinstance_; // Define ptr for singleton instance

private:

   enum TFH_DefCapacity_e 
   { 
      TFH_SWITCH_BOX_LIST_DEF_CAPACITY = 64
   };
};

#endif
