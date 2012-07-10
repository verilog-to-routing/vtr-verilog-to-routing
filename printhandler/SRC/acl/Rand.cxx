//===========================================================================//
// Purpose : Function definitions for rand functions.
//
//           Functions include:
//           - 
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "Rand.h"

// Initialize the "singleton" class
//
Rand* Rand::pInstance = 0;

//===========================================================================//
Rand::Rand()
   :
   seed( 1 ),
   index( 0 ),
   buffer( NULL )
{
   this->buffer = new unsigned int[ ACL_RAND_BUFFER_LEN ];

   this->InitBuffer();
}

//===========================================================================//
void Rand::SetSeed( unsigned long newSeed )
{
   this->index = 0;
   this->seed  = newSeed;

   if ( this->buffer )
      delete[] this->buffer;

   this->buffer = new unsigned int[ ACL_RAND_BUFFER_LEN ];
   this->InitBuffer();
   
   int j, k;
   unsigned int mask;
   unsigned int msb;
      
   // Fill the buffer with 15-bit values
   //
   for ( j = 0; j < ACL_RAND_BUFFER_LEN; j++ ) 
      this->buffer[j] = this->MyRand(); 
   
   // Set some of the MS bits to 1
   for ( j = 0; j < ACL_RAND_BUFFER_LEN; j++ )     
      if ( this->MyRand() > 16384 ) 
         this->buffer[j] |= 0x8000; 
   
   msb  = 0x8000;  // To turn on the diagonal bit
   mask = 0xffff;  // To turn off the leftmost bits
   
   for ( j = 0; j < 16; j++ )
   {
      k = 11 * j + 3;          // Select a word to operate on
      this->buffer[k] &= mask; // Turn off bits left of the diagonal
      this->buffer[k] |= msb;  // Turn on the diagonal bit
   
      mask >>= 1; 
      msb  >>= 1; 
   } 
}

//===========================================================================//
void Rand::InitBuffer()
{
   unsigned int* b = this->buffer;
   
   b[  0]=15301; b[  1]=57764; b[  2]=10921; b[  3]=56345; b[  4]=19316;
   b[  5]=43154; b[  6]=54727; b[  7]=49252; b[  8]=32360; b[  9]=49582;
   b[ 10]=26124; b[ 11]=25833; b[ 12]=34404; b[ 13]=11030; b[ 14]=26232;
   b[ 15]=13965; b[ 16]=16051; b[ 17]=63635; b[ 18]=55860; b[ 19]= 5184;
   b[ 20]=15931; b[ 21]=39782; b[ 22]=16845; b[ 23]=11371; b[ 24]=38624;
   b[ 25]=10328; b[ 26]= 9139; b[ 27]= 1684; b[ 28]=48668; b[ 29]=59388;
   b[ 30]=13297; b[ 31]= 1364; b[ 32]=56028; b[ 33]=15687; b[ 34]=63279;
   b[ 35]=27771; b[ 36]= 5277; b[ 37]=44628; b[ 38]=31973; b[ 39]=46977;
   b[ 40]=16327; b[ 41]=23408; b[ 42]=36065; b[ 43]=52272; b[ 44]=33610;
   b[ 45]=61549; b[ 46]=58364; b[ 47]= 3472; b[ 48]=21367; b[ 49]=56357;
   b[ 50]=56345; b[ 51]=54035; b[ 52]= 7712; b[ 53]=55884; b[ 54]=39774;
   b[ 55]=10241; b[ 56]=50164; b[ 57]=47995; b[ 58]= 1718; b[ 59]=46887;
   b[ 60]=47892; b[ 61]= 6010; b[ 62]=29575; b[ 63]=54972; b[ 64]=30458;
   b[ 65]=21966; b[ 66]=54449; b[ 67]=10387; b[ 68]= 4492; b[ 69]=  644;
   b[ 70]=57031; b[ 71]=41607; b[ 72]=61820; b[ 73]=54588; b[ 74]=40849;
   b[ 75]=54052; b[ 76]=59875; b[ 77]=43128; b[ 78]=50370; b[ 79]=44691;
   b[ 80]=  286; b[ 81]=12071; b[ 82]= 3574; b[ 83]=61384; b[ 84]=15592;
   b[ 85]=45677; b[ 86]= 9711; b[ 87]=23022; b[ 88]=35256; b[ 89]=45493;
   b[ 90]=48913; b[ 91]=  146; b[ 92]= 9053; b[ 93]= 5881; b[ 94]=36635;
   b[ 95]=43280; b[ 96]=53464; b[ 97]= 8529; b[ 98]=34344; b[ 99]=64955;
   b[100]=38266; b[101]=12730; b[102]=  101; b[103]=16208; b[104]=12607;
   b[105]=58921; b[106]=22036; b[107]= 8221; b[108]=31337; b[109]=11984;
   b[110]=20290; b[111]=26734; b[112]=19552; b[113]=   48; b[114]=31940;
   b[115]=43448; b[116]=34762; b[117]=53344; b[118]=60664; b[119]=12809;
   b[120]=57318; b[121]=17436; b[122]=44730; b[123]=19375; b[124]=   30;
   b[125]=17425; b[126]=14117; b[127]= 5416; b[128]=23853; b[129]=55783;
   b[130]=57995; b[131]=32074; b[132]=26526; b[133]= 2192; b[134]=11447;
   b[135]=   11; b[136]=53446; b[137]=35152; b[138]=64610; b[139]=64883;
   b[140]=26899; b[141]=25357; b[142]= 7667; b[143]= 3577; b[144]=39414;
   b[145]=51161; b[146]=    4; b[147]=58427; b[148]=57342; b[149]=58557;
   b[150]=53233; b[151]= 1066; b[152]=29237; b[153]=36808; b[154]=19370;
   b[155]=17493; b[156]=37568; b[157]=    3; b[158]=61468; b[159]=38876;
   b[160]=17586; b[161]=64937; b[162]=21716; b[163]=56472; b[164]=58160;
   b[165]=44955; b[166]=55221; b[167]=63880; b[168]=    1; b[169]=32200;
   b[170]=62066; b[171]=22911; b[172]=24090; b[173]=10438; b[174]=40783;
   b[175]=36364; b[176]=14999; b[177]= 2489; b[178]=43284; b[179]= 9898;
   b[180]=39612; b[181]= 9245; b[182]=  593; b[183]=34857; b[184]=41054;
   b[185]=30162; b[186]=65497; b[187]=53340; b[188]=27209; b[189]=45417;
   b[190]=37497; b[191]= 4612; b[192]=58397; b[193]=52910; b[194]=56313;
   b[195]=62716; b[196]=22377; b[197]=40310; b[198]=15190; b[199]=34471;
   b[200]=64005; b[201]=18090; b[202]=11326; b[203]=50839; b[204]=62901;
   b[205]=59284; b[206]= 5580; b[207]=15231; b[208]= 9467; b[209]=13161;
   b[210]=58500; b[211]= 7259; b[212]=  317; b[213]=50968; b[214]= 2962;
   b[215]=23006; b[216]=32280; b[217]= 6994; b[218]=18751; b[219]= 5148;
   b[220]=52739; b[221]=49370; b[222]=51892; b[223]=18552; b[224]=52264;
   b[225]=54031; b[226]= 2804; b[227]=17360; b[228]= 1919; b[229]=19639;
   b[230]= 2323; b[231]= 9448; b[232]=43821; b[233]=11022; b[234]=45500;
   b[235]=31509; b[236]=49180; b[237]=35598; b[238]=38883; b[239]=19754;
   b[240]=  987; b[241]=11521; b[242]=55494; b[243]=38056; b[244]=20664;
   b[245]= 2629; b[246]=50986; b[247]=31009; b[248]=54043; b[249]=59743;
}

