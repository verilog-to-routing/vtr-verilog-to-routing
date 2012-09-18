//===========================================================================//
// Purpose : Method definitions for the TPO_HierMap class.
//
//           Public methods include:
//           - TPO_HierMap_c, ~TPO_HierMap_c
//           - operator=
//           - operator==, operator!=
//           - Print
//
//===========================================================================//

#include "TIO_PrintHandler.h"

#include "TPO_HierMap.h"

//===========================================================================//
// Method         : TPO_HierMap_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TPO_HierMap_c::TPO_HierMap_c( 
      void )
      :
      hierNameList_( TPO_HIER_NAME_LIST_DEF_CAPACITY )
{
}

//===========================================================================//
TPO_HierMap_c::TPO_HierMap_c( 
      const string&             srInstName,
      const TPO_HierNameList_t& hierNameList )
      :
      srInstName_( srInstName ),
      hierNameList_( hierNameList )
{
}

//===========================================================================//
TPO_HierMap_c::TPO_HierMap_c( 
      const char*               pszInstName,
      const TPO_HierNameList_t& hierNameList )
      :
      srInstName_( TIO_PSZ_STR( pszInstName )),
      hierNameList_( hierNameList )
{
}

//===========================================================================//
TPO_HierMap_c::TPO_HierMap_c( 
      const TPO_HierMap_c& hierMap )
      :
      srInstName_( hierMap.srInstName_ ),
      hierNameList_( hierMap.hierNameList_ )
{
}

//===========================================================================//
// Method         : ~TPO_HierMap_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TPO_HierMap_c::~TPO_HierMap_c( 
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
TPO_HierMap_c& TPO_HierMap_c::operator=( 
      const TPO_HierMap_c& hierMap )
{
   if( &hierMap != this )
   {
      this->srInstName_ = hierMap.srInstName_;
      this->hierNameList_ = hierMap.hierNameList_;
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
bool TPO_HierMap_c::operator==( 
      const TPO_HierMap_c& hierMap ) const
{
   return(( this->srInstName_ == hierMap.srInstName_ ) &&
          ( this->hierNameList_ == hierMap.hierNameList_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TPO_HierMap_c::operator!=( 
      const TPO_HierMap_c& hierMap ) const
{
   return( !this->operator==( hierMap ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TPO_HierMap_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<pack \"%s\" >\n", 
                                        TIO_SR_STR( this->srInstName_ ));
   spaceLen += 3;

   printHandler.Write( pfile, spaceLen, "<hier " );
   for( size_t i = 0; i < this->hierNameList_.GetLength( ); ++i )
   {
      const TC_Name_c& hierName = *this->hierNameList_[i];
      printHandler.Write( pfile, 0, "\"%s\" ", 
                                    TIO_PSZ_STR( hierName.GetName( )));
   }
   printHandler.Write( pfile, 0, ">\n" );

   spaceLen -= 3;
   printHandler.Write( pfile, spaceLen, "/>\n" );
}
