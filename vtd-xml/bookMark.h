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
#ifndef BM_H
#define BN_H

#include "customTypes.h"

#include "cexcept.h"
#include "xpath.h"
/**
 * Bookmark is a single instance of a node position.
 * You can save the cursor's position into a bookMark instance
 * You can also point the cursor to the node position of  previously 
 * saved bookMark instance. 
 * 
 */
typedef struct bookMark{
	int* ba;
	int ba_len;
	VTDNav *vn1;
} BookMark;

/* constructor for bookMark */
BookMark *createBookMark();
BookMark *createBookMark2(VTDNav *vn);

void freeBookMark(BookMark *bm);
void unbind4BookMark(BookMark *bm);
void bind4BookMark(BookMark *bm,VTDNav *vn);
VTDNav* getNav4BookMark(BookMark *bm);
Boolean setCursorPosition(BookMark *bm, VTDNav *vn);
Boolean setCursorPosition2(BookMark *bm);
/**
* Record the cursor position
* This method is implemented to be lenient on loading in
* that it can load nodes from any VTDNav object
* if vn is null, return false
*/
Boolean recordCursorPosition(BookMark *bm, VTDNav *vn);
/**
* Record cursor position of the VTDNav object as embedded in the
* bookmark
*/
Boolean recordCursorPosition2(BookMark *bm);
/**
* Compare the bookmarks to ensure they represent the same
* node in the same VTDnav instance
*/
Boolean equal4BookMark(BookMark *bm1, BookMark *bm2);
/**
* Returns the hash code which is a unique integer for every node
*/
int hashCode4BookMark(BookMark *bm);
/**
* Compare the bookmarks to ensure they represent the same
* node in the same VTDnav instance
*/
Boolean deepEqual4BookMark(BookMark *bm1, BookMark *bm2);
#endif