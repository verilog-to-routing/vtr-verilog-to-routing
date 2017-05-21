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
/* This header file contains various structures to tracking various data structures used 
   during the parsing of XPath expressions */
#ifndef HELPER_H
#define HELPER_H

#include "customTypes.h"
#include <stdlib.h>
struct obj {
   void* object;
   struct obj *next;
} ;
/*extern _thread struct obj *objList;
extern _thread struct obj *ptr1;
extern _thread struct obj *ptr2;*/

/* add to a global linked list during yyparse, to prevent mem leak */ 
void addObj(void *obj); 
/* if there is anything wrong during parsing */
void freeAllObj();  
/* this function is called if yyparse returns 0 */
void resetObj();

#endif
