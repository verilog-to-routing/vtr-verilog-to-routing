%{
/* 
* Copyright (C) 2002-2015 XimpleWare, info@ximpleware.com
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#include "customTypes.h"
#include <string.h>
#include <stdio.h>
#include "xpath.h"
#include "helper.h"

Step *tmpStep;
Step *tmpStep2;
NodeTest *tmpNt;
locationPathExpr *tmpLPExpr ;
aList *tmpAl;
Predicate *tmpP;

static expr *x;
static NsList *xpathNSList = NULL;
static ExprList *xpathExprList = NULL;
extern int isName;
extern int colonPosition;
expr* ex;
%}

%union {
	UCSChar*   literal;
	axisType at;
	int	 integer;
	double	 number;
	struct nametype {
		UCSChar* prefix;
		UCSChar* qname;
		UCSChar* localname;
	} name;
	struct {
       		nodeTestType nt;
       		UCSChar*   arg;
	} ntest;
	funcName fname;
	expr *expression;
 	locationPathExpr *lpe;
 	unionExpr *une;
	pathExpr *pe;
	Step *s;
	aList *a;
	NodeTest *nodetest;
	Predicate* p;
}

%token <at> AXISNAME 
%token <literal> LITERAL
%token <number>  NUMBER
%token <name> NAME
%token <fname> FNAME
%token <ntest> NTEST
%type <expression> Expr OrExpr AndExpr EqualityExpr RelationalExpr AdditiveExpr
%type <expression> MultiplicativeExpr UnaryExpr FilterExpr
%type <expression> FunctionCall Argument PrimaryExpr VariableReference
%type <une> UnionExpr
%type <lpe> LocationPath
%type <pe> PathExpr
%type <s> Step AbbreviatedStep RelativeLocationPath  AbsoluteLocationPath
%type <s> AbbreviatedAbsoluteLocationPath  AbbreviatedRelativeLocationPath
%type <p> Predicate PredicateList
%type <a> ArgumentList
%type <at> AxisSpecifier AbbreviatedAxisSpecifier
%type <fname> FunctionName
%type <nodetest> NodeTest
%token OR AND EQ NE GT LT GE LE ADD SUB MULT DIV MOD
%token LP RP DOLLAR  UNION SLASH DSLASH COMMA ERROR
%token AT DOT DDOT LB RB DOLLAR
%left  OR
%left  AND
%left  EQ NE
%left  GE LE GT LT
%nonassoc UMINUS

%%
Expr 		:    OrExpr  { x=$$ = $1;}
       		;

OrExpr 		:    AndExpr  {$$ = $1;}
		|    OrExpr OR AndExpr {
								Try {
										$$ = (expr *)createBinaryExpr($1, OP_OR,$3);
										addObj($$);
									}
								Catch(e){
									//freeAllObj();
									 YYABORT;
								} 
		
								}
		;

AndExpr		:    EqualityExpr { $$ = $1;}
	 	|    AndExpr AND EqualityExpr {
	 							Try {
	 									$$ = (expr *)createBinaryExpr($1,OP_AND,$3);
	 									addObj($$);
	 								}
	 							Catch(e){
	 								//freeAllObj();
									YYABORT;
								}
								}
		;

EqualityExpr    :    RelationalExpr  { $$ = $1;}
		|    EqualityExpr EQ RelationalExpr  { 
								Try {
										if ($1->isFinal($1) && $1->isString($1)){
											if ($3->getFuncOpCode($3)==FN_NAME){
												funcExpr *tmp = (funcExpr *)$3;
												tmp->opCode = FN_MATCH_NAME;
												addArg_fne(tmp,$1);
												$$ = tmp;
											} else if ($3->getFuncOpCode($3)==FN_LOCAL_NAME) {
												funcExpr *tmp = (funcExpr *)$3;
												tmp->opCode = FN_MATCH_LOCAL_NAME;
												addArg_fne(tmp,$1);
												$$ = tmp;
											} else
											   $$ = (expr *)createBinaryExpr($1,OP_EQ,$3);
										} else if ($3->isFinal($3) && $3->isString($3)){
											if ($1->getFuncOpCode($1)==FN_NAME){
												funcExpr *tmp = (funcExpr *)$1;
												tmp->opCode = FN_MATCH_NAME;
												addArg_fne(tmp,$3);
												$$ = tmp;
											} else if ($1->getFuncOpCode($1)==FN_LOCAL_NAME) {
												funcExpr *tmp = (funcExpr *)$1;
												tmp->opCode = FN_MATCH_LOCAL_NAME;
												addArg_fne(tmp,$3);
												$$ = tmp;
										    } else
											   $$ = (expr *)createBinaryExpr($1,OP_EQ,$3);
										}
										else 
	 										$$ = (expr *)createBinaryExpr($1,OP_EQ,$3);
	 									addObj($$);
	 								}
	 							Catch(e){
	 								//freeAllObj();
									YYABORT;
								}
								}
		|    EqualityExpr NE RelationalExpr {
			 					Try {
								if ($1->isFinal($1) && $1->isString($1)){
											if ($3->getFuncOpCode($3)==FN_NAME){
												funcExpr *tmp = (funcExpr *)$3;
												tmp->opCode = FN_NOT_MATCH_NAME;
												addArg_fne(tmp,$1);
												$$ = tmp;
											} else if ($3->getFuncOpCode($3)==FN_LOCAL_NAME) {
												funcExpr *tmp = (funcExpr *)$3;
												tmp->opCode = FN_NOT_MATCH_LOCAL_NAME;
												addArg_fne(tmp,$1);
												$$ = tmp;
											} else
											   $$ = (expr *)createBinaryExpr($1,OP_NE,$3);
										} else if ($3->isFinal($3) && $3->isString($3)){
											if ($1->getFuncOpCode($1)==FN_NAME){
												funcExpr *tmp = (funcExpr *)$1;
												tmp->opCode = FN_NOT_MATCH_NAME;
												addArg_fne(tmp,$3);
												$$ = tmp;
											} else if ($1->getFuncOpCode($1)==FN_LOCAL_NAME) {
												funcExpr *tmp = (funcExpr *)$1;
												tmp->opCode = FN_NOT_MATCH_LOCAL_NAME;
												addArg_fne(tmp,$3);
												$$ = tmp;
										    } else
											   $$ = (expr *)createBinaryExpr($1,OP_NE,$3);
										}
										else 
	 									$$ = (expr *)createBinaryExpr($1,OP_NE,$3);
	 									addObj($$);
	 								}
	 							Catch(e){
	 								//freeAllObj();
									YYABORT;
								}
								}
		;

RelationalExpr  :    AdditiveExpr  { $$ = $1; }
  		|    RelationalExpr LT AdditiveExpr  {
			 					Try {
	 									$$ =(expr *) createBinaryExpr($1,OP_LT,$3);
	 									addObj($$);
	 								}
	 							Catch(e){
	 								//freeAllObj();
									YYABORT;
								}
								}
		|    RelationalExpr GT AdditiveExpr  {
			 					Try {
	 									$$ = (expr *)createBinaryExpr($1,OP_GT,$3);
	 									addObj($$);	 									
	 								}
	 							Catch(e){
	 								//freeAllObj();
									YYABORT;
								}
								}
		|    RelationalExpr LE AdditiveExpr  {
			 					Try {
	 									$$ = (expr *)createBinaryExpr($1,OP_LE,$3);
	 									addObj($$);
	 								}
	 							Catch(e){
	 								//freeAllObj();
									YYABORT;
								}
								}
		|    RelationalExpr GE AdditiveExpr  {
			 					Try {
	 									$$ = (expr *)createBinaryExpr($1,OP_GE,$3);
	 									addObj($$);
	 								}
	 							Catch(e){
	 								//freeAllObj();
									YYABORT;
								}
								}
		;

AdditiveExpr    :    MultiplicativeExpr  {$$  = $1; }
		|    AdditiveExpr ADD MultiplicativeExpr  {
			 					Try {
	 									$$ = (expr *)createBinaryExpr($1,OP_ADD,$3);
	 									addObj($$);
	 								}
	 							Catch(e){
	 								//freeAllObj();
									YYABORT;
								}
								}
		|    AdditiveExpr SUB MultiplicativeExpr {
			 					Try {
	 									$$ =(expr *) createBinaryExpr($1,OP_SUB,$3);
	 									addObj($$);
	 								}
	 							Catch(e){
	 								//freeAllObj();
									YYABORT;
								}
								}
		;

MultiplicativeExpr    :  UnaryExpr  {$$ = $1; }
   		|    MultiplicativeExpr MULT UnaryExpr  {
			 					Try {
	 									$$ = (expr *)createBinaryExpr($1,OP_MULT,$3);
	 									addObj($$);
	 								}
	 							Catch(e){
	 								//freeAllObj();
									YYABORT;
								}
								}
		|    MultiplicativeExpr DIV UnaryExpr  {
			 					Try {
	 									$$ =(expr *) createBinaryExpr($1,OP_DIV,$3);
	 									addObj($$);
	 								}
	 							Catch(e){
	 								//freeAllObj();
									YYABORT;
								}
								}
		|    MultiplicativeExpr MOD UnaryExpr  {
			 					Try {
	 									$$ = (expr *)createBinaryExpr($1,OP_MOD,$3);
	 									addObj($$);
	 								}
	 							Catch(e){
	 								//freeAllObj();
									YYABORT;
								}
								}
		;

UnaryExpr    	:    UnionExpr  { 
								if ($1->next!=NULL){
									$$ = (expr *)($1->next);
								}else
									$$ = $1;
									
									
								}
  		|    SUB UnaryExpr  %prec UMINUS  {
			 					Try {
	 									
	 										$$ = (expr *)createUnaryExpr(OP_NE,$2);
	 										addObj($$); 									
	 									
	 								}
	 							Catch(e){
	 								//freeAllObj();
									YYABORT;
								}
								}
		;


UnionExpr    	:    PathExpr  	{ 
									Try {
										$$ = createUnionExpr((expr *)$1);
										addObj($$);
									}
									Catch(e) {
										YYABORT;
									 }
								
								}		
   		|    PathExpr UNION UnionExpr {
   										Try {
   										   $$ = createUnionExpr((expr *)$1);
   										   addObj($$);
   										   $$->next = $3;
   										}
   										Catch(e){
   											YYABORT;   											
   										}
   										
   		                         /*freeAllObj();*/
   		                         //YYABORT;
   		                         
   		                         }
		;


