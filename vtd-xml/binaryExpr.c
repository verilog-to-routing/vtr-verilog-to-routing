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
#include "xpath.h"
#include <math.h>
#define BUF_SZ_EXP 7
static Boolean computeComp(binaryExpr *be, opType op,VTDNav *vn);
static Boolean compNumericalNodeSet(binaryExpr *be, expr* left, expr* right, VTDNav *vn, opType op);
static Boolean compNodeSetNumerical(binaryExpr *be, expr* left, expr* right, VTDNav *vn, opType op);
static Boolean compStringNodeSet(binaryExpr *be, expr* left, expr* right, VTDNav *vn, opType op);
static Boolean compNodeSetString(binaryExpr *be, expr* left, expr* right, VTDNav *vn, opType op);
static Boolean compNodeSetNodeSet(binaryExpr *be, expr* left, expr* right, VTDNav *vn, opType op);
static Boolean compNumbers(binaryExpr *be, double d1, double d2, opType op);
static Boolean compareVNumber1(binaryExpr *be, int i, VTDNav *vn, double d, opType op);
static Boolean compareVNumber2(binaryExpr *be, int i, VTDNav *vn, double d, opType op);
static int getStringVal(VTDNav *vn, int i);
static Boolean compareVString1(binaryExpr *be,int k, VTDNav *vn, UCSChar *s, opType op);
static Boolean compareVString2(binaryExpr *be,int k, VTDNav *vn, UCSChar *s, opType op);
static Boolean compareVV(binaryExpr *be,int k,  VTDNav *vn, int j,opType op);

static Boolean compareVString1(binaryExpr *be,int k, VTDNav *vn, UCSChar *s, opType op){
	int i = compareTokenString(vn,k, s);
	switch (i) {
		case -1:
			if (op == OP_NE || op == OP_LT || op == OP_LE) {
				return TRUE;
			}
			break;
		case 0:
			if (op == OP_EQ || op == OP_LE || op == OP_GE) {
				return TRUE;
			}
			break;
		case 1:
			if (op == OP_NE || op == OP_GE || op == OP_GT) {
				return TRUE;
			}
	}
	return FALSE;
}
static Boolean compareVString2(binaryExpr *be,int k, VTDNav *vn, UCSChar *s, opType op){
	int i = compareTokenString(vn,k, s);
	switch(i){
			case -1:
				if (op== OP_NE || op == OP_GT || op == OP_GE){
					return TRUE;
				}
				break;
			case 0:
				if (op==OP_EQ || op == OP_LE || op == OP_GE ){
					return TRUE;
				}
				break;
			case 1:
				if (op == OP_NE || op==OP_LE  || op == OP_LT ){
					return TRUE;
				}
	}
	return FALSE;

}
static Boolean compareVV(binaryExpr *be,int k,  VTDNav *vn, int j,opType op){
	int i = XPathStringVal_Matches2(vn,k, vn, j);
	switch(i){

			case 1:
				if (op == OP_NE || op==OP_GE  || op == OP_GT ){
					return TRUE;
				}
				break;
			case 0:
				if (op==OP_EQ || op == OP_LE || op == OP_GE ){
					return TRUE;
				}
				break;
			case -1:
				if (op== OP_NE || op == OP_LT || op == OP_LE){
					return TRUE;
				}
	}
	return FALSE;
}

static Boolean compareVNumber1(binaryExpr *be, int i, VTDNav *vn, double d , opType op){
		double d1 /*= parseDouble(vn,i)*/;
		tokenType t = getTokenType(vn,i);
		if (t == TOKEN_STARTING_TAG || t == TOKEN_DOCUMENT) {
			d1 = XPathStringVal2Double(vn,i);
		}
		else {
			int k= getStringVal(vn, i);
			d1 = parseDouble(vn,k);
		}
	    switch (op){
	    	case OP_EQ:
	    	    return d == d1;
	    	case OP_NE:
	    		return d != d1;
	    	case OP_GE:
	    	    return d >= d1;
	    	case OP_LE:
	    	    return d <= d1;
	    	case OP_GT:
	    	    return d > d1;
	    	default:
	    	    return d < d1;
	    }
}

