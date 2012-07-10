//===========================================================================//
// Purpose : StringC class replacement for RWCString based on std::string 
// Author  : Jon Sykes
// Date    : 04/25/03
//---------------------------------------------------------------------------//
//                                            _____ ___     _   ___ ___ ___ 
// U.S. Application Specific Products        |_   _|_ _|   /_\ / __|_ _/ __|
//                                             | |  | |   / _ \\__ \| | (__ 
// ASIC Software Engineering Services          |_| |___| /_/ \_\___/___\___|
//
// Property of Texas Instruments -- For Unrestricted Use -- Unauthorized
// reproduction and/or sale is strictly prohibited.  This product is 
// protected under copyright law as an unpublished work.
//
// Created 2003, (C) Copyright 2003 Texas Instruments.  
//
// All rights reserved.
//
//   ---------------------------------------------------------------------
//      These commodities are under U.S. Government 'distribution
//      license' control.  As such, they are not to be re-exported
//      without prior approval from the U.S. Department of Commerce.
//   ---------------------------------------------------------------------
//
//  04/16/04  tomf  : TR62980 - Added <cctype> for toupper in gcc 3.2.3
//===========================================================================//
#ifndef STRING_H
#define STRING_H

#include <string>
#include <algorithm>
#include <cctype>

#include <stdio.h>

#define TMP_STR_LEN 256

class StringC
{
public:
   enum stripType { leading = 0x1, trailing = 0x2, both = 0x3 };

public:
   StringC( void );
   StringC( const char* pszString );
   StringC( const char c );
   StringC( const StringC& s );
  ~StringC( void );

   StringC&     operator=( const char c );
   StringC&     operator=( const char* cs );
   StringC&     operator=( const StringC& s );
   StringC&     operator=( const int i );

   StringC&     operator+=( const char c );
   StringC&     operator+=( const StringC& s );
   StringC&     operator+=( const char* cs );
   StringC&     operator+=( const int i );

   char&       operator[]( size_t pos );
               operator const char*() const { return srString_.c_str(); }

   // !!! This is different from RogueWave !!!
   //
   // STL does not support the concept of a substring reference into a
   // string. You cannot assign to an STL substring, assuming it is a 
   // reference into the original string.
   //
   StringC      operator()( size_t start, size_t len ) const;

   char&       operator()( size_t i );
   char        operator()( size_t i ) const;

   const char* data() const;
   unsigned    hash() const;
   StringC      strip( stripType t = trailing, char c = ' ' ) const;
   bool        isNull( void ) const;
   bool        contains( const char c ) const;
   bool        contains( const char* p ) const;
   bool        contains( const StringC& p ) const;

   size_t      index( const char c, size_t i = 0 ) const;

   size_t      index( const char* p, size_t i = 0 ) const;
   size_t      index( const StringC& p, size_t i = 0 ) const;

   size_t      rindex( const char* p, size_t i = std::string::npos ) const;
   size_t      rindex( const StringC& p, size_t i = std::string::npos ) const;

   size_t      last( const char c ) const;
   size_t      last( const char* p ) const;
   size_t      first( const char c ) const;
   size_t      first( const char* p ) const;
   size_t      length( void ) const;
   int         compareTo( const char* cs ) const;
   int         compareTo( const StringC& s ) const;

               // Note the size_t s parameter is ignored
   StringC&     append( const char* cs, size_t s = 0 );

   StringC&     append( const char c );

   void        toUpper( void );
   void        toLower( void );
   void        remove( size_t pos );
   void        remove( size_t pos, size_t end );
   StringC&     replace( size_t pos, size_t n, const char* cs);
   StringC&     replace( size_t pos, size_t n, const char* cs, size_t );
   StringC&     replace( size_t pos, size_t n, const StringC& s );
   StringC&     replace( size_t pos, size_t n, const StringC& s, size_t );

   size_t      find( const char c, size_t i = 0 ) const;

   int         count( const char c ) const;

private:
   std::string srString_;

};

