/* 
 * Copyright (C) 2002-2007 XimpleWare, info@ximpleware.com
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
// vtd-xml.cpp : Defines the entry point for the console application.
//

#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../xpath.h"
#include "../helper.h"
#include "../vtdGen.h"
//#include "lex.yy.c"
//#include "l8.tab.c"


// since 2.10 declare exception context as thread local storage
_thread struct exception_context the_exception_context[1];


#if 1
void main(){
	wchar_t* tests[] =	{
L"/descendant::test:*",
L"/descendant::test:*",
L"/descendant::test:e",
L"/descendant::test:f",
L"/root/test:*/text()",
L"a/d[2]/e",
L"*[c or d]",
L"a/c[d]",
L"a/c[d=\"Text for D\"]",
L"a[3][@a1=\"va1\"]",
L"a[@a1=\"va1\"][1]",
L"a[@a1=\"va1\"]",
L"/root/a/c/../@a1",
L"/root/a/..",
L".//c",
L".",
L"//c/d",
L"//c",
L"/root/a//d",
L"/root/a/d",
L"//a",
L"//d",
L"a/d[2]/e",
L"*/d",
L"a[last()]",
L"a[1]",
L"/root/a[1]/@*",
L"/root/a[1]/@a1",
L"text()",
L"*",
L"a",
L"/root",
L"child::*[self::a or self::b][position()=last() - 1]",
L"child::*[self::a or self::b]",
L"child::a/child::c[child::d='Text for D']",
L"child::a/child::c[child::d]",
L"child::a[position()=1][attribute::a1=\"va1\"]",
L"child::a[attribute::a1=\"va1\"][position()=1]",
L"child::a[attribute::a2=\"va2\"]",
L"child::a/child::d[position()=2]/child::e[position()=1]",
L"/descendant::a[position()=2]",
L"child::a[1]/child::d[1]/preceding-sibling::c[position()=1]",
L"child::a[2]/following-sibling::b[position()=1]",
L"child::a[position()=2]",
L"child::a[position()=last()-1]",
L"child::a[position()=last()]",
L"child::a[position()=1]",
L"/descendant::a/child::c",
L"/",
L"/descendant::a",
L"/child::root/child::*/child::d",
L"child::b/descendant::a",
L"self::root",
L"/child::root/child::a/descendant-or-self::a",
L"/child::root/child::a/child::c/ancestor::root",
L"/child::root/descendant::a",
L"/child::root/child::a/attribute::*",
L"/child::root/child::a/attribute::a1",
L"/child::root/child::node()",
L"/child::root/child::a/child::d/child::text()",
L"/child::root/child::*",
L"/child::root/child::a",
L"/root/a[(1+1-1)*2 div 2]",
L"/child::root/child::a/ancestor-or-self::a",
L"'hello'",
L"1+2-3+count(/a/b/c)"
};
					

	AutoPilot *ap = NULL;
	int ii = 0,i;
	//FILE *f = NULL;
	//UByte *xml = NULL;
	//VTDGen *vg = NULL;
	VTDNav *vn = NULL;
	//struct stat s;
	exception e;
	int a,result;
	expr *expression = NULL;
	double d= 0;

	Try{
		ap = createAutoPilot2();
		declareXPathNameSpace(ap,L"test",L"jimmy");
		//declareXPathNameSpace(ap,L"c",L"larry");
		for(i=0;i<66;i++){
		//selectXPath(ap,L"/a:b/c:d ");
			wprintf(L"i ==> %d \n",i);
			wprintf(L"input test string ==> %ls\n",tests[i]);
			if (selectXPath(ap, tests[i])){
				//setVTDNav(ap,vn);
				wprintf(L"output string ===>");
				printExprString(ap);
				wprintf(L"\n");				
			} else {
				wprintf(L" XPath parsing failed \n");
			}

		}
		wprintf(L" \n\n ************** \n");

	}Catch(e){
		wprintf(L"%s\n",e.msg);
	}
	//freeVTDGen(vg);
	//freeVTDNav(vn);
	freeAutoPilot(ap);
}
#endif

