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

#ifndef VTDNAV_H
#define VTDNAV_H
#include "customTypes.h"
#include "fastIntBuffer.h"
#include "fastLongBuffer.h"
#include "contextBuffer.h"
#include "UTF8Char.h"
#include "XMLChar.h"
#include "decoder.h"
#include <math.h>
//#include "bookMark.h"
//#include "elementFragmentNs.h"


#define MASK_TOKEN_FULL_LEN 0x000fffff00000000LL
#define MASK_TOKEN_PRE_LEN 0x000ff80000000000LL
#define MASK_TOKEN_QN_LEN  0x000007ff00000000LL
#define MASK_TOKEN_OFFSET1  0x000000003fffffffLL
#define MASK_TOKEN_OFFSET2  0x000000007fffffffLL

#define MASK_TOKEN_TYPE  0xf000000000000000LL
#define MASK_TOKEN_DEPTH  0x0ff0000000000000LL
#define MASK_TOKEN_NS_MARK 0x00000000c0000000LL

#define XPATH_STRING_MODE_NORMAL 0
#define XPATH_STRING_MODE_UPPERCASE 1
#define XPATH_STRING_MODE_LOWERCASE 2

//#define ROOT 0
//#define PARENT 1
//#define FIRST_CHILD 2
//#define LAST_CHILD 3
//#define NEXT_SIBLING 4
//#define PREV_SIBLING 5

#define R 0
#define P 1
#define FC 2
#define LC 3
#define NS 4
#define PS 5

struct vTDNav;
struct bookMark;

typedef void (*freeVTDNav_) (struct vTDNav *vn);
typedef void (*recoverNode_)(struct vTDNav *vn, int index);
typedef Boolean (*toElement_)(struct vTDNav *vn, navDir d);
typedef Boolean (*toElement2_)(struct vTDNav *vn, navDir d, UCSChar *s);
typedef Boolean (*toElementNS_)(struct vTDNav *vn, navDir d, UCSChar *URL, UCSChar *ln);
/*typedef int (*iterate_)(struct vTDNav *vn, int dp, UCSChar *en, Boolean special);
typedef int (*iterateNS_)(struct vTDNav *vn, int dp, UCSChar *URL, UCSChar *ln);
typedef Boolean (*iterate_preceding_)(struct vTDNav *vn,UCSChar *en, int* a, Boolean special);
typedef Boolean (*iterate_precedingNS_)(struct vTDNav *vn,UCSChar *URL, UCSChar *ln, int* a);
typedef Boolean (*iterate_following_)(struct vTDNav *vn,UCSChar *en, Boolean special);
typedef Boolean (*iterate_followingNS_)(struct vTDNav *vn, UCSChar *URL, UCSChar *ln);*/
typedef struct vTDNav* (*duplicateNav_)(struct vTDNav *vn);
typedef struct vTDNav* (*cloneNav_)(struct vTDNav *vn);
typedef void (*resolveLC_)(struct vTDNav *vn);
typedef Boolean (*pop_)(struct vTDNav *vn);
typedef Boolean (*pop2_)(struct vTDNav *vn);
typedef Boolean (*push_)(struct vTDNav *vn);
typedef Boolean (*push2_)(struct vTDNav *vn);
typedef void (*sampleState_)(struct vTDNav *vn, FastIntBuffer *fib);
typedef Boolean (*writeIndex_VTDNav_)(struct vTDNav *vn, FILE *f);
typedef Boolean (*writeIndex2_VTDNav_)(struct vTDNav *vn, char *fileName);
typedef Boolean (*writeSeparateIndex_VTDNav_)(struct vTDNav *vn, char *vtdIndex);
typedef Boolean (*toNode_)(struct vTDNav *vn, navDir d);
typedef void (*sync_)(struct vTDNav *vn,int depth, int index);
typedef void (*dumpState_)(struct vTDNav *vn);
typedef Boolean (*verifyNodeCorrectness_)(struct vTDNav *vn);

typedef struct Vn_helper {
	int index;
	int offset;
	int endOffset;
	int type;
	int depth;
	int tokenType;
} vn_helper;

