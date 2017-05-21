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
#include "vtdNav.h"
#include "indexHandler.h"
#include "elementFragmentNs.h"
#include "fastIntBuffer.h"
#include "bookMark.h"

static Long getChar(VTDNav *vn,int offset);
static Long getCharResolved(VTDNav *vn,int offset);
static int getCharUnit(VTDNav *vn, int index);
static Long handle_utf8(VTDNav *vn, Long temp, int offset);
static Long handle_utf16le(VTDNav *vn, int offset);
static Long handle_utf16be(VTDNav *vn, int offset);
static inline Boolean isElement(VTDNav  *vn, int index);
static inline Boolean isElementOrDocument(VTDNav *vn, int index);
static inline Boolean isWS(int ch);
//static Boolean matchRawTokenString1(VTDNav *vn, int offset, int len, UCSChar *s);
static Boolean matchRawTokenString2(VTDNav *vn, Long l, UCSChar *s);
static Boolean matchRawTokenString3(VTDNav *vn, int index, UCSChar *s);
//static Boolean matchTokenString1(VTDNav *vn, int offset, int len, UCSChar *s);
static Boolean matchTokenString2(VTDNav *vn, Long l, UCSChar *s);
static inline int NSval(VTDNav *vn, int i);
static int parseInt2(VTDNav *vn, int index, int radix);
static Long parseLong2(VTDNav *vn, int index, int radix);

static Boolean resolveNS(VTDNav *vn, UCSChar *URL);
static Boolean resolveNS2(VTDNav *vn, UCSChar *URL, int offset, int len); //UCSChar *ln);
static int lookupNS2(VTDNav *vn, int offset, int len);
static Long getChar4OtherEncoding(VTDNav *vn, int offset);
static int decode(VTDNav *vn,int offset);
//static int compareRawTokenString2(VTDNav *vn, int offset, int len, UCSChar *s);
static int compareTokenString2(VTDNav *vn, int offset, int len, UCSChar *s);
static UCSChar *toStringUpperCase2(VTDNav *vn, int os, int len);
static UCSChar *toStringLowerCase2(VTDNav *vn, int os, int len);
//static UCSChar *toRawStringUpperCase(VTDNav *vn, int index);
//static UCSChar *toRawStringLowerCase(VTDNav *vn, int index);
static UCSChar *toRawStringUpperCase2(VTDNav *vn, int os, int len);
static UCSChar *toRawStringLowerCase2(VTDNav *vn, int os, int len);
static void resolveLC_l1(VTDNav *vn);
static void resolveLC_l2(VTDNav *vn);
static void resolveLC_l3(VTDNav *vn);
static void _resolveLC_l3(VTDNav_L5 *vn);
static void resolveLC_l4(VTDNav_L5 *vn);
static void resolveLC_l5(VTDNav_L5 *vn);
static void recoverNode_l1(VTDNav *vn,int index);
static void recoverNode_l2(VTDNav *vn,int index);
static void recoverNode_l3(VTDNav *vn,int index);
static void _recoverNode_l3(VTDNav_L5 *vn,int index);
static void recoverNode_l4(VTDNav_L5 *vn,int index);
static void recoverNode_l5(VTDNav_L5 *vn,int index);
static int compareNormalizedTokenString2(VTDNav *vn,int offset, int len, UCSChar *s); 
static void _freeVTDNav(VTDNav *vn);
static Boolean _toNode(VTDNav *vn, navDir direction);
static Boolean _toNode_L5(VTDNav_L5 *vn, navDir direction);
static Boolean _toElement(VTDNav *vn, navDir direction);
static Boolean _toElement2(VTDNav *vn, navDir direction, UCSChar *en);
static Boolean _toElementNS(VTDNav *vn, navDir direction, UCSChar *URL, UCSChar *ln);
static Boolean _writeIndex_VTDNav(VTDNav *vn, FILE *f);
static Boolean _writeIndex2_VTDNav(VTDNav *vn, char *fileName);
static Boolean _writeSeparateIndex_VTDNav(VTDNav *vn, char *vtdIndex);
static VTDNav *_duplicateNav(VTDNav *vn);
static VTDNav *_cloneNav(VTDNav *vn);
static void _recoverNode(VTDNav *vn, int index);
static void _resolveLC(VTDNav *vn);
static void _resolveLC_L5(VTDNav_L5 *vn);
//static void _freeVTDNav_L5(VTDNav_L5 *vn);
//Free VTDNav object
static void _freeVTDNav(VTDNav *vn);
//static int _iterate_L5(VTDNav_L5 *vn, int dp, UCSChar *en, Boolean special);
/* DupliateNav duplicates an instance of VTDNav but doesn't retain the original node position*/
static VTDNav *_duplicateNav_L5(VTDNav_L5 *vn);
/* ClineNav duplicates an instance of VTDNav, also copies node position over */
static VTDNav *_cloneNav_L5(VTDNav_L5 *vn);
static void _recoverNode_L5(VTDNav_L5 *vn, int index);
//if ns is FALSE, return FALSE immediately
//static int _iterateNS_L5(VTDNav_L5 *vn, int dp, UCSChar *URL, UCSChar *ln);

// This function is called by selectElement_P in autoPilot
//static Boolean _iterate_preceding_L5(VTDNav_L5 *vn,UCSChar *en, int* a, Boolean special);

// This function is called by selectElementNS_P in autoPilot
//static Boolean _iterate_precedingNS_L5(VTDNav_L5 *vn,UCSChar *URL, UCSChar *ln, int* a);
// This function is called by selectElement_F in autoPilot
//static Boolean _iterate_following_L5(VTDNav_L5 *vn,UCSChar *en, Boolean special);
// This function is called by selectElementNS_F in autoPilot
//static Boolean _iterate_followingNS_L5(VTDNav_L5 *vn, UCSChar *URL, UCSChar *ln);

//Load the context info from ContextBuffer.
//Info saved including LC and current state of the context 
static Boolean _pop_L5(VTDNav_L5 *vn);
static Boolean _pop(VTDNav *vn);
static Boolean _pop2(VTDNav *vn);
static Boolean _push(VTDNav *vn);
static Boolean _push2(VTDNav *vn);
static Boolean _pop2_L5(VTDNav_L5 *vn);
static void _sampleState(VTDNav *vn, FastIntBuffer *fib);
//Store the context info into the ContextBuffer.
//Info saved including LC and current state of the context 
static Boolean _push_L5(VTDNav_L5 *vn);
static Boolean _push2_L5(VTDNav_L5 *vn);
static void _sampleState_L5(VTDNav_L5 *vn, FastIntBuffer *fib);
static Boolean _toElement_L5(VTDNav_L5 *vn, navDir direction);
static Boolean _toElement2_L5(VTDNav_L5 *vn, navDir direction, UCSChar *en);
static Boolean _toElementNS_L5(VTDNav_L5 *vn, navDir direction, UCSChar *URL, UCSChar *ln);

static Boolean _writeIndex_VTDNav_L5(VTDNav_L5 *vn, FILE *f);
static Boolean _writeIndex2_VTDNav_L5(VTDNav_L5 *vn, char *fileName);
static Boolean _writeSeparateIndex_VTDNav_L5(VTDNav_L5 *vn, char *fileName);


static inline void sync(VTDNav *vn, int depth, int index);

static Boolean nodeToElement_L5(VTDNav *vn, int dir);

static Boolean nodeToElement(VTDNav *vn, int dir);

static Boolean toNode_LastChild(VTDNav *vn);

static Boolean toNode_LastChild_L5(VTDNav_L5 *vn);

static Boolean toNode_PrevSibling(VTDNav *vn);

static Boolean toNode_PrevSibling_L5(VTDNav_L5 *vn);

static void _sync(VTDNav *vn,int depth, int index);

static void _sync_L5(VTDNav_L5 *vn, int depth, int index);

static Boolean _verifyNodeCorrectness(VTDNav *vn);

static Boolean _verifyNodeCorrectness_L5(VTDNav_L5 *vn);

static int _toString(VTDNav *vn, UCSChar *s, int index, int offset);

static int _toStringLowerCase(VTDNav *vn, UCSChar *s, int index, int offset);

static int _toStringUpperCase(VTDNav *vn, UCSChar *s, int index, int offset);

static UCSChar* _getXPathStringVal(VTDNav *vn, int index,short mode);
/*Create VTDNav object*/
static Long handle_utf8(VTDNav *vn, Long temp, int offset){
	int c,d,a,i;
	Long val;
	//temp = vn->XMLDoc[vn->currentOffset];

	switch (UTF8Char_byteCount(temp)) {

			case 2 :
				c = 0x1f;
				d = 6;
				a = 1;
				break;
			case 3 :
				c = 0x0f;
				d = 12;
				a = 2;
				break;
			case 4 :
				c = 0x07;
				d = 18;
				a = 3;
				break;
			case 5 :
				c = 0x03;
				d = 24;
				a = 4;
				break;
			case 6 :
				c = 0x01;
				d = 30;
				a = 5;
				break;
			default :{
				throwException(nav_exception,0,
					"navigation exception during getChar",
					"UTF 8 encoding error: should never happen");
					 }
	}

	val = (temp & c) << d;
	i = a - 1;
	while (i >= 0) {
		temp = vn->XMLDoc[offset + a - i];
		if ((temp & 0xc0) != 0x80){
			throwException(nav_exception,0,
				"navigation exception during getChar",
				"UTF 8 encoding error: should never happen");
		}
		val = val | ((temp & 0x3f) << ((i<<2)+ (i<<1)));
		i--;
	}

	return val | ((Long)(a+1)<<32);
}

static Long handle_utf16le(VTDNav *vn, int offset){	
	int val,temp =
		vn->XMLDoc[(offset << 1) + 1]
	<< 8 | vn->XMLDoc[offset << 1];
	if (temp < 0xdc00 || temp > 0xdfff) {
		if (temp == '\r') {
			if (vn->XMLDoc[(offset << 1) + 2] == '\n'
				&& vn->XMLDoc[(offset << 1) + 3] == 0) {
					return '\n' | (2LL<<32) ;
			} else {
				return '\n' | (1LL<<32);
			}
		}
		return temp | (1LL<<32);
	} else {
		val = temp;
		temp =
			(vn->XMLDoc[(offset << 1) + 3]
		<< 8) | vn->XMLDoc[(offset << 1) + 2];
		if (temp < 0xd800 || temp > 0xdc00) {
			throwException(nav_exception,0,
				"navigation exception during getChar",
				"UTF 16 LE encoding error: should never happen");
		}
		val = ((temp - 0xd800) << 10) + (val - 0xdc00) + 0x10000;

		return val | (2LL<<32);
	}
}
static Long handle_utf16be(VTDNav *vn, int offset){
	int val, temp =
		vn->XMLDoc[offset << 1]
	<< 8 | vn->XMLDoc[(offset << 1) + 1];
	if ((temp < 0xd800)
		|| (temp >= 0xdc00)) { // not a high surrogate
			if (temp == '\r') {
				if (vn->XMLDoc[(offset << 1) + 3] == '\n'
					&& vn->XMLDoc[(offset << 1) + 2] == 0) {  
						return '\n'|(2LL<<32);
				} else {
					return '\n'|(1LL<<32);
				}
			}
			//currentOffset++;
			return temp| (1LL<<32);
	} else {
		val = temp;
		temp =
			vn->XMLDoc[(offset << 1) + 2]
		<< 8 | vn->XMLDoc[(offset << 1) + 3];
		if (temp < 0xdc00 || temp > 0xdfff) {
			throwException(nav_exception,0,
				"navigation exception during getChar",
				"UTF 16 BE encoding error: should never happen");
		}
		val = ((val - 0xd800) <<10) + (temp - 0xdc00) + 0x10000;
		return val | (2LL<<32);
	}
}

static Boolean isDigit(int c);
static Boolean matchSubString(VTDNav*vn, int os,  int index, UCSChar *s);
static Boolean matchSubString2(VTDNav*vn, int os,  int index, UCSChar *s);
//static Boolean matchSubString2(VTDNav*vn,int os, int index, UCSChar *s)
VTDNav *createVTDNav(int r, encoding_t enc, Boolean ns, int depth,
					 UByte *x, int xLen, FastLongBuffer *vtd, FastLongBuffer *l1,
					 FastLongBuffer *l2, FastIntBuffer *l3, int so, int len, Boolean br){
						 VTDNav* vn = NULL;
						 int i;
						 exception e;

						 if (l1 == NULL ||
							 l2 == NULL ||
							 l3 == NULL ||
							 vtd == NULL||
							 x == NULL ||
							 so<0 ||
							 len < 0 ||
							 xLen < 0 || // size of x
							 r < 0 ||
							 depth < 0 ||
							 (enc <FORMAT_ASCII || 
							 enc>FORMAT_UTF_16LE) 
							 )
						 {
							 throwException2(invalid_argument,
								 "Invalid argument when creating VTDGen failed ");
							 return NULL;
						 }

						 vn = (VTDNav *) malloc(sizeof(VTDNav));
						 if (vn==NULL){
							 throwException2(out_of_mem,							 
								 "VTDNav allocation failed ");
							 return NULL;
						 }
						 vn->__freeVTDNav=_freeVTDNav;
						 vn->__cloneNav=_cloneNav;
						 vn->__duplicateNav = _duplicateNav;
						 vn->__verifyNodeCorrectness = _verifyNodeCorrectness;
						 /*vn->__iterate = _iterate;
						 vn->__iterate_following = _iterate_following;
						 vn->__iterate_followingNS = _iterate_followingNS;
						 vn->__iterate_preceding = _iterate_preceding;
						 vn->__iterate_precedingNS = _iterate_precedingNS;
						 vn->__iterateNS = _iterateNS;*/
						 vn->__sync = _sync;
						 vn->__toNode = _toNode;
						 vn->__pop = _pop;
						 vn->__pop2 = _pop2;
						 vn->__push = _push;
						 vn->__push2 = _push2;
						 vn->__recoverNode = _recoverNode;
						 vn->__resolveLC = _resolveLC;
						 vn->__sampleState = _sampleState;
						 vn->__toElement = _toElement;
						 vn->__toElement2 = _toElement2;
						 vn->__toElementNS = _toElementNS;
						 vn->__writeIndex_VTDNav = _writeIndex_VTDNav;
						 vn->__writeIndex2_VTDNav = _writeIndex2_VTDNav;
						 vn->__writeSeparateIndex_VTDNav = _writeSeparateIndex_VTDNav;
						 vn->l1Buffer = l1;
						 vn->l2Buffer = l2;
						 vn->l3Buffer = l3;
						 vn->vtdBuffer= vtd;
						 vn->XMLDoc = x;

						 vn->encoding = enc;
						 vn->rootIndex = r;
						 vn->nestingLevel = depth +1;

						 vn->ns = ns;

						 if (ns == TRUE)
							 vn->offsetMask = MASK_TOKEN_OFFSET1;
						 else 
							 vn->offsetMask = MASK_TOKEN_OFFSET2;



						 vn->atTerminal = FALSE;
						 vn->context = (int *)malloc(vn->nestingLevel*sizeof(int));
						 if (vn->context == NULL){
							 throwException2(out_of_mem,							 
								 "VTDNav allocation failed ");
							 return NULL;
						 }
						 vn->context[0] = 0;
						 for (i=1;i<vn->nestingLevel;i++){
							 vn->context[i] = -1;
						 }
						 //vn->currentOffset = 0;
						 Try{
							 vn->contextBuf = createContextBuffer2(10, vn->nestingLevel+9);
							 vn->contextBuf2 = createContextBuffer2(10,vn->nestingLevel+9);
						 }Catch(e){
							 freeContextBuffer(vn->contextBuf);
							 freeContextBuffer(vn->contextBuf2);
							 //free(vn->stackTemp);
							 free(vn->context);
							 free(vn);
							 throwException2(out_of_mem,							 
								 "VTDNav allocation failed ");
							 return NULL;
						 }

						 vn->stackTemp = (int *)malloc((vn->nestingLevel+9)*sizeof(int));

						 if (vn->contextBuf == NULL 
							 || vn->stackTemp == NULL){
								 freeContextBuffer(vn->contextBuf);
								 freeContextBuffer(vn->contextBuf2);
								 free(vn->stackTemp);
								 free(vn->context);
								 free(vn);
								 throwException2(out_of_mem,							 
									 "VTDNav allocation failed ");
								 return NULL;
						 }
						 vn->count = 0;
						 vn->l1index = vn->l2index = vn->l3index = -1;
						 vn->l2lower = vn->l2upper = -1;
						 vn->l3lower = vn->l3upper = -1;
						 vn->docOffset = so;
						 vn->docLen = len;
						 vn->vtdSize = vtd->size;
						 vn->bufLen = xLen;
						 vn->br = br;
						 vn->LN = 0;
						 vn->shallowDepth = TRUE;
						 vn->maxLCDepthPlusOne = 4;
						 vn->name=NULL;
						 vn->localName=NULL;
						 vn->URIName=NULL;
						 vn->currentNode=NULL;
						 vn->fib = createFastIntBuffer2(5);
						 return vn;
}
static int getNextChar(VTDNav *vn, vn_helper *h);
//Free VTDNav object
//it doesn't free the memory block containing XML doc
void _freeVTDNav(VTDNav *vn)
{	
	if (vn!=NULL){
		freeContextBuffer(vn->contextBuf);
		freeContextBuffer(vn->contextBuf2);
		if (vn->br == FALSE && vn->master){
			freeFastLongBuffer(vn->vtdBuffer);
			freeFastLongBuffer(vn->l1Buffer);
			freeFastLongBuffer(vn->l2Buffer);
			freeFastIntBuffer(vn->l3Buffer);
		}
		//freeFastIntBuffer(vn->fib);
		free(vn->h1);
		vn->h1=NULL;
		free(vn->h2);
		vn->h2=NULL;
		free(vn->context);
		free(vn->stackTemp);
		if (vn->name!= NULL)
			free(vn->name);
		if (vn->localName != NULL)
			free(vn->localName);
		if (vn->URIName != NULL)
			free(vn->URIName);
		if (vn->currentNode != NULL)
			freeBookMark( vn->currentNode);
		//free(vn->XMLDoc);
		freeFastIntBuffer(vn->fib);
		free(vn);
	}						 
}

//Return the attribute count of the element at the cursor position.
int getAttrCount(VTDNav *vn){
	int count = 0;
	int index;
	if(vn->context[0]==-1) return 0;
	index= getCurrentIndex(vn) + 1;
	while (index < vn->vtdSize) {
		int type = getTokenType(vn,index);
		if (type == TOKEN_ATTR_NAME
			|| type == TOKEN_ATTR_VAL
			|| type == TOKEN_ATTR_NS) {
				if (type == TOKEN_ATTR_NAME
					|| (!vn->ns && (type == TOKEN_ATTR_NS))) {
						count++;
				}
		} else
			break;
		index++;
	}
	return count;
}

//Get the token index of the attribute value given an attribute name.     
int getAttrVal(VTDNav *vn, UCSChar *an){
	//int size = vn->vtdBuffer->size;
	int index;
	tokenType type;
	if (vn->context[0] ==-1)
		return -1;

	index = (vn->context[0] != 0) ? vn->context[vn->context[0]] + 1 : vn->rootIndex + 1;


	if(index<vn->vtdSize)
		type = getTokenType(vn,index);
	else 
		return -1;

	if (vn->ns == FALSE) {
		while ((type == TOKEN_ATTR_NAME || type == TOKEN_ATTR_NS)) {
			if (matchRawTokenString3(vn,index,
				an)) { // ns node visible only ns is disabled
					return index + 1;
			}
			index += 2;
			if (index>=vn->vtdSize)
				return -1;
			type = getTokenType(vn,index);
		}
	} else {
		while ((type == TOKEN_ATTR_NAME || type == TOKEN_ATTR_NS)) {
			if (type == TOKEN_ATTR_NAME
				&& matchRawTokenString(vn,
				index,
				an)) { // ns node visible only ns is disabled
					return index + 1;
			}
			index += 2;
			if (index>=vn->vtdSize)
				return -1;
			type = getTokenType(vn,index);
		}
	}
	return -1;
}

//Get the token index of the attribute value of given URL and local name.
//If ns is not enabled, the lookup will return -1, indicating a no-found.
//Also namespace nodes are invisible using this method.
int getAttrValNS(VTDNav *vn, UCSChar* URL, UCSChar *ln){

	int size, index;
	tokenType type;
	if (!vn->ns || vn->context[0]==-1)
		return -1;
	if (URL!=NULL && wcslen(URL)==0)
        	URL = NULL;
	if (URL == NULL)
		return getAttrVal(vn,ln);
	size = vn->vtdBuffer->size;
	index = (vn->context[0] != 0) ? vn->context[vn->context[0]] + 1 : vn->rootIndex + 1;
	// point to the token next to the element tag

	if(index<vn->vtdSize)
		type = getTokenType(vn,index);
	else 
		return -1;

	while ((type == TOKEN_ATTR_NAME || type == TOKEN_ATTR_NS)) {
		int i = getTokenLength(vn, index);
		int offset = getTokenOffset(vn, index);
		int preLen = (i >> 16) & 0xffff;
		int fullLen = i & 0xffff;
		if (preLen != 0
			// attribute name without a prefix is not bound to any namespaces
			&& matchRawTokenString1(vn,
			offset + preLen + 1,
			fullLen - preLen - 1,
			ln)
			&& resolveNS2(vn, URL, offset, preLen)) {
				return index + 1;
		}else if (	preLen==3 
    				&& matchRawTokenString1(vn,offset, 3,L"xml")
    				&& wcscmp(URL,L"http://www.w3.org/XML/1998/namespace")){
    			// prefix matches "xml"
    			return index+1;
    		}
		index += 2;
		if (index >=vn->vtdSize)
			return -1;
		type = getTokenType(vn,index);
	}
	return -1;
}
//This function decodes the underlying byte array into corresponding 
//UCS2 char representation .
//It doesn't resolves built-in entity and character references.
//Length will never be zero
static Long getChar(VTDNav *vn,int offset){
	Long temp = 0;
	switch (vn->encoding) {
			case FORMAT_ASCII : 
			case FORMAT_ISO_8859_1 :			
				temp = vn->XMLDoc[offset];
				if (temp == '\r') {
					if (vn->XMLDoc[offset + 1] == '\n') {
						return '\n'|(2LL<<32);
					} else {
						return '\n'|(1LL<<32);
					}
				}   
				return temp|(1LL<<32);

			case FORMAT_UTF8 :
				temp = vn->XMLDoc[offset];
				if (temp<=127){
					if (temp == '\r') {
						if (vn->XMLDoc[offset + 1] == '\n') {
							return '\n'|(2LL<<32);
						} else {
							return '\n'|(1LL<<32);
						}
					}
					return (temp|(1LL<<32));
				}
				return handle_utf8(vn,temp,offset);

			case FORMAT_UTF_16BE :
				return handle_utf16be(vn, offset);

			case FORMAT_UTF_16LE :
				return handle_utf16le(vn,offset);
				// implement UTF-16LE to UCS4 conversion

				//System.out.println("UTF 16 LE unimplemented for now");

			default :
				return getChar4OtherEncoding(vn,offset);

	}


}


//This method decodes the underlying byte array into corresponding 
//UCS2 char representation .
//Also it resolves built-in entity and character references.
static Long getCharResolved(VTDNav *vn,int offset){
	int ch = 0;
	int val = 0;
	Long inc =2;
	Long l = getChar(vn,offset);
	ch = (int)l;

	if (ch != '&')
		return l;


	// let us handle references here
	//currentOffset++;
	offset++;
	ch = getCharUnit(vn,offset);
	offset++;
	switch (ch) {
			case '#' :

				ch = getCharUnit(vn,offset);

				if (ch == 'x') {
					while (TRUE) {
						offset++;
						inc++;
						ch = getCharUnit(vn,offset);

						if (ch >= '0' && ch <= '9') {
							val = (val << 4) + (ch - '0');
						} else if (ch >= 'a' && ch <= 'f') {
							val = (val << 4) + (ch - 'a' + 10);
						} else if (ch >= 'A' && ch <= 'F') {
							val = (val << 4) + (ch - 'A' + 10);
						} else if (ch == ';') {
							inc++;
							break;
						} else{
							throwException(nav_exception,0,
								"navigation exception during getCharResolved",
								"Illegal char in a char reference");
						}
						//throw new NavException("Illegal char in a char reference");
					}
				} else {
					while (TRUE) {

						ch = getCharUnit(vn,offset);
						offset++;
						inc++;
						if (ch >= '0' && ch <= '9') {
							val = val * 10 + (ch - '0');
						} else if (ch == ';') {
							break;
						} else{
							throwException(nav_exception,0,
								"navigation exception during getCharResolved",
								"Illegal char in a char reference");
						}
					}
				}
				break;

			case 'a' :
				ch = getCharUnit(vn, offset);
				if (ch == 'm') {
					if (getCharUnit(vn, offset + 1) == 'p'
						&& getCharUnit(vn, offset + 2) == ';') {
							inc = 5;
							val = '&';
					} else{
						throwException(nav_exception,0,
							"navigation exception during getCharResolved",
							"illegal builtin reference");
					}
					//	throw new NavException("illegal builtin reference");
				} else if (ch == 'p') {
					if (getCharUnit(vn,offset + 1) == 'o'
						&& getCharUnit(vn,offset + 2) == 's'
						&& getCharUnit(vn,offset + 3) == ';') {
							inc = 6;
							val = '\'';
					} else{
						throwException(nav_exception,0,
							"navigation exception during getCharResolved",
							"illegal builtin reference");
					}
					//throw new NavException("illegal builtin reference");
				} else{
					throwException(nav_exception,0,
						"navigation exception during getCharResolved",
						"illegal builtin reference");
				}
				//	throw new NavException("illegal builtin reference");
				break;

			case 'q' :

				if (getCharUnit(vn,offset) == 'u'
					&& getCharUnit(vn,offset + 1) == 'o'
					&& getCharUnit(vn,offset + 2) == 't'
					&& getCharUnit(vn,offset + 3) == ';') {
						inc = 6;
						val = '\"';
				} else{
					throwException(nav_exception,0,
						"navigation exception during getCharResolved",
						"illegal builtin reference");
				}
				//throw new NavException("illegal builtin reference");
				break;
			case 'l' :
				if (getCharUnit(vn,offset) == 't'
					&& getCharUnit(vn,offset + 1) == ';') {
						inc = 4;
						val = '<';
				} else{
					throwException(nav_exception,0,
						"navigation exception during getCharResolved",
						"illegal builtin reference");
				}
				//throw new NavException("illegal builtin reference");
				break;
			case 'g' :
				if (getCharUnit(vn,offset) == 't'
					&& getCharUnit(vn,offset + 1) == ';') {
						inc = 4;
						val = '>';
				} else{
					throwException(nav_exception,0,
						"navigation exception during getCharResolved",
						"illegal builtin reference");
				}
				break;

			default :

				throwException(nav_exception,0,
					"navigation exception during getCharResolved",
					"Invalid entity char");
	}
	return val | (inc << 32);
}



/*Get the next char unit which gets decoded automatically*/
static int getCharUnit(VTDNav *vn, int offset){
	return (vn->encoding <=FORMAT_UTF8)
		? vn->XMLDoc[offset]:
	(vn->encoding <= FORMAT_WIN_1258)
		? decode(vn,offset)
		: ((vn->encoding == FORMAT_UTF_16BE)
		? (vn->XMLDoc[offset << 1]
	<< 8 | vn->XMLDoc[(offset << 1) + 1])
		: (vn->XMLDoc[(offset << 1) + 1]
	<< 8 | vn->XMLDoc[offset << 1]));
}

// Get the starting offset and length of an element fragment
// encoded in a long, upper 32 bit is length; lower 32 bit is offset
Long getElementFragment(VTDNav *vn){
	// a little scanning is needed
	// has next sibling case
	// if not
	int depth = getCurrentDepth(vn);
	int so; 
	int length = 0;
	int temp;
	int size, so2, d, i;

	if (depth == -1)
		return ((Long)vn->docLen)<<32 | vn->docOffset;

	if (depth == -1){
		    int i=lower32At(vn->vtdBuffer,0);
		    if (i==0)
		        return ((Long)vn->docLen)<<32| vn->docOffset;
		    else
		        return ((Long)(vn->docLen-32))| 32;
	}
	// for an element with next sibling
	so = getTokenOffset(vn,getCurrentIndex2(vn)) - 1;
	if (toElement(vn,NEXT_SIBLING)) {

		int temp = getCurrentIndex(vn);
		int so2;
		// rewind 
		while (getTokenDepth(vn,temp) == depth && 
					(getTokenType(vn,temp)== TOKEN_COMMENT || 
							getTokenType(vn,temp)==TOKEN_PI_VAL ||
							getTokenType(vn,temp)==TOKEN_PI_NAME)) {
				
				temp--;
			}
		/*if (temp!=temp2)
			temp++;*/
		//temp++;
		so2 = getTokenOffset(vn,temp) - 1;
		// look for the first '>'
		while (getCharUnit(vn,so2) != '>') {
			so2--;
		}
		length = so2 - so + 1;
		toElement(vn, PREV_SIBLING);
		if (vn->encoding <= FORMAT_WIN_1258)
			return ((Long) length) << 32 | so;
		else
			return ((Long) length) << 33 | (so << 1);
	}

	// for root element
	if (depth == 0) {
		int temp = vn->vtdBuffer->size - 1;
		Boolean b = FALSE;
		int so2 = 0;
		while (getTokenDepth(vn,temp) == -1) {
			temp--; // backward scan
			b = TRUE;
		}
		if (b == FALSE)
			so2 =
			(vn->encoding <= FORMAT_WIN_1258)
			? (vn->docOffset + vn->docLen - 1)
			: ((vn->docOffset + vn->docLen) >> 1) - 1;
		else
			so2 = getTokenOffset(vn,temp + 1);
		while (getCharUnit(vn,so2) != '>') {
			so2--;
		}
		length = so2 - so + 1;
		if (vn->encoding <= FORMAT_WIN_1258)
			return ((Long) length) << 32 | so;
		else
			return ((Long) length) << 33 | (so << 1);
	}
	// for a non-root element with no next sibling
	temp = getCurrentIndex(vn) + 1;
	size = vn->vtdBuffer->size;
	// temp is not the last entry in VTD buffer
	if (temp < size - 1) {
		while (temp < size && getTokenDepth(vn,temp) >= depth) {
			temp++;
		}
		if (temp != size) {
			int d =
				depth
				- getTokenDepth(vn,temp)
				+ ((getTokenType(vn, temp) == TOKEN_STARTING_TAG) ? 1 : 0);
			int so2 = getTokenOffset(vn,temp) - 1;
			int i = 0;
			// scan backward
			while (i < d) {
				if (getCharUnit(vn,so2) == '>')
					i++;
				so2--;
			}
			length = so2 - so + 2;
			if (vn->encoding <= FORMAT_WIN_1258)
				return ((Long) length) << 32 | so;
			else
				return ((Long) length) << 33 | (so << 1);
		}
	}
	// temp is the last entry
	// scan forward search for /> or </cc>
	so2 =
		(vn->encoding <= FORMAT_WIN_1258)
		? (vn->docOffset + vn->docLen - 1)
		: ((vn->docOffset + vn->docLen) >> 1) - 1;
	d = depth + 1;
	i = 0;
	while (i < d) {
		if (getCharUnit(vn,so2) == '>') {
			i++;
		}
		so2--;
	}

	length = so2 - so + 2;

	if (vn->encoding <= FORMAT_WIN_1258)
		return ((Long) length) << 32 | so;
	else
		return ((Long) length) << 33 | (so << 1);
}


// Get the starting offset and length of an element content 
// ie. everything warpped around the starting and ending tags of the 
// cursor element, return -1 if the cursor element is an empty element
// 
Long getContentFragment(VTDNav *vn){
	int so,length,so2,temp,size,d,i;
	Long ll;
		int depth = getCurrentDepth(vn);
//		 document length and offset returned if depth == -1
		if (depth == -1){
		    int i=lower32At(vn->vtdBuffer,0);
		    if (i==0)
		        return ((Long)vn->docLen)<<32| vn->docOffset;
		    else
		        return ((Long)(vn->docLen-32))| 32;
		}

		ll= getOffsetAfterHead(vn);
		//so = getOffsetAfterHead(vn);
		if (ll<-1)
			return -1L;
		so =  (int)ll;
		length = 0;
		

		// for an element with next sibling
		if (toElement(vn,NEXT_SIBLING)) {

			int temp = getCurrentIndex(vn);
			// rewind
			while (getTokenDepth(vn,temp) == depth && 
					(getTokenType(vn,temp)== TOKEN_COMMENT || 
							getTokenType(vn,temp)==TOKEN_PI_VAL ||
							getTokenType(vn,temp)==TOKEN_PI_NAME)) {
				
				temp--;
			}
			/*if (temp!=temp2)
				temp++;*/
			//temp++;
			so2 = getTokenOffset(vn,temp) - 1;
			// look for the first '>'
			while (getCharUnit(vn,so2) != '>') {
				so2--;
			}
			while (getCharUnit(vn,so2) != '/') {
				so2--;
			}
			while (getCharUnit(vn,so2) != '<') {
				so2--;
			}
			length = so2 - so;
			toElement(vn,PREV_SIBLING);
			if (vn->encoding <= FORMAT_WIN_1258)
				return ((Long) length) << 32 | so;
			else
				return ((Long) length) << 33 | (so << 1);
		}

		// for root element
		if (depth == 0) {
			int temp = vn->vtdBuffer->size - 1;
			Boolean b = FALSE;
			int so2 = 0;
			while (getTokenDepth(vn,temp) == -1) {
				temp--; // backward scan
				b = TRUE;
			}
			if (b == FALSE)
				so2 =
					(vn->encoding <= FORMAT_WIN_1258 )
						? (vn->docOffset + vn->docLen - 1)
						: ((vn->docOffset + vn->docLen) >> 1) - 1;
			else
				so2 = getTokenOffset(vn,temp + 1);
			while (getCharUnit(vn,so2) != '>') {
				so2--;
			}
			while (getCharUnit(vn,so2) != '/') {
				so2--;
			}
			while (getCharUnit(vn,so2) != '<') {
				so2--;
			}
			length = so2 - so;
			if (vn->encoding <= FORMAT_WIN_1258)
				return ((Long) length) << 32 | so;
			else
				return ((Long) length) << 33 | (so << 1);
		}
		// for a non-root element with no next sibling
		temp = getCurrentIndex(vn) + 1;
		size = vn->vtdBuffer->size;
		// temp is not the last entry in VTD buffer
		if (temp < size) {
			while (temp < size && getTokenDepth(vn,temp) >= depth) {
				temp++;
			}
			if (temp != size) {
				int d =
					depth
						- getTokenDepth(vn,temp)
						+ ((getTokenType(vn,temp) == TOKEN_STARTING_TAG) ? 1 : 0);
				int so2 = getTokenOffset(vn,temp) - 1;
				int i = 0;
				// scan backward
				while (i < d) {
					if (getCharUnit(vn,so2) == '>')
						i++;
					so2--;
				}
				while (getCharUnit(vn,so2) != '/') {
					so2--;
				}
				while (getCharUnit(vn,so2) != '<') {
					so2--;
				}
				length = so2 - so;
				if (vn->encoding <= FORMAT_WIN_1258)
					return ((Long) length) << 32 | so;
				else
					return ((Long) length) << 33 | (so << 1);
			}
			/*
             * int so2 = getTokenOffset(temp - 1) - 1; int d = depth -
             * getTokenDepth(temp - 1); int i = 0; while (i < d) { if
             * (getCharUnit(so2) == '>') { i++; } so2--; } length = so2 - so +
             * 2; if (encoding < 3) return ((long) length) < < 32 | so; else
             * return ((long) length) < < 33 | (so < < 1);
             */
		}
		// temp is the last entry
		// scan forward search for /> or </cc>
		
		so2 =
			(vn->encoding <= FORMAT_WIN_1258)
				? (vn->docOffset + vn->docLen - 1)
				: ((vn->docOffset + vn->docLen) >> 1) - 1;
			   
	    d = depth + 1;
	    
	    i = 0;
        while (i < d) {
            if (getCharUnit(vn,so2) == '>') {
                i++;
            }
            so2--;
        }
        while (getCharUnit(vn,so2) != '/') {
			so2--;
		}
		while (getCharUnit(vn,so2) != '<') {
			so2--;
		}

		length = so2 - so;

		if (vn->encoding <= FORMAT_WIN_1258)
			return ((Long) length) << 32 | so;
		else
			return ((Long) length) << 33 | (so << 1);
}
// This function returns of the token index of the type character data or CDATA.
// Notice that it is intended to support data orient XML (not mixed-content XML).
int getText(VTDNav *vn){
	if (vn->context[0]==-1)
		return -1;
	else {
		int index = (vn->context[0] != 0) ? 
			vn->context[vn->context[0]] + 1 : vn->rootIndex + 1;
		int depth = getCurrentDepth(vn);
		tokenType type;
		if (index<vn->vtdSize && !vn->atTerminal)
			type= getTokenType(vn,index);
		else
			return -1;

		while (TRUE) {
			if (type == TOKEN_CHARACTER_DATA || type == TOKEN_CDATA_VAL) {
				if (depth == getTokenDepth(vn,index))
					return index;
				else
					return -1;
			} else if (type == TOKEN_ATTR_NS || type == TOKEN_ATTR_NAME) {
				index += 2; // assuming a single token for attr val
			} else if (
				type == TOKEN_PI_NAME
				|| type == TOKEN_PI_VAL
				|| type == TOKEN_COMMENT) {
					if (depth == getTokenDepth(vn,index)) {
						index += 1;
					} else
						return -1;
			} else
				return -1;
			if (index >= vn->vtdSize) 
				break;
			type = getTokenType(vn, index);
		}
		return -1;
	}
}

//Get the depth value of a token (>=0)
/*int getTokenDepth(VTDNav *vn, int index){
	int i;
	i = (int) ((longAt(vn->vtdBuffer,index) & MASK_TOKEN_DEPTH) >> 52);

	if (i != 255)
		return i;
	return -1;
}*/

//Get the token length at the given index value
//please refer to VTD spec for more details
int getTokenLength(VTDNav *vn, int index){
	Long i = 0;
	//int j=0;
	int depth;
	int len = 0;
	int type = getTokenType(vn,index), temp=0;
	//int val;
	switch (type) {
			case TOKEN_ATTR_NAME :
			case TOKEN_ATTR_NS :
			case TOKEN_STARTING_TAG :
				i = longAt(vn->vtdBuffer, index);

				return (vn->ns == FALSE)
					? (int) ((i & MASK_TOKEN_QN_LEN) >> 32)
					: ((int) ((i & MASK_TOKEN_QN_LEN)
					>> 32)
					| ((int) ((i & MASK_TOKEN_PRE_LEN)
					>> 32)
					<< 5));

				break;
			case TOKEN_CHARACTER_DATA:
			case TOKEN_CDATA_VAL:
			case TOKEN_COMMENT: // make sure this is total length
				depth = getTokenDepth(vn,index);
				do{

					len = len +  (int)
						((longAt(vn->vtdBuffer, index) 
						& MASK_TOKEN_FULL_LEN) >> 32);
					temp =  getTokenOffset(vn, index)+(int)
						((longAt(vn->vtdBuffer,index)& MASK_TOKEN_FULL_LEN) >> 32);
					index++;						
				}
				while(index < vn->vtdSize && depth == getTokenDepth(vn,index) 
					&& type == getTokenType(vn,index) && temp == getTokenOffset(vn,index));
				//if (int k=0)
				return len;
			default :

				return (int)
					((longAt(vn->vtdBuffer,index) & MASK_TOKEN_FULL_LEN) >> 32);

				break;
	}
}



//Test whether current element has an attribute with the matching name.
Boolean hasAttr(VTDNav *vn, UCSChar *an){
	tokenType type;

	int size = vn->vtdBuffer->size;
	int index = (vn->context[0] != 0) ? vn->context[vn->context[0]] + 1 : vn->rootIndex + 1;

	if (vn->context[0]==-1) 
		return FALSE;

	if (index >= size)
		return FALSE;

	type = getTokenType(vn,index);
	if (vn->ns == FALSE) {
		if (wcscmp(an,L"*")==0) {
			if (type == TOKEN_ATTR_NAME || type == TOKEN_ATTR_NS)
				return TRUE;
			else
				return FALSE;
		} else {
			while (index < size
				&& (type == TOKEN_ATTR_NAME
				|| type == TOKEN_ATTR_NS)) { // ns tokens becomes visible
					if (matchRawTokenString(vn,index, an))
						return TRUE;
					index += 2;
					type = getTokenType(vn,index);
			}
			return FALSE;
		}
	} else {
		if (wcscmp(an,L"*")==0) {
			while (index < size
				&& (getTokenType(vn,index) == TOKEN_ATTR_NAME
				|| getTokenType(vn,index) == TOKEN_ATTR_NS)) {
					if (type == TOKEN_ATTR_NAME)
						// TOKEN_ATTR_NS is invisible when ns == TRUE
						return TRUE;
					index += 2;
					type = getTokenType(vn,index);
			}
			return FALSE;

		} else {
			while (index < size
				&& (type == TOKEN_ATTR_NAME || type == TOKEN_ATTR_NS)) {
					if (type == TOKEN_ATTR_NAME
						&& matchRawTokenString(vn,index, an))
						return TRUE;
					index += 2;
					type = getTokenType(vn,index);
			}
			return FALSE;
		}
	}
}

//Test whether the current element has an attribute with 
//matching namespace URL and localname.
Boolean hasAttrNS(VTDNav *vn, UCSChar *URL, UCSChar *localName){
	if (vn->context[0]==-1)
		return FALSE;
	return (getAttrValNS(vn,URL, localName) != -1);
}

//Test the token type, to see if it is a starting tag.
static inline Boolean isElement(VTDNav  *vn, int index){
	return (((longAt(vn->vtdBuffer,index) & MASK_TOKEN_TYPE) >> 60) & 0xf)
		== TOKEN_STARTING_TAG;
}

// Test the token type, to see if it is a starting tag or document token
static inline Boolean isElementOrDocument(VTDNav  *vn, int index){
	int i = 0;
 	i= (int)(((longAt(vn->vtdBuffer,index) & MASK_TOKEN_TYPE) >> 60) & 0xf);
	return (i == TOKEN_STARTING_TAG || i == TOKEN_DOCUMENT); 
}

//Test whether ch is a white space character or not.
static inline Boolean isWS(int ch){
	return (ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r');
}
//This method is similar to getElementByName in DOM except it doesn't
//return the nodeset, instead it iterates over those nodes.
int iterate(VTDNav *vn, int dp, UCSChar *en,Boolean special){

	int index = getCurrentIndex(vn) + 1;
	tokenType tt;
	//int size = vtdBuffer->size();
	while (index < vn->vtdSize) {
		tt = getTokenType(vn,index);
		if (tt == TOKEN_ATTR_NAME ||
			tt == TOKEN_ATTR_NS|| 
			tt ==TOKEN_PI_NAME ){
				index +=2;
				continue;
		}

		if (tt == TOKEN_STARTING_TAG 
			|| tt == TOKEN_DOCUMENT) {
			int depth = getTokenDepth(vn,index);
			if (depth > dp) {
				vn->context[0] = depth;
				if (depth > 0)
					vn->context[depth] = index;

				if (special || matchElement(vn, en)) {
					if (dp < vn->maxLCDepthPlusOne)
						resolveLC(vn);
					return TRUE;
				}
			} else {
				return FALSE;
			}
		}
		index++;
	}
	return FALSE;
}

//This method is similar to getElementByName in DOM except it doesn't
//return the nodeset, instead it iterates over those nodes .
//When URL is "*" it will match any namespace
//if ns is FALSE, return FALSE immediately
int iterateNS(VTDNav *vn, int dp, UCSChar *URL, UCSChar *ln){
	int index;
	tokenType tt;
	if (vn->ns == FALSE)
		return FALSE;

	index = getCurrentIndex(vn) + 1;
	while (index < vn->vtdSize) {
		tt = getTokenType(vn,index);
		if (tt == TOKEN_ATTR_NAME ||
			tt == TOKEN_ATTR_NS || 
			tt == TOKEN_PI_NAME){
				index +=2;
				continue;
		}
		if (tt == TOKEN_STARTING_TAG || tt == TOKEN_DOCUMENT) {
			int depth = getTokenDepth(vn,index);
			if (depth > dp) {
				vn->context[0] = depth;
				if (depth>0)
					vn->context[depth] = index;
				if (matchElementNS(vn,URL, ln)) {
					if (dp < vn->maxLCDepthPlusOne)
						resolveLC(vn);
					return TRUE;
				}
			} else {
				return FALSE;
			}
		}
		index++;

	}
	return FALSE;
}


// This function is called by selectElement_P in autoPilot
Boolean iterate_preceding(VTDNav *vn,UCSChar *en, int* a, int endIndex){
		int index = getCurrentIndex(vn) +1;
		tokenType tokenType;
		int depth;
		//int depth = getTokenDepth(index);
		//int size = vtdBuffer.size;
		while (index< endIndex) {
			tokenType = getTokenType(vn,index);
			switch(tokenType){
			case TOKEN_ATTR_NAME:
			case TOKEN_ATTR_NS:
				index = index + 2;
				continue;
			case TOKEN_STARTING_TAG:
			//case TOKEN_DOCUMENT:
				depth = getTokenDepth(vn,index);
				if (index!=a[depth]){
					if (wcscmp(en,L"*")==0||matchRawTokenString(vn,index,en)){
						vn->context[0] = depth;
						if (depth > 0)
							vn->context[depth] = index;
						if (depth < vn->maxLCDepthPlusOne)
							resolveLC(vn);
						vn->atTerminal = FALSE;
						return TRUE;
					}else{
						vn->context[depth] = index;
						index++;
						continue;
					}
				}else{
					vn->context[depth] = index;
					index++;
					continue;
				}
			case TOKEN_CHARACTER_DATA:
			case TOKEN_CDATA_VAL:
			case TOKEN_COMMENT: 
					index++; 
					continue;
			case TOKEN_PI_NAME:
				index+=2;
				continue;
			}
			index++;
		}
		return FALSE;	
}

// This function is called by selectElementNS_P in autoPilot
Boolean iterate_precedingNS(VTDNav *vn,UCSChar *URL, UCSChar *ln, int* a,int endIndex){
		int index = getCurrentIndex(vn) - 1;
		tokenType tokenType;
		int depth;
		//int depth = getTokenDepth(index);
		//int size = vtdBuffer.size;
		while (index< endIndex) {
			tokenType = getTokenType(vn,index);
			switch(tokenType){
			case TOKEN_ATTR_NAME:
			case TOKEN_ATTR_NS:
				index = index + 2;
				continue;
			case TOKEN_STARTING_TAG:
			//case TOKEN_DOCUMENT:
				depth = getTokenDepth(vn,index);
				if (index!=a[depth]){
					vn->context[0] = depth;
					if (depth > 0)
						vn->context[depth] = index;
					if (matchElementNS(vn,ln,URL)){						
						if (depth < vn->maxLCDepthPlusOne)
							resolveLC(vn);
						vn->atTerminal = FALSE;
						return TRUE;
					}else{
						vn->context[depth] = index;
						index++;
						continue;
					}
				}else{
					vn->context[depth] = index;
					index++;
					continue;
				}
			case TOKEN_CHARACTER_DATA:
			case TOKEN_CDATA_VAL:
			case TOKEN_COMMENT: 
					index++; 
					continue;
			case TOKEN_PI_NAME:
				index+=2;
				continue;
			}
			index++;
		}
		return FALSE;
}


// This function is called by selectElement_F in autoPilot

Boolean iterate_following(VTDNav *vn,UCSChar *en, Boolean special){
	int index = getCurrentIndex(vn) + 1;
		tokenType tokenType;
		//int size = vtdBuffer.size;
		while (index < vn->vtdSize) {
			tokenType = getTokenType(vn,index);
			if (tokenType == TOKEN_ATTR_NAME
					|| tokenType == TOKEN_ATTR_NS
					|| tokenType == TOKEN_PI_NAME) {
				index = index + 2;
				continue;
			}
			// if (isElementOrDocument(index)) {
			if (tokenType == TOKEN_STARTING_TAG
					|| tokenType == TOKEN_DOCUMENT) {
				int depth = getTokenDepth(vn,index);
				vn->context[0] = depth;
				if (depth > 0)
					vn->context[depth] = index;
				if (special || matchElement(vn,en)) {
					if (depth < vn->maxLCDepthPlusOne)
						resolveLC(vn);
					return TRUE;
				}
			}
			index++;
		}
		return FALSE;	
}


// This function is called by selectElementNS_F in autoPilot

Boolean iterate_followingNS(VTDNav *vn, UCSChar *URL, UCSChar *ln){
	int index = getCurrentIndex(vn) + 1;
	tokenType tokenType;
	//int size = vtdBuffer.size;
	while (index < vn->vtdSize) {
		tokenType = getTokenType(vn,index);
		if (tokenType == TOKEN_ATTR_NAME
			|| tokenType == TOKEN_ATTR_NS
			|| tokenType == TOKEN_PI_NAME) {
				index = index + 2;
				continue;
		}
		// if (isElementOrDocument(index)) {
		if (tokenType == TOKEN_STARTING_TAG
			|| tokenType == TOKEN_DOCUMENT) {
				int depth = getTokenDepth(vn,index);
				vn->context[0] = depth;
				if (depth > 0)
					vn->context[depth] = index;
				if (matchElementNS(vn,URL, ln)) {
					if (depth < vn->maxLCDepthPlusOne)
						resolveLC(vn);
					return TRUE;
				}
		}
		index++;
	}
	return FALSE;
}



//Test if the current element matches the given name.
Boolean matchElement(VTDNav *vn, UCSChar *en){
	/*if (en == NULL){
	throwException2(invalid_argument,
	"matchElement's element name can't be null");
	}*/

	// throw new IllegalArgumentException(" Element name can't be null ");
	if (wcscmp(en,L"*") == 0 && vn->context[0] !=-1)
		return TRUE;
	if (vn->context[0]==-1)
		return FALSE;
	return matchRawTokenString3(vn,
		(vn->context[0] == 0) ? vn->rootIndex : vn->context[vn->context[0]],
		en);
}

//Test whether the current element matches the given namespace URL and localname.
//URL, when set to "*", matches any namespace (including null), when set to null, defines a "always-no-match"
//ln is the localname that, when set to *, matches any localname
Boolean matchElementNS(VTDNav *vn, UCSChar *URL, UCSChar *ln){
	if (vn->context[0]==-1)
		return FALSE;
	else {
		int i =
			getTokenLength(vn, (vn->context[0] != 0) ? vn->context[vn->context[0]] : vn->rootIndex);
		int offset =
			getTokenOffset(vn, (vn->context[0] != 0) ? vn->context[vn->context[0]] : vn->rootIndex);
		int preLen = (i >> 16) & 0xffff;
		int fullLen = i & 0xffff;
		if (URL!=NULL && wcslen(URL)==0)
        	URL = NULL;
		if (wcscmp(ln, L"*")== 0
			|| ((preLen != 0)
			? matchRawTokenString1(vn,
			offset + preLen + 1,
			fullLen - preLen - 1,
			ln)
			: matchRawTokenString1(vn,
			offset,
			fullLen,
			ln))) { // no prefix, search for xmlns
				if (((URL != NULL) ? wcscmp(URL,L"*")==0 : FALSE)
					|| (resolveNS2(vn, URL, offset, preLen) == TRUE))
					return TRUE;

				if ( preLen==3 
    			&& matchRawTokenString1(vn,offset, preLen,L"xml")
    			&& wcscmp(URL,L"http://www.w3.org/XML/1998/namespace"))
    			return TRUE;   
		}
		return FALSE;
	}
}

//Match a string against a token with given offset and len, entities 
//doesn't get resolved.
Boolean matchRawTokenString1(VTDNav *vn, int offset, int len, UCSChar *s){
  return (Boolean) (compareRawTokenString2(vn,offset,len,s)==0);
}

//Match a string with a token represented by a long (upper 32 len, lower 32 offset).
static Boolean matchRawTokenString2(VTDNav *vn, Long l, UCSChar *s){
	int len;
	int offset;
	/*if (s == NULL){
	throwException2(invalid_argument,
	" invalid argument for matchRawTokenString2, s can't be NULL");
	}*/
	//throw new IllegalArgumentException("string can't be null");
	len = (int) ((l & MASK_TOKEN_FULL_LEN) >> 32);
	// a little hardcode is always bad
	offset = (int) l;
	return (compareRawTokenString2(vn, offset, len, s)==0); 
}

Boolean matchRawTokenString3(VTDNav *vn, int index, UCSChar *s) {
	int type = getTokenType(vn,index);
	int len =
		(type == TOKEN_STARTING_TAG
			|| type == TOKEN_ATTR_NAME
			|| type == TOKEN_ATTR_NS)
		? getTokenLength(vn,index) & 0xffff
		: getTokenLength(vn,index);

	int len2 = (int)((longAt(vn->vtdBuffer,index)& MASK_TOKEN_FULL_LEN) >> 43);
	int os2 = (len2 == 0) ? 0 : len2 + 1;
	// upper 16 bit is zero or for prefix

	//currentOffset = getTokenOffset(index);
	// point currentOffset to the beginning of the token
	// for UTF 8 and ISO, the performance is a little better by avoid
	// calling getChar() everytime
	return compareRawTokenString2(vn,getTokenOffset(vn,index) + os2, len - os2, s) == 0;
}

//Match the string against the token at the given index value. When a token
//is an attribute name or starting tag, qualified name is what gets matched against
Boolean matchRawTokenString(VTDNav *vn, int index, UCSChar *s){	
	tokenType type;
	int len;
	int offset;
	/*if (s == NULL){
	//	 throwException2(invalid_argument,
	//		 " invalid argument for matchRawTokenString, s can't be NULL");
	}*/
	type = getTokenType(vn,index);
	len =
		(type == TOKEN_STARTING_TAG
		|| type == TOKEN_ATTR_NAME
		|| type == TOKEN_ATTR_NS)
		? getTokenLength(vn, index) & 0xffff
		: getTokenLength(vn, index);
	// upper 16 bit is zero or for prefix

	offset = getTokenOffset(vn, index);
	// point currentOffset to the beginning of the token
	// for UTF 8 and ISO, the performance is a little better by avoid calling getChar() everytime
	return compareRawTokenString2(vn, offset, len, s)==0;
}

//Match a string against a token with given offset and len, entities get 
//resolved properly.
static Boolean matchTokenString1(VTDNav *vn, int offset, int len, UCSChar *s){
	int endOffset;
	int l;
	Long l1;
	if (s == NULL){ 
		throwException2(invalid_argument,
			" invalid argument for matchRawTokenString1, s can't be NULL");
	}
	//throw new IllegalArgumentException("string can't be null");

	//vn->currentOffset = offset;
	endOffset = offset + len;

	if (vn->encoding < FORMAT_UTF8) {
		int i = 0;
		l = (int)wcslen(s);
		for (i = 0; i < l && offset < endOffset; i++) {
			if ((vn->XMLDoc[offset] & 0xff) != '&') {
				if (s[i] != (vn->XMLDoc[offset] & 0xff))
					return FALSE;
				offset++;
			} else {
				l1 = getCharResolved(vn,offset);
				if (s[i] != (int)l1) {
					return FALSE;
				}
				offset += (int)(l1>>32);
			}
		}
		if (i == l && offset == endOffset)
			return TRUE;
		else
			return FALSE;
	} else {
		int i = 0;
		l = (int)wcslen(s);
		for (i = 0; i < l && offset < endOffset; i++) {
			l1 = getCharResolved(vn,offset);
			offset += (int)(l1>>32);
			if (s[i] != (int)l1) {
				return FALSE;
			}
		}
		if (i == l && offset == endOffset)
			return TRUE;
		else
			return FALSE;
	} //return TRUE;
}

//Match a string against a "non-extractive" token represented by a 
//long (upper 32 len, lower 32 offset).
static Boolean matchTokenString2(VTDNav *vn, Long l, UCSChar *s){
	int len,offset;
	if (s == NULL){
		throwException2(invalid_argument,
			" invalid argument for matchTokenString2, s can't be NULL");
	}
	//	 throw new IllegalArgumentException("string can't be null");

	len = (int) ((l & MASK_TOKEN_FULL_LEN) >> 32);
						 // a little hardcode is always bad
	offset = (int) l;
	return matchRawTokenString1(vn,offset, len, s);
}


Boolean matchTokens(VTDNav *vn1, int i1, VTDNav *vn2, int i2){
	return compareTokens(vn1,i1,vn2,i2)==0;
}
//Match the string against the token at the given index value. When a token
//is an attribute name or starting tag, qualified name is what gets matched against
Boolean matchTokenString(VTDNav *vn, int index, UCSChar *s){
	tokenType type;
	int offset;
	int len;
	if (s == NULL){
		throwException2(invalid_argument,
			" invalid argument for matchTokenString, s can't be NULL");
	}
	type = getTokenType(vn,index);
	len =
		(type == TOKEN_STARTING_TAG
		|| type == TOKEN_ATTR_NAME
		|| type == TOKEN_ATTR_NS)
		? getTokenLength(vn,index) & 0xffff
		: getTokenLength(vn,index);
	// upper 16 bit is zero or for prefix

	offset = getTokenOffset(vn,index);
	// point currentOffset to the beginning of the token
	// for UTF 8 and ISO, the performance is a little better by avoid calling getChar() everytime
	return matchTokenString1(vn,offset, len, s);
}

//Evaluate the namespace indicator in bit 31 and bit 30.
static inline int NSval(VTDNav *vn, int i){
	return (int) (longAt(vn->vtdBuffer,i) & MASK_TOKEN_NS_MARK);
}

//Convert a vtd token into a double.
double parseDouble(VTDNav *vn, int index){
	int ch;
	int end;
	Long l;
	int t = getTokenType(vn,index);
	Boolean b = (t==TOKEN_CHARACTER_DATA) || (t== TOKEN_ATTR_VAL);
	Long left, right, scale, exp;
	double v;
	Boolean neg;
	int offset = getTokenOffset(vn,index);
	end = offset + getTokenLength(vn,index);
	//past the last one by one

	//ch = b?getCharResolved(vn):getChar(vn);
	{
		l = b? getCharResolved(vn,offset):getChar(vn,offset);
		ch = (int)l;
		offset += (int)(l>>32);
	}
	while (offset <= end) { /* trim leading whitespaces*/
		if (!isWS(ch))
			break;
		l = b? getCharResolved(vn,offset):getChar(vn,offset);
		ch = (int)l;
		offset += (int)(l>>32);
	}

	if (offset > end) {/* all whitespace */
		double d1 = 0.0;
		return d1/d1;
	}

	neg = (ch == '-');

	if (ch == '-' || ch == '+')
	{
		l = b? getCharResolved(vn,offset):getChar(vn,offset);
		ch = (int)l;
		offset += (int)(l>>32);
	}

	/*left part of decimal*/
	left = 0;
	while (offset <= end) {
		/*must be <= since we get the next one at last.*/

		int dig = Character_digit((char) ch, 10); /*only consider decimal*/
		if (dig < 0)
			break;

		left = left * 10 + dig;

		{
			l = b? getCharResolved(vn,offset):getChar(vn,offset);
			ch = (int)l;
			offset += (int)(l>>32);
		}
	}

	/*right part of decimal*/
	right = 0;
	scale = 1;
	if (ch == '.') {
		{
			l = b? getCharResolved(vn,offset):getChar(vn,offset);
			ch = (int)l;
			offset += (int)(l>>32);
		}

		while (offset <= end) {
			/*must be <= since we get the next one at last.*/

			int dig = Character_digit((char) ch, 10);
			/*only consider decimal */
			if (dig < 0)
				break;

			right = right * 10 + dig;
			scale *= 10;

			{
				l = b? getCharResolved(vn,offset):getChar(vn,offset);
				ch = (int)l;
				offset += (int)(l>>32);
			}
		}
	}

	/*exponent*/
	exp = 0;
	if (ch == 'E' || ch == 'e') {
		Boolean expneg;
		int cur;
		{
			l = b? getCharResolved(vn,offset):getChar(vn,offset);
			ch = (int)l;
			offset += (int)(l>>32);
		}
		expneg = (ch == '-'); /*sign for exp*/
		if (ch == '+' || ch == '-')
		{
			l = b? getCharResolved(vn,offset):getChar(vn,offset);
			ch = (int)l;
			offset += (int)(l>>32);
		} /*skip the +/- sign*/

		cur = offset;
		/*remember the indx, used to find a invalid number like 1.23E*/

		while (offset <= end) {
			/*must be <= since we get the next one at last.*/

			int dig = Character_digit((char) ch, 10);
			/*only consider decimal*/
			if (dig < 0)
				break;
			exp = exp * 10 + dig;
			{
				l = b? getCharResolved(vn,offset):getChar(vn,offset);
				ch = (int)l;
				offset += (int)(l>>32);
			}
		}
		if (cur == offset){// all whitespace
			double d1 = 0.0;
			return d1/d1;
			//throw new NavException("Empty string");
		}
		if (expneg)
			exp = (-exp);
	}

	//anything left must be space
	while (offset <= end) {
		if (!isWS(ch)){// all whitespace
			double d1 = 0.0;
			return d1/d1;
		}
		// throw new NavException(toString(index));

		{
			l = b? getCharResolved(vn,offset):getChar(vn,offset);
			ch = (int)l;
			offset += (int)(l>>32);
		}
	}

	v = (double) left;
	if (right != 0)
		v += ((double) right) / (double) scale;

	if (exp != 0)
		v = v* pow(10,(double) exp);

	return ((neg) ? (-v) : v);
}

//Convert a vtd token into a float.
float parseFloat(VTDNav *vn, int index){

	Long exp,left, right, scale;
	float f;
	int end, ch;
	int t = getTokenType(vn,index);
	Boolean b = (t==TOKEN_CHARACTER_DATA) || (t== TOKEN_ATTR_VAL);
	Boolean neg;
	double v;
	Long l;
	int offset = getTokenOffset(vn,index);
	end = offset + getTokenLength(vn, index);
	//past the last one by one

	{
		l = b? getCharResolved(vn,offset):getChar(vn,offset);
		ch = (int)l;
		offset += (int)(l>>32);
	}

	while (offset <= end) { // trim leading whitespaces
		if (!isWS(ch))
			break;
		{
			l = b? getCharResolved(vn,offset):getChar(vn,offset);
			ch = (int)l;
			offset += (int)(l>>32);
		}
	}

	if (offset > end){// all whitespace
		float d1 = 0.0;
		return d1/d1;
	}
	//throw new NavException("Empty string");

	neg = (ch == '-');

	if (ch == '-' || ch == '+')
	{
		l = b? getCharResolved(vn,offset):getChar(vn,offset);
		ch = (int)l;
		offset += (int)(l>>32);
	} //get another one if it is sign.

	//left part of decimal
	left = 0;
	while (offset<= end) {
		//must be <= since we get the next one at last.

		int dig = Character_digit((char) ch, 10); //only consider decimal
		if (dig < 0)
			break;

		left = left * 10 + dig;

		{
			l = b? getCharResolved(vn,offset):getChar(vn,offset);
			ch = (int)l;
			offset += (int)(l>>32);
		}
	}

	//right part of decimal
	right = 0;
	scale = 1;
	if (ch == '.') {
		{
			l = b? getCharResolved(vn,offset):getChar(vn,offset);
			ch = (int)l;
			offset += (int)(l>>32);
		}

		while (offset <= end) {
			//must be <= since we get the next one at last.

			int dig = Character_digit((char) ch, 10);
			//only consider decimal
			if (dig < 0)
				break;

			right = right * 10 + dig;
			scale *= 10;

			{
				l = b? getCharResolved(vn,offset):getChar(vn,offset);
				ch = (int)l;
				offset += (int)(l>>32);
			}
		}
	}

	//exponent
	exp = 0;
	if (ch == 'E' || ch == 'e') {
		Boolean expneg;
		int cur;
		{
			l = b? getCharResolved(vn,offset):getChar(vn,offset);
			ch = (int)l;
			offset += (int)(l>>32);
		}
		expneg = (ch == '-'); //sign for exp
		if (ch == '+' || ch == '-')
		{
			l = b? getCharResolved(vn,offset):getChar(vn,offset);
			ch = (int)l;
			offset += (int)(l>>32);
		}//skip the +/- sign

		cur =offset;
		//remember the indx, used to find a invalid number like 1.23E

		while (offset <= end) {
			//must be <= since we get the next one at last.

			int dig = Character_digit((char) ch, 10);
			//only consider decimal
			if (dig < 0)
				break;

			exp = exp * 10 + dig;

			{
				l = b? getCharResolved(vn,offset):getChar(vn,offset);
				ch = (int)l;
				offset += (int)(l>>32);
			}
		}

		if (cur == offset){// all whitespace
			float f = 0;
			return f/f;
		}
		//	 throw new NavException(toString(index));
		//found a invalid number like 1.23E

		if (expneg)
			exp = (-exp);
	}

	/*anything left must be space*/
	while (offset <= end) {
		if (!isWS(ch)){/* all whitespace */
			float f = 0.0;
			return f/f;
		}
		{
			l = b? getCharResolved(vn,offset):getChar(vn,offset);
			ch = (int)l;
			offset += (int)(l>>32);
		}
	}

	v = (double) left;
	if (right != 0)
		v += ((double) right) / (double) scale;

	if (exp != 0)
		v = v * pow(10, (double)exp);


	f = (float) v;

	/*try to handle overflow/underflow */
	if (v >= MAXFLOAT)
		f = MAXFLOAT;
	else if (v <= MINFLOAT)
		f = MINFLOAT;

	if (neg)
		f = -f;
	return f;
}

/*Convert a vtd token into an int*/
int parseInt(VTDNav *vn, int index){
	return parseInt2(vn,index,10);
}

/*Convert a vtd token into an Int according to given radix.*/
static int parseInt2(VTDNav *vn, int index, int radix){
	Long pos, result;
	int endOffset, c;
	Boolean neg;

	Long l;
	int t = getTokenType(vn,index);
	Boolean b = (t==TOKEN_CHARACTER_DATA) || (t== TOKEN_ATTR_VAL);
	int offset = getTokenOffset(vn,index);
	endOffset = offset + getTokenLength(vn,index);

	{
		l = b? getCharResolved(vn,offset):getChar(vn,offset);
		c = (int)l;
		offset += (int)(l>>32);
	}

	// trim leading whitespaces
	while ((c == ' ' || c == '\n' || c == '\t' || c == '\r')
		&& (offset<= endOffset))
	{
		l = b? getCharResolved(vn,offset):getChar(vn,offset);
		c = (int)l;
		offset += (int)(l>>32);
	}

	if (offset > endOffset) {// all whitespace
		throwException2(number_format_exception,
			" empty string for parseInt2");
	}

	neg = (c == '-');
	if (neg || c == '+')
	{
		l = b? getCharResolved(vn,offset):getChar(vn,offset);
		c = (int)l;
		offset += (int)(l>>32);
	}//skip sign

	result = 0;
	pos = 1;
	while (offset <= endOffset) {
		int digit = Character_digit((char) c, radix);
		if (digit < 0)
			break;

		//Note: for binary we can simply shift to left to improve performance
		result = result * radix + digit;
		//pos *= radix;

		{
			l = b? getCharResolved(vn,offset):getChar(vn,offset);
			c = (int)l;
			offset += (int)(l>>32);
		}
	}

	if (result > MAXINT) {// all whitespace
		throwException2(number_format_exception,
			" integer value over(under) flow");
	}
	// throw new NumberFormatException("Overflow: " + toString(index));

	// take care of the trailing
	while (offset <= endOffset && isWS(c)) {
		l = b? getCharResolved(vn,offset):getChar(vn,offset);
		c = (int)l;
		offset += (int)(l>>32);
	}
	if (offset == (endOffset + 1))
		return (int) ((neg) ? (-result) : result);
	else{// all whitespace
		throwException2(number_format_exception,
			" invalid char during parseInt2");
		return -1;
	}
	// throw new NumberFormatException(toString(index));
}

//Convert a vtd token into a long
Long parseLong(VTDNav *vn, int index){
	return parseLong2(vn, index, 10);
}

//Convert a vtd token into a long according to given radix.
static Long parseLong2(VTDNav *vn, int index, int radix){
	int endOffset, c;
	Long result, pos;
	Boolean neg;
	Long l;
	int t = getTokenType(vn,index);
	Boolean b = (t==TOKEN_CHARACTER_DATA) || (t== TOKEN_ATTR_VAL);

	int offset = getTokenOffset(vn, index);
	endOffset = offset + getTokenLength(vn, index);

	{
		l = b? getCharResolved(vn,offset):getChar(vn,offset);
		c = (int)l;
		offset += (int)(l>>32);
	}
	// trim leading whitespaces
	while ((c == ' ' || c == '\n' || c == '\t' || c == '\r')
		&& (offset <= endOffset))
	{
		l = b? getCharResolved(vn,offset):getChar(vn,offset);
		c = (int)l;
		offset += (int)(l>>32);
	}

	if (offset > endOffset) {// all whitespace
		throwException2(number_format_exception,
			" empty string for parseLong2");
	}

	neg = (c == '-');
	if (neg || c == '+')
	{
		l = b? getCharResolved(vn,offset):getChar(vn,offset);
		c = (int)l;
		offset += (int)(l>>32);
	}//skip sign

	result = 0;
	pos = 1;
	while (offset <= endOffset) {
		int digit = Character_digit((char) c, radix);
		if (digit < 0)
			break;

		//Note: for binary we can simply shift to left to improve performance
		result = result * radix + digit;
		//pos *= radix;

		{
			l = b? getCharResolved(vn,offset):getChar(vn,offset);
			c = (int)l;
			offset += (int)(l>>32);
		}
	}

	if (result > MAXLONG) {// all whitespace
		throwException2(number_format_exception,
			" long value over(under) flow");
	}
	// throw new NumberFormatException("Overflow: " + toString(index));

	// take care of the trailing
	while (offset <= endOffset && isWS(c)) {
		{
			l = b? getCharResolved(vn,offset):getChar(vn,offset);
			c = (int)l;
			offset += (int)(l>>32);
		}
	}
	if (offset == (endOffset + 1))
		return  ((neg) ? (-result) : result);
	else{// all whitespace
		throwException2(number_format_exception,
			" invalid char during parseLong2");
		return -1;
	}
	//throw new NumberFormatException(toString(index));
}

//Load the context info from contextBuf.
//Info saved including LC and current state of the context 
Boolean _pop(VTDNav *vn){
	Boolean b = load(vn->contextBuf,vn->stackTemp);
	int i;
	if (b == FALSE)
		return FALSE;
	for ( i = 0; i < vn->nestingLevel; i++) {
		vn->context[i] = vn->stackTemp[i];
	}
	vn->l1index = vn->stackTemp[vn->nestingLevel];
	vn->l2index = vn->stackTemp[vn->nestingLevel + 1];
	vn->l3index = vn->stackTemp[vn->nestingLevel + 2];
	vn->l2lower = vn->stackTemp[vn->nestingLevel + 3];
	vn->l2upper = vn->stackTemp[vn->nestingLevel + 4];
	vn->l3lower = vn->stackTemp[vn->nestingLevel + 5];
	vn->l3upper = vn->stackTemp[vn->nestingLevel + 6];
	vn->atTerminal = (vn->stackTemp[vn->nestingLevel + 7] == 1);
	vn->LN  = vn->stackTemp[vn->nestingLevel + 8];
	return TRUE;
}


//Load the context info from contextBuf2.
//Info saved including LC and current state of the context 
// this function is for XPath evaluation only
Boolean _pop2(VTDNav *vn){
	Boolean b = load(vn->contextBuf2,vn->stackTemp);
	int i;
	if (b == FALSE)
		return FALSE;
	for ( i = 0; i < vn->nestingLevel; i++) {
		vn->context[i] = vn->stackTemp[i];
	}
	vn->l1index = vn->stackTemp[vn->nestingLevel];
	vn->l2index = vn->stackTemp[vn->nestingLevel + 1];
	vn->l3index = vn->stackTemp[vn->nestingLevel + 2];
	vn->l2lower = vn->stackTemp[vn->nestingLevel + 3];
	vn->l2upper = vn->stackTemp[vn->nestingLevel + 4];
	vn->l3lower = vn->stackTemp[vn->nestingLevel + 5];
	vn->l3upper = vn->stackTemp[vn->nestingLevel + 6];
	vn->atTerminal = (vn->stackTemp[vn->nestingLevel + 7] == 1);
	vn->LN  = vn->stackTemp[vn->nestingLevel + 8];
	return TRUE;
}

//Store the context info into the contextBuf.
//Info saved including LC and current state of the context 
Boolean _push(VTDNav *vn){
	int i;
	for (i = 0; i < vn->nestingLevel; i++) {
		vn->stackTemp[i] = vn->context[i];
	}
	vn->stackTemp[vn->nestingLevel] = vn->l1index;
	vn->stackTemp[vn->nestingLevel + 1] = vn->l2index;
	vn->stackTemp[vn->nestingLevel + 2] = vn->l3index;
	vn->stackTemp[vn->nestingLevel + 3] = vn->l2lower;
	vn->stackTemp[vn->nestingLevel + 4] = vn->l2upper;
	vn->stackTemp[vn->nestingLevel + 5] = vn->l3lower;
	vn->stackTemp[vn->nestingLevel + 6] = vn->l3upper;
	if (vn->atTerminal)
		vn->stackTemp[vn->nestingLevel + 7]=1;
	else 
		vn->stackTemp[vn->nestingLevel + 7]=0;
	vn->stackTemp[vn->nestingLevel + 8] = vn->LN;
	store(vn->contextBuf,vn->stackTemp);
	return TRUE;
}

//Store the context info into the contextBuf2.
//Info saved including LC and current state of the context 
// This function is for XPath evaluation only
Boolean _push2(VTDNav *vn){
	int i;
	for (i = 0; i < vn->nestingLevel; i++) {
		vn->stackTemp[i] = vn->context[i];
	}
	vn->stackTemp[vn->nestingLevel] = vn->l1index;
	vn->stackTemp[vn->nestingLevel + 1] = vn->l2index;
	vn->stackTemp[vn->nestingLevel + 2] = vn->l3index;
	vn->stackTemp[vn->nestingLevel + 3] = vn->l2lower;
	vn->stackTemp[vn->nestingLevel + 4] = vn->l2upper;
	vn->stackTemp[vn->nestingLevel + 5] = vn->l3lower;
	vn->stackTemp[vn->nestingLevel + 6] = vn->l3upper;
	if (vn->atTerminal)
		vn->stackTemp[vn->nestingLevel + 7]=1;
	else 
		vn->stackTemp[vn->nestingLevel + 7]=0;
	vn->stackTemp[vn->nestingLevel + 8] = vn->LN;
	store(vn->contextBuf2,vn->stackTemp);
	return TRUE;
}


//Sync up the current context with location cache.
void _resolveLC(VTDNav *vn){	
	if (vn->context[0]<=0)
		return;
	resolveLC_l1(vn);
	if (vn->context[0] == 1)
		return;
	resolveLC_l2(vn);	
	if (vn->context[0] == 2)
		return;
	resolveLC_l3(vn);

	/*if (vn->context[0] == 3)
	break;*/
}

//Test whether the URL is defined in the document.
static Boolean resolveNS(VTDNav *vn, UCSChar *URL){
	int i, offset, preLen;

	if (vn->context[0]==-1)
		return FALSE;
	i =
		getTokenLength(vn,(vn->context[0] != 0) ? vn->context[vn->context[0]] : vn->rootIndex);
	offset =
		getTokenOffset(vn,(vn->context[0] != 0) ? vn->context[vn->context[0]] : vn->rootIndex);
	preLen = (i >> 16) & 0xffff;

	i = lookupNS2(vn, offset, preLen);
	switch(i){
							 case 0: if (URL== NULL){
								 if (getTokenLength(vn,i)==0)
								 return TRUE;
									 } else {
										 return FALSE;
									 }
							 default:
								 if (URL == NULL){
									 return FALSE;
								 }
								 else {
									 return matchTokenString2(vn,i,URL);
								 }
	}
}

//Test whether the URL is defined in the document.
//Null is allowed to indicate the name space should be undefined.
static Boolean resolveNS2(VTDNav *vn, UCSChar *URL, int offset, int len){
	int i;
	i = lookupNS2(vn, offset, len);
	switch(i){
							 case 0: if (URL== NULL){
								 return TRUE;
									 } else {
										 return FALSE;
									 }
							 default:
								 if (URL == NULL){
									 return FALSE;
								 }
								 else {
									 return matchTokenString(vn,i,URL);
								 }
	}
}

void _sampleState(VTDNav *vn, FastIntBuffer *fib){
	int i=0;
	for (i=0;i<=vn->context[0];i++)
		   appendInt(fib,vn->context[i]);
		if (vn->atTerminal) appendInt(fib,vn->LN);
	
	if (vn->context[0]>=1)							
		appendInt(fib, vn->l1index);
	else return;

	if (vn->context[0]>=2){
		appendInt(fib,vn->l2index);
		appendInt(fib,vn->l2upper);
		appendInt(fib,vn->l2lower);
	}
	else return;

	if (vn->context[0]>=3){
		appendInt(fib,vn->l3index);
		appendInt(fib,vn->l3upper);
		appendInt(fib,vn->l3lower);
	}
	else return;

}
// A generic navigation method.
// Move the current to the element according to the direction constants
// If no such element, no position change and return FALSE (0).
/* Legal direction constants are 	<br>
* <pre>		ROOT            0  </pre>
* <pre>		PARENT          1  </pre>
* <pre>		FIRST_CHILD     2  </pre>
* <pre>		LAST_CHILD      3  </pre>
* <pre>		NEXT_SIBLING    4  </pre>
* <pre>		PREV_SIBLING    5  </pre>
* <br>
*/
Boolean _toElement(VTDNav *vn, navDir direction){
	int size,i;
	switch (direction) {
			case ROOT :
				if (vn->context[0] != 0) {
					/*for (i = 1; i <= vn->context[0]; i++) {
					vn->context[i] = 0xffffffff;
					}*/
					vn->context[0] = 0;
				}
				vn->atTerminal = FALSE;
				//vn->l1index = vn->l2index = vn->l3index = -1;
				return TRUE;
			case PARENT :
				if (vn->atTerminal == TRUE){
					vn->atTerminal = FALSE;
					return TRUE;
				}
				if (vn->context[0] > 0) {
					//vn->context[vn->context[0]] = vn->context[vn->context[0] + 1] = 0xffffffff;
					//vn->context[vn->context[0]] = -1;
					vn->context[0]--;
					return TRUE;
				} else if (vn->context[0]==0){
					vn->context[0]=-1;
					return TRUE;
				}else {
					return FALSE;
				}
			case FIRST_CHILD :
			case LAST_CHILD :
				if (vn->atTerminal) return FALSE;
				switch (vn->context[0]) {
			case -1:
				vn->context[0] = 0;
				return TRUE;
			case 0 :
				if (vn->l1Buffer->size > 0) {
					vn->context[0] = 1;
					vn->l1index =
						(direction == FIRST_CHILD)
						? 0
						: (vn->l1Buffer->size - 1);
					vn->context[1] = upper32At(vn->l1Buffer,vn->l1index);
					//(int) (vtdToken >> 32);
					return TRUE;
				} else
					return FALSE;
			case 1 :
				vn->l2lower = lower32At(vn->l1Buffer,vn->l1index);
				if (vn->l2lower == -1) {
					return FALSE;
				}
				vn->context[0] = 2;
				vn->l2upper = vn->l2Buffer->size - 1;
				size = vn->l1Buffer->size;
				for (i = vn->l1index + 1; i < size; i++) {
					int temp = lower32At(vn->l1Buffer,i);
					if (temp != 0xffffffff) {
						vn->l2upper = temp - 1;
						break;
					}
				}
				//System.out.println(" l2 upper: " + l2upper + " l2 lower : " + l2lower);
				vn->l2index =
					(direction == FIRST_CHILD) ? vn->l2lower : vn->l2upper;
				vn->context[2] = upper32At(vn->l2Buffer,vn->l2index);
				return TRUE;

			case 2 :
				vn->l3lower = lower32At(vn->l2Buffer,vn->l2index);
				if (vn->l3lower == -1) {
					return FALSE;
				}
				vn->context[0] = 3;
				size = vn->l2Buffer->size;
				vn->l3upper = vn->l3Buffer->size - 1;

				for (i = vn->l2index + 1; i < size; i++) {
					int temp = lower32At(vn->l2Buffer,i);
					if (temp != 0xffffffff) {
						vn->l3upper = temp - 1;
						break;
					}
				}
				//System.out.println(" l3 upper : " + l3upper + " l3 lower : " + l3lower);
				vn->l3index =
					(direction == FIRST_CHILD) ? vn->l3lower : vn->l3upper;
				vn->context[3] = intAt(vn->l3Buffer,vn->l3index);

				return TRUE;

			default :
				if (direction == FIRST_CHILD) {
					int index;
					size = vn->vtdBuffer->size;
					index = vn->context[vn->context[0]] + 1;
					while (index < size) {
						Long temp = longAt(vn->vtdBuffer,index);
						int token_type;

						token_type =
							(int) ((MASK_TOKEN_TYPE & temp) >> 60)
							& 0xf;


						if (token_type == TOKEN_STARTING_TAG) {

							int depth =
								(int) ((MASK_TOKEN_DEPTH & temp) >> 52);

							if (depth <= vn->context[0]) {
								return FALSE;
							} else if (depth == (vn->context[0] + 1)) {
								vn->context[0] += 1;
								vn->context[vn->context[0]] = index;
								return TRUE;
							}
						}

						index++;
					} // what condition  
					return FALSE;
				} else {
					int index = vn->context[vn->context[0]] + 1;
					int last_index = -1;
					size = vn->vtdBuffer->size;
					while (index < size) {
						Long temp = longAt(vn->vtdBuffer,index);
						int token_type;
						int depth;

						depth =
							(int) ((MASK_TOKEN_DEPTH & temp) >> 52);



						token_type =
							(int) ((MASK_TOKEN_TYPE & temp) >> 60)
							& 0xf;

						if (token_type == TOKEN_STARTING_TAG) {
							if (depth <= vn->context[0]) {
								break;
							} else if (depth == (vn->context[0] + 1)) {
								last_index = index;
							}
						}
						index++;
					}

					if (last_index == -1) {
						return FALSE;
					} else {
						vn->context[0] += 1;
						vn->context[vn->context[0]] = last_index;
						return TRUE;
					}
				}
				}

			case NEXT_SIBLING :
			case PREV_SIBLING :
				if (vn->atTerminal) return nodeToElement(vn,direction);
				switch (vn->context[0]) {
			case -1:
			case 0 :
				return FALSE;
			case 1 :
				if (direction == NEXT_SIBLING) {
					if (vn->l1index + 1 >= vn->l1Buffer->size) {
						return FALSE;
					}

					vn->l1index++; // global incremental
				} else {
					if (vn->l1index - 1 < 0) {
						return FALSE;
					}
					vn->l1index--; // global incremental
				}
				vn->context[1] = upper32At(vn->l1Buffer,vn->l1index);
				return TRUE;
			case 2 :
				if (direction == NEXT_SIBLING) {
					if (vn->l2index + 1 > vn->l2upper) {
						return FALSE;
					}
					vn->l2index++;
				} else {
					if (vn->l2index - 1 < vn->l2lower) {
						return FALSE;
					}
					vn->l2index--;
				}
				vn->context[2] = upper32At(vn->l2Buffer,vn->l2index);
				return TRUE;
			case 3 :
				if (direction == NEXT_SIBLING) {
					if (vn->l3index + 1 > vn->l3upper) {
						return FALSE;
					}
					vn->l3index++;
				} else {
					if (vn->l3index - 1 < vn->l3lower) {
						return FALSE;
					}
					vn->l3index--;
				}
				vn->context[3] = intAt(vn->l3Buffer,vn->l3index);
				return TRUE;
			default :
				//int index = context[context[0]] + 1;

				if (direction == NEXT_SIBLING) {
					int index = vn->context[vn->context[0]] + 1;
					int size = vn->vtdBuffer->size;
					while (index < size) {
						Long temp = longAt(vn->vtdBuffer,index);
						int token_type;

						token_type =
							(int) ((MASK_TOKEN_TYPE & temp) >> 60)
							& 0xf;

						if (token_type == TOKEN_STARTING_TAG) {
							int depth =
								(int) ((MASK_TOKEN_DEPTH & temp) >> 52);

							if (depth < vn->context[0]) {
								return FALSE;
							} else if (depth == (vn->context[0])) {
								vn->context[vn->context[0]] = index;
								return TRUE;
							}
						}
						index++;
					}
					return FALSE;
				} else {
					int index = vn->context[vn->context[0]] - 1;
					while (index > vn->context[vn->context[0] - 1]) {
						// scan backforward
						Long temp =longAt(vn->vtdBuffer,index);

						tokenType token_type =
							(int) ((MASK_TOKEN_TYPE & temp) >> 60)
							& 0xf;

						if (token_type == TOKEN_STARTING_TAG) {

							int depth =
								(int) ((MASK_TOKEN_DEPTH & temp) >> 52);

							/*if (depth < vn->context[0]) {
							return FALSE;
							} else */
							if (depth == (vn->context[0])) {
								vn->context[vn->context[0]] = index;
								return TRUE;
							}
						}
						index--;
					} // what condition          	    
					return FALSE;
				}
				}

			default :
				return FALSE;
	}

}

/**
* A generic navigation method.
* Move the current to the element according to the direction 
* constants and the element name
* If no such element, no position change and return FALSE (0).
* "*" matches any element
* Legal direction constants are 	<br>
* <pre>		ROOT            0  </pre>
* <pre>		PARENT          1  </pre>
* <pre>		FIRST_CHILD     2  </pre>
* <pre>		LAST_CHILD      3  </pre>
* <pre>		NEXT_SIBLING    4  </pre>
* <pre>		PREV_SIBLING    5  </pre>
* <br>
* for ROOT and PARENT, element name will be ignored.
*/
Boolean _toElement2(VTDNav *vn, navDir direction, UCSChar *en){
	//int size;
	int temp=-1;
	int d=-1;
	int val = 0;
	Boolean b= FALSE; 
	if (en == NULL){
		throwException2(invalid_argument,
			"inavlid argument for toElement2");
	}
	if (wcscmp(en,L"*") == 0)
		return toElement(vn,direction);
	switch (direction) {
			case ROOT :
				return toElement(vn,ROOT);

			case PARENT :
				return toElement(vn,PARENT);

			case FIRST_CHILD :
				if (vn->atTerminal)return FALSE;
				if (toElement(vn,FIRST_CHILD) == FALSE)
					return FALSE;
				// check current element name
				if (matchElement(vn,en) == FALSE) {
					if (toElement2(vn,NEXT_SIBLING, en) == TRUE)
						return TRUE;
					else {
						//toParentElement();
						//vn->context[vn->context[0]] = 0xffffffff;
						vn->context[0]--;
						return FALSE;
					}
				} else
					return TRUE;

			case LAST_CHILD :
				if (vn->atTerminal)return FALSE;
				if (toElement(vn,LAST_CHILD) == FALSE)
					return FALSE;
				if (matchElement(vn,en) == FALSE){
					if (toElement2(vn,PREV_SIBLING, en) == TRUE)
						return TRUE;
					else {
						//vn->context[vn->context[0]] = 0xffffffff;
						vn->context[0]--;
						//toParentElement();
						return FALSE;
					}
				} else
					return TRUE;

			case NEXT_SIBLING :
				if (vn->atTerminal){					
					if (nodeToElement(vn,NEXT_SIBLING)){
						b=TRUE;
						if (matchElement(vn,en)){
							return TRUE;
						}					
					}else
						return FALSE;
				}
				
				if (!b){
					d = vn->context[0];
					temp = vn->context[d]; // store the current position

					switch(d)
					{
					case -1:
					case 0: return FALSE;
					case 1: val = vn->l1index; break;
					case 2: val = vn->l2index; break;
					case 3: val = vn->l3index; break;
					default:
						break;
					}
				}
				//if (d == 0)
				//	return FALSE;
				while (toElement(vn,NEXT_SIBLING)) {
					if (matchElement(vn,en)) {
						return TRUE;
					}
				}
				if (b){
					vn->context[0]--;//LN value should not change
					vn->atTerminal=TRUE;
					return FALSE;
				} else {
					switch(d)
					{
					case 1: vn->l1index = val; break;
					case 2: vn->l2index = val; break;
					case 3: vn->l3index = val; break;
					default:
						break;
					}
					vn->context[d] = temp;
					return FALSE;
				}

			case PREV_SIBLING :
				if (vn->atTerminal){					
					if (nodeToElement(vn,PREV_SIBLING)){
						b=TRUE;
						if (matchElement(vn,en)){
							return TRUE;
						}					
					}else
						return FALSE;
				}				
				if (!b){
					d = vn->context[0];
					temp = vn->context[d]; // store the current position
					switch(d)
					{
					case -1:
					case 0: return FALSE;
					case 1: val = vn->l1index; break;
					case 2: val = vn->l2index; break;
					case 3: val = vn->l3index; break;
					default:
						break;
					}
				}
				//if (d == 0)
				//	return FALSE;
				while (toElement(vn,PREV_SIBLING)) {
					if (matchElement(vn,en)) {
						return TRUE;
					}
				}
				if (b){
					vn->context[0]--;//LN value should not change
					vn->atTerminal=TRUE;
					return FALSE;
				} else{	
					switch(d)
					{
					case 1: vn->l1index = val; break;
					case 2: vn->l2index = val; break;
					case 3: vn->l3index = val; break;
					default:
						break;
					}
					vn->context[d] = temp;
					return FALSE;
				}

			default :
				return FALSE;
				//throw new NavException("illegal navigation options");
	}
}
/*	
* A generic navigation function with namespace support.
* Move the current to the element according to the direction constants and the prefix and local names
* If no such element, no position change and return FALSE(0).
* URL * matches any namespace, including undefined namespaces
* a null URL means hte namespace prefix is undefined for the element
* ln *  matches any localname
* Legal direction constants are<br>
* <pre>		ROOT            0  </pre>
* <pre>		PARENT          1  </pre>
* <pre>		FIRST_CHILD     2  </pre>
* <pre>		LAST_CHILD      3  </pre>
* <pre>		NEXT_SIBLING    4  </pre>
* <pre>		PREV_SIBLING    5  </pre>
* <br>
* for ROOT and PARENT, element name will be ignored.
* If not ns enabled, return FALSE immediately with no position change.
*/
Boolean _toElementNS(VTDNav *vn, navDir direction, UCSChar *URL, UCSChar *ln){
	//int size;
	int temp=-1;
	int d=-1;
	int val = 0;
	Boolean b=FALSE;
	if (vn->ns == FALSE)
		return FALSE;
	switch (direction) {
			case ROOT :
				return toElement(vn,ROOT);

			case PARENT :
				return toElement(vn,PARENT);

			case FIRST_CHILD :
				if (vn->atTerminal)return FALSE;
				if (toElement(vn,FIRST_CHILD) == FALSE)
					return FALSE;
				// check current element name
				if (matchElementNS(vn,URL, ln) == FALSE) {
					if (toElementNS(vn,NEXT_SIBLING, URL, ln) == TRUE )
						return TRUE;
					else {
						//toParentElement();
						//vn->context[vn->context[0]] = 0xffffffff;
						vn->context[0]--;
						return FALSE;
					}
				} else
					return TRUE;

			case LAST_CHILD :
				if (vn->atTerminal)return FALSE;
				if (toElement(vn,LAST_CHILD) == FALSE)
					return FALSE;
				if (matchElementNS(vn, URL, ln) == FALSE) {
					if (toElementNS(vn, PREV_SIBLING, URL, ln) == TRUE)
						return TRUE;
					else {
						//vn->context[vn->context[0]] = 0xffffffff;
						vn->context[0]--;
						//toParentElement();
						return FALSE;
					}
				} else
					return TRUE;

			case NEXT_SIBLING :
				if (vn->atTerminal){					
					if (nodeToElement(vn,NEXT_SIBLING)){
						b=TRUE;
						if (matchElementNS(vn,URL,ln)){
							return TRUE;
						}					
					}else
						return FALSE;
				}
				if (!b){
					d = vn->context[0];
					temp = vn->context[d]; // store the current position
					switch(d)
					{
					case -1:
					case 0: return FALSE;
					case 1: val = vn->l1index; break;
					case 2: val = vn->l2index; break;
					case 3: val = vn->l3index; break;
					default:
						break;
					}
				}
				//if (d == 0)
				//	return FALSE;
				while (toElement(vn,NEXT_SIBLING)) {
					if (matchElementNS(vn,URL, ln)) {
						return TRUE;
					}
				}
				if (b){					
					vn->context[0]--;//LN value should not change
					vn->atTerminal=TRUE;
					return FALSE;
				}
				else{
					switch(d)
					{
					case 1: vn->l1index = val; break;
					case 2: vn->l2index = val; break;
					case 3: vn->l3index = val; break;
					default:
						break;
					}
					vn->context[d] = temp;
					return FALSE;
				}

			case PREV_SIBLING :
				if (vn->atTerminal){
					if (nodeToElement(vn,PREV_SIBLING)){
						b=TRUE;
						if (matchElementNS(vn,URL,ln)){
							return TRUE;
						}					
					}else
						return FALSE;
					}
				if (!b){
					d = vn->context[0];
					temp = vn->context[d]; // store the current position
					switch(d)
					{
					case -1: 
					case  0: return FALSE;
					case 1: val = vn->l1index; break;
					case 2: val = vn->l2index; break;
					case 3: val = vn->l3index; break;
					default:
						break;
					}
				}
				//if (d == 0)
				//	return FALSE;
				while (toElement(vn,PREV_SIBLING)) {
					if (matchElementNS(vn,URL, ln)) {
						return TRUE;
					}
				}
				if (b){
					vn->context[0]--;//LN value should not change
					vn->atTerminal=TRUE;
					return FALSE;
				}else {
					switch(d)
					{
					case 1: vn->l1index = val; break;
					case 2: vn->l2index = val; break;
					case 3: vn->l3index = val; break;
					default:
						break;
					}
					vn->context[d] = temp;
					return FALSE;
				}
			default :
				return FALSE;
				//throw new NavException("illegal navigation options");
	}

}

/*This method normalizes a token into a string in a way that resembles DOM.
The leading and trailing white space characters will be stripped.
The entity and character references will be resolved
Multiple whitespaces char will be collapsed into one.*/

UCSChar *toNormalizedString(VTDNav *vn, int index){
	tokenType type = getTokenType(vn,index);

	if (type!=TOKEN_CHARACTER_DATA &&
		type!= TOKEN_ATTR_VAL)
		return toRawString(vn, index);
	else {
		int len, endOffset;
		int ch,k,offset;
		Long l;
		Boolean d;
		UCSChar *s = NULL;


		len = getTokenLength(vn,index);

		s = (UCSChar *)malloc(sizeof(UCSChar)*(len+1));
		if (s == NULL)
		{
			throwException2(out_of_mem,
				" string allocation failed in toNormalizedString ");
		}

		offset = getTokenOffset(vn ,index);
		endOffset = len + offset - 1; // point to the last character


		// trim off the leading whitespaces

		while (TRUE) {
			int temp = offset;
			l = getChar(vn,offset);
			ch = (int)l;
			offset += (UCSChar)(l>>32);

			if (!isWS(ch)) {
				offset = temp;
				break;
			}
		}

		d = FALSE;
		k = 0;
		while (offset <= endOffset) {
			l= getCharResolved(vn,offset);
			ch = (int)l;
			offset += (int)(l>>32);

			if (isWS(ch)&& getCharUnit(vn,offset - 1) != ';') {
				d = TRUE;
			} else {
				if (d == FALSE)
					//sb.append((char) ch); // java only supports 16 bit unicode
					s[k++] = ch;
				else {
					//sb.append(' ');
					s[k++] = (UCSChar) ' ';
					//sb.append((char) ch);
					s[k++] = (UCSChar) ch;
					d = FALSE;
				}
			}
		}
		s[k] = 0;
		return s;
	}
}

/*Convert a token at the given index to a String, 
(built-in entity and char references not resolved)
(entities and char references not expanded).*/
UCSChar *toRawString(VTDNav *vn, int index){						
	int offset;
	tokenType type = getTokenType(vn,index);
	int len;						 
	/*UCSChar *s = NULL;*/

	if (type == TOKEN_STARTING_TAG
		|| type == TOKEN_ATTR_NAME
		|| type == TOKEN_ATTR_NS)
		len = getTokenLength(vn,index) & 0xffff;
	else
		len = getTokenLength(vn,index);

	offset = getTokenOffset(vn,index);
	return toRawString2(vn, offset, len);
}

UCSChar *toRawString2(VTDNav *vn, int os, int len){
	int offset = os, endOffset=os+len,k=0;
	Long l;
	UCSChar *s = NULL;
	s = (UCSChar *)malloc(sizeof(UCSChar)*(len+1));
	if (s == NULL)
	{
		throwException2(out_of_mem,
			" string allocation failed in toRawString2 ");
	}
	/*if (vn->encoding > FORMAT_WIN_1258){
		offset = offset>>1;
		endOffset = endOffset>>1;
	}*/
	while (offset < endOffset) {
		l = getChar(vn,offset);
		offset += (int)(l>>32);
		s[k++] = (UCSChar)l; // java only support 16 bit unit code
	}
	s[k] = 0;
	return s;
}


//Convert a token at the given index to a String, (entities and char 
//references resolved).
// An attribute name or an element name will get the UCS2 string of qualified name 
UCSChar *toString(VTDNav *vn, int index){
	int offset;
	tokenType type = getTokenType(vn,index);
	int len;

	/*UCSChar *s = NULL;*/
	if (type!=TOKEN_CHARACTER_DATA &&
		type!= TOKEN_ATTR_VAL)
		return toRawString(vn,index);

	len = getTokenLength(vn,index);

	offset = getTokenOffset(vn,index);
	return toString2(vn,offset,len);
}

UCSChar *toString2(VTDNav *vn, int os, int len){
	int offset = os, endOffset=os+len,k=0;
	Long l;
	UCSChar *s = NULL;
	s = (UCSChar *)malloc(sizeof(UCSChar)*(len+1));
	if (s == NULL)
	{
		throwException2(out_of_mem,
			" string allocation failed in toString2 ");
	}
	/*if (vn->encoding > FORMAT_WIN_1258){
		offset = offset>>1;
		endOffset = endOffset>>1;
	}*/
	while (offset < endOffset) {
		l = getCharResolved(vn,offset);
		offset += (int)(l>>32);
		s[k++] = (UCSChar)l; // java only support 16 bit unit code
	}
	s[k] = 0;
	return s;
}

//Get the starting offset of the token at the given index.

/*int getTokenOffset(VTDNav *vn, int index){

	return (int) (longAt(vn->vtdBuffer,index) & vn->offsetMask);

}*/
// Get the XML document 
/*UByte* getXML(VTDNav *vn){
	return vn->XMLDoc;
}*/

//Get the token type of the token at the given index value.
/*tokenType getTokenType(VTDNav *vn, int index){
	return (tokenType) ((longAt(vn->vtdBuffer,index) & MASK_TOKEN_TYPE) >> 60) & 0xf;
}*/

//Get the depth (>=0) of the current element.
/*int getCurrentDepth(VTDNav *vn){
	return vn->context[0];
}*/
// Get the index value of the current element/text/attribute.
/*int getCurrentIndex(VTDNav *vn){
	if (vn->atTerminal)
		return vn->LN;
	switch(vn->context[0]){
		case -1: return 0;
		case 0: return vn->rootIndex;
		default: return vn->context[vn->context[0]];
	}
	//return (vn->context[0] == 0) ? vn->rootIndex : vn->context[vn->context[0]];
}*/

/*int getCurrentIndex2(VTDNav *vn){
	switch(vn->context[0]){
		case -1: return 0;
		case 0: return vn->rootIndex;
		default: return vn->context[vn->context[0]];
	}
	//return (vn->context[0] == 0) ? vn->rootIndex : vn->context[vn->context[0]];
}*/

/**
* Get the encoding of the XML document.
* <pre>   0  ASCII       </pre>
* <pre>   1  ISO-8859-1  </pre>
* <pre>   2  UTF-8       </pre>
* <pre>   3  UTF-16BE    </pre>
* <pre>   4  UTF-16LE    </pre>
*/
/*inline encoding_t getEncoding(VTDNav *vn){
	return vn->encoding;
}*/

// Get the maximum nesting depth of the XML document (>0).
// max depth is nestingLevel -1

// max depth is nestingLevel -1
/*inline int getNestingLevel(VTDNav *vn){
	return vn->nestingLevel;
}*/
// Get root index value.
/*inline int getRootIndex(VTDNav *vn){
	return vn->rootIndex;
}*/

//Get total number of VTD tokens for the current XML document.
/*inline int getTokenCount(VTDNav *vn){
	return vn->vtdSize;
}*/


/**
* Set the value of atTerminal
* This function only gets called in XPath eval
* when a step calls for @* or child::text()
*/

/*inline void setAtTerminal(VTDNav* vn, Boolean b){
	vn->atTerminal = b;
}*/

/**
* Get the value of atTerminal
* This function only gets called in XPath eval
*/
/*inline Boolean getAtTerminal(VTDNav *vn){
	return vn->atTerminal;
}*/


/*inline int swap_bytes(int i){
	return (((i & 0xff) << 24) |
		((i & 0xff00) <<8) |
		((i & 0xff0000) >> 8) |
		((i & 0xff000000) >> 24)&0xff);
}*/


static int lookupNS2(VTDNav *vn, int offset, int len){
	Long l; 
	int i,k;
	tokenType type;
	Boolean hasNS = FALSE;
	int size = vn->vtdBuffer->size;
	// look for a match in the current hiearchy and return TRUE
	for (i = vn->context[0]; i >= 0; i--) {
		int s = (i != 0) ? vn->context[i] : vn->rootIndex;
		switch (NSval(vn,s)) { // checked the ns marking
				case 0xc0000000 :
					s = s + 1;
					if (s>=size)
						break;
					type = getTokenType(vn,s);

					while (s < size
						&& (type == TOKEN_ATTR_NAME || type == TOKEN_ATTR_NS)) {
							if (type == TOKEN_ATTR_NS) {
								// Get the token length
								int temp = getTokenLength(vn, s);
								int preLen = ((temp >> 16) & 0xffff);
								int fullLen = temp & 0xffff;
								int os = getTokenOffset(vn, s);
								// xmlns found
								if (temp == 5 && len == 0) {
									return s+1;
								} else if ((fullLen - preLen - 1) == len) {
									// prefix length identical to local part of ns declaration
									Boolean a = TRUE;
									int j;
									for (j = 0; j < len; j++) {
										if (getCharUnit(vn,os + preLen + 1 + j)
											!= getCharUnit(vn,offset + j)) {
												a = FALSE;
												break;
										}
									}
									if (a == TRUE) {
										return s+1;
									}
								}
							}
							//return (URL != null) ? TRUE : FALSE;
							s += 2;
							type = getTokenType(vn,s);
					}
					break;
				case 0x80000000 :
					break;
				default : // check the ns existence, mark bit 31:30 to 11 or 10
					k = s + 1;
					if (k>=size)
						break;
					type = getTokenType(vn,k);

					while ((type == TOKEN_ATTR_NAME || type == TOKEN_ATTR_NS)) {
						if (type == TOKEN_ATTR_NS) {
							// Get the token length

							int temp = getTokenLength(vn, k);
							int preLen = ((temp >> 16) & 0xffff);
							int fullLen = temp & 0xffff;
							int os = getTokenOffset(vn, k);
							hasNS = TRUE;
							// xmlns found
							if (temp == 5 && len == 0) {
								l = longAt(vn->vtdBuffer,s);
								hasNS = FALSE;

								modifyEntryFLB(vn->vtdBuffer,
									s,
									l | 0x00000000c0000000L);
								return k+1;
							} else if ((fullLen - preLen - 1) == len) {
								// prefix length identical to local part of ns declaration
								Boolean a = TRUE;
								int j;
								for (j = 0; j < len; j++) {
									if (getCharUnit(vn, os + preLen + 1 + j)
										!= getCharUnit(vn, offset + j)) {
											a = FALSE;
											break;
									}
								}
								if (a == TRUE) {
									l = longAt(vn->vtdBuffer,s);
									//hasNS = FALSE;

									modifyEntryFLB(vn->vtdBuffer,
										s,
										l | 0x00000000c0000000L);

									return k+1;
								}
							}
						}
						//return (URL != null) ? TRUE : FALSE;
						k += 2;
						if (k>=size)
							break;
						type = getTokenType(vn,k);
					}
					l = longAt(vn->vtdBuffer, s);

					if (hasNS) {
						hasNS = FALSE;
						modifyEntryFLB(vn->vtdBuffer, s, l | 0x00000000c0000000L);
					} else {
						modifyEntryFLB(vn->vtdBuffer, s, l | 0x0000000080000000L);
					}

					break;
		}
	}
	return 0;

}

int lookupNS(VTDNav *vn){
	int i, offset, preLen;

	if (vn->context[0]==-1)
		return FALSE;
	i =
		getTokenLength(vn,(vn->context[0] != 0) ? vn->context[vn->context[0]] : vn->rootIndex);
	offset =
		getTokenOffset(vn,(vn->context[0] != 0) ? vn->context[vn->context[0]] : vn->rootIndex);
	preLen = (i >> 16) & 0xffff;

	return lookupNS2(vn,offset, preLen);
}

static Long getChar4OtherEncoding(VTDNav *vn, int offset){

	if (vn->encoding <= FORMAT_WIN_1258) 
	{ 
		int temp = decode(vn,offset); 
		if (temp == '\r') 
		{ 
			if (vn->XMLDoc[offset + 1] == '\n') 
			{ 
				return '\n' | (2LL << 32); 
			} 
			else 
			{ 
				return '\n' | (1LL << 32); 
			} 
		} 
		return temp | (1LL << 32); 
	} 
	{
		throwException(nav_exception,0,
			"navigation exception during getChar4OtherEncoding",	
			"Unknown encoding error: should never happen");
		return 0;
	}
}

static int decode(VTDNav *vn,int offset)
{
	char ch = vn->XMLDoc[offset];
	switch (vn->encoding)
	{
	case FORMAT_ISO_8859_2:
		return iso_8859_2_decode(ch);
	case FORMAT_ISO_8859_3:
		return iso_8859_3_decode(ch);
	case FORMAT_ISO_8859_4:
		return iso_8859_4_decode(ch);
	case FORMAT_ISO_8859_5:
		return iso_8859_5_decode(ch);
	case FORMAT_ISO_8859_6:
		return iso_8859_6_decode(ch);
	case FORMAT_ISO_8859_7:
		return iso_8859_7_decode(ch);
	case FORMAT_ISO_8859_8:
		return iso_8859_8_decode(ch);
	case FORMAT_ISO_8859_9:
		return iso_8859_9_decode(ch);
	case FORMAT_ISO_8859_10:
		return iso_8859_10_decode(ch);
	case FORMAT_ISO_8859_11:
		return iso_8859_11_decode(ch);
	case FORMAT_ISO_8859_13:
		return iso_8859_13_decode(ch);
	case FORMAT_ISO_8859_14:
		return iso_8859_14_decode(ch);
	case FORMAT_ISO_8859_15:
		return iso_8859_15_decode(ch);
	case FORMAT_WIN_1250:
		return windows_1250_decode(ch);
	case FORMAT_WIN_1251:
		return windows_1251_decode(ch);
	case FORMAT_WIN_1252:
		return windows_1252_decode(ch);
	case FORMAT_WIN_1253:
		return windows_1253_decode(ch);
	case FORMAT_WIN_1254:
		return windows_1254_decode(ch);
	case FORMAT_WIN_1255:
		return windows_1255_decode(ch);
	case FORMAT_WIN_1256:
		return windows_1256_decode(ch);
	case FORMAT_WIN_1257:
		return windows_1257_decode(ch);
	default:
		return windows_1258_decode(ch);
	}
}

void throwException(enum exception_type et, int sub_type, char* msg, char* submsg){
	exception e;
	e.et = et;
	e.subtype = sub_type;
	e.msg = msg;
	e.sub_msg = submsg;
	Throw e;
}

void throwException2 (enum exception_type et, char *msg){
	exception e;
	e.et = et;
	e.msg = msg;
	Throw e;
}

/* overwrite */
Boolean overWrite(VTDNav *vn, int index, UByte* ba, int offset, int len){
	int t;
	if (ba == NULL
		|| index >= vn->vtdSize
		|| offset < 0){
			throwException2(invalid_argument,"Illegal argument for overwrite");
			return FALSE;
	}
	if (vn->encoding >= FORMAT_UTF_16BE
		&& (((len & 1) == 1)
		|| ((offset & 1) == 1)))
	{
		// for UTF 16, len and offset must be integer multiple
		// of 2
		return FALSE;
	}
	t = getTokenType(vn,index);
	if (t == TOKEN_CHARACTER_DATA
		|| t == TOKEN_ATTR_VAL
		|| t == TOKEN_CDATA_VAL)
	{
		int os, length,temp,k;
		length = getTokenLength(vn,index);
		if (length < len)
			return FALSE;
		os = getTokenOffset(vn,index);
		temp = length - len;
		// get XML doc
		//Array.Copy(ba, offset, XMLDoc.getBytes(), os, len);
		memcpy(vn->XMLDoc+os,ba+offset,len);
		for (k = 0; k < temp; )
		{
			if (vn->encoding < FORMAT_UTF_16BE)
			{
				// write white spaces
				vn->XMLDoc[os + len + k] = ' ';
				k++;
			}
			else
			{
				if (vn->encoding == FORMAT_UTF_16BE)
				{
					vn->XMLDoc[os + len + k] = 0;
					vn->XMLDoc[os + len + k+1] = ' ';
				}
				else
				{
					vn->XMLDoc[os + len + k] = ' ';
					vn->XMLDoc[os + len + k + 1] = 0;                            
				}
				k += 2;
			}
		}
		return TRUE;
	}
	return FALSE;
}

int compareRawTokenString2(VTDNav *vn, int offset, int len, UCSChar *s){
	int endOffset;
	int l,i;
	Long l1;
	//throw new IllegalArgumentException("string can't be null");

	//vn->currentOffset = offset;
	endOffset = offset + len;

	l = (int)wcslen(s);
	for (i = 0; i < l && offset < endOffset; i++) {
		l1 = getChar(vn,offset);
		if (s[i] < (int) l1)
            return 1;
        if (s[i] > (int) l1)
            return -1;
		offset += (int)(l1 >> 32);
	}

	if (i == l && offset < endOffset)
		return 1;
	if (i < l && offset == endOffset)
		return -1;
	return 0;
}

int compareTokenString2(VTDNav *vn, int offset, int len, UCSChar *s){
	int endOffset;
	int l,i;
	Long l1;

	//throw new IllegalArgumentException("string can't be null");

	//vn->currentOffset = offset;
	endOffset = offset + len;

	l = (int)wcslen(s);
	for (i = 0; i < l && offset < endOffset; i++) {
		l1 = getCharResolved(vn,offset);
		offset += (int)(l1>>32);
		if (s[i] < (int) l1)
            return 1;
        if (s[i] > (int) l1)
            return -1;
	}

	if (i == l && offset < endOffset)
		return 1;
	if (i < l && offset == endOffset)
		return -1;
	return 0;

}

int compareTokenString(VTDNav *vn,int index, UCSChar *s){
	tokenType type;
	int offset;
	int len;

	type = getTokenType(vn,index);
	len =
		(type == TOKEN_STARTING_TAG
		|| type == TOKEN_ATTR_NAME
		|| type == TOKEN_ATTR_NS)
		? getTokenLength(vn,index) & 0xffff
		: getTokenLength(vn,index);
	// upper 16 bit is zero or for prefix

	offset = getTokenOffset(vn,index);
	// point currentOffset to the beginning of the token
	// for UTF 8 and ISO, the performance is a little better by avoid calling getChar() everytime
	if (type == TOKEN_CHARACTER_DATA
		|| type == TOKEN_ATTR_VAL)
		return compareTokenString2(vn,offset, len, s);
	else
		return compareRawTokenString2(vn,offset, len,s);
}

int compareRawTokenString(VTDNav *vn, int index, UCSChar *s){
	tokenType type;
	int len;
	int offset;
	/*if (s == NULL){
	//	 throwException2(invalid_argument,
	//		 " invalid argument for matchRawTokenString, s can't be NULL");
	}*/
	type = getTokenType(vn,index);
	len =
		(type == TOKEN_STARTING_TAG
		|| type == TOKEN_ATTR_NAME
		|| type == TOKEN_ATTR_NS)
		? getTokenLength(vn, index) & 0xffff
		: getTokenLength(vn, index);
	// upper 16 bit is zero or for prefix

	offset = getTokenOffset(vn, index);
	// point currentOffset to the beginning of the token
	// for UTF 8 and ISO, the performance is a little better by avoid calling getChar() everytime
	return compareRawTokenString2(vn, offset, len, s);
}

int compareTokens(VTDNav *vn1, int i1, VTDNav *vn2, int i2){
	int t1, t2;
	int ch1, ch2;
	int len1, len2;
	int endOffset1, endOffset2;
	Long l;
	int offset1, offset2;
	/*if (vn2 ==null){
	throw new NavException(" One of VTD objects is null!!");
	}*/

	if ( i1 ==i2 && vn1 == vn2)
		return 0;

	t1 = getTokenType(vn1,i1);
	t2 = getTokenType(vn2,i2);

	offset1 = getTokenOffset(vn1,i1);
	offset2 = getTokenOffset(vn2,i2);

	len1 =
		(t1 == TOKEN_STARTING_TAG
		|| t1 == TOKEN_ATTR_NAME
		|| t1 == TOKEN_ATTR_NS)
		? getTokenLength(vn1,i1) & 0xffff
		: getTokenLength(vn1,i1);

	len2 = 
		(t2 == TOKEN_STARTING_TAG
		|| t2 == TOKEN_ATTR_NAME
		|| t2 == TOKEN_ATTR_NS)
		? getTokenLength(vn2,i2) & 0xffff
		: getTokenLength(vn2,i2);

	endOffset1 = len1 + offset1;
	endOffset2 = len2 + offset2;

	for(;offset1<endOffset1 && offset2< endOffset2;){
		if(t1 == TOKEN_CHARACTER_DATA
			|| t1== TOKEN_ATTR_VAL){
				l = getCharResolved(vn1,offset1);
		} else{ 
			l = getChar(vn1,offset1);
		}
		ch1 = (int)l;
		offset1 += (int)(l>>32);

		if(t2 == TOKEN_CHARACTER_DATA
			|| t2== TOKEN_ATTR_VAL){
				l = getCharResolved(vn2,offset2);
		} else{ 
			l = getChar(vn2,offset2);
		}
		ch2 = (int)l;
		offset2 += (int)(l>>32);
		if (ch1 > ch2)
			return 1;
		if (ch1 < ch2)
			return -1;
	}

	if (offset1 == endOffset1 
		&& offset2 < endOffset2)
		return -1;
	else if (offset1 < endOffset1 
		&& offset2 == endOffset2)
		return 1;
	else
		return 0;
}

/* Write VTD+XML into a FILE pointer */
Boolean _writeIndex_VTDNav(VTDNav *vn, FILE *f){
	return _writeIndex_L3(1, 
                vn->encoding, 
                vn->ns, 
                TRUE, 
				vn->nestingLevel-1, 
                3, 
                vn->rootIndex, 
                vn->XMLDoc, 
                vn->docOffset, 
                vn->docLen, 
				vn->vtdBuffer, 
                vn->l1Buffer, 
                vn->l2Buffer, 
                vn->l3Buffer, 
                f);
}
Boolean _writeIndex2_VTDNav(VTDNav *vn, char *fileName){
	FILE *f = NULL;
	Boolean b = FALSE;
	f = fopen(fileName,"wb");
	
	if (f==NULL){
		throwException2(invalid_argument,"fileName not valid");
		return FALSE;
	}
	b = writeIndex_VTDNav(vn,f);
	fclose(f);
	return b;
}
/* pre-calculate the VTD+XML index size without generating the actual index */
Long getIndexSize2(VTDNav *vn){
		int size;
	    if ( (vn->docLen & 7)==0)
	       size = vn->docLen;
	    else
	       size = ((vn->docLen >>3)+1)<<3;
	    
	    size += (vn->vtdBuffer->size<<3)+
	            (vn->l1Buffer->size<<3)+
	            (vn->l2Buffer->size<<3);
	    
		if ((vn->l3Buffer->size & 1) == 0){ //even
	        size += vn->l3Buffer->size<<2;
	    } else {
	        size += (vn->l3Buffer->size+1)<<2; //odd
	    }
	    return size+64;
}

// Get the element fragment object corresponding to a ns 
// compensated element 
ElementFragmentNs *getElementFragmentNs(VTDNav *vn){

	if (vn->ns == FALSE){  
		throwException2(nav_exception,"getElementFragmentNS can only be called when ns is tured on");
		return NULL;
	}else {
		//fill the fib with integer 
		// first get the list of name space nodes 
		FastIntBuffer *fib = NULL;
		int i = 0;	    
		int count=0;
		int type;

		int* ia = vn->context;
		int d =ia[0]; // -1 for document node, 0 for root element;
		int c = getCurrentIndex2(vn);
		int len = (c == 0 || c == vn->rootIndex )? 0: 
			(getTokenLength(vn,c) & 0xffff); // get the length of qualified node
		int newSz; Long l;
		fib = createFastIntBuffer2(3); // init size 8


		// put the neighboring ATTR_NS nodes into the array
		// and record the total # of them	     

		if (d > 0){ // depth > 0 every node except document and root element
			int k=getCurrentIndex2(vn)+1;
			if (k<vn->vtdSize){				
				while(k<vn->vtdSize) {
					type = getTokenType(vn,k);
					if(type==TOKEN_ATTR_NAME || type==TOKEN_ATTR_NS){
						if (type == TOKEN_ATTR_NS){    
							appendInt(fib,k);
							//System.out.println(" ns name ==>" + toString(k));
						}
						k += 2;
					}
					else
						break;
					//type = getTokenType(vn,k);
				}

			}
			count = fib->size;
			d--; 
			while (d >= 0) {                
				// then search for ns node in the vinicity of the ancestor nodes
				if (d > 0) {
					// starting point
					k = ia[d]+1;
				} else {
					// starting point
					k = vn->rootIndex+1;
				}
				if (k<vn->vtdSize){
					
					while(k<vn->vtdSize){ 
						type = getTokenType(vn,k);
						if (type==TOKEN_ATTR_NAME || type==TOKEN_ATTR_NS){
							Boolean unique = TRUE;
							if (type == TOKEN_ATTR_NS){
								int z = 0;
								for (z=0;z<fib->size;z++){
									//System.out.println("fib size ==> "+fib.size());
									//if (fib->size==4);
									if (matchTokens(vn, intAt(fib,z),vn,k)){
										unique = FALSE;
										break;
									} 

								}            
								if (unique)
									appendInt(fib,k);
							}
						}
							k+=2;
							//type = getTokenType(vn,k);
					}
				}
				d--;
			}
			// System.out.println("count ===> "+count);
			// then restore the name space node by shifting the array
			newSz= fib->size-count;
			for (i= 0; i<newSz; i++ ){
				modifyEntryFIB(fib,i,intAt(fib,i+count));                
			}
			resizeFIB(fib,newSz);
		}

		l= getElementFragment(vn);
		return createElementFragmentNs(vn,l,fib,len);
	}
}

/* dump XML text into a given file name */
void dumpXML(VTDNav *vn, char *fileName){
	FILE *f=fopen(fileName,"w");
	if (f!=NULL){
		dumpXML2(vn,f);
		fclose(f);
	} else {
		throwException2(io_exception,"can't open file");
	}	
}

/* dump XML text into a given file descriptor */
void dumpXML2(VTDNav *vn, FILE *f){	
	size_t i = fwrite(vn->XMLDoc+vn->docOffset,1,vn->docLen,f);
	if (i < (size_t) vn->docLen)
		throwException2(io_exception, "can't complete the write");	
}

/* Get the string length of a token as if it is converted into a normalized UCS string*/
int getNormalizedStringLength(VTDNav *vn, int index){	
	tokenType type = getTokenType(vn,index);
	if (type!=TOKEN_CHARACTER_DATA &&
		type!= TOKEN_ATTR_VAL)
		return getRawStringLength(vn,index); 
	else{
		int len, endOffset, len1=0;
		int ch,k,offset;
		Long l;
		Boolean d;

		len = getTokenLength(vn,index);
		offset = getTokenOffset(vn ,index);
		endOffset = len + offset - 1; // point to the last character

		// trim off the leading whitespaces
		while (TRUE) {
			int temp = offset;
			l = getChar(vn,offset);
			ch = (int)l;
			offset += (UCSChar)(l>>32);

			if (!isWS(ch)) {
				offset = temp;
				break;
			}
		}

		d = FALSE;
		k = 0;
		while (offset <= endOffset) {
			l= getCharResolved(vn,offset);
			ch = (int)l;
			offset += (int)(l>>32);

			if (isWS(ch)&& getCharUnit(vn,offset - 1) != ';') {
				d = TRUE;
			} else {
				if (d == FALSE)
					//sb.append((char) ch); // java only supports 16 bit unicode
					len1++;
				else {
					//sb.append(' ');
					len1 = len1+2;
					d = FALSE;
				}
			}
		}
		return len1;
	}
}

/* getStringLength return the string length of a token as if the token is converted into 
   a string (entity resolved)*/
int getStringLength(VTDNav *vn, int index){
	    tokenType type = getTokenType(vn,index);
        if (type != TOKEN_CHARACTER_DATA && type != TOKEN_ATTR_VAL)
            return getRawStringLength(vn, index);
		else {
			int len = 0, len1 = 0, offset, endOffset;
			Long l;
			len = getTokenLength(vn, index);
			offset = getTokenOffset(vn, index);
			endOffset = offset + len;			

			while (offset < endOffset) {
				l = getCharResolved(vn,offset);
				offset += (int) (l >> 32);
				len1++;
			}
			return len1;
		}
}

/*Get the string length as if the token is converted into a UCS string (entity not resolved) */
int getRawStringLength(VTDNav *vn, int index){
	    tokenType type = getTokenType(vn,index);
        int len = 0, len1 = 0, offset, endOffset;
		Long l;
        if (type == TOKEN_STARTING_TAG || type == TOKEN_ATTR_NAME
                || type == TOKEN_ATTR_NS)
            len = getTokenLength(vn, index) & 0xffff;
        else
            len = getTokenLength(vn, index);
		if (vn->encoding!=FORMAT_UTF8 &&
			vn->encoding!=FORMAT_UTF_16BE &&
        	vn->encoding!=FORMAT_UTF_16LE) {
        	return len;
        }
        offset = getTokenOffset(vn,index);
        endOffset = offset + len;
        
        while (offset < endOffset) {
            l = getChar(vn, offset);
            offset += (int) (l >> 32);
            len1++;
        }
        return len1;
}

/* Get the offset value right after head (e.g. <a b='b' c='c'> ) */
Long getOffsetAfterHead(VTDNav *vn){
	    int i = getCurrentIndex(vn),j,offset;
		//encoding_t enc;
 	    if (getTokenType(vn,i)!= TOKEN_STARTING_TAG){
	        return -1;
 	    }
	    j=i+1;
 	    while (j<vn->vtdSize && (getTokenType(vn,j)==TOKEN_ATTR_NAME
	            || getTokenType(vn,j)== TOKEN_ATTR_NS)){
	        j += 2;
 	    }

		if (i+1==j)
	    {
			offset = getTokenOffset(vn,i)+((vn->ns==FALSE)?getTokenLength(vn,i):(getTokenLength(vn,i)&0xff));
 	    }else {
			offset = getTokenOffset(vn,j-1)+((vn->ns==FALSE)?getTokenLength(vn,j-1):(getTokenLength(vn,j-1)&0xff))+1;
	    }

 	    while(getCharUnit(vn,offset)!='>'){
	        offset++;
	    }
 
	    if (getCharUnit(vn,offset-1)=='/')
 	        return (-1LL<<32)|offset;
	    else
 	        return  offset+1;

}


/* Write the VTDs and LCs into an Separate index file*/
Boolean _writeSeparateIndex_VTDNav(VTDNav *vn, char *VTDIndexFile){
	FILE *f = NULL;
	Boolean b = FALSE;
	f = fopen(VTDIndexFile,"wb");
	
	if (f==NULL){
		throwException2(invalid_argument,"fileName not valid");
		//return FALSE;
	}

	b=_writeSeparateIndex_L3( (Byte)2, 
                vn->encoding, 
                vn->ns, 
                TRUE, 
				vn->nestingLevel-1, 
                3, 
                vn->rootIndex, 
                vn->docOffset, 
                vn->docLen, 
				vn->vtdBuffer, 
                vn->l1Buffer, 
                vn->l2Buffer, 
                vn->l3Buffer, 
                f);
	
	fclose(f);
	return b;
}


/* Test the start of token content at index i matches the content 
of s, notice that this is to save the string allocation cost of
using String's built-in startsWidth */
Boolean startsWith(VTDNav *vn, int index, UCSChar *s){
	tokenType type = getTokenType(vn, index);
	int len =
		(type == TOKEN_STARTING_TAG
		|| type == TOKEN_ATTR_NAME
		|| type == TOKEN_ATTR_NS)
		? getTokenLength(vn,index) & 0xffff
		: getTokenLength(vn,index);
	int offset = getTokenOffset(vn, index);
	Long l1; 
	size_t l,i;
	int i1;
	int endOffset = offset + len;
	Boolean b = (type == TOKEN_CHARACTER_DATA 
		|| type == TOKEN_ATTR_VAL);
	//       System.out.print("currentOffset :" + currentOffset);
	l = wcslen(s);
	if (l> (size_t)len)
		return FALSE;
	//System.out.println(s);
	for (i = 0; i < l && offset < endOffset; i++) {
		if (b)
			l1 = getCharResolved(vn, offset);
		else
			l1 = getChar(vn, offset);
		i1 = s[i];
		if (i1 != (int) l1)
			return FALSE;
		offset += (int) (l1 >> 32);
	}	    
	return TRUE;

}

/*Test the end of token content at index i matches the content 
of s, notice that this is to save the string allocation cost of
using String's built-in endsWidth */
Boolean endsWith(VTDNav *vn, int index, UCSChar *s){
	tokenType type = getTokenType(vn,index);
	size_t len =
		(type == TOKEN_STARTING_TAG
		|| type == TOKEN_ATTR_NAME
		|| type == TOKEN_ATTR_NS)
		? (size_t)getTokenLength(vn,index) & 0xffff
		: (size_t)getTokenLength(vn,index);
	int offset = getTokenOffset(vn, index);
	Long l1; 
	size_t l,i,i2;
	int i1;
	Boolean b = (type == TOKEN_CHARACTER_DATA 
		|| type == TOKEN_ATTR_VAL);
	//int endOffset = offset + len;

	//       System.out.print("currentOffset :" + currentOffset);
	l = wcslen(s);
	if (l>len)
		return FALSE;
	i2 = (size_t)getStringLength(vn,index);
	if (l> i2)
		return FALSE;
	i2 = i2 - l; // calculate the # of chars to be skipped
	// eat away first several chars
	for (i = 0; i < i2; i++) {
		if (b)
			l1 = getCharResolved(vn,offset);
		else 
			l1 = getChar(vn,offset);
		offset += (int) (l1 >> 32);
	}
	//System.out.println(s);
	for (i = 0; i < l; i++) {
		if (b)
			l1 = getCharResolved(vn,offset);
		else
			l1 = getChar(vn,offset);
		i1 = s[i];
		if (i1 != (int) l1)
			return FALSE;
		offset += (int) (l1 >> 32);
	}	    
	return TRUE;
}

/*Test whether a given token contains s. notie that this function
directly operates on the byte content of the token to avoid string creation */

Boolean contains(VTDNav *vn, int index, UCSChar *s){
	tokenType type = getTokenType(vn,index);
	size_t len =
		(type == TOKEN_STARTING_TAG
		|| type == TOKEN_ATTR_NAME
		|| type == TOKEN_ATTR_NS)
		? getTokenLength(vn,index) & 0xffff
		: getTokenLength(vn,index);
	int offset = getTokenOffset(vn,index);
	Long l1;
	size_t l,i;
	int i1;
	int endOffset = offset + len;

	Boolean b = (type == TOKEN_CHARACTER_DATA 
		|| type == TOKEN_ATTR_VAL);
	//       System.out.print("currentOffset :" + currentOffset);
	int gOffset = offset;
	l = wcslen(s);
	if (l> len)
		return FALSE;
	//System.out.println(s);
	while( offset<endOffset){
		gOffset = offset;
		for (i = 0; i < l && gOffset < endOffset; i++) {
			if (b)
				l1 = getCharResolved(vn,gOffset);
			else
				l1 = getChar(vn,gOffset);
			i1 = s[i];
			gOffset += (int) (l1 >> 32);
			if (i ==0)
				offset = gOffset;
			if (i1 != (int) l1)
				break;				
		}
		if (i==l)
			return TRUE;
	}
	return FALSE;
}


UCSChar *toStringUpperCase2(VTDNav *vn, int os, int len){
	int offset = os, endOffset=os+len,k=0;
	Long l;
	UCSChar *s = (UCSChar *)malloc(sizeof(UCSChar)*(len+1));
	if (s == NULL)
	{
		throwException2(out_of_mem,
			" string allocation failed in toString ");
	}
	while (offset < endOffset) {
		l = getCharResolved(vn,offset);
		offset += (int)(l>>32);
		if ((int)l>96 && (int)l<123)
			s[k++] = (UCSChar)(l-32); // java only support 16 bit unit code
		else 
			s[k++] = (UCSChar)l;
	}
	s[k] = 0;
	return s;
}

UCSChar *toStringLowerCase2(VTDNav *vn, int os, int len){
	int offset = os, endOffset=os+len,k=0;
	Long l;
	UCSChar *s = (UCSChar *)malloc(sizeof(UCSChar)*(len+1));
	if (s == NULL)
	{
		throwException2(out_of_mem,
			" string allocation failed in toString ");
	}
	while (offset < endOffset) {
		l = getCharResolved(vn,offset);
		offset += (int)(l>>32);
		if ((int)l>64 && (int)l<91)
			s[k++] = (UCSChar)(l+32); // java only support 16 bit unit code
		else 
			s[k++] = (UCSChar)l; // java only support 16 bit unit code
	}
	s[k] = 0;
	return s;
}

/* Convert a token at the given index to a String and any lower case
   character will be converted to upper case, (entities and char
   references resolved).*/
UCSChar *toStringUpperCase(VTDNav *vn, int index){
	int offset,len;
	tokenType type = getTokenType(vn,index);

	/*UCSChar *s = NULL;*/
	if (type!=TOKEN_CHARACTER_DATA &&
		type!= TOKEN_ATTR_VAL)
		return toRawStringUpperCase(vn,index);

	len = getTokenLength(vn,index);

	offset = getTokenOffset(vn,index);
	return toStringUpperCase2(vn,offset,len);
}

/* Convert a token at the given index to a String and any upper case
   character will be converted to lower case, (entities and char
   references resolved).*/
UCSChar *toStringLowerCase(VTDNav *vn, int index){
	int offset,len;
	tokenType type = getTokenType(vn,index);

	/*UCSChar *s = NULL;*/
	if (type!=TOKEN_CHARACTER_DATA &&
		type!= TOKEN_ATTR_VAL)
		return toRawStringLowerCase(vn,index);

	len = getTokenLength(vn,index);

	offset = getTokenOffset(vn,index);
	return toStringLowerCase2(vn,offset,len);
}

UCSChar *toRawStringUpperCase(VTDNav *vn, int index){
	int offset,len;
	tokenType type = getTokenType(vn,index);					 
	/*UCSChar *s = NULL;*/

	if (type == TOKEN_STARTING_TAG
		|| type == TOKEN_ATTR_NAME
		|| type == TOKEN_ATTR_NS)
		len = getTokenLength(vn,index) & 0xffff;
	else
		len = getTokenLength(vn,index);

	offset = getTokenOffset(vn,index);
	return toRawStringUpperCase2(vn, offset, len);
}
UCSChar *toRawStringLowerCase(VTDNav *vn, int index){
	int offset,len;
	tokenType type = getTokenType(vn,index);						 
	/*UCSChar *s = NULL;*/

	if (type == TOKEN_STARTING_TAG
		|| type == TOKEN_ATTR_NAME
		|| type == TOKEN_ATTR_NS)
		len = getTokenLength(vn,index) & 0xffff;
	else
		len = getTokenLength(vn,index);

	offset = getTokenOffset(vn,index);
	return toRawStringLowerCase2(vn, offset, len);
}
static UCSChar *toRawStringUpperCase2(VTDNav *vn, int os, int len){
	int offset = os, endOffset=os+len,k=0;
	Long l;
	UCSChar *s = (UCSChar *)malloc(sizeof(UCSChar)*(len+1));
	if (s == NULL)
	{
		throwException2(out_of_mem,
			" string allocation failed in toString ");
	}
	while (offset < endOffset) {
		l = getChar(vn,offset);
		offset += (int)(l>>32);
		if ((int)l>96 && (int)l<123)
			s[k++] = (UCSChar)(l-32); // java only support 16 bit unit code
		else 
			s[k++] = (UCSChar)l;
	}
	s[k] = 0;
	return s;
}

static UCSChar *toRawStringLowerCase2(VTDNav *vn, int os, int len){
	int offset = os, endOffset=os+len,k=0;
	Long l;
	UCSChar *s = (UCSChar *)malloc(sizeof(UCSChar)*(len+1));
	if (s == NULL)
	{
		throwException2(out_of_mem,
			" string allocation failed in toString ");
	}
	while (offset < endOffset) {
		l = getChar(vn,offset);
		offset += (int)(l>>32);
		if ((int)l>64 && (int)l<91)
			s[k++] = (UCSChar)(l+32); // java only support 16 bit unit code
		else 
			s[k++] = (UCSChar)l; // java only support 16 bit unit code
	}
	s[k] = 0;
	return s;
}

static void resolveLC_l1(VTDNav *vn){
		int k;
		if (vn->l1index < 0
		|| vn->l1index >= vn->l1Buffer->size
		|| vn->context[1] != upper32At(vn->l1Buffer, vn->l1index)) {
			if (vn->l1index >= vn->l1Buffer->size || vn->l1index <0) {
				vn->l1index = 0;
			}
			if (vn->l1index+1< vn->l1Buffer->size 
				&& vn->context[1] != upper32At(vn->l1Buffer,vn->l1index + 1)) {
					int init_guess;
					k = vn->context[1];
					init_guess =
						(int) (vn->l1Buffer->size
						* ((float) /*vn->context[1]*/k / vn->vtdBuffer->size));
					if (upper32At(vn->l1Buffer,init_guess) > k /*vn->context[1]*/) {
						while (upper32At(vn->l1Buffer,init_guess)
							!= k /*vn->context[1]*/) {
								init_guess--;
						}
					} else if (
						upper32At(vn->l1Buffer,init_guess) < k /*vn->context[1]*/) {
							while (upper32At(vn->l1Buffer,init_guess)
								!= k /*vn->context[1]*/) {
									init_guess++;
							}
					}
					vn->l1index = init_guess;
			} else{
				if (vn->context[1]>=upper32At(vn->l1Buffer,vn->l1index)){
					while(vn->context[1]!=upper32At(vn->l1Buffer,vn->l1index)
						&& vn->l1index<vn->l1Buffer->size){
							vn->l1index++;								
					}
				} else {
					while(vn->context[1]!=upper32At(vn->l1Buffer,vn->l1index)
						&& vn->l1index >= 0){
							vn->l1index--;								
					}
				}

			}
			// for iterations, l1index+1 is the logical next value for l1index
	}
}
static void resolveLC_l2(VTDNav *vn){
	int temp = lower32At(vn->l1Buffer,vn->l1index),i,k;
	if (vn->l2lower != temp) {
		vn->l2lower = temp;
		// l2lower shouldn't be -1 !!!!  l2lower and l2upper always get resolved simultaneously
		vn->l2index = vn->l2lower;
		vn->l2upper = vn->l2Buffer->size - 1;
		k = vn->l1Buffer->size;
		for (i = vn->l1index + 1; i < k; i++) {
			temp = lower32At(vn->l1Buffer,i);
			if (temp != -1) {
				vn->l2upper = temp - 1;
				break;
			}
		}
	} // intelligent guess again ??

	if (vn->l2index < 0
		|| vn->l2index >= vn->l2Buffer->size
		|| vn->context[2] != upper32At(vn->l2Buffer,vn->l2index)) {
			if (vn->l2index >= vn->l2Buffer->size || vn->l2index<0)
				vn->l2index = vn->l2lower;
			if (vn->l2index+1< vn->l2Buffer->size 
				&& vn->context[2] == upper32At(vn->l2Buffer,vn->l2index + 1))
				vn->l2index = vn->l2index + 1;
			else if (vn->l2upper - vn->l2lower >= 16) {
				int init_guess =
					vn->l2lower
					+ (int) ((vn->l2upper - vn->l2lower)
					* ((float) vn->context[2]
				- upper32At(vn->l2Buffer,vn->l2lower))
					/ (upper32At(vn->l2Buffer,vn->l2upper)
					- upper32At(vn->l2Buffer,vn->l2lower)));
				if (upper32At(vn->l2Buffer,init_guess) > vn->context[2]) {
					while (vn->context[2]
					!= upper32At(vn->l2Buffer,init_guess))
						init_guess--;
				} else if (
					upper32At(vn->l2Buffer,init_guess) < vn->context[2]) {
						while (vn->context[2]
						!= upper32At(vn->l2Buffer,init_guess))
							init_guess++;
				}
				vn->l2index = init_guess;
			} else if (vn->context[2]<upper32At(vn->l2Buffer,vn->l2index)){
				while (vn->context[2] != upper32At(vn->l2Buffer,vn->l2index)) {
					vn->l2index--;
				}
			}
			else {
				while (vn->context[2] != upper32At(vn->l2Buffer,vn->l2index)) {
					vn->l2index++;
				}
			}
	}
}

static void resolveLC_l3(VTDNav *vn){
	int i,k,temp = lower32At(vn->l2Buffer,vn->l2index);
	k = vn->l2Buffer->size;
	if (vn->l3lower != temp) {
		//l3lower and l3upper are always together
		vn->l3lower = temp;
		// l3lower shouldn't be -1
		vn->l3index = vn->l3lower;
		vn->l3upper = vn->l3Buffer->size - 1;
		for (i = vn->l2index + 1; i < k; i++) {
			temp = lower32At(vn->l2Buffer,i);
			if (temp != -1) {
				vn->l3upper = temp - 1;
				break;
			}
		}
	}

	if (vn->l3index < 0
		|| vn->l3index >= vn->l3Buffer->size
		|| vn->context[3] != intAt(vn->l3Buffer,vn->l3index)) {
			if (vn->l3index >= vn->l3Buffer->size || vn->l3index<0)
				vn->l3index = vn->l3lower;
			if (vn->l3index+1 < vn->l3Buffer->size  
				&& vn->context[3] == intAt(vn->l3Buffer,vn->l3index + 1))
				vn->l3index = vn->l3index + 1;
			else if (vn->l3upper - vn->l3lower >= 16) {
				int init_guess =
					vn->l3lower
					+ (int) ((vn->l3upper - vn->l3lower)
					* ((float) (vn->context[3]
				- intAt(vn->l3Buffer,vn->l3lower))
					/ (intAt(vn->l3Buffer,vn->l3upper)
					- intAt(vn->l3Buffer,vn->l3lower))));
				if (intAt(vn->l3Buffer,init_guess) > vn->context[3]) {
					while (vn->context[3] != intAt(vn->l3Buffer, init_guess))
						init_guess--;
				} else if (intAt(vn->l3Buffer,init_guess) < vn->context[3]) {
					while (vn->context[3] != intAt(vn->l3Buffer,init_guess))
						init_guess++;
				}
				vn->l3index = init_guess;
			} else if (vn->context[3] < intAt(vn->l3Buffer, vn->l3index)){
				while (vn->context[3] != intAt(vn->l3Buffer,vn->l3index)) {
					vn->l3index--;
				}
			} else {
				while (vn->context[3] != intAt(vn->l3Buffer,vn->l3index)) {
					vn->l3index++;
				}
			}
	}
}


/* DupliateNav duplicates an instance of VTDNav but doesn't retain the original node position*/
VTDNav* _duplicateNav(VTDNav *vn){
	VTDNav* vn1 = createVTDNav( vn->rootIndex,
		vn->encoding,
		vn->ns,
		vn->nestingLevel-1,
		vn->XMLDoc,
		vn->bufLen,
		vn->vtdBuffer,
		vn->l1Buffer,
		vn->l2Buffer,
		vn->l3Buffer,
		vn->docOffset,
		vn->docLen,
		vn->br);	
	vn1->master = FALSE;
	return vn1;
}


/* ClineNav duplicates an instance of VTDNav, also copies node position over */
VTDNav *_cloneNav(VTDNav *vn){
	VTDNav* vn1 = createVTDNav( vn->rootIndex,
		vn->encoding,
		vn->ns,
		vn->nestingLevel-1,
		vn->XMLDoc,
		vn->bufLen,
		vn->vtdBuffer,
		vn->l1Buffer,
		vn->l2Buffer,
		vn->l3Buffer,
		vn->docOffset,
		vn->docLen,
		vn->br);	
	vn1->master = FALSE;
	vn1->atTerminal = vn->atTerminal;
	vn1->LN = vn->LN;
	if (vn->context[0]!=-1)
		memcpy(vn1->context,vn->context, vn->context[0]);
		//System.arraycopy(vn.context, 0, vn.context, 0, this.context[0] );
	else 
		vn1->context[0]=-1;
	vn1->l1index = vn->l1index; 
	if (getCurrentDepth(vn)>1){
		vn1->l2index = vn->l2index;
		vn1->l2upper = vn->l2upper;
		vn1->l2lower = vn->l2lower;
	}
	if (getCurrentDepth(vn) > 2) {
		vn1->l3lower = vn->l3lower;
		vn1->l3index = vn->l3index;
		vn1->l3upper = vn->l3upper;
	}
	return vn1;
}

/* This method takes a vtd index, and recover its correspondin
 * node position, the index can only be of node type element,
 * document, attribute name, attribute value or character data,
 * or CDATA  */
void _recoverNode(VTDNav *vn, int index){
	int d, type,t;
	//exception e;
	if (index <0 || index>=vn->vtdSize ){
		throwException2(invalid_argument,"Invalid VTD index");
		//throw new NavException("Invalid VTD index");
	}

	type = getTokenType(vn,index);

	if (//type == VTDNav.TOKEN_COMMENT ||
		//	type == VTDNav.TOKEN_PI_NAME ||
		type == TOKEN_PI_VAL ||
		type == TOKEN_DEC_ATTR_NAME ||
		type == TOKEN_DEC_ATTR_VAL ||
		type == TOKEN_ATTR_VAL){
			throwException2( nav_exception, "Invalid VTD index");
			//throw new NavException("Token type not yet supported");
	}
	// get depth
	d = getTokenDepth(vn,index);
	// handle document node;	
	switch (d){
		case -1:
			vn->context[0]=-1;
			if (index != 0){
				vn->LN = index;
				vn->atTerminal = TRUE;
			}else
				vn->atTerminal = FALSE;
			return;
		case 0:
			vn->context[0]=0;
			if (index != vn->rootIndex){
				vn->LN = index;
				vn->atTerminal = TRUE;
				if (type> TOKEN_ATTR_NS)
					sync(vn,0,index);
			}else
				vn->atTerminal = FALSE;
			return;		
	}
	vn->context[0]=d;
	if (type != TOKEN_STARTING_TAG){
		vn->LN = index;
		vn->atTerminal = TRUE;
	}else
		vn->atTerminal = FALSE;
	// search LC level 1
	recoverNode_l1(vn,index);

	if (d==1){
		if (vn->atTerminal && type>TOKEN_ATTR_NS)
			 sync(vn,1,index);
		return;
	}
	// search LC level 2
	recoverNode_l2(vn,index);
	if (d==2){
		//resolveLC();
		if (vn->atTerminal && type>TOKEN_ATTR_NS)
				 sync(vn,2,index);
		return;
	}
	// search LC level 3
	recoverNode_l3(vn,index);
	if (d==3){
		//resolveLC();
		if (vn->atTerminal && type>TOKEN_ATTR_NS)
				 sync(vn,3,index);
		return;
	}
	// scan backward
	if ( type == TOKEN_STARTING_TAG ){
		vn->context[d] = index;
	} else{
		int t = index-1;
		while( !(getTokenType(vn,t)==TOKEN_STARTING_TAG && 
			getTokenDepth(vn,t)==d)){
				t--;
		}
		vn->context[d] = t;
	}
	t = vn->context[d]-1;
	d--;
	while(d>3){
		while( !(getTokenType(vn,t)==TOKEN_STARTING_TAG && 
			getTokenDepth(vn,t)==d)){
				t--;
		}
		vn->context[d] = t;
		d--;
	}
}

static void recoverNode_l1(VTDNav *vn,int index){
	int i;
	if(vn->context[1]==index){

	}
	else if (vn->l1index!=-1 && vn->context[1]>index 
		&& vn->l1index+1<vn->l1Buffer->size
		&& upper32At(vn->l1Buffer,vn->l1index+1) < index){

	}
	else {
		i= (index/vn->vtdSize)*vn->l1Buffer->size;
		if (i>=vn->l1Buffer->size)
			i=vn->l1Buffer->size-1;

		if (upper32At(vn->l1Buffer,i)< index) {
			while(i<vn->l1Buffer->size-1 && 
				upper32At(vn->l1Buffer,i)<index){
					i++;
			}
			if (upper32At(vn->l1Buffer,i)>index)
				i--;
		} else {
			while(upper32At(vn->l1Buffer,i)>index){
				i--;
			}
		}	
		vn->context[1] = upper32At(vn->l1Buffer,i);
		vn->l1index = i;
	}
}

static void recoverNode_l2(VTDNav *vn,int index){
	int i = lower32At(vn->l1Buffer,vn->l1index),k,t1,t2;

	if (vn->l2lower != i) {
		vn->l2lower = i;
		// l2lower shouldn't be -1 !!!! l2lower and l2upper always get
		// resolved simultaneously
		//l2index = l2lower;
		vn->l2upper = vn->l2Buffer->size - 1;
		for (k = vn->l1index + 1; k < vn->l1Buffer->size; k++) {
			i = lower32At(vn->l1Buffer,k);
			if (i != 0xffffffff) {
				vn->l2upper = i - 1;
				break;
			}
		}
	}
	// guess what i would be in l2 cache
	t1=upper32At(vn->l2Buffer, vn->l2lower);
	t2=upper32At(vn->l2Buffer, vn->l2upper);
	//System.out.print("   t2  ==>"+t2+"   t1  ==>"+t1);
	i= min(vn->l2lower+ 
		(int)(((float)(index-t1)/(t2-t1+1))*(vn->l2upper-vn->l2lower))
		,vn->l2upper) ;
	//System.out.print("  i1  "+i);
	while(i<vn->l2Buffer->size-1 && upper32At(vn->l2Buffer,i)<index){
		i++;	
	}
	//System.out.println(" ==== i2    "+i+"    index  ==>  "+index);

	while (upper32At(vn->l2Buffer,i)>index && i>0)
		i--;
	vn->context[2] = upper32At(vn->l2Buffer,i);
	vn->l2index = i;
}

static void recoverNode_l3(VTDNav *vn,int index){
	int i = lower32At(vn->l2Buffer,vn->l2index),k,t1,t2;
		
		if (vn->l3lower != i) {
			//l3lower and l3upper are always together
			vn->l3lower = i;
			// l3lower shouldn't be -1
			//l3index = l3lower;
			vn->l3upper = vn->l3Buffer->size - 1;
			for (k = vn->l2index + 1; k < vn->l2Buffer->size; k++) {
				i = lower32At(vn->l2Buffer,k);
				if (i != 0xffffffff) {
					vn->l3upper = i - 1;
					break;
				}
			}
		}
		t1=intAt(vn->l3Buffer,vn->l3lower);
		t2=intAt(vn->l3Buffer,vn->l3upper);

		i= min(vn->l3lower+ (int)(((float)(index-t1)/(t2-t1+1))*(vn->l3upper-vn->l3lower)),vn->l3upper) ;
		while(i<vn->l3Buffer->size-1 && intAt(vn->l3Buffer,i)<index){
			i++;	
		}
		while (intAt(vn->l3Buffer,i)>index && i>0)
			i--;
		//System.out.println(" i ===> "+i);
		vn->context[3] = intAt(vn->l3Buffer,i);
		vn->l3index = i;
}

/**
	 * Match the string against the token at the given index value. The token will be
     * interpreted as if it is normalized (i.e. all white space char (\r\n\a ) is replaced
     * by a white space, char entities and entity references will be replaced by their correspondin
     * char see xml 1.0 spec interpretation of attribute value normalization) */

Boolean matchNormalizedTokenString2(VTDNav *vn,int index, UCSChar *s){
		tokenType type = getTokenType(vn,index);
		int len =
			(type == TOKEN_STARTING_TAG
				|| type == TOKEN_ATTR_NAME
				|| type == TOKEN_ATTR_NS)
				? getTokenLength(vn,index) & 0xffff
				: getTokenLength(vn,index);

		return compareNormalizedTokenString2(vn,getTokenOffset(vn,index), len, s)==0;
}

int compareNormalizedTokenString2(VTDNav *vn,int offset, int len,
			UCSChar *s) {
		int i,i1, l, temp;
		Long l1,l2;
		//boolean b = FALSE;
		// this.currentOffset = offset;
		int endOffset = offset + len;

		// System.out.print("currentOffset :" + currentOffset);
		l = wcslen(s);
		// System.out.println(s);
		for (i = 0; i < l && offset < endOffset;) {
			l1 = getCharResolved(vn,offset);
			temp = (int) l1;
			l2 = (l1>>32);
			if (l2<=2 && isWS(temp))
				temp = ' ';
			i1 = s[i];
			if (i1 < temp)
				return 1;
			if (i1 > temp)
				return -1;
			i++;			
			offset += (int) (l1 >> 32);
		}

		if (i == l && offset < endOffset)
			return 1;
		if (i < l && offset == endOffset)
			return -1;
		return 0;
		// return -1;
	}

/**
	 * Return the byte offset and length of up to i sibling fragments. If 
	 * there is a i+1 sibling element, the cursor element would 
	 * move to it; otherwise, there is no cursor movement. If the cursor isn't 
	 * positioned at an element (due to XPath evaluation), then -1 will be 
	 * returned
	 * @param i number of silbing elements including the cursor element
	 * @return a long encoding byte offset (bit 31 to bit 0), length (bit 62 
	 * to bit 32) of those fragments 
	 * @throws NavException
	 */

Long getSiblingElementFragments(VTDNav *vn,int i){
	int so, len;
	Long l;
	BookMark *bm;
		if (i<=0)
			throwException2(invalid_argument," # of sibling can be less or equal to 0");
		// get starting char offset
		if(vn->atTerminal==TRUE)
			return -1L;
		// so is the char offset
		so = getTokenOffset(vn,getCurrentIndex(vn))-1;
		// char offset to byte offset conversion
		if (vn->encoding>=FORMAT_UTF_16BE)
			so = so<<1;
		bm = createBookMark(vn);
		recordCursorPosition(bm,vn);
		while(i>1 && toElement(vn,NEXT_SIBLING)){
			i--;
		}
		l= getElementFragment(vn);
		len = (int)l+(int)(l>>32)-so;
		if (i==1 && toElement(vn,NEXT_SIBLING)){
		}else
			setCursorPosition(bm,vn);
		freeBookMark(bm);
		return (((Long)len)<<32)|so;
}

/**
	 * Match the string against the token at the given index value. The token will be
     * interpreted as if it is normalized (i.e. all white space char (\r\n\a ) is replaced
     * by a white space, char entities and entity references will be replaced by their correspondin
     * char see xml 1.0 spec interpretation of attribute value normalization) 
	 */
//Boolean matchNormalizedTokenString2(VTDNav *vn, int index, UCSChar *s){

//}

/**
	 * (New since version 2.9)
	 * Shallow Normalization follows the rules below to normalize a token into
	 * a string
	 * *#xD#xA gets converted to #xA
	 * *For a character reference, append the referenced character to the normalized value.
	 * *For an entity reference, recursively apply step 3 of this algorithm to the replacement text of the entity.
	 * *For a white space character (#x20, #xD, #xA, #x9), append a space character (#x20) to the normalized value.
	 * *For another character, append the character to the normalized value.*/
UCSChar* toNormalizedString2(VTDNav *vn, int index){
		tokenType type = getTokenType(vn,index);
		Long l;
		UCSChar *s;int ch,k;
		int len,offset, endOffset;
		if (type!=TOKEN_CHARACTER_DATA &&
				type!= TOKEN_ATTR_VAL)
			return toRawString(vn,index); 
		
		len = getTokenLength(vn,index);
		if (len == 0)
			return wcsdup(L"");
		offset = getTokenOffset(vn,index);
		endOffset = len + offset - 1; // point to the last character

		s = (UCSChar *)malloc(sizeof(UCSChar)*(len+1));
		if (s == NULL)
		{
			throwException2(out_of_mem,
				" string allocation failed in toNormalizedString2 ");
		}
		
		
		

		//boolean d = FALSE;
		while (offset <= endOffset) {
			l = getCharResolved(vn,offset);
			ch = (int)l;
			offset += (int)(l>>32);
			if (isWS(ch) && (l>>32)<=2) {
				//d = TRUE;
				s[k++] = (UCSChar)' ';
				//sb.append(' ');
			} else{
				s[k++] = (UCSChar)ch;
				//sb.append((char) ch);
			}
		}
		return s;
}


UCSChar *getPrefixString(VTDNav *vn, int i){
	if (vn->ns == FALSE)
			return NULL;
	{
		tokenType type = getTokenType(vn,i);
		if (type != TOKEN_ATTR_NAME && type != TOKEN_STARTING_TAG)
			return NULL;
		else {
			int offset = getTokenOffset(vn,i);
			int preLen = getTokenLength(vn,i) >> 16;
			if (preLen !=0)
				return toRawString2(vn,offset,preLen);
		}
		return NULL;
	}
}

void _resolveLC_L5(VTDNav_L5 *vn){
		if(vn->context[0]<=0)
			return;
		resolveLC_l1((VTDNav *)vn);
		if (vn->context[0] == 1)
			return;
		resolveLC_l2((VTDNav *)vn);
		if (vn->context[0] == 2)
			return;	
		_resolveLC_l3(vn);
		if (vn->context[0] == 3)
			return;	
		resolveLC_l4(vn);
		if (vn->context[0] == 4)
			return;	
		resolveLC_l5(vn);
}

void _freeVTDNav_L5(VTDNav_L5 *vn){
	if (vn!=NULL){
	freeContextBuffer (vn->contextBuf);
	freeContextBuffer(vn->contextBuf2);
	if (vn->br == FALSE && vn->master){
		freeFastLongBuffer (vn->vtdBuffer);
		freeFastLongBuffer(vn->l1Buffer);
		freeFastLongBuffer(vn->l2Buffer);
		freeFastLongBuffer(vn->_l3Buffer);
		freeFastLongBuffer(vn->l4Buffer);
		freeFastIntBuffer(vn->l5Buffer);
	}
	free( vn->h1);
	vn->h1=NULL;
	free( vn->h2);
	vn->h2=NULL;
	free( vn->context);
	free( vn->stackTemp);
	if (vn->name!= NULL)
		free(vn->name);
	if (vn->localName != NULL)
		free(vn->localName);
	if (vn->URIName != NULL)
		free(vn->URIName);
	if (vn->currentNode != NULL)
		freeBookMark( vn->currentNode);
	freeFastIntBuffer(vn->fib);
	free(vn);
	}
}

VTDNav_L5 *createVTDNav_L5(int r, encoding_t enc, Boolean ns, int depth,
						   UByte *x, int xLen, FastLongBuffer *vtd, FastLongBuffer *l1,
						   FastLongBuffer *l2, FastLongBuffer *l3, FastLongBuffer *l4, 
						   FastIntBuffer *l5, int so, int len,Boolean br){

							   VTDNav_L5* vn = NULL;
							   int i;
							   exception e;

							   if (l1 == NULL ||
								   l2 == NULL ||
								   l3 == NULL ||
								   l4 == NULL ||
								   l5 == NULL ||
								   vtd == NULL||
								   x == NULL ||
								   so<0 ||
								   len < 0 ||
								   xLen < 0 || // size of x
								   r < 0 ||
								   depth < 0 ||
								   (enc <FORMAT_ASCII || 
								   enc>FORMAT_UTF_16LE) 
								   )
							   {
								   throwException2(invalid_argument,
									   "Invalid argument when creating VTDGen failed ");
								   return NULL;
							   }

							   vn = (VTDNav_L5 *) malloc(sizeof(VTDNav_L5));
							   if (vn==NULL){
								   throwException2(out_of_mem,							 
									   "VTDNav allocation failed ");
								   return NULL;
							   }
							   //temp = (int *)malloc(4*16);
							   vn->__freeVTDNav=(freeVTDNav_) _freeVTDNav_L5;
							   vn->__cloneNav=(cloneNav_)_cloneNav_L5;
							   vn->__duplicateNav =(duplicateNav_) _duplicateNav_L5;
							   vn->__verifyNodeCorrectness = (verifyNodeCorrectness_) _verifyNodeCorrectness_L5;
							   /*vn->__iterate = (iterate_)_iterate;
							   vn->__iterate_following = (iterate_following_)_iterate_following;
							   vn->__iterate_followingNS = (iterate_followingNS_)_iterate_followingNS;
							   vn->__iterate_preceding = (iterate_preceding_)_iterate_preceding;
							   vn->__iterate_precedingNS = (iterate_precedingNS_)_iterate_precedingNS;
							   vn->__iterateNS =(iterateNS_) _iterateNS;*/
							   vn->__sync = (sync_)_sync_L5;		
							   vn->__toNode = (toNode_)_toNode_L5;
							   vn->__pop = (pop_)_pop_L5;
							   vn->__pop2 = (pop2_)_pop2_L5;
							   vn->__push = (push_)_push_L5;
							   vn->__push2 = (push2_)_push2_L5;
							   vn->__recoverNode = (recoverNode_)_recoverNode_L5;
							   vn->__resolveLC = (resolveLC_)_resolveLC_L5;
							   vn->__sampleState = (sampleState_)_sampleState_L5;
							   vn->__toElement = (toElement_)_toElement_L5;
							   vn->__toElement2 = (toElement2_)_toElement2_L5;
							   vn->__toElementNS = (toElementNS_)_toElementNS_L5;
							   vn->__writeIndex_VTDNav = (writeIndex_VTDNav_)_writeIndex_VTDNav_L5;
							   vn->__writeIndex2_VTDNav = (writeIndex2_VTDNav_)_writeIndex2_VTDNav_L5;							
							   vn->__writeSeparateIndex_VTDNav = (writeSeparateIndex_VTDNav_)_writeSeparateIndex_VTDNav_L5;
							   vn->count = 0;
							   vn->l1Buffer = l1;
							   vn->l2Buffer = l2;
							   vn->_l3Buffer = l3;
							   vn->l4Buffer = l4;
							   vn->l5Buffer = l5;
							   vn->vtdBuffer= vtd;
							   vn->XMLDoc = x;

							   vn->encoding = enc;
							   vn->rootIndex = r;
							   vn->nestingLevel = depth +1;

							   vn->ns = ns;

							   if (ns == TRUE)
								   vn->offsetMask = MASK_TOKEN_OFFSET1;
							   else 
								   vn->offsetMask = MASK_TOKEN_OFFSET2;

							   vn->atTerminal = FALSE;					
							   vn->context = (int *)malloc(vn->nestingLevel*sizeof(int));
							   if (vn->context == NULL){
								   throwException2(out_of_mem,							 
									   "VTDNav allocation failed ");
								   return NULL;
							   }
							   vn->context[0] = 0;
							   for (i=1;i<vn->nestingLevel;i++){
								   vn->context[i] = -1;
							   }
							   //vn->currentOffset = 0;
							   Try{
								   vn->contextBuf = createContextBuffer2(10, vn->nestingLevel+15);
								   vn->contextBuf2 = createContextBuffer2(10,vn->nestingLevel+15);
							   }Catch(e){
								   freeContextBuffer(vn->contextBuf);
								   freeContextBuffer(vn->contextBuf2);
								   //free(vn->stackTemp);
								   free(vn->context);
								   free(vn);
								   throwException2(out_of_mem,							 
									   "VTDNav allocation failed ");
								   return NULL;
							   }

							   vn->stackTemp = (int *)malloc((vn->nestingLevel+15)*sizeof(int));

							   if (vn->contextBuf == NULL 
								   || vn->stackTemp == NULL){
									   freeContextBuffer(vn->contextBuf);
									   freeContextBuffer(vn->contextBuf2);
									   free(vn->stackTemp);
									   free(vn->context);
									   free(vn);
									   throwException2(out_of_mem,							 
										   "VTDNav allocation failed ");
									   return NULL;
							   }
							   vn->l1index = vn->l2index = vn->l3index = vn->l4index = vn->l5index=-1;
							   vn->l2lower = vn->l2upper = -1;
							   vn->l3lower = vn->l3upper = -1;
							   vn->l4lower = vn->l4upper = -1;
							   vn->l5lower = vn->l5upper = -1;
							   vn->docOffset = so;
							   vn->docLen = len;
							   vn->vtdSize = vtd->size;
							   vn->bufLen = xLen;
							   vn->br = br;
							   vn->LN = 0;
							   vn->shallowDepth = FALSE;
							   vn->maxLCDepthPlusOne = 6;
							   vn->name=NULL;
							   vn->localName=NULL;
							   vn->URIName=NULL;
							   vn->currentNode=NULL;
							   vn->fib = createFastIntBuffer2(5);
							   return vn;

}

/*int _iterate_L5(VTDNav_L5 *vn, int dp, UCSChar *en, Boolean special){
	// get the current depth
	int index = getCurrentIndex((VTDNav *)vn) + 1;
	tokenType tokenType;
	//int size = vtdBuffer.size;
	while (index < vn->vtdSize) {
		tokenType = getTokenType((VTDNav *)vn,index);
		if (tokenType==TOKEN_ATTR_NAME
			|| tokenType ==TOKEN_ATTR_NS){			  
				index = index+2;
				continue;
		}
		if (isElementOrDocument((VTDNav *)vn,index)) {
			int depth = getTokenDepth((VTDNav *) vn, index);
			if (depth > dp) {
				vn->context[0] = depth;
				if (depth>0)
					vn->context[depth] = index;
				if (special || matchElement((VTDNav *)vn,en)) {
					if (dp< 6)
						resolveLC((VTDNav *)vn);
					return TRUE;
				}
			} else {
				return FALSE;
			}
		}
		index++;

	}
	return FALSE;
}*/

/* DupliateNav duplicates an instance of VTDNav but doesn't retain the original node position*/
VTDNav *_duplicateNav_L5(VTDNav_L5 *vn){	
	VTDNav_L5* vn1 = createVTDNav_L5( vn->rootIndex,
		vn->encoding,
		vn->ns,
		vn->nestingLevel-1,
		vn->XMLDoc,
		vn->bufLen,
		vn->vtdBuffer,
		vn->l1Buffer,
		vn->l2Buffer,
		vn->_l3Buffer,
		vn->l4Buffer,
		vn->l5Buffer,
		vn->docOffset,
		vn->docLen,
		vn->br);	
	vn1->master = FALSE;
	return (VTDNav *)vn1;
}

/* CloneNav duplicates an instance of VTDNav, also copies node position over */
VTDNav *_cloneNav_L5(VTDNav_L5 *vn){
		VTDNav_L5* vn1 = createVTDNav_L5( vn->rootIndex,
		vn->encoding,
		vn->ns,
		vn->nestingLevel-1,
		vn->XMLDoc,
		vn->bufLen,
		vn->vtdBuffer,
		vn->l1Buffer,
		vn->l2Buffer,
		vn->_l3Buffer,
		vn->l4Buffer,
		vn->l5Buffer,
		vn->docOffset,
		vn->docLen,
		vn->br);	
	vn1->master = FALSE;
	vn1->atTerminal = vn->atTerminal;
	vn1->LN = vn->LN;
	if (vn->context[0]!=-1)
		memcpy(vn1->context,vn->context, vn->context[0]);
		//System.arraycopy(vn.context, 0, vn.context, 0, this.context[0] );
	else 
		vn1->context[0]=-1;
	vn1->l1index = vn->l1index; 
	if (getCurrentDepth((VTDNav *)vn)>1){
		vn1->l2index = vn->l2index;
		vn1->l2upper = vn->l2upper;
		vn1->l2lower = vn->l2lower;
	}
	if (getCurrentDepth((VTDNav *)vn) > 2) {
		vn1->l3lower = vn->l3lower;
		vn1->l3index = vn->l3index;
		vn1->l3upper = vn->l3upper;
	}
	if (getCurrentDepth((VTDNav *)vn) > 3) {
		vn1->l4lower = vn->l4lower;
		vn1->l4index = vn->l4index;
		vn1->l4upper = vn->l4upper;
	}
	if (getCurrentDepth((VTDNav *)vn) > 4) {
		vn1->l5lower = vn->l5lower;
		vn1->l5index = vn->l5index;
		vn1->l5upper = vn->l5upper;
	}
	return (VTDNav *)vn1;
}

/* This method takes a vtd index, and recover its correspondin
 * node position, the index can only be of node type element,
 * document, attribute name, attribute value or character data,
 * or CDATA  */
void _recoverNode_L5(VTDNav_L5 *vn, int index){
	int t,d;
	tokenType type;
	if (index <0 || index>=vn->vtdSize )
		throwException2(nav_exception,"Invalid VTD index");

	type = getTokenType((VTDNav *)vn,index);
	if (//type == VTDNav.TOKEN_COMMENT ||
		//	type == VTDNav.TOKEN_PI_NAME ||
		type == TOKEN_PI_VAL ||
		type == TOKEN_DEC_ATTR_NAME ||
		type == TOKEN_DEC_ATTR_VAL ||
		type == TOKEN_ATTR_VAL)
		throwException2(nav_exception,"Token type not yet supported");

	// get depth
	d = getTokenDepth((VTDNav *)vn,index);
	// handle document node;
	switch (d){
		case -1:
			vn->context[0]=-1;
			if (index != 0){
				vn->LN = index;
				vn->atTerminal = TRUE;
			}			
			return;
		case 0:
			vn->context[0]=0;
			if (index != vn->rootIndex){
				vn->LN = index;
				vn->atTerminal = TRUE;
			}
			return;		
	}
	vn->context[0]=d;
	if (type != TOKEN_STARTING_TAG){
		vn->LN = index;
		vn->atTerminal = TRUE;
	}
	// search LC level 1
	recoverNode_l1((VTDNav *)vn,index);

	if (d==1)
		return;
	// search LC level 2
	recoverNode_l2((VTDNav *)vn,index);
	if (d==2){
		//resolveLC();
		return;
	}
	// search LC level 3
	_recoverNode_l3(vn,index);
	if (d==3){
		//resolveLC();
		return;
	}

	recoverNode_l4(vn,index);
	if (d==4){
		//resolveLC();
		return;
	}

	recoverNode_l5(vn,index);
	if (d==5){
		//resolveLC();
		return;
	}

	// scan backward
	if ( type == TOKEN_STARTING_TAG ){
		vn->context[d] = index;
	} else{
		int t = index-1;
		while( !(getTokenType((VTDNav *)vn,t)==TOKEN_STARTING_TAG && 
			getTokenDepth((VTDNav *)vn,t)==d)){
				t--;
		}
		vn->context[d] = t;
	}
	t = vn->context[d]-1;
	d--;
	while(d>5){
		while( !(getTokenType((VTDNav *)vn,t)==TOKEN_STARTING_TAG && 
			getTokenDepth((VTDNav *)vn,t)==d)){
				t--;
		}
		vn->context[d] = t;
		d--;
	}
}


//This method is similar to getElementByName in DOM except it doesn't
//return the nodeset, instead it iterates over those nodes .
//When URL is "*" it will match any namespace
//if ns is FALSE, return FALSE immediately
/*int _iterateNS_L5(VTDNav_L5 *vn, int dp, UCSChar *URL, UCSChar *ln){
	int index;
	tokenType tokenType;
	if (vn->ns == FALSE)
		return FALSE;

	index = getCurrentIndex((VTDNav *)vn) + 1;
	while (index < vn->vtdSize) {
		tokenType = getTokenType((VTDNav *)vn, index);
		if(tokenType==TOKEN_ATTR_NAME
			|| tokenType == TOKEN_ATTR_NS){
				index = index+2;
				continue;
		}
		if (isElementOrDocument((VTDNav *)vn,index)) {
			int depth = getTokenDepth((VTDNav *)vn,index);
			if (depth > dp) {
				vn->context[0] = depth;
				if (depth>0)
					vn->context[depth] = index;
				if (matchElementNS((VTDNav *)vn,URL, ln)) {
					if (dp < 6)
						resolveLC((VTDNav *)vn);
					return TRUE;
				}
			} else {
				return FALSE;
			}
		}
		index++;
	}
	return FALSE;
}*/


// This function is called by selectElement_P in autoPilot
/*Boolean _iterate_preceding_L5(VTDNav_L5 *vn,UCSChar *en, int* a, Boolean special){
		int t,d,i,index = getCurrentIndex((VTDNav *)vn) - 1;
		//int depth = getTokenDepth(index);
		//int size = vtdBuffer.size;
		while (index >  0) {
			if (isElementOrDocument((VTDNav *)vn,index)) {
				int depth = getTokenDepth((VTDNav *)vn,index);
				vn->context[0] = depth;
				//context[depth]=index;
				if (depth>0){
					vn->context[depth] = index;
					t = index -1;
					for (i=depth-1;i>0;i--){
						if (vn->context[i]>index || vn->context[i] == -1){
							while(t>0){
								d = getTokenDepth((VTDNav *)vn,t);
								if ( d == i && isElement((VTDNav *)vn,t)){
									vn->context[i] = t;
									break;
								}
								t--;
							}							
						}else
							break;
					}
				}
				//dumpContext();
				if (index!= a[depth] && (special || matchElement((VTDNav *)vn,en))) {
					if (depth <6)
						resolveLC((VTDNav *)vn);
					return TRUE;
				}
			} 
			index--;
		}
		return FALSE;	
}*/

// This function is called by selectElementNS_P in autoPilot
/*Boolean _iterate_precedingNS_L5(VTDNav_L5 *vn,UCSChar *URL, UCSChar *ln, int* a){
		int t,d,i,index = getCurrentIndex((VTDNav *)vn) - 1;
		
		//int depth = getTokenDepth(index);
		//int size = vtdBuffer.size;
		while (index > 0 ) {
			if (isElementOrDocument((VTDNav *)vn,index)) {
				int depth = getTokenDepth((VTDNav *)vn,index);
				vn->context[0] = depth;
				//context[depth]=index;
				if (depth>0){
					vn->context[depth] = index;
					t = index -1;
					for (i=depth-1;i>0;i--){
						if (vn->context[i]>index || vn->context[i]==-1){
							while(t>0){
								d = getTokenDepth((VTDNav *)vn,t);
								if ( d == i && isElement((VTDNav *)vn,t)){
									vn->context[i] = t;
									break;
								}
								t--;
							}							
						}else
							break;
					}
				}
				//dumpContext();
				if (index != a[depth] && matchElementNS((VTDNav *)vn,URL,ln)) {	
					if (depth <6)
						resolveLC((VTDNav *)vn);
					return TRUE;
				}
			} 
			index--;
		}
		return FALSE;	
}*/
// This function is called by selectElement_F in autoPilot
/*Boolean _iterate_following_L5(VTDNav_L5 *vn,UCSChar *en, Boolean special){
		int index = getCurrentIndex((VTDNav *)vn) + 1;
		//int size = vtdBuffer.size;
		while (index < vn->vtdSize) {
			if (isElementOrDocument((VTDNav *)vn,index)) {
				int depth = getTokenDepth((VTDNav *)vn,index);
				vn->context[0] = depth;
				if (depth>0)
					vn->context[depth] = index;
				if (special || matchElement((VTDNav *)vn,en)) {	
					if (depth <6)
					  resolveLC((VTDNav *)vn);
					return TRUE;
				}
			} 
			index++;
		}
		return FALSE;	
}*/
// This function is called by selectElementNS_F in autoPilot
/*Boolean _iterate_followingNS_L5(VTDNav_L5 *vn, UCSChar *URL, UCSChar *ln){
		int index = getCurrentIndex((VTDNav *)vn) + 1;
		//int size = vtdBuffer.size;
		while (index < vn->vtdSize) {
			if (isElementOrDocument((VTDNav *)vn, index)) {
				int depth = getTokenDepth((VTDNav *)vn,index);
				vn->context[0] = depth;
				if (depth>0)
					vn->context[depth] = index;
				if (matchElementNS((VTDNav *)vn,URL,ln)) {	
					if (depth <6)
						resolveLC((VTDNav *)vn);
					return TRUE;
				}
			} 
			index++;
		}
		return FALSE;
}*/

//Load the context info from ContextBuffer.
//Info saved including LC and current state of the context 
Boolean _pop_L5(VTDNav_L5 *vn){
	Boolean b =load(vn->contextBuf,vn->stackTemp);
	int i;
	if (b == FALSE)
		return FALSE;
	for (i = 0; i < vn->nestingLevel; i++) {
		vn->context[i] = vn->stackTemp[i];
	}

	vn->l1index = vn->stackTemp[vn->nestingLevel];
	vn->l2index = vn->stackTemp[vn->nestingLevel + 1];
	vn->l3index = vn->stackTemp[vn->nestingLevel + 2];
	vn->l4index = vn->stackTemp[vn->nestingLevel + 3];
	vn->l5index = vn->stackTemp[vn->nestingLevel + 4];
	vn->l2lower = vn->stackTemp[vn->nestingLevel + 5];
	vn->l2upper = vn->stackTemp[vn->nestingLevel + 6];
	vn->l3lower = vn->stackTemp[vn->nestingLevel + 7];
	vn->l3upper = vn->stackTemp[vn->nestingLevel + 8];
	vn->l4lower = vn->stackTemp[vn->nestingLevel + 9];
	vn->l4upper = vn->stackTemp[vn->nestingLevel + 10];
	vn->l5lower = vn->stackTemp[vn->nestingLevel + 11];
	vn->l5upper = vn->stackTemp[vn->nestingLevel + 12];
	vn->atTerminal = (vn->stackTemp[vn->nestingLevel + 13] == 1);
	vn->LN = vn->stackTemp[vn->nestingLevel+14];
	return TRUE;
}

Boolean _pop2_L5(VTDNav_L5 *vn){
	Boolean b =load(vn->contextBuf2,vn->stackTemp);
	int i;
	if (b == FALSE)
		return FALSE;
	for (i = 0; i < vn->nestingLevel; i++) {
		vn->context[i] = vn->stackTemp[i];
	}

	vn->l1index = vn->stackTemp[vn->nestingLevel];
	vn->l2index = vn->stackTemp[vn->nestingLevel + 1];
	vn->l3index = vn->stackTemp[vn->nestingLevel + 2];
	vn->l4index = vn->stackTemp[vn->nestingLevel + 3];
	vn->l5index = vn->stackTemp[vn->nestingLevel + 4];
	vn->l2lower = vn->stackTemp[vn->nestingLevel + 5];
	vn->l2upper = vn->stackTemp[vn->nestingLevel + 6];
	vn->l3lower = vn->stackTemp[vn->nestingLevel + 7];
	vn->l3upper = vn->stackTemp[vn->nestingLevel + 8];
	vn->l4lower = vn->stackTemp[vn->nestingLevel + 9];
	vn->l4upper = vn->stackTemp[vn->nestingLevel + 10];
	vn->l5lower = vn->stackTemp[vn->nestingLevel + 11];
	vn->l5upper = vn->stackTemp[vn->nestingLevel + 12];
	vn->atTerminal = (vn->stackTemp[vn->nestingLevel + 13] == 1);
	vn->LN = vn->stackTemp[vn->nestingLevel+14];
	return TRUE;
}
//Store the context info into the ContextBuffer.
//Info saved including LC and current state of the context 
Boolean _push_L5(VTDNav_L5 *vn){
	int i;
	for (i = 0; i < vn->nestingLevel; i++) {
		vn->stackTemp[i] = vn->context[i];
	}
	vn->stackTemp[vn->nestingLevel] = vn->l1index;
	vn->stackTemp[vn->nestingLevel + 1] = vn->l2index;
	vn->stackTemp[vn->nestingLevel + 2] = vn->l3index;
	vn->stackTemp[vn->nestingLevel + 3] = vn->l4index;
	vn->stackTemp[vn->nestingLevel + 4] = vn->l5index;
	vn->stackTemp[vn->nestingLevel + 5] = vn->l2lower;
	vn->stackTemp[vn->nestingLevel + 6] = vn->l2upper;
	vn->stackTemp[vn->nestingLevel + 7] = vn->l3lower;
	vn->stackTemp[vn->nestingLevel + 8] = vn->l3upper;
	vn->stackTemp[vn->nestingLevel + 9] = vn->l4lower;
	vn->stackTemp[vn->nestingLevel + 10] = vn->l4upper;
	vn->stackTemp[vn->nestingLevel + 11] = vn->l5lower;
	vn->stackTemp[vn->nestingLevel + 12] = vn->l5upper;

	if (vn->atTerminal)
		vn->stackTemp[vn->nestingLevel + 13] =1;
	else
		vn->stackTemp[vn->nestingLevel + 13] =0;
	vn->stackTemp[vn->nestingLevel+14] = vn->LN; 
	store(vn->contextBuf,vn->stackTemp);
	return TRUE;
}

Boolean _push2_L5(VTDNav_L5 *vn){
	int i;
	for (i = 0; i < vn->nestingLevel; i++) {
		vn->stackTemp[i] = vn->context[i];
	}
	vn->stackTemp[vn->nestingLevel] = vn->l1index;
	vn->stackTemp[vn->nestingLevel + 1] = vn->l2index;
	vn->stackTemp[vn->nestingLevel + 2] = vn->l3index;
	vn->stackTemp[vn->nestingLevel + 3] = vn->l4index;
	vn->stackTemp[vn->nestingLevel + 4] = vn->l5index;
	vn->stackTemp[vn->nestingLevel + 5] = vn->l2lower;
	vn->stackTemp[vn->nestingLevel + 6] = vn->l2upper;
	vn->stackTemp[vn->nestingLevel + 7] = vn->l3lower;
	vn->stackTemp[vn->nestingLevel + 8] = vn->l3upper;
	vn->stackTemp[vn->nestingLevel + 9] = vn->l4lower;
	vn->stackTemp[vn->nestingLevel + 10] = vn->l4upper;
	vn->stackTemp[vn->nestingLevel + 11] = vn->l5lower;
	vn->stackTemp[vn->nestingLevel + 12] = vn->l5upper;

	if (vn->atTerminal)
		vn->stackTemp[vn->nestingLevel + 13] =1;
	else
		vn->stackTemp[vn->nestingLevel + 13] =0;
	vn->stackTemp[vn->nestingLevel+14] = vn->LN; 
	store(vn->contextBuf2,vn->stackTemp);
	return TRUE;
}
void _sampleState_L5(VTDNav_L5 *vn, FastIntBuffer *fib){
		if (vn->context[0]>=1)
			appendInt(fib,vn->l1index);
		else return;
		
		if (vn->context[0]>=2){
			appendInt(fib,vn->l2index);
			appendInt(fib,vn->l2lower);
			appendInt(fib,vn->l2upper);				
		}
		else return;
		
		if (vn->context[0]>=3){
		   appendInt(fib,vn->l3index);
		   appendInt(fib,vn->l3lower);
		   appendInt(fib,vn->l3upper);
		}
		else return;
		
		if (vn->context[0]>=4){
			   appendInt(fib,vn->l4index);
			   appendInt(fib,vn->l4lower);
			   appendInt(fib,vn->l4upper);	
		}
		else return;
		
		if (vn->context[0]>=5){  
			appendInt(fib,vn->l5index);
			appendInt(fib,vn->l5lower);
			appendInt(fib,vn->l5upper);			
		}
		else return;
}

Boolean _toElement_L5(VTDNav_L5 *vn, navDir direction){
		int size,i;
		switch (direction) {
			case ROOT : // to document element!
				if (vn->context[0] != 0) {
					/*
                     * for (int i = 1; i <= context[0]; i++) { context[i] =
                     * 0xffffffff; }
                     */
					vn->context[0] = 0;
				}
				vn->atTerminal = FALSE;
				vn->l1index = vn->l2index = vn->l3index = -1;
				return TRUE;
			case PARENT :
				if (vn->atTerminal == TRUE){
					vn->atTerminal = FALSE;
					return TRUE;
				}
				if (vn->context[0] > 0) {
					//context[context[0]] = context[context[0] + 1] =
                    // 0xffffffff;
					vn->context[vn->context[0]] = -1;
					vn->context[0]--;
					return TRUE;
				}else if (vn->context[0]==0){
					vn->context[0]=-1; //to be compatible with XPath Data model
					return TRUE;
 				}
				else {
					return FALSE;
				}
			case FIRST_CHILD :
			case LAST_CHILD :
				if (vn->atTerminal) return FALSE;
				switch (vn->context[0]) {
				    case -1:
				    	vn->context[0] = 0;
				    	return TRUE;
					case 0 :
						if (vn->l1Buffer->size > 0) {
							vn->context[0] = 1;
							vn->l1index =
								(direction == FIRST_CHILD)
									? 0
									: (vn->l1Buffer->size - 1);
							vn->context[1] = upper32At(vn->l1Buffer,vn->l1index);
							//(int) (vtdToken >> 32);
							return TRUE;
						} else
							return FALSE;
					case 1 :
						vn->l2lower = lower32At(vn->l1Buffer,vn->l1index);
						if (vn->l2lower == -1) {
							return FALSE;
						}
						vn->context[0] = 2;
						vn->l2upper = vn->l2Buffer->size - 1;
						size = vn->l1Buffer->size;
						for (i = vn->l1index + 1; i < size; i++) {
							int temp = lower32At(vn->l1Buffer,i);
							if (temp != -1) {
								vn->l2upper = temp - 1;
								break;
							}
						}
						//System.out.println(" l2 upper: " + l2upper + " l2
                        // lower : " + l2lower);
						vn->l2index =
							(direction == FIRST_CHILD) ? vn->l2lower : vn->l2upper;
						vn->context[2] = upper32At(vn->l2Buffer,vn->l2index);
						return TRUE;
						
					case 2 :
						vn->l3lower = lower32At(vn->l2Buffer,vn->l2index);
						if (vn->l3lower == -1) {
							return FALSE;
						}
						vn->context[0] = 3;
						vn->l3upper = vn->_l3Buffer->size - 1;
						size = vn->l2Buffer->size;
						for (i = vn->l2index + 1; i < size; i++) {
							int temp = lower32At(vn->l2Buffer,i);
							if (temp != -1) {
								vn->l3upper = temp - 1;
								break;
							}
						}
						//System.out.println(" l2 upper: " + l2upper + " l2
                        // lower : " + l2lower);
						vn->l3index =
							(direction == FIRST_CHILD) ? vn->l3lower : vn->l3upper;
						vn->context[3] = upper32At(vn->_l3Buffer,vn->l3index);
						return TRUE;
						
					case 3 :
						vn->l4lower = lower32At(vn->_l3Buffer,vn->l3index);
						if (vn->l4lower == -1) {
							return FALSE;
						}
						vn->context[0] = 4;
						vn->l4upper = vn->l4Buffer->size - 1;
						size = vn->_l3Buffer->size;
						for (i = vn->l3index + 1; i < size; i++) {
							int temp = lower32At(vn->_l3Buffer,i);
							if (temp != -1) {
								vn->l4upper = temp - 1;
								break;
							}
						}
						//System.out.println(" l2 upper: " + l2upper + " l2
                        // lower : " + l2lower);
						vn->l4index =
							(direction == FIRST_CHILD) ? vn->l4lower : vn->l4upper;
						vn->context[4] = upper32At(vn->l4Buffer,vn->l4index);
						return TRUE;

					case 4 :
						vn->l5lower = lower32At(vn->l4Buffer,vn->l4index);
						if (vn->l5lower == -1) {
							return FALSE;
						}
						vn->context[0] = 5;

						vn->l5upper = vn->l5Buffer->size - 1;
						size = vn->l4Buffer->size;
						for (i = vn->l4index + 1; i < size; i++) {
							int temp = lower32At(vn->l4Buffer,i);
							if (temp != -1) {
								vn->l5upper = temp - 1;
								break;
							}
						}
						//System.out.println(" l3 upper : " + l3upper + " l3
                        // lower : " + l3lower);
						vn->l5index =
							(direction == FIRST_CHILD) ? vn->l5lower : vn->l5upper;
						vn->context[5] = intAt(vn->l5Buffer,vn->l5index);

						return TRUE;

					default :
						if (direction == FIRST_CHILD) {
							int index;
							size = vn->vtdBuffer->size;
							index = vn->context[vn->context[0]] + 1;
							while (index < size) {
								Long temp = longAt(vn->vtdBuffer,index);
								int token_type =
									(int) ((MASK_TOKEN_TYPE & temp) >> 60)
										& 0xf;

								if (token_type == TOKEN_STARTING_TAG) {
									int depth =
										(int) ((MASK_TOKEN_DEPTH & temp) >> 52);
									if (depth <= vn->context[0]) {
										return FALSE;
									} else if (depth == (vn->context[0] + 1)) {
										vn->context[0] += 1;
										vn->context[vn->context[0]] = index;
										return TRUE;
									}
								}

								index++;
							} // what condition
							return FALSE;
						} else {
							int index = vn->context[vn->context[0]] + 1;
							int last_index = -1;
							size = vn->vtdBuffer->size;
							while (index < size) {
								Long temp = longAt(vn->vtdBuffer,index);
								int depth =
									(int) ((MASK_TOKEN_DEPTH & temp) >> 52);
								int token_type =
									(int) ((MASK_TOKEN_TYPE & temp) >> 60)
										& 0xf;
								
								if (token_type == TOKEN_STARTING_TAG) {
									if (depth <= vn->context[0]) {
										break;
									} else if (depth == (vn->context[0] + 1)) {
										last_index = index;
									}
								}

								index++;
							}
							if (last_index == -1) {
								return FALSE;
							} else {
								vn->context[0] += 1;
								vn->context[vn->context[0]] = last_index;
								return TRUE;
							}
						}
				}

			case NEXT_SIBLING :
			case PREV_SIBLING :
				if(vn->atTerminal)return FALSE;
				switch (vn->context[0]) {
					case -1:
					case 0 :
						return FALSE;
					case 1 :
						if (direction == NEXT_SIBLING) {
							if (vn->l1index + 1 >= vn->l1Buffer->size) {
								return FALSE;
							}

							vn->l1index++; // global incremental
						} else {
							if (vn->l1index - 1 < 0) {
								return FALSE;
							}
							vn->l1index--; // global incremental
						}
						vn->context[1] = upper32At(vn->l1Buffer,vn->l1index);
						return TRUE;
					case 2 :
						if (direction == NEXT_SIBLING) {
							if (vn->l2index + 1 > vn->l2upper) {
								return FALSE;
							}
							vn->l2index++;
						} else {
							if (vn->l2index - 1 < vn->l2lower) {
								return FALSE;
							}
							vn->l2index--;
						}
						vn->context[2] = upper32At(vn->l2Buffer,vn->l2index);
						return TRUE;
						
					case 3 :
						if (direction == NEXT_SIBLING) {
							if (vn->l3index + 1 > vn->l3upper) {
								return FALSE;
							}
							vn->l3index++;
						} else {
							if (vn->l3index - 1 < vn->l3lower) {
								return FALSE;
							}
							vn->l3index--;
						}
						vn->context[3] = upper32At(vn->_l3Buffer,vn->l3index);
						return TRUE;
						
					case 4 :
						if (direction == NEXT_SIBLING) {
							if (vn->l4index + 1 > vn->l4upper) {
								return FALSE;
							}
							vn->l4index++;
						} else {
							if (vn->l4index - 1 < vn->l4lower) {
								return FALSE;
							}
							vn->l4index--;
						}
						vn->context[4] = upper32At(vn->l4Buffer,vn->l4index);
						return TRUE;
					case 5 :
						if (direction == NEXT_SIBLING) {
							if (vn->l5index + 1 > vn->l5upper) {
								return FALSE;
							}
							vn->l5index++;
						} else {
							if (vn->l5index - 1 < vn->l5lower) {
								return FALSE;
							}
							vn->l5index--;
						}
						vn->context[5] = intAt(vn->l5Buffer,vn->l5index);
						return TRUE;
					default :
						//int index = context[context[0]] + 1;

						if (direction == NEXT_SIBLING) {
							int index = vn->context[vn->context[0]] + 1;
							size = vn->vtdBuffer->size;
							while (index < size) {
								Long temp = longAt(vn->vtdBuffer,index);
								int token_type =
									(int) ((MASK_TOKEN_TYPE & temp) >> 60)
										& 0xf;

								if (token_type == TOKEN_STARTING_TAG) {
									int depth =
										(int) ((MASK_TOKEN_DEPTH & temp) >> 52);
									if (depth < vn->context[0]) {
										return FALSE;
									} else if (depth == (vn->context[0])) {
										vn->context[vn->context[0]] = index;
										return TRUE;
									}
								}
								index++;
							}
							return FALSE;
						} else {
							int index = vn->context[vn->context[0]] - 1;
							while (index > vn->context[vn->context[0] - 1]) {
								// scan backforward
								Long temp = longAt(vn->vtdBuffer,index);
								int token_type =
									(int) ((MASK_TOKEN_TYPE & temp) >> 60)
										& 0xf;

								if (token_type == TOKEN_STARTING_TAG) {
									int depth =
										(int) ((MASK_TOKEN_DEPTH & temp) >> 52);
									/*
                                     * if (depth < context[0]) { return FALSE; }
                                     * else
                                     */
									if (depth == (vn->context[0])) {
										vn->context[vn->context[0]] = index;
										return TRUE;
									}
								}
								index--;
							} // what condition
							return FALSE;
						}
				}

			default :
				throwException2(nav_exception,"illegal navigation options");
				return FALSE;
		}
}
Boolean _toElement2_L5(VTDNav_L5 *vn, navDir direction, UCSChar *en){
		int temp;
		int d;
		int val=0;
		if (en == NULL)
			throwException2( invalid_argument, " Element name can't be null ");
		if (wcscmp(en,L"*"))
			return _toElement_L5(vn,direction);
		switch (direction) {
			case ROOT :
				return _toElement_L5(vn, ROOT);

			case PARENT :
				return _toElement_L5(vn, PARENT);

			case FIRST_CHILD :
				if (vn->atTerminal)return FALSE;
				if (_toElement_L5(vn, FIRST_CHILD) == FALSE)
					return FALSE;
				// check current element name
				if (matchElement((VTDNav *)vn, en) == FALSE) {
					if (_toElement2_L5(vn, NEXT_SIBLING, en) == TRUE)
						return TRUE;
					else {
						//toParentElement();
						//context[context[0]] = 0xffffffff;
						vn->context[0]--;
						return FALSE;
					}
				} else
					return TRUE;

			case LAST_CHILD :
				if (vn->atTerminal)return FALSE;
				if (_toElement_L5(vn,LAST_CHILD) == FALSE)
					return FALSE;
				if (matchElement((VTDNav *)vn, en) == FALSE) {
					if (_toElement2_L5(vn,PREV_SIBLING, en) == TRUE)
						return TRUE;
					else {
						//context[context[0]] = 0xffffffff;
						vn->context[0]--;
						//toParentElement();
						return FALSE;
					}
				} else
					return TRUE;

			case NEXT_SIBLING :
				if (vn->atTerminal)return FALSE;
				d = vn->context[0];
				
				switch(d)
				{
				  case -1:
				  case 0: return FALSE;
				  case 1: val = vn->l1index; break;
				  case 2: val = vn->l2index; break;
				  case 3: val = vn->l3index; break;
				  case 4: val = vn->l4index; break;
				  case 5: val = vn->l5index; break;
				  	//default:
				}
				temp = vn->context[d]; // store the current position
				
				while (_toElement_L5(vn,NEXT_SIBLING)) {
					if (matchElement((VTDNav *)vn,en)) {
						return TRUE;
					}
				}
				switch(d)
				{
				  case 1: vn->l1index = val; break;
				  case 2: vn->l2index = val; break;
				  case 3: vn->l3index = val; break;
				  case 4: vn->l4index = val; break;
				  case 5: vn->l5index = val; break;
				  	//default:
				}
				vn->context[d] = temp;
				return FALSE;

			case PREV_SIBLING :
				if (vn->atTerminal) return FALSE;
				d = vn->context[0];
				switch(d)
				{
				  case -1:
				  case 0: return FALSE;
				  case 1: val = vn->l1index; break;
				  case 2: val = vn->l2index; break;
				  case 3: val = vn->l3index; break;
				  case 4: val = vn->l4index; break;
				  case 5: val = vn->l5index; break;
				  	//default:
				}
				temp = vn->context[d]; // store the current position
				
				while (_toElement_L5(vn,PREV_SIBLING)) {
					if (matchElement((VTDNav *)vn,en)) {
						return TRUE;
					}
				}
				switch(d)
				{
				  case 1: vn->l1index = val; break;
				  case 2: vn->l2index = val; break;
				  case 3: vn->l3index = val; break;
				  case 4: vn->l4index = val; break;
				  case 5: vn->l5index = val; break;
				  	//default:
				}
				vn->context[d] = temp;
				return FALSE;

			default :
				throwException2( nav_exception,"illegal navigation options");
				return FALSE;
		}
}
Boolean _toElementNS_L5(VTDNav_L5 *vn, navDir direction, UCSChar *URL, UCSChar *ln){
		int size,i;
		switch (direction) {
			case ROOT : // to document element!
				if (vn->context[0] != 0) {
					/*
                     * for (int i = 1; i <= context[0]; i++) { context[i] =
                     * 0xffffffff; }
                     */
					vn->context[0] = 0;
				}
				vn->atTerminal = FALSE;
				vn->l1index = vn->l2index = vn->l3index = -1;
				return TRUE;
			case PARENT :
				if (vn->atTerminal == TRUE){
					vn->atTerminal = FALSE;
					return TRUE;
				}
				if (vn->context[0] > 0) {
					//context[context[0]] = context[context[0] + 1] =
                    // 0xffffffff;
					vn->context[vn->context[0]] = -1;
					vn->context[0]--;
					return TRUE;
				}else if (vn->context[0]==0){
					vn->context[0]=-1; //to be compatible with XPath Data model
					return TRUE;
 				}
				else {
					return FALSE;
				}
			case FIRST_CHILD :
			case LAST_CHILD :
				if (vn->atTerminal) return FALSE;
				switch (vn->context[0]) {
				    case -1:
				    	vn->context[0] = 0;
				    	return TRUE;
					case 0 :
						if (vn->l1Buffer->size > 0) {
							vn->context[0] = 1;
							vn->l1index =
								(direction == FIRST_CHILD)
									? 0
									: (vn->l1Buffer->size - 1);
							vn->context[1] = upper32At(vn->l1Buffer,vn->l1index);
							//(int) (vtdToken >> 32);
							return TRUE;
						} else
							return FALSE;
					case 1 :
						vn->l2lower = lower32At(vn->l1Buffer,vn->l1index);
						if (vn->l2lower == -1) {
							return FALSE;
						}
						vn->context[0] = 2;
						vn->l2upper = vn->l2Buffer->size - 1;
						size = vn->l1Buffer->size;
						for (i = vn->l1index + 1; i < size; i++) {
							int temp = lower32At(vn->l1Buffer,i);
							if (temp != -1) {
								vn->l2upper = temp - 1;
								break;
							}
						}
						//System.out.println(" l2 upper: " + l2upper + " l2
                        // lower : " + l2lower);
						vn->l2index =
							(direction == FIRST_CHILD) ? vn->l2lower : vn->l2upper;
						vn->context[2] = upper32At(vn->l2Buffer,vn->l2index);
						return TRUE;
						
					case 2 :
						vn->l3lower = lower32At(vn->l2Buffer,vn->l2index);
						if (vn->l3lower == -1) {
							return FALSE;
						}
						vn->context[0] = 3;
						vn->l3upper = vn->_l3Buffer->size - 1;
						size = vn->l2Buffer->size;
						for (i = vn->l2index + 1; i < size; i++) {
							int temp = lower32At(vn->l2Buffer,i);
							if (temp != -1) {
								vn->l3upper = temp - 1;
								break;
							}
						}
						//System.out.println(" l2 upper: " + l2upper + " l2
                        // lower : " + l2lower);
						vn->l3index =
							(direction == FIRST_CHILD) ? vn->l3lower : vn->l3upper;
						vn->context[3] = upper32At(vn->_l3Buffer,vn->l3index);
						return TRUE;
						
					case 3 :
						vn->l4lower = lower32At(vn->_l3Buffer,vn->l3index);
						if (vn->l4lower == -1) {
							return FALSE;
						}
						vn->context[0] = 4;
						vn->l4upper = vn->l4Buffer->size - 1;
						size = vn->_l3Buffer->size;
						for (i = vn->l3index + 1; i < size; i++) {
							int temp = lower32At(vn->_l3Buffer,i);
							if (temp != -1) {
								vn->l4upper = temp - 1;
								break;
							}
						}
						//System.out.println(" l2 upper: " + l2upper + " l2
                        // lower : " + l2lower);
						vn->l4index =
							(direction == FIRST_CHILD) ? vn->l4lower : vn->l4upper;
						vn->context[4] = upper32At(vn->l4Buffer,vn->l4index);
						return TRUE;

					case 4 :
						vn->l5lower = lower32At(vn->l4Buffer,vn->l4index);
						if (vn->l5lower == -1) {
							return FALSE;
						}
						vn->context[0] = 5;

						vn->l5upper = vn->l5Buffer->size - 1;
						size = vn->l4Buffer->size;
						for (i = vn->l4index + 1; i < size; i++) {
							int temp = lower32At(vn->l4Buffer,i);
							if (temp != -1) {
								vn->l5upper = temp - 1;
								break;
							}
						}
						//System.out.println(" l3 upper : " + l3upper + " l3
                        // lower : " + l3lower);
						vn->l5index =
							(direction == FIRST_CHILD) ? vn->l5lower : vn->l5upper;
						vn->context[5] = intAt(vn->l5Buffer,vn->l5index);

						return TRUE;

					default :
						if (direction == FIRST_CHILD) {
							int index;
							size = vn->vtdBuffer->size;
							index = vn->context[vn->context[0]] + 1;
							while (index < size) {
								Long temp = longAt(vn->vtdBuffer,index);
								int token_type =
									(int) ((MASK_TOKEN_TYPE & temp) >> 60)
										& 0xf;

								if (token_type == TOKEN_STARTING_TAG) {
									int depth =
										(int) ((MASK_TOKEN_DEPTH & temp) >> 52);
									if (depth <= vn->context[0]) {
										return FALSE;
									} else if (depth == (vn->context[0] + 1)) {
										vn->context[0] += 1;
										vn->context[vn->context[0]] = index;
										return TRUE;
									}
								}

								index++;
							} // what condition
							return FALSE;
						} else {
							int index = vn->context[vn->context[0]] + 1;
							int last_index = -1;
							size = vn->vtdBuffer->size;
							while (index < size) {
								Long temp = longAt(vn->vtdBuffer,index);
								int depth =
									(int) ((MASK_TOKEN_DEPTH & temp) >> 52);
								int token_type =
									(int) ((MASK_TOKEN_TYPE & temp) >> 60)
										& 0xf;
								
								if (token_type == TOKEN_STARTING_TAG) {
									if (depth <= vn->context[0]) {
										break;
									} else if (depth == (vn->context[0] + 1)) {
										last_index = index;
									}
								}

								index++;
							}
							if (last_index == -1) {
								return FALSE;
							} else {
								vn->context[0] += 1;
								vn->context[vn->context[0]] = last_index;
								return TRUE;
							}
						}
				}

			case NEXT_SIBLING :
			case PREV_SIBLING :
				if(vn->atTerminal)return FALSE;
				switch (vn->context[0]) {
					case -1:
					case 0 :
						return FALSE;
					case 1 :
						if (direction == NEXT_SIBLING) {
							if (vn->l1index + 1 >= vn->l1Buffer->size) {
								return FALSE;
							}

							vn->l1index++; // global incremental
						} else {
							if (vn->l1index - 1 < 0) {
								return FALSE;
							}
							vn->l1index--; // global incremental
						}
						vn->context[1] = upper32At(vn->l1Buffer,vn->l1index);
						return TRUE;
					case 2 :
						if (direction == NEXT_SIBLING) {
							if (vn->l2index + 1 > vn->l2upper) {
								return FALSE;
							}
							vn->l2index++;
						} else {
							if (vn->l2index - 1 < vn->l2lower) {
								return FALSE;
							}
							vn->l2index--;
						}
						vn->context[2] = upper32At(vn->l2Buffer, vn->l2index);
						return TRUE;
						
					case 3 :
						if (direction == NEXT_SIBLING) {
							if (vn->l3index + 1 > vn->l3upper) {
								return FALSE;
							}
							vn->l3index++;
						} else {
							if (vn->l3index - 1 < vn->l3lower) {
								return FALSE;
							}
							vn->l3index--;
						}
						vn->context[3] = upper32At(vn->_l3Buffer,vn->l3index);
						return TRUE;
						
					case 4 :
						if (direction == NEXT_SIBLING) {
							if (vn->l4index + 1 > vn->l4upper) {
								return FALSE;
							}
							vn->l4index++;
						} else {
							if (vn->l4index - 1 < vn->l4lower) {
								return FALSE;
							}
							vn->l4index--;
						}
						vn->context[4] = upper32At(vn->l4Buffer,vn->l4index);
						return TRUE;
					case 5 :
						if (direction == NEXT_SIBLING) {
							if (vn->l5index + 1 > vn->l5upper) {
								return FALSE;
							}
							vn->l5index++;
						} else {
							if (vn->l5index - 1 < vn->l5lower) {
								return FALSE;
							}
							vn->l5index--;
						}
						vn->context[5] = intAt(vn->l5Buffer,vn->l5index);
						return TRUE;
					default :
						//int index = context[context[0]] + 1;

						if (direction == NEXT_SIBLING) {
							int index = vn->context[vn->context[0]] + 1;
							size = vn->vtdBuffer->size;
							while (index < size) {
								Long temp = longAt(vn->vtdBuffer,index);
								int token_type =
									(int) ((MASK_TOKEN_TYPE & temp) >> 60)
										& 0xf;

								if (token_type == TOKEN_STARTING_TAG) {
									int depth =
										(int) ((MASK_TOKEN_DEPTH & temp) >> 52);
									if (depth < vn->context[0]) {
										return FALSE;
									} else if (depth == (vn->context[0])) {
										vn->context[vn->context[0]] = index;
										return TRUE;
									}
								}
								index++;
							}
							return FALSE;
						} else {
							int index = vn->context[vn->context[0]] - 1;
							while (index > vn->context[vn->context[0] - 1]) {
								// scan backforward
								Long temp = longAt(vn->vtdBuffer,index);
								int token_type =
									(int) ((MASK_TOKEN_TYPE & temp) >> 60)
										& 0xf;

								if (token_type == TOKEN_STARTING_TAG) {
									int depth =
										(int) ((MASK_TOKEN_DEPTH & temp) >> 52);
									/*
                                     * if (depth < context[0]) { return FALSE; }
                                     * else
                                     */
									if (depth == (vn->context[0])) {
										vn->context[vn->context[0]] = index;
										return TRUE;
									}
								}
								index--;
							} // what condition
							return FALSE;
						}
				}

			default :
				throwException2( nav_exception,"illegal navigation options");
				return FALSE;
		}
}


void _resolveLC_l3(VTDNav_L5 *vn){
	int i,temp = lower32At(vn->l2Buffer,vn->l2index);
	if (vn->l3lower != temp) {
		vn->l3lower = temp;
		// l2lower shouldn't be -1 !!!! l2lower and l2upper always get
		// resolved simultaneously
		vn->l3index = vn->l3lower;
		vn->l3upper = vn->_l3Buffer->size - 1;
		for (i = vn->l2index + 1; i < vn->l2Buffer->size; i++) {
			temp = lower32At(vn->l2Buffer,i);
			if (temp != -1) {
				vn->l3upper = temp - 1;
				break;
			}
		}
	} // intelligent guess again ??

	if (vn->l3index < 0 || vn->l3index >= vn->_l3Buffer->size
		|| vn->context[3] != upper32At(vn->_l3Buffer, vn->l3index)) {

			if (vn->l3index >= vn->_l3Buffer->size || vn->l3index<0)
				vn->l3index = vn->l3lower;
			if (vn->l3index+1< vn->_l3Buffer->size&& vn->context[3] == upper32At(vn->_l3Buffer,vn->l3index + 1))
				vn->l3index = vn->l3index + 1;
			else if (vn->l3upper - vn->l3lower >= 16) {
				int init_guess = vn->l3lower
					+ (int) ((vn->l3upper - vn->l3lower)
					* ((float) vn->context[3] - upper32At(vn->_l3Buffer,vn->l3lower)) / (upper32At(vn->_l3Buffer,vn->l3upper) - upper32At(vn->_l3Buffer,vn->l3lower)));
				if (upper32At(vn->_l3Buffer,init_guess) > vn->context[3]) {
					while (vn->context[3] != upper32At(vn->_l3Buffer,init_guess))
						init_guess--;
				} else if (upper32At(vn->_l3Buffer,init_guess) < vn->context[3]) {
					while (vn->context[3] != upper32At(vn->_l3Buffer,init_guess))
						init_guess++;
				}
				vn->l3index = init_guess;
			} else if (vn->context[3]<upper32At(vn->_l3Buffer,vn->l3index)){

				while ( vn->context[3] != upper32At(vn->_l3Buffer,vn->l3index)) {
					vn->l3index--;
				}
			}
			else { 
				while(vn->context[3]!=upper32At(vn->_l3Buffer,vn->l3index))
					vn->l3index++;
			}	
	}
}
void resolveLC_l4(VTDNav_L5 *vn){
	int i,temp = lower32At(vn->_l3Buffer,vn->l3index);
	if (vn->l4lower != temp) {
		vn->l4lower = temp;
		// l2lower shouldn't be -1 !!!! l2lower and l2upper always get
		// resolved simultaneously
		vn->l4index = vn->l4lower;
		vn->l4upper = vn->l4Buffer->size - 1;
		for (i = vn->l3index + 1; i < vn->_l3Buffer->size; i++) {
			temp =lower32At(vn->_l3Buffer,i);
			if (temp != -1) {
				vn->l4upper = temp - 1;
				break;
			}
		}
	} // intelligent guess again ??

	if (vn->l4index < 0 || vn->l4index >= vn->l4Buffer->size
		|| vn->context[4] != upper32At(vn->l4Buffer,vn->l4index)) {

			if (vn->l4index >= vn->l4Buffer->size || vn->l4index<0)
				vn->l4index = vn->l4lower;
			if (vn->l4index+1< vn->l4Buffer->size&& vn->context[4] ==upper32At( vn->l4Buffer,vn->l4index + 1))
				vn->l4index = vn->l4index + 1;
			else if (vn->l4upper - vn->l4lower >= 16) {
				int init_guess = vn->l4lower
					+ (int) ((vn->l4upper - vn->l4lower)
					* ((float) vn->context[4] - upper32At(vn->l4Buffer,vn->l4lower)) / 
					(upper32At(vn->l4Buffer, vn->l4upper) - upper32At(vn->l4Buffer,vn->l4lower)));
				if (upper32At(vn->l4Buffer,init_guess) > vn->context[4]) {
					while (vn->context[4] != upper32At(vn->l4Buffer,init_guess))
						init_guess--;
				} else if (upper32At(vn->l4Buffer,init_guess) < vn->context[4]) {
					while (vn->context[4] != upper32At(vn->l4Buffer,init_guess))
						init_guess++;
				}
				vn->l4index = init_guess;
			} else if (vn->context[4]<upper32At(vn->l4Buffer,vn->l4index)){

				while ( vn->context[4] != upper32At(vn->l4Buffer,vn->l4index)) {
					vn->l4index--;
				}
			}
			else { 
				while(vn->context[4]!=upper32At(vn->l4Buffer,vn->l4index))
					vn->l4index++;
			}	
	}
}
void resolveLC_l5(VTDNav_L5 *vn){
	int i,temp = lower32At(vn->l4Buffer,vn->l4index);
	if (vn->l5lower != temp) {
		//l3lower and l3upper are always together
		vn->l5lower = temp;
		// l3lower shouldn't be -1
		vn->l5index = vn->l5lower;
		vn->l5upper = vn->l5Buffer->size - 1;
		for (i = vn->l4index + 1; i < vn->l4Buffer->size; i++) {
			temp = lower32At(vn->l4Buffer,i);
			if (temp != -1) {
				vn->l5upper = temp - 1;
				break;
			}
		}
	}

	if (vn->l5index < 0 || vn->l5index >= vn->l5Buffer->size
		|| vn->context[5] != intAt(vn->l5Buffer,vn->l5index)) {
			if (vn->l5index >= vn->l5Buffer->size || vn->l5index <0)
				vn->l5index = vn->l5lower;
			if (vn->l5index+1 < vn->l5Buffer->size &&
				vn->context[5] == intAt(vn->l5Buffer,vn->l5index + 1))
				vn->l5index = vn->l5index + 1;
			else if (vn->l5upper - vn->l5lower >= 16) {
				int init_guess = vn->l5lower
					+ (int) ((vn->l5upper - vn->l5lower) * ((float) (vn->context[5] - intAt(vn->l5Buffer
					,vn->l5lower)) / (intAt(vn->l5Buffer,vn->l5upper) - intAt(vn->l5Buffer,vn->l5lower))));
				if (intAt(vn->l5Buffer,init_guess) > vn->context[5]) {
					while (vn->context[5] != intAt(vn->l5Buffer,init_guess))
						init_guess--;
				} else if (intAt(vn->l5Buffer,init_guess) < vn->context[5]) {
					while (vn->context[5] != intAt(vn->l5Buffer,init_guess))
						init_guess++;
				}
				vn->l5index = init_guess;
			} else if (vn->context[5]<intAt(vn->l5Buffer,vn->l5index)){
				while (vn->context[5] != intAt(vn->l5Buffer,vn->l5index)) {
					vn->l5index--;
				}
			} else {
				while (vn->context[5] != intAt(vn->l5Buffer,vn->l5index)) {
					vn->l5index++;
				}
			}
	}
}

void _recoverNode_l3(VTDNav_L5 *vn,int index){
		int k,t1,t2,i = lower32At(vn->l2Buffer,vn->l2index);
		
		if (vn->l3lower != i) {
			vn->l3lower = i;
			// l2lower shouldn't be -1 !!!! l2lower and l2upper always get
			// resolved simultaneously
			//l2index = l2lower;
			vn->l3upper = vn->_l3Buffer->size - 1;
			for (k = vn->l2index + 1; k < vn->l2Buffer->size; k++) {
				i = lower32At(vn->l2Buffer,k);
				if (i != -1) {
					vn->l3upper = i - 1;
					break;
				}
			}
		}
		// guess what i would be in l2 cache
		t1=upper32At(vn->_l3Buffer,vn->l3lower);
		t2=upper32At(vn->_l3Buffer,vn->l3upper);
		//System.out.print("   t2  ==>"+t2+"   t1  ==>"+t1);
		i= min(vn->l3lower+ (int)(((float)(index-t1)/(t2-t1+1))*(vn->l3upper-vn->l3lower)),vn->l3upper) ;
		//System.out.print("  i1  "+i);
		while(i<vn->_l3Buffer->size-1 && upper32At(vn->_l3Buffer,i)<index){
			i++;	
		}
		//System.out.println(" ==== i2    "+i+"    index  ==>  "+index);
		
		while (upper32At(vn->_l3Buffer,i)>index && i>0)
			i--;
		vn->context[3] = upper32At(vn->_l3Buffer,i);
		vn->l3index = i;
}

void recoverNode_l4(VTDNav_L5 *vn,int index){
	
	int t1,t2,k,i = lower32At(vn->_l3Buffer,vn->l3index);

	if (vn->l4lower != i) {
		vn->l4lower = i;
		// l2lower shouldn't be -1 !!!! l2lower and l2upper always get
		// resolved simultaneously
		//l2index = l2lower;
		vn->l4upper = vn->l4Buffer->size - 1;
		for (k = vn->l3index + 1; k < vn->_l3Buffer->size; k++) {
			i = lower32At(vn->_l3Buffer,k);
			if (i != -1) {
				vn->l4upper = i - 1;
				break;
			}
		}
	}
	// guess what i would be in l2 cache
	t1=upper32At(vn->l4Buffer,vn->l4lower);
	t2=upper32At(vn->l4Buffer,vn->l4upper);
	//System.out.print("   t2  ==>"+t2+"   t1  ==>"+t1);
	i= min(vn->l4lower+ (int)(((float)(index-t1)/(t2-t1+1))*(vn->l4upper-vn->l4lower)),vn->l4upper) ;
	//System.out.print("  i1  "+i);
	while(i<vn->l4Buffer->size-1 && upper32At(vn->l4Buffer,i)<index){
		i++;	
	}
	//System.out.println(" ==== i2    "+i+"    index  ==>  "+index);

	while (upper32At(vn->l4Buffer,i)>index && i>0)
		i--;
	vn->context[4] = upper32At(vn->l4Buffer,i);
	vn->l4index = i;
}

void recoverNode_l5(VTDNav_L5 *vn,int index){
			int t1,t2,k,i = lower32At(vn->l4Buffer,vn->l4index);
		
		if (vn->l5lower != i) {
			//l3lower and l3upper are always together
			vn->l5lower = i;
			// l3lower shouldn't be -1
			//l3index = l3lower;
			vn->l5upper = vn->l5Buffer->size - 1;
			for (k = vn->l4index + 1; k < vn->l4Buffer->size; k++) {
				i = lower32At(vn->l4Buffer,k);
				if (i != 0xffffffff) {
					vn->l5upper = i - 1;
					break;
				}
			}
		}
		t1=intAt(vn->l5Buffer,vn->l5lower);
		t2=intAt(vn->l5Buffer,vn->l5upper);
		i= min(vn->l5lower+ (int)(((float)(index-t1)/(t2-t1+1))*(vn->l5upper-vn->l5lower)),vn->l5upper) ;
		while(i<vn->l5Buffer->size-1 && intAt(vn->l5Buffer,i)<index){
			i++;	
		}
		while (intAt(vn->l5Buffer,i)>index && i>0)
			i--;
		//System.out.println(" i ===> "+i);
		vn->context[5] = intAt( vn->l5Buffer,i);
		vn->l5index = i;
}

Boolean _writeIndex_VTDNav_L5(VTDNav_L5 *vn, FILE *f){
	return _writeIndex_L5(1, 
                vn->encoding, 
                vn->ns, 
                TRUE, 
				vn->nestingLevel-1, 
                3, 
                vn->rootIndex, 
                vn->XMLDoc, 
                vn->docOffset, 
                vn->docLen, 
				vn->vtdBuffer, 
                vn->l1Buffer, 
                vn->l2Buffer, 
                vn->_l3Buffer, 
				vn->l4Buffer,
				vn->l5Buffer,
                f);
}
Boolean _writeIndex2_VTDNav_L5(VTDNav_L5 *vn, char *fileName){
	FILE *f = NULL;
	Boolean b = FALSE;
	f = fopen(fileName,"wb");
	
	if (f==NULL){
		throwException2(invalid_argument,"fileName not valid");
		return FALSE;
	}
	b = _writeIndex_VTDNav_L5(vn,f);
	fclose(f);
	return b;
}
Boolean _writeSeparateIndex_VTDNav_L5(VTDNav_L5 *vn, char *VTDIndexFile){
	FILE *f = NULL;
	Boolean b = FALSE;
	f = fopen(VTDIndexFile,"wb");
	
	if (f==NULL){
		throwException2(invalid_argument,"fileName not valid");
		//return FALSE;
	}

	b=_writeSeparateIndex_L5( (Byte)2, 
                vn->encoding, 
                vn->ns, 
                TRUE, 
				vn->nestingLevel-1, 
                3, 
                vn->rootIndex, 
                vn->docOffset, 
                vn->docLen, 
				vn->vtdBuffer, 
                vn->l1Buffer, 
                vn->l2Buffer, 
                vn->_l3Buffer, 
				vn->l4Buffer,
				vn->l5Buffer,
                f);
	
	fclose(f);
	return b;
}

Long getOffsetBeforeTail(VTDNav *vn){
		Long l = getElementFragment(vn);
		
		int offset = (int)l;
		int len = (int)(l>>32);
		// byte to char conversion
		if (vn->encoding >= FORMAT_UTF_16BE){
			offset <<= 1;
			len <<=1;
		}
		offset += len;
		if (getCharUnit(vn,offset-2)=='/')
			return 0xffffffff00000000LL | offset-1;
		else{
			offset -= 2;
			while(getCharUnit(vn,offset)!='/'){
		        offset--;	        
		    }
			return offset -1;
		}
		//return 1;
}
/*Boolean writeSeparateIndex_VTDNav(VTDNav *vn, char *vtdIndex){
	return vn->__writeSeparateIndex_VTDNav(vn,vtdIndex);
}*/

/* Write VTD+XML into a file of given name */
/*Boolean writeIndex2_VTDNav(VTDNav *vn, char *fileName){
	return vn->__writeIndex2_VTDNav(vn,fileName);
}*/

/* Write VTD+XML into a FILE pointer */
/*Boolean writeIndex_VTDNav(VTDNav *vn, FILE *f){
	return vn->__writeIndex_VTDNav(vn,f);
}

Boolean toElementNS(VTDNav *vn, navDir direction, UCSChar *URL, UCSChar *ln){
	return vn->__toElementNS(vn,direction,URL, ln);
}

VTDNav* duplicateNav(VTDNav *vn){return vn->__duplicateNav(vn);}

VTDNav* cloneNav(VTDNav *vn){return vn->__cloneNav(vn);}

void recoverNode(VTDNav *vn, int index){vn->__recoverNode(vn,index);}

void resolveLC(VTDNav *vn){
	vn->__resolveLC(vn);
}

Boolean pop(VTDNav *vn){return vn->__pop(vn);}

Boolean pop2(VTDNav *vn){return vn->__pop2(vn);}

Boolean push(VTDNav *vn){return vn->__push(vn);}

Boolean push2(VTDNav *vn){return vn->__push2(vn);}

void sampleState(VTDNav *vn, FastIntBuffer *fib){
	vn->__sampleState(vn,fib);
}

void freeVTDNav(VTDNav *vn){
	vn->__freeVTDNav(vn);
}


Boolean toElement(VTDNav *vn, navDir direction){
	return vn->__toElement(vn,direction);
}


Boolean toElement2(VTDNav *vn, navDir direction, UCSChar *en){
	return vn->__toElement2(vn,direction,en);
}*/

Boolean nodeToElement(VTDNav *vn,int direction){
		switch(direction){
		case NEXT_SIBLING:
			switch (vn->context[0]) {
			case 0:
				if (vn->l1index!=-1){
					vn->context[0]=1;
					vn->context[1]=upper32At(vn->l1Buffer,vn->l1index);
					vn->atTerminal=FALSE;
					return TRUE;
				}else
					return FALSE;
			case 1:
				if (vn->l2index!=-1){
					vn->context[0]=2;
					vn->context[2]=upper32At(vn->l2Buffer,vn->l2index);
					vn->atTerminal=FALSE;
					return TRUE;
				}else
					return FALSE;
				
			case 2:
				if (vn->l3index!=-1){
					vn->context[0]=3;
					vn->context[3]=intAt(vn->l3Buffer,vn->l3index);
					vn->atTerminal=FALSE;
					return TRUE;
				}else
					return FALSE;
			default:{
				int index = vn->LN + 1;
				int size = vn->vtdBuffer->size;
				while (index < size) {
					Long temp = longAt(vn->vtdBuffer,index);
					int token_type =
						(int) ((MASK_TOKEN_TYPE & temp) >> 60)
							& 0xf;

					if (token_type == TOKEN_STARTING_TAG) {
						int depth =
							(int) ((MASK_TOKEN_DEPTH & temp) >> 52);
						if (depth < vn->context[0]) {
							return FALSE;
						} else if (depth == (vn->context[0])) {
							vn->context[vn->context[0]] = index;
							return TRUE;
						}
					}
					index++;
				}
				return FALSE;}
				
			}
		case PREV_SIBLING:
			switch (vn->context[0]) {
			case 0:
				if (vn->l1index!=-1 && vn->l1index>0){
					vn->l1index--;
					vn->context[0]=1;
					vn->context[1]=upper32At(vn->l1Buffer,vn->l1index);
					vn->atTerminal=FALSE;
					return TRUE;					
				}else
					return FALSE;
			case 1:
				if (vn->l2index!=-1 && vn->l2index>vn->l2lower){
					vn->l2index--;
					vn->context[0]=2;
					vn->context[2]=upper32At(vn->l2Buffer,vn->l2index);
					vn->atTerminal=FALSE;
					return TRUE;					
				}else
					return FALSE;
			case 2:
				if (vn->l2index!=-1 && vn->l3index>vn->l3lower){
					vn->l3index--;
					vn->context[0]=3;
					vn->context[3]=intAt(vn->l3Buffer,vn->l3index);
					vn->atTerminal=FALSE;
					return TRUE;					
				}else
					return FALSE;
				
			default:{
				int index = vn->LN -1;
				while (index > vn->context[vn->context[0] - 1]) {
					// scan backforward
					Long temp = longAt(vn->vtdBuffer,index);
					int token_type =
						(int) ((MASK_TOKEN_TYPE & temp) >> 60)
							& 0xf;

					if (token_type == TOKEN_STARTING_TAG) {
						int depth =
							(int) ((MASK_TOKEN_DEPTH & temp) >> 52);
						/*
                         * if (depth < context[0]) { return FALSE; }
                         * else
                         */
						if (depth == (vn->context[0])) {
							vn->context[vn->context[0]] = index;
							return TRUE;
						}
					}
					index--;
				} // what condition
				return FALSE;
					}
			}
		}
		return FALSE;
	}


	Boolean iterate_following_node(VTDNav *vn){
			int index = getCurrentIndex(vn) + 1;
		tokenType tokenType;
		int depth;
		//int size = vtdBuffer.size;
		while (index < vn->vtdSize) {
			tokenType = getTokenType(vn,index);
			switch(tokenType){
			case TOKEN_ATTR_NAME:
			case TOKEN_ATTR_NS:
				index = index + 2;
				continue;
			case TOKEN_STARTING_TAG:
			case TOKEN_DOCUMENT:
				depth = getTokenDepth(vn,index);
				vn->context[0] = depth;
				if (depth > 0)
					vn->context[depth] = index;
				if (depth < vn->maxLCDepthPlusOne)
					resolveLC(vn);
				vn->atTerminal = FALSE;
				return TRUE;	
				
			case TOKEN_CHARACTER_DATA:
			case TOKEN_CDATA_VAL:
			case TOKEN_COMMENT:
			case TOKEN_PI_NAME:
				depth = getTokenDepth(vn,index);
				
				vn->context[0]=depth;
				vn->LN = index;
				vn->atTerminal = TRUE;
				sync(vn,depth,index);
				return TRUE;
			}
			index++;
		}
		return FALSE;	
	}

	Boolean iterate_preceding_node(VTDNav *vn,int a[], int endIndex){
		int index = getCurrentIndex(vn)+1;
		tokenType tokenType;
		int depth;
		//int t,d;
		//int depth = getTokenDepth(index);
		//int size = vtdBuffer.size;
		while (index <  endIndex) {
			tokenType = getTokenType(vn,index);
			switch(tokenType){
			case TOKEN_ATTR_NAME:
			case TOKEN_ATTR_NS:
				index = index + 2;
				continue;
			case TOKEN_STARTING_TAG:
			//case TOKEN_DOCUMENT:
				depth = getTokenDepth(vn,index);
				if (depth>0 && (index!=a[depth])){
					vn->context[0] = depth;
					if (depth > 0)
						vn->context[depth] = index;
					if (depth < vn->maxLCDepthPlusOne)
						resolveLC(vn);
					vn->atTerminal = FALSE;
					return TRUE;	
				}else{
					if (depth > 0)
						vn->context[depth] = index;
					if (depth < vn->maxLCDepthPlusOne)
						resolveLC(vn);
					index++;
					vn->atTerminal = FALSE;
					continue;
				}
			
			case TOKEN_CHARACTER_DATA:
			case TOKEN_CDATA_VAL:
			case TOKEN_COMMENT:
			case TOKEN_PI_NAME:
				depth = getTokenDepth(vn,index);
				
				vn->context[0]=depth;
				vn->LN = index;
				vn->atTerminal = TRUE;
				sync(vn,depth,index);
				return TRUE;
			}
			index++;
		}
		return FALSE;	
	}


	Boolean _toNode(VTDNav *vn, navDir dir){
			int index,tokenType,depth,lastEntry,tmp;
		//count++;
		//System.out.println("count ==>"+ count);
		switch(dir){
		case ROOT:
			if (vn->context[0] != 0) {
				/*
				 * for (int i = 1; i <= context[0]; i++) { context[i] =
				 * 0xffffffff; }
				 */
				vn->context[0] = 0;
			}
			vn->atTerminal = FALSE;
			//l1index = l2index = l3index = -1;
			return TRUE;
		case PARENT:
			if (vn->atTerminal == TRUE){
				vn->atTerminal = FALSE;
				return TRUE;
			}
			if (vn->context[0] > 0) {
				//context[context[0]] = context[context[0] + 1] =
                // 0xffffffff;
				vn->context[vn->context[0]] = -1;
				vn->context[0]--;
				return TRUE;
			}else if (vn->context[0]==0){
				vn->context[0]=-1; //to be compatible with XPath Data model
				return TRUE;
				}
			else {
				return FALSE;
			}
		case FIRST_CHILD:
			if(vn->atTerminal)return FALSE;
			switch (vn->context[0]) {
			case -1:
				//starting with root element
				//scan backward, if there is a pi | comment node
				index = vn->rootIndex-1;
				
				while(index >0){
					tokenType = getTokenType(vn,index);
					switch(tokenType){
					case TOKEN_COMMENT: index--; break;
					case TOKEN_PI_VAL:  index-=2;break;
					default:
						goto loop1;
					}
				}
loop1:
				index++; // points to
				if (index!=vn->rootIndex){
					vn->atTerminal = TRUE;
					vn->LN = index;
				}else{
					vn->context[0]=0;
				}
				return TRUE;
			case 0:
				if (vn->l1Buffer->size!=0){
					index = upper32At(vn->l1Buffer,0)-1;
					//rewind
					 while(index>vn->rootIndex){
						tokenType = getTokenType(vn,index);
						switch(tokenType){
						case TOKEN_CHARACTER_DATA:
						case TOKEN_COMMENT:
						case TOKEN_CDATA_VAL:
							index--;
							break;
						case TOKEN_PI_VAL:
							index-=2;
							break;
						default:
							goto loop2;
						}
					}
loop2:
					index++;
					vn->l1index = 0;	
					if(index == upper32At(vn->l1Buffer,0)){
						vn->context[0]=1;
						vn->context[1]= upper32At(vn->l1Buffer,0);
						vn->atTerminal = FALSE;				
					}else {
						vn->atTerminal = TRUE;
						vn->LN = index;						
					}
					return TRUE;
					
				}else{					
					//get to the first non-attr node after the starting tag
					index = vn->rootIndex+1;
					while(index<vn->vtdSize){
						tokenType = getTokenType(vn,index);
						switch(tokenType){
						case TOKEN_ATTR_NAME:
						case TOKEN_ATTR_NS:
							index+=2;
							break;
						default:
							if (getTokenDepth(vn,index)==0){
								vn->atTerminal = TRUE;
								vn->LN = index;
								return TRUE;
							}else
								return FALSE;
								
						}
					}
					return FALSE;
				}
								
			case 1: 
				if (lower32At(vn->l1Buffer,vn->l1index)!=-1){
					// l2upper and l2lower
					vn->l2lower = lower32At(vn->l1Buffer,vn->l1index);
					tmp = vn->l1index+1;
					vn->l2upper = vn->l2Buffer->size-1;
					while(tmp<vn->l1Buffer->size){
						if (lower32At(vn->l1Buffer,tmp)!=-1){
							vn->l2upper = lower32At(vn->l1Buffer,tmp)-1;
							break;
						}else
							tmp++;
					}
					//if (tmp==l1Buffer.size){
					//	l2upper = l2Buffer.size-1;
					//}					
					index = vn->context[1]+1;
					tmp = upper32At(vn->l2Buffer,vn->l2lower);
					while(index<tmp){
						tokenType = getTokenType(vn,index);
						switch(tokenType){
						case TOKEN_ATTR_NAME:
						case TOKEN_ATTR_NS:
							index+=2;
							break;
						default:
							vn->l2index = vn->l2lower;
							vn->atTerminal = TRUE;
							vn->LN = index;
							return TRUE;															
						}
					}
					vn->l2index = vn->l2lower;
					vn->context[0] = 2;
					vn->context[2] = index;
					return TRUE;				
				}else{
					index = vn->context[1]+1;
					while(index<vn->vtdSize){
						tokenType = getTokenType(vn,index);
						switch(tokenType){
						case TOKEN_ATTR_NAME:
						case TOKEN_ATTR_NS:
							index+=2;
							break;
						default:
							if (getTokenDepth(vn,index)==1 && getTokenType(vn,index)!=TOKEN_STARTING_TAG){
								vn->atTerminal = TRUE;
								vn->LN = index;
								return TRUE;
							}else
								return FALSE;
								
						}
					}
					return FALSE;
				}
				
			case 2:
				if (lower32At(vn->l2Buffer,vn->l2index)!=-1){
					// l2upper and l2lower
					vn->l3lower = lower32At(vn->l2Buffer,vn->l2index);
					tmp = vn->l2index+1;
					vn->l3upper = vn->l3Buffer->size-1;
					while(tmp<vn->l2Buffer->size){
						if (lower32At(vn->l2Buffer,tmp)!=-1){
							vn->l3upper = lower32At(vn->l2Buffer,tmp)-1;
							break;
						}else
							tmp++;
					}
					//if (tmp==l2Buffer.size){
					//	l3upper = l3Buffer.size-1;
					//}					
					index = vn->context[2]+1;
					tmp = intAt(vn->l3Buffer,vn->l3lower);
					while(index<tmp){
						tokenType = getTokenType(vn,index);
						switch(tokenType){
						case TOKEN_ATTR_NAME:
						case TOKEN_ATTR_NS:
							index+=2;
							break;
						default:
							vn->l3index = vn->l3lower;
							vn->atTerminal = TRUE;
							vn->LN = index;
							return TRUE;															
						}
					}
					vn->l3index = vn->l3lower;
					vn->context[0] = 3;
					vn->context[3] = index;
					return TRUE;				
				}else{
					index = vn->context[2]+1;
					while(index<vn->vtdSize){
						tokenType = getTokenType(vn,index);
						switch(tokenType){
						case TOKEN_ATTR_NAME:
						case TOKEN_ATTR_NS:
							index+=2;
							break;
						default:
							if (getTokenDepth(vn,index)==2 && getTokenType(vn,index)!=TOKEN_STARTING_TAG){
								vn->atTerminal = TRUE;
								vn->LN = index;
								return TRUE;
							}else
								return FALSE;
								
						}
					}
					return FALSE;
				}				
			default:				
				index = vn->context[vn->context[0]] + 1;
				while (index < vn->vtdBuffer->size) {
					Long temp = longAt(vn->vtdBuffer,index);
					tokenType =
						(int) ((MASK_TOKEN_TYPE & temp) >> 60)&0x0f;
					switch(tokenType){
					case TOKEN_STARTING_TAG:
						depth =
							(int) ((MASK_TOKEN_DEPTH & temp) >> 52);
						if (depth <= vn->context[0]){
							return FALSE;
						}else if (depth == (vn->context[0] + 1)) {
							vn->context[0] += 1;
							vn->context[vn->context[0]] = index;
							return TRUE;
						}else
							throwException2(nav_exception,"impossible condition");
						
					case TOKEN_ATTR_NAME:
					case TOKEN_ATTR_NS: index+=2;break;
					case TOKEN_CHARACTER_DATA:
					case TOKEN_COMMENT:
					case TOKEN_CDATA_VAL:
						depth =
							(int) ((MASK_TOKEN_DEPTH & temp) >> 52);
						if (depth < vn->context[0]){
							return FALSE;
						}else if (depth == (vn->context[0])) {
							//System.out.println("inside to Node next sibling");
							vn->LN = index;
							vn->atTerminal = TRUE;
							return TRUE;
						} else
							throwException2(nav_exception,"impossible condition");
					case TOKEN_PI_NAME:
						depth =
							(int) ((MASK_TOKEN_DEPTH & temp) >> 52);
						if (depth < vn->context[0]){
							return FALSE;
						}else if (depth == (vn->context[0])) {
							vn->LN = index;
							vn->atTerminal = TRUE;
							return TRUE;
						} else
							throwException2(nav_exception,"impossible condition");
 					}
					//index++;
				} // what condition
				return FALSE;
			}
		case LAST_CHILD:
			if(vn->atTerminal)return FALSE;
			return toNode_LastChild(vn);
			
		case NEXT_SIBLING:
			switch (vn->context[0]) {
			case -1:
				if(vn->atTerminal){
					index = vn->LN;
					tokenType = getTokenType(vn,index);
					switch(tokenType){
					case TOKEN_PI_NAME: 
						index+=2;
						break;
						//break loop2;
					case TOKEN_COMMENT:
						index++;
						break;
					}
					
					if (index <vn->vtdSize){
						tokenType = getTokenType(vn,index);
						depth = getTokenDepth(vn,index);
						if (depth == -1){
							vn->LN = index;
							return TRUE;
						}else{
							vn->atTerminal = FALSE;
							vn->context[0]=0;
							return TRUE;
								// depth has to be zero
						}						
					}else
						return FALSE;
					
				}else{
					return FALSE;
				}
				//break;
			case 0:
				if(vn->atTerminal){
					index = vn->LN;
					tokenType = getTokenType(vn,vn->LN);
					if (tokenType==TOKEN_ATTR_NAME)
						return FALSE;
					//index++;
					if (vn->l1Buffer->size!=0){
						if (index <upper32At( vn->l1Buffer,vn->l1index)){
							index++;
							if (tokenType==TOKEN_PI_NAME)
								index++;
							if (index <= upper32At(vn->l1Buffer,vn->l1index)){
								if (index ==upper32At( vn->l1Buffer,vn->l1index)){
									vn->atTerminal = FALSE;
									vn->context[0]=1;
									vn->context[1]=index;
									return TRUE;
								}
								depth = getTokenDepth(vn,index);
								if (depth!=0)
									return FALSE;
								vn->LN = index;
								vn->atTerminal = TRUE;
								return TRUE;
							}else{
								return FALSE;
							}
						}else if ( vn->l1index < vn->l1Buffer->size -1){ // whether l1index is the last entry in l1 buffer
							vn->l1index++;
							if (tokenType==TOKEN_PI_NAME)
								index++;
							if (index <= upper32At(vn->l1Buffer,vn->l1index)){
								if (index == upper32At(vn->l1Buffer,vn->l1index)){
									vn->atTerminal = FALSE;
									vn->context[0]=1;
									vn->context[1]=index;
									return TRUE;
								}
								depth = getTokenDepth(vn,index);
								if (depth!=0)
									return FALSE;
								vn->LN = index;
								vn->atTerminal = TRUE;
								return TRUE;
							}else{
								return FALSE;
							}
						}else{
							index++;
							if (tokenType==TOKEN_PI_NAME)
								index++;
							if (index < vn->vtdSize){
								depth = getTokenDepth(vn,index);
								if (depth!=0)
									return FALSE;
								vn->LN = index;
								vn->atTerminal = TRUE;
								return TRUE;
							}else{
								return FALSE;
							}
						}						
					}else{
						index++;
						if (tokenType==TOKEN_PI_NAME)
							index++;
						if (index < vn->vtdSize){
							depth = getTokenDepth(vn,index);
							if (depth!=0)
								return FALSE;
							vn->LN = index;
							vn->atTerminal = TRUE;
							return TRUE;
						}else{
							return FALSE;
						}
					}
					
				}else{
					index = vn->vtdSize-1;
					depth = -2;
					// get to the end, then rewind
					while(index > vn->rootIndex){
						depth = getTokenDepth(vn,index);
						if (depth ==-1){
							index--;
						} else
							break;								
					}			
					index++;
					if (index>=vn->vtdSize )
						return FALSE;
					else{
						vn->context[0]=-1;
						vn->LN = index;
						vn->atTerminal = TRUE;
						return TRUE;
					}
				}
				//break;
			case 1:
				if(vn->atTerminal){
					tokenType=getTokenType(vn,vn->LN);
					if (tokenType==TOKEN_ATTR_NAME)
						return FALSE;
					if (lower32At(vn->l1Buffer,vn->l1index) != -1) {
						if (vn->LN < upper32At(vn->l2Buffer,vn->l2upper)) {
							tmp = upper32At(vn->l2Buffer,vn->l2index);
							index = vn->LN + 1;
							if (tokenType == TOKEN_PI_NAME)
								index++;

							if (index < tmp) {
								vn->LN = index;
								return TRUE;
							} else {
								vn->context[0] = 2;
								vn->context[2] = tmp;
								vn->atTerminal = FALSE;
								return TRUE;
							}
						} else {
							index = vn->LN + 1;
							if (tokenType == TOKEN_PI_NAME)
								index++;
							if (index < vn->vtdSize) {
								depth = getTokenDepth(vn,index);
								if (depth==1 && getTokenType(vn,index)!= TOKEN_STARTING_TAG){
									vn->LN = index;
									vn->atTerminal = TRUE;
									return TRUE;
								}
								return FALSE;
							} else
								return FALSE;
						}						
					}else{
						index= vn->LN+1;
						if (tokenType==TOKEN_PI_NAME)
							index++;
						if (index < vn->vtdSize){
							depth = getTokenDepth(vn,index);
							if (depth==1 && getTokenType(vn,index)!= TOKEN_STARTING_TAG){
								vn->LN = index;
								vn->atTerminal = TRUE;
								return TRUE;
							}
							return FALSE;
						}else{
							return FALSE;
						}
					}					
				}else{
					if (vn->l1index != vn->l1Buffer->size-1){
						// not the last one
						//rewind
						vn->l1index++;
						index = lastEntry = upper32At(vn->l1Buffer,vn->l1index)-1;
						while(getTokenDepth(vn,index)==0){
							index--;
						}
						if (lastEntry==index){
							vn->atTerminal=FALSE;
							vn->context[0]=1;
							vn->context[1]=index+1;
							return TRUE;
						} else {
							vn->atTerminal = TRUE;
							vn->context[0]=0;
							vn->LN = index+1;
							return TRUE;							
						}
					}else{
						index = vn->vtdSize-1;
						while(index > upper32At(vn->l1Buffer,vn->l1index) && getTokenDepth(vn,index)<=0){
							index--;
						}
						
						if (index == vn->vtdSize-1 ){
							if (getTokenDepth(vn,index)==0){
								vn->context[0]=0;
								vn->LN = index;
								vn->atTerminal = TRUE;
								return TRUE;
							}else
								return FALSE;
						}
						index++;
						if (getTokenDepth(vn,index)==0){
							vn->context[0]=0;
							vn->LN = index;
							vn->atTerminal = TRUE;
							return TRUE;
						}else{
							return FALSE;
						}
					}
				}
				
			case 2:
				if(vn->atTerminal){
					tokenType=getTokenType(vn,vn->LN);
					if (tokenType==TOKEN_ATTR_NAME)
						return FALSE;
					if (lower32At(vn->l2Buffer,vn->l2index) != -1) {
						if (vn->LN <intAt( vn->l3Buffer,vn->l3upper)) {
							tmp =intAt(vn->l3Buffer,vn->l3index);
							index = vn->LN + 1;
							if (tokenType == TOKEN_PI_NAME)
								index++;

							if (index < tmp) {
								vn->LN = index;
								return TRUE;
							} else {
								vn->context[0] = 3;
								vn->context[3] = tmp;
								vn->atTerminal = FALSE;
								return TRUE;
							}
						} else {
							index = vn->LN + 1;
							if (tokenType == TOKEN_PI_NAME)
								index++;
							if (index < vn->vtdSize) {
								depth = getTokenDepth(vn,index);
								if (depth==2 && getTokenType(vn,index)!=TOKEN_STARTING_TAG){									
									vn->LN = index;
									return TRUE;
								}
								return FALSE;
							} 
							return FALSE;
						}						
					}else{
						index= vn->LN+1;
						if (tokenType==TOKEN_PI_NAME)
							index++;
						if (index <vn->vtdSize){
							depth = getTokenDepth(vn,index);
							if (depth==2 && getTokenType(vn,index)!= TOKEN_STARTING_TAG){
								vn->LN = index;
								vn->atTerminal = TRUE;
								return TRUE;
							}
							return FALSE;
						}else{
							return FALSE;
						}
					}					
				}else{
					//l2index < l2upper
					if (vn->l2index< vn->l2upper){
						tmp = upper32At(vn->l2Buffer,vn->l2index);
						vn->l2index++;
						lastEntry = index = upper32At(vn->l2Buffer,vn->l2index)-1;
						//rewind
						while(index>tmp){
							if (getTokenDepth(vn,index)==1){
								tokenType = getTokenType(vn,index);
								switch(tokenType){
								case TOKEN_CHARACTER_DATA:
								case TOKEN_COMMENT:
								case TOKEN_CDATA_VAL:
									index--;
									break;
								case TOKEN_PI_VAL:
									index = index -2;
									break;
								default: goto loop22;
								}
							}else
								goto loop22;
						}
loop22: 
						if (index == lastEntry){
							vn->context[0]=2;
							vn->context[2] = index+1;
							return TRUE;
						}
						vn->context[0]=1;
						vn->LN = index+1;
						vn->atTerminal = TRUE;
						return TRUE;						
					}else{
						lastEntry = index = vn->vtdSize-1;
						if (vn->l1index!=vn->l1Buffer->size-1){
							lastEntry = index = upper32At(vn->l1Buffer,vn->l1index+1)-1;
						}
						tmp = upper32At(vn->l2Buffer,vn->l2upper);// pointing to last level 2 element
						
						//rewind
						while(index>tmp){
							if (getTokenDepth(vn,index)<2)
								index--;
							else
								break;
						}
						
						if (( /*lastEntry!=index &&*/ getTokenDepth(vn,index)==1)){
							vn->LN = index;
							vn->atTerminal = TRUE;
							vn->context[0]=1;
							return TRUE;
						}
						
						if (/*getTokenDepth(index+1)==1 
								&& getTokenType(index+1)!= TOKEN_STARTING_TAG 
								&&index !=tmp+1*/
							lastEntry!=index && getTokenDepth(vn,index+1)==1){ //index has moved							
							vn->LN = index+1;
							vn->atTerminal = TRUE;
							vn->context[0]=1;
							return TRUE;
						}
						
						return FALSE;
					}
					
				}
				//break;
			case 3:
				if(!vn->atTerminal){
					//l2index < l2upper
					if (vn->l3index< vn->l3upper){
						//System.out.println(l3index+"  "+l3upper+" "+l3lower+" "+l3Buffer.size+" ");
						tmp = intAt(vn->l3Buffer,vn->l3index);
						vn->l3index++;
						//lastEntry = index = vtdSize-1;
						//if (l3index <l3Buffer.size-1){
						lastEntry = index = intAt(vn->l3Buffer,vn->l3index)-1;
						//}
						//rewind
						while(index>tmp){
							if (getTokenDepth(vn,index)==2){
								tokenType = getTokenType(vn,index);
								switch(tokenType){
								case TOKEN_CHARACTER_DATA:
								case TOKEN_COMMENT:
								case TOKEN_CDATA_VAL:
									index--;
									break;
								case TOKEN_PI_VAL:
									index = index -2;
									break;
								default:
									goto loop222;
								}
							}else
								goto loop222;
						}
loop222:
						if (index == lastEntry){
							vn->context[0]=3;
							vn->context[3] = index+1;
							return TRUE;
						}
						vn->context[0]=2;
						vn->LN = index+1;
						vn->atTerminal = TRUE;
						return TRUE;						
					}else{
						lastEntry = index = vn->vtdSize-1;
						
						if (vn->l1index != vn->l1Buffer->size-1){
							lastEntry=index = upper32At(vn->l1Buffer,vn->l1index+1)-1;
						}
						
						if (vn->l2index != vn->l2Buffer->size-1 && vn->l2index != vn->l2upper){
							lastEntry=index = upper32At(vn->l2Buffer,vn->l2index+1)-1;
						}
						// inser here
						tmp = intAt(vn->l3Buffer,vn->l3index);
						
						//rewind
						while(index>tmp){
							if (getTokenDepth(vn,index)<3)
								index--;
							else
								break;
						}
						if ((/*lastEntry==index &&*/ getTokenDepth(vn,index)==2)){
							vn->LN = index;
							vn->atTerminal = TRUE;
							vn->context[0]=2;
							return TRUE;
						}
						
						if (lastEntry!=index && getTokenDepth(vn,index+1)==2){
							vn->LN = index+1;
							vn->atTerminal = TRUE;
							vn->context[0]=2;
							return TRUE;
						}
						
						return FALSE;
					}
					
				}
				//break;
			default:
				if (vn->atTerminal){
					tokenType=getTokenType(vn,vn->LN);
					if (tokenType==TOKEN_ATTR_NAME)
						return FALSE;
					index = vn->LN+1;
					tmp = vn->context[0]+1;
				}
				else{
					index = vn->context[vn->context[0]] + 1;
					tmp = vn->context[0];
				}
				while (index < vn->vtdSize) {
					Long temp = longAt(vn->vtdBuffer,index);
					tokenType = (int) ((MASK_TOKEN_TYPE & temp) >> 60) & 0x0f;
					depth = (int) ((MASK_TOKEN_DEPTH & temp) >> 52);
					switch (tokenType) {
					case TOKEN_STARTING_TAG:						
						if (depth < tmp) {
							return FALSE;
						} else if (depth == tmp) {
							vn->context[0]=tmp;
							vn->context[vn->context[0]] = index;
							vn->atTerminal = FALSE;
							return TRUE;
						}else 
							index++;
						break;
					case TOKEN_ATTR_NAME:
					case TOKEN_ATTR_NS:
						index += 2;
						break;
					case TOKEN_CHARACTER_DATA:
					case TOKEN_COMMENT:
					case TOKEN_CDATA_VAL:
						//depth = (int) ((MASK_TOKEN_DEPTH & temp) >> 52);
						if (depth < tmp-1) {
							return FALSE;
						} else if (depth == tmp-1) {
							vn->LN = index;
							vn->context[0]= tmp-1;
							vn->atTerminal = TRUE;
							return TRUE;
						} else
							index++;
						break;
					case TOKEN_PI_NAME:
						//depth = (int) ((MASK_TOKEN_DEPTH & temp) >> 52);
						if (depth < tmp-1) {
							return FALSE;
						} else if (depth == tmp-1) {
							vn->LN = index;
							vn->context[0]= tmp-1;
							vn->atTerminal = TRUE;
							return TRUE;
						} else
							index += 2;
						break;
					default:
						index++;
					}
					
				}
				return FALSE;
			}		
		case PREV_SIBLING:
			return toNode_PrevSibling(vn);
		default :
			throwException2( nav_exception,"illegal navigation options");
		}	
		return FALSE;
	}

	Boolean _toNode_L5(VTDNav_L5 *vn, navDir dir){
			int index,tokenType,depth,lastEntry,tmp;
		//count++;
		//System.out.println("count ==>"+ count);
		switch(dir){
		case ROOT:
			if (vn->context[0] != 0) {
				/*
				 * for (int i = 1; i <= context[0]; i++) { context[i] =
				 * 0xffffffff; }
				 */
				vn->context[0] = 0;
			}
			vn->atTerminal = FALSE;
			//l1index = l2index = l3index = l2lower=l2upper=l3lower=l3upper=l4lower=l4upper=l5lower=l5upper=-1;
			return TRUE;
		case PARENT:
			if (vn->atTerminal == TRUE){
				vn->atTerminal = FALSE;
				return TRUE;
			}
			if (vn->context[0] > 0) {
				//context[context[0]] = context[context[0] + 1] =
                // 0xffffffff;
				vn->context[vn->context[0]] = -1;
				vn->context[0]--;
				return TRUE;
			}else if (vn->context[0]==0){
				vn->context[0]=-1; //to be compatible with XPath Data model
				return TRUE;
				}
			else {
				return FALSE;
			}
		case FIRST_CHILD:
			if(vn->atTerminal)return FALSE;
			switch (vn->context[0]) {
			case -1:
				//starting with root element
				//scan backward, if there is a pi | comment node
				index = vn->rootIndex-1;
				
				while(index >0){
					tokenType = getTokenType((VTDNav *)vn,index);
					switch(tokenType){
					case TOKEN_COMMENT: index--; break;
					case TOKEN_PI_VAL:  index-=2;break;
					default:
						goto loop11;
					}
				}
loop11:
				index++; // points to
				if (index!=vn->rootIndex){
					vn->atTerminal = TRUE;
					vn->LN = index;
				}else{
					vn->context[0]=0;
				}
				return TRUE;
			case 0:
				if (vn->l1Buffer->size!=0){
					index = upper32At(vn->l1Buffer,0)-1;
					//rewind
					 while(index>vn->rootIndex){
						tokenType = getTokenType((VTDNav *)vn,index);
						switch(tokenType){
						case TOKEN_CHARACTER_DATA:
						case TOKEN_COMMENT:
						case TOKEN_CDATA_VAL:
							index--;
							break;
						case TOKEN_PI_VAL:
							index-=2;
							break;
						default:
							goto loop1;
						}
					}
					 loop1:
					index++;
					vn->l1index = 0;	
					if(index == upper32At(vn->l1Buffer,0)){
						vn->context[0]=1;
						vn->context[1]=upper32At(vn->l1Buffer,0);
						vn->atTerminal = FALSE;				
					}else {
						vn->atTerminal = TRUE;
						vn->LN = index;						
					}
					return TRUE;
					
				}else{					
					//get to the first non-attr node after the starting tag
					index = vn->rootIndex+1;
					while(index<vn->vtdSize){
						tokenType = getTokenType((VTDNav *)vn,index);
						switch(tokenType){
						case TOKEN_ATTR_NAME:
						case TOKEN_ATTR_NS:
							index+=2;
							break;
						default:
							if (getTokenDepth((VTDNav *)vn,index)==0){
								vn->atTerminal = TRUE;
								vn->LN = index;
								return TRUE;
							}else
								return FALSE;
								
						}
					}
					return FALSE;
				}
								
			case 1: 
				if (lower32At(vn->l1Buffer,vn->l1index)!=-1){
					// l2upper and l2lower
					vn->l2lower = lower32At(vn->l1Buffer,vn->l1index);
					tmp = vn->l1index+1;
					while(tmp<vn->l1Buffer->size){
						if (lower32At(vn->l1Buffer,tmp)!=-1){
							vn->l2upper = lower32At(vn->l1Buffer,tmp)-1;
							break;
						}else
							tmp++;
					}
					if (tmp==vn->l1Buffer->size){
						vn->l2upper = vn->l2Buffer->size-1;
					}					
					index = vn->context[1]+1;
					tmp = upper32At(vn->l2Buffer,vn->l2lower);
					while(index<tmp){
						tokenType = getTokenType((VTDNav *)vn,index);
						switch(tokenType){
						case TOKEN_ATTR_NAME:
						case TOKEN_ATTR_NS:
							index+=2;
							break;
						default:
							vn->l2index = vn->l2lower;
							vn->atTerminal = TRUE;
							vn->LN = index;
							return TRUE;															
						}
					}
					vn->l2index = vn->l2lower;
					vn->context[0] = 2;
					vn->context[2] = index;
					return TRUE;				
				}else{
					index = vn->context[1]+1;
					while(index<vn->vtdSize){
						tokenType = getTokenType((VTDNav *)vn,index);
						switch(tokenType){
						case TOKEN_ATTR_NAME:
						case TOKEN_ATTR_NS:
							index+=2;
							break;
						default:
							if (getTokenDepth((VTDNav *)vn,index)==1 && getTokenType((VTDNav *)vn,index)!=TOKEN_STARTING_TAG){
								vn->atTerminal = TRUE;
								vn->LN = index;
								return TRUE;
							}else
								return FALSE;
								
						}
					}
					return FALSE;
				}
				
			case 2:
				if (lower32At(vn->l2Buffer,vn->l2index)!=-1){
					// l2upper and l2lower
					vn->l3lower = lower32At(vn->l2Buffer,vn->l2index);
					tmp = vn->l2index+1;
					while(tmp<vn->l2Buffer->size){
						if (lower32At(vn->l2Buffer,tmp)!=-1){
							vn->l3upper =lower32At( vn->l2Buffer,tmp)-1;
							break;
						}else
							tmp++;
					}
					if (tmp==vn->l2Buffer->size){
						vn->l3upper = vn->_l3Buffer->size-1;
					}					
					index = vn->context[2]+1;
					tmp = upper32At(vn->_l3Buffer,vn->l3lower);
					while(index<tmp){
						tokenType = getTokenType((VTDNav *)vn,index);
						switch(tokenType){
						case TOKEN_ATTR_NAME:
						case TOKEN_ATTR_NS:
							index+=2;
							break;
						default:
							vn->l3index = vn->l3lower;
							vn->atTerminal = TRUE;
							vn->LN = index;
							return TRUE;															
						}
					}
					vn->l3index = vn->l3lower;
					vn->context[0] = 3;
					vn->context[3] = index;
					return TRUE;				
				}else{
					index = vn->context[2]+1;
					while(index<vn->vtdSize){
						tokenType = getTokenType((VTDNav *)vn,index);
						switch(tokenType){
						case TOKEN_ATTR_NAME:
						case TOKEN_ATTR_NS:
							index+=2;
							break;
						default:
							if (getTokenDepth((VTDNav *)vn,index)==2 && getTokenType((VTDNav *)vn,index)!=TOKEN_STARTING_TAG){
								vn->atTerminal = TRUE;
								vn->LN = index;
								return TRUE;
							}else
								return FALSE;
								
						}
					}
					return FALSE;
				}		
			case 3:
				if (lower32At(vn->_l3Buffer,vn->l3index)!=-1){
				// l2upper and l2lower
				vn->l4lower = lower32At(vn->_l3Buffer,vn->l3index);
				tmp = vn->l3index+1;
				while(tmp<vn->_l3Buffer->size){
					if (lower32At(vn->_l3Buffer,tmp)!=-1){
						vn->l4upper = lower32At(vn->_l3Buffer,tmp)-1;
						break;
					}else
						tmp++;
				}
				if (tmp==vn->_l3Buffer->size){
					vn->l4upper = vn->l4Buffer->size-1;
				}					
				index = vn->context[3]+1;
				tmp = upper32At(vn->l4Buffer,vn->l4lower);
				while(index<tmp){
					tokenType = getTokenType((VTDNav *)vn,index);
					switch(tokenType){
					case TOKEN_ATTR_NAME:
					case TOKEN_ATTR_NS:
						index+=2;
						break;
					default:
						vn->l4index = vn->l4lower;
						vn->atTerminal = TRUE;
						vn->LN = index;
						return TRUE;															
					}
				}
				vn->l4index = vn->l4lower;
				vn->context[0] = 4;
				vn->context[4] = index;
				return TRUE;				
			}else{
				index = vn->context[3]+1;
				while(index<vn->vtdSize){
					tokenType = getTokenType((VTDNav *)vn,index);
					switch(tokenType){
					case TOKEN_ATTR_NAME:
					case TOKEN_ATTR_NS:
						index+=2;
						break;
					default:
						if (getTokenDepth((VTDNav *)vn,index)==3 && getTokenType((VTDNav *)vn,index)!=TOKEN_STARTING_TAG){
							vn->atTerminal = TRUE;
							vn->LN = index;
							return TRUE;
						}else
							return FALSE;
							
					}
				}
				return FALSE;
			}		
			case 4:
				if (lower32At(vn->l4Buffer,vn->l4index)!=-1){
				// l2upper and l2lower
				vn->l5lower = lower32At(vn->l4Buffer,vn->l4index);
				tmp = vn->l4index+1;
				while(tmp<vn->l4Buffer->size){
					if (lower32At(vn->l4Buffer,tmp)!=-1){
						vn->l5upper = lower32At(vn->l4Buffer,tmp)-1;
						break;
					}else
						tmp++;
				}
				if (tmp==vn->l4Buffer->size){
					vn->l5upper = vn->l5Buffer->size-1;
				}					
				index = vn->context[4]+1;
				tmp =intAt( vn->l5Buffer,vn->l5lower);
				while(index<tmp){
					tokenType = getTokenType((VTDNav *)vn,index);
					switch(tokenType){
					case TOKEN_ATTR_NAME:
					case TOKEN_ATTR_NS:
						index+=2;
						break;
					default:
						vn->l5index = vn->l5lower;
						vn->atTerminal = TRUE;
						vn->LN = index;
						return TRUE;															
					}
				}
				vn->l5index = vn->l5lower;
				vn->context[0] = 5;
				vn->context[5] = index;
				return TRUE;				
			}else{
				index = vn->context[4]+1;
				while(index<vn->vtdSize){
					tokenType = getTokenType((VTDNav *)vn,index);
					switch(tokenType){
					case TOKEN_ATTR_NAME:
					case TOKEN_ATTR_NS:
						index+=2;
						break;
					default:
						if (getTokenDepth((VTDNav *)vn,index)==4 && getTokenType((VTDNav *)vn,index)!=TOKEN_STARTING_TAG){
							vn->atTerminal = TRUE;
							vn->LN = index;
							return TRUE;
						}else
							return FALSE;
							
					}
				}
				return FALSE;
			}		
				
			default:				
				index = vn->context[vn->context[0]] + 1;
				while (index < vn->vtdBuffer->size) {
					Long temp = longAt(vn->vtdBuffer,index);
					tokenType =
						(int) ((MASK_TOKEN_TYPE & temp) >> 60) & 0x0f;
					switch(tokenType){
					case TOKEN_STARTING_TAG:
						depth =
							(int) ((MASK_TOKEN_DEPTH & temp) >> 52);
						if (depth <= vn->context[0]){
							return FALSE;
						}else if (depth == (vn->context[0] + 1)) {
							vn->context[0] += 1;
							vn->context[vn->context[0]] = index;
							return TRUE;
						}else
							throwException2(nav_exception,"impossible condition");
					case TOKEN_ATTR_NAME:
					case TOKEN_ATTR_NS: index+=2;break;
					case TOKEN_CHARACTER_DATA:
					case TOKEN_COMMENT:
					case TOKEN_CDATA_VAL:
						depth =
							(int) ((MASK_TOKEN_DEPTH & temp) >> 52);
						if (depth < vn->context[0]){
							return FALSE;
						}else if (depth == (vn->context[0])) {
							vn->LN = index;
							vn->atTerminal = TRUE;
							return TRUE;
						} else 
							index++;
					case TOKEN_PI_NAME:
						depth =
							(int) ((MASK_TOKEN_DEPTH & temp) >> 52);
						if (depth < vn->context[0]){
							return FALSE;
						}else if (depth == (vn->context[0])) {
							vn->LN = index;
							vn->atTerminal = TRUE;
							return TRUE;
						} else 
							index+=2;
 					}
					//index++;
				} // what condition
				return FALSE;
			}
		case LAST_CHILD:
			if(vn->atTerminal)return FALSE;
			return toNode_LastChild_L5((VTDNav_L5 *)vn);
			
		case NEXT_SIBLING:
			switch (vn->context[0]) {
			case -1:
				if(vn->atTerminal){
					index = vn->LN;
					tokenType = getTokenType((VTDNav *)vn,index);
					switch(tokenType){
					case TOKEN_PI_NAME: 
						index+=2;
						break;
						//break loop2;
					case TOKEN_COMMENT:
						index++;
						break;
					}
					
					if (index <vn->vtdSize){
						tokenType = getTokenType((VTDNav *)vn,index);
						depth = getTokenDepth((VTDNav *)vn,index);
						if (depth == -1){
							vn->LN = index;
							return TRUE;
						}else{
							vn->atTerminal = FALSE;
							vn->context[0]=0;
							return TRUE;
								// depth has to be zero
						}						
					}else
						return FALSE;
					
				}else{
					return FALSE;
				}
				//break;
			case 0:
				if(vn->atTerminal){
					index = vn->LN;
					tokenType=getTokenType((VTDNav *)vn,vn->LN);
					if (tokenType==TOKEN_ATTR_NAME)
						return FALSE;
					//index++;
					if (vn->l1Buffer->size!=0){
						if (index < upper32At(vn->l1Buffer,vn->l1index)){
							index++;
							if (tokenType==TOKEN_PI_NAME)
								index++;
							if (index <= upper32At(vn->l1Buffer,vn->l1index)){
								if (index == upper32At(vn->l1Buffer,vn->l1index)){
									vn->atTerminal = FALSE;
									vn->context[0]=1;
									vn->context[1]=index;
									return TRUE;
								}
								depth = getTokenDepth((VTDNav *)vn,index);
								if (depth!=0)
									return FALSE;
								vn->LN = index;
								vn->atTerminal = TRUE;
								return TRUE;
							}else{
								return FALSE;
							}
						}else if ( vn->l1index < vn->l1Buffer->size -1){ // whether lindex is the last entry is l1 buffer
							vn->l1index++;
							if (tokenType==TOKEN_PI_NAME)
								index++;
							if (index <=upper32At( vn->l1Buffer,vn->l1index)){
								if (index == upper32At(vn->l1Buffer,vn->l1index)){
									vn->atTerminal = FALSE;
									vn->context[0]=1;
									vn->context[1]=index;
									return TRUE;
								}
								depth = getTokenDepth((VTDNav *)vn,index);
								if (depth!=0)
									return FALSE;
								vn->LN = index;
								vn->atTerminal = TRUE;
								return TRUE;
							}else{
								return FALSE;
							}
						}else{
							index++;
							if (tokenType==TOKEN_PI_NAME)
								index++;
							if (index < vn->vtdSize){
								depth = getTokenDepth((VTDNav *)vn,index);
								if (depth!=0)
									return FALSE;
								vn->LN = index;
								vn->atTerminal = TRUE;
								return TRUE;
							}else{
								return FALSE;
							}
						}						
					}else{
						index++;
						if (tokenType==TOKEN_PI_NAME)
							index++;
						if (index < vn->vtdSize){
							depth = getTokenDepth((VTDNav *)vn,index);
							if (depth!=0)
								return FALSE;
							vn->LN = index;
							vn->atTerminal = TRUE;
							return TRUE;
						}else{
							return FALSE;
						}
					}
					
				}else{
					index = vn->vtdSize-1;
					depth = -2;
					// get to the end, then rewind
					while(index > vn->rootIndex){
						depth = getTokenDepth((VTDNav *)vn,index);
						if (depth ==-1){
							index--;
						} else
							break;								
					}			
					index++;
					if (index>=vn->vtdSize )
						return FALSE;
					else{
						vn->context[0]=-1;
						vn->LN = index;
						vn->atTerminal = TRUE;
						return TRUE;
					}
				}
				//break;
			case 1:
				if(vn->atTerminal){
					tokenType=getTokenType((VTDNav *)vn,vn->LN);
					if (tokenType==TOKEN_ATTR_NAME)
						return FALSE;
					if (lower32At(vn->l1Buffer,vn->l1index) != -1) {
						if (vn->LN < upper32At(vn->l2Buffer,vn->l2upper)) {
							tmp = upper32At(vn->l2Buffer,vn->l2index);
							index = vn->LN + 1;
							if (tokenType == TOKEN_PI_NAME)
								index++;

							if (index < tmp) {
								vn->LN = index;
								return TRUE;
							} else {
								vn->context[0] = 2;
								vn->context[2] = tmp;
								vn->atTerminal = FALSE;
								return TRUE;
							}
						} else {
							index = vn->LN + 1;
							if (tokenType == TOKEN_PI_NAME)
								index++;
							if (index < vn->vtdSize) {
								depth = getTokenDepth((VTDNav *)vn,index);
								if (depth==1 && getTokenType((VTDNav *)vn,index)!= TOKEN_STARTING_TAG){
									vn->LN = index;
									vn->atTerminal = TRUE;
									return TRUE;
								}
								return FALSE;
							} else
								return FALSE;
						}						
					}else{
						index= vn->LN+1;
						if (getTokenType((VTDNav *)vn,vn->LN)==TOKEN_PI_NAME)
							index++;
						if (index < vn->vtdSize){
							depth = getTokenDepth((VTDNav *)vn,index);
							if (depth==1 && getTokenType((VTDNav *)vn,index)!= TOKEN_STARTING_TAG){
								vn->LN = index;
								vn->atTerminal = TRUE;
								return TRUE;
							}
							return FALSE;
						}else{
							return FALSE;
						}
					}					
				}else{
					if (vn->l1index != vn->l1Buffer->size-1){
						// not the last one
						//rewind
						vn->l1index++;
						index = lastEntry =upper32At( vn->l1Buffer,vn->l1index)-1;
						while(getTokenDepth((VTDNav *)vn,index)==0){
							index--;
						}
						if (lastEntry==index){
							vn->atTerminal=FALSE;
							vn->context[0]=1;
							vn->context[1]=index+1;
							return TRUE;
						} else {
							vn->atTerminal = TRUE;
							vn->context[0]=0;
							vn->LN = index+1;
							return TRUE;							
						}
					}else{
						index = vn->vtdSize-1;
						while(index > upper32At(vn->l1Buffer,vn->l1index) && getTokenDepth((VTDNav *)vn,index)<=0){
							index--;
						}
						
						if (index == vn->vtdSize-1 ){
							if (getTokenDepth((VTDNav *)vn,index)==0){
								vn->context[0]=0;
								vn->LN = index;
								vn->atTerminal = TRUE;
								return TRUE;
							}else
								return FALSE;
						}
						index++;
						if (getTokenDepth((VTDNav *)vn,index)==0){
							vn->context[0]=0;
							vn->LN = index;
							vn->atTerminal = TRUE;
							return TRUE;
						}else{
							return FALSE;
						}
					}
				}
				
			case 2:
				if(vn->atTerminal){
					tokenType=getTokenType((VTDNav *)vn,vn->LN);
					if (tokenType==TOKEN_ATTR_NAME)
						return FALSE;
					if (lower32At(vn->l2Buffer,vn->l2index) != -1) {
						if (vn->LN < upper32At(vn->_l3Buffer,vn->l3upper)) {
							tmp = upper32At(vn->_l3Buffer,vn->l3index);
							index = vn->LN + 1;
							if (tokenType== TOKEN_PI_NAME)
								index++;

							if (index < tmp) {
								vn->LN = index;
								return TRUE;
							} else {
								vn->context[0] = 3;
								vn->context[3] = tmp;
								vn->atTerminal = FALSE;
								return TRUE;
							}
						} else {
							index = vn->LN + 1;
							if (tokenType == TOKEN_PI_NAME)
								index++;
							if (index < vn->vtdSize) {
								depth = getTokenDepth((VTDNav *)vn,index);
								if (depth==2 && getTokenType((VTDNav *)vn,index)!=TOKEN_STARTING_TAG){									
									vn->LN = index;
									return TRUE;
								}
								return FALSE;
							} 
							return FALSE;
						}						
					}else{
						index= vn->LN+1;
						if (tokenType==TOKEN_PI_NAME)
							index++;
						if (index < vn->vtdSize){
							depth = getTokenDepth((VTDNav *)vn,index);
							if (depth==2 && getTokenType((VTDNav *)vn,index)!= TOKEN_STARTING_TAG){
								vn->LN = index;
								vn->atTerminal = TRUE;
								return TRUE;
							}
							return FALSE;
						}else{
							return FALSE;
						}
					}					
				}else{
					//l2index < l2upper
					if (vn->l2index< vn->l2upper){
						tmp = upper32At(vn->l2Buffer,vn->l2index);
						vn->l2index++;
						lastEntry = index = upper32At(vn->l2Buffer,vn->l2index)-1;
						//rewind
						while(index>tmp){
							if (getTokenDepth((VTDNav *)vn,index)==1){
								tokenType = getTokenType((VTDNav *)vn,index);
								switch(tokenType){
								case TOKEN_CHARACTER_DATA:
								case TOKEN_COMMENT:
								case TOKEN_CDATA_VAL:
									index--;
									break;
								case TOKEN_PI_VAL:
									index = index -2;
									break;
									default: goto loop20;
								}
							}else
								goto loop20;
						}
						loop20:
						if (index == lastEntry){
							vn->context[0]=2;
							vn->context[2] = index+1;
							return TRUE;
						}
						vn->context[0]=1;
						vn->LN = index+1;
						vn->atTerminal = TRUE;
						return TRUE;						
					}else{
						lastEntry = index = vn->vtdSize-1;
						if (vn->l1index!=vn->l1Buffer->size-1){
							lastEntry = index = upper32At(vn->l1Buffer,vn->l1index+1)-1;
						}
						tmp = upper32At(vn->l2Buffer,vn->l2index);
						
						//rewind
						while(index>tmp){
							if (getTokenDepth((VTDNav *)vn,index)<2)
								index--;
							else
								break;
						}
						if ((/*lastEntry>=index &&*/ getTokenDepth((VTDNav *)vn,index)==1)){
							vn->LN = index;
							vn->atTerminal = TRUE;
							vn->context[0]=1;
							return TRUE;
						}
						
						if (lastEntry!=index && getTokenDepth((VTDNav *)vn,index+1)==1 ){
							vn->LN = index+1;
							vn->atTerminal = TRUE;
							vn->context[0]=1;
							return TRUE;
						}
						
						return FALSE;
					}
					
				}
				//break;
			case 3:
				if(vn->atTerminal){
					tokenType=getTokenType((VTDNav *)vn,vn->LN);
					if (tokenType==TOKEN_ATTR_NAME)
						return FALSE;
					if (lower32At(vn->_l3Buffer,vn->l3index) != -1) {
						if (vn->LN < upper32At(vn->l4Buffer,vn->l4upper)) {
							tmp = upper32At(vn->l4Buffer,vn->l4index);
							index = vn->LN + 1;
							if (tokenType == TOKEN_PI_NAME)
								index++;
							if (index < tmp) {
								vn->LN = index;
								return TRUE;
							} else {
								vn->context[0] = 4;
								vn->context[4] = tmp;
								vn->atTerminal = FALSE;
								return TRUE;
							}
						} else {
							index = vn->LN + 1;
							if (tokenType == TOKEN_PI_NAME)
								index++;
							if (index < vn->vtdSize) {
								depth = getTokenDepth((VTDNav *)vn,index);
								if (depth==3 && getTokenType((VTDNav *)vn,index)!=TOKEN_STARTING_TAG){									
									vn->LN = index;
									return TRUE;
								}
								return FALSE;
							} 
							return FALSE;
						}						
					}else{
						index= vn->LN+1;
						if (tokenType==TOKEN_PI_NAME)
							index++;
						if (index < vn->vtdSize){
							depth = getTokenDepth((VTDNav *)vn,index);
							if (depth==3 && getTokenType((VTDNav *)vn,index)!= TOKEN_STARTING_TAG){
								vn->LN = index;
								vn->atTerminal = TRUE;
								return TRUE;
							}
							return FALSE;
						}else{
							return FALSE;
						}
					}					
				}else{
					//l2index < l2upper
					if (vn->l3index< vn->l3upper){
						tmp = upper32At(vn->_l3Buffer,vn->l3index);
						vn->l3index++;
						lastEntry = index = upper32At(vn->_l3Buffer,vn->l3index)-1;
						//rewind
						while(index>tmp){
							if (getTokenDepth((VTDNav *)vn,index)==2){
								tokenType = getTokenType((VTDNav *)vn,index);
								switch(tokenType){
								case TOKEN_CHARACTER_DATA:
								case TOKEN_COMMENT:
								case TOKEN_CDATA_VAL:
									index--;
									break;
								case TOKEN_PI_VAL:
									index = index -2;
									break;
								default:
									goto loop22;
								}
							}else
								goto loop22;
						}
loop22:
						if (index == lastEntry){
							vn->context[0]=3;
							vn->context[3] = index+1;
							return TRUE;
						}
						vn->context[0]=2;
						vn->LN = index+1;
						vn->atTerminal = TRUE;
						return TRUE;						
					}else{
						lastEntry = index = vn->vtdSize-1;
						
						if (vn->l1index != vn->l1Buffer->size-1){
							lastEntry = index = upper32At(vn->l1Buffer,vn->l1index+1)-1;
						}
						
						if (vn->l2index != vn->l2Buffer->size-1 && vn->l2index != vn->l2upper){
							lastEntry = index = upper32At(vn->l2Buffer,vn->l2index+1)-1;
						}
						// insert here
						tmp = upper32At(vn->_l3Buffer,vn->l3index);
						
						//rewind
						while(index>tmp){
							if (getTokenDepth((VTDNav *)vn,index)<3)
								index--;
							else
								break;
						}
						if ((/*lastEntry==index &&*/ getTokenDepth((VTDNav *)vn,index)==2)){
							vn->LN = index;
							vn->atTerminal = TRUE;
							vn->context[0]=2;
							return TRUE;
						}
						
						if (lastEntry!=index && getTokenDepth((VTDNav *)vn,index+1)==2 ){
							vn->LN = index+1;
							vn->atTerminal = TRUE;
							vn->context[0]=2;
							return TRUE;
						}						
						return FALSE;
					}					
				}
			case 4:				
				if(vn->atTerminal){
					tokenType=getTokenType((VTDNav *)vn,vn->LN);
					if (tokenType==TOKEN_ATTR_NAME)
						return FALSE;
					if (lower32At(vn->l4Buffer,vn->l4index) != -1) {
						if (vn->LN < intAt(vn->l5Buffer,vn->l5upper)) {
							tmp = intAt(vn->l5Buffer,vn->l5index);
							index = vn->LN + 1;
							if (tokenType == TOKEN_PI_NAME)
								index++;
							if (index < tmp) {
								vn->LN = index;
								return TRUE;
							} else {
								vn->context[0] = 5;
								vn->context[5] = tmp;
								vn->atTerminal = FALSE;
								return TRUE;
							}
						} else {
							index = vn->LN + 1;
							if (tokenType == TOKEN_PI_NAME)
								index++;
							if (index < vn->vtdSize) {
								depth = getTokenDepth((VTDNav *)vn,index);
								if (depth==4 && getTokenType((VTDNav *)vn,index)!=TOKEN_STARTING_TAG){									
									vn->LN = index;
									return TRUE;
								}
								return FALSE;
							} 
							return FALSE;
						}						
					}else{
						index= vn->LN+1;
						if (tokenType==TOKEN_PI_NAME)
							index++;
						if (index < vn->vtdSize){
							depth = getTokenDepth((VTDNav *)vn,index);
							if (depth==4 && getTokenType((VTDNav *)vn,index)!= TOKEN_STARTING_TAG){
								vn->LN = index;
								vn->atTerminal = TRUE;
								return TRUE;
							}
							return FALSE;
						}else{
							return FALSE;
						}
					}					
				}else{
					//l2index < l2upper
					if (vn->l4index< vn->l4upper){
						tmp = upper32At(vn->l4Buffer,vn->l4index);
						vn->l4index++;
						lastEntry = index = upper32At(vn->l4Buffer,vn->l4index)-1;
						//rewind
						while(index>tmp){
							if (getTokenDepth((VTDNav *)vn,index)==3){
								tokenType = getTokenType((VTDNav *)vn,index);
								switch(tokenType){
								case TOKEN_CHARACTER_DATA:
								case TOKEN_COMMENT:
								case TOKEN_CDATA_VAL:
									index--;
									break;
								case TOKEN_PI_VAL:
									index = index -2;
									break;
									default:
										goto loop23;
								}
							}else
								goto loop23;
						}
loop23:
						if (index == lastEntry){
							vn->context[0] = 4;
							vn->context[4] = index+1;
							return TRUE;
						}
						vn->context[0]=3;
						vn->LN = index+1;
						vn->atTerminal = TRUE;
						return TRUE;						
					}else{						
						lastEntry = index = vn->vtdSize-1;
						
						if (vn->l1index != vn->l1Buffer->size-1){
							lastEntry = index = upper32At(vn->l1Buffer,vn->l1index+1)-1;
						}
						
						if (vn->l2index != vn->l2Buffer->size-1 && vn->l2index != vn->l2upper){
							lastEntry = index =upper32At( vn->l2Buffer,vn->l2index+1)-1;
						}

						if (vn->l3index != vn->_l3Buffer->size-1 && vn->l3index != vn->l3upper){
							lastEntry = index = upper32At(vn->_l3Buffer,vn->l3index+1)-1;
						}
						// insert here
						tmp = upper32At(vn->l4Buffer,vn->l4index);
						
						//rewind
						while(index>tmp){
							if (getTokenDepth((VTDNav *)vn,index)<4)
								index--;
							else
								break;
						}
						if ((/*lastEntry==index &&*/ getTokenDepth((VTDNav *)vn,index)==3)){
							vn->LN = index;
							vn->atTerminal = TRUE;
							vn->context[0]=3;
							return TRUE;
						}
						
						if (lastEntry!=index && getTokenDepth((VTDNav *)vn,index+1)==3){
							vn->LN = index+1;
							vn->atTerminal = TRUE;
							vn->context[0]=3;
							return TRUE;
						}						
						return FALSE;
					}					
				}
			
			case 5:				
				if(!vn->atTerminal){
				//l2index < l2upper
				if (vn->l5index< vn->l5upper){
					tmp = intAt(vn->l5Buffer,vn->l5index);
					vn->l5index++;
					lastEntry = index = intAt(vn->l5Buffer,vn->l5index)-1;
					//rewind
					while(index>tmp){
						if (getTokenDepth((VTDNav *)vn,index)==4){
							tokenType = getTokenType((VTDNav *)vn,index);
							switch(tokenType){
							case TOKEN_CHARACTER_DATA:
							case TOKEN_COMMENT:
							case TOKEN_CDATA_VAL:
								index--;
								break;
							case TOKEN_PI_VAL:
								index = index -2;
								break;
								default:
									goto loop24;
							}
						}else
							goto loop24;
					}
loop24:
					if (index == lastEntry){
						vn->context[0]= 5;
						vn->context[5] = index+1;
						return TRUE;
					}
					vn->context[0]=4;
					vn->LN = index+1;
					vn->atTerminal = TRUE;
					return TRUE;						
				}else{
					lastEntry = index = vn->vtdSize-1;
					
					if (vn->l1index != vn->l1Buffer->size-1){
						lastEntry = index = upper32At(vn->l1Buffer,vn->l1index+1)-1;
					}
					
					if (vn->l2index != vn->l2Buffer->size-1 && vn->l2index != vn->l2upper){
						lastEntry = index =upper32At(vn->l2Buffer,vn->l2index+1)-1;
					}
					
					if (vn->l3index != vn->_l3Buffer->size-1 && vn->l3index != vn->l3upper){
						lastEntry = index = upper32At(vn->_l3Buffer,vn->l3index+1)-1;
					}
					if (vn->l4index != vn->l4Buffer->size-1 && vn->l4index != vn->l4upper){
						lastEntry = index = upper32At(vn->l4Buffer,vn->l4index+1)-1;
					}
					// inser here
					tmp = intAt(vn->l5Buffer,vn->l5index);
					
					//rewind
					while(index>tmp){
						if (getTokenDepth((VTDNav *)vn,index)<5)
							index--;
						else
							break;
					}
					if ((/*lastEntry==index &&*/ getTokenDepth((VTDNav *)vn,index)==4)){
						vn->LN = index;
						vn->atTerminal = TRUE;
						vn->context[0]=4;
						return TRUE;
					}
					
					if (lastEntry!=index && getTokenDepth((VTDNav *)vn,index+1)==4){
						vn->LN = index+1;
						vn->atTerminal = TRUE;
						vn->context[0]=4;
						return TRUE;
					}
					
					return FALSE;
				}
				
			}
				//break;
			default:
				if (vn->atTerminal){
					tokenType=getTokenType((VTDNav *)vn,vn->LN);
					if (tokenType==TOKEN_ATTR_NAME)
						return FALSE;
					index = vn->LN+1;
					tmp = vn->context[0]+1;
				}
				else{
					index = vn->context[vn->context[0]] + 1;
					tmp = vn->context[0];
				}
				while (index < vn->vtdSize) {
					Long temp = longAt(vn->vtdBuffer,index);
					tokenType = (int) ((MASK_TOKEN_TYPE & temp) >> 60) &0x0f;
					depth = (int) ((MASK_TOKEN_DEPTH & temp) >> 52);
					switch (tokenType) {
					case TOKEN_STARTING_TAG:						
						if (depth < tmp) {
							return FALSE;
						} else if (depth == tmp) {
							vn->context[0]=tmp;
							vn->context[vn->context[0]] = index;
							vn->atTerminal = FALSE;
							return TRUE;
						}else 
							index++;
						break;
					case TOKEN_ATTR_NAME:
					case TOKEN_ATTR_NS:
						index += 2;
						break;
					case TOKEN_CHARACTER_DATA:
					case TOKEN_COMMENT:
					case TOKEN_CDATA_VAL:
						//depth = (int) ((MASK_TOKEN_DEPTH & temp) >> 52);
						if (depth < tmp-1) {
							return FALSE;
						} else if (depth == (tmp-1)) {
							vn->context[0]=tmp-1;
							vn->LN = index;
							vn->atTerminal = TRUE;
							return TRUE;
						} else
							index++;
						break;
					case TOKEN_PI_NAME:
						//depth = (int) ((MASK_TOKEN_DEPTH & temp) >> 52);
						if (depth < tmp-1) {
							return FALSE;
						} else if (depth == tmp-1) {
							vn->context[0]=tmp-1;
							vn->LN = index;
							vn->atTerminal = TRUE;
							return TRUE;
						} else
							index += 2;
						break;
					default:
						index++;
					}
					
				}
				return FALSE;
			}		
		case PREV_SIBLING:
			return toNode_PrevSibling_L5(vn);
		default :
			throwException2(nav_exception,"illegal navigation options");
		}	
		return FALSE;
	}

	Boolean toNode_LastChild_L5(VTDNav_L5 *vn){
				
		int depth,index,tokenType,lastEntry,tmp;
		switch (vn->context[0]) {
		case -1:
			index = vn->vtdSize-1;
			tokenType = getTokenType((VTDNav *)vn,index);
			depth = getTokenDepth((VTDNav *)vn,index);
			if (depth == -1) {
				switch (tokenType) {
					case TOKEN_COMMENT:
						vn->LN = index;
						vn->atTerminal = TRUE;
						return TRUE;							
					case TOKEN_PI_VAL:
						vn->LN = index -1;
						vn->atTerminal = TRUE;
						return TRUE;													
				}	
			}
			vn->context[0]=0;
			return TRUE;
			
		case 0:
			if (vn->l1Buffer->size!=0){
				lastEntry = upper32At(vn->l1Buffer,vn->l1Buffer->size-1);
				index = vn->vtdSize-1;
				while(index > lastEntry){
					depth = getTokenDepth((VTDNav *)vn,index);
					if (depth==-1){
						index--;
						continue;
					} else if (depth ==0){
						tokenType = getTokenType((VTDNav *)vn,index);
						switch(tokenType){
						case TOKEN_CHARACTER_DATA:
						case TOKEN_COMMENT:
						case TOKEN_CDATA_VAL:
							vn->LN = index;
							vn->atTerminal = TRUE;
							vn->l1index = vn->l1Buffer->size -1;
							return TRUE;
						case TOKEN_PI_VAL:
							vn->LN = index -1;
							vn->atTerminal = TRUE;
							vn->l1index = vn->l1Buffer->size -1;
							return TRUE;
						default:
							return FALSE;
						} 	
					}else {
						vn->l1index = vn->l1Buffer->size -1;
						vn->context[0]= 1;
						vn->context[1]= lastEntry;
						return TRUE;
					}
				}
				vn->l1index = vn->l1Buffer->size -1;
				vn->context[0]= 1;
				vn->context[1]= lastEntry;
				return TRUE;
			}else{
				index = vn->vtdSize - 1;
				while(index>vn->rootIndex){
					depth = getTokenDepth((VTDNav *)vn,index);
					if (depth == -1){
						index--;
						continue;
					}
					tokenType = getTokenType((VTDNav *)vn,index);
					switch(tokenType){
					case TOKEN_CHARACTER_DATA:
					case TOKEN_COMMENT:
					case TOKEN_CDATA_VAL:
						vn->LN = index;
						vn->atTerminal = TRUE;
						return TRUE;
					case TOKEN_PI_VAL:
						vn->LN = index-1;
						vn->atTerminal = TRUE;
						return TRUE;
					default:
						return FALSE;
					}
				}
				return FALSE;
			}
			
		case 1:
			if (lower32At(vn->l1Buffer,vn->l1index)!=-1){
				vn->l2lower = lower32At(vn->l1Buffer,vn->l1index);
				tmp = vn->l1index+1;
				while(tmp<vn->l1Buffer->size){
					if (lower32At(vn->l1Buffer,tmp)!=-1){
						vn->l2upper = lower32At(vn->l1Buffer,tmp)-1;
						break;
					}else
						tmp++;
				}
				if (tmp==vn->l1Buffer->size){
					vn->l2upper = vn->l2Buffer->size-1;
				}					
				vn->l2index = vn->l2upper;
				index =vn->vtdSize-1;
				if (vn->l1index != vn->l1Buffer->size-1){
					index = upper32At(vn->l1Buffer,vn->l1index+1)-1;
				}
				tmp = upper32At(vn->l2Buffer,vn->l2index);
				// rewind and find the first node of depth 1
				while(index > tmp){
					depth = getTokenDepth((VTDNav *)vn,index);
					if (depth <1)
						index--;
					else if (depth == 1){
						tokenType = getTokenType((VTDNav *)vn,index);
						if (tokenType == TOKEN_PI_VAL)
							vn->LN = index-1;
						else
							vn->LN = index;
						vn->atTerminal = TRUE;
						//context[0]=1;
						return TRUE;
					}else
						break;							
				}
				vn->context[0]=2;
				vn->context[2]=tmp;
				return TRUE;
				
				
			}else{
				index = vn->context[1]+1;
				 while(index<vn->vtdSize){
					tokenType = getTokenType((VTDNav *)vn,index);
					switch(tokenType){
					case TOKEN_ATTR_NAME:
					case TOKEN_ATTR_NS:
						index+=2;
						break;
					default: goto loop;
					}
				}
				loop:					
				if (index< vn->vtdSize && getTokenDepth((VTDNav *)vn,index)==1 && getTokenType((VTDNav *)vn,index)!=TOKEN_STARTING_TAG){
					lastEntry = index;
					index++;
					//scan forward
					while(index<vn->vtdSize){
						tokenType = getTokenType((VTDNav *)vn,index);
						depth = getTokenDepth((VTDNav *)vn,index);
						if (depth == 1){
							switch(tokenType){
							case TOKEN_CHARACTER_DATA:
							case TOKEN_COMMENT:
							case TOKEN_CDATA_VAL:
								lastEntry = index;
								index++;
								break;
							case TOKEN_PI_NAME:
								lastEntry = index;
								index+=2;
								break;
							default:
								goto loop2;
							}
						}else
							goto loop2;
					}
loop2:
					vn->LN = lastEntry;
					vn->atTerminal = TRUE;
					return TRUE;
				}else
					return FALSE;					
			}
			
		case 2:		
			if (lower32At(vn->l2Buffer,vn->l2index)!=-1){
				vn->l3lower = lower32At(vn->l2Buffer,vn->l2index);
				tmp = vn->l2index+1;
				while(tmp<vn->l2Buffer->size){
					if (lower32At(vn->l2Buffer,tmp)!=-1){
						vn->l3upper = lower32At(vn->l2Buffer,tmp)-1;
						break;
					}else
						tmp++;
				}
				if (tmp==vn->l2Buffer->size){
					vn->l3upper = vn->_l3Buffer->size-1;
				}					
				vn->l3index = vn->l3upper;
				index =vn->vtdSize-1;
				
				if (vn->l1index != vn->l1Buffer->size-1){
					index = upper32At(vn->l1Buffer,vn->l1index+1)-1;
				}
				
				if (vn->l2index != vn->l2Buffer->size-1 && vn->l2index != vn->l2upper){
					index = upper32At(vn->l2Buffer,vn->l2index+1)-1;
				}
				tmp = upper32At(vn->_l3Buffer,vn->l3index);
				// rewind and find the first node of depth 1
				while(index > tmp){
					depth = getTokenDepth((VTDNav *)vn,index);
					if (depth <2)
						index--;
					else if (depth == 2){
						tokenType = getTokenType((VTDNav *)vn,index);
						if (tokenType == TOKEN_PI_VAL)
							vn->LN = index-1;
						else
							vn->LN = index;
						vn->atTerminal = TRUE;
						//context[0]=1;
						return TRUE;
					}else
						break;							
				}
				vn->context[0]=3;
				vn->context[3]=tmp;
				return TRUE;					
			}else{
				index = vn->context[2]+1;
				 while(index<vn->vtdSize){
					tokenType = getTokenType((VTDNav *)vn,index);
					switch(tokenType){
					case TOKEN_ATTR_NAME:
					case TOKEN_ATTR_NS:
						index+=2;
						break;
					default: goto loop3;
					}
				}
				loop3:					
				if (index< vn->vtdSize && getTokenDepth((VTDNav *)vn,index)==2 && getTokenType((VTDNav *)vn,index)!=TOKEN_STARTING_TAG){
					lastEntry = index;
					index++;
					//scan forward
					while(index<vn->vtdSize){
						tokenType = getTokenType((VTDNav *)vn,index);
						depth = getTokenDepth((VTDNav *)vn,index);
						if (depth == 2){
							switch(tokenType){
							case TOKEN_CHARACTER_DATA:
							case TOKEN_COMMENT:
							case TOKEN_CDATA_VAL:
								lastEntry = index;
								index++;
								break;
							case TOKEN_PI_NAME:
								lastEntry = index;
								index+=2;
								break;
							default:
								goto loop22;
							}
						}else
							goto loop22;
					}
loop22:
					vn->LN = lastEntry;
					vn->atTerminal = TRUE;
					return TRUE;
				}else
					return FALSE;					
			}	
			
		case 3:
			if (lower32At(vn->_l3Buffer,vn->l3index) != -1) {
				vn->l4lower = lower32At(vn->_l3Buffer,vn->l3index);
				tmp = vn->l3index + 1;
				while (tmp < vn->_l3Buffer->size) {
					if (lower32At(vn->_l3Buffer,tmp) != -1) {
						vn->l4upper = lower32At(vn->_l3Buffer,tmp) - 1;
						break;
					} else
						tmp++;
				}
				if (tmp == vn->_l3Buffer->size) {
					vn->l4upper = vn->l4Buffer->size - 1;
				}
				vn->l4index = vn->l4upper;
				index = vn->vtdSize - 1;

				if (vn->l1index != vn->l1Buffer->size - 1) {
					index =upper32At(vn->l1Buffer,vn->l1index + 1) - 1;
				}

				if (vn->l2index != vn->l2Buffer->size - 1 && vn->l2index != vn->l2upper) {
					index = upper32At(vn->l2Buffer,vn->l2index + 1) - 1;
				}

				if (vn->l3index != vn->_l3Buffer->size - 1 && vn->l3index != vn->l3upper) {
					index = upper32At(vn->_l3Buffer,vn->l3index + 1) - 1;
				}

				tmp = upper32At(vn->l4Buffer,vn->l4index);
				// rewind and find the first node of depth 1
				while (index > tmp) {
					depth = getTokenDepth((VTDNav *)vn,index);
					if (depth < 3)
						index--;
					else if (depth == 3) {
						tokenType = getTokenType((VTDNav *)vn,index);
						if (tokenType == TOKEN_PI_VAL)
							vn->LN = index - 1;
						else
							vn->LN = index;
						vn->atTerminal = TRUE;
						// context[0]=1;
						return TRUE;
					} else
						break;
				}
				vn->context[0] = 4;
				vn->context[4] = tmp;
				return TRUE;
			} else {
				index = vn->context[3] + 1;
				 while (index < vn->vtdSize) {
					tokenType = getTokenType((VTDNav *)vn,index);
					switch (tokenType) {
					case TOKEN_ATTR_NAME:
					case TOKEN_ATTR_NS:
						index += 2;
						break;
					default:
						goto loop11;
					}
				}
loop11:
				if (index < vn->vtdSize && getTokenDepth((VTDNav *)vn,index) == 3
						&& getTokenType((VTDNav *)vn,index) != TOKEN_STARTING_TAG) {
					lastEntry = index;
					index++;
					// scan forward
					while (index < vn->vtdSize) {
						tokenType = getTokenType((VTDNav *)vn,index);
						depth = getTokenDepth((VTDNav *)vn,index);
						if (depth == 3) {
							switch (tokenType) {
							case TOKEN_CHARACTER_DATA:
							case TOKEN_COMMENT:
							case TOKEN_CDATA_VAL:
								lastEntry = index;
								index++;
								break;
							case TOKEN_PI_NAME:
								lastEntry = index;
								index += 2;
								break;
							default:
								goto loop222;
							}
						} else
							goto loop222;
					}
loop222: 
					vn->LN = lastEntry;
					vn->atTerminal = TRUE;
					return TRUE;
				} else
					return FALSE;
			}
		case 4: 
			if (lower32At(vn->l4Buffer,vn->l4index)!=-1){
				vn->l5lower = lower32At(vn->l4Buffer,vn->l4index);
				tmp = vn->l4index+1;
				while(tmp<vn->l4Buffer->size){
					if (lower32At(vn->l4Buffer,tmp)!=-1){
						vn->l5upper = lower32At(vn->l4Buffer,tmp)-1;
						break;
					}else
						tmp++;
				}
				if (tmp==vn->l4Buffer->size){
					vn->l5upper = vn->l5Buffer->size-1;
				}					
				vn->l5index = vn->l5upper;
				index =vn->vtdSize-1;
				
				if (vn->l1index != vn->l1Buffer->size-1){
					index = upper32At(vn->l1Buffer,vn->l1index+1)-1;
				}
				
				if (vn->l2index != vn->l2Buffer->size-1 && vn->l2index != vn->l2upper){
					index = upper32At(vn->l2Buffer,vn->l2index+1)-1;
				}

				if (vn->l3index != vn->_l3Buffer->size-1 && vn->l3index != vn->l3upper){
					index = upper32At(vn->_l3Buffer,vn->l3index+1)-1;
				}

				if (vn->l4index != vn->l4Buffer->size-1 && vn->l4index != vn->l4upper){
					index = upper32At(vn->l4Buffer,vn->l4index+1)-1;
				}

				tmp = intAt(vn->l5Buffer,vn->l5index);
				// rewind and find the first node of depth 1
				while(index > tmp){
					depth = getTokenDepth((VTDNav *)vn,index);
					if (depth <4)
						index--;
					else if (depth == 4){
						tokenType = getTokenType((VTDNav *)vn,index);
						if (tokenType == TOKEN_PI_VAL)
							vn->LN = index-1;
						else
							vn->LN = index;
						vn->atTerminal = TRUE;
						//context[0]=1;
						return TRUE;
					}else
						break;							
				}
				vn->context[0]=5;
				vn->context[5]=tmp;
				return TRUE;					
			}else{
				index = vn->context[4]+1;
				 while(index<vn->vtdSize){
					tokenType = getTokenType((VTDNav *)vn,index);
					switch(tokenType){
					case TOKEN_ATTR_NAME:
					case TOKEN_ATTR_NS:
						index+=2;
						break;
					default: goto loop1111;
					}
				}
					loop1111:				
				if (index< vn->vtdSize 
					&& getTokenDepth((VTDNav *)vn,index)==4 
					&& getTokenType((VTDNav *)vn,index)!=TOKEN_STARTING_TAG){
					lastEntry = index;
					index++;
					//scan forward
					while(index<vn->vtdSize){
						tokenType = getTokenType((VTDNav *)vn,index);
						depth = getTokenDepth((VTDNav *)vn,index);
						if (depth == 4){
							switch(tokenType){
							case TOKEN_CHARACTER_DATA:
							case TOKEN_COMMENT:
							case TOKEN_CDATA_VAL:
								lastEntry = index;
								index++;
								break;
							case TOKEN_PI_NAME:
								lastEntry = index;
								index+=2;
								break;
							default:
								goto loop33;
							}
						}else
							goto loop33;
					}
loop33:
					vn->LN = lastEntry;
					vn->atTerminal = TRUE;
					return TRUE;
				}else
					return FALSE;					
			}	
			
		default:
			index = vn->context[vn->context[0]] + 1;
			lastEntry  = -1;vn-> atTerminal = FALSE;
			while (index < vn->vtdBuffer->size) {
				Long temp = longAt(vn->vtdBuffer,index);
				tokenType =
					(int) ((MASK_TOKEN_TYPE & temp) >> 60) & 0x0f;
				depth =getTokenDepth((VTDNav *)vn,index);
				switch(tokenType){
				case TOKEN_STARTING_TAG:
					if (depth <= vn->context[0]){
						if (lastEntry !=-1){
							if (vn->atTerminal){
								vn->LN = lastEntry;									
							}else{
								vn->context[0]+=1;
								vn->context[vn->context[0]] = lastEntry;
							}
							return TRUE;									
						} else
							return FALSE;
					}else if (depth == (vn->context[0] + 1)) {
						lastEntry = index;
						vn->atTerminal= FALSE;
					}
					index++;
					break;
				case TOKEN_ATTR_NAME:
				case TOKEN_ATTR_NS: index+=2;break;
				case TOKEN_CHARACTER_DATA:
				case TOKEN_COMMENT:
				case TOKEN_CDATA_VAL:						
					if (depth < vn->context[0]){
						if (lastEntry !=-1){
							if (vn->atTerminal){
								vn->LN = lastEntry;
							}
							else{
								vn->context[0]++;
								vn->context[vn->context[0]]=lastEntry;									
							}
							return TRUE;
						}else
							return FALSE;
					}else if (depth == (vn->context[0])) {
						lastEntry = index;
						vn->atTerminal = TRUE;
					}
					index++;
					break;
				case TOKEN_PI_NAME:
					if (depth < vn->context[0]){
						if (lastEntry !=-1){
							if (vn->atTerminal){
								vn->LN = lastEntry;
							}
							else{
								vn->context[0]++;
								vn->context[vn->context[0]]=lastEntry;									
							}
							return TRUE;
						}else
							return FALSE;
					}else if (depth == (vn->context[0])) {
						lastEntry = index;
						vn->atTerminal = TRUE;
					}
					index+=2;
					break;
					}
				//index++;
			} // what condition
			if (lastEntry !=-1){
				if (vn->atTerminal){
					vn->LN = lastEntry;
				}
				else{
					vn->context[0]++;
					vn->context[vn->context[0]]=lastEntry;									
				}
				return TRUE;
			}else
				return FALSE;
		}
	}

	Boolean toNode_LastChild(VTDNav *vn){
		
		int depth,index,tokenType,lastEntry,tmp;
		switch (vn->context[0]) {
		case -1:
			index = vn->vtdSize-1;
			tokenType = getTokenType(vn,index);
			depth = getTokenDepth(vn,index);
			if (depth == -1) {
				switch (tokenType) {
					case TOKEN_COMMENT:
						vn->LN = index;
						vn->atTerminal = TRUE;
						return TRUE;							
					case TOKEN_PI_VAL:
						vn->LN = index -1;
						vn->atTerminal = TRUE;
						return TRUE;													
				}	
			}
			vn->context[0]=0;
			return TRUE;
			
		case 0:
			if (vn->l1Buffer->size!=0){
				lastEntry = upper32At(vn->l1Buffer,vn->l1Buffer->size-1);
				index = vn->vtdSize-1;
				while(index > lastEntry){
					depth = getTokenDepth(vn,index);
					if (depth==-1){
						index--;
						continue;
					} else if (depth ==0){
						tokenType = getTokenType(vn,index);
						switch(tokenType){
						case TOKEN_CHARACTER_DATA:
						case TOKEN_COMMENT:
						case TOKEN_CDATA_VAL:
							vn->LN = index;
							vn->atTerminal = TRUE;
							vn->l1index = vn->l1Buffer->size -1;
							return TRUE;
						case TOKEN_PI_VAL:
							vn->LN = index -1;
							vn->atTerminal = TRUE;
							vn->l1index = vn->l1Buffer->size -1;
							return TRUE;
						default:
							return FALSE;
						} 	
					}else {
						vn->l1index = vn->l1Buffer->size -1;
						vn->context[0]= 1;
						vn->context[1]= lastEntry;
						return TRUE;
					}
				}
				vn->l1index = vn->l1Buffer->size -1;
				vn->context[0]= 1;
				vn->context[1]= lastEntry;
				return TRUE;
			}else{
				index = vn->vtdSize - 1;
				while(index>vn->rootIndex){
					depth = getTokenDepth(vn,index);
					if (depth == -1){
						index--;
						continue;
					}
					tokenType = getTokenType(vn,index);
					switch(tokenType){
					case TOKEN_CHARACTER_DATA:
					case TOKEN_COMMENT:
					case TOKEN_CDATA_VAL:
						vn->LN = index;
						vn->atTerminal = TRUE;
						return TRUE;
					case TOKEN_PI_VAL:
						vn->LN = index-1;
						vn->atTerminal = TRUE;
						return TRUE;
					default:
						return FALSE;
					}
				}
				return FALSE;
			}
			
		case 1:
			if (lower32At(vn->l1Buffer,vn->l1index)!=-1){
				vn->l2lower = lower32At(vn->l1Buffer,vn->l1index);
				tmp = vn->l1index+1;
				while(tmp<vn->l1Buffer->size){
					if (lower32At(vn->l1Buffer,tmp)!=-1){
						vn->l2upper = lower32At(vn->l1Buffer,tmp)-1;
						break;
					}else
						tmp++;
				}
				if (tmp==vn->l1Buffer->size){
					vn->l2upper = vn->l2Buffer->size-1;
				}					
				vn->l2index = vn->l2upper;
				index =vn->vtdSize-1;
				if (vn->l1index != vn->l1Buffer->size-1){
					index =upper32At( vn->l1Buffer,vn->l1index+1)-1;
				}
				tmp = upper32At(vn->l2Buffer,vn->l2index);
				// rewind and find the first node of depth 1
				while(index > tmp){
					depth = getTokenDepth(vn,index);
					if (depth <1)
						index--;
					else if (depth == 1){
						tokenType = getTokenType(vn,index);
						if (tokenType == TOKEN_PI_VAL)
							vn->LN = index-1;
						else
							vn->LN = index;
						vn->atTerminal = TRUE;
						//context[0]=1;
						return TRUE;
					}else
						break;							
				}
				vn->context[0]=2;
				vn->context[2]=tmp;
				return TRUE;
				
				
			}else{
				index = vn->context[1]+1;
				while(index<vn->vtdSize){
					tokenType = getTokenType(vn,index);
					switch(tokenType){
					case TOKEN_ATTR_NAME:
					case TOKEN_ATTR_NS:
						index+=2;
						break;
					default: goto loop;
					}
				}
				loop: 					
				if (index< vn->vtdSize && getTokenDepth(vn,index)==1 && getTokenType(vn,index)!=TOKEN_STARTING_TAG){
					lastEntry = index;
					index++;
					//scan forward
					while(index<vn->vtdSize){
						tokenType = getTokenType(vn,index);
						depth = getTokenDepth(vn,index);
						if (depth == 1){
							switch(tokenType){
							case TOKEN_CHARACTER_DATA:
							case TOKEN_COMMENT:
							case TOKEN_CDATA_VAL:
								lastEntry = index;
								index++;
								break;
							case TOKEN_PI_NAME:
								lastEntry = index;
								index+=2;
								break;
							default:
								goto loop2;
							}
						}else
							goto loop2;
					}
				loop2:
					vn->LN = lastEntry;
					vn->atTerminal = TRUE;
					return TRUE;
				}else
					return FALSE;					
			}
			
		case 2:		
			if (lower32At(vn->l2Buffer,vn->l2index)!=-1){
				vn->l3lower = lower32At(vn->l2Buffer,vn->l2index);
				tmp = vn->l2index+1;
				while(tmp<vn->l2Buffer->size){
					if (lower32At(vn->l2Buffer,tmp)!=-1){
						vn->l3upper = lower32At(vn->l2Buffer,tmp)-1;
						break;
					}else
						tmp++;
				}
				if (tmp==vn->l2Buffer->size){
					vn->l3upper = vn->l3Buffer->size-1;
				}					
				vn->l3index = vn->l3upper;
				index =vn->vtdSize-1;
				
				if (vn->l1index != vn->l1Buffer->size-1){
					index = upper32At(vn->l1Buffer,vn->l1index+1)-1;
				}
				
				if (vn->l2index != vn->l2Buffer->size-1 && vn->l2index != vn->l2upper){
					index = upper32At(vn->l2Buffer,vn->l2index+1)-1;
				}
				tmp = intAt(vn->l3Buffer,vn->l3index);
				// rewind and find the first node of depth 1
				while(index > tmp){
					depth = getTokenDepth((VTDNav *)vn,index);
					if (depth <2)
						index--;
					else if (depth == 2){
						tokenType = getTokenType((VTDNav *)vn,index);
						if (tokenType == TOKEN_PI_VAL)
							vn->LN = index-1;
						else
							vn->LN = index;
						vn->atTerminal = TRUE;
						//context[0]=1;
						return TRUE;
					}else
						break;							
				}
				vn->context[0]=3;
				vn->context[3]=tmp;
				return TRUE;					
			}else{
				index = vn->context[2]+1;
				 while(index<vn->vtdSize){
					tokenType = getTokenType(vn,index);
					switch(tokenType){
					case TOKEN_ATTR_NAME:
					case TOKEN_ATTR_NS:
						index+=2;
						break;
					default: goto loop3;
					}
				}
					loop3:				
				if (index< vn->vtdSize && getTokenDepth(vn,index)==2 && getTokenType(vn,index)!=TOKEN_STARTING_TAG){
					lastEntry = index;
					index++;
					//scan forward
					while(index<vn->vtdSize){
						tokenType = getTokenType(vn,index);
						depth = getTokenDepth(vn,index);
						if (depth == 2){
							switch(tokenType){
							case TOKEN_CHARACTER_DATA:
							case TOKEN_COMMENT:
							case TOKEN_CDATA_VAL:
								lastEntry = index;
								index++;
								break;
							case TOKEN_PI_NAME:
								lastEntry = index;
								index+=2;
								break;
							default:
								goto loop22;
							}
						}else
							goto loop22;
					}

					loop22:
					vn->LN = lastEntry;
					vn->atTerminal = TRUE;
					return TRUE;
				}else
					return FALSE;					
			}				
		default:
			index = vn->context[vn->context[0]] + 1;
			lastEntry  = -1; vn->atTerminal = FALSE;
			while (index < vn->vtdBuffer->size) {
				Long temp = longAt(vn->vtdBuffer,index);
				tokenType =
					(int) ((MASK_TOKEN_TYPE & temp) >> 60)& 0x0f;
				depth =getTokenDepth(vn,index);
				switch(tokenType){
				case TOKEN_STARTING_TAG:
					if (depth <= vn->context[0]){
						if (lastEntry !=-1){
							if (vn->atTerminal){
								vn->LN = lastEntry;									
							}else{
								vn->context[0]+=1;
								vn->context[vn->context[0]] = lastEntry;
								vn->atTerminal = FALSE;
							}
							return TRUE;									
						} else
							return FALSE;
					}else if (depth == (vn->context[0] + 1)) {
						lastEntry = index;
						vn->atTerminal = FALSE;
					}
					index++;
					break;
				case TOKEN_ATTR_NAME:
				case TOKEN_ATTR_NS: index+=2;break;
				case TOKEN_CHARACTER_DATA:
				case TOKEN_COMMENT:
				case TOKEN_CDATA_VAL:						
					if (depth < vn->context[0]){
						if (lastEntry !=-1){
							if (vn->atTerminal){
								vn->LN = lastEntry;
							}
							else{
								vn->context[0]++;
								vn->context[vn->context[0]]=lastEntry;									
							}
							return TRUE;
						}else
							return FALSE;
					}else if (depth == (vn->context[0])) {
						lastEntry = index;
						vn->atTerminal = TRUE;
					}
					index++;
					break;
				case TOKEN_PI_NAME:
					if (depth < vn->context[0]){
						if (lastEntry !=-1){
							if (vn->atTerminal){
								vn->LN = lastEntry;
							}
							else{
								vn->context[0]++;
								vn->context[vn->context[0]]=lastEntry;									
							}
							return TRUE;
						}else
							return FALSE;
					}else if (depth == (vn->context[0])) {
						lastEntry = index;
						vn->atTerminal = TRUE;
					}
					index+=2;
					break;
					}
				//index++;
			} // what condition
			if (lastEntry !=-1){
				if (vn->atTerminal){
					vn->LN = lastEntry;
				}
				else{
					vn->context[0]++;
					vn->context[vn->context[0]]=lastEntry;									
				}
				return TRUE;
			}else
				return FALSE;
		}
	
	}


	Boolean toNode_PrevSibling(VTDNav *vn){
	
		int index,tokenType,depth,tmp;
		switch (vn->context[0]) {
		case -1:
			if(vn->atTerminal){
				index = vn->LN-1;
				if (index>0){
					depth = getTokenDepth(vn,index);
					if (depth==-1){
						tokenType = getTokenType(vn,index);
						switch (tokenType) {
						case TOKEN_PI_VAL:
							index--;
						case TOKEN_COMMENT:
							vn->LN = index;
							return TRUE;
						default:
							return FALSE;
						}
					}else{
						vn->context[0] = 0;
						vn->atTerminal = FALSE;
						return TRUE;
					}
				}else{
					return FALSE;
				}
			}else{
				return FALSE;
			}
		
		case 0:
			if(vn->atTerminal){
				if (vn->l1Buffer->size!=0){
					// three cases
					if (vn->LN < upper32At(vn->l1Buffer,vn->l1index)){
						index = vn->LN-1;
						if (index>vn->rootIndex){
							tokenType = getTokenType(vn,index);
							depth = getTokenDepth(vn,index);								
							if (depth == 0){
								switch(tokenType){									
								case TOKEN_CHARACTER_DATA:
								case TOKEN_COMMENT:
								case TOKEN_CDATA_VAL:
									vn->LN = index;
									return TRUE;
								case TOKEN_PI_VAL:
									vn->LN = index -1;
									return TRUE;
								}
							}								
							if (vn->l1index==0)
								return FALSE;
							vn->l1index--;
							vn->atTerminal = FALSE;
							vn->context[0]=1;
							vn->context[1]= upper32At(vn->l1Buffer,vn->l1index);
							return TRUE;
						}else 
							return FALSE;
					} else {
						index = vn->LN -1;
						if (index>upper32At(vn->l1Buffer,vn->l1index)){
							tokenType = getTokenType(vn,index);
							depth = getTokenDepth(vn,index);								
							if (depth == 0){
								switch(tokenType){									
								case TOKEN_CHARACTER_DATA:
								case TOKEN_COMMENT:
								case TOKEN_CDATA_VAL:
									vn->LN = index;
									return TRUE;
								case TOKEN_PI_VAL:
									vn->LN = index -1;
									return TRUE;
								}
							}										
						}
						vn->atTerminal = FALSE;
						vn->context[0]=1;
						vn->context[1]= upper32At(vn->l1Buffer,vn->l1index);
						return TRUE;
					}						
				}else{
					index =vn-> LN-1;
					if (index>vn->rootIndex){
						tokenType=getTokenType(vn,index);
						switch (tokenType) {
						case TOKEN_PI_VAL:
							index--;
						case TOKEN_CHARACTER_DATA:
						case TOKEN_COMMENT:
						case TOKEN_CDATA_VAL:
						
							vn->LN = index;
							vn->atTerminal = TRUE;
							vn->context[0]=0;
							return TRUE;
						default:
							return FALSE;
						}
					}
					return FALSE;
				}
				
			}else{
				index = vn->rootIndex-1;
				if (index>0){
					tokenType = getTokenType(vn,index);
					switch (tokenType) {
					case TOKEN_PI_VAL:
						index--;
					case TOKEN_COMMENT:
						vn->LN = index;
						vn->atTerminal = TRUE;
						vn->context[0]=-1;
						return TRUE;
					default:
						return FALSE;
					}
				}else{
					return FALSE;
				}
			}
			//break;
		case 1:
			if(vn->atTerminal){
				if (lower32At(vn->l1Buffer,vn->l1index)!=-1){
					tmp = upper32At(vn->l2Buffer,vn->l2index);
					if (vn->LN > tmp){
						index = vn->LN-1;
						if (getTokenType(vn,index)==TOKEN_PI_VAL){
							index--;
						}
						if (getTokenDepth(vn,index)==1){
							vn->LN = index;
							return TRUE;
						}else{
							vn->atTerminal = FALSE;
							vn->context[0]=2;
							vn->context[2]=tmp;
							return TRUE;
						}
					} else if (vn->l2index!=vn->l2lower){
						vn->l2index--;
						vn->atTerminal = FALSE;
						vn->context[0]=2;
						vn->context[2]=upper32At(vn->l2Buffer,vn->l2index);
						return TRUE;
					} else {
						index = vn->LN-1;
						tokenType = getTokenType(vn,index);
						switch (tokenType) {
						case TOKEN_PI_VAL:
							index--;
						case TOKEN_CHARACTER_DATA:
						case TOKEN_COMMENT:
						case TOKEN_CDATA_VAL:
						
							vn->LN = index;
							vn->atTerminal = TRUE;
							vn->context[0]=1;
							return TRUE;
						default:
							return FALSE;
						}
					}
				}else{
					index= vn->LN-1;
					if (getTokenType(vn,index)==TOKEN_PI_VAL)
						index--;
					if (index > vn->context[1]){
						tokenType = getTokenType(vn,index);
						if (tokenType!= TOKEN_ATTR_VAL){
							vn->LN = index;
							vn->atTerminal = TRUE;
							return TRUE;
						}else
							return FALSE;
					}else{
						return FALSE;
					}
				}					
			}else{
				index = vn->context[1]-1;	
				tokenType = getTokenType(vn,index);
				if (getTokenDepth(vn,index)==0
						&& tokenType!= TOKEN_ATTR_VAL
						&& tokenType!= TOKEN_STARTING_TAG){
					if (tokenType==TOKEN_PI_VAL)
						index--;
					vn->context[0]=0;
					vn->atTerminal = TRUE;
					vn->LN = index;
					return TRUE;
				}else{
					// no more prev sibling element
					if (vn->l1index != 0){
						vn->l1index--;
						vn->context[1] = upper32At(vn->l1Buffer,vn->l1index);
						return TRUE;
					}else
						return FALSE;
				}													
			}
			//break;
		case 2:
			if(vn->atTerminal){
				if (lower32At(vn->l2Buffer,vn->l2index)!=-1){
					tmp =intAt(vn->l3Buffer,vn->l3index);
					if (vn->LN > tmp){
						index = vn->LN-1;
						if (getTokenType(vn,index)==TOKEN_PI_VAL){
							index--;
						}
						if (getTokenDepth(vn,index)==2){
							vn->LN = index;
							return TRUE;
						}else{
							vn->atTerminal = FALSE;
							vn->context[0]=3;
							vn->context[3]=tmp;
							return TRUE;
						}
					} else if (vn->l3index!=vn->l3lower){
						vn->l3index--;
						vn->atTerminal = FALSE;
						vn->context[0]=3;
						vn->context[3]=intAt(vn->l3Buffer,vn->l3index);
						return TRUE;
					} else {
						index = vn->LN-1;
						tokenType = getTokenType(vn,index);
						switch (tokenType) {
						case TOKEN_PI_VAL:
							index--;
						case TOKEN_CHARACTER_DATA:
						case TOKEN_COMMENT:
						case TOKEN_CDATA_VAL:
						
							vn->LN = index;
							vn->atTerminal = TRUE;
							vn->context[0]=2;
							return TRUE;
						default:
							return FALSE;
						}
					}
				}else{
					index= vn->LN-1;
					if (getTokenType(vn,index)==TOKEN_PI_VAL)
						index--;
					if (index > vn->context[2]){
						tokenType = getTokenType(vn,index);
						if (tokenType!= TOKEN_ATTR_VAL){
							vn->LN = index;
							vn->atTerminal = TRUE;
							return TRUE;
						}else
							return FALSE;
					}else{
						return FALSE;
					}
				}	
			}else{
				index = vn->context[2]-1;	
				tokenType = getTokenType(vn,index);
				if (getTokenDepth(vn,index)==1
						&& tokenType!= TOKEN_ATTR_VAL
						&& tokenType!= TOKEN_STARTING_TAG){
					if (tokenType==TOKEN_PI_VAL)
						index--;
					vn->context[0]=1;
					vn->atTerminal = TRUE;
					vn->LN = index;
					return TRUE;
				}else{
					// no more prev sibling element
					if (vn->l2index != vn->l2lower){
						vn->l2index--;
						vn->context[2] = upper32At(vn->l2Buffer,vn->l2index);
						return TRUE;
					}else
						return FALSE;
				}		
			}				
			//break;
		case 3:
			if(!vn->atTerminal){
				index = vn->context[3]-1;	
				tokenType = getTokenType(vn,index);
				if (getTokenDepth(vn,index)==2
						&& tokenType!= TOKEN_ATTR_VAL
						&& tokenType!= TOKEN_STARTING_TAG){
					if (tokenType==TOKEN_PI_VAL)
						index--;
					vn->context[0]=2;
					vn->atTerminal = TRUE;
					vn->LN = index;
					return TRUE;
				}else{
					// no more prev sibling element
					if (vn->l3index != vn->l3lower){
						vn->l3index--;
						vn->context[3] = intAt(vn->l3Buffer,vn->l3index);
						return TRUE;
					}else
						return FALSE;
				}		
			}
			//break;
		default:
			if (vn->atTerminal){
				index = vn->LN-1;
				tmp =vn->context[0]+1;
			}
			else{
				index = vn->context[vn->context[0]] - 1;
				tmp = vn->context[0];
			}
			while (index > vn->context[tmp-1]) {
				Long temp = longAt(vn->vtdBuffer,index);
				tokenType = (int) ((MASK_TOKEN_TYPE & temp) >>  60);
				depth = (int) ((MASK_TOKEN_DEPTH & temp) >> 52);
				switch (tokenType) {
				case TOKEN_STARTING_TAG:
					
					/*if (depth < tmp) {
						return FALSE;
					} else*/ if (depth == tmp) {
						vn->context[0] = depth;
						vn->context[vn->context[0]] = index;
						vn->atTerminal = FALSE;
						return TRUE;
					}else 
						index--;
					break;
				case TOKEN_ATTR_VAL:
				//case TOKEN_ATTR_NS:
					index -= 2;
					break;
				case TOKEN_CHARACTER_DATA:
				case TOKEN_COMMENT:
				case TOKEN_CDATA_VAL:
					//depth = (int) ((MASK_TOKEN_DEPTH & temp) >> 52);
					/*if (depth < tmp-1) {
						return FALSE;
					} else*/ if (depth == tmp-1) {
						vn->context[0] = tmp-1;
						vn->LN = index;
						vn->atTerminal = TRUE;
						return TRUE;
					} else
						index--;
					break;
				case TOKEN_PI_VAL:
					//depth = (int) ((MASK_TOKEN_DEPTH & temp) >> 52);
					/*if (depth < context[0]) {
						return FALSE;
					} else*/
					if (depth == (tmp-1)) {
						vn->context[0] = tmp-1;
						vn->LN = index-1;
						vn->atTerminal = TRUE;
						return TRUE;
					} else
						index -= 2;
					break;
				default:
					index--;
				}
				
			}
			return FALSE;
		}		
	}

	
	
	Boolean toNode_PrevSibling_L5(VTDNav_L5 *vn){
		int index,tokenType,depth,tmp;
		switch (vn->context[0]) {
		case -1:
			if(vn->atTerminal){
				index = vn->LN-1;
				if (index>0){
					depth = getTokenDepth((VTDNav *)vn,index);
					if (depth==-1){
						tokenType = getTokenType((VTDNav *)vn,index);
						switch (tokenType) {
						case TOKEN_PI_VAL:
							index--;
						case TOKEN_COMMENT:
							vn->LN = index;
							return TRUE;
						default:
							return FALSE;
						}
					}else{
						vn->context[0] = 0;
						vn->atTerminal = FALSE;
						return TRUE;
					}
				}else{
					return FALSE;
				}
			}else{
				return FALSE;
			}
		
		case 0:
			if(vn->atTerminal){
				if (vn->l1Buffer->size!=0){
					// three cases
					if (vn->LN < upper32At(vn->l1Buffer,vn->l1index)){
						index = vn->LN-1;
						if (index>vn->rootIndex){
							tokenType = getTokenType((VTDNav *)vn,index);
							depth = getTokenDepth((VTDNav *)vn,index);								
							if (depth == 0){
								switch(tokenType){									
								case TOKEN_CHARACTER_DATA:
								case TOKEN_COMMENT:
								case TOKEN_CDATA_VAL:
									vn->LN = index;
									return TRUE;
								case TOKEN_PI_VAL:
									vn->LN = index -1;
									return TRUE;
								}
							}								
							if (vn->l1index==0)
								return FALSE;
							vn->l1index--;
							vn->atTerminal = FALSE;
							vn->context[0]=1;
							vn->context[1]= upper32At(vn->l1Buffer,vn->l1index);
							return TRUE;
						}else 
							return FALSE;
					} else {
						index =vn-> LN -1;
						if (index>upper32At(vn->l1Buffer,vn->l1index)){
							tokenType = getTokenType((VTDNav *)vn,index);
							depth = getTokenDepth((VTDNav *)vn,index);								
							if (depth == 0){
								switch(tokenType){									
								case TOKEN_CHARACTER_DATA:
								case TOKEN_COMMENT:
								case TOKEN_CDATA_VAL:
									vn->LN = index;
									return TRUE;
								case TOKEN_PI_VAL:
									vn->LN = index -1;
									return TRUE;
								}
							}										
						}
						vn->atTerminal = FALSE;
						vn->context[0]=1;
						vn->context[1]= upper32At(vn->l1Buffer,vn->l1index);
						return TRUE;
					}						
				}else{
					index = vn->LN-1;
					if (index>vn->rootIndex){
						tokenType=getTokenType((VTDNav *)vn,index);
						switch (tokenType) {
						case TOKEN_PI_VAL:
							index--;
						case TOKEN_CHARACTER_DATA:
						case TOKEN_COMMENT:
						case TOKEN_CDATA_VAL:
						
							vn->LN = index;
							vn->atTerminal = TRUE;
							vn->context[0]=0;
							return TRUE;
						default:
							return FALSE;
						}
					}
				}
				return FALSE;
				
			}else{
				index = vn->rootIndex-1;
				if (index>0){
					tokenType = getTokenType((VTDNav *)vn,index);
					switch (tokenType) {
					case TOKEN_PI_VAL:
						index--;
					case TOKEN_COMMENT:
						vn->LN = index;
						vn->atTerminal = TRUE;
						vn->context[0]=-1;
						return TRUE;
					default:
						return FALSE;
					}
				}else{
					return FALSE;
				}
			}
			//break;
		case 1:
			if(vn->atTerminal){
				if (lower32At(vn->l1Buffer,vn->l1index)!=-1){
					tmp = upper32At(vn->l2Buffer,vn->l2index);
					if (vn->LN > tmp){
						index = vn->LN-1;
						if (getTokenType((VTDNav *)vn,index)==TOKEN_PI_VAL){
							index--;
						}
						if (getTokenDepth((VTDNav *)vn,index)==1){
							vn->LN = index;
							return TRUE;
						}else{
							vn->atTerminal = FALSE;
							vn->context[0]=2;
							vn->context[2]=tmp;
							return TRUE;
						}
					} else if (vn->l2index!=vn->l2lower){
						vn->l2index--;
						vn->atTerminal = FALSE;
						vn->context[0]=2;
						vn->context[2]=upper32At(vn->l2Buffer,vn->l2index);
						return TRUE;
					} else {
						index = vn->LN-1;
						tokenType = getTokenType((VTDNav *)vn,index);
						switch (tokenType) {
						case TOKEN_PI_VAL:
							index--;
						case TOKEN_CHARACTER_DATA:
						case TOKEN_COMMENT:
						case TOKEN_CDATA_VAL:
						
							vn->LN = index;
							vn->atTerminal = TRUE;
							vn->context[0]=1;
							return TRUE;
						default:
							return FALSE;
						}
					}
				}else{
					index= vn->LN-1;
					if (getTokenType((VTDNav *)vn,index)==TOKEN_PI_VAL)
						index--;
					if (index > vn->context[1]){
						tokenType = getTokenType((VTDNav *)vn,index);
						if (tokenType!= TOKEN_ATTR_VAL){
							vn->LN = index;
							vn->atTerminal = TRUE;
							return TRUE;
						}else
							return FALSE;
					}else{
						return FALSE;
					}
				}					
			}else{
				index = vn->context[1]-1;	
				tokenType = getTokenType((VTDNav *)vn,index);
				if (getTokenDepth((VTDNav *)vn,index)==0
						&& tokenType!= TOKEN_ATTR_VAL
						&& tokenType!= TOKEN_STARTING_TAG){
					if (tokenType==TOKEN_PI_VAL)
						index--;
					vn->context[0]=0;
					vn->atTerminal = TRUE;
					vn->LN = index;
					return TRUE;
				}else{
					// no more prev sibling element
					if (vn->l1index != 0){
						vn->l1index--;
						vn->context[1] = upper32At(vn->l1Buffer,vn->l1index);
						return TRUE;
					}else
						return FALSE;
				}													
			}
			//break;
		case 2:
			if(vn->atTerminal){
				if (lower32At(vn->l2Buffer,vn->l2index)!=-1){
					tmp = upper32At(vn->_l3Buffer,vn->l3index);
					if (vn->LN > tmp){
						index = vn->LN-1;
						if (getTokenType((VTDNav *)vn,index)==TOKEN_PI_VAL){
							index--;
						}
						if (getTokenDepth((VTDNav *)vn,index)==2){
							vn->LN = index;
							return TRUE;
						}else{
							vn->atTerminal = FALSE;
							vn->context[0]=3;
							vn->context[3]=tmp;
							return TRUE;
						}
					} else if (vn->l3index!=vn->l3lower){
						vn->l3index--;
						vn->atTerminal = FALSE;
						vn->context[0]=3;
						vn->context[3]=upper32At(vn->_l3Buffer,vn->l3index);
						return TRUE;
					} else {
						index = vn->LN-1;
						tokenType = getTokenType((VTDNav *)vn,index);
						switch (tokenType) {
						case TOKEN_PI_VAL:
							index--;
						case TOKEN_CHARACTER_DATA:
						case TOKEN_COMMENT:
						case TOKEN_CDATA_VAL:
						
							vn->LN = index;
							vn->atTerminal = TRUE;
							vn->context[0]=2;
							return TRUE;
						default:
							return FALSE;
						}
					}
				}else{
					index= vn->LN-1;
					if (getTokenType((VTDNav *)vn,index)==TOKEN_PI_VAL)
						index--;
					if (index > vn->context[2]){
						tokenType = getTokenType((VTDNav *)vn,index);
						if (tokenType!= TOKEN_ATTR_VAL){
							vn->LN = index;
							vn->atTerminal = TRUE;
							return TRUE;
						}else
							return FALSE;
					}else{
						return FALSE;
					}
				}	
			}else{
				index = vn->context[2]-1;	
				tokenType = getTokenType((VTDNav *)vn,index);
				if (getTokenDepth((VTDNav *)vn,index)==1
						&& tokenType!= TOKEN_ATTR_VAL
						&& tokenType!= TOKEN_STARTING_TAG){
					if (tokenType==TOKEN_PI_VAL)
						index--;
					vn->context[0]=1;
					vn->atTerminal = TRUE;
					vn->LN = index;
					return TRUE;
				}else{
					// no more prev sibling element
					if (vn->l2index != vn->l2lower){
						vn->l2index--;
						vn->context[2] = upper32At(vn->l2Buffer,vn->l2index);
						return TRUE;
					}else
						return FALSE;
				}		
			}				
			//break;
		case 3:
			if(vn->atTerminal){
				if (lower32At(vn->_l3Buffer,vn->l3index)!=-1){
					tmp = upper32At(vn->l4Buffer,vn->l4index);
					if (vn->LN > tmp){
						index = vn->LN-1;
						if (getTokenType((VTDNav *)vn,index)==TOKEN_PI_VAL){
							index--;
						}
						if (getTokenDepth((VTDNav *)vn,index)==3){
							vn->LN = index;
							return TRUE;
						}else{
							vn->atTerminal = FALSE;
							vn->context[0]=4;
							vn->context[4]=tmp;
							return TRUE;
						}
					} else if (vn->l4index!=vn->l4lower){
						vn->l4index--;
						vn->atTerminal = FALSE;
						vn->context[0]=4;
						vn->context[4]=upper32At(vn->l4Buffer,vn->l4index);
						return TRUE;
					} else {
						index = vn->LN-1;
						tokenType = getTokenType((VTDNav *)vn,index);
						switch (tokenType) {
						case TOKEN_PI_VAL:
							index--;
						case TOKEN_CHARACTER_DATA:
						case TOKEN_COMMENT:
						case TOKEN_CDATA_VAL:
						
							vn->LN = index;
							vn->atTerminal = TRUE;
							vn->context[0]=3;
							return TRUE;
						default:
							return FALSE;
						}
					}
				}else{
					index= vn->LN-1;
					if (getTokenType((VTDNav *)vn,index)==TOKEN_PI_VAL)
						index--;
					if (index > vn->context[3]){
						tokenType = getTokenType((VTDNav *)vn,index);
						if (tokenType!= TOKEN_ATTR_VAL){
							vn->LN = index;
							vn->atTerminal = TRUE;
							return TRUE;
						}else
							return FALSE;
					}else{
						return FALSE;
					}
				}	
			}else{
				index = vn->context[3]-1;	
				tokenType = getTokenType((VTDNav *)vn,index);
				if (getTokenDepth((VTDNav *)vn,index)==2
						&& tokenType!= TOKEN_ATTR_VAL
						&& tokenType!= TOKEN_STARTING_TAG){
					if (tokenType==TOKEN_PI_VAL)
						index--;
					vn->context[0]=2;
					vn->atTerminal = TRUE;
					vn->LN = index;
					return TRUE;
				}else{
					// no more prev sibling element
					if (vn->l3index != vn->l3lower){
						vn->l3index--;
						vn->context[3] = upper32At(vn->_l3Buffer,vn->l3index);
						return TRUE;
					}else
						return FALSE;
				}		
			}				
		case 4: 			
			if(vn->atTerminal){
				if (lower32At(vn->l4Buffer,vn->l4index)!=-1){
					tmp = intAt(vn->l5Buffer,vn->l5index);
					if (vn->LN > tmp){
						index = vn->LN-1;
						if (getTokenType((VTDNav *)vn,index)==TOKEN_PI_VAL){
							index--;
						}
						if (getTokenDepth((VTDNav *)vn,index)==4){
							vn->LN = index;
							return TRUE;
						}else{
							vn->atTerminal = FALSE;
							vn->context[0]=5;
							vn->context[5]=tmp;
							return TRUE;
						}
					} else if (vn->l5index!=vn->l5lower){
						vn->l5index--;
						vn->atTerminal = FALSE;
						vn->context[0]=5;
						vn->context[5]=intAt(vn->l5Buffer,vn->l5index);
						return TRUE;
					} else {
						index = vn->LN-1;
						tokenType = getTokenType((VTDNav *)vn,index);
						switch (tokenType) {
						case TOKEN_PI_VAL:
							index--;
						case TOKEN_CHARACTER_DATA:
						case TOKEN_COMMENT:
						case TOKEN_CDATA_VAL:
						
							vn->LN = index;
							vn->atTerminal = TRUE;
							vn->context[0]=4;
							return TRUE;
						default:
							return FALSE;
						}
					}
				}else{
					index= vn->LN-1;
					if (getTokenType((VTDNav *)vn,index)==TOKEN_PI_VAL)
						index--;
					if (index > vn->context[4]){
						tokenType = getTokenType((VTDNav *)vn,index);
						if (tokenType!= TOKEN_ATTR_VAL){
							vn->LN = index;
							vn->atTerminal = TRUE;
							return TRUE;
						}else
							return FALSE;
					}else{
						return FALSE;
					}
				}	
			}else{
				index = vn->context[4]-1;	
				tokenType = getTokenType((VTDNav *)vn,index);
				if (getTokenDepth((VTDNav *)vn,index)==3
						&& tokenType!= TOKEN_ATTR_VAL
						&& tokenType!= TOKEN_STARTING_TAG){
					if (tokenType==TOKEN_PI_VAL)
						index--;
					vn->context[0]=3;
					vn->atTerminal = TRUE;
					vn->LN = index;
					return TRUE;
				}else{
					// no more prev sibling element
					if (vn->l4index != vn->l4lower){
						vn->l4index--;
						vn->context[4] = upper32At(vn->l4Buffer,vn->l4index);
						return TRUE;
					}else
						return FALSE;
				}		
			}				
		case 5: 
			if(!vn->atTerminal){
				index = vn->context[5]-1;	
				tokenType = getTokenType((VTDNav *)vn,index);
				if (getTokenDepth((VTDNav *)vn,index)==4
						&& tokenType!= TOKEN_ATTR_VAL
						&& tokenType!= TOKEN_STARTING_TAG){
					if (tokenType==TOKEN_PI_VAL)
						index--;
					vn->context[0]=4;
					vn->atTerminal = TRUE;
					vn->LN = index;
					return TRUE;
				}else{
					// no more prev sibling element
					if (vn->l5index != vn->l5lower){
						vn->l5index--;
						vn->context[5] = intAt(vn->l5Buffer,vn->l5index);
						return TRUE;
					}else
						return FALSE;
				}		
			}
		default:
			if (vn->atTerminal){
				index = vn->LN-1;
				tmp = vn->context[0]+1;
			}
			else{
				index =vn-> context[vn->context[0]] - 1;
				tmp = vn->context[0];
			}
			while (index > vn->context[tmp-1]) {
				Long temp = longAt(vn->vtdBuffer,index);
				tokenType = (int) ((MASK_TOKEN_TYPE & temp) >> 60) & 0x0f;
				depth = (int) ((MASK_TOKEN_DEPTH & temp) >> 52);
				switch (tokenType) {
				case TOKEN_STARTING_TAG:
					if (depth == tmp) {
						vn->context[0] = tmp;
						vn->context[vn->context[0]] = index;
						vn->atTerminal = FALSE;
						return TRUE;
					}else 
						index--;
					break;
				case TOKEN_ATTR_VAL:
				//case TOKEN_ATTR_NS:
					index -= 2;
					break;
				case TOKEN_CHARACTER_DATA:
				case TOKEN_COMMENT:
				case TOKEN_CDATA_VAL:
					if (depth == tmp-1) {
						vn->context[0]=tmp-1;
						vn->LN = index;
						vn->atTerminal = TRUE;
						return TRUE;
					} else
						index--;
					break;
				case TOKEN_PI_VAL:
					if (depth == (tmp-1)) {
						vn->context[0] = tmp-1;
						vn->LN = index-1;
						vn->atTerminal = TRUE;
						return TRUE;
					} else
						index -= 2;
					break;
				default:
					index--;
				}
				
			}
			return FALSE;
		}		
	}

	void _sync(VTDNav *vn,int depth, int index){
		int i, temp,size;
		//int t=-1;
		switch(depth){
		case -1: return;
		case 0: 
			if(vn->l1Buffer->size!=0){
				if (vn->l1index==-1)
					vn->l1index=0;
				
				if (index>upper32At( vn->l1Buffer, vn->l1Buffer->size-1)){
					vn->l1index = vn->l1Buffer->size-1;
					return;
				}
				
				if (index > upper32At(vn->l1Buffer,vn->l1index)){
					while (vn->l1index < vn->l1Buffer->size - 1 && upper32At(vn->l1Buffer,vn->l1index) < index) {
						vn->l1index++;
					}
				}
				else{
					while (vn->l1index >0 && upper32At(vn->l1Buffer,vn->l1index-1) > index) {
						vn->l1index--;
					}
				}
				//assert(index<l1Buffer.upper32At(l1index));
			}
			break;
		case 1:
			if (lower32At(vn->l1Buffer,vn->l1index)!=-1){
				if (vn->l2lower!=lower32At(vn->l1Buffer,vn->l1index)){
					vn->l2lower =vn-> l2index=lower32At(vn->l1Buffer,vn->l1index);
					vn->l2upper = vn->l2Buffer->size - 1;
					size =vn-> l1Buffer->size;
					for (i = vn->l1index + 1; i < size; i++) {
						temp = lower32At(vn->l1Buffer,i);
						if (temp != 0xffffffff) {
							vn->l2upper = temp - 1;
							break;
						}
					}
					//l2upper = l1Buffer.lower32At(l1index);
				} 
				
				if (index>upper32At(vn->l2Buffer,vn->l2index)){
					while (vn->l2index < vn->l2upper && upper32At(vn->l2Buffer,vn->l2index)< index){
						vn->l2index++;					
					}
				} else {
					while(vn->l2index > vn->l2lower && upper32At(vn->l2Buffer,vn->l2index-1)> index){
						vn->l2index--;
					}
				}
				
				//assert(index<l2Buffer.upper32At(l2index));
			}
			
			break;
		case 2:		
			if (lower32At(vn->l2Buffer,vn->l2index)!=-1){
				if (vn->l3lower!=lower32At(vn->l2Buffer,vn->l2index)){
					vn->l3index = vn->l3lower =lower32At( vn->l2Buffer,vn->l2index);
					vn->l3upper = vn->l3Buffer->size - 1;
					size = vn->l2Buffer->size;
					for (i = vn->l2index + 1; i < size; i++) {
						temp = lower32At(vn->l2Buffer,i);
						if (temp != 0xffffffff) {
							vn->l3upper = temp - 1;
							break;
						}
					}
				}
				if (index>intAt(vn->l3Buffer,vn->l3index)){
					while (vn->l3index < vn->l3upper &&intAt(vn->l3Buffer, vn->l3index)<index  ){
						vn->l3index++;
					}
				}else {
					while(vn->l3index > vn->l3lower && intAt(vn->l3Buffer,vn->l3index-1)> index){
						vn->l3index--;
					}
				}
				
				//assert(index<l3Buffer.intAt(l3index));
			}
			break;
		default:
			if (lower32At(vn->l2Buffer,vn->l2index)!=-1){
				if (vn->l3lower!=lower32At(vn->l2Buffer,vn->l2index)){
					vn->l3index = vn->l3lower = lower32At(vn->l2Buffer,vn->l2index);
					vn->l3upper = vn->l3Buffer->size - 1;
					size = vn->l2Buffer->size;
					for (i = vn->l2index + 1; i < size; i++) {
						temp = lower32At(vn->l2Buffer,i);
						if (temp != 0xffffffff) {
							vn->l3upper = temp - 1;
							break;
						}
					}
				}
				//if (context[3]> l3Buffer.intAt(l3index)){
					while (vn->context[3] !=intAt( vn->l3Buffer,vn->l3index)){
						vn->l3index++;
					}
				//} else {
				//	while (context[3] != l3Buffer.intAt(l3index)){
				//		l3index--;
				//	}
				//}
				
				//assert(index<l3Buffer.intAt(l3index));
			}
			break;
		}
	}

	void _sync_L5(VTDNav_L5 *vn,int depth, int index){
		int i,size;
			switch(depth){
		case -1: return;
		case 0: 
			if(vn->l1Buffer->size!=0){
				if (vn->l1index==-1)
					vn->l1index=0;
				
				if (index> upper32At(vn->l1Buffer,vn->l1Buffer->size-1)){
					vn->l1index = vn->l1Buffer->size-1;
					return;
				}
				
				if (index > upper32At(vn->l1Buffer,vn->l1index)){
					while (vn->l1index < vn->l1Buffer->size - 1 && upper32At(vn->l1Buffer,vn->l1index) < index) {
						vn->l1index++;
					}
					//l1index--;
				}
				else{
					while (vn->l1index >0 && upper32At(vn->l1Buffer,vn->l1index-1) > index) {
						vn->l1index--;
					}
				}
				//assert(index<l1Buffer.upper32At(l1index));
			}
			break;
		case 1:
			if (lower32At(vn->l1Buffer,vn->l1index)!=-1){
				if (vn->l2lower!=lower32At(vn->l1Buffer,vn->l1index)){
					vn->l2lower = vn->l2index=lower32At(vn->l1Buffer,vn->l1index);
					vn->l2upper = vn->l2Buffer->size - 1;
					size = vn->l1Buffer->size;
					for (i = vn->l1index + 1; i < size; i++) {
						int temp = lower32At(vn->l1Buffer,i);
						if (temp != 0xffffffff) {
							vn->l2upper = temp - 1;
							break;
						}
					}
					//l2upper = l1Buffer.lower32At(l1index);
				} 
				
				if (index>upper32At(vn->l2Buffer,vn->l2index)){
					while (vn->l2index < vn->l2upper && upper32At(vn->l2Buffer,vn->l2index)< index){
						vn->l2index++;					
					}
				} else {
					while(vn->l2index > vn->l2lower && upper32At(vn->l2Buffer,vn->l2index-1)> index){
						vn->l2index--;
					}
				}
				//assert(index<l2Buffer.upper32At(l2index));
			}
			
			break;
		case 2:
			if (lower32At(vn->l2Buffer,vn->l2index)!=-1){
				if (vn->l3lower!=lower32At(vn->l2Buffer,vn->l2index)){
					vn->l3index = vn->l3lower = lower32At(vn->l2Buffer,vn->l2index);
					vn->l3upper = vn->_l3Buffer->size - 1;
					size = vn->l2Buffer->size;
					for (i = vn->l2index + 1; i < size; i++) {
						int temp = lower32At(vn->l2Buffer,i);
						if (temp != 0xffffffff) {
							vn->l3upper = temp - 1;
							break;
						}
					}
				}
				if (index>upper32At(vn->_l3Buffer,vn->l3index)){
					while (vn->l3index < vn->l3upper && upper32At(vn->_l3Buffer,vn->l3index)<index  ){
						vn->l3index++;
					}
				}else {
					while(vn->l3index > vn->l3lower && upper32At(vn->_l3Buffer,vn->l3index-1)> index){
						vn->l3index--;
					}
				}
				//assert(index<l3Buffer.intAt(l3index));
			}
			break;
			
		case 3:
			if (lower32At(vn->_l3Buffer,vn->l3index)!=-1){
				if (vn->l4lower!=lower32At(vn->_l3Buffer,vn->l3index)){
					vn->l4index = vn->l4lower =lower32At( vn->_l3Buffer, vn->l3index);
					vn->l4upper = vn->l4Buffer->size - 1;
					size = vn->_l3Buffer->size;
					for ( i = vn->l3index + 1; i < size; i++) {
						int temp =lower32At( vn->_l3Buffer,i);
						if (temp != 0xffffffff) {
							vn->l4upper = temp - 1;
							break;
						}
					}
				}
				
				if (index>upper32At(vn->l4Buffer,vn->l4index)){
					while (vn->l4index <vn-> l4upper && upper32At(vn->l4Buffer,vn->l4index)< index){
						vn->l4index++;					
					}
				} else {
					while(vn->l4index >vn->l4lower && upper32At(vn->l4Buffer,vn->l4index-1)> index){
						vn->l4index--;
					}
				}
				//assert(index<l3Buffer.intAt(l3index));
			}
			break;
			
		case 4:
			if (lower32At(vn->l4Buffer,vn->l4index)!=-1){
				if (vn->l5lower!=lower32At(vn->l4Buffer,vn->l4index)){
					vn->l5index = vn->l5lower = lower32At(vn->l4Buffer,vn->l4index);
					vn->l5upper = vn->l5Buffer->size - 1;
					size = vn->l4Buffer->size;
					for (i = vn->l4index + 1; i < size; i++) {
						int temp = lower32At(vn->l4Buffer,i);
						if (temp != 0xffffffff) {
							vn->l5upper = temp - 1;
							break;
						}
					}
				}
				
				if (index>intAt(vn->l5Buffer,vn->l5index)){
					while (vn->l5index < vn->l5upper && intAt(vn->l5Buffer,vn->l5index)<index  ){
						vn->l5index++;
					}
				}else {
					while(vn->l5index > vn->l5lower && intAt(vn->l5Buffer,vn->l5index-1)> index){
						vn->l5index--;
					}
				}
				//assert(index<l3Buffer.intAt(l3index));
			}
			break;
			
		default:
			if (lower32At(vn->l4Buffer,vn->l4index)!=-1){
				if (vn->l5lower!=lower32At(vn->l4Buffer,vn->l4index)){
					vn->l5index = vn->l5lower = lower32At(vn->l4Buffer,vn->l4index);
					vn->l5upper = vn->l5Buffer->size - 1;
					size = vn->l4Buffer->size;
					for (i =vn-> l4index + 1; i < size; i++) {
						int temp = lower32At(vn->l4Buffer,i);
						if (temp != 0xffffffff) {
							vn->l5upper = temp - 1;
							break;
						}
					}
				}
				
				//if (context[5]> l5Buffer.intAt(l5index)){
				while (vn->context[5] !=intAt( vn->l5Buffer,vn->l5index)){
					vn->l5index++;
				}
				/*} else {
					while (context[5] != l5Buffer.intAt(l5index)){
						l5index--;
					}
				}*/
				
				//assert(index<l3Buffer.intAt(l3index));
			}
			break;
		}
	}

	inline void sync(VTDNav *vn, int depth, int index){
		vn->__sync(vn,depth, index);
	}


	Boolean _verifyNodeCorrectness(VTDNav *vn){
		int i1, i2, tmp;
	 	if (vn->atTerminal){
			// check l1 index, l2 index, l2lower, l2upper, l3 index, l3 lower, l3 upper
			if (getTokenDepth(vn,vn->LN)!=vn->context[0])
				return FALSE;
			switch(vn->context[0]){
				case -1: return TRUE;
				case 0: 
					//if (getTokenDepth(LN)!=0)
					//	return FALSE;
					if (vn->l1Buffer->size!=0){
						if (vn->l1index>=vn->l1Buffer->size || vn->l1index<0)
							return FALSE;
						if (vn->l1index != vn->l1Buffer->size-1){
							
							if (upper32At(vn->l1Buffer,vn->l1index)<vn->LN)
								return FALSE;								
						}						
						return TRUE;
					}else
						return TRUE;
					
			case 1:
				if (vn->LN>vn->context[1]){
					//if (getTokenDepth(LN) != 1)
					//	return FALSE;
					if (vn->l1index<0 || vn->l1index>vn->l1Buffer->size)
						return FALSE;
					//int i1, i2; // l2lower, l2upper and l2index
					i1 = lower32At(vn->l1Buffer,vn->l1index);
					if (i1 != -1) {
						if (i1 != vn->l2lower)
							return FALSE;
						tmp = vn->l1index + 1;
						i2 = vn->l2Buffer->size - 1;
						while (tmp < vn->l1Buffer->size) {
							if (lower32At(vn->l1Buffer,tmp) != -1) {
								i2 = lower32At(vn->l1Buffer,tmp) - 1;
								break;
							} else
								tmp++;
						}
						if (vn->l2upper != i2)
							return FALSE;
						if (vn->l2index > vn->l2upper || vn->l2index <vn-> l2lower)
							return FALSE;
						if (vn->l2index != vn->l2upper) {
							if (upper32At(vn->l2Buffer,vn->l2index) < vn->LN)
								return FALSE;
						} 
						if (vn->l2index!=vn->l2lower){
							if (upper32At(vn->l2Buffer,vn->l2index-1)>vn->LN)
								return FALSE;
						}
					}
					return TRUE;
				}else
					return FALSE;
			case 2:  
				if (vn->LN>vn->context[2] && vn->context[2]> vn->context[1]){
					//if (getTokenDepth(LN) != 2)
					//	return FALSE;
					if (vn->l1index<0 || vn->l1index>vn->l1Buffer->size)
						return FALSE;
					
					i1 = lower32At(vn->l1Buffer,vn->l1index);
					if(i1==-1)return FALSE;
					if (i1!=vn->l2lower)
						return FALSE;
					tmp = vn->l1index+1;
					i2 = vn->l2Buffer->size-1;
					while(tmp<vn->l1Buffer->size){
						if (lower32At(vn->l1Buffer,tmp)!=-1){
							i2 = lower32At(vn->l1Buffer,tmp)-1;
							break;
						}else
							tmp++;
					}
					if(vn->context[2]!=upper32At(vn->l2Buffer,vn->l2index)){
						return FALSE;
					}
					if (vn->l2index>vn->l2upper || vn->l2index < vn->l2lower){
						return FALSE;
					}
					//l3 
					i1 = lower32At(vn->l2Buffer,vn->l2index);
					if (i1!=-1){
						if (vn->l3lower!=i1)
							return FALSE;
						i2 = vn->l3Buffer->size-1;
						tmp = vn->l2index+1;
						
						while(tmp<vn->l2Buffer->size){
							if (lower32At(vn->l2Buffer,tmp)!=-1){
								i2 = lower32At(vn->l2Buffer,tmp)-1;
								break;
							}else
								tmp++;
						}
						
						if (vn->l3lower!=i1)
							return FALSE;
						
						if (vn->l3upper!=i2)
							return FALSE;
						
						if (vn->l3index<i1 || vn->l3index>i2)
							return FALSE;
						
						if (vn->l3index != vn->l3upper) {
							if (intAt(vn->l3Buffer,vn->l3index) < vn->LN)
								return FALSE;
						} 		
						if (vn->l3index!=vn->l3lower){
							if (intAt(vn->l3Buffer,vn->l3index-1)>vn->LN)
								return FALSE;
						}
					}
					return TRUE;
				}else 
					return FALSE;
				
			default:  
				//if (getTokenDepth(LN) != 2)
				//	return FALSE;
				if (vn->l1index<0 || vn->l1index>vn->l1Buffer->size)
					return FALSE;
				//int i1,i2, i3; //l2lower, l2upper and l2index
				i1 = lower32At(vn->l1Buffer,vn->l1index);
				
				if (i1==-1)return FALSE;
				if (i1!=vn->l2lower)
					return FALSE;
				tmp = vn->l1index+1;
				i2 = vn->l2Buffer->size-1;
				while(tmp<vn->l1Buffer->size){
					if (lower32At(vn->l1Buffer,tmp)!=-1){
						i2 = lower32At(vn->l1Buffer,tmp)-1;
						break;
					}else
						tmp++;
				}
				if(vn->context[2]!=upper32At(vn->l2Buffer,vn->l2index)){
					return FALSE;
				}
				if (vn->l2index>vn->l2upper || vn->l2index < vn->l2lower){
					return FALSE;
				}
				//l3 
				i1 = lower32At(vn->l2Buffer,vn->l2index);
				if (i1==-1)
					return FALSE;
				if (i1!=vn->l3lower)
					return FALSE;
				i2 = vn->l3Buffer->size-1;
				tmp = vn->l2index+1;
				
				while(tmp<vn->l2Buffer->size){
					if (lower32At(vn->l2Buffer,tmp)!=-1){
						i2 = lower32At(vn->l2Buffer,tmp)-1;
						break;
					}else
						tmp++;
				}
					
				if (vn->l3lower!=i1)
					return FALSE;
					
				if (vn->l3upper!=i2)
					return FALSE;
					
				if (vn->l3index<i1 || vn->l3index>i2)
					return FALSE;
					
				if (vn->context[vn->context[0]]>vn->LN)
					return FALSE;
				
				if (vn->context[0]==3){
					if (vn->l3index!=vn->l3upper){
						if(intAt(vn->l3Buffer,vn->l3index)>vn->LN)
							return FALSE;
					}
					if (vn->l3index+1 <= vn->l3Buffer->size-1){
						if (intAt(vn->l3Buffer,vn->l3index+1)<vn->LN){
							return FALSE;
						}
					}
				}
							
			}
			return TRUE;
				
			
		}else {
			switch(vn->context[0]){
			case -1:
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
				default:return TRUE;
			}
			
		}
	}


	Boolean _verifyNodeCorrectness_L5(VTDNav_L5 *vn){
		int i1, i2, tmp;
		 	if (vn->atTerminal){
			// check l1 index, l2 index, l2lower, l2upper, l3 index, l3 lower, l3 upper
			if (getTokenDepth((VTDNav *)vn,vn->LN)!=vn->context[0])
				return FALSE;
			switch(vn->context[0]){
				case -1: return TRUE;
				case 0: 
					//if (getTokenDepth(LN)!=0)
					//	return FALSE;
					if (vn->l1Buffer->size!=0){
						if (vn->l1index>=vn->l1Buffer->size || vn->l1index<0)
							return FALSE;
						if (vn->l1index != vn->l1Buffer->size-1){
							
							if (upper32At(vn->l1Buffer,vn->l1index)<vn->LN)
								return FALSE;								
						}						
						return TRUE;
					}else
						return TRUE;
					
			case 1:
				if (vn->LN>vn->context[1]){
					//if (getTokenDepth(LN) != 1)
					//	return FALSE;
					if (vn->l1index<0 || vn->l1index>vn->l1Buffer->size)
						return FALSE;
					//int i1, i2, i3; // l2lower, l2upper and l2index
					i1 = lower32At(vn->l1Buffer,vn->l1index);
					if (i1 != -1) {
						
						int tmp = vn->l1index + 1;
						i2 = vn->l2Buffer->size - 1;
						while (tmp < vn->l1Buffer->size) {
							if (lower32At(vn->l1Buffer,tmp) != -1) {
								i2 = lower32At(vn->l1Buffer,tmp) - 1;
								break;
							} else
								tmp++;
						}
						if (i1 != vn->l2lower)
							return FALSE;						
						if (vn->l2upper != i2)
							return FALSE;
						if (vn->l2index > vn->l2upper || vn->l2index < vn->l2lower)
							return FALSE;
						if (vn->l2index != vn->l2upper) {
							if (upper32At(vn->l2Buffer,vn->l2index) < vn->LN)
								return FALSE;
						} 
					}
					return TRUE;
				}else
					return FALSE;
			case 2:  
				if (vn->LN>vn->context[2] && vn->context[2]> vn->context[1]){
					//if (getTokenDepth(LN) != 2)
					//	return FALSE;
					if (vn->l1index<0 || vn->l1index>vn->l1Buffer->size)
						return FALSE;
					//int i1,i2, i3; //l2lower, l2upper and l2index
					i1 = lower32At(vn->l1Buffer,vn->l1index);
					if(i1==-1)return FALSE;
					if (i1!=vn->l2lower)
						return FALSE;
					tmp = vn->l1index+1;
					i2 = vn->l2Buffer->size-1;
					while(tmp<vn->l1Buffer->size){
						if (lower32At(vn->l1Buffer,tmp)!=-1){
							i2 = lower32At(vn->l1Buffer,tmp)-1;
							break;
						}else
							tmp++;
					}
					if(vn->context[2]!=upper32At(vn->l2Buffer,vn->l2index)){
						return FALSE;
					}
					if (vn->l2index>vn->l2upper || vn->l2index < vn->l2lower){
						return FALSE;
					}
					//l3 
					i1 = lower32At(vn->l2Buffer,vn->l2index);
					if (i1!=-1){
						if (vn->l3lower!=i1)
							return FALSE;
						i2 = vn->_l3Buffer->size-1;
						tmp = vn->l2index+1;
						
						while(tmp<vn->l2Buffer->size){
							if (lower32At(vn->l2Buffer,tmp)!=-1){
								i2 = lower32At(vn->l2Buffer,tmp)-1;
								break;
							}else
								tmp++;
						}
						
						if (vn->l3lower!=i1)
							return FALSE;
						
						if (vn->l3upper!=i2)
							return FALSE;
						
						if (vn->l3index > vn->l3upper || vn->l3index < vn->l3lower)
							return FALSE;
						if (vn->l3index != vn->l3upper) {
							if (upper32At(vn->_l3Buffer,vn->l3index) < vn->LN)
								return FALSE;
						} 
					}
					return TRUE;
				}else 
					return FALSE;
				
			case 3:
				if (vn->LN>vn->context[3] && vn->context[3]> vn->context[2] &&vn-> context[2]> vn->context[1]){
					//if (getTokenDepth(LN) != 2)
					//	return FALSE;
					if (vn->l1index<0 || vn->l1index>vn->l1Buffer->size)
						return FALSE;
					//int i1,i2, i3; //l2lower, l2upper and l2index
					i1 = lower32At(vn->l1Buffer, vn->l1index);
					if(i1==-1)return FALSE;
					if (i1!=vn->l2lower)
						return FALSE;
					tmp = vn->l1index+1;
					i2 = vn->l2Buffer->size-1;
					while(tmp<vn->l1Buffer->size){
						if (lower32At(vn->l1Buffer,tmp)!=-1){
							i2 = lower32At(vn->l1Buffer,tmp)-1;
							break;
						}else
							tmp++;
					}
					if(vn->context[2]!=upper32At(vn->l2Buffer,vn->l2index)){
						return FALSE;
					}
					if (vn->l2index>vn->l2upper || vn->l2index < vn->l2lower){
						return FALSE;
					}
					//l3 
					i1 = lower32At(vn->l2Buffer,vn->l2index);
					if (i1==-1){return FALSE;}
					if (vn->l3lower!=i1)
						return FALSE;
					i2 = vn->_l3Buffer->size-1;
					tmp = vn->l2index+1;
						
					while(tmp<vn->l2Buffer->size){
						if (lower32At(vn->l2Buffer,tmp)!=-1){
							i2 = lower32At(vn->l2Buffer,tmp)-1;
							break;
						}else
							tmp++;
					}
						
					if (vn->l3lower!=i1)
						return FALSE;
						
					if (vn->l3upper!=i2)
						return FALSE;
						
					if (vn->l3index > vn->l3upper || vn->l3index < vn->l3lower)
							return FALSE;
					/*if (l3index != l3upper) {
						if (l3Buffer.upper32At(l3index) < LN)
							return FALSE;
					} */
					//l4
					i1 = lower32At(vn->_l3Buffer,vn->l3index);
					if (i1!=-1){
						if (vn->l4lower!=i1)
							return FALSE;
						i2 = vn->l4Buffer->size-1;
						tmp = vn->l3index+1;
						
						while(tmp<vn->_l3Buffer->size){
							if (lower32At(vn->_l3Buffer,tmp)!=-1){
								i2 = lower32At(vn->_l3Buffer,tmp)-1;
								break;
							}else
								tmp++;
						}
						
						if (vn->l4lower!=i1)
							return FALSE;
						
						if (vn->l4upper!=i2)
							return FALSE;
						
						if (vn->l4index > vn->l4upper || vn->l4index < vn->l4lower)
							return FALSE;
						if (vn->l4index != vn->l4upper) {
							if (upper32At(vn->l4Buffer,vn->l4index) < vn->LN)
								return FALSE;
						} 
					}
					return TRUE;
				}else 
					return FALSE;
			case 4:
				if (vn->LN>vn->context[3] && vn->context[3]> vn->context[2] && vn->context[2]> vn->context[1]){
					//if (getTokenDepth(LN) != 2)
					//	return FALSE;
					if (vn->l1index<0 || vn->l1index>vn->l1Buffer->size)
						return FALSE;
					//int i1,i2, i3; //l2lower, l2upper and l2index
					i1 = lower32At(vn->l1Buffer,vn->l1index);
					if(i1==-1)return FALSE;
					if (i1!=vn->l2lower)
						return FALSE;
					tmp = vn->l1index+1;
					i2 = vn->l2Buffer->size-1;
					while(tmp<vn->l1Buffer->size){
						if (lower32At(vn->l1Buffer,tmp)!=-1){
							i2 = lower32At(vn->l1Buffer,tmp)-1;
							break;
						}else
							tmp++;
					}
					if(vn->context[2]!=upper32At(vn->l2Buffer,vn->l2index)){
						return FALSE;
					}
					if (vn->l2index>vn->l2upper || vn->l2index < vn->l2lower){
						return FALSE;
					}
					//l3 
					i1 = lower32At(vn->l2Buffer,vn->l2index);
					if (i1==-1){return FALSE;}
					if (vn->l3lower!=i1)
						return FALSE;
					i2 = vn->_l3Buffer->size-1;
					tmp = vn->l2index+1;
						
					while(tmp<vn->l2Buffer->size){
						if (lower32At(vn->l2Buffer,tmp)!=-1){
							i2 = lower32At(vn->l2Buffer,tmp)-1;
							break;
						}else
							tmp++;
					}
						
					if (vn->l3lower!=i1)
						return FALSE;
						
					if (vn->l3upper!=i2)
						return FALSE;
						
					if (vn->l3index > vn->l3upper || vn->l3index < vn->l3lower)
							return FALSE;
					/*if (l3index != l3upper) {
						if (l3Buffer.upper32At(l3index) < LN)
							return FALSE;
					} */
					i1 = lower32At(vn->_l3Buffer,vn->l3index);
					if (i1==-1){ return FALSE;}
					if (vn->l4lower!=i1)
						return FALSE;
					i2 = vn->l4Buffer->size-1;
					tmp = vn->l3index+1;
						
					while(tmp<vn->_l3Buffer->size){
						if (lower32At(vn->_l3Buffer,tmp)!=-1){
							i2 = lower32At(vn->_l3Buffer,tmp)-1;
							break;
						}else
							tmp++;
					}
						
					if (vn->l4lower!=i1)
						return FALSE;
						
					if (vn->l4upper!=i2)
						return FALSE;
						
					if (vn->l4index > vn->l4upper || vn->l4index < vn->l4lower)
						return FALSE;
					/*if (l4index != l4upper) {
						if (l4Buffer.upper32At(l4index) < LN)
							return FALSE;
					}*/
					i1=lower32At(vn->l4Buffer,vn->l4index);
					if (i1!=-1){
						if (i1!=vn->l5lower)return FALSE;
						i2 = vn->l5Buffer->size-1;
						tmp = vn->l4index+1;
						
						while(tmp<vn->l4Buffer->size){
							if (lower32At(vn->l4Buffer,tmp)!=-1){
								i2 = lower32At(vn->l4Buffer,tmp)-1;
								break;
							}else
								tmp++;
						}
						
						if (vn->l5lower!=i1)
							return FALSE;
						
						if (vn->l5upper!=i2)
							return FALSE;
						
						if (vn->l5index<i1 ||vn-> l5index>i2)
							return FALSE;
						
						if (vn->l5index != vn->l5upper) {
							if (intAt(vn->l5Buffer,vn->l5index) < vn->LN)
								return FALSE;
						} 				
					}
					return TRUE;
				}else 
					return FALSE;
				
				
			default:  
				if (vn->l1index<0 || vn->l1index>vn->l1Buffer->size)
					return FALSE;
				//int i1,i2,i3; //l2lower, l2upper and l2index
				i1 = lower32At(vn->l1Buffer,vn->l1index);
				if(i1==-1)return FALSE;
				if (i1!=vn->l2lower)
					return FALSE;
				tmp = vn->l1index+1;
				i2 = vn->l2Buffer->size-1;
				while(tmp<vn->l1Buffer->size){
					if (lower32At(vn->l1Buffer,tmp)!=-1){
						i2 = lower32At(vn->l1Buffer,tmp)-1;
						break;
					}else
						tmp++;
				}
				if(vn->context[2]!=upper32At(vn->l2Buffer,vn->l2index)){
					return FALSE;
				}
				if (vn->l2index>vn->l2upper || vn->l2index < vn->l2lower){
					return FALSE;
				}
				//l3 
				i1 = lower32At(vn->l2Buffer,vn->l2index);
				if (i1==-1){return FALSE;}
				if (vn->l3lower!=i1)
					return FALSE;
				i2 = vn->_l3Buffer->size-1;
				tmp = vn->l2index+1;
					
				while(tmp<vn->l2Buffer->size){
					if (lower32At(vn->l2Buffer,tmp)!=-1){
						i2 = lower32At(vn->l2Buffer,tmp)-1;
						break;
					}else
						tmp++;
				}
					
				if (vn->l3lower!=i1)
					return FALSE;
					
				if (vn->l3upper!=i2)
					return FALSE;
					
				if (vn->l3index >vn->l3upper || vn->l3index < vn->l3lower)
						return FALSE;
				
				i1 = lower32At(vn->_l3Buffer,vn->l3index);
				if (i1==-1){ return FALSE;}
				if (vn->l4lower!=i1)
					return FALSE;
				i2 = vn->l4Buffer->size-1;
				tmp = vn->l3index+1;
					
				while(tmp<vn->_l3Buffer->size){
					if (lower32At(vn->_l3Buffer,tmp)!=-1){
						i2 = lower32At(vn->_l3Buffer,tmp)-1;
						break;
					}else
						tmp++;
				}
					
				if (vn->l4lower!=i1)
					return FALSE;
					
				if (vn->l4upper!=i2)
					return FALSE;
					
				if (vn->l4index > vn->l4upper || vn->l4index < vn->l4lower)
					return FALSE;
				/*if (l4index != l4upper) {
					if (l4Buffer.upper32At(l4index) < LN)
						return FALSE;
				}*/
				i1=lower32At(vn->l4Buffer,vn->l4index);
				
				if (i1!=vn->l5lower)return FALSE;
				i2 = vn->l5Buffer->size-1;
				tmp = vn->l4index+1;
					
				while(tmp<vn->l4Buffer->size){
					if (lower32At(vn->l4Buffer,tmp)!=-1){
						i2 = lower32At(vn->l4Buffer,tmp)-1;
						break;
					}else
						tmp++;
				}
					
				if (vn->l5lower!=i1)
					return FALSE;
					
				if (vn->l5upper!=i2)
					return FALSE;
					
				if (vn->l5index<i1 || vn->l5index>i2)
					return FALSE;
					
				if (vn->context[vn->context[0]]>vn->LN)
					return FALSE;
				
				if (vn->context[0]==5){
					if (vn->l5index!=vn->l5upper){
						if(intAt(vn->l5Buffer,vn->l5index)>vn->LN)
							return FALSE;
					}
					if (vn->l5index+1 <= vn->l5Buffer->size-1){
						if (intAt(vn->l5Buffer,vn->l5index+1)<vn->LN){
							return FALSE;
						}
					}
				}
				return TRUE;
			}
		}else {
			switch(vn->context[0]){
			case -1:
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
				default:return TRUE;
			}
			
		}
	}


	Boolean iterateNode(VTDNav *vn, int dp){
	int index = getCurrentIndex(vn) + 1;
		int tokenType,depth;
		// int size = vtdBuffer.size;
		while (index < vn->vtdSize) {
			tokenType = getTokenType(vn,index);
			switch(tokenType){
			case TOKEN_ATTR_NAME:
			case TOKEN_ATTR_NS:
				index = index + 2;
				continue;
			case TOKEN_STARTING_TAG:
			case TOKEN_DOCUMENT:
				depth = getTokenDepth(vn,index);
				vn->atTerminal = FALSE;
				if (depth > dp) {
					vn->context[0] = depth;
					if (depth > 0)
						vn->context[depth] = index;
					if (dp < vn->maxLCDepthPlusOne)
						resolveLC(vn);
					return TRUE;					
				} else {
					return FALSE;
				}
			case TOKEN_CHARACTER_DATA:
			case TOKEN_COMMENT:
			case TOKEN_PI_NAME:
			case TOKEN_CDATA_VAL:
				depth = getTokenDepth(vn,index);
				
				if (depth >= dp){
					sync(vn,depth,index);
					vn->LN= index;
					vn->context[0]= depth;
					vn->atTerminal = TRUE;
					return TRUE;
				}
				return FALSE;
			default:
				index ++;
			}			
		}
		return FALSE;	
	}

	void setCurrentNode(VTDNav *vn){
		if (vn->currentNode == NULL) {
			vn->currentNode = createBookMark2(vn);
		} else {
			recordCursorPosition2(vn->currentNode);//   ->recordCursorPosition();
		}
	}
	
	void loadCurrentNode(VTDNav *vn){
		setCursorPosition2(vn->currentNode);
	}
	
	void fillXPathString(VTDNav *vn,FastIntBuffer *indexBuffer,FastIntBuffer *countBuffer){
		int index = getCurrentIndex(vn) + 1;
		int tokenType, depth, t = 0, length, i = 0;
		int dp = vn->context[0];
		//int size = vtdBuffer.size;
		// store all text tokens underneath the current element node
		while (index < vn->vtdSize) {
			tokenType = getTokenType(vn,index);
			depth = getTokenDepth(vn,index);
			if (depth<dp ||
				(depth == dp && tokenType == TOKEN_STARTING_TAG)) {
				break;
			}

			if (tokenType == TOKEN_CHARACTER_DATA
				|| tokenType == TOKEN_CDATA_VAL) {
				length = getTokenLength(vn,index);
				t += length;
				appendInt(vn->fib,index);
				if (length > MAX_TOKEN_LENGTH) {
					while (length > MAX_TOKEN_LENGTH) {
						length -= MAX_TOKEN_LENGTH;
						i++;
					}
					index += i + 1;
				}
				else
					index++;
				continue;
				//
			}
			else if (tokenType == TOKEN_ATTR_NAME
				|| tokenType == TOKEN_ATTR_NS) {
				index = index + 2;
				continue;
			}
			index++;
		}
	}

	inline UCSChar *getXPathStringVal(VTDNav *vn,short mode){
		return _getXPathStringVal(vn,getCurrentIndex(vn),mode);
	}
	inline UCSChar *getXPathStringVal2(VTDNav *vn,int i,short mode){
		return _getXPathStringVal(vn, i, mode);
	}

	static UCSChar* _getXPathStringVal(VTDNav *vn,int j,short mode){
		//exception ee;
		int tokenType,os=0;
		UCSChar *sb=NULL;
		int index = j + 1;
		int depth, t = 0, i = 0;
		int dp = getTokenDepth(vn,j);
		//int size = vtdBuffer.size;
		// store all text tokens underneath the current element node
		while (index < vn->vtdSize) {
			tokenType = getTokenType(vn,index);
			depth = getTokenDepth(vn,index);
			t = t + getTokenLength2(vn,index);
			if (depth < dp || (depth == dp && tokenType == TOKEN_STARTING_TAG)) {
				break;
			}
			switch (tokenType) {
		case TOKEN_ATTR_NAME:
		case TOKEN_ATTR_NS:
		case TOKEN_PI_NAME:
			index = index + 2;
			continue;

		case TOKEN_CHARACTER_DATA:
		case TOKEN_CDATA_VAL:
			appendInt(vn->fib,index);
			index++;
			continue;
			}
			index++;
		}

		// calculate the total length
		sb = malloc(sizeof(UCSChar)*(t + 1));
		//int os = 0;
		for (t = 0; t < vn->fib->size; t++) {
			switch (mode) {
		case 0:
			os = _toString(vn,sb, intAt(vn->fib,t), os);
			break;
		case 1:
			os = _toStringUpperCase(vn,sb, intAt(vn->fib,t), os);
			break;
		case 2:
			os = _toStringLowerCase(vn,sb, intAt(vn->fib,t), os);
			break;
		default:
			throwException2( nav_exception, "Invaild xpath string val mode");
			}

		}

		// clear the fib and return a string
		clearFastIntBuffer(vn->fib);
		sb[os] = 0; // END Of STRING
		return sb;
	}

	static int _toString(VTDNav *vn, UCSChar *s, int index, int offset){
		int len = getTokenLength2(vn,index), k = offset;
		int os = getTokenOffset(vn,index);
		int type = getTokenType(vn,index);
		int endOffset = os + len;
		Long l;
		while (os < endOffset) {
			if (type!=TOKEN_CDATA_VAL)
				l = getCharResolved(vn,os);
			else
				l = getChar(vn,os);
			os += (int) (l >> 32);
			s[k++] = (UCSChar) l; // java only support 16 bit unit code
		}

		return k;
	}

	static int _toStringLowerCase(VTDNav *vn, UCSChar *s, int index, int offset){
		int len = getTokenLength2(vn,index), k = offset;
		int os = getTokenOffset(vn,index);
		int type = getTokenType(vn,index);
		int endOffset = os + len;
		Long l;
		while (os < endOffset) {
			if (type!=TOKEN_CDATA_VAL)
				l = getCharResolved(vn,os);
			else
				l = getChar(vn,os);
			os += (int) (l >> 32);
			if ((int) l > 64 && (int) l < 91)
				s[k++] = (UCSChar)(l + 32); // java only support 16 bit unit code
			else
				s[k++] = (UCSChar) l; // java only support 16 bit unit code
		}
		return k;
	}

	static int _toStringUpperCase(VTDNav *vn, UCSChar *s, int index, int offset){

		int len = getTokenLength2(vn,index), k = offset;
		int os = getTokenOffset(vn,index);
		int type = getTokenType(vn,index);
		int endOffset = os + len;
		Long l;
		while (os < endOffset) {
			if (type!=TOKEN_CDATA_VAL)
				l = getCharResolved(vn,os);
			else
				l = getChar(vn,os);
			os += (int) (l >> 32);
			//s[k++] = (UCSChar)l; // java only support 16 bit unit code
			if ((int) l > 96 && (int) l < 123)
				s[k++] = (UCSChar)(l - 32); // java only support 16 bit unit code
			else
				s[k++] = (UCSChar) l;

		}

		return k;
	}



	UCSChar *toNormalizedXPathString(VTDNav *vn,int j){			
		// TODO Auto-generated method stub
		int tokenType;UCSChar* sb =NULL;int state=0;
		int index = j + 1;
		int depth, t=0;
		int dp = getTokenDepth(vn,j);
		Boolean r = FALSE;//default
		//int size = vtdBuffer.size;
		// store all text tokens underneath the current element node
		while (index < vn->vtdSize) {
		    tokenType = getTokenType(vn,index);
		    depth = getTokenDepth(vn,index);
		    
		    if (depth<dp || 
		    		(depth==dp && tokenType==TOKEN_STARTING_TAG)){
		    	break;
		    }
		    
		    if (tokenType==TOKEN_CHARACTER_DATA || tokenType==TOKEN_CDATA_VAL){
		    	//if (!match)
		    	t=t+getTokenLength2(vn,index);
		    	appendInt(vn->fib,index);
		    	index++;
		    	continue;
		    } else if (tokenType==TOKEN_ATTR_NAME
			        || tokenType ==TOKEN_ATTR_NS
			        || tokenType ==TOKEN_PI_NAME){			  
			    index = index+2;
			    continue;
			}			
			index++;
		}
		//allocate string buffer
		sb = malloc(sizeof(UCSChar)*(t+1));
		t=0;
		//now create teh string, leading zeros/trailling zero stripped, multiple ws collapse into one
		index =0;

		while(index<vn->fib->size){
			int offset= getTokenOffset(vn,intAt(vn->fib,index));
			int len = getTokenLength2(vn,intAt(vn->fib,index));
			int endOS= offset + len;
			int c;
			Long l;
			
			int type = getTokenType(vn,intAt(vn->fib,index));
			if (type==TOKEN_CHARACTER_DATA){
				while(offset < endOS){
					l = getCharResolved(vn,offset);
					c = (int)l;
					offset += (int)(l>>32);
					
					switch(state){
					case 0:
						if(isWS(c)){
							
						}else{
							sb[t++]=(char)c;
							
							state =1;
						}
						break;
						
					case 1:
						if(isWS(c)){
							sb[t++]=(char)' ';
							state =2;
						}else{
							sb[t++]=(char)c;
						}
						break;
						
					case 2:
						if (isWS(c)){
							
						}else{
							sb[t++]=((char)c);
							state = 1;
						}
						break;
					
					}
				}
			}else{
				while(offset < endOS){
					l = getChar(vn,offset);
					c = (int)l;
					offset += (int)(l>>32);
					
					switch(state){
					case 0:
						if(isWS(c)){
							
						}else{
							sb[t++]=((char)c);
							state =1;
						}
						break;
					case 1:
						if(isWS(c)){
							sb[t++]=((char)' ');
							state =2;
						}else{
							sb[t++]=((char)c);
						}
						break;
					
					case 2: 
						if (isWS(c)){
							
						}else{
							sb[t++]=((char)c);
							state = 1;
						}
						break;
					
					}
				}
			}			
			
			index++;
			//System.out.println(sb.toString());
		}

		//put end of string character
		sb[t]=0;
		clearFastIntBuffer(vn->fib);
		//String s =sb.toString();
		//System.out.println();
		return sb;
}
	Boolean XPathStringVal_Contains(VTDNav *vn,int j, UCSChar *s){
		int tokenType;
		int index = j + 1;
		int depth, t=0, i=0,offset, endOffset,len,type,c;
		Long l;
		Boolean result=FALSE;
		int dp = getTokenDepth(vn,j);
		//int size = vtdBuffer.size;
		// store all text tokens underneath the current element node
		while (index < vn->vtdSize) {
		    tokenType = getTokenType(vn,index);
		    depth = getTokenDepth(vn,index);
		    //t=t+getTokenLength2(index);
		    if (depth<dp || 
		    		(depth==dp && tokenType==TOKEN_STARTING_TAG)){
		    	break;
		    }
		    
		    if (tokenType==TOKEN_CHARACTER_DATA
		    		|| tokenType==TOKEN_CDATA_VAL){
		    	//length = getTokenLength2(index);
		    	//t += length;
		    	appendInt(vn->fib,index);
		    	index++;
		    	continue;
		    	//
		    } else if (tokenType==TOKEN_ATTR_NAME
			        || tokenType == TOKEN_ATTR_NS
			        || tokenType == TOKEN_PI_NAME){			  
			    index = index+2;
			    continue;
			}			
			index++;
		}
		
		index=0;
		while(index<vn->fib->size){
			type = getTokenType(vn,intAt(vn->fib,index));
			offset = getTokenOffset(vn,intAt(vn->fib,index));
			len = getTokenLength(vn,intAt(vn->fib,index));
			endOffset = offset+len;
			while(offset<endOffset){
				if (type==TOKEN_CHARACTER_DATA)
					l = getCharResolved(vn,offset);
				else
					l = getChar(vn,offset);
				c = (int)l;
				if (c==s[0]&& matchSubString(vn,offset,  index, s)){
					result=TRUE;
					goto loop;
				}else
					offset += (int)(l>>32);
			}			
			index++;
		}
		
		loop:
		clearFastIntBuffer(vn->fib);
		return result;
	}
	
	Boolean XPathStringVal_StartsWith(VTDNav *vn,int j, UCSChar *s){
		int tokenType;
		int index = j + 1;
		int depth,i=0, offset, endOffset, len,len2=wcslen(s),c;
		Long l;
		int dp = getTokenDepth(vn,j);
		Boolean r = FALSE;//default
		//int size = vtdBuffer.size;
		// store all text tokens underneath the current element node
		while (index < vn->vtdSize) {
		    tokenType = getTokenType(vn,index);
		    depth = getTokenDepth(vn,index);
		    //t=t+getTokenLength2(index);
		    if (depth<dp ||
		    		(depth==dp && tokenType==TOKEN_STARTING_TAG)){
		    	break;
		    }
		    
		    if (tokenType==TOKEN_CHARACTER_DATA ){
		    	//if (!match)
		    	offset = getTokenOffset(vn,index);
		    	len = getTokenLength2(vn,index);
		    	endOffset = offset + len;
		    	while(offset<endOffset){
		    		l = getCharResolved(vn,offset);
		    		c = (int)l;
		    		
		    		if (i< len2&& c == s[i]){
		    			offset += (int)(l>>32);
		    			i++;
		    		}else if (i==len2)
		    			return TRUE;
		    		else
		    			return FALSE;
		    		
		    	}
		    	index++;
		    	continue;
		    }else if( tokenType==TOKEN_CDATA_VAL){
		    	offset = getTokenOffset(vn,index);
		    	len = getTokenLength2(vn,index);
		    	endOffset = offset + len;
		    	while(offset<endOffset){
		    		l = getChar(vn,offset);
		    		c = (int)l;
		    		
		    		if (i< len2&& c == s[i]){
		    			offset += (int)(l>>32);
		    			i++;
		    		}else if (i==len2)
		    			return TRUE;
		    		else
		    			return FALSE;
		    		
		    	}
		    	index++;
		    	continue;
		    }else if (tokenType==TOKEN_ATTR_NAME
			        || tokenType == TOKEN_ATTR_NS
			        || tokenType == TOKEN_PI_NAME){			  
			    
		    	index = index+2;
			    continue;
			}			
			index++;
		}
		return FALSE;
	}
	
	Boolean XPathStringVal_EndsWith(VTDNav *vn,int j, UCSChar *s){
		int tokenType;
		int index = j + 1,len=wcslen(s);
		int depth, t=0, i=0,d=0, offset,endOffset,type;
		Boolean b=FALSE;
		Long l;
		int dp = getTokenDepth(vn,j);
		//int size = vtdBuffer.size;
		// store all text tokens underneath the current element node
		while (index < vn->vtdSize) {
		    tokenType = getTokenType(vn,index);
		    depth = getTokenDepth(vn,index);
		    //t=t+getStringLength(index);
		    if (depth<dp || 
		    		(depth==dp && tokenType==TOKEN_STARTING_TAG)){
		    	break;
		    }
		    
		    if (tokenType==TOKEN_CHARACTER_DATA
		    		|| tokenType==TOKEN_CDATA_VAL){
		    	//length = getTokenLength2(index);
		    	//t += length;
		    	appendInt(vn->fib,index);
		    	index++;
		    	continue;
		    	//
		    } else if (tokenType==TOKEN_ATTR_NAME
			        || tokenType ==TOKEN_ATTR_NS
			        || tokenType ==TOKEN_PI_NAME){			  
			    index = index+2;
			    continue;
			}			
			index++;
		}
		//if (t<s.length())
		//	return false;
		for (i=vn->fib->size-1;i>=0;i--){
			t+=getStringLength(vn,intAt(vn->fib,i));
			if (t>=len){
				d = t-len;//# of chars to be skipped
				break;
			}
		}
		
		if (i<-1)return FALSE;
		type = getTokenType(vn,intAt(vn->fib,i));
		offset = getTokenOffset(vn,intAt(vn->fib,i));
		endOffset = offset+getTokenLength2(vn,intAt(vn->fib,i));
		for(j=0;j<d;j++){
			if (type==TOKEN_CHARACTER_DATA){
				l=getCharResolved(vn,offset);
			}else
				l=getChar(vn,offset);
			offset += (int)(l>>32);
		}
		b =matchSubString(vn,offset, i,s);
		clearFastIntBuffer(vn->fib);
		return b;
	}

	Boolean matchSubString(VTDNav* vn, int os, int index, UCSChar *s){
		int offset = os, endOffset=getTokenOffset(vn,intAt( vn->fib,index))+getTokenLength(vn,intAt(vn->fib,index)), 
			type =getTokenType(vn,intAt(vn->fib,index)), c;Long l;
		int i=0;int len=wcslen(s);
		Boolean b=FALSE;
		while(offset<endOffset){
			if (type==TOKEN_CHARACTER_DATA)
				l = getCharResolved(vn,offset);
			else
				l = getChar(vn,offset);
			c = (int)l;
			if (i<len-1 && c==s[i]){		
				offset += (int)(l>>32);
				i++;
			}else if(i==len-1)
				return TRUE;
			else
				return FALSE;				
		}
		index++;
		while(index<vn->fib->size){		
			offset = getTokenOffset(vn,intAt(vn->fib,index));
			endOffset = offset + getTokenLength2(vn,intAt(vn->fib,index));
			type = getTokenType(vn,intAt(vn->fib,index));
			while(offset<endOffset){
				if (type==TOKEN_CHARACTER_DATA)
					l = getCharResolved(vn,offset);
				else
					l = getChar(vn,offset);
				c = (int)l;
				if (i<len && c==s[i]){		
					offset += (int)(l>>32);
					i++;
				}else if(i==len)
					return TRUE;
				else
					return FALSE;				
			}
			index++;
		}while(index<vn->fib->size);
		if (i==len)
			return TRUE;
		return FALSE;
	}


	Boolean matchSubString2(VTDNav* vn, int os,  int index, UCSChar *s){
		int offset = os, endOffset= getTokenOffset(vn,intAt(vn->fib,index))+getTokenLength(vn,intAt(vn->fib,index)), 
			type =getTokenType(vn,intAt(vn->fib,index)), c;Long l;
		size_t i=0;
		Boolean b=FALSE;
		while(offset<endOffset){
			if (type==TOKEN_CHARACTER_DATA)
				l = getCharResolved(vn,offset);
			else
				l = getChar(vn,offset);
			c = (int)l;
			if (i<wcslen(s) && c==s[i]){		
				offset += (int)(l>>32);
				i++;
			}else if(i==wcslen(s))
				return TRUE;
			else
				return FALSE;				
		}
		index++;
		while(index<vn->fib->size){		
			offset = getTokenOffset(vn, intAt(vn->fib,index));
			endOffset = offset + getTokenLength2(vn,intAt(vn->fib,index));
			type = getTokenType(vn, intAt(vn->fib,index));
			while(offset<endOffset){
				if (type==TOKEN_CHARACTER_DATA)
					l = getCharResolved(vn,offset);
				else
					l = getChar(vn,offset);
				c = (int)l;//System.out.println("c-===>"+(char)c);
				if (i<wcslen(s) && c==s[i]){		
					offset += (int)(l>>32);
					i++;
				}else if(i==wcslen(s)){
					goto loop;
				}
				else
					return FALSE;				
			}
			index++;
		}while(index<vn->fib->size);
		loop:if ( wcslen(s) ==i  &&index == vn->fib->size && endOffset == offset)
			return TRUE;
		return FALSE;
	}
	void dumpFragment(VTDNav *vn,Long l, char *fileName){
	//long l = getElementFragment();
		int os = (int)l;
		int len = (int)(l>>32);
		FILE *f=fopen(fileName,"w");
		if (f!=NULL){
			size_t i = fwrite(vn->XMLDoc+vn->docOffset+os,1,len,f);
			if (i < (size_t) vn->docLen)
				throwException2(io_exception, "can't complete the write");	
			fclose(f);
		} else {
			throwException2(io_exception,"can't open file");
		}	
		
	}

	void dumpFragment2(VTDNav *vn,char *fileName){
		Long l = getElementFragment(vn);
		dumpFragment(vn,l,fileName);
	}
	
	void dumpElementFragmentNs(VTDNav *vn,char *fileName){
		ElementFragmentNs *efn= getElementFragmentNs(vn);
		FILE *f=fopen(fileName,"w");
		if (f!=NULL){
			writeFragmentToFile(efn,f);
			fclose(f);
			freeElementFragmentNs(efn);
		}else{
			freeElementFragmentNs(efn);
			throwException2(io_exception,"can't open file");
		}
	}
    
	Long expandWhiteSpace(VTDNav *vn,Long l){
		int offset = (int)l, len=(int) (l>>32);int endOffset;
		//long l=0;
		if (vn->encoding >= FORMAT_UTF_16BE) {
			offset >>= 1;
			len >>= 1;
		}
		// first expand the trailing white spaces
		endOffset = offset+len;
		while(isWS(getCharUnit(vn,endOffset))){
			endOffset++;
		}
		
		// then the leading whtie spaces
		offset--;
		while(isWS(getCharUnit(vn,offset))){
			offset--;
		}
		offset++;
		len = endOffset - offset;

		if (vn->encoding >= FORMAT_UTF_16BE){
			len <<=1;
			offset <<= 1;
		}
		
		return ((Long)offset) | (((Long)len)<<32);
	}
	
	Long trimWhiteSpace(VTDNav *vn,Long l){
		int offset = (int)l, len=(int) (l>>32);int endOffset;
		//long l=0;
		if (vn->encoding >= FORMAT_UTF_16BE) {
			offset >>= 1;
			len >>= 1;
		}
		endOffset= offset+len;
		// first trim the leading white spaces
		
		while(isWS(getCharUnit(vn,offset))){
			offset++;
		}
		
		// then trim the trailing white spaces
		//int endOffset = offset+len-1;
		endOffset--;
		while(isWS(getCharUnit(vn,endOffset))){
			endOffset--;
		}
		
		endOffset ++;
		
		len = endOffset - offset;

		if (vn->encoding >= FORMAT_UTF_16BE){
			len <<=1;
			offset <<= 1;
		}
		
		return ((Long)offset) | (((Long)len)<<32);
		//return -1;
	}


	double XPathStringVal2Double(VTDNav *vn,int j){
		tokenType tokenType; double d1 = 0.0;
		double d= d1/d1; 
		Boolean minus=FALSE; 
		Boolean exponent_seen=FALSE; 
		Boolean minusE=FALSE;
		int index = j + 1;
		int depth,i=0, offset, endOffset, len,c;
		Long l;
		int state =0;
		double left=0,right=0,v;
		int exp=0;
		double scale=1;
		
		int dp = getTokenDepth(vn,j);
		//boolean r = false;//default
		
		//int size = vtdBuffer.size;
		// store all text tokens underneath the current element node
		while (index < vn->vtdSize) {
		    tokenType = getTokenType(vn,index);
		    depth = getTokenDepth(vn,index);
		    //t=t+getTokenLength2(index);
		    if (depth<dp ||
		    		(depth==dp && tokenType==TOKEN_STARTING_TAG)){
		    	break;
		    }
		    
		    if (tokenType==TOKEN_CHARACTER_DATA || tokenType== TOKEN_CDATA_VAL ){
		    	//if (!match)
		    	offset = getTokenOffset(vn,index);
		    	len = getTokenLength2(vn,index);
		    	endOffset = offset + len;
		    	while(offset<endOffset){
		    		if (tokenType==TOKEN_CHARACTER_DATA )
		    			l = getCharResolved(vn,offset);
		    		else
		    			l = getChar(vn,offset);
		    		c = (int)l;
		    		offset += (int)(l>>32);
		    		switch (state){
		    		// consume white spaces
		    		case 0: 
						if (isWS(c)) {
							break;
						} else if (c == '-' || c == '+') {
							if (c == '-')
								minus = TRUE;
							state = 1;
						} else if (isDigit(c)) {
							left = left * 10 + (c - '0');
							state = 1;
						} else
							return d1/d1;

						break;
		    				// test digits or .
		    		case 1: 
		    			if (isDigit(c)){
		    				left = left*10+ (c-'0');
		    				state =1;
		    			} else if (c=='.'){
		    				state = 2;
		    			} else if (c=='e'|| c=='E'){
		    				exponent_seen =  TRUE;
		    				state =4;
		    			}else 
		    				return d1/d1;
		    			break;
		    			// test digits before .
		    		case 2: 
		    			if(isDigit(c)){
		    				right = right*10+(c-'0');
		    				scale = scale*10;
		    				state =3;
		    			}else
		    				return d1/d1;
		    			break;
		    			// test digits after .
		    		case 3:
		    			if(isDigit(c)){
		    				right = right*10+(c-'0');
		    				scale = scale*10;
		    			}else if (c=='e' ||c== 'E'){
		    				exponent_seen=TRUE;
		    				state=4;
		    			}else if (isWS(c)){
		    				state = 6;
		    			}
		    			else
		    				return d1/d1;
		    			break;
		    			
		    			// test exponent digits
		    			
		    		case 4:
		    			if (c=='-' || c=='+'){
		    				if (c=='-'){
		    					minusE= TRUE;
		    				}
		    				state =5;
		    			}else if (isDigit(c)){
		    				exp = exp*10+(c-'0');
		    				state = 5;
		    			}else
		    				return d1/d1;
		    			break;
		    			// test -+ after exponen
		    		case 5:
		    			if (isDigit(c)){
		    				exp = exp*10+(c-'0');
		    			} else if (isWS(c)){
		    				state =6;
		    			}else 
		    				return d1/d1;
		    		    break;
		    		    // test digits after e
		    		case 6:
		    			if (!isWS(c))
		    				return d1/d1;
		    			break;
		    		}
		    	}
		    	//index++;
		    	//continue;
		    }else if (tokenType==TOKEN_ATTR_NAME
			        || tokenType == TOKEN_ATTR_NS
			        || tokenType == TOKEN_PI_NAME){			  
			    
		    	index = index+2;
			    continue;
			}		    
			index++;
		}
		//return false;
		v = (double) left;
		if (right != 0)
			v += ((double) right) / (double) scale;

		if (exp != 0)
			v = (minusE)? v /pow(10,exp): v*pow(10, exp);

		return ((minus) ? (-v) : v);
	}


	Boolean isDigit(int c){
		if (c>='0' && c<='9')
			return TRUE;
		else 
			return FALSE;
	}

	int getNextChar(VTDNav *vn, vn_helper *h){
		Long l;
		int result;		
		if (h->type==0){// single token
			if (h->offset == h->endOffset)
				return -1;
			if (h->tokenType == TOKEN_CHARACTER_DATA &&
					h->tokenType !=TOKEN_ATTR_VAL){ 
				l = getCharResolved(vn,h->offset);
			}else {
				l = getChar(vn,h->offset);
			}
			h->offset += (int)(l>>32);
			result = (int)l;
			return result;
			
		}else {// text value
			if (h->offset < h->endOffset){
				//return result;
				if (h->tokenType != TOKEN_PI_VAL &&
					h->tokenType !=TOKEN_CHARACTER_DATA){ 
					l = getChar(vn,h->offset);
				}else {
					l = getChar(vn,h->offset);
				}
				h->offset += (int)(l>>32);
				result = (int)l;	
				return result;
			}else{
				h->index++;
				while (h->index < vn->vtdSize) {
				    tokenType tokenType = getTokenType(vn,h->index);
				    int depth = getTokenDepth(vn,h->index);
				    //t=t+getTokenLength2(index);
				    if (depth<h->depth || 
				    		(depth==h->depth && tokenType==TOKEN_STARTING_TAG)){
				    	break;
				    }
				    
				    if (tokenType=TOKEN_CHARACTER_DATA
				    		|| tokenType==TOKEN_CDATA_VAL){
				    	//length = getTokenLength2(index);
				    	//t += length;
				    	//fib.append(index);
				    	h->offset = getTokenOffset(vn,h->index);
				    	h->endOffset = getTokenOffset(vn,h->index)+getTokenLength2(vn,h->index);
				    	h->tokenType = tokenType;
				    	//h2.index++;
				    	return getNextChar(vn,h);
				    	//
				    } else if (tokenType==TOKEN_ATTR_NAME
					        || tokenType == TOKEN_ATTR_NS
					        || tokenType == TOKEN_PI_NAME){			  
					    h->index = h->index+2;
					    continue;
					}			
					h->index++;
				}
				return -1;
			}
		}
		//return -1;
	}

	Boolean XPathStringVal_Matches(VTDNav *vn,int j, UCSChar *s){
		tokenType tokenType;
		int index = j + 1;
		int depth, t=0, i=0,offset;
		//Long l;
		Boolean result=FALSE;
		int dp = getTokenDepth(vn,j);
		//int size = vtdBuffer.size;
		// store all text tokens underneath the current element node
		while (index < vn->vtdSize) {
		    tokenType = getTokenType(vn,index);
		    depth = getTokenDepth(vn,index);
		    //t=t+getTokenLength2(index);
		    if (depth<dp || 
		    		(depth==dp && tokenType==TOKEN_STARTING_TAG)){
		    	break;
		    }
		    
		    if (tokenType==TOKEN_CHARACTER_DATA
		    		|| tokenType==TOKEN_CDATA_VAL){
		    	//length = getTokenLength2(index);
		    	//t += length;
		    	appendInt(vn->fib,index);
		    	index++;
		    	continue;
		    	//
		    } else if (tokenType==TOKEN_ATTR_NAME
			        || tokenType ==TOKEN_ATTR_NS
			        || tokenType ==TOKEN_PI_NAME){			  
			    index = index+2;
			    continue;
			}			
			index++;
		}
		
		index=0;
		//type = getTokenType(fib.intAt(index));
		offset = getTokenOffset(vn,intAt(vn->fib,0));
		result = matchSubString2(vn,offset, index, s);		
		clearFastIntBuffer(vn->fib);
		return result;
	}

	int XPathStringVal_Matches2(VTDNav *vn,int j, VTDNav *vn2, int k /*k is a token index */){
		tokenType tokenType1, tokenType2;
		int c1,c2;
		if (vn->h1==NULL){
			vn->h1 = (vn_helper *)malloc( sizeof(vn_helper));
		}
		
		if (vn->h2==NULL){
			vn->h2 = (vn_helper *)malloc(sizeof(vn_helper));
		}
		
		tokenType1 = getTokenType(vn,j);
		tokenType2 = getTokenType(vn2,k);
		
		if (tokenType1 == TOKEN_STARTING_TAG || tokenType1 == TOKEN_DOCUMENT ){
			vn->h1->index = j + 1;
			vn->h1->type = 1;
			vn->h1->depth = getTokenDepth(vn,j);
			vn->h1->offset  = -1;
			while (vn->h1->index < vn->vtdSize) {
			    int tokenType = getTokenType(vn,vn->h1->index);
			    int depth = getTokenDepth(vn,vn->h1->index);
			    //t=t+getTokenLength2(index);
			    if (depth<vn->h1->depth || 
			    		(depth==vn->h1->depth && tokenType==TOKEN_STARTING_TAG)){
			    	break;
			    }
			    
			    if (tokenType==TOKEN_CHARACTER_DATA
			    		|| tokenType==TOKEN_CDATA_VAL){
			    	//length = getTokenLength2(index);
			    	//t += length;
			    	//fib.append(index);
			    	vn->h1->offset = getTokenOffset(vn,vn->h1->index);
			    	vn->h1->endOffset = getTokenOffset(vn,vn->h1->index)+getTokenLength2(vn,vn->h1->index);
			    	//h1.index++;
			    	vn->h1->tokenType=tokenType;
			    	goto loop1;
			    	//
			    } else if (tokenType==TOKEN_ATTR_NAME
				        || tokenType == TOKEN_ATTR_NS
				        || tokenType == TOKEN_PI_NAME){			  
				    vn->h1->index = vn->h1->index+2;
				    continue;
				}			
				vn->h1->index++;
			}
			loop1:;
		}
		else{ 
			vn->h1->index = -1;
			vn->h1->type = 0;
			vn->h1->offset = getTokenOffset(vn,j);
			vn->h1->endOffset = getTokenOffset(vn,j)+getTokenLength(vn,j);
			vn->h1->tokenType = getTokenType(vn,j);
		}
		
		if (tokenType2 ==TOKEN_STARTING_TAG || tokenType2 ==TOKEN_DOCUMENT ){
			vn->h2->index = k + 1;
			vn->h2->type = 1;
			vn->h2->depth = getTokenDepth(vn2,k);
			vn->h2->offset = -1;
			while (vn->h2->index < vn->vtdSize) {
			    tokenType tokenType = getTokenType(vn2,vn->h2->index);
			    int depth = getTokenDepth(vn2,vn->h2->index);
			    //t=t+getTokenLength2(index);
			    if (depth<vn->h2->depth || 
			    		(depth==vn->h2->depth && tokenType==TOKEN_STARTING_TAG)){
			    	break;
			    }
			    
			    if (tokenType==TOKEN_CHARACTER_DATA
			    		|| tokenType==TOKEN_CDATA_VAL){
			    	//length = getTokenLength2(index);
			    	//t += length;
			    	//fib.append(index);
			    	vn->h2->offset = getTokenOffset(vn2,vn->h2->index);
			    	vn->h2->endOffset = getTokenOffset(vn2,vn->h2->index)+getTokenLength2(vn2,vn->h2->index);
			    	vn->h2->tokenType = tokenType;
			    	//h2.index++;
			    	goto loop2;
			    	//
			    } else if (tokenType==TOKEN_ATTR_NAME
				        || tokenType ==TOKEN_ATTR_NS
				        || tokenType ==TOKEN_PI_NAME){			  
				    vn->h2->index = vn->h2->index+2;
				    continue;
				}			
				vn->h2->index++;
			}
			loop2: ;
		}
		else{ 
			vn->h2->index = -1;
			vn->h2->type= 0;
			vn->h2->offset =getTokenOffset( vn2,k);
			vn->h2->endOffset= getTokenOffset(vn2,k)+getTokenLength(vn2,k);
			vn->h2->tokenType =getTokenType( vn2,k);
		}
		
		// set the offset
		 c1=-1, c2=-1;
		do{
			c1=getNextChar(vn, vn->h1); 
			c2=getNextChar(vn2,vn->h2);		
			if (c1!=c2){
				if (c1>c2)
					return 1;
				else 
					return -1;
				//return false;
			}
		} while(c1!=-1 && c2!=-1);
		
		if (c1==c2){
			return 0;
		}
		else {
			if (c1!=-1)
				return 1;
			else 
				return -1;
		}
			//return false;
	}