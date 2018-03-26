/**CFile****************************************************************

  FileName    [bm.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Boolean Matching package.]

  Synopsis    [Check P-equivalence and PP-equivalence of two circuits.]

  Author      [Hadi Katebi]
  
  Affiliation [University of Michigan]

  Date        [Ver. 1.0. Started - January, 2009.]

  Revision    [No revisions so far]

  Comments    [This is the cleaned up version of the code I used for DATE 2010 publication.]              
              [If you have any question or if you find a bug, contact me at hadik@umich.edu.]
              [I don't guarantee that I can fix all the bugs, but I can definitely point you to 
               the right direction so you can fix the bugs yourself].

  Debugging   [There are some part of the code that are commented out. Those parts mostly print
               the contents of the data structures to the standard output. Un-comment them if you 
               find them useful for debugging.]

***********************************************************************/

#include "base/abc/abc.h"
#include "opt/sim/sim.h"
#include "sat/bsat/satSolver.h"
//#include "bdd/extrab/extraBdd.h"

ABC_NAMESPACE_IMPL_START

#define FALSE 0
#define TRUE  1

int match1by1(Abc_Ntk_t * pNtk1, Vec_Ptr_t ** nodesInLevel1, Vec_Int_t ** iMatch1, Vec_Int_t ** iDep1, Vec_Int_t * matchedInputs1, int * iGroup1, Vec_Int_t ** oMatch1, int * oGroup1,
               Abc_Ntk_t * pNtk2, Vec_Ptr_t ** nodesInLevel2, Vec_Int_t ** iMatch2, Vec_Int_t ** iDep2, Vec_Int_t * matchedInputs2, int * iGroup2, Vec_Int_t ** oMatch2, int * oGroup2,
               Vec_Int_t * matchedOutputs1, Vec_Int_t * matchedOutputs2, Vec_Int_t * oMatchedGroups, Vec_Int_t * iNonSingleton, int ii, int idx);

int Abc_NtkBmSat( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, Vec_Ptr_t * iMatchPairs, Vec_Ptr_t * oMatchPairs, Vec_Int_t * mismatch, int mode);

void getDependencies(Abc_Ntk_t *pNtk, Vec_Int_t** iDep, Vec_Int_t** oDep)
{    
    Vec_Ptr_t * vSuppFun;
    int i, j;    
    
    vSuppFun = Sim_ComputeFunSupp(pNtk, 0);
    for(i = 0; i < Abc_NtkPoNum(pNtk); i++) {
        char * seg = (char *)vSuppFun->pArray[i];
        
        for(j = 0; j < Abc_NtkPiNum(pNtk); j+=8) {
            if(((*seg) & 0x01) == 0x01)
                Vec_IntPushOrder(oDep[i], j);
            if(((*seg) & 0x02) == 0x02)
                Vec_IntPushOrder(oDep[i], j+1);
            if(((*seg) & 0x04) == 0x04)
                Vec_IntPushOrder(oDep[i], j+2);
            if(((*seg) & 0x08) == 0x08)
                Vec_IntPushOrder(oDep[i], j+3);
            if(((*seg) & 0x10) == 0x10)
                Vec_IntPushOrder(oDep[i], j+4);
            if(((*seg) & 0x20) == 0x20)
                Vec_IntPushOrder(oDep[i], j+5);
            if(((*seg) & 0x40) == 0x40)
                Vec_IntPushOrder(oDep[i], j+6);
            if(((*seg) & 0x80) == 0x80)
                Vec_IntPushOrder(oDep[i], j+7);

            seg++;
        }
    }

    for(i = 0; i < Abc_NtkPoNum(pNtk); i++)
        for(j = 0; j < Vec_IntSize(oDep[i]); j++)
            Vec_IntPush(iDep[Vec_IntEntry(oDep[i], j)], i);    
    

    /*for(i = 0; i < Abc_NtkPoNum(pNtk); i++)
    {
        printf("Output %d: ", i);
        for(j = 0; j < Vec_IntSize(oDep[i]); j++)
            printf("%d ", Vec_IntEntry(oDep[i], j));
        printf("\n");
    }

    printf("\n");

    for(i = 0; i < Abc_NtkPiNum(pNtk); i++)
    {
        printf("Input %d: ", i);
        for(j = 0; j < Vec_IntSize(iDep[i]); j++)
            printf("%d ", Vec_IntEntry(iDep[i], j));
        printf("\n");
    }

    printf("\n");    */    
}

void initMatchList(Abc_Ntk_t *pNtk, Vec_Int_t** iDep, Vec_Int_t** oDep, Vec_Int_t** iMatch, int* iLastItem, Vec_Int_t** oMatch, int* oLastItem, int* iGroup, int* oGroup, int p_equivalence)
{
    int i, j, curr;
    Vec_Int_t** temp;

    if(!p_equivalence) {
        temp = ABC_ALLOC( Vec_Int_t*, Abc_NtkPiNum(pNtk)+1);

        for(i = 0; i < Abc_NtkPiNum(pNtk)+1; i++)    
            temp[i] = Vec_IntAlloc( 0 );    

        for(i = 0; i < Abc_NtkPoNum(pNtk); i++)        
            Vec_IntPush(temp[Vec_IntSize(oDep[i])], i);

        curr = 0;
        for(i = 0; i < Abc_NtkPiNum(pNtk)+1; i++)
        {
            if(Vec_IntSize(temp[i]) == 0)
                Vec_IntFree( temp[i] );

            else
            {
                oMatch[curr] = temp[i];
                
                for(j = 0; j < Vec_IntSize(temp[i]); j++)
                    oGroup[Vec_IntEntry(oMatch[curr], j)] = curr;
            
                curr++;
            }
        }

        *oLastItem = curr;

        ABC_FREE( temp );
    }
    else {
        // the else part fixes the outputs for P-equivalence checking
        for(i = 0; i < Abc_NtkPoNum(pNtk); i++)
        {
            Vec_IntPush(oMatch[i], i);
            oGroup[i] = i;
            (*oLastItem) = Abc_NtkPoNum(pNtk);
        }
    }
        
    /*for(j = 0; j < *oLastItem; j++)
    {
        printf("oMatch %d: ", j);
        for(i = 0; i < Vec_IntSize(oMatch[j]); i++)
            printf("%d ", Vec_IntEntry(oMatch[j], i));
        printf("\n");
    }

    for(i = 0; i < Abc_NtkPoNum(pNtk); i++)
        printf("%d: %d ", i, oGroup[i]);*/

    //////////////////////////////////////////////////////////////////////////////

    temp = ABC_ALLOC( Vec_Int_t*, Abc_NtkPoNum(pNtk)+1 );

    for(i = 0; i < Abc_NtkPoNum(pNtk)+1; i++)    
        temp[i] = Vec_IntAlloc( 0 );

    for(i = 0; i < Abc_NtkPiNum(pNtk); i++)        
        Vec_IntPush(temp[Vec_IntSize(iDep[i])], i);

    curr = 0;
    for(i = 0; i < Abc_NtkPoNum(pNtk)+1; i++)
    {
        if(Vec_IntSize(temp[i]) == 0)
            Vec_IntFree( temp[i] );
        else
        {
            iMatch[curr] = temp[i];
            for(j = 0; j < Vec_IntSize(iMatch[curr]); j++)
                iGroup[Vec_IntEntry(iMatch[curr], j)] = curr;
            curr++;
        }        
    }

    *iLastItem = curr;

    ABC_FREE( temp );        

    /*printf("\n");
    for(j = 0; j < *iLastItem; j++)
    {
        printf("iMatch %d: ", j);
        for(i = 0; i < Vec_IntSize(iMatch[j]); i++)
            printf("%d ", Vec_IntEntry(iMatch[j], i));
        printf("\n");
    }

    for(i = 0; i < Abc_NtkPiNum(pNtk); i++)
        printf("%d: %d ", i, iGroup[i]);
    printf("\n");*/
}

void iSortDependencies(Abc_Ntk_t *pNtk, Vec_Int_t** iDep, int* oGroup)
{
    int i, j, k;    
    Vec_Int_t * temp;    
    Vec_Int_t * oGroupList;    

    oGroupList = Vec_IntAlloc( 10 );

    for(i = 0; i < Abc_NtkPiNum(pNtk); i++)
    {
        if(Vec_IntSize(iDep[i]) == 1)
            continue;
        
        temp = Vec_IntAlloc( Vec_IntSize(iDep[i]) );        

        for(j = 0; j < Vec_IntSize(iDep[i]); j++)
            Vec_IntPushUniqueOrder(oGroupList, oGroup[Vec_IntEntry(iDep[i], j)]);

        for(j = 0; j < Vec_IntSize(oGroupList); j++)
        {
            for(k = 0; k < Vec_IntSize(iDep[i]); k++)
                if(oGroup[Vec_IntEntry(iDep[i], k)] == Vec_IntEntry(oGroupList, j))
                {
                    Vec_IntPush( temp, Vec_IntEntry(iDep[i], k) );        
                    Vec_IntRemove( iDep[i], Vec_IntEntry(iDep[i], k) );
                    k--;
                }
        }        

        Vec_IntFree( iDep[i] );        
        iDep[i] = temp;
        Vec_IntClear( oGroupList );

        /*printf("Input %d: ", i);
        for(j = 0; j < Vec_IntSize(iDep[i]); j++)
            printf("%d ", Vec_IntEntry(iDep[i], j));
        printf("\n");*/
    }
    
    Vec_IntFree( oGroupList );
}

void oSortDependencies(Abc_Ntk_t *pNtk, Vec_Int_t** oDep, int* iGroup)
{
    int i, j, k;    
    Vec_Int_t * temp;
    Vec_Int_t * iGroupList;    
    
    iGroupList = Vec_IntAlloc( 10 );

    for(i = 0; i < Abc_NtkPoNum(pNtk); i++)
    {
        if(Vec_IntSize(oDep[i]) == 1)
            continue;    
        
        temp = Vec_IntAlloc( Vec_IntSize(oDep[i]) );

        for(j = 0; j < Vec_IntSize(oDep[i]); j++)
            Vec_IntPushUniqueOrder(iGroupList, iGroup[Vec_IntEntry(oDep[i], j)]);        

        for(j = 0; j < Vec_IntSize(iGroupList); j++)
        {
            for(k = 0; k < Vec_IntSize(oDep[i]); k++)
                if(iGroup[Vec_IntEntry(oDep[i], k)] == Vec_IntEntry(iGroupList, j))
                {
                    Vec_IntPush( temp, Vec_IntEntry(oDep[i], k) );
                    Vec_IntRemove( oDep[i], Vec_IntEntry(oDep[i], k) );
                    k--;
                }
        }

        Vec_IntFree( oDep[i] );
        oDep[i] = temp;
        Vec_IntClear( iGroupList );

        /*printf("Output %d: ", i);
        for(j = 0; j < Vec_IntSize(oDep[i]); j++)
            printf("%d ", Vec_IntEntry(oDep[i], j));
        printf("\n");*/
    }
    
    Vec_IntFree( iGroupList );
}

