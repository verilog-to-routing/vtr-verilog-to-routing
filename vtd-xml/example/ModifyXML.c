#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../xpath1.h"
#include "../helper.h"
#include "../vtdGen.h"
#include "../XMLModifier.h"

// since 2.10 declare exception context as thread local storage
_thread struct exception_context the_exception_context[1];

int main(){
	exception e;
	VTDGen *vg = NULL;
	VTDNav *vn = NULL;
	AutoPilot *ap = NULL;
	XMLModifier *xm = NULL;
	FastLongBuffer *flb = NULL;
	int i,j,k;	
	
	Try{
		vg = createVTDGen();
		if (parseFile(vg,TRUE,"d:/ximpleware_2.2_c/vtd-xml/old.xml")){
			vn = getNav(vg);
			xm = createXMLModifier();
			bind4XMLModifier(xm,vn);			

		        // first update the value of attr
         		i = getAttrVal(vn,L"attr");
         		if (i!=-1){
              			updateToken(xm,i,L"value");
         		}

         		// navigate to <a>
        		if (toElement2(vn, FIRST_CHILD,L"a")) {
                  		// update the text content of <a>
                   		i=getText(vn);
                   		if (i!=-1){
                      			updateToken(xm,i,L" new content ");
                   		}
                   		// insert an element before <a> (which is the cursor element)
                   		insertBeforeElement(xm,L"<b/>\n\t"); 
                   		// insert an element after <a> (which is the cursor element)
                   		insertAfterElement(xm,L"\n\t<c/>"); 
				}
				flb = xm->flb;
				for (j=0;j<flb->size;j++){
					Long l = longAt(flb,j);
					Long l2 = l &(~0x1fffffffffffffffLL);
					if (l2 == MASK_INSERT_BYTE){
						printf(" insert byte %x\n",l);
					}else if ( l2 == MASK_INSERT_SEGMENT_BYTE){
						printf(" insert byte segment %x\n",l);
					}else if (l2==MASK_DELETE){
						printf(" delete %x\n",l);
					} else {
						printf(" insert fragment %x\n",l);
					}
				}
				output2(xm,"new.xml");
				free(vn->XMLDoc);
		}
		

	}Catch(e){
	}
	freeVTDGen(vg);
	freeVTDNav(vn);
	freeXMLModifier(xm);
	return 0;

}
