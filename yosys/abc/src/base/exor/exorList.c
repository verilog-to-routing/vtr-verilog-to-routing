/**CFile****************************************************************

  FileName    [exorList.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Exclusive sum-of-product minimization.]

  Synopsis    [Cube lists.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: exorList.c,v 1.0 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

////////////////////////////////////////////////////////////////////////
///                                                                  ///
///                  Implementation of EXORCISM - 4                  ///
///              An Exclusive Sum-of-Product Minimizer               ///
///                                                                  ///
///               Alan Mishchenko  <alanmi@ee.pdx.edu>               ///
///                                                                  ///
////////////////////////////////////////////////////////////////////////
///                                                                  ///
///                 Iterative Cube Set Minimization                  ///
///                  Iterative ExorLink Procedure                    ///
///                  Support of Cube Pair Queques                    ///
///                                                                  ///
///  Ver. 1.0. Started - July 18, 2000. Last update - July 20, 2000  ///
///  Ver. 1.1. Started - July 24, 2000. Last update - July 29, 2000  ///
///  Ver. 1.2. Started - July 30, 2000. Last update - July 31, 2000  ///
///  Ver. 1.4. Started -  Aug 10, 2000. Last update -  Aug 26, 2000  ///
///  Ver. 1.5. Started -  Aug 30, 2000. Last update -  Aug 30, 2000  ///
///  Ver. 1.6. Started -  Sep 11, 2000. Last update -  Sep 15, 2000  ///
///  Ver. 1.7. Started -  Sep 20, 2000. Last update -  Sep 23, 2000  ///
///                                                                  ///
////////////////////////////////////////////////////////////////////////
///   This software was tested with the BDD package "CUDD", v.2.3.0  ///
///                          by Fabio Somenzi                        ///
///                  http://vlsi.colorado.edu/~fabio/                ///
////////////////////////////////////////////////////////////////////////

#include "exor.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                       EXTERNAL VARIABLES                         ///
////////////////////////////////////////////////////////////////////////

// information about options and the cover
extern cinfo g_CoverInfo;

// the look-up table for the number of 1's in unsigned short
extern unsigned char BitCount[];

////////////////////////////////////////////////////////////////////////
///                       EXTERNAL FUNCTIONS                         ///
////////////////////////////////////////////////////////////////////////

extern int GetDistance( Cube* pC1, Cube* pC2 );
// distance computation for two cubes
extern int GetDistancePlus( Cube* pC1, Cube* pC2 );

extern void ExorVar( Cube* pC, int Var, varvalue Val );

extern void AddToFreeCubes( Cube* pC );
// returns a simplified cube back into the free list

//extern void PrintCube( ostream& DebugStream, Cube* pC );
// debug output for cubes

extern Cube* GetFreeCube();

////////////////////////////////////////////////////////////////////////
/// ExorLink Functions
extern int ExorLinkCubeIteratorStart( Cube** pGroup, Cube* pC1, Cube* pC2, cubedist Dist );
// this function starts the Exor-Link IteratorCubePair, which iterates
// through the cube groups starting from the group with min literals
// returns 1 on success, returns 0 if the cubes have wrong distance

extern int ExorLinkCubeIteratorNext( Cube** pGroup );
// give the next group in the decreasing order of sum of literals
// returns 1 on success, returns 0 if there are no more groups

extern int ExorLinkCubeIteratorPick( Cube** pGroup, int g );
// gives the group #g in the order in which the groups were given
// during iteration
// returns 1 on success, returns 0 if something g is too large

extern void ExorLinkCubeIteratorCleanUp( int fTakeLastGroup );
// removes the cubes from the store back into the list of free cubes
// if fTakeLastGroup is 0, removes all cubes
// if fTakeLastGroup is 1, does not store the last group

////////////////////////////////////////////////////////////////////////
///                    FUNCTIONS OF THIS MODULE                      ///
////////////////////////////////////////////////////////////////////////

// iterative ExorLink
int IterativelyApplyExorLink2( char fDistEnable );   
int IterativelyApplyExorLink3( char fDistEnable );   
int IterativelyApplyExorLink4( char fDistEnable );   

// function which performs distance computation and simplifes on the fly
// it is also called from the Pseudo-Kronecker module when cubes are added
int CheckForCloseCubes( Cube* p, int fAddCube );
int CheckAndInsert( Cube* p );

// this function changes the cube back after it was DIST-1 transformed
void UndoRecentChanges();

////////////////////////////////////////////////////////////////////////
// iterating through adjucency pair queques (with authomatic garbage collection)

// start an iterator through cubes of dist CubeDist,
// the resulting pointers are written into ppC1 and ppC2
int IteratorCubePairStart( cubedist Dist, Cube** ppC1, Cube** ppC2 );
// gives the next VALID cube pair (the previous one is automatically dequequed)
int IteratorCubePairNext();

////////////////////////////////////////////////////////////////////////
// the cube storage

// cube storage allocation/delocation
int AllocateCubeSets( int nVarsIn, int nVarsOut );
void DelocateCubeSets();

// insert/extract a cube into/from the storage
void CubeInsert( Cube* p );
Cube* CubeExtract( Cube* p );

////////////////////////////////////////////////////////////////////////
// Cube Set Iterator
Cube* IterCubeSetStart();
// starts an iterator that traverses all the cubes in the ring
Cube* IterCubeSetNext();
// returns the next cube in the ring
// to use it again after it has returned NULL, call IterCubeSetStart() first

////////////////////////////////////////////////////////////////////////
// cube adjacency queques

// adjacency queque allocation/delocation procedures
int AllocateQueques( int nPlaces );
void DelocateQueques();

// conditional adding cube pairs to queques
// reset temporarily stored new range of cube pairs
static void NewRangeReset();
// add temporarily stored new range of cube pairs to the queque
static void NewRangeAdd();
// insert one cube pair into the new range
static void NewRangeInsertCubePair( cubedist Dist, Cube* p1, Cube* p2 );

static void MarkSet();
static void MarkRewind();

void PrintQuequeStats();
int GetQuequeStats( cubedist Dist );

// iterating through the queque (with authomatic garbage collection)
// start an iterator through cubes of dist CubeDist,
// the resulting pointers are written into ppC1 and ppC2
int IteratorCubePairStart( cubedist Dist, Cube** ppC1, Cube** ppC2 );
// gives the next VALID cube pair (the previous one is automatically dequequed)
int IteratorCubePairNext();

////////////////////////////////////////////////////////////////////////
///                      EXPORTED VARIABLES                          ///
////////////////////////////////////////////////////////////////////////`

// the number of allocated places
int s_nPosAlloc;
// the maximum number of occupied places
int s_nPosMax[3];

////////////////////////////////////////////////////////////////////////
///                      Minimization Strategy                       ///
////////////////////////////////////////////////////////////////////////

// 1) check that ExorLink for this cube pair can be performed
//    (it may happen that the group is outdated due to recent reshaping)
// 2) find out what is the improvement achieved by each cube group
// 3) depending on the distance, do the following:
//    a) if ( Dist == 2 ) 
//           try both cube groups, 
//           if one of them leads to improvement, take the cube group right away
//           if none of them leads to improment
//              - take the last one (because it reshapes)
//              - take the last one only in case it does not increase literals
//    b) if ( Dist == 3 )
//           try groups one by one
//           if one of them leads to improvement, take the group right away
//           if none of them leads to improvement
//              - take the group which reshapes
//              - take the reshaping group only in case it does not increase literals
//           if none of them leads to reshaping, do not take any of them
//    c) if ( Dist == 4 )
//           try groups one by one
//           if one of the leads to reshaping, take it right away
//           if none of them leads to reshaping, do not take any of them

////////////////////////////////////////////////////////////////////////
///                       STATIC VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

// Cube set is a list of cubes
static Cube* s_List;

///////////////////////////////////////////////////////////////////////////
// undo information
///////////////////////////////////////////////////////////////////////////
static struct
{
    int fInput;   // 1 if the input was changed
    Cube* p;      // the pointer to the modified cube
    int PrevQa;
    int PrevPa;
    int PrevQq;
    int PrevPq;
    int PrevPz;
    int Var;      // the number of variable that was changed
    int Value;    // the value what was there
    int PrevID;   // the previous ID of the removed cube
} s_ChangeStore;
///////////////////////////////////////////////////////////////////////////

// enable pair accumulation
// from the begginning (while the starting cover is generated)
// only the distance 2 accumulation is enabled
static int s_fDistEnable2 = 1;
static int s_fDistEnable3;
static int s_fDistEnable4;

// temporary storage for cubes generated by the ExorLink iterator
static Cube* s_CubeGroup[5];
// the marks telling whether the given cube is inserted
static int s_fInserted[5];

// enable selection only those Dist2 and Dist3 that do not increase literals
int s_fDecreaseLiterals = 0;

// the counters for display
static int s_cEnquequed;
static int s_cAttempts;
static int s_cReshapes;

// the number of cubes before ExorLink starts
static int s_nCubesBefore;
// the distance code specific for each ExorLink
static cubedist s_Dist;

// other variables
static int s_Gain;
static int s_GainTotal;
static int s_GroupCounter;
static int s_GroupBest;
static Cube *s_pC1, *s_pC2;

////////////////////////////////////////////////////////////////////////
///                  Iterative ExorLink Operation                    ///
////////////////////////////////////////////////////////////////////////

int CheckAndInsert( Cube* p )
{
//  return CheckForCloseCubes( p, 1 );
    CubeInsert( p );
    return 0;
}

int IterativelyApplyExorLink2( char fDistEnable )   
// MEMO: instead of inserting the cubes that have already been checked
// by running CheckForCloseCubes again, try inserting them without checking
// and observe the difference (it will save 50% of checking time)
{
    int z;

    // this var is specific to ExorLink-2
    s_Dist = (cubedist)0;

    // enable pair accumulation
    s_fDistEnable2 = fDistEnable & 1;
    s_fDistEnable3 = fDistEnable & 2;
    s_fDistEnable4 = fDistEnable & 4;

    // initialize counters
    s_cEnquequed = GetQuequeStats( s_Dist );
    s_cAttempts  = 0;
    s_cReshapes  = 0;

    // remember the number of cubes before minimization
    s_nCubesBefore = g_CoverInfo.nCubesInUse;

    for ( z = IteratorCubePairStart( s_Dist, &s_pC1, &s_pC2 ); z; z = IteratorCubePairNext() )
    {
        s_cAttempts++;
        // start ExorLink of the given Distance
        if ( ExorLinkCubeIteratorStart( s_CubeGroup, s_pC1, s_pC2, s_Dist ) )
        {
            // extract old cubes from storage (to prevent EXORing with their derivitives)
            CubeExtract( s_pC1 );
            CubeExtract( s_pC2 );

            // mark the current position in the cube pair queques
            MarkSet();

            // check the first group (generated by ExorLinkCubeIteratorStart())
            if ( CheckForCloseCubes( s_CubeGroup[0], 0 ) ) 
            {  // the first cube leads to improvement - it is already inserted
                CheckForCloseCubes( s_CubeGroup[1], 1 ); // insert the second cube
                goto SUCCESS;
            }
            if ( CheckForCloseCubes( s_CubeGroup[1], 0 ) ) 
            {  // the second cube leads to improvement - it is already inserted
                CheckForCloseCubes( s_CubeGroup[0], 1 ); // insert the first cube
//              CheckAndInsert( s_CubeGroup[0] );
                goto SUCCESS;
            }
            // the first group does not lead to improvement

            // rewind to the previously marked position in the cube pair queques
            MarkRewind();

            // generate the second group
            ExorLinkCubeIteratorNext( s_CubeGroup );

            // check the second group
            if ( CheckForCloseCubes( s_CubeGroup[0], 0 ) ) 
            { // the first cube leads to improvement - it is already inserted
                CheckForCloseCubes( s_CubeGroup[1], 1 ); // insert the second cube
                goto SUCCESS;
            }
            if ( CheckForCloseCubes( s_CubeGroup[1], 0 ) ) 
            { // the second cube leads to improvement - it is already inserted
                CheckForCloseCubes( s_CubeGroup[0], 1 ); // insert the first cube
//              CheckAndInsert( s_CubeGroup[0] );
                goto SUCCESS;
            }
            // the second group does not lead to improvement

            // decide whether to accept the second group, depending on literals
            if ( s_fDecreaseLiterals )
            {
                if ( g_CoverInfo.fUseQCost ? 
                     s_CubeGroup[0]->q + s_CubeGroup[1]->q >= s_pC1->q + s_pC2->q :
                     s_CubeGroup[0]->a + s_CubeGroup[1]->a >= s_pC1->a + s_pC2->a ) 
                // the group increases literals
                { // do not take the last group
                            
                    // rewind to the previously marked position in the cube pair queques
                    MarkRewind();

                    // return the old cubes back to storage
                    CubeInsert( s_pC1 );
                    CubeInsert( s_pC2 );
                    // clean the results of generating ExorLinked cubes
                    ExorLinkCubeIteratorCleanUp( 0 );
                    continue;
                }
            }

            // take the last group
            // there is no need to test these cubes again,
            // because they have been tested and did not yield any improvement
            CubeInsert( s_CubeGroup[0] );
            CubeInsert( s_CubeGroup[1] );
//          CheckForCloseCubes( s_CubeGroup[0], 1 ); 
//          CheckForCloseCubes( s_CubeGroup[1], 1 ); 

SUCCESS:
            // clean the results of generating ExorLinked cubes
            ExorLinkCubeIteratorCleanUp( 1 ); // take the last group
            // free old cubes
            AddToFreeCubes( s_pC1 );
            AddToFreeCubes( s_pC2 );
            // increate the counter
            s_cReshapes++;
        }
    }
    // print the report
    if ( g_CoverInfo.Verbosity == 2 )
    {
    printf( "ExLink-%d", 2 );
    printf( ": Que= %5d", s_cEnquequed );
    printf( "  Att= %4d", s_cAttempts );
    printf( "  Resh= %4d", s_cReshapes );
    printf( "  NoResh= %4d", s_cAttempts - s_cReshapes );
    printf( "  Cubes= %3d", g_CoverInfo.nCubesInUse );
    printf( "  (%d)", s_nCubesBefore - g_CoverInfo.nCubesInUse );
    printf( "  Lits= %5d", CountLiterals() );
    printf( "  QCost = %6d", CountQCost() );
    printf( "\n" );
    }

    // return the number of cubes gained in the process
    return s_nCubesBefore - g_CoverInfo.nCubesInUse;
}

int IterativelyApplyExorLink3( char fDistEnable )   
{
    int z, c, d;
    // this var is specific to ExorLink-3
    s_Dist = (cubedist)1;

    // enable pair accumulation
    s_fDistEnable2 = fDistEnable & 1;
    s_fDistEnable3 = fDistEnable & 2;
    s_fDistEnable4 = fDistEnable & 4;

    // initialize counters
    s_cEnquequed = GetQuequeStats( s_Dist );
    s_cAttempts  = 0;
    s_cReshapes  = 0;

    // remember the number of cubes before minimization
    s_nCubesBefore = g_CoverInfo.nCubesInUse;

    for ( z = IteratorCubePairStart( s_Dist, &s_pC1, &s_pC2 ); z; z = IteratorCubePairNext() )
    {
        s_cAttempts++;
        // start ExorLink of the given Distance
        if ( ExorLinkCubeIteratorStart( s_CubeGroup, s_pC1, s_pC2, s_Dist ) )
        {
            // extract old cubes from storage (to prevent EXORing with their derivitives)
            CubeExtract( s_pC1 );
            CubeExtract( s_pC2 );

            // mark the current position in the cube pair queques
            MarkSet();

            // check cube groups one by one
            s_GroupCounter = 0;
            do  
            {   // check the cubes of this group one by one
                for ( c = 0; c < 3; c++ )
                if ( !s_CubeGroup[c]->fMark ) // this cube has not yet been checked
                {
                    s_Gain = CheckForCloseCubes( s_CubeGroup[c], 0 ); // do not insert the cube, by default
                    if ( s_Gain ) 
                    { // this cube leads to improvement or reshaping - it is already inserted

                        // decide whether to accept this group based on literal count
                        if ( s_fDecreaseLiterals && s_Gain == 1 )
                        if ( g_CoverInfo.fUseQCost ?
                             s_CubeGroup[0]->q + s_CubeGroup[1]->q + s_CubeGroup[2]->q > s_pC1->q + s_pC2->q + s_ChangeStore.PrevQq :
                             s_CubeGroup[0]->a + s_CubeGroup[1]->a + s_CubeGroup[2]->a > s_pC1->a + s_pC2->a + s_ChangeStore.PrevQa 
                           ) // the group increases literals
                        { // do not take this group
                            // remember the group
                            s_GroupBest = s_GroupCounter;
                            // undo changes to be able to continue checking other groups
                            UndoRecentChanges();
                            break;
                        }

                        // take this group
                        for ( d = 0; d < 3; d++ ) // insert other cubes
                            if ( d != c )
                            {
                                CheckForCloseCubes( s_CubeGroup[d], 1 ); 
//                              if ( s_CubeGroup[d]->fMark )
//                                  CheckAndInsert( s_CubeGroup[d] );
//                                  CheckOnlyOneCube( s_CubeGroup[d] );
//                                  CheckForCloseCubes( s_CubeGroup[d], 1 ); 
//                              else
//                                  CheckForCloseCubes( s_CubeGroup[d], 1 ); 
                            }

                        // clean the results of generating ExorLinked cubes
                        ExorLinkCubeIteratorCleanUp( 1 ); // take the last group
                        // free old cubes
                        AddToFreeCubes( s_pC1 );
                        AddToFreeCubes( s_pC2 );
                        // update the counter
                        s_cReshapes++;
                        goto END_OF_LOOP;
                    }
                    else // mark the cube as checked
                        s_CubeGroup[c]->fMark = 1;
                }
                // the group is not taken - find the new group
                s_GroupCounter++;
                                        
                // rewind to the previously marked position in the cube pair queques
                MarkRewind();
            } 
            while ( ExorLinkCubeIteratorNext( s_CubeGroup ) );
            // none of the groups leads to improvement

            // return the old cubes back to storage
            CubeInsert( s_pC1 );
            CubeInsert( s_pC2 );
            // clean the results of generating ExorLinked cubes
            ExorLinkCubeIteratorCleanUp( 0 );
        }
END_OF_LOOP: {}
    }

    // print the report
    if ( g_CoverInfo.Verbosity == 2 )
    {
    printf( "ExLink-%d", 3 );
    printf( ": Que= %5d", s_cEnquequed );
    printf( "  Att= %4d", s_cAttempts );
    printf( "  Resh= %4d", s_cReshapes );
    printf( "  NoResh= %4d", s_cAttempts - s_cReshapes );
    printf( "  Cubes= %3d", g_CoverInfo.nCubesInUse );
    printf( "  (%d)", s_nCubesBefore - g_CoverInfo.nCubesInUse );
    printf( "  Lits= %5d", CountLiterals() );
    printf( "  QCost = %6d", CountQCost() );
    printf( "\n" );
    }

    // return the number of cubes gained in the process
    return s_nCubesBefore - g_CoverInfo.nCubesInUse;
}

int IterativelyApplyExorLink4( char fDistEnable )   
{
    int z, c;
    // this var is specific to ExorLink-4
    s_Dist = (cubedist)2;

    // enable pair accumulation
    s_fDistEnable2 = fDistEnable & 1;
    s_fDistEnable3 = fDistEnable & 2;
    s_fDistEnable4 = fDistEnable & 4;

    // initialize counters
    s_cEnquequed = GetQuequeStats( s_Dist );
    s_cAttempts  = 0;
    s_cReshapes  = 0;

    // remember the number of cubes before minimization
    s_nCubesBefore = g_CoverInfo.nCubesInUse;

    for ( z = IteratorCubePairStart( s_Dist, &s_pC1, &s_pC2 ); z; z = IteratorCubePairNext() )
    {
        s_cAttempts++;
        // start ExorLink of the given Distance
        if ( ExorLinkCubeIteratorStart( s_CubeGroup, s_pC1, s_pC2, s_Dist ) )
        {
            // extract old cubes from storage (to prevent EXORing with their derivitives)
            CubeExtract( s_pC1 );
            CubeExtract( s_pC2 );

            // mark the current position in the cube pair queques
            MarkSet();

            // check cube groups one by one
            do  
            {   // check the cubes of this group one by one
                s_GainTotal = 0;
                for ( c = 0; c < 4; c++ )
                if ( !s_CubeGroup[c]->fMark ) // this cube has not yet been checked
                {
                    s_Gain = CheckForCloseCubes( s_CubeGroup[c], 0 ); // do not insert the cube, by default
                    // if the cube leads to gain, it is already inserted
                    s_fInserted[c] = (int)(s_Gain>0);
                    // increment the total gain
                    s_GainTotal += s_Gain;
                }
                else
                    s_fInserted[c] = 0; // the cube has already been checked - it is not inserted

                if ( s_GainTotal == 0 ) // the group does not lead to any gain
                { // mark the cubes
                    for ( c = 0; c < 4; c++ )
                        s_CubeGroup[c]->fMark = 1;
                }
                else if ( s_GainTotal == 1 ) // the group does not lead to substantial gain, too
                { 
                    // undo changes to be able to continue checking groups
                    UndoRecentChanges();
                    // mark those cubes that were not inserted
                    for ( c = 0; c < 4; c++ )
                        s_CubeGroup[c]->fMark = !s_fInserted[c];
                }
                else // if ( s_GainTotal > 1 ) // the group reshapes or improves
                { // accept the group
                    for ( c = 0; c < 4; c++ ) // insert other cubes
                        if ( !s_fInserted[c] )
                            CheckForCloseCubes( s_CubeGroup[c], 1 ); 
//                          CheckAndInsert( s_CubeGroup[c] );
                    // clean the results of generating ExorLinked cubes
                    ExorLinkCubeIteratorCleanUp( 1 ); // take the last group
                    // free old cubes
                    AddToFreeCubes( s_pC1 );
                    AddToFreeCubes( s_pC2 );
                    // update the counter
                    s_cReshapes++;
                    goto END_OF_LOOP;
                }
                                        
                // rewind to the previously marked position in the cube pair queques
                MarkRewind();
            } 
            while ( ExorLinkCubeIteratorNext( s_CubeGroup ) );
            // none of the groups leads to improvement

            // return the old cubes back to storage
            CubeInsert( s_pC1 );
            CubeInsert( s_pC2 );
            // clean the results of generating ExorLinked cubes
            ExorLinkCubeIteratorCleanUp( 0 );
        }
END_OF_LOOP: {}
    }

    // print the report
    if ( g_CoverInfo.Verbosity == 2 )
    {
    printf( "ExLink-%d", 4 );
    printf( ": Que= %5d", s_cEnquequed );
    printf( "  Att= %4d", s_cAttempts );
    printf( "  Resh= %4d", s_cReshapes );
    printf( "  NoResh= %4d", s_cAttempts - s_cReshapes );
    printf( "  Cubes= %3d", g_CoverInfo.nCubesInUse );
    printf( "  (%d)", s_nCubesBefore - g_CoverInfo.nCubesInUse );
    printf( "  Lits= %5d", CountLiterals() );
    printf( "  QCost = %6d", CountQCost() );
    printf( "\n" );
    }

    // return the number of cubes gained in the process
    return s_nCubesBefore - g_CoverInfo.nCubesInUse;
}

// local static variables
Cube* s_q;
int s_Distance;
int s_DiffVarNum;
int s_DiffVarValueP_old;
int s_DiffVarValueP_new;
int s_DiffVarValueQ;

int CheckForCloseCubes( Cube* p, int fAddCube )
// checks the cube storage for a cube that is dist-0 and dist-1 removed 
// from the given one (p) if such a cube is found, extracts it from the data 
// structure, EXORs it with the given cube, adds the resultant cube
// to the data structure and performed the same check for the resultant cube;
// returns the number of cubes gained in the process of reduction;
// if an adjacent cube is not found, inserts the cube only if (fAddCube==1)!!!
{
    // start the new range
    NewRangeReset();

    for ( s_q = s_List; s_q; s_q = s_q->Next )
    {
        s_Distance = GetDistancePlus( p, s_q );
        if ( s_Distance > 4 )
        {
        }
        else if ( s_Distance == 4 )
        {
            if ( s_fDistEnable4 ) 
                NewRangeInsertCubePair( DIST4, p, s_q );
        }
        else if ( s_Distance == 3 )
        {
            if ( s_fDistEnable3 ) 
                NewRangeInsertCubePair( DIST3, p, s_q );
        }
        else if ( s_Distance == 2 )
        { 
            if ( s_fDistEnable2 ) 
                NewRangeInsertCubePair( DIST2, p, s_q );
        }
        else if ( s_Distance == 1 )
        {   // extract the cube from the data structure

            //////////////////////////////////////////////////////////
            // store the changes
            s_ChangeStore.fInput = (s_DiffVarNum != -1);
            s_ChangeStore.p      = p;
            s_ChangeStore.PrevQa = s_q->a;
            s_ChangeStore.PrevPa = p->a;
            s_ChangeStore.PrevQq = s_q->q;
            s_ChangeStore.PrevPq = p->q;
            s_ChangeStore.PrevPz = p->z;
            s_ChangeStore.Var    = s_DiffVarNum;
            s_ChangeStore.Value  = s_DiffVarValueQ;
            s_ChangeStore.PrevID = s_q->ID;
            //////////////////////////////////////////////////////////

            CubeExtract( s_q );
            // perform the EXOR of the two cubes and write the result into p

            // it is important that the resultant cube is written into p!!!

            if ( s_DiffVarNum == -1 )
            {
                int i;
                // exor the output part
                p->z = 0;
                for ( i = 0; i < g_CoverInfo.nWordsOut; i++ )
                {
                    p->pCubeDataOut[i] ^= s_q->pCubeDataOut[i];
                    p->z += BIT_COUNT(p->pCubeDataOut[i]);
                }
            }
            else
            {
                // the cube has already been updated by GetDistancePlus()

                // modify the parameters of the number of literals in the new cube
//              p->a += s_UpdateLiterals[ s_DiffVarValueP ][ s_DiffVarValueQ ];
                if ( s_DiffVarValueP_old == VAR_NEG || s_DiffVarValueP_old == VAR_POS )
                    p->a--;
                if ( s_DiffVarValueP_new == VAR_NEG || s_DiffVarValueP_new == VAR_POS )
                    p->a++;
                p->q = ComputeQCostBits(p);
            }

            // move q to the free cube list
            AddToFreeCubes( s_q );

            // make sure that nobody with use the pairs created so far
//          NewRangeReset();
            // call the function again for the new cube
            return 1 + CheckForCloseCubes( p, 1 );
        }
        else // if ( Distance == 0 )
        {   // extract the second cube from the data structure and add them both to the free list
            AddToFreeCubes( p );
            AddToFreeCubes( CubeExtract( s_q ) );

            // make sure that nobody with use the pairs created so far
            NewRangeReset();
            return 2;
        }
    }
    
    // add the cube to the data structure if needed
    if ( fAddCube )
        CubeInsert( p );

    // add temporarily stored new range of cube pairs to the queque
    NewRangeAdd();

    return 0;
}

void UndoRecentChanges()
{
    Cube * p, * q;
    // get back cube q that was deleted
    q = GetFreeCube();
    // restore the ID
    q->ID = s_ChangeStore.PrevID;
    // insert the cube into storage again
    CubeInsert( q );

    // extract cube p
    p = CubeExtract( s_ChangeStore.p );

    // modify it back
    if ( s_ChangeStore.fInput ) // the input has changed
    {
        ExorVar( p, s_ChangeStore.Var, (varvalue)s_ChangeStore.Value );
        p->a = s_ChangeStore.PrevPa;
        p->q = s_ChangeStore.PrevPq;
        // p->z did not change
    }
    else // if ( s_ChangeStore.fInput ) // the output has changed
    {
        int i;
        for ( i = 0; i < g_CoverInfo.nWordsOut; i++ )
            p->pCubeDataOut[i] ^= q->pCubeDataOut[i];
        p->z = s_ChangeStore.PrevPz;
        // p->a did not change
    }
}

///////////////////////////////////////////////////////////////////
///               CUBE SET MANIPULATION PROCEDURES              ///
///////////////////////////////////////////////////////////////////

// Cube set is a list of cubes
//static Cube* s_List;

///////////////////////////////////////////////////////////////////
///                 Memory Allocation/Delocation                ///
///////////////////////////////////////////////////////////////////

int AllocateCubeSets( int nVarsIn, int nVarsOut )
{
    s_List = NULL;

    // clean other data
    s_fDistEnable2 = 1;
    s_fDistEnable3 = 0;
    s_fDistEnable4 = 0;
    memset( s_CubeGroup, 0, sizeof(void *) * 5 );
    memset( s_fInserted, 0, sizeof(int) * 5 );
    s_fDecreaseLiterals = 0;
    s_cEnquequed = 0;
    s_cAttempts = 0;
    s_cReshapes = 0;
    s_nCubesBefore = 0;
    s_Gain = 0;
    s_GainTotal = 0;
    s_GroupCounter = 0;
    s_GroupBest = 0;
    s_pC1 = s_pC2 = NULL;

    return 4;
}

void DelocateCubeSets()
{
}

///////////////////////////////////////////////////////////////////
///                     Insertion Operators                     ///
///////////////////////////////////////////////////////////////////

void CubeInsert( Cube* p )
// inserts the cube into storage (puts it at the beginning of the list)
{
    assert( p->Prev == NULL && p->Next == NULL );
    assert( p->ID );

    if ( s_List == NULL )
        s_List = p;
    else
    {
        p->Next = s_List;

        s_List->Prev = p;
        s_List = p;
    }

    g_CoverInfo.nCubesInUse++;
}

Cube* CubeExtract( Cube* p )
// extracts the cube from storage
{
//  assert( p->Prev && p->Next ); // can be done only with rings
    assert( p->ID );

//  if ( s_List == p )
//      s_List = p->Next;
//  if ( p->Prev )
//      p->Prev->Next = p->Next;

    if ( s_List == p )
        s_List = p->Next;
    else
        p->Prev->Next = p->Next;

    if ( p->Next )
        p->Next->Prev = p->Prev;

    p->Prev = NULL;
    p->Next = NULL;

    g_CoverInfo.nCubesInUse--;
    return p;
}

///////////////////////////////////////////////////////////////////
///                       CUBE ITERATOR                         ///
///////////////////////////////////////////////////////////////////

// the iterator starts from the Head and stops when it sees NULL
Cube* s_pCubeLast;

///////////////////////////////////////////////////////////////////
///                     Cube Set Iterator                       ///
///////////////////////////////////////////////////////////////////

Cube* IterCubeSetStart()
// starts an iterator that traverses all the cubes in the ring
{
    assert( s_pCubeLast == NULL );

    // check whether the List has cubes
    if ( s_List == NULL )
        return NULL;

    return ( s_pCubeLast = s_List );
}

Cube* IterCubeSetNext()
// returns the next cube in the cube set
// to use it again after it has returned NULL, first call IterCubeSetStart()
{
    assert( s_pCubeLast );
    return ( s_pCubeLast = s_pCubeLast->Next );
}

///////////////////////////////////////////////////////////////////
////                      ADJACENCY QUEQUES                  //////
///////////////////////////////////////////////////////////////////

typedef struct
{
    Cube** pC1;      // the pointer to the first cube
    Cube** pC2;      // the pointer to the second cube
    byte*  ID1;      // the ID of the first cube
    byte*  ID2;      // the ID of the second cube
    int  PosOut;     // extract position
    int  PosIn;      // insert position
    int  PosCur;     // temporary insert position
    int  PosMark;    // the marked position
    int  fEmpty;     // this flag is 1 if there is nothing in the queque
} que;

static que s_Que[3];  // Dist-2, Dist-3, Dist-4 queques

// the number of allocated places
//int s_nPosAlloc;
// the maximum number of occupied places
//int s_nPosMax[3];

//////////////////////////////////////////////////////////////////////
//            Conditional Adding Cube Pairs To Queques              //
//////////////////////////////////////////////////////////////////////

int GetPosDiff( int PosBeg, int PosEnd )
{
    return (PosEnd - PosBeg + s_nPosAlloc) % s_nPosAlloc;
}

void MarkSet()
// sets marks in the cube pair queques
{
    s_Que[0].PosMark = s_Que[0].PosIn;
    s_Que[1].PosMark = s_Que[1].PosIn;
    s_Que[2].PosMark = s_Que[2].PosIn;
}

void MarkRewind()
// rewinds the queques to the previously set marks
{
    s_Que[0].PosIn = s_Que[0].PosMark;
    s_Que[1].PosIn = s_Que[1].PosMark;
    s_Que[2].PosIn = s_Que[2].PosMark;
}

void NewRangeReset()
// resets temporarily stored new range of cube pairs
{
    s_Que[0].PosCur = s_Que[0].PosIn;
    s_Que[1].PosCur = s_Que[1].PosIn;
    s_Que[2].PosCur = s_Que[2].PosIn;
}

void NewRangeAdd()
// adds temporarily stored new range of cube pairs to the queque
{
    s_Que[0].PosIn = s_Que[0].PosCur;
    s_Que[1].PosIn = s_Que[1].PosCur;
    s_Que[2].PosIn = s_Que[2].PosCur;
}

void NewRangeInsertCubePair( cubedist Dist, Cube* p1, Cube* p2 )
// insert one cube pair into the new range
{
    que* p = &s_Que[Dist];
    int Pos = p->PosCur;

    if ( p->fEmpty || Pos != p->PosOut )
    {
        p->pC1[Pos] = p1;
        p->pC2[Pos] = p2;
        p->ID1[Pos] = p1->ID;
        p->ID2[Pos] = p2->ID;

        p->PosCur = (p->PosCur+1)%s_nPosAlloc;
    }
    else
        assert(0);
//      cout << endl << "DIST-" << (int)(Dist+2) << ": Have run out of queque space!" << endl;
}

void PrintQuequeStats()
{
/*
    cout << endl << "Queque statistics: ";
    cout << " Alloc = " << s_nPosAlloc;
    cout << "   DIST2 = " << GetPosDiff( s_Que[0].PosOut, s_Que[0].PosIn );
    cout << "   DIST3 = " << GetPosDiff( s_Que[1].PosOut, s_Que[1].PosIn );
    cout << "   DIST4 = " << GetPosDiff( s_Que[2].PosOut, s_Que[2].PosIn );
    cout << endl;
    cout << endl;
*/
}

