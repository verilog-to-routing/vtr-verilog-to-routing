/* 
* Copyright (C) 2002-2015 XimpleWare, info@ximpleware.com
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the houne that it will be useful,
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
static Boolean isUnique_une(unionExpr *une,int i);

Boolean isUnique_une(unionExpr *une, int i){
	return isUniqueIntHash(une->ih,i);
}

unionExpr *createUnionExpr(expr *e){

	unionExpr *une = (unionExpr *)malloc(sizeof(unionExpr));
	if (une==NULL){
		throwException2(out_of_mem,
			"unionExpr allocation failed ");
		return NULL;
	}
	une->freeExpr = (free_Expr)&freeUnionExpr;
	une->evalBoolean = (eval_Boolean)&evalBoolean_une;
	une->evalNodeSet = (eval_NodeSet)&evalNodeSet_une;
	une->evalNumber  = (eval_Number)&evalNumber_une;
	une->evalString  = (eval_String)&evalString_une;
	une->isNumerical = (is_Numerical)&isNumerical_une;
	une->isBoolean = (is_Boolean)&isBoolean_une;
	une->isString =  (is_String)&isString_une;
	une->isNodeSet = (is_NodeSet)&isNodeSet_une;
	une->requireContextSize = (require_ContextSize)&requireContextSize_une;
	une->setContextSize = (set_ContextSize)&setContextSize_une;
	une->setPosition = (set_Position)&setPosition_une;
	une->reset = (reset_)&reset_une;
	une->toString = (to_String)&toString_une;
	une->adjust =  (adjust_)&adjust_une;
	une->isFinal = (isFinal_)&isFinal_une;
	une->markCacheable = (markCacheable_)&markCacheable_une;
	une->markCacheable2 = (markCacheable2_)&markCacheable2_une;
	une->clearCache = (clearCache_)&clearCache_une;
	une->fe = e;
	une->next = NULL;
	une->current = une;
	une->evalState = 0;
	une->ih = NULL;
	une->needReordering = TRUE;
	une->getFuncOpCode = (getFuncOpCode_)&getFuncOpCode;
	return une;

}
void freeUnionExpr(unionExpr *e){
	unionExpr *tmp, *tmp2;
	if (e->fe!=NULL)
		e->fe->freeExpr(e->fe);
	freeIntHash(e->ih);
	tmp = e->next;
    free(e);
	while(tmp!= NULL){
		tmp->fe->freeExpr(tmp->fe);
		freeIntHash(tmp->ih);
		tmp2 = tmp;
		tmp = tmp->next;		
		free(tmp2);
	}
}

int	evalNodeSet_une (unionExpr *e,VTDNav *vn){
	int a;
	/*if (e->next == NULL) {
	return e->fe->evalNodeSet(e->fe,vn);
	} else {*/
	while (TRUE) {
		switch (e->evalState) {
				case 0:
					if (e->ih == NULL ){
						exception ee;
						Try{
							e->ih = createIntHash();
						}
						Catch(ee){
							Throw ee;
						}
					}
					if (e->current != NULL) {
						push2(vn);
						while ((a = e->current->fe->evalNodeSet(e->current->fe,vn)) != -1) {
							if (isUnique_une(e,a)) {
								e->evalState = 1;
								return a;
							}
						}
						e->evalState = 2;
						pop2(vn);
						break;
					} else
						e->evalState = 3;
					break;

				case 1:
					while ((a = e->current->fe->evalNodeSet(e->current->fe, vn)) != -1) {
						if (isUnique_une(e,a)) {
							e->evalState = 1;
							return a;
						}
					}
					e->evalState = 2;
					pop2(vn);
					break;

				case 2:
					e->current = e->current->next;
					if (e->current != NULL) {
						push2(vn);
						while ((a = e->current->fe->evalNodeSet(e->current->fe, vn)) != -1) {
							if (isUnique_une(e,a)) {
								e->evalState = 1;
								return a;
							}
						}
						pop2(vn);
						break;
					} else
						e->evalState = 3;
					break;

				case 3:
					return -1;

				default:
					throwException2(other_exception,
						"Invalid state evaluating unionExpr");
		}
	}
	//}
}