typedef struct vTDNav{
	freeVTDNav_ __freeVTDNav;
	recoverNode_ __recoverNode;
	toElement_ __toElement;
	toElement2_ __toElement2;
	toElementNS_ __toElementNS;
	toNode_   __toNode;
	sync_  __sync;
	verifyNodeCorrectness_ __verifyNodeCorrectness;
	dumpState_  __dumpState;
	/*iterate_  __iterate;
	iterateNS_  __iterateNS;
	iterate_preceding_ __iterate_preceding;
	iterate_precedingNS_ __iterate_precedingNS;
	iterate_following_ __iterate_following;
	iterate_followingNS_ __iterate_followingNS;*/
	duplicateNav_ __duplicateNav;
	cloneNav_ __cloneNav;
	resolveLC_  __resolveLC;
	pop_ __pop;
	pop2_ __pop2;
	push_ __push;
	push2_ __push2;
	sampleState_ __sampleState;
	writeIndex_VTDNav_ __writeIndex_VTDNav;
	writeIndex2_VTDNav_ __writeIndex2_VTDNav;
	writeSeparateIndex_VTDNav_ __writeSeparateIndex_VTDNav;

	int rootIndex;
	int nestingLevel;
	int* context; // context object
	Boolean atTerminal; // Add this model to be compatible with XPath data model, 
						// true if attribute axis or text()
// location cache part
	int l2upper;
	int l2lower;
	int l3upper;
	int l3lower;
	int l2index;
	int l3index;
	int l1index;
	Boolean shallowDepth;
    
	FastLongBuffer *vtdBuffer;
	FastLongBuffer *l1Buffer;
	FastLongBuffer *l2Buffer;
	FastIntBuffer *l3Buffer;
	UByte* XMLDoc;
	FastIntBuffer *fib;
	FastIntBuffer *fib2;
	UCSChar *name;
	int nameIndex;
	UCSChar *localName;
	int localNameIndex;
	struct bookMark *currentNode;
	UCSChar  *URIName;
	int count;
    
	Long offsetMask;
	ContextBuffer *contextBuf;
	ContextBuffer *contextBuf2;// this is reserved for XPath
	
	int LN;// record txt and attrbute for XPath eval purposes

	encoding_t encoding;

	//int currentOffset;
	//int currentOffset2;

	Boolean ns;
	int* stackTemp;
	int docOffset;	 // starting offset of the XML doc wrt XMLDoc
	int docLen;  // size of XML document
	int vtdSize; // # of entries in vtdBuffer equvalent 
	             // to calling size(FastLongBuffer *flb) defined in fastLongBuffer.h
	int bufLen; // size of XMLDoc in bytes
	Boolean br; // buffer reuse flag
	Boolean master; // true if vn is obtained by calling getNav(), otherwise false
	                // useful for implementing dupliateNav() and cloneNav();
	short maxLCDepthPlusOne;
	vn_helper *h1;
	vn_helper *h2;

} VTDNav;







