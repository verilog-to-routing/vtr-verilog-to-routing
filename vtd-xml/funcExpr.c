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
#include "XMLModifier.h"

static double sum(funcExpr *fne, VTDNav *vn);
static int argCount(funcExpr *fne);
static int count(funcExpr *fne, VTDNav *vn);
static UCSChar *getLocalName(funcExpr *fne, VTDNav *vn);
static UCSChar *getName(funcExpr *fne, VTDNav *vn);
static UCSChar *getNameSpaceURI(funcExpr *fne, VTDNav *vn);
static UCSChar *getString(funcExpr *fne, VTDNav *vn);
static Boolean contains_fe(funcExpr *fne, VTDNav *vn);
static UCSChar* concat(funcExpr *fne, VTDNav *vn);
static Boolean startsWith_fe(funcExpr *fne, VTDNav *vn);
static Boolean endsWith_fe(funcExpr *fne, VTDNav *vn);
static UCSChar* subString(funcExpr *fne, VTDNav *vn);
static UCSChar* normalizeString(funcExpr *fne, VTDNav *vn);
static UCSChar* subStringBefore(funcExpr *fne, VTDNav *vn);
static UCSChar* subStringAfter(funcExpr *fne, VTDNav *vn);
static UCSChar* translate(funcExpr *fne, VTDNav *vn);
static Boolean isWS(UCSChar c);
static Boolean lang(funcExpr *fne, VTDNav *vn, UCSChar* s);
static inline UCSChar* normalize(UCSChar *s);
static double myround(double v);
//static int evalFirstArgumentListNodeSet(funcExpr *fne, VTDNav *vn);
//static int evalFirstArgumentListNodeSet2(funcExpr *fne, VTDNav *vn);
static UCSChar* upperCase(funcExpr *fne, VTDNav *vn);
static UCSChar* lowerCase(funcExpr *fne, VTDNav *vn);
static double roundHalfToEven(funcExpr *fne, VTDNav *vn);
static Boolean matchName(funcExpr *fne,VTDNav *vn);
static Boolean matchLocalName(funcExpr *fne,VTDNav *vn);
		//bool isElementAvailable(VTDNav *vn);
		
static UCSChar* formatNumber(funcExpr *fne,VTDNav *vn);
static UCSChar* generateID(funcExpr *fne,VTDNav *vn);
static int evalFirstArgumentListNodeSet(funcExpr *fne, VTDNav *vn);
static int evalFirstArgumentListNodeSet2(funcExpr *fne, VTDNav *vn);
		
static int getStringVal(funcExpr *fne,VTDNav *vn,int i);
static inline UCSChar *getSystemProperty(funcExpr *fne,VTDNav *vn){return createEmptyString();}
static inline Boolean isElementAvailable(funcExpr *fne,VTDNav *vn){return FALSE;}
static inline Boolean isFunctionAvailable(funcExpr *fne,VTDNav *vn){return FALSE;}
static UByte* doubleCapacity(UByte *b, size_t cap);
//static Long getBytes_UTF8(UCSChar *s);

static double myround(double v)
{
 return (v>0.0) ? floor(v+0.5) : ceil(v-0.5);
}





static UCSChar *fname(funcExpr *fne,funcName i);


static UCSChar* upperCase(funcExpr *fne, VTDNav *vn){
	exception ee;
	
		if (fne->argCount1 == 1){
			if (fne->al->e->isNodeSet(fne->al->e)){
				int a = evalFirstArgumentListNodeSet(fne,vn);
				if (a==-1)
					return createEmptyString(); 
				else{
					Try{
						int t = getTokenType(vn,a);
						if (t!= TOKEN_STARTING_TAG && t!=TOKEN_DOCUMENT)
							return toStringUpperCase(vn,a);
						return getXPathStringVal2(vn,a,(short)1);
					}Catch(ee){
					}
					return createEmptyString();
				}	
			}else {
				UCSChar *s = fne->al->e->evalString(fne->al->e, vn);
				UCSChar *s1= s;
				int i=0;
				while( s1[i] != 0){
					if (s1[i]>96 && s1[i] <123){
						s1[i] -=32;
					}
					i++;
				}
				return s;
			}
	}else{
		throwException2(invalid_argument,
			"upper-case()'s  <funcExpr> argument count is invalid");
		return createEmptyString();
	}
}
static UCSChar* lowerCase(funcExpr *fne, VTDNav *vn){
	exception ee;
	if (fne->argCount1 == 1){
		if (fne->al->e->isNodeSet(fne->al->e)){
		int a = evalFirstArgumentListNodeSet(fne,vn);
		if (a==-1)
		    return createEmptyString(); 
		else{
        	Try{
        		int t = getTokenType(vn,a);
        		if (t!= TOKEN_STARTING_TAG && t!=TOKEN_DOCUMENT)
        			return toStringLowerCase(vn,a);
        		return getXPathStringVal2(vn,a,(short)2);
        	}Catch(ee){
	       	}
		   	return createEmptyString();
        }	
		}else {
			UCSChar *s1 = fne->al->e->evalString(fne->al->e,vn);
			int i=0;
			while( s1[i] != 0){
				if (s1[i]>64 && s1[i]<91){
					s1[i] +=32;
				}
				i++;
			}
			return s1;
		}
		
	}else{
		throwException2(invalid_argument,
			"lower-case()'s  <funcExpr> argument count is invalid");
		return createEmptyString();
	}
}

static UCSChar *getString(funcExpr *fne, VTDNav *vn){
	if (fne->argCount1== 0){
		exception e;
		Try{
			if (vn->atTerminal){
				if (getTokenType(vn,vn->LN) == TOKEN_CDATA_VAL )
					return toRawString(vn,vn->LN);
				return toString(vn,vn->LN);
			}
			return getXPathStringVal(vn, getCurrentIndex(vn));
		}
		Catch(e){
			 // this will almost never occur
		}
	}
	else if (fne->argCount1 == 1){
		return fne->al->e->evalString(fne->al->e, vn);
	} else 
		throwException2(invalid_argument,
			"string()'s  <funcExpr> argument count is invalid");
	return createEmptyString();
}

static UCSChar *getLocalName(funcExpr *fne, VTDNav *vn){
	exception e;
	int index;
	int type;
	if (fne->argCount1== 0){
		Try{
			index = getCurrentIndex(vn);
			type = getTokenType(vn,index);
			if (vn->ns && (type == TOKEN_STARTING_TAG || type == TOKEN_ATTR_NAME)) {
				int offset = getTokenOffset(vn,index);
				int length = getTokenLength(vn,index);
				if (length < 0x10000){
					if (vn->localNameIndex!=index){
						vn->localNameIndex = index;
						if (vn->localName !=NULL)
							free( vn->localName);
						vn->localName = toRawString(vn,index);
					}
					return vn->localName;
				}
				else {
					int preLen = length >> 16;
					int QLen = length & 0xffff;
					if (preLen != 0){
						     
						if (vn->localNameIndex != index){
                        		vn->localNameIndex = index;
								if (vn->localName!=NULL)
									free( vn->localName);
                        		vn->localName = toRawString2(vn,offset + preLen+1, QLen
                                        - preLen - 1);
                        
						}
                        
						return vn->localName;
						//return vn->toRawString(offset + preLen+1, QLen
						//- preLen - 1);
					}else {
						if (vn->localNameIndex != index){
							vn->localNameIndex = index;
							if (vn->localName!=NULL)
								free( vn->localName);
							vn->localName = toRawString2(vn,offset, QLen);
						}
						return vn->localName;
					}
				}
			} else if (type ==TOKEN_PI_NAME){
				if (vn->localNameIndex != index){
                		vn->localNameIndex = index;
						if (vn->localName!=NULL)
							free( vn->localName);
                		vn->localName = toRawString(vn,index);
                	}
                    return vn->localName;
			}
		}
		Catch(e){

		}
	}
	else if (fne->argCount1 == 1){
		int a = -1;
		int type;
		push2(vn);
		Try{
			a = fne->al->e->evalNodeSet(fne->al->e,vn);
			fne->al->e->reset(fne->al->e,vn);
			pop2(vn);
		}Catch(e){
			fne->al->e->reset(fne->al->e,vn);
			pop2(vn);
		}
		if (a == -1 || vn->ns == FALSE)
			return createEmptyString();
		type = getTokenType(vn,a);

		if (type != TOKEN_STARTING_TAG && type != TOKEN_ATTR_NAME)
			return createEmptyString();
		Try {
			int offset = getTokenOffset(vn,a);
			int length = getTokenLength(vn,a);
			if (length < 0x10000)
				return toRawString(vn,a);
			else {
				int preLen = length >> 16;
				int QLen = length & 0xffff;
				if (preLen != 0)
					return toRawString2(vn, offset + preLen+1,
					QLen - preLen - 1);
			}
		} Catch (e) {
			 // this will almost never occur
		}
	} else {
		throwException2(invalid_argument,
			"local-name()'s  <funcExpr> argument count is invalid");
	}
	return createEmptyString();
}
static UCSChar *getNameSpaceURI(funcExpr *fne, VTDNav *vn){
		exception e;
	if (fne->argCount1== 0){
		Try{
			int i = getCurrentIndex(vn);
			int type = getTokenType(vn,i);
			if (vn->ns && (type == TOKEN_STARTING_TAG
				|| type == TOKEN_ATTR_NAME)) {
				int a = lookupNS(vn);
				if (a == 0)
					return createEmptyString();
				else
					return toString(vn,a);
			}
		}Catch (e){
		}
	}
	else if (fne->argCount1 == 1){
		int a = -1, size;
		push2(vn);
		size = vn->contextBuf2->size;
		Try{
			a = fne->al->e->evalNodeSet(fne->al->e,vn);
			if (a==-1 || vn->ns == FALSE){
				return createEmptyString();
			}
			else {
				int type = getTokenType(vn,a);
				if (type == TOKEN_STARTING_TAG
					|| type == TOKEN_ATTR_NAME){
						UCSChar *tmp = toString(vn,lookupNS(vn));
						fne->al->e->reset(fne->al->e,vn);
						vn->contextBuf2->size = size;
						pop2(vn);
						return tmp;
				}
			}
		}Catch(e){
			
		}
		fne->al->e->reset(fne->al->e,vn);
		vn->contextBuf2->size = size;
		pop2(vn);
	} else {
		throwException2(invalid_argument,
			"namespace-uri()'s  <funcExpr> argument count is invalid");
	}
	return createEmptyString();
}

