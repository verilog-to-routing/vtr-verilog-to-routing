/**CFile****************************************************************

  FileName    [giaAiger.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Procedures to read/write binary AIGER format developed by
  Armin Biere, Johannes Kepler University (http://fmv.jku.at/)]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaAiger.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/tim/tim.h"
#include "base/main/main.h"

ABC_NAMESPACE_IMPL_START

#define XAIG_VERBOSE 0

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
void Gia_FileFixName( char * pFileName )
{
    char * pName;
    for ( pName = pFileName; *pName; pName++ )
        if ( *pName == '>' )
            *pName = '\\';
}
char * Gia_FileNameGeneric( char * FileName )
{
    char * pDot, * pRes;
    pRes = Abc_UtilStrsav( FileName );
    if ( (pDot = strrchr( pRes, '.' )) )
        *pDot = 0;
    return pRes;
}
int Gia_FileSize( char * pFileName )
{
    FILE * pFile;
    int nFileSize;
    pFile = fopen( pFileName, "r" );
    if ( pFile == NULL )
    {
        printf( "Gia_FileSize(): The file is unavailable (absent or open).\n" );
        return 0;
    }
    fseek( pFile, 0, SEEK_END );  
    nFileSize = ftell( pFile ); 
    fclose( pFile );
    return nFileSize;
}
void Gia_FileWriteBufferSize( FILE * pFile, int nSize )
{
    unsigned char Buffer[5];
    Gia_AigerWriteInt( Buffer, nSize );
    fwrite( Buffer, 1, 4, pFile );
}

/**Function*************************************************************

  Synopsis    [Create the array of literals to be written.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_AigerCollectLiterals( Gia_Man_t * p )
{
    Vec_Int_t * vLits;
    Gia_Obj_t * pObj;
    int i;
    vLits = Vec_IntAlloc( Gia_ManPoNum(p) );
    Gia_ManForEachRi( p, pObj, i )
        Vec_IntPush( vLits, Gia_ObjFaninLit0p(p, pObj) );
    Gia_ManForEachPo( p, pObj, i )
        Vec_IntPush( vLits, Gia_ObjFaninLit0p(p, pObj) );
    return vLits;
}
Vec_Int_t * Gia_AigerReadLiterals( unsigned char ** ppPos, int nEntries )
{
    Vec_Int_t * vLits;
    int Lit, LitPrev, Diff, i;
    vLits = Vec_IntAlloc( nEntries );
    LitPrev = Gia_AigerReadUnsigned( ppPos );
    Vec_IntPush( vLits, LitPrev );
    for ( i = 1; i < nEntries; i++ )
    {
//        Diff = Lit - LitPrev;
//        Diff = (Lit < LitPrev)? -Diff : Diff;
//        Diff = ((2 * Diff) << 1) | (int)(Lit < LitPrev);
        Diff = Gia_AigerReadUnsigned( ppPos );
        Diff = (Diff & 1)? -(Diff >> 1) : Diff >> 1;
        Lit  = Diff + LitPrev;
        Vec_IntPush( vLits, Lit );
        LitPrev = Lit;
    }
    return vLits;
}
Vec_Str_t * Gia_AigerWriteLiterals( Vec_Int_t * vLits )
{
    Vec_Str_t * vBinary;
    int Pos = 0, Lit, LitPrev, Diff, i;
    vBinary = Vec_StrAlloc( 2 * Vec_IntSize(vLits) );
    LitPrev = Vec_IntEntry( vLits, 0 );
    Pos = Gia_AigerWriteUnsignedBuffer( (unsigned char *)Vec_StrArray(vBinary), Pos, LitPrev ); 
    Vec_IntForEachEntryStart( vLits, Lit, i, 1 )
    {
        Diff = Lit - LitPrev;
        Diff = (Lit < LitPrev)? -Diff : Diff;
        Diff = (Diff << 1) | (int)(Lit < LitPrev);
        Pos = Gia_AigerWriteUnsignedBuffer( (unsigned char *)Vec_StrArray(vBinary), Pos, Diff );
        LitPrev = Lit;
        if ( Pos + 10 > vBinary->nCap )
            Vec_StrGrow( vBinary, vBinary->nCap+1 );
    }
    vBinary->nSize = Pos;
/*
    // verify
    {
        extern Vec_Int_t * Gia_AigerReadLiterals( char ** ppPos, int nEntries );
        char * pPos = Vec_StrArray( vBinary );
        Vec_Int_t * vTemp = Gia_AigerReadLiterals( &pPos, Vec_IntSize(vLits) );
        for ( i = 0; i < Vec_IntSize(vLits); i++ )
        {
            int Entry1 = Vec_IntEntry(vLits,i);
            int Entry2 = Vec_IntEntry(vTemp,i);
            assert( Entry1 == Entry2 );
        }
        Vec_IntFree( vTemp );
    }
*/
    return vBinary;
}

