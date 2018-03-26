/**CFile****************************************************************

  FileName    [utilNam.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Memory recycling utilities.]

  Synopsis    [Internal declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: utilNam.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__misc__util__utilNam_h
#define ABC__misc__util__utilNam_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Abc_Nam_t_           Abc_Nam_t;

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

#define Abc_NamManForEachObj( p, pStr, i )  \
    for ( i = 1; (i < Abc_NamObjNumMax(p)) && ((pStr) = Abc_NamStr(p, i)); i++ )

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== utilNam.c ===============================================================*/
extern Abc_Nam_t *     Abc_NamStart( int nObjs, int nAveSize );
extern void            Abc_NamStop( Abc_Nam_t * p );
extern void            Abc_NamPrint( Abc_Nam_t * p, char * pFileName );
extern Abc_Nam_t *     Abc_NamRef( Abc_Nam_t * p );
extern void            Abc_NamDeref( Abc_Nam_t * p );
extern int             Abc_NamObjNumMax( Abc_Nam_t * p );
extern int             Abc_NamMemUsed( Abc_Nam_t * p );
extern int             Abc_NamMemAlloc( Abc_Nam_t * p );
extern int             Abc_NamStrFind( Abc_Nam_t * p, char * pStr );
extern int             Abc_NamStrFindLim( Abc_Nam_t * p, char * pStr, char * pLim );
extern int             Abc_NamStrFindOrAdd( Abc_Nam_t * p, char * pStr, int * pfFound );
extern int             Abc_NamStrFindOrAddLim( Abc_Nam_t * p, char * pStr, char * pLim, int * pfFound );
extern int             Abc_NamStrFindOrAddF( Abc_Nam_t * p, const char * format, ...  );
extern char *          Abc_NamStr( Abc_Nam_t * p, int id );
extern Vec_Str_t *     Abc_NamBuffer( Abc_Nam_t * p );
extern Vec_Int_t *     Abc_NamComputeIdMap( Abc_Nam_t * p1, Abc_Nam_t * p2 );
extern int             Abc_NamReportCommon( Vec_Int_t * vNameIds1, Abc_Nam_t * p1, Abc_Nam_t * p2 );
extern char *          Abc_NamReportUnique( Vec_Int_t * vNameIds1, Abc_Nam_t * p1, Abc_Nam_t * p2 );


ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

