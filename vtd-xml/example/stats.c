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
	int i = 0,count=0,par_count=0,v=0;
	
	char* filename = "./bioinfo.xml";
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
		bind(ap,vn);
		if (selectXPath(ap,L"/bix/package/command/parlist")){
			while(evalXPath(ap)!= -1){
				count++;
			}
		}
		
		if (selectXPath(ap,L"/bix/package/command/parlist/par")){
			while(evalXPath(ap)!= -1){
				par_count++;
			}
		}
		wprintf(L"count ==> %d \n",count);
		wprintf(L"par_count ==> %d \n",par_count);

		toElement(vn,ROOT);
		selectElement(ap,L"par");
		while(iterateAP(ap)){
			if (getCurrentDepth(vn) == 4){
				v++;
			}
		}
		wprintf(L"verify ==> %d \n",v);
		fclose(f);
		// remember C has no automatic garbage collector
		// needs to deallocate manually.
		freeVTDNav(vn);
		freeVTDGen(vg);
		freeAutoPilot(ap);

	}
	Catch (e) {
		if (e.et == parse_exception)
			printf("parse exception e ==> %s \n %s\n", e.msg, e.sub_msg);	
		// manual garbage collection here
		freeVTDGen(vg);
	}
  return 0;
}	