double	evalNumber_une (unionExpr *une,VTDNav *vn){
	double d1 = 0.0;
	exception ee;
	double d=d1/d1;
	int a=0x7fffffff,k = -1,size;
	push2(vn);
	size = vn->contextBuf2->size;
	Try {
		/*a = evalNodeSet_une(une,vn);*/
		while ((k = evalNodeSet_une(une,vn)) != -1) {
			//a = evalNodeSet(vn);
			if (k<a)
				a = k;
		}
		if (a == 0x7fffffff)
			a = -1;
		if (a != -1) {
			int t = getTokenType(vn,a);
			if (t == TOKEN_ATTR_NAME) {
				d = parseDouble(vn,a+1);
			} else if (t == TOKEN_STARTING_TAG || t ==TOKEN_DOCUMENT) {
				/*UCSChar *s =getXPathStringVal( vn,0), *s1;
				d  = wcstod(s,&s1);
				free( s);*/
				d = XPathStringVal2Double(vn,a);
			}else if (t == TOKEN_PI_NAME) {
				if (a+1 < vn->vtdSize || getTokenType(vn,a+1)==TOKEN_PI_VAL)
					//s = vn.toString(a+1); 	
					d = parseDouble(vn,a+1);                	
			}else 
				d = parseDouble(vn,a);
		}
	} Catch (ee) {

	}
	vn->contextBuf2->size = size;
	reset_une(une,vn);
	pop2(vn);
	//return s;
	return d;
}
UCSChar* evalString_une  (unionExpr *une,VTDNav *vn){
	exception ee;
	int a=0x7ffffff,k=-1,size;
	UCSChar *s = NULL;	
	if (une->fe->isNodeSet(une->fe)==FALSE){   
		return une->fe->evalString(une->fe,vn);   
	}	
	//int a = -1;
	push2(vn);
    size = vn->contextBuf2->size;
     
	Try {
        /* a = evalNodeSet_une(e,vn);*/
		while ((k = evalNodeSet_une(une,vn)) != -1) {
			//a = evalNodeSet(vn);
			if (k<a)
				a = k;
		}
		if (a == 0x7fffffff)
			a = -1;
         if (a != -1) {
            	int t = getTokenType(vn,a);
                switch(t){
			 case TOKEN_STARTING_TAG:
			 case TOKEN_DOCUMENT:
				 s = getXPathStringVal(vn,0);
				 break;
			 case TOKEN_ATTR_NAME:
				 s = toString(vn,a + 1);
				 break;
			 case TOKEN_PI_NAME:
				 //if (a + 1 < vn.vtdSize
				 //		|| vn.getTokenType(a + 1) == VTDNav.TOKEN_PI_VAL)
				 s = toString(vn,a + 1);
				 break;
			 default:
				 s = toString(vn,a);
				 break;
			 }		
            }else
				wcsdup(L"");
        } Catch (ee) {

        }
        vn->contextBuf2->size = size;
        reset_une(une,vn);
        pop2(vn);
        return s;
}
Boolean evalBoolean_une (unionExpr *e,VTDNav *vn){
	exception ee;
	Boolean b = FALSE;
	int size;
	/*if (e->fe->isNodeSet(e->fe)==FALSE){
	return e->fe->evalBoolean(e->fe,vn);
	}else{*/
	push2(vn);
	/* record teh stack size*/
	size = vn->contextBuf2->size;
	Try{	
		b = (evalNodeSet_une(e,vn) != -1);
	}Catch (ee){
	}
	/*rewind stack */
	vn->contextBuf2->size = size;
	reset_une(e,vn);
	pop2(vn);
	return b;
	//}
	
	/*else if (e->fe->isNumerical(e->fe)){
           double dval = e->fe->evalNumber(e->fe,vn);
            if (dval == 0.0 || dval!=dval )
    			return FALSE;
    		return TRUE;
	}else {
		 UCSChar *s = e->fe->evalString(e->fe,vn);
		 if (s==NULL || wcslen(s)==0){
			 free(s);
             return FALSE;
		 }
		 free(s);
         return TRUE;
	}*/

}
Boolean isBoolean_une (unionExpr *e){
	return e->fe->isBoolean(e->fe);
}
Boolean isNumerical_une (unionExpr *e){
	return e->fe->isNumerical(e->fe);
}
Boolean isString_une (unionExpr *e){
	return e->fe->isString(e->fe);
}
Boolean isNodeSet_une (unionExpr *e){
	return e->fe->isNodeSet(e->fe);
}
Boolean requireContextSize_une(unionExpr *e){
	// boolean b = false;
	unionExpr* tmp = e;

	while (tmp != NULL) {
		if (tmp->fe->requireContextSize(tmp->fe)== TRUE){
			return TRUE;
		}            
		tmp = tmp->next;
	}        
	return FALSE;
}
void reset_une(unionExpr *e, VTDNav *vn){
	unionExpr *tmp;
	e->fe->reset(e->fe,vn);
	e->current = e;
	tmp = e->next;
    while (tmp != NULL) {
        tmp->fe->reset(tmp->fe,vn);
        tmp = tmp->next;
    }
	if (e->ih!=NULL)
		resetIntHash(e->ih);
	e->evalState = 0;
}

