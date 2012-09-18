//===========================================================================//
// Purpose : Method definitions for the TLO_Cell class.
//
//           Public methods include:
//           - TLO_Cell_c, ~TLO_Cell_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
//           - PrintBLIF
//           - AddPort
//           - AddPin
//           - FindPortCount
//           - HasPortType
//
//===========================================================================//

#include "TC_MinGrid.h"
#include "TC_StringUtils.h"

#include "TCT_Generic.h"

#include "TIO_PrintHandler.h"

#include "TLO_StringUtils.h"
#include "TLO_Cell.h"

//===========================================================================//
// Method         : TLO_Cell_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TLO_Cell_c::TLO_Cell_c( 
      void )
      :
      portList_( TLO_PORT_LIST_DEF_CAPACITY ),
      pinList_( TLO_PIN_LIST_DEF_CAPACITY ),
      source_( TLO_CELL_SOURCE_UNDEFINED )
{
   this->timing_.inputPinCap = 0.0;
   this->timing_.inputPinDelay = 0.0;
} 

//===========================================================================//
TLO_Cell_c::TLO_Cell_c( 
      const string&          srName,
            TLO_CellSource_t source )
      :
      srName_( srName ),
      portList_( TLO_PORT_LIST_DEF_CAPACITY ),
      pinList_( TLO_PIN_LIST_DEF_CAPACITY ),
      source_( source )
{
   this->timing_.inputPinCap = 0.0;
   this->timing_.inputPinDelay = 0.0;
} 

//===========================================================================//
TLO_Cell_c::TLO_Cell_c( 
      const char*            pszName,
            TLO_CellSource_t source )
      :
      srName_( TIO_PSZ_STR( pszName )),
      portList_( TLO_PORT_LIST_DEF_CAPACITY ),
      pinList_( TLO_PIN_LIST_DEF_CAPACITY ),
      source_( source )
{
   this->timing_.inputPinCap = 0.0;
   this->timing_.inputPinDelay = 0.0;
} 

//===========================================================================//
TLO_Cell_c::TLO_Cell_c( 
      const TLO_Cell_c& cell )
      :
      srName_( cell.srName_ ),
      srMasterName_( cell.srMasterName_ ),
      portList_( cell.portList_ ),
      pinList_( cell.pinList_ ),
      source_( cell.source_ )
{
   this->timing_.inputPinCap = cell.timing_.inputPinCap;
   this->timing_.inputPinDelay = cell.timing_.inputPinDelay;
} 

