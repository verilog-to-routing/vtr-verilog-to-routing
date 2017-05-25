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
#include "vtdGen.h"
#include "indexHandler.h"
#include <sys/stat.h>
typedef enum pState {
STATE_LT_SEEN,
STATE_START_TAG,
STATE_END_TAG,
STATE_ATTR_NAME,
STATE_ATTR_VAL,
STATE_TEXT,
STATE_DOC_START,
STATE_DOC_END,
STATE_PI_TAG,
STATE_PI_VAL,
STATE_DEC_ATTR_NAME,
STATE_COMMENT,
STATE_CDATA, 
STATE_DOCTYPE,
STATE_END_COMMENT,
STATE_END_PI
} parseState;

/* internal functions */
static int  entityIdentifier(VTDGen *vg);
static void printLineNumber(VTDGen *vg);
static inline int getChar(VTDGen *vg);
static Boolean skip4OtherEncoding(VTDGen *vg, int ch1);
static int handleOtherEncoding(VTDGen *vg);
static int handle_16le(VTDGen *vg);
static int handle_16be(VTDGen *vg);
static int handle_utf8(VTDGen *vg,int temp);
static void matchISOEncoding(VTDGen *vg);
static void matchCPEncoding(VTDGen *vg);
static void matchWindowsEncoding(VTDGen *vg);
static void matchUTFEncoding(VTDGen *vg);
static inline Boolean skipUTF8(VTDGen *vg,int temp,int ch);
static inline Boolean skip_16be(VTDGen *vg, int ch);
static inline Boolean skip_16le(VTDGen *vg, int ch);
//static int getCharAfterSe(VTDGen *vg);
static inline int getCharAfterS(VTDGen *vg);
static inline Boolean skipChar(VTDGen *vg, int ch);
static inline void writeVTD(VTDGen *vg, int offset, int length, tokenType token_type, int depth);
static inline void _writeVTD(VTDGen *vg, int offset, int length, tokenType token_type, int depth);
static inline void writeVTDText(VTDGen *vg, int offset, int length, tokenType token_type, int depth);
static inline void writeVTD_L5(VTDGen *vg, int offset, int length, tokenType token_type, int depth);
static void finishUp(VTDGen *vg);

static void decide_encoding(VTDGen *vg);
static int process_end_pi(VTDGen *vg);
static int process_end_comment(VTDGen *vg);
static int process_comment(VTDGen *vg);
static int process_doc_type(VTDGen *vg);
static int process_cdata(VTDGen *vg);
static int process_pi_val(VTDGen *vg);
static int process_pi_tag(VTDGen *vg);
static int process_dec_attr(VTDGen *vg);
static void throwInvalidEncodingException();
static int process_start_doc(VTDGen *vg);
static int process_end_doc(VTDGen *vg);
static int process_qm_seen(VTDGen *vg);
static int process_ex_seen(VTDGen *vg);
static void addWhiteSpaceRecord(VTDGen *vg);
static void qualifyAttributes(VTDGen *vg);
static void qualifyElement(VTDGen *vg);
static Boolean matchXML(VTDGen *vg, int byte_offset);
static Boolean matchURL(VTDGen *vg, int bos1, int len1, int bos2, int len2);
static int identifyNsURL(VTDGen *vg, int byte_offset, int length);
static int getCharUnit(VTDGen *vg, int byte_offset);
static void disallow_xmlns(VTDGen *vg, int byte_offset);
static void checkQualifiedAttributeUniqueness(VTDGen *vg);
static Boolean checkPrefix(VTDGen *vg, int os, int len);
static Boolean checkPrefix2(VTDGen *vg, int os, int len);
static void checkAttributeUniqueness(VTDGen *vg);
static Long _getCharResolved(VTDGen *vg,int byte_offset);
static Long _getChar(VTDGen *vg, int byte_offset);
static int decode(VTDGen *vg, int byte_offset);
static Long _handle_16be(VTDGen *vg, int byte_offset);
static Long _handle_16le(VTDGen *vg, int byte_offset);
static Long _handle_utf8(VTDGen *vg,int c,int offset);
static Long _handleOtherEncoding(VTDGen *vg,int byte_offset);
static void handleOtherTextChar2(VTDGen *vg,int ch);
static void handleOtherTextChar(VTDGen *vg, int ch);


/* create VTDGen */
VTDGen *createVTDGen(){
	Long* l = NULL, *l1=NULL; int* l2=NULL;
	Long* ts = NULL;
	VTDGen *vg = NULL;

	l = (Long*) malloc(ATTR_NAME_ARRAY_SIZE*sizeof(Long));
	if (l==NULL){
		throwException2(out_of_mem,
			"VTDGen allocation failed ");
		return NULL;
	}
	l1 = (Long*) malloc(ATTR_NAME_ARRAY_SIZE*sizeof(Long));
	if (l1==NULL){
		throwException2(out_of_mem,
			"VTDGen allocation failed ");
		return NULL;
	}
	l2 = (int*) malloc(ATTR_NAME_ARRAY_SIZE*sizeof(int));
	if (l2==NULL){
		throwException2(out_of_mem,
			"VTDGen allocation failed ");
		return NULL;
	}
	ts = (Long*) malloc(TAG_STACK_SIZE * sizeof(Long));
	if (ts==NULL) {
		free(l);
		throwException2(out_of_mem,
			"VTDGen allocation failed ");
		return NULL;
	}

	vg = (VTDGen *)malloc(sizeof(VTDGen));
	if (vg==NULL){
		free(l);free(l1);free(l2);
		free(ts);
		throwException2(out_of_mem,
			"VTDGen allocation failed ");
		return NULL;
	}
	vg->nsBuffer1 = createFastIntBuffer(4);
	vg->nsBuffer2 = createFastLongBuffer(4);
	vg->nsBuffer3 = createFastLongBuffer(4);
	vg->currentElementRecord = 0;
	vg->singleByteEncoding = TRUE;
	vg->shallowDepth = TRUE;

	vg->anaLen = ATTR_NAME_ARRAY_SIZE;
	vg->panaLen = ATTR_NAME_ARRAY_SIZE;
	vg->attr_name_array = l;
	vg->prefixed_attr_name_array= l1;
	vg->prefix_URL_array = l2;
	vg->tag_stack = ts;

	vg->VTDDepth = 0;
	vg->LcDepth = 3;

	vg->VTDBuffer =	vg->l1Buffer = 	vg->l2Buffer = NULL;
	vg->l3Buffer = NULL;
	vg->bufLen = vg->docLen = vg->docLen = vg->last_depth = 0;
	vg->last_l4_index = vg->last_l3_index = vg->last_l2_index = vg->last_l1_index = 0;
	vg->XMLDoc = NULL;
	vg->rootIndex = vg->endOffset= 0;
	vg->ns = vg->offset = vg->prev_offset =0;
	vg->stateTransfered = TRUE; // free VTDGen won't free all location cache and VTD buffer

	vg->br = FALSE;
	vg->ws = FALSE;
	return vg;
}

// free VTDGen, it doesn't free the memory block containing XML doc
void freeVTDGen(VTDGen *vg){
	if (vg != NULL){
		free(vg->attr_name_array);
		free(vg->prefix_URL_array);
		free(vg->prefixed_attr_name_array);
		free(vg->tag_stack);
		freeFastLongBuffer(vg->nsBuffer3);
		freeFastLongBuffer(vg->nsBuffer2);
		freeFastIntBuffer(vg->nsBuffer1);
		if (vg->stateTransfered == FALSE || vg->br == TRUE){
			//free(vg->XMLDoc);
			freeFastLongBuffer(vg->VTDBuffer);
			freeFastLongBuffer(vg->l1Buffer);
			freeFastLongBuffer(vg->l2Buffer);
			freeFastIntBuffer(vg->l3Buffer);
		}
		free(vg);
	}	
}

// clear the internal state of VTDGen so it can process 
// the next XML file
void clear(VTDGen *vg){
	if (vg->br == FALSE){
		if (vg->stateTransfered == FALSE){
			//free(vg->XMLDoc);
			freeFastLongBuffer(vg->VTDBuffer);			
			freeFastLongBuffer(vg->l1Buffer);
			freeFastLongBuffer(vg->l2Buffer);
			freeFastIntBuffer(vg->l3Buffer);
			freeFastLongBuffer(vg->_l3Buffer);
			freeFastLongBuffer(vg->_l4Buffer);
			freeFastIntBuffer(vg->_l5Buffer);
			
		}
		vg->VTDBuffer = NULL;
		vg->l1Buffer = NULL;
		vg->l2Buffer = NULL;
		vg->l3Buffer = NULL;
		vg->_l3Buffer = NULL;
		vg->_l4Buffer = NULL;
		vg->_l5Buffer = NULL;
		vg->XMLDoc = NULL;
	}
	
	vg->currentElementRecord = 0;
	clearFastIntBuffer(vg->nsBuffer1);
	clearFastLongBuffer(vg->nsBuffer2);
	clearFastLongBuffer(vg->nsBuffer3);
	vg->last_depth = vg->last_l1_index = 
		vg->last_l2_index = vg->last_l3_index =0;
	vg->offset = vg->temp_offset = 0;
	vg->rootIndex = 0;

	vg->depth = -1;
	vg->increment = 1;
	vg->BOM_detected = FALSE;
	vg->must_utf_8 = FALSE;
	vg->ch = vg->ch_temp = 0;
	vg->encoding = FORMAT_UTF8;

}

//detect whether the entity is valid or not and increment offset.
static int  entityIdentifier(VTDGen *vg){
	int ch = getChar(vg);
	int val = 0;
	switch (ch) {
			case '#' :
				ch = getChar(vg);
				if (ch == 'x') {
					while (TRUE) {
						ch = getChar(vg);
						if (ch >= '0' && ch <= '9') {
							val = (val << 4) + (ch - '0');
						} else if (ch >= 'a' && ch <= 'f') {
							val = (val << 4) + (ch - 'a' + 10);
						} else if (ch >= 'A' && ch <= 'F') {
							val = (val << 4) + (ch - 'A' + 10);
						} else if (ch == ';') {
							return val;
						} else{
							throwException( parse_exception,0,
								"Parse exception in entityIdentifier",
								"Errors in char reference: Illegal char following &#x.");
						}
					}
				} else {
					while (TRUE) {
						if (ch >= '0' && ch <= '9') {
							val = val * 10 + (ch - '0');
						} else if (ch == ';') {
							break;
						} else{
							throwException( parse_exception,0,
								"Parse exception in entityIdentifier",
								"Errors in char reference: Illegal char following &#.");
						}
						ch = getChar(vg);
					}
				}
				if (!XMLChar_isValidChar(val)) {
					throwException( parse_exception,0,
								"Parse exception in entityIdentifier",
								"Errors in entity reference: Invalid XML char.");
				}
				return val;
				//break;

			case 'a' :
				ch = getChar(vg);
				if (ch == 'm') {
					if (getChar(vg) == 'p' && getChar(vg) == ';') {
						return '&';
					} else{
						throwException( parse_exception,0,
								"Parse exception in entityIdentifier",
								"Errors in Entity: Illegal builtin reference");
					}
				} else if (ch == 'p') {
					if (getChar(vg) == 'o'
						&& getChar(vg) == 's'
						&& getChar(vg) == ';') {
							//System.out.println(" entity for ' ");
							return '\'';
					} else{
						throwException( parse_exception,0,
								"Parse exception in entityIdentifier",
								"Errors in Entity: Illegal builtin reference");
					}
				} else{
					throwException( parse_exception,0,
								"Parse exception in entityIdentifier",
								"Errors in Entity: Illegal builtin reference");
				}

			case 'q' :
				if (getChar(vg) == 'u'
					&& getChar(vg) == 'o'
					&& getChar(vg) == 't'
					&& getChar(vg) == ';') {
						return '"';
				} else{
					throwException( parse_exception,0,
								"Parse exception in entityIdentifier",
								"Errors in Entity: Illegal builtin reference");
				}
			case 'l' :
				if (getChar(vg) == 't' && getChar(vg) == ';') {
					return '<';
				} else{
					throwException( parse_exception,0,
								"Parse exception in entityIdentifier",
								"Errors in Entity: Illegal builtin reference");
				}

			case 'g' :
				if (getChar(vg) == 't' && getChar(vg) == ';') {
					return '>';
				} else{
					throwException( parse_exception,0,
								"Parse exception in entityIdentifier",
								"Errors in Entity: Illegal builtin reference");
				}
			default :
				throwException( parse_exception,0,
								"Parse exception in entityIdentifier",
								"Errors in Entity: Illegal builtin reference");
	}
	return 0;
}



// The string indicating the position (line number:offset) 
// of the offset if there is an exception.
static void printLineNumber(VTDGen *vg){
	int so = vg->docOffset;
	int lineNumber = 0;
	int lineOffset = 0;
	//int end = vg->offset;

	if (vg->encoding < FORMAT_UTF_16BE) {
		while (so <= vg->offset-1) {
			if (vg->XMLDoc[so] == '\n') {
				lineNumber++;
				lineOffset = so;
			}
			//lineOffset++;
			so++;
		}
		lineOffset = vg->offset - lineOffset;
	} else if (vg->encoding == FORMAT_UTF_16BE) {
		while (so <= vg->offset-2) {
			if (vg->XMLDoc[so + 1] == '\n' && vg->XMLDoc[so] == 0) {
				lineNumber++;
				lineOffset = so;
			}
			so += 2;
		}
		lineOffset = (vg->offset - lineOffset) >> 1;
	} else {
		while (so <= vg->offset-2) {
			if (vg->XMLDoc[so] == '\n' && vg->XMLDoc[so + 1] == 0) {
				lineNumber++;
				lineOffset = so;
			}
			so += 2;
		}
		lineOffset = (vg->offset - lineOffset) >> 1;
	}
	//return "\nLine Number: " + lineNumber + " Offset: " + lineOffset;
	//printf("\nLine Number: %d  Offset: %d \n",lineNumber+1, lineOffset-1);
}

/* This method automatically converts the underlying byte 
 representation character into the right UCS character format.*/

static inline int getChar(VTDGen *vg){
	int temp = 0;
	if (vg->offset >= vg->endOffset){
		throwException(parse_exception,0,
			"Parse exception in getChar",
			"Premature EOF reached, XML document incomplete");			
	}
	switch (vg->encoding) {
			case FORMAT_ASCII :
				temp = vg->XMLDoc[vg->offset];
				if (temp <128) {
					vg->offset++;
					return temp;
				}else
				    throwException( parse_exception,0,
								"Parse exception in getChar", "invalid ASCII char detected");
			case FORMAT_ISO_8859_1 :
				temp = vg->XMLDoc[vg->offset];
				vg->offset++;
				return temp;
			case FORMAT_UTF8 :

				temp = vg->XMLDoc[vg->offset];
				if (temp <128) {
					vg->offset++;
					return temp;
				}
				//temp = temp & 0xff;
				return handle_utf8(vg,temp);


			case FORMAT_UTF_16BE :
				// implement UTF-16BE to UCS4 conversion
				return handle_16be(vg);

			case FORMAT_UTF_16LE :
				return handle_16le(vg);


			default :
				return handleOtherEncoding(vg);

	}
}

static int handle_16le(VTDGen *vg){
	int temp,val;
	temp = vg->XMLDoc[vg->offset + 1] << 8 | vg->XMLDoc[vg->offset];
	if (temp < 0xd800 || temp > 0xdfff) { // check for low surrogate
		vg->offset += 2;
		return temp;
	} else {
		if(temp<0xd800 || temp>0xdbff){
			throwException( parse_exception,0,
								"Parse exception in getChar",
								"UTF 16 LE encoding error: should never happen");
		}
		val = temp;
		temp = vg->XMLDoc[vg->offset + 3] << 8 | vg->XMLDoc[vg->offset + 2];
		if (temp < 0xdc00 || temp > 0xdfff) {
			throwException( parse_exception,0,
								"Parse exception in getChar",
								"UTF 16 LE encoding error: should never happen");		
		}
		val = ((temp - 0xd800) << 10) + (val - 0xdc00) + 0x10000;
		vg->offset += 4;
		return val;
	}
}


static Boolean skip4OtherEncoding(VTDGen *vg, int ch1){
	UByte ch = vg->XMLDoc[vg->offset];
	int temp;
	switch (vg->encoding)
	{
	case FORMAT_ISO_8859_2:
		temp = iso_8859_2_decode(ch);
		break;

	case FORMAT_ISO_8859_3:
		temp = iso_8859_3_decode(ch);
		break;

	case FORMAT_ISO_8859_4:
		temp = iso_8859_4_decode(ch);
		break;
	case FORMAT_ISO_8859_5:
		temp = iso_8859_5_decode(ch);
		break;
	case FORMAT_ISO_8859_6:
		temp = iso_8859_6_decode(ch);
		break;

	case FORMAT_ISO_8859_7:
		temp = iso_8859_7_decode(ch);
		break;

	case FORMAT_ISO_8859_8:
		temp = iso_8859_8_decode(ch);
		break;

	case FORMAT_ISO_8859_9:
		temp = iso_8859_9_decode(ch);
		break;

	case FORMAT_ISO_8859_10:
		temp = iso_8859_10_decode(ch);
		break;

	case FORMAT_ISO_8859_11:
		temp = iso_8859_11_decode(ch);
		break;

	case FORMAT_ISO_8859_13:
		temp = iso_8859_13_decode(ch);
		break;

	case FORMAT_ISO_8859_14:
		temp = iso_8859_14_decode(ch);
		break;

	case FORMAT_ISO_8859_15:
		temp = iso_8859_15_decode(ch);
		break;

	case FORMAT_WIN_1250:
		temp = windows_1250_decode(ch);
		break;

	case FORMAT_WIN_1251:
		temp = windows_1251_decode(ch);
		break;

	case FORMAT_WIN_1252:
		temp = windows_1252_decode(ch);
		break;

	case FORMAT_WIN_1253:
		temp = windows_1253_decode(ch);
		break;

	case FORMAT_WIN_1254:
		temp = windows_1254_decode(ch);
		break;

	case FORMAT_WIN_1255:
		temp = windows_1255_decode(ch);
		break;

	case FORMAT_WIN_1256:
		temp = windows_1256_decode(ch);
		break;

	case FORMAT_WIN_1257:
		temp = windows_1257_decode(ch);
		break;

	case FORMAT_WIN_1258:
		temp = windows_1258_decode(ch);
		break;

	default:
		throwException( parse_exception,0,
			"Parse exception in skipChar",
			"Unknown encoding");	
	}

	if (temp == ch1)
	{
		vg->offset++;
		return TRUE;
	}
	else
		return FALSE;

}
static int handleOtherEncoding(VTDGen *vg){
	UByte ch = vg->XMLDoc[vg->offset++];
	switch (vg->encoding)
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

	case FORMAT_WIN_1258:
		return windows_1258_decode(ch);
	default:
		throwException( parse_exception,0,
			"Parse exception in getChar",
			"Unknown encoding");
		
	}
	return 0;
}
static int handle_16be(VTDGen *vg){
	int temp,val;
	temp = vg->XMLDoc[vg->offset] << 8 | vg->XMLDoc[vg->offset + 1];
	if ((temp < 0xd800)
		|| (temp > 0xdfff)) { // not a high surrogate
			vg->offset += 2;
			return temp;
	} else {
		if(temp<0xd800 || temp>0xdbff){
			throwException( parse_exception,0,
				"Parse exception in getChar",
				"UTF 16 BE encoding error: should never happen");
		}
		val = temp;
		temp = vg->XMLDoc[vg->offset + 2] << 8 | vg->XMLDoc[vg->offset + 3];
		if (temp < 0xdc00 || temp > 0xdfff) {
			throwException( parse_exception,0,
				"Parse exception in getChar",
				"UTF 16 BE encoding error: should never happen");
		}
		val = ((val - 0xd800) <<10) + (temp - 0xdc00) + 0x10000;
		vg->offset += 4;
		return val;
	}
}

