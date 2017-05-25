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
#include "transcoder.h"
#include <stdio.h>
#include <stdlib.h>

static inline void my_fputc(int ch, FILE *f){
	int i = fputc(ch, f);
	if (i==EOF){
		throwException2(io_exception,
			"End of File reach in fputc");
	}
}
int UTF16BE_Coder_encode(UByte* output,int offset,int ch){
	if (ch<0x10000){ 
		output[offset+1] = (ch & 0xff);
		output[offset] =  ((ch & 0xff00) >> 8);
		return 2 + offset;
	} else {
		int tmp = ch-0x10000;
		int w1 = 0xd800 | (tmp & 0xffc00);
		int w2 = 0xdc00 | (tmp & 0x3ff);
		output[offset] = ((w1 & 0xff00) >> 8);
		output[offset+1] = (w1 & 0xff);
		output[offset+2] = ((w2 & 0xff00) >> 8);
		output[offset+3] = ((w2 & 0xff));
		return 4 + offset;
	}
}

Long UTF16BE_Coder_decode(UByte *input,int offset){
	int val; 
	Long l;
	int temp = (input[offset] << 8) | input[offset+1];
	if (temp < 0xd800 || temp > 0xdfff) { 
		l = offset+2;
		return (l<<32)|temp;
	}else {
		val = temp;
		temp = (input[offset+2] << 8) | input[offset+3];
		val = ((temp - 0xd800) << 10) + (val - 0xdc00) + 0x10000;
		l = offset+4;
		return (l<<32)|temp;
	}
}

int UTF16BE_Coder_getLen(int ch){
  if (ch <0x10000)
            return 2;
        else
            return 4;
}

void UTF16BE_Coder_encodeAndWrite(FILE* f, int ch){
	if (ch<0x10000){ 
		//output[offset] =  (byte)((ch & 0xff00) >> 8);
		//os.write((ch & 0xff00) >> 8);
		my_fputc((ch & 0xff00) >> 8,f);
		//output[offset+1] = (byte)(ch & 0xff);
		//os.write(ch);
		my_fputc(ch,f);
		//return 2 + offset;
	} else {
		int tmp = ch-0x10000;
		int w1 = 0xd800 | (tmp & 0xffc00);
		int w2 = 0xdc00 | (tmp & 0x3ff);
		my_fputc(((w1 & 0xff00) >> 8),f);
		my_fputc((w1 & 0xff),f);
		my_fputc(((w2 & 0xff00) >> 8),f);
		my_fputc((w2 & 0xff),f);
	}
}

int UTF16LE_Coder_encode(UByte* output,int offset,int ch){
	if (ch<0x10000){ 
		output[offset] = (ch & 0xff);
		output[offset+1] = ((ch & 0xff00) >> 8);
		return 2 + offset;
	} else {
		int tmp = ch - 0x10000;
		int w1 = 0xd800 | (tmp & 0xffc00);
		int w2 = 0xdc00 | (tmp & 0x3ff);
		output[offset] = (w1 & 0xff);
		output[offset+1] = ((w1 & 0xff00) >> 8);
		output[offset+2] =(w2 & 0xff);
		output[offset+3] = ((w2 & 0xff00) >> 8);
		return 4 + offset;
	}
}
Long UTF16LE_Coder_decode(UByte *input,int offset){
	int val; Long l;
	int temp = (input[offset+1] << 8) | input[offset];
	if (temp < 0xd800 || temp > 0xdfff) { 
		l = offset+2;
		return (l<<32)|temp;
	}else {
		val = temp;
		temp = (input[offset+3] << 8) | input[offset+2];
		val = ((temp - 0xd800) << 10) + (val - 0xdc00) + 0x10000;
		l = offset+4;
		return (l<<32)|temp;
	}
}

int UTF16LE_Coder_getLen(int ch){
	    if (ch <0x10000)
            return 2;
        else
            return 4;
}

void UTF16LE_Coder_encodeAndWrite(FILE* f, int ch){
	if (ch<0x10000){ 
		my_fputc((ch & 0xff),f);
		my_fputc((ch & 0xff00) >> 8,f);
	} else {
		int tmp = ch - 0x10000;
		int w1 = 0xd800 | (tmp & 0xffc00);
		int w2 = 0xdc00 | (tmp & 0x3ff);
		my_fputc(w1 & 0xff, f);
		my_fputc((w1 & 0xff00) >> 8, f);
		my_fputc(w2 & 0xff, f);
		my_fputc((w2 & 0xff00) >> 8, f);
	}
}

