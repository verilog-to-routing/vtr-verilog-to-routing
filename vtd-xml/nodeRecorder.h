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
#ifndef NODERECORDER_H
#define NODERECORDER_H
#include "vtdNav.h"

typedef struct nodeRecorder{
	VTDNav *vn;
	FastIntBuffer *fib;
	int position;
	int size;
	int count;	
} NodeRecorder;

NodeRecorder* createNodeRecorder2();
NodeRecorder* createNodeRecorder(VTDNav *vn);
void freeNodeRecorder(NodeRecorder *nr);
void record(NodeRecorder *nr);
void resetPointer(NodeRecorder *nr);
void clearNodeRecorder(NodeRecorder *nr);
int iterateNodeRecorder(NodeRecorder *nr);
void bind4NodeRecorder(NodeRecorder *nr,VTDNav *vn);
#endif