int oSplitByDep(Abc_Ntk_t *pNtk, Vec_Int_t** oDep, Vec_Int_t** oMatch, int* oGroup, int* oLastItem, int* iGroup)
{
    int i, j, k;
    int numOfItemsAdded;
    Vec_Int_t * array, * sortedArray;    

    numOfItemsAdded = 0;

    for(i = 0; i < *oLastItem; i++)
    {
        if(Vec_IntSize(oMatch[i]) == 1)
            continue;

        array = Vec_IntAlloc( Vec_IntSize(oMatch[i]) );
        sortedArray = Vec_IntAlloc( Vec_IntSize(oMatch[i]) );

        for(j = 0; j < Vec_IntSize(oMatch[i]); j++)
        {
            int factor, encode;

            encode = 0;
            factor = 1;    
            
            for(k = 0; k < Vec_IntSize(oDep[Vec_IntEntry(oMatch[i], j)]); k++)                                        
                encode += iGroup[Vec_IntEntry(oDep[Vec_IntEntry(oMatch[i], j)], k)] * factor;                        
            
            Vec_IntPush(array, encode);
            Vec_IntPushUniqueOrder(sortedArray, encode);

            if( encode < 0)
                printf("WARNING! Integer overflow!");

            //printf("%d ", Vec_IntEntry(array, j));
        }                
        
        while( Vec_IntSize(sortedArray) > 1 )
        {            
            for(k = 0; k < Vec_IntSize(oMatch[i]); k++) 
            {
                if(Vec_IntEntry(array, k) == Vec_IntEntryLast(sortedArray))
                {
                    Vec_IntPush(oMatch[*oLastItem+numOfItemsAdded], Vec_IntEntry(oMatch[i], k));
                    oGroup[Vec_IntEntry(oMatch[i], k)] = *oLastItem+numOfItemsAdded;
                    Vec_IntRemove( oMatch[i], Vec_IntEntry(oMatch[i], k) );        
                    Vec_IntRemove( array, Vec_IntEntry(array, k) );            
                    k--;
                }    
            }            
            numOfItemsAdded++;
            Vec_IntPop(sortedArray);                        
        }

        Vec_IntFree( array );        
        Vec_IntFree( sortedArray );        
        //printf("\n");
    }    

    *oLastItem += numOfItemsAdded;

    /*printf("\n");
    for(j = 0; j < *oLastItem ; j++)
    {
        printf("oMatch %d: ", j);
        for(i = 0; i < Vec_IntSize(oMatch[j]); i++)
            printf("%d ", Vec_IntEntry(oMatch[j], i));
        printf("\n");
    }*/

    return numOfItemsAdded;
}

int iSplitByDep(Abc_Ntk_t *pNtk, Vec_Int_t** iDep, Vec_Int_t** iMatch, int* iGroup, int* iLastItem, int* oGroup)
{
    int i, j, k;    
    int numOfItemsAdded = 0;
    Vec_Int_t * array, * sortedArray;    

    for(i = 0; i < *iLastItem; i++)
    {
        if(Vec_IntSize(iMatch[i]) == 1)
            continue;
        
        array = Vec_IntAlloc( Vec_IntSize(iMatch[i]) );
        sortedArray = Vec_IntAlloc( Vec_IntSize(iMatch[i]) );

        for(j = 0; j < Vec_IntSize(iMatch[i]); j++)
        {        
            int factor, encode;

            encode = 0;
            factor = 1;    
            
            for(k = 0; k < Vec_IntSize(iDep[Vec_IntEntry(iMatch[i], j)]); k++)                            
                encode += oGroup[Vec_IntEntry(iDep[Vec_IntEntry(iMatch[i], j)], k)] * factor;                        

            Vec_IntPush(array, encode);
            Vec_IntPushUniqueOrder(sortedArray, encode);
            
            //printf("%d ", Vec_IntEntry(array, j));
        }            
                
        while( Vec_IntSize(sortedArray) > 1 )
        {            
            for(k = 0; k < Vec_IntSize(iMatch[i]); k++) 
            {
                if(Vec_IntEntry(array, k) == Vec_IntEntryLast(sortedArray))
                {
                    Vec_IntPush(iMatch[*iLastItem+numOfItemsAdded], Vec_IntEntry(iMatch[i], k));
                    iGroup[Vec_IntEntry(iMatch[i], k)] = *iLastItem+numOfItemsAdded;
                    Vec_IntRemove( iMatch[i], Vec_IntEntry(iMatch[i], k) );                            
                    Vec_IntRemove( array, Vec_IntEntry(array, k) );    
                    k--;
                }
            }
            numOfItemsAdded++;
            Vec_IntPop(sortedArray);    
        }

        Vec_IntFree( array );        
        Vec_IntFree( sortedArray );    
        //printf("\n");
    }    

    *iLastItem += numOfItemsAdded;

    /*printf("\n");
    for(j = 0; j < *iLastItem ; j++)
    {
        printf("iMatch %d: ", j);
        for(i = 0; i < Vec_IntSize(iMatch[j]); i++)
            printf("%d ", Vec_IntEntry(iMatch[j], i));
        printf("\n");
    }*/

    return numOfItemsAdded;    
}

Vec_Ptr_t ** findTopologicalOrder( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t ** vNodes;
    Abc_Obj_t * pObj, * pFanout;
    int i, k;    
    
    extern void Abc_NtkDfsReverse_rec( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes );
    
    // start the array of nodes
    vNodes = ABC_ALLOC(Vec_Ptr_t *, Abc_NtkPiNum(pNtk));
    for(i = 0; i < Abc_NtkPiNum(pNtk); i++)
        vNodes[i] = Vec_PtrAlloc(50);    
    
    Abc_NtkForEachCi( pNtk, pObj, i )
    {
        // set the traversal ID
        Abc_NtkIncrementTravId( pNtk );
        Abc_NodeSetTravIdCurrent( pObj );
        pObj = Abc_ObjFanout0Ntk(pObj);
        Abc_ObjForEachFanout( pObj, pFanout, k )
            Abc_NtkDfsReverse_rec( pFanout, vNodes[i] );
    }
   
    return vNodes;
}


int * Abc_NtkSimulateOneNode( Abc_Ntk_t * pNtk, int * pModel, int input, Vec_Ptr_t ** topOrder )
{
    Abc_Obj_t * pNode;
    Vec_Ptr_t * vNodes;
    int * pValues, Value0, Value1, i;
   
    vNodes = Vec_PtrAlloc( 50 );
/*
    printf( "Counter example: " );
    Abc_NtkForEachCi( pNtk, pNode, i )
        printf( " %d", pModel[i] );
    printf( "\n" );
*/
    // increment the trav ID
    Abc_NtkIncrementTravId( pNtk );
    // set the CI values     
    Abc_AigConst1(pNtk)->pCopy = (Abc_Obj_t *)1;
    pNode = Abc_NtkCi(pNtk, input);
    pNode->iTemp = pModel[input];
    
    // simulate in the topological order    
    for(i = Vec_PtrSize(topOrder[input])-1; i >= 0; i--)
    {
        pNode = (Abc_Obj_t *)Vec_PtrEntry(topOrder[input], i);        
        
        Value0 = ((int)(ABC_PTRUINT_T)Abc_ObjFanin0(pNode)->pCopy) ^ Abc_ObjFaninC0(pNode);
        Value1 = ((int)(ABC_PTRUINT_T)Abc_ObjFanin1(pNode)->pCopy) ^ Abc_ObjFaninC1(pNode);
        
        if( pNode->iTemp != (Value0 & Value1))
        {
            pNode->iTemp = (Value0 & Value1);
            Vec_PtrPush(vNodes, pNode);
        }
    
    }    
    // fill the output values
    pValues = ABC_ALLOC( int, Abc_NtkCoNum(pNtk) );
    Abc_NtkForEachCo( pNtk, pNode, i )
        pValues[i] = ((int)(ABC_PTRUINT_T)Abc_ObjFanin0(pNode)->pCopy) ^ Abc_ObjFaninC0(pNode);
    
    pNode = Abc_NtkCi(pNtk, input);
    if(pNode->pCopy == (Abc_Obj_t *)1)
        pNode->pCopy = (Abc_Obj_t *)0;
    else
        pNode->pCopy = (Abc_Obj_t *)1;

    for(i = 0; i < Vec_PtrSize(vNodes); i++)
    {
        pNode = (Abc_Obj_t *)Vec_PtrEntry(vNodes, i);

        if(pNode->pCopy == (Abc_Obj_t *)1)
            pNode->pCopy = (Abc_Obj_t *)0;
        else
            pNode->pCopy = (Abc_Obj_t *)1;
    }

    Vec_PtrFree( vNodes );

    return pValues;
}