static UCSChar *getName(funcExpr *fne, VTDNav *vn){
	exception e;
	int a;
	int type;
	if (fne->argCount1== 0){
		a = getCurrentIndex(vn);
		type = getTokenType(vn,a);
		if ( type == TOKEN_STARTING_TAG
			|| type == TOKEN_ATTR_NAME
			|| type == TOKEN_PI_NAME){
			Try{
				if (vn->nameIndex!=a){
					if (vn->name !=NULL)
						free( vn->name);
	            	vn->name = toRawString(vn,a);
	            	vn->nameIndex = a;	
	            }
            	return vn->name;
			}Catch(e){
				return createEmptyString();
			}
		}
		else
			return createEmptyString();
	}
	else if (fne->argCount1 == 1){
		a = -1;
		push2(vn);
		Try{
			a = fne->al->e->evalNodeSet(fne->al->e, vn);
			fne->al->e->reset(fne->al->e,vn);
			pop2(vn);
		}Catch(e){
			fne->al->e->reset(fne->al->e,vn);
			pop2(vn);
		}
		Try{
			if (a == -1 || vn->ns == FALSE){
				return createEmptyString();
			}
			else{
				int type = getTokenType(vn,a);
				if ( type== TOKEN_STARTING_TAG || type == TOKEN_ATTR_NAME  || type == TOKEN_PI_NAME)
					return toRawString(vn,a);
				return createEmptyString();
			}
		}Catch(e){
		}

		//return fne->al->e->evalString(fne->al->e, vn);
	} else 
		throwException2(invalid_argument,"name()'s  <funcExpr> argument count is invalid");
	return createEmptyString();
}


aList *createAlist(){
	aList *al = (aList *)malloc(sizeof(aList));
	if (al==NULL){
		throwException2(out_of_mem,
		  "funcExpr allocation failed ");
	}
	al->next  = NULL;
	return al;
}

void  freeAlist(aList *al){
	aList *temp = al;
	while(temp!=NULL){
		if (temp->e!= NULL )
			(temp->e->freeExpr)(temp->e);
		temp = temp->next;
	}
}

void  reset_al(aList *al, VTDNav *vn){
	aList *temp = al;
	while(temp!=NULL){
		if (temp->e!= NULL )
			(temp->e->reset)(temp->e,vn);
		temp = temp->next;
	}
}

void  toString_al(aList *al, UCSChar* string){
	aList *temp = al;

	while(temp!=NULL){
		(temp->e->toString)(temp->e, string);
		temp = temp->next;
		if (temp!=NULL)
			wprintf(L",");
	}
}


double sum(funcExpr *fne, VTDNav *vn){
	exception e;
	double d = 0;
	double n=0;
	int i1;
	/*if (argCount(fne) != 1
		|| fne->al->e->isNodeSet(fne->al->e) == FALSE){
			e.et = invalid_argument;
			e.msg = "sum() <funcExpr> 's argument has to be a node set ";
			Throw e;
		}*/

	push2(vn);
	Try {
		fne->a = 0;
		while ((fne->a = fne->al->e->evalNodeSet(fne->al->e,vn)) != -1) {
			tokenType t = getTokenType(vn,fne->a);
			if (t == TOKEN_STARTING_TAG){
				i1 = getText(vn);
				if (i1!=-1)
					d += parseDouble(vn,i1);
				if (d!=d)
					break;
			}
			else if (t == TOKEN_ATTR_NAME
				|| t == TOKEN_ATTR_NS){
					d += parseDouble(vn,fne->a+1);
					if (d!=d)
						break;
				}
			else if (t == TOKEN_CHARACTER_DATA
				|| t == TOKEN_CDATA_VAL){
					d += parseDouble(vn, fne->a);
					if (d != d)
						break;
				}
				//    fib1.append(i);
		}
		fne->al->e->reset(fne->al->e,vn);
		pop2(vn);
		return d;
	} Catch (e) {
		fne->al->e->reset(fne->al->e,vn);
		pop2(vn);
	}
	return n/n;
}

int argCount(funcExpr *fne){
	aList *temp = fne->al;
	int count = 0;
	while(temp!=NULL){
		count++;
		temp = temp->next;
	}
	return count;
}
int count(funcExpr *fne, VTDNav *vn){
	exception e;
	int a = -1;

	/*if (argCount(fne)!=1
		||(fne->al->e->isNodeSet)(fne->al->e)==FALSE){
			e.et = invalid_argument;
			e.msg = "count <funcExpr> 's argument has to be a node set ";
			Throw e;
		}
	*/
	push2(vn);
	Try{
		a = 0;
		fne->al->e->adjust(fne->al->e,vn->vtdSize);
		while(fne->al->e->evalNodeSet(fne->al->e,vn)!=-1){
			a ++;
		}
		fne->al->e->reset(fne->al->e,vn);
		pop2(vn);

	}Catch(e){
		fne->al->e->reset(fne->al->e,vn);
		pop2(vn);
	}
	return a;
}


UCSChar *fname(funcExpr *fne, funcName i){
	switch(fne->opCode){
			case FN_LAST: 			return L"last";
			case FN_POSITION: 		return L"position";
			case FN_COUNT: 			return L"count";
			case FN_LOCAL_NAME: 		return L"local-name";
			case FN_NAMESPACE_URI: 		return L"namespace-uri";
			case FN_NAME: 			return L"name";
			case FN_STRING: 			return L"string";
			case FN_CONCAT: 			return L"concat";
			case FN_STARTS_WITH:		return L"starts-with";
			case FN_CONTAINS: 		return L"contains";
			case FN_SUBSTRING_BEFORE: 	return L"substring_before";
			case FN_SUBSTRING_AFTER: 		return L"substring_after";
			case FN_SUBSTRING: 		return L"substring";
			case FN_STRING_LENGTH: 		return L"string-length";
			case FN_NORMALIZE_SPACE: 		return L"normalize-space";
			case FN_TRANSLATE:	 	return L"translate";
			case FN_BOOLEAN: 			return L"boolean";
			case FN_NOT: 			return L"not";
			case FN_TRUE: 			return L"true";
			case FN_FALSE: 			return L"false";
			case FN_LANG: 			return L"lang";
			case FN_NUMBER:			return L"number";
			case FN_SUM: 			return L"sum";
			case FN_FLOOR: 			return L"floor";
			case FN_CEILING: 		return L"ceiling";
			case FN_ROUND:		    return L"round";
			case FN_ABS:		    return L"abs";
			case FN_ROUND_HALF_TO_EVEN:		return L"round-half-to-even";
			case FN_ROUND_HALF_TO_ODD:		return L"round-half-to-odd";
			case FN_CODE_POINTS_TO_STRING: 	  return L"code-points-to-string";
			case FN_COMPARE:		   return L"compare";
			case FN_UPPER_CASE:		   return L"upper-case";
			case FN_LOWER_CASE:		   return L"lower-case";
			case FN_ENDS_WITH:		   return L"ends-with";
			case FN_QNAME:			   return L"QName";
			case FN_LOCAL_NAME_FROM_QNAME:		  return L"local-name-from-QName";
			case FN_NAMESPACE_URI_FROM_QNAME:	  return L"namespace-uri-from-QName";
			case FN_NAMESPACE_URI_FOR_PREFIX:	  return L"namespace-uri-for-prefix";
			case FN_RESOLVE_QNAME:		  return L"resolve-QName";
			case FN_IRI_TO_URI:			  return L"iri-to-uri";
			case FN_ESCAPE_HTML_URI:	  return L"escape-html-uri";
			case FN_ENCODE_FOR_URI:	return L"encode-for-uri";
			case FN_MATCH_NAME:		return L"match-name";
			case FN_MATCH_LOCAL_NAME:	return L"match-local-name";
			case FN_NOT_MATCH_NAME:	return L"not-match-name";
			case FN_NOT_MATCH_LOCAL_NAME: return L"not-match-local-name";
			case FN_CURRENT:			return L"current";
			case FN_GENERATE_ID:		return L"generate-id";
			case FN_FORMAT_NUMBER:	return L"format-number";
			case FN_KEY:		return L"key";
			//default:			return (UCSChar *)L"encode-for-uri";
			default:			return L"encode-for-uri";

	}
}