typedef struct vTDNav_L5{
	freeVTDNav_ __freeVTDNav;
	recoverNode_ __recoverNode;
	toElement_ __toElement;
	toElement2_ __toElement2;
	toElementNS_ __toElementNS;
	toNode_   __toNode;
	sync_  __sync;
	verifyNodeCorrectness_ __verifyNodeCorrectness;
	dumpState_  __dumpState;
	/*iterate_  __iterate;
	iterateNS_  __iterateNS;
	iterate_preceding_ __iterate_preceding;
	iterate_precedingNS_ __iterate_precedingNS;
	iterate_following_ __iterate_following;
	iterate_followingNS_ __iterate_followingNS;*/
	duplicateNav_ __duplicateNav;
	cloneNav_ __cloneNav;
	resolveLC_  __resolveLC;
	pop_ __pop;
	pop2_ __pop2;
	push_ __push;
	push2_ __push2;
	sampleState_ __sampleState;
	writeIndex_VTDNav_ __writeIndex_VTDNav;
	writeIndex2_VTDNav_ __writeIndex2_VTDNav;
	writeSeparateIndex_VTDNav_ __writeSeparateIndex_VTDNav;

	int rootIndex;
	int nestingLevel;
	int* context; // context object
	Boolean atTerminal; // Add this model to be compatible with XPath data model, 
						// true if attribute axis or text()
// location cache part
	int l2upper;
	int l2lower;
	int l3upper;
	int l3lower;
	int l2index;
	int l3index;
	int l1index;
	Boolean shallowDepth;

    
	FastLongBuffer *vtdBuffer;
	FastLongBuffer *l1Buffer;
	FastLongBuffer *l2Buffer;
	FastIntBuffer *l3Buffer;
	UByte* XMLDoc;
		FastIntBuffer *fib;
		FastIntBuffer *fib2;
		UCSChar *name;
		int nameIndex;
		UCSChar *localName;
		int localNameIndex;
		struct bookMark *currentNode;
		UCSChar  *URIName;
		int count;
    
	Long offsetMask;
	ContextBuffer *contextBuf;
	ContextBuffer *contextBuf2;// this is reserved for XPath
	
	int LN;// record txt and attrbute for XPath eval purposes

	encoding_t encoding;

	//int currentOffset;
	//int currentOffset2;

	Boolean ns;
	int* stackTemp;
	int docOffset;	 // starting offset of the XML doc wrt XMLDoc
	int docLen;  // size of XML document
	int vtdSize; // # of entries in vtdBuffer equvalent 
	             // to calling size(FastLongBuffer *flb) defined in fastLongBuffer.h
	int bufLen; // size of XMLDoc in bytes
	Boolean br; // buffer reuse flag
	Boolean master; // true if vn is obtained by calling getNav(), otherwise false
	                // useful for implementing dupliateNav() and cloneNav();
	short maxLCDepthPlusOne;
	int l4index;
	int l5index;
	int l4upper;
	int l4lower;
	int l5upper;
	int l5lower;
	FastLongBuffer *_l3Buffer;
	FastLongBuffer *l4Buffer;
	FastIntBuffer *l5Buffer;
	vn_helper *h1;
	vn_helper *h2;

} VTDNav_L5;






//functions
//Create VTDNav object

VTDNav *createVTDNav(int r, encoding_t enc, Boolean ns, int depth,
					 UByte *x, int xLen, FastLongBuffer *vtd, FastLongBuffer *l1,
					 FastLongBuffer *l2, FastIntBuffer *l3, int so, int len,Boolean br);

//Free VTDNav object

void freeVTDNav(VTDNav *vn);

//Return the attribute count of the element at the cursor position.
int getAttrCount(VTDNav *vn);

//Get the token index of the attribute value given an attribute name.     
int getAttrVal(VTDNav *vn, UCSChar *attrName);

//Get the token index of the attribute value of given URL and local name.
//If ns is not enabled, the lookup will return -1, indicating a no-found.
//Also namespace nodes are invisible using this method.
int getAttrValNS(VTDNav *vn, UCSChar* URL, UCSChar *localName);


//Get the depth (>=0) of the current element.
extern inline int getCurrentDepth(VTDNav *vn){
	return vn->context[0];
}
// Get the index value of the current element.
//extern inline int getCurrentIndex(VTDNav *vn);
extern inline int getCurrentIndex(VTDNav *vn){
	if (vn->atTerminal)
		return vn->LN;
	switch(vn->context[0]){
		case -1: return 0;
		case 0: return vn->rootIndex;
		default: return vn->context[vn->context[0]];
	}
	//return (vn->context[0] == 0) ? vn->rootIndex : vn->context[vn->context[0]];
}

//extern inline int getCurrentIndex2(VTDNav *vn);
extern inline int getCurrentIndex2(VTDNav *vn){
	switch(vn->context[0]){
		case -1: return 0;
		case 0: return vn->rootIndex;
		default: return vn->context[vn->context[0]];
	}
	//return (vn->context[0] == 0) ? vn->rootIndex : vn->context[vn->context[0]];
}
// Get the starting offset and length of an element
// encoded in a long, upper 32 bit is length; lower 32 bit is offset
Long getElementFragment(VTDNav *vn);
Long getContentFragment(VTDNav *vn);
// Get the element fragment object corresponding to a ns 
// compensated element
struct elementFragmentNs *getElementFragmentNs(VTDNav *vn);

