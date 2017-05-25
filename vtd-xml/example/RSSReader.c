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

// since 2.10 declare exception context as thread local storage
_thread struct exception_context the_exception_context[1];

int main(){

	exception e;
	FILE *f = NULL;
	int i = 0,t,result,count=0;
 	wchar_t *tmpString;	
	
	char* filename = "./servers.xml";
	struct stat s;
	UByte *xml = NULL; // this is the buffer containing the XML content, UByte means unsigned byte
	VTDGen *vg = NULL; // This is the VTDGen that parses XML
	VTDNav *vn = NULL; // This is the VTDNav that navigates the VTD records
	AutoPilot *ap = NULL;

	// allocate a piece of buffer then reads in the document content
	// assume "c:\soap2.xml" is the name of the file
	f = fopen(filename,"r");

	stat(filename,&s);

	i = (int) s.st_size;	
	wprintf(L"size of the file is %d \n",i);
	xml = (UByte *)malloc(sizeof(UByte) *i);
	i = fread(xml,sizeof(UByte),i,f);
	Try{
		vg = createVTDGen();
		setDoc(vg,xml,i);
		parse(vg,TRUE);
		vn = getNav(vg);
		ap = createAutoPilot2();
		declareXPathNameSpace(ap,L"ns1",L"http://purl.org/dc/elements/1.1/");
		if (selectXPath(ap,L"//ns1:*")){
			bind(ap,vn);
			while((result=evalXPath(ap))!= -1){
				wprintf(L"result is %d \n",result);
				tmpString = toString(vn,result);
                		wprintf(L"Element name ==> %ls \n",tmpString);
				free(tmpString);
				t = getText(vn);
				if (t!=-1){
					tmpString = toNormalizedString(vn,t);
					wprintf(L" text ==> %ls \n",tmpString);
					free(tmpString);
				}
				wprintf(L"\n =======================\n ");
				count ++;
			}
		}
		wprintf(L"\nTotal number of elements %d \n",count);
		fclose(f);
		// remember C has no automatic garbage collector
		// needs to deallocate manually.
		freeVTDNav(vn);
		freeVTDGen(vg);
		freeAutoPilot(ap);
	}
	Catch (e) {
		if (e.et == parse_exception)
			wprintf(L"parse exception e ==> %s \n %s\n", e.msg, e.sub_msg);	
		// manual garbage collection here
		freeVTDGen(vg);
	}
  return 0;
}	