funcExpr *createFuncExpr(funcName oc, aList *a){
	funcExpr *fne = (funcExpr *)malloc(sizeof(funcExpr));
	if (fne==NULL){
		throwException2(out_of_mem,"funcExpr allocation failed ");
		return NULL;
	}

	fne->freeExpr = (free_Expr)&freeFuncExpr;
	fne->evalBoolean = (eval_Boolean)&evalBoolean_fne;
	fne->evalNodeSet = (eval_NodeSet)&evalNodeSet_fne;
	fne->evalNumber  = (eval_Number)&evalNumber_fne;
	fne->evalString  = (eval_String)&evalString_fne;
	fne->isNumerical = (is_Numerical)&isNumerical_fne;
	fne->isBoolean = (is_Boolean)&isBoolean_fne;
	fne->isString =  (is_String)&isString_fne;
	fne->isNodeSet = (is_NodeSet)&isNodeSet_fne;
	fne->requireContextSize = (require_ContextSize)&requireContextSize_fne;
	fne->setContextSize = (set_ContextSize)&setContextSize_fne;
	fne->setPosition = (set_Position)&setPosition_fne;
	fne->reset = (reset_)&reset_fne;
	fne->toString = (to_String)&toString_fne;
	fne->adjust = (adjust_)&adjust_fne;
	fne->isFinal = (isFinal_)&isFinal_fne;
	fne->markCacheable=(markCacheable_)&markCacheable_fne;
	fne->markCacheable2=(markCacheable2_)&markCacheable2_fne;
	fne->clearCache = (clearCache_)&clearCache_fne;
	fne->getFuncOpCode = (getFuncOpCode_)&getFuncOpCode_fne;
	fne->a = 0;
	fne->opCode = oc;
	fne->al = a;
	fne->isBool = fne->isStr= fne->isNum = fne->isNode= FALSE;
	fne->a=0;
	fne->state = XPATH_EVAL_START;
	fne->xslVN  = fne->newVN = NULL;
	fne->vg = NULL;
	fne->argCount1 = argCount(fne);
	 switch(oc){
			case FN_LAST: 			fne->isNum = TRUE;break;
			case FN_POSITION: 		fne->isNum = TRUE;break;
			case FN_COUNT: 			fne->isNum = TRUE;break;
			case FN_LOCAL_NAME: 		fne->isStr = TRUE; break;
			case FN_NAMESPACE_URI: 	fne->isStr = TRUE; break;
			case FN_NAME: 			fne->isStr = TRUE; break;
			case FN_STRING: 			fne->isStr = TRUE; break;
			case FN_CONCAT: 			fne->isStr = TRUE; break;
			case FN_STARTS_WITH:		fne->isBool= TRUE;break;
			case FN_CONTAINS: 		fne->isBool= TRUE;break;
			case FN_SUBSTRING_BEFORE: fne->isStr = TRUE; break;
			case FN_SUBSTRING_AFTER: 	fne->isStr = TRUE; break;
			case FN_SUBSTRING: 		fne->isStr = TRUE; break;
			case FN_STRING_LENGTH: 	fne->isNum = TRUE;break;
			case FN_NORMALIZE_SPACE: 	fne->isStr = TRUE; break;
			case FN_TRANSLATE:	 	fne->isStr= TRUE;break;
			case FN_BOOLEAN: 			fne->isBool =TRUE;break;
			case FN_NOT: 			    fne->isBool =TRUE;break;
			case FN_TRUE: 			fne->isBool = TRUE;break;
			case FN_FALSE: 			fne->isBool = TRUE;break;
			case FN_LANG: 			fne->isBool = TRUE;break;
			case FN_NUMBER:			fne->isNum = TRUE;break;
			case FN_SUM: 			    fne->isNum = TRUE;break;
			case FN_FLOOR: 			fne->isNum = TRUE;break;
			case FN_CEILING: 			fne->isNum = TRUE;break;
			case FN_ROUND:		   fne->isNum = TRUE;break;
			case FN_ABS:		   fne->isNum = TRUE;break;
			case FN_ROUND_HALF_TO_EVEN:		   fne->isNum = TRUE;break;
			case FN_ROUND_HALF_TO_ODD:		   fne->isNum = TRUE;break;
			case FN_CODE_POINTS_TO_STRING: 	   fne->isStr = TRUE;break;
			case FN_COMPARE:		   fne->isBool = TRUE; break;
			case FN_UPPER_CASE:		   fne->isStr = TRUE; break;
			case FN_LOWER_CASE:		   fne->isStr = TRUE; break;
			case FN_ENDS_WITH:		   fne->isBool = TRUE; break;
			case FN_QNAME:		   fne->isStr = TRUE; break;
			case FN_LOCAL_NAME_FROM_QNAME:		   fne->isStr = TRUE; break;
			case FN_NAMESPACE_URI_FROM_QNAME:	   fne->isStr = TRUE; break;
			case FN_NAMESPACE_URI_FOR_PREFIX:	   fne->isStr = TRUE; break;
			case FN_RESOLVE_QNAME:		   fne->isStr = TRUE; break;
			case FN_IRI_TO_URI:			   fne->isStr = TRUE; break;
			case FN_ESCAPE_HTML_URI:	   fne->isStr = TRUE; break;
			case FN_ENCODE_FOR_URI:	fne->isStr  = TRUE; break;
			case FN_MATCH_NAME:		fne->isBool  =TRUE; break;
			case FN_MATCH_LOCAL_NAME: fne->isBool =TRUE;break;
			case FN_NOT_MATCH_NAME:		fne->isBool  =TRUE; break;
			case FN_NOT_MATCH_LOCAL_NAME: fne->isBool =TRUE;break;
			case FN_GENERATE_ID : 	fne->isStr  = TRUE; break;
			case FN_FORMAT_NUMBER:  	fne->isStr  = TRUE;break;
			case FN_KEY:				fne->isNode  = TRUE; fne->state = XPATH_EVAL_START; fne->vg = createVTDGen();break;
			case FN_DOCUMENT:			fne->isNode  = TRUE; fne->state = XPATH_EVAL_START; fne->vg = createVTDGen();break;
			case FN_CURRENT:			fne->isNode  = TRUE; fne->state = XPATH_EVAL_START; fne->vg = createVTDGen();break;
			case FN_SYSTEM_PROPERTY: 	fne->isStr  = TRUE; break;
			case FN_ELEMENT_AVAILABLE: fne->isBool  = TRUE; break;
			//case FN_FUNCTION_AVAILABLE: isBool  = true; break;
			default:		fne->isBool = TRUE; break;	
			//default:		fne->isStr = TRUE; break;	
	  }
	return fne;
}
void freeFuncExpr(funcExpr *fne){
	if (fne ==NULL) return;
	if (fne->al!=NULL)
		freeAlist(fne->al);
	free(fne);
}

