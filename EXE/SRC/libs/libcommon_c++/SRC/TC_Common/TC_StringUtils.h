//===========================================================================//
// Purpose : Function prototypes for miscellaneous helpful string functions.
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

#ifndef TC_STRING_UTILS_H
#define TC_STRING_UTILS_H

#include <string>
using namespace std;

#include "TC_Typedefs.h"

//===========================================================================//
// Purpose        : Function prototypes
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
int TC_CompareStrings( const char* pszStringA, const char* pszStringB );
int TC_CompareStrings( const string& srStringA, const string& srStringB );
int TC_CompareStrings( const void* pvoidA, const void* pvoidB );

bool TC_FormatStringCentered( const char* pszString,
                              size_t lenRefer,
                              char* pszCentered,
                              size_t lenCentered );
bool TC_FormatStringFilled( char cFill,
                            char* pszFilled,
                            size_t lenFilled );

bool TC_FormatStringDateTimeStamp( char* pszDateTimeStamp, 
                                   size_t lenDateTimeStamp,
                                   const char* pszPrefix = 0,
                                   const char* pszPostfix = 0 );

bool TC_FormatStringFileNameLineNum( char* pszFileNameLineNum, 
                                     size_t lenFileNameLineNum,
                                     const char* pszFileName,
                                     unsigned int fileNum,
                                     const char* pszPrefix = 0,
                                     const char* pszPostfix = 0 );

void TC_FormatStringNameIndex( const char* pszName, 
                               size_t index,
                               string* psrNameIndex );
void TC_FormatStringNameIndex( const string& srName,
                               size_t index,
                               string* psrNameIndex );

bool TC_ParseStringNameIndex( const char* pszNameIndex,
                              string* psrName,
                              size_t* pindex );
bool TC_ParseStringNameIndex( const string& srNameIndex,
                              string* psrName,
                              size_t* pindex );
bool TC_ParseStringNameIndices( const char* pszNameIndices,
                                string* psrName,
                                size_t* pindex_i,
                                size_t* pindex_j );
bool TC_ParseStringNameIndices( const string& srNameIndices,
                                string* psrName,
                                size_t* pindex_i,
                                size_t* pindex_j );

void TC_ExtractStringSideMode( TC_SideMode_t sideMode, string* psrSideMode );
void TC_ExtractStringTypeMode( TC_TypeMode_t typeMode, string* psrTypeMode );

bool TC_AddStringPrefix( char* pszString, const char* pszPrefix );
bool TC_AddStringPostfix( char* pszString, const char* pszPostfix );

int TC_stricmp( const char* pszStringA, const char* pszStringB );
int TC_stricmp( const string& srStringA, const string& srStringB );
int TC_strnicmp( const char* pszStringA, const char* pszStringB, int i );
int TC_strnicmp( const string& srStringA, const string& srStringB, int i );

char* TC_strdup( const char* pszString );
char* TC_strdup( const string& srString );

#endif
