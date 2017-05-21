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
#include "../elementFragmentNs.h"

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
		selectXPath(ap,L"(/*/*/*)[position()>1 and position()<4]");
		selectXPath(ap2,L"/*/*/*");
		if (parseFile(vg,TRUE,"soap2.xml")){
			//FILE *f1 = fopen("d:/new3.xml","wb");
			vn = getNav(vg);
			bind(ap,vn);
			bind(ap2,vn);
			bind4XMLModifier(xm,vn);
			evalXPath(ap2);
			ef = getElementFragmentNs(vn);
			
			while( (i=evalXPath(ap))!=-1){
				insertAfterElement4(xm,ef);
				printf(" index %d \n",i);
			}
			//fwrite(vn->XMLDoc+vn->docOffset,sizeof(UByte),vn->docLen,f1);
			output2(xm,"new3.xml");
			//fclose(f1);
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