int	evalNodeSet_fne (funcExpr *fne,VTDNav *vn){
	exception ee;int a;
	switch (fne->opCode) {
		case FN_CURRENT:
			if (fne->state == XPATH_EVAL_START) {
				loadCurrentNode(vn);
				fne->state = XPATH_EVAL_END;
				return getCurrentIndex2(vn);
			} else {
				return -1;
			}
		// break;
		case FN_DOCUMENT:
			if (fne->argCount1 == 1) {
				if (!fne->al->e->isNodeSet(fne->al->e)) {
					if (fne->state == XPATH_EVAL_START) {
						UCSChar *s = fne->al->e->evalString(fne->al->e,vn);
						Long l= getBytes_UTF8(s);
						//delete s;
						int len=(int)(l>>32);
						char *s1 = (char *)l;
						if (wcslen(s) == 0) {
							fne->newVN = fne->xslVN;
							fne->newVN->context[0] = -1;
						} else if (parseFile(fne->vg,TRUE, s1)) {
							fne->newVN = getNav(fne->vg);
							
							fne->newVN->context[0] = -1;
							fne->newVN->URIName = s;
						} else {
							fne->state = XPATH_EVAL_END;
							free(s);
							free(s1);
							return -1;
						}
						fne->state = XPATH_EVAL_END;
						free(s);
						free(s1);
						return 0;
					} else {
						return -1;
					}
				} else {
					Try {
						if (fne->state != XPATH_EVAL_END) {
							a = fne->al->e->evalNodeSet(fne->al->e,vn);
							if (a != -1) {
								UCSChar *s = toString(vn,getStringVal(fne,vn,a));
								Long l= getBytes_UTF8(s);
								//delete s;
								int len=(int)(l>>32);
								char *s1 = (char *)l;
								if (wcslen(s) == 0) {
									fne->newVN = fne->xslVN;
									fne->newVN->context[0] = -1;
								} else if (parseFile(fne->vg,TRUE,s1)) {
									fne->newVN = getNav(fne->vg);
									fne->newVN->context[0] = -1;
									fne->newVN->URIName = s;
								} else {
									fne->state = XPATH_EVAL_END;
									free( s);
									free(s1);
									return -1;
								}
								fne->state = XPATH_EVAL_END;
								free( s);
								free(s1);
								return 0;
							} else {
								fne->state = XPATH_EVAL_END;
								return -1;
							}
						} else
							return -1;
					} Catch (ee) {

					}
				}
			}

			break;
		case FN_KEY:
			throwException2( xpath_eval_exception," key() not yet implemented ");
			// break;
		}
		throwException2( xpath_eval_exception, " Function Expr can't eval to node set ");
	return -1;
}
double	evalNumber_fne (funcExpr *fne,VTDNav *vn){
	int ac;
	size_t len;
	UCSChar *tmpString = NULL;
	switch(fne->opCode){
			case FN_LAST:  /*if (argCount(fne)!=0 ){
								throwException2(invalid_argument,
										"last()'s  <funcExpr> argument count is invalid");
							}*/
						   return fne->contextSize;
			case FN_POSITION:  /* if (argCount(fne)!=0 ){
									throwException2(invalid_argument,
										"position()'s  <funcExpr> argument count is invalid");
								}*/

								return fne->position;

			case FN_COUNT: 		return count(fne, vn);

			case FN_NUMBER:		/*if (argCount(fne)!=1){
									throwException2(invalid_argument,
										"number()'s  <funcExpr> argument count is invalid");
								}*/
								return fne->al->e->evalNumber(fne->al->e,vn);

			case FN_SUM:	    return sum(fne,vn);
			case FN_FLOOR: 		/*if (argCount(fne)!=1 ){
									throwException2(invalid_argument,
										"floor()'s  <funcExpr> argument count is invalid");
								}*/
								return floor(fne->al->e->evalNumber(fne->al->e,vn));

			case FN_CEILING:	/*if (argCount(fne)!=1 ){
									throwException2(invalid_argument,
										"ceiling()'s  <funcExpr> argument count is invalid");
								}*/
								return ceil(fne->al->e->evalNumber(fne->al->e,vn));

			case FN_STRING_LENGTH:
								ac = fne->argCount1;
			    				if (ac == 0){
									exception e;
			    				    Try{
			    				        if (vn->atTerminal == TRUE){
			    				            tokenType type = getTokenType(vn,vn->LN);

											if (type == TOKEN_ATTR_NAME
			    				                || type == TOKEN_ATTR_NS){
													len = getStringLength(vn,vn->LN+1);
			    				                //return vn.toString(vn.LN+1).length();
			    				            } else {
												    len = getStringLength(vn,vn->LN);
			    				                //return vn.toString(vn.LN).length();
			    				            }
											return len;

			    				        }else {
			    				            int i = getText(vn);
			    				            if (i==-1)
			    				                return 0;
											else {
												return getStringLength(vn,i);
											}
			    				        }
			    				    }Catch ( e){
			    				        return 0;
			    				    }
			    				} else if (ac == 1){
									tmpString = fne->al->e->evalString(fne->al->e,vn);
									len = wcslen(tmpString);
									free(tmpString);
			    				    return len;
			    				} else {
									throwException2(invalid_argument,
										"string-length()'s  <funcExpr> argument count is invalid");
			    				}

			case FN_ROUND: 	//if (argCount(fne)==1 )
								return myround(fne->al->e->evalNumber(fne->al->e,vn));
							/*else {
								throwException2(invalid_argument,
									"round()'s  <funcExpr> argument count is invalid");
							}*/
			case FN_ABS:    //if (argCount(fne)==1 )
								return fabs(fne->al->e->evalNumber(fne->al->e,vn));
							/*else {
								throwException2(invalid_argument,
									"abs()'s  <funcExpr> argument count is invalid");
							}*/
			case FN_ROUND_HALF_TO_EVEN:
							return roundHalfToEven(fne, vn);
			case FN_ROUND_HALF_TO_ODD:
				    throwException2(other_exception, "functions not yet supported");
			
			default: if (fne->isBool){
						if (evalBoolean_fne(fne,vn))
							return 1;
						else
							return 0;
					 }else {
                         	double d  = 0, result;
							UCSChar *string = evalString_fne(fne,vn);
							UCSChar *temp;
							if (wcslen(string)==0){
								free(string);
								return d/d;
							}

							result = wcstod(string,&temp);
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
							free(string);
							return result;

					 }

					 }
}

UCSChar* evalString_fne (funcExpr *fne, VTDNav *vn){
	UCSChar *tmp;

	switch(fne->opCode){
			case FN_LOCAL_NAME:
				return getLocalName(fne,vn);
			case FN_NAMESPACE_URI:
				return getNameSpaceURI(fne, vn);
			case FN_NAME:
				return getName(fne,vn);
			case FN_STRING:
				return getString(fne,vn);

			case FN_CONCAT:
				return concat(fne,vn);
			case FN_SUBSTRING_BEFORE: return subStringBefore(fne,vn);
			case FN_SUBSTRING_AFTER: return subStringAfter(fne,vn);

			case FN_SUBSTRING: 	return subString(fne,vn);
			case FN_TRANSLATE:  return translate(fne,vn);
			
			case FN_NORMALIZE_SPACE: return normalizeString(fne,vn);

			case FN_CODE_POINTS_TO_STRING:throwException2(other_exception, "functions not yet supported");
  			case FN_UPPER_CASE: return upperCase(fne,vn);
  			case FN_LOWER_CASE: return lowerCase(fne,vn);
  			case FN_QNAME:
  			case FN_LOCAL_NAME_FROM_QNAME:
   			case FN_NAMESPACE_URI_FROM_QNAME:
  			case FN_NAMESPACE_URI_FOR_PREFIX:
  			case FN_RESOLVE_QNAME:
  			case FN_IRI_TO_URI:
  			case FN_ESCAPE_HTML_URI:
  			case FN_ENCODE_FOR_URI:
				throwException2(other_exception, "functions not yet supported");
			case FN_GENERATE_ID: return generateID(fne,vn);
			case FN_FORMAT_NUMBER: return formatNumber(fne,vn);
			case FN_SYSTEM_PROPERTY: return getSystemProperty(fne,vn);
			default: if (isBoolean_fne(fne)){
			    		if (evalBoolean_fne(fne,vn)== TRUE)
			    		    tmp = wcsdup(L"true");
			    		else
			    		    tmp = wcsdup(L"false");
						if (tmp == NULL){
							throwException2(out_of_mem,
								"allocate string failed in funcExpr's evalString()");
						}
						return tmp;
					 } else {
						 double d1 = 0;
						 double d = evalNumber_fne(fne,vn);
						 Boolean b = FALSE;
							if (d != d){
								tmp = wcsdup(L"NaN");
								b = TRUE;
							}
							else if ( d == 1/d1){
								tmp = wcsdup(L"Infinity");
								b = TRUE;
							}
							else if (d == -1/d1){
								tmp = wcsdup(L"-Infinity");
								b = TRUE;
							}	else
								tmp = malloc(sizeof(UCSChar)<<8);

							if (tmp == NULL) {
									throwException2(out_of_mem,
										"allocate string failed in funcExpr's evalString()");
							}
							if(b)
								return tmp;

						    if (d == (Long) d){
								swprintf(tmp,64,L"%d",(Long)d);
							} else {
								swprintf(tmp,64,L"%f", d);
							}
							return tmp;
					 }
	  }
}

