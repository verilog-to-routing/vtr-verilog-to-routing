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
#include "fastIntBuffer.h"
static void quickSort_ascending(FastIntBuffer *fib,int i1, int i2);
static void quickSort_descending(FastIntBuffer *fib,int i1, int i2);
/* Create FastIntBuffer with initial page size of 1024 ints */
FastIntBuffer *createFastIntBuffer(){
	FastIntBuffer *fib = NULL;
	ArrayList *al= createArrayList();
	if (al==NULL){
		throwException2(out_of_mem,
			"FastIntBuffer allocation failed ");
		return NULL;
	}

	fib = (FastIntBuffer *)malloc(sizeof(FastIntBuffer));
	if (fib==NULL) {
		freeArrayList(al);
		throwException2(out_of_mem,
			"FastIntBuffer allocation failed ");
		return NULL;
	}

	fib->size = 0;
	fib->capacity = 0;
	fib->pageSize = 1<<10;
	fib->exp = 10;
	fib->r = 1023;
	fib->al = al;
	return fib;
}

/* Create FastIntBuffer with initial page size of (1<<e) ints */
FastIntBuffer *createFastIntBuffer2(int exp){
	FastIntBuffer *fib = NULL;
	ArrayList *al= createArrayList();
	if (al==NULL){
		throwException2(out_of_mem,
			"FastIntBuffer allocation failed ");
	}
	fib = (FastIntBuffer *)malloc(sizeof(FastIntBuffer));
	if (fib==NULL) {
		freeArrayList(al); 
		throwException2(out_of_mem,
			"FastIntBuffer allocation failed ");
	}

	fib->size = 0;
	fib->capacity = 0;
	fib->pageSize = 1<<exp;
	fib->exp = exp;
	fib->r = (1<<exp) - 1;
	fib->al = al;
	return fib;
}

/* Free FastIntBuffer */
void freeFastIntBuffer(FastIntBuffer *fib){
	if (fib != NULL) {
		freeArrayList(fib->al);
		free(fib);
	}
}

/* Append i to the end of FastIntBuffer */
void appendInt(FastIntBuffer *fib, int i){
	//int lastBufferIndex;
	/*int* lastBuffer = NULL;
    if (fib->al->size == 0) {
		lastBuffer = (int *)malloc(sizeof(int)<<fib->exp);
		if (lastBuffer == NULL){
			throwException2(out_of_mem,
				" append int failed in FastIntBuffer");
		}
        add(fib->al,lastBuffer);
        fib->capacity = fib->pageSize;
    } else {
		lastBufferIndex = min((fib->size>>fib->exp),fib->al->size-1);
		lastBuffer = (int *)get(fib->al,lastBufferIndex);

    }*/
    if (fib->size < fib->capacity) {
		((int *)get(fib->al,fib->size>>fib->exp))[fib->size & fib->r] = i;
        //lastBuffer[fib->size & fib->r] = i;
        fib->size += 1;
    } else
        {
		int *newBuffer = (int *)malloc(sizeof(int)<<fib->exp);
		if (newBuffer == NULL){
			throwException2(out_of_mem,
				" append int failed in FastIntBuffer");
		}
		fib->size++;
		fib->capacity += fib->pageSize;
		add(fib->al,newBuffer);
        newBuffer[0] = i;		
	}
}