/**
 * Get the encoding of the XML document.
 * <pre>   0  ASCII       </pre>
 * <pre>   1  ISO-8859-1  </pre>
 * <pre>   2  UTF-8       </pre>
 * <pre>   3  UTF-16BE    </pre>
 * <pre>   4  UTF-16LE    </pre>
 */
extern inline encoding_t getEncoding(VTDNav *vn){
	return vn->encoding;
}
// Get the maximum nesting depth of the XML document (>0).
// max depth is nestingLevel -1

// max depth is nestingLevel -1
//extern inline int getNestingLevel(VTDNav *vn);
extern inline int getNestingLevel(VTDNav *vn){
	return vn->nestingLevel;
}
// Get root index value.
//extern inline int getRootIndex(VTDNav *vn);
extern inline int getRootIndex(VTDNav *vn){
	return vn->rootIndex;
}
// This function returns of the token index of the type character data or CDATA.
// Notice that it is intended to support data orient XML (not mixed-content XML).
int getText(VTDNav *vn);

//Get total number of VTD tokens for the current XML document.
//extern inline int getTokenCount(VTDNav *vn);
extern inline int getTokenCount(VTDNav *vn){
	return vn->vtdSize;
}

//Get the depth value of a token (>=0)
//int getTokenDepth(VTDNav *vn, int index);
extern inline int getTokenDepth(VTDNav *vn, int index){
	int i;
	i = (int) ((longAt(vn->vtdBuffer,index) & MASK_TOKEN_DEPTH) >> 52);

	if (i != 255)
		return i;
	return -1;
}
//Get the token length at the given index value
//please refer to VTD spec for more details
int getTokenLength(VTDNav *vn, int index);

//Get the starting offset of the token at the given index.
//extern inline int getTokenOffset(VTDNav *vn, int index);
extern inline int getTokenOffset(VTDNav *vn, int index){
	return (int) (longAt(vn->vtdBuffer,index) & vn->offsetMask);
}
// Get the XML document 
//extern inline UByte* getXML(VTDNav *vn);
extern inline UByte* getXML(VTDNav *vn){
	return vn->XMLDoc;
}
//Get the token type of the token at the given index value.
//extern inline	tokenType getTokenType(VTDNav *vn, int index);
extern inline tokenType getTokenType(VTDNav *vn, int index){
	return (tokenType) ((longAt(vn->vtdBuffer,index) & MASK_TOKEN_TYPE) >> 60) & 0xf;
}
//Test whether current element has an attribute with the matching name.
extern inline Boolean hasAttr(VTDNav *vn, UCSChar *attrName);

//Test whether the current element has an attribute with 
//matching namespace URL and localname.
extern inline Boolean hasAttrNS(VTDNav *vn, UCSChar *URL, UCSChar *localName);

//This method is similar to getElementByName in DOM except it doesn't
//return the nodeset, instead it iterates over those nodes.
extern int iterate(VTDNav *vn, int dp, UCSChar *en, Boolean special);

/*inline int iterate(VTDNav *vn, int dp, UCSChar *en, Boolean special){
	return vn->__iterate(vn,dp,en,special);
}*/

//This method is similar to getElementByName in DOM except it doesn't
//return the nodeset, instead it iterates over those nodes .
//When URL is "*" it will match any namespace
//if ns is false, return false immediately
extern int iterateNS(VTDNav *vn, int dp, UCSChar *URL, UCSChar *ln);

/*inline int iterateNS(VTDNav *vn, int dp, UCSChar *URL, UCSChar *ln){
	return vn->__iterateNS(vn,dp,URL,ln);
}*/

// This function is called by selectElement_P in autoPilot
extern Boolean iterate_preceding(VTDNav *vn,UCSChar *en, int* a, int endIndex);

/*inline Boolean iterate_preceding(VTDNav *vn,UCSChar *en, int* a, Boolean special){
	return vn->__iterate_preceding(vn,en,a,special);
}*/

// This function is called by selectElementNS_P in autoPilot
extern Boolean iterate_precedingNS(VTDNav *vn,UCSChar *URL, UCSChar *ln, int* a, int endIndex);