/* evaluate boolean value of a functional expression*/
Boolean evalBoolean_fne (funcExpr *fne,VTDNav *vn){
	switch(fne->opCode){
			case FN_STARTS_WITH:
				/*if (argCount(fne)!=2){
					throwException2(invalid_argument,
						"starts-with()'s <funcExpr> argument count is invalid");
				}*/
				return startsWith_fe(fne,vn);

			case FN_CONTAINS:
				/*if (argCount(fne)!=2){
					throwException2(invalid_argument,
						"contains()'s <funcExpr> argument count is invalid");
				}*/
				return contains_fe(fne,vn);

			case FN_TRUE:
				/*if (argCount(fne)!=0){
					throwException2(invalid_argument,
						"true()'s <funcExpr> argument count is invalid");
				}*/
				return TRUE;
			case FN_FALSE:
				if (argCount(fne)!=0){
					throwException2(invalid_argument,
						"false()'s <funcExpr> argument count is invalid");
				}
				return FALSE;
			case FN_BOOLEAN:
				/*if (argCount(fne)!=1){
					throwException2(invalid_argument,
						"boolean()'s <funcExpr> argument count is invalid");
				}*/
				return fne->al->e->evalBoolean(fne->al->e, vn);
			case FN_NOT:
				/*if (argCount(fne)!=1){
					throwException2(invalid_argument,
						"not()'s <funcExpr> argument count is invalid");
				}*/
				return !fne->al->e->evalBoolean(fne->al->e,vn);
			case FN_LANG:
				/*if (argCount(fne)!=1){
					throwException2(invalid_argument,
						"boolean()'s <funcExpr> argument count is invalid");
				}*/
				return lang(fne, vn, fne->al->e->evalString(fne->al->e,vn));

			case FN_COMPARE:
				throwException2(other_exception, "functions not yet supported");
			
			case FN_ENDS_WITH:
				/*if (argCount(fne)!=2){
					throwException2(invalid_argument,
						"ends-with()'s <funcExpr> argument count is invalid");
				}*/
				return endsWith_fe(fne,vn);
			case FN_MATCH_NAME:return matchName(fne,vn);
		    case FN_MATCH_LOCAL_NAME: return matchLocalName(fne,vn);
		    case FN_NOT_MATCH_NAME:return !matchName(fne,vn);
		    case FN_NOT_MATCH_LOCAL_NAME: return !matchLocalName(fne,vn);
		    case FN_ELEMENT_AVAILABLE: return isElementAvailable(fne,vn);
		    case FN_FUNCTION_AVAILABLE: return isElementAvailable(fne,vn);
				
			default:
				if (isNumerical_fne(fne)){
					double d = evalNumber_fne(fne, vn);
					if (d==0 || d!=d)
						return FALSE;
					return TRUE;
				}else{
					UCSChar *tmp = evalString_fne(fne, vn);
					size_t len = wcslen(tmp);
					free(tmp);
					return len!=0;
				}
	}
}

Boolean isBoolean_fne (funcExpr *fne){
	return fne->isBool;
}

Boolean isNumerical_fne (funcExpr *fne){
	return fne->isNum;
}

Boolean isString_fne (funcExpr *fne){
	return fne->isStr;
}

Boolean isNodeSet_fne (funcExpr *fne){
	return FALSE;
}

Boolean requireContextSize_fne(funcExpr *fne){
	    if (fne->opCode == FN_LAST)
	        return TRUE;
	    else {
			aList *temp = fne->al;
	        while(temp!=NULL){
	            if (temp->e->requireContextSize(temp->e)){
	                return TRUE;
	            }
	            temp = temp->next;
	        }
	    }
	    return FALSE;
}

void	reset_fne(funcExpr *fne, VTDNav *vn){
		fne->a = 0;
	    //contextSize = 0;
		fne->state= XPATH_EVAL_START;
		if (fne->al!=NULL)
			reset_al(fne->al,vn);
}

void	setContextSize_fne(funcExpr *fne,int s){
	    if (fne->opCode == FN_LAST){
	        fne->contextSize = s;
	        //System.out.println("contextSize: "+size);
	    } else {
	        aList *temp = fne->al;
	        //boolean b = false;
	        while(temp!=NULL){
	            temp->e->setContextSize(temp->e,s);
	            temp = temp->next;
	        }
	    }
}

void	setPosition_fne(funcExpr *fne,int pos){
	    if (fne->opCode == FN_POSITION){
	        fne->position = pos;
	        //System.out.println("PO: "+size);
	    } else {
	        aList *temp = fne->al;
	        while(temp!=NULL){
	            temp->e->setPosition(temp->e,pos);
	            temp = temp->next;
	        }
	    }
}

void   toString_fne(funcExpr *fne, UCSChar* string){
	if (fne->al == NULL){
		wprintf(L"%ls()",fname(fne,fne->opCode));
	}else {
		wprintf(L"%ls(",fname(fne,fne->opCode));
		toString_al(fne->al,string);
		wprintf(L")");
	}
}
static Boolean contains_fe(funcExpr *fne, VTDNav *vn){
	Boolean b=FALSE;
	exception ee;
	UCSChar* s1=NULL;
	UCSChar* s2 = fne->al->next->e->evalString(fne->al->next->e,vn);
	if(fne->al->e->isNodeSet(fne->al->e)){
		//UCSChar* s1 = al->e->evalString( vn);
		int a = evalFirstArgumentListNodeSet(fne,vn);
		if (a==-1)
			return FALSE;
		Try {
			int t=getTokenType(vn,a);
			if (t!=TOKEN_STARTING_TAG && t!=TOKEN_DOCUMENT)
				b= contains(vn,a, s2);
			else
				b= XPathStringVal_Contains(vn,a,s2);
			free(s2);
			return b;
		}Catch(ee){
			free(s2);
			return FALSE;
		}		
	}
	
	s1 = fne->al->e->evalString(fne->al->e, vn);
	//Boolean b = FALSE;
	if (s1 == NULL || s2 == NULL)
		return FALSE;
	if (wcsstr(s1,s2)!=NULL)
		b = TRUE;
	free(s1);
	free(s2);
    return b;
}
static UCSChar* concat(funcExpr *fne, VTDNav *vn){
	size_t totalLen = 0,capacity = 16;aList *temp=NULL;
	size_t len = 0;
	UCSChar *result = NULL, *s = NULL, *tempBuf = NULL;
	result = malloc(sizeof(UCSChar)<<4);
	if (result==NULL){
		throwException2(out_of_mem,
			"String allocation failed in concat");
	}
	result[0]=0;/*end of string set*/

	//if (argCount(fne)>=2){
		temp = fne->al;
		while(temp!=NULL){
			/*perform concatenation here*/
			s = temp->e->evalString(temp->e,vn);
			len = wcslen(s);
			totalLen += len;
			if (totalLen+1> capacity){
				while(totalLen+1> capacity)
					capacity = (capacity<<1);
				tempBuf = result;
				result = realloc(result, capacity*sizeof(UCSChar));
				if (result == NULL) {
					free(tempBuf);
					free(s);
					throwException2(out_of_mem,
						"String allocation failed in concat");
				}
			}
			wcscat(result,s);
			temp = temp->next;
		}
		return result;
	//}
	//throwException2(invalid_argument,
	//	"concat()'s <funcExpr> argument count is invalid");
	//return NULL;
}
/* can be optimized to test whether the argument returns a VTD index */
static Boolean startsWith_fe(funcExpr *fne, VTDNav *vn){
	Boolean b = FALSE;
	exception ee;
	UCSChar* s1,*s2 = fne->al->next->e->evalString(fne->al->next->e,vn);
	if (fne->al->e->isNodeSet(fne->al)){
			//boolean b = false;
			int a = evalFirstArgumentListNodeSet(fne,vn);
			
	        if (a==-1)
	        	return wcslen(s2)==0;
	        else{
	        	Try{
	        		int t = getTokenType(vn,a);
	        		if (t!=TOKEN_STARTING_TAG&& t!=TOKEN_DOCUMENT)
	        			b= startsWith(vn,a, s2);
	        		else 
	        			b= XPathStringVal_StartsWith(vn,a,s2);
					free( s2);
					return b;
	        	}Catch(ee){
					free( s2);
					return FALSE;
	        	}
	        	
	        }								
		} 
	//UCSChar* s3 = NULL;
	s1 = fne->al->e->evalString(fne->al->e, vn);
	
	
	if (wcsstr(s1,s2)==s1)
		b = TRUE;
	free(s1);
	free(s2);
    return b;
}
/* can be optimized to test whether the argument returns a VTD index */
static Boolean endsWith_fe(funcExpr *fne, VTDNav *vn){
	Boolean b = FALSE;
	exception ee;size_t l1,l2;
	UCSChar* s1=NULL,*s2 = fne->al->next->e->evalString(fne->al->next->e,vn);
	if (fne->al->e->isNodeSet(fne->al->e)){
		int a = evalFirstArgumentListNodeSet(fne,vn);
		if (a==-1)
			return wcslen(s2)==0;
		else{
			Try{
				int t=getTokenType(vn,a);
				if (t!=TOKEN_STARTING_TAG && t!=TOKEN_DOCUMENT)
					b= endsWith(vn,a, s2);
				else
					b= XPathStringVal_EndsWith(vn,a, s2);
				free(s2);
				return b;
			}Catch(ee){
				free( s2);
				return FALSE;
			}
			
			
		}								
	}	
    
	s1 = fne->al->e->evalString(fne->al->e, vn);
	l1 = wcslen(s1);
	l2 = wcslen(s2);
	//UCSChar* s3 = NULL;
	
	if (wcsstr(s1+(l1-l2),s2)== (s1+l1-l2)){
		//
		b = TRUE;

	}
	free(s1);
	free(s2);
    return b;
}
static UCSChar* subString(funcExpr *fne, VTDNav *vn){
	UCSChar *str;
	size_t len;
	if (fne->argCount1==2){
		double d1;
		int temp;
		
		str = fne->al->e->evalString(fne->al->e,vn);
		len = wcslen(str);
		d1 = fne->al->next->e->evalNumber(fne->al->next->e,vn);
		
		if (d1!=d1 || d1>len)
			return createEmptyString();
		
		d1--; //change it to zero based index
		
		temp = max((int)d1,0);

		len -= temp; //calculate the length to copy
		wcsncpy(str,str+temp,len);
		*(str+len) = 0;
		
		return str;

	} else if (fne->argCount1==3){
		double d1, d2;
		int temp1, endIdx;
		size_t cpLen;

		str = fne->al->e->evalString(fne->al->e,vn);
		len = wcslen(str);

		d1 = floor(fne->al->next->e->evalNumber(fne->al->next->e,vn)+0.5);
		d2 = floor(fne->al->next->next->e->evalNumber(fne->al->next->next->e,vn)+0.5);

		if ((d1+d2)!=(d1+d2) || d1>len)
			return createEmptyString();

		d1--;//change it to zero based index
		temp1 = max((int)d1,0);

		endIdx = (int)d1 + (int)d2;
		cpLen = min((int)len, endIdx);

		cpLen -= temp1; //calculate copy length

		wcsncpy(str,str+temp1,cpLen);
		*(str+cpLen)=0;

		return str;
	}
	throwException2(invalid_argument,
		"substring()'s <funcExpr> argument count is invalid");
	return NULL;
}

