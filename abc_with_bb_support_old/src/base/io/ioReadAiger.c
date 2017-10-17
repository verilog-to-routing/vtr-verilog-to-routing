/**CFile****************************************************************

  FileName    [ioReadAiger.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedures to read binary AIGER format developed by
  Armin Biere, Johannes Kepler University (http://fmv.jku.at/)]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - December 16, 2006.]

  Revision    [$Id: ioReadAiger.c,v 1.00 2006/12/16 00:00:00 alanmi Exp $]

***********************************************************************/

#include "io.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

unsigned Io_ReadAigerDecode( char ** ppPos );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reads the AIG in the binary AIGER format.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Io_ReadAiger( char * pFileName, int fCheck )
{
    ProgressBar * pProgress;
    FILE * pFile;
    Vec_Ptr_t * vNodes, * vTerms;
    Abc_Obj_t * pObj, * pNode0, * pNode1;
    Abc_Ntk_t * pNtkNew;
    int nTotal, nInputs, nOutputs, nLatches, nAnds, nFileSize, iTerm, nDigits, i;
    char * pContents, * pDrivers, * pSymbols, * pCur, * pName, * pType;
    unsigned uLit0, uLit1, uLit;

    // read the file into the buffer
    nFileSize = Extra_FileSize( pFileName );
    pFile = fopen( pFileName, "rb" );
    pContents = ALLOC( char, nFileSize );
    fread( pContents, nFileSize, 1, pFile );
    fclose( pFile );

    // check if the input file format is correct
    if ( strncmp(pContents, "aig", 3) != 0 )
    {
        fprintf( stdout, "Wrong input file format.\n" );
        return NULL;
    }

    // allocate the empty AIG
    pNtkNew = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );
    pName = Extra_FileNameGeneric( pFileName );
    pNtkNew->pName = Extra_UtilStrsav( pName );
    pNtkNew->pSpec = Extra_UtilStrsav( pFileName );
    free( pName );


    // read the file type
    pCur = pContents;         while ( *pCur++ != ' ' );
    // read the number of objects
    nTotal = atoi( pCur );    while ( *pCur++ != ' ' );
    // read the number of inputs
    nInputs = atoi( pCur );   while ( *pCur++ != ' ' );
    // read the number of latches
    nLatches = atoi( pCur );  while ( *pCur++ != ' ' );
    // read the number of outputs
    nOutputs = atoi( pCur );  while ( *pCur++ != ' ' );
    // read the number of nodes
    nAnds = atoi( pCur );     while ( *pCur++ != '\n' );  
    // check the parameters
    if ( nTotal != nInputs + nLatches + nAnds )
    {
        fprintf( stdout, "The paramters are wrong.\n" );
        return NULL;
    }

    // prepare the array of nodes
    vNodes = Vec_PtrAlloc( 1 + nInputs + nLatches + nAnds );
    Vec_PtrPush( vNodes, Abc_ObjNot( Abc_AigConst1(pNtkNew) ) );

    // create the PIs
    for ( i = 0; i < nInputs; i++ )
    {
        pObj = Abc_NtkCreatePi(pNtkNew);    
        Vec_PtrPush( vNodes, pObj );
    }
    // create the POs
    for ( i = 0; i < nOutputs; i++ )
    {
        pObj = Abc_NtkCreatePo(pNtkNew);   
    }
    // create the latches
    nDigits = Extra_Base10Log( nLatches );
    for ( i = 0; i < nLatches; i++ )
    {
        pObj = Abc_NtkCreateLatch(pNtkNew);
        Abc_LatchSetInit0( pObj );
        pNode0 = Abc_NtkCreateBi(pNtkNew);
        pNode1 = Abc_NtkCreateBo(pNtkNew);
        Abc_ObjAddFanin( pObj, pNode0 );
        Abc_ObjAddFanin( pNode1, pObj );
        Vec_PtrPush( vNodes, pNode1 );
        // assign names to latch and its input
        Abc_ObjAssignName( pObj, Abc_ObjNameDummy("_L", i, nDigits), NULL );

//        printf( "Creating latch %s with input %d and output %d.\n", Abc_ObjName(pObj), pNode0->Id, pNode1->Id );
    } 
    
    // remember the beginning of latch/PO literals
    pDrivers = pCur;

    // scroll to the beginning of the binary data
    for ( i = 0; i < nLatches + nOutputs; )
        if ( *pCur++ == '\n' )
            i++;

    // create the AND gates
    pProgress = Extra_ProgressBarStart( stdout, nAnds );
    for ( i = 0; i < nAnds; i++ )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        uLit = ((i + 1 + nInputs + nLatches) << 1);
        uLit1 = uLit  - Io_ReadAigerDecode( &pCur );
        uLit0 = uLit1 - Io_ReadAigerDecode( &pCur );
