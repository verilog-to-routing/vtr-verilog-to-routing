#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include "../xpath1.h"
#include "../helper.h"
#include "../vtdGen.h"
#include "../vtdNav.h"
#include "../autoPilot.h"
#include "../XMLModifier.h"
#include "../nodeRecorder.h"
#include "../bookMark.h"

// since 2.10 declare exception context as thread local storage
_thread struct exception_context the_exception_context[1];

int main(int argc, wchar_t* argv[])
{
  exception e;
  VTDGen *vg = NULL, *vg2=NULL;
  VTDNav *vn = NULL;
  AutoPilot *ap = NULL;
  int result;
  Long l;
  Try{	
	  vg = createVTDGen();
	  // parse file	
	  if (parseFile(vg,TRUE,"oldpo.xml")==FALSE){
			// parsing failed
			free(vg->XMLDoc);
			freeVTDGen(vg);
			return;	
	  }

	  // write index
	  writeSeparateIndex(vg,"oldpo.vtd");
	  vg2 = createVTDGen();
	  loadSeparateIndex(vg2,"oldpo.xml","oldpo.vtd");
	  vn = getNav(vg2);
	  ap = createAutoPilot(vn);

	  //bind(ap, vn); 
	  selectXPath(ap,L"//*");
		
	  while( (result= evalXPath(ap))!=-1){
		    UCSChar* s = toString(vn,result);
			wprintf(L" result == %d  %s\n",result,s);
			//insertAfterHead2(xm,"abcd",4);
			//updateToken(xm, result, L"abc");	
	  }
	  free(vn->XMLDoc);
	  freeAutoPilot(ap);
	  freeVTDNav(vn);	  
	  freeVTDGen(vg);
	  freeVTDGen(vg2);
	  
	}
	Catch (e) {
		if (e.et == parse_exception)
			wprintf(L"parse exception e ==> %s \n %s\n", e.msg, e.sub_msg);	
		// manual garbage collection here
		freeVTDGen(vg);
	}	
	return 0;
}