static UCSChar* subStringBefore(funcExpr *fne, VTDNav *vn){
	if ((fne)->argCount1 == 2){
		UCSChar* s1 = fne->al->e->evalString(fne->al->e, vn);
		UCSChar* s2 = fne->al->next->e->evalString(fne->al->next->e,vn);
		UCSChar* temp = NULL;
		if ((temp=wcsstr(s1,s2))!=NULL){
			*(temp) = 0;
			free(s2);
			return s1;
		}
		else{
			*s2 = 0;
			free(s1);
			return s2;
		}
	}
	throwException2(invalid_argument,
		"substring-before()'s <funcExpr> argument count is invalid");
	return NULL;
}

static UCSChar* subStringAfter(funcExpr *fne, VTDNav *vn){
	if ((fne)->argCount1 == 2){
		UCSChar* s1 = fne->al->e->evalString(fne->al->e, vn);
		UCSChar* s2 = fne->al->next->e->evalString(fne->al->next->e,vn);
		size_t /*len1=wcslen(s1),*/len2=wcslen(s2);
		UCSChar* temp = NULL;
		if ((temp=wcsstr(s1,s2))!=NULL){
			size_t len = wcslen(temp);
			wcsncpy(s1,temp+len2,len-len2);
			*(s1+len-len2) = 0;
			free(s2);
			return s1;
		}
		else{
			*s2 = 0;
			free(s1);
			return s2;
		}
	}
	throwException2(invalid_argument,
		"substring-after()'s <funcExpr> argument count is invalid");
	return NULL;

}

static UCSChar* translate(funcExpr *fne, VTDNav *vn){

	if (argCount(fne)==3)
	{
		UCSChar* repStr;
		UCSChar* temp;
		size_t lenRepStr, lenIdxStr;
		int idx;

		UCSChar* str = fne->al->e->evalString(fne->al->e,vn);
		UCSChar* idxStr = fne->al->next->e->evalString(fne->al->next->e,vn);

		if(str == NULL || idxStr == NULL) return str;

		repStr = fne->al->next->next->e->evalString(fne->al->next->next->e,vn);		
		lenRepStr = (repStr != NULL) ? wcslen(repStr) : 0;
		
		lenIdxStr = wcslen(idxStr);
		
		//create a temp string to hold used char
		temp = (UCSChar*) malloc(sizeof(UCSChar) * (lenIdxStr + 1));
		if (temp == NULL)
		{
			throwException2(out_of_mem,
			"String allocation failed in translate");
		}

		*temp = L'\0';

		for (idx = 0; idx < (int)lenIdxStr; idx++)
		{

			UCSChar idxChar = idxStr[idx];
			if(wcschr(temp,idxChar) == NULL) //this char has not been used
			{			
				if(idx < (int)lenRepStr) //replace
				{
					UCSChar repChar = repStr[idx];
					int i;

					for(i = 0; str[i] != '\0'; i++)
					{
						if(str[i] == idxChar) str[i] = repChar;
					}
				}
				else //remove
				{
					int i,j;
					for(i = j = 0; str[i] != '\0'; i++)
					{
						if(str[i] != idxChar) str[j++] = str[i];
					}

					str[j] = '\0';
				}

				wcsncat(temp,&idxChar,1);
			}
		}
		
		free(temp);

		return str;
	}

	throwException2(invalid_argument,
		"translate()'s <funcExpr> argument count is invalid");

	return NULL;
}

static UCSChar* normalizeString(funcExpr *fne, VTDNav *vn){
	exception e;
	if (fne->argCount1 == 0){
		UCSChar *s = NULL;
		
		Try{
			if (vn->atTerminal)
			{
				int ttype = getTokenType(vn,vn->LN);
				if (ttype == TOKEN_CDATA_VAL)
					s =toRawString(vn,vn->LN);
				else if (ttype == TOKEN_ATTR_NAME
					|| ttype == TOKEN_ATTR_NS){
						s = toNormalizedString(vn,vn->LN+1);
				}else{
					s= toNormalizedString(vn,vn->LN);
				}
			}else{
				int i = getCurrentIndex(vn);
	            int t = getTokenType(vn,i);
	            if (t==TOKEN_STARTING_TAG || t==TOKEN_DOCUMENT){
	            	s = toNormalizedXPathString(vn,i);
	            }else
	                s= toNormalizedString(vn,i);
			}
			return s;
		}
		Catch(e){
			return createEmptyString();
		}
	} else if (fne->argCount1 ==1){
		UCSChar *s=NULL;
		if (fne->al->e->isNodeSet(fne->al->e)){
			//boolean b = false;
			int a = evalFirstArgumentListNodeSet(fne,vn);
		    if (a==-1)
		       	return wcsdup(L""); 
		    else {		        	
		       	Try{
		        		int t = getTokenType(vn,a);
		        		if (t==TOKEN_STARTING_TAG || t==TOKEN_DOCUMENT){
		        			s = toNormalizedXPathString(vn,a);
		        		}else
		        			s = toNormalizedString(vn,a); 
		        } Catch (e){
		        }
		        return s;	
		    }	    	
	   	}else {
			UCSChar *s = fne->al->e->evalString(fne->al->e,vn);
			return normalize(s);
	    }
	}
	return NULL;
}

static UCSChar* normalize(UCSChar *s){
	 int os=0; size_t i,len;
	 len = wcslen(s);
	 // strip off leading ws
	 for (i = 0; i < len; i++)
	 {
		 if (isWS(s[i]))
		 {
		 }
		 else
		 {
			 break;
		 }
	 }
	 while (i < len)
	 {
		 UCSChar c = s[i];
		 if (!isWS(c))
		 {
			 s[os++]=c;
			 i++;
		 }
		 else
		 {
			 while (i < len)
			 {
				 c = s[i];
				 if (isWS(c))
					 i++;
				 else
					 break;
			 }
			 if (i < len){
				 s[os++]=' ';
			 }
		 }
	 }
	 s[os]=0;
	 return s;
}

static Boolean isWS(UCSChar c)
{
	if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
		return TRUE;
	return FALSE;
}

int adjust_fne(funcExpr *fne, int n){
	switch(fne->opCode){
		case FN_COUNT:
		case FN_SUM:
			return fne->al->e->adjust(fne->al->e,n);

		default:
			return 0;
	}
}


