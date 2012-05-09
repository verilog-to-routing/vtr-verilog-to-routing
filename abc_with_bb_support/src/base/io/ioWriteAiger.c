/**CFile****************************************************************

  FileName    [ioWriteAiger.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedures to write binary AIGER format developed by
  Armin Biere, Johannes Kepler University (http://fmv.jku.at/)]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - December 16, 2006.]

  Revision    [$Id: ioWriteAiger.c,v 1.00 2006/12/16 00:00:00 alanmi Exp $]

***********************************************************************/

#include "io.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

/*
    The following is taken from the AIGER format description, 
    which can be found at http://fmv.jku.at/aiger
*/


/*
         The AIGER And-Inverter Graph (AIG) Format Version 20061129
         ----------------------------------------------------------
              Armin Biere, Johannes Kepler University, 2006

  This report describes the AIG file format as used by the AIGER library.
  The purpose of this report is not only to motivate and document the
  format, but also to allow independent implementations of writers and
  readers by giving precise and unambiguous definitions.

  ...

Introduction

  The name AIGER contains as one part the acronym AIG of And-Inverter
  Graphs and also if pronounced in German sounds like the name of the
  'Eiger', a mountain in the Swiss alps.  This choice should emphasize the
  origin of this format. It was first openly discussed at the Alpine
  Verification Meeting 2006 in Ascona as a way to provide a simple, compact
  file format for a model checking competition affiliated to CAV 2007.

  ...

Binary Format Definition

  The binary format is semantically a subset of the ASCII format with a
  slightly different syntax.  The binary format may need to reencode
  literals, but translating a file in binary format into ASCII format and
  then back in to binary format will result in the same file.

  The main differences of the binary format to the ASCII format are as
  follows.  After the header the list of input literals and all the
  current state literals of a latch can be omitted.  Furthermore the
  definitions of the AND gates are binary encoded.  However, the symbol
  table and the comment section are as in the ASCII format.

  The header of an AIGER file in binary format has 'aig' as format
  identifier, but otherwise is identical to the ASCII header.  The standard
  file extension for the binary format is therefore '.aig'. 
  
  A header for the binary format is still in ASCII encoding:

    aig M I L O A

  Constants, variables and literals are handled in the same way as in the
  ASCII format.  The first simplifying restriction is on the variable
  indices of inputs and latches.  The variable indices of inputs come first,
  followed by the pseudo-primary inputs of the latches and then the variable
  indices of all LHS of AND gates:

    input variable indices        1,          2,  ... ,  I
    latch variable indices      I+1,        I+2,  ... ,  (I+L)
    AND variable indices      I+L+1,      I+L+2,  ... ,  (I+L+A) == M

  The corresponding unsigned literals are

    input literals                2,          4,  ... ,  2*I
    latch literals            2*I+2,      2*I+4,  ... ,  2*(I+L)
    AND literals          2*(I+L)+2,  2*(I+L)+4,  ... ,  2*(I+L+A) == 2*M
                    
  All literals have to be defined, and therefore 'M = I + L + A'.  With this
  restriction it becomes possible that the inputs and the current state
  literals of the latches do not have to be listed explicitly.  Therefore,
  after the header only the list of 'L' next state literals follows, one per
  latch on a single line, and then the 'O' outputs, again one per line.

  In the binary format we assume that the AND gates are ordered and respect
  the child parent relation.  AND gates with smaller literals on the LHS
  come first.  Therefore we can assume that the literals on the right-hand
  side of a definition of an AND gate are smaller than the LHS literal.
  Furthermore we can sort the literals on the RHS, such that the larger
  literal comes first.  A definition thus consists of three literals
    
      lhs rhs0 rhs1

  with 'lhs' even and 'lhs > rhs0 >= rhs1'.  Also the variable indices are
  pairwise different to avoid combinational self loops.  Since the LHS
  indices of the definitions are all consecutive (as even integers),
  the binary format does not have to keep 'lhs'.  In addition, we can use
  the order restriction and only write the differences 'delta0' and 'delta1'
  instead of 'rhs0' and 'rhs1', with

      delta0 = lhs - rhs0,  delta1 = rhs0 - rhs1
  
  The differences will all be strictly positive, and in practice often very
  small.  We can take advantage of this fact by the simple little-endian
  encoding of unsigned integers of the next section.  After the binary delta
  encoding of the RHSs of all AND gates, the optional symbol table and
  optional comment section start in the same format as in the ASCII case.

  ...

*/

static unsigned Io_ObjMakeLit( int Var, int fCompl )                 { return (Var << 1) | fCompl;    }
static unsigned Io_ObjAigerNum( Abc_Obj_t * pObj )                   { return (unsigned)pObj->pCopy;  }
static void     Io_ObjSetAigerNum( Abc_Obj_t * pObj, unsigned Num )  { pObj->pCopy = (void *)Num;     }