int GetQuequeStats( cubedist Dist )
{
    return GetPosDiff( s_Que[Dist].PosOut, s_Que[Dist].PosIn );
}

//////////////////////////////////////////////////////////////////////
//                       Queque Iterators                           //
//////////////////////////////////////////////////////////////////////

// iterating through the queque (with authomatic garbage collection)
// only one iterator can be active at a time
static struct
{
    int fStarted;    // status of the iterator (1 if working)
    cubedist Dist;   // the currently iterated queque
    Cube** ppC1;     // the position where the first cube pointer goes
    Cube** ppC2;     // the position where the second cube pointer goes
    int PosStop;     // the stop position (to prevent the iterator from
                     // choking when new pairs are added during iteration)
    int CutValue;    // the number of literals below which the cubes are not used
} s_Iter;

static que* pQ;
static Cube *p1, *p2;

int IteratorCubePairStart( cubedist CubeDist, Cube** ppC1, Cube** ppC2 )
// start an iterator through cubes of dist CubeDist,
// the resulting pointers are written into ppC1 and ppC2
// returns 1 if the first cube pair is found
{
    int fEntryFound;

    assert( s_Iter.fStarted == 0 );
    assert( CubeDist >= 0 && CubeDist <= 2 );

    s_Iter.fStarted = 1;
    s_Iter.Dist = CubeDist;
    s_Iter.ppC1 = ppC1;
    s_Iter.ppC2 = ppC2;

    s_Iter.PosStop = s_Que[ CubeDist ].PosIn;

    // determine the cut value
//  s_Iter.CutValue = s_nLiteralsInUse/s_nCubesInUse/2;
    s_Iter.CutValue = -1;

    fEntryFound = 0;
    // go through the entries while there is something in the queque
    for ( pQ = &s_Que[ CubeDist ]; pQ->PosOut != s_Iter.PosStop; pQ->PosOut = (pQ->PosOut+1)%s_nPosAlloc )
    {
        p1 = pQ->pC1[ pQ->PosOut ];
        p2 = pQ->pC2[ pQ->PosOut ];

        // check whether the entry is valid
        if ( p1->ID == pQ->ID1[ pQ->PosOut ] && 
             p2->ID == pQ->ID2[ pQ->PosOut ] ) //&&
             //p1->x + p1->y + p2->x + p2->y > s_Iter.CutValue )
        {
             fEntryFound = 1;
             break;
        }
    }

    if ( fEntryFound )
    { // write the result into the pick-up place
        *ppC1 = pQ->pC1[ pQ->PosOut ];
        *ppC2 = pQ->pC2[ pQ->PosOut ];

        pQ->PosOut = (pQ->PosOut+1)%s_nPosAlloc;
    }
    else
        s_Iter.fStarted = 0;
    return fEntryFound;
}