int refineIOBySimulation(Abc_Ntk_t *pNtk, Vec_Int_t** iMatch, int* iLastItem, int * iGroup, Vec_Int_t** iDep, Vec_Int_t** oMatch, int* oLastItem, int * oGroup, Vec_Int_t** oDep, char * vPiValues, int * observability, Vec_Ptr_t ** topOrder)
{
    Abc_Obj_t * pObj;
    int * pModel;//, ** pModel2;
    int * output, * output2;
    int lastItem;
    int i, j, k;
    Vec_Int_t * iComputedNum, * iComputedNumSorted;    
    Vec_Int_t * oComputedNum;                // encoding the number of flips        
    int factor;    
    int isRefined = FALSE;    

    pModel = ABC_ALLOC( int, Abc_NtkCiNum(pNtk) );    

    Abc_NtkForEachPi( pNtk, pObj, i )
        pModel[i] = vPiValues[i] - '0';
    Abc_NtkForEachLatch( pNtk, pObj, i )
        pModel[Abc_NtkPiNum(pNtk)+i] = pObj->iData - 1;

    output = Abc_NtkVerifySimulatePattern( pNtk, pModel );    

    oComputedNum = Vec_IntAlloc( Abc_NtkPoNum(pNtk) );
    for(i = 0; i < Abc_NtkPoNum(pNtk); i++)
        Vec_IntPush(oComputedNum, 0);
    
    /****************************************************************************************/
    /********** group outputs that produce 1 and outputs that produce 0 together ************/    
    
    lastItem = *oLastItem;
    for(i = 0; i < lastItem && (*oLastItem) != Abc_NtkPoNum(pNtk); i++)
    {
        int flag = FALSE;

        if(Vec_IntSize(oMatch[i]) == 1)
            continue;

        for(j = 1; j < Vec_IntSize(oMatch[i]); j++)
            if(output[Vec_IntEntry(oMatch[i], 0)] != output[Vec_IntEntry(oMatch[i], j)])
            {
                flag = TRUE;
                break;
            }

        if(flag)
        {
            for(j = 0; j < Vec_IntSize(oMatch[i]); j++)
                if(output[Vec_IntEntry(oMatch[i], j)])
                {
                    Vec_IntPush(oMatch[*oLastItem], Vec_IntEntry(oMatch[i], j));
                    oGroup[Vec_IntEntry(oMatch[i], j)] = *oLastItem;    
                    Vec_IntRemove(oMatch[i], Vec_IntEntry(oMatch[i], j));    
                    j--;
                }

            (*oLastItem)++;
        }
    }

    if( (*oLastItem) > lastItem )
    {
        isRefined = TRUE;
        iSortDependencies(pNtk, iDep, oGroup);    
    }

    /****************************************************************************************/
    /************* group inputs that make the same number of flips in outpus ****************/        

    lastItem = *iLastItem;
    for(i = 0; i < lastItem && (*iLastItem) != Abc_NtkPiNum(pNtk); i++)
    {
        int num;

        if(Vec_IntSize(iMatch[i]) == 1)
            continue;

        iComputedNum = Vec_IntAlloc( Vec_IntSize(iMatch[i]) );
        iComputedNumSorted = Vec_IntAlloc( Vec_IntSize(iMatch[i]) );    
        
        for(j = 0; j < Vec_IntSize(iMatch[i]); j++)
        {
            if( vPiValues[Vec_IntEntry(iMatch[i], j)] == '0' )                
                pModel[Vec_IntEntry(iMatch[i], j)] = 1;
            else                 
                pModel[Vec_IntEntry(iMatch[i], j)] = 0;        
            
            //output2 = Abc_NtkVerifySimulatePattern( pNtk, pModel );    
            output2 = Abc_NtkSimulateOneNode( pNtk, pModel, Vec_IntEntry(iMatch[i], j), topOrder );
            
            num = 0;
            factor = 1;    
            for(k = 0; k < Vec_IntSize(iDep[Vec_IntEntry(iMatch[i], j)]); k++)
            {
                int outputIndex = Vec_IntEntry(iDep[Vec_IntEntry(iMatch[i], j)], k);        

                if(output2[outputIndex])
                    num += (oGroup[outputIndex] + 1) * factor;                
            
                if(output[outputIndex] != output2[outputIndex])
                {
                    int temp = Vec_IntEntry(oComputedNum, outputIndex) + i + 1;
                    Vec_IntWriteEntry(oComputedNum, outputIndex, temp);    
                    observability[Vec_IntEntry(iMatch[i], j)]++;
                }
            }

            Vec_IntPush(iComputedNum, num);
            Vec_IntPushUniqueOrder(iComputedNumSorted, num);            

            pModel[Vec_IntEntry(iMatch[i], j)] = vPiValues[Vec_IntEntry(iMatch[i], j)] - '0';
            ABC_FREE( output2 );
        }    

        while( Vec_IntSize( iComputedNumSorted ) > 1 )
        {
            for(k = 0; k < Vec_IntSize(iMatch[i]); k++)
            {                            
                if(Vec_IntEntry(iComputedNum, k) == Vec_IntEntryLast(iComputedNumSorted) )
                {
                    Vec_IntPush(iMatch[*iLastItem], Vec_IntEntry(iMatch[i], k));
                    iGroup[Vec_IntEntry(iMatch[i], k)] = *iLastItem;
                    Vec_IntRemove( iMatch[i], Vec_IntEntry(iMatch[i], k) );
                    Vec_IntRemove( iComputedNum, Vec_IntEntry(iComputedNum, k) );        
                    k--;
                }                
            }
            (*iLastItem)++;    
            Vec_IntPop( iComputedNumSorted );
        }    

        Vec_IntFree( iComputedNum );
        Vec_IntFree( iComputedNumSorted );
    }

    if( (*iLastItem) > lastItem )
    {
        isRefined = TRUE;
        oSortDependencies(pNtk, oDep, iGroup);
    }

    /****************************************************************************************/
    /********** encode the number of flips in each output by flipping the outputs ***********/
    /********** and group all the outputs that have the same encoding         ***********/    

    lastItem = *oLastItem;
    for(i = 0; i < lastItem && (*oLastItem) != Abc_NtkPoNum(pNtk); i++)
    {
        Vec_Int_t * encode, * sortedEncode;                // encoding the number of flips                    

        if(Vec_IntSize(oMatch[i]) == 1)
            continue;
            
        encode = Vec_IntAlloc( Vec_IntSize(oMatch[i]) );
        sortedEncode = Vec_IntAlloc( Vec_IntSize(oMatch[i]) );                    
    
        for(j = 0; j < Vec_IntSize(oMatch[i]); j++)
        {
            Vec_IntPush(encode, Vec_IntEntry(oComputedNum, Vec_IntEntry(oMatch[i], j)) );
            Vec_IntPushUniqueOrder( sortedEncode, Vec_IntEntry(encode, j) );
        }

        while( Vec_IntSize(sortedEncode) > 1 )
        {
            for(j = 0; j < Vec_IntSize(oMatch[i]); j++)
                if(Vec_IntEntry(encode, j) == Vec_IntEntryLast(sortedEncode))
                {
                    Vec_IntPush(oMatch[*oLastItem], Vec_IntEntry(oMatch[i], j));
                    oGroup[Vec_IntEntry(oMatch[i], j)] = *oLastItem;
                    Vec_IntRemove( oMatch[i], Vec_IntEntry(oMatch[i], j) );
                    Vec_IntRemove( encode, Vec_IntEntry(encode, j) );
                    j --;
                }                
            
            (*oLastItem)++;
            Vec_IntPop( sortedEncode );
        }

        Vec_IntFree( encode );
        Vec_IntFree( sortedEncode );
    }

    if( (*oLastItem) > lastItem )
        isRefined = TRUE;    

    ABC_FREE( pModel );
    ABC_FREE( output );
    Vec_IntFree( oComputedNum );

    return isRefined;    
}

Abc_Ntk_t * Abc_NtkMiterBm( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, Vec_Ptr_t * iCurrMatch, Vec_Ptr_t * oCurrMatch )
{
    char Buffer[1000];
    Abc_Ntk_t * pNtkMiter;

    pNtkMiter = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );
    sprintf( Buffer, "%s_%s_miter", pNtk1->pName, pNtk2->pName );
    pNtkMiter->pName = Extra_UtilStrsav(Buffer);

    //Abc_NtkMiterPrepare( pNtk1, pNtk2, pNtkMiter, fComb, nPartSize );
    {
        Abc_Obj_t * pObj, * pObjNew;
        int i;

        Abc_AigConst1(pNtk1)->pCopy = Abc_AigConst1(pNtkMiter);
        Abc_AigConst1(pNtk2)->pCopy = Abc_AigConst1(pNtkMiter);

        // create new PIs and remember them in the old PIs
        if(iCurrMatch == NULL)
        {
            Abc_NtkForEachCi( pNtk1, pObj, i )
            {
                pObjNew = Abc_NtkCreatePi( pNtkMiter );
                // remember this PI in the old PIs
                pObj->pCopy = pObjNew;
                pObj = Abc_NtkCi(pNtk2, i);  
                pObj->pCopy = pObjNew;
                // add name
                Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), NULL );
            }
        }
        else
        {
            for(i = 0; i < Vec_PtrSize( iCurrMatch ); i += 2)
            {
                pObjNew = Abc_NtkCreatePi( pNtkMiter );
                pObj = (Abc_Obj_t *)Vec_PtrEntry(iCurrMatch, i);
                pObj->pCopy = pObjNew;
                pObj = (Abc_Obj_t *)Vec_PtrEntry(iCurrMatch, i+1);
                pObj->pCopy = pObjNew;
                // add name
                Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), NULL );
            }
        }

        // create the only PO
        pObjNew = Abc_NtkCreatePo( pNtkMiter );
        // add the PO name
        Abc_ObjAssignName( pObjNew, "miter", NULL );
    }

    // Abc_NtkMiterAddOne( pNtk1, pNtkMiter );
    {
        Abc_Obj_t * pNode;
        int i;
        assert( Abc_NtkIsDfsOrdered(pNtk1) );
        Abc_AigForEachAnd( pNtk1, pNode, i )
            pNode->pCopy = Abc_AigAnd( (Abc_Aig_t *)pNtkMiter->pManFunc, Abc_ObjChild0Copy(pNode), Abc_ObjChild1Copy(pNode) );
    }

    // Abc_NtkMiterAddOne( pNtk2, pNtkMiter );
    {
        Abc_Obj_t * pNode;
        int i;
        assert( Abc_NtkIsDfsOrdered(pNtk2) );
        Abc_AigForEachAnd( pNtk2, pNode, i )
            pNode->pCopy = Abc_AigAnd( (Abc_Aig_t *)pNtkMiter->pManFunc, Abc_ObjChild0Copy(pNode), Abc_ObjChild1Copy(pNode) );
    }
    
    // Abc_NtkMiterFinalize( pNtk1, pNtk2, pNtkMiter, fComb, nPartSize );    
    {
        Vec_Ptr_t * vPairs;
        Abc_Obj_t * pMiter;
        int i;

        vPairs = Vec_PtrAlloc( 100 );
        
        // collect the CO nodes for the miter
        if(oCurrMatch != NULL)
        {
            for(i = 0; i < Vec_PtrSize( oCurrMatch ); i += 2)
            {
                Vec_PtrPush( vPairs, Abc_ObjChild0Copy((Abc_Obj_t *)Vec_PtrEntry(oCurrMatch, i)) );
                Vec_PtrPush( vPairs, Abc_ObjChild0Copy((Abc_Obj_t *)Vec_PtrEntry(oCurrMatch, i+1)) );
            }
        }
        else
        {
            Abc_Obj_t * pNode;

            Abc_NtkForEachCo( pNtk1, pNode, i )
            {
                Vec_PtrPush( vPairs, Abc_ObjChild0Copy(pNode) );
                pNode = Abc_NtkCo( pNtk2, i );
                Vec_PtrPush( vPairs, Abc_ObjChild0Copy(pNode) );
            }
        }
        
     pMiter = Abc_AigMiter( (Abc_Aig_t *)pNtkMiter->pManFunc, vPairs, 0 );
     Abc_ObjAddFanin( Abc_NtkPo(pNtkMiter,0), pMiter );
     Vec_PtrFree(vPairs);
    }

    //Abc_AigCleanup(pNtkMiter->pManFunc);
    
    return pNtkMiter;
}

int * pValues1__, * pValues2__;

void Abc_NtkVerifyReportError( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int * pModel, Vec_Int_t * mismatch )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pNode;    
    int nErrors, nPrinted, i, iNode = -1;

    assert( Abc_NtkCiNum(pNtk1) == Abc_NtkCiNum(pNtk2) );
    assert( Abc_NtkCoNum(pNtk1) == Abc_NtkCoNum(pNtk2) );
    // get the CO values under this model
    pValues1__ = Abc_NtkVerifySimulatePattern( pNtk1, pModel );
    pValues2__ = Abc_NtkVerifySimulatePattern( pNtk2, pModel );
    // count the mismatches
    nErrors = 0;
    for ( i = 0; i < Abc_NtkCoNum(pNtk1); i++ )
        nErrors += (int)( pValues1__[i] != pValues2__[i] );
    //printf( "Verification failed for at least %d outputs: ", nErrors );
    // print the first 3 outputs
    nPrinted = 0;
    for ( i = 0; i < Abc_NtkCoNum(pNtk1); i++ )
        if ( pValues1__[i] != pValues2__[i] )
        {
            if ( iNode == -1 )
                iNode = i;
            //printf( " %s", Abc_ObjName(Abc_NtkCo(pNtk1,i)) );
            if ( ++nPrinted == 3 )
                break;
        }
    /*if ( nPrinted != nErrors )
        printf( " ..." );
    printf( "\n" );*/
    // report mismatch for the first output
    if ( iNode >= 0 )
    {
        /*printf( "Output %s: Value in Network1 = %d. Value in Network2 = %d.\n", 
            Abc_ObjName(Abc_NtkCo(pNtk1,iNode)), pValues1[iNode], pValues2[iNode] );
        printf( "Input pattern: " );*/
        // collect PIs in the cone
        pNode = Abc_NtkCo(pNtk1,iNode);
        vNodes = Abc_NtkNodeSupport( pNtk1, &pNode, 1 );
        // set the PI numbers
        Abc_NtkForEachCi( pNtk1, pNode, i )
            pNode->iTemp = i;
        // print the model
        pNode = (Abc_Obj_t *)Vec_PtrEntry( vNodes, 0 );
        if ( Abc_ObjIsCi(pNode) )
        {
            Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
            {
                assert( Abc_ObjIsCi(pNode) );
                //printf( " %s=%d", Abc_ObjName(pNode), pModel[(int)pNode->pCopy] );
                Vec_IntPush(mismatch, Abc_ObjId(pNode)-1);
                Vec_IntPush(mismatch, pModel[(int)(size_t)pNode->pCopy]);
            }
        }
        //printf( "\n" );
        Vec_PtrFree( vNodes );
    }
    free( pValues1__ );
    free( pValues2__ );
}

