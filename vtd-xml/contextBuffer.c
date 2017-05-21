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
#include "contextBuffer.h"


/* Create ContextBuffer with incSize of i */
ContextBuffer *createContextBuffer(int i){
	ArrayList *al = createArrayList();
	ContextBuffer *cb = NULL;
	if (al==NULL) {
		throwException2(out_of_mem,
			"ContextBuffer allocation failed ");
	}
	cb = (ContextBuffer *)malloc(sizeof(ContextBuffer));
	if (cb == NULL || i<0){
		freeArrayList(al);
		throwException2(out_of_mem,
			"ContextBuffer allocation failed ");
	}
	cb->capacity = 0;
	cb->pageSize = 1024;
	cb->n = 10;
	cb->r = cb->pageSize -1;
	cb->incSize = i;
	cb->size = 0;

	cb->al = al;
	return cb;
}


// Create ContextBuffer with page size (1<<e) and increment Size of i
ContextBuffer *createContextBuffer2(int exp, int i){
	
	ArrayList *al = createArrayList();
	ContextBuffer *cb = NULL;
	if (al==NULL){
		throwException2(out_of_mem,
			"ContextBuffer allocation failed ");
		return NULL;
	}
	cb = (ContextBuffer *)malloc(sizeof(ContextBuffer));
	if (cb == NULL || exp<0 || i<0){
		freeArrayList(al);
		throwException2(out_of_mem,
			"ContextBuffer allocation failed ");
		return NULL;
	}

	cb->capacity = 0;
	cb->pageSize = (1<<exp);
	cb->n = exp;
	cb->r = cb->pageSize -1;
	cb->incSize = i;

	cb->size = 0;
	cb->al = al;
	return cb;
}


// Free ContextBuffer
void freeContextBuffer(ContextBuffer *cb){

	// no way to guard against a garbage pointer
	if (cb!=NULL) {
		freeArrayList(cb->al);
		free(cb);
	}
}


// Pop the content value back into an integer array.
Boolean load(ContextBuffer *cb, int* output){
	int startingOffset, len, first_index, last_index,i;
	//exception e;
	if (cb->size < cb->incSize) {
        return FALSE;
    }

    startingOffset = cb->size - cb->incSize;
    len = cb->incSize;


    first_index = (startingOffset >> cb->n);
    last_index = ((startingOffset + len) >>cb->n);
    if (((startingOffset + len)& cb->r)== 0) {
        last_index--;
    }

    if (first_index == last_index) {
        /* to see if there is a need to go across buffer boundry*/

		memcpy(output,
			((int *)get(cb->al,first_index))+((startingOffset & cb->r)),
			len<<2);
    } else {
        int int_array_offset = 0;
        for (i = first_index; i <= last_index; i++) {
            int* currentChunk = (int *) get(cb->al,i);
            if (i == first_index) /* first section */
                {
					memcpy(output,
						currentChunk+ ((startingOffset&cb->r)),
						(cb->pageSize - (startingOffset & cb->r))<<2);
              
                int_array_offset += cb->pageSize - (startingOffset &cb->r);
            } else if (i == last_index) /* last sections*/
                {
					memcpy(output+int_array_offset,
						currentChunk, 
						(len-int_array_offset)<<2);

            } else {
				memcpy(output+int_array_offset, currentChunk, (cb->pageSize)<<2);
                int_array_offset += cb->pageSize;
            }
        }
    }

    cb->size -= cb->incSize;
    return TRUE;
}


/* Push the array content on to the stack .*/
void store(ContextBuffer *cb, int *input){
	int *lastBuffer; 
	int lastBufferIndex;
	int i;
    
	if (cb->al->size == 0){
		lastBuffer = (int *)malloc(sizeof(int)*cb->pageSize);
		if (lastBuffer == NULL){
			throwException2(out_of_mem,
				"Fail to increase the size of ArrayList of ContextBuffer");
		}
		add(cb->al,lastBuffer);
		lastBufferIndex = 0;
        cb->capacity = cb->pageSize;
    } else {
		lastBufferIndex = min(cb->size>>cb->n, cb->al->size -1);
		lastBuffer = (int *)get(cb->al,lastBufferIndex);
    }

    if ((cb->size + cb->incSize) < cb->capacity) {
		if (cb->size + cb->incSize < ((lastBufferIndex+1)<<cb->n)){
			memcpy(lastBuffer+(cb->size& cb->r),input, cb->incSize <<2);
		}else {
			int offset = cb->pageSize - (cb->size & cb->r);
			int l = cb->incSize - offset;
			int k = l>>cb->n;
			int z;
			memcpy(lastBuffer+(cb->size& cb->r),input, 
				offset<<2);
			for (z=1;z<=k;z++){
				memcpy(get(cb->al,lastBufferIndex+z),input+offset,cb->pageSize<<2);
				offset += cb->pageSize;
			}
			memcpy(get(cb->al,lastBufferIndex+z),input+offset, (l & cb->r)<<2);
		}

        cb->size += cb->incSize;
		return;
    } else /* new buffers needed */
        {
        /* compute the number of additional buffers needed*/
        int k =
			((cb->incSize + cb->size)>>cb->n)+
			(((cb->incSize + cb->size)&cb->r)>0? 1 : 0)-
			(cb->capacity>>cb->n);
		memcpy(lastBuffer+((cb->size&cb->r)),input, (cb->capacity-cb->size)<<2);

        for (i = 0; i < k; i++) {
            int *newBuffer = (int *)malloc(cb->pageSize*sizeof(int));
			if (newBuffer == NULL){
				throwException2(out_of_mem,
					"Fail to increase the size of ArrayList of ContextBuffer");
			}
            if (i < k - 1) {
                // full copy 
                //System.arraycopy(input, pageSize * i + capacity - size, newBuffer, 0, pageSize);
				memcpy(newBuffer, 
					input + ((cb->pageSize*i + cb->capacity - cb->size)),
					cb->pageSize<<2);
					
            } else {
				memcpy(newBuffer,
					input + ((cb->pageSize * i + cb->capacity - cb->size)), 
					(cb->incSize+cb->size - cb->pageSize*i - cb->capacity)<<2);
            }
			add(cb->al,newBuffer);
        }
        cb->size += cb->incSize;
        cb->capacity += (k << cb->n);
		}
}


/* Manage the buffer size to reduce unused spaces */
void resize(ContextBuffer *cb){}
