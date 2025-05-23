/**CFile****************************************************************

  FileName    [ioWriteBook.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedures to write Bookshelf files.]

  Author      [Myungchul Kim]
  
  Affiliation [U of Michigan]

  Date        [Ver. 1.0. Started - October 25, 2008.]

  Revision    [$Id: ioWriteBook.c,v 1.00 2005/11/10 00:00:00 mckima Exp $]

***********************************************************************/

#include <math.h>

#include "base/main/main.h"
#include "map/mio/mio.h"
#include "ioAbc.h"

ABC_NAMESPACE_IMPL_START

#define    NODES       0
#define    PL       1
#define coreHeight 1
#define termWidth  1
#define termHeight 1

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static unsigned Io_NtkWriteNodes( FILE * pFile, Abc_Ntk_t * pNtk );
static void Io_NtkWritePiPoNodes( FILE * pFile, Abc_Ntk_t * pNtk );
static void Io_NtkWriteLatchNode( FILE * pFile, Abc_Obj_t * pLatch, int NodesOrPl );
static unsigned Io_NtkWriteIntNode( FILE * pFile, Abc_Obj_t * pNode, int NodesOrPl );
static unsigned Io_NtkWriteNodeGate( FILE * pFile, Abc_Obj_t * pNode );
static void Io_NtkWriteNets( FILE * pFile, Abc_Ntk_t * pNtk );
static void Io_NtkWriteIntNet( FILE * pFile, Abc_Obj_t * pNode );
static void Io_NtkBuildLayout( FILE * pFile1, FILE *pFile2, Abc_Ntk_t * pNtk, double aspectRatio, double whiteSpace, unsigned coreCellArea );
static void Io_NtkWriteScl( FILE * pFile, unsigned numCoreRows, double layoutWidth );
static void Io_NtkWritePl( FILE * pFile, Abc_Ntk_t * pNtk, unsigned numTerms, double layoutHeight, double layoutWidth );
static Vec_Ptr_t * Io_NtkOrderingPads( Abc_Ntk_t * pNtk, Vec_Ptr_t * vTerms ); 
static Abc_Obj_t * Io_NtkBfsPads( Abc_Ntk_t * pNtk, Abc_Obj_t * pCurrEntry, unsigned numTerms, int * pOrdered );
static int Abc_NodeIsNand2( Abc_Obj_t * pNode );
static int Abc_NodeIsNor2( Abc_Obj_t * pNode );
static int Abc_NodeIsAnd2( Abc_Obj_t * pNode );
static int Abc_NodeIsOr2( Abc_Obj_t * pNode );
static int Abc_NodeIsXor2( Abc_Obj_t * pNode );
static int Abc_NodeIsXnor2( Abc_Obj_t * pNode );

static inline double Abc_Rint( double x )   { return (double)(int)x;  }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Write the network into a Bookshelf file with the given name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteBookLogic( Abc_Ntk_t * pNtk, char * FileName )
{
    Abc_Ntk_t * pNtkTemp;
    // derive the netlist
    pNtkTemp = Abc_NtkToNetlist(pNtk);
    if ( pNtkTemp == NULL )
    {
        fprintf( stdout, "Writing BOOK has failed.\n" );
        return;
    }
    Io_WriteBook( pNtkTemp, FileName );
    Abc_NtkDelete( pNtkTemp );
}