int Abc_NtkMiterSatBm( Abc_Ntk_t * pNtk, ABC_INT64_T nConfLimit, ABC_INT64_T nInsLimit, int fVerbose, ABC_INT64_T * pNumConfs, ABC_INT64_T * pNumInspects)
{
    static sat_solver * pSat = NULL;
    lbool   status;
    int RetValue = 0;
    abctime clk;    

    extern int Abc_NodeAddClausesTop( sat_solver * pSat, Abc_Obj_t * pNode, Vec_Int_t * vVars );
    extern Vec_Int_t * Abc_NtkGetCiSatVarNums( Abc_Ntk_t * pNtk );    
 
    if ( pNumConfs )
        *pNumConfs = 0;
    if ( pNumInspects )
        *pNumInspects = 0;

    assert( Abc_NtkLatchNum(pNtk) == 0 );

//    if ( Abc_NtkPoNum(pNtk) > 1 )
//        fprintf( stdout, "Warning: The miter has %d outputs. SAT will try to prove all of them.\n", Abc_NtkPoNum(pNtk) );

    // load clauses into the sat_solver
    clk = Abc_Clock();

            
        
    pSat = (sat_solver *)Abc_NtkMiterSatCreate( pNtk, 0 );
            
    if ( pSat == NULL )
        return 1;
//printf( "%d \n", pSat->clauses.size );
//sat_solver_delete( pSat );
//return 1;

//    printf( "Created SAT problem with %d variable and %d clauses. ", sat_solver_nvars(pSat), sat_solver_nclauses(pSat) );
//    PRT( "Time", Abc_Clock() - clk );

    // simplify the problem
    clk = Abc_Clock();
    status = sat_solver_simplify(pSat);
//    printf( "Simplified the problem to %d variables and %d clauses. ", sat_solver_nvars(pSat), sat_solver_nclauses(pSat) );
//    PRT( "Time", Abc_Clock() - clk );
    if ( status == 0 )
    {
        sat_solver_delete( pSat );
//        printf( "The problem is UNSATISFIABLE after simplification.\n" );
        return 1;
    }

    // solve the miter
    clk = Abc_Clock();
    if ( fVerbose )
        pSat->verbosity = 1;
    status = sat_solver_solve( pSat, NULL, NULL, (ABC_INT64_T)nConfLimit, (ABC_INT64_T)nInsLimit, (ABC_INT64_T)0, (ABC_INT64_T)0 );
    if ( status == l_Undef )
    {
//        printf( "The problem timed out.\n" );
        RetValue = -1;
    }
    else if ( status == l_True )
    {
//        printf( "The problem is SATISFIABLE.\n" );
        RetValue = 0;
    }
    else if ( status == l_False )
    {
//        printf( "The problem is UNSATISFIABLE.\n" );
        RetValue = 1;
    }
    else
        assert( 0 );
//    PRT( "SAT sat_solver time", Abc_Clock() - clk );
//    printf( "The number of conflicts = %d.\n", (int)pSat->sat_solver_stats.conflicts );

    // if the problem is SAT, get the counterexample
    if ( status == l_True )
    {
//        Vec_Int_t * vCiIds = Abc_NtkGetCiIds( pNtk );
        Vec_Int_t * vCiIds = Abc_NtkGetCiSatVarNums( pNtk );
        pNtk->pModel = Sat_SolverGetModel( pSat, vCiIds->pArray, vCiIds->nSize );
        Vec_IntFree( vCiIds );
    }
    // free the sat_solver
    if ( fVerbose )
        Sat_SolverPrintStats( stdout, pSat );

    if ( pNumConfs )
        *pNumConfs = (int)pSat->stats.conflicts;
    if ( pNumInspects )
        *pNumInspects = (int)pSat->stats.inspects;

//sat_solver_store_write( pSat, "trace.cnf" );    
    sat_solver_store_free( pSat );
    sat_solver_delete( pSat );
    return RetValue;
}

int Abc_NtkBmSat( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, Vec_Ptr_t * iMatchPairs, Vec_Ptr_t * oMatchPairs, Vec_Int_t * mismatch, int mode)
{    
    extern Abc_Ntk_t * Abc_NtkMulti( Abc_Ntk_t * pNtk, int nThresh, int nFaninMax, int fCnf, int fMulti, int fSimple, int fFactor );    

    Abc_Ntk_t * pMiter = NULL;
    Abc_Ntk_t * pCnf;
    int RetValue = 0;

    // get the miter of the two networks
    if( mode == 0 )
    {
        //Abc_NtkDelete( pMiter );
        pMiter = Abc_NtkMiterBm( pNtk1, pNtk2, iMatchPairs, oMatchPairs );
    }
    else if( mode == 1 )        // add new outputs
    {        
        int i;
        Abc_Obj_t * pObj;
        Vec_Ptr_t * vPairs;
        Abc_Obj_t * pNtkMiter;        

        vPairs = Vec_PtrAlloc( 100 );

        Abc_NtkForEachCo( pMiter, pObj, i )            
            Abc_ObjRemoveFanins( pObj );

        for(i = 0; i < Vec_PtrSize( oMatchPairs ); i += 2)
        {
            Vec_PtrPush( vPairs, Abc_ObjChild0Copy((Abc_Obj_t *)Vec_PtrEntry(oMatchPairs, i)) );
            Vec_PtrPush( vPairs, Abc_ObjChild0Copy((Abc_Obj_t *)Vec_PtrEntry(oMatchPairs, i+1)) );
        }
        pNtkMiter = Abc_AigMiter( (Abc_Aig_t *)pMiter->pManFunc, vPairs, 0 );
        Abc_ObjAddFanin( Abc_NtkPo(pMiter,0), pNtkMiter );     
        Vec_PtrFree( vPairs);
    }
    else if( mode == 2 )        // add some outputs
    {

    }
    else if( mode == 3)            // remove all outputs
    {
    }

    if ( pMiter == NULL )
    {
        printf("Miter computation has failed.");        
        return -1;
    }
    RetValue = Abc_NtkMiterIsConstant( pMiter );
    if ( RetValue == 0)
    {        
        /*printf("Networks are NOT EQUIVALENT after structural hashing.");    */
        // report the error        
        if(mismatch != NULL)
        {
            pMiter->pModel = Abc_NtkVerifyGetCleanModel( pMiter, 1 );
            Abc_NtkVerifyReportError( pNtk1, pNtk2, pMiter->pModel, mismatch );
            ABC_FREE( pMiter->pModel );
        }        
        Abc_NtkDelete( pMiter );
        return RetValue;
    }
    if( RetValue == 1 )
    {        
        /*printf("Networks are equivalent after structural hashing.");    */
        Abc_NtkDelete( pMiter );
        return RetValue;
    }

    // convert the miter into a CNF
    //if(mode == 0)
    pCnf = Abc_NtkMulti( pMiter, 0, 100, 1, 0, 0, 0 );
    Abc_NtkDelete( pMiter );
    if ( pCnf == NULL )
    {        
        printf("Renoding for CNF has failed.");    
        return -1;
    }

    // solve the CNF using the SAT solver
    RetValue = Abc_NtkMiterSat( pCnf, (ABC_INT64_T)10000, (ABC_INT64_T)0, 0, NULL, NULL);
    /*if ( RetValue == -1 )
        printf("Networks are undecided (SAT solver timed out).");    
    else if ( RetValue == 0 )        
        printf("Networks are NOT EQUIVALENT after SAT.");    
    else
        printf("Networks are equivalent after SAT.");    */
    if ( mismatch != NULL && pCnf->pModel )
        Abc_NtkVerifyReportError( pNtk1, pNtk2, pCnf->pModel, mismatch );

    ABC_FREE( pCnf->pModel );
    Abc_NtkDelete( pCnf );

    return RetValue;
}

int checkEquivalence( Abc_Ntk_t * pNtk1, Vec_Int_t* matchedInputs1, Vec_Int_t * matchedOutputs1,
                       Abc_Ntk_t * pNtk2, Vec_Int_t* matchedInputs2, Vec_Int_t * matchedOutputs2)
{    
    Vec_Ptr_t * iMatchPairs, * oMatchPairs;
    int i;
    int result;

    iMatchPairs = Vec_PtrAlloc( Abc_NtkPiNum( pNtk1 ) * 2);
    oMatchPairs = Vec_PtrAlloc( Abc_NtkPoNum( pNtk1 ) * 2);

    for(i = 0; i < Abc_NtkPiNum(pNtk1); i++)
    {    
        Vec_PtrPush(iMatchPairs, Abc_NtkPi(pNtk2, Vec_IntEntry(matchedInputs2, i)));
        Vec_PtrPush(iMatchPairs, Abc_NtkPi(pNtk1, Vec_IntEntry(matchedInputs1, i)));
    }
                

    for(i = 0; i < Abc_NtkPoNum(pNtk1); i++)        
    {
        Vec_PtrPush(oMatchPairs, Abc_NtkPo(pNtk2, Vec_IntEntry(matchedOutputs2, i)));
        Vec_PtrPush(oMatchPairs, Abc_NtkPo(pNtk1, Vec_IntEntry(matchedOutputs1, i)));
    }        

    result = Abc_NtkBmSat(pNtk1, pNtk2, iMatchPairs, oMatchPairs, NULL, 0);

    if( result )
        printf("*** Circuits are equivalent ***\n");
    else
        printf("*** Circuits are NOT equivalent ***\n");

    Vec_PtrFree( iMatchPairs );
    Vec_PtrFree( oMatchPairs );    

    return result;
}