Boolean lang(funcExpr *fne, VTDNav *vn, UCSChar* s){
	exception ee;
	Boolean b = FALSE;
	push2(vn);
	Try {
		while (getCurrentDepth(vn) >= 0) {
			int i = getAttrVal(vn,L"xml:lang");
			if (i!=-1){
				b = matchTokenString(vn,i,s);
				break;
			}
			toElement(vn,PARENT);
		}
	} Catch (ee) {

	}
	pop2(vn);
	return b;
}

//int evalFirstArgumentListNodeSet(funcExpr *fne, VTDNav *vn){
//	exception e;
//	int size, a;
//	push2(vn);
//	size = vn->contextBuf2->size;
//    a = -1;
//    Try {
//		a = fne->al->e->evalNodeSet(fne->al->e, vn);
//        if (a != -1) {
//           if (getTokenType(vn, a) == TOKEN_ATTR_NAME) {
//                a++;
//           }
//           if (getTokenType(vn, a) == TOKEN_STARTING_TAG) {
//               a = getText(vn);
//           }
//       }	            
//    } Catch (e) {
//    }
//	vn->contextBuf2->size = size;
//	fne->al->e->reset(fne->al->e,vn);
//    pop2(vn);
//    return a;
//}
//
//int evalFirstArgumentListNodeSet2(funcExpr *fne, VTDNav *vn){
//	exception e;
//	int size, a;
//	push2(vn);
//	size = vn->contextBuf2->size;
//    a = -1;
//    Try {
//		a = fne->al->e->evalNodeSet(fne->al->e, vn);            
//    } Catch (e) {
//    }
//	vn->contextBuf2->size = size;
//	fne->al->e->reset(fne->al->e,vn);
//    pop2(vn);
//    return a;
//}

static double roundHalfToEvenPositive(double value, long precision){
		int i;
		double result = 0;
		const double ROUNDING_EPSILON  = 0.00000001;
	    Long dec = 1;
		Long intPart = (Long)value;
	    
	    //shif the decimal point by precision
	    Long absPre = abs(precision);
	    
	    for(i = 0; i < absPre; i++){
	    	dec *= 10;
	    }
	    
	    if(precision > 0) value *= dec;
	    else if (precision < 0)value /= dec;
	    	    
	    //'value' is exctly halfway between two integers
	    if(fabs(value -((double)intPart +(double)0.5)) < ROUNDING_EPSILON){
	    	// 'ipart' is even 
	    	if(intPart%2 == 0){
	    		result = intPart;
	    	}else{// nearest even integer
	    		result = (Long)ceil( intPart + (double)0.5 );
	    	}
	    }else{
	    	//use the usual round to closest	    
	    	result = myround(value);
	    }
	    
	    //shif the decimal point back to where it was
	    if(precision > 0) result /= dec;
	    else if(precision < 0) result *= dec;
	    
		return result;
		
		
	}

static double roundHalfToEven(funcExpr *fne, VTDNav *vn) {
		double value;
		Long precision;
		int numArg = argCount(fne);

	    if (numArg < 1 || numArg > 2){
	    	throwException2(invalid_argument,
				"Argument count for roundHalfToEven() is invalid.");	    	
	    }

		value = (double)fne->al->e->evalNumber(fne->al->e, vn);	    
	    precision = (numArg == 2)? (Long)floor((double)fne->al->next->e->evalNumber(fne->al->next->e,vn)+0.5) : 0;
	    
	    if(value < 0) return -roundHalfToEvenPositive(-value, precision);	    
	    else return roundHalfToEvenPositive(value, precision);
	}


Boolean isFinal_fne(funcExpr *e){
	aList* temp = e->al;
	Boolean s= FALSE;
	if (temp ==NULL)
		return FALSE;
	if (temp->e==NULL)
		return FALSE;
	s=TRUE;
	while(temp!=NULL){
		s= s && temp->e->isFinal(temp->e);
		if (!s)
			return FALSE;
		temp = temp->next;
	}
	return s;	
}
		
void markCacheable2_fne(funcExpr *e){
	aList *temp = e->al;
	while(temp!=NULL ){
		if (temp->e!=NULL){
			if (temp->e->isFinal(temp->e) && temp->e->isNodeSet(temp->e)){
				cachedExpr *ce = createCachedExpr(temp->e);
				temp->e = (expr *)ce;
			}
			temp->e->markCacheable2(temp->e);
		}
		temp = temp->next; 
	}
}
void markCacheable_fne(funcExpr *e){
	aList *temp = e->al;
	while(temp!=NULL){
		if (temp->e!=NULL)
			temp->e->markCacheable(temp->e);
		temp = temp->next; 
	}
}
void clearCache_fne(funcExpr *e){
	aList *temp =e->al;
	while(temp!=NULL ){
		if (temp->e!=NULL){				
			temp->e->clearCache(temp->e);
		}
		temp = temp->next; 
	}
}
Boolean checkArgumentCount(funcExpr *e){
	switch(e->opCode){
		case FN_LAST: 			return e->argCount1==0;
		case FN_POSITION: 		return e->argCount1==0;
		case FN_COUNT: 			return (e->argCount1==1 && e->al->e->isNodeSet(e->al->e));
		
		case FN_LOCAL_NAME: 	return (e->argCount1==0 ||(e->argCount1==1 && e->al->e->isNodeSet(e->al->e)));
		case FN_NAMESPACE_URI: 	return (e->argCount1==0 ||(e->argCount1==1 && e->al->e->isNodeSet(e->al->e)));
		case FN_NAME: 			return (e->argCount1==0 ||(e->argCount1==1 && e->al->e->isNodeSet(e->al->e)));
		case FN_STRING: 			return e->argCount1 < 2;
		case FN_CONCAT: 			return e->argCount1 > 1;
		case FN_STARTS_WITH:		return e->argCount1 ==2;
		case FN_CONTAINS: 		return e->argCount1 ==2;
		case FN_SUBSTRING_BEFORE: return e->argCount1==2;
		case FN_SUBSTRING_AFTER: 	return e->argCount1==2;
		case FN_SUBSTRING: 		return e->argCount1==2 || e->argCount1==3;
		case FN_STRING_LENGTH: 	return e->argCount1<2;
		case FN_NORMALIZE_SPACE: 	return e->argCount1 <2;
		case FN_TRANSLATE:	 	return e->argCount1 ==3;
		case FN_BOOLEAN: 			return e->argCount1 ==1;
		case FN_NOT: 			    return e->argCount1 ==1;
		case FN_TRUE: 			return e->argCount1 ==0;
		case FN_FALSE: 			return e->argCount1 ==0;
		case FN_LANG: 			return (e->argCount1==1);
		case FN_NUMBER:			return e->argCount1==1;
		case FN_SUM: 			    return (e->argCount1==1 && e->al->e->isNodeSet(e->al->e));
		case FN_FLOOR: 			return e->argCount1==1;
		case FN_CEILING: 			return e->argCount1==1;
		case FN_ROUND:			return e->argCount1==1;
		case FN_ABS:				return e->argCount1==1;
		case FN_ROUND_HALF_TO_EVEN :
										return e->argCount1==1 || e->argCount1 == 2;
		case FN_ROUND_HALF_TO_ODD:
										return e->argCount1==1 || e->argCount1 == 2;
		case FN_CODE_POINTS_TO_STRING:
		    							break;
		case FN_COMPARE:			break;
		case FN_UPPER_CASE:		return e->argCount1==1;
		case FN_LOWER_CASE:		return e->argCount1==1;
		case FN_ENDS_WITH:		return e->argCount1==2;
		case FN_QNAME:			break;
		case FN_LOCAL_NAME_FROM_QNAME:
		    							break;
		case FN_NAMESPACE_URI_FROM_QNAME:
		    							break;
		case FN_NAMESPACE_URI_FOR_PREFIX:
		    							break;
		case FN_RESOLVE_QNAME:	break;
		case FN_IRI_TO_URI:    	break;
		case FN_ESCAPE_HTML_URI:	break;
		case FN_ENCODE_FOR_URI:	break;
		case FN_MATCH_NAME:		return e->argCount1==1 || e->argCount1 == 2;
		case FN_MATCH_LOCAL_NAME: return e->argCount1==1 || e->argCount1 == 2;
		case FN_NOT_MATCH_NAME:		return e->argCount1==1 || e->argCount1 == 2;
		case FN_NOT_MATCH_LOCAL_NAME: return e->argCount1==1 || e->argCount1 == 2;
		case FN_CURRENT:			return e->argCount1==0;
		case FN_GENERATE_ID : 	return e->argCount1==0 || (e->argCount1 ==1 && e->al->e->isNodeSet(e->al->e));
		case FN_FORMAT_NUMBER:  	return e->argCount1==2 || e->argCount1== 3;
		case FN_KEY:				return e->argCount1==2;
		case FN_DOCUMENT:			return e->argCount1==1 || (e->argCount1==2 && e->al->next->e->isNodeSet(e->al->e));	
		case FN_SYSTEM_PROPERTY: 	return e->argCount1==1 && e->al->e->isString(e->al->e) ;
		case FN_ELEMENT_AVAILABLE: return e->argCount1==1 && e->al->e->isString(e->al->e);
		case FN_FUNCTION_AVAILABLE: return e->argCount1==1 && e->al->e->isString(e->al->e);
		}
		return FALSE;	
}
	


