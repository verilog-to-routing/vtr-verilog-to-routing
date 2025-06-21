/**CFile****************************************************************

  FileName    [saigIoa.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Sequential AIG package.]

  Synopsis    [Input/output for sequential AIGs using BLIF files.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: saigIoa.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include <math.h>
#include "saig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Saig_ObjName( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    static char Buffer[1000];
    if ( Aig_ObjIsNode(pObj) || Aig_ObjIsConst1(pObj) )
        sprintf( Buffer, "n%0*d", (unsigned char)Abc_Base10Log(Aig_ManObjNumMax(p)), Aig_ObjId(pObj) );
    else if ( Saig_ObjIsPi(p, pObj) )
        sprintf( Buffer, "pi%0*d", (unsigned char)Abc_Base10Log(Saig_ManPiNum(p)), Aig_ObjCioId(pObj) );
    else if ( Saig_ObjIsPo(p, pObj) )
        sprintf( Buffer, "po%0*d", (unsigned char)Abc_Base10Log(Saig_ManPoNum(p)), Aig_ObjCioId(pObj) );
    else if ( Saig_ObjIsLo(p, pObj) )
        sprintf( Buffer, "lo%0*d", (unsigned char)Abc_Base10Log(Saig_ManRegNum(p)), Aig_ObjCioId(pObj) - Saig_ManPiNum(p) );
    else if ( Saig_ObjIsLi(p, pObj) )
        sprintf( Buffer, "li%0*d", (unsigned char)Abc_Base10Log(Saig_ManRegNum(p)), Aig_ObjCioId(pObj) - Saig_ManPoNum(p) );
    else 
        assert( 0 );
    return Buffer;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManDumpBlif( Aig_Man_t * p, char * pFileName )
{
    FILE * pFile;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo;
    int i;
    if ( Aig_ManCoNum(p) == 0 )
    {
        printf( "Aig_ManDumpBlif(): AIG manager does not have POs.\n" );
        return;
    }
    Aig_ManSetCioIds( p );
    // write input file
    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        printf( "Saig_ManDumpBlif(): Cannot open file for writing.\n" );
        return;
    }
    fprintf( pFile, "# BLIF file written by procedure Saig_ManDumpBlif()\n" );
    fprintf( pFile, "# If unedited, this file can be read by Saig_ManReadBlif()\n" );
    fprintf( pFile, "# AIG stats: pi=%d po=%d reg=%d and=%d obj=%d maxid=%d\n", 
        Saig_ManPiNum(p), Saig_ManPoNum(p), Saig_ManRegNum(p), 
        Aig_ManNodeNum(p), Aig_ManObjNum(p), Aig_ManObjNumMax(p) );
    fprintf( pFile, ".model %s\n", p->pName );
    // write primary inputs
    fprintf( pFile, ".inputs" );
    Aig_ManForEachPiSeq( p, pObj, i )
        fprintf( pFile, " %s", Saig_ObjName(p, pObj) );
    fprintf( pFile, "\n" );
    // write primary outputs
    fprintf( pFile, ".outputs" );
    Aig_ManForEachPoSeq( p, pObj, i )
        fprintf( pFile, " %s", Saig_ObjName(p, pObj) );
    fprintf( pFile, "\n" );
    // write registers
    if ( Aig_ManRegNum(p) )
    {
        Aig_ManForEachLiLoSeq( p, pObjLi, pObjLo, i )
        {
            fprintf( pFile, ".latch" );
            fprintf( pFile, " %s", Saig_ObjName(p, pObjLi) );
            fprintf( pFile, " %s", Saig_ObjName(p, pObjLo) );
            fprintf( pFile, " 0\n" );
        }
    } 
    // check if constant is used
    if ( Aig_ObjRefs(Aig_ManConst1(p)) )
        fprintf( pFile, ".names %s\n 1\n", Saig_ObjName(p, Aig_ManConst1(p)) );
    // write the nodes in the DFS order
    Aig_ManForEachNode( p, pObj, i )
    {
        fprintf( pFile, ".names" );
        fprintf( pFile, " %s", Saig_ObjName(p, Aig_ObjFanin0(pObj)) );
        fprintf( pFile, " %s", Saig_ObjName(p, Aig_ObjFanin1(pObj)) );
        fprintf( pFile, " %s", Saig_ObjName(p, pObj) );
        fprintf( pFile, "\n%d%d 1\n", !Aig_ObjFaninC0(pObj), !Aig_ObjFaninC1(pObj) );
    }
    // write the POs
    Aig_ManForEachCo( p, pObj, i )
    {
        fprintf( pFile, ".names" );
        fprintf( pFile, " %s", Saig_ObjName(p, Aig_ObjFanin0(pObj)) );
        fprintf( pFile, " %s", Saig_ObjName(p, pObj) );
        fprintf( pFile, "\n%d 1\n", !Aig_ObjFaninC0(pObj) );
    }
    fprintf( pFile, ".end\n" );
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    [Reads one token from file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Saig_ManReadToken( FILE * pFile )
{
    static char Buffer[1000];
    if ( fscanf( pFile, "%s", Buffer ) == 1 )
        return Buffer;
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Returns the corresponding number.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManReadNumber( Aig_Man_t * p, char * pToken )
{
    if ( pToken[0] == 'n' )
        return atoi( pToken + 1 );
    if ( pToken[0] == 'p' )
        return atoi( pToken + 2 );
    if ( pToken[0] == 'l' )
        return atoi( pToken + 2 );
    assert( 0 );
    return -1;
}

/**Function*************************************************************

  Synopsis    [Returns the corresponding node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Saig_ManReadNode( Aig_Man_t * p, int * pNum2Id, char * pToken )
{
    int Num;
    if ( pToken[0] == 'n' )
    {
        Num = atoi( pToken + 1 );
        return Aig_ManObj( p, pNum2Id[Num] );
    }
    if ( pToken[0] == 'p' )
    {
        pToken++;
        if ( pToken[0] == 'i' )
        {
            Num = atoi( pToken + 1 );
            return Aig_ManCi( p, Num );
        }
        if ( pToken[0] == 'o' )
            return NULL;
        assert( 0 );
        return NULL;
    }
    if ( pToken[0] == 'l' )
    {
        pToken++;
        if ( pToken[0] == 'o' )
        {
            Num = atoi( pToken + 1 );
            return Saig_ManLo( p, Num );
        }
        if ( pToken[0] == 'i' )
            return NULL;
        assert( 0 );
        return NULL;
    }
    assert( 0 );
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Reads BLIF previously dumped by Saig_ManDumpBlif().]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManReadBlif( char * pFileName )
{
    FILE * pFile;
    Aig_Man_t * p;
    Aig_Obj_t * pFanin0, * pFanin1, * pNode;
    char * pToken;
    int i, nPis, nPos, nRegs, Number;
    int * pNum2Id = NULL;  // mapping of node numbers in the file into AIG node IDs
    // open the file
    pFile = fopen( pFileName, "r" );
    if ( pFile == NULL )
    {
        printf( "Saig_ManReadBlif(): Cannot open file for reading.\n" );
        return NULL;
    }
    // skip through the comments
    for ( i = 0; (pToken = Saig_ManReadToken( pFile )) && pToken[0] != '.'; i++ );
    if ( pToken == NULL ) 
        { printf( "Saig_ManReadBlif(): Error 1.\n" ); return NULL; }
    // get he model
    pToken = Saig_ManReadToken( pFile );
    if ( pToken == NULL ) 
        { printf( "Saig_ManReadBlif(): Error 2.\n" ); return NULL; }
    // start the package
    p = Aig_ManStart( 10000 );
    p->pName = Abc_UtilStrsav( pToken );
    p->pSpec = Abc_UtilStrsav( pFileName );
    // count PIs
    pToken = Saig_ManReadToken( pFile );
    if ( pToken == NULL || strcmp( pToken, ".inputs" ) ) 
        { printf( "Saig_ManReadBlif(): Error 3.\n" ); Aig_ManStop(p); return NULL; }
    for ( nPis = 0; (pToken = Saig_ManReadToken( pFile )) && pToken[0] != '.'; nPis++ );
    // count POs
    if ( pToken == NULL || strcmp( pToken, ".outputs" ) ) 
        { printf( "Saig_ManReadBlif(): Error 4.\n" ); Aig_ManStop(p); return NULL; }
    for ( nPos = 0; (pToken = Saig_ManReadToken( pFile )) && pToken[0] != '.'; nPos++ );
    // count latches
    if ( pToken == NULL ) 
        { printf( "Saig_ManReadBlif(): Error 5.\n" ); Aig_ManStop(p); return NULL; }
    for ( nRegs = 0; strcmp( pToken, ".latch" ) == 0; nRegs++ )
    {
        pToken = Saig_ManReadToken( pFile );
        if ( pToken == NULL ) 
            { printf( "Saig_ManReadBlif(): Error 6.\n" ); Aig_ManStop(p); return NULL; }
        pToken = Saig_ManReadToken( pFile );
        if ( pToken == NULL ) 
            { printf( "Saig_ManReadBlif(): Error 7.\n" ); Aig_ManStop(p); return NULL; }
        pToken = Saig_ManReadToken( pFile );
        if ( pToken == NULL ) 
            { printf( "Saig_ManReadBlif(): Error 8.\n" ); Aig_ManStop(p); return NULL; }
        pToken = Saig_ManReadToken( pFile );
        if ( pToken == NULL ) 
            { printf( "Saig_ManReadBlif(): Error 9.\n" ); Aig_ManStop(p); return NULL; }
    }
    // create PIs and LOs
    for ( i = 0; i < nPis + nRegs; i++ )
        Aig_ObjCreateCi( p );
    Aig_ManSetRegNum( p, nRegs );
    // create nodes
    for ( i = 0; strcmp( pToken, ".names" ) == 0; i++ )
    {
        // first token
        pToken = Saig_ManReadToken( pFile );
        if ( i == 0 && pToken[0] == 'n' )
        { // constant node
            // read 1
            pToken = Saig_ManReadToken( pFile );
            if ( pToken == NULL || strcmp( pToken, "1" ) ) 
                { printf( "Saig_ManReadBlif(): Error 10.\n" ); Aig_ManStop(p); return NULL; }
            // read next
            pToken = Saig_ManReadToken( pFile );
            if ( pToken == NULL ) 
                { printf( "Saig_ManReadBlif(): Error 11.\n" ); Aig_ManStop(p); return NULL; }
            continue;
        }
        pFanin0 = Saig_ManReadNode( p, pNum2Id, pToken );

        // second token
        pToken = Saig_ManReadToken( pFile );
        if ( (pToken[0] == 'p' && pToken[1] == 'o') || (pToken[0] == 'l' && pToken[1] == 'i') )
        { // buffer
            // read complemented attribute
            pToken = Saig_ManReadToken( pFile );
            if ( pToken == NULL ) 
                { printf( "Saig_ManReadBlif(): Error 12.\n" ); Aig_ManStop(p); return NULL; }
            if ( pToken[0] == '0' )
                pFanin0 = Aig_Not(pFanin0);
            // read 1
            pToken = Saig_ManReadToken( pFile );
            if ( pToken == NULL || strcmp( pToken, "1" ) ) 
                { printf( "Saig_ManReadBlif(): Error 13.\n" ); Aig_ManStop(p); return NULL; }
            Aig_ObjCreateCo( p, pFanin0 );
            // read next
            pToken = Saig_ManReadToken( pFile );
            if ( pToken == NULL ) 
                { printf( "Saig_ManReadBlif(): Error 14.\n" ); Aig_ManStop(p); return NULL; }
            continue;
        }

        // third token
        // regular node
        pFanin1 = Saig_ManReadNode( p, pNum2Id, pToken );
        pToken = Saig_ManReadToken( pFile );
        Number = Saig_ManReadNumber( p, pToken );
        // allocate mapping
        if ( pNum2Id == NULL )
        {
//            extern double pow( double x, double y );
            int Size = (int)pow(10.0, (double)(strlen(pToken) - 1));
            pNum2Id = ABC_CALLOC( int, Size );
        }

        // other tokens
        // get the complemented attributes
        pToken = Saig_ManReadToken( pFile );
        if ( pToken == NULL ) 
            { printf( "Saig_ManReadBlif(): Error 15.\n" ); Aig_ManStop(p); return NULL; }
        if ( pToken[0] == '0' )
            pFanin0 = Aig_Not(pFanin0);
        if ( pToken[1] == '0' )
            pFanin1 = Aig_Not(pFanin1);
        // read 1
        pToken = Saig_ManReadToken( pFile );
        if ( pToken == NULL || strcmp( pToken, "1" ) ) 
            { printf( "Saig_ManReadBlif(): Error 16.\n" ); Aig_ManStop(p); return NULL; }
        // read next
        pToken = Saig_ManReadToken( pFile );
        if ( pToken == NULL ) 
            { printf( "Saig_ManReadBlif(): Error 17.\n" ); Aig_ManStop(p); return NULL; }

        // create new node
        pNode = Aig_And( p, pFanin0, pFanin1 );
        if ( Aig_IsComplement(pNode) )
            { printf( "Saig_ManReadBlif(): Error 18.\n" ); Aig_ManStop(p); return NULL; }
        // set mapping
        pNum2Id[ Number ] = pNode->Id;
    }
    if ( pToken == NULL || strcmp( pToken, ".end" ) ) 
        { printf( "Saig_ManReadBlif(): Error 19.\n" ); Aig_ManStop(p); return NULL; }
    if ( nPos + nRegs != Aig_ManCoNum(p) ) 
        { printf( "Saig_ManReadBlif(): Error 20.\n" ); Aig_ManStop(p); return NULL; }
    // add non-node objects to the mapping
    Aig_ManForEachCi( p, pNode, i )
        pNum2Id[pNode->Id] = pNode->Id;
//    ABC_FREE( pNum2Id );
    p->pData = pNum2Id;
    // check the new manager
    Aig_ManSetRegNum( p, nRegs );
    if ( !Aig_ManCheck(p) )
        printf( "Saig_ManReadBlif(): Check has failed.\n" );
    return p;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