static Boolean compareVNumber2(binaryExpr *be, int i, VTDNav *vn, double d, opType op){
	    double d1 = parseDouble(vn,i);
		tokenType t = getTokenType(vn, i);
		if (t == TOKEN_STARTING_TAG || t == TOKEN_DOCUMENT) {
			d1 = XPathStringVal2Double(vn, i);
		}
		else {
			int k = getStringVal(vn, i);
			d1 = parseDouble(vn, k);
		}
	    switch (op){
	    	case OP_EQ:
	    	    return d1 == d;
	    	case OP_NE:
	    		return d1 != d;
	    	case OP_GE:
	    	    return d1 >= d;
	    	case OP_LE:
	    	    return d1 <= d;
	    	case OP_GT:
	    	    return d1 > d;
	    	default:
	    	    return d1 < d;
	    }
}

static Boolean compNumbers(binaryExpr *be, double d1, double d2, opType op){
	    switch (op) {
        case OP_LE:
            return d1 <= d2;
        case OP_GE:
            return d1 >= d2;
        case OP_LT:
            return d1 < d2;
        case OP_GT:
            return d1 > d2;
        }
        return FALSE;
}
static int getStringVal(VTDNav *vn, int i){
	int i1,t = getTokenType(vn,i);
	if (t == TOKEN_STARTING_TAG){
		i1 = getText(vn);
		return i1;
	}
	else if (t == TOKEN_ATTR_NAME
		|| t == TOKEN_ATTR_NS || t==TOKEN_PI_NAME)
		return i+1;
	else
		 return i;
}

static Boolean compNodeSetNumerical(binaryExpr *be, expr* left, expr* right, VTDNav *vn, opType op){
	exception e;
	int i,stackSize;
	double d;
	Try {
		   d = be->right->evalNumber(be->right,vn);
            push2(vn);
			stackSize = vn->contextBuf2->size;
            while ((i = be->left->evalNodeSet(be->left,vn)) != -1) {
                //i1 = getStringVal(vn,i);
                if (compareVNumber2(be,i,vn,d,op)){
                    be->left->reset(be->left,vn);
                    vn->contextBuf2->size = stackSize;
                    pop2(vn);
                    return TRUE;
                }
            }
            vn->contextBuf2->size = stackSize;
            pop2(vn);
            be->left->reset(be->left,vn);
            return FALSE;
	} Catch (e) {
		//fib1.clear();
		//fib2.clear();
		e.et = other_exception;
		e.msg = "Undefined behavior in evalBoolean_be";
		Throw e;
		//throw new RuntimeException("Undefined behavior");
	}
	return FALSE;
}

static Boolean compNumericalNodeSet(binaryExpr *be, expr* left, expr* right, VTDNav *vn, opType op){
	exception e;
	int i,stackSize;
	double d;
	Try {
		   d = be->left->evalNumber(be->left,vn);
            push2(vn);
			stackSize = vn->contextBuf2->size;
            while ((i = be->right->evalNodeSet(be->right,vn)) != -1) {
                //i1 = getStringVal(vn,i);
                if (/*i1!=-1 &&*/ compareVNumber1(be,i,vn,d,op)){
                    be->right->reset(be->right,vn);
                    vn->contextBuf2->size = stackSize;
                    pop2(vn);
                    return TRUE;
                }
            }
            vn->contextBuf2->size = stackSize;
            pop2(vn);
            be->right->reset(be->right,vn);
            return FALSE;
	} Catch (e) {
		e.et = other_exception;
		e.msg = "Undefined behavior in evalBoolean_be";
		Throw e;
	}
	return FALSE;
}

static Boolean compEmptyNodeSet(opType op, UCSChar *s){
	if (op == OP_NE ){
	        if (wcslen(s)==0) {
	            return FALSE;
	        } else
	            return TRUE;
	    }else{
	        if (wcslen(s)==0) {
	            return TRUE;
	        } else
	            return FALSE;
	    }
}