Abc_Ntk_t * computeCofactor(Abc_Ntk_t * pNtk, Vec_Ptr_t ** nodesInLevel, int * bitVector, Vec_Int_t * currInputs)
{    
    Abc_Ntk_t * subNtk;    
    Abc_Obj_t * pObj, * pObjNew;
    int i, j, numOfLevels;        

    numOfLevels = Abc_AigLevel( pNtk );        // number of levels excludes PI/POs    

    // start a new network
    subNtk = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );    
    subNtk->pName = Extra_UtilStrsav("subNtk");

    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(subNtk);    

    // clean the node copy fields and mark the nodes that need to be copied to the new network
    Abc_NtkCleanCopy( pNtk );

    if(bitVector != NULL)
    {
        for(i = 0; i < Abc_NtkPiNum(pNtk); i++)
            if(bitVector[i])
            {
                pObj = Abc_NtkPi(pNtk, i);
                pObj->pCopy = (Abc_Obj_t *)(1);
            }
    }

    for(i = 0; i < Vec_IntSize(currInputs); i++)
    {
        pObj = Abc_NtkPi(pNtk, Vec_IntEntry(currInputs, i));
        pObjNew = Abc_NtkDupObj( subNtk, pObj, 1 );
        pObj->pCopy = pObjNew;
    }
        

    // i = 0 are the inputs and the inputs are not added to the 2d array ( nodesInLevel )
    for( i = 0; i <= numOfLevels; i++ )    
        for( j = 0; j < Vec_PtrSize( nodesInLevel[i] ); j++)
        {
            pObj = (Abc_Obj_t *)Vec_PtrEntry( nodesInLevel[i], j );

            if(Abc_ObjChild0Copy(pObj) == NULL && Abc_ObjChild1Copy(pObj) == NULL)
                pObj->pCopy = NULL;
            else if(Abc_ObjChild0Copy(pObj) == NULL && Abc_ObjChild1Copy(pObj) == (void*)(1))
                pObj->pCopy = NULL;
            else if(Abc_ObjChild0Copy(pObj) == NULL && (Abc_ObjChild1Copy(pObj) != (NULL) && Abc_ObjChild1Copy(pObj) != (void*)(1)) )
                pObj->pCopy = NULL;
            else if(Abc_ObjChild0Copy(pObj) == (void*)(1) && Abc_ObjChild1Copy(pObj) == NULL)
                pObj->pCopy = NULL;
            else if(Abc_ObjChild0Copy(pObj) == (void*)(1) && Abc_ObjChild1Copy(pObj) == (void*)(1))
                pObj->pCopy = (Abc_Obj_t *)(1);
            else if(Abc_ObjChild0Copy(pObj) == (void*)(1) && (Abc_ObjChild1Copy(pObj) != (NULL) && Abc_ObjChild1Copy(pObj) != (void*)(1)) )
                pObj->pCopy = Abc_ObjChild1Copy(pObj);
            else if( (Abc_ObjChild0Copy(pObj) != (NULL) && Abc_ObjChild0Copy(pObj) != (void*)(1)) && Abc_ObjChild1Copy(pObj) == NULL )
                pObj->pCopy = NULL;
            else if( (Abc_ObjChild0Copy(pObj) != (NULL) && Abc_ObjChild0Copy(pObj) != (void*)(1)) && Abc_ObjChild1Copy(pObj) == (void*)(1) )
                pObj->pCopy = Abc_ObjChild0Copy(pObj);
            else if( (Abc_ObjChild0Copy(pObj) != (NULL) && Abc_ObjChild0Copy(pObj) != (void*)(1)) &&
                     (Abc_ObjChild1Copy(pObj) != (NULL) && Abc_ObjChild1Copy(pObj) != (void*)(1)) )
                pObj->pCopy = Abc_AigAnd( (Abc_Aig_t *)subNtk->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj) );                    
        }    

    for(i = 0; i < Abc_NtkPoNum(pNtk); i++)
    {
        pObj = Abc_NtkPo(pNtk, i);
        pObjNew = Abc_NtkDupObj( subNtk, pObj, 1 );
        
        if( Abc_ObjChild0Copy(pObj) == NULL)
        {
            Abc_ObjAddFanin( pObjNew, Abc_AigConst1(subNtk));
            pObjNew->fCompl0 = 1;
        }
        else if( Abc_ObjChild0Copy(pObj) == (void*)(1) )
        {
            Abc_ObjAddFanin( pObjNew, Abc_AigConst1(subNtk));
            pObjNew->fCompl0 = 0;
        }
        else
            Abc_ObjAddFanin( pObjNew, Abc_ObjChild0Copy(pObj) );
    }

    return subNtk;
}

FILE *matchFile;

int matchNonSingletonOutputs(Abc_Ntk_t * pNtk1, Vec_Ptr_t ** nodesInLevel1, Vec_Int_t ** iMatch1, Vec_Int_t ** iDep1, Vec_Int_t * matchedInputs1, int * iGroup1, Vec_Int_t ** oMatch1, int * oGroup1,
                              Abc_Ntk_t * pNtk2, Vec_Ptr_t ** nodesInLevel2, Vec_Int_t ** iMatch2, Vec_Int_t ** iDep2, Vec_Int_t * matchedInputs2, int * iGroup2, Vec_Int_t ** oMatch2, int * oGroup2,
                              Vec_Int_t * matchedOutputs1, Vec_Int_t * matchedOutputs2, Vec_Int_t * oMatchedGroups, Vec_Int_t * iNonSingleton,                         
                              Abc_Ntk_t * subNtk1, Abc_Ntk_t * subNtk2, Vec_Ptr_t * oMatchPairs,
                              Vec_Int_t * oNonSingleton, int oI, int idx, int ii, int iidx)
{        
    static int MATCH_FOUND;
    int i;
    int j, temp;
    Vec_Int_t * mismatch;        
    int * skipList;    
    static int counter = 0;

    MATCH_FOUND = FALSE;
    
    if( oI == Vec_IntSize( oNonSingleton ) )
    {
        if( iNonSingleton != NULL)            
            if( match1by1(pNtk1, nodesInLevel1, iMatch1, iDep1, matchedInputs1, iGroup1, oMatch1, oGroup1,
                          pNtk2, nodesInLevel2, iMatch2, iDep2, matchedInputs2, iGroup2, oMatch2, oGroup2,
                           matchedOutputs1, matchedOutputs2, oMatchedGroups, iNonSingleton, ii, iidx) )
                MATCH_FOUND = TRUE;    

        if( iNonSingleton == NULL)
            MATCH_FOUND = TRUE;

        return MATCH_FOUND;
    }

    i = Vec_IntEntry(oNonSingleton, oI);

    mismatch = Vec_IntAlloc(10);
    
    skipList = ABC_ALLOC(int, Vec_IntSize(oMatch1[i]));

    for(j = 0; j < Vec_IntSize(oMatch1[i]); j++)
        skipList[j] = FALSE;

    Vec_PtrPush(oMatchPairs, Abc_NtkPo(subNtk1, Vec_IntEntry(oMatch1[i], idx)) );
    Vec_IntPush(matchedOutputs1, Vec_IntEntry(oMatch1[i], idx));

    for(j = 0; j < Vec_IntSize( oMatch2[i] ) && MATCH_FOUND == FALSE; j++)
    {
        if( Vec_IntEntry(oMatch2[i], j) == -1 || skipList[j] == TRUE)
            continue;

        Vec_PtrPush(oMatchPairs, Abc_NtkPo(subNtk2, Vec_IntEntry(oMatch2[i], j)));
        Vec_IntPush(matchedOutputs2, Vec_IntEntry(oMatch2[i], j));

        counter++;        
        if( Abc_NtkBmSat( subNtk1, subNtk2, NULL, oMatchPairs, mismatch, 0) )
        {
            /*fprintf(matchFile, "%s matched to %s\n", Abc_ObjName(Abc_NtkPo(pNtk1, Vec_IntEntry(oMatch1[i], idx))), 
                                         Abc_ObjName(Abc_NtkPo(pNtk2, Vec_IntEntry(oMatch2[i], j))));            */

            temp =  Vec_IntEntry(oMatch2[i], j);
            Vec_IntWriteEntry(oMatch2[i], j, -1);
            
            if(idx != Vec_IntSize( oMatch1[i] ) - 1)
                // call the same function with idx+1
                matchNonSingletonOutputs(pNtk1, nodesInLevel1, iMatch1, iDep1, matchedInputs1, iGroup1, oMatch1, oGroup1,
                                         pNtk2, nodesInLevel2, iMatch2, iDep2, matchedInputs2, iGroup2, oMatch2, oGroup2,
                                         matchedOutputs1, matchedOutputs2, oMatchedGroups, iNonSingleton,                         
                                         subNtk1, subNtk2, oMatchPairs,
                                         oNonSingleton, oI, idx+1, ii, iidx);
            else    
                // call the same function with idx = 0 and oI++
                matchNonSingletonOutputs(pNtk1, nodesInLevel1, iMatch1, iDep1, matchedInputs1, iGroup1, oMatch1, oGroup1,
                                         pNtk2, nodesInLevel2, iMatch2, iDep2, matchedInputs2, iGroup2, oMatch2, oGroup2,
                                         matchedOutputs1, matchedOutputs2, oMatchedGroups, iNonSingleton,                         
                                         subNtk1, subNtk2, oMatchPairs,
                                         oNonSingleton, oI+1, 0, ii, iidx);

            Vec_IntWriteEntry(oMatch2[i], j, temp);
        }        
        else
        {
            int * output1, * output2;
            int k;
            Abc_Obj_t * pObj;
            int * pModel;
            char * vPiValues;            
            

            vPiValues = ABC_ALLOC( char,  Abc_NtkPiNum(subNtk1) + 1);
            vPiValues[Abc_NtkPiNum(subNtk1)] = '\0';    
            
            for(k = 0; k < Abc_NtkPiNum(subNtk1); k++)
                vPiValues[k] = '0';

            for(k = 0; k < Vec_IntSize(mismatch); k += 2)            
                vPiValues[Vec_IntEntry(mismatch, k)] = Vec_IntEntry(mismatch, k+1);                        

            pModel = ABC_ALLOC( int, Abc_NtkCiNum(subNtk1) );    

            Abc_NtkForEachPi( subNtk1, pObj, k )
                pModel[k] = vPiValues[k] - '0';
            Abc_NtkForEachLatch( subNtk1, pObj, k )
                pModel[Abc_NtkPiNum(subNtk1)+k] = pObj->iData - 1;            
    
            output1 = Abc_NtkVerifySimulatePattern( subNtk1, pModel );

            Abc_NtkForEachLatch( subNtk2, pObj, k )
                pModel[Abc_NtkPiNum(subNtk2)+k] = pObj->iData - 1;

            output2 = Abc_NtkVerifySimulatePattern( subNtk2, pModel );
            

            for(k = 0; k < Vec_IntSize( oMatch1[i] ); k++)
                if(output1[Vec_IntEntry(oMatch1[i], idx)] != output2[Vec_IntEntry(oMatch2[i], k)])
                {
                    skipList[k] = TRUE;    
                    /*printf("Output is SKIPPED");*/
                }
                
            ABC_FREE( vPiValues );
            ABC_FREE( pModel );
            ABC_FREE( output1 );
            ABC_FREE( output2 );
        }
        
        if(MATCH_FOUND == FALSE )
        {
            Vec_PtrPop(oMatchPairs);
            Vec_IntPop(matchedOutputs2);
        }
    }

    if(MATCH_FOUND == FALSE )
    {
        Vec_PtrPop(oMatchPairs);
        Vec_IntPop(matchedOutputs1);
    }

    if(MATCH_FOUND && counter != 0)
    {        
        /*printf("Number of OUTPUT SAT instances = %d", counter);*/
        counter = 0;
    }

    ABC_FREE( mismatch );
    ABC_FREE( skipList );

    return MATCH_FOUND;
}