/**Function*************************************************************

  Synopsis    [Reads the AIG in the binary AIGER format.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_AigerReadFromMemory( char * pContents, int nFileSize, int fGiaSimple, int fSkipStrash, int fCheck )
{
    Gia_Man_t * pNew, * pTemp;
    Vec_Int_t * vLits = NULL, * vPoTypes = NULL;
    Vec_Int_t * vNodes, * vDrivers, * vInits = NULL;
    int iObj, iNode0, iNode1, fHieOnly = 0;
    int nTotal, nInputs, nOutputs, nLatches, nAnds, i;
    int nBad = 0, nConstr = 0, nJust = 0, nFair = 0;
    unsigned char * pDrivers, * pSymbols, * pCur;
    unsigned uLit0, uLit1, uLit;

    // read the parameters (M I L O A + B C J F)
    pCur = (unsigned char *)pContents;         while ( *pCur != ' ' ) pCur++; pCur++;
    // read the number of objects
    nTotal = atoi( (const char *)pCur );    while ( *pCur != ' ' ) pCur++; pCur++;
    // read the number of inputs
    nInputs = atoi( (const char *)pCur );   while ( *pCur != ' ' ) pCur++; pCur++;
    // read the number of latches
    nLatches = atoi( (const char *)pCur );  while ( *pCur != ' ' ) pCur++; pCur++;
    // read the number of outputs
    nOutputs = atoi( (const char *)pCur );  while ( *pCur != ' ' ) pCur++; pCur++;
    // read the number of nodes
    nAnds = atoi( (const char *)pCur );     while ( *pCur != ' ' && *pCur != '\n' ) pCur++; 
    if ( *pCur == ' ' )
    {
//        assert( nOutputs == 0 );
        // read the number of properties
        pCur++;
        nBad = atoi( (const char *)pCur );     while ( *pCur != ' ' && *pCur != '\n' ) pCur++; 
        nOutputs += nBad;
    }
    if ( *pCur == ' ' )
    {
        // read the number of properties
        pCur++;
        nConstr = atoi( (const char *)pCur );     while ( *pCur != ' ' && *pCur != '\n' ) pCur++; 
        nOutputs += nConstr;
    }
    if ( *pCur == ' ' )
    {
        // read the number of properties
        pCur++;
        nJust = atoi( (const char *)pCur );     while ( *pCur != ' ' && *pCur != '\n' ) pCur++; 
        nOutputs += nJust;
    }
    if ( *pCur == ' ' )
    {
        // read the number of properties
        pCur++;
        nFair = atoi( (const char *)pCur );     while ( *pCur != ' ' && *pCur != '\n' ) pCur++; 
        nOutputs += nFair;
    }
    if ( *pCur != '\n' )
    {
        fprintf( stdout, "The parameter line is in a wrong format.\n" );
        return NULL;
    }
    pCur++;

    // check the parameters
    if ( nTotal != nInputs + nLatches + nAnds )
    {
        fprintf( stdout, "The number of objects does not match.\n" );
        return NULL;
    }
    if ( nJust || nFair )
    {
        fprintf( stdout, "Reading AIGER files with liveness properties is currently not supported.\n" );
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
    pNew = Gia_ManStart( nTotal + nLatches + nOutputs + 1 );
    pNew->nConstrs = nConstr;
    pNew->fGiaSimple = fGiaSimple;

    // prepare the array of nodes
    vNodes = Vec_IntAlloc( 1 + nTotal );
    Vec_IntPush( vNodes, 0 );

    // create the PIs
    for ( i = 0; i < nInputs + nLatches; i++ )
    {
        iObj = Gia_ManAppendCi(pNew);    
        Vec_IntPush( vNodes, iObj );
    }

    // remember the beginning of latch/PO literals
    pDrivers = pCur;
    if ( pContents[3] == ' ' ) // standard AIGER
    {
        // scroll to the beginning of the binary data
        for ( i = 0; i < nLatches + nOutputs; )
            if ( *pCur++ == '\n' )
                i++;
    }
    else // modified AIGER
    {
        vLits = Gia_AigerReadLiterals( &pCur, nLatches + nOutputs );
    }

    // create the AND gates
    if ( !fGiaSimple && !fSkipStrash )
        Gia_ManHashAlloc( pNew );
    for ( i = 0; i < nAnds; i++ )
    {
        uLit = ((i + 1 + nInputs + nLatches) << 1);
        uLit1 = uLit  - Gia_AigerReadUnsigned( &pCur );
        uLit0 = uLit1 - Gia_AigerReadUnsigned( &pCur );
//        assert( uLit1 > uLit0 );
        iNode0 = Abc_LitNotCond( Vec_IntEntry(vNodes, uLit0 >> 1), uLit0 & 1 );
        iNode1 = Abc_LitNotCond( Vec_IntEntry(vNodes, uLit1 >> 1), uLit1 & 1 );
        assert( Vec_IntSize(vNodes) == i + 1 + nInputs + nLatches );
        if ( !fGiaSimple && fSkipStrash )
        {
            if ( iNode0 == iNode1 )
                Vec_IntPush( vNodes, Gia_ManAppendBuf(pNew, iNode0) );
            else
                Vec_IntPush( vNodes, Gia_ManAppendAnd(pNew, iNode0, iNode1) );
        }
        else
            Vec_IntPush( vNodes, Gia_ManHashAnd(pNew, iNode0, iNode1) );
    }
    if ( !fGiaSimple && !fSkipStrash )
        Gia_ManHashStop( pNew );

    // remember the place where symbols begin
    pSymbols = pCur;

    // read the latch driver literals
    vDrivers = Vec_IntAlloc( nLatches + nOutputs );
    if ( pContents[3] == ' ' ) // standard AIGER
    {
        vInits = Vec_IntAlloc( nLatches );
        pCur = pDrivers;
        for ( i = 0; i < nLatches; i++ )
        {
            uLit0 = atoi( (char *)pCur );   
            while ( *pCur != ' ' && *pCur != '\n' ) 
                pCur++;
            if ( *pCur == ' ' )
            {
                pCur++;
                Vec_IntPush( vInits, atoi( (char *)pCur ) );
                while ( *pCur++ != '\n' );
            }
            else
            {
                pCur++;
                Vec_IntPush( vInits, 0 );
            }
            iNode0 = Abc_LitNotCond( Vec_IntEntry(vNodes, uLit0 >> 1), (uLit0 & 1) );
            Vec_IntPush( vDrivers, iNode0 );
        }
        // read the PO driver literals
        for ( i = 0; i < nOutputs; i++ )
        {
            uLit0 = atoi( (char *)pCur );   while ( *pCur++ != '\n' );
            iNode0 = Abc_LitNotCond( Vec_IntEntry(vNodes, uLit0 >> 1), (uLit0 & 1) );
            Vec_IntPush( vDrivers, iNode0 );
        }

    }
    else
    {
        // read the latch driver literals
        for ( i = 0; i < nLatches; i++ )
        {
            uLit0 = Vec_IntEntry( vLits, i );
            iNode0 = Abc_LitNotCond( Vec_IntEntry(vNodes, uLit0 >> 1), (uLit0 & 1) );
            Vec_IntPush( vDrivers, iNode0 );
        }
        // read the PO driver literals
        for ( i = 0; i < nOutputs; i++ )
        {
            uLit0 = Vec_IntEntry( vLits, i+nLatches );
            iNode0 = Abc_LitNotCond( Vec_IntEntry(vNodes, uLit0 >> 1), (uLit0 & 1) );
            Vec_IntPush( vDrivers, iNode0 );
        }
        Vec_IntFree( vLits );
    }

    // create the POs
    for ( i = 0; i < nOutputs; i++ )
        Gia_ManAppendCo( pNew, Vec_IntEntry(vDrivers, nLatches + i) );
    for ( i = 0; i < nLatches; i++ )
        Gia_ManAppendCo( pNew, Vec_IntEntry(vDrivers, i) );
    Vec_IntFree( vDrivers );

    // create the latches
    Gia_ManSetRegNum( pNew, nLatches );

    // read signal names if they are of the special type
    pCur = pSymbols;
    if ( pCur < (unsigned char *)pContents + nFileSize && *pCur != 'c' )
    {
        int fBreakUsed = 0;
        unsigned char * pCurOld = pCur;
        pNew->vUserPiIds = Vec_IntStartFull( nInputs );
        pNew->vUserPoIds = Vec_IntStartFull( nOutputs );
        pNew->vUserFfIds = Vec_IntStartFull( nLatches );
        while ( pCur < (unsigned char *)pContents + nFileSize && *pCur != 'c' )
        {
            int iTerm;
            char * pType = (char *)pCur;
            // check terminal type
            if ( *pCur != 'i' && *pCur != 'o' && *pCur != 'l'  )
            {
//                fprintf( stdout, "Wrong terminal type.\n" );
                fBreakUsed = 1;
                break;
            }
            // get terminal number
            iTerm = atoi( (char *)++pCur );  while ( *pCur++ != ' ' );
            // skip spaces
            while ( *pCur == ' ' )
                pCur++;
            // decode the user numbers:
            // flops are named: @l<num>
            // PIs are named: @i<num>
            // POs are named: @o<num>
            if ( *pCur++ != '@' )
            {
                fBreakUsed = 1;
                break;
            }
            if ( *pCur == 'i' && *pType == 'i' )
                Vec_IntWriteEntry( pNew->vUserPiIds, iTerm, atoi((char *)pCur+1) );
            else if ( *pCur == 'o' && *pType == 'o' )
                Vec_IntWriteEntry( pNew->vUserPoIds, iTerm, atoi((char *)pCur+1) );
            else if ( *pCur == 'l' && *pType == 'l' )
                Vec_IntWriteEntry( pNew->vUserFfIds, iTerm, atoi((char *)pCur+1) );
            else
            {
                fprintf( stdout, "Wrong name format.\n" );
                fBreakUsed = 1;
                break;
            }
            // skip digits
            while ( *pCur++ != '\n' );
        }
        // in case of abnormal termination, remove the arrays
        if ( fBreakUsed )
        {
            unsigned char * pName;
            int Entry, nInvars, nConstr, iTerm;

            Vec_Int_t * vPoNames = Vec_IntStartFull( nOutputs );

            Vec_IntFreeP( &pNew->vUserPiIds );
            Vec_IntFreeP( &pNew->vUserPoIds );
            Vec_IntFreeP( &pNew->vUserFfIds );

            // try to figure out signal names
            fBreakUsed = 0;
            pCur = (unsigned char *)pCurOld;
            while ( pCur < (unsigned char *)pContents + nFileSize && *pCur != 'c' )
            {
                // get the terminal type
                if ( *pCur == 'i' || *pCur == 'l' )
                {
                    // skip till the end of the line
                    while ( *pCur++ != '\n' );
                    *(pCur-1) = 0;
                    continue;
                }
                if ( *pCur != 'o' )
                {
//                    fprintf( stdout, "Wrong terminal type.\n" );
                    fBreakUsed = 1;
                    break;
                }
                // get the terminal number
                iTerm = atoi( (char *)++pCur );  while ( *pCur++ != ' ' );
                // get the node
                if ( iTerm < 0 || iTerm >= nOutputs )
                {
                    fprintf( stdout, "The output number (%d) is out of range.\n", iTerm );
                    fBreakUsed = 1;
                    break;
                }
                if ( Vec_IntEntry(vPoNames, iTerm) != ~0 )
                {
                    fprintf( stdout, "The output number (%d) is listed twice.\n", iTerm );
                    fBreakUsed = 1;
                    break;
                }

                // get the name
                pName = pCur;          while ( *pCur++ != '\n' );
                *(pCur-1) = 0;
                // assign the name
                Vec_IntWriteEntry( vPoNames, iTerm, pName - (unsigned char *)pContents );
            } 

            // check that all names are assigned
            if ( !fBreakUsed )
            {
                nInvars = nConstr = 0;
                vPoTypes = Vec_IntStart( Gia_ManPoNum(pNew) );
                Vec_IntForEachEntry( vPoNames, Entry, i )
                {
                    if ( Entry == ~0 )
                        continue;
                    if ( strncmp( pContents+Entry, "constraint:", 11 ) == 0 )
                    {
                        Vec_IntWriteEntry( vPoTypes, i, 1 );
                        nConstr++;
                    }
                    if ( strncmp( pContents+Entry, "invariant:", 10 ) == 0 )
                    {
                        Vec_IntWriteEntry( vPoTypes, i, 2 );
                        nInvars++;
                    }
                }
                if ( nConstr )
                    fprintf( stdout, "Recognized and added %d constraints.\n", nConstr );
                if ( nInvars )
                    fprintf( stdout, "Recognized and skipped %d invariants.\n", nInvars );
                if ( nConstr == 0 && nInvars == 0 )
                    Vec_IntFreeP( &vPoTypes );
            }
            Vec_IntFree( vPoNames );
        }
    }


    // check if there are other types of information to read
    if ( pCur + 1 < (unsigned char *)pContents + nFileSize && *pCur == 'c' )
    {
        int fVerbose = XAIG_VERBOSE;
        Vec_Str_t * vStr;
        unsigned char * pCurTemp;
        pCur++;
        // skip new line if present
//        if ( *pCur == '\n' )
//            pCur++;
        while ( pCur < (unsigned char *)pContents + nFileSize )
        {
            // read extra AIG
            if ( *pCur == 'a' )
            {
                pCur++;
                vStr = Vec_StrStart( Gia_AigerReadInt(pCur) );             pCur += 4;
                memcpy( Vec_StrArray(vStr), pCur, Vec_StrSize(vStr) );
                pCur += Vec_StrSize(vStr);
                pNew->pAigExtra = Gia_AigerReadFromMemory( Vec_StrArray(vStr), Vec_StrSize(vStr), 0, 0, 0 );
                Vec_StrFree( vStr );
                if ( fVerbose ) printf( "Finished reading extension \"a\".\n" );
            }
            // read number of constraints
            else if ( *pCur == 'c' )
            {
                pCur++;
                assert( Gia_AigerReadInt(pCur) == 4 );                     pCur += 4;
                pNew->nConstrs = Gia_AigerReadInt( pCur );                 pCur += 4;
                if ( fVerbose ) printf( "Finished reading extension \"c\".\n" );
            }
            // read delay information
            else if ( *pCur == 'd' )
            {
                pCur++;
                assert( Gia_AigerReadInt(pCur) == 4 );                     pCur += 4;
                pNew->nAnd2Delay = Gia_AigerReadInt(pCur);                 pCur += 4;
                if ( fVerbose ) printf( "Finished reading extension \"d\".\n" );
            }
            else if ( *pCur == 'i' )
            {
                pCur++;
                nInputs = Gia_AigerReadInt(pCur)/4;                        pCur += 4;
                pNew->vInArrs  = Vec_FltStart( nInputs );
                memcpy( Vec_FltArray(pNew->vInArrs),  pCur, 4*nInputs );   pCur += 4*nInputs;
                if ( fVerbose ) printf( "Finished reading extension \"i\".\n" );
            }
            else if ( *pCur == 'o' )
            {
                pCur++;
                nOutputs = Gia_AigerReadInt(pCur)/4;                       pCur += 4;
                pNew->vOutReqs  = Vec_FltStart( nOutputs );
                memcpy( Vec_FltArray(pNew->vOutReqs),  pCur, 4*nOutputs ); pCur += 4*nOutputs;
                if ( fVerbose ) printf( "Finished reading extension \"o\".\n" );
            }
            // read equivalence classes
            else if ( *pCur == 'e' )
            {
                extern Gia_Rpr_t * Gia_AigerReadEquivClasses( unsigned char ** ppPos, int nSize );
                pCur++;
                pCurTemp = pCur + Gia_AigerReadInt(pCur) + 4;              pCur += 4;
                pNew->pReprs = Gia_AigerReadEquivClasses( &pCur, Gia_ManObjNum(pNew) );
                pNew->pNexts = Gia_ManDeriveNexts( pNew );
                assert( pCur == pCurTemp );
                if ( fVerbose ) printf( "Finished reading extension \"e\".\n" );
            }
            // read flop classes
            else if ( *pCur == 'f' )
            {
                pCur++;
                assert( Gia_AigerReadInt(pCur) == 4*Gia_ManRegNum(pNew) );   pCur += 4;
                pNew->vFlopClasses  = Vec_IntStart( Gia_ManRegNum(pNew) );
                memcpy( Vec_IntArray(pNew->vFlopClasses),  pCur, 4*Gia_ManRegNum(pNew) );   pCur += 4*Gia_ManRegNum(pNew);
                if ( fVerbose ) printf( "Finished reading extension \"f\".\n" );
            }
            // read gate classes
            else if ( *pCur == 'g' )
            {
                pCur++;
                assert( Gia_AigerReadInt(pCur) == 4*Gia_ManObjNum(pNew) );   pCur += 4;
                pNew->vGateClasses  = Vec_IntStart( Gia_ManObjNum(pNew) );
                memcpy( Vec_IntArray(pNew->vGateClasses),  pCur, 4*Gia_ManObjNum(pNew) );   pCur += 4*Gia_ManObjNum(pNew);
                if ( fVerbose ) printf( "Finished reading extension \"g\".\n" );
            }
            // read hierarchy information
            else if ( *pCur == 'h' )
            {
                pCur++;
                vStr = Vec_StrStart( Gia_AigerReadInt(pCur) );          pCur += 4;
                memcpy( Vec_StrArray(vStr), pCur, Vec_StrSize(vStr) );
                pCur += Vec_StrSize(vStr);
                pNew->pManTime = Tim_ManLoad( vStr, 1 );
                Vec_StrFree( vStr );
                fHieOnly = 1;
                if ( fVerbose ) printf( "Finished reading extension \"h\".\n" );
            }
            // read packing
            else if ( *pCur == 'k' )
            {
                extern Vec_Int_t * Gia_AigerReadPacking( unsigned char ** ppPos, int nSize );
                int nSize;
                pCur++;
                nSize = Gia_AigerReadInt(pCur);
                pCurTemp = pCur + nSize + 4;                            pCur += 4;
                pNew->vPacking = Gia_AigerReadPacking( &pCur, nSize ); 
                assert( pCur == pCurTemp );
                if ( fVerbose ) printf( "Finished reading extension \"k\".\n" );
            }
            // read mapping
            else if ( *pCur == 'm' )
            {
                extern int * Gia_AigerReadMapping( unsigned char ** ppPos, int nSize );
                extern int * Gia_AigerReadMappingSimple( unsigned char ** ppPos, int nSize );
                extern Vec_Int_t * Gia_AigerReadMappingDoc( unsigned char ** ppPos, int nObjs );
                int nSize;
                pCur++;
                nSize = Gia_AigerReadInt(pCur);
                pCurTemp = pCur + nSize + 4;           pCur += 4;
                pNew->vMapping = Gia_AigerReadMappingDoc( &pCur, Gia_ManObjNum(pNew) );
                assert( pCur == pCurTemp );
                if ( fVerbose ) printf( "Finished reading extension \"m\".\n" );
            }
            // read model name
            else if ( *pCur == 'n' )
            {
                pCur++;
                if ( (*pCur >= 'a' && *pCur <= 'z') || (*pCur >= 'A' && *pCur <= 'Z') || (*pCur >= '0' && *pCur <= '9') )
                {
                    pNew->pName = Abc_UtilStrsav( (char *)pCur );       pCur += strlen(pNew->pName) + 1;
                }
                else
                {
                    pCurTemp = pCur + Gia_AigerReadInt(pCur) + 4;       pCur += 4;
                    ABC_FREE( pNew->pName );
                    pNew->pName = Abc_UtilStrsav( (char *)pCur );       pCur += strlen(pNew->pName) + 1;
                    assert( pCur == pCurTemp );
                }
            }
            // read placement
            else if ( *pCur == 'p' )
            {
                Gia_Plc_t * pPlacement;
                pCur++;
                pCurTemp = pCur + Gia_AigerReadInt(pCur) + 4;           pCur += 4;
                pPlacement = ABC_ALLOC( Gia_Plc_t, Gia_ManObjNum(pNew) );
                memcpy( pPlacement, pCur, 4*Gia_ManObjNum(pNew) );      pCur += 4*Gia_ManObjNum(pNew);
                assert( pCur == pCurTemp );
                pNew->pPlacement = pPlacement;
                if ( fVerbose ) printf( "Finished reading extension \"p\".\n" );
            }
            // read register classes
            else if ( *pCur == 'r' )
            {
                int i, nRegs;
                pCur++;
                pCurTemp = pCur + Gia_AigerReadInt(pCur) + 4;           pCur += 4;
                nRegs = Gia_AigerReadInt(pCur);                         pCur += 4;
                //nRegs = (pCurTemp - pCur)/4;
                pNew->vRegClasses = Vec_IntAlloc( nRegs );
                for ( i = 0; i < nRegs; i++ )
                    Vec_IntPush( pNew->vRegClasses, Gia_AigerReadInt(pCur) ), pCur += 4;
                assert( pCur == pCurTemp );
                if ( fVerbose ) printf( "Finished reading extension \"r\".\n" );
            }
            // read register inits
            else if ( *pCur == 's' )
            {
                int i, nRegs;
                pCur++;
                pCurTemp = pCur + Gia_AigerReadInt(pCur) + 4;           pCur += 4;
                nRegs = Gia_AigerReadInt(pCur);                         pCur += 4;
                pNew->vRegInits = Vec_IntAlloc( nRegs );
                for ( i = 0; i < nRegs; i++ )
                    Vec_IntPush( pNew->vRegInits, Gia_AigerReadInt(pCur) ), pCur += 4;
                assert( pCur == pCurTemp );
                if ( fVerbose ) printf( "Finished reading extension \"s\".\n" );
            }
            // read configuration data
            else if ( *pCur == 'b' )
            {
                int nSize;
                pCur++;
                nSize = Gia_AigerReadInt(pCur);
                pCurTemp = pCur + nSize + 4;                            pCur += 4;
                pNew->pCellStr = Abc_UtilStrsav( (char*)pCur );         pCur += strlen((char*)pCur) + 1;
                nSize = nSize - strlen(pNew->pCellStr) - 1;
                assert( nSize % 4 == 0 );
                pNew->vConfigs = Vec_IntAlloc(nSize / 4);
//                memcpy(Vec_IntArray(pNew->vConfigs), pCur, nSize);      pCur += nSize;
                for ( i = 0; i < nSize / 4; i++ )
                    Vec_IntPush( pNew->vConfigs, Gia_AigerReadInt(pCur) ), pCur += 4;
                assert( pCur == pCurTemp );
                if ( fVerbose ) printf( "Finished reading extension \"b\".\n" );
            }
            // read choices
            else if ( *pCur == 'q' )
            {
                int i, nPairs, iRepr, iNode;
                assert( !Gia_ManHasChoices(pNew) );
                pNew->pSibls = ABC_CALLOC( int, Gia_ManObjNum(pNew) );
                pCur++;
                pCurTemp = pCur + Gia_AigerReadInt(pCur) + 4;           pCur += 4;
                nPairs = Gia_AigerReadInt(pCur);                        pCur += 4;
                for ( i = 0; i < nPairs; i++ )
                {
                    iRepr = Gia_AigerReadInt(pCur);                     pCur += 4;
                    iNode = Gia_AigerReadInt(pCur);                     pCur += 4;
                    pNew->pSibls[iRepr] = iNode;
                    assert( iRepr > iNode );
                }
                assert( pCur == pCurTemp );
                if ( fVerbose ) printf( "Finished reading extension \"q\".\n" );
            }
            // read switching activity
            else if ( *pCur == 'u' )
            { 
                unsigned char * pSwitching;
                pCur++;
                pCurTemp = pCur + Gia_AigerReadInt(pCur) + 4;           pCur += 4;
                pSwitching = ABC_ALLOC( unsigned char, Gia_ManObjNum(pNew) );
                memcpy( pSwitching, pCur, Gia_ManObjNum(pNew) );        pCur += Gia_ManObjNum(pNew);
                assert( pCur == pCurTemp );
                if ( fVerbose ) printf( "Finished reading extension \"s\".\n" );
            }
            // read timing manager
            else if ( *pCur == 't' )
            {
                pCur++;
                vStr = Vec_StrStart( Gia_AigerReadInt(pCur) );          pCur += 4;
                memcpy( Vec_StrArray(vStr), pCur, Vec_StrSize(vStr) );  pCur += Vec_StrSize(vStr);
                pNew->pManTime = Tim_ManLoad( vStr, 0 );
                Vec_StrFree( vStr );
                if ( fVerbose ) printf( "Finished reading extension \"t\".\n" );
            }
            // read object classes
            else if ( *pCur == 'v' )
            {
                pCur++;
                pNew->vObjClasses = Vec_IntStart( Gia_AigerReadInt(pCur)/4 ); pCur += 4;
                memcpy( Vec_IntArray(pNew->vObjClasses), pCur, 4*Vec_IntSize(pNew->vObjClasses) );
                pCur += 4*Vec_IntSize(pNew->vObjClasses);
                if ( fVerbose ) printf( "Finished reading extension \"v\".\n" );
            }
            // read edge information
            else if ( *pCur == 'w' )
            {
                Vec_Int_t * vPairs;
                int i, nPairs;
                pCur++;
                pCurTemp = pCur + Gia_AigerReadInt(pCur) + 4;           pCur += 4;
                nPairs = Gia_AigerReadInt(pCur);                        pCur += 4;
                vPairs = Vec_IntAlloc( 2*nPairs );
                for ( i = 0; i < 2*nPairs; i++ )
                    Vec_IntPush( vPairs, Gia_AigerReadInt(pCur) ),      pCur += 4;
                assert( pCur == pCurTemp );
                if ( fSkipStrash )
                {
                    Gia_ManEdgeFromArray( pNew, vPairs );
                    if ( fVerbose ) printf( "Finished reading extension \"w\".\n" );
                }
                else
                    printf( "Cannot read extension \"w\" because AIG is rehashed. Use \"&r -s <file.aig>\".\n" );
                Vec_IntFree( vPairs );
            }
            else break;
        }
    }

    // skipping the comments
    Vec_IntFree( vNodes );

    // update polarity of the additional outputs
    if ( nBad || nConstr || nJust || nFair )
        Gia_ManInvertConstraints( pNew );

    // clean the PO drivers
    if ( vPoTypes )
    {
        pNew = Gia_ManDupWithConstraints( pTemp = pNew, vPoTypes );
        Gia_ManStop( pTemp );
        Vec_IntFreeP( &vPoTypes );
    }

    if ( !fGiaSimple && !fSkipStrash && Gia_ManHasDangling(pNew) )
    {
        Tim_Man_t * pManTime;
        Vec_Int_t * vFlopMap, * vGateMap, * vObjMap;
        vFlopMap = pNew->vFlopClasses; pNew->vFlopClasses = NULL;
        vGateMap = pNew->vGateClasses; pNew->vGateClasses = NULL;
        vObjMap  = pNew->vObjClasses;  pNew->vObjClasses  = NULL;
        pManTime = (Tim_Man_t *)pNew->pManTime; pNew->pManTime     = NULL;
        pNew = Gia_ManCleanup( pTemp = pNew );
        if ( (vGateMap || vObjMap) && (Gia_ManObjNum(pNew) < Gia_ManObjNum(pTemp)) )
            printf( "Cleanup removed objects after reading. Old gate/object abstraction maps are invalid!\n" );
        Gia_ManStop( pTemp );
        pNew->vFlopClasses = vFlopMap;
        pNew->vGateClasses = vGateMap;
        pNew->vObjClasses  = vObjMap;
        pNew->pManTime     = pManTime;
    }

    if ( fHieOnly )
    {
//        Tim_ManPrint( (Tim_Man_t *)pNew->pManTime );
        if ( Abc_FrameReadLibBox() == NULL )
            printf( "Warning: Creating unit-delay box delay tables because box library is not available.\n" );
        Tim_ManCreate( (Tim_Man_t *)pNew->pManTime, Abc_FrameReadLibBox(), pNew->vInArrs, pNew->vOutReqs );
    }
    Vec_FltFreeP( &pNew->vInArrs );
    Vec_FltFreeP( &pNew->vOutReqs );
/*
    // check the result
    if ( fCheck && !Gia_ManCheck( pNew ) )
    {
        printf( "Gia_AigerRead: The network check has failed.\n" );
        Gia_ManStop( pNew );
        return NULL;
    }
*/

    if ( vInits && Vec_IntSum(vInits) )
    {
        char * pInit = ABC_ALLOC( char, Vec_IntSize(vInits) + 1 );
        Gia_Obj_t * pObj;
        int i;
        assert( Vec_IntSize(vInits) == Gia_ManRegNum(pNew) );
        Gia_ManForEachRo( pNew, pObj, i )
        {
            if ( Vec_IntEntry(vInits, i) == 0 )
                pInit[i] = '0';
            else if ( Vec_IntEntry(vInits, i) == 1 )
                pInit[i] = '1';
            else 
            {
                assert( Vec_IntEntry(vInits, i) == Abc_Var2Lit(Gia_ObjId(pNew, pObj), 0) );
                // unitialized value of the latch is the latch literal according to http://fmv.jku.at/hwmcc11/beyond1.pdf
                pInit[i] = 'X';
            }
        }
        pInit[i] = 0;
        pNew = Gia_ManDupZeroUndc( pTemp = pNew, pInit, fGiaSimple, 1 );
        pNew->nConstrs = pTemp->nConstrs; pTemp->nConstrs = 0;
        Gia_ManStop( pTemp );
        ABC_FREE( pInit );
    }
    Vec_IntFreeP( &vInits );
    if ( !fGiaSimple && !fSkipStrash && pNew->vMapping )
    {
        Abc_Print( 0, "Structural hashing enabled while reading AIGER invalidated the mapping.  Consider using \"&r -s\".\n" );
        Vec_IntFreeP( &pNew->vMapping );
    }
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Reads the AIG in the binary AIGER format.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_AigerRead( char * pFileName, int fGiaSimple, int fSkipStrash, int fCheck )
{
    FILE * pFile;
    Gia_Man_t * pNew;
    char * pName, * pContents;
    int nFileSize;
    int RetValue;

    // read the file into the buffer
    Gia_FileFixName( pFileName );
    nFileSize = Gia_FileSize( pFileName );
    pFile = fopen( pFileName, "rb" );
    pContents = ABC_ALLOC( char, nFileSize );
    RetValue = fread( pContents, nFileSize, 1, pFile );
    fclose( pFile );

    pNew = Gia_AigerReadFromMemory( pContents, nFileSize, fGiaSimple, fSkipStrash, fCheck );
    ABC_FREE( pContents );
    if ( pNew )
    {
        ABC_FREE( pNew->pName );
        pName = Gia_FileNameGeneric( pFileName );
        pNew->pName = Abc_UtilStrsav( pName );
        ABC_FREE( pName );

        assert( pNew->pSpec == NULL );
        pNew->pSpec = Abc_UtilStrsav( pFileName );
    }
    return pNew;
}



