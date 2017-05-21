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
/* intHash implements a simple hash table that speeds up the
   uniqueness checking */
#include "xpath.h"

/* the constructor */
IntHash* createIntHash(){
	/*int i=0;
	IntHash *ih = (IntHash *) malloc(sizeof(IntHash));
	if (ih==NULL) {
		throwException2(out_of_mem,
			"IntHash allocation failed ");
	}
	ih->storage = (FastIntBuffer **) malloc(sizeof(FastIntBuffer*)<<ih_hashWidthE);
	ih->hw = ih_hashWidth;
	ih->m1 = ih_mask1;
	ih->m2 = ih_mask2;
	ih->maxDepth = 0;
	ih->pse = ih_pageSizeE;*/
	/* initialize everything to null */
	/*for (i=0;i<ih->hw;i++){
		ih->storage[i]= NULL;
	}*/
	// default size is 2^11=2048
	return createIntHash2(0);
}
IntHash* createIntHash2(int hashWidthExpo){
	int i=0;
	IntHash *ih = (IntHash *) malloc(sizeof(IntHash));
	if (ih==NULL) {
		throwException2(out_of_mem,
			"IntHash allocation failed ");
		return NULL;
	}
	ih->storage = (FastIntBuffer **) malloc(sizeof(FastIntBuffer*)<<hashWidthExpo);
	ih->hw = 1<<hashWidthExpo;
	ih->m1 = ih->hw -1;
	ih->m2 = (~ih->m1) & 0xffffffff;
	ih->maxDepth = 0;
	ih->pse = ih_pageSizeE;
	/* initialize everything to null */
	for (i=0;i<ih->hw;i++){
		ih->storage[i]= NULL;
	}
	return ih;
}
/* free intHash */
void freeIntHash(IntHash *ih){
	int i=0;
	if (ih !=NULL){
		for (i=0;i<=ih->maxDepth;i++){
			freeFastIntBuffer(ih->storage[i]);
		}
		free(ih->storage);
		free(ih);
	}
}


/* Test whether the input i is unique;
   if not, insert into the hash table and return false
   otherwise, return true */
Boolean isUniqueIntHash(IntHash *ih,int i){
	int j,size;
	int temp = i & ih->m1;
	if (temp>ih->maxDepth){
		ih->maxDepth = temp;
	}
	if (ih->storage[temp]==NULL) {
		ih->storage[temp]= createFastIntBuffer2(ih->pse);
		if (ih->storage[temp]==NULL) {
			throwException2(out_of_mem,
				"FastIntBuffer allocation failed ");
		}
		appendInt(ih->storage[temp],i);
		return TRUE;
	}
	else{
		size = ih->storage[temp]->size;
		for (j=0;j<size;j++){
			if (i == intAt(ih->storage[temp],j)){
				return FALSE;
			}
		}
		appendInt(ih->storage[temp],i);
		return TRUE;
	}
}

/* reset intHash */
void resetIntHash(IntHash *ih){
	int i=0;
	for (i=0;i<=ih->maxDepth;i++){
		if (ih->storage[i]!=NULL){
			clearFastIntBuffer(ih->storage[i]);
		}
	}
}

int determineHashWidth(int i){
	if (i<(1<<8))
		return 3;
	if (i<(1<<9))
		return 4;
	if (i<(1<<10))
		return 5;
	if (i<(1<<11))
		return 6;
	if (i<(1<<12))
		return 7;
	if (i<(1<<13))
		return 8;
	if (i<(1<<14))
		return 9;
	if (i<(1<<15))
		return 10;
	if (i<(1<<16))
		return 11;
	if (i<(1<<17))
   	    return 12;
   	if (i<(1<<18))
   	    return 13;
   	if (i<(1<<19))
   	    return 14;
   	if (i<(1<<20))
   	    return 15;
   	if (i<(1<<21))
   	    return 16;
   	if (i<(1<<22))
   	    return 17;
   	if (i<(1<<23))
   	    return 18;
   	if (i<(1<<25))
   	    return 19;
   	if (i<(1<<27))
   	    return 20;
   	if (i<(1<<29))
   	    return 21;
   	return 22;

}
