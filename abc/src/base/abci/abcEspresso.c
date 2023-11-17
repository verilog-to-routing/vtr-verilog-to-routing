/**CFile****************************************************************

  FileName    [abcEspresso.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Procedures to minimize SOPs using Espresso.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcEspresso.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "misc/espresso/espresso.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void        Abc_NodeEspresso( Abc_Obj_t * pNode );
static pset_family Abc_SopToEspresso( char * pSop );
static char *      Abc_SopFromEspresso( Extra_MmFlex_t * pMan, pset_family Cover );
static pset_family Abc_EspressoMinimize( pset_family pOnset, pset_family pDcset );
 
////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Minimizes SOP representations using Espresso.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkEspresso( Abc_Ntk_t * pNtk, int fVerbose )
{
    Abc_Obj_t * pNode;
    int i;
    assert( Abc_NtkIsLogic(pNtk) );
    // convert the network to have SOPs
    if ( Abc_NtkHasMapping(pNtk) )
        Abc_NtkMapToSop(pNtk);
    else if ( Abc_NtkHasBdd(pNtk) )
    {
        if ( !Abc_NtkBddToSop(pNtk, -1, ABC_INFINITY, 1) )
        {
            printf( "Abc_NtkEspresso(): Converting to SOPs has failed.\n" );
            return;
        }
    }
    // minimize SOPs of all nodes
    Abc_NtkForEachNode( pNtk, pNode, i )
        if ( i ) Abc_NodeEspresso( pNode );
}

/**Function*************************************************************

  Synopsis    [Minimizes SOP representation of one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeEspresso( Abc_Obj_t * pNode )
{
    extern void define_cube_size( int n );
    pset_family Cover;
    int fCompl;

    assert( Abc_ObjIsNode(pNode) );
    // define the cube for this node
    define_cube_size( Abc_ObjFaninNum(pNode) );
    // create the Espresso cover
    fCompl = Abc_SopIsComplement( pNode->pData );
    Cover = Abc_SopToEspresso( pNode->pData );
    // perform minimization
    Cover = Abc_EspressoMinimize( Cover, NULL ); // deletes also cover
    // convert back onto the node's SOP representation
    pNode->pData = Abc_SopFromEspresso( pNode->pNtk->pManFunc, Cover );
    if ( fCompl ) Abc_SopComplement( pNode->pData );
    sf_free(Cover);
}

/**Function*************************************************************

  Synopsis    [Converts SOP in ABC into SOP representation in Espresso.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
pset_family Abc_SopToEspresso( char * pSop )
{
    char *      pCube;
    pset_family Cover;
    pset        set;
    int         nCubes, nVars, Value, v;
    
    if ( pSop == NULL ) 
        return NULL;
    
    nVars  = Abc_SopGetVarNum(pSop);
    nCubes = Abc_SopGetCubeNum(pSop);
    assert( cube.size == 2 * nVars );
    
    if ( Abc_SopIsConst0(pSop) ) 
    {
        Cover = sf_new(0, cube.size);
        return Cover;
    }
    if ( Abc_SopIsConst1(pSop) ) 
    {
        Cover = sf_new(1, cube.size);
        set = GETSET(Cover, Cover->count++);
        set_copy( set, cube.fullset );
        return Cover;
    }

    // create the cover
    Cover = sf_new(nCubes, cube.size);
    // fill in the cubes
    Abc_SopForEachCube( pSop, nVars, pCube )
    {
        set = GETSET(Cover, Cover->count++);
        set_copy( set, cube.fullset );
        Abc_CubeForEachVar( pCube, Value, v )
        {
            if ( Value == '0' )
                set_remove(set, 2*v+1);
            else if ( Value == '1' )
                set_remove(set, 2*v);
        }
    }
    return Cover;
}

/**Function*************************************************************

  Synopsis    [Converts SOP representation in Espresso into SOP in ABC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_SopFromEspresso( Extra_MmFlex_t * pMan, pset_family Cover )
{
    pset set;
    char * pSop, * pCube;
    int Lit, nVars, nCubes, i, k;

    nVars  = Cover->sf_size/2;
    nCubes = Cover->count;

    pSop = Abc_SopStart( pMan, nCubes, nVars );

    // go through the cubes
    i = 0;
    Abc_SopForEachCube( pSop, nVars, pCube )
    {
        set = GETSET(Cover, i++);
        for ( k = 0; k < nVars; k++ )
        {
            Lit = GETINPUT(set, k);
            if ( Lit == ZERO )
                pCube[k] = '0';
            else if ( Lit == ONE )
                pCube[k] = '1';
        }
    }
    return pSop;
}


/**Function*************************************************************

  Synopsis    [Minimizes the cover using Espresso.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
pset_family Abc_EspressoMinimize( pset_family pOnset, pset_family pDcset )
{
    pset_family pOffset;
    int fNewDcset, i;
    int fSimple = 0;
    int fSparse = 0;

    if ( fSimple )
    {
        for ( i = 0; i < cube.num_vars; i++ )
            pOnset = d1merge( pOnset, i );
        pOnset = sf_contain( pOnset );
        return pOnset;
    }

    // create the dcset
    fNewDcset = (pDcset == NULL);
    if ( pDcset == NULL )
        pDcset = sf_new( 1, cube.size );
    pDcset->wsize   = pOnset->wsize;
    pDcset->sf_size = pOnset->sf_size;

    // derive the offset
    if ( pDcset->sf_size == 0 || pDcset->count == 0 )
        pOffset = complement(cube1list(pOnset));
    else
        pOffset = complement(cube2list(pOnset, pDcset)); 

    // perform minimization
    skip_make_sparse = !fSparse;
    pOnset = espresso( pOnset, pDcset, pOffset );

    // free covers
    sf_free( pOffset ); 
    if ( fNewDcset )
        sf_free( pDcset );
    return pOnset;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