/**Function*************************************************************

  Synopsis    [Writes the AIG in into the memory buffer.]

  Description [The resulting buffer constains the AIG in AIGER format. 
  The resulting buffer should be deallocated by the user.]
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Str_t * Gia_AigerWriteIntoMemoryStr( Gia_Man_t * p )
{
    Vec_Str_t * vBuffer;
    Gia_Obj_t * pObj;
    int nNodes = 0, i, uLit, uLit0, uLit1; 
    // set the node numbers to be used in the output file
    Gia_ManConst0(p)->Value = nNodes++;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = nNodes++;
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = nNodes++;

    // write the header "M I L O A" where M = I + L + A
    vBuffer = Vec_StrAlloc( 3*Gia_ManObjNum(p) );
    Vec_StrPrintStr( vBuffer, "aig " );
    Vec_StrPrintNum( vBuffer, Gia_ManCandNum(p) );
    Vec_StrPrintStr( vBuffer, " " );
    Vec_StrPrintNum( vBuffer, Gia_ManPiNum(p) );
    Vec_StrPrintStr( vBuffer, " " );
    Vec_StrPrintNum( vBuffer, Gia_ManRegNum(p) );
    Vec_StrPrintStr( vBuffer, " " );
    Vec_StrPrintNum( vBuffer, Gia_ManPoNum(p) );
    Vec_StrPrintStr( vBuffer, " " );
    Vec_StrPrintNum( vBuffer, Gia_ManAndNum(p) );
    Vec_StrPrintStr( vBuffer, "\n" );

    // write latch drivers
    Gia_ManForEachRi( p, pObj, i )
    {
        uLit = Abc_Var2Lit( Gia_ObjValue(Gia_ObjFanin0(pObj)), Gia_ObjFaninC0(pObj) );
        Vec_StrPrintNum( vBuffer, uLit );
        Vec_StrPrintStr( vBuffer, "\n" );
    }

    // write PO drivers
    Gia_ManForEachPo( p, pObj, i )
    {
        uLit = Abc_Var2Lit( Gia_ObjValue(Gia_ObjFanin0(pObj)), Gia_ObjFaninC0(pObj) );
        Vec_StrPrintNum( vBuffer, uLit );
        Vec_StrPrintStr( vBuffer, "\n" );
    }
    // write the nodes into the buffer
    Gia_ManForEachAnd( p, pObj, i )
    {
        uLit  = Abc_Var2Lit( Gia_ObjValue(pObj), 0 );
        uLit0 = Abc_Var2Lit( Gia_ObjValue(Gia_ObjFanin0(pObj)), Gia_ObjFaninC0(pObj) );
        uLit1 = Abc_Var2Lit( Gia_ObjValue(Gia_ObjFanin1(pObj)), Gia_ObjFaninC1(pObj) );
        assert( uLit0 != uLit1 );
        if ( uLit0 > uLit1 )
        {
            int Temp = uLit0;
            uLit0 = uLit1;
            uLit1 = Temp;
        }
        Gia_AigerWriteUnsigned( vBuffer, uLit  - uLit1 );
        Gia_AigerWriteUnsigned( vBuffer, uLit1 - uLit0 );
    }
    Vec_StrPrintStr( vBuffer, "c" );
    return vBuffer;
}

/**Function*************************************************************

  Synopsis    [Writes the AIG in into the memory buffer.]

  Description [The resulting buffer constains the AIG in AIGER format.
  The CI/CO/AND nodes are assumed to be ordered according to some rule.
  The resulting buffer should be deallocated by the user.]
  
  SideEffects [Note that in vCos, PIs are order first, followed by latches!]

  SeeAlso     []

***********************************************************************/
Vec_Str_t * Gia_AigerWriteIntoMemoryStrPart( Gia_Man_t * p, Vec_Int_t * vCis, Vec_Int_t * vAnds, Vec_Int_t * vCos, int nRegs )
{
    Vec_Str_t * vBuffer;
    Gia_Obj_t * pObj;
    int nNodes = 0, i, uLit, uLit0, uLit1; 
    // set the node numbers to be used in the output file
    Gia_ManConst0(p)->Value = nNodes++;
    Gia_ManForEachObjVec( vCis, p, pObj, i )
    {
        assert( Gia_ObjIsCi(pObj) );
        pObj->Value = nNodes++;
    }
    Gia_ManForEachObjVec( vAnds, p, pObj, i )
    {
        assert( Gia_ObjIsAnd(pObj) );
        pObj->Value = nNodes++;
    }

    // write the header "M I L O A" where M = I + L + A
    vBuffer = Vec_StrAlloc( 3*Gia_ManObjNum(p) );
    Vec_StrPrintStr( vBuffer, "aig " );
    Vec_StrPrintNum( vBuffer, Vec_IntSize(vCis) + Vec_IntSize(vAnds) );
    Vec_StrPrintStr( vBuffer, " " );
    Vec_StrPrintNum( vBuffer, Vec_IntSize(vCis) - nRegs );
    Vec_StrPrintStr( vBuffer, " " );
    Vec_StrPrintNum( vBuffer, nRegs );
    Vec_StrPrintStr( vBuffer, " " );
    Vec_StrPrintNum( vBuffer, Vec_IntSize(vCos) - nRegs );
    Vec_StrPrintStr( vBuffer, " " );
    Vec_StrPrintNum( vBuffer, Vec_IntSize(vAnds) );
    Vec_StrPrintStr( vBuffer, "\n" );

    // write latch drivers
    Gia_ManForEachObjVec( vCos, p, pObj, i )
    {
        assert( Gia_ObjIsCo(pObj) );
        if ( i < Vec_IntSize(vCos) - nRegs )
            continue;
        uLit = Abc_Var2Lit( Gia_ObjValue(Gia_ObjFanin0(pObj)), Gia_ObjFaninC0(pObj) );
        Vec_StrPrintNum( vBuffer, uLit );
        Vec_StrPrintStr( vBuffer, "\n" );
    }
    // write output drivers
    Gia_ManForEachObjVec( vCos, p, pObj, i )
    {
        assert( Gia_ObjIsCo(pObj) );
        if ( i >= Vec_IntSize(vCos) - nRegs )
            continue;
        uLit = Abc_Var2Lit( Gia_ObjValue(Gia_ObjFanin0(pObj)), Gia_ObjFaninC0(pObj) );
        Vec_StrPrintNum( vBuffer, uLit );
        Vec_StrPrintStr( vBuffer, "\n" );
    }

    // write the nodes into the buffer
    Gia_ManForEachObjVec( vAnds, p, pObj, i )
    {
        uLit  = Abc_Var2Lit( Gia_ObjValue(pObj), 0 );
        uLit0 = Abc_Var2Lit( Gia_ObjValue(Gia_ObjFanin0(pObj)), Gia_ObjFaninC0(pObj) );
        uLit1 = Abc_Var2Lit( Gia_ObjValue(Gia_ObjFanin1(pObj)), Gia_ObjFaninC1(pObj) );
        assert( uLit0 != uLit1 );
        if ( uLit0 > uLit1 )
        {
            int Temp = uLit0;
            uLit0 = uLit1;
            uLit1 = Temp;
        }
        Gia_AigerWriteUnsigned( vBuffer, uLit  - uLit1 );
        Gia_AigerWriteUnsigned( vBuffer, uLit1 - uLit0 );
    }
    Vec_StrPrintStr( vBuffer, "c" );
    return vBuffer;
}

