//===========================================================================//
// Purpose : Enums, typedefs, and defines for TO_Output class.
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

#ifndef TO_TYPEDEFS_H
#define TO_TYPEDEFS_H

//---------------------------------------------------------------------------//
// Define LAFF file constants and typedefs
//---------------------------------------------------------------------------//

enum TO_LaffColorId_e
{
   TO_LAFF_COLOR_UNKNOWN = 0,
   TO_LAFF_COLOR_RED     = 1,
   TO_LAFF_COLOR_GREEN   = 2,
   TO_LAFF_COLOR_BLUE    = 3,
   TO_LAFF_COLOR_CYAN    = 4,
   TO_LAFF_COLOR_YELLOW  = 5,
   TO_LAFF_COLOR_PINK    = 6,
   TO_LAFF_COLOR_WHITE   = 7
};
typedef enum TO_LaffColorId_e TO_LaffColorId_t;

enum TO_LaffFillId_e
{
   TO_LAFF_FILL_UNDEFINED  = 0,
   TO_LAFF_FILL_PATTERN_10 = 1,
   TO_LAFF_FILL_PATTERN_50 = 3,
   TO_LAFF_FILL_PATTERN_70 = 0,
   TO_LAFF_FILL_PATTERN_90 = 9
};
typedef enum TO_LaffFillId_e TO_LaffFillId_t;

enum TO_LaffLayerId_e
{
   TO_LAFF_LAYER_UNDEFINED = 0,
   TO_LAFF_LAYER_PB        = 10 + TO_LAFF_FILL_PATTERN_50,
   TO_LAFF_LAYER_IO        = 20 + TO_LAFF_FILL_PATTERN_50,
   TO_LAFF_LAYER_SB        = 30 + TO_LAFF_FILL_PATTERN_50,
   TO_LAFF_LAYER_CB        = 40 + TO_LAFF_FILL_PATTERN_70,
   TO_LAFF_LAYER_CH        = 50 + TO_LAFF_FILL_PATTERN_10,
   TO_LAFF_LAYER_CV        = 60 + TO_LAFF_FILL_PATTERN_10,
   TO_LAFF_LAYER_SH        = 70 + TO_LAFF_FILL_PATTERN_90,
   TO_LAFF_LAYER_SV        = 80 + TO_LAFF_FILL_PATTERN_90,
   TO_LAFF_LAYER_IG        = 100,
   TO_LAFF_LAYER_IM        = 110,
   TO_LAFF_LAYER_IP        = 120,
   TO_LAFF_LAYER_BB        = 130
};
typedef enum TO_LaffLayerId_e TO_LaffLayerId_t;

#endif