static int handle_utf8(VTDGen *vg, int temp){
	int c,d,a,i;
	int val;
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
			default :
				throwException( parse_exception,0,
					"Parse exception in getChar",
					"UTF 8 encoding error: should never happen");			
	}
	val = (temp & c) << d;
	i = a - 1;
	while (i >= 0) {
		temp = vg->XMLDoc[vg->offset + a - i];
		if ((temp & 0xc0) != 0x80){
			throwException( parse_exception,0,
					"Parse exception in getChar",
					"UTF 8 encoding error: should never happen");	
		}
		val = val | ((temp & 0x3f) << ((i<<2)+(i<<1)));
		i--;
	}
	vg->offset += a + 1;
	return val;

}

static void throwInvalidEncodingException (){
	throwException(parse_exception,0,
		"Parse exception in parse()",
		"XML decl error: Invalid Encoding");
}




static void matchISOEncoding(VTDGen *vg){
	if ((skipChar(vg,'s')
		|| skipChar(vg,'S'))
		&& (skipChar(vg,'o')
		|| skipChar(vg,'O'))
		&& skipChar(vg,'-')
		&& skipChar(vg,'8')
		&& skipChar(vg,'8')
		&& skipChar(vg,'5')
		&& skipChar(vg,'9')
		&& skipChar(vg,'-')
		) {
			if (vg->encoding != FORMAT_UTF_16LE
				&& vg->encoding	!= FORMAT_UTF_16BE) {
					if (vg->must_utf_8){
						throwException(parse_exception,0,
							"Parse exception in parse()",
							"XML decl error: Can't switch from UTF-8");
					}
					if(skipChar(vg,'1')){
						if (skipChar(vg,vg->ch_temp)){
							vg->encoding = FORMAT_ISO_8859_1;
							_writeVTD(vg,
								vg->temp_offset,
								10,
								TOKEN_DEC_ATTR_VAL,
								vg->depth);
							return;
						} else if (skipChar(vg,'0')){
							vg->encoding = FORMAT_ISO_8859_10;
							iso_8859_10_chars_init();
							_writeVTD(vg,
								vg->temp_offset,
								11,
								TOKEN_DEC_ATTR_VAL,
								vg->depth);
						}else if (skipChar(vg,'1')){
							vg->encoding = FORMAT_ISO_8859_11;
							iso_8859_11_chars_init();
							_writeVTD(vg,
								vg->temp_offset,
								11,
								TOKEN_DEC_ATTR_VAL,
								vg->depth);
						}else if (skipChar(vg,'3')){
							vg->encoding = FORMAT_ISO_8859_13;
							iso_8859_13_chars_init();
							_writeVTD(vg,
								vg->temp_offset,
								11,
								TOKEN_DEC_ATTR_VAL,
								vg->depth);
						}else if (skipChar(vg,'4')){
							vg->encoding = FORMAT_ISO_8859_14;
							iso_8859_14_chars_init();
							_writeVTD(vg,
								vg->temp_offset,
								11,
								TOKEN_DEC_ATTR_VAL,
								vg->depth);
						}else if (skipChar(vg,'5')){
							vg->encoding = FORMAT_ISO_8859_15;
							iso_8859_15_chars_init();
							_writeVTD(vg,
								vg->temp_offset,
								11,
								TOKEN_DEC_ATTR_VAL,
								vg->depth);
						}
						else
							throwInvalidEncodingException ();		
					}else if(skipChar(vg,'2')){						
						vg->encoding = FORMAT_ISO_8859_2;
						iso_8859_2_chars_init();
						_writeVTD(vg,
							vg->temp_offset,
							10,
							TOKEN_DEC_ATTR_VAL,
							vg->depth);
					}else if(skipChar(vg,'3')){						
						vg->encoding = FORMAT_ISO_8859_3;
						iso_8859_3_chars_init();
						_writeVTD(vg,
							vg->temp_offset,
							10,
							TOKEN_DEC_ATTR_VAL,
							vg->depth);
					}else if(skipChar(vg,'4')){						
						vg->encoding = FORMAT_ISO_8859_4;
						iso_8859_4_chars_init();
						_writeVTD(vg,
							vg->temp_offset,
							10,
							TOKEN_DEC_ATTR_VAL,
							vg->depth);
					}else if(skipChar(vg,'5')){						
						vg->encoding = FORMAT_ISO_8859_5;
						iso_8859_5_chars_init();
						_writeVTD(vg,
							vg->temp_offset,
							10,
							TOKEN_DEC_ATTR_VAL,
							vg->depth);
					}else if(skipChar(vg,'6')){							
						vg->encoding = FORMAT_ISO_8859_6;
						iso_8859_6_chars_init();							
						_writeVTD(vg,
							vg->temp_offset,
							10,
							TOKEN_DEC_ATTR_VAL,
							vg->depth);
					}else if(skipChar(vg,'7')){						
						vg->encoding = FORMAT_ISO_8859_7;
						iso_8859_7_chars_init();								
						_writeVTD(vg,
							vg->temp_offset,
							10,
							TOKEN_DEC_ATTR_VAL,
							vg->depth);												
					}else if(skipChar(vg,'8')){						
						vg->encoding = FORMAT_ISO_8859_8;
						iso_8859_8_chars_init();								
						_writeVTD(vg,
							vg->temp_offset,
							10,
							TOKEN_DEC_ATTR_VAL,
							vg->depth);												
					}else if(skipChar(vg,'9')){						
						vg->encoding = FORMAT_ISO_8859_9;
						iso_8859_9_chars_init();
						_writeVTD(vg,
							vg->temp_offset,
							10,
							TOKEN_DEC_ATTR_VAL,
							vg->depth);
					}else 
						throwInvalidEncodingException();
					if (skipChar(vg,vg->ch_temp))
						return;
			} else{
				throwException(parse_exception,0,
							"Parse exception in parse()",
							"XML decl error: Can't switch encoding to ISO-8859");
			}

	}
	throwInvalidEncodingException ();
}


static void matchCPEncoding(VTDGen *vg){
	if ((skipChar(vg,'p')
		|| skipChar(vg,'P'))
		&& skipChar(vg,'-')
		&& skipChar(vg,'1')
		&& skipChar(vg,'2')
		&& skipChar(vg,'5')) {
			if (vg->encoding != FORMAT_UTF_16LE
				&& vg->encoding	!= FORMAT_UTF_16BE) {
					if (vg->must_utf_8){
						throwException(parse_exception,0,
							"Parse exception in parse()",
							"XML decl error: Can't switch from UTF-8");
					}
					if(skipChar(vg,'0')){						
						vg->encoding = FORMAT_WIN_1250;
						windows_1250_chars_init();
						_writeVTD(vg,
							vg->temp_offset,
							6,
							TOKEN_DEC_ATTR_VAL,
							vg->depth);
					}else if(skipChar(vg,'1')){						
						vg->encoding = FORMAT_WIN_1251;
						windows_1251_chars_init();
						_writeVTD(vg,
							vg->temp_offset,
							6,
							TOKEN_DEC_ATTR_VAL,
							vg->depth);
					}else if(skipChar(vg,'2')){						
						vg->encoding = FORMAT_WIN_1252;
						windows_1252_chars_init();
						_writeVTD(vg,
							vg->temp_offset,
							6,
							TOKEN_DEC_ATTR_VAL,
							vg->depth);
					}else if(skipChar(vg,'3')){						
						vg->encoding = FORMAT_WIN_1253;
						windows_1253_chars_init();
						_writeVTD(vg,
							vg->temp_offset,
							6,
							TOKEN_DEC_ATTR_VAL,
							vg->depth);
					}else if(skipChar(vg,'4')){						
						vg->encoding = FORMAT_WIN_1250;
						windows_1250_chars_init();
						_writeVTD(vg,
							vg->temp_offset,
							6,
							TOKEN_DEC_ATTR_VAL,
							vg->depth);
					}else if(skipChar(vg,'5')){						
						vg->encoding = FORMAT_WIN_1255;
						windows_1255_chars_init();
						_writeVTD(vg,
							vg->temp_offset,
							6,
							TOKEN_DEC_ATTR_VAL,
							vg->depth);
					}else if(skipChar(vg,'6')){							
						vg->encoding = FORMAT_WIN_1256;
						windows_1256_chars_init();							
						_writeVTD(vg,
							vg->temp_offset,
							6,
							TOKEN_DEC_ATTR_VAL,
							vg->depth);

					}else if(skipChar(vg,'7')){						
						vg->encoding = FORMAT_WIN_1257;
						windows_1257_chars_init();								
						_writeVTD(vg,
							vg->temp_offset,
							6,
							TOKEN_DEC_ATTR_VAL,
							vg->depth);												
					}else if(skipChar(vg,'8')){						
						vg->encoding = FORMAT_WIN_1258;
						windows_1258_chars_init();							
						_writeVTD(vg,
							vg->temp_offset,
							6,
							TOKEN_DEC_ATTR_VAL,
							vg->depth);												
					}
					else
						throwInvalidEncodingException ();
					if (skipChar(vg,vg->ch_temp))
						return;
			}
			else{
				throwException(parse_exception,0,
							"Parse exception in parse()",
							"XML decl error: Can't switch encoding to ISO-8859");
			}
	}
	throwInvalidEncodingException ();
}

static void matchWindowsEncoding(VTDGen *vg){

	if ((skipChar(vg,'i')
		|| skipChar(vg,'I'))
		&&(skipChar(vg,'n')
		|| skipChar(vg,'N'))
		&&(skipChar(vg,'d')
		|| skipChar(vg,'D'))
		&&(skipChar(vg,'o')
		|| skipChar(vg,'O'))
		&&(skipChar(vg,'w')
		|| skipChar(vg,'W'))
		&&(skipChar(vg,'s')
		|| skipChar(vg,'S'))
		&& skipChar(vg,'-')
		&& skipChar(vg,'1')
		&& skipChar(vg,'2')
		&& skipChar(vg,'5')) {
			if (vg->encoding != FORMAT_UTF_16LE
				&& vg->encoding	!= FORMAT_UTF_16BE) {
					if (vg->must_utf_8){
						throwException(parse_exception,0,
							"Parse exception in parse()",
							"XML decl error: Can't switch from UTF-8");
					}
					if(skipChar(vg,'0')){						
						vg->encoding = FORMAT_WIN_1250;
						windows_1250_chars_init();
						_writeVTD(vg,
							vg->temp_offset,
							12,
							TOKEN_DEC_ATTR_VAL,
							vg->depth);
					}else if(skipChar(vg,'1')){						
						vg->encoding = FORMAT_WIN_1251;
						windows_1251_chars_init();
						_writeVTD(vg,
							vg->temp_offset,
							12,
							TOKEN_DEC_ATTR_VAL,
							vg->depth);
					}else if(skipChar(vg,'2')){						
						vg->encoding = FORMAT_WIN_1252;
						windows_1252_chars_init();
						_writeVTD(vg,
							vg->temp_offset,
							12,
							TOKEN_DEC_ATTR_VAL,
							vg->depth);
					}else if(skipChar(vg,'3')){						
						vg->encoding = FORMAT_WIN_1253;
						windows_1253_chars_init();
						_writeVTD(vg,
							vg->temp_offset,
							12,
							TOKEN_DEC_ATTR_VAL,
							vg->depth);
					}else if(skipChar(vg,'4')){						
						vg->encoding = FORMAT_WIN_1250;
						windows_1250_chars_init();
						_writeVTD(vg,
							vg->temp_offset,
							12,
							TOKEN_DEC_ATTR_VAL,
							vg->depth);
					}else if(skipChar(vg,'5')){						
						vg->encoding = FORMAT_WIN_1255;
						windows_1255_chars_init();
						_writeVTD(vg,
							vg->temp_offset,
							12,
							TOKEN_DEC_ATTR_VAL,
							vg->depth);
					}else if(skipChar(vg,'6')){							
						vg->encoding = FORMAT_WIN_1256;
						windows_1256_chars_init();							
						_writeVTD(vg,
							vg->temp_offset,
							12,
							TOKEN_DEC_ATTR_VAL,
							vg->depth);

					}else if(skipChar(vg,'7')){						
						vg->encoding = FORMAT_WIN_1257;
						windows_1257_chars_init();								
						_writeVTD(vg,
							vg->temp_offset,
							12,
							TOKEN_DEC_ATTR_VAL,
							vg->depth);												
					}else if(skipChar(vg,'8')){						
						vg->encoding = FORMAT_WIN_1258;
						windows_1258_chars_init();							
						_writeVTD(vg,
							vg->temp_offset,
							12,
							TOKEN_DEC_ATTR_VAL,
							vg->depth);												
					}
					else
						throwInvalidEncodingException ();
					if (skipChar(vg,vg->ch_temp))
						return;
			}
			else{
				throwException(parse_exception,0,
							"Parse exception in parse()",
							"XML decl error: Can't switch encoding to ISO-8859");
			}

	}
	throwInvalidEncodingException ();

}

static void matchUTFEncoding(VTDGen *vg){
	if ((skipChar(vg,'s')
		|| skipChar(vg,'S'))){
			if (skipChar(vg,'-')
				&& (skipChar(vg,'a')
				|| skipChar(vg,'A'))
				&& (skipChar(vg,'s')
				|| skipChar(vg,'S'))
				&& (skipChar(vg,'c')
				|| skipChar(vg,'C'))
				&& (skipChar(vg,'i')
				|| skipChar(vg,'I'))
				&& (skipChar(vg,'i')
				|| skipChar(vg,'I'))
				&& skipChar(vg,vg->ch_temp)) {
					if (vg->encoding
						!= FORMAT_UTF_16LE
						&& vg->encoding
						!= FORMAT_UTF_16BE) {
							if (vg->must_utf_8){
								throwException(parse_exception,0,
									"Parse exception in parse()",
									"Can't switch from UTF-8");
							}
							vg->encoding = FORMAT_ASCII;							
							if (vg->encoding
								< FORMAT_UTF_16BE){
									writeVTD(vg,
										vg->temp_offset,
										5,
										TOKEN_DEC_ATTR_VAL,
										vg->depth);
							}
							else{
								writeVTD(vg,
									vg->temp_offset
									>> 1,
									5,
									TOKEN_DEC_ATTR_VAL,
									vg->depth);
							}
							return;

					} else
					{		
						throwException(parse_exception,0,
							"Parse exception in parse()",
							"XML decl error: Can't switch encoding to US-ASCII");
					}
			} else{		
				throwException(parse_exception,0,
							"Parse exception in parse()",
							"XML decl error: Invalid Encoding");
			}
	}
	if ((skipChar(vg,'t')
		|| skipChar(vg,'T'))
		&& (skipChar(vg,'f')
		|| skipChar(vg,'F'))
		&& skipChar(vg,'-')) {
			if (skipChar(vg,'8')
				&& skipChar(vg,vg->ch_temp)) {
					if (vg->encoding
						!= FORMAT_UTF_16LE
						&& vg->encoding
						!= FORMAT_UTF_16BE) {
							if (vg->encoding
								< FORMAT_UTF_16BE)
							{
								writeVTD(vg,
									vg->temp_offset,
									5,
									TOKEN_DEC_ATTR_VAL,
									vg->depth);
							}
							else
							{
								writeVTD(vg,
									vg->temp_offset
									>> 1,
									5,
									TOKEN_DEC_ATTR_VAL,
									vg->depth);
							}
							return;
					} else
					{		
						throwException(parse_exception,0,
							"Parse exception in parse()",
							"XML decl error: Can't switch encoding to UTF-8");
					}					
			}
			if (skipChar(vg,'1')
				&& skipChar(vg,'6')) {
					if (skipChar(vg,vg->ch_temp)) {
						if (vg->encoding
							== FORMAT_UTF_16LE
							|| vg->encoding
							== FORMAT_UTF_16BE) {
								if (vg->BOM_detected == FALSE){
									throwException(parse_exception,0,
										"Parse exception in parse()",
										"BOM not detected for UTF-16");
								}
								if (vg->encoding
									< FORMAT_UTF_16BE)
								{
									writeVTD(vg,
										vg->temp_offset,
										6,
										TOKEN_DEC_ATTR_VAL,
										vg->depth);
								}
								else{
									writeVTD(vg,
										vg->temp_offset
										>> 1,
										6,
										TOKEN_DEC_ATTR_VAL,
										vg->depth);
								}
								return;
						}

					throwException(parse_exception,0,
							"Parse exception in parse()",
							"XML decl error: Can't switch encoding to UTF-16");
					} else if (
						(skipChar(vg,'l')
						|| skipChar(vg,'L'))
						&& (skipChar(vg,'e')
						|| skipChar(vg,'E'))
						&& skipChar(vg,vg->ch_temp)) {
							if (vg->encoding
								== FORMAT_UTF_16LE) {									
									if (vg->encoding
										< FORMAT_UTF_16BE)
									{
										writeVTD(vg,
											vg->temp_offset,
											7,
											TOKEN_DEC_ATTR_VAL,
											vg->depth);
									}
									else
									{
										writeVTD(vg,
											vg->temp_offset
											>> 1,
											7,
											TOKEN_DEC_ATTR_VAL,
											vg->depth);
									}
									return;
							}
							throwException(parse_exception,0,
								"Parse exception in parse()",
								"XML decl error: Can't switch encoding to UTF-16LE");					
					} else if (
						(skipChar(vg,'b')
						|| skipChar(vg,'B'))
						&& (skipChar(vg,'e')
						|| skipChar(vg,'E'))
						&& skipChar(vg,vg->ch_temp)) {
							if (vg->encoding
								== FORMAT_UTF_16BE) {
									if (vg->encoding
										< FORMAT_UTF_16BE)
									{
										writeVTD(vg,
											vg->temp_offset,
											7,
											TOKEN_DEC_ATTR_VAL,
											vg->depth);
									}
									else{
										writeVTD(vg,
											vg->temp_offset
											>> 1,
											7,
											TOKEN_DEC_ATTR_VAL,
											vg->depth);
									}
									return;
							}
							throwException(parse_exception,0,
								"Parse exception in parse()",
								"XML decl error: Can't switch encoding to UTF-16BE");				
					}
					throwException(parse_exception,0,
							"Parse exception in parse()",
							"XML decl error: Invalid encoding");
			}
	}
}

static Boolean skipUTF8(VTDGen *vg,int temp, int ch){
	int val, a,c,d,i;
	switch (UTF8Char_byteCount(temp)) { 
			case 2 :
				c = 0x1f;
				d = 6; // 
				a = 1; //
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
			default :
				throwException(parse_exception,0,
							"Parse exception in parse()",
							"UTF 8 encoding error: should never happen");
	}
	val = (temp & c) << d;
	i = a - 1;
	while (i >= 0) {
		temp = vg->XMLDoc[vg->offset + a - i];
		if ((temp & 0xc0) != 0x80){
			throwException(parse_exception,0,			
				"Parse exception in parse()",
				"UTF 8 encoding error: should never happen");
		}
		val = val | ((temp & 0x3f) << ((i<<2)+(i<<1)));
		i--;
	}

	if (val == ch) {
		vg->offset += a + 1;
		return TRUE;
	} else {
		return FALSE;
	}
}
static Boolean skip_16be(VTDGen *vg, int ch){
	int temp,val;
	temp = vg->XMLDoc[vg->offset] << 8 | vg->XMLDoc[vg->offset + 1];
	if ((temp < 0xd800)
		|| (temp >= 0xdc00)) { // not a high surrogate
			//offset += 2;
			if (temp == ch) {
				vg->offset += 2;
				return TRUE;
			} else
				return FALSE;
	} else {
		val = temp;
		temp = vg->XMLDoc[vg->offset + 2] << 8 | vg->XMLDoc[vg->offset + 3];
		if (temp < 0xdc00 || temp > 0xdfff) {
			// has to be a low surrogate here{
			throwException(parse_exception,0,
							"Parse exception in parse()",
							"UTF 16 BE encoding error: should never happen");
		}
		val = ((val - 0xd800) << 10) + (temp - 0xdc00) + 0x10000;

		if (val == ch) {
			vg->offset += 4;
			return TRUE;
		} else
			return FALSE;
	}
}
static Boolean skip_16le(VTDGen *vg, int ch){
	int temp,val;
	temp = vg->XMLDoc[vg->offset + 1] << 8 | vg->XMLDoc[vg->offset];
	if (temp < 0xdc00 || temp > 0xdfff) { // check for low surrogate
		if (temp == ch) {
			vg->offset += 2;
			return TRUE;
		} else {
			return FALSE;
		}
	} else {
		val = temp;
		temp = vg->XMLDoc[vg->offset + 3] << 8 | vg->XMLDoc[vg->offset + 2];
		if (temp < 0xd800 || temp > 0xdc00) {
			// has to be high surrogate
			// has to be a low surrogate here{
			throwException(parse_exception,0,			
				"Parse exception in parse()",
				"UTF 16 LE encoding error: should never happen");
		}
		val = ((temp - 0xd800)<<10) + (val - 0xdc00) + 0x10000;

		if (val == ch) {
			vg->offset += 4;
			return TRUE;
		} else
			return FALSE;
	}
}