/**Function*************************************************************

  Synopsis    [Writes the AIG in the binary AIGER format.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_AigerWrite( Gia_Man_t * pInit, char * pFileName, int fWriteSymbols, int fCompact )
{
    int fVerbose = XAIG_VERBOSE;
    FILE * pFile;
    Gia_Man_t * p;
    Gia_Obj_t * pObj;
    Vec_Str_t * vStrExt;
    int i, nBufferSize, Pos;
    unsigned char * pBuffer;
    unsigned uLit0, uLit1, uLit;
    assert( pInit->nXors == 0 && pInit->nMuxes == 0 );

    if ( Gia_ManCoNum(pInit) == 0 )
    {
        printf( "AIG cannot be written because it has no POs.\n" );
        return;
    }

    // start the output stream
    pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        fprintf( stdout, "Gia_AigerWrite(): Cannot open the output file \"%s\".\n", pFileName );
        return;
    }

    // create normalized AIG
    if ( !Gia_ManIsNormalized(pInit) )
    {
//        printf( "Gia_AigerWrite(): Normalizing AIG for writing.\n" );
        p = Gia_ManDupNormalize( pInit, 0 );
        Gia_ManTransferMapping( p, pInit );
        Gia_ManTransferPacking( p, pInit );
        Gia_ManTransferTiming( p, pInit );
        p->vNamesIn   = pInit->vNamesIn;   pInit->vNamesIn   = NULL;
        p->vNamesOut  = pInit->vNamesOut;  pInit->vNamesOut  = NULL;
        p->nConstrs   = pInit->nConstrs;   pInit->nConstrs   = 0;
    }
    else
        p = pInit;

    // write the header "M I L O A" where M = I + L + A
    fprintf( pFile, "aig%s %u %u %u %u %u", 
        fCompact? "2" : "",
        Gia_ManCiNum(p) + Gia_ManAndNum(p), 
        Gia_ManPiNum(p),
        Gia_ManRegNum(p),
        Gia_ManConstrNum(p) ? 0 : Gia_ManPoNum(p),
        Gia_ManAndNum(p) );
    // write the extended header "B C J F"
    if ( Gia_ManConstrNum(p) )
        fprintf( pFile, " %u %u", Gia_ManPoNum(p) - Gia_ManConstrNum(p), Gia_ManConstrNum(p) );
    fprintf( pFile, "\n" ); 

    Gia_ManInvertConstraints( p );
    if ( !fCompact ) 
    {
        // write latch drivers
        Gia_ManForEachRi( p, pObj, i )
            fprintf( pFile, "%u\n", Gia_ObjFaninLit0p(p, pObj) );
        // write PO drivers
        Gia_ManForEachPo( p, pObj, i )
            fprintf( pFile, "%u\n", Gia_ObjFaninLit0p(p, pObj) );
    }
    else
    {
        Vec_Int_t * vLits = Gia_AigerCollectLiterals( p );
        Vec_Str_t * vBinary = Gia_AigerWriteLiterals( vLits );
        fwrite( Vec_StrArray(vBinary), 1, Vec_StrSize(vBinary), pFile );
        Vec_StrFree( vBinary );
        Vec_IntFree( vLits );
    }
    Gia_ManInvertConstraints( p );

    // write the nodes into the buffer
    Pos = 0;
    nBufferSize = 8 * Gia_ManAndNum(p) + 100; // skeptically assuming 3 chars per one AIG edge
    pBuffer = ABC_ALLOC( unsigned char, nBufferSize );
    Gia_ManForEachAnd( p, pObj, i )
    {
        uLit  = Abc_Var2Lit( i, 0 );
        uLit0 = Gia_ObjFaninLit0( pObj, i );
        uLit1 = Gia_ObjFaninLit1( pObj, i );
        assert( p->fGiaSimple || Gia_ManBufNum(p) || uLit0 < uLit1 );
        Pos = Gia_AigerWriteUnsignedBuffer( pBuffer, Pos, uLit  - uLit1 );
        Pos = Gia_AigerWriteUnsignedBuffer( pBuffer, Pos, uLit1 - uLit0 );
        if ( Pos > nBufferSize - 10 )
        {
            printf( "Gia_AigerWrite(): AIGER generation has failed because the allocated buffer is too small.\n" );
            fclose( pFile );
            if ( p != pInit )
                Gia_ManStop( p );
            return;
        }
    }
    assert( Pos < nBufferSize );

    // write the buffer
    fwrite( pBuffer, 1, Pos, pFile );
    ABC_FREE( pBuffer );

    // write the symbol table
    if ( p->vNamesIn && p->vNamesOut )
    {
        assert( Vec_PtrSize(p->vNamesIn)  == Gia_ManCiNum(p) );
        assert( Vec_PtrSize(p->vNamesOut) == Gia_ManCoNum(p) );
        // write PIs
        Gia_ManForEachPi( p, pObj, i )
            fprintf( pFile, "i%d %s\n", i, (char *)Vec_PtrEntry(p->vNamesIn, i) );
        // write latches
        Gia_ManForEachRo( p, pObj, i )
            fprintf( pFile, "l%d %s\n", i, (char *)Vec_PtrEntry(p->vNamesIn, Gia_ManPiNum(p) + i) );
        // write POs
        Gia_ManForEachPo( p, pObj, i )
            fprintf( pFile, "o%d %s\n", i, (char *)Vec_PtrEntry(p->vNamesOut, i) );
    }

    // write the comment
//    fprintf( pFile, "c\n" );
    fprintf( pFile, "c" );

    // write additional AIG
    if ( p->pAigExtra )
    {
        fprintf( pFile, "a" );
        vStrExt = Gia_AigerWriteIntoMemoryStr( p->pAigExtra );
        Gia_FileWriteBufferSize( pFile, Vec_StrSize(vStrExt) );
        fwrite( Vec_StrArray(vStrExt), 1, Vec_StrSize(vStrExt), pFile );
        Vec_StrFree( vStrExt );
        if ( fVerbose ) printf( "Finished writing extension \"a\".\n" );
    }
    // write constraints
    if ( p->nConstrs )
    {
        fprintf( pFile, "c" );
        Gia_FileWriteBufferSize( pFile, 4 );
        Gia_FileWriteBufferSize( pFile, p->nConstrs );
    }
    // write timing information
    if ( p->nAnd2Delay )
    {
        fprintf( pFile, "d" );
        Gia_FileWriteBufferSize( pFile, 4 );
        Gia_FileWriteBufferSize( pFile, p->nAnd2Delay );
    }
    if ( p->pManTime )
    {
        float * pTimes;
        pTimes = Tim_ManGetArrTimes( (Tim_Man_t *)p->pManTime );
        if ( pTimes )
        {
            fprintf( pFile, "i" );
            Gia_FileWriteBufferSize( pFile, 4*Tim_ManPiNum((Tim_Man_t *)p->pManTime) );
            fwrite( pTimes, 1, 4*Tim_ManPiNum((Tim_Man_t *)p->pManTime), pFile );
            ABC_FREE( pTimes );
            if ( fVerbose ) printf( "Finished writing extension \"i\".\n" );
        }
        pTimes = Tim_ManGetReqTimes( (Tim_Man_t *)p->pManTime );
        if ( pTimes )
        {
            fprintf( pFile, "o" );
            Gia_FileWriteBufferSize( pFile, 4*Tim_ManPoNum((Tim_Man_t *)p->pManTime) );
            fwrite( pTimes, 1, 4*Tim_ManPoNum((Tim_Man_t *)p->pManTime), pFile );
            ABC_FREE( pTimes );
            if ( fVerbose ) printf( "Finished writing extension \"o\".\n" );
        }
    }
    // write equivalences
    if ( p->pReprs && p->pNexts )
    {
        extern Vec_Str_t * Gia_WriteEquivClasses( Gia_Man_t * p );
        fprintf( pFile, "e" );
        vStrExt = Gia_WriteEquivClasses( p );
        Gia_FileWriteBufferSize( pFile, Vec_StrSize(vStrExt) );
        fwrite( Vec_StrArray(vStrExt), 1, Vec_StrSize(vStrExt), pFile );
        Vec_StrFree( vStrExt );
    }
    // write flop classes
    if ( p->vFlopClasses )
    {
        fprintf( pFile, "f" );
        Gia_FileWriteBufferSize( pFile, 4*Gia_ManRegNum(p) );
        assert( Vec_IntSize(p->vFlopClasses) == Gia_ManRegNum(p) );
        fwrite( Vec_IntArray(p->vFlopClasses), 1, 4*Gia_ManRegNum(p), pFile );
    }
    // write gate classes
    if ( p->vGateClasses )
    {
        fprintf( pFile, "g" );
        Gia_FileWriteBufferSize( pFile, 4*Gia_ManObjNum(p) );
        assert( Vec_IntSize(p->vGateClasses) == Gia_ManObjNum(p) );
        fwrite( Vec_IntArray(p->vGateClasses), 1, 4*Gia_ManObjNum(p), pFile );
    }
    // write hierarchy info
    if ( p->pManTime )
    {
        fprintf( pFile, "h" );
        vStrExt = Tim_ManSave( (Tim_Man_t *)p->pManTime, 1 );
        Gia_FileWriteBufferSize( pFile, Vec_StrSize(vStrExt) );
        fwrite( Vec_StrArray(vStrExt), 1, Vec_StrSize(vStrExt), pFile );
        Vec_StrFree( vStrExt );
        if ( fVerbose ) printf( "Finished writing extension \"h\".\n" );
    }
    // write packing
    if ( p->vPacking )
    {
        extern Vec_Str_t * Gia_WritePacking( Vec_Int_t * vPacking );
        fprintf( pFile, "k" );
        vStrExt = Gia_WritePacking( p->vPacking );
        Gia_FileWriteBufferSize( pFile, Vec_StrSize(vStrExt) );
        fwrite( Vec_StrArray(vStrExt), 1, Vec_StrSize(vStrExt), pFile );
        Vec_StrFree( vStrExt );
        if ( fVerbose ) printf( "Finished writing extension \"k\".\n" );
    }
    // write edges
    if ( p->vEdge1 )
    {
        Vec_Int_t * vPairs = Gia_ManEdgeToArray( p );
        int i;
        fprintf( pFile, "w" );
        Gia_FileWriteBufferSize( pFile, 4*(Vec_IntSize(vPairs)+1) );
        Gia_FileWriteBufferSize( pFile, Vec_IntSize(vPairs)/2 );
        for ( i = 0; i < Vec_IntSize(vPairs); i++ )
            Gia_FileWriteBufferSize( pFile, Vec_IntEntry(vPairs, i) );
        Vec_IntFree( vPairs );
    }
    // write mapping
    if ( Gia_ManHasMapping(p) )
    {
        extern Vec_Str_t * Gia_AigerWriteMapping( Gia_Man_t * p );
        extern Vec_Str_t * Gia_AigerWriteMappingSimple( Gia_Man_t * p );
        extern Vec_Str_t * Gia_AigerWriteMappingDoc( Gia_Man_t * p );
        fprintf( pFile, "m" );
        vStrExt = Gia_AigerWriteMappingDoc( p );
        Gia_FileWriteBufferSize( pFile, Vec_StrSize(vStrExt) );
        fwrite( Vec_StrArray(vStrExt), 1, Vec_StrSize(vStrExt), pFile );
        Vec_StrFree( vStrExt );
        if ( fVerbose ) printf( "Finished writing extension \"m\".\n" );
    }
    // write placement
    if ( p->pPlacement )
    {
        fprintf( pFile, "p" );
        Gia_FileWriteBufferSize( pFile, 4*Gia_ManObjNum(p) );
        fwrite( p->pPlacement, 1, 4*Gia_ManObjNum(p), pFile );
    }
    // write register classes
    if ( p->vRegClasses )
    {
        int i;
        fprintf( pFile, "r" );
        Gia_FileWriteBufferSize( pFile, 4*(Vec_IntSize(p->vRegClasses)+1) );
        Gia_FileWriteBufferSize( pFile, Vec_IntSize(p->vRegClasses) );
        for ( i = 0; i < Vec_IntSize(p->vRegClasses); i++ )
            Gia_FileWriteBufferSize( pFile, Vec_IntEntry(p->vRegClasses, i) );
    }
    // write register inits
    if ( p->vRegInits )
    {
        int i;
        fprintf( pFile, "s" );
        Gia_FileWriteBufferSize( pFile, 4*(Vec_IntSize(p->vRegInits)+1) );
        Gia_FileWriteBufferSize( pFile, Vec_IntSize(p->vRegInits) );
        for ( i = 0; i < Vec_IntSize(p->vRegInits); i++ )
            Gia_FileWriteBufferSize( pFile, Vec_IntEntry(p->vRegInits, i) );
    }
    // write configuration data
    if ( p->vConfigs )
    {
        fprintf( pFile, "b" );
        assert( p->pCellStr != NULL );
        Gia_FileWriteBufferSize( pFile, 4*Vec_IntSize(p->vConfigs) + strlen(p->pCellStr) + 1 );
        fwrite( p->pCellStr, 1, strlen(p->pCellStr) + 1, pFile );
//        fwrite( Vec_IntArray(p->vConfigs), 1, 4*Vec_IntSize(p->vConfigs), pFile );
        for ( i = 0; i < Vec_IntSize(p->vConfigs); i++ )
            Gia_FileWriteBufferSize( pFile, Vec_IntEntry(p->vConfigs, i) );
    }
    // write choices
    if ( Gia_ManHasChoices(p) )
    {
        int i, nPairs = 0;
        fprintf( pFile, "q" );
        for ( i = 0; i < Gia_ManObjNum(p); i++ )
            nPairs += (Gia_ObjSibl(p, i) > 0);
        Gia_FileWriteBufferSize( pFile, 4*(nPairs * 2 + 1) );
        Gia_FileWriteBufferSize( pFile, nPairs );
        for ( i = 0; i < Gia_ManObjNum(p); i++ )
            if ( Gia_ObjSibl(p, i) )
            {
                assert( i > Gia_ObjSibl(p, i) );
                Gia_FileWriteBufferSize( pFile, i );
                Gia_FileWriteBufferSize( pFile, Gia_ObjSibl(p, i) );
            }
        if ( fVerbose ) printf( "Finished writing extension \"q\".\n" );
    }
    // write switching activity
    if ( p->pSwitching )
    {
        fprintf( pFile, "u" );
        Gia_FileWriteBufferSize( pFile, Gia_ManObjNum(p) );
        fwrite( p->pSwitching, 1, Gia_ManObjNum(p), pFile );
    }
/*
    // write timing information
    if ( p->pManTime )
    {
        fprintf( pFile, "t" );
        vStrExt = Tim_ManSave( (Tim_Man_t *)p->pManTime, 0 );
        Gia_FileWriteBufferSize( pFile, Vec_StrSize(vStrExt) );
        fwrite( Vec_StrArray(vStrExt), 1, Vec_StrSize(vStrExt), pFile );
        Vec_StrFree( vStrExt );
    }
*/
    // write object classes
    if ( p->vObjClasses )
    {
        fprintf( pFile, "v" );
        Gia_FileWriteBufferSize( pFile, 4*Gia_ManObjNum(p) );
        assert( Vec_IntSize(p->vObjClasses) == Gia_ManObjNum(p) );
        fwrite( Vec_IntArray(p->vObjClasses), 1, 4*Gia_ManObjNum(p), pFile );
    }
    // write name
    if ( p->pName )
    {
        fprintf( pFile, "n" );
        Gia_FileWriteBufferSize( pFile, strlen(p->pName)+1 );
        fwrite( p->pName, 1, strlen(p->pName), pFile );
        fprintf( pFile, "%c", '\0' );
    }
    // write comments
    fprintf( pFile, "\nThis file was produced by the GIA package in ABC on %s\n", Gia_TimeStamp() );
    fprintf( pFile, "For information about AIGER format, refer to %s\n", "http://fmv.jku.at/aiger" );
    fclose( pFile );
    if ( p != pInit )
    {
        pInit->pManTime  = p->pManTime;  p->pManTime = NULL;
        pInit->vNamesIn  = p->vNamesIn;  p->vNamesIn = NULL;
        pInit->vNamesOut = p->vNamesOut; p->vNamesOut = NULL;
        Gia_ManStop( p );
    }
}

