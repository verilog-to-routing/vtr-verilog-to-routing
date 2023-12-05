/**CFile****************************************************************

  FileName    [sfmCnf.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based optimization using internal don't-cares.]

  Synopsis    [CNF computation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: sfmCnf.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sfmInt.h"
#include "bool/kit/kit.h"

ABC_NAMESPACE_IMPL_START


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
void Sfm_PrintCnf( Vec_Str_t * vCnf )
{
    signed char Entry;
    int i, Lit;
    Vec_StrForEachEntry( vCnf, Entry, i )
    {
        Lit = (int)Entry;
        if ( Lit == -1 )
            printf( "\n" );
        else
            printf( "%s%d ", Abc_LitIsCompl(Lit) ? "-":"", Abc_Lit2Var(Lit) );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sfm_TruthToCnf( word Truth, word * pTruth, int nVars, Vec_Int_t * vCover, Vec_Str_t * vCnf )
{
    int w, nWords = Abc_Truth6WordNum( nVars );
    Vec_StrClear( vCnf );
    if ( nVars <= 6 )
    {
        if ( Truth == 0 || ~Truth == 0 )
        {
            assert( nVars == 0 );
            Vec_StrPush( vCnf, (char)(Truth == 0) );
            Vec_StrPush( vCnf, (char)-1 );
            return 1;
        }
    }
    else
    {
        // const0
        for ( w = 0; w < nWords; w++ )
            if ( pTruth[w] )
                break;
        if ( w == nWords )
        {
            Vec_StrPush( vCnf, (char)1 );
            Vec_StrPush( vCnf, (char)-1 );
            assert( 0 );
            return 1;
        }
        // const1
        for ( w = 0; w < nWords; w++ )
            if ( ~pTruth[w] )
                break;
        if ( w == nWords )
        {
            Vec_StrPush( vCnf, (char)0 );
            Vec_StrPush( vCnf, (char)-1 );
            assert( 0 );
            return 1;
        }
    }
    {
        int i, k, c, RetValue, Literal, Cube, nCubes = 0;
        assert( nVars > 0 );
        for ( c = 0; c < 2; c ++ )
        {
            if ( nVars <= 6 )
            {
                Truth = c ? ~Truth : Truth;
                RetValue = Kit_TruthIsop( (unsigned *)&Truth, nVars, vCover, 0 );
            }
            else
            {
                if ( c )
                    for ( k = 0; k < nWords; k++ )
                        pTruth[k] = ~pTruth[k];
                RetValue = Kit_TruthIsop( (unsigned *)pTruth, nVars, vCover, 0 );
                if ( c )
                    for ( k = 0; k < nWords; k++ )
                        pTruth[k] = ~pTruth[k];
            }
            assert( RetValue == 0 );
            nCubes += Vec_IntSize( vCover );
            Vec_IntForEachEntry( vCover, Cube, i )
            {
                for ( k = 0; k < nVars; k++ )
                {
                    Literal = 3 & (Cube >> (k << 1));
                    if ( Literal == 1 )      // '0'  -> pos lit
                        Vec_StrPush( vCnf, (char)Abc_Var2Lit(k, 0) );
                    else if ( Literal == 2 ) // '1'  -> neg lit
                        Vec_StrPush( vCnf, (char)Abc_Var2Lit(k, 1) );
                    else if ( Literal != 0 )
                        assert( 0 );
                }
                Vec_StrPush( vCnf, (char)Abc_Var2Lit(nVars, c) );
                Vec_StrPush( vCnf, (char)-1 );
            }
        }
        return nCubes;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Wec_t * Sfm_CreateCnf( Sfm_Ntk_t * p )
{
    Vec_Wec_t * vCnfs;
    Vec_Str_t * vCnf, * vCnfBase;
    word uTruth;
    int i, nCubes;
    vCnf = Vec_StrAlloc( 100 );
    vCnfs = Vec_WecStart( p->nObjs );
    Vec_WrdForEachEntryStartStop( p->vTruths, uTruth, i, p->nPis, Vec_WrdSize(p->vTruths)-p->nPos )
    {
        int Offset  = Vec_IntEntry( p->vStarts, i );
        word * pRes = Vec_WrdSize(p->vTruths2) ? Vec_WrdEntryP( p->vTruths2, Offset ) : NULL;
        nCubes = Sfm_TruthToCnf( uTruth, pRes, Sfm_ObjFaninNum(p, i), p->vCover, vCnf );
        vCnfBase = (Vec_Str_t *)Vec_WecEntry( vCnfs, i );
        Vec_StrGrow( vCnfBase, Vec_StrSize(vCnf) );
        memcpy( Vec_StrArray(vCnfBase), Vec_StrArray(vCnf), (size_t)Vec_StrSize(vCnf) );
        vCnfBase->nSize = Vec_StrSize(vCnf);
    }
    Vec_StrFree( vCnf );
    return vCnfs;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sfm_TranslateCnf( Vec_Wec_t * vRes, Vec_Str_t * vCnf, Vec_Int_t * vFaninMap, int iPivotVar )
{
    Vec_Int_t * vClause;
    signed char Entry;
    int i, Lit;
    assert( Vec_StrEntry(vCnf, 1) != -1 || Vec_IntSize(vFaninMap) == 1 );
    Vec_WecClear( vRes );
    vClause = Vec_WecPushLevel( vRes );
    Vec_StrForEachEntry( vCnf, Entry, i )
    {
        if ( (int)Entry == -1 )
        {
            vClause = Vec_WecPushLevel( vRes );
            continue;
        }
        Lit = Abc_Lit2LitV( Vec_IntArray(vFaninMap), (int)Entry );
        Lit = Abc_LitNotCond( Lit, Abc_Lit2Var(Lit) == iPivotVar );
        Vec_IntPush( vClause, Lit );
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

