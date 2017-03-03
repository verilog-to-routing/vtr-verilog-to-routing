/*===========================================================================*
 * Purpose : Typedefs and function prototypes for the TIO_PrintHandler class
 *           external 'C' interface.
 *
 *===========================================================================*/

/*---------------------------------------------------------------------------*
 * Copyright (C) 2012-2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com) *
 *                                                                           *
 * Permission is hereby granted, free of charge, to any person obtaining a   *
 * copy of this software and associated documentation files (the "Software"),*
 * to deal in the Software without restriction, including without limitation *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,  *
 * and/or sell copies of the Software, and to permit persons to whom the     *
 * Software is furnished to do so, subject to the following conditions:      *
 *                                                                           *
 * The above copyright notice and this permission notice shall be included   *
 * in all copies or substantial portions of the Software.                    *
 *                                                                           *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS   *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF                *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN * 
 * NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,  *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR     *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE *
 * USE OR OTHER DEALINGS IN THE SOFTWARE.                                    *
 *---------------------------------------------------------------------------*/

#ifndef TIO_PRINT_HANDLER_EXTERN_H
#define TIO_PRINT_HANDLER_EXTERN_H

#include "TIO_Typedefs.h"

/*---------------------------------------------------------------------------*
 * Define typedefs
 *---------------------------------------------------------------------------*/

enum TIO_MessageMode_e 
{
   TIO_MESSAGE_UNDEFINED = 0,
   TIO_MESSAGE_INFO,
   TIO_MESSAGE_WARNING,
   TIO_MESSAGE_ERROR,
   TIO_MESSAGE_TRACE,
   TIO_MESSAGE_DIRECT
};
typedef enum TIO_MessageMode_e TIO_MessageMode_t;

enum TIO_FilterMode_e 
{
   TIO_FILTER_UNDEFINED = 0,
   TIO_FILTER_ACCEPT,
   TIO_FILTER_REJECT
};
typedef enum TIO_FilterMode_e TIO_FilterMode_t;

/*---------------------------------------------------------------------------*
 * Define function prototypes
 *---------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C"
{
#endif

void PrintHandlerNew( const char* pszLogFileName );
void PrintHandlerDelete( void );
void PrintHandlerInit( unsigned char enableTimeStamps,
                       unsigned char enableFileLines,
                       unsigned long maxWarningCount,
                       unsigned long maxErrorCount );
int PrintHandlerExists( void );
void PrintHandlerFilter( TIO_MessageMode_t messageMode,
                         TIO_FilterMode_t filterMode,
                         const char* pszFilter );

unsigned char PrintHandlerMessage( TIO_MessageMode_t messageMode,
                                   const char* pszMessage, ... );

void PrintHandlerInfo( const char* pszMessage, ... );
unsigned char PrintHandlerWarning( const char* pszFileName,
                                   unsigned int lineNum,
                                   const char* pszMessage, ... );
unsigned char PrintHandlerError( const char* pszFileName,
                                 unsigned int lineNum,
                                 const char* pszMessage, ... );
void PrintHandlerTrace( const char* pszMessage, ... );
void PrintHandlerDirect( const char* pszMessage, ... );

#ifdef __cplusplus
}
#endif

#endif 