inline StringC operator+( const StringC& s1, const StringC& s2 );
inline StringC operator+( const StringC& s,  const char* cs );
inline StringC operator+( const char* cs, const StringC& s );

inline bool   operator==( const StringC& s1, const StringC& s2 );
inline bool   operator!=( const StringC& s1, const StringC& s2 );
inline bool   operator< ( const StringC& s1, const StringC& s2 );
inline bool   operator> ( const StringC& s1, const StringC& s2 );
inline bool   operator<=( const StringC& s1, const StringC& s2 );
inline bool   operator>=( const StringC& s1, const StringC& s2 );

inline bool   operator==( const StringC& s, const char* cs );
inline bool   operator!=( const StringC& s, const char* cs );
inline bool   operator< ( const StringC& s, const char* cs );
inline bool   operator> ( const StringC& s, const char* cs );
inline bool   operator<=( const StringC& s, const char* cs );
inline bool   operator>=( const StringC& s, const char* cs );

inline bool   operator==( const char* cs, const StringC& s );
inline bool   operator!=( const char* cs, const StringC& s );
inline bool   operator< ( const char* cs, const StringC& s );
inline bool   operator> ( const char* cs, const StringC& s );
inline bool   operator<=( const char* cs, const StringC& s );
inline bool   operator>=( const char* cs, const StringC& s );


//===========================================================================//
// Inline functions
//===========================================================================//
inline StringC::StringC( const char* pszString )
   : srString_( pszString ? pszString : "" )
{
}

//===========================================================================//
inline StringC::StringC( const StringC& srString )
   : srString_( srString.srString_ )
{
}

//===========================================================================//
inline StringC::StringC( const char c )
{
   this->srString_ = c;
}

//===========================================================================//
inline StringC::StringC( void )
   : srString_( "" )
{
}

//===========================================================================//
inline StringC::~StringC( void )
{
}
//===========================================================================//
inline int StringC::compareTo( const char* cs ) const
{
   return( this->srString_.compare( cs ) );
}

//===========================================================================//
inline int StringC::compareTo( const StringC& s ) const
{
   return( this->srString_.compare( s ) );
}

//===========================================================================//
inline char& StringC::operator[]( size_t pos )
{
   return( this->srString_[ pos ] );
}

//===========================================================================//
inline StringC StringC::operator()( size_t start, size_t len ) const
{
   StringC srSubStr;
   srSubStr.srString_ = this->srString_.substr( start, len );
   return( srSubStr );
}

//===========================================================================//
inline char& StringC::operator()( size_t i )
{
   return( this->srString_[ i ] );
}

//===========================================================================//
inline char StringC::operator()( size_t i ) const
{
   return( this->srString_[ i ] );
}

//===========================================================================//
inline StringC& StringC::operator=( const StringC& s )
{
   this->srString_ = s.srString_;
   return( *this );
}

//===========================================================================//
inline StringC& StringC::operator=( const char* cs )
{
   this->srString_ = cs;
   return( *this );
}

//===========================================================================//
inline StringC& StringC::operator=( const char c )
{
   this->srString_ = c;
   return( *this );
}

//===========================================================================//
inline StringC& StringC::operator=( const int i )
{
   char szString[ TMP_STR_LEN ];

   sprintf( szString, "%d", i );

   this->srString_ = szString;

   return( *this );
}

//===========================================================================//
inline StringC& StringC::operator+=( const StringC& srString )
{
   this->srString_ += srString.srString_;
   return( *this );
}

//===========================================================================//
inline StringC& StringC::operator+=( const char* cs )
{
   this->srString_ += cs;
   return( *this );
}

//===========================================================================//
inline StringC& StringC::operator+=( const char c )
{
   this->srString_ += c;
   return( *this );
}

//===========================================================================//
inline StringC& StringC::operator+=( const int i )
{
   char szString[ TMP_STR_LEN ];

   sprintf( szString, "%d", i );

   this->srString_ += szString;

   return( *this );
}

