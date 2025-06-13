/**CFile****************************************************************

  FileName    [ioReadBaf.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedures to read AIG in the binary format.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: ioReadBaf.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ioAbc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reads the AIG in the binary format.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Io_ReadBaf( char * pFileName, int fCheck )
{
    ProgressBar * pProgress;
    FILE * pFile;
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj, * pNode0, * pNode1;
    Abc_Ntk_t * pNtkNew;
    int nInputs, nOutputs, nLatches, nAnds, nFileSize, Num, i;
    char * pContents, * pName, * pCur;
    unsigned * pBufferNode;
    int RetValue;

    // read the file into the buffer
    nFileSize = Extra_FileSize( pFileName );
    pFile = fopen( pFileName, "rb" );
    pContents = ABC_ALLOC( char, nFileSize );
    RetValue = fread( pContents, nFileSize, 1, pFile );
    fclose( pFile );

    // skip the comments (comment lines begin with '#' and end with '\n')
    for ( pCur = pContents; *pCur == '#'; )
        while ( *pCur++ != '\n' );

    // read the name
    pName = pCur;             while ( *pCur++ );
    // read the number of inputs
    nInputs = atoi( pCur );   while ( *pCur++ );
    // read the number of outputs
    nOutputs = atoi( pCur );  while ( *pCur++ );
    // read the number of latches
    nLatches = atoi( pCur );  while ( *pCur++ );
    // read the number of nodes
    nAnds = atoi( pCur );     while ( *pCur++ );

    // allocate the empty AIG
    pNtkNew = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );
    pNtkNew->pName = Extra_UtilStrsav( pName );
    pNtkNew->pSpec = Extra_UtilStrsav( pFileName );

    // prepare the array of nodes
    vNodes = Vec_PtrAlloc( 1 + nInputs + nLatches + nAnds );
    Vec_PtrPush( vNodes, Abc_AigConst1(pNtkNew) );

    // create the PIs
    for ( i = 0; i < nInputs; i++ )
    {
        pObj = Abc_NtkCreatePi(pNtkNew);    
        Abc_ObjAssignName( pObj, pCur, NULL );  while ( *pCur++ );
        Vec_PtrPush( vNodes, pObj );
    }
    // create the POs
    for ( i = 0; i < nOutputs; i++ )
    {
        pObj = Abc_NtkCreatePo(pNtkNew);   
        Abc_ObjAssignName( pObj, pCur, NULL );  while ( *pCur++ ); 
    }
    // create the latches
    for ( i = 0; i < nLatches; i++ )
    {
        pObj = Abc_NtkCreateLatch(pNtkNew);
        Abc_ObjAssignName( pObj, pCur, NULL );  while ( *pCur++ ); 

        pNode0 = Abc_NtkCreateBi(pNtkNew);
        Abc_ObjAssignName( pNode0, pCur, NULL );  while ( *pCur++ ); 

        pNode1 = Abc_NtkCreateBo(pNtkNew);
        Abc_ObjAssignName( pNode1, pCur, NULL );  while ( *pCur++ ); 
        Vec_PtrPush( vNodes, pNode1 );

        Abc_ObjAddFanin( pObj, pNode0 );
        Abc_ObjAddFanin( pNode1, pObj );
    }

    // get the pointer to the beginning of the node array
    pBufferNode = (unsigned *)(pContents + (nFileSize - (2 * nAnds + nOutputs + nLatches) * sizeof(int)) );
    // make sure we are at the place where the nodes begin
    if ( pBufferNode != (unsigned *)pCur )
    {
        ABC_FREE( pContents );
        Vec_PtrFree( vNodes );
        Abc_NtkDelete( pNtkNew );
        printf( "Warning: Internal reader error.\n" );
        return NULL;
    }

    // create the AND gates
    pProgress = Extra_ProgressBarStart( stdout, nAnds );
    for ( i = 0; i < nAnds; i++ )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        pNode0 = Abc_ObjNotCond( (Abc_Obj_t *)Vec_PtrEntry(vNodes, pBufferNode[2*i+0] >> 1), pBufferNode[2*i+0] & 1 );
        pNode1 = Abc_ObjNotCond( (Abc_Obj_t *)Vec_PtrEntry(vNodes, pBufferNode[2*i+1] >> 1), pBufferNode[2*i+1] & 1 );
        Vec_PtrPush( vNodes, Abc_AigAnd((Abc_Aig_t *)pNtkNew->pManFunc, pNode0, pNode1) );
    }
    Extra_ProgressBarStop( pProgress );

    // read the POs
    Abc_NtkForEachCo( pNtkNew, pObj, i )
    {
        Num = pBufferNode[2*nAnds+i];
        if ( Abc_ObjFanoutNum(pObj) > 0 && Abc_ObjIsLatch(Abc_ObjFanout0(pObj)) )
        {
            Abc_ObjSetData( Abc_ObjFanout0(pObj), (void *)(ABC_PTRINT_T)(Num & 3) );
            Num >>= 2;
        }
        pNode0 = Abc_ObjNotCond( (Abc_Obj_t *)Vec_PtrEntry(vNodes, Num >> 1), Num & 1 );
        Abc_ObjAddFanin( pObj, pNode0 );
    }
    ABC_FREE( pContents );
    Vec_PtrFree( vNodes );

    // remove the extra nodes
//    Abc_AigCleanup( (Abc_Aig_t *)pNtkNew->pManFunc );

    // check the result
    if ( fCheck && !Abc_NtkCheckRead( pNtkNew ) )
    {
        printf( "Io_ReadBaf: The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;

}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

