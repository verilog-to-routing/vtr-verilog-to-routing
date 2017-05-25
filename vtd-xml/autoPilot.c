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
/*VTD-XML is protected by US patent 7133857, 7260652, an 7761459*/
#include "autoPilot.h"
/* This method insert a prefix/URL pair into the nsList, if there are prefix duplicates 
 the URL in the list is replaced with new URL*/
static _thread struct nsList *nl;
static _thread struct exprList *el;

static void insertItem(AutoPilot *ap, UCSChar *prefix, UCSChar *URL);
static void insertExpr(AutoPilot *ap, UCSChar *varName, expr *e);
static Boolean checkNsUniqueness(AutoPilot *ap, int index);

/* This method insert a prefix/URL pair into the nsList, if there are prefix duplicates 
   the URL is overwritten */
void insertItem(AutoPilot *ap, UCSChar *prefix, UCSChar *URL){
	NsList *tmp = nl;
	/* search first */
	if (tmp == NULL){
		tmp = (NsList *)malloc(sizeof(NsList));
		if (tmp == NULL){
			throwException2(out_of_mem, "out of memory in insertItem");
		}
		tmp->next = NULL; 
		tmp->prefix = prefix;
		tmp->URL = URL;
		nl = tmp;
		return;
	}else {
		if (wcscmp(tmp->prefix,prefix) == 0){
			tmp->URL = URL; /* overwritten */
			return;
		}
		while(tmp->next!=NULL){
			if (wcscmp(tmp->prefix,prefix) == 0){
				tmp->URL = URL; /* overwritten */
				return;
			}
			tmp = tmp->next;
		}
		
		tmp->next = (NsList *)malloc(sizeof(NsList));
		if (tmp->next == NULL){
			throwException2(out_of_mem, "out of memory in insertItem");
		}
		tmp->next->next = NULL; 
		tmp->next->prefix = prefix;
		tmp->next->URL = URL;
		return;
	}
}

/* This method insert a name/expr pair into the exprList, if there are name duplicates 
   the expr is overwritten */
void insertExpr(AutoPilot *ap, UCSChar *varName, expr *e){
	struct exprList *tmp = el;
	
	/* search first */
	if (tmp == NULL){
		tmp = (struct exprList *)malloc(sizeof(struct exprList));
		if (tmp == NULL){
			throwException2(out_of_mem, "out of memory in insertExpr");
		}
		tmp->next = NULL; 
		tmp->variableName = varName;
		tmp->ve = e;
		el = tmp;
		return;
	}else {
		if (wcscmp(tmp->variableName, varName) == 0){
			tmp->ve->freeExpr(tmp->ve);
			tmp->ve = e; /* overwritten */
			return;
		}
		while(tmp->next!=NULL){
			if (wcscmp(tmp->variableName,varName) == 0){
				tmp->ve->freeExpr(tmp->ve);
				tmp->ve = e; /* overwritten */
				return;
			}
			tmp = tmp->next;
		}
		
		tmp->next = (struct exprList *)malloc(sizeof(struct exprList));
		if (tmp->next == NULL){
			throwException2(out_of_mem, "out of memory in insertExpr");
		}
		tmp->next->next = NULL; 
		tmp->next->variableName = varName;
		tmp->next->ve = e;
		return;
	}
}

/* given a prefix, find the URL */
UCSChar *lookup(NsList *l, UCSChar *prefix){
	NsList *tmp = l;
	if (tmp==NULL)
		return NULL;
	while(tmp!= NULL){
		if (wcscmp(tmp->prefix,prefix)==0){
			return tmp->URL;
		}
		tmp = tmp->next;
	}
	return NULL;
}

/*given a variable Name, find the expression */
expr *getExprFromList(ExprList *el, UCSChar *varName){
	ExprList *tmp = el;
	if (tmp==NULL)
		return NULL;
	while(tmp!= NULL){
		if (wcscmp(tmp->variableName,varName)==0){
			return tmp->ve;
		}
		tmp = tmp->next;
	}
	return NULL;
}