//===========================================================================//
Rand::~Rand()
{
   delete[] this->buffer;
}

//===========================================================================//
const unsigned int Rand::MyRand()  
{
   this->seed = this->seed * 0x015a4e35L + 1; 
   return( static_cast< unsigned int > ( ( this->seed >> 16 ) & 0x7fff ) ); 
}

//===========================================================================//
const int Rand::GetRandInt()
{
   // This code returns a random unsigned integer k uniformly distributed in
   // the interval 0 <= k < ACL_RAND_MAX.
   //
   register size_t j; 
   register unsigned int new_rand; 
   
   if ( this->index >= 147 ) 
      j = this->index - 147;  // Wrap pointer around
   else 
      j = this->index + 103; 
   
   new_rand = this->buffer[ this->index ] ^= this->buffer[ j ]; 

   // Increment pointer for next time
   //
   if (this->index >= ( ACL_RAND_BUFFER_LEN - 1 ) )
      this->index = 0; 
   else 
      this->index++; 
   
   return( new_rand ); 
}

//===========================================================================//
const int Rand::GetRandIntMax()
{
   return( ACL_RAND_MAX );
}

//===========================================================================//
const int Rand::GetRandIntN( const unsigned n )
{
   // This code returns a random unsigned integer k uniformly distributed in
   // the interval 0 <= k < n.
   
   register size_t j; 
   register unsigned int new_rand, limit; 
   
   limit = ( ACL_RAND_MAX / n ) * n; 
   
   do
   {
      new_rand = this->GetRandInt(); 

      if ( this->index >= 147 ) 
         j = this->index - 147;    // Wrap pointer around
      else 
         j = this->index + 103; 
    
      new_rand = this->buffer[ this->index ] ^= this->buffer[ j ]; 

      // Increment pointer for next time
      //
      if ( this->index >= ( ACL_RAND_BUFFER_LEN - 1 ) )
         this->index = 0; 
      else 
         this->index++; 
   }
   while( new_rand >= limit ); 

   return( new_rand % n ); 
}

//===========================================================================//
const double Rand::GetRandDouble()
{
   // This code returns a random double z uniformly distributed in
   // the interval 0 <= z < 1.
   
   register size_t j; 
   register unsigned int new_rand; 
   
   if ( this->index >= 147 ) 
      j = this->index - 147;   // Wrap pointer around
   else 
      j = this->index + 103; 
   
   new_rand = this->buffer[ this->index ] ^= this->buffer[ j ];  
   
   // Increment pointer for next time
   //
   if ( this->index >= ( ACL_RAND_BUFFER_LEN - 1 ) )    
    this->index = 0; 
   else 
    this->index++;
   
   return( new_rand / 65536.0 );  // Return a number in [0.0 to 1.0)
}


