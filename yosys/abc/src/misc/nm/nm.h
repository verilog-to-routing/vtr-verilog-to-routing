/**CFilextern e****************************************************************

  FileName    [nm.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Name manager.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: nm.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__misc__nm__nm_h
#define ABC__misc__nm__nm_h


/*
    This manager is designed to store ID-to-name and name-to-ID mapping
    for Boolean networks and And-Inverter Graphs.
    
    In a netlist, net names are unique. In this case, there is a one-to-one
    mapping between IDs and names.

    In a logic network, which do not have nets, several objects may have
    the same name. For example, a latch output and a primary output.
    Another example, a primary input and an input to a black box.
    In this case, for each ID on an object there is only one name, 
    but for each name may be several IDs of objects having this name.

    The name manager maps ID-to-name uniquely but it allows one name to 
    be mapped into several IDs. When a query to find an ID of the object
    by its name is submitted, it is possible to specify the object type, 
    which will help select one of several IDs. If the type is -1, and 
    there is more than one object with the given name, any object with 
    the given name is returned.
*/

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

typedef struct Nm_Man_t_ Nm_Man_t;

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== nmApi.c ==========================================================*/
extern Nm_Man_t *   Nm_ManCreate( int nSize );
extern void         Nm_ManFree( Nm_Man_t * p );
extern int          Nm_ManNumEntries( Nm_Man_t * p );
extern char *       Nm_ManStoreIdName( Nm_Man_t * p, int ObjId, int Type, char * pName, char * pSuffix );
extern void         Nm_ManDeleteIdName( Nm_Man_t * p, int ObjId );
extern char *       Nm_ManCreateUniqueName( Nm_Man_t * p, int ObjId );
extern char *       Nm_ManFindNameById( Nm_Man_t * p, int ObjId );
extern int          Nm_ManFindIdByName( Nm_Man_t * p, char * pName, int Type );
extern int          Nm_ManFindIdByNameTwoTypes( Nm_Man_t * p, char * pName, int Type1, int Type2 );
extern Vec_Int_t *  Nm_ManReturnNameIds( Nm_Man_t * p );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

