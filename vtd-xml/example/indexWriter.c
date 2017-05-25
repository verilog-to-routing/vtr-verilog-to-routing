#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../vtdGen.h"
// since 2.10 declare exception context as thread local storage
_thread struct exception_context the_exception_context[1];

int main(){

	exception e;
	FILE *fw = NULL;
	char* filename = "oldpo.xml";
	VTDGen *vg = NULL; // This is the VTDGen that parses XML

	// allocate a piece of buffer then reads in the document content
	// assume "c:\soap2.xml" is the name of the file
	fw = fopen("oldpo.vxl","wb");
	//stat(filename,&s);

	//i = (int) s.st_size;	
	//wprintf(L"size of the file is %d \n",i);
	//xml = (UByte *)malloc(sizeof(UByte) *i);
	//i = fread(xml,sizeof(UByte),i,f);
	Try{
		vg = createVTDGen();
		if (parseFile(vg,TRUE,"oldpo.xml")){
			writeIndex(vg,fw);
		}
		freeVTDGen(vg);
		fclose(fw);
	}
	Catch (e) {
		if (e.et == parse_exception)
			wprintf(L"parse exception e ==> %s \n %s\n", e.msg, e.sub_msg);	
		// manual garbage collection here
		freeVTDGen(vg);
	}
  return 0;
}
