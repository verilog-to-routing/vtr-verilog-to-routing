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

#ifndef CONTEXTBUFFER_H
#define CONTEXTBUFFER_H

#include <stdlib.h>
#include "arrayList.h"
#include "customTypes.h"

//data structure for the global stack used in VTDNav
typedef struct contextBuffer{
	ArrayList *al;
	int capacity;
	int incSize;
	int n;
	int pageSize;
	int r;
	int size;
} ContextBuffer;

// Create ContextBuffer with incSize of i
ContextBuffer *createContextBuffer(int i);
// Create ContextBuffer with page size (1<<e) and increment Size of i
ContextBuffer *createContextBuffer2(int e, int i);
// Free ContextBuffer
void freeContextBuffer(ContextBuffer *cb);
// Pop the content value back into an integer array.
Boolean load(ContextBuffer *cb, int* output);
// Push the array content on to the stack.
void store(ContextBuffer *cb, int *input);
// Manage the buffer size to reduce unused spaces
void resize(ContextBuffer *cb);

#endif
