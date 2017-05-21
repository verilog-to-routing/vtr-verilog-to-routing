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
#include "../XMLModifier.h"

// since 2.10 declare exception context as thread local storage
_thread struct exception_context the_exception_context[1];

int main(){

	exception e;
	FILE *f = NULL ,*fw = NULL;
	int i = 0,t,result,count=0;
 	wchar_t *tmpString;	
	
	char* filename = "oldpo.xml";
	struct stat s;
	UByte *xml = NULL; // this is the buffer containing the XML content, UByte means unsigned byte
	VTDGen *vg = NULL; // This is the VTDGen that parses XML
	VTDNav *vn = NULL; // This is the VTDNav that navigates the VTD records
	AutoPilot *ap = NULL;
	XMLModifier *xm = NULL;

	// allocate a piece of buffer then reads in the document content
	// assume "c:\soap2.xml" is the name of the file
	f = fopen(filename,"rb");
	fw = fopen("newpo.xml","wb");
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
		xm = createXMLModifier();
		if (selectXPath(ap,L"/purchaseOrder/items/item[@partNum='872-AA']")){
			bind(ap,vn);
			bind4XMLModifier(xm,vn);
			while((result=evalXPath(ap))!= -1){
				remove4XMLModifier(xm);
				insertBeforeElement(xm,L"<something/>");	
			}
		}
		if (selectXPath(ap,L"/purchaseOrder/items/item/USPrice[.<40]/text()")){
			while((result=evalXPath(ap))!= -1){
				updateToken(xm,result,L"200");
			}
		}
		output(xm,fw);
		
		fclose(f);
		fclose(fw);
		// remember C has no automatic garbage collector
		// needs to deallocate manually.
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
	