static Boolean compStringNodeSet(binaryExpr *be, expr* left, expr* right, VTDNav *vn, opType op){
	exception e;
	int i,i1,stackSize;
	UCSChar *s=NULL;
	Boolean b;
	Try {
		    s = be->left->evalString(be->left,vn);
            push2(vn);
            stackSize = vn->contextBuf2->size;
            while ((i = be->right->evalNodeSet(be->right,vn)) != -1) {
				tokenType t = getTokenType(vn,i);
				if (t != TOKEN_STARTING_TAG
					&& t != TOKEN_DOCUMENT) {
					i1 = getStringVal(vn, i);
					if (i1 != -1) {
						b = compareVString2(be, i1, vn, s, op);
						if (b) {
							be->right->reset(be->right, vn);
							vn->contextBuf2->size = stackSize;
							pop2(vn);
							free(s);
							return b;
						}
					}
				}
				else {
					Boolean b = XPathStringVal_Matches(vn,i, s);
					if (b) {
						be->right->reset(be->right, vn);
						vn->contextBuf2->size = stackSize;
						pop2(vn);
						free(s);
						return b;
					}
				}
            }
            vn->contextBuf2->size = stackSize;
            pop2(vn);
            be->right->reset(be->right,vn);
			//b = compEmptyNodeSet(op,s);
			free(s);
            return FALSE; //b;
	} Catch ( e) {
		e.et = other_exception;
		e.msg = "undefined run time behavior in computerEQNE";
		Throw e;
	}
	return FALSE;
}
static Boolean compNodeSetString(binaryExpr *be, expr* left, expr* right, VTDNav *vn, opType op){
	exception e;
	int i,i1 = 0,stackSize;
	UCSChar *s=NULL;
	Boolean b;
	Try {
		    s = be->right->evalString(be->right,vn);
            push2(vn);
            stackSize = vn->contextBuf2->size;
			while ((i = be->left->evalNodeSet(be->left, vn)) != -1) {
				tokenType t = getTokenType(vn, i);
				if (t != TOKEN_STARTING_TAG
					&& t != TOKEN_DOCUMENT) {

					i1 = getStringVal(vn, i);
					if (i1 != -1) {
						b = compareVString2(be, i1, vn, s, op);
						if (b) {
							be->left->reset(be->left, vn);
							vn->contextBuf2->size = stackSize;
							pop2(vn);
							free(s);
							return b;
						}
					}
				}
				else {
					Boolean b = XPathStringVal_Matches(vn,i, s);
					if (b) {
						be->left->reset(be->left, vn);
						vn->contextBuf2->size = stackSize;
						pop2(vn);
						free(s);
						return b;
					}
				}
			}
				
            vn->contextBuf2->size = stackSize;
            pop2(vn);
            be->left->reset(be->left,vn);
            //b = compEmptyNodeSet(op, s);
			free(s);
			return FALSE;//b;
	} Catch ( e) {
		e.et = other_exception;
		e.msg = "undefined run time behavior in computerEQNE";
		Throw e;
	}
	return FALSE;
}

static Boolean compNodeSetNodeSet(binaryExpr *be, expr* left, expr* right, VTDNav *vn, opType op){
	exception e;
	int i,i1,k,s1,stackSize;
	Try {
		if (be->fib1 == NULL)
			be->fib1 = createFastIntBuffer2(BUF_SZ_EXP);

	          push2(vn);
	          stackSize = vn->contextBuf2->size;
	          while ((i = be->left->evalNodeSet(be->left,vn)) != -1) {
	              i1 = getStringVal(vn,i);
	              if (i1 != -1)
	              appendInt(be->fib1,i1);
	          }
			  be->left->reset(be->left,vn);
	          vn->contextBuf2->size = stackSize;
	          pop2(vn);
	          push2(vn);
	          stackSize = vn->contextBuf2->size;
	          while ((i = be->right->evalNodeSet(be->right,vn)) != -1) {
	              i1 = getStringVal(vn,i);
	              if (i1 != -1){
	                  s1 = be->fib1->size;
	                  for (k = 0; k < s1; k++) {
		                  Boolean b = compareVV(be,intAt(be->fib1,k),vn,i1,op);
		                  if (b){
		                      clearFastIntBuffer(be->fib1);
		                      vn->contextBuf2->size = stackSize;
		        	          pop2(vn);
		        	          be->right->reset(be->right,vn);
		                      return TRUE;
		                  }
		              }
	              }
	          }
	          vn->contextBuf2->size = stackSize;
	          pop2(vn);
	          be->right->reset(be->right,vn);
	          clearFastIntBuffer(be->fib1);
	          return FALSE;

	} Catch (e) {
		if (e.et == out_of_mem){
			Throw e;
		}
		if(be->fib1!=NULL)
			clearFastIntBuffer(be->fib1);

		e.et = other_exception;
		e.msg = "undefined run time behavior in computerEQNE";
		Throw e;
	}
	return FALSE;
}