// The entity aware version of getCharAfterS
//static int getCharAfterSe(VTDGen *vg){
//	int n = 0;
//	int temp; //offset saver
//	while (TRUE) {
//		n = getChar(vg);
//		if (!XMLChar_isSpaceChar(n)) {
//			if (n != '&')
//				return n;
//			else {
//				temp = vg->offset;
//				if (!XMLChar_isSpaceChar(entityIdentifier(vg))) {
//					vg->offset = temp; // rewind
//					return '&';
//				}
//			}
//		}
//	}
//}

// The entity ignorant version of getCharAfterS
static int getCharAfterS(VTDGen *vg){
	int n, k;
	n = k = 0;
	 do {
		n = getChar(vg);
		if ((n == ' ' || n == '\n' || n =='\t'|| n == '\r'  )) {
		} else
			return n;
		n = getChar(vg);
		if ((n == ' ' || n == '\n' || n =='\t'|| n == '\r'  )) {
		} else
			return n;
	}while (TRUE);
}

// Returns the VTDNav object after parsing, it also cleans 
// internal state so VTDGen can process the next file.
VTDNav *getNav(VTDGen *vg){
	VTDNav *vn; 
	if (vg->shallowDepth)
	    vn = createVTDNav( vg->rootIndex,
		vg->encoding,
		vg->ns,
		vg->VTDDepth,
		vg->XMLDoc,
		vg->bufLen,
		vg->VTDBuffer,
		vg->l1Buffer,
		vg->l2Buffer,
		vg->l3Buffer,
		vg->docOffset,
		vg->docLen,
		vg->br);
	else
		vn= (VTDNav *)createVTDNav_L5(vg->rootIndex,
		vg->encoding,
		vg->ns,
		vg->VTDDepth,
		vg->XMLDoc,
		vg->bufLen,
		vg->VTDBuffer,
		vg->l1Buffer,
		vg->l2Buffer,
		vg->_l3Buffer,
		vg->_l4Buffer,
		vg->_l5Buffer,
		vg->docOffset,
		vg->docLen,
		vg->br);

	/*vg->l1Buffer  = vg->l2Buffer=vg->VTDBuffer = NULL;
	vg->l3Buffer = NULL;*/
	vg->stateTransfered = TRUE;
	vn->master = TRUE;
	clear(vg);
	return vn;

}

// Get the offset value of previous character.
static int getPrevOffset(VTDGen *vg){
	int prevOffset = vg->offset;
	switch (vg->encoding) {
			case FORMAT_UTF8 :
				do {
					prevOffset--;
				} while (vg->XMLDoc[prevOffset] >= 128 && 
					((vg->XMLDoc[prevOffset] & 0xc0) == 0x80));
				return prevOffset;
			case FORMAT_ASCII :
			case FORMAT_ISO_8859_1 :
			case FORMAT_ISO_8859_2:
			case FORMAT_ISO_8859_3:
			case FORMAT_ISO_8859_4:
			case FORMAT_ISO_8859_5:
			case FORMAT_ISO_8859_6:
			case FORMAT_ISO_8859_7:
			case FORMAT_ISO_8859_8:
			case FORMAT_ISO_8859_9:
			case FORMAT_ISO_8859_10:
			case FORMAT_WIN_1250:
			case FORMAT_WIN_1251:
			case FORMAT_WIN_1252:
			case FORMAT_WIN_1253:
			case FORMAT_WIN_1254:
			case FORMAT_WIN_1255:
			case FORMAT_WIN_1256:
			case FORMAT_WIN_1257:
			case FORMAT_WIN_1258:

				return vg->offset - 1;
			case FORMAT_UTF_16LE :
				if (vg->XMLDoc[vg->offset - 2] < 0xDC00
					|| vg->XMLDoc[vg->offset - 2] > 0xDFFFF) {
						return vg->offset - 2;
				} else
					return vg->offset - 4;
			case FORMAT_UTF_16BE :
				if (vg->XMLDoc[vg->offset - 1] < 0xDC00
					|| vg->XMLDoc[vg->offset - 1] > 0xDFFFF) {
						return vg->offset - 2;
				} else
					return vg->offset - 4;
			default :
				throwException(parse_exception,0,
							"Parse exception in parse()",
							"Other Error: Should never happen");	
				return 0;
	}
}

// Generating VTD tokens and Location cache info.
// One specifies whether the parsing is namespace aware or not.
void parse(VTDGen *vg, Boolean ns){

	/* define internal variables*/
	exception e;
	
	
	/*int prev_ch = 0, prev2_ch = 0; */
	//int j;
	volatile parseState parser_state = STATE_DOC_START;
	/*boolean has_amp = false; */ 
	
	//Boolean unique;
	//Boolean unequal;
	Boolean helper = FALSE;
	Boolean default_ns = FALSE;
	Boolean isXML = FALSE;
	/*Boolean BOM_detected = FALSE;*/
	/*Boolean must_utf_8 = FALSE; */
	Long x;
	/*char char_temp; //holds the ' or " indicating start of attr val */
	int sos = 0, sl = 0;
	vg->length1 = vg->length2 = 0;
	XMLChar_init();
	vg->ns = ns;
	vg->is_ns = FALSE;
	vg->encoding = FORMAT_UTF8;
	vg->attr_count =vg->prefixed_attr_count=0; /*, ch = 0, ch_temp = 0*/
	vg->singleByteEncoding= TRUE;

	/* first check first 2 bytes BOM to determine if encoding is UTF16*/
	decide_encoding(vg);

	/* enter the main finite state machine */
	Try {
		_writeVTD(vg,0,0,TOKEN_DOCUMENT,vg->depth);
		while (TRUE) {
			switch (parser_state) {

					case STATE_LT_SEEN : 
						vg->temp_offset = vg->offset;
						vg->ch = getChar(vg);
						if (XMLChar_isNameStartChar(vg->ch)) {
							vg->depth++;
							parser_state = STATE_START_TAG;
						} else {
							switch (vg->ch) {
					case '/' :
						parser_state = STATE_END_TAG;
						break;
					case '?' :
						parser_state = process_qm_seen(vg);
						break;
					case '!' : 
						parser_state = process_ex_seen(vg);
						break;
					default :throwException(parse_exception,0,
												"Parse exception in parse()",
												"Other Error: Invalid char after <");
							}
						}
						break;

					case STATE_START_TAG : 
						do{
							vg->ch = getChar(vg);
							if (XMLChar_isNameChar(vg->ch)) {
								if (vg->ch == ':') {
									vg->length2 = vg->offset - vg->temp_offset - vg->increment;
									if (vg->ns && checkPrefix2(vg,vg->temp_offset,vg->length2))
										throwException2(parse_exception,
												"xmlns can't be an element prefix ");
												//+ formatLineNumber(offset));
								}
							} else
								break;
						}while (TRUE);
						vg->length1 = vg->offset - vg->temp_offset - vg->increment;
						x = ((Long) vg->length1 << 32) + vg->temp_offset;
						vg->tag_stack[vg->depth] = x;
						if (vg->depth > MAX_DEPTH) {
							throwException(parse_exception,0,
												"Parse exception in parse()",
												"Other Error: Depth exceeds MAX_DEPTH");
						}
						if (vg->depth > vg->VTDDepth)
							vg->VTDDepth = vg->depth;
						if (vg->singleByteEncoding){
							if (vg->length2>MAX_PREFIX_LENGTH 
								|| vg->length1 > MAX_QNAME_LENGTH){
									throwException(parse_exception,0,
												"Parse exception in parse()",
												"Token Length Error: Starting tag prefix or qname length too long");
							}
							if (vg->shallowDepth)
								writeVTD(vg,
								(vg->temp_offset),
								(vg->length2 << 11) | vg->length1,
								TOKEN_STARTING_TAG,
								vg->depth);
							else
								writeVTD_L5(vg,
								(vg->temp_offset),
								(vg->length2 << 11) | vg->length1,
								TOKEN_STARTING_TAG,
								vg->depth);

						}
						else{
							if ((vg->length2>(MAX_PREFIX_LENGTH<<1)) 
								|| (vg->length1 > (MAX_QNAME_LENGTH <<1))){
									throwException(parse_exception,0,
												"Parse exception in parse()",
												"Token Length Error: Starting tag prefix or qname length too long");
							}
							if (vg->shallowDepth)
								writeVTD(vg,
								(vg->temp_offset) >> 1,
								(vg->length2 << 10) | (vg->length1 >> 1),
								TOKEN_STARTING_TAG,
								vg->depth);
							else
								writeVTD_L5(vg,
								(vg->temp_offset) >> 1,
								(vg->length2 << 10) | (vg->length1 >> 1),
								TOKEN_STARTING_TAG,
								vg->depth);
						}

						if (vg->ns) {
							if (vg->length2!=0){
								vg->length2 += vg->increment;
								vg->currentElementRecord = (((Long)((vg->length2<<16)|vg->length1))<<32) 
								| vg->temp_offset;
							} else
								vg->currentElementRecord = 0;
						
							if (vg->depth <= vg->nsBuffer1->size - 1) {
								int t;
								resizeFIB(vg->nsBuffer1,vg->depth);
								t= intAt(vg->nsBuffer1,vg->depth-1)+1;
								resizeFLB(vg->nsBuffer2,t);
								resizeFLB(vg->nsBuffer3,t);
							}
						}
						vg->length2 = 0;
						if (XMLChar_isSpaceChar(vg->ch)) {
							vg->ch = getCharAfterS(vg);
							if (XMLChar_isNameStartChar(vg->ch)) {
								vg->temp_offset = getPrevOffset(vg);
								parser_state = STATE_ATTR_NAME;
								break;
							}
						}
						helper = TRUE;
						if (vg->ch == '/') {
							vg->depth--;
							helper = FALSE;
							vg->ch = getChar(vg);
						}
						if (vg->ch == '>') {
							if (vg->ns){
								appendInt(vg->nsBuffer1,vg->nsBuffer3->size-1);
								if (vg->currentElementRecord !=0)
									qualifyElement(vg);
							}
							if (vg->depth != -1) {
								vg->temp_offset = vg->offset;
								//vg->ch = getCharAfterSe(vg); // consume WSs
								vg->ch = getCharAfterS(vg);
								if (vg->ch == '<') {
									if (vg->ws) 
										addWhiteSpaceRecord(vg);
									parser_state = STATE_LT_SEEN;
									if (skipChar(vg,'/')) {
										if (helper){
											vg->length1 =
												vg->offset
												- vg->temp_offset
												- (vg->increment<<1);

											if (vg->singleByteEncoding)
												writeVTDText(vg,
												(vg->temp_offset),
												vg->length1,
												TOKEN_CHARACTER_DATA,
												vg->depth);
											else
												writeVTDText(vg,
												(vg->temp_offset) >> 1,
												(vg->length1 >> 1),
												TOKEN_CHARACTER_DATA,
												vg->depth);

										}
										//offset += vg->length1;
										parser_state = STATE_END_TAG;
										break;
									}
								} else if (XMLChar_isContentChar(vg->ch)) {
								
									parser_state = STATE_TEXT;
								}else {
									parser_state = STATE_TEXT;
									handleOtherTextChar2(vg,vg->ch);
								} 
							} else {
								parser_state = STATE_DOC_END;
							}
							break;
						}

						throwException(parse_exception,0,
												"Parse exception in parse()",
												"Starting tag Error: Invalid char in starting tag");

					case STATE_END_TAG :
						vg->temp_offset = vg->offset;

						sos = (int) vg->tag_stack[vg->depth];
						sl = (int) (vg->tag_stack[vg->depth] >> 32);
						vg->offset = vg->temp_offset + sl;
						if (vg->offset >= vg->endOffset){
							throwException(parse_exception,0,
												"Parse exception in parse()",
												"Premature EOF reached, XML document incomplete");
						}

						if (memcmp(vg->XMLDoc+vg->temp_offset, vg->XMLDoc+sos, sl)!=0){
							throwException(parse_exception,0,
												"Parse exception in parse()",
												"Ending tag error: Start/ending tag mismatch");
						}

						vg->depth--;
						vg->ch = getCharAfterS(vg);

						if (vg->ch!='>'){
							throwException(parse_exception,0,
												"Parse exception in parse()",
												"Ending tag error: Invalid char in end tag");

						}


						if (vg->depth != -1) {
							vg->temp_offset = vg->offset;
							vg->ch = getCharAfterS(vg);
							if (vg->ch == '<'){
								if (vg->ws) 
							    	addWhiteSpaceRecord(vg);
								parser_state = STATE_LT_SEEN;
							}
							else if (XMLChar_isContentChar(vg->ch)) {
								parser_state = STATE_TEXT;
							} else {
								handleOtherTextChar2(vg, vg->ch);
								parser_state = STATE_TEXT;
							}
						} else
							parser_state = STATE_DOC_END;
						break;
						
					case STATE_ATTR_NAME :

						if (vg->ch == 'x') {
							if (skipChar(vg,'m')
								&& skipChar(vg,'l')
								&& skipChar(vg,'n')
								&& skipChar(vg,'s')) {
									vg->ch = getChar(vg);
									if (vg->ch == '='
										|| XMLChar_isSpaceChar(vg->ch)) {
									vg->is_ns = TRUE;
									default_ns = TRUE;
								}else if( vg->ch == ':') {
									vg->is_ns = TRUE; //break;
									default_ns = FALSE;
								}
							}
						}
						 do{
							if (XMLChar_isNameChar(vg->ch)) {
								if (vg->ch == ':') {
									vg->length2 = vg->offset - vg->temp_offset - vg->increment;
								}
								vg->ch = getChar(vg);
							} else
								break;
						}while (TRUE);
						vg->length1 = getPrevOffset(vg) - vg->temp_offset;
						if (vg->is_ns && vg->ns){
							// make sure postfix isn't xmlns
							if (!default_ns){
								if (vg->increment==1 && (vg->length1-vg->length2-1 == 5)
										|| (vg->increment==2 && (vg->length1-vg->length2-2==10)))
									disallow_xmlns(vg,vg->temp_offset+vg->length2+vg->increment);
							    
							// if the post fix is xml, signal it
							    if (vg->increment==1 && (vg->length1-vg->length2-1 == 3)
										|| (vg->increment==2 && (vg->length1-vg->length2-2==6)))
							    	isXML = matchXML(vg,vg->temp_offset+vg->length2+vg->increment);
							}
						}
						checkAttributeUniqueness(vg);
						//unique = TRUE;
						//for (i = 0; i < vg->attr_count; i++) {
						//	int prevLen;
						//	unequal = FALSE;
						//	prevLen = (int) vg->attr_name_array[i];
						//	if (vg->length1 == prevLen) {
						//		int prevOffset =
						//			(int) (vg->attr_name_array[i] >> 32);
						//		for (j = 0; j < prevLen; j++) {
						//			if (vg->XMLDoc[prevOffset + j]
						//			!= vg->XMLDoc[vg->temp_offset + j]) {
						//				unequal = TRUE;
						//				break;
						//			}
						//		}
						//	} else
						//		unequal = TRUE;
						//	unique = unique && unequal;
						//}
						//if (!unique && vg->attr_count != 0){		
						//	throwException(parse_exception,0,
						//						"Parse exception in parse()",
						//						"Error in attr: Attr name not unique");
						//}
						//unique = TRUE;
						//if (vg->attr_count < vg->anaLen) {
						//	vg->attr_name_array[vg->attr_count] =
						//		((Long) (vg->temp_offset) << 32) + vg->length1;
						//	vg->attr_count++;
						//} else 
						//{
						//	Long* temp_array = vg->attr_name_array;
						//	vg->attr_name_array = 
						//		(Long *)malloc(sizeof(Long)*
						//		(vg->attr_count + ATTR_NAME_ARRAY_SIZE));
						//	
						//	if (vg->attr_name_array == NULL){
						//		throwException(parse_exception,0,
						//						"Parse exception in parse()",
						//						"alloc mem for attr_name_array_failed");
						//	}
						//	vg->anaLen = vg->attr_count + ATTR_NAME_ARRAY_SIZE;

						//	for (i = 0; i < vg->attr_count; i++) {
						//		vg->attr_name_array[i] = temp_array[i];
						//	}
						//	vg->attr_name_array[vg->attr_count] =
						//		((Long) (vg->temp_offset) << 32) + vg->length1;
						//	vg->attr_count++;
						//}
						
						if (vg->is_ns) {//if the prefix is xmlns: or xmlns
							if (vg->singleByteEncoding){
								if (vg->length2>MAX_PREFIX_LENGTH 
									|| vg->length1 > MAX_QNAME_LENGTH){
										throwException(parse_exception,0,
												"Parse exception in parse()",
												"Token Length Error: Attr NS prefix or qname length too long");
								}

								_writeVTD(vg,
									vg->temp_offset,
									(vg->length2 << 11) | vg->length1,
									TOKEN_ATTR_NS,
									vg->depth);

							}
							else{
								if (vg->length2>(MAX_PREFIX_LENGTH<<1) 
									|| vg->length1 >(MAX_QNAME_LENGTH<<1)){
										throwException(parse_exception,0,
												"Parse exception in parse()",
												"Token Length Error: Attr NS prefix or qname length too long");
								}

								_writeVTD(vg,
									vg->temp_offset >> 1,
									(vg->length2 << 10) | (vg->length1 >> 1),
									TOKEN_ATTR_NS,
									vg->depth);

							}
							if (vg->ns) {								
								//unprefixed xmlns are not recorded
								if (vg->length2 != 0 && !isXML) {
									//nsBuffer2.append(VTDBuffer.size() - 1);
									Long l = ((Long) ((vg->length2 << 16) | vg->length1)) << 32
										| vg->temp_offset;
									appendLong(vg->nsBuffer3,l); // byte offset and byte
									// length
								}
							}
							//vg->is_ns = FALSE;
						} else {
							if (vg->singleByteEncoding){
								if (vg->length2>MAX_PREFIX_LENGTH 
									|| vg->length1 > MAX_QNAME_LENGTH){
										throwException(parse_exception,0,
												"Parse exception in parse()",
												"Token Length Error: Attr name prefix or qname length too long");
								}
								_writeVTD(vg,
									vg->temp_offset,
									(vg->length2 << 11) | vg->length1,
									TOKEN_ATTR_NAME,
									vg->depth);

							}
							else {
								if (vg->length2> (MAX_PREFIX_LENGTH <<1)
									|| vg->length1 > (MAX_QNAME_LENGTH<<1)){
										throwException(parse_exception,0,
												"Parse exception in parse()",
												"Token Length Error: Attr name prefix or qname length too long");
								}

								_writeVTD(vg,
									vg->temp_offset >> 1,
									(vg->length2 << 10) | (vg->length1 >> 1),
									TOKEN_ATTR_NAME,
									vg->depth);

							}
							// append to nsBuffer2
							
							
						}
						vg->length2 = 0;
						if (XMLChar_isSpaceChar(vg->ch)) {
							vg->ch = getCharAfterS(vg);
						}
						if (vg->ch != '='){		
							throwException(parse_exception,0,
												"Parse exception in parse()",
												"Error in attr: invalid char");
						}
						vg->ch_temp = getCharAfterS(vg);
						if (vg->ch_temp != '"' && vg->ch_temp != '\''){		
							throwException(parse_exception,0,
												"Parse exception in parse()",
												"Error in attr: invalid char (should be ' or \" )");
						}
						vg->temp_offset = vg->offset;
						parser_state = STATE_ATTR_VAL;
						break;

					case STATE_ATTR_VAL :
						 do {
							vg->ch = getChar(vg);
							if (XMLChar_isValidChar(vg->ch) && vg->ch != '<') {
								if (vg->ch == vg->ch_temp)
									break;
								if (vg->ch == '&') {
									if (!XMLChar_isValidChar(entityIdentifier(vg))) {		
										throwException(parse_exception,0,
												"Parse exception in parse()",
												"Error in attr: Invalid XML char");
									}
								}

							} else {		
								throwException(parse_exception,0,
												"Parse exception in parse()",
												"Error in attr: Invalid XML char");
							}
						}while (TRUE);

						vg->length1 = vg->offset - vg->temp_offset - vg->increment;
						if (vg->ns && vg->is_ns){
							int t;
							if (!default_ns && vg->length1==0){
								throwException2(parse_exception," non-default ns URL can't be empty");
									//+formatLineNumber());								
							}
							//identify nsURL return 0,1,2
							t= identifyNsURL(vg,vg->temp_offset, vg->length1);
							if (isXML){//xmlns:xml
								if (t!=1)
									//URL points to "http://www.w3.org/XML/1998/namespace"
									throwException2(parse_exception,"xmlns:xml can only point to"\
									"\"http://www.w3.org/XML/1998/namespace\"" );
									//+ formatLineNumber());

							} else {
								if (!default_ns)
									appendLong(vg->nsBuffer2,((Long)vg->temp_offset<<32) | vg->length1);
								if (t!=0){		
									if (t==1)
										throwException2(parse_exception,"namespace declaration can't point to"\
										" \"http://www.w3.org/XML/1998/namespace\"" );
										//+ formatLineNumber());
									throwException2(parse_exception,"namespace declaration can't point to"\
										" \"http://www.w3.org/2000/xmlns/\"" );
										//+ formatLineNumber());	
								}
							}							
							// no ns URL points to 
							//"http://www.w3.org/2000/xmlns/"

							// no ns URL points to  
							//"http://www.w3.org/XML/1998/namespace"
						}
						
						
						if (vg->singleByteEncoding){
							if (vg->length1 > MAX_TOKEN_LENGTH){
								throwException(parse_exception,0,
												"Parse exception in parse()",
												"Token Length Error: ATTR_VAL length too long");
							}
							_writeVTD(vg,
								vg->temp_offset,
								vg->length1,
								TOKEN_ATTR_VAL,
								vg->depth);
						}
						else{
							if (vg->length1 > (MAX_TOKEN_LENGTH << 1)){
								throwException(parse_exception,0,
												"Parse exception in parse()",
												"Token Length Error: ATTR_VAL length too long");
							}
							_writeVTD(vg,
								vg->temp_offset >> 1,
								vg->length1 >> 1,
								TOKEN_ATTR_VAL,
								vg->depth);
						}
						isXML = FALSE;
						vg->is_ns = FALSE;
						vg->ch = getChar(vg);
						if (XMLChar_isSpaceChar(vg->ch)) {
							vg->ch = getCharAfterS(vg);
							if (XMLChar_isNameStartChar(vg->ch)) {
								vg->temp_offset = vg->offset - vg->increment;
								parser_state = STATE_ATTR_NAME;
								break;
							}
						}

						helper = TRUE;
						if (vg->ch == '/') {
							vg->depth--;
							helper = FALSE;
							vg->ch = getChar(vg);
						}

						if (vg->ch == '>') {
							if (vg->ns ){
								appendInt(vg->nsBuffer1,vg->nsBuffer3->size-1);
								if (vg->prefixed_attr_count>0)
									qualifyAttributes(vg);
								if (vg->prefixed_attr_count > 1)
									checkQualifiedAttributeUniqueness(vg);
								if (vg->currentElementRecord!=0)
									qualifyElement(vg);
								vg->prefixed_attr_count=0;
							}
							vg->attr_count = 0;
							if (vg->depth != -1) {
								vg->temp_offset = vg->offset;
								//vg->ch = getCharAfterSe(vg);
								vg->ch = getCharAfterS(vg);
								if (vg->ch == '<') {
									if (vg->ws) 
								    	addWhiteSpaceRecord(vg);
									parser_state = STATE_LT_SEEN;
									if (skipChar(vg,'/')) {
										if (helper){
											vg->length1 =
												vg->offset
												- vg->temp_offset
												- (vg->increment<<1);

											if (vg->singleByteEncoding)
												writeVTDText(vg,
												(vg->temp_offset),
												vg->length1,
												TOKEN_CHARACTER_DATA,
												vg->depth);
											else
												writeVTDText(vg,
												(vg->temp_offset) >> 1,
												(vg->length1 >> 1),
												TOKEN_CHARACTER_DATA,
												vg->depth);

										}
										//offset += vg->length1;
										parser_state = STATE_END_TAG;
										break;
									}
								} else if (XMLChar_isContentChar(vg->ch)) {
									parser_state = STATE_TEXT;
								}  else {
									handleOtherTextChar2(vg, vg->ch);
									parser_state = STATE_TEXT;
								}
							} else {
								parser_state = STATE_DOC_END;
							}
							break;
						}

						throwException(parse_exception,0,
												"Parse exception in parse()",
												"Starting tag Error: Invalid char in starting tag");

					case STATE_TEXT :
						if (vg->depth == -1){throwException(parse_exception,0,
												"Parse exception in parse()",
												"Error in text: Char data at the wrong place");
						}
						 do {
							vg->ch = getChar(vg);
							if (XMLChar_isContentChar(vg->ch)) {
							} else if (vg->ch == '<') {
								break;
							} else 
								handleOtherTextChar(vg,vg->ch);
							vg->ch = getChar(vg);
							if (XMLChar_isContentChar(vg->ch)) {
							} else if (vg->ch == '<') {
								break;
							} else 
								handleOtherTextChar(vg,vg->ch);
						}while (TRUE);
						vg->length1 = vg->offset - vg->increment - vg->temp_offset;						
						if (vg->singleByteEncoding){
							writeVTDText(vg,
								vg->temp_offset,
								vg->length1,
								TOKEN_CHARACTER_DATA,
								vg->depth);
						}
						else{
							writeVTDText(vg,
								vg->temp_offset >> 1,
								vg->length1 >> 1,
								TOKEN_CHARACTER_DATA,
								vg->depth);
						}
						parser_state = STATE_LT_SEEN;
						break;

					case STATE_DOC_START :
						parser_state = process_start_doc(vg);
						break;

					case STATE_DOC_END :
						parser_state = process_end_doc(vg);
						break;

					case STATE_PI_TAG :
						parser_state = process_pi_tag(vg);
						break;

					case STATE_PI_VAL :
						parser_state = process_pi_val(vg);
						break;

					case STATE_DEC_ATTR_NAME :
						parser_state = process_dec_attr(vg);
						break;

					case STATE_COMMENT :
						parser_state = process_comment(vg);
						break;

					case STATE_CDATA :
						parser_state = process_cdata(vg);
						break;

					case STATE_DOCTYPE :
						parser_state = process_doc_type(vg);
						break;

					case STATE_END_PI :										  

						parser_state = process_end_pi(vg);
						break;

					case STATE_END_COMMENT :
						parser_state = process_end_comment(vg);
						break;

					default :	
						throwException(parse_exception,0,
												"Parse exception in parse()",
												"Other error: invalid parser state");
			}
		}
	} 
	Catch (e) {
		if (parser_state != STATE_DOC_END 
			|| e.subtype == -1){
				printf("%s: %s\n",e.msg, e.sub_msg);
				printLineNumber(vg);
				//printf("\n Last Offset val ===> %d \n",vg->offset);
				Throw e;
		}
		finishUp(vg);
	}


}

