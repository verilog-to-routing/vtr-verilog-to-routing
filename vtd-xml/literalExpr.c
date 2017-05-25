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

literalExpr *createLiteralExpr(UCSChar *st){
	literalExpr *l = (literalExpr*) malloc(sizeof(literalExpr));
	if (l==NULL){
		throwException2(out_of_mem,"literalExpr allocation failed ");
		return NULL;
	}
	l->freeExpr = (free_Expr)&freeLiteralExpr;
	l->evalBoolean = (eval_Boolean)&evalBoolean_le;
	l->evalNodeSet = (eval_NodeSet)&evalNodeSet_le;
	l->evalNumber  = (eval_Number)&evalNumber_le;
	l->evalString  = (eval_String)&evalString_le;
	l->isNumerical = (is_Numerical)&isNumerical_le;
	l->isBoolean = (is_Boolean)&isBoolean_le;
	l->isString =  (is_String)&isString_le;
	l->isNodeSet = (is_NodeSet)&isNodeSet_le;
	l->requireContextSize = (require_ContextSize)&requireContextSize_le;
	l->setContextSize = (set_ContextSize)&setContextSize_le;
	l->setPosition = (set_Position)&setPosition_le;
	l->reset = (reset_)&reset_le;
	l->toString = (to_String)&toString_le;
	l->adjust= (adjust_)&adjust_le;
	l->s= st;
	l->clearCache = (clearCache_)&clearCache_le;
	l->markCacheable = (markCacheable_)&markCacheable_le;
	l->markCacheable2 = (markCacheable2_)&markCacheable2_le;
	l->isFinal = (isFinal_) &isFinal_le;
	l->getFuncOpCode = (getFuncOpCode_)&getFuncOpCode;
	return l;

}

void freeLiteralExpr(literalExpr *le){
	if (le == NULL) return;
	free(le->s);// this assume s is dynamically created
	free(le);
}

int	evalNodeSet_le (literalExpr *le,VTDNav *vn){
	throwException2(xpath_eval_exception,
		"LiteralExpr can't eval to a node set!");
	return -1;
}

double	evalNumber_le (literalExpr *le,VTDNav *vn){
	double d  = 0, result;
	UCSChar *temp;
	if (wcslen(le->s)==0)
		return d/d;
	result = wcstod(le->s,&temp);
	while(*temp!=0){
		if ( *temp == L' '
			|| *temp == L'\n'
			|| *temp == L'\t'
			|| *temp == L'\r'){
				temp++;
			}
		else
			return d/d; //NaN
	}
	return result;
}

UCSChar* evalString_le (literalExpr *le,VTDNav *vn){
	return wcsdup(le->s);
}

Boolean evalBoolean_le (literalExpr *le,VTDNav *vn){
	size_t len = wcslen(le->s);
	return len != 0;
}

Boolean isBoolean_le (literalExpr *le){
	return FALSE;
}

Boolean isNumerical_le (literalExpr *le){
	return FALSE;
}

Boolean isString_le (literalExpr *le){
	return TRUE;
}

Boolean isNodeSet_le (literalExpr *le){
	return FALSE;
}

Boolean requireContextSize_le(literalExpr *le){
	return FALSE;
}

void	reset_le(literalExpr *le, VTDNav *vn){
}

void	setContextSize_le(literalExpr *le,int s){
}

void	setPosition_le(literalExpr *le,int pos){
}

void    toString_le(literalExpr *le, UCSChar* string){
	wprintf(L"\"");
	wprintf(L"%ls",le->s);
	wprintf(L"\"");
}
int adjust_le(literalExpr *le, int n){
	return 0;
}

Boolean isFinal_le(literalExpr *e){return TRUE;}
void markCacheable_le(literalExpr *e){}
void markCacheable2_le(literalExpr *e){}
void clearCache_le(literalExpr *e){}

char* getAxisString(axisType at){

	switch(at){
		case AXIS_CHILD : return "child::";
		case AXIS_DESCENDANT : return "descendant::";
		case AXIS_PARENT :	return "parent::";
		case AXIS_ANCESTOR :	return "ancestor::";
		case AXIS_FOLLOWING_SIBLING :	return "following-sibling::";
		case AXIS_PRECEDING_SIBLING :	return "preceding-sibling::";
		case AXIS_FOLLOWING :	return "following::";
		case AXIS_PRECEDING :	return "preceding::";
		case AXIS_ATTRIBUTE :	return "attribute::";
		case AXIS_NAMESPACE :	return "namespace::";
		case AXIS_SELF :	return "self::";
		case AXIS_DESCENDANT_OR_SELF :	return "descendant-or-self::";
		default :	return "ancestor-or-self::";
	}
}

funcName getFuncOpCode(struct Expr *e) {

	return -1;
}