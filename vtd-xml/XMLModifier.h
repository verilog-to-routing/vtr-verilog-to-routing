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
#ifndef XMLMODIFIER_H
#define XMLMODIFIER_H
#include "vtdGen.h"
#include "elementFragmentNs.h"
#include "xpath.h"
#include "transcoder.h"

#define MASK_DELETE 0x00000000000000000LL
#define MASK_INSERT_SEGMENT_BYTE  0x2000000000000000LL
#define MASK_INSERT_BYTE 0x4000000000000000LL
#define MASK_INSERT_SEGMENT_STRING 0x6000000000000000LL
#define MASK_INSERT_STRING 0x8000000000000000LL
#define MASK_INSERT_FRAGMENT_NS  0xa000000000000000LL
#define MASK_INSERT_SEGMENT_BYTE_ENCLOSED 0x6000000000000000LL
#define MASK_INSERT_BYTE_ENCLOSED 0x8000000000000000LL
#define MASK_INSERT_FRAGMENT_NS_ENCLOSED 0xe000000000000000LL
#define MASK_NULL 0xc000000000000000LL
//#define MASK_INSERT_BYTE2 0xc000000000000000L

typedef Long (*getBytes)(UCSChar *s);
typedef struct xMLModifier{
	encoding_t encoding;
	IntHash *deleteHash;
	IntHash *insertHash;
	FastLongBuffer *flb; /* lower 32 bit offset, upper 29 bits
						 length, upper 3 bits */
	FastLongBuffer *fob; /*lower 32 bits the object pointer,
						 upper 32 bits the length of the byte array*/
	VTDNav *md; /*master document*/
	getBytes gbytes;
} XMLModifier;

/*typedef struct byteSegment{
	UByte* byteArray;
	int contentOffset;
	int contentLen;
} ByteSegment;

ByteSegment* createByteSegment();
void freeByteSegment(ByteSegment *bs);*/

XMLModifier *createXMLModifier();
XMLModifier *createXMLModifier2(VTDNav *vn);
void freeXMLModifier(XMLModifier *xm);
void bind4XMLModifier(XMLModifier *xm, VTDNav *md);
   /**
     * Removes content from the master XML document
     * It first calls getCurrentIndex() if the result is
     * a starting tag, then the entire element referred to
     * by the starting tag is removed
     * If the result is an attribute name or ns node, then
     * the corresponding attribute name/value pair is removed
     * If the token type is one of text, CDATA or commment,
     * then the entire node, including the starting and ending
     * delimiting text surrounding the content, is removed
     *
     */

Long getBytes_UTF8(UCSChar *s);
Long getBytes_UTF16LE(UCSChar *s);
Long getBytes_UTF16BE(UCSChar *s);
Long getBytes_ISO_8859_1(UCSChar *s);
Long getBytes_ASCII(UCSChar *s);

void remove4XMLModifier(XMLModifier *xm);
void removeToken(XMLModifier *xm, int i);
void removeAttribute(XMLModifier *xm,int attrNameIndex);
void removeContent(XMLModifier *xm, int offset, int len);

void updateToken(XMLModifier *xm, int index, UCSChar *newContent);
void updateToken2(XMLModifier *xm, int index, UByte *byteContent, int contentOffset, int contentLen);
void updateToken3(XMLModifier *xm, int index, UByte *byteContent, int contentOffset, int contentLen, encoding_t src_encoding);
void updateToken4(XMLModifier *xm, int index, VTDNav *vn, int contentOffset, int contentLen);

void insertAfterElement(XMLModifier *xm, UCSChar *s);
void insertBeforeElement(XMLModifier *xm, UCSChar *s);
void insertBeforeTail(XMLModifier *xm, UCSChar *s);
void insertAttribute(XMLModifier *xm, UCSChar *attr);
void insertAfterHead(XMLModifier *xm, UCSChar *attr);

void insertAfterElement2(XMLModifier *xm, UByte* ba, int arrayLen);
void insertBeforeElement2(XMLModifier *xm, UByte* ba, int arrayLen);
void insertAfterHead2(XMLModifier *xm, UByte* ba, int arrayLen);
void insertBeforeTail2(XMLModifier *xm, UByte* ba, int arrayLen);

void insertAfterElement3(XMLModifier *xm, UByte* ba, int contentOffset, int contentLen);
void insertBeforeElement3(XMLModifier *xm, UByte* ba, int contentOffset, int contentLen);
void insertAfterHead3(XMLModifier *xm, UByte* ba, int contentOffset, int contentLen);
void insertBeforeTail3(XMLModifier *xm, UByte* ba, int contentOffset, int contentLen);


void insertBeforeElement4(XMLModifier *xm, ElementFragmentNs *ef);
void insertAfterElement4(XMLModifier *xm, ElementFragmentNs *ef);
void insertAfterHead4(XMLModifier *xm, ElementFragmentNs *ef);
void insertBeforeTail4(XMLModifier *xm, ElementFragmentNs *ef);


void insertAfterElement5(XMLModifier *xm, encoding_t src_encoding, UByte* ba, int arrayLen);
void insertBeforeElement5(XMLModifier *xm, encoding_t src_encoding, UByte* ba, int arrayLen);
void insertAfterHead5(XMLModifier *xm, encoding_t src_encoding, UByte* ba, int arrayLen);
void insertBeforeTail5(XMLModifier *xm, encoding_t src_encoding, UByte* ba, int arrayLen);


void insertAfterElement6(XMLModifier *xm, encoding_t src_encoding, UByte* ba, int contentOffset, int contentLen);
void insertBeforeElement6(XMLModifier *xm, encoding_t src_encoding, UByte* ba, int contentOffset, int contentLen);
void insertAfterHead6(XMLModifier *xm, encoding_t src_encoding, UByte* ba, int contentOffset, int contentLen);
void insertBeforeTail6(XMLModifier *xm, encoding_t src_encoding, UByte* ba, int contentOffset, int contentLen);


void insertAfterElement7(XMLModifier *xm, VTDNav *vn1, int contentOffset, int contentLen);
void insertBeforeElement7(XMLModifier *xm, VTDNav *vn1, int contentOffset, int contentLen);
void insertAfterHead7(XMLModifier *xm, VTDNav *vn1, int contentOffset, int contentLen);
void insertBeforeTail7(XMLModifier *xm, VTDNav *vn1, int contentOffset, int contentLen);
/*
void updateToken2(XMLModifier *xm, int index, UByte *newContentBytes, int len);
void insertAfterElement2(XMLModifier *xm, UByte *b, int len);
void insertBeforeElement2(XMLModifier *xm, UByte *b, int len);
void insertAttribute2(XMLModifier *xm, UByte *attr, int len);
*/

void output(XMLModifier *xm, FILE *f);
void output2(XMLModifier *xm, char *fileName);

void updateElementName(XMLModifier *xm, UCSChar* elementName);

void resetXMLModifier(XMLModifier *xm);



#endif