// parse a file directly (from a given FILE *)
// be careful that you must manually obtain the byte * to free the XML document
Boolean parseFile(VTDGen *vg, Boolean ns,char *fileName){
	FILE *f = NULL;
	exception e;
	int status,len;
	UByte *ba=NULL;
	struct stat buffer;
	f = fopen(fileName,"rb");
	if (f==NULL){
		//throwException2(invalid_argument,"fileName not valid");
		return FALSE;
	}
	status = stat(fileName,&buffer);
	if (status !=0){
		fclose(f);
		//throwException2(parse_exception,"error occurred in parseFile");
		return FALSE;
	}
	len = buffer.st_size;
	ba = malloc(len);
	if (ba == NULL){
		fclose(f);
		//throwException2(out_of_mem,"error occurred in parseFile");
		return FALSE;
	}
	if (fread(ba,1,len,f)!=len){
		fclose(f);
		return FALSE;
	}
	setDoc(vg,ba,len);
	Try{
		parse(vg,ns);
	}Catch(e){
		free(ba);
		clear(vg);
		fclose(f);
		printf("%s\n",e.msg);
		printf("%s\n",e.sub_msg);
		//throwException2(out_of_mem,"error occurred in parseFile");
		return FALSE;
	}
	
	fclose(f);
	return TRUE;
}

// set the XML Doc container and turn on buffer reuse
void setDoc_BR(VTDGen *vg, UByte *ba, int len){
	setDoc_BR2(vg,ba,len,0,len);
}

//Set the XMLDoc container.Also set the offset and len of the document 
void setDoc_BR2(VTDGen *vg, UByte *ba, int len, int os, int docLen){
	int a,i1=8,i2=9,i3=11;
	vg->br = TRUE;
	vg->depth = -1;
	vg->increment = 1;
	vg->BOM_detected = FALSE;
	vg->must_utf_8 = FALSE;
	vg->ch = vg->ch_temp = 0;
	vg->temp_offset = 0;
	vg->XMLDoc = ba;
	vg->docOffset = vg->offset = os;
	vg->docLen = docLen;
	vg->bufLen = len;
	vg->endOffset = os + docLen;
	vg->last_depth = vg->last_l3_index = vg->last_l2_index = vg->last_l1_index;
	vg->currentElementRecord = 0;
	clearFastIntBuffer(vg->nsBuffer1);
	clearFastLongBuffer(vg->nsBuffer2);
	clearFastLongBuffer(vg->nsBuffer3);
	if (vg->shallowDepth){
		if (vg->VTDBuffer == NULL){
			if (vg->docLen <= 1024) {
				a = 6; i1=5; i2=5;i3=5;
			} else if (vg->docLen <= 4096 * 2){
				a = 7; i1=6; i2=6; i3=6;
			}
			else if (vg->docLen <=1024*16){
				a =8; i1 = 7;i2=7;i3=7;
			}
			else if (vg->docLen <= 1024 * 16 * 4) {
				a = 10;
			} else if (vg->docLen <= 1024 * 256) {
				a = 12;
			} else {
				a = 15;
			}
			vg->VTDBuffer = createFastLongBuffer3(a, len>>(a+1)); 
			vg->l1Buffer = createFastLongBuffer2(i1); 
			vg->l2Buffer = createFastLongBuffer2(i2);
			vg->l3Buffer = createFastIntBuffer2(i3); 
		}
		else {
			vg->VTDBuffer->size = 0;
			vg->l1Buffer->size = 0;
			vg->l2Buffer->size = 0;
			vg->l3Buffer->size = 0;
		}
	}else{
		int i1 = 8, i2 = 9, i3 = 11, i4 = 11, i5 = 11;
		if (docLen <= 1024) {
			// a = 1024; //set the floor
			a = 6;
			i1 = 5;
			i2 = 5;
			i3 = 5;
			i4 = 5;
			i5 = 5;
		} else if (docLen <= 4096) {
			a = 7;
			i1 = 6;
			i2 = 6;
			i3 = 6;
			i4 = 6;
			i5 = 6;
		} else if (docLen <= 1024 * 16) {
			a = 8;
			i1 = 7;
			i2 = 7;
			i3 = 7;
		} else if (docLen <= 1024 * 16 * 4) {
			// a = 2048;
			a = 11;
			i2 = 8;
			i3 = 8;
			i4 = 8;
			i5 = 8;
		} else if (docLen <= 1024 * 256) {
			// a = 1024 * 4;
			a = 12;
			i1 = 8;
			i2 = 9;
			i3 = 9;
			i4 = 9;
			i5 = 9;
		} else if (docLen <= 1024 * 1024) {
			// a = 1024 * 4;
			a = 12;
			i1 = 8;
			i3 = 10;
			i4 = 10;
			i5 = 10;
		} else {
			// a = 1 << 15;
			a = 15;
			i1 = 8;
		}
		if (vg->VTDBuffer == NULL){
			vg->VTDBuffer = createFastLongBuffer3(a, len>>(a+1)); 
			vg->l1Buffer = createFastLongBuffer2(i1); 
			vg->l2Buffer = createFastLongBuffer2(i2);
			vg->_l3Buffer = createFastLongBuffer2(i3); 
			vg->_l4Buffer = createFastLongBuffer2(i4); 
			vg->_l5Buffer = createFastIntBuffer2(i5); 
		}else{
			vg->VTDBuffer->size = 0;
			vg->l1Buffer->size = 0;
			vg->l2Buffer->size = 0;
			vg->_l3Buffer->size = 0;
			vg->_l4Buffer->size = 0;
			vg->_l5Buffer->size = 0;
		}

	}
	vg->stateTransfered = FALSE;
}

// Set the XMLDoc container.
void setDoc(VTDGen *vg, UByte *ba, int len){
	setDoc2(vg, ba, len, 0, len);
}

/* Set the XMLDoc container.Also set the offset and len of the document
   len is the size of the byte buffer
   docLen is the length of the XML content in byte */
void setDoc2(VTDGen *vg, UByte *ba, int len, int os, int docLen){
	int a,i1=8,i2=9,i3=11;
	vg->br = FALSE;
	vg->depth = -1;
	vg->increment = 1;
	vg->BOM_detected = FALSE;
	vg->must_utf_8 = FALSE;
	vg->ch = vg->ch_temp = 0;
	vg->temp_offset = 0;
	vg->XMLDoc = ba;
	vg->docOffset = vg->offset = os;
	vg->docLen = docLen;
	vg->bufLen =len;
	vg->endOffset = os + docLen;
	vg->last_depth = vg->last_l3_index = vg->last_l2_index = vg->last_l1_index;
	vg->currentElementRecord = 0;
	clearFastIntBuffer(vg->nsBuffer1);
	clearFastLongBuffer(vg->nsBuffer2);
	clearFastLongBuffer(vg->nsBuffer3);
	if (vg->shallowDepth){
		if (vg->docLen <= 1024) {		
			a = 6; i1=5; i2=5;i3=5;
		} else if (vg->docLen <= 4096 * 2){
			a = 7; i1=6; i2=6; i3=6;
		}else if (vg->docLen <=1024*16){
			a =8; i1 = 7;i2=7;i3=7;
		}
		else if (vg->docLen <= 1024 * 16 * 4) {
			a = 11; i2= 8; i3=8;
		} else if (vg->docLen <= 1024 * 256) {		
			a = 12;
		} else {	
			a = 15;
		}

		if (vg->stateTransfered == FALSE && vg->VTDBuffer != NULL){
			freeFastLongBuffer(vg->VTDBuffer);
			freeFastLongBuffer(vg->l1Buffer);
			freeFastLongBuffer(vg->l2Buffer);
			freeFastIntBuffer(vg->l3Buffer);		
		}
		vg->VTDBuffer = createFastLongBuffer3(a, len>>(a+1)); 
		vg->l1Buffer = createFastLongBuffer2(i1);
		vg->l2Buffer = createFastLongBuffer2(i2); 
		vg->l3Buffer = createFastIntBuffer2(i3);
	}else{
		int i1 = 7, i2 = 9, i3 = 11, i4 = 11, i5 = 11;
			if (vg->docLen <= 1024) {
				// a = 1024; //set the floor
				a = 6;
				i1 = 5;
				i2 = 5;
				i3 = 5;
				i4 = 5;
				i5 = 5;
			} else if (vg->docLen <= 4096) {
				a = 7;
				i1 = 6;
				i2 = 6;
				i3 = 6;
				i4 = 6;
				i5 = 6;
			} else if (vg->docLen <= 1024 * 16) {
				a = 8;
				i1 = 7;
				i2 = 7;
				i3 = 7;
			} else if (vg->docLen <= 1024 * 16 * 4) {
				// a = 2048;
				a = 11;
				i2 = 8;
				i3 = 8;
				i4 = 8;
				i5 = 8;
			} else if (vg->docLen <= 1024 * 256) {
				// a = 1024 * 4;
				a = 12;
				i1 = 8;
				i2 = 9;
				i3 = 9;
				i4 = 9;
				i5 = 9;
			} else if (vg->docLen <= 1024 * 1024) {
				// a = 1024 * 4;
				a = 12;
				i1 = 8;
				i3 = 10;
				i4 = 10;
				i5 = 10;
			} else {
				// a = 1 << 15;
				a = 15;
				i1 = 8;
			}
			if (vg->stateTransfered == FALSE && vg->VTDBuffer != NULL){
			freeFastLongBuffer(vg->VTDBuffer);
			freeFastLongBuffer(vg->l1Buffer);
			freeFastLongBuffer(vg->l2Buffer);
			freeFastLongBuffer(vg->_l3Buffer);
			freeFastLongBuffer(vg->_l4Buffer);
			freeFastIntBuffer(vg->_l5Buffer);
		}
		vg->VTDBuffer = createFastLongBuffer3(a, len>>(a+1)); 
		vg->l1Buffer = createFastLongBuffer2(i1);
		vg->l2Buffer = createFastLongBuffer2(i2); 
		vg->_l3Buffer = createFastLongBuffer2(i3);
		vg->_l4Buffer = createFastLongBuffer2(i4);
		vg->_l5Buffer = createFastIntBuffer2(i5);

	}

	vg->stateTransfered = FALSE;
}

/* Increments offset only when the next char matches a given value. */
static Boolean skipChar(VTDGen *vg, int ch){
	int temp = 0;
	if (vg->offset >= vg->endOffset){
		throwException(parse_exception,0,
			"Parse exception in parse()",
			"Premature EOF reached, XML document incomplete");
	}	
	switch (vg->encoding) {
			case FORMAT_ASCII :
				temp = vg->XMLDoc[vg->offset];
				if (temp>127){
					throwException(parse_exception,0,
						"Parse exception in parse()",
						"Invalid char for ASCII encoding");
				}
				if (ch == temp) {
					vg->offset++;
					return TRUE;
				} else {
					return FALSE;
				}
			case FORMAT_ISO_8859_1 :
				temp = vg->XMLDoc[vg->offset];
				if (temp == ch) {
					vg->offset++;
					return TRUE;
				} else {
					return FALSE;
				}
			case FORMAT_UTF8 :
				temp = vg->XMLDoc[vg->offset];
				if (temp <128) {
					if (ch == temp) {
						vg->offset++;
						return TRUE;
					} else {
						return FALSE;
					}
				}
				return skipUTF8(vg,temp,ch);

			case FORMAT_UTF_16BE :
				return skip_16be(vg,ch);

			case FORMAT_UTF_16LE :
				return skip_16le(vg,ch);

			default :
				return skip4OtherEncoding(vg,ch);
	}
}

/* Write the VTD and lc into their storage container.
   needs to take care byte endian
   the current implementation only swap bytes for vtd for small endians
   LCs are not swapped, so when they got persisted, byte swap may be needed*/

