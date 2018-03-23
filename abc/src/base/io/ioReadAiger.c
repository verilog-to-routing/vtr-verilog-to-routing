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

// The code in this file is developed in collaboration with Mark Jarvin of Toronto.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "misc/bzlib/bzlib.h"
#include "misc/zlib/zlib.h"
#include "ioAbc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Extracts one unsigned AIG edge from the input buffer.]

  Description [This procedure is a slightly modified version of Armin Biere's
  procedure "unsigned decode (FILE * file)". ]
  
  SideEffects [Updates the current reading position.]

  SeeAlso     []

***********************************************************************/
static inline unsigned Io_ReadAigerDecode( char ** ppPos )
{
    unsigned x = 0, i = 0;
    unsigned char ch;

//    while ((ch = getnoneofch (file)) & 0x80)
    while ((ch = *(*ppPos)++) & 0x80)
        x |= (ch & 0x7f) << (7 * i++);

    return x | (ch << (7 * i));
}

/**Function*************************************************************

  Synopsis    [Decodes the encoded array of literals.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Io_WriteDecodeLiterals( char ** ppPos, int nEntries )
{
    Vec_Int_t * vLits;
    int Lit, LitPrev, Diff, i;
    vLits = Vec_IntAlloc( nEntries );
    LitPrev = Io_ReadAigerDecode( ppPos );
    Vec_IntPush( vLits, LitPrev );
    for ( i = 1; i < nEntries; i++ )
    {
//        Diff = Lit - LitPrev;
//        Diff = (Lit < LitPrev)? -Diff : Diff;
//        Diff = ((2 * Diff) << 1) | (int)(Lit < LitPrev);
        Diff = Io_ReadAigerDecode( ppPos );
        Diff = (Diff & 1)? -(Diff >> 1) : Diff >> 1;
        Lit  = Diff + LitPrev;
        Vec_IntPush( vLits, Lit );
        LitPrev = Lit;
    }
    return vLits;
}


/**Function*************************************************************

  Synopsis    [Reads the file into a character buffer.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
typedef struct buflist {
  char buf[1<<20];
  int nBuf;
  struct buflist * next;
} buflist;

static char * Ioa_ReadLoadFileBz2Aig( char * pFileName, int * pFileSize )
{
    FILE    * pFile;
    int       nFileSize = 0;
    char    * pContents;
    BZFILE  * b;
    int       bzError;
    struct buflist * pNext;
    buflist * bufHead = NULL, * buf = NULL;
    int RetValue;

    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Ioa_ReadLoadFileBz2(): The file is unavailable (absent or open).\n" );
        return NULL;
    }
    b = BZ2_bzReadOpen(&bzError,pFile,0,0,NULL,0);
    if (bzError != BZ_OK) {
        printf( "Ioa_ReadLoadFileBz2(): BZ2_bzReadOpen() failed with error %d.\n",bzError );
        return NULL;
    }
    do {
        if (!bufHead)
            buf = bufHead = ABC_ALLOC( buflist, 1 );
        else
            buf = buf->next = ABC_ALLOC( buflist, 1 );
        nFileSize += buf->nBuf = BZ2_bzRead(&bzError,b,buf->buf,1<<20);
        buf->next = NULL;
    } while (bzError == BZ_OK);
    if (bzError == BZ_STREAM_END) {
        // we're okay
        char * p;
        int nBytes = 0;
        BZ2_bzReadClose(&bzError,b);
        p = pContents = ABC_ALLOC( char, nFileSize + 10 );
        buf = bufHead;
        do {
            memcpy(p+nBytes,buf->buf,buf->nBuf);
            nBytes += buf->nBuf;
//        } while((buf = buf->next));
            pNext = buf->next;
            ABC_FREE( buf );
        } while((buf = pNext));
    } else if (bzError == BZ_DATA_ERROR_MAGIC) {
        // not a BZIP2 file
        BZ2_bzReadClose(&bzError,b);
        fseek( pFile, 0, SEEK_END );
        nFileSize = ftell( pFile );
        if ( nFileSize == 0 )
        {
            printf( "Ioa_ReadLoadFileBz2(): The file is empty.\n" );
            return NULL;
        }
        pContents = ABC_ALLOC( char, nFileSize + 10 );
        rewind( pFile );
        RetValue = fread( pContents, nFileSize, 1, pFile );
    } else { 
        // Some other error.
        printf( "Ioa_ReadLoadFileBz2(): Unable to read the compressed BLIF.\n" );
        return NULL;
    }
    fclose( pFile );
    // finish off the file with the spare .end line
    // some benchmarks suddenly break off without this line
//    strcpy( pContents + nFileSize, "\n.end\n" );
    *pFileSize = nFileSize;
    return pContents;
}

/**Function*************************************************************

  Synopsis    [Reads the file into a character buffer.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static char * Ioa_ReadLoadFileGzAig( char * pFileName, int * pFileSize )
{
    const int READ_BLOCK_SIZE = 100000;
    gzFile pFile;
    char * pContents;
    int amtRead, readBlock, nFileSize = READ_BLOCK_SIZE;
    pFile = gzopen( pFileName, "rb" ); // if pFileName doesn't end in ".gz" then this acts as a passthrough to fopen
    pContents = ABC_ALLOC( char, nFileSize );        
    readBlock = 0;
    while ((amtRead = gzread(pFile, pContents + readBlock * READ_BLOCK_SIZE, READ_BLOCK_SIZE)) == READ_BLOCK_SIZE) {
        //printf("%d: read %d bytes\n", readBlock, amtRead);
        nFileSize += READ_BLOCK_SIZE;
        pContents = ABC_REALLOC(char, pContents, nFileSize);
        ++readBlock;
    }
    //printf("%d: read %d bytes\n", readBlock, amtRead);
    assert( amtRead != -1 ); // indicates a zlib error
    nFileSize -= (READ_BLOCK_SIZE - amtRead);
    gzclose(pFile);
    *pFileSize = nFileSize;
    return pContents;
}


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
    Vec_Int_t * vLits = NULL;
    Abc_Obj_t * pObj, * pNode0, * pNode1;
    Abc_Ntk_t * pNtkNew;
    int nTotal, nInputs, nOutputs, nLatches, nAnds;
    int nBad = 0, nConstr = 0, nJust = 0, nFair = 0;
    int nFileSize = -1, iTerm, nDigits, i;
    char * pContents, * pDrivers = NULL, * pSymbols, * pCur, * pName, * pType;
    unsigned uLit0, uLit1, uLit;
    int RetValue;

    // read the file into the buffer
    if ( !strncmp(pFileName+strlen(pFileName)-4,".bz2",4) )
        pContents = Ioa_ReadLoadFileBz2Aig( pFileName, &nFileSize );
    else if ( !strncmp(pFileName+strlen(pFileName)-3,".gz",3) )
        pContents = Ioa_ReadLoadFileGzAig( pFileName, &nFileSize );
    else
    {
//        pContents = Ioa_ReadLoadFile( pFileName );
        nFileSize = Extra_FileSize( pFileName );
        pFile = fopen( pFileName, "rb" );
        pContents = ABC_ALLOC( char, nFileSize );
        RetValue = fread( pContents, nFileSize, 1, pFile );
        fclose( pFile );
    }


    // check if the input file format is correct
    if ( strncmp(pContents, "aig", 3) != 0 || (pContents[3] != ' ' && pContents[3] != '2') )
    {
        fprintf( stdout, "Wrong input file format.\n" );
        ABC_FREE( pContents );
        return NULL;
    }

    // read the parameters (M I L O A + B C J F)
    pCur = pContents;         while ( *pCur != ' ' ) pCur++; pCur++;
    // read the number of objects
    nTotal = atoi( pCur );    while ( *pCur != ' ' ) pCur++; pCur++;
    // read the number of inputs
    nInputs = atoi( pCur );   while ( *pCur != ' ' ) pCur++; pCur++;
    // read the number of latches
    nLatches = atoi( pCur );  while ( *pCur != ' ' ) pCur++; pCur++;
    // read the number of outputs
    nOutputs = atoi( pCur );  while ( *pCur != ' ' ) pCur++; pCur++;
    // read the number of nodes
    nAnds = atoi( pCur );     while ( *pCur != ' ' && *pCur != '\n' ) pCur++; 
    if ( *pCur == ' ' )
    {
//        assert( nOutputs == 0 );
        // read the number of properties
        pCur++;
        nBad = atoi( pCur );     while ( *pCur != ' ' && *pCur != '\n' ) pCur++; 
        nOutputs += nBad;
    }
    if ( *pCur == ' ' )
    {
        // read the number of properties
        pCur++;
        nConstr = atoi( pCur );     while ( *pCur != ' ' && *pCur != '\n' ) pCur++; 
        nOutputs += nConstr;
    }
    if ( *pCur == ' ' )
    {
        // read the number of properties
        pCur++;
        nJust = atoi( pCur );     while ( *pCur != ' ' && *pCur != '\n' ) pCur++; 
        nOutputs += nJust;
    }
    if ( *pCur == ' ' )
    {
        // read the number of properties
        pCur++;
        nFair = atoi( pCur );     while ( *pCur != ' ' && *pCur != '\n' ) pCur++; 
        nOutputs += nFair;
    }
    if ( *pCur != '\n' )
    {
        fprintf( stdout, "The parameter line is in a wrong format.\n" );
        ABC_FREE( pContents );
        return NULL;
    }
    pCur++;

    // check the parameters
    if ( nTotal != nInputs + nLatches + nAnds )
    {
        fprintf( stdout, "The number of objects does not match.\n" );
        ABC_FREE( pContents );
        return NULL;
    }
    if ( nJust || nFair )
    {
        fprintf( stdout, "Reading AIGER files with liveness properties is currently not supported.\n" );
        ABC_FREE( pContents );
        return NULL;
    }

    if ( nConstr )
    {
        if ( nConstr == 1 )
            fprintf( stdout, "Warning: The last output is interpreted as a constraint.\n" );
        else
            fprintf( stdout, "Warning: The last %d outputs are interpreted as constraints.\n", nConstr );
    }

    // allocate the empty AIG
    pNtkNew = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );
    pName = Extra_FileNameGeneric( pFileName );
    pNtkNew->pName = Extra_UtilStrsav( pName );
    pNtkNew->pSpec = Extra_UtilStrsav( pFileName );
    ABC_FREE( pName );
    pNtkNew->nConstrs = nConstr;

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
    nDigits = Abc_Base10Log( nLatches );
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
//        Abc_ObjAssignName( pObj, Abc_ObjNameDummy("_L", i, nDigits), NULL );
//        printf( "Creating latch %s with input %d and output %d.\n", Abc_ObjName(pObj), pNode0->Id, pNode1->Id );
    } 
    

    if ( pContents[3] == ' ' ) // standard AIGER
    {
        // remember the beginning of latch/PO literals
        pDrivers = pCur;
        // scroll to the beginning of the binary data
        for ( i = 0; i < nLatches + nOutputs; )
            if ( *pCur++ == '\n' )
                i++;
    }
    else // modified AIGER
    { 
        vLits = Io_WriteDecodeLiterals( &pCur, nLatches + nOutputs );
    }

    // create the AND gates
    pProgress = Extra_ProgressBarStart( stdout, nAnds );
    for ( i = 0; i < nAnds; i++ )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        uLit = ((i + 1 + nInputs + nLatches) << 1);
        uLit1 = uLit  - Io_ReadAigerDecode( &pCur );
        uLit0 = uLit1 - Io_ReadAigerDecode( &pCur );
//        assert( uLit1 > uLit0 );
        pNode0 = Abc_ObjNotCond( (Abc_Obj_t *)Vec_PtrEntry(vNodes, uLit0 >> 1), uLit0 & 1 );
        pNode1 = Abc_ObjNotCond( (Abc_Obj_t *)Vec_PtrEntry(vNodes, uLit1 >> 1), uLit1 & 1 );
        assert( Vec_PtrSize(vNodes) == i + 1 + nInputs + nLatches );
        Vec_PtrPush( vNodes, Abc_AigAnd((Abc_Aig_t *)pNtkNew->pManFunc, pNode0, pNode1) );
    }
    Extra_ProgressBarStop( pProgress );

    // remember the place where symbols begin
    pSymbols = pCur;

    // read the latch driver literals
    pCur = pDrivers;
    if ( pContents[3] == ' ' ) // standard AIGER
    {
        Abc_NtkForEachLatchInput( pNtkNew, pObj, i )
        {
            uLit0 = atoi( pCur );  while ( *pCur != ' ' && *pCur != '\n' ) pCur++; 
            if ( *pCur == ' ' ) // read initial value
            {
                int Init;
                pCur++;
                Init = atoi( pCur );
                if ( Init == 0 )
                    Abc_LatchSetInit0( Abc_NtkBox(pNtkNew, i) );
                else if ( Init == 1 )
                    Abc_LatchSetInit1( Abc_NtkBox(pNtkNew, i) );
                else 
                {
                    assert( Init == Abc_Var2Lit(1+Abc_NtkPiNum(pNtkNew)+i, 0) ); 
                    // unitialized value of the latch is the latch literal according to http://fmv.jku.at/hwmcc11/beyond1.pdf
                    Abc_LatchSetInitDc( Abc_NtkBox(pNtkNew, i) );
                }
                while ( *pCur != ' ' && *pCur != '\n' ) pCur++;
            }
            if ( *pCur != '\n' )
            {
                fprintf( stdout, "The initial value of latch number %d is not recongnized.\n", i );
                return NULL;
            }
            pCur++;

            pNode0 = Abc_ObjNotCond( (Abc_Obj_t *)Vec_PtrEntry(vNodes, uLit0 >> 1), (uLit0 & 1) );//^ (uLit0 < 2) );
            Abc_ObjAddFanin( pObj, pNode0 );
        }
        // read the PO driver literals
        Abc_NtkForEachPo( pNtkNew, pObj, i )
        {
            uLit0 = atoi( pCur );  while ( *pCur++ != '\n' );
            pNode0 = Abc_ObjNotCond( (Abc_Obj_t *)Vec_PtrEntry(vNodes, uLit0 >> 1), (uLit0 & 1) );//^ (uLit0 < 2) );
            Abc_ObjAddFanin( pObj, pNode0 );
        }
    }
    else
    {
        // read the latch driver literals
        Abc_NtkForEachLatchInput( pNtkNew, pObj, i )
        {
            uLit0 = Vec_IntEntry( vLits, i );
            pNode0 = Abc_ObjNotCond( (Abc_Obj_t *)Vec_PtrEntry(vNodes, uLit0 >> 1), (uLit0 & 1) );
            Abc_ObjAddFanin( pObj, pNode0 );
        }
        // read the PO driver literals
        Abc_NtkForEachPo( pNtkNew, pObj, i )
        {
            uLit0 = Vec_IntEntry( vLits, i+Abc_NtkLatchNum(pNtkNew) );
            pNode0 = Abc_ObjNotCond( (Abc_Obj_t *)Vec_PtrEntry(vNodes, uLit0 >> 1), (uLit0 & 1) );
            Abc_ObjAddFanin( pObj, pNode0 );
        }
        Vec_IntFree( vLits );
    }
 
    // read the names if present
    pCur = pSymbols;
    if ( pCur < pContents + nFileSize && *pCur != 'c' )
    {
        int Counter = 0;
        while ( pCur < pContents + nFileSize && *pCur != 'c' )
        {
            // get the terminal type
            pType = pCur;
            if ( *pCur == 'i' )
                vTerms = pNtkNew->vPis;
            else if ( *pCur == 'l' )
                vTerms = pNtkNew->vBoxes;
            else if ( *pCur == 'o' || *pCur == 'b' || *pCur == 'c' || *pCur == 'j' || *pCur == 'f' )
                vTerms = pNtkNew->vPos;
            else
            {
//                fprintf( stdout, "Wrong terminal type.\n" );
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
            pObj = (Abc_Obj_t *)Vec_PtrEntry( vTerms, iTerm );
            if ( *pType == 'l' )
                pObj = Abc_ObjFanout0(pObj);
            // assign the name
            pName = pCur;          while ( *pCur++ != '\n' );
            // assign this name 
            *(pCur-1) = 0;
            Abc_ObjAssignName( pObj, pName, NULL );
            if ( *pType == 'l' )
            {
                Abc_ObjAssignName( Abc_ObjFanin0(pObj), Abc_ObjName(pObj), "L" );
                Abc_ObjAssignName( Abc_ObjFanin0(Abc_ObjFanin0(pObj)), Abc_ObjName(pObj), "_in" );
            }
            // mark the node as named
            pObj->pCopy = (Abc_Obj_t *)Abc_ObjName(pObj);
        } 

        // assign the remaining names
        Abc_NtkForEachPi( pNtkNew, pObj, i )
        {
            if ( pObj->pCopy ) continue;
            Abc_ObjAssignName( pObj, Abc_ObjName(pObj), NULL );
            Counter++;
        }
        Abc_NtkForEachLatchOutput( pNtkNew, pObj, i )
        {
            if ( pObj->pCopy ) continue;
            Abc_ObjAssignName( pObj, Abc_ObjName(pObj), NULL );
            Abc_ObjAssignName( Abc_ObjFanin0(pObj), Abc_ObjName(pObj), "L" );
            Abc_ObjAssignName( Abc_ObjFanin0(Abc_ObjFanin0(pObj)), Abc_ObjName(pObj), "_in" );
            Counter++;
        }
        Abc_NtkForEachPo( pNtkNew, pObj, i )
        {
            if ( pObj->pCopy ) continue;
            Abc_ObjAssignName( pObj, Abc_ObjName(pObj), NULL );
            Counter++;
        }
//        if ( Counter )
//            printf( "Io_ReadAiger(): Added %d default names for nameless I/O/register objects.\n", Counter );
    }
    else
    {
//        printf( "Io_ReadAiger(): I/O/register names are not given. Generating short names.\n" );
        Abc_NtkShortNames( pNtkNew );
    }

    // read the name of the model if given
    pCur = pSymbols;
    if ( pCur + 1 < pContents + nFileSize && *pCur == 'c' )
    {
        pCur++;
        if ( *pCur == 'n' )
        {
            pCur++;
            // read model name
            if ( strlen(pCur) > 0 )
            {
                ABC_FREE( pNtkNew->pName );
                pNtkNew->pName = Extra_UtilStrsav( pCur );
            }
        }
    }

    // skipping the comments
    ABC_FREE( pContents );
    Vec_PtrFree( vNodes );

    // remove the extra nodes
    Abc_AigCleanup( (Abc_Aig_t *)pNtkNew->pManFunc );

    // update polarity of the additional outputs
    if ( nBad || nConstr || nJust || nFair )
        Abc_NtkInvertConstraints( pNtkNew );

    // check the result
    if ( fCheck && !Abc_NtkCheckRead( pNtkNew ) )
    {
        printf( "Io_ReadAiger: The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