int IteratorCubePairNext()
// gives the next VALID cube pair (the previous one is automatically dequequed)
{
    int fEntryFound = 0;
    assert( s_Iter.fStarted );

    // go through the entries while there is something in the queque
    for ( pQ = &s_Que[ s_Iter.Dist ]; pQ->PosOut != s_Iter.PosStop; pQ->PosOut = (pQ->PosOut+1)%s_nPosAlloc )
    {
        p1 = pQ->pC1[ pQ->PosOut ];
        p2 = pQ->pC2[ pQ->PosOut ];

        // check whether the entry is valid
        if ( p1->ID == pQ->ID1[ pQ->PosOut ] && 
             p2->ID == pQ->ID2[ pQ->PosOut ] ) //&&
             //p1->x + p1->y + p2->x + p2->y > s_Iter.CutValue )
        {
             fEntryFound = 1;
             break;
        }
    }

    if ( fEntryFound )
    { // write the result into the pick-up place
        *(s_Iter.ppC1) = pQ->pC1[ pQ->PosOut ];
        *(s_Iter.ppC2) = pQ->pC2[ pQ->PosOut ];

        pQ->PosOut = (pQ->PosOut+1)%s_nPosAlloc;
    }
    else // iteration has finished
        s_Iter.fStarted = 0;

    return fEntryFound;
}

