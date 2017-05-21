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

#ifndef ELEMENT_FRAGMENT_NS_H
#define ELEMENT_FRAGMENT_NS_H
#include "vtdNav.h"
#include "transcoder.h"

typedef struct elementFragmentNs {
  VTDNav *vn;
  Long l;
  FastIntBuffer *fib;
  int stLen;
} ElementFragmentNs;

static Boolean ws_init = FALSE;
static UByte ws[5]= {0,' ',0,'=',0};
ElementFragmentNs* createElementFragmentNs();
void freeElementFragmentNs(ElementFragmentNs *ef);
int getFragmentSize(ElementFragmentNs *ef);
int getFragmentSize2(ElementFragmentNs *ef, encoding_t encoding);

Long getOffsetLeng(ElementFragmentNs *ef);
UByte* toFragmentBytes(ElementFragmentNs *ef);
UByte* toFragmentBytes2(ElementFragmentNs *ef, encoding_t dest_encoding);
void writeFragmentToFile(ElementFragmentNs *ef, FILE *f);
void writeFragmentToFile2(ElementFragmentNs *ef, FILE *f, encoding_t dest_encoding);

#endif