static void writeVTD(VTDGen *vg, int offset, int length, tokenType token_type, int depth){
	appendLong(vg->VTDBuffer,((Long) ((token_type << 28)
		| ((depth & 0xff) << 20) | length) << 32)
		| offset);

	// remember VTD depth start from zero

	switch (depth) {
				case 0 :
					//rootIndex = VTDBuffer.size() - 1;
					vg->rootIndex = vg->VTDBuffer->size - 1;
					break;
				case 1 :
					if (vg->last_depth == 1) {

						appendLong(vg->l1Buffer,
							((Long) vg->last_l1_index << 32) | 0xffffffffLL);

					} else if (vg->last_depth == 2) {

						appendLong(vg->l2Buffer,
							((Long) vg->last_l2_index << 32) | 0xffffffffLL);

					}
					vg->last_l1_index = vg->VTDBuffer->size - 1;
					vg->last_depth = 1;
					break;
				case 2 :
					if (vg->last_depth == 1) {
						appendLong(vg->l1Buffer,
							((Long) vg->last_l1_index << 32) + vg->l2Buffer->size);

					} else if (vg->last_depth == 2) {
						appendLong(vg->l2Buffer,
							((Long) vg->last_l2_index << 32) | 0xffffffffLL);

					}
					vg->last_l2_index = vg->VTDBuffer->size - 1;
					vg->last_depth = 2;
					break;

				case 3 :
					appendInt(vg->l3Buffer, vg->VTDBuffer->size - 1);
					if (vg->last_depth == 2) {
						appendLong(vg->l2Buffer,
							((Long)vg->last_l2_index << 32) + vg->l3Buffer->size - 1);
					}

					vg->last_depth = 3;
					break;
				default :					
					break;
	}
}


/* finishing up */
void finishUp(VTDGen *vg){
	if (vg->shallowDepth){
		if (vg->last_depth == 1) {
			appendLong(vg->l1Buffer,((Long) vg->last_l1_index << 32) | 0xffffffffLL);
		} else if (vg->last_depth == 2) {
			appendLong(vg->l2Buffer,((Long) vg->last_l2_index << 32) | 0xffffffffLL);
		}
	}else{
		if (vg->last_depth == 1) {
			appendLong(vg->l1Buffer,((Long) vg->last_l1_index << 32) | 0xffffffffLL);
		} else if (vg->last_depth == 2) {
			appendLong(vg->l2Buffer,((Long) vg->last_l2_index << 32) | 0xffffffffLL);
		}else if (vg->last_depth == 3) {
			appendLong(vg->_l3Buffer,((Long) vg->last_l3_index << 32) | 0xffffffffLL);
		}else if (vg->last_depth == 4) {
			appendLong(vg->_l4Buffer,((Long) vg->last_l4_index << 32) | 0xffffffffLL);
		}
	}
}

void decide_encoding(VTDGen *vg){
	if (vg->docLen==0)
	        throwException2(parse_exception,"Document is zero sized ");
	if (vg->XMLDoc[vg->offset] == (UByte) -2) {
		vg->increment = 2;
		if (vg->XMLDoc[vg->offset + 1] == (UByte)-1) {
			vg->offset += 2;
			vg->encoding = FORMAT_UTF_16BE;
			vg->BOM_detected = TRUE;
		} else{
			throwException(parse_exception,0,	
				"Parse exception in parse()",
				"Unknown Character encoding: should be 0xff 0xfe");
		}
	} else if (vg->XMLDoc[vg->offset] == (UByte)-1) {
		vg->increment = 2;
		if (vg->XMLDoc[vg->offset + 1] == (UByte)-2) {
			vg->offset += 2;
			vg->encoding = FORMAT_UTF_16LE;
			vg->BOM_detected = TRUE;
		} else{
			throwException(parse_exception,0,	
				"Parse exception in parse()",
				"Unknown Character encoding");
		}
	} else if (vg->XMLDoc[vg->offset] == (UByte)-17) {
		if (vg->XMLDoc[vg->offset+1] == (UByte)-69 && vg->XMLDoc[vg->offset+2]==(UByte)-65){
			vg->offset +=3;
			vg->must_utf_8= TRUE;
		}
		else {
			throwException(parse_exception,0,	
				"Parse exception in parse()",
				"Unknown Character encoding: not UTF-8");
		}

	} else if (vg->XMLDoc[vg->offset] == 0) {
		if (vg->XMLDoc[vg->offset+1] == 0x3c 
			&& vg->XMLDoc[vg->offset+2] == 0 
			&& vg->XMLDoc[vg->offset+3] == 0x3f){
				vg->encoding = FORMAT_UTF_16BE;
				vg->increment = 2;
		}
		else{
			throwException(parse_exception,0,	
				"Parse exception in parse()",
				"Unknown Character encoding: not UTF-16BE");
		}
	} else if (vg->XMLDoc[vg->offset] == 0x3c) {
		if (vg->XMLDoc[vg->offset+1] == 0 
			&& vg->XMLDoc[vg->offset+2] == 0x3f 
			&& vg->XMLDoc[vg->offset+3] == 0x0){
				vg->increment = 2;
				vg->encoding = FORMAT_UTF_16LE;
		}
	}

	if (vg->encoding < FORMAT_UTF_16BE) {
		if (vg->ns == TRUE) {
			if (((Long) vg->offset + vg->docLen) >= (1U << 30)){
				throwException(parse_exception,0,	
					"Parse exception in parse()",
					"Other error: file size too large >=1G ");
			}
		}else {
			if (((Long) vg->offset + vg->docLen) >= (1U << 31)){
				throwException(parse_exception,0,	
					"Parse exception in parse()",
					"Other error: file size too large >=2G ");
			}
		}
	} else {
		if ((unsigned int)(vg->offset - 2 + vg->docLen) >= (1U<< 31)){
			throwException(parse_exception,0,	
				"Parse exception in parse()",
				"Other error: file size too large");
		}
	}
	if (vg->encoding >= FORMAT_UTF_16BE){
		vg->singleByteEncoding = FALSE;
	}
}
int process_end_pi(VTDGen *vg){
	int parser_state=0;
	vg->ch = getChar(vg);
	if (XMLChar_isNameStartChar(vg->ch)) {
		if ((vg->ch == 'x' || vg->ch == 'X')
			&& (skipChar(vg,'m') || skipChar(vg,'M'))
			&& (skipChar(vg,'l') && skipChar(vg,'L'))) {
				//vg->temp_offset = offset;
				vg->ch = getChar(vg);
				if (XMLChar_isSpaceChar(vg->ch) || vg->ch == '?'){		
					throwException(parse_exception,0,	
						"Parse exception in parse()",
						"Error in PI: [xX][mM][lL] not a valid PI target");
				}
		}

		while (TRUE) {
			//vg->ch = getChar(vg);
			if (!XMLChar_isNameChar(vg->ch)) {
				break;
			}
			vg->ch = getChar(vg);
		}

		vg->length1 = vg->offset - vg->temp_offset - vg->increment;
		if (vg->encoding < FORMAT_UTF_16BE){
			if (vg->length1 > MAX_TOKEN_LENGTH){
				throwException(parse_exception,0,	
					"Parse exception in parse()",
					"Token Length Error: PI_NAME length too long");
			}
			writeVTD(vg,
				vg->temp_offset,
				vg->length1,
				TOKEN_PI_NAME,
				vg->depth);
		}
		else{
			if (vg->length1 > (MAX_TOKEN_LENGTH<<1)){
				throwException(parse_exception,0,	
					"Parse exception in parse()",
					"Token Length Error: PI_NAME length too long");
			}
			writeVTD(vg,
				vg->temp_offset >> 1,
				vg->length1 >> 1,
				TOKEN_PI_NAME,
				vg->depth);
		}
		//vg->length1 = 0;
		vg->temp_offset = vg->offset;
		if (XMLChar_isSpaceChar(vg->ch)) {
			vg->ch = getCharAfterS(vg);

			while (TRUE) {
				if (XMLChar_isValidChar(vg->ch)) {
					if (vg->ch == '?') {
						if (skipChar(vg,'>')) {
							parser_state = STATE_DOC_END;
							break;
						} else{		
							throwException(parse_exception,0,	
								"Parse exception in parse()",
								"Error in PI: invalid termination sequence");
						}
					}
				} else{		
					throwException(parse_exception,0,	
						"Parse exception in parse()",
						"Error in PI: Invalid char in PI val");
				}
				vg->ch = getChar(vg);
			}
			vg->length1 = vg->offset - vg->temp_offset - (vg->increment<<1);
			if (vg->encoding < FORMAT_UTF_16BE){
				if (vg->length1 > MAX_TOKEN_LENGTH){
					throwException(parse_exception,0,	
						"Parse exception in parse()",
						"Token Length Error: PI_VAL length too long");
				}
				writeVTD(vg,
					vg->temp_offset,
					vg->length1,
					TOKEN_PI_VAL,
					vg->depth);
			}
			else{
				if (vg->length1 > (MAX_TOKEN_LENGTH << 1)){
					throwException(parse_exception,0,	
						"Parse exception in parse()",
						"Token Length Error: PI_VAL length too long");
				}
				writeVTD(vg,
					vg->temp_offset >> 1,
					vg->length1 >> 1,
					TOKEN_PI_VAL,
					vg->depth);
			}
		} else {
			if (vg->singleByteEncoding){
				_writeVTD(vg,
					(vg->temp_offset),
					0,
					TOKEN_PI_NAME,
					vg->depth);
			}
			else{
				_writeVTD(vg,
					(vg->temp_offset) >> 1,
					0,
					TOKEN_PI_NAME,
					vg->depth);
			}
			if ((vg->ch == '?') && skipChar(vg,'>')) {
				parser_state = STATE_DOC_END;
			} else{		
				throwException(parse_exception,0,	
					"Parse exception in parse()",
					"Error in PI: invalid termination sequence");
			}
		}
	} else{		
		throwException(parse_exception,0,	
			"Parse exception in parse()",
			"Error in PI: invalid char in PI target");
	}
	return parser_state;
}
int process_end_comment(VTDGen *vg){
	int parser_state=0;
	while (TRUE) {
		vg->ch = getChar(vg);
		if (XMLChar_isValidChar(vg->ch)) {
			if (vg->ch == '-' && skipChar(vg,'-')) {
				vg->length1 =
					vg->offset - vg->temp_offset - (vg->increment<<1);
				break;
			}
		} else{		
			throwException(parse_exception,0,	
				"Parse exception in parse()",
				"Error in comment: Invalid Char");
		}
		/*throw new ParseException(
		"Error in comment: Invalid Char"
		+ formatLineNumber());*/
	}
	if (getChar(vg) == '>') {
		//System.out.println(" " + vg->temp_offset + " " + vg->length1 + " comment " + vg->depth);
		if (vg->singleByteEncoding){
			writeVTDText(vg,
				vg->temp_offset,
				vg->length1,
				TOKEN_COMMENT,
				vg->depth);
		}
		else
		{
			writeVTDText(vg,
				vg->temp_offset >> 1,
				vg->length1 >> 1,
				TOKEN_COMMENT,
				vg->depth);
		}
		//vg->length1 = 0;
		parser_state = STATE_DOC_END;
		return parser_state;
	}		
	throwException(parse_exception,0,	
		"Parse exception in parse()",
		"Error in comment: '-->' expected");
	return 0;

}
int process_comment(VTDGen *vg){
	int parser_state=0;
	while (TRUE) {
		vg->ch = getChar(vg);
		if (XMLChar_isValidChar(vg->ch)) {
			if (vg->ch == '-' && skipChar(vg,'-')) {
				vg->length1 =
					vg->offset - vg->temp_offset - (vg->increment<<1);
				break;
			}
		} else{		
			throwException(parse_exception,0,	
				"Parse exception in parse()",
				"Error in comment: Invalid Char");
		}
	}
	if (getChar(vg) == '>') {
		if (vg->singleByteEncoding){
			writeVTDText(vg,
				vg->temp_offset,
				vg->length1,
				TOKEN_COMMENT,
				vg->depth);
		}
		else{
			writeVTDText(vg,
				vg->temp_offset >> 1,
				vg->length1 >> 1,
				TOKEN_COMMENT,
				vg->depth);
		}
		vg->temp_offset = vg->offset;
		//vg->ch = getCharAfterSe(vg);
		vg->ch = getCharAfterS(vg);
		if (vg->ch == '<') {
			if (vg->ws)
				addWhiteSpaceRecord(vg);
			parser_state = STATE_LT_SEEN;
		} else if (XMLChar_isContentChar(vg->ch)) {
			parser_state = STATE_TEXT;
		} else if (vg->ch == '&') {
			entityIdentifier(vg);
			parser_state = STATE_TEXT;
		} else if (vg->ch == ']') {
			if (skipChar(vg,']')) {
				while (skipChar(vg,']')) {
				}
				if (skipChar(vg,'>')){		
					throwException(parse_exception,0,	
						"Parse exception in parse()",
						"Error in text content: ]]> in text content");
				}
			}
			parser_state = STATE_TEXT;
		}
		else{		
			throwException(parse_exception,0,	
				"Parse exception in parse()",
				"XML decl error");
		}
		return parser_state;
	} else{		
		throwException(parse_exception,0,	
			"Parse exception in parse()",
			"Error in comment: Invalid terminating sequence");
		return 0;
	}
}
int process_doc_type(VTDGen *vg){
	int parser_state=0;
	int	z = 1;

	while (TRUE) {
		vg->ch = getChar(vg);
		if (XMLChar_isValidChar(vg->ch)) {
			if (vg->ch == '>')
				z--;
			else if (vg->ch == '<')
				z++;
			if (z == 0)
				break;
		} else{		
			throwException(parse_exception,0,	
				"Parse exception in parse()",
				"Error in DOCTYPE: Invalid char");
		}
	}
	vg->length1 = vg->offset - vg->temp_offset - vg->increment;
	if (vg->singleByteEncoding){
		if (vg->length1 > MAX_TOKEN_LENGTH){
			throwException(parse_exception,0,	
				"Parse exception in parse()",
				"Token Length Error: DTD_VAL length too long");
		}
		_writeVTD(vg,
			vg->temp_offset,
			vg->length1,
			TOKEN_DTD_VAL,
			vg->depth);
	}
	else{
		if (vg->length1 > (MAX_TOKEN_LENGTH<<1)){
			throwException(parse_exception,0,	
				"Parse exception in parse()",
				"Token Length Error: DTD_VAL length too long");
		}
		_writeVTD(vg,
			vg->temp_offset >> 1,
			vg->length1 >> 1,
			TOKEN_DTD_VAL,
			vg->depth);
	}
	vg->ch = getCharAfterS(vg);
	if (vg->ch == '<') {
		parser_state = STATE_LT_SEEN;
	} else{		
		throwException(parse_exception,0,	
			"Parse exception in parse()",
			"Other Error: Invalid char in xml");
	}
	return parser_state;
}

