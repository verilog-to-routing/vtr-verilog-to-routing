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
#ifndef CUSTOMTYPE_H
#define CUSTOMTYPE_H
#include <wchar.h>
#include "cexcept.h"
#include <string.h>
#include <stdlib.h>

//#include <float.h>
// those are for min and max of a float number
#define MAXFLOAT 3.402823466e+38F
#define MINFLOAT 1.175494351e-38F

#define MAXINT  0x7fffffff
#define MININT 0x80000001

#define MAXLONG 0x7fffffffffffffffLL
#define MINLONG 0x8000000000000001LL

//check C compiler type gcc, vcc, or intel CC, digital Mars
#ifdef _MSC_VER 
 #define _thread __declspec(thread)
#define inline __inline
#elif defined (__GNUC__) 
 #define _thread __thread
#define inline inline
#elif defined(__ICC) && defined(_WIN32)  
 #define _thread __declspec(thread)
#define inline inline
#elif defined(__DMC__)
 #define _thread __declspec(thread)
#define inline __inline
#else
 #define _thread __thread
#define inline __inline
#endif


//#define inline __inline
//#define _UNICODE
typedef wchar_t UCSChar;
typedef long long Long;
typedef char Byte;
typedef unsigned char UByte;
// VTD-XML's own definition of Boolean
typedef enum Bool {FALSE, 
					  TRUE} 
					Boolean;

// VTD navigation parameters
typedef enum direction {ROOT,
						PARENT,
						FIRST_CHILD,
						LAST_CHILD,
						NEXT_SIBLING,
						PREV_SIBLING} 
					navDir;


typedef enum XMLencoding {FORMAT_ASCII,
						  FORMAT_ISO_8859_1,
						  FORMAT_UTF8,
						  FORMAT_ISO_8859_2,
						  FORMAT_ISO_8859_3,
						  FORMAT_ISO_8859_4,
						  FORMAT_ISO_8859_5,
						  FORMAT_ISO_8859_6,
						  FORMAT_ISO_8859_7,
						  FORMAT_ISO_8859_8,
						  FORMAT_ISO_8859_9,
						  FORMAT_ISO_8859_10,
						  FORMAT_ISO_8859_11,
						  FORMAT_ISO_8859_12,
						  FORMAT_ISO_8859_13,
						  FORMAT_ISO_8859_14,
						  FORMAT_ISO_8859_15,
						  FORMAT_ISO_8859_16,
						  FORMAT_WIN_1250,
						  FORMAT_WIN_1251,
						  FORMAT_WIN_1252,
						  FORMAT_WIN_1253,
						  FORMAT_WIN_1254,
						  FORMAT_WIN_1255,
						  FORMAT_WIN_1256,
						  FORMAT_WIN_1257,
						  FORMAT_WIN_1258,
						  FORMAT_UTF_16BE = 63,
						  FORMAT_UTF_16LE =64} 
					encoding_t;


typedef enum VTDtokentype {TOKEN_STARTING_TAG,
						   TOKEN_ENDING_TAG,
						   TOKEN_ATTR_NAME,
						   TOKEN_ATTR_NS,
						   TOKEN_ATTR_VAL,
						   TOKEN_CHARACTER_DATA,
						   TOKEN_COMMENT,
						   TOKEN_PI_NAME,
						   TOKEN_PI_VAL,
						   TOKEN_DEC_ATTR_NAME,
						   TOKEN_DEC_ATTR_VAL,
						   TOKEN_CDATA_VAL,
						   TOKEN_DTD_VAL,
						   TOKEN_DOCUMENT}
					tokenType;

typedef enum SType {ASCENDING,DESCENDING}
					sortType;


					enum exception_type {out_of_mem, 
					  					 invalid_argument,
										 array_out_of_bound,
										 parse_exception,
										 nav_exception,
										 pilot_exception,
										 number_format_exception,
										 xpath_parse_exception,
										 xpath_eval_exception,
										 modify_exception,
										 index_write_exception,
										 index_read_exception,
										 io_exception,
										 transcode_exception,
										 other_exception};



typedef struct vtd_exception {
						enum exception_type et;
						int subtype; // subtype to be defined later
						const char* msg;
						const char* sub_msg;
					} exception;
					
					define_exception_type(exception);
					extern _thread struct exception_context the_exception_context[1];

extern void throwException(enum exception_type et1, int sub_type, char* msg, char* submsg);
extern void throwException2(enum exception_type et1, char *msg);

//#define NaN  (0/0.0)
#ifndef isNaN
#define isNaN(x) ((x) != (x))
#endif

#ifndef min
#define min(a,b)  (((a) > (b)) ? (b) : (a)) 
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#endif