void setContextSize_une(unionExpr *e,int s){
	unionExpr *tmp;
    e->current = e;
    e->current->fe->setContextSize(e->current->fe, s);
    tmp = e->next;
    while (tmp != NULL) {
        tmp->fe->setContextSize(tmp->fe,s);
        tmp = tmp->next;
    }
}

void	setPosition_une(unionExpr *e,int pos){
	unionExpr *tmp;
    e->current = e;
    e->current->fe->setPosition(e->current->fe, pos);
    tmp = e->next;
    while (tmp != NULL) {
        tmp->fe->setPosition(tmp->fe,pos);
        tmp = tmp->next;
    }
}

void    toString_une(unionExpr *e, UCSChar* string){
       if (e->next == NULL)
            e->fe->toString(e->fe,string);
	   else{
            e->fe->toString(e->fe,string);
			wprintf(L" | ");
			e->next->toString((expr *)e->next, string);
	   }
}

int adjust_une(unionExpr *une, int n){
	    int i=une->fe->adjust(une->fe,n);
		unionExpr *tmp = NULL;		
        tmp = une->next;
        while (tmp != NULL) {
            tmp->fe->adjust(tmp->fe,n);
            tmp = tmp->next;
        }
		if (une->ih!=NULL && i==une->ih->e)
		{}else{
			freeIntHash(une->ih);
			une->ih = createIntHash2(i);
		}
		return i;
}

Boolean isFinal_une(unionExpr *une){
	unionExpr *tmp = une;
	while (tmp != NULL) {
		if (tmp->fe->isFinal(tmp->fe)== FALSE){            	
			return FALSE;
		}
		tmp = tmp->next;
	}        
	return TRUE;
}

void markCacheable_une(unionExpr *une){
	unionExpr *tmp = une;
	while (tmp != NULL) {
		tmp->fe->markCacheable(tmp->fe);
		tmp = tmp->next;
	}  
}

void markCacheable2_une(unionExpr *une){
	unionExpr *tmp = une;
	while (tmp != NULL) {
		if (tmp->fe->isFinal(tmp->fe) && tmp->fe->isNodeSet(tmp->fe)){
			cachedExpr *ce = createCachedExpr(tmp->fe);
			tmp->fe = (expr *)ce;	
		}   
		tmp->fe->markCacheable2(tmp->fe);     		       	
		tmp = tmp->next;
	}  
}

void clearCache_une(unionExpr *une){
	unionExpr *tmp = une;
	while (tmp != NULL) {
		tmp->fe->clearCache(tmp->fe); 		       	
		tmp = tmp->next;
	} 
}