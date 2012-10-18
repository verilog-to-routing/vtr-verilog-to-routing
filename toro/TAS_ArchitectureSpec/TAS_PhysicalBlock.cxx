//===========================================================================//
// Purpose : Method definitions for the TAS_PhysicalBlock class.
//
//           Public methods include:
//           - TAS_PhysicalBlock_c, ~TAS_PhysicalBlock_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - PrintXML
//
//===========================================================================//

#include "TC_MinGrid.h"
#include "TC_StringUtils.h"
#include "TCT_Generic.h"

#include "TIO_PrintHandler.h"

#include "TAS_StringUtils.h"
#include "TAS_PhysicalBlock.h"

//===========================================================================//
// Method         : TAS_PhysicalBlock_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAS_PhysicalBlock_c::TAS_PhysicalBlock_c( 
      void )
      :
      height( 0 ),
      capacity( 0 ),
      numPB( 0 ),
      modelType( TAS_PHYSICAL_BLOCK_MODEL_UNDEFINED ),
      classType( TAS_CLASS_UNDEFINED ),
      modeNameList( TAS_MODE_NAME_LIST_DEF_CAPACITY ),
      modeList( TAS_MODE_LIST_DEF_CAPACITY ),
      physicalBlockList( TAS_PHYSICAL_BLOCK_LIST_DEF_CAPACITY ),
      interconnectList( TAS_INTERCONNECT_LIST_DEF_CAPACITY ),
      portList( TAS_PORT_LIST_DEF_CAPACITY ),
      pinAssignPattern( TAS_PIN_ASSIGN_PATTERN_UNDEFINED ),
      pinAssignList( TAS_PIN_ASSIGN_LIST_DEF_CAPACITY ),
      gridAssignList( TAS_GRID_ASSIGN_LIST_DEF_CAPACITY )
{
   this->fcIn.SetDir( TC_TYPE_INPUT );
   this->fcOut.SetDir( TC_TYPE_OUTPUT );

   this->sorted.modeList.SetCapacity( TAS_MODE_LIST_DEF_CAPACITY );
   this->sorted.physicalBlockList.SetCapacity( TAS_PHYSICAL_BLOCK_LIST_DEF_CAPACITY );

   this->usage_ = TAS_USAGE_UNDEFINED;

   this->pphysicalBlockParent_ = 0;
   this->pmodeParent_ = 0;
}

//===========================================================================//
TAS_PhysicalBlock_c::TAS_PhysicalBlock_c( 
      const string& srName_ )
      :
      srName( srName_ ),
      height( 0 ),
      capacity( 0 ),
      numPB( 0 ),
      modelType( TAS_PHYSICAL_BLOCK_MODEL_UNDEFINED ),
      classType( TAS_CLASS_UNDEFINED ),
      modeNameList( TAS_MODE_NAME_LIST_DEF_CAPACITY ),
      modeList( TAS_MODE_LIST_DEF_CAPACITY ),
      physicalBlockList( TAS_PHYSICAL_BLOCK_LIST_DEF_CAPACITY ),
      interconnectList( TAS_INTERCONNECT_LIST_DEF_CAPACITY ),
      portList( TAS_PORT_LIST_DEF_CAPACITY ),
      pinAssignPattern( TAS_PIN_ASSIGN_PATTERN_UNDEFINED ),
      pinAssignList( TAS_PIN_ASSIGN_LIST_DEF_CAPACITY ),
      gridAssignList( TAS_GRID_ASSIGN_LIST_DEF_CAPACITY )
{
   this->fcIn.SetDir( TC_TYPE_INPUT );
   this->fcOut.SetDir( TC_TYPE_OUTPUT );

   this->sorted.modeList.SetCapacity( TAS_MODE_LIST_DEF_CAPACITY );
   this->sorted.physicalBlockList.SetCapacity( TAS_PHYSICAL_BLOCK_LIST_DEF_CAPACITY );

   this->usage_ = TAS_USAGE_UNDEFINED;

   this->pphysicalBlockParent_ = 0;
   this->pmodeParent_ = 0;
}

