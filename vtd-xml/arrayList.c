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
#include "arrayList.h"

//Implementation for functions defined in arrayList.h


// allocate arrayList with initial capacity of 10, 
// throw exception and return NULL if allocation failed
 ArrayList *createArrayList(){
	
	int i=0;
	ArrayList *al = (ArrayList*) malloc(sizeof(ArrayList));
	if (al==NULL) {
		throwException2(out_of_mem,
			"ArrayList allocation failed ");
	}
	
	al->storage = (void **)malloc(10 * sizeof(void *)); // initial capacity of 10;
	if (al->storage == NULL){
		free(al);		
		throwException2(out_of_mem,
			"ArrayList allocation failed ");
	}
	for ( i=0;i<10;i++){
		al->storage[i] = NULL;
	}
    al->capacity = 10;
    al->size = 0;
    return al;
}

 // create ArrayList with initialCapacity
 // return NULL if allocation failed

 ArrayList *createArrayList2(int initialCapacity){
	 int i;
	ArrayList *al = (ArrayList*) malloc(sizeof(ArrayList));
	if (al==NULL) {
		throwException2(out_of_mem,"ArrayList allocation failed ");
	}
	al->storage = (void **)malloc(initialCapacity * sizeof(void *)); // initial capacity of 10;
	if (al->storage == NULL){
		free(al);		
		throwException2(out_of_mem,
			"ArrayList allocation failed ");
	}

	for (i=0;i<initialCapacity;i++)
		al->storage[i] = NULL;
    al->capacity = initialCapacity;
    al->size = 0;
    return al;
}
// free ArrayList
 void freeArrayList(ArrayList *al){
	int i=0;
	if (al!=NULL){
	for (i=0;i<al->size;i++)
		free(al->storage[i]); // it is ok, only pointers to mem blocks are stored
		free(al->storage);
		free(al);
	}	
}

// add the element to the end of the storage, 
// return the status of add
// 1 if ok, 0 if failed
 int add(ArrayList *al, void *element){
	int t = 0,k=0;
	void **v=NULL;
	if (al->size < al->capacity){
		al->storage[al->size] = element;
		al->size++;
		return al->size;
	}
	else{
		t = al->capacity + AL_GROW_INC;
		v = (void **)malloc(t*sizeof(void *));
		if (v==NULL) {
			throwException2(out_of_mem,
				"ArrayList add failed ");
		} // add failed
	
		
		for (k=0;k<al->size;k++)
		{
               v[k] = al->storage[k]; // copy content
		}
		for (k=al->size;k<al->capacity;k++){
			v[k] = NULL;			// the remaining ones set to NULL
		}
		v[al->size]=element;
		al->capacity = t;
		al->size++;
		free(al->storage);
		al->storage = v;
		return al->size;
	}
}
// enforcing in-the-range index value is left to the application logic
// get the object pointer at the index position
//(user program has to perform out-of-range checking)

/* void *get(ArrayList *al, int index){
	return al->storage[index];
}*/
//#define get(al, index) al->storage[index]
