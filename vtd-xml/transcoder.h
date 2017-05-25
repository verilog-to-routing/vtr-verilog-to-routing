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
#ifndef TRANSCODER_H
#define TRANSCODER_H

#include "customTypes.h"
int UTF16BE_Coder_encode(UByte* output,int offset,int ch);
Long UTF16BE_Coder_decode(UByte *input,int offset);
int UTF16BE_Coder_getLen(int ch);
void encodeAndWrite(FILE* f, int ch);

int UTF16LE_Coder_encode(UByte* output,int offset,int ch);
Long UTF16LE_Coder_decode(UByte *input,int offset);
int UTF16LE_Coder_getLen(int ch);
void UTF16LE_Coder_encodeAndWrite(FILE* f, int ch);

int UTF8_Coder_encode(UByte* output,int offset,int ch);
Long UTF8_Coder_decode(UByte *input,int offset);
int UTF8_Coder_getLen(int ch);
void UTF8_Coder_encodeAndWrite(FILE* f, int ch);

int ASCII_Coder_encode(UByte* output,int offset,int ch);
Long ASCII_Coder_decode(UByte *input,int offset);
int ASCII_Coder_getLen(int ch);
void ASCII_Coder_encodeAndWrite(FILE* f, int ch);

int ISO8859_1_Coder_encode(UByte* output,int offset,int ch);
Long ISO8859_1_Coder_decode(UByte *input,int offset);
int ISO8859_1_Coder_getLen(int ch);
void ISO8859_1_Coder_encodeAndWrite(FILE* f, int ch);

Long Transcoder_decode(UByte* input, int offset, encoding_t input_encoding);
int  Transcoder_encode(UByte* output, int offset, int ch, encoding_t output_encoding);
void Transcoder_encodeAndWrite(FILE* f, int ch, encoding_t output_encoding);
int  Transcoder_getLen(int ch, encoding_t output_encoding);
int  Transcoder_getOutLength(UByte* input, int offset,int length,encoding_t input_encoding, encoding_t output_encoding);
Long Transcoder_transcode(UByte* input, int offset, int length, encoding_t input_encoding, encoding_t output_encoding);
void Transcoder_transcodeAndFill(UByte* input, UByte* output,int offset, int length, encoding_t input_encoding, encoding_t output_encoding);
int Transcoder_transcodeAndFill2(int initOutPosition, UByte* input, UByte* output, int offset, int length, encoding_t input_encoding, encoding_t output_encodin);
void Transcoder_transcodeAndWrite(UByte* input, FILE* f,int offset, int length, encoding_t input_encoding, encoding_t output_encoding);

#endif