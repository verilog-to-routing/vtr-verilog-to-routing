#include "everything.h"

// since 2.10 declare exception context as thread local storage
_thread struct exception_context the_exception_context[1];
#if 1

int main(){

	exception e;
	VTDGen *vg = NULL; // This is the VTDGen that parses XML
	VTDNav *vn = NULL; // This is the VTDNav that navigates the VTD records
	AutoPilot *ap = NULL;
	XMLModifier *xm = NULL;
	int result;
	Try{
		vg = createVTDGen();
		if (parseFile(vg,FALSE,"D:/vtd-xml-tutorials/CSharp_tutorial_by_code_examples/10/test.xml")){
			//parse(vg,TRUE);
			vn = getNav(vg);
			ap = createAutoPilot2();
			xm = createXMLModifier();
			if (selectXPath(ap,L"//*")){
				bind(ap,vn);
				bind4XMLModifier(xm,vn);
				while((result=evalXPath(ap))!= -1){
					updateElementName(xm,L"lalala");
				}
				output2(xm,"D:/vtd-xml-tutorials/c_tutorial_by_code_examples/10/out.xml");
			}			
		}
		freeVTDNav(vn);
		freeVTDGen(vg);
		freeXMLModifier(xm);
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
#endif