//===========================================================================//
TAS_PhysicalBlock_c::TAS_PhysicalBlock_c( 
      const char* pszName )
      :
      srName( TIO_PSZ_STR( pszName )),
      height( 0 ),
      capacity( 0 ),
      numPB( 0 ),
      modelType( TAS_PHYSICAL_BLOCK_MODEL_UNDEFINED ),
      classType( TAS_CLASS_UNDEFINED ),
      modeNameList( TAS_MODE_NAME_LIST_DEF_CAPACITY ),
      modeList( TAS_MODE_LIST_DEF_CAPACITY ),
      physicalBlockList( TAS_PHYSICAL_BLOCK_LIST_DEF_CAPACITY ),
      interconnectList( TAS_INTERCONNECT_LIST_DEF_CAPACITY ),
      portList( TAS_PORT_LIST_DEF_CAPACITY ),
      pinAssignPattern( TAS_PIN_ASSIGN_PATTERN_UNDEFINED ),
      pinAssignList( TAS_PIN_ASSIGN_LIST_DEF_CAPACITY ),
      gridAssignList( TAS_GRID_ASSIGN_LIST_DEF_CAPACITY )
{
   this->fcIn.SetDir( TC_TYPE_INPUT );
   this->fcOut.SetDir( TC_TYPE_OUTPUT );

   this->sorted.modeList.SetCapacity( TAS_MODE_LIST_DEF_CAPACITY );
   this->sorted.physicalBlockList.SetCapacity( TAS_PHYSICAL_BLOCK_LIST_DEF_CAPACITY );

   this->usage_ = TAS_USAGE_UNDEFINED;

   this->pphysicalBlockParent_ = 0;
   this->pmodeParent_ = 0;
}

//===========================================================================//
TAS_PhysicalBlock_c::TAS_PhysicalBlock_c( 
      const TAS_PhysicalBlock_c& physicalBlock )
      :
      srName( physicalBlock.srName ),
      height( physicalBlock.height ),
      capacity( physicalBlock.capacity ),
      numPB( physicalBlock.numPB ),
      modelType( physicalBlock.modelType ),
      srModelName( physicalBlock.srModelName ),
      classType( physicalBlock.classType ),
      fcIn( physicalBlock.fcIn ),
      fcOut( physicalBlock.fcOut ),
      modeNameList( physicalBlock.modeNameList ),
      modeList( physicalBlock.modeList ),
      physicalBlockList( physicalBlock.physicalBlockList ),
      interconnectList( physicalBlock.interconnectList ),
      portList( physicalBlock.portList ),
      timingDelayLists( physicalBlock.timingDelayLists ),
      pinAssignPattern( physicalBlock.pinAssignPattern ),
      pinAssignList( physicalBlock.pinAssignList ),
      gridAssignList( physicalBlock.gridAssignList )
{
   this->sorted.modeList = physicalBlock.sorted.modeList;
   this->sorted.physicalBlockList =physicalBlock.sorted.physicalBlockList;

   this->dims_ = physicalBlock.dims_;
   this->origin_ = physicalBlock.origin_;

   this->usage_ = physicalBlock.usage_;

   this->pphysicalBlockParent_ = 0;
   this->pmodeParent_ = 0;
}