static int process_cdata(VTDGen *vg){
	int parser_state=0;
	while (TRUE) {
		vg->ch = getChar(vg);
		if (XMLChar_isValidChar(vg->ch)) {
			if (vg->ch == ']' && skipChar(vg,']')) {
				while (skipChar(vg,']'));
				if (skipChar(vg,'>')) {
					break;
				} 
			}
		} else{		
			throwException(parse_exception,0,	
				"Parse exception in parse()",
				"Error in CDATA: Invalid Char");
		}
	}
	vg->length1 = vg->offset - vg->temp_offset - vg->increment - (vg->increment<<1);
	if (vg->singleByteEncoding){
		writeVTDText(vg,
			vg->temp_offset,
			vg->length1,
			TOKEN_CDATA_VAL,
			vg->depth);
	}
	else{
		writeVTDText(vg,
			vg->temp_offset >> 1,
			vg->length1 >> 1,
			TOKEN_CDATA_VAL,
			vg->depth);
	}
	vg->temp_offset = vg->offset;
	//vg->ch = getCharAfterSe(vg);
	vg->ch = getCharAfterS(vg);
	if (vg->ch == '<') {
		if (vg->ws) 
		    addWhiteSpaceRecord(vg);
		parser_state = STATE_LT_SEEN;
	} else if (XMLChar_isContentChar(vg->ch)) {
		//vg->temp_offset = vg->offset-1;
		parser_state = STATE_TEXT;
	} else if (vg->ch == '&') {
		//vg->temp_offset = vg->offset-1;
		entityIdentifier(vg);
		parser_state = STATE_TEXT;
	} else if (vg->ch == ']') {
		//vg->temp_offset = vg->offset-1;
		if (skipChar(vg,']')) {
			while (skipChar(vg,']')) {
			}
			if (skipChar(vg,'>')){		
				throwException(parse_exception,0,	
					"Parse exception in parse()",
					"Error in text content: ]]> in text content");
			}
		}
		parser_state = STATE_TEXT;
	}
	else{		
		throwException(parse_exception,0,	
			"Parse exception in parse()",
			"Other Error: Invalid char in xml");
	}
	return parser_state;
}
static int process_pi_val(VTDGen *vg){
	int parser_state;
	
	if (!XMLChar_isSpaceChar(vg->ch)){		
		throwException(parse_exception,0,	
			"Parse exception in parse()",
			"Other Error: Invalid char in xml");
	}
	vg->temp_offset = vg->offset;
	vg->ch = getChar(vg);
	while (TRUE) {
		if (XMLChar_isValidChar(vg->ch)) {
			if (vg->ch == '?'){
				if (skipChar(vg,'>')) {
					break;
				} /*else{		
					throwException(parse_exception,0,	
						"Parse exception in parse()",
						"Error in PI: invalid termination sequence for PI");
				}*/
			}
		} else{		
			throwException(parse_exception,0,	
				"Parse exception in parse()",
				"Errors in PI: Invalid char in PI val");
		}
		vg->ch = getChar(vg);
	}
	vg->length1 = vg->offset - vg->temp_offset - (vg->increment<<1);

	if (vg->singleByteEncoding){
		if (vg->length1 > MAX_TOKEN_LENGTH){
			throwException(parse_exception,0,	
				"Parse exception in parse()",
				"Token Length Error: PI_VAL length too long");
		}
		_writeVTD(vg,vg->temp_offset, vg->length1, TOKEN_PI_VAL, vg->depth);
	}
	else{
		if (vg->length1 > (MAX_TOKEN_LENGTH << 1)){
			throwException(parse_exception,0,	
				"Parse exception in parse()",
				"Token Length Error: PI_VAL length too long");
		}
		_writeVTD(vg,
			vg->temp_offset >> 1,
			vg->length1 >> 1,
			TOKEN_PI_VAL,
			vg->depth);
	}

	vg->temp_offset = vg->offset;
	//vg->ch = getCharAfterSe(vg);
	vg->ch = getCharAfterS(vg);
	if (vg->ch == '<') {
		if (vg->ws) 
		    addWhiteSpaceRecord(vg);
		parser_state = STATE_LT_SEEN;
	} else if (XMLChar_isContentChar(vg->ch)) {
		parser_state = STATE_TEXT;
	} else if (vg->ch == '&') {
		entityIdentifier(vg);
		parser_state = STATE_TEXT;
	} else if (vg->ch == ']') {
		if (skipChar(vg,']')) {
			while (skipChar(vg,']')) {
			}
			if (skipChar(vg,'>')){		
				throwException(parse_exception,0,	
					"Parse exception in parse()",
					"Error in text content: ]]> in text content");
			}
		}
		parser_state = STATE_TEXT;
	}
	else{		
		throwException(parse_exception,0,	
			"Parse exception in parse()",
			"Error in text content: Invalid char");
	}
	return parser_state;
}
int process_pi_tag(VTDGen *vg){
	int parser_state=0;
	while (TRUE) {
		vg->ch = getChar(vg);
		if (!XMLChar_isNameChar(vg->ch))
			break;
	}

	vg->length1 = vg->offset - vg->temp_offset - vg->increment;
	if (vg->singleByteEncoding){
		if (vg->length1 > MAX_TOKEN_LENGTH){
			throwException(parse_exception,0,	
				"Parse exception in parse()",
				"Token Length Error: PI_TAG length too long");
		}
		_writeVTD(vg,
			(vg->temp_offset),
			vg->length1,
			TOKEN_PI_NAME,
			vg->depth);
	}
	else{													
		if (vg->length1 > (MAX_TOKEN_LENGTH << 1)){
			throwException(parse_exception,0,	
				"Parse exception in parse()",
				"Token Length Error: PI_TAG length too long");
		}
		_writeVTD(vg,
			(vg->temp_offset) >> 1,
			(vg->length1 >> 1),
			TOKEN_PI_NAME,
			vg->depth);
	}
	//vg->temp_offset = vg->offset;
	//if (XMLChar_isSpaceChar(vg->ch)) {
	//	vg->ch = getChar(vg);
	//}
	if (vg->ch == '?') {
		if (vg->singleByteEncoding){
			_writeVTD(vg,
				(vg->temp_offset),
				0,
				TOKEN_PI_VAL,
				vg->depth);
		}
		else{
			_writeVTD(vg,
				(vg->temp_offset) >> 1,
				0,
				TOKEN_PI_VAL,
				vg->depth);
		}
		if (skipChar(vg,'>')) {
			vg->temp_offset = vg->offset;
			//vg->ch = getCharAfterSe(vg);
			vg->ch = getCharAfterS(vg);
			if (vg->ch == '<') {
				if (vg->ws) 
				    addWhiteSpaceRecord(vg);
				parser_state = STATE_LT_SEEN;
			} else if (XMLChar_isContentChar(vg->ch)) {
				parser_state = STATE_TEXT;
			} else if (vg->ch == '&') {
				//has_amp = true;
				entityIdentifier(vg);
				parser_state = STATE_TEXT;
			} else if (vg->ch == ']') {
				if (skipChar(vg,']')) {
					while (skipChar(vg,']')) {
					}
					if (skipChar(vg,'>')){		
						throwException(parse_exception,0,
							"Parse exception in parse()",
							"Error in text content: ]]> in text content");
					}
				}
				parser_state = STATE_TEXT;
			}else
			{		
				throwException(parse_exception,0,
					"Parse exception in parse()",
					"Error in text content: Invalid char");
			}
			return parser_state;
		} else
		{		
			throwException(parse_exception,0,	
				"Parse exception in parse()",
				"Error in PI: invalid termination sequence");
		}
	}
	parser_state = STATE_PI_VAL;
	return parser_state;
}
static int process_dec_attr(VTDGen *vg){
	int parser_state=0;
	if (vg->ch == 'v'
		&& skipChar(vg,'e')
		&& skipChar(vg,'r')
		&& skipChar(vg,'s')
		&& skipChar(vg,'i')
		&& skipChar(vg,'o')
		&& skipChar(vg,'n')) {
			vg->ch = getCharAfterS(vg);
			if (vg->ch == '=') {
				if (vg->singleByteEncoding){
					_writeVTD(vg,
						vg->temp_offset - 1,
						7,
						TOKEN_DEC_ATTR_NAME,
						vg->depth);
				}
				else{
					_writeVTD(vg,
						(vg->temp_offset - 2) >> 1,
						7,
						TOKEN_DEC_ATTR_NAME,
						vg->depth);
				}
			} 
			else
			{		
				throwException(parse_exception,0,	
					"Parse exception in parse()",
					"XML decl error: Invalid char");
			}
	} else{		
		throwException(parse_exception,0,
			"Parse exception in parse()",
			"XML decl error: should be version");
	}
	vg->ch_temp = getCharAfterS(vg);
	if (vg->ch_temp != '\'' && vg->ch_temp != '"')
	{		
		throwException(parse_exception,0,
			"Parse exception in parse()",
			"XML decl error: Invalid char to start attr name");
	}
	vg->temp_offset = vg->offset;
	/* support 1.0 or 1.1*/
	if (skipChar(vg,'1')
		&& skipChar(vg,'.')
		&& (skipChar(vg,'0') || skipChar(vg,'1'))) {
			if (vg->singleByteEncoding){
				_writeVTD(vg,
					vg->temp_offset,
					3,
					TOKEN_DEC_ATTR_VAL,
					vg->depth);
			}
			else{
				_writeVTD(vg,
					vg->temp_offset >> 1,
					3,
					TOKEN_DEC_ATTR_VAL,
					vg->depth);
			}
	} else
	{		
		throwException(parse_exception,0,	
			"Parse exception in parse()",
			"XML decl error: Invalid version(other than 1.0 or 1.1) detected");
	}
	if (!skipChar(vg,vg->ch_temp))
	{		
		throwException(parse_exception,0,		
			"Parse exception in parse()",
			"XML decl error: version not terminated properly");
	}
	vg->ch = getChar(vg);
	//? space or e 
	if (XMLChar_isSpaceChar(vg->ch)) {
		vg->ch = getCharAfterS(vg);
		vg->temp_offset = vg->offset - vg->increment;
		if (vg->ch == 'e') {
			if (skipChar(vg,'n')
				&& skipChar(vg,'c')
				&& skipChar(vg,'o')
				&& skipChar(vg,'d')
				&& skipChar(vg,'i')
				&& skipChar(vg,'n')
				&& skipChar(vg,'g')) {
					vg->ch = getChar(vg);
					if (XMLChar_isSpaceChar(vg->ch))
						vg->ch = getCharAfterS(vg);
					if (vg->ch == '=') {
						if (vg->singleByteEncoding){
							_writeVTD(vg,
								vg->temp_offset,
								8,
								TOKEN_DEC_ATTR_NAME,
								vg->depth);
						}
						else{
							_writeVTD(vg,
								vg->temp_offset >> 1,
								8,
								TOKEN_DEC_ATTR_NAME,
								vg->depth);
						}
					} else{		
						throwException(parse_exception,0,	
								"Parse exception in parse()",
								"XML decl error: Invalid char");
					}
					vg->ch_temp = getCharAfterS(vg);
					if (vg->ch_temp != '"' && vg->ch_temp != '\'')
					{		
						throwException(parse_exception,0,	
								"Parse exception in parse()",
								"XML decl error: Invalid char to start attr name");
					}
					vg->temp_offset = vg->offset;
					vg->ch = getChar(vg);
					switch (vg->ch) {
					case 'a' :
					case 'A' :{
						if ((skipChar(vg,'s')
							|| skipChar(vg,'S'))
							&& (skipChar(vg,'c')
							|| skipChar(vg,'C'))
							&& (skipChar(vg,'i')
							|| skipChar(vg,'I'))
							&& (skipChar(vg,'i')
							|| skipChar(vg,'I'))
							&& skipChar(vg,vg->ch_temp)) {
								if (vg->encoding != FORMAT_UTF_16LE
									&& vg->encoding
									!= FORMAT_UTF_16BE) {
										if (vg->must_utf_8){
											throwException(parse_exception,0,	
												"Parse exception in parse()",
												"Can't switch from UTF-8");
										}
										vg->encoding = FORMAT_ASCII;


										_writeVTD(vg,
											vg->temp_offset >> 1,
											5,
											TOKEN_DEC_ATTR_VAL,
											vg->depth);
										
										break;
								} else
								{		
									throwException(parse_exception,0,	
										"Parse exception in parse()",
										"XML decl error: Can't switch encoding to ASCII");
								}
								
						}

						throwException(parse_exception,0,	
								"Parse exception in parse()",
								"XML decl error: Invalid Encoding");
							  }
							
					case 'c':
					case 'C': 
						matchCPEncoding(vg);
						break;
					case 'i' :
					case 'I' :
						matchISOEncoding(vg);
						break;
					case 'u' :
					case 'U' :
						matchUTFEncoding(vg);
						break;
					case 'w':
					case 'W':
						matchWindowsEncoding(vg);
						break;
					default :
						throwInvalidEncodingException();
						break;						
					}
					vg->ch = getChar(vg);
					if (XMLChar_isSpaceChar(vg->ch))
						vg->ch = getCharAfterS(vg);
					vg->temp_offset = vg->offset - vg->increment;
			} else{		
				throwException(parse_exception,0,	
								"Parse exception in parse()",
								"XML decl Error: Invalid char");
			}
		}

		if (vg->ch == 's') {
			if (skipChar(vg,'t')
				&& skipChar(vg,'a')
				&& skipChar(vg,'n')
				&& skipChar(vg,'d')
				&& skipChar(vg,'a')
				&& skipChar(vg,'l')
				&& skipChar(vg,'o')
				&& skipChar(vg,'n')
				&& skipChar(vg,'e')) {

					vg->ch = getCharAfterS(vg);
					if (vg->ch != '='){		
						throwException(parse_exception,0,	
								"Parse exception in parse()",
								"XML decl error: Invalid char");
					}
					if (vg->singleByteEncoding){
						_writeVTD(vg,
							vg->temp_offset,
							10,
							TOKEN_DEC_ATTR_NAME,
							vg->depth);
					}
					else{
						_writeVTD(vg,
							vg->temp_offset >> 1,
							10,
							TOKEN_DEC_ATTR_NAME,
							vg->depth);
					}
					vg->ch_temp = getCharAfterS(vg);
					vg->temp_offset = vg->offset;
					if (vg->ch_temp != '"' && vg->ch_temp != '\''){		
						throwException(parse_exception,0,	
								"Parse exception in parse()",
								"XML decl error: Invalid char to start attr name");
					}
					vg->ch = getChar(vg);
					if (vg->ch == 'y') {
						if (skipChar(vg,'e')
							&& skipChar(vg,'s')
							&& skipChar(vg,vg->ch_temp)) {
								if (vg->singleByteEncoding)
									_writeVTD(vg,
									vg->temp_offset,
									3,
									TOKEN_DEC_ATTR_VAL,
									vg->depth);
								else
									_writeVTD(vg,
									vg->temp_offset >> 1,
									3,
									TOKEN_DEC_ATTR_VAL,
									vg->depth);
						} else{		
							throwException(parse_exception,0,	
								"Parse exception in parse()",
								"XML decl error: invalid val for standalone");
						}
					} else if (vg->ch == 'n') {
						if (skipChar(vg,'o')
							&& skipChar(vg,vg->ch_temp)) {
								if (vg->singleByteEncoding)
									_writeVTD(vg,
									vg->temp_offset,
									2,
									TOKEN_DEC_ATTR_VAL,
									vg->depth);
								else
									_writeVTD(vg,
									vg->temp_offset >> 1,
									2,
									TOKEN_DEC_ATTR_VAL,
									vg->depth);
						} else{		
							throwException(parse_exception,0,	
								"Parse exception in parse()",
								"XML decl error: invalid val for standalone");
						}
					} else{		
						throwException(parse_exception,0,	
							"Parse exception in parse()",
							"XML decl error: invalid val for standalone");
					}
			} else{		
				throwException(parse_exception,0,
					"Parse exception in parse()",
					"XML decl error");
			}
			vg->ch = getChar(vg);
			if (XMLChar_isSpaceChar(vg->ch))
				vg->ch = getCharAfterS(vg);
		}
	}

	if (vg->ch == '?' && skipChar(vg,'>')) {
		vg->temp_offset = vg->offset;
		vg->ch = getCharAfterS(vg);
		if (vg->ch == '<') {
			parser_state = STATE_LT_SEEN;
		} else{		
			throwException(parse_exception,0,
				"Parse exception in parse()",
				"Other Error: Invalid Char in XML");
		}
	} else{		
		throwException(parse_exception,0,
				"Parse exception in parse()",
				"XML decl Error: Invalid termination sequence");
	}
	return parser_state;

}


static int process_start_doc(VTDGen *vg){
	int c = getChar(vg);
	if (c == '<') {
		vg->temp_offset = vg->offset;
		/* xml decl has to be right after the start of the document*/
		if (skipChar(vg,'?')
			&& (skipChar(vg,'x') || skipChar(vg,'X'))
			&& (skipChar(vg,'m') || skipChar(vg,'M'))
			&& (skipChar(vg,'l') || skipChar(vg,'L'))) {
				if (skipChar(vg,' ')
					|| skipChar(vg,'\t')
					|| skipChar(vg,'\n')
					|| skipChar(vg,'\r')) {
						vg->ch = getCharAfterS(vg);
						vg->temp_offset = vg->offset;
						return STATE_DEC_ATTR_NAME;
				} else if (skipChar(vg,'?')){
					throwException(parse_exception,0,
						"Parse exception in parse()",
						"Error in XML decl: premature ending");
				}
		}
		vg->offset = vg->temp_offset;
		return STATE_LT_SEEN;
	}else if (c==' '||c=='\n'||c=='\r'||c=='\t'){ 
		if (getCharAfterS(vg)=='<'){ 
			return STATE_LT_SEEN;; 
		} 
	}                 


	throwException(parse_exception,0,
		"Parse exception in parse()",
		"Other Error: XML not starting properly");
	return 0;

}

static int process_end_doc(VTDGen *vg){
	vg->ch = getCharAfterS(vg);
	if (vg->ch == '<') {
		if (skipChar(vg,'?')) {
			vg->temp_offset = vg->offset;
			return STATE_END_PI;
		} else if (
			skipChar(vg,'!')
			&& skipChar(vg,'-')
			&& skipChar(vg,'-')) {
				vg->temp_offset = vg->offset;
				return STATE_END_COMMENT;
		}
	}
	/*printf("**********************\n");
	printf(" char is %i \n", vg->ch);
	printf("**********************\n");*/
	throwException(parse_exception,-1,
		"Parse exception in parse()",
		"Other Error: XML not terminated properly");
	return 0;
}

static int process_qm_seen(VTDGen *vg){
	vg->temp_offset = vg->offset;
	vg->ch = getChar(vg);
	if (XMLChar_isNameStartChar(vg->ch)) {
		if ((vg->ch == 'x' || vg->ch == 'X')
			&& (skipChar(vg,'m') || skipChar(vg,'M'))
			&& (skipChar(vg,'l') || skipChar(vg,'L'))) {
				vg->ch = getChar(vg);
				if (vg->ch == '?'
					|| XMLChar_isSpaceChar(vg->ch)){
						throwException(parse_exception,0,
							"Parse exception in parse()",
							"Error in PI: [xX][mM][lL] not a valid PI targetname");
				}
				vg->offset = getPrevOffset(vg);
		}
		return STATE_PI_TAG;
	}
	throwException(parse_exception,0,
		"Parse exception in parse()",
		"Other Error: First char after <? invalid");
	return 0;
}

static int process_ex_seen(VTDGen *vg){
	Boolean hasDTD = FALSE;
	int parser_state;
	vg->ch = getChar(vg);
	switch (vg->ch) {
					case '-' :
						if (skipChar(vg,'-')) {
							vg->temp_offset = vg->offset;
							parser_state = STATE_COMMENT;
							break;
						} else
						{		
							throwException(parse_exception,0,
								"Parse exception in parse()",
								"Error in comment: Invalid char sequence to start a comment");
						}
					case '[' :
						if (skipChar(vg,'C')
							&& skipChar(vg,'D')
							&& skipChar(vg,'A')
							&& skipChar(vg,'T')
							&& skipChar(vg,'A')
							&& skipChar(vg,'[')
							&& (vg->depth != -1)) {
								vg->temp_offset = vg->offset;
								parser_state = STATE_CDATA;
								break;
						} else {
							if (vg->depth == -1){
								throwException(parse_exception,0,
									"Parse exception in parse()",
									"Error in CDATA: Invalid char sequence for CDATA");
							}
							throwException(parse_exception,0,
								"Parse exception in parse()",
								"Error in CDATA: Invalid char sequence for CDATA");
						}

					case 'D' :
						if (skipChar(vg,'O')
							&& skipChar(vg,'C')
							&& skipChar(vg,'T')
							&& skipChar(vg,'Y')
							&& skipChar(vg,'P')
							&& skipChar(vg,'E')
							&& (vg->depth == -1)
							&& !hasDTD) {
								hasDTD = TRUE;
								vg->temp_offset = vg->offset;
								parser_state = STATE_DOCTYPE;
								break;
						} else {
							if (hasDTD == TRUE){
								throwException(parse_exception,0,
									"Parse exception in parse()",
									"Error for DOCTYPE: Only DOCTYPE allowed");
							}
							if (vg->depth != -1){
								throwException(parse_exception,0,
									"Parse exception in parse()",
									"Error for DOCTYPE: DTD at wrong place");
							}

							throwException(parse_exception,0,
								"Parse exception in parse()",
								"Error for DOCTYPE: Invalid char sequence for DOCTYPE");
						}
					default :

						throwException(parse_exception,0,
							"Parse exception in parse()",
							"Other Error: Unrecognized char after <!");
	}
	return parser_state;
}

static void addWhiteSpaceRecord(VTDGen *vg){
	if (vg->depth > -1) {
			vg->length1 = vg->offset - vg->increment - vg->temp_offset;
			if (vg->length1 != 0)
				if (vg->encoding < FORMAT_UTF_16BE)
					writeVTDText(vg,vg->temp_offset, vg->length1, 
							TOKEN_CHARACTER_DATA, vg->depth);
				else
					writeVTDText(vg, vg->temp_offset >> 1,vg->length1 >> 1,
							TOKEN_CHARACTER_DATA, vg->depth);
		}
}

/* Load VTD+XML from a FILE pointer */
VTDNav* loadIndex2(VTDGen *vg, FILE *f){
	clear(vg);
	free(vg->XMLDoc);
	if (_readIndex(f,vg))
	return getNav(vg);
	else 
		return NULL;
}

/* load VTD+XML from a byte array */
VTDNav* loadIndex3(VTDGen *vg, UByte* ba,int len){
	clear(vg);
	free(vg->XMLDoc);
	if (_readIndex2(ba,len,vg))
	return getNav(vg);
	else return NULL;
}

/* Write VTD+XML into a FILE pointer */
Boolean writeIndex(VTDGen *vg, FILE *f){
	if (vg->shallowDepth)
	 return _writeIndex_L3(1, 
                vg->encoding, 
                vg->ns, 
                TRUE, 
                vg->VTDDepth, 
                3, 
                vg->rootIndex, 
                vg->XMLDoc, 
                vg->docOffset, 
                vg->docLen, 
                vg->VTDBuffer, 
                vg->l1Buffer, 
                vg->l2Buffer, 
                vg->l3Buffer, 
                f);
	else
		 return _writeIndex_L5(1, 
                vg->encoding, 
                vg->ns, 
                TRUE, 
                vg->VTDDepth, 
                5, 
                vg->rootIndex, 
                vg->XMLDoc, 
                vg->docOffset, 
                vg->docLen, 
                vg->VTDBuffer, 
                vg->l1Buffer, 
                vg->l2Buffer, 
                vg->_l3Buffer, 
				vg->_l4Buffer,
				vg->_l5Buffer,
                f);

}
/* Write VTD+XML into a file */
Boolean writeIndex2(VTDGen *vg, char *fileName){
	FILE *f = NULL;
	Boolean b = FALSE;
	f = fopen(fileName,"wb");
	
	if (f==NULL){
		throwException2(invalid_argument,"fileName not valid");
		return FALSE;
	}
	b = writeIndex(vg,f);
	fclose(f);
	return b;
}

/* pre-calculate the integrated VTD+XML index size without generating the actual index */
Long getIndexSize(VTDGen *vg){
		int size;
	    if ( (vg->docLen & 7)==0)
	       size = vg->docLen;
	    else
	       size = ((vg->docLen >>3)+1)<<3;
	    
	    size += (vg->VTDBuffer->size<<3)+
	            (vg->l1Buffer->size<<3)+
	            (vg->l2Buffer->size<<3);
	    
		if ((vg->l3Buffer->size & 1) == 0){ //even
	        size += vg->l3Buffer->size<<2;
	    } else {
	        size += (vg->l3Buffer->size+1)<<2; //odd
	    }
	    return size+64;
}