/*create AutoPilot throw exception if allocation failed*/
AutoPilot *createAutoPilot(VTDNav *v){
	AutoPilot *ap = NULL;
	if (v == NULL){
		throwException2(invalid_argument,
			" createAutoPilot failed: can't take NULL VTDNav pointer");
		return NULL;
	}

	ap = (AutoPilot *)malloc(sizeof(AutoPilot));
	if (ap == NULL){
		throwException2(out_of_mem,
			"createAutoPilot failed");
		return NULL;
	}
    ap->elementName = NULL;
	ap->elementName2 = NULL;
	ap->localName = NULL;
	ap->URL = NULL;
    ap->vn = v;
    ap->it = UNDEFINED; /* initial state: not defined */
    ap->ft = TRUE;
	//ap->startIndex = -1;
	ap->xpe = NULL;
	nl = NULL;
	ap->contextCopy = NULL;
	ap->special = FALSE;
	ap->fib = NULL;
	ap->cachingOption = TRUE;
	ap->stackSize=0;
	return ap;
}

/* a argument-less constructor for autoPilot, 
   out_of_mem exception if allocation failed  */
AutoPilot *createAutoPilot2(){
	AutoPilot *ap = NULL;
	ap = (AutoPilot *)malloc(sizeof(AutoPilot));
	if (ap == NULL){
		throwException2(out_of_mem,
			"createAutoPilot failed");
		return NULL;
	}
    ap->elementName = NULL;
	ap->elementName2 = NULL;
	ap->localName = NULL;
	ap->URL = NULL;
    ap->vn = NULL;
    //depth = v.getCurrentDepth();
    ap->it = UNDEFINED; // not defined
    ap->ft = TRUE;
	ap->xpe = NULL;
	//ap->startIndex = -1;
	nl = NULL;
	ap->contextCopy = NULL;
	ap->special = FALSE;
	ap->fib = NULL;
	ap->cachingOption = TRUE;
	ap->stackSize=0;
	return ap;
}

/* free AutoPilot */
void freeAutoPilot(AutoPilot *ap){
	
	if (ap!=NULL){
		/*nl has been made a package scoped static variable*/
		/*NsList *tmp = nl;
		NsList *tmp2 = NULL;
		while(tmp!=NULL){
			tmp2 = tmp->next;
			free(tmp);
			tmp  = tmp2;			
		}*/
		if (ap->contextCopy!=NULL)
			free(ap->contextCopy);
		if (ap->xpe!= NULL)
			ap->xpe->freeExpr(ap->xpe);
		if (ap->fib!=NULL)
			freeFastIntBuffer(ap->fib);
		if (ap->elementName2!=NULL)
			free(ap->elementName2);
	}	
	free(ap);

}

void printExprString(AutoPilot *ap){
	if (ap->xpe!=NULL)
		ap->xpe->toString(ap->xpe,NULL);
}

/* Select an attribute name for iteration, * choose all attributes of an element
   this function is not called directly */
void selectAttr(AutoPilot *ap, UCSChar *an){
	if (an == NULL){
		 throwException2( invalid_argument,
			 " invalid argument for selectElement, elementName can't be NULL");
	}
	ap->it= ATTR;
    ap->ft = TRUE;
    ap->size = getTokenCount(ap->vn);
	ap->elementName = an;
}

/* Select an attribute name, both local part and namespace URL part*/
void selectAttrNS(AutoPilot *ap, UCSChar *URL, UCSChar *ln){
	if (ln == NULL){
		 throwException2( invalid_argument,
			 " invalid argument for selectElement, localName can't be NULL");
	}
	ap->it = ATTR_NS;
    ap->ft = TRUE;
    ap->localName = ln;
    ap->URL = URL;
}



/*Select the element name before iterating*/
void selectElement(AutoPilot *ap, UCSChar *en){
    if (en == NULL){
		 throwException2( invalid_argument,
			 " invalid argument for selectElement, elementName can't be NULL");
	 }
    ap->it = SIMPLE;
    ap->depth = getCurrentDepth(ap->vn);
    ap->elementName = en;
    ap->ft = TRUE;
}

/*Select the element name (name space version) before iterating.
// * URL, if set to *, matches every namespace
// * URL, if set to null, indicates the namespace is undefined.
// * localname, if set to *, matches any localname */
void selectElementNS(AutoPilot *ap, UCSChar *URL, UCSChar *ln){
    if (ln == NULL){
		 throwException2( invalid_argument,
			 " invalid argument for selectElementNS, localName can't be NULL");
	}
    ap->it = SIMPLE_NS;
    ap->depth = getCurrentDepth(ap->vn);
    ap->localName = ln;
	ap->URL = URL;
    ap->ft = TRUE;
}