int UTF8_Coder_encode(UByte* output,int offset,int ch){
	if (ch < 128){
		output[offset] = ch;
		return offset+1;
	}
	if (ch < 0x800){
		output[offset]= (((ch & 0x7c0) >> 6) | 0xc0);
		output[offset+1] = ((ch & 0x3f) | 0x80);
		return offset+2;
	}
	if (ch < 0xe000){
		output[offset]= (((ch & 0xf000) >> 12) | 0xe0);
		output[offset+1] = (((ch & 0xfc) >> 6) | 0x80);
		output[offset+2] = ((ch & 0x3f) | 0x80);
		return offset+3;
	}

	output[offset]= (((ch & 0x1c0000) >> 18) | 0xf0);
	output[offset+1] = (((ch & 0x3f0) >> 12) | 0x80);
	output[offset+2] = (((ch & 0xfc) >> 6) | 0x80);
	output[offset+3] = ((ch & 0x3f) | 0x80);

	return offset+4;        
}

Long UTF8_Coder_decode(UByte *input,int offset){
	Long l = 0;
	int c=0;
	UByte val = input[offset];
	if (val <128){
		l = offset + 1;
		return (l<<32) | val; 
	}

	if ((val & 0xe0 )== 0xc0){
		l = offset + 2;
		c = (((int) (val& 0x1f))<< 6)| (input[offset+1] & 0x3f);
		return (l<<32) | c;
	}

	if ((val & 0xf0) == 0xe0){
		l = offset + 3;
		c = (((int) (val& 0x0f))<<12) | 
			(((int)input[offset+1] & 0x3f)<<6) |
			(input[offset+2] & 0x3f);
		return (l<<32) | c;
	}

	l = offset+4;
	c = (((int) (val& 0x07))<<18) | 
		(((int)input[offset+1] & 0x3f)<<12) |
		(((int)input[offset+2] & 0x3f)<< 6) |
		(input[offset+3] & 0x3f);

	return (l<<32) | c;
}

int UTF8_Coder_getLen(int ch){
	if (ch < 128)
		return 1;
	if (ch < 0x800)
		return 2;
	if (ch < 0xe000)
		return 3;
	if (ch < 0x10000)
		return 4;
	return 5;
}

void UTF8_Coder_encodeAndWrite(FILE* f, int ch){
	if (ch < 128){
		my_fputc(ch,f);
		return;
	}
	if (ch < 0x800){
		my_fputc(((ch & 0x7c0) >> 6) | 0xc0,f);
		my_fputc((ch & 0x3f) | 0x80,f );
		return;
	}
	if (ch < 0xe000){
		my_fputc(((ch & 0xf000) >> 12) | 0xe0,f);
		my_fputc(((ch & 0xfc) >> 6) | 0x80,f);
		my_fputc((ch & 0x3f) | 0x80,f);
		return;
	}
	my_fputc(((ch & 0x1c0000) >> 18) | 0xf0, f);
	my_fputc(((ch & 0x3f0) >> 12) | 0x80, f);
	my_fputc(((ch & 0xfc) >> 6) | 0x80, f);
	my_fputc((ch & 0x3f) | 0x80, f);
}

int ASCII_Coder_encode(UByte* output,int offset,int ch){
	output[offset] = ch;
    return offset+1;
}

Long ASCII_Coder_decode(UByte *input,int offset){
    Long l = input[offset];
    return (((Long)(offset+1))<<32) | l ;
}

int ASCII_Coder_getLen(int ch){
	if (ch>127){
		throwException2(transcode_exception,"Invalid UCS char for ASCII format");
		return -1;
	}
	else
		return 1;
}

void ASCII_Coder_encodeAndWrite(FILE* f, int ch){
	if (ch>127)
		throwException2(transcode_exception,"Invalid UCS char for ASCII format");
	my_fputc(ch,f);
}

int ISO8859_1_Coder_encode(UByte* output,int offset,int ch){
	output[offset] = ch;
	return offset+1;
}
Long ISO8859_1_Coder_decode(UByte *input,int offset){
	Long l = input[offset] & 0xff;
	return (((Long)(offset+1))<<32) | l ;
}
int ISO8859_1_Coder_getLen(int ch){
	if (ch>255)
	{
		throwException2(transcode_exception,"Invalid UCS char for ISO-8859-1 format");
		return -1;
	}
	else
		return 1;
}
void ISO8859_1_Coder_encodeAndWrite(FILE* f, int ch){
	if (ch>255)
		throwException2(transcode_exception,"Invalid UCS char for ISO-8859-1 format");
	my_fputc(ch,f);
}

Long Transcoder_decode(UByte* input, int offset, encoding_t input_encoding){
 switch (input_encoding) {
        case FORMAT_ASCII:
            return ASCII_Coder_decode(input, offset);
        case FORMAT_UTF8:
            return UTF8_Coder_decode(input, offset);
        case FORMAT_ISO_8859_1:
            return ISO8859_1_Coder_decode(input, offset);
        case FORMAT_UTF_16LE:
            return UTF16LE_Coder_decode(input, offset);
        case FORMAT_UTF_16BE:
            return UTF16BE_Coder_decode(input, offset);
        default:
            throwException2(transcode_exception,"Unsupported encoding");
			return -1;
        }
}