//===========================================================================//
inline StringC& StringC::append( const char* cs, size_t /* s */ )
{
   this->srString_ += cs;
   return( *this );
}

//===========================================================================//
inline StringC& StringC::append( const char c )
{
   this->srString_ += c;
   return( *this );
}

//===========================================================================//
inline void StringC::toUpper( void )
{
   std::transform( this->srString_.begin(), this->srString_.end(),
                   this->srString_.begin(), _toupper );
}

//===========================================================================//
inline void StringC::toLower( void )
{
   std::transform( this->srString_.begin(), this->srString_.end(),
                   this->srString_.begin(), _tolower );
}

//===========================================================================//
inline size_t StringC::last( const char c ) const
{
   return( this->srString_.find_last_of( c ) );
}

//===========================================================================//
inline size_t StringC::first( const char c ) const
{
   return( this->srString_.find_first_of( c ) );
}

//===========================================================================//
inline size_t StringC::last( const char* p ) const
{
   return( this->srString_.find_last_of( p ) );
}

//===========================================================================//
inline size_t StringC::first( const char* p ) const
{
   return( this->srString_.find_first_of( p ) );
}

//===========================================================================//
inline size_t StringC::index( const char c, size_t i ) const
{
   return( this->srString_.find( c, i ) );
}
//===========================================================================//
inline size_t StringC::index( const char* p, size_t i ) const
{
   return( this->srString_.find( p, i ) );
}

//===========================================================================//
inline size_t StringC::index( const StringC& p, size_t i ) const
{
   return( this->srString_.find( p.srString_, i ) );
}

//===========================================================================//
inline size_t StringC::rindex( const char* p, size_t i ) const
{
   return( this->srString_.rfind( p, i ) );
}

//===========================================================================//
inline size_t StringC::rindex( const StringC& p, size_t i ) const
{
   return( this->srString_.rfind( p.srString_, i ) );
}

//===========================================================================//
inline void StringC::remove( size_t pos )
{
   std::string::size_type end = this->srString_.size() - 1;
   this->remove( pos, end );
}
//===========================================================================//
inline void StringC::remove( size_t pos, size_t end )
{
   this->srString_.erase( static_cast<std::string::size_type>( pos ), end );
}

//===========================================================================//
inline bool StringC::isNull( void ) const
{
   return ( ( this->srString_.size() > 0 ) ? false : true );
}

//===========================================================================//
inline bool StringC::contains( const char c ) const
{
   return( ( this->srString_.find( c ) != std::string::npos ) ? true : false );
}

//===========================================================================//
inline bool StringC::contains( const char* p ) const
{
   return( ( this->srString_.find( p ) != std::string::npos ) ? true : false );
}

//===========================================================================//
inline bool StringC::contains( const StringC& p ) const
{
   return( ( this->srString_.find( p.srString_ ) != std::string::npos ) ? true : false );
}

//===========================================================================//
inline const char* StringC::data( void ) const
{
   return( this->srString_.c_str() );
}

//===========================================================================//
inline size_t StringC::length( void ) const
{
   return( this->srString_.size() );
}

//===========================================================================//
inline StringC& StringC::replace( size_t pos, size_t n, const char* cs )
{
   this->srString_.replace( pos, n, cs );
   return( *this );
}

//===========================================================================//
inline StringC& StringC::replace( size_t pos, size_t n1, const char* cs, size_t n2)
{
   this->srString_.replace( pos, n1, cs, n2 );
   return( *this );
}

//===========================================================================//
inline StringC& StringC::replace( size_t pos, size_t n, const StringC& s )
{
   this->srString_.replace( pos, n, s.srString_ );
   return( *this );
}

//===========================================================================//
inline StringC& StringC::replace( size_t pos, size_t n1, const StringC& s, size_t n2 )
{
   this->srString_.replace( pos, n1, s.srString_.c_str(), n2 );
   return( *this );
}

