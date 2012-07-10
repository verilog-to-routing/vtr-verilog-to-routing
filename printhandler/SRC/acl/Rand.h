//===========================================================================//
// Purpose : A platform independent psuedo-random generator class.
//
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
// Created 2004, (C) Copyright 2004 Texas Instruments.  
//
// All rights reserved.
//
//   ---------------------------------------------------------------------
//      These commodities are under U.S. Government 'distribution
//      license' control.  As such, they are not to be re-exported
//      without prior approval from the U.S. Department of Commerce.
//   ---------------------------------------------------------------------
//
//===========================================================================//
#ifndef ACL_RAND_H
#define ACL_RAND_H

//===========================================================================//
// Function prototypes
//---------------------------------------------------------------------------//
// Author          : Jon Sykes
// Version history
// 03/08/04 jsykes : Original
//===========================================================================//

#define ACL_RAND_MAX        static_cast< unsigned int >( 65536 )
#define ACL_RAND_BUFFER_LEN 250

#ifndef _LP64
   #define ACL_RAND_NOTHROW
#else
   #include <new>
   #define ACL_RAND_NOTHROW (std::nothrow)
#endif

class Rand
{
public:
   static Rand& Instance();
   static void NewInstance( void );
   static void DeleteInstance( void );

   void                SetSeed( unsigned long newSeed );
   const unsigned long GetSeed();

   const int           GetRandInt();              // Return UDI in 0 <= k < ACL_RAND_MAX
   const int           GetRandIntN( unsigned n ); // Return UDI in 0 <= k < 65536
   const int           GetRandIntMax();           // Return max + 1 of inum()
   const double        GetRandDouble();           // Return UDD z in 0 <= z < 1

private:
   Rand( void );
  ~Rand( void );

   void                InitBuffer( void );
   const unsigned int  MyRand( void );

private:
   unsigned long       seed;
   size_t              index;
   unsigned int*       buffer;

   static Rand*     pInstance;   // Define ptr for singleton instance
};

//===========================================================================//
// Inline definitions
//===========================================================================//
inline Rand& Rand::Instance( void )
{
   if( !pInstance )
   {
      pInstance = new ACL_RAND_NOTHROW Rand;
   }

   return *pInstance;
}

//===========================================================================//
inline void Rand::NewInstance( void )
{
   pInstance = new ACL_RAND_NOTHROW Rand;
}

//===========================================================================//
inline void Rand::DeleteInstance( void )
{
   if ( pInstance != NULL )
   {
      delete pInstance;
      pInstance = NULL;
   }
}

//===========================================================================//
inline const unsigned long Rand::GetSeed()
{
   return( this->seed );
}

//===========================================================================//

#endif