int  Transcoder_encode(UByte* output, int offset, int ch, encoding_t output_encoding){
	switch (output_encoding) {
		case FORMAT_ASCII:
			return ASCII_Coder_encode(output, offset, ch);
		case FORMAT_UTF8:
			return UTF8_Coder_encode(output, offset, ch);
		case FORMAT_ISO_8859_1:
			return ISO8859_1_Coder_encode(output, offset, ch);
		case FORMAT_UTF_16LE:
			return UTF16LE_Coder_encode(output, offset, ch);
		case FORMAT_UTF_16BE:
			return UTF16BE_Coder_encode(output, offset, ch);
		default:
			throwException2(transcode_exception, "Unsupported encoding");
			return -1;
	}
}

void Transcoder_encodeAndWrite(FILE* f, int ch, encoding_t output_encoding){
        switch (output_encoding) {
        case FORMAT_ASCII:
             ASCII_Coder_encodeAndWrite(f, ch);
        	 return;
        case FORMAT_UTF8:
             UTF8_Coder_encodeAndWrite(f,  ch);
        	 return;
        case FORMAT_ISO_8859_1:
             ISO8859_1_Coder_encodeAndWrite(f, ch);
        	 return;
        case FORMAT_UTF_16LE:
             UTF16LE_Coder_encodeAndWrite(f, ch);
        	 return;
        case FORMAT_UTF_16BE:
             UTF16BE_Coder_encodeAndWrite(f, ch);
        	 return;
        default:
            throwException2(transcode_exception, "Unsupported encoding");
			return;
        }
}

int  Transcoder_getLen(int ch, encoding_t output_encoding){
	switch (output_encoding) {
		case FORMAT_ASCII:
			return ASCII_Coder_getLen(ch);
		case FORMAT_UTF8:
			return UTF8_Coder_getLen(ch);
		case FORMAT_ISO_8859_1:
			return ISO8859_1_Coder_getLen(ch);
		case FORMAT_UTF_16LE:
			return UTF16LE_Coder_getLen(ch);
		case FORMAT_UTF_16BE:
			return UTF16BE_Coder_getLen(ch);
		default:
			throwException2(transcode_exception,"Unsupported encoding");
			return -1;
	}
}

int  Transcoder_getOutLength(UByte* input, int offset,int length,encoding_t input_encoding, encoding_t output_encoding){
	int len = 0;
	int k = offset;
	int c;
	while (k < offset + length) {
		Long l = Transcoder_decode(input, k, input_encoding);
		k = (int) (l >>32);
		c = (int) l;
		len = len + Transcoder_getLen(c, output_encoding);
	}
	return len;
}

Long Transcoder_transcode(UByte* input, int offset, int length, encoding_t input_encoding, encoding_t output_encoding){
	//check input and output encoding

	// calculate the length of the output byte array
	int i = Transcoder_getOutLength(input, offset, length, input_encoding,
		output_encoding);
	// allocate the byte array
	
	UByte* output = (UByte*) malloc(i);
	if (output==NULL){
		throwException2(out_of_mem, "allocate byte buffer failed in Transcoder_transcode");
	}
	// fill the byte array with output encoding
	Transcoder_transcodeAndFill(input, output, offset, length, input_encoding,
		output_encoding);
	return ((Long)i)<<32 | (int)output;
}

void Transcoder_transcodeAndFill(UByte* input, UByte* output,int offset, int length, encoding_t input_encoding, encoding_t output_encoding){
	/*int len = 0;*/
	int k = offset;
	int c, i = 0;
	while (k < offset + length) {
		Long l = Transcoder_decode(input, k, input_encoding);
		k = (int) (l >> 32);
		c = (int) l;
		i = Transcoder_encode(output, i, c, output_encoding);
	}
}

int Transcoder_transcodeAndFill2(int initOutPosition, UByte* input, UByte* output, int offset, int length, 
								  encoding_t input_encoding, encoding_t output_encoding){
	/*int len = 0;*/
	int k = offset;
	int c, i = initOutPosition;
	while (k < offset + length) {
		Long l = Transcoder_decode(input, k, input_encoding);
		k = (int) (l >> 32);
		c = (int) l;
		i = Transcoder_encode(output, i, c, output_encoding);
	}
	return i;
}

void Transcoder_transcodeAndWrite(UByte* input, FILE* f,int offset, int length, encoding_t input_encoding, encoding_t output_encoding){
	/*int len = 0;*/
	int k = offset;
	int c;
	while (k < offset + length) {
		Long l = Transcoder_decode(input, k, input_encoding);
		k = (int) (l >> 32);
		c = (int) l;
		Transcoder_encodeAndWrite(f, c, output_encoding);
	}
}