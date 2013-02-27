/*===========================================================================*
 * Purpose : Typedefs and function prototypes for the TIO_PrintHandler class
 *           external 'C' interface.
 *
 *===========================================================================*/

/*---------------------------------------------------------------------------*
 * Copyright (C) 2012-2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com) *
 *                                                                           *
 * This program is free software; you can redistribute it and/or modify it   *
 * under the terms of the GNU General Public License as published by the     *
 * Free Software Foundation; version 3 of the License, or any later version. *
 *                                                                           *
 * This program is distributed in the hope that it will be useful, but       *
 * WITHOUT ANY WARRANTY; without even an implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License   *
 * for more details.                                                         *
 *                                                                           *
 * You should have received a copy of the GNU General Public License along   *
 * with this program; if not, see <http://www.gnu.org/licenses>.             *
 *---------------------------------------------------------------------------*/

#ifndef TIO_PRINT_HANDLER_EXTERN_H
#define TIO_PRINT_HANDLER_EXTERN_H

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

void PrintHandlerNew( char* pszLogFileName );
void PrintHandlerDelete( void );
void PrintHandlerInit( unsigned char enableTimeStamps,
                       unsigned long maxWarningCount,
                       unsigned long maxErrorCount );
int PrintHandlerExists( void );
void PrintHandlerFilter( TIO_MessageMode_t messageMode,
                         TIO_FilterMode_t filterMode,
                         char* pszFilter );
unsigned char PrintHandlerMessage( TIO_MessageMode_t messageMode,
                                   char* pszMessage,
                                   ... );
#ifdef __cplusplus
}
#endif

#endif
