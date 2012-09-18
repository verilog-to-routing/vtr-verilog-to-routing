//===========================================================================//
// Purpose : Method definitions for the TCD_CircuitDesign class.
//
//           Public methods include:
//           - TCD_CircuitDesign_c, ~TCD_CircuitDesign_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - PrintBLIF
//           - InitDefaults
//           - InitValidate
//           - IsValid
//
//           Private methods include:
//           - InitDefaultsNetList_
//           - InitDefaultsNetNameList_
//           - InitValidateNetList_
//           - InitValidateInstList_
//
//===========================================================================//

#include "TIO_PrintHandler.h"

#include "TCD_CircuitDesign.h"

//===========================================================================//
// Method         : TCD_CircuitDesign_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TCD_CircuitDesign_c::TCD_CircuitDesign_c( 
      void )
      :
      blockList( TCD_BLOCK_LIST_DEF_CAPACITY ),
      portList( TCD_PORT_LIST_DEF_CAPACITY ),
      instList( TCD_INST_LIST_DEF_CAPACITY ),
      cellList( TCD_CELL_LIST_DEF_CAPACITY ),
      netList( TCD_NET_LIST_DEF_CAPACITY ),
      netNameList( TCD_NET_NAME_LIST_DEF_CAPACITY )
{
} 

//===========================================================================//
TCD_CircuitDesign_c::TCD_CircuitDesign_c( 
      const TCD_CircuitDesign_c& circuitDesign )
      :
      srName( circuitDesign.srName ),
      blockList( circuitDesign.blockList ),
      portList( circuitDesign.portList ),
      instList( circuitDesign.instList ),
      cellList( circuitDesign.cellList ),
      netList( circuitDesign.netList ),
      netNameList( circuitDesign.netNameList )
{
} 