/*inline Boolean iterate_precedingNS(VTDNav *vn,UCSChar *URL, UCSChar *ln, int* a){
	return vn->__iterate_precedingNS(vn,URL,ln,a);
}*/

// This function is called by selectElement_F in autoPilot
extern Boolean iterate_following(VTDNav *vn,UCSChar *en, Boolean special);
/*inline Boolean iterate_following(VTDNav *vn,UCSChar *en, Boolean special){
	return vn->__iterate_following(vn,en,special);
}*/

// This function is called by selectElementNS_F in autoPilot
extern Boolean iterate_followingNS(VTDNav *vn, UCSChar *URL, UCSChar *ln);

/*inline Boolean iterate_followingNS(VTDNav *vn, UCSChar *URL, UCSChar *ln){
	return vn->__iterate_followingNS(vn,URL,ln);
}*/



//Test if the current element matches the given name.
extern Boolean matchElement(VTDNav *vn, UCSChar *en);

//Test whether the current element matches the given namespace URL and localname.
//URL, when set to "*", matches any namespace (including null), when set to null, defines a "always-no-match"
//ln is the localname that, when set to *, matches any localname
extern Boolean matchElementNS(VTDNav *vn, UCSChar *URL, UCSChar *ln);

//Match the string against the token at the given index value. When a token
//is an attribute name or starting tag, qualified name is what gets matched against
extern Boolean matchRawTokenString(VTDNav *vn, int index, UCSChar *s);
//This method matches two VTD tokens of 2 VTDNavs
extern Boolean matchTokens(VTDNav *vn, int i1, VTDNav *vn2, int i2);

//Match the string against the token at the given index value. When a token
//is an attribute name or starting tag, qualified name is what gets matched against
extern Boolean matchTokenString(VTDNav *vn, int index, UCSChar *s);

//Convert a vtd token into a double.
extern double parseDouble(VTDNav *vn, int index);

//Convert a vtd token into a float.
extern float parseFloat(VTDNav *vn, int index);

//Convert a vtd token into an int
//extern int parseInt(VTDNav *vn, int index);
extern int parseInt(VTDNav *vn, int index);/*{
	return parseInt2(vn,index,10);
}*/
//Convert a vtd token into a long
//extern Long parseLong(VTDNav *vn, int index);
extern Long parseLong(VTDNav *vn, int index);/*{
	return parseLong2(vn, index, 10);
}*/

//Load the context info from ContextBuffer.
//Info saved including LC and current state of the context 
extern inline Boolean pop(VTDNav *vn){return vn->__pop(vn);}

extern inline Boolean pop2(VTDNav *vn){return vn->__pop2(vn);}
//Store the context info into the ContextBuffer.
//Info saved including LC and current state of the context 

extern inline Boolean push(VTDNav *vn){return vn->__push(vn);}

extern inline Boolean push2(VTDNav *vn){return vn->__push2(vn);}


//extern void sampleState(VTDNav *vn, FastIntBuffer *fib);
extern inline Boolean writeSeparateIndex_VTDNav(VTDNav *vn, char *vtdIndex){
	return vn->__writeSeparateIndex_VTDNav(vn,vtdIndex);
}

/* Write VTD+XML into a file of given name */
extern inline Boolean writeIndex2_VTDNav(VTDNav *vn, char *fileName){
	return vn->__writeIndex2_VTDNav(vn,fileName);
}

/* Write VTD+XML into a FILE pointer */
extern inline Boolean writeIndex_VTDNav(VTDNav *vn, FILE *f){
	return vn->__writeIndex_VTDNav(vn,f);
}


extern inline VTDNav* duplicateNav(VTDNav *vn){return vn->__duplicateNav(vn);}

extern inline VTDNav* cloneNav(VTDNav *vn){return vn->__cloneNav(vn);}

extern inline void recoverNode(VTDNav *vn, int index){vn->__recoverNode(vn,index);}

extern inline void resolveLC(VTDNav *vn){
	vn->__resolveLC(vn);
}

extern inline void sampleState(VTDNav *vn, FastIntBuffer *fib){
	vn->__sampleState(vn,fib);
}

extern inline void freeVTDNav(VTDNav *vn){
	vn->__freeVTDNav(vn);
}