/**Function*************************************************************

  Synopsis    [Writes the AIG in the binary AIGER format.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_DumpAiger( Gia_Man_t * p, char * pFilePrefix, int iFileNum, int nFileNumDigits )
{
    char Buffer[100];
    sprintf( Buffer, "%s%0*d.aig", pFilePrefix, nFileNumDigits, iFileNum );
    Gia_AigerWrite( p, Buffer, 0, 0 );
}

/**Function*************************************************************

  Synopsis    [Writes the AIG in the binary AIGER format.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_AigerWriteSimple( Gia_Man_t * pInit, char * pFileName )
{
    FILE * pFile;
    Vec_Str_t * vStr;
    if ( Gia_ManPoNum(pInit) == 0 )
    {
        printf( "Gia_AigerWriteSimple(): AIG cannot be written because it has no POs.\n" );
        return;
    }
    // start the output stream
    pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        fprintf( stdout, "Gia_AigerWriteSimple(): Cannot open the output file \"%s\".\n", pFileName );
        return;
    }
    // write the buffer
    vStr = Gia_AigerWriteIntoMemoryStr( pInit );
    fwrite( Vec_StrArray(vStr), 1, Vec_StrSize(vStr), pFile );
    Vec_StrFree( vStr );
    fclose( pFile );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

