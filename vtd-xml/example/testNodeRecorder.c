#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../vtdGen.h"
#include "../autoPilot.h"
#include "../nodeRecorder.h"
// since 2.10 declare exception context as thread local storage
_thread struct exception_context the_exception_context[1];
// this example shows you how to use nodeRecorder
// to save the locations of nodes
// You must be careful not to overuse nodeRecorder
// because it could be memory inefficient to save
// a large # of nodes
int main(){

	exception e;
	FILE *fw = NULL;
	char* filename = "n.xml";
	VTDGen *vg = NULL; // This is the VTDGen that parses XML
	AutoPilot *ap = NULL;
	VTDNav *vn = NULL;
	int i;
	NodeRecorder *nr= NULL;
	
	Try{
		vg = createVTDGen();
		if (parseFile(vg,TRUE,"newpo.xml")){
			ap = createAutoPilot2();
			nr = createNodeRecorder2();
			vn = getNav(vg);
			bind(ap,vn);
			bind4NodeRecorder(nr,vn);
			if (selectXPath(ap,L"(/*/*/*)[position()=1 or position()=10]")){
				while((i=evalXPath(ap))!=-1){
					record(nr);
				}
				resetXPath(ap);
				// reset the nr before iteration
				resetPointer(nr);
				while((i=iterateNodeRecorder(nr))!=-1){
					wprintf(L"string value ==> %s \n",toString(vn,i));
				}
				clearNodeRecorder(nr);

				while((i=evalXPath(ap))!=-1){
					record(nr);
				}
				resetXPath(ap);

				resetPointer(nr);
				while((i=iterateNodeRecorder(nr))!=-1){
					wprintf(L"string value ==> %s \n",toString(vn,i));
				}
				clearNodeRecorder(nr);
			}
		}
		freeVTDGen(vg);
		freeAutoPilot(ap);
		free(vn->XMLDoc);
		freeVTDNav(vn);
		freeNodeRecorder(nr);
		
	}
	Catch (e) {
		if (e.et == parse_exception)
			wprintf(L"parse exception e ==> %s \n %s\n", e.msg, e.sub_msg);	
		// manual garbage collection here
		freeVTDGen(vg);
	}
  return 0;
}