/**
 * this function is intended to be called during XPath evaluation
 * Select all descendent elements along the descendent axis, without ns awareness
 * @param en
 */
void selectElement_D(AutoPilot *ap, UCSChar *en){
    if (en == NULL){
		 throwException2( invalid_argument,
			 " invalid argument for selectElement_D, elementName can't be NULL");
	 }
	ap->it = DESCENDANT;
	ap->depth = getCurrentDepth(ap->vn);
	ap->elementName = en;
	ap->ft = TRUE;
}

/**
 * this function is intended to be called during XPath evaluation
 * Select all descendent elements along the Descendent axis, with ns awareness
 */
void selectElementNS_D(AutoPilot *ap, UCSChar *URL, UCSChar *ln){
    if (ln == NULL){
		 throwException2( invalid_argument,
			 " invalid argument for selectElementNS_D, localName can't be NULL");
	}
	ap->it = DESCENDANT_NS;
    ap->depth = getCurrentDepth(ap->vn);
    ap->localName = ln;
	ap->URL = URL;
    ap->ft = TRUE;
}


/**
 * this function is intended to be called during XPath evaluation
 * Select all descendent elements along the following axis, without ns awareness
 * @param en
 */
void selectElement_F(AutoPilot *ap, UCSChar *en){
    if (en == NULL){
		throwException2( invalid_argument,
			" invalid argument for selectElement_F, elementName can't be NULL");
	 }
	ap->it = FOLLOWING;
	ap->ft = TRUE;
	ap->elementName = en;
}
/**
 * this function is intended to be called during XPath evaluation
 * Select all descendent elements along the following axis, with ns awareness
 */
void selectElementNS_F(AutoPilot *ap, UCSChar *URL, UCSChar *ln){
    if (ln == NULL){
		 throwException2( invalid_argument,
			 " invalid argument for selectElementNS_F, localName can't be NULL");
	}
	ap->it = FOLLOWING_NS;
    ap->ft= TRUE;
    ap->localName = ln;
    ap->URL = URL;
}

/**
 * this function is intended to be called during XPath evaluation
 * Select all descendent elements along the preceding axis, without ns awareness
 * @param en
 */
void selectElement_P(AutoPilot *ap, UCSChar *en){
	int i;
	int a = sizeof(int)* ap->vn->nestingLevel;
    if (en == NULL){
		 throwException2( invalid_argument,
			 " invalid argument for selectElement_P, elementName can't be NULL");
	 }
	ap->depth = getCurrentDepth(ap->vn);
	ap->it = PRECEDING;
    ap->ft = TRUE;	
	ap->elementName = en;
    ap->contextCopy = (int *)malloc(a); //(int[])vn.context.clone();
	memcpy(ap->contextCopy,ap->vn->context,a);
	ap->endIndex=getCurrentIndex2(ap->vn);
	for(i = ap->vn->context[0]+1 ; i<ap->vn->nestingLevel ; i++){
        ap->contextCopy[i]=-1;
    }
    ap->contextCopy[0]=ap->vn->rootIndex;
}

/**
 * this function is intended to be called during XPath evaluation
 * Select all descendent elements along the preceding axis, with ns awareness
 */
void selectElementNS_P(AutoPilot *ap, UCSChar *URL, UCSChar *ln){
	int i;
	int a = sizeof(int)* ap->vn->nestingLevel;
    if (ln == NULL){
		 throwException2( invalid_argument,
			 " invalid argument for selectElementNS_P, localName can't be NULL");
	}
	ap->depth = getCurrentDepth(ap->vn);
	ap->it = PRECEDING_NS;
    ap->ft = TRUE;	
	ap->URL = URL;
	ap->localName = ln;
    ap->contextCopy = (int *)malloc(a); //(int[])vn.context.clone();
	memcpy(ap->contextCopy,ap->vn->context,a);
	ap->endIndex=getCurrentIndex2(ap->vn);
	for(i = ap->vn->context[0]+1 ; i<ap->vn->nestingLevel ; i++){
        ap->contextCopy[i]=-1;
    }
    ap->contextCopy[0]=ap->vn->rootIndex;
}


/**
 * Setspecial is used by XPath evaluator to distinguish between
 * node() and *
 * node() corresponding to b= true;
 */
void setSpecial(AutoPilot *ap, Boolean b){
	ap->special = b;
}

