//===========================================================================//
// Purpose : Declaration and inline definitions for a TIO_SkinHandler_c 
//           singleton class.  This class handles various "skin" strings
//           (e.g. University of Toronto's VPR tool vs. TI's internal toro*).
//
//           Inline methods include:
//           - GetProgramName
//           - GetProgramTitle
//           - GetBinaryName
//           - GetSourceName
//           - GetCopyright
//           - IsVPR, IsToro
//
//===========================================================================//

#include <string>
using namespace std;

#ifndef TIO_SKIN_HANDLER_H
#define TIO_SKIN_HANDLER_H

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
class TIO_SkinHandler_c
{
public:

   static void NewInstance( void );
   static void DeleteInstance( void );
   static TIO_SkinHandler_c& GetInstance( void );
   static bool HasInstance( void );

   void Set( int skinMode );

   const char* GetProgramName( void ) const;
   const char* GetProgramTitle( void ) const;
   const char* GetBinaryName( void ) const;
   const char* GetSourceName( void ) const;
   const char* GetCopyright( void ) const;

   bool IsVPR( void ) const;
   bool IsToro( void ) const;

public:

   enum TIO_SkinMode_e { TIO_SKIN_UNDEFINED = 0,
                         TIO_SKIN_VPR,
                         TIO_SKIN_TORO };

protected:

   TIO_SkinHandler_c( void );
   ~TIO_SkinHandler_c( void );

private:

   int skinMode_;            // Define skin mode (e.g. VPR vs. toro*)

   string srProgramName_;    // Define program name string
   string srProgramTitle_;   // Define program title banner
   string srBinaryName_;     // Define binary name
   string srSourceName_;     // Define source name
   string srCopyright_;      // Define copyright string

   static TIO_SkinHandler_c* pinstance_; // Define ptr for singleton instance
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
inline const char* TIO_SkinHandler_c::GetProgramName(
      void ) const
{
   return( this->srProgramName_.length( ) ? this->srProgramName_.data( ) : 0 );
}

//===========================================================================//
inline const char* TIO_SkinHandler_c::GetProgramTitle(
      void ) const
{
   return( this->srProgramTitle_.length( ) ? this->srProgramTitle_.data( ) : 0 );
}

//===========================================================================//
inline const char* TIO_SkinHandler_c::GetBinaryName(
      void ) const
{
   return( this->srBinaryName_.length( ) ? this->srBinaryName_.data( ) : 0 );
}

//===========================================================================//
inline const char* TIO_SkinHandler_c::GetSourceName(
      void ) const
{
   return( this->srSourceName_.length( ) ? this->srSourceName_.data( ) : 0 );
}

//===========================================================================//
inline const char* TIO_SkinHandler_c::GetCopyright(
      void ) const
{
   return( this->srCopyright_.length( ) ? this->srCopyright_.data( ) : 0 );
}

//===========================================================================//
inline bool TIO_SkinHandler_c::IsVPR( 
      void ) const
{
   return( this->skinMode_ == TIO_SkinHandler_c::TIO_SKIN_VPR ? true : false );
}

//===========================================================================//
inline bool TIO_SkinHandler_c::IsToro( 
      void ) const
{
   return( this->skinMode_ == TIO_SkinHandler_c::TIO_SKIN_TORO ? true : false );
}

#endif
