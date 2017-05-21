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
#ifndef VTDGEN_H
#define VTDGEN_H

//#include "customTypes.h"
#include "fastLongBuffer.h"
#include "fastIntBuffer.h"
#include "vtdNav.h"
#include "UTF8Char.h"
#include "XMLChar.h"
#include "decoder.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


// define document encoding

// other constants

#define ATTR_NAME_ARRAY_SIZE 16
#define TAG_STACK_SIZE 256
#define MAX_DEPTH 254
#define MAX_TOKEN_LENGTH ((1<<20)-1)
#define MAX_PREFIX_LENGTH ((1<<9)-1)
#define MAX_QNAME_LENGTH ((1<<11)-1)

typedef struct vTDGen {
	int ns,is_ns;
	int VTDDepth;
	encoding_t encoding;
	int last_depth;
	int last_l1_index;
	int last_l2_index;
	int last_l3_index;
	int last_l4_index;

	int increment;
	Boolean BOM_detected;
	Boolean must_utf_8;
	int ch;
	int ch_temp;
	
	int offset;
	Boolean ws;
	int temp_offset;
	int length1,length2;
	int depth;

	int prev_offset;
	int rootIndex;
	UByte* XMLDoc; // byte buffer containing
	int docLen; // length of XML (can be a segment of XMLDoc)
	int bufLen; // length of XMLDoc (possibly bigger than docLen)

	// buffers
	FastLongBuffer *VTDBuffer;
	FastLongBuffer *l1Buffer;
	FastLongBuffer *l2Buffer;
	FastIntBuffer *l3Buffer;
	FastLongBuffer *_l3Buffer;
	FastLongBuffer *_l4Buffer;
	FastIntBuffer *_l5Buffer;
	
	FastIntBuffer  *nsBuffer1;
	FastLongBuffer *nsBuffer2;
    FastLongBuffer *nsBuffer3;
	Long currentElementRecord;

	Boolean br;//buffer reuse flag
	Boolean stateTransfered; // indicate whether VTDNav has received all LC and VTD buffers

	int endOffset;
	Long* tag_stack;
	Long* attr_name_array;
	int attr_count;
	Long* prefixed_attr_name_array;
	int* prefix_URL_array;
	int  prefixed_attr_count;
	int anaLen;
	int panaLen;
	int docOffset;
	short LcDepth;
	Boolean singleByteEncoding;
	Boolean shallowDepth;
} VTDGen;

// create VTDGen
VTDGen *createVTDGen();

void selectLcDepth(VTDGen *vg,int d);

// free VTDGen
void freeVTDGen(VTDGen *vg);

// clear the internal state of VTDGen so it can process 
// the next XML file
void clear(VTDGen *vg);

// Returns the VTDNav object after parsing, it also cleans 
// internal state so VTDGen can process the next file.
VTDNav *getNav(VTDGen *vg);

// Generating VTD tokens and Location cache info.
// One specifies whether the parsing is namespace aware or not.
void parse(VTDGen *vg, Boolean ns);

// Generating VTD tokens and Location cache info for an XML file
Boolean parseFile(VTDGen *vg, Boolean ns,char *fileName);

// Set the XMLDoc container.
void setDoc(VTDGen *vg, UByte *byteArray, int arrayLen);

// Set the XMLDoc container.Also set the offset and len of the document 
void setDoc2(VTDGen *vg, UByte *byteArray, int arrayLen, int offset, int docLen);

// set the XML Doc container and turn on buffer reuse
void setDoc_BR(VTDGen *vg, UByte *byteArray, int arrayLen);

//Set the XMLDoc container.Also set the offset and len of the document 
void setDoc_BR2(VTDGen *vg, UByte *byteArray, int arrayLen, int offset, int docLen);

/* load vtd+xml file from a file */
VTDNav* loadIndex(VTDGen *vg, char *fileName); 
/* Load VTD+XML from a FILE pointer */
VTDNav* loadIndex2(VTDGen *vg, FILE *f); 

/* load VTD+XML from a byte array */
VTDNav* loadIndex3(VTDGen *vg, UByte* ba,int len);

/* Write VTD+XML into a FILE pointer */
Boolean writeIndex(VTDGen *vg, FILE *f);

/* Write VTD+XML into a file of given name, this file will be created on hard disk */
Boolean writeIndex2(VTDGen *vg, char *fileName);

/* Pre-calculate the integrated VTD+XML index size without generating the actual index */
Long getIndexSize(VTDGen *vg);

/* Write the VTDs and LCs into an file*/
void writeSeparateIndex(VTDGen *vg, char *vtdIndex);

/* Load the separate VTD index and XmL file.*/
VTDNav* loadSeparateIndex(VTDGen *vg, char *XMLFile, char *VTDIndexFile);

/* configure the VTDGen to enable or disable (disabled by default) white space nodes */
void enableIgnoredWhiteSpace(VTDGen *vg, Boolean b);

#endif
