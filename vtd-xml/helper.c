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
#include "helper.h"
_thread struct obj *objList;
_thread struct obj *ptr1;
_thread struct obj *ptr2;

void addObj(void *o){	
	if (objList ==NULL){
		objList = ptr1 = (struct obj *)malloc(sizeof(struct obj));
		if (ptr1 == NULL){
			throwException2(out_of_mem, "out of memory in addObj during parsing XPath");
			//e.et = out_of_mem;
			//Throw e;
		} 		

	} else {
		ptr1->next = malloc(sizeof(struct obj));	
		if (ptr1->next == NULL){
			throwException2(out_of_mem, "out of memory in addObj during parsing XPath");
			//e.et = out_of_mem;
			//Throw e;
		} 	
		ptr1 = ptr1->next;
	}
		ptr1->object = o;
		ptr1->next = NULL;
	
}
void freeAllObj(){
	ptr1 = objList;
	while(ptr1!=NULL){
		free(ptr1->object);
		ptr2 = ptr1->next;
		free(ptr1);
		ptr1 = ptr2;
	}
	ptr1 = ptr2 = objList = NULL;
}
void resetObj(){
	ptr1 = objList;
	while(ptr1!=NULL){
		ptr2 = ptr1->next;
		free(ptr1);
		ptr1 = ptr2;
	}
	ptr1 = ptr2 = objList = NULL;
}