/* Write the VTDs and LCs into a file*/
/* offset shift in xml should be zero*/
void writeSeparateIndex(VTDGen *vg, char *VTDIndexFile){
	FILE *f = NULL;
	//Boolean b = FALSE;
	f = fopen(VTDIndexFile,"wb");
	
	if (f==NULL){
		throwException2(invalid_argument,"fileName not valid");
		//return FALSE;
	}

	if (vg->shallowDepth)
		_writeSeparateIndex_L3( (Byte)2, 
                vg->encoding, 
                vg->ns, 
                TRUE, 
                vg->VTDDepth, 
                3, 
                vg->rootIndex, 
                //vg->XMLDoc, 
                vg->docOffset, 
                vg->docLen, 
                vg->VTDBuffer, 
                vg->l1Buffer, 
                vg->l2Buffer, 
                vg->l3Buffer, 
                f);
	else
		_writeSeparateIndex_L5( (Byte)2, 
                vg->encoding, 
                vg->ns, 
                TRUE, 
                vg->VTDDepth, 
                3, 
                vg->rootIndex, 
                //vg->XMLDoc, 
                vg->docOffset, 
                vg->docLen, 
                vg->VTDBuffer, 
                vg->l1Buffer, 
                vg->l2Buffer, 
                vg->_l3Buffer, 
				vg->_l4Buffer,
				vg->_l5Buffer,
                f);	
	fclose(f);
	//return b;
}

/* Load the separate VTD index and XmL file.*/
VTDNav* loadSeparateIndex(VTDGen *vg, char *XMLFile, char *VTDIndexFile){
	FILE *vf = NULL, *xf=NULL;
	Boolean b = FALSE;
	struct stat s;
	unsigned int xsize;

	vf = fopen(VTDIndexFile,"rb");
	xf = fopen(XMLFile, "rb");
	stat(XMLFile,&s);
	xsize = (unsigned int) s.st_size;

	if (xf==NULL||vf==NULL){
		throwException2(invalid_argument,"fileName not valid");
		return NULL;
	}
	// clear internal state of vg
	clear(vg);
	free(vg->XMLDoc);

	//get xml file size
	b=_readSeparateIndex(xf, xsize, vf, vg);
	if (b==TRUE){
		return getNav(vg);
	}else {
		return NULL;
	}
	//
}

/* configure the VTDGen to enable or disable (disabled by default) white space nodes */
void enableIgnoredWhiteSpace(VTDGen *vg, Boolean b){
	vg->ws = b;
}



static void qualifyAttributes(VTDGen *vg){
	int i1= vg->nsBuffer3->size-1;
		int j= 0,i=0;
		// two cases:
		// 1. the current element has no prefix, look for xmlns
		// 2. the current element has prefix, look for xmlns:something
		while(j<vg->prefixed_attr_count){		
			int preLen = (int)((vg->prefixed_attr_name_array[j] & 0xffff0000LL)>>16);
			int preOs = (int) (vg->prefixed_attr_name_array[j]>>32);
			//System.out.println(new String(XMLDoc, preOs, preLen)+"===");
			i = i1;
			while(i>=0){
				int t = upper32At(vg->nsBuffer3,i);
				// with prefix, get full length and prefix length
				if ( (t&0xffff) - (t>>16) == preLen+vg->increment){
					// doing byte comparison here
					int os =lower32At( vg->nsBuffer3,i)+(t>>16)+vg->increment;
					//System.out.println(new String(XMLDoc, os, preLen)+"");
					int k=0;
					for (;k<preLen;k++){
						//System.out.println(i+" "+(char)(XMLDoc[os+k])+"<===>"+(char)(XMLDoc[preOs+k]));
						if (vg->XMLDoc[os+k]!=vg->XMLDoc[preOs+k])
							break;
					}
					if (k==preLen){
						break; // found the match
					}
				}
				/*if ( (nsBuffer3.upper32At(i) & 0xffff0000) == 0){
					return;
				}*/
				i--;
			}
			if (i<0)
				throwException2(parse_exception,"Name space qualification Exception: prefixed attribute not qualified\n");
						//+formatLineNumber(preOs));
			else
				vg->prefix_URL_array[j] = i;
			j++;
			// no need to check if xml is the prefix
		}
}

static void qualifyElement(VTDGen *vg){
	int i= vg->nsBuffer3->size-1;
	// two cases:
	// 1. the current element has no prefix, look for xmlns
	// 2. the current element has prefix, look for xmlns:something

	int preLen = (int)((vg->currentElementRecord & 0xffff000000000000LL)>>48);
	int preOs = (int)vg->currentElementRecord;
	while(i>=0){
		int t = upper32At(vg->nsBuffer3,i);
		// with prefix, get full length and prefix length
		if ( (t&0xffff) - (t>>16) == preLen){
			// doing byte comparison here
			int os = lower32At(vg->nsBuffer3,i)+(t>>16)+vg->increment;
			int k=0;
			for (;k<preLen-vg->increment;k++){
				if (vg->XMLDoc[os+k]!=vg->XMLDoc[preOs+k])
					break;
			}
			if (k==preLen-vg->increment)
				return; // found the match
		}
		/*if ( (nsBuffer3.upper32At(i) & 0xffff0000) == 0){
		return;
		}*/
		i--;
	}
	// no need to check if xml is the prefix
	if (checkPrefix(vg,preOs, preLen))
		return;


	// print line # column# and full element name
	throwException2(parse_exception,
			"Name space qualification Exception: Element not qualified\n");
			//	+formatLineNumber((int)currentElementRecord));
}

static Boolean matchXML(VTDGen *vg, int byte_offset){
			// TODO Auto-generated method stub
		if (vg->encoding<FORMAT_UTF_16BE){
			 if (vg->XMLDoc[byte_offset]=='x'
					&& vg->XMLDoc[byte_offset+1]=='m'
					&& vg->XMLDoc[byte_offset+2]=='l')
					return TRUE;		
		}else{
			 if (vg->encoding==FORMAT_UTF_16LE){
				if (vg->XMLDoc[byte_offset]=='x' && vg->XMLDoc[byte_offset+1]==0
					&& vg->XMLDoc[byte_offset+2]=='m' && vg->XMLDoc[byte_offset+3]==0
					&& vg->XMLDoc[byte_offset+4]=='l' && vg->XMLDoc[byte_offset+5]==0)
					return TRUE;
			} else {
				if (vg->XMLDoc[byte_offset]== 0 && vg->XMLDoc[byte_offset+1]=='x'
						&& vg->XMLDoc[byte_offset+2]==0 && vg->XMLDoc[byte_offset+3]=='m'
						&& vg->XMLDoc[byte_offset+4]==0 && vg->XMLDoc[byte_offset+5]=='l')
						return TRUE;
			}
		}		
		return FALSE;
}

static Boolean matchURL(VTDGen *vg, int bos1, int len1, int bos2, int len2){
		Long l1,l2;
		int i1=bos1, i2=bos2, i3=bos1+len1,i4=bos2+len2;
		//System.out.println("--->"+new String(XMLDoc, bos1, len1)+" "+new String(XMLDoc,bos2,len2));
		while(i1<i3 && i2<i4){
			l1 = _getCharResolved(vg,i1);
			l2 = _getCharResolved(vg,i2);
			if ((int)l1!=(int)l2)
				return FALSE;
			i1 += (int)(l1>>32);
			i2 += (int)(l2>>32);
		}
		if (i1==i3 && i2==i4)
			return TRUE;
		return FALSE;
}

static int identifyNsURL(VTDGen *vg, int byte_offset, int length){
		UCSChar* URL1 = L"2000/xmlns/";
		UCSChar* URL2 = L"http://www.w3.org/XML/1998/namespace";
		Long l;
		int i,t,g=byte_offset+length;
		int os=byte_offset;
		if (length <29
				|| (vg->increment==2 && length<58) )
			return 0;
		
		for (i=0; i<18 && os<g; i++){
			l = _getCharResolved(vg,os);
			//System.out.println("char ==>"+(char)l);
			if (URL2[i]!= (int)l)
				return 0;
			os += (int)(l>>32);
		}
		
		//store offset value 
		t = os;
		
		for (i=0;i<11 && os<g;i++){
			l = _getCharResolved(vg,os);
			if (URL1[i]!= (int)l)
				break;
			os += (int)(l>>32);
		}
		if (os == g)
			return 2;
		
		//so far a match
		os = t;
		for (i=18;i<36 && os<g;i++){
			l = _getCharResolved(vg,os);
			if (URL2[i]!= (int)l)
				return 0;
			os += (int)(l>>32);
		}
		if (os==g)
			return 1;
			
		return 0;
}

static int getCharUnit(VTDGen *vg, int byte_offset){
	return (vg->encoding <= 2)
		? vg->XMLDoc[byte_offset] & 0xff
		: (vg->encoding < FORMAT_UTF_16BE)
		? decode(vg,byte_offset):(vg->encoding == FORMAT_UTF_16BE)
		? (((int)vg->XMLDoc[byte_offset])
		<< 8 | vg->XMLDoc[byte_offset+1])
		: (((int)vg->XMLDoc[byte_offset + 1])
		<< 8 | vg->XMLDoc[byte_offset]);
}

static void disallow_xmlns(VTDGen *vg, int byte_offset){
			// TODO Auto-generated method stub
		if (vg->encoding<FORMAT_UTF_16BE){
			 if (vg->XMLDoc[byte_offset]=='x'
					&& vg->XMLDoc[byte_offset+1]=='m'
					&& vg->XMLDoc[byte_offset+2]=='l'
					&& vg->XMLDoc[byte_offset+3]=='n'
					&& vg->XMLDoc[byte_offset+4]=='s')
					throwException2(parse_exception,
							"xmlns as a ns prefix can't be re-declared");
					//formatLineNumber(byte_offset));
			
		}else{
			 if (vg->encoding==FORMAT_UTF_16LE){
				if (vg->XMLDoc[byte_offset]=='x' && vg->XMLDoc[byte_offset+1]==0
					&& vg->XMLDoc[byte_offset+2]=='m' && vg->XMLDoc[byte_offset+3]==0
					&& vg->XMLDoc[byte_offset+4]=='l' && vg->XMLDoc[byte_offset+5]==0
					&& vg->XMLDoc[byte_offset+6]=='n' && vg->XMLDoc[byte_offset+7]==0
					&& vg->XMLDoc[byte_offset+8]=='s' && vg->XMLDoc[byte_offset+9]==0)
					throwException2(parse_exception,
							"xmlns as a ns prefix can't be re-declared");
			} else {
				if (vg->XMLDoc[byte_offset]== 0 && vg->XMLDoc[byte_offset+1]=='x'
						&& vg->XMLDoc[byte_offset+2]==0 && vg->XMLDoc[byte_offset+3]=='m'
						&& vg->XMLDoc[byte_offset+4]==0 && vg->XMLDoc[byte_offset+5]=='l'
						&& vg->XMLDoc[byte_offset+6]==0 && vg->XMLDoc[byte_offset+7]=='n'
						&& vg->XMLDoc[byte_offset+8]==0 && vg->XMLDoc[byte_offset+9]=='s')
						throwException2(parse_exception,"xmlns as a ns prefix can't be re-declared");
								//+formatLineNumber(offset));
			}
		}		
}

static void checkQualifiedAttributeUniqueness(VTDGen *vg){
			// TODO Auto-generated method stub
		int  preLen1,os1,postLen1,URLLen1,URLOs1, 
			 preLen2, os2,postLen2, URLLen2, URLOs2,k,i,j;
		for (i=0;i<vg->prefixed_attr_count;i++){
			preLen1 = (int)((vg->prefixed_attr_name_array[i] & 0xffff0000LL)>>16);
			postLen1 = (int) ((vg->prefixed_attr_name_array[i] & 0xffffLL))-preLen1-vg->increment;
			os1 = (int) (vg->prefixed_attr_name_array[i]>>32) + preLen1+vg->increment;
			URLLen1 = lower32At(vg->nsBuffer2,vg->prefix_URL_array[i]);
			URLOs1 =  upper32At(vg->nsBuffer2,vg->prefix_URL_array[i]);
			for (j=i+1;j<vg->prefixed_attr_count;j++){
				// prefix of i matches that of j
				preLen2 = (int)((vg->prefixed_attr_name_array[j] & 0xffff0000LL)>>16);
				postLen2 = (int) ((vg->prefixed_attr_name_array[j] & 0xffffL))-preLen2-vg->increment;
				os2 = (int)(vg->prefixed_attr_name_array[j]>>32) + preLen2 + vg->increment;
				//System.out.println(new String(XMLDoc,os1, postLen1)
				//	+" "+ new String(XMLDoc, os2, postLen2));
				if (postLen1 == postLen2){
					k=0;
					/*for (;k<postLen1;k++){
					//System.out.println(i+" "+(char)(XMLDoc[os+k])+"<===>"+(char)(XMLDoc[preOs+k]));
						if (vg->XMLDoc[os1+k]!=vg->XMLDoc[os2+k])
							break;
					}*/
					if (memcmp(vg->XMLDoc+os1, vg->XMLDoc+os2,postLen1)==0){
							// found the match
						URLLen2 = lower32At(vg->nsBuffer2,vg->prefix_URL_array[j]);
						URLOs2 =  upper32At(vg->nsBuffer2,vg->prefix_URL_array[j]);
						//System.out.println(" URLOs1 ===>" + URLOs1);
						//System.out.println("nsBuffer2 ===>"+nsBuffer2.longAt(i)+" i==>"+i);
						//System.out.println("URLLen2 "+ URLLen2+" URLLen1 "+ URLLen1+" ");
						if (matchURL(vg,URLOs1, URLLen1, URLOs2, URLLen2))
							throwException2(parse_exception," qualified attribute names collide ");
							//	+ formatLineNumber(os2));
				    }
				}				
			}
			//System.out.println("======");
		}
}

static Boolean checkPrefix(VTDGen *vg, int os, int len){
			//int i=0;
		if (vg->encoding < FORMAT_UTF_16BE){
			if (len==4	&&	vg->XMLDoc[os]=='x'
				&& vg->XMLDoc[os+1]=='m' && vg->XMLDoc[os+2]=='l'){
				return TRUE;
			}
		}else if (vg->encoding == FORMAT_UTF_16BE){
			if (len==8	&&	vg->XMLDoc[os]==0 && vg->XMLDoc[os+1]=='x'
				&& vg->XMLDoc[os+2]==0 && vg->XMLDoc[os+3]=='m' 
				&& vg->XMLDoc[os+4]==0 && vg->XMLDoc[os+5]=='l'){
				return TRUE;
			}
		}else {
			if (len==8	&&	vg->XMLDoc[os]=='x' && vg->XMLDoc[os+1]==0
				&& vg->XMLDoc[os+2]=='m' && vg->XMLDoc[os+3]==0 
				&& vg->XMLDoc[os+4]=='l' && vg->XMLDoc[os+5]==0){
				return TRUE;
			}
		}
		return FALSE;
}

static Boolean checkPrefix2(VTDGen *vg, int os, int len){
			//int i=0;
		if (vg->encoding < FORMAT_UTF_16BE){
			if ( len==5 && vg->XMLDoc[os]=='x'
				&& vg->XMLDoc[os+1]=='m' && vg->XMLDoc[os+2]=='l'
				&& vg->XMLDoc[os+3]=='n' && vg->XMLDoc[os+4]=='s'){
				return TRUE;
			}
		}else if (vg->encoding == FORMAT_UTF_16BE){
			if ( len==10 && vg->XMLDoc[os]==0 && vg->XMLDoc[os+1]=='x'
				&& vg->XMLDoc[os+2]==0 && vg->XMLDoc[os+3]=='m' 
				&& vg->XMLDoc[os+4]==0 && vg->XMLDoc[os+5]=='l'
				&& vg->XMLDoc[os+6]==0 && vg->XMLDoc[os+7]=='n' 
				&& vg->XMLDoc[os+8]==0 && vg->XMLDoc[os+9]=='s'		
			){
				return TRUE;
			}
		}else {
			if ( len==10 && vg->XMLDoc[os]=='x' && vg->XMLDoc[os+1]==0
				&& vg->XMLDoc[os+2]=='m' && vg->XMLDoc[os+3]==0 
				&& vg->XMLDoc[os+4]=='l' && vg->XMLDoc[os+5]==0
				&& vg->XMLDoc[os+6]=='n' && vg->XMLDoc[os+3]==0 
				&& vg->XMLDoc[os+8]=='s' && vg->XMLDoc[os+5]==0				
			){
				return TRUE;
			}
		}
		return FALSE;
}

static void checkAttributeUniqueness(VTDGen *vg){
	Boolean unique = TRUE;
	Boolean unequal;
	//int prevLen;
	int i;
	for (i = 0; i < vg->attr_count; i++) {
		int prevLen = (int) vg->attr_name_array[i];
		unequal = FALSE;	
		if (vg->length1 == prevLen) {
			int prevOffset =
				(int) (vg->attr_name_array[i] >> 32);
			if (memcmp(vg->XMLDoc+prevOffset,vg->XMLDoc+vg->temp_offset,prevLen)!=0)
				unequal = TRUE;
			/*for (j = 0; j < prevLen; j++) {
				if (vg->XMLDoc[prevOffset + j]
				!= vg->XMLDoc[vg->temp_offset + j]) {
					unequal = TRUE;
					break;
				}
			}*/
		} else
			unequal = TRUE;
		unique = unique && unequal;
	}
	if (!unique && vg->attr_count != 0){		
		throwException(parse_exception,0,
			"Parse exception in parse()",
			"Error in attr: Attr name not unique");
	}
	unique = TRUE;
	if (vg->attr_count < vg->anaLen) {
		vg->attr_name_array[vg->attr_count] =
			((Long) (vg->temp_offset) << 32) + vg->length1;
		vg->attr_count++;
	} else 
	{
		Long* temp_array = vg->attr_name_array;
		vg->attr_name_array = 
			(Long *)malloc(sizeof(Long)*
			(vg->attr_count + ATTR_NAME_ARRAY_SIZE));

		if (vg->attr_name_array == NULL){
			throwException(parse_exception,0,
				"Parse exception in parse()",
				"alloc mem for attr_name_array_failed");
		}
		vg->anaLen = vg->attr_count + ATTR_NAME_ARRAY_SIZE;

		for (i = 0; i < vg->attr_count; i++) {
			vg->attr_name_array[i] = temp_array[i];
		}
		vg->attr_name_array[vg->attr_count] =
			((Long) (vg->temp_offset) << 32) + vg->length1;
		vg->attr_count++;
	}
		// insert prefix attr node into the prefixed_attr_name array
		// xml:something will not be inserted
		//System.out.println(" prefixed attr count ===>"+prefixed_attr_count);
		//System.out.println(" length2 ===>"+length2);
		if (vg->ns && !vg->is_ns && vg->length2!=0 ){
			if ((vg->increment==1 && vg->length2 ==3 && matchXML(vg,vg->temp_offset))
					|| (vg->increment==2 &&vg->length2 ==6 &&  matchXML(vg,vg->temp_offset))){
				return;
			}
			else if (vg->prefixed_attr_count < vg->panaLen){
				vg->prefixed_attr_name_array[vg->prefixed_attr_count] =
					((Long) (vg->temp_offset) << 32) | (vg->length2<<16)| vg->length1;
				vg->prefixed_attr_count++;
			}else {
				Long* temp_array1 = vg->prefixed_attr_name_array;
				vg->prefixed_attr_name_array =(Long *)malloc(sizeof(Long)*(vg->prefixed_attr_count + ATTR_NAME_ARRAY_SIZE));
				vg->panaLen = vg->prefixed_attr_count+ATTR_NAME_ARRAY_SIZE;
					//new long[vg->prefixed_attr_count + ATTR_NAME_ARRAY_SIZE];
				vg->prefix_URL_array = (int *)malloc(sizeof(int)*(vg->prefixed_attr_count + ATTR_NAME_ARRAY_SIZE));
					//new int[vg->prefixed_attr_count + ATTR_NAME_ARRAY_SIZE];
				memcpy(vg->prefixed_attr_name_array,temp_array1,vg->prefixed_attr_count);
				//System.arraycopy(temp_array1, 0, vg->prefixed_attr_name_array, 0, vg->prefixed_attr_count);
				//System.arraycopy(temp_array1, 0, prefixed_attr_val_array, 0, prefixed_attr_count)
				/*for (int i = 0; i < attr_count; i++) {
					attr_name_array[i] = temp_array[i];
				}*/
				vg->prefixed_attr_name_array[vg->prefixed_attr_count] =
					((Long) (vg->temp_offset) << 32) | (vg->length2<<16)| vg->length1;
				vg->prefixed_attr_count++;
			}
		}
}

