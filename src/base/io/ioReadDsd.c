/**CFile****************************************************************

  FileName    [ioReadDsd.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedure to read network from file.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: ioReadDsd.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ioAbc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Finds the end of the part.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Io_ReadDsdFindEnd( char * pCur )
{
    char * pEnd;
    int nParts = 0;
    assert( *pCur == '(' );
    for ( pEnd = pCur; *pEnd; pEnd++ )
    {
        if ( *pEnd == '(' )
            nParts++;
        else if ( *pEnd == ')' )
            nParts--;
        if ( nParts == 0 )
            return pEnd;
    }
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Splits the formula into parts.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadDsdStrSplit( char * pCur, char * pParts[], int * pTypeXor )
{
    int fAnd = 0, fXor = 0, fPri = 0, nParts = 0;
    assert( *pCur );
    // process the parts
    while ( 1 )
    {
        // save the current part
        pParts[nParts++] = pCur;
        // skip the complement
        if ( *pCur == '!' )
            pCur++;
        // skip var
        if ( *pCur >= 'a' && *pCur <= 'z' )
            pCur++;
        else
        {
            // skip hex truth table
            while ( (*pCur >= '0' && *pCur <= '9') || (*pCur >= 'A' && *pCur <= 'F') )
                pCur++;
            // process parentheses
            if ( *pCur != '(' )
            {
                printf( "Cannot find the opening parenthesis.\n" );
                break;
            }
            // find the corresponding closing parenthesis
            pCur = Io_ReadDsdFindEnd( pCur );
            if ( pCur == NULL )
            {
                printf( "Cannot find the closing parenthesis.\n" );
                break;
            }
            pCur++;
        }
        // check the end
        if ( *pCur == 0 )
            break;
        // check symbol
        if ( *pCur != '*' && *pCur != '+' && *pCur != ',' )
        {
            printf( "Wrong separating symbol.\n" );
            break;
        }
        // remember the symbol
        fAnd |= (*pCur == '*');
        fXor |= (*pCur == '+');
        fPri |= (*pCur == ',');
        *pCur++ = 0;
    }
    // check separating symbols
    if ( fAnd + fXor + fPri > 1 )
    {
        printf( "Different types of separating symbol ennPartsed.\n" );
        return 0;
    }
    *pTypeXor = fXor;
    return nParts;
}

/**Function*************************************************************

  Synopsis    [Recursively parses the formula.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Io_ReadDsd_rec( Abc_Ntk_t * pNtk, char * pCur, char * pSop )
{
    Abc_Obj_t * pObj, * pFanin;
    char * pEnd, * pParts[32];
    int i, nParts, TypeExor;

    // consider complemented formula
    if ( *pCur == '!' )
    {
        pObj = Io_ReadDsd_rec( pNtk, pCur + 1, NULL );
        return Abc_NtkCreateNodeInv( pNtk, pObj );
    }
    if ( *pCur == '(' )
    {
        assert( pCur[strlen(pCur)-1] == ')' );
        pCur[strlen(pCur)-1] = 0;
        nParts = Io_ReadDsdStrSplit( pCur+1, pParts, &TypeExor );
        if ( nParts == 0 )
        {
            Abc_NtkDelete( pNtk );
            return NULL;
        }
        pObj = Abc_NtkCreateNode( pNtk );
        if ( pSop )
        {
//            for ( i = nParts - 1; i >= 0; i-- )
            for ( i = 0; i < nParts; i++ )
            {
                pFanin = Io_ReadDsd_rec( pNtk, pParts[i], NULL );
                if ( pFanin == NULL )
                    return NULL;
                Abc_ObjAddFanin( pObj, pFanin );
            }
        }
        else
        {
            for ( i = 0; i < nParts; i++ )
            {
                pFanin = Io_ReadDsd_rec( pNtk, pParts[i], NULL );
                if ( pFanin == NULL )
                    return NULL;
                Abc_ObjAddFanin( pObj, pFanin );
            }
        }
        if ( pSop )
            pObj->pData = Abc_SopRegister( (Mem_Flex_t *)pNtk->pManFunc, pSop );
        else if ( TypeExor )
            pObj->pData = Abc_SopCreateXorSpecial( (Mem_Flex_t *)pNtk->pManFunc, nParts );
        else
            pObj->pData = Abc_SopCreateAnd( (Mem_Flex_t *)pNtk->pManFunc, nParts, NULL );
        return pObj;
    }
    if ( *pCur >= 'a' && *pCur <= 'z' )
    {
        assert( *(pCur+1) == 0 );
        return Abc_NtkPi( pNtk, *pCur - 'a' );
    }

    // skip hex truth table
    pEnd = pCur;
    while ( (*pEnd >= '0' && *pEnd <= '9') || (*pEnd >= 'A' && *pEnd <= 'F') )
        pEnd++;
    if ( *pEnd != '(' )
    {
        printf( "Cannot find the end of hexidecimal truth table.\n" );
        return NULL;
    }

    // parse the truth table
    *pEnd = 0;
    pSop = Abc_SopFromTruthHex( pCur );
    *pEnd = '(';
    pObj = Io_ReadDsd_rec( pNtk, pEnd, pSop );
    ABC_FREE( pSop );
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Derives the DSD network of the formula.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Io_ReadDsd( char * pForm )
{
    Abc_Ntk_t * pNtk;
    Abc_Obj_t * pObj, * pTop;
    Vec_Ptr_t * vNames;
    char * pCur, * pFormCopy;
    int i, nInputs;

    // count the number of elementary variables
    nInputs = 0;
    for ( pCur = pForm; *pCur; pCur++ )
        if ( *pCur >= 'a' && *pCur <= 'z' )
            nInputs = Abc_MaxInt( nInputs, *pCur - 'a' );
    nInputs++;

    // create the network
    pNtk = Abc_NtkAlloc( ABC_NTK_LOGIC, ABC_FUNC_SOP, 1 );
    pNtk->pName = Extra_UtilStrsav( "dsd" );

    // create PIs
    vNames = Abc_NodeGetFakeNames( nInputs );
    for ( i = 0; i < nInputs; i++ )
        Abc_ObjAssignName( Abc_NtkCreatePi(pNtk), (char *)Vec_PtrEntry(vNames, i), NULL );
    Abc_NodeFreeNames( vNames );

    // transform the formula by inserting parentheses
    // this transforms strings like PRIME(a,b,cd) into (PRIME((a),(b),(cd)))
    pCur = pFormCopy = ABC_ALLOC( char, 3 * strlen(pForm) + 10 );
    *pCur++ = '(';
    for ( ; *pForm; pForm++ )
        if ( *pForm == '(' )
        {
            *pCur++ = '(';
            *pCur++ = '(';
        }
        else if ( *pForm == ')' )
        {
            *pCur++ = ')';
            *pCur++ = ')';
        }
        else if ( *pForm == ',' )
        {
            *pCur++ = ')';
            *pCur++ = ',';
            *pCur++ = '(';
        }
        else
            *pCur++ = *pForm;
    *pCur++ = ')';
    *pCur = 0;

    // parse the formula
    pObj = Io_ReadDsd_rec( pNtk, pFormCopy, NULL );
    ABC_FREE( pFormCopy );
    if ( pObj == NULL )
        return NULL;

    // create output
    pTop = Abc_NtkCreatePo(pNtk);
    Abc_ObjAssignName( pTop, "F", NULL );
    Abc_ObjAddFanin( pTop, pObj );

    // create the only PO
    if ( !Abc_NtkCheck( pNtk ) )
    {
        fprintf( stdout, "Io_ReadDsd(): Network check has failed.\n" );
        Abc_NtkDelete( pNtk );
        return NULL;
    }
    return pNtk;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_IMPL_END

