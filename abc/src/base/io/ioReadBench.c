/**CFile****************************************************************

  FileName    [ioReadBench.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedures to read BENCH files.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: ioReadBench.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ioAbc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Abc_Ntk_t * Io_ReadBenchNetwork( Extra_FileReader_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reads the network from a BENCH file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Io_ReadBench( char * pFileName, int fCheck )
{
    Extra_FileReader_t * p;
    Abc_Ntk_t * pNtk;

    // start the file
    p = Extra_FileReaderAlloc( pFileName, "#", "\n\r", " \t,()=" );
    if ( p == NULL )
        return NULL;

    // read the network
    pNtk = Io_ReadBenchNetwork( p );
    Extra_FileReaderFree( p );
    if ( pNtk == NULL )
        return NULL;

    // make sure that everything is okay with the network structure
    if ( fCheck && !Abc_NtkCheckRead( pNtk ) )
    {
        printf( "Io_ReadBench: The network check has failed.\n" );
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
Abc_Ntk_t * Io_ReadBenchNetwork( Extra_FileReader_t * p )
{
    ProgressBar * pProgress;
    Vec_Ptr_t * vTokens;
    Abc_Ntk_t * pNtk;
    Abc_Obj_t * pNode, * pNet;
    Vec_Str_t * vString;
    unsigned uTruth[8];
    char * pType, ** ppNames, * pString;
    int iLine, nNames, nDigits, fLutsPresent = 0;
    
    // allocate the empty network
    pNtk = Abc_NtkStartRead( Extra_FileReaderGetFileName(p) );
    pNtk->nConstrs = 0;

    // go through the lines of the file
    vString = Vec_StrAlloc( 100 );
    pProgress = Extra_ProgressBarStart( stdout, Extra_FileReaderGetFileSize(p) );
    for ( iLine = 0; (vTokens = (Vec_Ptr_t *)Extra_FileReaderGetTokens(p)); iLine++ )
    {
        Extra_ProgressBarUpdate( pProgress, Extra_FileReaderGetCurPosition(p), NULL );

        if ( vTokens->nSize == 1 )
        {
            printf( "%s: Wrong input file format.\n", Extra_FileReaderGetFileName(p) );
            Vec_StrFree( vString );
            Abc_NtkDelete( pNtk );
            return NULL;
        }

        // get the type of the line
        if ( strncmp( (char *)vTokens->pArray[0], "INPUT", 5 ) == 0 )
            Io_ReadCreatePi( pNtk, (char *)vTokens->pArray[1] );
        else if ( strncmp( (char *)vTokens->pArray[0], "OUTPUT", 5 ) == 0 )
            Io_ReadCreatePo( pNtk, (char *)vTokens->pArray[1] );
        else 
        {
            // get the node name and the node type
            pType = (char *)vTokens->pArray[1];
            if ( strncmp(pType, "DFF", 3) == 0 ) // works for both DFF and DFFRSE
            {
                if ( Vec_PtrSize(vTokens) == 6 )
                {
                    // create primary input to represent flop output
                    char pNetName[1000]; char * pName; int i;
                    char * pFlopOut   = (char *)vTokens->pArray[0];
                    Abc_Obj_t * pNet  = Abc_NtkFindOrCreateNet( pNtk, pFlopOut );
                    Abc_Obj_t * pTerm = Abc_NtkCreatePi( pNtk );
                    Abc_ObjAddFanin( pNet, pTerm );
                    // create four primary outputs to represent flop inputs
                    Vec_PtrForEachEntryStart( char *, vTokens, pName, i, 2 )
                    {
                        sprintf( pNetName, "%s_%s", pFlopOut, pName );
                        pNet  = Abc_NtkFindOrCreateNet( pNtk, pName );
                        pTerm = Abc_NtkCreateNodeBuf( pNtk, pNet );
                        pNet  = Abc_NtkFindOrCreateNet( pNtk, pNetName );
                        Abc_ObjAddFanin( pNet, pTerm );
                        pTerm = Abc_NtkCreatePo( pNtk );
                        Abc_ObjAddFanin( pTerm, pNet );
                    }
                    pNtk->nConstrs++;
                }
                else
                {
                    pNode = Io_ReadCreateLatch( pNtk, (char *)vTokens->pArray[2], (char *)vTokens->pArray[0] );
    //                Abc_LatchSetInit0( pNode );
                    if ( pType[3] == '0' )
                        Abc_LatchSetInit0( pNode );
                    else if ( pType[3] == '1' )
                        Abc_LatchSetInit1( pNode );
                    else
                        Abc_LatchSetInitDc( pNode );
                }
            }
            else if ( strcmp(pType, "LUT") == 0 )
            {
                fLutsPresent = 1;
                ppNames = (char **)vTokens->pArray + 3;
                nNames  = vTokens->nSize - 3;
                // check the number of inputs
                if ( nNames > 8 )
                {
                    printf( "%s: Currently cannot read truth tables with more than 8 inputs (%d).\n", Extra_FileReaderGetFileName(p), nNames );
                    Vec_StrFree( vString );
                    Abc_NtkDelete( pNtk );
                    return NULL;
                }
                // get the hex string
                pString = (char *)vTokens->pArray[2];
                if ( strncmp( pString, "0x", 2 ) )
                {
                    printf( "%s: The LUT signature (%s) does not look like a hexadecimal beginning with \"0x\".\n", Extra_FileReaderGetFileName(p), pString );
                    Vec_StrFree( vString );
                    Abc_NtkDelete( pNtk );
                    return NULL;
                }
                pString += 2;
                // pad the string with zero's if needed
                nDigits = (1 << nNames) / 4;
                if ( nDigits == 0 )
                    nDigits = 1;
                if ( strlen(pString) < (unsigned)nDigits )
                {
                    Vec_StrFill( vString, nDigits - strlen(pString), '0' );
                    Vec_StrPrintStr( vString, pString );
                    Vec_StrPush( vString, 0 );
                    pString = Vec_StrArray( vString );
                }
                // read the hex number from the string
                if ( !Extra_ReadHexadecimal( uTruth, pString, nNames ) )
                {
                    printf( "%s: Reading hexadecimal number (%s) has failed.\n", Extra_FileReaderGetFileName(p), pString );
                    Vec_StrFree( vString );
                    Abc_NtkDelete( pNtk );
                    return NULL;
                }
                // check if the node is a constant node
                if ( Extra_TruthIsConst0(uTruth, nNames) )
                {
                    pNode = Io_ReadCreateNode( pNtk, (char *)vTokens->pArray[0], ppNames, 0 );
                    Abc_ObjSetData( pNode, Abc_SopRegister( (Mem_Flex_t *)pNtk->pManFunc, " 0\n" ) );
                }
                else if ( Extra_TruthIsConst1(uTruth, nNames) )
                {
                    pNode = Io_ReadCreateNode( pNtk, (char *)vTokens->pArray[0], ppNames, 0 );
                    Abc_ObjSetData( pNode, Abc_SopRegister( (Mem_Flex_t *)pNtk->pManFunc, " 1\n" ) );
                }
                else
                {
                    // create the node
                    pNode = Io_ReadCreateNode( pNtk, (char *)vTokens->pArray[0], ppNames, nNames );
                    assert( nNames > 0 );
                    if ( nNames > 1 )
                        Abc_ObjSetData( pNode, Abc_SopCreateFromTruth((Mem_Flex_t *)pNtk->pManFunc, nNames, uTruth) );
                    else if ( pString[0] == '2' )
                        Abc_ObjSetData( pNode, Abc_SopCreateBuf((Mem_Flex_t *)pNtk->pManFunc) );
                    else if ( pString[0] == '1' )
                        Abc_ObjSetData( pNode, Abc_SopCreateInv((Mem_Flex_t *)pNtk->pManFunc) );
                    else
                    {
                        printf( "%s: Reading truth table (%s) of single-input node has failed.\n", Extra_FileReaderGetFileName(p), pString );
                        Vec_StrFree( vString );
                        Abc_NtkDelete( pNtk );
                        return NULL;
                    }
                }
            }
            else
            {
                // create a new node and add it to the network
                ppNames = (char **)vTokens->pArray + 2;
                nNames  = vTokens->nSize - 2;
                pNode = Io_ReadCreateNode( pNtk, (char *)vTokens->pArray[0], ppNames, nNames );
                // assign the cover
                if ( strcmp(pType, "AND") == 0 )
                    Abc_ObjSetData( pNode, Abc_SopCreateAnd((Mem_Flex_t *)pNtk->pManFunc, nNames, NULL) );
                else if ( strcmp(pType, "OR") == 0 )
                    Abc_ObjSetData( pNode, Abc_SopCreateOr((Mem_Flex_t *)pNtk->pManFunc, nNames, NULL) );
                else if ( strcmp(pType, "NAND") == 0 )
                    Abc_ObjSetData( pNode, Abc_SopCreateNand((Mem_Flex_t *)pNtk->pManFunc, nNames) );
                else if ( strcmp(pType, "NOR") == 0 )
                    Abc_ObjSetData( pNode, Abc_SopCreateNor((Mem_Flex_t *)pNtk->pManFunc, nNames) );
                else if ( strcmp(pType, "XOR") == 0 )
                    Abc_ObjSetData( pNode, Abc_SopCreateXor((Mem_Flex_t *)pNtk->pManFunc, nNames) );
                else if ( strcmp(pType, "NXOR") == 0 || strcmp(pType, "XNOR") == 0 )
                    Abc_ObjSetData( pNode, Abc_SopCreateNxor((Mem_Flex_t *)pNtk->pManFunc, nNames) );
                else if ( strncmp(pType, "BUF", 3) == 0 )
                    Abc_ObjSetData( pNode, Abc_SopCreateBuf((Mem_Flex_t *)pNtk->pManFunc) );
                else if ( strcmp(pType, "NOT") == 0 )
                    Abc_ObjSetData( pNode, Abc_SopCreateInv((Mem_Flex_t *)pNtk->pManFunc) );
                else if ( strncmp(pType, "MUX", 3) == 0 )
//                    Abc_ObjSetData( pNode, Abc_SopRegister(pNtk->pManFunc, "1-0 1\n-11 1\n") );
                    Abc_ObjSetData( pNode, Abc_SopRegister((Mem_Flex_t *)pNtk->pManFunc, "0-1 1\n11- 1\n") );
                else if ( strncmp(pType, "gnd", 3) == 0 )
                    Abc_ObjSetData( pNode, Abc_SopRegister( (Mem_Flex_t *)pNtk->pManFunc, " 0\n" ) );
                else if ( strncmp(pType, "vdd", 3) == 0 )
                    Abc_ObjSetData( pNode, Abc_SopRegister( (Mem_Flex_t *)pNtk->pManFunc, " 1\n" ) );
                else
                {
                    printf( "Io_ReadBenchNetwork(): Cannot determine gate type \"%s\" in line %d.\n", pType, Extra_FileReaderGetLineNumber(p, 0) );
                    Vec_StrFree( vString );
                    Abc_NtkDelete( pNtk );
                    return NULL;
                }
            }
        }
    }
    Extra_ProgressBarStop( pProgress );
    Vec_StrFree( vString );

    // check if constant 0 is present
    if ( (pNet = Abc_NtkFindNet( pNtk, "gnd" )) )
    {
        if ( Abc_ObjFaninNum(pNet) == 0 )
            Io_ReadCreateConst( pNtk, "gnd", 0 );
    }
    if ( (pNet = Abc_NtkFindNet( pNtk, "1" )) )
    {
        if ( Abc_ObjFaninNum(pNet) == 0 )
        {
            printf( "Io_ReadBenchNetwork(): Adding constant 0 fanin to non-driven net \"1\".\n" );
            Io_ReadCreateConst( pNtk, "1", 0 );
        }
    }
    // check if constant 1 is present
    if ( (pNet = Abc_NtkFindNet( pNtk, "vdd" )) )
    {
        if ( Abc_ObjFaninNum(pNet) == 0 )
            Io_ReadCreateConst( pNtk, "vdd", 1 );
    }
    if ( (pNet = Abc_NtkFindNet( pNtk, "2" )) )
    {
        if ( Abc_ObjFaninNum(pNet) == 0 )
        {
            printf( "Io_ReadBenchNetwork(): Adding constant 1 fanin to non-driven net \"2\".\n" );
            Io_ReadCreateConst( pNtk, "2", 1 );
        }
    }

    Abc_NtkFinalizeRead( pNtk );

    // if LUTs are present, collapse the truth tables into cubes
    if ( fLutsPresent )
    {
        if ( !Abc_NtkToBdd(pNtk) )
        {
            printf( "Io_ReadBenchNetwork(): Converting to BDD has failed.\n" );
            Abc_NtkDelete( pNtk );
            return NULL;
        }
        if ( !Abc_NtkToSop(pNtk, -1, ABC_INFINITY) )
        {
            printf( "Io_ReadBenchNetwork(): Converting to SOP has failed.\n" );
            Abc_NtkDelete( pNtk );
            return NULL;
        }
    }
    return pNtk;
}


/**Function*************************************************************

  Synopsis    [Reads initial state in BENCH format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_ReadBenchInit( Abc_Ntk_t * pNtk, char * pFileName )
{
    char pBuffer[1000];
    FILE * pFile;
    char * pToken;
    Abc_Obj_t * pObj;
    int Num;
    pFile = fopen( pFileName, "r" );
    if ( pFile == NULL )
    {
        printf( "Io_ReadBenchInit(): Failed to open file \"%s\".\n", pFileName );
        return;
    }
    while ( fgets( pBuffer, 999, pFile ) )
    {
        pToken = strtok( pBuffer, " \n\t\r" );
        // find the latch output
        Num = Nm_ManFindIdByName( pNtk->pManName, pToken, ABC_OBJ_BO );
        if ( Num < 0 )
        {
            printf( "Io_ReadBenchInit(): Cannot find register with output %s.\n", pToken );
            continue;
        }
        pObj = Abc_ObjFanin0( Abc_NtkObj( pNtk, Num ) );
        if ( !Abc_ObjIsLatch(pObj) )
        {
            printf( "Io_ReadBenchInit(): The signal is not a register output %s.\n", pToken );
            continue;
        }
        // assign the new init state
        pToken = strtok( NULL, " \n\t\r" );
        if ( pToken[0] == '0' )
            Abc_LatchSetInit0( pObj );
        else if ( pToken[0] == '1' )
            Abc_LatchSetInit1( pObj );
        else if ( pToken[0] == '2' )
            Abc_LatchSetInitDc( pObj );
        else
        {
            printf( "Io_ReadBenchInit(): The signal %s has unknown initial value (%s).\n", 
                Abc_ObjName(Abc_ObjFanout0(pObj)), pToken );
            continue;
        }
    }
    fclose( pFile );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_IMPL_END