Boolean computeComp(binaryExpr *be, opType op,VTDNav *vn){

	Boolean btemp = FALSE;
	UCSChar *st1=NULL, *st2=NULL;
	switch (be->ct) {
	case NS_NS:return compNodeSetNodeSet(be,be->left, be->right, vn, op);
	case NS_N:return compNodeSetNumerical(be,be->left, be->right, vn, op);
	case NS_S:return compNodeSetString(be,be->left, be->right, vn, op);
		//case NS_B:
	case N_NS:return compNumericalNodeSet(be,be->left, be->right, vn, op);
		//case N_N:   break;
		//case N_S:   break;
		//case N_B:
	case S_NS:return compStringNodeSet(be,be->left, be->right, vn, op);
		//case S_N:
		//case S_S:
		//case S_B:
		//case B_NS:
		//case B_N:
		//case B_S:
		//default:break;
	}
	//if (be->left->isNodeSet(be->left) && be->right->isNodeSet(be->right)) {
	//	return compNodeSetNodeSet(be, be->left, be->right, vn, be->op);
	//} else {
	//	// first argument is always numerical, second a node set
	//	if (be->left->isNumerical(be->left) && be->right->isNodeSet(be->right)){
	//		return compNumericalNodeSet(be, be->left, be->right, vn,be->op);
	//	}
	//    if (be->left->isNodeSet(be->left) && be->right->isNumerical(be->right)){
	//		return compNodeSetNumerical(be, be->left, be->right, vn,be->op);
	//	}
	//	// first argument is always String, second a node set
	//	if (be->left->isString(be->left) && be->right->isNodeSet(be->right)){
	//		return compStringNodeSet(be, be->left, be->right,vn,be->op);
	//	}
	//	if (be->left->isNodeSet(be->left) && be->right->isString(be->right)){
	//		return compNodeSetString(be, be->left, be->right,vn,be->op);
	//	}

	//}
	if (op == OP_EQ || op == OP_NE){
	if (be->left->isBoolean(be->left) || be->right->isBoolean(be->right)){
		if (op == OP_EQ)
			return be->left->isBoolean(be->left) == be->right->isBoolean(be->right);
		else
			return be->left->isBoolean(be->left) != be->right->isBoolean(be->right);
	}

	if (be->left->isNumerical(be->left) || be->right->isNumerical(be->right)){
		if (op == OP_EQ)
			return be->left->evalNumber(be->left,vn) == be->right->evalNumber(be->right,vn);
		else
			return be->left->evalNumber(be->left,vn) != be->right->evalNumber(be->right,vn);
	}
	st1 = be->left->evalString(be->left,vn);
	st2 = be->right->evalString(be->right,vn);
	/*if (st1 == NULL || st2 == NULL){
		btemp = FALSE;
	}else{*/
	btemp = wcscmp(st1, st2);
	/*}*/
	free(st1);
	free(st2);
	if (op == OP_EQ){
		return btemp==0;
	}
	else
		return btemp!=0;
	}
	return compNumbers(be, be->left->evalNumber(be->left,vn),
		be->right->evalNumber(be->right,vn),op);
}

binaryExpr *createBinaryExpr(expr *e1, opType op, expr *e2){
	binaryExpr *be = malloc(sizeof(binaryExpr));
	if (be == NULL){
		throwException2(out_of_mem,
			"binaryExpr allocation failed");
		return NULL;
	}
	be->freeExpr = (free_Expr) &freeBinaryExpr;
	be->evalBoolean = (eval_Boolean)&evalBoolean_be;
	be->evalNodeSet = (eval_NodeSet)&evalNodeSet_be;
	be->evalNumber  = (eval_Number)&evalNumber_be;
	be->evalString  = (eval_String)&evalString_be;
	be->isNumerical = (is_Numerical)&isNumerical_be;
	be->isBoolean = (is_Boolean)&isBoolean_be;
	be->isString =  (is_String)&isString_be;
	be->isNodeSet = (is_NodeSet)&isNodeSet_be;
	be->requireContextSize = (require_ContextSize) &requireContextSize_be;
	be->setContextSize = (set_ContextSize)&setContextSize_be;
	be->setPosition = (set_Position)&setPosition_be;
	be->reset = (reset_)&reset_be;
	be->toString = (to_String)&toString_be;
	be->adjust = (adjust_)&adjust_be;
	be->isFinal= (isFinal_)&isFinal_be;
	be->markCacheable = (markCacheable_)&markCacheable_be;
	be->markCacheable2 = (markCacheable2_)&markCacheable2_be;
	be->clearCache = (clearCache_)&clearCache_be;
	be->getFuncOpCode = (getFuncOpCode_)&getFuncOpCode;
	be->left = e1;
	be->op = op;
	be->right = e2;
	be->fib1 = NULL;
	switch(be->op){
	 	case OP_ADD:
		case OP_SUB:
		case OP_MULT:
		case OP_DIV:
		case OP_MOD: be->isNum = TRUE; be->isBool = FALSE; break;
		case OP_OR :
		case OP_AND:
		case OP_EQ:
		case OP_NE:
		case OP_LE:
		case OP_GE:
		case OP_LT:
		default: be->isNum= FALSE; be->isBool = TRUE;
	}
	return be;
}