// A generic navigation method.
// Move the current to the element according to the direction constants
// If no such element, no position change and return false (0).
/* Legal direction constants are 	<br>
	 * <pre>		ROOT            0  </pre>
	 * <pre>		PARENT          1  </pre>
	 * <pre>		FIRST_CHILD     2  </pre>
	 * <pre>		LAST_CHILD      3  </pre>
	 * <pre>		NEXT_SIBLING    4  </pre>
	 * <pre>		PREV_SIBLING    5  </pre>
	 * <br>
	 */

extern inline Boolean toElement(VTDNav *vn, navDir direction){
	return vn->__toElement(vn,direction);
}


extern inline Boolean toNode(VTDNav *vn, navDir direction){
	return vn->__toNode(vn,direction);
}



extern inline void dumpState(VTDNav *vn){
	vn->__dumpState(vn);
}

extern inline Boolean verifyNodeCorrectness(VTDNav *vn){
	return vn->__verifyNodeCorrectness(vn);
}
/**
 * A generic navigation method.
 * Move the current to the element according to the direction 
 * constants and the element name
 * If no such element, no position change and return false (0).
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
extern inline Boolean toElement2(VTDNav *vn, navDir direction, UCSChar *en){
	return vn->__toElement2(vn,direction,en);
}/*	
 * A generic navigation function with namespace support.
 * Move the current to the element according to the direction constants and the prefix and local names
 * If no such element, no position change and return false(0).
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
 * If not ns enabled, return false immediately with no position change.
 */

extern inline Boolean toElementNS(VTDNav *vn, navDir direction, UCSChar *URL, UCSChar *ln){
	return vn->__toElementNS(vn,direction,URL, ln);
}

//This method normalizes a token into a string in a way that resembles DOM.
//The leading and trailing white space characters will be stripped.
//The entity and character references will be resolved
//Multiple whitespaces char will be collapsed into one.
extern UCSChar *toNormalizedString(VTDNav *vn, int index);

//Convert a token at the given index to a String, 
//(built-in entity and char references not resolved)
//(entities and char references not expanded).
//os and len are in bytes
extern UCSChar *toRawString(VTDNav *vn, int index);
extern UCSChar *toRawString2(VTDNav *vn, int os, int len);

//Convert a token at the given index to a String, (entities and char 
//references resolved).
// An attribute name or an element name will get the UCS2 string of qualified name 
//os and len are in bytes
extern UCSChar *toString(VTDNav *vn, int index);
extern UCSChar *toString2(VTDNav *vn, int os, int len);

/**
 * Set the value of atTerminal
 * This function only gets called in XPath eval
 * when a step calls for @* or child::text()
 */

//inline void setAtTerminal(VTDNav* vn, Boolean b);
//extern inline void setAtTerminal(VTDNav* vn, Boolean b);
/*{
	vn->atTerminal = b;
}*/
extern inline void setAtTerminal(VTDNav* vn, Boolean b){
	vn->atTerminal = b;
}
/**
 * Get the value of atTerminal
 * This function only gets called in XPath eval
 */
//inline Boolean getAtTerminal(VTDNav *vn);
//extern inline Boolean getAtTerminal(VTDNav *vn);
/*{
	return vn->atTerminal;
}*/
extern inline Boolean getAtTerminal(VTDNav *vn){
	return vn->atTerminal;
}

//extern inline int swap_bytes(int i);
extern inline int swap_bytes(int i){
	return (((i & 0xff) << 24) |
		((i & 0xff00) <<8) |
		((i & 0xff0000) >> 8) |
		((i & 0xff000000) >> 24)&0xff);
}

extern int lookupNS(VTDNav *vn);

/* Write VTD+XML into a FILE pointer */
//Boolean writeIndex_VTDNav(VTDNav *vn, FILE *f);

/* overwrite */
extern Boolean overWrite(VTDNav *vn, int index, UByte* ba, int offset, int len);

extern int compareTokenString(VTDNav *vn,int index, UCSChar *s);