static Long _getCharResolved(VTDGen *vg,int byte_offset){

	int ch = 0;
	int val = 0;
	Long inc = 2<<(vg->increment-1);
	Long l = _getChar(vg,byte_offset);

	ch = (int)l;

	if (ch != '&')
		return l;

	// let us handle references here
	//currentOffset++;
	byte_offset+=vg->increment;
	ch = getCharUnit(vg,byte_offset);
	byte_offset+=vg->increment;
	switch (ch) {
				case '#' :

					ch = getCharUnit(vg,byte_offset);

					if (ch == 'x') {
						while (TRUE) {
							byte_offset+=vg->increment;
							inc+=vg->increment;
							ch = getCharUnit(vg,byte_offset);

							if (ch >= '0' && ch <= '9') {
								val = (val << 4) + (ch - '0');
							} else if (ch >= 'a' && ch <= 'f') {
								val = (val << 4) + (ch - 'a' + 10);
							} else if (ch >= 'A' && ch <= 'F') {
								val = (val << 4) + (ch - 'A' + 10);
							} else if (ch == ';') {
								inc+=vg->increment;
								break;
							} 
						}
					} else {
						while (TRUE) {
							ch = getCharUnit(vg,byte_offset);
							byte_offset+=vg->increment;
							inc+=vg->increment;
							if (ch >= '0' && ch <= '9') {
								val = val * 10 + (ch - '0');
							} else if (ch == ';') {
								break;
							} 						
						}
					}
					break;

				case 'a' :
					ch = getCharUnit(vg,byte_offset);
					if (vg->encoding<FORMAT_UTF_16BE){
						if (ch == 'm') {
							if (getCharUnit(vg,byte_offset + 1) == 'p'
								&& getCharUnit(vg,byte_offset + 2) == ';') {
									inc = 5;
									val = '&';
							} 
						} else if (ch == 'p') {
							if (getCharUnit(vg,byte_offset + 1) == 'o'
								&& getCharUnit(vg,byte_offset + 2) == 's'
								&& getCharUnit(vg,byte_offset + 3) == ';') {
									inc = 6;
									val = '\'';
							} 
						} 
					}else{
						if (ch == 'm') {
							if (getCharUnit(vg,byte_offset + 2) == 'p'
								&& getCharUnit(vg,byte_offset + 4) == ';') {
									inc = 10;
									val = '&';
							} 
						} else if (ch == 'p') {
							if (getCharUnit(vg,byte_offset + 2) == 'o'
								&& getCharUnit(vg,byte_offset + 4) == 's'
								&& getCharUnit(vg,byte_offset + 6) == ';') {
									inc = 12;
									val = '\'';
							} 
						} 
					}
					break;

				case 'q' :

					if (vg->encoding<FORMAT_UTF_16BE){
						if (getCharUnit(vg,byte_offset) == 'u'
							&& getCharUnit(vg,byte_offset + 1) == 'o'
							&& getCharUnit(vg,byte_offset + 2) == 't'
							&& getCharUnit(vg,byte_offset + 3) ==';') {
								inc = 6;
								val = '\"';
						} }
					else{
						if (getCharUnit(vg,byte_offset) == 'u'
							&& getCharUnit(vg,byte_offset + 2) == 'o'
							&& getCharUnit(vg,byte_offset + 4) == 't'
							&& getCharUnit(vg,byte_offset + 6) ==';') {
								inc = 12;
								val = '\"';
						}
					}
					break;
				case 'l' :
					if (vg->encoding<FORMAT_UTF_16BE){
						if (getCharUnit(vg,byte_offset) == 't'
							&& getCharUnit(vg,byte_offset + 1) == ';') {
								//offset += 2;
								inc = 4;
								val = '<';
						} 
					}else{
						if (getCharUnit(vg,byte_offset) == 't'
							&& getCharUnit(vg,byte_offset + 2) == ';') {
								//offset += 2;
								inc = 8;
								val = '<';
						} 
					}
					break;
				case 'g' :
					if (vg->encoding<FORMAT_UTF_16BE){
						if (getCharUnit(vg,byte_offset) == 't'
							&& getCharUnit(vg,byte_offset + 1) == ';') {
								inc = 4;
								val = '>';
						} 
					}else {
						if (getCharUnit(vg,byte_offset) == 't'
							&& getCharUnit(vg,byte_offset + 2) == ';') {
								inc = 8;
								val = '>';
						} 
					}
					break;
	}

	//currentOffset++;
	return val | (inc << 32);
}

static inline Long _getChar(VTDGen *vg, int offset){
	int c;
	switch (vg->encoding) {
			case FORMAT_ASCII :
				c =  vg->XMLDoc[offset];
				if (c=='\r' && vg->XMLDoc[offset+1]=='\n')
					return (2LL<<32)|'\n';
				return (1LL<<32)|c;
			case FORMAT_ISO_8859_1 :
				c = vg->XMLDoc[offset];
				if (c=='\r' && vg->XMLDoc[offset+1]=='\n')
					return (2LL<<32)|'\n';
				return (1LL<<32)|c;
			case FORMAT_UTF8 :
				c = vg->XMLDoc[offset];
			   if (c>=0){
				if (c == '\r') {
					if (vg->XMLDoc[offset + 1] == '\n') {
						return '\n'|(2LL<<32);
					} else {
						return '\n'|(1LL<<32);
					}
				}
				//currentOffset++;
				return c|(1LL<<32);
			}				
			return _handle_utf8(vg,c,offset);

			case FORMAT_UTF_16BE :
				// implement UTF-16BE to UCS4 conversion
				return _handle_16be(vg, offset);

			case FORMAT_UTF_16LE :
				return _handle_16le(vg, offset);

			default :
				return _handleOtherEncoding(vg,offset);

	}
}

int decode(VTDGen *vg, int byte_offset){
	    int b = vg->XMLDoc[byte_offset];
	switch(vg->encoding){
		case FORMAT_ISO_8859_2: return iso_8859_2_decode(b);
		case FORMAT_ISO_8859_3: return iso_8859_3_decode(b);
		case FORMAT_ISO_8859_4: return iso_8859_4_decode(b);
		case FORMAT_ISO_8859_5: return iso_8859_5_decode(b);
		case FORMAT_ISO_8859_6: return iso_8859_6_decode(b);
		case FORMAT_ISO_8859_7: return iso_8859_7_decode(b);
		case FORMAT_ISO_8859_8: return iso_8859_8_decode(b);
		case FORMAT_ISO_8859_9: return iso_8859_9_decode(b);
		case FORMAT_ISO_8859_10: return iso_8859_10_decode(b);
		case FORMAT_ISO_8859_11: return iso_8859_11_decode(b);
		case FORMAT_ISO_8859_13: return iso_8859_13_decode(b);
		case FORMAT_ISO_8859_14: return iso_8859_14_decode(b);
		case FORMAT_ISO_8859_15: return iso_8859_15_decode(b);
		
		case FORMAT_WIN_1250: return windows_1250_decode( b);
		case FORMAT_WIN_1251: return windows_1251_decode( b);
		case FORMAT_WIN_1252: return windows_1252_decode( b);
		case FORMAT_WIN_1253: return windows_1253_decode( b);
		case FORMAT_WIN_1254: return windows_1254_decode( b);
		case FORMAT_WIN_1255: return windows_1255_decode( b);
		case FORMAT_WIN_1256: return windows_1256_decode( b);
		case FORMAT_WIN_1257: return windows_1257_decode( b);
		case FORMAT_WIN_1258: return windows_1258_decode( b);   
	}
	return 0;
}

Long _handle_16be(VTDGen *vg, int offset){
	Long val; 

	int temp =
		((vg->XMLDoc[offset ] & 0xff)	<< 8) 
		|(vg->XMLDoc[offset + 1]& 0xff);
	if ((temp < 0xd800)
		|| (temp > 0xdfff)) { // not a high surrogate
			if (temp == '\r') {
				if (vg->XMLDoc[offset  + 3] == '\n'
					&& vg->XMLDoc[offset + 2] == 0) {

						return '\n'|(4LL<<32);
				} else {
					return '\n'|(2LL<<32);
				}
			}
			//currentOffset++;
			return temp| (2LL<<32);
	} else {
		val = temp;
		temp =
			((vg->XMLDoc[offset + 2] & 0xff)
			<< 8) | (vg->XMLDoc[offset+ 3] & 0xff);
		val = ((temp - 0xd800) << 10) + (val - 0xdc00) + 0x10000;
		//currentOffset += 2;
		return val | (4LL<<32);
	}
}

Long _handle_16le(VTDGen *vg, int offset){
	// implement UTF-16LE to UCS4 conversion
	int val, temp =
		(vg->XMLDoc[offset + 1 ] & 0xff)
		<< 8 | (vg->XMLDoc[offset] & 0xff);
	if (temp < 0xdc00 || temp > 0xdfff) { // check for low surrogate
		if (temp == '\r') {
			if (vg->XMLDoc[offset + 2] == '\n'
				&& vg->XMLDoc[offset + 3] == 0) {
					return '\n' | (4LL<<32) ;
			} else {
				return '\n' | (2LL<<32);
			}
		}
		return temp | (2LL<<32);
	} else {
		val = temp;
		temp =
			(vg->XMLDoc[offset + 3]&0xff)
			<< 8 | (vg->XMLDoc[offset + 2] & 0xff);
		val = ((temp - 0xd800)<<10) + (val - 0xdc00) + 0x10000;

		return val | (4LL<<32);
	}
}

Long _handle_utf8(VTDGen *vg, int temp,int offset){
	int c=0, d=0, a=0,i; 
	Long val;
	switch (UTF8Char_byteCount((int)temp & 0xff)) {
			case 2:
				c = 0x1f;
				d = 6;
				a = 1;
				break;
			case 3:
				c = 0x0f;
				d = 12;
				a = 2;
				break;
			case 4:
				c = 0x07;
				d = 18;
				a = 3;
				break;
			case 5:
				c = 0x03;
				d = 24;
				a = 4;
				break;
			case 6:
				c = 0x01;
				d = 30;
				a = 5;
				break;
	}

	val = (temp & c) << d;
	i = a - 1;
	while (i >= 0) {
		temp = vg->XMLDoc[offset + a - i];
		val = val | ((temp & 0x3f) << ((i << 2) + (i << 1)));
		i--;
	}
	//currentOffset += a + 1;
	return val | (((Long)(a+1))<<32);
}

Long _handleOtherEncoding(VTDGen *vg,int byte_offset){
	    int b = vg->XMLDoc[byte_offset];
		if (b=='\r' && vg->XMLDoc[byte_offset+1]=='\n')
				return (2LL<<32)|'\n';
	switch(vg->encoding){
		case FORMAT_ISO_8859_2: return iso_8859_2_decode(b)|(1LL<<32);
		case FORMAT_ISO_8859_3: return iso_8859_3_decode(b)|(1LL<<32);
		case FORMAT_ISO_8859_4: return iso_8859_4_decode(b)|(1LL<<32);
		case FORMAT_ISO_8859_5: return iso_8859_5_decode(b)|(1LL<<32);
		case FORMAT_ISO_8859_6: return iso_8859_6_decode(b)|(1LL<<32);
		case FORMAT_ISO_8859_7: return iso_8859_7_decode(b)|(1LL<<32);
		case FORMAT_ISO_8859_8: return iso_8859_8_decode(b)|(1LL<<32);
		case FORMAT_ISO_8859_9: return iso_8859_9_decode(b)|(1LL<<32);
		case FORMAT_ISO_8859_10: return iso_8859_10_decode(b)|(1LL<<32);
		case FORMAT_ISO_8859_11: return iso_8859_11_decode(b)|(1LL<<32);
		case FORMAT_ISO_8859_13: return iso_8859_13_decode(b)|(1LL<<32);
		case FORMAT_ISO_8859_14: return iso_8859_14_decode(b)|(1LL<<32);
		case FORMAT_ISO_8859_15: return iso_8859_15_decode(b)|(1LL<<32);
		
		case FORMAT_WIN_1250: return windows_1250_decode( b)|(1LL<<32);
		case FORMAT_WIN_1251: return windows_1251_decode( b)|(1LL<<32);
		case FORMAT_WIN_1252: return windows_1252_decode( b)|(1LL<<32);
		case FORMAT_WIN_1253: return windows_1253_decode( b)|(1LL<<32);
		case FORMAT_WIN_1254: return windows_1254_decode( b)|(1LL<<32);
		case FORMAT_WIN_1255: return windows_1255_decode( b)|(1LL<<32);
		case FORMAT_WIN_1256: return windows_1256_decode( b)|(1LL<<32);
		case FORMAT_WIN_1257: return windows_1257_decode( b)|(1LL<<32);
		case FORMAT_WIN_1258: return windows_1258_decode( b)|(1LL<<32);   
	}
	return 0LL;
}


void handleOtherTextChar(VTDGen *vg, int ch){
	if (ch == '&') {
		//has_amp = true;	
		if (!XMLChar_isValidChar(entityIdentifier(vg)))
			throwException2(parse_exception,
			"Error in text content: Invalid char in text content ");
		//+ formatLineNumber());
		//parser_state = STATE_TEXT;
	}  else if (ch == ']') {
		if (skipChar(vg,']')) {
			while (skipChar(vg,']')) {
			}
			if (skipChar(vg,'>'))
				throwException2(parse_exception,
				"Error in text content: ]]> in text content");
			//+ formatLineNumber());
		}	
	} else
		throwException2(parse_exception,
		"Error in text content: Invalid char in text content ");
	//+ formatLineNumber());		
}

void handleOtherTextChar2(VTDGen *vg,int ch){
	if (ch == '&') {
		//has_amp = true;
		//temp_offset = offset;
		entityIdentifier(vg);
		//parser_state = STATE_TEXT;
	} else if (ch == ']') {
		if (skipChar(vg,']')) {
			while (skipChar(vg,']')) {
			}
			if (skipChar(vg,'>'))
				throwException2(parse_exception,
				"Error in text content: ]]> in text content");
			//+ formatLineNumber());
		}
		//parser_state = STATE_TEXT;
	}else
		throwException2(parse_exception,
		"Error in text content: Invalid char");
		//+ formatLineNumber());
}

void _writeVTD(VTDGen *vg, int offset, int length, tokenType token_type, int depth){
	appendLong(vg->VTDBuffer,((Long) ((token_type << 28)
					| ((depth & 0xff) << 20) | length) << 32)
					| offset);
}

void writeVTDText(VTDGen *vg, int offset, int length, tokenType token_type, int depth){
	if (length > MAX_TOKEN_LENGTH) {
		int k;
		int r_offset = offset;
		Long l = ((Long)((token_type << 28)
			| ((depth & 0xff) << 20) 
			| MAX_TOKEN_LENGTH) << 32);
		for (k = length; k > MAX_TOKEN_LENGTH; k = k - MAX_TOKEN_LENGTH) {
			appendLong(vg->VTDBuffer, l | r_offset);
			r_offset += MAX_TOKEN_LENGTH;
		}
		appendLong(vg->VTDBuffer,((Long) ((token_type << 28)
			| ((depth & 0xff) << 20) | k) << 32)
			| r_offset);
	} else {
		appendLong(vg->VTDBuffer,((Long) ((token_type << 28)
			| ((depth & 0xff) << 20) | length) << 32)
			| offset);
	}
}

void writeVTD_L5(VTDGen *vg, int offset, int length, tokenType token_type, int depth){	
	appendLong(vg->VTDBuffer,((Long) ((token_type << 28)
		| ((depth & 0xff) << 20) | length) << 32)
		| offset);

	// remember VTD depth start from zero

	switch (depth) {
			case 0:
				vg->rootIndex = vg->VTDBuffer->size - 1;
				break;
			case 1:
				if (vg->last_depth == 1) {
					appendLong(vg->l1Buffer,((Long) vg->last_l1_index << 32) | 0xffffffffLL);
				} else if (vg->last_depth == 2) {
					appendLong(vg->l2Buffer,((Long) vg->last_l2_index << 32) | 0xffffffffLL);
				} else if (vg->last_depth ==3) {
					appendLong(vg->_l3Buffer,((Long) vg->last_l3_index << 32) | 0xffffffffLL);
				} else if (vg->last_depth ==4){
					appendLong(vg->_l4Buffer,((Long) vg->last_l4_index << 32) | 0xffffffffLL);
				}
				vg->last_l1_index = vg->VTDBuffer->size - 1;
				vg->last_depth = 1;
				break;
			case 2:
				if (vg->last_depth == 1) {
					appendLong(vg->l1Buffer,((Long) vg->last_l1_index << 32)
						+ vg->l2Buffer->size);
				} else if (vg->last_depth == 2) {
					appendLong(vg->l2Buffer,((Long) vg->last_l2_index << 32) | 0xffffffffLL);
				} else if (vg->last_depth ==3) {
					appendLong(vg->_l3Buffer,((Long) vg->last_l3_index << 32) | 0xffffffffLL);
				} else if (vg->last_depth ==4){
					appendLong(vg->_l4Buffer,((Long) vg->last_l4_index << 32) | 0xffffffffLL);
				}
				vg->last_l2_index = vg->VTDBuffer->size - 1;
				vg->last_depth = 2;
				break;

			case 3:
				/*if (last_depth == 1) {
				l1Buffer.append(((long) last_l1_index << 32)
				+ l2Buffer.size);
				} else*/ 
				if (vg->last_depth == 2) {
					appendLong(vg->l2Buffer,((Long) vg->last_l2_index << 32) 
						+ vg->_l3Buffer->size);
				} else if (vg->last_depth ==3) {
					appendLong(vg->_l3Buffer,((Long) vg->last_l3_index << 32) | 0xffffffffLL);
				} else if (vg->last_depth ==4){
					appendLong(vg->_l4Buffer,((Long) vg->last_l4_index << 32) | 0xffffffffLL);
				}
				vg->last_l3_index = vg->VTDBuffer->size - 1;
				vg->last_depth = 3;
				break;

			case 4:
				/*if (last_depth == 1) {
				l1Buffer.append(((long) last_l1_index << 32)
				+ l2Buffer.size);
				} else if (last_depth == 2) {
				l2Buffer.append(((long) last_l2_index << 32) | 0xffffffffLL);
				} else*/ 
				if (vg->last_depth ==3) {
					appendLong(vg->_l3Buffer,((Long) vg->last_l3_index << 32) 
						+ vg->_l4Buffer->size);
				} else if (vg->last_depth ==4){
					appendLong(vg->_l4Buffer,((Long) vg->last_l4_index << 32) | 0xffffffffLL);
				}
				vg->last_l4_index = vg->VTDBuffer->size - 1;
				vg->last_depth = 4;
				break;
			case 5:
				appendInt(vg->_l5Buffer,vg->VTDBuffer->size - 1);
				if (vg->last_depth == 4) {
					appendLong(vg->_l4Buffer,((Long) vg->last_l4_index << 32)
						+ vg->_l5Buffer->size - 1);
				}
				vg->last_depth = 5;
				break;

				//default:
				//rootIndex = VTDBuffer.size() - 1;
	}
}

void selectLcDepth(VTDGen *vg,int i){
	if (i!=3 &&i!=5)
		throwException2(parse_exception,"LcDepth can only take the value of 3 or 5");
	if (i==5)
		vg->shallowDepth = FALSE;
}

VTDNav* loadIndex(VTDGen *vg, char *fileName){
	FILE *f = NULL; 
	VTDNav *vn = NULL;
	//exception e;
	//int status,len;
	UByte *ba=NULL;
	//struct stat buffer;
	f = fopen(fileName,"rb");
	if (f==NULL){
		throwException2(invalid_argument,"fileName not valid");
		return NULL;
	}
	vn=loadIndex2(vg,f);
	fclose(f);
	return vn;

}