void freeBinaryExpr(binaryExpr *be){
	if (be == NULL) return;
	be->left->freeExpr(be->left);
	be->right->freeExpr(be->right);
	freeFastIntBuffer(be->fib1);
	free(be);
}

int	evalNodeSet_be (binaryExpr *be,VTDNav *vn){
	throwException2(xpath_eval_exception,
		"can't evaluate nodeset on a binary expr");
	return -1;
}

double	evalNumber_be (binaryExpr *be,VTDNav *vn){
	switch(be->op){
			case OP_ADD: return be->left->evalNumber(be->left,vn) + be->right->evalNumber(be->right,vn);
			case OP_SUB: return be->left->evalNumber(be->left,vn) - be->right->evalNumber(be->right,vn);
			case OP_MULT:return be->left->evalNumber(be->left,vn) * be->right->evalNumber(be->right,vn);
			case OP_DIV: return be->left->evalNumber(be->left,vn) / be->right->evalNumber(be->right,vn);
			case OP_MOD: return fmod(be->left->evalNumber(be->left,vn), be->right->evalNumber(be->right,vn));
			default	: if (evalBoolean_be(be,vn) == TRUE)
						  return 1;
				return 0;

	}
}


UCSChar* evalString_be  (binaryExpr *be,VTDNav *vn){
	double n = 0.0;
	Boolean b = FALSE;
	UCSChar *tmp;
	if(isNumerical_be(be)){
		double d = evalNumber_be(be,vn);
		if (d != d){
			tmp = wcsdup(L"NaN");
			b= TRUE;
		}
		else if ( d == 1/n){
			tmp = wcsdup(L"Infinity");
			b = TRUE;
		}
		else if (d == -1/n){
			tmp = wcsdup(L"-Infinity");
			b  = TRUE;
		}else
		tmp = malloc(sizeof(UCSChar)<<8);

		if (tmp == NULL) {
			throwException2(out_of_mem,
				"string allocation in evalString_be failed ");
		}
		if (b)
			return tmp;
		if (d == (Long) d){
			swprintf(tmp,64,L"%d",(Long) d);
		} else {
			swprintf(tmp,64,L"%f", d);
		}
		return tmp;
	} else {
		Boolean b = evalBoolean_be(be,vn);
		if (b)
			tmp= wcsdup(L"true");
		else
			tmp= wcsdup(L"false");
		if (tmp == NULL){
			throwException2(out_of_mem,
				"String allocation failed in evalString_be");
		}
		return tmp;

	}
}

Boolean evalBoolean_be (binaryExpr *be,VTDNav *vn){
	// i,i1=0, s1, s2;
	//int stackSize;
	//expr *e1, *e2;
	//int t;
	//Boolean b = FALSE;
	double dval;
	switch(be->op){
			case OP_OR: return be->left->evalBoolean(be->left,vn)
						 || be->right->evalBoolean(be->right,vn);
			case OP_AND:return be->left->evalBoolean(be->left,vn)
						 && be->right->evalBoolean(be->right,vn);
			case OP_EQ:
			case OP_NE:
			case OP_LE:
			case OP_GE:
			case OP_LT:
			case OP_GT:  return computeComp(be, be->op,vn);
			default: dval = evalNumber_be(be,vn);
				if (dval ==-0.0 || dval ==+0.0 || (dval!=dval))
					return FALSE;
				return TRUE;
	}
}

Boolean isBoolean_be (binaryExpr *be){
	return be->isBool;
}

Boolean isNumerical_be (binaryExpr *be){
	return be->isNum;
}

Boolean isString_be (binaryExpr *be){
	return FALSE;
}

Boolean isNodeSet_be (binaryExpr *be){
	return FALSE;
}

Boolean requireContextSize_be(binaryExpr *be){
	return be->left->requireContextSize(be->left)
		|| be->right->requireContextSize(be->right);
}

void reset_be(binaryExpr *be, VTDNav *vn){
	be->left->reset(be->left,vn);
	be->right->reset(be->right,vn);
}