//===========================================================================//
// Method         : ~TAS_PhysicalBlock_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAS_PhysicalBlock_c::~TAS_PhysicalBlock_c( 
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
TAS_PhysicalBlock_c& TAS_PhysicalBlock_c::operator=( 
      const TAS_PhysicalBlock_c& physicalBlock )
{
   if( &physicalBlock != this )
   {
      this->srName = physicalBlock.srName;
      this->height = physicalBlock.height;
      this->capacity = physicalBlock.capacity;
      this->numPB = physicalBlock.numPB;
      this->modelType = physicalBlock.modelType;
      this->srModelName = physicalBlock.srModelName;
      this->classType = physicalBlock.classType;
      this->fcIn = physicalBlock.fcIn;
      this->fcOut = physicalBlock.fcOut;
      this->modeNameList = physicalBlock.modeNameList;
      this->modeList = physicalBlock.modeList;
      this->physicalBlockList = physicalBlock.physicalBlockList;
      this->interconnectList = physicalBlock.interconnectList;
      this->portList = physicalBlock.portList;
      this->timingDelayLists = physicalBlock.timingDelayLists;
      this->pinAssignPattern = physicalBlock.pinAssignPattern;
      this->pinAssignList = physicalBlock.pinAssignList;
      this->gridAssignList = physicalBlock.gridAssignList;
      this->sorted.modeList = physicalBlock.sorted.modeList;
      this->sorted.physicalBlockList = physicalBlock.sorted.physicalBlockList;
      this->dims_ = physicalBlock.dims_;
      this->origin_ = physicalBlock.origin_;
      this->usage_ = physicalBlock.usage_;
      this->pphysicalBlockParent_ = 0;
      this->pmodeParent_ = 0;
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
bool TAS_PhysicalBlock_c::operator==( 
      const TAS_PhysicalBlock_c& physicalBlock ) const
{
   return(( this->srName == physicalBlock.srName ) ? true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAS_PhysicalBlock_c::operator!=( 
      const TAS_PhysicalBlock_c& physicalBlock ) const
{
   return( !this->operator==( physicalBlock ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAS_PhysicalBlock_c::Print( 
      FILE*           pfile,
      size_t          spaceLen,
      size_t          hierLevel,
      TAS_ModeList_t* pmodeList ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   const char* pszUsage = "";
   switch( this->usage_ )
   {
   case TAS_USAGE_PHYSICAL_BLOCK: pszUsage = "pb"; break;
   case TAS_USAGE_INPUT_OUTPUT:   pszUsage = "io"; break; 
   case TAS_USAGE_UNDEFINED:      pszUsage = ( this->capacity ? "io" : "pb" ); break;
   }
   printHandler.Write( pfile, spaceLen, "<%s \"%s\" ", TIO_PSZ_STR( pszUsage ),
                                                       TIO_SR_STR( this->srName ));
   if( this->height > 0 )
   {
      printHandler.Write( pfile, 0, "height = %u ", this->height );
   }
   if( this->capacity > 0 )
   {
      printHandler.Write( pfile, 0, "capacity = %u ", this->capacity );
   }
   if( this->numPB > 0 )
   {
      printHandler.Write( pfile, 0, "count = %u ", this->numPB );
   }
   if( this->srModelName.length( ))
   {
      printHandler.Write( pfile, 0, "cell = \"%s\" ", TIO_SR_STR( this->srModelName ));
   }

   if(( this->classType == TAS_CLASS_LUT ) ||
      ( this->classType == TAS_CLASS_FLIPFLOP ) ||
      ( this->classType == TAS_CLASS_MEMORY ))
   {
      string srClassType;
      TAS_ExtractStringClassType( this->classType, &srClassType );
      printHandler.Write( pfile, 0, "class = %s ",
 	                            TIO_SR_STR( srClassType ));
   }

   this->fcIn.Print( pfile, 0 );
   this->fcOut.Print( pfile, 0 );

   if( this->dims_.IsValid( ) || this->origin_.IsValid( ))
   {
      printHandler.Write( pfile, 0, "\n    " ); 

      if( this->dims_.IsValid( ))
      {
         printHandler.Write( pfile, 0, "size = %0.*f %0.*f ", precision, this->dims_.width, 
                                                              precision, this->dims_.height );
      }
      if( this->origin_.IsValid( ))
      {
         printHandler.Write( pfile, 0, "origin = %0.*f %0.*f ", precision, this->origin_.x, 
                                                                precision, this->origin_.y );
      }
   }
   printHandler.Write( pfile, 0, "/>\n" );
   spaceLen += 3;
   
   if( this->modeNameList.IsValid( ))
   {
      const char* pszS = ( this->modeNameList.GetLength( ) > 1 ? "s" : "" );
      printHandler.Write( pfile, spaceLen, "<mode%s ", TIO_PSZ_STR( pszS ));
      for( size_t i = 0; i < this->modeNameList.GetLength( ); ++i )
      {
         const TC_Name_c& modeName = *this->modeNameList[i];
         printHandler.Write( pfile, 0, "\"%s\" ", TIO_PSZ_STR( modeName.GetName( )));
      }
      printHandler.Write( pfile, 0, "/>\n" );
   }
   if( this->modeList.IsValid( ))
   {
      const char* pszS = ( this->modeList.GetLength( ) > 1 ? "s" : "" );
      printHandler.Write( pfile, spaceLen, "<mode%s ", TIO_PSZ_STR( pszS ));
      for( size_t i = 0; i < this->modeList.GetLength( ); ++i )
      {
         const TAS_Mode_c& mode = *this->modeList[i];
         printHandler.Write( pfile, 0, "\"%s\" ", TIO_SR_STR( mode.srName ));
      }
      printHandler.Write( pfile, 0, "/>\n" );
   }
   if( !this->modeNameList.IsValid( ) &&
       !this->modeList.IsValid( ) &&
       this->physicalBlockList.IsValid( ))
   {
      printHandler.Write( pfile, spaceLen, "<mode \"%s\" />\n", TIO_SR_STR( this->srName ));
   }

   this->portList.Print( pfile, spaceLen );
   this->timingDelayLists.Print( pfile, spaceLen );

   if( this->pinAssignList.IsValid( ))
   {
      this->pinAssignList.Print( pfile, spaceLen );
   }
   else if( this->pinAssignPattern != TAS_PIN_ASSIGN_PATTERN_UNDEFINED )
   {
      string srPattern;
      TAS_ExtractStringPinAssignPatternType( this->pinAssignPattern, &srPattern );
      printHandler.Write( pfile, spaceLen, "<pin_assign %s />\n", TIO_SR_STR( srPattern ));
   }
   this->gridAssignList.Print( pfile, spaceLen );

   spaceLen -= 3;
   printHandler.Write( pfile, spaceLen, "</%s>\n", pszUsage );

   for( size_t i = 0; i < this->modeList.GetLength( ); ++i )
   {
      const TAS_Mode_c& mode = *this->modeList[i];
      for( size_t j = 0; j < mode.physicalBlockList.GetLength( ); ++j )
      {
         const TAS_PhysicalBlock_c& physicalBlock = *mode.physicalBlockList[j];
         physicalBlock.Print( pfile, spaceLen, hierLevel+1, pmodeList );
      }
   }
   for( size_t i = 0; i < this->physicalBlockList.GetLength( ); ++i )
   {
      const TAS_PhysicalBlock_c& physicalBlock = *this->physicalBlockList[i];
      physicalBlock.Print( pfile, spaceLen, hierLevel+1, pmodeList );
   }

   if( this->modeList.IsValid( ))
   {
      for( size_t i = 0; i < this->modeList.GetLength( ); ++i )
      {
         const TAS_Mode_c& mode = *this->modeList[i];
	 if( pmodeList )
         { 
            if( pmodeList->IsMember( mode ))
	       continue;

            pmodeList->Add( mode );
	 }
         mode.Print( pfile, spaceLen, hierLevel+1, pmodeList );
      }
   }
   else if( this->physicalBlockList.IsValid( ))
   {
      TAS_Mode_c mode( this->srName );
      mode.physicalBlockList = this->physicalBlockList;
      mode.interconnectList = this->interconnectList;
      mode.Print( pfile, spaceLen, hierLevel+1, pmodeList );
   }
}

//===========================================================================//
// Method         : PrintXML
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAS_PhysicalBlock_c::PrintXML( 
      void ) const
{
   FILE* pfile = 0;
   size_t spaceLen = 0;

   this->PrintXML( pfile, spaceLen );
}

//===========================================================================//
void TAS_PhysicalBlock_c::PrintXML( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<pb_type name=\"%s\" ",
		                        TIO_SR_STR( this->srName ));
   if( this->height > 0 )
   {
      printHandler.Write( pfile, 0, "height=\"%u\" ",
                                           this->height );
   }
   if( this->capacity > 0 )
   {
      printHandler.Write( pfile, 0, "capacity=\"%u\" ",
                                           this->capacity );
   }
   if( this->srModelName.length( ))
   {
      printHandler.Write( pfile, 0, "blif_model=\"%s\" ",
	 	                    TIO_SR_STR( this->srModelName ));
   }
   if( this->numPB > 0 )
   {
      printHandler.Write( pfile, 0, "num_pb=\"%u\" ",
	 	                    this->numPB );
   }

   if(( this->classType == TAS_CLASS_LUT ) ||
      ( this->classType == TAS_CLASS_FLIPFLOP ) ||
      ( this->classType == TAS_CLASS_MEMORY ))
   {
      string srClassType;
      TAS_ExtractStringClassType( this->classType, &srClassType );
      printHandler.Write( pfile, 0, "class=\"%s\" ",
 	                            TIO_SR_STR( srClassType ));
   }

   printHandler.Write( pfile, 0, ">\n" );
   spaceLen += 3;

   this->fcIn.PrintXML( pfile, spaceLen );
   this->fcOut.PrintXML( pfile, spaceLen );

   if( this->modeNameList.IsValid( ))
   {
      for( size_t i = 0; i < this->modeNameList.GetLength( ); ++i )
      {
         const TC_Name_c& modeName = *this->modeNameList[i];
         printHandler.Write( pfile, spaceLen, "<mode \"%s\" />\n", TIO_PSZ_STR( modeName.GetName( )));
      }
   }
   if( this->modeList.IsValid( ))
   {
      for( size_t i = 0; i < this->modeList.GetLength( ); ++i )
      {
         this->modeList[i]->PrintXML( pfile, spaceLen );
      }
   }

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

   for( size_t i = 0 ; i < this->portList.GetLength( ); ++i )
   {
      const TLO_Port_c& port = *this->portList[i];

      string srPortType;
      TC_ExtractStringTypeMode( port.GetType( ), &srPortType );

      printHandler.Write( pfile, spaceLen, "<%s name=\"%s\" num_pins=\"%u\"",
	 	                           TIO_SR_STR( srPortType ),
	 	                           TIO_PSZ_STR( port.GetName( )),
			                   port.GetCount( ));
      if( port.GetClass( ) && *port.GetClass( ))
      {
         printHandler.Write( pfile, 0, " port_class=\"%s\"",
	 	                       TIO_PSZ_STR( port.GetClass( )));
      }
      if( port.IsEquivalent( ))
      {
         printHandler.Write( pfile, 0, " equivalent=\"%s\"",
                                       TIO_BOOL_VAL( port.IsEquivalent( )));
      }
      printHandler.Write( pfile, 0, "/>\n" );
   }

   this->timingDelayLists.PrintXML( pfile, spaceLen );

   string srPinAssignPattern;
   TAS_ExtractStringPinAssignPatternType( this->pinAssignPattern, &srPinAssignPattern );
   if( this->pinAssignPattern == TAS_PIN_ASSIGN_PATTERN_SPREAD )
   {
      printHandler.Write( pfile, spaceLen, "<pinlocations pattern=\"%s\">\n",
                                           TIO_SR_STR( srPinAssignPattern ));
      printHandler.Write( pfile, spaceLen, "</pinlocations>\n" );
   }
   else if( this->pinAssignPattern == TAS_PIN_ASSIGN_PATTERN_CUSTOM )
   {
      printHandler.Write( pfile, spaceLen, "<pinlocations pattern=\"%s\">\n",
                                           TIO_SR_STR( srPinAssignPattern ));
      for( size_t i = 0; i < this->pinAssignList.GetLength( ); ++i )
      {
         this->pinAssignList[i]->PrintXML( pfile, spaceLen + 3 );
      }
      printHandler.Write( pfile, spaceLen, "</pinlocations>\n" );
   }

   if( this->gridAssignList.IsValid( ))
   {
      printHandler.Write( pfile, spaceLen, "<gridlocations>\n" );
      for( size_t i = 0; i < this->gridAssignList.GetLength( ); ++i )
      {
         this->gridAssignList[i]->PrintXML( pfile, spaceLen + 3 );
      }
      printHandler.Write( pfile, spaceLen, "</gridlocations>\n" );
   }

   spaceLen -= 3;
   printHandler.Write( pfile, spaceLen, "</pb_type>\n" );
}
