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
	FILE *fo = NULL;
	int i = 0;
	
	Long l = 0;
	int len = 0;
	int offset = 0;
	
	char* filename = "./soap2.xml";
	struct stat s;
	UByte *xml = NULL; // this is the buffer containing the XML content, UByte means unsigned byte
	VTDGen *vg = NULL; // This is the VTDGen that parses XML
	VTDNav *vn = NULL; // This is the VTDNav that navigates the VTD records
	AutoPilot *ap = NULL;
	UByte *sm = "\n================\n";

	// allocate a piece of buffer then reads in the document content
	// assume "c:\soap2.xml" is the name of the file
	f = fopen(filename,"r");
	fo = fopen("./out.txt","w");

	stat(filename,&s);

	i = (int) s.st_size;	
	printf("size of thefile is %d \n",i);
	xml = (UByte *)malloc(sizeof(UByte) *i);
	fread(xml,sizeof(UByte),i,f);
	Try{
		vg = createVTDGen();
		setDoc(vg,xml,i);
		parse(vg,TRUE);
		vn = getNav(vg);
		ap = createAutoPilot2();
		declareXPathNameSpace(ap,L"ns1",L"http://www.w3.org/2003/05/soap-envelope");
		if (selectXPath(ap,L"/ns1:Envelope/ns1:Header/*[@ns1:mustUnderstand]")){
			bind(ap,vn);
			while(evalXPath(ap)!= -1){
				l = getElementFragment(vn);
				offset = (int) l;
				len = (int) (l>>32);
				fwrite((char *)(xml+offset),sizeof(UByte),len,fo);
				fwrite((char *) sm,sizeof(UByte),strlen(sm),fo);
			}
		}
		fclose(f);
		fclose(fo);
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