extern int compareRawTokenString(VTDNav *vn, int index, UCSChar *s);
extern Boolean matchRawTokenString1(VTDNav *vn, int offset, int len, UCSChar *s);
extern int compareRawTokenString2(VTDNav *vn, int offset, int len, UCSChar *s);
extern int compareTokens(VTDNav *vn, int i1, VTDNav *vn2, int i2);


/* Write VTD+XML into a FILE pointer */
extern Boolean writeIndex_VTDNav(VTDNav *vn, FILE *f);


/* Write VTD+XML into a file of given name */
extern Boolean writeIndex2_VTDNav(VTDNav *vn, char *fileName);


/* Write the VTDs and LCs into an file*/
extern Boolean writeSeparateIndex_VTDNav(VTDNav *vn, char *vtdIndex);

/* Write the VTDs and LCs into an file*/


/* pre-calculate the VTD+XML index size without generating the actual index */
extern Long getIndexSize2(VTDNav *vn);

/* dump XML text into a given file name */
extern void dumpXML(VTDNav *vn, char *fileName);

/* dump XML text into a given file descriptor */
extern void dumpXML2(VTDNav *vn, FILE *f);

/*Get the string length as if the token is converted into a normalized UCS string */
extern int getNormalizedStringLength(VTDNav *vn, int index);
/*Get the string length as if the token is converted into a UCS string (entity resolved) */
extern int getStringLength(VTDNav *vn, int index);
/*Get the string length as if the token is converted into a UCS string (entity not resolved) */
extern int getRawStringLength(VTDNav *vn, int index);
/* Get the offset value right after head (e.g. <a b='b' c='c'> ) */
extern Long getOffsetAfterHead(VTDNav *vn);

/* Test the start of token content at index i matches the content 
of s, notice that this is to save the string allocation cost of
using String's built-in startsWidth */
extern Boolean startsWith(VTDNav *vn, int index, UCSChar *s);

/*Test the end of token content at index i matches the content 
of s, notice that this is to save the string allocation cost of
using String's built-in endsWidth */

extern Boolean endsWith(VTDNav *vn, int index, UCSChar *s);

/*Test whether a given token contains s. notie that this function
directly operates on the byte content of the token to avoid string creation */

extern Boolean contains(VTDNav *vn, int index, UCSChar *s);

/* Convert a token at the given index to a String and any lower case
   character will be converted to upper case, (entities and char
   references resolved).*/
extern UCSChar *toStringUpperCase(VTDNav *vn, int index);

/* Convert a token at the given index to a String and any upper case
   character will be converted to lower case, (entities and char
   references resolved).*/
extern UCSChar *toStringLowerCase(VTDNav *vn, int index);

/* Convert a token at the given index to a String and any lower case
   character will be converted to upper case, (entities and char
   references resolved for character data and attr val).*/
extern UCSChar *toRawStringUpperCase(VTDNav *vn, int index);

/* Convert a token at the given index to a String and any upper case
   character will be converted to lower case, (entities and char
   references resolved for character data and attr val).*/
extern UCSChar *toRawStringLowerCase(VTDNav *vn, int index);

/* DupliateNav duplicates an instance of VTDNav but doesn't retain the original node position*/

extern VTDNav* duplicateNav(VTDNav *vn);

/* CloneNav duplicates an instance of VTDNav, also copies node position over */

extern VTDNav* cloneNav(VTDNav *vn);

/* This method takes a vtd index, and recover its correspondin
 * node position, the index can only be of node type element,
 * document, attribute name, attribute value or character data,
 * or CDATA  */
extern void recoverNode(VTDNav *vn, int index);

/**
	 * Match the string against the token at the given index value. The token will be
     * interpreted as if it is normalized (i.e. all white space char (\r\n\a ) is replaced
     * by a white space, char entities and entity references will be replaced by their correspondin
     * char see xml 1.0 spec interpretation of attribute value normalization) */

extern Boolean matchNormalizedTokenString2(VTDNav *vn,int index, UCSChar *s);

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

extern Long getSiblingElementFragments(VTDNav *vn,int i);

/**
	 * Match the string against the token at the given index value. The token will be
     * interpreted as if it is normalized (i.e. all white space char (\r\n\a ) is replaced
     * by a white space, char entities and entity references will be replaced by their correspondin
     * char see xml 1.0 spec interpretation of attribute value normalization) 
	 */
