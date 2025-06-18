/**CFile****************************************************************

  FileName    [sclCon.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Standard-cell library representation.]

  Synopsis    [Constraint manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 24, 2012.]

  Revision    [$Id: sclCon.h,v 1.0 2012/08/24 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__scl_Con__h
#define ABC__scl_Con__h

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Scl_Con_t_ Scl_Con_t;
struct Scl_Con_t_
{
    char *         pFileName;   // constraint file name
    char *         pModelName;  // current design name
    char *         pInCellDef;  // default input driving gate
    int            tInArrDef;   // default input arrival time
    int            tInSlewDef;  // default input slew
    int            tInLoadDef;  // default input load
    int            tOutReqDef;  // default output required time
    int            tOutLoadDef; // default output load
    Vec_Ptr_t      vInCells;    // input driving gate names
    Vec_Ptr_t      vInCellsPtr; // input driving gates
    Vec_Int_t      vInArrs;     // input arrival times
    Vec_Int_t      vInSlews;    // input slews
    Vec_Int_t      vInLoads;    // input loads
    Vec_Int_t      vOutReqs;    // output required times
    Vec_Int_t      vOutLoads;   // output loads
    Abc_Nam_t *    pNamI;       // input names
    Abc_Nam_t *    pNamO;       // output names
};

#define SCL_INPUT_CELL   "input_cell"
#define SCL_INPUT_ARR    "input_arrival"
#define SCL_INPUT_SLEW   "input_slew"
#define SCL_INPUT_LOAD   "input_load"
#define SCL_OUTPUT_REQ   "output_required"
#define SCL_OUTPUT_LOAD  "output_load"

#define SCL_DIRECTIVE(ITEM)     "."#ITEM
#define SCL_DEF_DIRECTIVE(ITEM) ".default_"#ITEM

#define SCL_NUM          1000
#define SCL_INFINITY    (0x3FFFFFFF)

static inline int       Scl_Flt2Int( float w )   { return SCL_NUM*w;        }
static inline float     Scl_Int2Flt( int i )     { return (float)i/SCL_NUM; }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Manager construction.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Scl_Con_t * Scl_ConAlloc( char * pFileName, Abc_Nam_t * pNamI, Abc_Nam_t * pNamO )
{
    Scl_Con_t * p  = ABC_CALLOC( Scl_Con_t, 1 );
    p->pFileName   = Abc_UtilStrsav( pFileName );
    p->pNamI       = pNamI;
    p->pNamO       = pNamO;
    p->pModelName  = NULL;
    p->pInCellDef  = NULL;
    p->tInArrDef   = SCL_INFINITY;
    p->tInSlewDef  = SCL_INFINITY;
    p->tInLoadDef  = SCL_INFINITY;
    p->tOutReqDef  = SCL_INFINITY;
    p->tOutLoadDef = SCL_INFINITY;
    Vec_PtrFill( &p->vInCells,  Abc_NamObjNumMax(pNamI)-1, NULL );
    Vec_IntFill( &p->vInArrs,   Abc_NamObjNumMax(pNamI)-1, SCL_INFINITY );
    Vec_IntFill( &p->vInSlews,  Abc_NamObjNumMax(pNamI)-1, SCL_INFINITY );
    Vec_IntFill( &p->vInLoads,  Abc_NamObjNumMax(pNamI)-1, SCL_INFINITY );
    Vec_IntFill( &p->vOutReqs,  Abc_NamObjNumMax(pNamO)-1, SCL_INFINITY );
    Vec_IntFill( &p->vOutLoads, Abc_NamObjNumMax(pNamO)-1, SCL_INFINITY );
    return p;
}
static inline void Scl_ConFree( Scl_Con_t * p )
{
    Vec_PtrErase( &p->vInCellsPtr );
    Vec_PtrFreeData( &p->vInCells );
    Vec_PtrErase( &p->vInCells );
    Vec_IntErase( &p->vInArrs );
    Vec_IntErase( &p->vInSlews );
    Vec_IntErase( &p->vInLoads );
    Vec_IntErase( &p->vOutReqs );
    Vec_IntErase( &p->vOutLoads );
    Abc_NamDeref( p->pNamI );
    Abc_NamDeref( p->pNamO );
    ABC_FREE( p->pInCellDef );
    ABC_FREE( p->pModelName );
    ABC_FREE( p->pFileName );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Manager serialization.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Scl_ConParse( Scl_Con_t * p, Abc_Nam_t * pNamI, Abc_Nam_t * pNamO )
{
    char Buffer[1000];
    char * pToken, * pToken2, * pToken3, * pName; 
    int i, Num = -1, nLines = 0; int Value;
    FILE * pFile = fopen( p->pFileName, "rb" );
    while ( fgets( Buffer, 1000, pFile ) )
    {
        nLines++;
        pToken = strtok( Buffer, " \t\r\n" );
        if ( pToken == NULL )
            continue;
        pToken2 = strtok( NULL, " \t\r\n" );
        if ( pToken2 == NULL )
        {
            printf( "Line %d: Skipping directive \"%s\" without argument.\n", nLines, pToken );
            continue;
        }
        pToken3 = strtok( NULL, " \t\r\n" );
        if ( !strcmp(pToken, ".model") )                                p->pModelName  = Abc_UtilStrsav(pToken2);
        else if ( !strcmp(pToken, SCL_DEF_DIRECTIVE(SCL_INPUT_CELL)) )  p->pInCellDef  = Abc_UtilStrsav(pToken2);
        else if ( !strcmp(pToken, SCL_DEF_DIRECTIVE(SCL_INPUT_ARR))  )  p->tInArrDef   = Scl_Flt2Int(atof(pToken2));
        else if ( !strcmp(pToken, SCL_DEF_DIRECTIVE(SCL_INPUT_SLEW)) )  p->tInSlewDef  = Scl_Flt2Int(atof(pToken2));
        else if ( !strcmp(pToken, SCL_DEF_DIRECTIVE(SCL_INPUT_LOAD)) )  p->tInLoadDef  = Scl_Flt2Int(atof(pToken2));
        else if ( !strcmp(pToken, SCL_DEF_DIRECTIVE(SCL_OUTPUT_REQ)) )  p->tOutReqDef  = Scl_Flt2Int(atof(pToken2));
        else if ( !strcmp(pToken, SCL_DEF_DIRECTIVE(SCL_OUTPUT_LOAD)))  p->tOutLoadDef = Scl_Flt2Int(atof(pToken2));
        else if ( pToken3 == NULL ) { printf( "Directive %s should be followed by two arguments.\n", pToken ); continue; }
        else if ( !strcmp(pToken, SCL_DIRECTIVE(SCL_INPUT_CELL))  )     if ( (Num = Abc_NamStrFind(pNamI, pToken2)) > 0 )  Vec_PtrWriteEntry( &p->vInCells,  Num-1, Abc_UtilStrsav(pToken3) );    else printf( "Line %d: Cannot find input \"%s\".\n", nLines, pToken2 );
        else if ( !strcmp(pToken, SCL_DIRECTIVE(SCL_INPUT_ARR))   )     if ( (Num = Abc_NamStrFind(pNamI, pToken2)) > 0 )  Vec_IntWriteEntry( &p->vInArrs,   Num-1, Scl_Flt2Int(atof(pToken3)) ); else printf( "Line %d: Cannot find input \"%s\".\n", nLines, pToken2 );
        else if ( !strcmp(pToken, SCL_DIRECTIVE(SCL_INPUT_SLEW))  )     if ( (Num = Abc_NamStrFind(pNamI, pToken2)) > 0 )  Vec_IntWriteEntry( &p->vInSlews,  Num-1, Scl_Flt2Int(atof(pToken3)) ); else printf( "Line %d: Cannot find input \"%s\".\n", nLines, pToken2 );
        else if ( !strcmp(pToken, SCL_DIRECTIVE(SCL_INPUT_LOAD))  )     if ( (Num = Abc_NamStrFind(pNamI, pToken2)) > 0 )  Vec_IntWriteEntry( &p->vInLoads,  Num-1, Scl_Flt2Int(atof(pToken3)) ); else printf( "Line %d: Cannot find input \"%s\".\n", nLines, pToken2 );
        else if ( !strcmp(pToken, SCL_DIRECTIVE(SCL_OUTPUT_REQ))  )     if ( (Num = Abc_NamStrFind(pNamO, pToken2)) > 0 )  Vec_IntWriteEntry( &p->vOutReqs,  Num-1, Scl_Flt2Int(atof(pToken3)) ); else printf( "Line %d: Cannot find output \"%s\".\n", nLines, pToken2 );
        else if ( !strcmp(pToken, SCL_DIRECTIVE(SCL_OUTPUT_LOAD)) )     if ( (Num = Abc_NamStrFind(pNamO, pToken2)) > 0 )  Vec_IntWriteEntry( &p->vOutLoads, Num-1, Scl_Flt2Int(atof(pToken3)) ); else printf( "Line %d: Cannot find output \"%s\".\n", nLines, pToken2 );
        else printf( "Line %d: Skipping unrecognized directive \"%s\".\n", nLines, pToken );
    }
    // set missing defaults
    if ( p->pInCellDef  == NULL )          p->pInCellDef  = NULL; // consider using buffer from the current library
    if ( p->tInArrDef   == SCL_INFINITY )  p->tInArrDef   = 0;
    if ( p->tInSlewDef  == SCL_INFINITY )  p->tInSlewDef  = 0;
    if ( p->tInLoadDef  == SCL_INFINITY )  p->tInLoadDef  = 0;
    if ( p->tOutReqDef  == SCL_INFINITY )  p->tOutReqDef  = 0;
    if ( p->tOutLoadDef == SCL_INFINITY )  p->tOutLoadDef = 0;
    // set individual defaults
    if ( p->pInCellDef )
    Vec_PtrForEachEntry(char *, &p->vInCells, pName, i) if ( pName == NULL )          Vec_PtrWriteEntry( &p->vInCells,  i, Abc_UtilStrsav(p->pInCellDef) );
    Vec_IntForEachEntry( &p->vInArrs, Value, i )        if ( Value == SCL_INFINITY )  Vec_IntWriteEntry( &p->vInArrs,   i, p->tInArrDef );
    Vec_IntForEachEntry( &p->vInSlews, Value, i )       if ( Value == SCL_INFINITY )  Vec_IntWriteEntry( &p->vInSlews,  i, p->tInSlewDef );
    Vec_IntForEachEntry( &p->vInLoads, Value, i )       if ( Value == SCL_INFINITY )  Vec_IntWriteEntry( &p->vInLoads,  i, p->tInLoadDef );
    Vec_IntForEachEntry( &p->vOutReqs, Value, i )       if ( Value == SCL_INFINITY )  Vec_IntWriteEntry( &p->vOutReqs,  i, p->tOutReqDef );
    Vec_IntForEachEntry( &p->vOutLoads, Value, i )      if ( Value == SCL_INFINITY )  Vec_IntWriteEntry( &p->vOutLoads, i, p->tOutLoadDef );

    fclose( pFile );
    return 1;
}
static inline Scl_Con_t * Scl_ConRead( char * pFileName, Abc_Nam_t * pNamI, Abc_Nam_t * pNamO )
{
    Scl_Con_t * p = Scl_ConAlloc( pFileName, pNamI, pNamO );
    if ( Scl_ConParse(p, pNamI, pNamO) )
        return p;
    Scl_ConFree( p );
    return NULL;
}
static inline void Scl_ConWrite( Scl_Con_t * p, char * pFileName )
{
    char * pName; int Value; int i;
    FILE * pFile = pFileName ? fopen( pFileName, "wb" ) : stdout;
    if ( pFile == NULL )
    {
        printf( "Cannot open output file \"%s\".\n", pFileName );
        return;
    }
    fprintf( pFile, ".model %s\n", p->pModelName );

    if ( p->pInCellDef )        fprintf( pFile, ".default_%s %s\n",   SCL_INPUT_CELL,  p->pInCellDef );
    if ( p->tInArrDef   != 0 )  fprintf( pFile, ".default_%s %.2f\n", SCL_INPUT_ARR,   Scl_Int2Flt(p->tInArrDef) );
    if ( p->tInSlewDef  != 0 )  fprintf( pFile, ".default_%s %.2f\n", SCL_INPUT_SLEW,  Scl_Int2Flt(p->tInSlewDef) );
    if ( p->tInLoadDef  != 0 )  fprintf( pFile, ".default_%s %.2f\n", SCL_INPUT_LOAD,  Scl_Int2Flt(p->tInLoadDef) );
    if ( p->tOutReqDef  != 0 )  fprintf( pFile, ".default_%s %.2f\n", SCL_OUTPUT_REQ,  Scl_Int2Flt(p->tOutReqDef) );
    if ( p->tOutLoadDef != 0 )  fprintf( pFile, ".default_%s %.2f\n", SCL_OUTPUT_LOAD, Scl_Int2Flt(p->tOutLoadDef) );

    Vec_PtrForEachEntry(char *, &p->vInCells, pName, i) if ( pName && (!p->pInCellDef || strcmp(pName,p->pInCellDef)) )  fprintf( pFile, ".%s %s %s\n",   SCL_INPUT_CELL,  Abc_NamStr(p->pNamI, i+1), pName );
    Vec_IntForEachEntry( &p->vInArrs, Value, i )        if ( Value != p->tInArrDef )   fprintf( pFile, ".%s %s %.2f\n", SCL_INPUT_ARR,   Abc_NamStr(p->pNamI, i+1), Scl_Int2Flt(Value) );
    Vec_IntForEachEntry( &p->vInSlews, Value, i )       if ( Value != p->tInSlewDef )  fprintf( pFile, ".%s %s %.2f\n", SCL_INPUT_SLEW,  Abc_NamStr(p->pNamI, i+1), Scl_Int2Flt(Value) );
    Vec_IntForEachEntry( &p->vInLoads, Value, i )       if ( Value != p->tInLoadDef )  fprintf( pFile, ".%s %s %.2f\n", SCL_INPUT_LOAD,  Abc_NamStr(p->pNamI, i+1), Scl_Int2Flt(Value) );
    Vec_IntForEachEntry( &p->vOutReqs, Value, i )       if ( Value != p->tOutReqDef )  fprintf( pFile, ".%s %s %.2f\n", SCL_OUTPUT_REQ,  Abc_NamStr(p->pNamO, i+1), Scl_Int2Flt(Value) );
    Vec_IntForEachEntry( &p->vOutLoads, Value, i )      if ( Value != p->tOutLoadDef ) fprintf( pFile, ".%s %s %.2f\n", SCL_OUTPUT_LOAD, Abc_NamStr(p->pNamO, i+1), Scl_Int2Flt(Value) );

    if ( pFile != stdout )
        fclose ( pFile );
}

/**Function*************************************************************

  Synopsis    [Internal APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int    Scl_ConHasInCells_( Scl_Con_t * p )        { return Vec_PtrCountZero(&p->vInCells)  != Vec_PtrSize(&p->vInCells);  }
static inline int    Scl_ConHasInArrs_( Scl_Con_t * p )         { return Vec_IntCountZero(&p->vInArrs)   != Vec_IntSize(&p->vInArrs);   }
static inline int    Scl_ConHasInSlews_( Scl_Con_t * p )        { return Vec_IntCountZero(&p->vInSlews)  != Vec_IntSize(&p->vInSlews);  }
static inline int    Scl_ConHasInLoads_( Scl_Con_t * p )        { return Vec_IntCountZero(&p->vInLoads)  != Vec_IntSize(&p->vInLoads);  }
static inline int    Scl_ConHasOutReqs_( Scl_Con_t * p )        { return Vec_IntCountZero(&p->vOutReqs)  != Vec_IntSize(&p->vOutReqs);  }
static inline int    Scl_ConHasOutLoads_( Scl_Con_t * p )       { return Vec_IntCountZero(&p->vOutLoads) != Vec_IntSize(&p->vOutLoads); }

static inline char * Scl_ConGetInCell_( Scl_Con_t * p, int i )  { return (char*)Vec_PtrEntry( &p->vInCells,  i ); }
static inline int    Scl_ConGetInArr_( Scl_Con_t * p, int i )   { return Vec_IntEntry( &p->vInArrs,   i ); }
static inline int    Scl_ConGetInSlew_( Scl_Con_t * p, int i )  { return Vec_IntEntry( &p->vInSlews,  i ); }
static inline int    Scl_ConGetInLoad_( Scl_Con_t * p, int i )  { return Vec_IntEntry( &p->vInLoads,  i ); }
static inline int    Scl_ConGetOutReq_( Scl_Con_t * p, int i )  { return Vec_IntEntry( &p->vOutReqs,  i ); }
static inline int    Scl_ConGetOutLoad_( Scl_Con_t * p, int i ) { return Vec_IntEntry( &p->vOutLoads, i ); }

/**Function*************************************************************

  Synopsis    [External APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
extern Scl_Con_t *   Scl_ConReadMan();

static inline int    Scl_ConIsRunning()               { return Scl_ConReadMan() != NULL;                  }

static inline int    Scl_ConHasInCells()              { return Scl_ConHasInCells_ ( Scl_ConReadMan() );   }
static inline int    Scl_ConHasInArrs()               { return Scl_ConHasInArrs_  ( Scl_ConReadMan() );   }
static inline int    Scl_ConHasInSlews()              { return Scl_ConHasInSlews_ ( Scl_ConReadMan() );   }
static inline int    Scl_ConHasInLoads()              { return Scl_ConHasInLoads_ ( Scl_ConReadMan() );   }
static inline int    Scl_ConHasOutReqs()              { return Scl_ConHasOutReqs_ ( Scl_ConReadMan() );   }
static inline int    Scl_ConHasOutLoads()             { return Scl_ConHasOutLoads_( Scl_ConReadMan() );   }

static inline char * Scl_ConGetInCell( int i )        { return Scl_ConGetInCell_ ( Scl_ConReadMan(), i ); }
static inline int    Scl_ConGetInArr( int i )         { return Scl_ConGetInArr_  ( Scl_ConReadMan(), i ); }
static inline int    Scl_ConGetInSlew( int i )        { return Scl_ConGetInSlew_ ( Scl_ConReadMan(), i ); }
static inline int    Scl_ConGetInLoad( int i )        { return Scl_ConGetInLoad_ ( Scl_ConReadMan(), i ); }
static inline int    Scl_ConGetOutReq( int i )        { return Scl_ConGetOutReq_ ( Scl_ConReadMan(), i ); }
static inline int    Scl_ConGetOutLoad( int i )       { return Scl_ConGetOutLoad_( Scl_ConReadMan(), i ); }

static inline float  Scl_ConGetInArrFloat( int i )    { return Scl_Int2Flt( Scl_ConGetInArr(i) );         }
static inline float  Scl_ConGetInSlewFloat( int i )   { return Scl_Int2Flt( Scl_ConGetInSlew(i) );        }
static inline float  Scl_ConGetInLoadFloat( int i )   { return Scl_Int2Flt( Scl_ConGetInLoad(i) );        }
static inline float  Scl_ConGetOutReqFloat( int i )   { return Scl_Int2Flt( Scl_ConGetOutReq(i) );        }
static inline float  Scl_ConGetOutLoadFloat( int i )  { return Scl_Int2Flt( Scl_ConGetOutLoad(i) );       }


ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

