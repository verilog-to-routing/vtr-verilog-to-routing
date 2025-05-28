/**CFile****************************************************************

  FileName    [ioWriteHMetis.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedures to write hMetis format developed by
  George Karypis and Vipin Kumar from the University of Minnesota 
  (https://karypis.github.io/glaros/files/sw/hmetis/manual.pdf)]

  Author      [Jingren Wang]

  Affiliation []

  Date        [Ver. 1.0. Started - December 16, 2006.]

  Revision    []

***********************************************************************/
#include "ioAbc.h"

ABC_NAMESPACE_IMPL_START

void Io_WriteHMetis( Abc_Ntk_t *pNtk, char *pFileName, int fSkipPo, int fWeightEdges, int fVerbose )
{
    Abc_Obj_t *pObj;
    Abc_Obj_t *pFanout;
    int i, j;
    Vec_Int_t *vHyperEdgeEachWrite;
    int iEntry;
    int nHyperNodesNum = 0;
    // check that the network is valid
    assert( Abc_NtkIsStrash( pNtk ) && Abc_NtkIsComb( pNtk ) );

    FILE *pFHMetis = fopen( pFileName, "wb" );
    Vec_Ptr_t *vHyperEdges = Vec_PtrAlloc( 1000 );
    if ( pFHMetis == NULL )
    {
        fprintf( stdout, "Io_WriteHMetis(): Cannot open the output file \"%s\".\n", pFileName );
        fclose( pFHMetis );
        return;
    }

    // show pi/po/and number
    if ( fVerbose )
    {
        Abc_Print( 1, "Writing hMetis file \"%s\" with %d nodes (%d pi, %d po, %d and nodes).\n", pFileName, Abc_NtkObjNum( pNtk ), Abc_NtkPiNum( pNtk ), Abc_NtkPoNum( pNtk ), Abc_NtkNodeNum( pNtk ) );
    }

    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        Vec_Int_t *vHyperEdgeEach = Vec_IntAlloc( 20 );
        // push the node itself, which is a source node
        Vec_IntPush( vHyperEdgeEach, Abc_ObjId( pObj ) );
        // iterate through all the fanouts(sink) of the node
        if ( !Abc_ObjIsCo( pObj ) )
        {
            Abc_ObjForEachFanout( pObj, pFanout, j )
            {
                Vec_IntPush( vHyperEdgeEach, Abc_ObjId( pFanout ) );
            }
        } else
        {
            if ( fSkipPo )
            {
                continue;
            }
            Vec_IntPush( vHyperEdgeEach, Abc_ObjId( Abc_ObjFanin0( pObj ) ) );
        }
        Vec_PtrPush( vHyperEdges, vHyperEdgeEach );
    }
    
    nHyperNodesNum = Abc_NtkObjNum( pNtk );

    // write the number of hyperedges and the number of vertices
    if ( fWeightEdges )
    {
        fprintf( pFHMetis, "%d %d 1\n", Vec_PtrSize( vHyperEdges ), nHyperNodesNum );
    } else
    {
        fprintf( pFHMetis, "%d %d\n", Vec_PtrSize( vHyperEdges ), nHyperNodesNum );
    }
    // write the hyperedges
    Vec_PtrForEachEntry( Vec_Int_t *, vHyperEdges, vHyperEdgeEachWrite, i )
    {
        if ( fWeightEdges )
        {
            fprintf( pFHMetis, "%d ", Vec_IntSize( vHyperEdgeEachWrite ) );
        }

        Vec_IntForEachEntry( vHyperEdgeEachWrite, iEntry, j )
        {
            if ( j == Vec_IntSize( vHyperEdgeEachWrite ) - 1 )
            {
                fprintf( pFHMetis, "%d", iEntry );
            } else
            {
                fprintf( pFHMetis, "%d ", iEntry );
            }
        }
        fprintf( pFHMetis, "\n" );
    }
    // comments should be started with "%" in hMetis format
    fprintf( pFHMetis, "%%This file was written by ABC on %s\n", Extra_TimeStamp() );
    fprintf( pFHMetis, "%%For information about hMetis format, refer to %s\n", "https://karypis.github.io/glaros/files/sw/hmetis/manual.pdf" );
    Vec_PtrFreeFree( vHyperEdges );
    fclose( pFHMetis );
}

ABC_NAMESPACE_IMPL_END