/*Iterate over all the selected element nodes in document order.*/
Boolean iterateAP(AutoPilot *ap){
	switch (ap->it) {
		case SIMPLE :
			if (ap->vn->atTerminal)
				return FALSE;
			if (ap->ft == FALSE)
				return iterate(ap->vn, ap->depth, ap->elementName, ap->special);
			else {
				ap->ft = FALSE;
				if (ap->special || matchElement(ap->vn, ap->elementName)) {
					return TRUE;
				} else
					return iterate(ap->vn, ap->depth, ap->elementName,ap->special);
			}
		case SIMPLE_NS :
			if (ap->vn->atTerminal)
				return FALSE;
			if (ap->ft == FALSE)
				return iterateNS(ap->vn, ap->depth, ap->URL, ap->localName);
			else {
				ap->ft = FALSE;
				if (matchElementNS(ap->vn, ap->URL, ap->localName)) {
					
					return TRUE;
				} else
					return iterateNS(ap->vn, ap->depth, ap->URL, ap->localName);
			}
		case DESCENDANT:
			if (ap->vn->atTerminal)
				return FALSE;
			return iterate(ap->vn, ap->depth, ap->elementName, ap->special);
		case DESCENDANT_NS:
         	if (ap->vn->atTerminal)
         	    return FALSE;         	
         	return iterateNS(ap->vn, ap->depth, ap->URL, ap->localName);
		case FOLLOWING:
		   	if (ap->vn->atTerminal)
         	    return FALSE;
            if (ap->ft == FALSE)
                return iterate_following(ap->vn, ap->elementName, ap->special);
            else {
            	ap->ft = FALSE;
            	while(TRUE){
            		while (toElement(ap->vn, NEXT_SIBLING)){
            			 if (ap->special || matchElement(ap->vn,ap->elementName)) {                	
                            return TRUE;
            			 }
            			 return iterate_following(ap->vn, ap->elementName, ap->special);
            		}
                    if (toElement(ap->vn, PARENT)==FALSE){
                        return FALSE;
                    } 
            	}
            }
		case FOLLOWING_NS:
        	if (ap->vn->atTerminal)
         	    return FALSE;
         	if (ap->ft == FALSE)
                return iterate_followingNS(ap->vn,ap->URL,ap->localName);
            else {
            	ap->ft = FALSE;
            	while(TRUE){
            		while (toElement(ap->vn, NEXT_SIBLING)){
            			 if (matchElementNS(ap->vn, ap->URL,ap->localName)) {                	
                            return TRUE;
            			 }
						 return iterate_followingNS(ap->vn, ap->URL,ap->localName);
            		}
                    if (toElement(ap->vn,PARENT)==FALSE){
						return FALSE;
                    } 
            	}
            }
		case PRECEDING:
			if (ap->vn->atTerminal)
         	    return FALSE;
			if (ap->ft){
				ap->ft = FALSE;
				toElement(ap->vn,ROOT);
			}
         	return iterate_preceding(ap->vn, ap->elementName, ap->contextCopy, ap->endIndex);

		case PRECEDING_NS:
			if (ap->vn->atTerminal)
         	    return FALSE;
			if (ap->ft){
				ap->ft = FALSE;
				toElement(ap->vn,ROOT);
			}
         	return iterate_precedingNS(ap->vn, ap->URL,ap->localName,ap->contextCopy,ap->endIndex);
		default :
			throwException2(pilot_exception,
				"unknow iteration type for iterateAP");
			return FALSE;
	}
}

/**
 * This method is meant to be called after calling
 * selectAttr() or selectAttrNs(), it will return the 
 * vtd index attribute name or -1 if there is none left
 * @return vtd index attribute name or -1 if there is none left
 */