PathExpr     	:    LocationPath  { $$ = (pathExpr *) $1;  }
   		|    FilterExpr  { $$ = (pathExpr *)$1;}
  		|    FilterExpr SLASH RelativeLocationPath  {  														
  														Try{
  															tmpLPExpr = createLocationPathExpr();
  															addObj(tmpLPExpr);
  															addObj(tmpLPExpr->ih);
  															addObj(tmpLPExpr->ih->storage);
  															setStep(tmpLPExpr, $3);
  															$$ = createPathExpr($1, tmpLPExpr);
  															addObj($$);
  															addObj($$->ih);
  															addObj($$->ih->storage);
  															tmpLPExpr = NULL;
  														}Catch(e){  														
  															//freeAllObj();
  															YYABORT;
  														}  		
  													}
  													
		|    FilterExpr DSLASH RelativeLocationPath {
														Try {
															tmpStep = createStep();
															addObj(tmpStep);
															setAxisType(tmpStep,AXIS_DESCENDANT_OR_SELF);
															tmpNt = createNodeTest();
															addObj(tmpNt);
															setTestType(tmpNt, NT_NODE);
															setNodeTest(tmpStep, tmpNt);
															tmpStep->nextS = $3;
															$3->prevS = tmpStep;
															tmpLPExpr = createLocationPathExpr();
															addObj(tmpLPExpr);
															addObj(tmpLPExpr->ih);
															addObj(tmpLPExpr->ih->storage);
															setStep(tmpLPExpr, tmpStep);
															$$ = createPathExpr($1, tmpLPExpr);
															addObj($$);
															addObj($$->ih);	
															addObj($$->ih->storage);															
														} Catch (e){
															//freeAllObj();		
															YYABORT;																											
														}								
													}
		;


