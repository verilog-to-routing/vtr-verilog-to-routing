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
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../xpath.h"
#include "../helper.h"
#include "../vtdGen.h"
#include "../vtdNav.h"
#include "../autoPilot.h"
#include "../XMLModifier.h"
#include "../nodeRecorder.h"
#include "../bookMark.h"

// since 2.10 declare exception context as thread local storage
_thread struct exception_context the_exception_context[1];

void main(){
	exception e;
	Try{
		VTDGen *vg = NULL; /* This is the VTDGen that parses XML */
		VTDNav *vn = NULL; /* This is the VTDNav that navigates the VTD records */
		AutoPilot *ap = NULL, *ap2=NULL;
		XMLModifier *xm = NULL;
		ElementFragmentNs *ef = NULL;
		int i= -1;
		Long l= -1;

		vg = createVTDGen();
		ap = createAutoPilot2();
		ap2 = createAutoPilot2();
		xm = createXMLModifier();
		//selectXPath(ap,L"(/*/*/*)[position()>1 and position()<4]");
		selectXPath(ap2,L"//@*");
		if (parseFile(vg,TRUE,"soap2.xml")){
			FILE *f1 = fopen("new3.xml","wb");
			vn = getNav(vg);
			//bind(ap,vn);
			bind(ap2,vn);
			//bind4XMLModifier(xm,vn);
			//i=evalXPath(ap2);
			//printf(" i's value is %d \n",i);
			//l=getElementFragment(vn);
			//ef = getElementFragmentNs(vn);
			//writeFragmentToFile(ef,f1);
			//fclose(f1);
			
			while( (i=evalXPath(ap2))!=-1){
				//insertAfterElement4(xm,ef);
				//insertAfterElement3(xm,vn->XMLDoc,(int)l,(int)(l>>32));
				printf(" i's value is %d \n",i);
				overWrite(vn,i+1,"",0,0);
			}
			fwrite(vn->XMLDoc+vn->docOffset,sizeof(UByte),vn->docLen,f1);
			//output2(xm,"d:/new3.xml");
			fclose(f1);
			free(vn->XMLDoc);
			freeVTDNav(vn);
		}
		freeElementFragmentNs(ef);
		freeXMLModifier(xm);
		freeAutoPilot(ap);
		freeAutoPilot(ap2);
		freeVTDGen(vg);
		
	}Catch(e){
		printf("exception !!!!!!!!!!! \n");
	}
}