//===========================================================================//
inline size_t StringC::find( const char c, size_t i ) const
{
   return( this->srString_.find( c, i ) );
}

//===========================================================================//
inline int StringC::count( const char c ) const
{
   int    i = 0;
   size_t p = 0;

   while( p != std::string::npos )
   {
      p = this->find( c, p);

      if ( p != std::string::npos )
      {
         i++;
         p++;
      }
   }

   return( i );
}

//===========================================================================//
inline StringC operator+( const StringC& s1, const StringC& s2 )
{
   StringC srString = s1;
   srString += s2;
   return( srString );
}

//===========================================================================//
inline StringC operator+( const StringC& s,  const char* cs )
{
   StringC srString = s;
   srString += cs;
   return( srString );
}

//===========================================================================//
inline StringC operator+( const char* cs, const StringC& s )
{
   StringC srString = cs;
   srString += s;
   return( srString );
}

//===========================================================================//

inline bool operator==( const char* cs, const StringC& s )
{
   return( ( s.compareTo( cs ) == 0 ) ? true : false );
}

//===========================================================================//
inline bool operator!=( const char* cs, const StringC& s )
{
   return( ( s.compareTo( cs ) != 0 ) ? true : false );
}

//===========================================================================//
inline bool operator<( const char* cs, const StringC& s )
{
   return( ( s.compareTo( cs ) > 0 ) ? true : false );
}

//===========================================================================//
inline bool operator>( const char* cs, const StringC& s )
{
   return( ( s.compareTo( cs ) < 0 ) ? true : false );
}

//===========================================================================//
inline bool operator<=( const char* cs, const StringC& s )
{
   return( ( s.compareTo( cs ) >= 0 ) ? true : false );
}

//===========================================================================//
inline bool operator>=( const char* cs, const StringC& s )
{
   return( ( s.compareTo( cs ) <= 0 ) ? true : false );
}

//===========================================================================//
inline bool operator==( const StringC& s, const char* cs )
{
   return( ( s.compareTo( cs ) == 0 ) ? true : false );
}

//===========================================================================//
inline bool operator!=( const StringC& s, const char* cs )
{
   return( ( s.compareTo( cs ) != 0 ) ? true : false );
}

//===========================================================================//
inline bool operator<( const StringC& s, const char* cs )
{
   return( ( s.compareTo( cs ) < 0 ) ? true : false );
}

//===========================================================================//
inline bool operator>( const StringC& s, const char* cs )
{
   return( ( s.compareTo( cs ) > 0 ) ? true : false );
}

//===========================================================================//
inline bool operator<=( const StringC& s, const char* cs )
{
   return( ( s.compareTo( cs ) <= 0 ) ? true : false );
}

//===========================================================================//
inline bool operator>=( const StringC& s, const char* cs )
{
   return( ( s.compareTo( cs ) >= 0 ) ? true : false );
}

//===========================================================================//
inline bool operator==( const StringC& s1, const StringC& s2 )
{
   return( ( s1.compareTo( s2 ) == 0 ) ? true : false );
}

//===========================================================================//
inline bool operator!=( const StringC& s1, const StringC& s2 )
{
   return( ( s1.compareTo( s2 ) != 0 ) ? true : false );
}

//===========================================================================//
inline bool operator<( const StringC& s1, const StringC& s2 )
{
   return( ( s1.compareTo( s2 ) < 0 ) ? true : false );
}

//===========================================================================//
inline bool operator>( const StringC& s1, const StringC& s2 )
{
   return( ( s1.compareTo( s2 ) > 0 ) ? true : false );
}

//===========================================================================//
inline bool operator<=( const StringC& s1, const StringC& s2 )
{
   return( ( s1.compareTo( s2 ) <= 0 ) ? true : false );
}

//===========================================================================//
inline bool operator>=( const StringC& s1, const StringC& s2 )
{
   return( ( s1.compareTo( s2 ) >= 0 ) ? true : false );
}

#endif