FilterExpr   	:    PrimaryExpr  { $$ = $1;}
  		|    FilterExpr Predicate  {Try {
  										$$ = (expr *)createFilterExpr($1,$2);
  										addObj($$);
  										}
  									Catch(e){
  										//freeAllObj();
  										YYABORT;
  									}
  								}  									
  								;



PrimaryExpr     :    VariableReference {$$ = $1;}
		|    LP Expr RP  { $$ = $2;} 
		|    LITERAL  { Try {
							$$ = (expr *)createLiteralExpr($1);
							addObj($$);
							addObj($1);
						} Catch (e) {
							//freeAllObj();
							YYABORT;	
						}						
					  } 
		|    NUMBER  { Try {
						    $$ = (expr *)createNumberExpr($1);
						    addObj($$);
					   } Catch (e) {
							//freeAllObj();
							YYABORT;
					   }
					 } 
		|    FunctionCall { $$ = $1;} 
		|    ERROR {
						//freeAllObj();
						//printf(" Invalid char encoutered \n");
						YYABORT;
					}
		;


FunctionCall    :    FunctionName LP ArgumentList RP { Try {
															$$ = (expr *)createFuncExpr($1, $3);
															addObj($$);
															if  (!checkArgumentCount($$))
																throwException2(xpath_parse_exception,"Invalid argument for functional expression");
															
													   }
													   Catch(e){
															//freeAllObj();
															YYABORT;
													   }

													 } 
		;