void	setContextSize_be(binaryExpr *be,int s){
	be->left->setContextSize(be->left,s);
	be->right->setContextSize(be->right,s);
}

void	setPosition_be(binaryExpr *be,int pos){
	be->left->setPosition(be->left,pos);
	be->right->setPosition(be->right,pos);
}

void    toString_be(binaryExpr *be, UCSChar* string){
	wprintf(L"(");
	be->left->toString(be->left,string);
	switch(be->op){
			case OP_ADD: wprintf(L" + "); break;
			case OP_SUB: wprintf(L" - "); break;
			case OP_MULT: wprintf(L" * "); break;
			case OP_DIV: wprintf(L" / "); break;
			case OP_MOD: wprintf(L" mod "); break;
			case OP_OR : wprintf(L" or ");break;
			case OP_AND: wprintf(L" and "); break;
			case OP_EQ: wprintf(L" = "); break;
			case OP_NE: wprintf(L" != "); break;
			case OP_LE: wprintf(L" <= "); break;
			case OP_GE: wprintf(L" >= "); break;
			case OP_LT: wprintf(L" < "); break;
			default: wprintf(L" > "); break;
	}
	be->right->toString(be->right,string);
	wprintf(L")");
}

UCSChar* createEmptyString(){
	UCSChar* es =  malloc(sizeof(UCSChar));
	if (es!=NULL){
		es[0] = 0;
		return es;
	}
	throwException2(out_of_mem,
		"string allocation faild in createEmptyString ");
	return NULL;
}

int adjust_be(binaryExpr *be, int n){
	int i = be->left->adjust(be->left,n);
	int j = be->right->adjust(be->right,n);
	return min(i,j);
}

int getStringIndex(expr *exp, VTDNav *vn){
	int a = -1,size;
	exception e;
	push2(vn);
	size = vn->contextBuf2->size;
	Try
	{
		a = exp->evalNodeSet(exp,vn);
		if (a != -1)
		{
			int t = getTokenType(vn,a);
			if (t == TOKEN_ATTR_NAME)
			{
				a++;
			}
			else if (getTokenType(vn,a) == TOKEN_STARTING_TAG)
			{
				a = getText(vn);
			}
			else if (t == TOKEN_PI_NAME)
			{
				a++;
			}
		}
	}
	Catch (e)
	{

	}
	vn->contextBuf2->size = size;
	exp->reset(exp,vn);
	pop2(vn);
	return a;

}


Boolean isFinal_be(binaryExpr *e){
	return e->left->isFinal(e->left) && e->right->isFinal(e->right);
}

void markCacheable_be(binaryExpr *e){
	e->left->markCacheable(e->left);
	e->right->markCacheable(e->right);	
}

void markCacheable2_be(binaryExpr *e){
	if (e->left->isFinal(e->left) && e->left->isNodeSet(e->left)){
		cachedExpr *ce = createCachedExpr(e->left);
		e->left = (expr *)ce;
	} 
	e->left->markCacheable2(e->left);
	if (e->right->isFinal(e->right) && e->right->isNodeSet(e->right)){
		cachedExpr *ce = createCachedExpr(e->right);
		e->right = (expr *)ce;
	} 
	e->right->markCacheable2(e->right);
}

void clearCache_be(binaryExpr *e){
	e->left->clearCache(e->left);
	e->right->clearCache(e->right);
}

compType computeCompType(binaryExpr *be) {
	if (be->left->isNodeSet(be->left)) {
		if (be->right->isNodeSet(be->right))
			return NS_NS;
		if (be->right->isNumerical(be->right))
			return NS_N;
		if (be->right->isString(be->right))
			return NS_S;
		return NS_B;
	}
	if (be->left->isNumerical(be->left)) {
		if (be->right->isNodeSet(be->right))
			return N_NS;
		if (be->right->isNumerical(be->right))
			return N_N;
		if (be->right->isString(be->right))
			return N_S;
		return N_B;
	}
	if (be->left->isString(be->left)) {
		if (be->right->isNodeSet(be->right))
			return S_NS;
		if (be->right->isNumerical(be->right))
			return S_N;
		if (be->right->isString(be->right))
			return S_S;
		return S_B;
	}
	if (be->right->isNodeSet(be->right))
		return B_NS;
	if (be->right->isNumerical(be->right))
		return B_N;
	if (be->right->isString(be->right))
		return B_S;
	return B_B;
}
