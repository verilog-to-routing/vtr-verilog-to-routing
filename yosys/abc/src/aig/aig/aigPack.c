/**CFile****************************************************************
 
  FileName    [aigPack.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [Bit-packing code.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigPack.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// resubstitution manager
typedef struct Aig_ManPack_t_ Aig_ManPack_t;
struct Aig_ManPack_t_
{
    Aig_Man_t *        pAig;        // working manager 
    Vec_Wrd_t *        vSigns;      // object signatures
    Vec_Wrd_t *        vPiPats;     // PI assignments
    Vec_Wrd_t *        vPiCare;     // PI care set
    int                iPatCur;     // current pattern
    int                fVerbose;    // verbosiness flag
    // statistics
    int                nPatTotal;   // number of all patterns
    int                nPatSkip;    // number of skipped patterns
    int                nPatRepeat;  // number of repeated patterns
};

static inline int Aig_Word6CountOnes( word t )  { return Aig_WordCountOnes( (unsigned)(t >> 32) ) + Aig_WordCountOnes( (unsigned)(t & 0xFFFFFFFF) ); }
static inline int Aig_Word6HasOneBit( word t )  { return (t & (t-1)) == 0; }


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_ManPack_t * Aig_ManPackAlloc( Aig_Man_t * pAig )
{
    Aig_ManPack_t * p;
    p = ABC_CALLOC( Aig_ManPack_t, 1 );
    p->pAig    = pAig;
    p->vSigns  = Vec_WrdStart( Aig_ManObjNumMax(pAig) );
    p->vPiPats = Vec_WrdStart( Aig_ManCiNum(pAig) );
    p->vPiCare = Vec_WrdStart( Aig_ManCiNum(pAig) );
    p->iPatCur = 1;
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManPackCountCares( Aig_ManPack_t * p )
{
    Aig_Obj_t * pObj;
    int i, Total = 0;
    Aig_ManForEachCi( p->pAig, pObj, i )
        Total += Aig_Word6CountOnes( Vec_WrdEntry(p->vPiCare, i) );
    return Total;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManPackPrintCare( Aig_ManPack_t * p )
{
    Aig_Obj_t * pObj;
    word Sign;
    int i;
    Aig_ManForEachCi( p->pAig, pObj, i )
    {
        Sign = Vec_WrdEntry( p->vPiCare, i );
//        Extra_PrintBinary( stdout, (unsigned *)&Sign, 64 );
//        printf( "\n" );
    }
//    printf( "\n" );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManPackFree( Aig_ManPack_t * p )
{
//    Aig_ManPackPrintCare( p );
    printf( "Patterns: " );
    printf( "Total = %6d. ",   p->nPatTotal );
    printf( "Skipped = %6d. ", p->nPatSkip );
    printf( "Cares = %6.2f %%  ", 100.0*Aig_ManPackCountCares(p)/Aig_ManCiNum(p->pAig)/64 );
    printf( "\n" );
    Vec_WrdFree( p->vSigns );
    Vec_WrdFree( p->vPiPats );
    Vec_WrdFree( p->vPiCare );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManPackSetRandom( Aig_ManPack_t * p )
{
    Aig_Obj_t * pObj;
    word Sign;
    int i;
    Aig_ManForEachCi( p->pAig, pObj, i )
    {
        Sign = (((word)Aig_ManRandom(0)) << 32) | ((word)Aig_ManRandom(0));
        Vec_WrdWriteEntry( p->vPiPats, i, Sign << 1 );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManPackSimulate( Aig_ManPack_t * p )
{
    Aig_Obj_t * pObj;
    word Sign, Sign0, Sign1;
    int i;
    // set the constant
    Vec_WrdWriteEntry( p->vSigns, 0, ~(word)0 );
    // transfer into the array
    Aig_ManForEachCi( p->pAig, pObj, i )
        Vec_WrdWriteEntry( p->vSigns, Aig_ObjId(pObj), Vec_WrdEntry(p->vPiPats, i) );
    // simulate internal nodes
    Aig_ManForEachNode( p->pAig, pObj, i )
    {
        Sign0 = Vec_WrdEntry( p->vSigns, Aig_ObjFaninId0(pObj) );
        Sign1 = Vec_WrdEntry( p->vSigns, Aig_ObjFaninId1(pObj) );
        if ( Aig_ObjFaninC0(pObj) && Aig_ObjFaninC1(pObj) )
            Sign = ~(Sign0 | Sign1);
        else if ( Aig_ObjFaninC0(pObj) )
            Sign = ~Sign0 &  Sign1;
        else if ( Aig_ObjFaninC1(pObj) )
            Sign =  Sign0 & ~Sign1;
        else
            Sign =  Sign0 &  Sign1;
        Vec_WrdWriteEntry( p->vSigns, Aig_ObjId(pObj), Sign );
    }
    // set the outputs
    Aig_ManForEachCo( p->pAig, pObj, i )
    {
        Sign0 = Vec_WrdEntry( p->vSigns, Aig_ObjFaninId0(pObj) );
        Sign  = Aig_ObjFaninC0(pObj) ? ~Sign0 : Sign0;
        Vec_WrdWriteEntry( p->vSigns, Aig_ObjId(pObj), Sign );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManPackPrintStats( Aig_ManPack_t * p )
{
    word Sign;
    Aig_Obj_t * pObj;
    int i, Total, Count, Counts[33] = {0}; // the number of nodes having that many patterns
    Aig_ManForEachNode( p->pAig, pObj, i )
    {
        Sign  = Vec_WrdEntry( p->vSigns, Aig_ObjId(pObj) );
        Count = Aig_Word6CountOnes( Sign );
        if ( Count > 32 )
            Count = 64 - Count;
        Counts[Count]++;        
    }
    // print statistics
    Total = 0;
    for ( i = 0; i <= 32; i++ )
    {
        Total += Counts[i];
        printf( "%2d : ", i );
        printf( "%6d  ", Counts[i] );
        printf( "%6.1f %%", 100.0*Counts[i]/Aig_ManNodeNum(p->pAig) );
        printf( "%6.1f %%", 100.0*Total/Aig_ManNodeNum(p->pAig) );
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Aig_ManPackConstNodes( Aig_ManPack_t * p )
{
    Vec_Int_t * vNodes;
    Aig_Obj_t * pObj;
    word Sign;
    int i;
    vNodes = Vec_IntAlloc( 1000 );
    Aig_ManForEachNode( p->pAig, pObj, i )
    {
        Sign  = Vec_WrdEntry( p->vSigns, Aig_ObjId(pObj) );
        if ( Sign == 0 || ~Sign == 0 || Aig_Word6HasOneBit(Sign) || Aig_Word6HasOneBit(~Sign) )
            Vec_IntPush( vNodes, Aig_ObjId(pObj) );
    }
    return vNodes;
}


/**Function*************************************************************

  Synopsis    [Packs patterns into array of simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

*************************************`**********************************/
int Aig_ManPackAddPatternTry( Aig_ManPack_t * p, int iBit, Vec_Int_t * vLits )
{
    word * pInfo, * pPres;
    int i, Lit;
    Vec_IntForEachEntry( vLits, Lit, i )
    {
        pInfo = Vec_WrdEntryP( p->vPiPats, Abc_Lit2Var(Lit) );
        pPres = Vec_WrdEntryP( p->vPiCare, Abc_Lit2Var(Lit) );
        if ( Abc_InfoHasBit( (unsigned *)pPres, iBit ) && 
             Abc_InfoHasBit( (unsigned *)pInfo, iBit ) == Abc_LitIsCompl(Lit) )
             return 0;
    }
    Vec_IntForEachEntry( vLits, Lit, i )
    {
        pInfo = Vec_WrdEntryP( p->vPiPats, Abc_Lit2Var(Lit) );
        pPres = Vec_WrdEntryP( p->vPiCare, Abc_Lit2Var(Lit) );
        Abc_InfoSetBit( (unsigned *)pPres, iBit );
        if ( Abc_InfoHasBit( (unsigned *)pInfo, iBit ) == Abc_LitIsCompl(Lit) )
             Abc_InfoXorBit( (unsigned *)pInfo, iBit );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManPackAddPattern( Aig_ManPack_t * p, Vec_Int_t * vLits )
{
    int k;
    for ( k = 1; k < 64; k++ )
        if ( Aig_ManPackAddPatternTry( p, k, vLits ) )
            break;
    if ( k == 64 )
    {
/*
        word * pInfo, * pPres;
        int i, Lit;
        Vec_IntForEachEntry( vLits, Lit, i )
            printf( "%d", Abc_LitIsCompl(Lit) );
        printf( "\n\n" );
        for ( k = 1; k < 64; k++ )
        {
            Vec_IntForEachEntry( vLits, Lit, i )
            {
                pInfo = Vec_WrdEntryP( p->vPiPats, Abc_Lit2Var(Lit) );
                pPres = Vec_WrdEntryP( p->vPiCare, Abc_Lit2Var(Lit) );
                if ( Abc_InfoHasBit( (unsigned *)pPres, k ) )
                    printf( "%d", Abc_InfoHasBit( (unsigned *)pInfo, k ) );
                else
                    printf( "-" );
            }
            printf( "\n" );
        }
*/
        p->nPatSkip++;
    }
    p->nPatTotal++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_ManPack_t * Aig_ManPackStart( Aig_Man_t * pAig )
{
    Aig_ManPack_t * p;
    p = Aig_ManPackAlloc( pAig );
    Aig_ManPackSetRandom( p );
    Aig_ManPackSimulate( p );
    Aig_ManPackPrintStats( p );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManPackStop( Aig_ManPack_t * p )
{
    Aig_ManPackSimulate( p );
    Aig_ManPackPrintStats( p );
    Aig_ManPackFree( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