ArgumentList	: 	{ $$ = NULL;}   
	     	|    Argument  {	Try {
	     							$$ = createAlist();
	     							addObj($$);
	     							$$->e = $1;  								     	
	     						}Catch (e){
	     							//freeAllObj();
	     							YYABORT;
	     						}	
	     					} 
	     	|    Argument COMMA ArgumentList  {
	     											Try {	     											
	     												$$ = createAlist();
	     												addObj($$);
	     												$$->e = $1;
	     												$$->next = $3;	     												
	     											} Catch (e){
	     												//freeAllObj();
	     												YYABORT;	     													     												
	     											}	     	
	     									  } 
		;

Argument   	:    Expr  {$$ = $1;} 
	   	; 


LocationPath    :    RelativeLocationPath	{ Try {
													$$ = createLocationPathExpr();
													addObj($$);
													addObj($$->ih);
													addObj($$->ih->storage);
													setStep($$, $1);
													optimize($$);
												  }
											  Catch (e) {
													//freeAllObj();
													YYABORT;
											  }
											}
		|    AbsoluteLocationPath	{		Try {  /*printf("absolute locationpath \n");*/
													$$ = createLocationPathExpr();
													addObj($$);
													addObj($$->ih);
													$$->pathType = ABSOLUTE_PATH;
													setStep($$, $1);
													optimize($$);
												  }
											  Catch (e) {
													//freeLocationPathExpr($$);
													//freeAllObj();
													YYABORT;
											  }
									}
		;

AbsoluteLocationPath :  SLASH 				{$$ = NULL;}
		|    SLASH RelativeLocationPath		{/*printf("slash detected \n");*/ $$ = $2;}
		|    AbbreviatedAbsoluteLocationPath	{$$ = $1;}
		;

RelativeLocationPath :  Step {$$ = $1;}
		|    Step SLASH RelativeLocationPath  { $1->nextS = $3;
												$3->prevS = $1;
												$$ = $1;		
												}
		|    AbbreviatedRelativeLocationPath {$$ = $1;}
		;


Step		:    AxisSpecifier NodeTest PredicateList {
														Try {
															$$ = createStep();
															addObj($$);
															setAxisType($$, $1);
															if ( ($1== AXIS_ATTRIBUTE
      														|| $1 == AXIS_NAMESPACE) && 
      														($2->testType>1)){
      																printf("%s axis can't operate on comment(), pi(), or text()\n", getAxisString($1));
      																throwException2(xpath_parse_exception," attr|namespace axis can't operate on comment(), pi(), or text()");
      	         											}
															setNodeTest($$, $2);
															setPredicate($$, $3);
															}
														Catch(e){
															//freeAllObj();
															YYABORT;
														}
														}
      		|    AbbreviatedStep			{$$ = $1;}
		;


NodeTest	:    NAME 				{ Try {
										$$ = createNodeTest();
										addObj($$);
										setTestType($$,NT_NAMETEST);
										//wprintf(L"$1.qname %ls\n",$1.qname);
										setNodeName($$,$1.qname);
										//addObj($1.qname);
										if ($1.localname!=NULL){
											setNodeNameNS($$,$1.prefix,$1.localname);
											//addObj($1.prefix);
											//addObj($1.localname);
											// the part for URL mapping goes here
											$$->URL = lookup(xpathNSList,$1.prefix);
											if ($$->URL == NULL){
												YYABORT;
											}
										}
										$1.prefix = $1.localname = $1.qname = NULL;
										
									  } Catch(e){
										//freeAllObj();
										YYABORT;
									  }
									} 
	 	|    NTEST				{ Try{
	 								$$ = createNodeTest();
	 								addObj($$);
	 								if ($1.nt !=3)
	 									setTestType($$,$1.nt);
	 								else {
	 									if ($1.arg == NULL)
	 										setTestType($$, $1.nt);
	 									else{
	 										setTestType($$,4);
	 										setNodeName($$, $1.arg);
	 									}
	 								}
	 								//setTestType($$,$1.nt);
	 								}
	 								Catch(e){
	 									//freeAllObj();
	 									YYABORT;
	 								}	 								
	 							}
		;

PredicateList	: { $$ = NULL;} 
	     	|    Predicate	PredicateList { $1->nextP = $2;
	     									$$ = $1;	
	     								  }  
		;

AxisSpecifier	:    AXISNAME				{$$  = $1;}
	      	|    AbbreviatedAxisSpecifier		{$$  = $1;}
	      	;

