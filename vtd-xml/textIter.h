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
#ifndef TEXTITER_H
#define TEXTITER_H

#include "customTypes.h"
#include "vtdNav.h"
#include "cexcept.h"

typedef struct textIter{
	int prevLocation;
	int depth;
	int index;
	VTDNav *vn;
	int lcIndex;
	int lcLower;
	int lcUpper;
	int sel_type;
	UCSChar *piName;
} TextIter;

/* create TextIter */
TextIter *createTextIter();
/* free TextIter */
void freeTextIter(TextIter* ti);
/* Obtain the current navigation position and element info from VTDNav.
 * So one can instantiate it once and use it for many different elements */
void touch(TextIter *ti, VTDNav *vn);
/* Get the index vals for the text nodes in document order.*/
int getNext(TextIter *ti);
/* Ask textIter to return character data or CDATA nodes*/
void selectText(TextIter *ti);
/*  Ask textIter to return comment nodes*/
void selectComment(TextIter *ti);
/* Ask TextIter to return processing instruction name 
 * no value */
void selectPI0(TextIter *ti);
/* Ask TextIter to return processing instruction of 
given name */
void selectPI1(TextIter *ti, UCSChar *piName);

#endif