int iterateAttr(AutoPilot *ap){
	int i;
	switch(ap->it){
			case ATTR:
				if (wcscmp(ap->elementName,L"*")==0){
					if (ap->ft != FALSE){
						ap->ft = FALSE;
						ap->index = getCurrentIndex2(ap->vn)+1;
					} else
						ap->index +=2;
					if (ap->vn->ns == FALSE){
						while(ap->index<ap->size){
							tokenType type = getTokenType(ap->vn,ap->index);
							if (type == TOKEN_ATTR_NAME
								|| type == TOKEN_ATTR_NS){
									//ap->vn->LN = ap->index;
									return ap->index;
								}else{   	    				
									return -1;
								}
						}
						return -1;
					}else {						
						while(ap->index<ap->size){
							tokenType type = getTokenType(ap->vn,ap->index);
							if (type == TOKEN_ATTR_NAME
								|| type == TOKEN_ATTR_NS){
									if (type == TOKEN_ATTR_NAME){
										//ap->vn->LN = ap->index;
										return ap->index;
									}
									else 
										ap->index += 2;	    						
								}else{   	    				
									return -1;
								}
						}
						return -1;
					}
				}else{
					if (ap->ft == FALSE){
						return -1;
					} else {
						ap->ft = FALSE;
						i = getAttrVal(ap->vn,ap->elementName);
						if(i!=-1){
							//ap->vn->LN = i-1;
							return i-1;
						}
						else 
							return -1;
					}   	    			
				}
			case ATTR_NS:
				if (ap->ft == FALSE){
					return -1;
				} else {
					ap->ft = FALSE;
				    i = getAttrValNS(ap->vn,ap->URL,ap->localName);
					if(i!=-1){
						//ap->vn->LN = i-1;
						return i-1;
					}
					else 
						return -1;
				} 
			default:
				throwException2(pilot_exception,
					"unknow iteration type for iterateAP");
				return -1;
	}
	
}


/* This method implements the attribute axis for XPath*/
int iterateAttr2(AutoPilot *ap){
	int i;
	switch(ap->it){
			case ATTR:
				if (wcscmp(ap->elementName,L"*")==0){
					if (ap->ft != FALSE){
						ap->ft = FALSE;
						ap->index = getCurrentIndex2(ap->vn)+1;
					} else
						ap->index +=2;
					if (ap->vn->ns == FALSE){
						while(ap->index<ap->size){
							tokenType type = getTokenType(ap->vn,ap->index);
							if (type == TOKEN_ATTR_NAME
								|| type == TOKEN_ATTR_NS){
									ap->vn->LN = ap->index;
									return ap->index;
								}else{   	    				
									return -1;
								}
						}
						return -1;
					}else {						
						while(ap->index<ap->size){
							tokenType type = getTokenType(ap->vn,ap->index);
							if (type == TOKEN_ATTR_NAME
								|| type == TOKEN_ATTR_NS){
									if (type == TOKEN_ATTR_NAME){
										ap->vn->LN = ap->index;
										return ap->index;
									}
									else 
										ap->index += 2;	    						
								}else{   	    				
									return -1;
								}
						}
						return -1;
					}
				}else{
					if (ap->ft == FALSE){
						return -1;
					} else {
						ap->ft = FALSE;
						i = getAttrVal(ap->vn,ap->elementName);
						if(i!=-1){
							ap->vn->LN = i-1;
							return i-1;
						}
						else 
							return -1;
					}   	    			
				}
			case ATTR_NS:
				if (ap->ft == FALSE){
					return -1;
				} else {
					ap->ft = FALSE;
				    i = getAttrValNS(ap->vn,ap->URL,ap->localName);
					if(i!=-1){
						ap->vn->LN = i-1;
						return i-1;
					}
					else 
						return -1;
				} 
			default:
				throwException2(pilot_exception,
					"unknow iteration type for iterateAP");
				return -1;
	}
	
}

/*
 * This function selects the string representing XPath expression
 * Usually evalXPath is called afterwards
 */
Boolean selectXPath(AutoPilot *ap, UCSChar *s){
	if (s==NULL){
		throwException2(xpath_parse_exception,
			" xpath input string can't be NULL ");
		return FALSE;
	}
	if(ap->xpe!=NULL)
		ap->xpe->freeExpr(ap->xpe);
	XMLChar_init();

	ap->xpe = xpathParse(s, nl,el);
	ap->ft = TRUE;
	if (ap->xpe == NULL){
		throwException2(xpath_parse_exception, "Invalid XPath expression");
		return FALSE;
	}
	if (ap->cachingOption)
		ap->xpe->markCacheable(ap->xpe);
	return TRUE;
	
}

/*
 * Evaluate XPath to a boolean
 */ 
Boolean evalXPathToBoolean(AutoPilot *ap){
	return	ap->xpe->evalBoolean(ap->xpe,ap->vn);
}

/*
 * Evaluate XPath to a String
 */
UCSChar* evalXPathToString(AutoPilot *ap){
	return ap->xpe->evalString(ap->xpe,ap->vn);
}