extern Boolean matchNormalizedTokenString2(VTDNav *vn, int index, UCSChar *s);

/**
	 * (New since version 2.9)
	 * Shallow Normalization follows the rules below to normalize a token into
	 * a string
	 * *#xD#xA gets converted to #xA
	 * *For a character reference, append the referenced character to the normalized value.
	 * *For an entity reference, recursively apply step 3 of this algorithm to the replacement text of the entity.
	 * *For a white space character (#x20, #xD, #xA, #x9), append a space character (#x20) to the normalized value.
	 * *For another character, append the character to the normalized value.*/
extern UCSChar* toNormalizedString2(VTDNav *vn, int index);


/** new since 2.9
	Return the prefix of a token as a string if the token 
    is of the type of starting tag, attribute name, if the 
    the prefix doesn't exist, a null string is returned;
    otherwise a null string is returned
*/
extern UCSChar *getPrefixString(VTDNav *vn, int index);

extern void resolveLC(VTDNav *vn);



extern VTDNav *createVTDNav(int r, encoding_t enc, Boolean ns, int depth,
					 UByte *x, int xLen, FastLongBuffer *vtd, FastLongBuffer *l1,
					 FastLongBuffer *l2, FastIntBuffer *l3, int so, int len,Boolean br);

extern VTDNav_L5 *createVTDNav_L5(int r, encoding_t enc, Boolean ns, int depth,
					 UByte *x, int xLen, FastLongBuffer *vtd, FastLongBuffer *l1,
					 FastLongBuffer *l2, FastLongBuffer *l3, FastLongBuffer *l4, 
					 FastIntBuffer *l5, int so, int len,Boolean br);

extern Long getOffsetBeforeTail(VTDNav *vn);

extern void dumpState_L5(VTDNav *vn);

extern void dumpState(VTDNav *vn);


extern Boolean iterate_following_node(VTDNav *vn);

extern Boolean iterate_preceding_node(VTDNav *vn,int a[], int index);

extern Boolean iterateNode(VTDNav *vn, int dp);

extern inline int getTokenLength2(VTDNav *vn,int index){
	return (int)((longAt(vn->vtdBuffer,index) & MASK_TOKEN_FULL_LEN) >> 32);
}

extern void setCurrentNode(VTDNav *vn);
extern void loadCurrentNode(VTDNav *vn);
extern void fillXPathString(VTDNav *vn,FastIntBuffer *indexBuffer,FastIntBuffer *countBuffer);
extern UCSChar *getXPathStringVal(VTDNav *vn,short mode);
extern UCSChar *getXPathStringVal2(VTDNav *vn,int i,short mode);

extern UCSChar *toNormalizedXPathString(VTDNav *vn,int j);
extern Boolean XPathStringVal_Contains(VTDNav *vn,int j, UCSChar *s);
extern Boolean XPathStringVal_StartsWith(VTDNav *vn,int j, UCSChar *s);
extern Boolean XPathStringVal_EndsWith(VTDNav *vn,int j, UCSChar *s);
extern void dumpFragment(VTDNav *vn,Long l, char *fileName);
extern void dumpFragment2(VTDNav *vn,char *fileName);
extern void dumpElementFragmentNs(VTDNav *vn,char *fileName);
extern Long expandWhiteSpace(VTDNav *vn,Long l);
extern Long trimWhiteSpace(VTDNav *vn,Long l);

extern double XPathStringVal2Double(VTDNav *vn,int j);
extern Boolean XPathStringVal_Matches(VTDNav *vn,int j, UCSChar *s);
extern int XPathStringVal_Matches2(VTDNav *vn,int j, VTDNav *vn2, int k /*k is a token index */);
//extern int 
//extern Boolean iterateNode(VTDNav *vn, int dp);



/* This method takes a vtd index, and recover its correspondin
 * node position, the index can only be of node type element,
 * document, attribute name, attribute value or character data,
 * or CDATA  */



//This method is similar to getElementByName in DOM except it doesn't
//return the nodeset, instead it iterates over those nodes .
//When URL is "*" it will match any namespace

#endif