int      Io_WriteAigerEncode( char * pBuffer, int Pos, unsigned x );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Writes the AIG in the binary AIGER format.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteAiger( Abc_Ntk_t * pNtk, char * pFileName, int fWriteSymbols )
{
    ProgressBar * pProgress;
    FILE * pFile;
    Abc_Obj_t * pObj, * pDriver;
    int i, nNodes, Pos, nBufferSize;
    unsigned char * pBuffer;
    unsigned uLit0, uLit1, uLit;

    assert( Abc_NtkIsStrash(pNtk) );
    // start the output stream
    pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        fprintf( stdout, "Io_WriteAiger(): Cannot open the output file \"%s\".\n", pFileName );
        return;
    }

    // set the node numbers to be used in the output file
    nNodes = 0;
    Io_ObjSetAigerNum( Abc_AigConst1(pNtk), nNodes++ );
    Abc_NtkForEachCi( pNtk, pObj, i )
        Io_ObjSetAigerNum( pObj, nNodes++ );
    Abc_AigForEachAnd( pNtk, pObj, i )
        Io_ObjSetAigerNum( pObj, nNodes++ );

    // write the header "M I L O A" where M = I + L + A
    fprintf( pFile, "aig %u %u %u %u %u\n", 
        Abc_NtkPiNum(pNtk) + Abc_NtkLatchNum(pNtk) + Abc_NtkNodeNum(pNtk), 
        Abc_NtkPiNum(pNtk),
        Abc_NtkLatchNum(pNtk),
        Abc_NtkPoNum(pNtk),
        Abc_NtkNodeNum(pNtk) );

    // if the driver node is a constant, we need to complement the literal below
    // because, in the AIGER format, literal 0/1 is represented as number 0/1
    // while, in ABC, constant 1 node has number 0 and so literal 0/1 will be 1/0

    // write latch drivers
    Abc_NtkForEachLatchInput( pNtk, pObj, i )
    {
        pDriver = Abc_ObjFanin0(pObj);
        fprintf( pFile, "%u\n", Io_ObjMakeLit( Io_ObjAigerNum(pDriver), Abc_ObjFaninC0(pObj) ^ (Io_ObjAigerNum(pDriver) == 0) ) );
    }

    // write PO drivers
    Abc_NtkForEachPo( pNtk, pObj, i )
    {
        pDriver = Abc_ObjFanin0(pObj);
        fprintf( pFile, "%u\n", Io_ObjMakeLit( Io_ObjAigerNum(pDriver), Abc_ObjFaninC0(pObj) ^ (Io_ObjAigerNum(pDriver) == 0) ) );
    }

    // write the nodes into the buffer
    Pos = 0;
    nBufferSize = 6 * Abc_NtkNodeNum(pNtk) + 100; // skeptically assuming 3 chars per one AIG edge
    pBuffer = ALLOC( char, nBufferSize );
    pProgress = Extra_ProgressBarStart( stdout, Abc_NtkObjNumMax(pNtk) );
    Abc_AigForEachAnd( pNtk, pObj, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        uLit  = Io_ObjMakeLit( Io_ObjAigerNum(pObj), 0 );
        uLit0 = Io_ObjMakeLit( Io_ObjAigerNum(Abc_ObjFanin0(pObj)), Abc_ObjFaninC0(pObj) );
        uLit1 = Io_ObjMakeLit( Io_ObjAigerNum(Abc_ObjFanin1(pObj)), Abc_ObjFaninC1(pObj) );
        assert( uLit0 < uLit1 );
        Pos = Io_WriteAigerEncode( pBuffer, Pos, uLit  - uLit1 );
        Pos = Io_WriteAigerEncode( pBuffer, Pos, uLit1 - uLit0 );
        if ( Pos > nBufferSize - 10 )
        {
            printf( "Io_WriteAiger(): AIGER generation has failed because the allocated buffer is too small.\n" );
	        fclose( pFile );
            return;
        }
    }
    assert( Pos < nBufferSize );
    Extra_ProgressBarStop( pProgress );

    // write the buffer
    fwrite( pBuffer, 1, Pos, pFile );
    free( pBuffer );

    // write the symbol table
    if ( fWriteSymbols )
    {
        // write PIs
        Abc_NtkForEachPi( pNtk, pObj, i )
            fprintf( pFile, "i%d %s\n", i, Abc_ObjName(pObj) );
        // write latches
        Abc_NtkForEachLatch( pNtk, pObj, i )
            fprintf( pFile, "l%d %s\n", i, Abc_ObjName(Abc_ObjFanout0(pObj)) );
        // write POs
        Abc_NtkForEachPo( pNtk, pObj, i )
            fprintf( pFile, "o%d %s\n", i, Abc_ObjName(pObj) );
    }

    // write the comment
    fprintf( pFile, "c\n" );
    fprintf( pFile, "%s\n", pNtk->pName );
    fprintf( pFile, "This file in the AIGER format was written by ABC on %s\n", Extra_TimeStamp() );
    fprintf( pFile, "For information about the format, refer to %s\n", "http://fmv.jku.at/aiger" );
	fclose( pFile );
}

/**Function*************************************************************

  Synopsis    [Adds one unsigned AIG edge to the output buffer.]

  Description [This procedure is a slightly modified version of Armin Biere's
  procedure "void encode (FILE * file, unsigned x)" ]
  
  SideEffects [Returns the current writing position.]

  SeeAlso     []

***********************************************************************/
int Io_WriteAigerEncode( char * pBuffer, int Pos, unsigned x )
{
    unsigned char ch;
    while (x & ~0x7f)
    {
        ch = (x & 0x7f) | 0x80;
//        putc (ch, file);
        pBuffer[Pos++] = ch;
        x >>= 7;
    }
    ch = x;
//    putc (ch, file);
    pBuffer[Pos++] = ch;
    return Pos;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


