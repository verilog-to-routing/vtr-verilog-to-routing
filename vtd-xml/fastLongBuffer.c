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
#include "fastLongBuffer.h"

/* create FastLongBuffer with default page size of 1024 longs */
FastLongBuffer *createFastLongBuffer(){
	FastLongBuffer *flb = NULL;
	ArrayList *al= createArrayList();
	if (al==NULL) {
		throwException2(out_of_mem,
			"FastLongBuffer allocation failed ");
		return NULL;
	}

	flb = (FastLongBuffer *)malloc(sizeof(FastLongBuffer));
	if (flb==NULL) {
		freeArrayList(al); 
		throwException2(out_of_mem,
			"FastLongBuffer allocation failed ");
		return NULL;
	}

	flb->size = 0;
	flb->capacity = 0;
	flb->pageSize = 1<<10;
	flb->exp = 10;
	flb->r = 1023;
	flb->al = al;

	return flb;
}
/* create FastLongBuffer with page size of (1<<e) longs*/
FastLongBuffer *createFastLongBuffer2(int exp){
	FastLongBuffer *flb = NULL;
	ArrayList *al= createArrayList();
	if (al==NULL){
		throwException2(out_of_mem,
			"FastLongBuffer allocation failed ");
		return NULL;
	}

	flb = (FastLongBuffer *)malloc(sizeof(FastLongBuffer));
	if (flb==NULL) {
		freeArrayList(al); 
		throwException2(out_of_mem,
			"FastLongBuffer allocation failed ");
		return NULL;
	}

	flb->size = 0;
	flb->capacity = 0;
	flb->pageSize = 1<<exp;
	flb->exp = exp;
	flb->r = (1<<exp)-1;
	flb->al = al;

	return flb;
}
/* create FastLongBuffer with page size of (1<<e) longs and initial capciaty of c longs*/
FastLongBuffer *createFastLongBuffer3(int exp, int c){
	FastLongBuffer *flb = NULL;
	ArrayList *al= createArrayList2(c);
	if (al==NULL){
		throwException2(out_of_mem,
			"FastLongBuffer allocation failed ");
	}

	flb = (FastLongBuffer *)malloc(sizeof(FastLongBuffer));
	if (flb==NULL) {
		free(al); 
		throwException2(out_of_mem,
			"FastLongBuffer allocation failed ");
	}

	flb->size = 0;
	flb->capacity = 0;
	flb->pageSize = 1<<exp;
	flb->exp = exp;
	flb->r = (1<<exp)-1;
	flb->al = al;

	return flb;
}
/* free FastLongBuffer */
void freeFastLongBuffer(FastLongBuffer *flb){
	if (flb != NULL) {
		freeArrayList(flb->al);
		free(flb);
	}
}
/* append a long array to the end of FastLongBuffer */
void appendLongArray(FastLongBuffer *flb, Long *longArray, int len){
	Long *lastBuffer = NULL;
	int lastBufferIndex;

	if (longArray == NULL || len <0) {
		throwException2(invalid_argument,
			"invalid argument for appendLongArray /n");
    }
    /* no additional buffer space needed */
	if (flb->al->size == 0)
	{
		lastBuffer = (Long *) malloc(sizeof(Long)<< flb->exp);
		if (lastBuffer == NULL){
			throwException2(out_of_mem,
				" appendLongArray failed to allocate mem ");
		}
		add(flb->al,lastBuffer);
		lastBufferIndex = 0;
		flb->capacity = flb->pageSize;
	}else {
		lastBufferIndex = min((flb->size>>flb->exp),flb->al->size-1);
		lastBuffer = (Long *)get(flb->al, lastBufferIndex);
	}


	if ((flb->size + len)< flb->capacity){
		if (flb->size + len <(lastBufferIndex+1)<<flb->exp){
			memcpy(lastBuffer+(flb->size&flb->r), longArray, len<<3);
		} 
		else {
			int offset = flb->pageSize -(flb->size&flb->r);
			int l = len - offset;
			int k = (l)>>flb->exp;
			int z;
			memcpy(lastBuffer+(flb->size & flb->r),
				longArray,offset<<3);
			for (z=1;z<=k;z++){
				memcpy(get(flb->al,lastBufferIndex+z),
					longArray+offset, flb->pageSize<<3);
				offset += flb->pageSize;
			}
			memcpy(get(flb->al,lastBufferIndex+z),
				longArray + offset, (l & flb->r)<<3);

		}
		flb->size += len;
		return;
    } else
        {
		int i;
		Long *newBuffer = NULL;

		int n = ((len + flb->size) >> flb->exp) 
			+(((len + flb->size) & flb->r)> 0 ? 1 : 0)
			- (flb->capacity >>flb->exp);
       
		memcpy(lastBuffer + (flb->size & flb->r),
			longArray,
			(flb->capacity - flb->size)<<3);

        for (i = 0; i < n; i++) {
			newBuffer = (Long *)malloc(sizeof(Long)<<flb->exp);
			if (newBuffer == NULL){
				throwException2(out_of_mem,
					" appendLongArray failed to allocate mem ");
			}
            if (i < n - 1) {
				memcpy( newBuffer,
					longArray + (i<<flb->exp) + flb->capacity - flb->size,
					flb->pageSize << 3);

            } else {
				memcpy(newBuffer,
					longArray + (i<<flb->exp) + flb->capacity - flb->size,
					(len + flb->size - (i<< flb->exp) - flb->capacity)<<3);
            }
			add(flb->al,newBuffer);
        }
		flb->size += len;
		flb->capacity += (n << flb->exp);
    }
}