UCSChar *formatNumber(funcExpr *e, VTDNav *vn){return createEmptyString();}
UCSChar *generateID(funcExpr *e, VTDNav *vn){
	if (e->argCount1== 0){
		UCSChar *str = malloc(sizeof(UCSChar)*14);
		swprintf(str,14, L"v%d", getCurrentIndex2(vn));
		return str;
		//return "v"+vn->getCurrentIndex2();
	}else if (e->argCount1== 1) {
		UCSChar *str = malloc(sizeof(UCSChar)*14);
		int i=evalFirstArgumentListNodeSet2(e,vn);
		swprintf(str,14, L"v%d",i);
		return str;
	} else 
	    throwException2(invalid_argument,
			"generate-id()'s argument count is invalid");
}


int evalFirstArgumentListNodeSet(funcExpr *e, VTDNav *vn){
	int size, a,k;
	exception ee;
	push2(vn);
    size = vn->contextBuf2->size;
    a = 0x7fffffff,k=-1;
    Try {
		if (e->al->e->needReordering) {
			while ((k = e->al->e->evalNodeSet(e->al->e,vn)) != -1) {
				//a = evalNodeSet(vn);
				if (k<a)
					a = k;
			}
			if (a == 0x7fffffff)
				a = -1;
		}
		else
            a = e->al->e->evalNodeSet(e->al->e,vn);
            if (a != -1) {
            	int t = getTokenType(vn,a);
                if (t == TOKEN_ATTR_NAME) {
                    a++;
                }
                else if (t == TOKEN_PI_NAME){
                	//if (a+1 < vn.vtdSize || vn.getTokenType(a+1)==VTDNav.TOKEN_PI_VAL)
                	a++;
                	//else 
                	//	a=-1;
                }
                 
                //else if (t== VTDNav.T)
            }	            
        } Catch (ee) {
        }
        vn->contextBuf2->size = size;
		e->al->e->reset( e->al->e,vn);
        pop2(vn);
        return a;
}

int evalFirstArgumentListNodeSet2(funcExpr *e, VTDNav *vn){
	int size, a,k;
	exception ee;
	push2(vn);
    size = vn->contextBuf2->size;
    a = 0x7ffffff,k= -1;
    Try {
		if (e->al->e->needReordering) {
			while ((k = e->al->e->evalNodeSet(e->al->e,vn)) != -1) {
				//a = evalNodeSet(vn);
				if (k<a)
					a = k;
			}
			if (k == 0x7fffffff)
				a = -1;
		}
		else
           a = e->al->e->evalNodeSet(e->al->e,vn);	            
    } Catch (ee) {
    }
    vn->contextBuf2->size = size;
    e->al->e->reset(e->al->e,vn);
    pop2(vn);
    return a;
}

Boolean matchName(funcExpr *e, VTDNav *vn){
	int a,type;exception ee;UCSChar *s1;
	Boolean b=FALSE;
	if (e->argCount1 == 1) {
		a = getCurrentIndex(vn);
		type = getTokenType(vn,a);
		s1 = e->al->e->evalString(e->al->e,vn);
		if (type == TOKEN_STARTING_TAG
			|| type == TOKEN_ATTR_NAME
			|| type == TOKEN_PI_NAME) {
				Try {
					b= matchRawTokenString(vn,a, s1);
				} Catch (ee) {
					b =FALSE;
				}
				free( s1);
				return b;
		} else
			return FALSE;
	} else if (e->argCount1 == 2) {
		a = evalFirstArgumentListNodeSet2(e,vn);
		s1 = e->al->next->e->evalString(e->al->next->e,vn);
		Try{
			if (a == -1 || vn->ns == FALSE)
				b = (wcscmp(L"",s1) == 0);
			//return false;
			else {
				type = getTokenType(vn,a);
				if (type ==  TOKEN_STARTING_TAG
					|| type ==  TOKEN_ATTR_NAME
					|| type ==  TOKEN_PI_NAME)
					b =matchRawTokenString(vn,a, s1);

			}
			free(s1);
			return b;
		} Catch (ee) {
		}
		return FALSE;
	} else
		throwException2(invalid_argument,
		"matchName()'s argument count is invalid");
	return FALSE;
}

Boolean matchLocalName(funcExpr *e,VTDNav *vn){
	Boolean b= FALSE;int index,type; UCSChar *s1;exception ee;
	 if (e->argCount1== 1){
	        Try{
	            index = getCurrentIndex(vn);
	            type = getTokenType(vn,index);
	            s1 = e->al->e->evalString(e->al->e,vn);
	            if (vn->ns && (type == TOKEN_STARTING_TAG 
	                    || type == TOKEN_ATTR_NAME)) {
                    int offset = getTokenOffset(vn,index);
                    int length = getTokenLength(vn,index);
                    if (length < 0x10000 || (length>>16)==0){
                    	b= (compareRawTokenString(vn,index, s1)==0);//vn.toRawString(index);
                    }
                    else {
                        int preLen = length >> 16;
                        int QLen = length & 0xffff;
                        if (preLen != 0){
                        	 b= (compareRawTokenString2(vn,offset + preLen+1, QLen
                                        - preLen - 1,s1)==0);                        	
                        }
                    }
                } else if (type == TOKEN_PI_NAME){
                	b= (compareRawTokenString(vn,index, s1)==0);
                } else 
                    b= (wcscmp(s1,L"")==0);
				free( s1);
				return b;
	        }Catch(ee){
	        	 return FALSE; // this will never occur
	        }
	        
	    } else if (e->argCount1 == 2){
	        int a=evalFirstArgumentListNodeSet2(e,vn);
	        s1 = e->al->next->e->evalString(e->al->next->e,vn);
			if (a == -1 || vn->ns == FALSE)
			    b= (wcscmp(L"",s1)==0);
			type = getTokenType(vn,a);
			if (type==TOKEN_STARTING_TAG || type== TOKEN_ATTR_NAME){
			    //return "".equals(s1);
			Try {			    
			    int offset = getTokenOffset(vn,a);
			    int length = getTokenLength(vn,a);
			    if (length < 0x10000 || (length>> 16)==0)
			       b= compareRawTokenString(vn,a,s1)==0;
			    else {
			        int preLen = length >> 16;
			        int QLen = length & 0xffff;
			        if (preLen != 0)
			            b= compareRawTokenString2(vn,offset + preLen+1, 
			                    QLen - preLen - 1,s1)==0;
			        /*else {
			            return vn.toRawString(offset, QLen);
			        }*/
			    }
			} Catch (ee) {
				 b= (wcscmp(L"",s1)==0); // this will almost never occur
			}		
			free( s1);
			return b;
			}else if (type == TOKEN_PI_NAME){
				Try{
					b= (compareRawTokenString(vn,a,s1)==0);
				}Catch(ee){
					b= (wcscmp(L"",s1)==0);
				}
				free(s1);
				return b;
			}
			 b= (wcscmp(L"",s1)==0);
			 free( s1);
			 return b;
	    } else 
	        throwException2
			(invalid_argument,"match-local-name()'s argument count is invalid");
		return FALSE;
}

UByte *doubleCapacity(UByte *b, size_t cap){
	b = realloc(b,cap);
	if (b == NULL){
		throwException2(out_of_mem,"Byte allocation failed in doubleCapacity");
	}
	return b;
}


int getStringVal(funcExpr *fne,VTDNav *vn,int i){
	tokenType t = getTokenType(vn,i);
	if (t == TOKEN_STARTING_TAG){
		int i1 = getText(vn);
		return i1;
	}
	else if (t == TOKEN_ATTR_NAME
		|| t == TOKEN_ATTR_NS ||t==TOKEN_PI_NAME)
		return i+1;
	else
		 return i;
}

funcName getFuncOpCode_fne(funcExpr *e) {
	return e->opCode;
}

void addArg_fne(funcExpr *fne, expr *e) {
	aList *al = fne->al; 
	if (fne->al == NULL) {
		fne->al = malloc(sizeof(aList));
		fne->al->e = e;
		fne->argCount1++;
		return;
	}
	while (al->next != NULL) {
		al = al->next;
	}
	fne->al->next = malloc(sizeof(aList));
	//al = new Alist();
	fne->al->next->e = e;
	fne->argCount1++;
}
