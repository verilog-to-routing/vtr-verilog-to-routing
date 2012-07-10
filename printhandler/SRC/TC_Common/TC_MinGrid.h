//===========================================================================//
// Purpose : Declaration and inline definitions for a TC_MinGrid minimum grid 
//           singleton class.  This class represents a minimum unit grid and
//           handles various transformations between floating-point and 
//           integer formats.
//
//           Inline methods include:
//           - GetGrid
//           - GetUnits
//           - GetMagnitude
//           - GetPrecision
//
//===========================================================================//

#ifndef TC_MIN_GRID_H
#define TC_MIN_GRID_H

// Define a default minumum grid
#define TC_MIN_GRID_DEF 0.001

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
class TC_MinGrid_c
{
public:

   static void NewInstance( void );
   static void DeleteInstance( void );
   static TC_MinGrid_c& GetInstance( void );

   void SetGrid( double minGrid );

   double GetGrid( void ) const;
   unsigned int GetUnits( void ) const;
   
   unsigned int GetMagnitude( void ) const;
   unsigned int GetPrecision( void ) const;

   double IntToFloat( int i ) const;
   double UnsignedIntToFloat( unsigned int i ) const;
   double LongIntToFloat( long int li ) const;

   int FloatToInt( double f ) const;
   unsigned int FloatToUnsignedInt( double f ) const;
   long int FloatToLongInt( double f ) const;

   double SnapToGrid( double f ) const;
   double SnapToGridCeil( double f ) const;

   bool IsOnGrid( double f, double* pf = 0 ) const;

protected:

   TC_MinGrid_c( void );
   ~TC_MinGrid_c( void );

private:

   // The min grid value is used to define the units, precision, and
   // magnitude values.  The following table shows several example min grid
   // values and asso. units, precision, and magnitude values.
   // 
   //    grid        0.1  0.01 0.001 0.025
   //              ----- ----- ----- -----
   //    units        10   100  1000    40
   //    magnitude    10   100  1000  1000
   //    precision    1      2     3     3

   double       grid_;              // Define min (manufacturing or data) grid
   unsigned int units_;             // Define database units per grid
   unsigned int magnitude_;         // Define database magnitude per units
   unsigned int precision_;         // Define format precision per grid

   static TC_MinGrid_c* pinstance_; // Define ptr for a singleton instance
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
inline double TC_MinGrid_c::GetGrid( 
      void ) const
{
   return( this->grid_ );
}

//===========================================================================//
inline unsigned int TC_MinGrid_c::GetUnits( 
      void ) const
{
   return( this->units_ );
}

//===========================================================================//
inline unsigned int TC_MinGrid_c::GetPrecision( 
      void ) const
{
   return( this->precision_ );
}

//===========================================================================//
inline unsigned int TC_MinGrid_c::GetMagnitude( 
      void ) const
{
   return( this->magnitude_ );
}

#endif