//===========================================================================//
// Method         : ~TCD_CircuitDesign_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TCD_CircuitDesign_c::~TCD_CircuitDesign_c( 
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
TCD_CircuitDesign_c& TCD_CircuitDesign_c::operator=( 
      const TCD_CircuitDesign_c& circuitDesign )
{
   if( &circuitDesign != this )
   {
      this->srName = circuitDesign.srName;
      this->blockList = circuitDesign.blockList;
      this->portList = circuitDesign.portList;
      this->instList = circuitDesign.instList;
      this->cellList = circuitDesign.cellList;
      this->netList = circuitDesign.netList;
      this->netNameList = circuitDesign.netNameList;
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
bool TCD_CircuitDesign_c::operator==( 
      const TCD_CircuitDesign_c& circuitDesign ) const
{
   return(( this->srName == circuitDesign.srName ) && 
          ( this->blockList == circuitDesign.blockList ) && 
          ( this->portList == circuitDesign.portList ) && 
          ( this->instList == circuitDesign.instList ) && 
          ( this->cellList == circuitDesign.cellList ) &&
          ( this->netList == circuitDesign.netList ) &&
          ( this->netNameList == circuitDesign.netNameList ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TCD_CircuitDesign_c::operator!=( 
      const TCD_CircuitDesign_c& circuitDesign ) const
{
   return( !this->operator==( circuitDesign ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TCD_CircuitDesign_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Write( pfile, spaceLen, "<circuit \"%s\" >\n",
                                        TIO_SR_STR( this->srName ));
   spaceLen += 3;

   for( size_t i = 0; i < this->blockList.GetLength( ); ++i )
   {
      printHandler.Write( pfile, spaceLen, "<block " );
      this->blockList[i]->Print( pfile, spaceLen + 3 );
      printHandler.Write( pfile, spaceLen, "</block>\n" );
   }
   for( size_t i = 0; i < this->portList.GetLength( ); ++i )
   {
      printHandler.Write( pfile, spaceLen, "<port " );
      this->portList[i]->Print( pfile, spaceLen + 3 );
      printHandler.Write( pfile, spaceLen, "</port>\n" );
   }
   for( size_t i = 0; i < this->instList.GetLength( ); ++i )
   {
      printHandler.Write( pfile, spaceLen, "<inst " );
      this->instList[i]->Print( pfile, spaceLen + 3 );
      printHandler.Write( pfile, spaceLen, "</inst>\n" );
   }
   for( size_t i = 0; i < this->cellList.GetLength( ); ++i )
   {
      printHandler.Write( pfile, spaceLen, "<cell " );
      this->cellList[i]->Print( pfile, spaceLen + 3 );
      printHandler.Write( pfile, spaceLen, "</cell>\n" );
   }
   for( size_t i = 0; i < this->netList.GetLength( ); ++i )
   {
      printHandler.Write( pfile, spaceLen, "<net " );
      this->netList[i]->Print( pfile, spaceLen + 3 );
      printHandler.Write( pfile, spaceLen, "</net>\n" );
   }

   spaceLen -= 3;
   printHandler.Write( pfile, spaceLen, "</circuit>\n" );
}

//===========================================================================//
// Method         : PrintBLIF
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TCD_CircuitDesign_c::PrintBLIF( 
      void ) const
{
   FILE* pfile = 0;
   size_t spaceLen = 0;

   this->PrintBLIF( pfile, spaceLen );
}

//===========================================================================//
void TCD_CircuitDesign_c::PrintBLIF( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Write( pfile, spaceLen, ".model %s\n",
                                        TIO_SR_STR( this->srName ));

   printHandler.Write( pfile, spaceLen, ".inputs" );
   for( size_t i = 0; i < this->portList.GetLength( ); ++i )
   {
      const TPO_Port_t& port = *this->portList[i];
      const TPO_PinList_t& pinList = port.GetPinList( );
      const TPO_Pin_t& pin = *pinList[0];
      if( pin.GetType( ) == TC_TYPE_OUTPUT )
      {
         printHandler.Write( pfile, spaceLen, " %s",
                                              TIO_PSZ_STR( pin.GetName( )));
      }
   }
   printHandler.Write( pfile, 0, "\n" );

   printHandler.Write( pfile, spaceLen, ".outputs" );
   for( size_t j = 0; j < this->portList.GetLength( ); ++j )
   {
      const TPO_Port_t& port = *this->portList[j];
      const TPO_PinList_t& pinList = port.GetPinList( );
      const TPO_Pin_t& pin = *pinList[0];
      if( pin.GetType( ) == TC_TYPE_INPUT )
      {
         printHandler.Write( pfile, spaceLen, " %s",
                                              TIO_PSZ_STR( pin.GetName( )));
      }
   }
   printHandler.Write( pfile, 0, "\n" );

   for( size_t i = 0; i < this->instList.GetLength( ); ++i )
   {
      if( this->instList[i]->GetSource( ) == TPO_INST_SOURCE_LATCH )
      {
         this->instList[i]->PrintBLIF( pfile, spaceLen );
      }
   }
   for( size_t i = 0; i < this->instList.GetLength( ); ++i )
   {
      if( this->instList[i]->GetSource( ) == TPO_INST_SOURCE_NAMES )
      {
         this->instList[i]->PrintBLIF( pfile, spaceLen );
      }
   }
   for( size_t i = 0; i < this->instList.GetLength( ); ++i )
   {
      if( this->instList[i]->GetSource( ) == TPO_INST_SOURCE_SUBCKT )
      {
         this->instList[i]->PrintBLIF( pfile, spaceLen );
      }
   }
   printHandler.Write( pfile, spaceLen, ".end\n" );

   for( size_t i = 0; i < this->cellList.GetLength( ); ++i )
   {
      this->cellList[i]->PrintBLIF( pfile, spaceLen );
   }
}

//===========================================================================//
// Method         : InitDefaults
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TCD_CircuitDesign_c::InitDefaults( 
      const string& srDefaultBaseName )
{
   bool ok = true;

   // Set default model name (if no ".model" defined) based on default name
   // (default name is assumed to be based on the input options file name)
   if( !this->srName.length( ))
   {
      this->srName = srDefaultBaseName;
   }

   if( !this->netList.IsValid( ))
   {
      this->InitDefaultsNetList_( this->instList, this->portList, this->cellList, 
                                  &this->netList );
   }
   ok = this->InitValidateNetList_( &this->netList );
   if( ok )
   {
      this->InitDefaultsNetNameList_( this->instList, this->portList,
                                      &this->netList, &this->netNameList );
   }
   return( ok );
}

//===========================================================================//
// Method         : InitValidate
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TCD_CircuitDesign_c::InitValidate( 
      void ) 
{
   bool ok = true;

   if( ok )
   {
      // Validate unique instance names
      // (based on ".names <input_1> <input_2> <input_3> ... <input_n> <output>")
      // (based on ".latch <input> <output> [<type> <control>] [<init_val>]")
      bool uniquifyShowWarning = true;
      bool uniquifyShowError = false;
      ok = this->instList.Uniquify( uniquifyShowWarning, uniquifyShowError,
                                    "instance list" );
   }

   if( ok )
   {
      ok = this->InitValidateInstList_( this->instList,
                                        this->cellList );
   }

// TBD ???  Given: .latch <input> <output> [<type> <control>] [<init_val>]
// TBD ???  can we validate input pin names exist in pin list (with matching type)?
// TBD ???  can we validate output pin name exists in pin list (with matching type)?

// TBD ???  Given: .latch <input> <output> [<type> <control>] [<init_val>]
// TBD ???  can we validate input pin name exists in pin list (with matching type)?
// TBD ???  can we validate output pin name exists in pin list (with matching type)?
// TBD ???  can we validate clock pin name exists in pin list (with matching type)?

   if( ok )
   {
      // Validate unique cell names
      // (based on ".subckt <model_name> <port>=<input|output> ...")
      bool uniquifyShowWarning = true;
      bool uniquifyShowError = false;
      ok = this->cellList.Uniquify( uniquifyShowWarning, uniquifyShowError,
                                    "cell list" );
   }

   if( ok )
   {
      // Validate unique input/output port names
      // (based on ".inputs <input_1> <input_2> <input_3> ... <input_n>")
      // (based on ".outputs <output_1> <output_2> <output_3> ... <output_n>")
      // (based on ".clock <clock_1> <clock_2> <clock_3> ... <clock_n>")
      bool uniquifyShowWarning = true;
      bool uniquifyShowError = false;
      ok = this->portList.Uniquify( uniquifyShowWarning, uniquifyShowError,
                                    "port list" );
   }

   if( ok )
   {
      // Validate unique cell pin names
      for( size_t i = 0; i < this->cellList.GetLength( ); ++i )
      {
         TLO_Cell_c* pcell = this->cellList[i];
         const char* pszCellName = pcell->GetName( );
         TLO_PinList_t pinList_ = pcell->GetPinList( );

         bool uniquifyShowWarning = true;
         bool uniquifyShowError = false;
         ok = pinList_.Uniquify( uniquifyShowWarning, uniquifyShowError,
                                 "cell", pszCellName, "pin" );
         if( !ok )
            break;

         if( pinList_.GetLength( ) != pcell->GetPinList( ).GetLength( ))
	 {
	    pcell->SetPinList( pinList_ );
         }
      }
   }

// TBD ???... May need to validate NetList's net...
// TBD ???... Each net must have exactly one OUTPUT port
// TBD ???... Each net must be one or more INPUT ports

   return( ok );
}

//===========================================================================//
// Method         : IsValid
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TCD_CircuitDesign_c::IsValid(
      void ) const
{
   return( this->srName.length( ) ? true : false );
}

//===========================================================================//
// Method         : InitDefaultsNetList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
void TCD_CircuitDesign_c::InitDefaultsNetList_(
      const TPO_InstList_t& instList_,
      const TPO_PortList_t& portList_,
      const TLO_CellList_t& cellList_,
            TNO_NetList_c*  pnetList_ ) const
{
   // Estimate initial net list length
   size_t netEstLength = portList_.GetLength( );
   for( size_t i = 0; i < instList_.GetLength( ); ++i )
   {
      const TPO_Inst_c& inst = *instList_[i];
      const TPO_PinList_t& pinList_ = inst.GetPinList( );
      netEstLength += pinList_.GetLength( );
   }

   // Allocate a local net list used to extract an initial list of net names
   TNO_NetList_c netList_( netEstLength );

   // Load a local net list based on all potential pin-based net names
   for( size_t i = 0; i < instList_.GetLength( ); ++i )
   {
      const TPO_Inst_c& inst = *instList_[i];
      const TPO_PinList_t& pinList_ = inst.GetPinList( );
      for( size_t j = 0; j < pinList_.GetLength( ); ++j )
      {
         const TPO_Pin_t& pin = *pinList_[j];
         netList_.Add( pin.GetName( ));
      }
   }
   for( size_t i = 0; i < portList_.GetLength( ); ++i )
   {
      const TPO_Port_t& port = *portList_[i];
      netList_.Add( port.GetName( ));
   }

   // Force unique sort to detect and remove all duplicate net names
   netList_.Uniquify( );

   // Load given (returned) net list based on local (sorted) net list
   pnetList_->SetCapacity( netList_.GetLength( ));
   for( size_t i = 0; i < netList_.GetLength( ); ++i )
   {
      TNO_Net_c net = *netList_[i];
      pnetList_->Add( net );
   }

   // Finally, repeat instance and port list iteration to update each net
   for( size_t i = 0; i < instList_.GetLength( ); ++i )
   {
      const TPO_Inst_c& inst = *instList_[i];
      const TPO_PinList_t& pinList_ = inst.GetPinList( );

      if( inst.GetSource( ) == TPO_INST_SOURCE_NAMES )
      {
         unsigned int inputCount = 0;
         for( size_t j = 0; j < pinList_.GetLength( ); ++j )
         {
            const TPO_Pin_t& pin = *pinList_[j];
            TNO_Net_c* pnet = pnetList_->Find( pin.GetName( ));

            if( pin.GetType( ) == TC_TYPE_OUTPUT )
            {
	       pnet->AddInstPin( inst.GetName( ), "out", TC_TYPE_OUTPUT );
            }
            else if( pin.GetType( ) == TC_TYPE_INPUT )
            {
               char szPinName[TIO_FORMAT_STRING_LEN_DATA];
               sprintf( szPinName, "in[%u]", inputCount++ );
	       pnet->AddInstPin( inst.GetName( ), szPinName, TC_TYPE_INPUT );
            }
	 }
      }
      else if( inst.GetSource( ) == TPO_INST_SOURCE_LATCH )
      {
         for( size_t j = 0; j < pinList_.GetLength( ); ++j )
         {
            const TPO_Pin_t& pin = *pinList_[j];
            TNO_Net_c* pnet = pnetList_->Find( pin.GetName( ));

            if( pin.GetType( ) == TC_TYPE_OUTPUT )
            {
	       pnet->AddInstPin( inst.GetName( ), "Q", TC_TYPE_OUTPUT );
            }
            else if( pin.GetType( ) == TC_TYPE_INPUT )
            {
	       pnet->AddInstPin( inst.GetName( ), "D", TC_TYPE_INPUT );
            }
            else if( pin.GetType( ) == TC_TYPE_CLOCK )
            {
               pnet->AddInstPin( inst.GetName( ), "clk", TC_TYPE_CLOCK );
	       pnet->SetRoutable( false );
            }
	 }
      }
      else if( inst.GetSource( ) == TPO_INST_SOURCE_SUBCKT )
      {
         const TPO_PinMapList_t& pinMapList_ = inst.GetSubcktPinMapList( );
         for( size_t j = 0; j < pinMapList_.GetLength( ); ++j )
         {
            const TPO_PinMap_c& pinMap = *pinMapList_[j];
            TNO_Net_c* pnet = pnetList_->Find( pinMap.GetInstPinName( ));

            const TLO_Cell_c* pcell = cellList_.Find( inst.GetCellName( ));
            if( !pcell )
	       continue;

   	    const TLO_PortList_t& portList__ = pcell->GetPortList( );
            const TLO_Port_c* pport = portList__.Find( pinMap.GetCellPinName( ));
            if( !pport )
	       continue;

            pnet->AddInstPin( pinMap.GetInstPinName( ), pinMap.GetCellPinName( ), 
                              pport->GetType( ));
         }
      }
   }

   for( size_t i = 0; i < portList_.GetLength( ); ++i )
   {
      const TPO_Port_t& port = *portList_[i];
      TNO_Net_c* pnet = pnetList_->Find( port.GetName( ));

      if( port.GetInputOutputType( ) == TC_TYPE_OUTPUT )
      {
         pnet->AddInstPin( port.GetName( ), "inpad", TC_TYPE_OUTPUT );
      }
      else if( port.GetInputOutputType( ) == TC_TYPE_INPUT )
      {
         pnet->AddInstPin( port.GetName( ), "outpad", TC_TYPE_INPUT );
      }
   }
}

//===========================================================================//
// Method         : InitDefaultsNetNameList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
void TCD_CircuitDesign_c::InitDefaultsNetNameList_(
      const TPO_InstList_t& instList_,
      const TPO_PortList_t& portList_,
            TNO_NetList_c*  pnetList_,
            TNO_NameList_t* pnetNameList_ ) const 
{
   TNO_NetList_c netList_( pnetList_->GetLength( ));
   for( size_t i = 0; i < pnetList_->GetLength( ); ++i )
   {
      const TNO_Net_c& net = *( *pnetList_ )[i];
      netList_.Add( net.GetName( ));
   }

   unsigned int netIndex = 0;
   for( size_t i = 0; i < portList_.GetLength( ); ++i )
   {
      const TPO_Port_t& port = *portList_[i];
      TNO_Net_c* pnet = netList_.Find( port.GetName( ));
      if( pnet && pnet->IsRoutable( ))
      {
   	 pnet->SetRoutable( false );
         pnetNameList_->Add( port.GetName( ));

         pnet = pnetList_->Find( port.GetName( ));
         pnet->SetIndex( netIndex );
         ++netIndex;
      }
   }
   for( size_t i = 0; i < instList_.GetLength( ); ++i )
   {
      const TPO_Inst_c& inst = *instList_[i];
      const TPO_PinList_t& pinList_ = inst.GetPinList( );
      for( size_t j = 0; j < pinList_.GetLength( ); ++j )
      {
         const TPO_Pin_t& pin = *pinList_[j];
         TNO_Net_c* pnet = netList_.Find( pin.GetName( ));
	 if( pnet && pnet->IsRoutable( ))
	 {
   	    pnet->SetRoutable( false );
            pnetNameList_->Add( pin.GetName( ));

            pnet = pnetList_->Find( pin.GetName( ));
	    pnet->SetIndex( netIndex );
            ++netIndex;
	 }
      }
   }
}

//===========================================================================//
// Method         : InitValidateNetList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
bool TCD_CircuitDesign_c::InitValidateNetList_(
      TNO_NetList_c* pnetList ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   bool ok = true;

   size_t zeroInputPinCount = 0;
   size_t zeroOutputPinCount = 0;
   size_t multiOutputPinCount = 0;

   TNO_NameList_t deleteNameList( pnetList->GetLength( ));

   for( size_t i = 0; i < pnetList->GetLength( ); ++i )
   {
      const TNO_Net_c& net = *( *pnetList )[i];
      if( !net.IsRoutable( ))
	 continue;

      size_t inputPinCount = net.FindInstPinCount( TC_TYPE_INPUT );
      if( inputPinCount == 0 )
      {      
         // [VPR] If input pad has 0 receivers, then delete from net list
         //       (this includes mcnc circuits, as well as ODIN subckts)
         ok = printHandler.Warning( "Invalid net \"%s\" detected.\n"
                                    "%sNo input pins found.\n"
                                    "%sA net with zero signals driving it may be invalid.\n",
                                    TIO_PSZ_STR( net.GetName( )),
                                    TIO_PREFIX_WARNING_SPACE,
                                    TIO_PREFIX_WARNING_SPACE );
         if( !ok )
            break;

         ++zeroInputPinCount;
         deleteNameList.Add( net.GetName( ));
      }

      size_t outputPinCount = net.FindInstPinCount( TC_TYPE_OUTPUT );
      if( outputPinCount == 0 )
      {
         ok = printHandler.Error( "Invalid net \"%s\" detected!\n"
                                  "%sNo output pins found.\n",
                                  TIO_PSZ_STR( net.GetName( )),
                                  TIO_PREFIX_ERROR_SPACE );
         if( !ok )
            break;

         ++zeroOutputPinCount;
         deleteNameList.Add( net.GetName( ));
      }
      else if( outputPinCount > 1 )
      {
         ok = printHandler.Error( "Invalid net \"%s\" detected!\n"
                                  "%sToo many output pins founds.\n",
                                  TIO_PSZ_STR( net.GetName( )),
                                  TIO_PREFIX_ERROR_SPACE );
         if( !ok )
            break;

         ++multiOutputPinCount;
         deleteNameList.Add( net.GetName( ));
      }
   }

   for( size_t i = 0; i < deleteNameList.GetLength( ); ++i )
   {
      const TC_Name_c& deleteName = *deleteNameList[i];
      pnetList->Delete( deleteName.GetName( ));
   }

   if( ok && zeroInputPinCount )
   {
      ok = printHandler.Warning( "Deleted %d nets with zero input pins.\n", 
                                 zeroInputPinCount );
   }
   if( ok && zeroOutputPinCount )
   {
      ok = printHandler.Warning( "Deleted %d nets with zero output pins.\n", 
                                 zeroOutputPinCount );
   }
   if( ok && multiOutputPinCount )
   {
      ok = printHandler.Warning( "Deleted %d nets with multiple output pins.\n", 
                                 multiOutputPinCount );
   }
   return( ok );
}

//===========================================================================//
// Method         : InitValidateInstList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TCD_CircuitDesign_c::InitValidateInstList_(
     const TPO_InstList_t& instList_,
     const TLO_CellList_t& cellList_ ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   bool ok = true;

   for( size_t i = 0; i < instList_.GetLength( ); ++i )
   {
      const TPO_Inst_c& inst = *instList_[i];

      if(( inst.GetSource( ) == TPO_INST_SOURCE_NAMES ) &&
         ( cellList_.IsMember( "names" )))
      {
         const TLO_Cell_c* pcell = cellList_.Find( "names" );
         size_t cellInputCount = pcell->FindPortCount( TC_TYPE_INPUT );
         size_t instInputCount = inst.FindPinCount( TC_TYPE_INPUT, cellList_ );

         if( instInputCount > cellInputCount )
         {
            const char* pszInstName = inst.GetName( );
            ok = printHandler.Error( "Invalid LUT size for .names \"%s\"!\n"
                                     "%sInput pin count exeeds VPR architecture LUT number of pins (%d)\n",
				     TIO_PSZ_STR( pszInstName ),
                                     TIO_PREFIX_ERROR_SPACE,
                                     cellInputCount );
         }
         if( !ok )
	    break;
      }

      const TPO_PinList_t& pinList = inst.GetPinList( );
      for( size_t j = 0; j < pinList.GetLength( ); ++j )
      {
	 const TPO_Pin_t& pin = *pinList[j];
	 const char* pszPinName = pin.GetName( );

         if( strcmp( pszPinName, "open" ) == 0 )
         {
            const char* pszSource = "instance";
	    if( inst.GetSource( ) == TPO_INST_SOURCE_NAMES )
            {
	       pszSource = ".names";
            }
	    if( inst.GetSource( ) == TPO_INST_SOURCE_LATCH )
            {
	       pszSource = ".latch";
            }
	    if( inst.GetSource( ) == TPO_INST_SOURCE_SUBCKT )
            {
	       pszSource = ".subckt";
            }

            ok = printHandler.Error( "Invalid pin name for %s \"%s\"!\n"
                                     "%sPin name \"open\" is a reserved VPR keyword\n",
				     TIO_PSZ_STR( pszSource ),
				     TIO_PSZ_STR( pszPinName ),
                                     TIO_PREFIX_ERROR_SPACE );
            if( !ok )
               break;
         }
      }
   }
   return( ok );
}