AbbreviatedAxisSpecifier  : { /*printf("abbreviated child axis \n");*/$$  = AXIS_CHILD;}
			  | AT	{$$ = AXIS_ATTRIBUTE;}
			;

AbbreviatedStep	:    DOT {Try{
								$$ = createStep();
								addObj($$);
								tmpNt = createNodeTest();
								addObj(tmpNt);
								setTestType(tmpNt, NT_NODE);
								setAxisType($$,AXIS_SELF);
								setNodeTest($$,tmpNt);
							}
							Catch(e){
								//freeAllObj();
								YYABORT;
							}
						}
		|    DDOT {
					Try{
						$$ = createStep();
						addObj($$);
						tmpNt = createNodeTest();
						addObj(tmpNt);
						setTestType(tmpNt, NT_NODE);
						setAxisType($$,AXIS_PARENT);
						setNodeTest($$,tmpNt);
					}
					Catch(e){
						//freeAllObj();
						YYABORT;
					}
		
					}
		;

AbbreviatedAbsoluteLocationPath : DSLASH RelativeLocationPath {
																Try{
																	$$ = createStep();
																	addObj($$);
																	setAxisType($$,AXIS_DESCENDANT_OR_SELF);
																	tmpNt = createNodeTest();
																	addObj(tmpNt);
																	setTestType(tmpNt, NT_NODE);
																	setNodeTest($$,tmpNt);
																	$$->nextS = $2;
																	$2->prevS = $$;																
																}Catch(e){
																	//freeAllObj();
																	YYABORT;																	
																}
																}
		;

AbbreviatedRelativeLocationPath : Step DSLASH RelativeLocationPath  {
																	Try{
																		$$= createStep();
																		addObj($$);
																		setAxisType($$,AXIS_DESCENDANT_OR_SELF);
																		tmpNt = createNodeTest();
																		addObj(tmpNt);
																		setTestType(tmpNt,NT_NODE);
																		setNodeTest($$, tmpNt);
																		$1->nextS = $$;
																		$$->prevS = $1;
																		$$->nextS = $3;
																		$3->prevS = $$;
																		$$ = $1;
																	}Catch(e){
																		//freeAllObj();
																		YYABORT;	
																	}
																	}
		;


Predicate 	:    LB Expr RB {
							   Try {
									$$ = createPredicate();
									addObj($$);
									$$->e = $2;
									if ($2->isFinal($2) && $2->isNumerical($2)){
										if ($$->d<1){
											throwException2(xpath_parse_excpetion,"Invalid index number <1");
										}
										$$->type = SIMPLE_P;
									}
									$$->requireContext=requireContextSize($2);

								} Catch(e){
									//freeAllObj();
									YYABORT;
								}
							}
	   	;

VariableReference : DOLLAR NAME {
								Try {
								    //addObj($2);
								    ex = getExprFromList(xpathExprList,$2.qname);
								    if (ex==NULL) {
										YYABORT;
								    }
								    $$ = (expr *)createVariableExpr($2.qname, ex);
									//$$ = createPredicate();
									addObj($$);
									//$$->e = $2;
								} Catch(e){
									//freeAllObj();
									YYABORT;
								}

							}
		  ;

FunctionName 	:  FNAME  {$$  = $1;}
	      ;
	      
%%
extern unsigned short *xpathInput;
extern unsigned short *xpathInputPtr;
extern unsigned short *xpathInputLimit;
extern int num_chars;
extern int isName;
expr *xpathParse(UCSChar *input, NsList *nl, ExprList *el){
	int l = wcslen(input);
	int i = 0;
	num_chars = 0;	
	xpathNSList = nl;
	xpathExprList = el;
	XMLChar_init();
	xpathInputPtr = xpathInput = (unsigned short *)malloc((l+1)<<1);
	// copy the string from wchar_t to unsigned short
	for (i=0;i<l;i++){
		xpathInput[i] = (unsigned short)input[i];
	}
	xpathInputLimit = (unsigned short *) (xpathInput + ((l)<<1));
	isName = 1;
	colonPosition = -1;
	if (yyparse() == 0){
		//isName = 1;
		//wprintf(L"yyparse return 0\n");
		free(xpathInput);
		xpathInput = xpathInputPtr = xpathInputLimit = NULL;
		resetObj();
		return x;
	} else {
		//wprintf(L"yyparse YYABORT\n");
		wprintf(L"xpath syntax error after or around the end of \n     %.*s\n", num_chars, input);
		free(xpathInput);
		xpathInput = xpathInputPtr = xpathInputLimit = NULL;
		//isName = 1;
		freeAllObj();
		return NULL;
	}
	
}

