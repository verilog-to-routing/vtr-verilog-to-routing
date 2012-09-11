/*===========================================================================*
 * Purpose : Typedefs and function prototypes for the TIO_PrintHandler class
 *           external 'C' interface.
 *
 *===========================================================================*/

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
