/**CFile****************************************************************

  FileName    [ioWriteBaf.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedures to write AIG in the binary format.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: ioWriteBaf.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ioAbc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

/*
    Binary Aig Format

    The motivation for this format is to have
    - compact binary representation of large AIGs (~10x more compact than BLIF)
    - consequently, fast reading/writing of large AIGs (~10x faster than BLIF)
    - representation for all tech-ind info related to an AIG
    - human-readable file header

    The header:
    (1) May contain several lines of human-readable comments.
        Each comment line begins with symbol '#' and ends with symbol '\n'.
    (2) Always contains the following data. 
        - benchmark name
        - number of primary inputs
        - number of primary outputs
        - number of latches
        - number of AIG nodes (excluding the constant 1 node)
        Each entry is followed by 0-byte (character '\0'):
    (3) Next follow the names of the PIs, POs, and latches in this order. 
        Each name is followed by 0-byte (character '\0').
        Inside each set of names (PIs, POs, latches) there should be no
        identical names but the PO names may coincide with PI/latch names.

    The body:
    (1) First part of the body contains binary information about the internal AIG nodes.
        Each internal AIG node is represented using two edges (each edge is a 4-byte integer). 
        Each integer is the fanin ID followed by 1-bit representation of the complemented attribute.
        (For example, complemented edge to node 10 will be represented as 2*10 + 1 = 21.)
        The IDs of the nodes are created as follows: Constant 1 node has ID=0. 
        CIs (PIs and latch outputs) have 1-based IDs assigned in that order.
        Each node in the array of the internal AIG nodes has the ID assigned in that order.
        The constant 1 node is not written into the file.
    (2) Second part of the body contains binary information about the edges connecting 
        the COs (POs and latch inputs) to the internal AIG nodes.
        Each edge is a 4-byte integer the same way as a node fanin.
        The latch initial value (2 bits) is stored in this integer.
*/

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Writes the AIG in the binary format.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteBaf( Abc_Ntk_t * pNtk, char * pFileName )
{
    ProgressBar * pProgress;
    FILE * pFile;
    Abc_Obj_t * pObj;
    int i, nNodes, nAnds, nBufferSize;
    unsigned * pBufferNode;
    assert( Abc_NtkIsStrash(pNtk) );
    // start the output stream
    pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        fprintf( stdout, "Io_WriteBaf(): Cannot open the output file \"%s\".\n", pFileName );
        return;
    }

    // write the comment
    fprintf( pFile, "# BAF (Binary Aig Format) for \"%s\" written by ABC on %s\n", pNtk->pName, Extra_TimeStamp() );

    // write the network name
    fprintf( pFile, "%s%c", pNtk->pName, 0 );
    // write the number of PIs
    fprintf( pFile, "%d%c", Abc_NtkPiNum(pNtk), 0 );
    // write the number of POs
    fprintf( pFile, "%d%c", Abc_NtkPoNum(pNtk), 0 );
    // write the number of latches
    fprintf( pFile, "%d%c", Abc_NtkLatchNum(pNtk), 0 );
    // write the number of internal nodes
    fprintf( pFile, "%d%c", Abc_NtkNodeNum(pNtk), 0 );

    // write PIs
    Abc_NtkForEachPi( pNtk, pObj, i )
        fprintf( pFile, "%s%c", Abc_ObjName(pObj), 0 );
    // write POs
    Abc_NtkForEachPo( pNtk, pObj, i )
        fprintf( pFile, "%s%c", Abc_ObjName(pObj), 0 );
    // write latches
    Abc_NtkForEachLatch( pNtk, pObj, i )
    {
        fprintf( pFile, "%s%c", Abc_ObjName(pObj), 0 );
        fprintf( pFile, "%s%c", Abc_ObjName(Abc_ObjFanin0(pObj)), 0 );
        fprintf( pFile, "%s%c", Abc_ObjName(Abc_ObjFanout0(pObj)), 0 );
    }

    // set the node numbers to be used in the output file
    Abc_NtkCleanCopy( pNtk );
    nNodes = 1;
    Abc_NtkForEachCi( pNtk, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)(ABC_PTRINT_T)nNodes++;
    Abc_AigForEachAnd( pNtk, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)(ABC_PTRINT_T)nNodes++;

    // write the nodes into the buffer
    nAnds = 0;
    nBufferSize = Abc_NtkNodeNum(pNtk) * 2 + Abc_NtkCoNum(pNtk);
    pBufferNode = ABC_ALLOC( unsigned, nBufferSize );
    pProgress = Extra_ProgressBarStart( stdout, nBufferSize );
    Abc_AigForEachAnd( pNtk, pObj, i )
    {
        Extra_ProgressBarUpdate( pProgress, nAnds, NULL );
        pBufferNode[nAnds++] = (((int)(ABC_PTRINT_T)Abc_ObjFanin0(pObj)->pCopy) << 1) | (int)Abc_ObjFaninC0(pObj);
        pBufferNode[nAnds++] = (((int)(ABC_PTRINT_T)Abc_ObjFanin1(pObj)->pCopy) << 1) | (int)Abc_ObjFaninC1(pObj);
    }

    // write the COs into the buffer
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        Extra_ProgressBarUpdate( pProgress, nAnds, NULL );
        pBufferNode[nAnds] = (((int)(ABC_PTRINT_T)Abc_ObjFanin0(pObj)->pCopy) << 1) | (int)Abc_ObjFaninC0(pObj);
        if ( Abc_ObjFanoutNum(pObj) > 0 && Abc_ObjIsLatch(Abc_ObjFanout0(pObj)) )
            pBufferNode[nAnds] = (pBufferNode[nAnds] << 2) | ((int)(ABC_PTRINT_T)Abc_ObjData(Abc_ObjFanout0(pObj)) & 3);
        nAnds++;
    }
    Extra_ProgressBarStop( pProgress );
    assert( nBufferSize == nAnds );

    // write the buffer
    fwrite( pBufferNode, 1, sizeof(int) * nBufferSize, pFile );
    fclose( pFile );
    ABC_FREE( pBufferNode );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