//        assert( uLit1 > uLit0 );
        pNode0 = Abc_ObjNotCond( Vec_PtrEntry(vNodes, uLit0 >> 1), uLit0 & 1 );
        pNode1 = Abc_ObjNotCond( Vec_PtrEntry(vNodes, uLit1 >> 1), uLit1 & 1 );
        assert( Vec_PtrSize(vNodes) == i + 1 + nInputs + nLatches );
        Vec_PtrPush( vNodes, Abc_AigAnd(pNtkNew->pManFunc, pNode0, pNode1) );
    }
    Extra_ProgressBarStop( pProgress );

    // remember the place where symbols begin
    pSymbols = pCur;

    // read the latch driver literals
    pCur = pDrivers;
    Abc_NtkForEachLatchInput( pNtkNew, pObj, i )
    {
        uLit0 = atoi( pCur );  while ( *pCur++ != '\n' );
        pNode0 = Abc_ObjNotCond( Vec_PtrEntry(vNodes, uLit0 >> 1), (uLit0 & 1) );//^ (uLit0 < 2) );
        Abc_ObjAddFanin( pObj, pNode0 );

//        printf( "Adding input %d to latch input %d.\n", pNode0->Id, pObj->Id );

    }
    // read the PO driver literals
    Abc_NtkForEachPo( pNtkNew, pObj, i )
    {
        uLit0 = atoi( pCur );  while ( *pCur++ != '\n' );
        pNode0 = Abc_ObjNotCond( Vec_PtrEntry(vNodes, uLit0 >> 1), (uLit0 & 1) );//^ (uLit0 < 2) );
        Abc_ObjAddFanin( pObj, pNode0 );
    }

    // read the names if present
    pCur = pSymbols;
    while ( pCur < pContents + nFileSize && *pCur != 'c' )
    {
        // get the terminal type
        pType = pCur;
        if ( *pCur == 'i' )
            vTerms = pNtkNew->vPis;
        else if ( *pCur == 'l' )
            vTerms = pNtkNew->vBoxes;
        else if ( *pCur == 'o' )
            vTerms = pNtkNew->vPos;
        else
        {
            fprintf( stdout, "Wrong terminal type.\n" );
            return NULL;
        }
        // get the terminal number
        iTerm = atoi( ++pCur );  while ( *pCur++ != ' ' );
        // get the node
        if ( iTerm >= Vec_PtrSize(vTerms) )
        {
            fprintf( stdout, "The number of terminal is out of bound.\n" );
            return NULL;
        }
        pObj = Vec_PtrEntry( vTerms, iTerm );
        if ( *pType == 'l' )
            pObj = Abc_ObjFanout0(pObj);
        // assign the name
        pName = pCur;          while ( *pCur++ != '\n' );
        // assign this name
        *(pCur-1) = 0;
        Abc_ObjAssignName( pObj, pName, NULL );
        if ( *pType == 'l' )
            Abc_ObjAssignName( Abc_ObjFanin0(Abc_ObjFanin0(pObj)), Abc_ObjName(pObj), "L" );
        // mark the node as named
        pObj->pCopy = (Abc_Obj_t *)Abc_ObjName(pObj);
    } 
    // skipping the comments
    free( pContents );
    Vec_PtrFree( vNodes );

    // assign the remaining names
    Abc_NtkForEachPi( pNtkNew, pObj, i )
    {
        if ( pObj->pCopy ) continue;
        Abc_ObjAssignName( pObj, Abc_ObjName(pObj), NULL );
    }
    Abc_NtkForEachLatchOutput( pNtkNew, pObj, i )
    {
        if ( pObj->pCopy ) continue;
        Abc_ObjAssignName( pObj, Abc_ObjName(pObj), NULL );
        Abc_ObjAssignName( Abc_ObjFanin0(Abc_ObjFanin0(pObj)), Abc_ObjName(pObj), NULL );
    }
    Abc_NtkForEachPo( pNtkNew, pObj, i )
    {
        if ( pObj->pCopy ) continue;
        Abc_ObjAssignName( pObj, Abc_ObjName(pObj), NULL );
    }

    // remove the extra nodes
    Abc_AigCleanup( pNtkNew->pManFunc );

    // check the result
    if ( fCheck && !Abc_NtkCheckRead( pNtkNew ) )
    {
        printf( "Io_ReadAiger: The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}


/**Function*************************************************************

  Synopsis    [Extracts one unsigned AIG edge from the input buffer.]

  Description [This procedure is a slightly modified version of Armin Biere's
  procedure "unsigned decode (FILE * file)". ]
  
  SideEffects [Updates the current reading position.]

  SeeAlso     []

***********************************************************************/
unsigned Io_ReadAigerDecode( char ** ppPos )
{
    unsigned x = 0, i = 0;
    unsigned char ch;

//    while ((ch = getnoneofch (file)) & 0x80)
    while ((ch = *(*ppPos)++) & 0x80)
        x |= (ch & 0x7f) << (7 * i++);

    return x | (ch << (7 * i));
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