/* append a long to the end of FastLongBuffer */
void appendLong(FastLongBuffer *flb, Long i){
	//Long *lastBuffer = NULL;
	//int lastBufferIndex;

	/*if (flb->al->size == 0) {
		lastBuffer = (Long *)malloc(sizeof(Long)<<flb->exp);
		if (lastBuffer == NULL){
			throwException2(out_of_mem,
				"AppendLong failed to allocate mem");
		}
		add(flb->al,lastBuffer);
		flb->capacity = flb->pageSize;
	}else{
		lastBufferIndex = min((flb->size>>flb->exp),flb->al->size-1);
		lastBuffer = (Long *)get(flb->al, lastBufferIndex);
	}*/

	if (flb->size < flb->capacity){
		((Long *)get(flb->al,flb->size >> flb->exp))[flb->size & flb->r] = i;
		//lastBuffer[flb->size & flb->r] = i;
		flb->size ++;
	}else {
		Long *newBuffer = (Long *)malloc(sizeof(Long)<<flb->exp);
		if (newBuffer == NULL){
			throwException2(out_of_mem,
				"AppendLong failed to allocate mem");
		}
		
		flb->size ++;
		flb->capacity += flb->pageSize;
		add(flb->al, newBuffer);
		newBuffer[0]=i;
	}
}

/* Return a selected chuck of long buffer as a long array.*/
Long *getLongArray(FastLongBuffer *flb, int offset, int len){
	Long *result = NULL;
	int first_index, last_index;
	if (flb->size <= 0 
		|| offset < 0 
		|| len <0 
		|| offset + len > flb->size) {
			throwException2(invalid_argument,
				"Invalid argument for getLongArray in FastLongBuffer");
		}
   
	result = (Long *)malloc(sizeof(Long)*len);

	if (result == NULL){
		throwException2(out_of_mem,
			" getLongArray failed to allocate mem in FastLongBuffer");
	}
	first_index = offset >> flb->exp;
	last_index = (offset+len) >> flb->exp;

	if (((offset + len) & flb->r)== 0){
		last_index--;
	}

    if (first_index == last_index) {
		memcpy(result, 
			(Long *)get(flb->al, first_index) + (offset & flb->r),
			len <<3);
    } else {
        int long_array_offset = 0;
		int i;
		Long *currentChunk;
        for (i = first_index; i <= last_index; i++) {
			currentChunk = (Long *)get(flb->al, i);
            if (i == first_index)
                {
         
				memcpy(result,
					currentChunk + (offset & flb->r),
					(flb->pageSize - (offset& flb->r))<<3);
				long_array_offset += flb->pageSize - (offset & flb->r);
            } else if (i == last_index)
                {
                memcpy(result + long_array_offset,
					currentChunk,
					(len - long_array_offset) <<3);


            } else {
                memcpy( result + long_array_offset,
					currentChunk,
					flb->pageSize <<3 );
				long_array_offset += flb->pageSize;
            }
		}
	}
		return result;
}