int match1by1(Abc_Ntk_t * pNtk1, Vec_Ptr_t ** nodesInLevel1, Vec_Int_t ** iMatch1, Vec_Int_t ** iDep1, Vec_Int_t * matchedInputs1, int * iGroup1, Vec_Int_t ** oMatch1, int * oGroup1,
               Abc_Ntk_t * pNtk2, Vec_Ptr_t ** nodesInLevel2, Vec_Int_t ** iMatch2, Vec_Int_t ** iDep2, Vec_Int_t * matchedInputs2, int * iGroup2, Vec_Int_t ** oMatch2, int * oGroup2,
               Vec_Int_t * matchedOutputs1, Vec_Int_t * matchedOutputs2, Vec_Int_t * oMatchedGroups, Vec_Int_t * iNonSingleton, int ii, int idx)
{
    static int MATCH_FOUND = FALSE;
    Abc_Ntk_t * subNtk1, * subNtk2;
    Vec_Int_t * oNonSingleton;    
    Vec_Ptr_t * oMatchPairs;
    int * skipList;
    int j, m;    
    int i;        
    static int counter = 0;

    MATCH_FOUND = FALSE;

    if( ii == Vec_IntSize(iNonSingleton) )
    {
        MATCH_FOUND = TRUE;
        return TRUE;
    }
    
    i = Vec_IntEntry(iNonSingleton, ii);

    if( idx == Vec_IntSize(iMatch1[i]) )
    {        
        // call again with the next element in iNonSingleton
        return match1by1(pNtk1, nodesInLevel1, iMatch1, iDep1, matchedInputs1, iGroup1, oMatch1, oGroup1,
                         pNtk2, nodesInLevel2, iMatch2, iDep2, matchedInputs2, iGroup2, oMatch2, oGroup2,
                         matchedOutputs1, matchedOutputs2, oMatchedGroups, iNonSingleton, ii+1, 0);            
        
    }    
    
    oNonSingleton = Vec_IntAlloc(10);
    oMatchPairs = Vec_PtrAlloc(100);    
    skipList = ABC_ALLOC(int, Vec_IntSize(iMatch1[i]));

    for(j = 0; j < Vec_IntSize(iMatch1[i]); j++)
        skipList[j] = FALSE;
    
    Vec_IntPush(matchedInputs1, Vec_IntEntry(iMatch1[i], idx));
    idx++;
    
    if(idx == 1)
    {
        for(j = 0; j < Vec_IntSize(iDep1[Vec_IntEntryLast(iMatch1[i])]); j++)
        {
            if( Vec_IntSize(oMatch1[oGroup1[Vec_IntEntry(iDep1[Vec_IntEntryLast(iMatch1[i])], j)]]) == 1 ) 
                continue;
            if( Vec_IntFind( oMatchedGroups, oGroup1[Vec_IntEntry(iDep1[Vec_IntEntryLast(iMatch1[i])], j)]) != -1)
                continue;
            
            Vec_IntPushUnique(oNonSingleton,  oGroup1[Vec_IntEntry(iDep1[Vec_IntEntryLast(iMatch1[i])], j)]);
            Vec_IntPushUnique(oMatchedGroups, oGroup1[Vec_IntEntry(iDep1[Vec_IntEntryLast(iMatch1[i])], j)]);        
        }
    }

    subNtk1 = computeCofactor(pNtk1, nodesInLevel1, NULL, matchedInputs1);

    for(j = idx-1; j < Vec_IntSize(iMatch2[i]) && MATCH_FOUND == FALSE; j++)
    {
        int tempJ;
        Vec_Int_t * mismatch;

        if( skipList[j] )
            continue;

        mismatch = Vec_IntAlloc(10);

        Vec_IntPush(matchedInputs2, Vec_IntEntry(iMatch2[i], j));        
        
        subNtk2 = computeCofactor(pNtk2, nodesInLevel2, NULL, matchedInputs2);            

        for(m = 0; m < Vec_IntSize(matchedOutputs1); m++)
        {
            Vec_PtrPush(oMatchPairs, Abc_NtkPo(subNtk1, Vec_IntEntry(matchedOutputs1, m)));
            Vec_PtrPush(oMatchPairs, Abc_NtkPo(subNtk2, Vec_IntEntry(matchedOutputs2, m)));
        }

        counter++;

        if( Abc_NtkBmSat( subNtk2, subNtk1, NULL, oMatchPairs, mismatch, 0) )                
        {
            if(idx-1 != j)
            {
                tempJ = Vec_IntEntry(iMatch2[i], idx-1);
                Vec_IntWriteEntry(iMatch2[i], idx-1, Vec_IntEntry(iMatch2[i], j));
                Vec_IntWriteEntry(iMatch2[i], j, tempJ);
            }

            /*fprintf(matchFile, "%s matched to %s\n", Abc_ObjName(Abc_NtkPi(pNtk1, Vec_IntEntry(iMatch1[i], idx-1))), 
                                                      Abc_ObjName(Abc_NtkPi(pNtk2, Vec_IntEntry(iMatch2[i], j))));*/

            // we look for a match for outputs in oNonSingleton                                
            matchNonSingletonOutputs(pNtk1, nodesInLevel1, iMatch1, iDep1, matchedInputs1, iGroup1, oMatch1, oGroup1,
                                     pNtk2, nodesInLevel2, iMatch2, iDep2, matchedInputs2, iGroup2, oMatch2, oGroup2,
                                     matchedOutputs1, matchedOutputs2, oMatchedGroups, iNonSingleton,                         
                                     subNtk1, subNtk2, oMatchPairs, oNonSingleton, 0, 0, ii, idx);
            
            
            if(idx-1 != j)
            {
                tempJ = Vec_IntEntry(iMatch2[i], idx-1);
                Vec_IntWriteEntry(iMatch2[i], idx-1, Vec_IntEntry(iMatch2[i], j));
                Vec_IntWriteEntry(iMatch2[i], j, tempJ);
            }
        }
        else
        {
            Abc_Ntk_t * FpNtk1, * FpNtk2;
            int * bitVector1, * bitVector2;
            Vec_Int_t * currInputs1, * currInputs2;            
            Vec_Ptr_t * vSupp;    
            Abc_Obj_t * pObj;
            int suppNum1 = 0;
            int * suppNum2;            
            
            bitVector1 = ABC_ALLOC( int, Abc_NtkPiNum(pNtk1) );
            bitVector2 = ABC_ALLOC( int, Abc_NtkPiNum(pNtk2) );

            currInputs1 = Vec_IntAlloc(10);
            currInputs2 = Vec_IntAlloc(10);        
            
            suppNum2 = ABC_ALLOC(int, Vec_IntSize(iMatch2[i])-idx+1);

            for(m = 0; m < Abc_NtkPiNum(pNtk1); m++)
            {
                bitVector1[m] = 0;
                bitVector2[m] = 0;
            }

            for(m = 0; m < Vec_IntSize(iMatch2[i])-idx+1; m++)                        
                suppNum2[m]= 0;            

            // First of all set the value of the inputs that are already matched and are in mismatch
            for(m = 0; m < Vec_IntSize(mismatch); m += 2)
            {
                int n = Vec_IntEntry(mismatch, m);
                    
                bitVector1[Vec_IntEntry(matchedInputs1, n)] = Vec_IntEntry(mismatch, m+1);
                bitVector2[Vec_IntEntry(matchedInputs2, n)] = Vec_IntEntry(mismatch, m+1);
                
            }
            
            for(m = idx-1; m < Vec_IntSize(iMatch1[i]); m++)
            {
                Vec_IntPush(currInputs1, Vec_IntEntry(iMatch1[i], m));
                Vec_IntPush(currInputs2, Vec_IntEntry(iMatch2[i], m));
            }

            // Then add all the inputs that are not yet matched to the currInputs
            for(m = 0; m < Abc_NtkPiNum(pNtk1); m++)
            {
                if(Vec_IntFind( matchedInputs1, m ) == -1)
                    Vec_IntPushUnique(currInputs1, m);

                if(Vec_IntFind( matchedInputs2, m ) == -1)
                    Vec_IntPushUnique(currInputs2, m);
            }

            FpNtk1 = computeCofactor(pNtk1, nodesInLevel1, bitVector1, currInputs1);            
            FpNtk2 = computeCofactor(pNtk2, nodesInLevel2, bitVector2, currInputs2);    

            Abc_NtkForEachPo( FpNtk1, pObj, m )
            {        
                int n;
                vSupp  = Abc_NtkNodeSupport( FpNtk1, &pObj, 1 );                        
                
                for(n = 0; n < vSupp->nSize; n++)
                    if( Abc_ObjId((Abc_Obj_t *)vSupp->pArray[n]) == 1 )
                        suppNum1 += Vec_IntFind( matchedOutputs1, m) + 1;                
                        
                Vec_PtrFree( vSupp );
            }
            
            Abc_NtkForEachPo( FpNtk2, pObj, m )
            {        
                int n;
                vSupp  = Abc_NtkNodeSupport( FpNtk2, &pObj, 1 );                        
                
                for(n = 0; n < vSupp->nSize; n++)
                    if( (int)Abc_ObjId((Abc_Obj_t *)vSupp->pArray[n])-1 < (Vec_IntSize(iMatch2[i]))-idx+1 &&
                        (int)Abc_ObjId((Abc_Obj_t *)vSupp->pArray[n])-1 >= 0)
                        suppNum2[Abc_ObjId((Abc_Obj_t *)vSupp->pArray[n])-1] += Vec_IntFind( matchedOutputs2, m) + 1;                
                        
                Vec_PtrFree( vSupp );
            }

            /*if(suppNum1 != 0)            
                printf("Ntk1 is trigged");            

            if(suppNum2[0] != 0)
                printf("Ntk2 is trigged");*/

            for(m = 0; m < Vec_IntSize(iMatch2[i])-idx+1; m++)
                if(suppNum2[m] != suppNum1)
                {
                    skipList[m+idx-1] = TRUE;                    
                    /*printf("input is skipped");*/
                }
                
            Abc_NtkDelete( FpNtk1 );
            Abc_NtkDelete( FpNtk2 );
            ABC_FREE( bitVector1 );    
             ABC_FREE( bitVector2 );    
            Vec_IntFree( currInputs1 );
            Vec_IntFree( currInputs2 );            
            ABC_FREE( suppNum2 );
        }
        
        Vec_PtrClear(oMatchPairs);
        Abc_NtkDelete( subNtk2 );
        Vec_IntFree(mismatch);

        //Vec_IntWriteEntry(iMatch2[i], j, tempJ);        
        
        if( MATCH_FOUND == FALSE )
            Vec_IntPop(matchedInputs2);
    }

    if( MATCH_FOUND == FALSE )
    {
        Vec_IntPop(matchedInputs1);    
        
        if(idx == 1)
        {
            for(m = 0; m < Vec_IntSize(oNonSingleton); m++)
                Vec_IntPop( oMatchedGroups );
        }
    }
    
    Vec_IntFree( oNonSingleton );    
    Vec_PtrFree( oMatchPairs );    
    ABC_FREE( skipList );
    Abc_NtkDelete( subNtk1 );    

    if(MATCH_FOUND && counter != 0)
    {        
        /*printf("Number of INPUT SAT instances = %d\n", counter);*/

        counter = 0;
    }

    return MATCH_FOUND;
}