/* Append int array of length "len" to the end of fastIntBuffer */
void appendIntArray(FastIntBuffer *fib, int* int_array, int len){
	int lastBufferIndex;
	int *lastBuffer=NULL;
	if (int_array == NULL || len <0) {
		throwException2(invalid_argument,
			"invalid argument for appendIntArray in FastIntBuffer");
	}

	if (fib->al->size == 0){
		lastBuffer = (int *)malloc(sizeof(int)<<fib->exp);
		if(lastBuffer==NULL){
			throwException2(out_of_mem,
				"out of memory for appendIntArray in FastIntBuffer");
		}       
		add(fib->al,lastBuffer);
		lastBufferIndex = 0;
		fib->capacity = fib->pageSize;
    } else {
		lastBufferIndex = min((fib->size>>fib->exp),fib->al->size-1);
		lastBuffer = (int *)get(fib->al,lastBufferIndex);
    }

    if ((fib->size + len) < fib->capacity) {
		if (fib->size + len <(lastBufferIndex+1)<<fib->exp){
			memcpy(lastBuffer+(fib->size&fib->r), int_array, len<<2);
		} 
		else {
			int offset = fib->pageSize -(fib->size&fib->r);
			int l = len - offset;
			int k = (l)>>fib->exp;
			int z;
			memcpy(lastBuffer+(fib->size & fib->r),
				int_array,offset);
			for (z=1;z<=k;z++){
				memcpy(get(fib->al,lastBufferIndex+z),
					int_array+offset, fib->pageSize<<2);
				offset += fib->pageSize;
			}
			memcpy(get(fib->al,lastBufferIndex+z),
				int_array + offset, l & fib->r);

		}
        fib->size += len;
		return;
    } else /* new buffers needed*/
        {
		int i;
		int n = ((len + fib->size)>>fib->exp)
			+(((len + fib->size) & fib->r) > 0 ? 1:0)
			- (fib->capacity >> fib->exp);
        /* create these buffers*/
        /* add to bufferArrayList */
		memcpy(lastBuffer+(fib->size & fib->r), 
			int_array,
			fib->capacity - fib->size);

        for (i = 0; i < n; i++) {
			int *newBuffer = (int *)malloc(sizeof(int)<<fib->exp);
            if (i < n - 1) {
				memcpy(newBuffer,
					int_array + (i<<fib->exp) + fib->capacity - fib->size,
					fib->pageSize<<2);
            } else {
				memcpy(newBuffer,
					int_array + (i<<fib->exp) + fib->capacity - fib->size,
					(len + fib->size - fib->capacity - (i<<fib->exp))<<2);
            }
			add(fib->al,newBuffer);
        }
        fib->size += len;
        fib->capacity += (n << fib->exp);
    }
}

/* Get the int array corresponding to content of FastIntBuffer 
   with the starting offset and len*/
int *getIntArray(FastIntBuffer *fib, int offset, int len){
	int *result;
	int first_index, last_index;
	if (fib->size <= 0 || 
		offset < 0 || 
		(offset+len)>fib->size) {
			throwException2(invalid_argument,
				"getIntArray in FastIntBuffer failed ");
    }
    result = (int *)malloc(len*sizeof(int)); // allocate result array
	if (result == NULL){
		throwException2(out_of_mem,
			"getIntArray in FastIntBuffer failed ");
	}

	first_index = offset >> fib->exp;
	last_index = (offset + len)>>fib->exp;
	if (((offset + len) & fib->r) == 0){
		last_index--;
	}

    if (first_index == last_index) {
		memcpy(result,
			(int *)get(fib->al,first_index)+ (offset & fib->r),
			len<<2);
    } else {
        int int_array_offset = 0;
		int i;
		int *currentChunk;
        for (i = first_index; i <= last_index; i++) {
            currentChunk = (int *)get(fib->al,i);
			if (i == first_index)
                {
				memcpy(result,
					currentChunk + (offset & fib->r),
					(fib->pageSize - (offset & fib->r))<<2);
				int_array_offset += fib->pageSize  - (offset & fib->r);
            } else if (i == last_index) // last sections
                {
					memcpy(result + int_array_offset,
						currentChunk,
						(len - int_array_offset)<<2);

            } else {
				memcpy(result + int_array_offset,
					currentChunk,
					fib->pageSize <<2);
                int_array_offset += fib->pageSize;
            }
        }
    }
    return result;
}