/* convert FastLongBuffer into a Long array */
Long* toLongArray(FastLongBuffer *flb){	
	Long *resultArray = NULL;
	int i;
    if (flb->size > 0) {
	// if (size()>0) {
        //long[] resultArray = new long[size];
		int k;
        int array_offset = 0;
		resultArray = (Long *)malloc(sizeof(Long)*flb->size);
		if (resultArray == NULL){
			throwException2(out_of_mem,
				"toLongArray failed to allocate mem");
		}
        //copy all the content int into the resultArray

		k = flb->al->size;
        //for (i = 0; i < bufferArrayList.size(); i++) {
		for (i=0; i< k;i++){
            /*System.arraycopy(
                (int[]) bufferArrayList.get(i),
                0,
                resultArray,
                array_offset,
                //(i == (bufferArrayList.size() - 1)) ? size - ((size>>exp)<<exp) : pageSize);
                (i == (bufferArrayList.size() - 1)) ? (size & r) : pageSize); */
			memcpy(resultArray + array_offset,
				(Long *)get(flb->al, i),
				((i == (flb->al->size - 1) ) ? ( flb->size & flb->r) : flb->pageSize )<<3); 
            array_offset += flb->pageSize;
        }
        return resultArray;
    }
    return NULL;
}
// get the long at the index position from FastLongBuffer
//Long longAt(FastLongBuffer *flb, int index);
/*Long longAt(FastLongBuffer *flb, int index){
	int pageNum,offset;
	if (index < 0 || index > flb->size - 1) {
		throwException2(invalid_argument,
			"invalid index range");
    }
	pageNum = (index >>flb->exp);
    offset = index & flb->r;
	return ((Long *)get(flb->al,pageNum))[offset];
}*/

// get the lower 32 bits from the index position from FastLongBuffer
/*int lower32At(FastLongBuffer *flb, int index){
	int pageNum,offset;
    if (index < 0 || index > flb->size) {
		throwException2(invalid_argument,
			" invalid index range");
    }
    pageNum =  (index >> flb->exp);
    offset = index & flb->r;
	return (int)((Long *)get(flb->al,pageNum))[offset];
}*/


// get the upper 32 bits from the index position from FastLongBuffer 
/*int upper32At(FastLongBuffer *flb, int index){
	int pageNum, offset;
    if (index < 0 || index > flb->size) {
		throwException2(invalid_argument,
			" invalid index range");
    }
    pageNum = (index >>flb->exp);
    offset = index & flb->r;
 	return (int) ((((Long *)get(flb->al,pageNum))[offset] & (((Long)0xffffffffL)<<32))>>32);
}*/

// replace the entry at the index position of FastLongBuffer with l
/*void modifyEntryFLB(FastLongBuffer *flb, int index, Long l){
    if (index < 0 || index > flb->size) {
		throwException2(invalid_argument,
			" invalid index range");
    }
	((Long *)get(flb->al,index>>flb->exp))[index & flb->r] = l;
}*/

// set the buffer size to zero, capacity untouched,
/*void clearFastLongBuffer (FastLongBuffer *flb){
	flb->size = 0;
}*/

// resize
Boolean resizeFLB(FastLongBuffer *flb, int newSz){
	if (newSz <= flb->capacity && newSz >=0){	
		flb->size = newSz;
		return TRUE; 
	}
	else	
		return FALSE;  
}