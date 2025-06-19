/**CFile****************************************************************

  FileName    [dauArray.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware unmapping.]

  Synopsis    [Array representation of DSD.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: dauArray.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "dauInt.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Dau_Dsd_t_ Dau_Dsd_t;
struct Dau_Dsd_t_
{
    unsigned       iVar    :   5;  // variable
    unsigned       nFans   :   5;  // fanin count
    unsigned       Depth   :   5;  // tree depth
    unsigned       Offset  :   5;  // the diff between this and other node
    unsigned       Data    :   5;  // user data
    unsigned       Type    :   3;  // node type
    unsigned       fCompl  :   1;  // the complemented attribute
    unsigned       fUnused :   1;  // this vertice is unused
};

static inline void Dau_DsdClean( Dau_Dsd_t * p ) { *((int *)p) = 0; }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
 
int Dau_DsdCountAnd( Dau_Dsd_t * p )
{
    int Count, Costs[7] = {0, 0, 0, 1, 3, 3, 3};
    for ( Count = 0; p->Type; p++ )
        Count += Costs[p->Type];
    return Count;
}

/*
void Dau_DsdMark( Dau_Dsd_t * p, int nSize, int * pMarks )
{
    int pStore[DAU_MAX_VAR] = {0};
    Dau_Dsd_t * q;
    if ( p->Type == DAU_DSD_CONST || p->Type == DAU_DSD_VAR )
        return;
    for ( q = p + nSize - 1; q >= p; q-- )
    {
        if ( q->Type == DAU_DSD_VAR )
            pStore[q->Depth] += pMarks[q->iVar];
        else 
        {
            q->Data = pStore[q->Depth+1]; pStore[q->Depth+1] = 0;
            pStore[q->Depth] += (q->Data == q->nFans);            
        }
    }
}
*/

int Dau_DsdConstruct( char * pDsd, Dau_Dsd_t * pStore )
{
    Dau_Dsd_t * pLevel[DAU_MAX_VAR];
    Dau_Dsd_t * q = pStore;
    int d = -1, fCompl = 0;
    if ( Dau_DsdIsConst(pDsd) )
    {
        Dau_DsdClean( q );
        q->Type   = DAU_DSD_CONST0;
        q->fCompl = Dau_DsdIsConst1(pDsd);
        return 1;
    }
    for ( --q; *pDsd; pDsd++ )
    {
        if ( *pDsd == '!' )
        {
            fCompl ^= 1;
            continue;
        }
        if ( *pDsd == ')' || *pDsd == ']' || *pDsd == '>' || *pDsd == '}' )
        {
            assert( fCompl == 0 );
            if ( --d >= 0 )
            {
                pLevel[d]->nFans++;
                if ( pLevel[d]->Data > pLevel[d+1]->Data )
                    pLevel[d]->Data = pLevel[d+1]->Data;
            }
            continue;
        }
        Dau_DsdClean( ++q );
        q->Data = 31;
        q->fCompl = fCompl;
        fCompl = 0;
        if ( *pDsd >= 'a' && *pDsd <= 'z' )
        {
            q->Type   = DAU_DSD_VAR;
            q->iVar   = *pDsd - 'a';
            q->Depth  = d + 1;
            if ( d >= 0 )
            {
                pLevel[d]->nFans++;
                if ( pLevel[d]->Data > q->iVar )
                    pLevel[d]->Data = q->iVar;
            }
            continue;
        }
        if ( *pDsd == '(' )
            q->Type = DAU_DSD_AND;
        else if ( *pDsd == '[' )
            q->Type = DAU_DSD_XOR;
        else if ( *pDsd == '<' )
            q->Type = DAU_DSD_MUX;
        else if ( *pDsd == '{' )
            q->Type = DAU_DSD_PRIME;
        else assert( 0 );
        pLevel[++d] = q;
        q->Depth = d;
    }
    assert( d == -1 );
    Dau_DsdClean( ++q );
    return q - pStore;
}

void Dau_DsdPrint( Dau_Dsd_t * p )
{
    char OpenType[7]  = {0, 0, 0, '(', '[', '<', '{'};
    char CloseType[7] = {0, 0, 0, ')', ']', '>', '}'};
    char pTypes[DAU_MAX_VAR];
    int d, pVisits[DAU_MAX_VAR];
    if ( p->Type == DAU_DSD_CONST0 )
    {
        printf( "%d\n", p->fCompl );
        return;
    }
    pVisits[0] = 1;
    for ( d = 0; p->Type; p++ )
    {
        if ( p->fCompl )
            printf( "!" );
        if ( p->Type == DAU_DSD_VAR )
        {
            printf( "%c", 'a' + p->iVar );
            while ( d > 0 && --pVisits[d] == 0 )
                printf( "%c", pTypes[d--] );
        }
        else
        {
            pVisits[++d] = p->nFans;
            printf( "%c", OpenType[p->Type] );
            printf( "%c", 'a' + p->Data );
            printf( "%d", p->Depth );
            pTypes[d]  = CloseType[p->Type];
        }
    }
    assert( d == 0 );
    printf( "\n" );
}

void Dau_DsdDepth( Dau_Dsd_t * p )
{
    int d, pVisits[DAU_MAX_VAR];
    if ( p->Type == DAU_DSD_CONST0 )
        return;
    pVisits[0] = 1;
    for ( d = 0; p->Type; p++ )
    {
        p->Depth = d;
        if ( p->Type == DAU_DSD_VAR )
            while ( d > 0 && --pVisits[d] == 0 )
                d--;
        else
            pVisits[++d] = p->nFans;
    }
    assert( d == 0 );
}

void Dau_DsdRemoveUseless( Dau_Dsd_t * p )
{
    Dau_Dsd_t * q = p, * pLevel[DAU_MAX_VAR];
    int d, fChange = 0, pVisits[DAU_MAX_VAR];
    if ( p->Type == DAU_DSD_CONST0 )
        return;
    pVisits[0] = 1;
    for ( d = 0; p->Type; p++ )
    {
        p->Depth = d;
        if ( p->Type == DAU_DSD_VAR )
            while ( d > 0 && --pVisits[d] == 0 )
                d--;
        else
        {
            if ( d > 0 && (pLevel[d-1]->Type == DAU_DSD_XOR && p->Type == DAU_DSD_XOR ||
                           pLevel[d-1]->Type == DAU_DSD_AND && p->Type == DAU_DSD_AND && !p->fCompl) )
            {
                pLevel[d-1]->nFans += p->nFans - 1;
                pVisits[d] += p->nFans - 1;
                p->fUnused = 1;
                fChange = 1;
            }
            else
            {
                pLevel[d++] = p;
                pVisits[d] = p->nFans;
            }
        }
    }
    assert( d == 0 );
    // compact
    if ( fChange )
    {
        for ( p = q; p->Type; p++ )
            if ( !p->fUnused )
                *q++ = *p;
        Dau_DsdClean( q );
    }
}

void Dau_DsdTest22()
{
    Dau_Dsd_t pStore[2 * DAU_MAX_VAR];
//    char * pDsd = "[(ab)c(f!(he))]";
//    char * pDsd = "[(abd)cf(f!{she})]";
    char * pDsd = "[(abd)[cf](f(sg(he)))]";
//    char * pDsd = "[(ab)[cf]]";
    int i, nSize = Dau_DsdConstruct( pDsd, pStore );
//    Dau_DsdDepth( pStore );
    Dau_DsdPrint( pStore );

    Dau_DsdRemoveUseless( pStore );

    Dau_DsdPrint( pStore );
    i = 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