float refineBySAT(Abc_Ntk_t * pNtk1, Vec_Int_t ** iMatch1, int * iGroup1, Vec_Int_t ** iDep1, int* iLastItem1, Vec_Int_t ** oMatch1, int * oGroup1, Vec_Int_t ** oDep1, int* oLastItem1, int * observability1,
                 Abc_Ntk_t * pNtk2, Vec_Int_t ** iMatch2, int * iGroup2, Vec_Int_t ** iDep2, int* iLastItem2, Vec_Int_t ** oMatch2, int * oGroup2, Vec_Int_t ** oDep2, int* oLastItem2, int * observability2)
{
    int i, j;    
    Abc_Obj_t * pObj;
    Vec_Int_t * iNonSingleton;
    Vec_Int_t * matchedInputs1, * matchedInputs2;
    Vec_Int_t * matchedOutputs1, * matchedOutputs2;
    Vec_Ptr_t ** nodesInLevel1, ** nodesInLevel2;
    Vec_Int_t * oMatchedGroups;
    FILE *result;    
    int matchFound;
    abctime clk = Abc_Clock();
    float satTime = 0.0;

    /*matchFile = fopen("satmatch.txt", "w");*/
    
    iNonSingleton = Vec_IntAlloc(10);    
    
    matchedInputs1 = Vec_IntAlloc( Abc_NtkPiNum(pNtk1) );
    matchedInputs2 = Vec_IntAlloc( Abc_NtkPiNum(pNtk2) );

    matchedOutputs1 = Vec_IntAlloc( Abc_NtkPoNum(pNtk1) );
    matchedOutputs2 = Vec_IntAlloc( Abc_NtkPoNum(pNtk2) );

    nodesInLevel1 = ABC_ALLOC( Vec_Ptr_t *, Abc_AigLevel( pNtk1 ) + 1);    // why numOfLevels+1? because the inputs are in level 0
    for(i = 0; i <= Abc_AigLevel( pNtk1 ); i++)
        nodesInLevel1[i] = Vec_PtrAlloc( 20 );

    // bucket sort the objects based on their levels
    Abc_AigForEachAnd( pNtk1, pObj, i )    
        Vec_PtrPush(nodesInLevel1[Abc_ObjLevel(pObj)], pObj);
    
    nodesInLevel2 = ABC_ALLOC( Vec_Ptr_t *, Abc_AigLevel( pNtk2 ) + 1);    // why numOfLevels+1? because the inputs are in level 0
    for(i = 0; i <= Abc_AigLevel( pNtk2 ); i++)
        nodesInLevel2[i] = Vec_PtrAlloc( 20 );

    // bucket sort the objects based on their levels
    Abc_AigForEachAnd( pNtk2, pObj, i )    
        Vec_PtrPush(nodesInLevel2[Abc_ObjLevel(pObj)], pObj);    

    oMatchedGroups = Vec_IntAlloc( 10 );

    for(i = 0; i < *iLastItem1; i++)
    {
        if(Vec_IntSize(iMatch1[i]) == 1)
        {
            Vec_IntPush(matchedInputs1, Vec_IntEntryLast(iMatch1[i]));
            Vec_IntPush(matchedInputs2, Vec_IntEntryLast(iMatch2[i]));        
        }
        else
            Vec_IntPush(iNonSingleton, i);
    }

    for(i = 0; i < *oLastItem1; i++)
    {
        if(Vec_IntSize(oMatch1[i]) == 1)
        {
            Vec_IntPush(matchedOutputs1, Vec_IntEntryLast(oMatch1[i]));
            Vec_IntPush(matchedOutputs2, Vec_IntEntryLast(oMatch2[i]));
        }
    }
    
    for(i = 0; i < Vec_IntSize(iNonSingleton) - 1; i++)
    {
        for(j = i + 1; j < Vec_IntSize(iNonSingleton); j++)        
            if( observability2[Vec_IntEntry(iMatch2[Vec_IntEntry(iNonSingleton, j)], 0)] > 
                observability2[Vec_IntEntry(iMatch2[Vec_IntEntry(iNonSingleton, i)], 0)] )
            {                
                int temp = Vec_IntEntry(iNonSingleton, i);
                Vec_IntWriteEntry( iNonSingleton, i, Vec_IntEntry(iNonSingleton, j) );
                Vec_IntWriteEntry( iNonSingleton, j, temp );                
            }
            else if( observability2[Vec_IntEntry(iMatch2[Vec_IntEntry(iNonSingleton, j)], 0)] == 
                     observability2[Vec_IntEntry(iMatch2[Vec_IntEntry(iNonSingleton, i)], 0)] )
            {
                if( Vec_IntSize(iMatch2[Vec_IntEntry(iNonSingleton, j)]) < Vec_IntSize(iMatch2[Vec_IntEntry(iNonSingleton, i)]) )
                {
                    int temp = Vec_IntEntry(iNonSingleton, i);
                    Vec_IntWriteEntry( iNonSingleton, i, Vec_IntEntry(iNonSingleton, j) );
                    Vec_IntWriteEntry( iNonSingleton, j, temp );                
                }
            }
    }    

    /*for(i = 0; i < Vec_IntSize(iNonSingleton) - 1; i++)
    {
        for(j = i + 1; j < Vec_IntSize(iNonSingleton); j++)        
            if( Vec_IntSize(oDep2[oGroup2[Vec_IntEntryLast(iMatch2[Vec_IntEntry(iNonSingleton, j)])]]) > 
                Vec_IntSize(oDep2[oGroup2[Vec_IntEntryLast(iMatch2[Vec_IntEntry(iNonSingleton, i)])]]) )
            {                
                int temp = Vec_IntEntry(iNonSingleton, i);
                Vec_IntWriteEntry( iNonSingleton, i, Vec_IntEntry(iNonSingleton, j) );
                Vec_IntWriteEntry( iNonSingleton, j, temp );                
            }
            else if( Vec_IntSize(oDep2[oGroup2[Vec_IntEntryLast(iMatch2[Vec_IntEntry(iNonSingleton, j)])]]) == 
                Vec_IntSize(oDep2[oGroup2[Vec_IntEntryLast(iMatch2[Vec_IntEntry(iNonSingleton, i)])]]) )
            {
                if( observability2[Vec_IntEntry(iMatch2[Vec_IntEntry(iNonSingleton, j)], 0)] > 
                        observability2[Vec_IntEntry(iMatch2[Vec_IntEntry(iNonSingleton, i)], 0)] )
                {
                    int temp = Vec_IntEntry(iNonSingleton, i);
                    Vec_IntWriteEntry( iNonSingleton, i, Vec_IntEntry(iNonSingleton, j) );
                    Vec_IntWriteEntry( iNonSingleton, j, temp );                
                }
            }
    }*/

    matchFound = match1by1(pNtk1, nodesInLevel1, iMatch1, iDep1, matchedInputs1, iGroup1, oMatch1, oGroup1,
                           pNtk2, nodesInLevel2, iMatch2, iDep2, matchedInputs2, iGroup2, oMatch2, oGroup2,
                           matchedOutputs1, matchedOutputs2, oMatchedGroups, iNonSingleton, 0, 0);

    if( matchFound && Vec_IntSize(matchedOutputs1) != Abc_NtkPoNum(pNtk1) )
    {
        Vec_Int_t * oNonSingleton;
        Vec_Ptr_t * oMatchPairs;
        Abc_Ntk_t * subNtk1, * subNtk2;

        oNonSingleton = Vec_IntAlloc( 10 );
        
        oMatchPairs = Vec_PtrAlloc(Abc_NtkPoNum(pNtk1) * 2);    

        for(i = 0; i < *oLastItem1; i++)
            if( Vec_IntSize(oMatch1[i]) > 1 && Vec_IntFind( oMatchedGroups, i) == -1 )
                Vec_IntPush(oNonSingleton, i);
            
        subNtk1 = computeCofactor(pNtk1, nodesInLevel1, NULL, matchedInputs1);
        subNtk2 = computeCofactor(pNtk2, nodesInLevel2, NULL, matchedInputs2);
        
        matchFound = matchNonSingletonOutputs(pNtk1, nodesInLevel1, iMatch1, iDep1, matchedInputs1, iGroup1, oMatch1, oGroup1,
                                 pNtk2, nodesInLevel2, iMatch2, iDep2, matchedInputs2, iGroup1, oMatch2, oGroup2,
                                 matchedOutputs1, matchedOutputs2, oMatchedGroups, NULL,                         
                                 subNtk1, subNtk2, oMatchPairs, oNonSingleton, 0, 0, 0, 0);

        Vec_IntFree( oNonSingleton );        
        Vec_PtrFree( oMatchPairs );

        Abc_NtkDelete(subNtk1);
        Abc_NtkDelete(subNtk2);
    }

    satTime = (float)(Abc_Clock() - clk)/(float)(CLOCKS_PER_SEC);    

    if( matchFound )
    {
          checkEquivalence( pNtk1, matchedInputs1, matchedOutputs1, pNtk2, matchedInputs2, matchedOutputs2);

        result = fopen("IOmatch.txt", "w");

        fprintf(result, "I/O = %d / %d \n\n", Abc_NtkPiNum(pNtk1), Abc_NtkPoNum(pNtk1));
        
        for(i = 0; i < Vec_IntSize(matchedInputs1) ; i++)
              fprintf(result, "{%s}\t{%s}\n", Abc_ObjName(Abc_NtkPi(pNtk1, Vec_IntEntry(matchedInputs1, i))), Abc_ObjName(Abc_NtkPi(pNtk2, Vec_IntEntry(matchedInputs2, i))) );                                    

        fprintf(result, "\n-----------------------------------------\n");

        for(i = 0; i < Vec_IntSize(matchedOutputs1) ; i++)
            fprintf(result, "{%s}\t{%s}\n", Abc_ObjName(Abc_NtkPo(pNtk1, Vec_IntEntry(matchedOutputs1, i))), Abc_ObjName(Abc_NtkPo(pNtk2, Vec_IntEntry(matchedOutputs2, i))) );                                    

        fclose( result );    
    }

    Vec_IntFree( matchedInputs1 );
    Vec_IntFree( matchedInputs2 );
    Vec_IntFree( matchedOutputs1 );
    Vec_IntFree( matchedOutputs2 );
    Vec_IntFree( iNonSingleton );
    Vec_IntFree( oMatchedGroups );
    
    for(i = 0; i <= Abc_AigLevel( pNtk1 ); i++)    
        Vec_PtrFree( nodesInLevel1[i] );
    for(i = 0; i <= Abc_AigLevel( pNtk2 ); i++)    
        Vec_PtrFree( nodesInLevel2[i] );
    

    ABC_FREE( nodesInLevel1 );
    ABC_FREE( nodesInLevel2 );
    /*fclose(matchFile);*/
    
    return satTime;
}

int checkListConsistency(Vec_Int_t ** iMatch1, Vec_Int_t ** oMatch1, Vec_Int_t ** iMatch2, Vec_Int_t ** oMatch2, int iLastItem1, int oLastItem1, int iLastItem2, int oLastItem2)
{
    //int i;

    if(iLastItem1 != iLastItem2 || oLastItem1 != oLastItem2)
        return FALSE;

    /*for(i = 0; i < iLastItem1; i++) {
        if(Vec_IntSize(iMatch1[i]) != Vec_IntSize(iMatch2[i]))
            return FALSE;
    }

    for(i = 0; i < oLastItem1; i++) {
        if(Vec_IntSize(oMatch1[i]) != Vec_IntSize(oMatch2[i]))
            return FALSE;
    }*/
        
    return TRUE;
}


