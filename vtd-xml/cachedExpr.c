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
//#include "fastIntBuffer.h"
#include "xpath.h"
#include <stdio.h>

cachedExpr *createCachedExpr(expr *e1){
	cachedExpr *ce = malloc(sizeof(cachedExpr));
	if (ce ==NULL){
		throwException2(out_of_mem,
			"cachedExpr allocation failed");
		return NULL;
	}
	ce->freeExpr = (free_Expr)&freeCachedExpr;
	ce->evalBoolean = (eval_Boolean)&evalBoolean_ce;
	ce->evalNodeSet = (eval_NodeSet)&evalNodeSet_ce;
	ce->evalNumber  = (eval_Number)&evalNumber_ce;
	ce->evalString  = (eval_String)&evalString_ce;
	ce->isNumerical = (is_Numerical)&isNumerical_ce;
	ce->isBoolean = (is_Boolean)&isBoolean_ce;
	ce->isString =  (is_String)&isString_ce;
	ce->isNodeSet = (is_NodeSet)&isNodeSet_ce;
	ce->requireContextSize = (require_ContextSize)&requireContextSize_ce;
	ce->setContextSize = (set_ContextSize)&setContextSize_ce;
	ce->setPosition = (set_Position)&setPosition_ce;
	ce->reset = (reset_)&reset_ce;
	ce->toString = (to_String)&toString_ce;
	ce->adjust = (adjust_)&adjust_ce;
	ce->isFinal = (isFinal_)&isFinal_ce;
	ce->markCacheable = (markCacheable_)&markCacheable_ce;
	ce->markCacheable2 = (markCacheable_)&markCacheable2_ce;
	ce->clearCache = (clearCache_)&clearCache_ce;
	ce->getFuncOpCode = (getFuncOpCode_)&getFuncOpCode;
	ce->e = e1;
	ce->ens =  NULL;
	ce->cached = FALSE;
	ce->count = 0;
	ce->vn1 = NULL;
	ce->es = NULL;
	return ce;
}

void freeCachedExpr(cachedExpr *ce){
	if (ce->e!=NULL)
		ce->e->freeExpr(ce->e);
	if (ce->ens!=NULL)
		free(ce->ens);
	if (ce->vn1!=NULL)
		freeVTDNav(ce->vn1);
	if (ce->es!=NULL)
		free(ce->es);
}
int	evalNodeSet_ce (cachedExpr *ce,VTDNav *vn){
			int i=-1;
		if (ce->cached){
			if (ce->count<ce->ens->size){
				i=intAt(ce->ens,ce->count);
				recoverNode(vn,i);
				ce->count++;
				return i;
			}else
				return -1;

		}else{
			ce->cached = TRUE;
			
			if (ce->ens==NULL){
				ce->ens = createFastIntBuffer2(8);//page size 64
			}
			//record node set
			while((i=ce->e->evalNodeSet(ce->e,vn))!=-1){
				appendInt(ce->ens,i);
			}
			ce->e->reset(ce->e,vn);
			if(ce->ens->size>0){
				i=intAt(ce->ens,ce->count);//count should be zero
				recoverNode(vn,i);
				vn->count++;
				return i;
			}else
				return -1;
		}
	
}

double	evalNumber_ce (cachedExpr *ce,VTDNav *vn){
	if (ce->cached){
		return ce->en;
	}else{
		ce->cached = TRUE;
		ce->en = ce->e->evalNumber(ce->e,vn);
		return ce->en;
	}
}

UCSChar* evalString_ce  (cachedExpr *ce,VTDNav *vn){
	if (ce->cached){
		return ce->es;
	}else{
		ce->cached = TRUE;
		ce->es = ce->e->evalString(ce->e,vn);
		return ce->es;
	}	
}

Boolean evalBoolean_ce (cachedExpr *ce,VTDNav *vn){
	if (ce->cached){
		return ce->eb;
	}else{
		ce->eb = ce->e->evalBoolean(ce->e,vn);
		return ce->eb;
	}
}

Boolean isBoolean_ce (cachedExpr *ce){
	return ce->e->isBoolean(ce->e);
}
Boolean isNumerical_ce (cachedExpr *ce){
	return ce->e->isNumerical(ce->e);
}
Boolean isString_ce (cachedExpr *ce){
	return ce->e->isString(ce->e);
}

Boolean isNodeSet_ce (cachedExpr *ce){
	return ce->e->isNodeSet(ce->e);
}

Boolean requireContextSize_ce(cachedExpr *ce){
	return ce->e->requireContextSize(ce->e);
}

void	reset_ce(cachedExpr *ce, VTDNav *vn){
	ce->count = 0;
	if (ce->e!=NULL && ce->vn1!=NULL)
		ce->e->reset(ce->e,vn);
}
void	setContextSize_ce(cachedExpr *ce,int s){
	ce->e->setContextSize(ce->e, s);
}
void	setPosition_ce(cachedExpr *ce,int pos){
	ce->e->setPosition(ce->e,pos);
}
void    toString_ce(cachedExpr *ce, UCSChar* string){
	printf( "cached(");
	ce->e->toString(ce->e,NULL);
	printf(")");
}
int	adjust_ce(cachedExpr *ce, int n){
	return ce->e->adjust(ce->e,n);
}

Boolean isFinal_ce(cachedExpr *ce){
	return ce->e->isFinal(ce->e);
}

void markCacheable_ce(cachedExpr *ce){
	ce->e->markCacheable(ce->e);
}

void markCacheable2_ce(cachedExpr *ce){
	ce->e->markCacheable2(ce->e);
}

void clearCache_ce(cachedExpr *ce){
	ce->e->clearCache(ce->e);
}