//===========================================================================//
// Method         : ~TLO_Cell_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TLO_Cell_c::~TLO_Cell_c( 
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
TLO_Cell_c& TLO_Cell_c::operator=( 
      const TLO_Cell_c& cell )
{
   if( &cell != this )
   {
      this->srName_ = cell.srName_;
      this->srMasterName_ = cell.srMasterName_;
      this->portList_ = cell.portList_;
      this->pinList_ = cell.pinList_;
      this->source_ = cell.source_;
      this->timing_.inputPinCap = cell.timing_.inputPinCap;
      this->timing_.inputPinDelay = cell.timing_.inputPinDelay;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator<
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TLO_Cell_c::operator<( 
      const TLO_Cell_c& cell ) const
{
   return(( TC_CompareStrings( this->srName_, cell.srName_ ) < 0 ) ? 
          true : false );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TLO_Cell_c::operator==( 
      const TLO_Cell_c& cell ) const
{
   return(( this->srName_ == cell.srName_ ) ? true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TLO_Cell_c::operator!=( 
      const TLO_Cell_c& cell ) const
{
   return( !this->operator==( cell ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TLO_Cell_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, 0, "\"%s\" ",
                                 TIO_SR_STR( this->srName_ ));

   if( this->srMasterName_.length( )) 
   {
      printHandler.Write( pfile, 0, "master = \"%s\" ",
                                    TIO_SR_STR( this->srMasterName_ ));
   }

   if( this->source_ != TLO_CELL_SOURCE_UNDEFINED )
   {
      string srSource;
      TLO_ExtractStringCellSource( this->source_, &srSource );
      printHandler.Write( pfile, 0, "source = %s ",
                                    TIO_SR_STR( srSource ));
   }

   if( TCTF_IsGT( this->timing_.inputPinCap, 0.0 ) ||
       TCTF_IsGT( this->timing_.inputPinDelay, 0.0 ))
   {
      TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
      unsigned int precision = MinGrid.GetPrecision( );

      printHandler.Write( pfile, 0, "cap_in = %0.*f ",
                                    precision, this->timing_.inputPinCap );
      printHandler.Write( pfile, 0, "delay_in = %0.*e ",
                                    precision + 1, this->timing_.inputPinDelay );
   }
   printHandler.Write( pfile, 0, ">\n" );

   if( this->portList_.IsValid( ))
   {
      this->portList_.Print( pfile, spaceLen );
   }
   if( this->pinList_.IsValid( ))
   {
      this->pinList_.Print( pfile, spaceLen );
   }
}

//===========================================================================//
// Method         : PrintBLIF
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TLO_Cell_c::PrintBLIF( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   switch( this->source_ )
   {
   case TLO_CELL_SOURCE_STANDARD:

      printHandler.Write( pfile, spaceLen, "%s\n",
                                           TIO_SR_STR( this->srName_ ));
      break;

   case TLO_CELL_SOURCE_CUSTOM:

      printHandler.Write( pfile, spaceLen, ".model %s\n",
                                           TIO_SR_STR( this->srName_ ));
      if( this->HasPortType( TC_TYPE_INPUT ))
      {
         printHandler.Write( pfile, spaceLen, ".input" );
         for( size_t i = 0; i < this->portList_.GetLength( ); ++i )
         {
	    const TLO_Port_c& port = *this->portList_[i];
	    if( port.GetType( ) == TC_TYPE_INPUT )
            {  
               printHandler.Write( pfile, spaceLen, " %s",
                                                    TIO_PSZ_STR( port.GetName( )));
            }
         }
         printHandler.Write( pfile, 0, "\n" );
      }
      if( this->HasPortType( TC_TYPE_OUTPUT ))
      {
         printHandler.Write( pfile, spaceLen, ".output" );
         for( size_t i = 0; i < this->portList_.GetLength( ); ++i )
         {
	    const TLO_Port_c& port = *this->portList_[i];
	    if( port.GetType( ) == TC_TYPE_OUTPUT )
	    {
               printHandler.Write( pfile, spaceLen, " %s",
                                                    TIO_PSZ_STR( port.GetName( )));
            }
         }
         printHandler.Write( pfile, 0, "\n" );
      }
      printHandler.Write( pfile, 0, ".end\n" );
      break;
   
   default:

      printHandler.Write( pfile, spaceLen, "// " );
      this->Print( pfile, spaceLen );
      break;
   }
}

//===========================================================================//
// Method         : AddPort
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TLO_Cell_c::AddPort(
      const string&       srName, 
            TC_TypeMode_t type )
{
   TLO_Port_c port( srName, type );
   this->AddPort( port );
}

//===========================================================================//
void TLO_Cell_c::AddPort(
      const char*         pszName,
            TC_TypeMode_t type )
{
   TLO_Port_c port( pszName, type );
   this->AddPort( port );
}

//===========================================================================//
void TLO_Cell_c::AddPort(
      const TLO_Port_c& port )
{
   this->portList_.Add( port );
}

//===========================================================================//
// Method         : AddPin
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TLO_Cell_c::AddPin(
      const TLO_Pin_t& pin )
{
   this->pinList_.Add( pin );
}

//===========================================================================//
// Method         : FindPortCount
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
size_t TLO_Cell_c::FindPortCount(
      TC_TypeMode_t type ) const
{
   size_t portCount = 0;

   for( size_t i = 0; i < this->portList_.GetLength( ); ++i )
   {
      const TLO_Port_c& port = *this->portList_[i];
      if( port.GetType( ) == type )
      {
	 ++portCount;
      }
   }   
   return( portCount );
}

//===========================================================================//
// Method         : HasPortType
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TLO_Cell_c::HasPortType(
      TC_TypeMode_t type ) const
{
   bool hasPortType = false;

   for( size_t i = 0; i < this->portList_.GetLength( ); ++i )
   {
      const TLO_Port_c& port = *this->portList_[i];
      if( port.GetType( ) == type )
      {
	 hasPortType = true;
	 break;
      }
   }   
   return( hasPortType );
}