/* 
 * Evaluate XPath to a number
 */
double evalXPathToNumber(AutoPilot *ap){
	return ap->xpe->evalNumber(ap->xpe,ap->vn);
}

/*
 * Evaluate XPath
 */
int evalXPath(AutoPilot *ap){	
	if (ap->xpe != NULL){
		if (ap->ft == TRUE){
			if (ap->vn != NULL){
	            ap->stackSize = ap->vn->contextBuf2->size;
			}
			ap->ft = FALSE;
			ap->xpe->adjust(ap->xpe, ap->vn->vtdSize);
		}
		return ap->xpe->evalNodeSet(ap->xpe,ap->vn);
	}
	throwException2(other_exception,"xpe is NULL in autoPilot");
	return -1;
}

/*
 * Reset XPath
 */
void resetXPath(AutoPilot *ap){
	if (ap->xpe != NULL && ap->vn!=NULL){
		ap->xpe->reset(ap->xpe,ap->vn);
		ap->vn->contextBuf2->size = ap->stackSize;
		ap->ft = TRUE;
		if (ap->cachingOption)
			ap->xpe->clearCache(ap->xpe);
	}
}

/*
 * This function creates URL ns prefix 
 *  and is intended to be called prior to selectXPath
 */
void declareXPathNameSpace(AutoPilot *ap, UCSChar* prefix, UCSChar *URL){
	if(ap!=NULL)
		insertItem(ap,prefix,URL);
}

/**
 * 
 */
void bind(AutoPilot *ap, VTDNav *new_vn){
	ap->elementName = NULL;
	ap->elementName2 = NULL;
    ap->vn = new_vn;
	ap->it = UNDEFINED; 
    ap->ft = TRUE;
    ap->size = 0;
    ap->special = FALSE;
}


/* Clear the namespace prefix URL bindings in the global list */
void clearXPathNameSpaces(){
	/*nl has been made a package scoped static variable*/
		NsList *tmp = nl;
		NsList *tmp2 = NULL;
		while(tmp!=NULL){
			tmp2 = tmp->next;
			free(tmp);
			tmp  = tmp2;			
		}
}

/* Clear the variable name and exprs in the global list  */
void clearVariableExprs(){
		ExprList *tmp = el;
		ExprList *tmp2 = NULL;
		while(tmp!=NULL){
			tmp->ve->freeExpr(tmp->ve);
			tmp2 = tmp->next;			
			free(tmp);
			tmp  = tmp2;			
		}
}

/* Declare the variable name and expression binding*/
void declareVariableExpr(AutoPilot *ap, UCSChar* varName, UCSChar* varExpr){
	exception e;
	expr* tmp = NULL;
	if (varExpr==NULL || varName==NULL){
		throwException2(xpath_parse_exception,
			" xpath input string or variable name can't be NULL ");
		//return FALSE;
	}
	tmp = getExprFromList(el,varName);
	if(tmp!=NULL)
		tmp->freeExpr(tmp);
	Try{
		tmp = xpathParse(varExpr, nl,el);
		//ap->ft = TRUE;
		if (tmp == NULL){
			throwException2(xpath_parse_exception, "variable expr declaration failed ");
		}
		insertExpr(ap,varName,tmp);
	}Catch(e){
		freeAllObj();
		Throw e;
	}
}

void selectNameSpace(AutoPilot *ap, UCSChar *en){
	if (en == NULL)
		throwException2(invalid_argument,"namespace name can't be null");
	ap->it = NAMESPACE;
    ap->ft = TRUE;
    ap->size = getTokenCount(ap->vn);
	ap->elementName = en;
	if (wcscmp(en,L"*")!=0){
		UCSChar *tmp = malloc(sizeof(UCSChar)*(wcslen(en)+7));
		if (tmp == NULL)
			throwException2(out_of_mem,"allocating string failed in selectNameSpace()");
		wcscpy(tmp,L"xmlns:");
		wcscpy(tmp+6,en);
		*(tmp+(wcslen(en)+6))= 0;
		ap->elementName2=tmp;
	}
    if (ap->fib== NULL)
    	ap->fib = createFastIntBuffer2(4);
    else 
    	clearFastIntBuffer(ap->fib);
}