/**Function*************************************************************

  Synopsis    [Write the network into a BOOK file with the given name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteBook( Abc_Ntk_t * pNtk, char * FileName )
{

    FILE * pFileNodes, * pFileNets, * pFileAux;
    FILE * pFileScl, * pFilePl, * pFileWts;
    char * FileExt = ABC_CALLOC( char, strlen(FileName)+7 );
    unsigned coreCellArea=0;
    Abc_Ntk_t * pExdc, * pNtkTemp;
    int i;

    assert( Abc_NtkIsNetlist(pNtk) );
    // start writing the files
    strcpy(FileExt, FileName);
    pFileNodes = fopen( strcat(FileExt,".nodes"), "w" );
    strcpy(FileExt, FileName);
    pFileNets  = fopen( strcat(FileExt,".nets"), "w" );
    strcpy(FileExt, FileName);
    pFileAux   = fopen( strcat(FileExt,".aux"), "w" );

        // write the aux file
    if ( (pFileNodes == NULL) || (pFileNets == NULL) || (pFileAux == NULL) )
    {
        fclose( pFileAux );
        fprintf( stdout, "Io_WriteBook(): Cannot open the output files.\n" );
        return;
    }
    fprintf( pFileAux, "RowBasedPlacement : %s.nodes %s.nets %s.scl %s.pl %s.wts", 
         FileName, FileName, FileName, FileName, FileName );
    fclose( pFileAux );

    // write the master network
    coreCellArea+=Io_NtkWriteNodes( pFileNodes, pNtk );
    Io_NtkWriteNets( pFileNets, pNtk );

    // write EXDC network if it exists
    pExdc = Abc_NtkExdc( pNtk );
    if ( pExdc )
    {
    coreCellArea+=Io_NtkWriteNodes( pFileNodes, pNtk );
        Io_NtkWriteNets( pFileNets, pNtk );
    }

    // make sure there is no logic hierarchy
    assert( Abc_NtkWhiteboxNum(pNtk) == 0 );

    // write the hierarchy if present
    if ( Abc_NtkBlackboxNum(pNtk) > 0 )
    {
        Vec_PtrForEachEntry( Abc_Ntk_t *, pNtk->pDesign->vModules, pNtkTemp, i )
        {
            if ( pNtkTemp == pNtk )
                continue;
            coreCellArea+=Io_NtkWriteNodes( pFileNodes, pNtkTemp );
            Io_NtkWriteNets( pFileNets, pNtkTemp );
        }
    }
    fclose( pFileNodes );
    fclose( pFileNets );

    strcpy(FileExt, FileName);
    pFileScl = fopen( strcat(FileExt,".scl"), "w" );
    strcpy(FileExt, FileName);
    pFilePl  = fopen( strcat(FileExt,".pl"), "w" );
    strcpy(FileExt, FileName);
    pFileWts   = fopen( strcat(FileExt,".wts"), "w" );
    ABC_FREE(FileExt);

    Io_NtkBuildLayout( pFileScl, pFilePl, pNtk, 1.0, 10, coreCellArea );
    fclose( pFileScl );
    fclose( pFilePl );
    fclose( pFileWts );
}

/**Function*************************************************************

  Synopsis    [Write the network into a BOOK file with the given name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Io_NtkWriteNodes( FILE * pFile, Abc_Ntk_t * pNtk )
{
    ProgressBar * pProgress;
    Abc_Obj_t * pLatch, * pNode;
    unsigned numTerms, numNodes, coreCellArea=0;
    int i;

    assert( Abc_NtkIsNetlist(pNtk) );
    // write the forehead
    numTerms=Abc_NtkPiNum(pNtk)+Abc_NtkPoNum(pNtk);
    numNodes=numTerms+Abc_NtkNodeNum(pNtk)+Abc_NtkLatchNum(pNtk);
    printf("NumNodes : %d\t", numNodes );
    printf("NumTerminals : %d\n", numTerms );
    fprintf( pFile, "UCLA    nodes    1.0\n");
    fprintf( pFile, "NumNodes : %d\n", numNodes );
    fprintf( pFile, "NumTerminals : %d\n", numTerms );
    // write the PI/POs
    Io_NtkWritePiPoNodes( pFile, pNtk );
    // write the latches
    if ( !Abc_NtkIsComb(pNtk) )
    {
        Abc_NtkForEachLatch( pNtk, pLatch, i )
        {
            Io_NtkWriteLatchNode( pFile, pLatch, NODES );
            coreCellArea+=6*coreHeight;
        }
    }
    // write each internal node
    pProgress = Extra_ProgressBarStart( stdout, Abc_NtkNodeNum(pNtk) );
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        coreCellArea+=Io_NtkWriteIntNode( pFile, pNode, NODES );
    }
    Extra_ProgressBarStop( pProgress );
    return coreCellArea;
}

/**Function*************************************************************

  Synopsis    [Writes the primary input nodes into a file]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWritePiPoNodes( FILE * pFile, Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pTerm, * pNet;
    int i;

    Abc_NtkForEachPi( pNtk, pTerm, i )
    {
        pNet = Abc_ObjFanout0(pTerm);
    fprintf( pFile, "i%s_input\t", Abc_ObjName(pNet) ); 
    fprintf( pFile, "terminal ");
    fprintf( pFile, " %d %d\n", termWidth, termHeight );
    }
    
    Abc_NtkForEachPo( pNtk, pTerm, i )
    {
    pNet = Abc_ObjFanin0(pTerm);
    fprintf( pFile, "o%s_output\t", Abc_ObjName(pNet) ); 
    fprintf( pFile, "terminal ");
    fprintf( pFile, " %d %d\n", termWidth, termHeight );
    }
}

/**Function*************************************************************

  Synopsis    [Write the latch nodes into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteLatchNode( FILE * pFile, Abc_Obj_t * pLatch, int NodesOrPl )
{
    Abc_Obj_t * pNetLi, * pNetLo;
    
    pNetLi = Abc_ObjFanin0( Abc_ObjFanin0(pLatch) );
    pNetLo = Abc_ObjFanout0( Abc_ObjFanout0(pLatch) );
    /// write the latch line
    fprintf( pFile, "%s_%s_latch\t", Abc_ObjName(pNetLi), Abc_ObjName(pNetLo) );
    if (NodesOrPl == NODES)
        fprintf( pFile, " %d %d\n", 6, 1 );
}

/**Function*************************************************************

  Synopsis    [Write the internal node into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Io_NtkWriteIntNode( FILE * pFile, Abc_Obj_t * pNode, int NodesOrPl )
{
    unsigned sizex=0, sizey=coreHeight, isize=0;
    //double nx, ny, xstep, ystep;    
    Abc_Obj_t * pNeti, *pNeto;
    int i;
        
    // write the network after mapping 
    if ( Abc_NtkHasMapping(pNode->pNtk) )
        sizex=Io_NtkWriteNodeGate( pFile, pNode );
    else
    {
    Abc_ObjForEachFanin( pNode, pNeti, i )
        fprintf( pFile, "%s_", Abc_ObjName(pNeti) );
    Abc_ObjForEachFanout( pNode, pNeto, i )
        fprintf( pFile, "%s_", Abc_ObjName(pNeto) );    
    fprintf( pFile, "name\t" );
    
    if(NodesOrPl == NODES)
    {
        isize=Abc_ObjFaninNum(pNode);
        if ( Abc_NodeIsConst0(pNode) || Abc_NodeIsConst1(pNode) ) 
            sizex=0;
        else if ( Abc_NodeIsInv(pNode) )
            sizex=1;
        else if ( Abc_NodeIsBuf(pNode) )
            sizex=2;
        else
        {
            assert( Abc_NtkHasSop(pNode->pNtk) );
            if ( Abc_NodeIsNand2(pNode) || Abc_NodeIsNor2(pNode) )
            sizex=2;
        else if ( Abc_NodeIsAnd2(pNode) || Abc_NodeIsOr2(pNode) )
            sizex=3;
        else if ( Abc_NodeIsXor2(pNode) || Abc_NodeIsXnor2(pNode) )
            sizex=5;
        else
        {
            assert( isize > 2 );
            sizex=isize+Abc_SopGetCubeNum((char *)pNode->pData);
        }
        }
    }
    }
    if(NodesOrPl == NODES)
    {
    fprintf( pFile, " %d %d\n", sizex, sizey );

    // Equally place pins. Size pins needs /  isize+#output+1
    isize= isize + Abc_ObjFanoutNum(pNode) + 1;
    }
    return sizex*sizey;
    /*
    xstep = sizex / isize;
    ystep = sizey / isize;
    nx= -0.5 * sizex;
    ny= -0.5 * sizey;

    Abc_ObjForEachFanin( pNode, pFanin, i )
    {
            nx+= xstep;
            ny+= ystep;
            if (fabs(nx) < 0.001)
                    nx= 0;
            if (fabs(ny) < 0.001)
                    ny= 0;
    }
    Abc_ObjForEachFanout( pNode, pFanout, i )
    {
            nx+= xstep;
            ny+= ystep;
            if (fabs(nx) < 0.001)
                    nx= 0;
            if (fabs(ny) < 0.001)
                    ny= 0;
    }
    */
}