void bmGateWay( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int p_equivalence )
{    
    Vec_Int_t ** iDep1, ** oDep1;
    Vec_Int_t ** iDep2, ** oDep2;
    Vec_Int_t ** iMatch1, ** oMatch1;
    Vec_Int_t ** iMatch2, ** oMatch2;        
    int * iGroup1, * oGroup1;
    int * iGroup2, * oGroup2;
    int iLastItem1, oLastItem1;
    int iLastItem2, oLastItem2;    
    int i, j;    
    
    char * vPiValues1, * vPiValues2;
    int * observability1, * observability2;
    abctime clk = Abc_Clock();
    float initTime;
    float simulTime;
    float satTime;
    Vec_Ptr_t ** topOrder1 = NULL, ** topOrder2 = NULL;

    extern void getDependencies(Abc_Ntk_t *pNtk, Vec_Int_t** iDep, Vec_Int_t** oDep);
    extern void initMatchList(Abc_Ntk_t *pNtk, Vec_Int_t** iDep, Vec_Int_t** oDep, Vec_Int_t** iMatch, int* iLastItem, Vec_Int_t** oMatch, int* oLastItem, int* iGroup, int* oGroup, int p_equivalence);        
    extern void iSortDependencies(Abc_Ntk_t *pNtk, Vec_Int_t** iDep, int* oGroup);
    extern void oSortDependencies(Abc_Ntk_t *pNtk, Vec_Int_t** oDep, int* iGroup);
    extern int iSplitByDep(Abc_Ntk_t *pNtk, Vec_Int_t** iDep, Vec_Int_t** iMatch, int* iGroup, int* iLastItem, int* oGroup);
    extern int oSplitByDep(Abc_Ntk_t *pNtk, Vec_Int_t** oDep, Vec_Int_t** oMatch, int* oGroup, int* oLastItem, int* iGroup);    
    extern Vec_Ptr_t ** findTopologicalOrder(Abc_Ntk_t * pNtk);
    extern int refineIOBySimulation(Abc_Ntk_t *pNtk, Vec_Int_t** iMatch, int* iLastItem, int * iGroup, Vec_Int_t** iDep, Vec_Int_t** oMatch, int* oLastItem, int * oGroup, Vec_Int_t** oDep, char * vPiValues, int * observability, Vec_Ptr_t ** topOrder);    
    extern float refineBySAT(Abc_Ntk_t * pNtk1, Vec_Int_t ** iMatch1, int * iGroup1, Vec_Int_t ** iDep1, int* iLastItem1, Vec_Int_t ** oMatch1, int * oGroup1, Vec_Int_t ** oDep1, int* oLastItem1, int * observability1,
                            Abc_Ntk_t * pNtk2, Vec_Int_t ** iMatch2, int * iGroup2, Vec_Int_t ** iDep2, int* iLastItem2, Vec_Int_t ** oMatch2, int * oGroup2, Vec_Int_t ** oDep2, int* oLastItem2, int * observability2);                
    int checkListConsistency(Vec_Int_t ** iMatch1, Vec_Int_t ** oMatch1, Vec_Int_t ** iMatch2, Vec_Int_t ** oMatch2, int iLastItem1, int oLastItem1, int iLastItem2, int oLastItem2);    

    iDep1 = ABC_ALLOC( Vec_Int_t*,  (unsigned)Abc_NtkPiNum(pNtk1) );
    oDep1 = ABC_ALLOC( Vec_Int_t*,  (unsigned)Abc_NtkPoNum(pNtk1) );

    iDep2 = ABC_ALLOC( Vec_Int_t*,  (unsigned)Abc_NtkPiNum(pNtk2) );
    oDep2 = ABC_ALLOC( Vec_Int_t*,  (unsigned)Abc_NtkPoNum(pNtk2) );

    iMatch1 = ABC_ALLOC( Vec_Int_t*,  (unsigned)Abc_NtkPiNum(pNtk1) );
    oMatch1 = ABC_ALLOC( Vec_Int_t*,  (unsigned)Abc_NtkPoNum(pNtk1) );

    iMatch2 = ABC_ALLOC( Vec_Int_t*,  (unsigned)Abc_NtkPiNum(pNtk2) );
    oMatch2 = ABC_ALLOC( Vec_Int_t*,  (unsigned)Abc_NtkPoNum(pNtk2) );        

    iGroup1 = ABC_ALLOC( int, Abc_NtkPiNum(pNtk1) );
    oGroup1 = ABC_ALLOC( int, Abc_NtkPoNum(pNtk1) );

    iGroup2 = ABC_ALLOC( int, Abc_NtkPiNum(pNtk2) );
    oGroup2 = ABC_ALLOC( int, Abc_NtkPoNum(pNtk2) );

    vPiValues1 = ABC_ALLOC( char,  Abc_NtkPiNum(pNtk1) + 1);
    vPiValues1[Abc_NtkPiNum(pNtk1)] = '\0';    

    vPiValues2 = ABC_ALLOC( char,  Abc_NtkPiNum(pNtk2) + 1);
    vPiValues2[Abc_NtkPiNum(pNtk2)] = '\0';    

    observability1 = ABC_ALLOC(int, (unsigned)Abc_NtkPiNum(pNtk1));
    observability2 = ABC_ALLOC(int, (unsigned)Abc_NtkPiNum(pNtk2));

    for(i = 0; i < Abc_NtkPiNum(pNtk1); i++)
    {        
        iDep1[i] = Vec_IntAlloc( 1 );
        iMatch1[i] = Vec_IntAlloc( 1 );

        iDep2[i] = Vec_IntAlloc( 1 );
        iMatch2[i] = Vec_IntAlloc( 1 );

        vPiValues1[i] = '0';
        vPiValues2[i] = '0';

        observability1[i] = 0;
        observability2[i] = 0;
    }

    for(i = 0; i < Abc_NtkPoNum(pNtk1); i++)
    {
        oDep1[i] = Vec_IntAlloc( 1 );
        oMatch1[i] = Vec_IntAlloc( 1 );

        oDep2[i] = Vec_IntAlloc( 1 );
        oMatch2[i] = Vec_IntAlloc( 1 );
    }    
    
    /************* Strashing ************/    
    pNtk1 = Abc_NtkStrash( pNtk1, 0, 0, 0 );    
    pNtk2 = Abc_NtkStrash( pNtk2, 0, 0, 0 );            
    printf("Network  strashing is done!\n");    
    /************************************/    
    
    /******* Getting Dependencies *******/    
    getDependencies(pNtk1, iDep1, oDep1);
    getDependencies(pNtk2, iDep2, oDep2);            
    printf("Getting dependencies is done!\n");    
    /************************************/

    /***** Intializing match lists ******/    
    initMatchList(pNtk1, iDep1, oDep1, iMatch1, &iLastItem1, oMatch1, &oLastItem1, iGroup1, oGroup1, p_equivalence);            
    initMatchList(pNtk2, iDep2, oDep2, iMatch2, &iLastItem2, oMatch2, &oLastItem2, iGroup2, oGroup2, p_equivalence);    
    printf("Initializing match lists is done!\n");        
    /************************************/

    if( !checkListConsistency(iMatch1, oMatch1, iMatch2, oMatch2, iLastItem1, oLastItem1, iLastItem2, oLastItem2) )
    {
        fprintf( stdout, "I/O dependencies of two circuits are different.\n");
        goto freeAndExit;
    }        

    printf("Refining IOs by dependencies ...");                
    // split match lists further by checking dependencies
    do
    {
        int iNumOfItemsAdded = 1, oNumOfItemsAdded = 1;        

        do
        {    
            if( oNumOfItemsAdded )
            {
                iSortDependencies(pNtk1, iDep1, oGroup1);
                iSortDependencies(pNtk2, iDep2, oGroup2);
            }
            
            if( iNumOfItemsAdded )
            {
                oSortDependencies(pNtk1, oDep1, iGroup1);
                oSortDependencies(pNtk2, oDep2, iGroup2);
            }

            if( iLastItem1 < Abc_NtkPiNum(pNtk1) )
            {                
                iSplitByDep(pNtk1, iDep1, iMatch1, iGroup1, &iLastItem1, oGroup1);
                if( oLastItem1 < Abc_NtkPoNum(pNtk1) )
                    oSplitByDep(pNtk1, oDep1, oMatch1, oGroup1, &oLastItem1, iGroup1);
            }                

            if( iLastItem2 < Abc_NtkPiNum(pNtk2) )
                iNumOfItemsAdded = iSplitByDep(pNtk2, iDep2, iMatch2, iGroup2, &iLastItem2, oGroup2);
            else
                iNumOfItemsAdded = 0;    
                
            if( oLastItem2 < Abc_NtkPoNum(pNtk2) )        
                oNumOfItemsAdded = oSplitByDep(pNtk2, oDep2, oMatch2, oGroup2, &oLastItem2, iGroup2);
            else
                oNumOfItemsAdded = 0;
            
            if(!checkListConsistency(iMatch1, oMatch1, iMatch2, oMatch2, iLastItem1, oLastItem1, iLastItem2, oLastItem2))
            {
                fprintf( stdout, "I/O dependencies of two circuits are different.\n");
                goto freeAndExit;
            }        
        }while(iNumOfItemsAdded != 0 || oNumOfItemsAdded != 0);

    }while(0);

    printf(" done!\n");

    initTime = ((float)(Abc_Clock() - clk)/(float)(CLOCKS_PER_SEC));    
    clk = Abc_Clock();

    topOrder1 = findTopologicalOrder(pNtk1);
    topOrder2 = findTopologicalOrder(pNtk2);

    printf("Refining IOs by simulation ...");                

    do
    {
        int counter = 0;
        int ioSuccess1, ioSuccess2;    
        
        do
        {
            for(i = 0; i < iLastItem1; i++)
            {
                int temp = (int)(SIM_RANDOM_UNSIGNED % 2);        
                
                if(Vec_IntSize(iMatch1[i]) != Vec_IntSize(iMatch2[i]))
                {
                    fprintf( stdout, "Input refinement by simulation finds two circuits different.\n");                
                    goto freeAndExit;
                }
                
                for(j = 0; j < Vec_IntSize(iMatch1[i]); j++)
                {
                    vPiValues1[Vec_IntEntry(iMatch1[i], j)] = temp + '0';
                    vPiValues2[Vec_IntEntry(iMatch2[i], j)] = temp + '0';    
                }            
            }                    
            
            ioSuccess1 = refineIOBySimulation(pNtk1, iMatch1, &iLastItem1, iGroup1, iDep1, oMatch1, &oLastItem1, oGroup1, oDep1, vPiValues1, observability1, topOrder1);                
            ioSuccess2 = refineIOBySimulation(pNtk2, iMatch2, &iLastItem2, iGroup2, iDep2, oMatch2, &oLastItem2, oGroup2, oDep2, vPiValues2, observability2, topOrder2);
            
            if(ioSuccess1 && ioSuccess2)
                counter = 0;
            else
                counter++;                        
            
            if(ioSuccess1 != ioSuccess2 ||
               !checkListConsistency(iMatch1, oMatch1, iMatch2, oMatch2, iLastItem1, oLastItem1, iLastItem2, oLastItem2))
            {
                fprintf( stdout, "Input refinement by simulation finds two circuits different.\n");                
                goto freeAndExit;
            }
        }while(counter <= 200);                
        
    }while(0);    

    printf(" done!\n");
    
    simulTime = (float)(Abc_Clock() - clk)/(float)(CLOCKS_PER_SEC);
    printf("SAT-based search started ...\n");

    satTime = refineBySAT(pNtk1, iMatch1, iGroup1, iDep1, &iLastItem1, oMatch1, oGroup1, oDep1, &oLastItem1, observability1,
                                  pNtk2, iMatch2, iGroup2, iDep2, &iLastItem2, oMatch2, oGroup2, oDep2, &oLastItem2, observability2);

    printf( "Init Time = %4.2f\n", initTime );    
    printf( "Simulation Time = %4.2f\n", simulTime );
    printf( "SAT Time = %4.2f\n", satTime );
    printf( "Overall Time = %4.2f\n", initTime + simulTime + satTime );
    
freeAndExit:

    for(i = 0; i < iLastItem1 ; i++)
    {            
        
        Vec_IntFree( iMatch1[i] );
        Vec_IntFree( iMatch2[i] );
    }
    
    for(i = 0; i < oLastItem1 ; i++)
    {            
        
        Vec_IntFree( oMatch1[i] );
        Vec_IntFree( oMatch2[i] );
    }

    for(i = 0; i < Abc_NtkPiNum(pNtk1); i++)
    {
        Vec_IntFree( iDep1[i] );
        Vec_IntFree( iDep2[i] );
        if(topOrder1 != NULL) {
            Vec_PtrFree( topOrder1[i] );
            Vec_PtrFree( topOrder2[i] );
        }
    }

    for(i = 0; i < Abc_NtkPoNum(pNtk1); i++)
    {
        Vec_IntFree( oDep1[i] );
        Vec_IntFree( oDep2[i] );
    }

    ABC_FREE( iMatch1 );
    ABC_FREE( iMatch2 );
    ABC_FREE( oMatch1 );
    ABC_FREE( oMatch2 );
    ABC_FREE( iDep1 );
    ABC_FREE( iDep2 );
    ABC_FREE( oDep1 );
    ABC_FREE( oDep2 );
    ABC_FREE( iGroup1 );
    ABC_FREE( iGroup2 );
    ABC_FREE( oGroup1 );
    ABC_FREE( oGroup2 );    
    ABC_FREE( vPiValues1 );
    ABC_FREE( vPiValues2 );
    ABC_FREE( observability1 );
    ABC_FREE( observability2 );
    if(topOrder1 != NULL) {
        ABC_FREE( topOrder1 );
        ABC_FREE( topOrder2 );
    }
}ABC_NAMESPACE_IMPL_END

