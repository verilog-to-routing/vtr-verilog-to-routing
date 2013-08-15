//===========================================================================//
// Purpose : Declaration and inline definitions for a TNO_Segment class.
//
//           Inline methods include:
//           - SetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetCompare (required for TCT_BSearch and TCT_QSort classes)
//           - GetOrient, GetChannel, GetTrack
//           - SetOrient, SetChannel, SetTrack
//           - IsValid
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

#ifndef TNO_SEGMENT_H
#define TNO_SEGMENT_H

#include <cstdio>
#include <string>
using namespace std;

#include "TGS_Typedefs.h"
#include "TGS_Region.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
class TNO_Segment_c
{
public:

   TNO_Segment_c( void );
   TNO_Segment_c( const string& srName );
   TNO_Segment_c( const char* pszName );
   TNO_Segment_c( const string& srName,
                  TGS_OrientMode_t orient,
                  const TGS_Region_c& channel,
                  unsigned int track );
   TNO_Segment_c( const char* pszName,
                  TGS_OrientMode_t orient,
                  const TGS_Region_c& channel,
                  unsigned int track );
   TNO_Segment_c( const TNO_Segment_c& segment );
   ~TNO_Segment_c( void );

   TNO_Segment_c& operator=( const TNO_Segment_c& segment );
   bool operator<( const TNO_Segment_c& segment ) const;
   bool operator==( const TNO_Segment_c& segment ) const;
   bool operator!=( const TNO_Segment_c& segment ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrSegment ) const;

   void SetName( const string& srName );
   void SetName( const char* pszName );
   const char* GetName( void ) const;
   const char* GetCompare( void ) const;

   TGS_OrientMode_t GetOrient( void ) const;
   const TGS_Region_c& GetChannel( void ) const;
   unsigned int GetTrack( void ) const;

   void SetOrient( TGS_OrientMode_t orient );
   void SetChannel( const TGS_Region_c& channel );
   void SetTrack( unsigned int track );

   void Clear( void );

   bool IsValid( void ) const;

private:

   string srName_;
   TGS_OrientMode_t orient_;
   TGS_Region_c channel_;
   unsigned int track_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
inline void TNO_Segment_c::SetName( 
      const string& srName )
{
   this->srName_ = srName;
}

//===========================================================================//
inline void TNO_Segment_c::SetName( 
      const char* pszName )
{
   this->srName_ = TIO_PSZ_STR( pszName );
}

//===========================================================================//
inline const char* TNO_Segment_c::GetName( 
      void ) const
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline const char* TNO_Segment_c::GetCompare( 
      void ) const 
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline const TGS_Region_c& TNO_Segment_c::GetChannel( 
      void ) const
{
   return( this->channel_ );
}

//===========================================================================//
inline TGS_OrientMode_t TNO_Segment_c::GetOrient(
      void ) const
{
   return( this->orient_ );
}

//===========================================================================//
inline unsigned int TNO_Segment_c::GetTrack( 
      void ) const
{
   return( this->track_ );
}

//===========================================================================//
inline void TNO_Segment_c::SetOrient(
      TGS_OrientMode_t orient )
{
   this->orient_ = orient;
}

//===========================================================================//
inline void TNO_Segment_c::SetChannel( 
      const TGS_Region_c& channel )
{
   this->channel_ = channel;
}

//===========================================================================//
inline void TNO_Segment_c::SetTrack( 
      unsigned int track )
{
   this->track_ = track;
}

//===========================================================================//
inline bool TNO_Segment_c::IsValid( 
      void ) const
{
   return( this->srName_.length( ) ? true : false );
}

#endif