/**Function*************************************************************

  Synopsis    [Writes the internal node after tech mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Io_NtkWriteNodeGate( FILE * pFile, Abc_Obj_t * pNode )
{
    Mio_Gate_t * pGate = (Mio_Gate_t *)pNode->pData;
    Mio_Pin_t * pGatePin;
    int i;
    // write the node gate
    for ( pGatePin = Mio_GateReadPins(pGate), i = 0; pGatePin; pGatePin = Mio_PinReadNext(pGatePin), i++ )
        fprintf( pFile, "%s_", Abc_ObjName( Abc_ObjFanin(pNode,i) ) );
    assert ( i == Abc_ObjFaninNum(pNode) );
    fprintf( pFile, "%s_%s\t", Abc_ObjName( Abc_ObjFanout0(pNode) ), Mio_GateReadName(pGate) );
    return Mio_GateReadArea(pGate);
}

/**Function*************************************************************

  Synopsis    [Write the nets into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteNets( FILE * pFile, Abc_Ntk_t * pNtk )
{
    ProgressBar * pProgress;
    Abc_Obj_t * pNet;
    unsigned numPin=0;
    int i;

    assert( Abc_NtkIsNetlist(pNtk) );
    // write the head
    Abc_NtkForEachNet( pNtk, pNet, i )
    numPin+=Abc_ObjFaninNum(pNet)+Abc_ObjFanoutNum(pNet);
    printf( "NumNets  : %d\t", Abc_NtkNetNum(pNtk) );
    printf( "NumPins      : %d\n\n", numPin );
    fprintf( pFile, "UCLA    nets    1.0\n");
    fprintf( pFile, "NumNets : %d\n", Abc_NtkNetNum(pNtk) );
    fprintf( pFile, "NumPins : %d\n", numPin );

    // write nets
    pProgress = Extra_ProgressBarStart( stdout, Abc_NtkNetNum(pNtk) );
    Abc_NtkForEachNet( pNtk, pNet, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        Io_NtkWriteIntNet( pFile, pNet );
    }
    Extra_ProgressBarStop( pProgress );
}

/**Function*************************************************************

  Synopsis    [Write the nets into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteIntNet( FILE * pFile, Abc_Obj_t * pNet )
{
    Abc_Obj_t * pFanin, * pFanout;
    Abc_Obj_t * pNeti, * pNeto;
    Abc_Obj_t * pNetLi, * pNetLo, * pLatch;
    int i, j;
    int NetDegree=Abc_ObjFaninNum(pNet)+Abc_ObjFanoutNum(pNet);

    fprintf( pFile, "NetDegree\t:\t\t%d\t\t%s\n", NetDegree, Abc_ObjName(Abc_ObjFanin0(pNet)) );

    pFanin=Abc_ObjFanin0(pNet);
    if ( Abc_ObjIsPi(pFanin) )
    fprintf( pFile, "i%s_input I\n", Abc_ObjName(pNet) );
    else 
    {
    if(!Abc_NtkIsComb(pNet->pNtk) && Abc_ObjFaninNum(pFanin) && Abc_ObjIsLatch(Abc_ObjFanin0(pFanin)) )
    {
        pLatch=Abc_ObjFanin0(pFanin);
        pNetLi=Abc_ObjFanin0(Abc_ObjFanin0(pLatch));
        pNetLo=Abc_ObjFanout0(Abc_ObjFanout0(pLatch));
        fprintf( pFile, "%s_%s_latch I : ", Abc_ObjName(pNetLi), Abc_ObjName(pNetLo) );
    }
    else
    {
        Abc_ObjForEachFanin( pFanin, pNeti, j )
        fprintf( pFile, "%s_", Abc_ObjName(pNeti) );
        Abc_ObjForEachFanout( pFanin, pNeto, j )
        fprintf( pFile, "%s_", Abc_ObjName(pNeto) );    
        if ( Abc_NtkHasMapping(pNet->pNtk) )
        fprintf( pFile, "%s : ", Mio_GateReadName((Mio_Gate_t *)pFanin->pData) ); 
        else
        fprintf( pFile, "name I : " );                                
    }
    // offsets are simlply 0.00 0.00 at the moment
    fprintf( pFile, "%.2f %.2f\n", .0, .0 );
    }

    Abc_ObjForEachFanout( pNet, pFanout, i )
    {
    if ( Abc_ObjIsPo(pFanout) )
        fprintf( pFile, "o%s_output O\n", Abc_ObjName(pNet) );
    else
    {
        if(!Abc_NtkIsComb(pNet->pNtk) && Abc_ObjFanoutNum(pFanout) && Abc_ObjIsLatch( Abc_ObjFanout0(pFanout) ) )
        {
        pLatch=Abc_ObjFanout0(pFanout);
        pNetLi=Abc_ObjFanin0(Abc_ObjFanin0(pLatch));
        pNetLo=Abc_ObjFanout0(Abc_ObjFanout0(pLatch));
        fprintf( pFile, "%s_%s_latch O : ", Abc_ObjName(pNetLi), Abc_ObjName(pNetLo) );
        }
        else
        {
        Abc_ObjForEachFanin( pFanout, pNeti, j )
        fprintf( pFile, "%s_", Abc_ObjName(pNeti) );
        Abc_ObjForEachFanout( pFanout, pNeto, j )
        fprintf( pFile, "%s_", Abc_ObjName(pNeto) );    
        if ( Abc_NtkHasMapping(pNet->pNtk) )
            fprintf( pFile, "%s : ", Mio_GateReadName((Mio_Gate_t *)pFanout->pData) ); 
        else
            fprintf( pFile, "name O : " );
        }
    // offsets are simlply 0.00 0.00 at the moment
    fprintf( pFile, "%.2f %.2f\n", .0, .0 );
    }
    }
}

/**Function*************************************************************

  Synopsis    [Write the network into a BOOK file with the given name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkBuildLayout( FILE * pFileScl, FILE * pFilePl, Abc_Ntk_t * pNtk, double aspectRatio, double whiteSpace, unsigned coreCellArea )
{
    unsigned numCoreCells=Abc_NtkNodeNum(pNtk)+Abc_NtkLatchNum(pNtk);
    double targetLayoutArea = coreCellArea/(1.0-(whiteSpace/100.0));
    unsigned numCoreRows=(aspectRatio>0.0) ? (Abc_Rint(sqrt(targetLayoutArea/aspectRatio)/coreHeight)) : 0;
    unsigned numTerms=Abc_NtkPiNum(pNtk)+Abc_NtkPoNum(pNtk);
    unsigned totalWidth=coreCellArea/coreHeight;
    double layoutHeight = numCoreRows * coreHeight;
    double layoutWidth = Abc_Rint(targetLayoutArea/layoutHeight);
    double actualLayoutArea = layoutWidth * layoutHeight;

    printf( "Core cell height(==site height) is %d\n", coreHeight );
    printf( "Total core cell width is %d giving an ave width of %f\n", totalWidth, (double)(totalWidth/numCoreCells));
    printf( "Target Dimensions:\n" );
    printf( "  Area  :   %f\n", targetLayoutArea );
    printf( "  WS%%   :   %f\n", whiteSpace );
    printf( "  AR    :   %f\n", aspectRatio );
    printf( "Actual Dimensions:\n" );
    printf( "  Width :   %f\n", layoutWidth );
    printf( "  Height:   %f (%d rows)\n", layoutHeight, numCoreRows);
    printf( "  Area  :   %f\n", actualLayoutArea );
    printf( "  WS%%   :   %f\n", 100*(actualLayoutArea-coreCellArea)/actualLayoutArea );
    printf( "  AR    :   %f\n\n", layoutWidth/layoutHeight );    

    Io_NtkWriteScl( pFileScl, numCoreRows, layoutWidth ); 
    Io_NtkWritePl( pFilePl, pNtk, numTerms, layoutHeight, layoutWidth );
}

/**Function*************************************************************

  Synopsis    [Write the network into a BOOK file with the given name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteScl( FILE * pFile, unsigned numCoreRows, double layoutWidth )
{
    int origin_y=0;
    char * rowOrients[2] = {"N", "FS"};
    char symmetry='Y';
    double sitewidth=1.0;
    double spacing=1.0;
        
    unsigned rowId;
    // write the forehead
    fprintf( pFile, "UCLA    scl    1.0\n\n" );
    fprintf( pFile, "Numrows : %d\n\n", numCoreRows );

    for( rowId=0 ; rowId<numCoreRows ; rowId++, origin_y += coreHeight )
    {
    fprintf( pFile, "CoreRow Horizontal\n" );
    fprintf( pFile, " Coordinate   : \t%d\n", origin_y);
    fprintf( pFile, " Height       : \t%d\n", coreHeight);
    fprintf( pFile, " Sitewidth    : \t%d\n", (unsigned)sitewidth );
    fprintf( pFile, " Sitespacing  : \t%d\n", (unsigned)spacing );
    fprintf( pFile, " Siteorient   : \t%s\n", rowOrients[rowId%2] );
    //if( coreRow[i].site.symmetry.rot90 || coreRow[i].site.symmetry.y || coreRow[i].site.symmetry.x )
    fprintf( pFile, " Sitesymmetry : \t%c\n", symmetry );
    //else fprintf( pFile, "Sitesymmetry            : \t\t\t1\n" );
    fprintf( pFile, " SubrowOrigin : \t%d Numsites :    \t%d\n", 0, (unsigned)layoutWidth );
    fprintf( pFile, "End\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Write the network into a BOOK file with the given name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWritePl( FILE * pFile, Abc_Ntk_t * pNtk, unsigned numTerms, double layoutWidth, double layoutHeight )
{
    Abc_Obj_t * pTerm, * pLatch, * pNode;
    Vec_Ptr_t * vTerms = Vec_PtrAlloc ( numTerms ); 
    Vec_Ptr_t * vOrderedTerms = Vec_PtrAlloc ( numTerms ); 
    double layoutPerim = 2*layoutWidth + 2*layoutHeight;
    double nextLoc_x, nextLoc_y;
    double delta;
    unsigned termsOnTop, termsOnBottom, termsOnLeft, termsOnRight;
    unsigned t;
    int i;

    termsOnTop = termsOnBottom = (unsigned)(Abc_Rint(numTerms*(layoutWidth/layoutPerim)));
    termsOnLeft = numTerms - (termsOnTop+termsOnBottom);
    termsOnRight = (unsigned)(ceil(termsOnLeft/2.0));
    termsOnLeft -= termsOnRight;

    Abc_NtkForEachPi( pNtk, pTerm, i )
    Vec_PtrPush( vTerms, pTerm );    
    Abc_NtkForEachPo( pNtk, pTerm, i )
    Vec_PtrPush( vTerms, pTerm );
    // Ordering Pads
    vOrderedTerms=Io_NtkOrderingPads( pNtk, vTerms );
    assert( termsOnTop+termsOnBottom+termsOnLeft+termsOnRight == (unsigned)Vec_PtrSize(vOrderedTerms) );

    printf( "Done constructing layout region\n" );
    printf( "Terminals: %d\n", numTerms );
    printf( "  Top:     %d\n", termsOnTop );
    printf( "  Bottom:  %d\n", termsOnBottom );
    printf( "  Left:    %d\n", termsOnLeft );
    printf( "  Right:   %d\n", termsOnRight );

    fprintf( pFile, "UCLA    pl    1.0\n\n" );

    nextLoc_x = floor(.0);
    nextLoc_y = ceil(layoutHeight + 2*coreHeight);
    delta   = layoutWidth / termsOnTop;
    for(t = 0; t < termsOnTop; t++)
    {
    pTerm = (Abc_Obj_t *)Vec_PtrEntry( vOrderedTerms, t );
    if( Abc_ObjIsPi(pTerm) )
        fprintf( pFile, "i%s_input\t\t", Abc_ObjName(Abc_ObjFanout0(pTerm)) ); 
    else
        fprintf( pFile, "o%s_output\t\t", Abc_ObjName(Abc_ObjFanin0(pTerm)) ); 
    if( t && Abc_Rint(nextLoc_x) < Abc_Rint(nextLoc_x-delta)+termWidth )
        nextLoc_x++;
    fprintf( pFile, "%d\t\t%d\t: %s /FIXED\n", (int)Abc_Rint(nextLoc_x), (int)Abc_Rint(nextLoc_y), "FS" );    
    nextLoc_x += delta;
    }

    nextLoc_x = floor(.0);
    nextLoc_y = floor(.0 - 2*coreHeight - termHeight);
    delta   = layoutWidth / termsOnBottom;
    for(;t < termsOnTop+termsOnBottom; t++)
    {
    pTerm = (Abc_Obj_t *)Vec_PtrEntry( vOrderedTerms, t );
    if( Abc_ObjIsPi(pTerm) )
        fprintf( pFile, "i%s_input\t\t", Abc_ObjName(Abc_ObjFanout0(pTerm)) ); 
    else
        fprintf( pFile, "o%s_output\t\t", Abc_ObjName(Abc_ObjFanin0(pTerm)) ); 
    if( t!=termsOnTop && Abc_Rint(nextLoc_x) < Abc_Rint(nextLoc_x-delta)+termWidth )
        nextLoc_x++;
    fprintf( pFile, "%d\t\t%d\t: %s /FIXED\n", (int)Abc_Rint(nextLoc_x), (int)Abc_Rint(nextLoc_y), "N" );    
    nextLoc_x     += delta;
    }

    nextLoc_x = floor(.0-2*coreHeight-termWidth);
    nextLoc_y = floor(.0);
    delta   = layoutHeight / termsOnLeft;
    for(;t < termsOnTop+termsOnBottom+termsOnLeft; t++)
    {
    pTerm = (Abc_Obj_t *)Vec_PtrEntry( vOrderedTerms, t );
    if( Abc_ObjIsPi(pTerm) )
        fprintf( pFile, "i%s_input\t\t", Abc_ObjName(Abc_ObjFanout0(pTerm)) ); 
    else
        fprintf( pFile, "o%s_output\t\t", Abc_ObjName(Abc_ObjFanin0(pTerm)) ); 
    if( Abc_Rint(nextLoc_y) < Abc_Rint(nextLoc_y-delta)+termHeight )
        nextLoc_y++;
    fprintf( pFile, "%d\t\t%d\t: %s /FIXED\n", (int)Abc_Rint(nextLoc_x), (int)Abc_Rint(nextLoc_y), "E" );    
    nextLoc_y     += delta;
    }

    nextLoc_x = ceil(layoutWidth+2*coreHeight);
    nextLoc_y = floor(.0);
    delta   = layoutHeight / termsOnRight;
    for(;t < termsOnTop+termsOnBottom+termsOnLeft+termsOnRight; t++)
    {
           pTerm = (Abc_Obj_t *)Vec_PtrEntry( vOrderedTerms, t );
    if( Abc_ObjIsPi(pTerm) )
        fprintf( pFile, "i%s_input\t\t", Abc_ObjName(Abc_ObjFanout0(pTerm)) ); 
    else
        fprintf( pFile, "o%s_output\t\t", Abc_ObjName(Abc_ObjFanin0(pTerm)) ); 
    if( Abc_Rint(nextLoc_y) < Abc_Rint(nextLoc_y-delta)+termHeight )
        nextLoc_y++;
    fprintf( pFile, "%d\t\t%d\t: %s /FIXED\n", (int)Abc_Rint(nextLoc_x), (int)Abc_Rint(nextLoc_y), "FW" );    
    nextLoc_y     += delta;
    }

    if( !Abc_NtkIsComb(pNtk) )
    {
    Abc_NtkForEachLatch( pNtk, pLatch, i )
    {
        Io_NtkWriteLatchNode( pFile, pLatch, PL );
        fprintf( pFile, "\t%d\t\t%d\t: %s\n", 0, 0, "N" );    
    }
    }
    
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
    Io_NtkWriteIntNode( pFile, pNode, PL );
    fprintf( pFile, "\t%d\t\t%d\t: %s\n", 0, 0, "N" );    
    }
}

/**Function*************************************************************

  Synopsis    [Returns the closest I/O to a given I/O.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Io_NtkOrderingPads( Abc_Ntk_t * pNtk, Vec_Ptr_t * vTerms )
{ 
    ProgressBar * pProgress;
    unsigned numTerms=Vec_PtrSize(vTerms); 
    unsigned termIdx=0, termCount=0;
    int * pOrdered = ABC_ALLOC(int, numTerms); 
    int newNeighbor=1;
    Vec_Ptr_t * vOrderedTerms = Vec_PtrAlloc ( numTerms );
    Abc_Obj_t * pNeighbor = NULL, * pNextTerm;     
    unsigned i; 

    for( i=0 ; i<numTerms ; i++ )
    pOrdered[i]=0; 
   
    pNextTerm = (Abc_Obj_t *)Vec_PtrEntry(vTerms, termIdx++);
    pProgress = Extra_ProgressBarStart( stdout, numTerms );
    while( termCount < numTerms && termIdx < numTerms )
    {
    if( pOrdered[Abc_ObjId(pNextTerm)] && !newNeighbor )
    {
        pNextTerm = (Abc_Obj_t *)Vec_PtrEntry( vTerms, termIdx++ );
        continue;
    }
    if(!Vec_PtrPushUnique( vOrderedTerms, pNextTerm ))
    {
        pOrdered[Abc_ObjId(pNextTerm)]=1;
        termCount++; 
    }
    pNeighbor=Io_NtkBfsPads( pNtk, pNextTerm, numTerms, pOrdered );
    if( (newNeighbor=!Vec_PtrPushUnique( vOrderedTerms, pNeighbor )) )
    {
       pOrdered[Abc_ObjId(pNeighbor)]=1;
       termCount++;
       pNextTerm=pNeighbor;
    }
    else if(termIdx < numTerms)
        pNextTerm = (Abc_Obj_t *)Vec_PtrEntry( vTerms, termIdx++ );

    Extra_ProgressBarUpdate( pProgress, termCount, NULL );
    }
    Extra_ProgressBarStop( pProgress );
    assert(termCount==numTerms);
    return vOrderedTerms;
}

/**Function*************************************************************

  Synopsis    [Returns the closest I/O to a given I/O.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Io_NtkBfsPads( Abc_Ntk_t * pNtk, Abc_Obj_t * pTerm, unsigned numTerms, int * pOrdered )
{
    Vec_Ptr_t * vNeighbors = Vec_PtrAlloc ( numTerms ); 
    Abc_Obj_t * pNet, * pNode, * pNeighbor;
    int foundNeighbor=0;
    int i;

    assert(Abc_ObjIsPi(pTerm) || Abc_ObjIsPo(pTerm) );
    Abc_NtkIncrementTravId ( pNtk );
    Abc_NodeSetTravIdCurrent( pTerm );
    if(Abc_ObjIsPi(pTerm))
    {
    pNet = Abc_ObjFanout0(pTerm);
    Abc_ObjForEachFanout( pNet, pNode, i ) 
        Vec_PtrPush( vNeighbors, pNode );
    }
    else 
    {
    pNet = Abc_ObjFanin0(pTerm);
    Abc_ObjForEachFanin( pNet, pNode, i ) 
        Vec_PtrPush( vNeighbors, pNode );   
    }

    while ( Vec_PtrSize(vNeighbors) >0 )
    {
    pNeighbor = (Abc_Obj_t *)Vec_PtrEntry( vNeighbors, 0 );
    assert( Abc_ObjIsNode(pNeighbor) || Abc_ObjIsTerm(pNeighbor) );
    Vec_PtrRemove( vNeighbors, pNeighbor );

    if( Abc_NodeIsTravIdCurrent( pNeighbor ) )
        continue;
    Abc_NodeSetTravIdCurrent( pNeighbor );

    if( ((Abc_ObjIsPi(pNeighbor) || Abc_ObjIsPo(pNeighbor))) && !pOrdered[Abc_ObjId(pNeighbor)] )
    {
        foundNeighbor=1;
        break;
    }
    if( Abc_ObjFanoutNum( pNeighbor ) )
    {   
        pNet=Abc_ObjFanout0( pNeighbor );
        if( !Abc_NtkIsComb(pNtk) && Abc_ObjIsLatch(pNet) )
        pNet=Abc_ObjFanout0( Abc_ObjFanout0(pNet) );
        Abc_ObjForEachFanout( pNet, pNode, i )
        if( !Abc_NodeIsTravIdCurrent(pNode) ) 
            Vec_PtrPush( vNeighbors, pNode ); 
    }
    if( Abc_ObjFaninNum( pNeighbor ) )
    {
        if( !Abc_NtkIsComb(pNtk) && Abc_ObjIsLatch(Abc_ObjFanin0(pNeighbor)) )
        pNeighbor=Abc_ObjFanin0( Abc_ObjFanin0(pNeighbor) );
        Abc_ObjForEachFanin( pNeighbor, pNet, i )
        if( !Abc_NodeIsTravIdCurrent(pNode=Abc_ObjFanin0(pNet)) ) 
            Vec_PtrPush( vNeighbors, pNode );
    }
    }
    return ( foundNeighbor ) ? pNeighbor : pTerm;
} 

/**Function*************************************************************

  Synopsis    [Test is the node is nand2.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeIsNand2( Abc_Obj_t * pNode )
{
    Abc_Ntk_t * pNtk = pNode->pNtk;
    assert( Abc_NtkIsNetlist(pNtk) );
    assert( Abc_ObjIsNode(pNode) ); 
    if ( Abc_ObjFaninNum(pNode) != 2 )
        return 0;
    if ( Abc_NtkHasSop(pNtk) )
    return ( !strcmp(((char *)pNode->pData), "-0 1\n0- 1\n") || 
         !strcmp(((char *)pNode->pData), "0- 1\n-0 1\n") || 
         !strcmp(((char *)pNode->pData), "11 0\n") );
    if ( Abc_NtkHasMapping(pNtk) )
        return pNode->pData == (void *)Mio_LibraryReadNand2((Mio_Library_t *)Abc_FrameReadLibGen());
    assert( 0 );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Test is the node is nand2.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeIsNor2( Abc_Obj_t * pNode )
{
    Abc_Ntk_t * pNtk = pNode->pNtk;
    assert( Abc_NtkIsNetlist(pNtk) );
    assert( Abc_ObjIsNode(pNode) ); 
    if ( Abc_ObjFaninNum(pNode) != 2 )
        return 0;
    if ( Abc_NtkHasSop(pNtk) )
        return ( !strcmp(((char *)pNode->pData), "00 1\n") );
    assert( 0 );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Test is the node is and2.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeIsAnd2( Abc_Obj_t * pNode )
{
    Abc_Ntk_t * pNtk = pNode->pNtk;
    assert( Abc_NtkIsNetlist(pNtk) );
    assert( Abc_ObjIsNode(pNode) ); 
    if ( Abc_ObjFaninNum(pNode) != 2 )
        return 0;
    if ( Abc_NtkHasSop(pNtk) )
    return Abc_SopIsAndType(((char *)pNode->pData));
    if ( Abc_NtkHasMapping(pNtk) )
        return pNode->pData == (void *)Mio_LibraryReadAnd2((Mio_Library_t *)Abc_FrameReadLibGen());
    assert( 0 );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Test is the node is or2.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeIsOr2( Abc_Obj_t * pNode )
{
    Abc_Ntk_t * pNtk = pNode->pNtk;
    assert( Abc_NtkIsNetlist(pNtk) );
    assert( Abc_ObjIsNode(pNode) ); 
    if ( Abc_ObjFaninNum(pNode) != 2 )
        return 0;
    if ( Abc_NtkHasSop(pNtk) )
    return ( Abc_SopIsOrType(((char *)pNode->pData))   || 
         !strcmp(((char *)pNode->pData), "01 0\n") ||
         !strcmp(((char *)pNode->pData), "10 0\n") ||
         !strcmp(((char *)pNode->pData), "00 0\n") );
         //off-sets, too
    assert( 0 );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Test is the node is xor2.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeIsXor2( Abc_Obj_t * pNode )
{
    Abc_Ntk_t * pNtk = pNode->pNtk;
    assert( Abc_NtkIsNetlist(pNtk) );
    assert( Abc_ObjIsNode(pNode) ); 
    if ( Abc_ObjFaninNum(pNode) != 2 )
        return 0;
    if ( Abc_NtkHasSop(pNtk) )
    return ( !strcmp(((char *)pNode->pData), "01 1\n10 1\n") || !strcmp(((char *)pNode->pData), "10 1\n01 1\n")  );
    assert( 0 );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Test is the node is xnor2.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeIsXnor2( Abc_Obj_t * pNode )
{
    Abc_Ntk_t * pNtk = pNode->pNtk;
    assert( Abc_NtkIsNetlist(pNtk) );
    assert( Abc_ObjIsNode(pNode) ); 
    if ( Abc_ObjFaninNum(pNode) != 2 )
        return 0;
    if ( Abc_NtkHasSop(pNtk) )
    return ( !strcmp(((char *)pNode->pData), "11 1\n00 1\n") || !strcmp(((char *)pNode->pData), "00 1\n11 1\n") );
    assert( 0 );
    return 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

