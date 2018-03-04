/**CFile****************************************************************

  FileName    [parseStack.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Stacks used by the formula parser.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 18, 2003.]

  Revision    [$Id: parseStack.c,v 1.0 2003/02/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "parseInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

struct ParseStackFnStruct
{
    void **     pData;        // the array of elements
    int           Top;          // the index
    int           Size;         // the stack size
};

struct ParseStackOpStruct
{
    int *         pData;        // the array of elements
    int           Top;          // the index
    int           Size;         // the stack size
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts the stack.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Parse_StackFn_t * Parse_StackFnStart( int nDepth )
{
    Parse_StackFn_t * p;
    p = ABC_ALLOC( Parse_StackFn_t, 1 );
    memset( p, 0, sizeof(Parse_StackFn_t) );
    p->pData = ABC_ALLOC( void *, nDepth );
    p->Size = nDepth;
    return p;
}

/**Function*************************************************************

  Synopsis    [Checks whether the stack is empty.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Parse_StackFnIsEmpty( Parse_StackFn_t * p )
{
    return (int)(p->Top == 0);
}

/**Function*************************************************************

  Synopsis    [Pushes an entry into the stack.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Parse_StackFnPush( Parse_StackFn_t * p, void * bFunc )
{
    if ( p->Top >= p->Size )
    {
        printf( "Parse_StackFnPush(): Stack size is too small!\n" );
        return;
    }
    p->pData[ p->Top++ ] = bFunc;
}

/**Function*************************************************************

  Synopsis    [Pops an entry out of the stack.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Parse_StackFnPop( Parse_StackFn_t * p )
{
    if ( p->Top == 0 )
    {
        printf( "Parse_StackFnPush(): Trying to extract data from the empty stack!\n" );
        return NULL;
    }
    return p->pData[ --p->Top ];
}

/**Function*************************************************************

  Synopsis    [Deletes the stack.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Parse_StackFnFree( Parse_StackFn_t * p )
{
    ABC_FREE( p->pData );
    ABC_FREE( p );
}




/**Function*************************************************************

  Synopsis    [Starts the stack.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Parse_StackOp_t * Parse_StackOpStart( int nDepth )
{
    Parse_StackOp_t * p;
    p = ABC_ALLOC( Parse_StackOp_t, 1 );
    memset( p, 0, sizeof(Parse_StackOp_t) );
    p->pData = ABC_ALLOC( int, nDepth );
    p->Size = nDepth;
    return p;
}

/**Function*************************************************************

  Synopsis    [Checks whether the stack is empty.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Parse_StackOpIsEmpty( Parse_StackOp_t * p )
{
    return (int)(p->Top == 0);
}

/**Function*************************************************************

  Synopsis    [Pushes an entry into the stack.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Parse_StackOpPush( Parse_StackOp_t * p, int Oper )
{
    if ( p->Top >= p->Size )
    {
        printf( "Parse_StackOpPush(): Stack size is too small!\n" );
        return;
    }
    p->pData[ p->Top++ ] = Oper;
}

/**Function*************************************************************

  Synopsis    [Pops an entry out of the stack.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Parse_StackOpPop( Parse_StackOp_t * p )
{
    if ( p->Top == 0 )
    {
        printf( "Parse_StackOpPush(): Trying to extract data from the empty stack!\n" );
        return -1;
    }
    return p->pData[ --p->Top ];
}

/**Function*************************************************************

  Synopsis    [Deletes the stack.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Parse_StackOpFree( Parse_StackOp_t * p )
{
    ABC_FREE( p->pData );
    ABC_FREE( p );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