int iterateNameSpace(AutoPilot *ap){
		if (ap->vn->ns == FALSE)
			return -1;
		if (ap->ft != FALSE) {
			ap->ft = FALSE;
			ap->index = getCurrentIndex2(ap->vn) + 1;
		} else
			ap->index += 2;

		while (ap->index < ap->size) {
			tokenType type = getTokenType(ap->vn,ap->index);
			if (type == TOKEN_ATTR_NAME || type == TOKEN_ATTR_NS) {
				if (type == TOKEN_ATTR_NS){ 
					if  (wcscmp(ap->elementName,L"*") == 0   
						|| matchRawTokenString(ap->vn,ap->index, ap->elementName2)
				    ){
				    	// check to see if the namespace has appeared before
				    	if (checkNsUniqueness(ap,ap->index)){
				    		ap->vn->LN = ap->index;
				    		ap->vn->atTerminal = TRUE;
				    		return ap->index;
				    	}
				    }
				} 
				ap->index += 2;
			} else {
				ap->vn->atTerminal = FALSE;
				if (toElement(ap->vn, P) == FALSE) {
					return -1;
				} else {
					ap->index = getCurrentIndex2(ap->vn) + 1;
				}
			}
		}

		return -1;
}

Boolean checkNsUniqueness(AutoPilot *ap, int index){
	int j;
	for (j=0;j<ap->fib->size;j++){
		if (compareTokens(ap->vn,intAt(ap->fib,j), ap->vn, index)==0)
			return FALSE;
	}		
	appendInt(ap->fib,index);
	return TRUE;
}

void selectNode(AutoPilot *ap){
	ap->ft = TRUE;
	ap->depth = getCurrentDepth(ap->vn);
	ap->it = SIMPLE_NODE;
}
void selectPrecedingNode(AutoPilot *ap){
	int i;
	int a = sizeof(int)* ap->vn->nestingLevel;
	ap->ft = TRUE;
	ap->depth = getCurrentDepth(ap->vn);
	ap->contextCopy = (int *)malloc(a); //(int[])vn.context.clone();
	memcpy(ap->contextCopy,ap->vn->context,a);
	   
	if (ap->contextCopy[0]!=-1){
	   for (i=ap->contextCopy[0]+1;i<ap->vn->nestingLevel;i++){
	 		ap->contextCopy[i]=0;
		}
	}//else{
	   //   for (int i=1;i<contextCopy.length;i++){
	   //	   contextCopy[i]=0;
	   //	   }
	   //}
	 ap->it = PRECEDING_NODE;
	 ap->endIndex = getCurrentIndex(ap->vn);
}

void selectFollowingNode(AutoPilot *ap){
	 ap->ft = TRUE;
	 ap->depth = getCurrentDepth(ap->vn);
	 ap->it = FOLLOWING_NODE;
}

void selectDescendantNode(AutoPilot *ap){
	 ap->ft = TRUE;
	 ap->depth = getCurrentDepth(ap->vn);
	 ap->it = DESCENDANT_NODE;
}


Boolean iterateAP2(AutoPilot *ap){
switch (ap->it) {
		case SIMPLE_NODE:
			if (ap->ft && ap->vn->atTerminal)
				return FALSE;
			if (ap->ft){
				ap->ft =FALSE;
				return TRUE;
			}
			return iterateNode(ap->vn,ap->depth);
			
		case DESCENDANT_NODE:
			if (ap->ft&&ap->vn->atTerminal)
				return FALSE;
			else{
				ap->ft=FALSE;
				return iterateNode(ap->vn,ap->depth);
			}
         	
		case FOLLOWING_NODE:
			if (ap->ft){
				Boolean b= FALSE;
				do{
					b = toNode(ap->vn,NEXT_SIBLING);
					if (b){
						ap->ft = FALSE;
						return TRUE;
					}else{
						b = toNode(ap->vn,PARENT);
					}
				}while(b);
				return FALSE;
			}			
			return iterate_following_node(ap->vn);
			
		case PRECEDING_NODE:
			if(ap->ft){
				ap->ft = FALSE;
				toNode(ap->vn,ROOT);
				toNode(ap->vn,PARENT);	
			}
			return iterate_preceding_node(ap->vn,ap->contextCopy,ap->endIndex);
		//case 
		default :
			throwException2(pilot_exception," iteration action type undefined");
			return FALSE;
}
}

void enableCaching(AutoPilot *ap, Boolean state){
	ap->cachingOption = state;
}

VTDNav* ap_getNav(AutoPilot *ap) {
	return ap->vn;
}