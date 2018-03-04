/**CFile****************************************************************

  FileName    [ltl_parser.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Liveness property checking.]

  Synopsis    [LTL checker.]

  Author      [Sayak Ray]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2009.]

  Revision    [$Id: ltl_parser.c,v 1.00 2009/01/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "aig/aig/aig.h"
#include "base/abc/abc.h"
#include "base/main/mainInt.h"

ABC_NAMESPACE_IMPL_START


enum ltlToken { AND, OR, NOT, IMPLY, GLOBALLY, EVENTUALLY, NEXT, UNTIL, BOOL };
enum ltlGrammerToken { OPERAND, LTL, BINOP, UOP };
typedef enum ltlToken tokenType;
typedef enum ltlGrammerToken ltlGrammerTokenType;

struct ltlNode_t
{
	tokenType type;
	char *name;
	Aig_Obj_t *pObj;
	struct ltlNode_t *left;
	struct ltlNode_t *right;
};

typedef struct ltlNode_t ltlNode;

ltlNode *generateTypedNode( tokenType new_type )
//void generateTypedNode( ltlNode *new_node, tokenType new_type )
{
	ltlNode *new_node;

	new_node = (ltlNode *)malloc( sizeof(ltlNode) );
	if( new_node )
	{
		new_node->type = new_type;
		new_node->pObj = NULL;
		new_node->name = NULL;
		new_node->left = NULL;
		new_node->right = NULL;
	}

	return new_node;
}

Aig_Obj_t *buildLogicFromLTLNode_combinationalOnly( Aig_Man_t *pAig, ltlNode *pLtlNode );

static inline int isNotVarNameSymbol( char c )
{
	return ( c == ' ' || c == '\t' || c == '\n' || c == ':' || c == '\0' );
}

void Abc_FrameCopyLTLDataBase( Abc_Frame_t *pAbc, Abc_Ntk_t * pNtk )
{
	char *pLtlFormula, *tempFormula;
	int i;

	if( pAbc->vLTLProperties_global != NULL )
	{
//		printf("Deleting exisitng LTL database from the frame\n");
        Vec_PtrFree( pAbc->vLTLProperties_global );
		pAbc->vLTLProperties_global = NULL;
	}
	pAbc->vLTLProperties_global = Vec_PtrAlloc(Vec_PtrSize(pNtk->vLtlProperties));
	Vec_PtrForEachEntry( char *, pNtk->vLtlProperties, pLtlFormula, i )
	{
		tempFormula = (char *)malloc( sizeof(char)*(strlen(pLtlFormula)+1) );
		sprintf( tempFormula, "%s", pLtlFormula );
		Vec_PtrPush( pAbc->vLTLProperties_global, tempFormula );
	}
}

char *getVarName( char *suffixFormula, int startLoc, int *endLocation )
{
	int i = startLoc, length;
	char *name;

	if( isNotVarNameSymbol( suffixFormula[startLoc] ) )
		return NULL;

	while( !isNotVarNameSymbol( suffixFormula[i] ) )
		i++;
	*endLocation = i;
	length = i - startLoc;
	name = (char *)malloc( sizeof(char) * (length + 1));
	for( i=0; i<length; i++ )
		name[i] = suffixFormula[i+startLoc];
	name[i] = '\0';

	return name;
}


int startOfSuffixString = 0;

int isUnexpectedEOS( char *formula, int index )
{
	assert( formula );
	if( index >= (int)strlen( formula ) )
	{
		printf("\nInvalid LTL formula: unexpected end of string..." );
		return 1;
	}
	return 0;
}

int isTemporalOperator( char *formula, int index )
{
	if( !(isUnexpectedEOS( formula, index ) || formula[ index ] == 'G' || formula[ index ] == 'F' || formula[ index ] == 'U' || formula[ index ] == 'X') )
	{
		printf("\nInvalid LTL formula: expecting temporal operator at the position %d....\n", index);
		return 0;
	}
	return 1;
}

ltlNode *readLtlFormula( char *formula )
{
	char ch;
	char *varName;
	int formulaLength, rememberEnd;
	int i = startOfSuffixString;
	ltlNode *curr_node, *temp_node_left, *temp_node_right;
	char prevChar;
	
	formulaLength = strlen( formula );
	if( isUnexpectedEOS( formula, startOfSuffixString ) )
	{
		printf("\nFAULTING POINT: formula = %s\nstartOfSuffixString = %d, formula[%d] = %c\n\n", formula, startOfSuffixString, startOfSuffixString - 1, formula[startOfSuffixString-1]);
		return NULL;
	}
			
	while( i < formulaLength )
	{
		ch = formula[i];

		switch(ch){
			case ' ':
			case '\n':
			case '\r':
			case '\t':
			case '\v':
			case '\f':
						i++;
						startOfSuffixString = i;
						break;
			case ':':
						i++;
						if( !isTemporalOperator( formula, i ) )
							return NULL;
						startOfSuffixString = i;
						break;
			case 'G':
						prevChar = formula[i-1];
						if( prevChar == ':' ) //i.e. 'G' is a temporal operator
						{
							i++;
							startOfSuffixString = i;
							temp_node_left = readLtlFormula( formula );
							if( temp_node_left == NULL )
								return NULL;
							else
							{
								curr_node = generateTypedNode(GLOBALLY);
								curr_node->left = temp_node_left;
								return curr_node;
							}
						}
						else	//i.e. 'G' must be starting a variable name
						{
							varName = getVarName( formula, i, &rememberEnd );
							if( !varName )
							{
								printf("\nInvalid LTL formula: expecting valid variable name token...aborting" );
								return NULL;
							}
							curr_node = generateTypedNode(BOOL);
							curr_node->name = varName;
							i = rememberEnd;
							startOfSuffixString = i;
							return curr_node;
						}
			case 'F':
						prevChar = formula[i-1];
						if( prevChar == ':' ) //i.e. 'F' is a temporal operator
						{
							i++;
							startOfSuffixString = i;
							temp_node_left = readLtlFormula( formula );
							if( temp_node_left == NULL )
								return NULL;
							else
							{
								curr_node = generateTypedNode(EVENTUALLY);
								curr_node->left = temp_node_left;
								return curr_node;
							}
						}
						else //i.e. 'F' must be starting a variable name
						{
							varName = getVarName( formula, i, &rememberEnd );
							if( !varName )
							{
								printf("\nInvalid LTL formula: expecting valid variable name token...aborting" );
								return NULL;
							}
							curr_node = generateTypedNode(BOOL);
							curr_node->name = varName;
							i = rememberEnd;
							startOfSuffixString = i;
							return curr_node;
						}	
			case 'X':
						prevChar = formula[i-1];
						if( prevChar == ':' ) //i.e. 'X' is a temporal operator
						{
							i++;
							startOfSuffixString = i;
							temp_node_left = readLtlFormula( formula );
							if( temp_node_left == NULL )
								return NULL;
							else
							{
								curr_node = generateTypedNode(NEXT);
								curr_node->left = temp_node_left;
								return curr_node;
							}
						}
						else //i.e. 'X' must be starting a variable name
						{
							varName = getVarName( formula, i, &rememberEnd );
							if( !varName )
							{
								printf("\nInvalid LTL formula: expecting valid variable name token...aborting" );
								return NULL;
							}
							curr_node = generateTypedNode(BOOL);
							curr_node->name = varName;
							i = rememberEnd;
							startOfSuffixString = i;
							return curr_node;
						}	
			case 'U':
						prevChar = formula[i-1];
						if( prevChar == ':' ) //i.e. 'X' is a temporal operator
						{
							i++;
							startOfSuffixString = i;
							temp_node_left = readLtlFormula( formula );
							if( temp_node_left == NULL )
								return NULL;
							temp_node_right = readLtlFormula( formula );
							if( temp_node_right == NULL )
							{
								//need to do memory management: if right subtree is NULL then left
								//subtree must be freed.
								return NULL;
							}
							curr_node = generateTypedNode(UNTIL);
							curr_node->left = temp_node_left;
							curr_node->right = temp_node_right;
							return curr_node;
						}
						else //i.e. 'U' must be starting a variable name
						{
							varName = getVarName( formula, i, &rememberEnd );
							if( !varName )
							{
								printf("\nInvalid LTL formula: expecting valid variable name token...aborting" );
								return NULL;
							}
							curr_node = generateTypedNode(BOOL);
							curr_node->name = varName;
							i = rememberEnd;
							startOfSuffixString = i;
							return curr_node;
						}	
			case '+':
						i++;
						startOfSuffixString = i;
						temp_node_left = readLtlFormula( formula );
						if( temp_node_left == NULL )
							return NULL;
						temp_node_right = readLtlFormula( formula );
						if( temp_node_right == NULL )
						{
							//need to do memory management: if right subtree is NULL then left
							//subtree must be freed.
							return NULL;
						}
						curr_node = generateTypedNode(OR);
						curr_node->left = temp_node_left;
						curr_node->right = temp_node_right;
						return curr_node;
			case '&':
						i++;
						startOfSuffixString = i;
						temp_node_left = readLtlFormula( formula );
						if( temp_node_left == NULL )
							return NULL;
						temp_node_right = readLtlFormula( formula );
						if( temp_node_right == NULL )
						{
							//need to do memory management: if right subtree is NULL then left
							//subtree must be freed.
							return NULL;
						}
						curr_node = generateTypedNode(AND);
						curr_node->left = temp_node_left;
						curr_node->right = temp_node_right;
						return curr_node;
			case '!':
						i++;
						startOfSuffixString = i;
						temp_node_left = readLtlFormula( formula );
						if( temp_node_left == NULL )
							return NULL;
						else
						{
							curr_node = generateTypedNode(NOT);
							curr_node->left = temp_node_left;
							return curr_node;
						}
			default:
				varName = getVarName( formula, i, &rememberEnd );
				if( !varName )
				{
					printf("\nInvalid LTL formula: expecting valid variable name token...aborting" );
					return NULL;
				}
				curr_node = generateTypedNode(BOOL);
				curr_node->name = varName;
				i = rememberEnd;
				startOfSuffixString = i;
				return curr_node;
		}
	}
	return NULL;
}

void resetGlobalVar()
{
	startOfSuffixString = 0;
}

ltlNode *parseFormulaCreateAST( char *inputFormula )
{
	ltlNode *temp;

	temp = readLtlFormula( inputFormula );
	//if( temp == NULL )
	//	printf("\nAST creation failed for formula %s", inputFormula );
	resetGlobalVar();
	return temp;
}

void traverseAbstractSyntaxTree( ltlNode *node )
{
	switch(node->type){
		case( AND ):
			printf("& ");
			assert( node->left != NULL );
			assert( node->right != NULL );
			traverseAbstractSyntaxTree( node->left );
			traverseAbstractSyntaxTree( node->right );
			return;
		case( OR ):
			printf("+ ");
			assert( node->left != NULL );
			assert( node->right != NULL );
			traverseAbstractSyntaxTree( node->left );
			traverseAbstractSyntaxTree( node->right );
			return;
		case( NOT ):
			printf("~ ");
			assert( node->left != NULL );
			traverseAbstractSyntaxTree( node->left );
			assert( node->right == NULL );
			return;
		case( GLOBALLY ):
			printf("G ");
			assert( node->left != NULL );
			traverseAbstractSyntaxTree( node->left );
			assert( node->right == NULL );
			return;
		case( EVENTUALLY ):
			printf("F ");
			assert( node->left != NULL );
			traverseAbstractSyntaxTree( node->left );
			assert( node->right == NULL );
			return;
		case( NEXT ):
			printf("X ");
			assert( node->left != NULL );
			traverseAbstractSyntaxTree( node->left );
			assert( node->right == NULL );
			return;
		case( UNTIL ):
			printf("U ");
			assert( node->left != NULL );
			assert( node->right != NULL );
			traverseAbstractSyntaxTree( node->left );
			traverseAbstractSyntaxTree( node->right );
			return;
		case( BOOL ):
			printf("%s ", node->name);
			assert( node->left == NULL );
			assert( node->right == NULL );
			return;
		default:
			printf("\nUnsupported token type: Exiting execution\n");
			exit(0);
	}
}

void traverseAbstractSyntaxTree_postFix( ltlNode *node )
{
	switch(node->type){
		case( AND ):
			printf("( ");
			assert( node->left != NULL );
			assert( node->right != NULL );
			traverseAbstractSyntaxTree_postFix( node->left );
			printf("& ");
			traverseAbstractSyntaxTree_postFix( node->right );
			printf(") ");
			return;
		case( OR ):
			printf("( ");
			assert( node->left != NULL );
			assert( node->right != NULL );
			traverseAbstractSyntaxTree_postFix( node->left );
			printf("+ ");
			traverseAbstractSyntaxTree_postFix( node->right );
			printf(") ");
			return;
		case( NOT ):
			printf("~ ");
			assert( node->left != NULL );
			traverseAbstractSyntaxTree_postFix( node->left );
			assert( node->right == NULL );
			return;
		case( GLOBALLY ):
			printf("G ");
			//printf("( ");
			assert( node->left != NULL );
			traverseAbstractSyntaxTree_postFix( node->left );
			assert( node->right == NULL );
			//printf(") ");
			return;
		case( EVENTUALLY ):
			printf("F ");
			//printf("( ");
			assert( node->left != NULL );
			traverseAbstractSyntaxTree_postFix( node->left );
			assert( node->right == NULL );
			//printf(") ");
			return;
		case( NEXT ):
			printf("X ");
			assert( node->left != NULL );
			traverseAbstractSyntaxTree_postFix( node->left );
			assert( node->right == NULL );
			return;
		case( UNTIL ):
			printf("( ");
			assert( node->left != NULL );
			assert( node->right != NULL );
			traverseAbstractSyntaxTree_postFix( node->left );
			printf("U ");
			traverseAbstractSyntaxTree_postFix( node->right );
			printf(") ");
			return;
		case( BOOL ):
			printf("%s ", node->name);
			assert( node->left == NULL );
			assert( node->right == NULL );
			return;
		default:
			printf("\nUnsupported token type: Exiting execution\n");
			exit(0);
	}
}

void populateAigPointerUnitGF( Aig_Man_t *pAigNew, ltlNode *topASTNode, Vec_Ptr_t *vSignal, Vec_Vec_t *vAigGFMap )
{
	ltlNode *nextNode, *nextToNextNode;
	int serialNumSignal;

	switch( topASTNode->type ){
		case AND:
		case OR:
		case IMPLY:
			populateAigPointerUnitGF( pAigNew, topASTNode->left, vSignal, vAigGFMap );
			populateAigPointerUnitGF( pAigNew, topASTNode->right, vSignal, vAigGFMap );
			return;
		case NOT:
			populateAigPointerUnitGF( pAigNew, topASTNode->left, vSignal, vAigGFMap );
			return;
		case GLOBALLY:
			nextNode = topASTNode->left;
			assert( nextNode->type = EVENTUALLY );
			nextToNextNode = nextNode->left;
			if( nextToNextNode->type == BOOL )
			{
				assert( nextToNextNode->pObj );
				serialNumSignal = Vec_PtrFind( vSignal, nextToNextNode->pObj );
				if( serialNumSignal == -1 )
				{					
					Vec_PtrPush( vSignal, nextToNextNode->pObj );
					serialNumSignal = Vec_PtrFind( vSignal, nextToNextNode->pObj );
				}
				//Vec_PtrPush( vGLOBALLY, topASTNode );
				Vec_VecPush( vAigGFMap, serialNumSignal, topASTNode );
			}
			else
			{
				assert( nextToNextNode->pObj == NULL );
				buildLogicFromLTLNode_combinationalOnly( pAigNew, nextToNextNode );
				serialNumSignal = Vec_PtrFind( vSignal, nextToNextNode->pObj );
				if( serialNumSignal == -1 )
				{
					Vec_PtrPush( vSignal, nextToNextNode->pObj );
					serialNumSignal = Vec_PtrFind( vSignal, nextToNextNode->pObj );
				}
				//Vec_PtrPush( vGLOBALLY, topASTNode );
				Vec_VecPush( vAigGFMap, serialNumSignal, topASTNode );
			}
			return;
		case BOOL:
			return;
		default:
			printf("\nINVALID situation: aborting...\n");
			exit(0);
	}
}

Aig_Obj_t *buildLogicFromLTLNode_combinationalOnly( Aig_Man_t *pAigNew, ltlNode *pLtlNode )
{
	Aig_Obj_t *leftAigObj, *rightAigObj;

	if( pLtlNode->pObj != NULL )
		return pLtlNode->pObj;
	else
	{
		assert( pLtlNode->type != BOOL );
		switch( pLtlNode->type ){
			case AND:
				assert( pLtlNode->left ); assert( pLtlNode->right );
				leftAigObj = buildLogicFromLTLNode_combinationalOnly( pAigNew, pLtlNode->left );
				rightAigObj = buildLogicFromLTLNode_combinationalOnly( pAigNew, pLtlNode->right );
				assert( leftAigObj ); assert( rightAigObj );
				pLtlNode->pObj = Aig_And( pAigNew, leftAigObj, rightAigObj );
				return pLtlNode->pObj;
			case OR:
				assert( pLtlNode->left ); assert( pLtlNode->right );
				leftAigObj = buildLogicFromLTLNode_combinationalOnly( pAigNew, pLtlNode->left );
				rightAigObj = buildLogicFromLTLNode_combinationalOnly( pAigNew, pLtlNode->right );
				assert( leftAigObj ); assert( rightAigObj );
				pLtlNode->pObj = Aig_Or( pAigNew, leftAigObj, rightAigObj );
				return pLtlNode->pObj;
			case NOT:
				assert( pLtlNode->left ); assert( pLtlNode->right == NULL ); 
				leftAigObj = buildLogicFromLTLNode_combinationalOnly( pAigNew, pLtlNode->left );
				assert( leftAigObj );
				pLtlNode->pObj = Aig_Not( leftAigObj );
				return pLtlNode->pObj;
			case GLOBALLY:
			case EVENTUALLY:
			case NEXT:
			case UNTIL:
				printf("FORBIDDEN node: ABORTING!!\n");
				exit(0);
			default:
				printf("\nSerious ERROR: attempting to create AIG node from a temporal node\n");
				exit(0);
		}
	}
}

Aig_Obj_t *buildLogicFromLTLNode( Aig_Man_t *pAig, ltlNode *pLtlNode )
{
	Aig_Obj_t *leftAigObj, *rightAigObj;

	if( pLtlNode->pObj != NULL )
		return pLtlNode->pObj;
	else
	{
		assert( pLtlNode->type != BOOL );
		switch( pLtlNode->type ){
			case AND:
				assert( pLtlNode->left ); assert( pLtlNode->right );
				leftAigObj = buildLogicFromLTLNode( pAig, pLtlNode->left );
				rightAigObj = buildLogicFromLTLNode( pAig, pLtlNode->right );
				assert( leftAigObj ); assert( rightAigObj );
				pLtlNode->pObj = Aig_And( pAig, leftAigObj, rightAigObj );
				return pLtlNode->pObj;
			case OR:
				assert( pLtlNode->left ); assert( pLtlNode->right );
				leftAigObj = buildLogicFromLTLNode( pAig, pLtlNode->left );
				rightAigObj = buildLogicFromLTLNode( pAig, pLtlNode->right );
				assert( leftAigObj ); assert( rightAigObj );
				pLtlNode->pObj = Aig_Or( pAig, leftAigObj, rightAigObj );
				return pLtlNode->pObj;
			case NOT:
				assert( pLtlNode->left ); assert( pLtlNode->right == NULL ); 
				leftAigObj = buildLogicFromLTLNode( pAig, pLtlNode->left );
				assert( leftAigObj );
				pLtlNode->pObj = Aig_Not( leftAigObj );
				return pLtlNode->pObj;
			case GLOBALLY:
			case EVENTUALLY:
			case NEXT:
			case UNTIL:
				printf("\nAttempting to create circuit with missing AIG pointer in a TEMPORAL node: ABORTING!!\n");
				exit(0);
			default:
				printf("\nSerious ERROR: attempting to create AIG node from a temporal node\n");
				exit(0);
		}
	}
}

int isNonTemporalSubformula( ltlNode *topNode )
{
	switch( topNode->type ){
		case AND:
		case OR:
		case IMPLY:
			return isNonTemporalSubformula( topNode->left) && isNonTemporalSubformula( topNode->right ) ;
		case NOT:
			assert( topNode->right == NULL );
			return isNonTemporalSubformula( topNode->left );
		case BOOL:
			return 1;
		default:
			return 0;
	}
}

int isWellFormed( ltlNode *topNode )
{
	ltlNode *nextNode;

	switch( topNode->type ){
		case AND:
		case OR:
		case IMPLY:
			return isWellFormed( topNode->left) && isWellFormed( topNode->right ) ;
		case NOT:
			assert( topNode->right == NULL );
			return isWellFormed( topNode->left );
		case BOOL:
			return 1;
		case GLOBALLY:
			nextNode = topNode->left;
			assert( topNode->right == NULL );
			if( nextNode->type != EVENTUALLY )
				return 0;
			else
			{
				assert( nextNode->right == NULL );
				return isNonTemporalSubformula( nextNode->left );
			}
		default:
			return 0;
	}
}

int checkBooleanConstant( char *targetName )
{
	if( strcmp( targetName, "true" ) == 0 ) 
		return 1;
	if( strcmp( targetName, "false" ) == 0 )
		return 0;
	return -1;
}

int checkSignalNameExistence( Abc_Ntk_t *pNtk, ltlNode *topASTNode )
{
	char *targetName;
	Abc_Obj_t * pNode;
	int i;
		
	switch( topASTNode->type ){
		case BOOL:
			targetName = topASTNode->name;
			//printf("\nTrying to match name %s\n", targetName);
			if( checkBooleanConstant( targetName ) != -1 )
				return 1;
			Abc_NtkForEachPo( pNtk, pNode, i )
			{
				if( strcmp( Abc_ObjName( pNode ), targetName ) == 0 )
				{
					//printf("\nVariable name \"%s\" MATCHED\n", targetName);
					return 1;
				}
			}
			printf("\nVariable name \"%s\" not found in the PO name list\n", targetName);
			return 0;
		case AND:
		case OR:
		case IMPLY:
		case UNTIL:
			assert( topASTNode->left != NULL );
			assert( topASTNode->right != NULL );
			return checkSignalNameExistence( pNtk, topASTNode->left ) && checkSignalNameExistence( pNtk, topASTNode->right );
			
		case NOT:
		case NEXT:
		case GLOBALLY:
		case EVENTUALLY:
			assert( topASTNode->left != NULL );
			assert( topASTNode->right == NULL );
			return checkSignalNameExistence( pNtk, topASTNode->left );
		default:
			printf("\nUNSUPPORTED LTL NODE TYPE:: Aborting execution\n");
			exit(0);
	}
}

void populateBoolWithAigNodePtr( Abc_Ntk_t *pNtk, Aig_Man_t *pAigOld, Aig_Man_t *pAigNew, ltlNode *topASTNode )
{
	char *targetName;
	Abc_Obj_t * pNode;
	int i;
	Aig_Obj_t *pObj, *pDriverImage;
	
	switch( topASTNode->type ){
		case BOOL:
			targetName = topASTNode->name;
			if( checkBooleanConstant( targetName ) == 1 )
			{
				topASTNode->pObj = Aig_ManConst1( pAigNew );
				return;
			}
			if( checkBooleanConstant( targetName ) == 0 )
			{
				topASTNode->pObj = Aig_Not(topASTNode->pObj = Aig_ManConst1( pAigNew ));
				return;
			}
			Abc_NtkForEachPo( pNtk, pNode, i )
				if( strcmp( Abc_ObjName( pNode ), targetName ) == 0 )
				{
					pObj = Aig_ManCo( pAigOld, i );
					assert( Aig_ObjIsCo( pObj ));
					pDriverImage = Aig_NotCond((Aig_Obj_t *)Aig_Regular(Aig_ObjChild0( pObj ))->pData, Aig_ObjFaninC0(pObj));
					topASTNode->pObj = pDriverImage;
					return;
				}
			assert(0);
		case AND:
		case OR:
		case IMPLY:
		case UNTIL:
			assert( topASTNode->left != NULL );
			assert( topASTNode->right != NULL );
			populateBoolWithAigNodePtr( pNtk, pAigOld, pAigNew, topASTNode->left );
			populateBoolWithAigNodePtr( pNtk, pAigOld, pAigNew, topASTNode->right );
			return;
		case NOT:
		case NEXT:
		case GLOBALLY:
		case EVENTUALLY:
			assert( topASTNode->left != NULL );
			assert( topASTNode->right == NULL );
			populateBoolWithAigNodePtr( pNtk, pAigOld, pAigNew, topASTNode->left );
			return;
		default:
			printf("\nUNSUPPORTED LTL NODE TYPE:: Aborting execution\n");
			exit(0);
	}
}

int checkAllBoolHaveAIGPointer( ltlNode *topASTNode )
{
	
	switch( topASTNode->type ){
		case BOOL:
			if( topASTNode->pObj != NULL )
				return 1;
			else
			{
				printf("\nfaulting PODMANDYO topASTNode->name = %s\n", topASTNode->name);
				return 0;
			}
		case AND:
		case OR:
		case IMPLY:
		case UNTIL:
			assert( topASTNode->left != NULL );
			assert( topASTNode->right != NULL );
			return checkAllBoolHaveAIGPointer( topASTNode->left ) && checkAllBoolHaveAIGPointer( topASTNode->right );
			
		case NOT:
		case NEXT:
		case GLOBALLY:
		case EVENTUALLY:
			assert( topASTNode->left != NULL );
			assert( topASTNode->right == NULL );
			return checkAllBoolHaveAIGPointer( topASTNode->left );
		default:
			printf("\nUNSUPPORTED LTL NODE TYPE:: Aborting execution\n");
			exit(0);
	}
}

void setAIGNodePtrOfGloballyNode( ltlNode *astNode, Aig_Obj_t *pObjLo )
{
	astNode->pObj = pObjLo;
}
			
Aig_Obj_t *retriveAIGPointerFromLTLNode( ltlNode *astNode )
{
	return astNode->pObj;
}


ABC_NAMESPACE_IMPL_END