/* convert the content of FastIntBuffer to int */
int* toIntArray(FastIntBuffer *fib){
	if (fib->size > 0) {
		int i;
		int array_offset = 0;
		int *resultArray = (int *)malloc(sizeof(int)*fib->size);
		if (resultArray == NULL){
			throwException2(invalid_argument,
				"toIntArray in FastIntBuffer failed ");
		}
        
        for (i = 0; i<fib->al->size;i++)
		{
			/*System.arraycopy(
                (int[]) bufferArrayList.get(i),
                0,
                resultArray,
                array_offset,
                (i == (bufferArrayList.size() - 1)) ? (size& r) : pageSize);*/
			memcpy(resultArray + array_offset, 
				(int *) get(fib->al,i),
				(i == fib->al->size -1)? (fib->size & fib->r) : fib->pageSize);
//            (i == (bufferArrayList.size() - 1)) ? size() % pageSize : pageSize);
            array_offset += fib->pageSize;
        }
        return resultArray;
    }
	return NULL;
}

/* Get the int at the index position of FastIntBuffer */
/*int intAt(FastIntBuffer *fib, int index){    
	if (index < 0 || index > fib->size - 1) {
		throwException2(invalid_argument,
			"invalid index range");
    }
	return ((int *) get(fib->al,index>>fib->exp))[index & fib->r];
}*/

/* Replace the value at the index position of FastIntBuffer 
   with newVal */
/*void modifyEntryFIB(FastIntBuffer *fib, int index, int newVal){    
	if (index < 0 || index > fib->size - 1) {
		throwException2(invalid_argument,
			"invalid index range");
    }
	((int *) get(fib->al,index>>fib->exp))[index & fib->r] = newVal;
}*/

// set the buffer size to zero, capacity untouched,
/*void clearFastIntBuffer (FastIntBuffer *fib){
	fib->size = 0;
}*/

/* Resize the fastIntBuffer, return true if resized successfully, otherwise,
  the size doesn't change, return false*/
Boolean resizeFIB(FastIntBuffer *fib, int newSz){
	 if (newSz <= fib->capacity && newSz >=0){
		 fib->size = newSz;
		 return TRUE;
	 }	 
	 else
		 return FALSE;
}


void sortFIB(FastIntBuffer *fib,sortType order){
	switch (order) {
		case ASCENDING:
			if (fib->size > 0)
				quickSort_ascending(fib,0, fib->size - 1);
			break;
		case DESCENDING:
			if (fib->size > 0)
				quickSort_descending(fib,0, fib->size - 1);
			break;
		default:
			throwException2( invalid_argument, "Sort type undefined");
		}
}

void quickSort_ascending(FastIntBuffer *fib,int lo, int hi){
     int i=lo, j=hi; 
     int h;
     //Object o;
     int x= intAt(fib,(lo+hi)/2);
     //  partition
     do
     {    
         while (intAt(fib,i)<x) i++; 
         while (intAt(fib,j)>x) j--;
         if (i<=j)
         {
             h=intAt(fib,i);
             modifyEntryFIB(fib,i,intAt(fib,j)); 
             modifyEntryFIB(fib,j,h);   
             i++; 
             j--;
         }
     } while (i<=j);
     //  recursion
     if (lo<j) quickSort_ascending(fib,lo, j);
     if (i<hi) quickSort_ascending(fib,i, hi);
}


void quickSort_descending(FastIntBuffer *fib,int lo, int hi){
     int i=lo, j=hi; 
     int h;
     //Object o;
     int x=intAt(fib,(lo+hi)/2);
     //  partition
     do
     {    
         while (intAt(fib,i)>x) i++; 
         while (intAt(fib,j)<x) j--;
         if (i<=j)
         {
             h=intAt(fib,i);
             modifyEntryFIB(fib,i,intAt(fib,j)); 
             modifyEntryFIB(fib,j,h);   
             i++; 
             j--;
         }
     } while (i<=j);
     //  recursion
     if (lo<j) quickSort_descending(fib,lo, j);
     if (i<hi) quickSort_descending(fib,i, hi);
}