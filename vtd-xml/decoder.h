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
/* this file decodes various functions and data structures that conver
   encodings format into Unicode */

#ifndef DECODER_H
#define DECODER_H
#include "customTypes.h"
							  
int iso_8859_2_chars[256];
int iso_8859_3_chars[256];
int iso_8859_4_chars[256];
int iso_8859_5_chars[256];
int iso_8859_6_chars[256];
int iso_8859_7_chars[256];
int iso_8859_8_chars[256];
int iso_8859_9_chars[256];
int iso_8859_10_chars[256];
int iso_8859_11_chars[256];
int iso_8859_13_chars[256];
int iso_8859_14_chars[256];
int iso_8859_15_chars[256];
int windows_1250_chars[256];
int windows_1251_chars[256];
int windows_1252_chars[256];
int windows_1253_chars[256];
int windows_1254_chars[256];
int windows_1255_chars[256];
int windows_1256_chars[256];
int windows_1257_chars[256];
int windows_1258_chars[256];

Boolean iso_8859_2_chars_ready;
//Boolean iso_8859_3_chars_ready;
Boolean iso_8859_3_chars_ready;
Boolean iso_8859_4_chars_ready;
Boolean iso_8859_5_chars_ready;
Boolean iso_8859_6_chars_ready;
Boolean iso_8859_7_chars_ready;
Boolean iso_8859_8_chars_ready;
Boolean iso_8859_9_chars_ready;
Boolean iso_8859_10_chars_ready;
Boolean iso_8859_11_chars_ready;
Boolean iso_8859_13_chars_ready;
Boolean iso_8859_14_chars_ready;
Boolean iso_8859_15_chars_ready;

Boolean windows_1250_chars_ready;
Boolean windows_1251_chars_ready;
Boolean windows_1252_chars_ready;
Boolean windows_1253_chars_ready;
Boolean windows_1254_chars_ready;
Boolean windows_1255_chars_ready;
Boolean windows_1256_chars_ready;
Boolean windows_1257_chars_ready;
Boolean windows_1258_chars_ready;

 void iso_8859_2_chars_init();
 void iso_8859_3_chars_init();
 void iso_8859_4_chars_init();
 void iso_8859_5_chars_init();
 void iso_8859_6_chars_init();
 void iso_8859_7_chars_init();
 void iso_8859_8_chars_init();
 void iso_8859_9_chars_init();
 void iso_8859_10_chars_init();
 void iso_8859_11_chars_init();
 void iso_8859_13_chars_init();
 void iso_8859_14_chars_init();
 void iso_8859_15_chars_init();

 void windows_1250_chars_init();
 void windows_1251_chars_init();
 void windows_1252_chars_init();
 void windows_1253_chars_init();
 void windows_1254_chars_init();
 void windows_1255_chars_init();
 void windows_1256_chars_init();
 void windows_1257_chars_init();
 void windows_1258_chars_init();
 
 
 

#define iso_8859_2_decode(b) iso_8859_2_chars[b]
#define iso_8859_3_decode(b) iso_8859_3_chars[b]
#define iso_8859_4_decode(b) iso_8859_4_chars[b]
#define iso_8859_5_decode(b) iso_8859_5_chars[b]
#define iso_8859_6_decode(b) iso_8859_6_chars[b]
#define iso_8859_7_decode(b) iso_8859_7_chars[b]
#define iso_8859_8_decode(b) iso_8859_8_chars[b]
#define iso_8859_9_decode(b) iso_8859_9_chars[b]
#define iso_8859_10_decode(b) iso_8859_10_chars[b]
#define iso_8859_11_decode(b) iso_8859_11_chars[b]
#define iso_8859_13_decode(b) iso_8859_13_chars[b]
#define iso_8859_14_decode(b) iso_8859_14_chars[b]
#define iso_8859_15_decode(b) iso_8859_15_chars[b]


#define windows_1250_decode( b) windows_1250_chars[b]
#define windows_1251_decode( b) windows_1251_chars[b]
#define windows_1252_decode( b) windows_1252_chars[b]
#define windows_1253_decode( b) windows_1253_chars[b]
#define windows_1254_decode( b) windows_1254_chars[b]
#define windows_1255_decode( b) windows_1255_chars[b]
#define windows_1256_decode( b) windows_1256_chars[b]
#define windows_1257_decode( b) windows_1257_chars[b]
#define windows_1258_decode( b) windows_1258_chars[b]

#endif
