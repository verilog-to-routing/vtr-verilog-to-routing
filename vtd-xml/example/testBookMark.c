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
	FILE *f = NULL;
	FILE *fo = NULL;
	int i = 0,j=0;
	
	Long l = 0;
	int len = 0;
	int offset = 0;

	VTDGen *vg = NULL; /* This is the VTDGen that parses XML */
	VTDNav *vn = NULL; /* This is the VTDNav that navigates the VTD records */
	AutoPilot *ap = NULL;
	BookMark *bm = NULL;
	vg = createVTDGen();
	ap = createAutoPilot2();
	if (parseFile(vg,TRUE,"oldpo.xml")){
		vn = getNav(vg);
		bind(ap,vn);
		bm = createBookMark();
		bind4BookMark(bm,vn);
		if (selectXPath(ap,L"/purchaseOrder/items/item[@partNum='872-AA']/USPrice[.>100]")){
			if (evalXPath(ap)!=-1){
				// remember the cursor position after navigation using xpath
				recordCursorPosition(bm,vn); //recordCursorPosition2(bm) also works.
				printf(" index val ==> %d \n", getCurrentIndex(vn));
			}
			toElement(vn, ROOT); // set the cursor to root
			printf(" index val ==> %d \n", getCurrentIndex(vn));
			setCursorPosition2(bm); // set the cursor back to remembered position
			printf(" index val ==> %d \n", getCurrentIndex(vn));

		}
	}
}

