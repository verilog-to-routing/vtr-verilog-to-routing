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
#ifndef AL_H
#define AL_H

#include "customTypes.h"
#include <stdlib.h>
// This define the arrayList struct and functions
// arrayList grows automatically when new buffers are added
// functions defined in this file allows one to create, de-allocate
// and add elements to the arrayList


#define AL_GROW_INC 16;
typedef struct arrayList {
	int capacity; // capacity of the arrayList
	int size;   // actual size
	void **storage; // where object pointers are stored
	               // storage needs to grow automatically
}ArrayList;

// that is all we need for now, more can be defined later

// allocate arrayList with initial capacity of 10, 
// return NULL if allocation failed
ArrayList* createArrayList(); 						  

// allocate arrayList with initial capacity, 
// return NULL if allocation failed
ArrayList* createArrayList2(int initialCapacity); 

// garbage collect arrayList
void freeArrayList(ArrayList* al); 

// add the element to the end of the storage, 
// return the status of add
// 1 if ok, 0 if failed
int add(ArrayList* al, void* element);

// get the object pointer at the index position
//(user program has to perform out-of-range checking)
//void* get(ArrayList* al, int index);
#define get(al,index) al->storage[index]
#endif
