/**CFile****************************************************************

  FileName    [ioReadEdif.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedure to read ISCAS benchmarks in EDIF.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: ioReadEdif.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ioAbc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Abc_Ntk_t * Io_ReadEdifNetwork( Extra_FileReader_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reads the network from an EDIF file.]

  Description [Works only for the ISCAS benchmarks.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Io_ReadEdif( char * pFileName, int fCheck )
{
    Extra_FileReader_t * p;
    Abc_Ntk_t * pNtk;

    printf( "Currently this parser does not work!\n" );
    return NULL;

    // start the file
    p = Extra_FileReaderAlloc( pFileName, "#", "\n\r", " \t()" );
    if ( p == NULL )
        return NULL;

    // read the network
    pNtk = Io_ReadEdifNetwork( p );
    Extra_FileReaderFree( p );
    if ( pNtk == NULL )
        return NULL;

    // make sure that everything is okay with the network structure
    if ( fCheck && !Abc_NtkCheckRead( pNtk ) )
    {
        printf( "Io_ReadEdif: The network check has failed.\n" );
        Abc_NtkDelete( pNtk );
        return NULL;
    }
    return pNtk;
}
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Io_ReadEdifNetwork( Extra_FileReader_t * p )
{
    ProgressBar * pProgress;
    Vec_Ptr_t * vTokens;
    Abc_Ntk_t * pNtk;
    Abc_Obj_t * pNet, * pObj, * pFanout;
    char * pGateName, * pNetName;
    int fTokensReady, iLine, i;

    // read the first line
    vTokens = (Vec_Ptr_t *)Extra_FileReaderGetTokens(p);
    if ( strcmp( (char *)vTokens->pArray[0], "edif" ) != 0 )
    {
        printf( "%s: Wrong input file format.\n", Extra_FileReaderGetFileName(p) );
        return NULL;
    }
    
    // allocate the empty network
    pNtk = Abc_NtkStartRead( Extra_FileReaderGetFileName(p) );

    // go through the lines of the file
    fTokensReady = 0;
    pProgress = Extra_ProgressBarStart( stdout, Extra_FileReaderGetFileSize(p) );
    for ( iLine = 1; fTokensReady || (vTokens = (Vec_Ptr_t *)Extra_FileReaderGetTokens(p)); iLine++ )
    {
        Extra_ProgressBarUpdate( pProgress, Extra_FileReaderGetCurPosition(p), NULL );

        // get the type of the line
        fTokensReady = 0;
        if ( strcmp( (char *)vTokens->pArray[0], "instance" ) == 0 )
        { 
            pNetName = (char *)vTokens->pArray[1];
            pNet = Abc_NtkFindOrCreateNet( pNtk, pNetName );
            vTokens = (Vec_Ptr_t *)Extra_FileReaderGetTokens(p);
            vTokens = (Vec_Ptr_t *)Extra_FileReaderGetTokens(p);
            pGateName = (char *)vTokens->pArray[1];
            if ( strncmp( pGateName, "Flip", 4 ) == 0 )
            {
                pObj = Abc_NtkCreateLatch( pNtk );
                Abc_LatchSetInit0( pObj );
            }
            else
            {
                pObj = Abc_NtkCreateNode( pNtk );
//                pObj->pData = Abc_NtkRegisterName( pNtk, pGateName );
                pObj->pData = Extra_UtilStrsav( pGateName ); // memory leak!!!
            }
            Abc_ObjAddFanin( pNet, pObj );
        }
        else if ( strcmp( (char *)vTokens->pArray[0], "net" ) == 0 )
        {
            pNetName = (char *)vTokens->pArray[1];
            if ( strcmp( pNetName, "CK" ) == 0 || strcmp( pNetName, "RESET" ) == 0 )
                continue;
            if ( strcmp( pNetName + strlen(pNetName) - 4, "_out" ) == 0 )
                pNetName[strlen(pNetName) - 4] = 0;
            pNet = Abc_NtkFindNet( pNtk, pNetName );
            assert( pNet );
            vTokens = (Vec_Ptr_t *)Extra_FileReaderGetTokens(p);
            vTokens = (Vec_Ptr_t *)Extra_FileReaderGetTokens(p);
            vTokens = (Vec_Ptr_t *)Extra_FileReaderGetTokens(p);
            while ( strcmp( (char *)vTokens->pArray[0], "portRef" ) == 0 )
            {
                if ( strcmp( pNetName, (char *)vTokens->pArray[3] ) != 0 )
                {
                    pFanout = Abc_NtkFindNet( pNtk, (char *)vTokens->pArray[3] );
                    Abc_ObjAddFanin( Abc_ObjFanin0(pFanout), pNet );
                }
                vTokens = (Vec_Ptr_t *)Extra_FileReaderGetTokens(p);
            }
            fTokensReady = 1;
        }
        else if ( strcmp( (char *)vTokens->pArray[0], "library" ) == 0 )
        {
            vTokens = (Vec_Ptr_t *)Extra_FileReaderGetTokens(p);
            vTokens = (Vec_Ptr_t *)Extra_FileReaderGetTokens(p);
            vTokens = (Vec_Ptr_t *)Extra_FileReaderGetTokens(p);
            vTokens = (Vec_Ptr_t *)Extra_FileReaderGetTokens(p);
            vTokens = (Vec_Ptr_t *)Extra_FileReaderGetTokens(p);
            while ( strcmp( (char *)vTokens->pArray[0], "port" ) == 0 )
            {
                pNetName = (char *)vTokens->pArray[1];
                if ( strcmp( pNetName, "CK" ) == 0 || strcmp( pNetName, "RESET" ) == 0 )
                {
                    vTokens = (Vec_Ptr_t *)Extra_FileReaderGetTokens(p);
                    continue;
                }
                if ( strcmp( pNetName + strlen(pNetName) - 3, "_PO" ) == 0 )
                    pNetName[strlen(pNetName) - 3] = 0;
                if ( strcmp( (char *)vTokens->pArray[3], "INPUT" ) == 0 )
                    Io_ReadCreatePi( pNtk, (char *)vTokens->pArray[1] );
                else if ( strcmp( (char *)vTokens->pArray[3], "OUTPUT" ) == 0 )
                    Io_ReadCreatePo( pNtk, (char *)vTokens->pArray[1] );
                else
                {
                    printf( "%s (line %d): Wrong interface specification.\n", Extra_FileReaderGetFileName(p), iLine );
                    Abc_NtkDelete( pNtk );
                    return NULL;
                }
                vTokens = (Vec_Ptr_t *)Extra_FileReaderGetTokens(p);
            }
        }
        else if ( strcmp( (char *)vTokens->pArray[0], "design" ) == 0 )
        {
            ABC_FREE( pNtk->pName ); 
            pNtk->pName = (char *)Extra_UtilStrsav( (char *)vTokens->pArray[3] );
            break;
        }
    }
    Extra_ProgressBarStop( pProgress );

    // assign logic functions
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        if ( strncmp( (char *)pObj->pData, "And", 3 ) == 0 )
            Abc_ObjSetData( pObj, Abc_SopCreateAnd((Mem_Flex_t *)pNtk->pManFunc, Abc_ObjFaninNum(pObj), NULL) );
        else if ( strncmp( (char *)pObj->pData, "Or", 2 ) == 0 )
            Abc_ObjSetData( pObj, Abc_SopCreateOr((Mem_Flex_t *)pNtk->pManFunc, Abc_ObjFaninNum(pObj), NULL) );
        else if ( strncmp( (char *)pObj->pData, "Nand", 4 ) == 0 )
            Abc_ObjSetData( pObj, Abc_SopCreateNand((Mem_Flex_t *)pNtk->pManFunc, Abc_ObjFaninNum(pObj)) );
        else if ( strncmp( (char *)pObj->pData, "Nor", 3 ) == 0 )
            Abc_ObjSetData( pObj, Abc_SopCreateNor((Mem_Flex_t *)pNtk->pManFunc, Abc_ObjFaninNum(pObj)) );
        else if ( strncmp( (char *)pObj->pData, "Exor", 4 ) == 0 )
            Abc_ObjSetData( pObj, Abc_SopCreateXor((Mem_Flex_t *)pNtk->pManFunc, Abc_ObjFaninNum(pObj)) );
        else if ( strncmp( (char *)pObj->pData, "Exnor", 5 ) == 0 )
            Abc_ObjSetData( pObj, Abc_SopCreateNxor((Mem_Flex_t *)pNtk->pManFunc, Abc_ObjFaninNum(pObj)) );
        else if ( strncmp( (char *)pObj->pData, "Inv", 3 ) == 0 )
            Abc_ObjSetData( pObj, Abc_SopCreateInv((Mem_Flex_t *)pNtk->pManFunc) );
        else if ( strncmp( (char *)pObj->pData, "Buf", 3 ) == 0 )
            Abc_ObjSetData( pObj, Abc_SopCreateBuf((Mem_Flex_t *)pNtk->pManFunc) );
        else
        {
            printf( "%s: Unknown gate type \"%s\".\n", Extra_FileReaderGetFileName(p), (char*)pObj->pData );
            Abc_NtkDelete( pNtk );
            return NULL;
        }
    }
    // check if constants have been added
//    if ( pNet = Abc_NtkFindNet( pNtk, "VDD" ) )
//        Io_ReadCreateConst( pNtk, "VDD", 1 );
//    if ( pNet = Abc_NtkFindNet( pNtk, "GND" ) )
//        Io_ReadCreateConst( pNtk, "GND", 0 );

    Abc_NtkFinalizeRead( pNtk );
    return pNtk;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_IMPL_END