//////////////////////////////////////////////////////////////////////
//                     Allocation/Delocation                        //
//////////////////////////////////////////////////////////////////////

int AllocateQueques( int nPlaces )
// nPlaces should be approximately nCubes*nCubes/10
// allocates memory for cube pair queques
{
    int i;
    s_nPosAlloc  = nPlaces;

    for ( i = 0; i < 3; i++ )
    {
        // clean data
        memset( &s_Que[i], 0, sizeof(que) );

        s_Que[i].pC1 = (Cube**) ABC_ALLOC( Cube*, nPlaces );
        s_Que[i].pC2 = (Cube**) ABC_ALLOC( Cube*, nPlaces );
        s_Que[i].ID1 = (byte*) ABC_ALLOC( byte, nPlaces );
        s_Que[i].ID2 = (byte*) ABC_ALLOC( byte, nPlaces );

        if ( s_Que[i].pC1==NULL || s_Que[i].pC2==NULL || s_Que[i].ID1==NULL || s_Que[i].ID2==NULL )
            return 0;

        s_nPosMax[i] = 0;
        s_Que[i].fEmpty = 1;
    }

    return nPlaces * (sizeof(Cube*) + sizeof(Cube*) + 2*sizeof(byte) );
}

void DelocateQueques()
{
    int i;
    for ( i = 0; i < 3; i++ )
    {
        ABC_FREE( s_Que[i].pC1 );
        ABC_FREE( s_Que[i].pC2 );
        ABC_FREE( s_Que[i].ID1 );
        ABC_FREE( s_Que[i].ID2 );
    }
}

///////////////////////////////////////////////////////////////////
////////////              End of File             /////////////////
///////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
